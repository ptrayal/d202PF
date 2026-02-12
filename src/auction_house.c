#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "auction.h"
#include "constants.h"

/* external functions */
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
extern int xap_objs;

#define UNUSED(x) (void)(x)


/**
 * Generates the global auction data filename.
 *
 * Returns:
 *   1 on success
 *   0 on failure (invalid input or buffer too small)
 *
 * Notes:
 *   - Uses a single global auction file.
 *   - Defensive against NULL pointers.
 *   - Prevents silent truncation from snprintf().
 */
int auction_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
    int written;

    /* Validate input */
    if (vnum == NOWHERE || filename == NULL || maxlen == 0)
        return 0;

    /*
     * Generate global auction filename.
     * Example: LIB_AUCTION "auction.dat"
     */
    written = snprintf(filename, maxlen, "%sauction.dat", LIB_AUCTION);

    /* Check for encoding error or truncation */
    if (written < 0 || (size_t)written >= maxlen)
        return 0;

    return 1;
}


/**
 * Recursively saves auction objects to file.
 *
 * Traversal order:
 *   1. Save sibling objects (next_auction)
 *   2. Save contained objects (nested containers)
 *   3. Save current object
 *
 * During save, parent container weights are temporarily reduced
 * to avoid cumulative weight duplication in stored data.
 * These weights must be restored later via auction_restore_weight().
 *
 * Returns:
 *   1 on success
 *   0 on failure
 */
int auction_save(struct obj_data *obj, FILE *fp, int location)
{
    int result;
    int obj_weight;
    struct obj_data *parent;

    /* Validate file pointer */
    if (fp == NULL)
        return 0;

    /* Base case */
    if (obj == NULL)
        return 1;

    /* Save sibling chain first */
    if (!auction_save(obj->next_auction, fp, location))
        return 0;

    /* Save contained objects (deeper location index) */
    if (!auction_save(obj->contains, fp, MIN(0, location) - 1))
        return 0;

    /* Save this object */
    result = Obj_to_store(obj, fp, location);
    if (!result)
        return 0;

    /*
     * Temporarily reduce parent container weights
     * to prevent cumulative weight duplication.
     */
    obj_weight = GET_OBJ_WEIGHT(obj);

    for (parent = obj->in_obj; parent; parent = parent->in_obj)
        GET_OBJ_WEIGHT(parent) -= obj_weight;

    return 1;
}


/**
 * Restores container weights after auction_save().
 *
 * During auction_save(), parent container weights are temporarily
 * reduced to avoid cumulative weight duplication in stored data.
 *
 * This function restores those weights by traversing the object
 * hierarchy in depth-first order and adding each object's weight
 * back to its immediate parent.
 *
 * Must be called after auction_save() completes.
 */
void auction_restore_weight(struct obj_data *obj)
{
    int obj_weight;

    if (obj == NULL)
        return;

    /* Restore contained objects first (depth-first) */
    auction_restore_weight(obj->contains);

    /* Restore sibling objects */
    auction_restore_weight(obj->next_content);

    /* Restore weight to parent container */
    if (obj->in_obj != NULL)
    {
        obj_weight = GET_OBJ_WEIGHT(obj);
        GET_OBJ_WEIGHT(obj->in_obj) += obj_weight;
    }
}


/**
 * Saves the auction room contents to disk safely.
 *
 * Uses a temporary file and atomic rename to prevent corruption
 * during crashes or partial writes.
 */
void auction_crashsave(room_vnum vnum)
{
    room_rnum rnum;
    char filename[256];
    char tmp_filename[300];
    FILE *fp = NULL;
    int save_ok = 0;

    /* Validate room */
    rnum = real_room(vnum);
    if (rnum == NOWHERE)
        return;

    /* Get auction filename */
    if (!auction_get_filename(vnum, filename, sizeof(filename)))
    {
        log("SYSERR: auction_crashsave(): Failed to generate filename.");
        return;
    }

    /* Create temporary filename */
    snprintf(tmp_filename, sizeof(tmp_filename), "%s.tmp", filename);

    /* Open temporary file */
    fp = fopen(tmp_filename, "wb");
    if (!fp)
    {
        log("SYSERR: Error opening auction temp file '%s': %s",
            tmp_filename, strerror(errno));
        return;
    }

    /* Perform save */
    save_ok = auction_save(world[rnum].contents, fp, 0);

    /*
     * Restore weights regardless of save result
     * to prevent corruption of in-memory objects.
     */
    auction_restore_weight(world[rnum].contents);

    /* Flush file buffer */
    if (fflush(fp) != 0)
        log("SYSERR: fflush() failed for auction file '%s'", tmp_filename);

#ifdef HAVE_FSYNC
    fsync(fileno(fp));
#endif

    if (fclose(fp) != 0)
        log("SYSERR: fclose() failed for auction file '%s'", tmp_filename);

    /* If save failed, remove temp file */
    if (!save_ok)
    {
        remove(tmp_filename);
        log("SYSERR: auction_crashsave(): Save failed, temp file removed.");
        return;
    }

    /* Atomically replace old file */
    if (rename(tmp_filename, filename) != 0)
    {
        log("SYSERR: Failed to rename auction temp file '%s' to '%s': %s",
            tmp_filename, filename, strerror(errno));
        remove(tmp_filename);
        return;
    }
}


#define AUCTION_OBJ_VALUE_COUNT 21

/**
 * Loads auction objects from disk into the specified room.
 *
 * Preserves original file format and container semantics.
 * Performs strict input validation to prevent corruption crashes.
 *
 * Returns:
 *   1 on success
 *   0 on failure
 */
int auction_load(room_vnum rvnum)
{
    FILE *fl;
    char filename[256];
    char line[256];
    struct obj_data *obj = NULL;
    struct obj_data *container_rows[MAX_BAG_ROWS];
    struct extra_descr_data *new_descr;
    room_rnum rrnum;
    int values[AUCTION_OBJ_VALUE_COUNT];
    int locate = 0;
    int object_vnum = 0;
    int i;

    rrnum = real_room(rvnum);
    if (rrnum == NOWHERE)
        return 0;

    if (!auction_get_filename(rvnum, filename, sizeof(filename)))
        return 0;

    fl = fopen(filename, "r+b");
    if (!fl)
    {
        if (errno != ENOENT)
            log("SYSERR: Error reading auction file '%s': %s",
                filename, strerror(errno));
        return 0;
    }

    /* Initialize container tracking */
    for (i = 0; i < MAX_BAG_ROWS; i++)
        container_rows[i] = NULL;

    /* Main parsing loop */
    while (get_line(fl, line))
    {
        if (*line != '#')
            continue;

        /* Parse object vnum */
        if (sscanf(line, "#%d", &object_vnum) != 1)
            continue;

        if (object_vnum == NOTHING)
        {
            obj = create_obj();
            obj->item_number = NOTHING;
        }
        else if (object_vnum < 0 || object_vnum >= 999999)
        {
            continue;
        }
        else
        {
            obj = read_object(object_vnum, VIRTUAL);
            if (!obj)
                continue;
        }

        /* Read object value line */
        if (!get_line(fl, line))
            break;

        if (sscanf(line,
                   "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                   &values[0], &values[1], &values[2], &values[3],
                   &values[4], &values[5], &values[6], &values[7],
                   &values[8], &values[9], &values[10], &values[11],
                   &values[12], &values[13], &values[14], &values[15],
                   &values[16], &values[17], &values[18], &values[19],
                   &values[20]) != AUCTION_OBJ_VALUE_COUNT)
        {
            log("SYSERR: auction_load(): Invalid object value format.");
            continue;
        }

        locate = values[0];

        for (i = 0; i < 16; i++)
            GET_OBJ_VAL(obj, i) = values[i + 1];

        /* Handle extended object (XAP) */
        if (!get_line(fl, line))
            break;

        if (!strcmp(line, "XAP"))
        {
            if ((obj->name = fread_string(fl, line)) == NULL)
                obj->name = strdup("undefined");

            if ((obj->short_description = fread_string(fl, line)) == NULL)
                obj->short_description = strdup("undefined");

            if ((obj->description = fread_string(fl, line)) == NULL)
                obj->description = strdup("undefined");

            obj->action_description = fread_string(fl, line);

            if (!get_line(fl, line))
                break;

            if (sscanf(line, "%d %d %d %d %d %d %d %d",
                       &values[0], &values[1], &values[2], &values[3],
                       &values[4], &values[5], &values[6], &values[7]) != 8)
            {
                log("SYSERR: auction_load(): Invalid XAP numeric line.");
                continue;
            }

            obj->type_flag = values[0];
            obj->wear_flags[0] = values[1];
            obj->wear_flags[1] = values[2];
            obj->wear_flags[2] = values[3];
            obj->wear_flags[3] = values[4];
            obj->weight = values[5];
            obj->cost = values[6];
            GET_OBJ_LEVEL(obj) = values[7];

            free_extra_descriptions(obj->ex_description);
            obj->ex_description = NULL;

            /* Parse extra sections */
            while (get_line(fl, line))
            {
                if (*line == '$' || *line == '#')
                    break;

                if (*line == 'E')
                {
                    CREATE(new_descr, struct extra_descr_data, 1);
                    new_descr->keyword = fread_string(fl, line);
                    new_descr->description = fread_string(fl, line);
                    new_descr->next = obj->ex_description;
                    obj->ex_description = new_descr;
                }
                else if (*line == 'A')
                {
                    if (!get_line(fl, line))
                        break;

                    if (sscanf(line, "%d %d %d",
                               &values[0], &values[1], &values[2]) == 3)
                    {
                        for (i = 0; i < MAX_OBJ_AFFECT; i++)
                        {
                            if (obj->affected[i].location == APPLY_NONE)
                            {
                                obj->affected[i].location = values[0];
                                obj->affected[i].modifier = values[1];
                                obj->affected[i].specific = values[2];
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* Place object in room */
        obj_to_room(obj, rrnum);

        /* Rebuild container hierarchy (safe bounds check) */
        if (locate < 0)
        {
            int depth_index = -locate - 1;

            if (depth_index >= 0 && depth_index < MAX_BAG_ROWS)
            {
                obj_from_room(obj);

                if (container_rows[depth_index])
                {
                    struct obj_data *tail = container_rows[depth_index];
                    while (tail->next_content)
                        tail = tail->next_content;

                    tail->next_content = obj;
                }
                else
                {
                    container_rows[depth_index] = obj;
                }
            }
        }
    }

    fclose(fl);
    return 1;
}


SPECIAL(auction_house) 
{

    if (!CMD_IS("buy") && !CMD_IS("sell") && !CMD_IS("list") && !CMD_IS("try"))
        return 0;

    char arg1[200] = {'\0'}, arg2[200] = {'\0'}, arg3[200] = {'\0'}, arg4[200] = {'\0'}, arg5[200] = {'\0'};
    struct obj_data *obj;

    one_argument(one_argument(one_argument(one_argument(one_argument(argument, arg1), arg2), arg3), arg4), arg5);

    if (CMD_IS("sell"))
    {

        if (!*arg1)
        {
            send_to_char(ch, "What item would you like to sell?\r\n");
            return 1;
        }

        if (!*arg2)
        {
            send_to_char(ch, "How much would you like to sell it for?\r\n");
            return 1;
        }

        if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
        {
            send_to_char(ch, "You do not have any item by that description.\r\n");
            return 1;
        }

    }
    else if CMD_IS("buy")
    {

    }
    else     // list
    {

    }

    return 1;
}
