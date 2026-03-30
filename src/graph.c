/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "feats.h"

/* external functions */
ACMD(do_say);
int has_favored_enemy(struct char_data *ch, struct char_data *victim);

/* external functions from weather_tracking.c */
extern int calculate_tracking_dc(struct char_data *ch, struct char_data *vict);
extern const char *get_tracking_condition_desc(int dc);

/* external variables */
extern const char *dirs[];

/* local functions */
int VALID_EDGE(room_rnum x, int y);
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(room_rnum src, room_rnum target);
ACMD(do_track);
void hunt_victim(struct char_data *ch);

struct bfs_queue_struct 
{
    room_rnum room;
    char dir;
    struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;

/* Utility macros */
#define MARK(room)	(SET_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

int VALID_EDGE(room_rnum x, int y)
{
    if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
        return 0;
    if (CONFIG_TRACK_T_DOORS == FALSE && IS_CLOSED(x, y))
        return 0;
    if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
        return 0;

    return 1;
}

void bfs_enqueue(room_rnum room, int dir)
{
    struct bfs_queue_struct *curr;

    CREATE(curr, struct bfs_queue_struct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = 0;

    if (queue_tail)
    {
        queue_tail->next = curr;
        queue_tail = curr;
    }
    else
        queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
    struct bfs_queue_struct *curr;

    curr = queue_head;

    if (!(queue_head = queue_head->next))
        queue_tail = 0;
    free(curr);
}


void bfs_clear_queue(void)
{
    while (queue_head)
        bfs_dequeue();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(room_rnum src, room_rnum target)
{
    int curr_dir;
    room_rnum curr_room;

    if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world)
    {
        log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
        return (BFS_ERROR);
    }
    if (src == target)
        return (BFS_ALREADY_THERE);

    /* clear marks first, some OLC systems will save the mark. */
    for (curr_room = 0; curr_room <= top_of_world; curr_room++)
        UNMARK(curr_room);

    MARK(src);

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
        if (VALID_EDGE(src, curr_dir))
        {
            MARK(TOROOM(src, curr_dir));
            bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
        }

    /* now, do the classic BFS. */
    while (queue_head)
    {
        if (queue_head->room == target)
        {
            curr_dir = queue_head->dir;
            bfs_clear_queue();
            return (curr_dir);
        }
        else
        {
            for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
                if (VALID_EDGE(queue_head->room, curr_dir))
                {
                    MARK(TOROOM(queue_head->room, curr_dir));
                    bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
                }
            bfs_dequeue();
        }
    }

    return (BFS_NO_PATH);
}


/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/

int num_rooms_between(room_rnum src, room_rnum target)
{

    int dir;
    room_rnum room = src;
    int num = 0;

    while (room != target)
    {
        dir = find_first_step(room, target);
        if (dir == BFS_ERROR || dir == BFS_NO_PATH)
            return 0;
        if (dir == BFS_ALREADY_THERE)
            return num;
        room = world[room].dir_option[dir]->to_room;
        num++;
    }

    return num;
}


ACMD(do_track)
{
    char arg[MAX_INPUT_LENGTH] = {'\0'};
    struct char_data *vict;
    int dir = 0;
    int bonus = 0;
    int roll = 0;
    int dc = 15; // Will be calculated
    int skill_total = 0;

    /* The character must have the track skill. */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SURVIVAL))
    {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }
    one_argument(argument, arg);
    if (!*arg)
    {
        send_to_char(ch, "Whom are you trying to track?\r\n");
        return;
    }
    /* The person can't see the victim. */
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    {
        send_to_char(ch, "No one is around by that name.\r\n");
        return;
    }
    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK) || (OUTSIDE(vict) && !IS_NPC(vict) && HAS_FEAT(vict, FEAT_TRACKLESS_STEP)))
    {
        send_to_char(ch, "You sense no trail.\r\n");
        return;
    }

    /* Calculate tracking DC based on weather, terrain, and sector */
    dc = calculate_tracking_dc(ch, vict);

    /* Check if tracking is impossible */
    if (dc >= 999)
    {
        send_to_char(ch, "You cannot track through this terrain.\r\n");
        return;
    }

    /* Favored enemy bonus */
    if (has_favored_enemy(ch, vict))
        bonus += dice(1, (GET_CLASS_RANKS(ch, CLASS_RANGER) / 5) + 1) * 2;

    /* Roll: d20 + Survival skill + bonuses */
    roll = dice(1, 20);
    skill_total = roll + get_skill_value(ch, SKILL_SURVIVAL) + bonus;

    /* Natural 1 always fails */
    if (roll == 1)
    {
        int tries = 10;
        /* Find a random direction. :) */
        do
        {
            dir = rand_number(0, NUM_OF_DIRS - 1);
        }
        while (!CAN_GO(ch, dir) && --tries);
        send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]);
        send_to_char(ch, "@r[You failed the tracking check - wrong direction!]@n\r\n");
        return;
    }

    /* Check if they beat the DC */
    if (skill_total < dc)
    {
        send_to_char(ch, "You search for tracks but cannot find a clear trail.\r\n");
        send_to_char(ch, "%s\r\n", get_tracking_condition_desc(dc));
        send_to_char(ch, "@y[DC: %d, Roll: %d+%d+%d=%d - Failed]@n\r\n",
                     dc, roll, get_skill_value(ch, SKILL_SURVIVAL), bonus, skill_total);
        return;
    }

    /* They passed the skill check. */
    dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict));

    switch (dir)
    {
    case BFS_ERROR:
        send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
        break;
    case BFS_ALREADY_THERE:
        send_to_char(ch, "You're already in the same room!!\r\n");
        break;
    case BFS_NO_PATH:
        send_to_char(ch, "You can't sense a trail to %s from here.\r\n", HMHR(vict));
        break;
    default:  /* Success! */
        /* Distance check - higher skill allows tracking farther */
        if (num_rooms_between(IN_ROOM(ch), IN_ROOM(vict)) > (skill_total * 50))
        {
            send_to_char(ch, "The trail is too faint to follow.\r\n");
            return;
        }

        send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]);
        send_to_char(ch, "@g[DC: %d, Roll: %d+%d+%d=%d - Success]@n\r\n",
                     dc, roll, get_skill_value(ch, SKILL_SURVIVAL), bonus, skill_total);
        break;
    }
}


void hunt_victim(struct char_data *ch)
{
    int dir = 0;
    byte found;
    struct char_data *tmp;

    if (!ch || !HUNTING(ch) || FIGHTING(ch))
        return;

    /* make sure the char still exists */
    for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
        if (HUNTING(ch) == tmp)
            found = TRUE;

    if (!found)
    {
        char actbuf[MAX_INPUT_LENGTH] = "Damn!  My prey is gone!!";

        do_say(ch, actbuf, 0, 0);
        HUNTING(ch) = NULL;
        return;
    }
    if ((dir = find_first_step(IN_ROOM(ch), IN_ROOM(HUNTING(ch)))) < 0)
    {
        char buf[MAX_INPUT_LENGTH] = {'\0'};

        snprintf(buf, sizeof(buf), "Damn!  I lost %s!", HMHR(HUNTING(ch)));
        do_say(ch, buf, 0, 0);
        HUNTING(ch) = NULL;
    }
    else
    {
        perform_move(ch, dir, 1);
        if (IN_ROOM(ch) == IN_ROOM(HUNTING(ch)))
            hit(ch, HUNTING(ch), TYPE_UNDEFINED);
    }
}
