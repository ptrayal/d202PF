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

int auction_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return (0);

  snprintf(filename, maxlen, LIB_AUCTION"auction");
  return (1);
}


int auction_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result = 0;

  if (obj) {
    auction_save(obj->next_auction, fp, location);
    auction_save(obj->contains, fp, MIN(0, location) - 1);
    result = Obj_to_store(obj, fp, location);
    if (!result)
      return (0);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
  }
  return (1);
}


void auction_restore_weight(struct obj_data *obj)
{
  if (obj) {
    auction_restore_weight(obj->contains);
    auction_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}


void auction_crashsave(room_vnum vnum)
{

  int rnum = 0;
  char buf[MAX_STRING_LENGTH]={'\0'};
  FILE *fp;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;
  if (!auction_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    log("SYSERR: Error saving auction file: %s", strerror(errno));
    return;
  }
  if (!auction_save(world[rnum].contents, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);
  auction_restore_weight(world[rnum].contents);
}

int auction_load(room_vnum rvnum) 
{
    FILE *fl;
    char fname[MAX_STRING_LENGTH] = { '\0' };
    char buf1[MAX_STRING_LENGTH] = { '\0' };
    char buf2[MAX_STRING_LENGTH] = { '\0' };
    char line[256] = { '\0' };
    struct obj_data *temp;
    int t[21], zwei = 0;
    int locate = 0;
    int j = 0;
    int nr = 0;
    int k = 0;
    int num_objs = 0;
    struct obj_data *obj1;
    struct obj_data *cont_row[MAX_BAG_ROWS];
    struct extra_descr_data *new_descr;
    room_rnum rrnum;

    if ((rrnum = real_room(rvnum)) == NOWHERE)
        return 0;

    if (!auction_get_filename(rvnum, fname, sizeof(fname)))
        return 0;

    if (!(fl = fopen(fname, "r+b")))
    {
        if (errno != ENOENT)    /* if it fails, NOT because of no file */
        {
            sprintf(buf1, "SYSERR: READING AUCTION FILE %s (5)", fname);
            log("%s: %s", buf1, strerror(errno));
        }
        return 0;
    }

    for (j = 0; j < MAX_BAG_ROWS; j++)
        cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

    if(!feof(fl))
        get_line(fl, line);
    while (!feof(fl))
    {
        temp = NULL;
        /* first, we get the number. Not too hard. */
        if(*line == '#')
        {
            if (sscanf(line, "#%d", &nr) != 1)
            {
                continue;
            }
            /* we have the number, check it, load obj. */
            if (nr == NOTHING)     /* then it is unique */
            {
                temp = create_obj();
                temp->item_number = NOTHING;
            }
            else if (nr < 0)
            {
                continue;
            }
            else
            {
                if(nr >= 999999)
                    continue;
                temp = read_object(nr, VIRTUAL);
                if (!temp)
                {
                    get_line(fl, line);
                    continue;
                }
            }

            get_line(fl, line);
            sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9,
                   t + 10, t + 11, t + 12, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18, t + 19, t + 20);
            locate = t[0];
            GET_OBJ_VAL(temp, 0) = t[1];
            GET_OBJ_VAL(temp, 1) = t[2];
            GET_OBJ_VAL(temp, 2) = t[3];
            GET_OBJ_VAL(temp, 3) = t[4];
            GET_OBJ_VAL(temp, 4) = t[5];
            GET_OBJ_VAL(temp, 5) = t[6];
            GET_OBJ_VAL(temp, 6) = t[7];
            GET_OBJ_VAL(temp, 7) = t[8];
            GET_OBJ_EXTRA(temp)[0] = t[9];
            GET_OBJ_EXTRA(temp)[1] = t[10];
            GET_OBJ_EXTRA(temp)[2] = t[11];
            GET_OBJ_EXTRA(temp)[3] = t[12];
            GET_OBJ_VAL(temp, 8) = t[13];
            GET_OBJ_VAL(temp, 9) = t[14];
            GET_OBJ_VAL(temp, 10) = t[15];
            GET_OBJ_VAL(temp, 11) = t[16];
            GET_OBJ_VAL(temp, 12) = t[17];
            GET_OBJ_VAL(temp, 13) = t[18];
            GET_OBJ_VAL(temp, 14) = t[19];
            GET_OBJ_VAL(temp, 15) = t[20];

            get_line(fl, line);
            /* read line check for xap. */
            if(!strcmp("XAP", line))
            {
                /* then this is a Xap Obj, requires
                                                 special care */
                if ((temp->name = fread_string(fl, buf2)) == NULL)
                {
                    temp->name = "undefined";
                }

                if ((temp->short_description = fread_string(fl, buf2)) == NULL)
                {
                    temp->short_description = "undefined";
                }

                if ((temp->description = fread_string(fl, buf2)) == NULL)
                {
                    temp->description = "undefined";
                }

                if ((temp->action_description = fread_string(fl, buf2)) == NULL)
                {
                    temp->action_description = 0;
                }
                if (!get_line(fl, line) ||
                        (sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7) != 8))
                {
                    fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
                    return 0;
                }
                temp->type_flag = t[0];
                temp->wear_flags[0] = t[1];
                temp->wear_flags[1] = t[2];
                temp->wear_flags[2] = t[3];
                temp->wear_flags[3] = t[4];
                temp->weight = t[5];
                temp->cost = t[6];
                GET_OBJ_LEVEL(temp) = t[7];


                /* we're clearing these for good luck */

                for (j = 0; j < MAX_OBJ_AFFECT; j++)
                {
                    temp->affected[j].location = APPLY_NONE;
                    temp->affected[j].modifier = 0;
                }

                free_extra_descriptions(temp->ex_description);
                temp->ex_description = NULL;

                get_line(fl, line);
                for (k = j = zwei = 0; !zwei && !feof(fl);)
                {
                    switch (*line)
                    {
                    case 'E':
                        CREATE(new_descr, struct extra_descr_data, 1);
                        new_descr->keyword = fread_string(fl, buf2);
                        new_descr->description = fread_string(fl, buf2);
                        new_descr->next = temp->ex_description;
                        temp->ex_description = new_descr;
                        get_line(fl, line);
                        break;
                    case 'A':
                        if (j >= MAX_OBJ_AFFECT)
                        {
                            log("SYSERR: Too many object affectations in loading rent file");
                        }
                        get_line(fl, line);
                        sscanf(line, "%d %d %d", t, t + 1, t + 2);

                        temp->affected[j].location = t[0];
                        temp->affected[j].modifier = t[1];
                        temp->affected[j].specific = t[2];
                        j++;
                        //GET_OBJ_LEVEL(temp) = set_object_level(temp);
                        get_line(fl, line);
                        break;
                    case 'G':
                        get_line(fl, line);
                        sscanf(line, "%ld", (long int *)&temp->generation);
                        get_line(fl, line);
                        break;
                    case 'U':
                        get_line(fl, line);
                        sscanf(line, "%lld", &temp->unique_id);
                        get_line(fl, line);
                        break;
                    case 'S':
                        if (j >= SPELLBOOK_SIZE)
                        {
                            log("SYSERR: Too many spells in spellbook loading rent file");
                        }
                        get_line(fl, line);
                        sscanf(line, "%d %d", t, t + 1);

                        if (!temp->sbinfo)
                        {
                            CREATE(temp->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
                            memset((char *) temp->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
                        }
                        temp->sbinfo[j].spellname = t[0];
                        temp->sbinfo[j].pages = t[1];
                        j++;
                        get_line(fl, line);
                        break;
                    case 'Z':
                        get_line(fl, line);
                        sscanf(line, "%d", (int *)&GET_OBJ_SIZE(temp));
                        get_line(fl, line);
                        break;

                    case '$':
                    case '#':
                        zwei = 1;
                        break;
                    default:
                        zwei = 1;
                        break;
                    }
                }      /* exit our for loop */
            }   /* exit our xap loop */
            if(temp != NULL)
            {
                num_objs++;
                obj_to_room(temp, rrnum);
            }
            else
            {
                continue;
            }

            /*No need to check if its equipped since rooms can't equip things --firebird_223*/

            for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
                if (cont_row[j])   /* no container -> back to ch's inventory */
                {
                    for (; cont_row[j]; cont_row[j] = obj1)
                    {
                        obj1 = cont_row[j]->next_content;
                        obj_to_room(cont_row[j], rrnum);
                    }
                    cont_row[j] = NULL;
                }

            if (j == -locate && cont_row[j])   /* content list existing */
            {
                if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER)
                {
                    /* take item ; fill ; give to char again */
                    obj_from_room(temp);
                    temp->contains = NULL;
                    for (; cont_row[j]; cont_row[j] = obj1)
                    {
                        obj1 = cont_row[j]->next_content;
                        obj_to_obj(cont_row[j], temp);
                    }
                    obj_to_room(temp, rrnum); /* add to inv first ... */
                }
                else     /* object isn't container -> empty content list */
                {
                    for (; cont_row[j]; cont_row[j] = obj1)
                    {
                        obj1 = cont_row[j]->next_content;
                        obj_to_room(cont_row[j], rrnum);
                    }
                    cont_row[j] = NULL;
                }
            }

            if (locate < 0 && locate >= -MAX_BAG_ROWS)
            {
                /* let obj be part of content list
                   but put it at the list's end thus having the items
                   in the same order as before renting */
                obj_from_room(temp);
                if ((obj1 = cont_row[-locate - 1]))
                {
                    while (obj1->next_content)
                        obj1 = obj1->next_content;
                    obj1->next_content = temp;
                }
                else
                    cont_row[-locate - 1] = temp;
            }
        }
        else
        {
            get_line(fl, line);
        }
    }


    fclose(fl);

    return 1;
}

SPECIAL(auction_house) 
{

    if (!CMD_IS("buy") && !CMD_IS("sell") && !CMD_IS("list") && !CMD_IS("try"))
        return 0;

    char arg1[200]={'\0'}, arg2[200]={'\0'}, arg3[200]={'\0'}, arg4[200]={'\0'}, arg5[200]={'\0'};
    struct obj_data *obj;

    one_argument(one_argument(one_argument(one_argument(one_argument(argument, arg1), arg2), arg3), arg4), arg5);

    if (CMD_IS("sell")) 
    {

        if (!*arg1) {
            send_to_char(ch, "What item would you like to sell?\r\n");
            return 1;
        }    

        if (!*arg2) {
            send_to_char(ch, "How much would you like to sell it for?\r\n");
            return 1;
        }

        if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
            send_to_char(ch, "You do not have any item by that description.\r\n");
            return 1;
        }

    } 
    else if CMD_IS("buy") 
    {

} else { // list

}

return 1;
}
