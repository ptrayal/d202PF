/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: act.movement.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "pets.h"
#include "oasis.h"

/* external functions */

int is_player_grouped(struct char_data *target, struct char_data *group);
int mob_exp_by_level(int level);
int special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
int buildwalk(struct char_data *ch, int dir);
struct obj_data *find_vehicle_by_vnum(int vnum);
void timed_dt(struct char_data *ch);
int find_house(room_vnum vnum);
int skill_roll(struct char_data *ch, int skillnum);
void advance_mob_level(struct char_data *ch, int whichclass);
void set_auto_mob_stats(struct char_data *mob);
void set_familiar_stats(struct char_data *ch);
int get_speed(struct char_data *ch);
int compute_summon_armor_class(struct char_data *ch, struct char_data *att);
int compute_mount_armor_class(struct char_data *ch, struct char_data *att);

/* local functions */
int has_boat(struct char_data *ch);
int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
int has_key(struct char_data *ch, obj_vnum key);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd);
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);

extern struct race_data race_list[NUM_RACES];
extern struct house_control_rec house_control[MAX_HOUSES];

void current_update(void)
{
  struct char_data *i, *next_char;
  char buf[MAX_STRING_LENGTH];
  int door =0;
 
  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if ((GET_POS(i) == POS_STANDING) && ROOM_FLAGGED(IN_ROOM(i), ROOM_CURRENT) &&
      ((door = rand_number(0, 18)) < NUM_OF_DIRS) && CAN_GO(i, door) && 
      !FIGHTING(i) && !IS_NPC(i) && (SECT(IN_ROOM(i)) == SECT_WATER_NOSWIM)) {
      snprintf(buf, sizeof(buf),"The strong current carries you %s!\r\n", dirs[door]);
      send_to_char(i, "%s", buf);
      snprintf(buf, sizeof(buf), "$n is taken %s by the rough current!", dirs[door]);
      act(buf, FALSE, i, 0, 0, TO_NOTVICT);
      perform_move(i, door, 1);
    }
  }
}


/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

/*
  if (ROOM_IDENTITY(IN_ROOM(ch)) == DEAD_SEA)
    return (1);
*/

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  return (0);
}

/* simple function to determine if char can fly */
int has_flight(struct char_data *ch)
{
  struct obj_data *obj;

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (AFF_FLAGGED(ch, AFF_FLYING) || AFF_FLAGGED(ch, AFF_ANGELIC))
    return (1);

  /* non-wearable flying items in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1); 

  return (0);
}
  

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
  char buf2[MAX_STRING_LENGTH];
  room_rnum was_in = IN_ROOM(ch);
  int need_movement;
  int riding = 0, ridden_by = 0, same_room = 0;
  int speed = 30;
  int explore_bonus = FALSE;
  int athletics = 0;

  if (RIDING(ch)) {
    riding = 1;
    speed = get_speed(RIDING(ch));
  }
  else {
    speed = get_speed(ch);
  }
  if (RIDDEN_BY(ch))
    ridden_by = 1;

 
  /* if they're mounted, are they in the same room w/ their mount(ee)? */
  if (riding && RIDING(ch)->in_room == ch->in_room)
    same_room = 1;
  else if (ridden_by && RIDDEN_BY(ch)->in_room == ch->in_room)
    same_room = 1;  
  
  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, throwaway))
    return (0);

  /* blocked by a leave trigger ? */
  if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    return (0);
  }

  if (affected_by_spell(ch, SPELL_AFF_DEFENSIVE_STANCE) && !HAS_FEAT(ch, FEAT_MOBILE_DEFENSE)) {
    send_to_char(ch, "You cannot move as you are in a defensive stance.\r\n");
    return (0);
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
      (SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) {
    if (!has_boat(ch)) {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }

  if ((zone_table[world[EXIT(ch, dir)->to_room].zone].zone_status < 2) &&
      (GET_ADMLEVEL(ch) < 1)) {
    send_to_char(ch, "That zone is not yet open and cannot yet be entered.\r\n");
    return (0);
  }


  if (zone_table[world[EXIT(ch, dir)->to_room].zone].zone_status == 3) {
    int enter_dungeon = FALSE;
    int min_j = 60;
    int max_j = 0;
    int j = 0;
    for (j = 1; j < NUM_LEVEL_RANGES; j++) {
      if (IS_SET(zone_table[world[EXIT(ch, dir)->to_room].zone].level_range, (1 << j))) {
        if (j < min_j)
          min_j = j;
        if (j > max_j)
          max_j = j;
        if (GET_CLASS_LEVEL(ch) == ((j * 2) - 1) || GET_CLASS_LEVEL(ch) == (j * 2)) {
          enter_dungeon = TRUE;
          break;
        }
        if ((ch)->mentor_level == ((j * 2) - 1) || (ch)->mentor_level == (j * 2)) {
          enter_dungeon = TRUE;
          break;
        }
      }
    }
    if (min_j == 60 || max_j == 0) {  
      send_to_char(ch, "That dungeon has not had its min or max level set, and thus cannot be entered.\r\n");
      return 0;
    }
    if (!enter_dungeon) {
      send_to_char(ch, "That dungeon has a min level of %d and a max level of %d.  You must mentor down to within that level range.\r\n",
                   (min_j * 2) - 1, max_j * 2);
      return 0;
    }
  }



  if (affected_by_spell(ch, SPELL_ENTANGLE) && affected_by_spell(ch, SPELL_ENTANGLED)) {
    send_to_char(ch, "You are entangled by animated vines, trees and plants and cannot move.\r\n");
    return (0);
  }

  if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_STUNNED)) {
    send_to_char(ch, "Your muscles won't respond.\r\n");
    return (0);
  }

  if (IS_OVER_LOAD(ch) && GET_ADMLEVEL(ch) == 0) {
    send_to_char(ch, "You are carrying too much wieght to move.\r\n");
    return (0);
  }

  /* if this room or the one we're going to needs flight, check for it */
  if ((SECT(IN_ROOM(ch)) == SECT_FLYING) ||
      (SECT(EXIT(ch, dir)->to_room) == SECT_FLYING)) {
    if (!has_flight(ch)) {
      send_to_char(ch, "You need wings to go there!\r\n");
      return (0);
    }
  }

  struct char_data *mob;
  sbyte block = FALSE;

  for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room) {
    if (!IS_NPC(mob))
      continue;

    if (dir == NORTH && MOB_FLAGGED(mob, MOB_BLOCK_N))
      block = TRUE;
    else if (dir == EAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTH && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == WEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == NORTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == NORTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == UP && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == DOWN && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;

    if (block && MOB_FLAGGED(mob, MOB_BLOCK_RACE) && GET_RACE(ch) == GET_RACE(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_CLASS) && GET_CLASS(ch) == GET_CLASS(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_RACE_FAMILY) && race_list[GET_RACE(ch)].family == race_list[GET_RACE(mob)].family)
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_LEVEL) && GET_CLASS_LEVEL(ch) > GET_HITDICE(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_GOOD(ch) > IS_GOOD(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_EVIL(ch) > IS_EVIL(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_NEUTRAL(ch) > IS_NEUTRAL(mob))
      block = FALSE;

    if (block)
      break;
  }

  if (block && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
    act("$N blocks your from travelling in that direction.", FALSE, ch, 0, mob, TO_CHAR);
    act("$n tries to leave the room, but $N blocks $m from travelling in their direction.", FALSE, ch, 0, mob, TO_ROOM);
    return 0;
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(IN_ROOM(ch))] +
		   movement_loss[SECT(EXIT(ch, dir)->to_room)]) / 2;

  if (HAS_FEAT(ch, FEAT_WOODLAND_STRIDE) && OUTSIDE(ch)) {
      need_movement = MAX(0, need_movement - 1);
  }

  if (is_flying(ch)) {
    need_movement -= 1;
  }

  /* Stealth increases your move cost, less if you are good at it */
  if (AFF_FLAGGED(ch, AFF_HIDE))
    need_movement *= ((roll_skill(ch, SKILL_STEALTH) > 15) ? 2 : 4);

  if (AFF_FLAGGED(ch, AFF_SNEAK))
    need_movement *= ((roll_skill(ch, SKILL_STEALTH) > 15) ? 1.2 : 2);

  if (ch->player_specials->mounted)
    athletics = skill_roll(ch, SKILL_RIDE) + (skill_roll(ch, SKILL_ACROBATICS) / 5);
  else
    athletics = skill_roll(ch, SKILL_ACROBATICS);
  

  if (HAS_FEAT(ch, FEAT_ENDURANCE))
    athletics += 10;

  need_movement *= (400 - (athletics * 5));

  need_movement /= speed;

  need_movement = MAX(1, need_movement);

  if (GET_ADMLEVEL(ch) > 0)
    need_movement = 0;

  if (SECT(IN_ROOM(ch)) == SECT_INSIDE || SECT(IN_ROOM(ch)) == SECT_CITY)
    need_movement = 0;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch) && !riding) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");
    return (0);
  }

  if (riding) {
    if (GET_MOVE(RIDING(ch)) < need_movement) {
      OUTPUT_TO_CHAR("Your mount is too exhausted.\r\n", ch);
      return 0;
    }
  }  	
	
  /* Check if the character needs a skill check to go that way. */
    if (EXIT(ch, dir)->dcskill != 0) {
    if (EXIT(ch, dir)->dcmove > roll_skill(ch, EXIT(ch, dir)->dcskill)) {
      send_to_char(ch, "Your skill in %s isn't enough to move that way!\r\n", spell_info[EXIT(ch, dir)->dcskill].name);
      /* A failed skill check still spends the movement points! */
      if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch))
        GET_MOVE(ch) -= need_movement;
      return (0);
    }
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(EXIT(ch, dir)->to_room)) && !ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_PLAYER_SHOP)) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    } else {
      int hs;
      if ((hs = find_house(GET_ROOM_VNUM(EXIT(ch, dir)->to_room))) != NOWHERE) {
        if (((time(0) - house_control[hs].last_payment) / (60 * 60 * 24 * 30)) > 0 && GET_ADMLEVEL(ch) == 0) {
          send_to_char(ch, "The owner has not paid their rent, and thus you cannot enter.\r\n");
          return (0);
        }
      }
    }
  }
  if ((riding == 1 || ridden_by == 1) && (IS_SET_AR(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_TUNNEL))) {
    OUTPUT_TO_CHAR("There isn't enough room there, while mounted.\r\n", ch);
    return 0;
  } else {  
	  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
	      num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) >= CONFIG_TUNNEL_SIZE) {
	    if (CONFIG_TUNNEL_SIZE > 1)
	      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
	    else
	      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
	    return (0);
	  }
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
	GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

 if (riding && skill_roll(ch, SKILL_RIDE) < 5) { 
    act("$N rears backwards, throwing you to the ground! Ouch!", FALSE, ch, 0, RIDING(ch), TO_CHAR);
    act("You rear backwards, throwing $n to the ground. Ouch!", FALSE, ch, 0, RIDING(ch), TO_VICT);
    act("$N rears backwards, throwing $n to the ground. Ouch!", FALSE, ch, 0, RIDING(ch), TO_NOTVICT);
    GET_POS(ch) = POS_SITTING;
    dismount_char(ch);
    damage(ch, ch, dice(1,6), TYPE_UNDEFINED, 0, -1, TYPE_UNDEFINED, 0, 1);
    return 0;
  }  
  
  /* Now we know we're allowed to go into the room. */
  if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !(riding || ridden_by))
    GET_MOVE(ch) -= need_movement;
  else if (riding)
    GET_MOVE(RIDING(ch)) -= need_movement;
  else if (ridden_by)
    GET_MOVE(RIDDEN_BY(ch)) -= need_movement;	

if (riding) {
        sprintf(buf2, "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
 } else if (ridden_by) {
    if (!AFF_FLAGGED(ch, AFF_TAMED) && skill_roll(ch, SKILL_RIDE) < 15) {
      act("You rear backwards, throwing $N to the ground!", FALSE, ch, 0, RIDDEN_BY(ch), TO_CHAR); 
      act("$n rears backwards, throwing you to the ground. Ouch!", FALSE, ch, 0, RIDDEN_BY(ch), TO_VICT);
      act("$n rears backwards, throwing $N to the ground. Ouch!", FALSE, ch, 0, RIDDEN_BY(ch), TO_NOTVICT);
      damage(RIDDEN_BY(ch), RIDDEN_BY(ch), dice(1,6), TYPE_UNDEFINED, 0, -1, TYPE_UNDEFINED, 0, 1);
      dismount_char(RIDDEN_BY(ch));
      GET_POS(RIDDEN_BY(ch)) = POS_SITTING;
      /*sprintf(buf2, "$n leaves %s.", dirs[dir]);
          act(buf2, TRUE, ch, 0, 0, TO_ROOM); */
      return 0;
    }
    else {
      sprintf(buf2, "$n rides $N %s", dirs[dir]);
      act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
    }
  } 
  else
  {
	  snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
	  if (AFF_FLAGGED(ch, AFF_SNEAK))
	    act(buf2, TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
	  else  
	    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }
    


  was_in = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) || !enter_wtrigger(&world[IN_ROOM(ch)], ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  if (ch->player_specials->rooms_visited[world[IN_ROOM(ch)].number] == 0) {
    ch->player_specials->num_of_rooms_visited++;
    if ((ch->player_specials->num_of_rooms_visited % 25) == 0) {
      explore_bonus = TRUE;
    }
  }
  ch->player_specials->rooms_visited[world[IN_ROOM(ch)].number]++;
  if (ch->player_specials->rooms_visited[world[IN_ROOM(ch)].number] > 50)
    ch->player_specials->rooms_visited[world[IN_ROOM(ch)].number] = 50;

    snprintf(buf2, sizeof(buf2), "%s%s",
            ((dir == UP) || (dir == DOWN) ? "" : "the "),
            ( dir == UP  ? "below" :
             (dir == DOWN) ? "above" : dirs[rev_dir[dir]]));
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n arrives from $T.", TRUE, ch, 0, buf2, TO_ROOM | TO_SNEAKRESIST);
  else
    act("$n arrives from $T.", TRUE, ch, 0, buf2, TO_ROOM);

  if (ch->desc != NULL)
    look_at_room(IN_ROOM(ch), ch, 0);

  if (explore_bonus) {
    send_to_char(ch, "\r\n@YYou gain a %d exp exploration bonus.@n\r\n", mob_exp_by_level(GET_LEVEL(ch) + 10));
    gain_exp(ch, mob_exp_by_level(GET_LEVEL(ch) + 10));
  }

  ch->exp_chain = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TIMED_DT) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE))
     timed_dt(NULL);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return (0);
  }

  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    look_at_room(IN_ROOM(ch), ch, 0);
  } else greet_memory_mtrigger(ch);

  return (1);
}


int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
    return (0);
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE || (EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) && (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && GET_ADMLEVEL(ch) == 0) {
    if (EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else
      send_to_char(ch, "It seems to be closed.\r\n");
  } else {
  
    if (RIDING(ch)) {
		if (GET_POS(RIDING(ch)) == POS_RESTING || GET_POS(RIDING(ch)) == POS_SITTING) {
		    OUTPUT_TO_CHAR("But your mount is resting!\r\n", ch);
		    return 0;
		}
    }
  
    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((IN_ROOM(k->follower) == was_in) &&
	  (GET_POS(k->follower) >= POS_STANDING) &&
          (CAN_SEE(k->follower, ch) || is_player_grouped(k->follower, ch))) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return (1);
  }
  return (0);
}


ACMD(do_move)
{
  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  perform_move(ch, subcmd - 1, 0);
}


int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) < 0 &&
        (door = search_block(dir, abbr_dirs, FALSE)) < 0) {	/* Partial Match */
      send_to_char(ch, "That's not a direction.\r\n");
      return (-1);
    }
    if (EXIT(ch, door)) {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword) {
	if (isname((char *) type, EXIT(ch, door)->keyword))
	  return (door);
	else {
	  send_to_char(ch, "I see no %s there.\r\n", type);
	  return (-1);
        }
      } else
	return (door);
    } else {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (isname((char *) type, EXIT(ch, door)->keyword))
	    return (door);

    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    return (-1);
  }
}


int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;
  int i;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      if (GET_OBJ_VNUM(GET_EQ(ch, i)) == key)
        return (1);

  return (0);
}



#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

const char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH];
  size_t len;
  room_rnum other_room = NOWHERE;
  struct room_direction_data *back = NULL;

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != IN_ROOM(ch))
	back = NULL;

  switch (scmd) {
  case SCMD_OPEN:
    OPEN_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_CLOSE:
    CLOSE_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      CLOSE_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_LOCK:
    LOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_UNLOCK:
    UNLOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      UNLOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_PICK:
    TOGGLE_LOCK(IN_ROOM(ch), obj, door);
    if (back)
      TOGGLE_LOCK(other_room, obj, rev_dir[door]);
    send_to_char(ch, "The lock quickly yields to your skills.\r\n");
    len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    break;
  }

  /* Notify the room. */
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s.",
	obj ? "" : "the ", obj ? "$p" : EXIT(ch, door)->keyword ? "$F" : "door");
  if (!obj || IN_ROOM(obj) != NOWHERE)
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
      send_to_room(EXIT(ch, door)->to_room, "The %s is %s%s from the other side.\r\n",
		back->keyword ? fname(back->keyword) : "door", cmd_door[scmd],
		scmd == SCMD_CLOSE ? "d" : "ed");
}

int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd)
{
  int skill_lvl;
  struct obj_data *tools = NULL;

  if (scmd != SCMD_PICK)
    return (1);

  /* PICKING_LOCKS is not an untrained skill */
  
  if (!GET_SKILL(ch, SKILL_DISABLE_DEVICE)) {
    send_to_char(ch, "You have no idea how!\r\n");
    return (0);
  }
  skill_lvl = get_skill_value(ch, SKILL_DISABLE_DEVICE);

  if (FIGHTING(ch))
      skill_lvl += dice(1, 20);
  else
      skill_lvl += 20; // take 20
  if ((tools = get_obj_in_list_vis(ch, "thieves,tools", NULL, ch->carrying))) {
      if (GET_OBJ_VNUM(tools) == 105)
          skill_lvl += 2;
  }


  if (keynum == NOTHING)
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  else if (pickproof)
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
  /* The -2 is here because that is a penality for not having a set of
   * thieves' tools. If the player has them, that modifier will be accounted
   * for in roll_skill, and negate (or surpass) this. 
   */
  else if (dclock > (skill_lvl - 2))
    send_to_char(ch, "You failed to pick the lock. [%d dc vs. %d skill]\r\n", dclock, skill_lvl - 2);
  else {
    send_to_char(ch, "Success! [%d dc vs. %d skill]\r\n", dclock, skill_lvl - 2);
    return (1);
  }
  return (0);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_VEHICLE)   && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_HATCH)     && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_WINDOW)    && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_PORTAL)    && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_SECRET(ch, obj, door) ((obj) ? \
                        (OBJVAL_FLAGGED(obj, CONT_SECRET)) : \
                        (EXIT_FLAGGED(EXIT(ch, door), EX_SECRET)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, VAL_KEY_KEYCODE)) : \
					(EXIT(ch, door)->key))
#define DOOR_DCLOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK)) : EXIT(ch, door)->dclock)
#define DOOR_DCHIDE(ch, obj, door)      ((obj) ? (GET_OBJ_VAL(obj, VAL_DOOR_DCHIDE)) : EXIT(ch, door)->dchide)
ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  
  if ((obj) || (door >= 0)) {
    if (!DOOR_DCHIDE(ch, obj, door)) {
      if (obj)
        GET_OBJ_VAL(obj, VAL_DOOR_DCHIDE) = 21;
      else
        EXIT(ch, door)->dchide = 21;
    }
    keynum = DOOR_KEY(ch, obj, door);
    if (!DOOR_DCLOCK(ch, obj, door)) {
      if (obj)
        GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK) = 21;
      else
        EXIT(ch, door)->dclock = 21;
    }
    if (door >= 0 && (EXIT_FLAGGED(EXIT(ch, door), EX_SECRET)) &&
        (get_skill_value(ch, SKILL_PERCEPTION) + 20) < EXIT(ch, door)->dchide)  {
        send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
        return;
    }
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && !ADM_FLAGGED(ch, ADM_NOKEYS) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), DOOR_DCLOCK(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}

int do_simple_enter(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
  room_rnum was_in = IN_ROOM(ch);
  int need_movement = 0;

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
      IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    return (0);
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
      (SECT(dest_room) == SECT_WATER_NOSWIM)) {
    if (!has_boat(ch)) {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(IN_ROOM(ch))] +
		   movement_loss[SECT(dest_room)]) / 2;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(dest_room)) && !ROOM_FLAGGED(dest_room, ROOM_PLAYER_SHOP)) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[dest_room])) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(dest_room, ROOM_GODROOM) &&
	GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)))
    GET_MOVE(ch) -= need_movement;
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);
  else
    act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) ) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
      if (AFF_FLAGGED(ch, AFF_SNEAK))
        act("$n arrives from a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
      else
        act("$n arrives from a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else {
      if (AFF_FLAGGED(ch, AFF_SNEAK))
        act("$n arrives from outside.", FALSE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
      else
        act("$n arrives from outside.", FALSE, ch, 0, 0, TO_ROOM);

    }
  if (ch->desc != NULL)
    look_at_room(IN_ROOM(ch), ch, 0);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

int perform_enter_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = IN_ROOM(ch);
  int could_move = FALSE;
  struct follow_type *k;

  if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE ||
      GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
      send_to_char(ch, "But it's closed!\r\n");
    } else if ((GET_OBJ_VAL(obj, VAL_PORTAL_DEST)            != NOWHERE) &&
               (real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE)) {
      if ((could_move = do_simple_enter(ch, obj, need_specials_check)))
        for (k = ch->followers; k; k = k->next)
          if ((IN_ROOM(k->follower) == was_in) &&
              (GET_POS(k->follower) >= POS_STANDING)) {
	    act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	    perform_enter_obj(k->follower, obj, 1);
          }
    } else {
       send_to_char(ch,
           "It doesn't look like you can enter it at the moment.\r\n");
    }
  } else {
    send_to_char(ch, "You can't enter that!\r\n");
  }
  return could_move;
}

ACMD(do_enter)
{
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH];
  int door, move_dir = -1;

  one_argument(argument, buf);

  if (*buf) { /* an argument was supplied, search for door keyword */
    /* Is the object in the room? */
    obj = get_obj_in_list_vis(ch,buf, NULL, world[IN_ROOM(ch)].contents);
    /* Is the object in the character's inventory? */
    if (!obj)
      obj = get_obj_in_list_vis(ch,buf, NULL, ch->carrying);
    /* Is the character carrying the object? */
    if (!obj)
      obj = get_obj_in_equip_vis(ch, buf, NULL, ch->equipment);
    /* We have an object to enter */
    if (obj)
      perform_enter_obj(ch, obj, 0);
    /* Is there a door to enter? */
    else {
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (EXIT(ch, door))
          if (EXIT(ch, door)->keyword)
            if (isname(buf, EXIT(ch, door)->keyword))
              move_dir = door;
      /* Did we find what they wanted to enter. */
      if (move_dir > -1)
        perform_move(ch, move_dir, 1);
      else
        send_to_char(ch, "There is no %s here.\r\n", buf);
    }
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS)) {
    send_to_char(ch, "You are already indoors.\r\n");
  } else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->to_room != NOWHERE)
          if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
              ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
            move_dir = door;
    if (move_dir > -1)
      perform_move(ch, move_dir, 1);
    else
      send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}

int do_simple_leave(struct char_data *ch, struct obj_data *obj, int need_specials_check)

{
  room_rnum was_in = IN_ROOM(ch), dest_room = NOWHERE;
  int need_movement = 0;
  struct obj_data *vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(obj, VAL_HATCH_DEST));

  if (vehicle == NULL) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
  }

  if ((dest_room = IN_ROOM(vehicle)) == NOWHERE) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
  }

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
      IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    return (0);
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
      (SECT(dest_room) == SECT_WATER_NOSWIM)) {
    if (!has_boat(ch)) {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(IN_ROOM(ch))] +
		   movement_loss[SECT(dest_room)]) / 2;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(dest_room)) && !ROOM_FLAGGED(dest_room, ROOM_PLAYER_SHOP)) {

      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[dest_room])) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)))
    GET_MOVE(ch) -= need_movement;

  if (AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);
  else
    act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM);


  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) ) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n arrives from inside $p.", TRUE, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);
  else 
    act("$n arrives from inside $p.", TRUE, ch, vehicle, 0, TO_ROOM);

  if (ch->desc != NULL) {
    act(obj->action_description, TRUE, ch, obj, 0, TO_CHAR);
    look_at_room(IN_ROOM(ch), ch, 0);
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

int perform_leave_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = IN_ROOM(ch);
  int could_move = FALSE;
  struct follow_type *k;

  if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
     send_to_char(ch, "But the way out is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(obj, VAL_HATCH_DEST) != NOWHERE)
      if ((could_move = do_simple_leave(ch, obj, need_specials_check)))
        for (k = ch->followers; k; k = k->next)
          if ((IN_ROOM(k->follower) == was_in) &&
              (GET_POS(k->follower) >= POS_STANDING)) {
            act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            perform_leave_obj(k->follower, obj, 1);
	  }
  }
  return could_move;
}


ACMD(do_leave)
{
  int door;
  struct obj_data *obj = NULL;

  for (obj = world[IN_ROOM(ch)].contents; obj ; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if (GET_OBJ_TYPE(obj) ==  ITEM_HATCH) {
	perform_leave_obj(ch, obj, 0);
        return;
      }

  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}


ACMD(do_stand)
{
  struct char_data *tch;

  if (AFF_FLAGGED(ch, AFF_STUNNED)) {
    send_to_char(ch, "You cannot stand up right now, you are stunned");
    return;
  }
  if (FIGHTING(ch) && IS_NPC(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;
  case POS_SITTING:
    if (FIGHTING(ch) && !IS_NPC(ch)) {
      if (ch->active_turn) {
        if (!can_use_available_actions(ch, ACTION_MOVE)) {
          send_to_char(ch, "You do not have a move action to stand up with.\r\n");
          break;
        }
      } else {
        send_to_char(ch, "It is not your turn in combat.\r\n");
        break;
      }
    }
    send_to_char(ch, "You stand up.\r\n");
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    /* May be sitting for some reason and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if ((is_player_grouped(tch, FIGHTING(ch)) || FIGHTING(tch) == ch) && (GET_TOTAL_AOO((tch)) < MAX(1, HAS_FEAT((tch), FEAT_COMBAT_REFLEXES) ? 
          1+ability_mod_value(GET_DEX((tch))): 1)))
        do_attack_of_opportunity(tch, ch, "Standing");
    }
    break;
  case POS_RESTING:
    if (FIGHTING(ch) && !IS_NPC(ch)) {
      if (ch->active_turn) {
        if (!can_use_available_actions(ch, ACTION_MOVE)) {
          send_to_char(ch, "You do not have a move action to stand up with.\r\n");
          break;
        }
      } else {
        send_to_char(ch, "It is not your turn in combat.\r\n");
        break;
      }
    }
    send_to_char(ch, "You stop resting, and stand up.\r\n");
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if ((is_player_grouped(tch, FIGHTING(ch)) || FIGHTING(tch) == ch) && (GET_TOTAL_AOO((tch)) < MAX(1, HAS_FEAT((tch), FEAT_COMBAT_REFLEXES) ? 
          1+ability_mod_value(GET_DEX((tch))): 1)))
        do_attack_of_opportunity(tch, ch, "Standing");
    }
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Do you not consider fighting as standing?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You sit down.\r\n");
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You sit down and rest your tired bones.\r\n");
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You rest your tired bones.\r\n");
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  }
}


ACMD(do_sleep)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}


ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_STANDING;
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else {
    send_to_char(ch, "You awaken, and stand up.\r\n");
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
  }
}


ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  } else {
    send_to_char(ch, "Whom do you wish to follow?\r\n");
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char(ch, "You are already following yourself.\r\n");
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}

/* Mounts (DAK) */
ACMD(do_mount) {

  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int chance;
  one_argument(argument, arg);
  char buf[400];  

  if (!*arg) {
    if (ch->player_specials->mount_num == 0) {
      send_to_char(ch, "You do not have a mount called.\r\n");
      return;
    }
    send_to_char(ch, "Your mount is a %s.\r\n", ch->player_specials->mount_desc);
    send_to_char(ch, "He has %d of %d hit points.\r\n", ch->player_specials->mount_cur_hit , ch->player_specials->mount_max_hit);
    send_to_char(ch, "He has an armor class of %d and damage reduction of %d.\r\n", compute_mount_armor_class(ch, NULL) , ch->player_specials->mount_dr );
    send_to_char(ch, "He has the following attacks:\r\n");

    int i = 0;
    for (i = 0; i < 5; i++) {
      if (ch->player_specials->mount_attack_to_hit[i] > 0) {
        send_to_char(ch, "Attack %d: +%d to hit for %dd%d+%d damage.\r\n", i + 1, ch->player_specials->mount_attack_to_hit[i], 
                   ch->player_specials->mount_attack_ndice[i],
                   ch->player_specials->mount_attack_sdice[i], ch->player_specials->mount_attack_dammod[i]);
      }
    }
    send_to_char(ch, "\r\n");
    return;
  } 

  if (ch->player_specials->mounted != MOUNT_NONE) {
    send_to_char(ch, "You are already mounted.  Please dismount first.\r\n");
    return;
  }


  if (is_abbrev(arg, "summon")) {
    if (ch->player_specials->summon_num == 0) {
      send_to_char(ch, "You do not have a summon to mount.\r\n");
      return;
    }
    if (pet_list[ch->player_specials->summon_num].mount == FALSE) {
      send_to_char(ch, "Your summon is not mountable.\r\n");
      return;
    }
    if (pet_list[ch->player_specials->summon_num].size <= get_size(ch)) {
      send_to_char(ch, "That mount is too small for you.\r\n");
      return;
    }
    sprintf(buf, "You mount %s.", ch->player_specials->summon_desc);
    act(buf, true, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n mounts %s.", ch->player_specials->summon_desc);
    act(buf, true, ch, 0, 0, TO_ROOM);
    ch->player_specials->mounted = MOUNT_SUMMON;
  }
  else {
    if (ch->player_specials->mount_num == 0) {
      send_to_char(ch, "You do not have a mount.\r\n");
      return;
    }
    if (pet_list[ch->player_specials->mount_num].size <= get_size(ch)) {
      send_to_char(ch, "That mount is too small for you.\r\n");
      return;
    }
    sprintf(buf, "You mount %s.", ch->player_specials->mount_desc);
    act(buf, true, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n mounts %s.", ch->player_specials->mount_desc);
    act(buf, true, ch, 0, 0, TO_ROOM);
    ch->player_specials->mounted = MOUNT_MOUNT;    
  }
  return;
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    OUTPUT_TO_CHAR("There is no-one by that name here.\r\n", ch);
    return;
  } else if (!IS_NPC(vict)) {
    OUTPUT_TO_CHAR("Ehh... no.\r\n", ch);
    return;
  } else if (RIDING(ch) || RIDDEN_BY(ch)) {
    OUTPUT_TO_CHAR("You are already mounted.\r\n", ch);
    return;
  } else if (RIDING(vict) || RIDDEN_BY(vict)) {
    OUTPUT_TO_CHAR("It is already mounted.\r\n", ch);
    return;
  } else if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
    OUTPUT_TO_CHAR("You can't mount that!\r\n", ch);
    return;
  } else if (AFF_FLAGGED(vict, AFF_CHARM) && vict->master != ch) {
    act("$E will not you mount $M.", false, ch, 0, vict, TO_VICT);
  } else if (skill_roll(ch, SKILL_RIDE) < 15) {
    act("You try to mount $N, but slip and fall off. Ouch! Brush up on your riding skills first!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to mount you, but slips and falls off. Ouch!", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to mount $N, but slips and falls off. Ouch!", TRUE, ch, 0, vict, TO_NOTVICT);
    damage(ch, ch, dice(1, 2), TYPE_UNDEFINED, 0, -1, TYPE_UNDEFINED, 0, 1);
    return;
  }
 
  if (skill_roll(ch, SKILL_HANDLE_ANIMAL) >= 15)
    SET_BIT_AR(AFF_FLAGS(vict), AFF_TAMED);
 
  act("You mount $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n mounts you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n mounts $N.", TRUE, ch, 0, vict, TO_NOTVICT);

 if (vict->master != ch) {
  if (vict->master)
      stop_follower(vict);

    add_follower(vict, ch);
 }

  GET_POS(ch) = POS_RIDING;

  mount_char(ch, vict);
  
  if (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED) &&!AFF_FLAGGED(vict, AFF_CHARM) && skill_roll(ch, SKILL_RIDE) < 5) {
    act("$N suddenly bucks upwards, throwing you violently to the ground!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You buck violently and throw $n to the ground.", FALSE, ch, 0, vict, TO_VICT);
    dismount_char(ch);
    GET_POS(ch) = POS_SITTING;


    chance = dice(1, 10);

    if (chance <= 3) 
    hit(vict, ch, TYPE_UNDEFINED);

    else 
     damage(ch, ch, dice(1,3), TYPE_UNDEFINED, 0, -1, TYPE_UNDEFINED, 0, 1);

  }
}


ACMD(do_dismount) {

  char buf[400];

  if (ch->player_specials->mounted == MOUNT_NONE) {
    send_to_char(ch, "You are not mounted.\r\n");
    return;
  }
  else if (ch->player_specials->mounted == MOUNT_SUMMON) {
    sprintf(buf, "You dismount %s.", ch->player_specials->summon_desc);
    act(buf, true, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n dismounts %s.", ch->player_specials->summon_desc);
    act(buf, true, ch, 0, 0, TO_ROOM);    
  }
  else {
    sprintf(buf, "You dismount %s.", ch->player_specials->mount_desc);
    act(buf, true, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n dismounts %s.", ch->player_specials->mount_desc);
    act(buf, true, ch, 0, 0, TO_ROOM);    
  }

  ch->player_specials->mounted = MOUNT_NONE;

  return;
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg) {
    OUTPUT_TO_CHAR("Dismount from who?\r\n", ch);
    return;
  } else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    OUTPUT_TO_CHAR("There is no-one by that name here.\r\n", ch);
    return;
  }
    else if (!RIDING(ch)) {
    OUTPUT_TO_CHAR("You aren't even riding anything.\r\n", ch);
    return;
  } else if (SECT(ch->in_room) == SECT_WATER_NOSWIM) {
    OUTPUT_TO_CHAR("Yah, right, and then drown...\r\n", ch);
    return;
  }
  
  act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
  act("$n dismounts from you.", FALSE, ch, 0, RIDING(ch), TO_VICT);
  act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_NOTVICT);


  GET_POS(ch) = POS_STANDING;

/*if ((vict->master == ch) || AFF_FLAGGED(vict, AFF_CHARM) || AFF_FLAGGED(vict, AFF_TAMED)){
   
   if (!allowNewFollower(ch, 3))
    {
      return;
    }
                                                                                
    add_follower(vict, ch);
} */
 
   dismount_char(ch);
}


ACMD(do_buck) {
  if (!RIDDEN_BY(ch)) {
    OUTPUT_TO_CHAR("You're not even being ridden!\r\n", ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_TAMED) || AFF_FLAGGED(ch, AFF_CHARM)){
    OUTPUT_TO_CHAR("But you're tamed!\r\n", ch);
    return;
  }
  
  act("You quickly buck, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_CHAR);
  act("$n quickly bucks, throwing you to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_VICT);
  act("$n quickly bucks, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_NOTVICT);
  GET_POS(RIDDEN_BY(ch)) = POS_SITTING;
  if (rand_number(0, 4)) {
    OUTPUT_TO_CHAR("You hit the ground hard!\r\n", RIDDEN_BY(ch));
    damage(RIDDEN_BY(ch), RIDDEN_BY(ch), dice(2,4), TYPE_UNDEFINED, 0, -1, TYPE_UNDEFINED, 0, 1);
  }
  dismount_char(ch);
  
  
  /*
   * you might want to call set_fighting() or some nonsense here if you
   * want the mount to attack the unseated rider or vice-versa.
   */
}

ACMD(do_tame) {
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  struct affected_type af;
  struct char_data *vict;
  int percent, chance;
  struct follow_type *f;
  int j = 0;

  percent = dice(1, 101) - GET_CHA(ch) ;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    OUTPUT_TO_CHAR("Tame who?\r\n", ch);
    return;
  } else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    OUTPUT_TO_CHAR("They're not here.\r\n", ch);
    return;
  } else if (!IS_NPC(vict)) {
    send_to_char(ch, "You can only tame mobs.\r\n");
	return;
  } else if (*arg2) {
    if (race_list[GET_RACE(vict)].family != RACE_TYPE_ANIMAL)  {
	  send_to_char(ch, "Only animals can be tamed to be familiars or animal companions.\r\n");
	  return;
	}
    if (is_abbrev(arg2, "companion")) {
	  if (!HAS_FEAT(ch, FEAT_ANIMAL_COMPANION)) {
	    send_to_char(ch, "You can't choose an animal companion.\r\n");
		return;
	  }
      for (f = ch->followers; f; f = f->next) {
        if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
	      if (GET_MOB_VNUM(f->follower) == GET_COMPANION_VNUM(ch))  {
	        send_to_char(ch, "You have already called your animal companion.\r\n");
		    return;
	      }
        }    
      }	  
	  GET_COMPANION_VNUM(ch) = GET_MOB_VNUM(vict);
	  SET_BIT_AR(AFF_FLAGS(vict), AFF_CHARM);
      GET_HITDICE(vict) = 1;
      GET_MAX_HIT(vict) = 0;
      for (j = 0; j <= GET_HITDICE(vict); j++)
        advance_mob_level(vict, GET_CLASS(vict));	  
      set_auto_mob_stats(vict);
	}
    else if (is_abbrev(arg2, "familiar")) {
	  if (!HAS_FEAT(ch, FEAT_SUMMON_FAMILIAR)) {
	    send_to_char(ch, "You can't choose a familar.\r\n");
		return;
	  }
      for (f = ch->followers; f; f = f->next) {
        if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
	      if (GET_MOB_VNUM(f->follower) == GET_FAMILIAR_VNUM(ch))  {
	        send_to_char(ch, "You have already called your familiar.\r\n");
		    return;
	      }
        }    
      }	  
	  GET_FAMILIAR_VNUM(ch) = GET_MOB_VNUM(vict);
	  SET_BIT_AR(AFF_FLAGS(vict), AFF_CHARM);
      GET_HITDICE(vict) = 1;
      GET_MAX_HIT(vict) = 0;
      for (j = 0; j <= GET_HITDICE(vict); j++)
        advance_mob_level(vict, GET_CLASS(vict));	  
      set_auto_mob_stats(vict);
	  set_familiar_stats(vict);
	}	
  } else if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
    OUTPUT_TO_CHAR("You can't do that to them.\r\n", ch);
    return;
  } else if (!GET_SKILL(ch, SKILL_HANDLE_ANIMAL)) {
    OUTPUT_TO_CHAR("You don't even know how to tame something.\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
    OUTPUT_TO_CHAR("You can't do that.\r\n", ch);
    return;
  } else if (RIDING(vict) || RIDDEN_BY(vict)) {
    OUTPUT_TO_CHAR("But they are mounted by someone!\r\n", ch);
    return;
  } else if (IS_AFFECTED(vict, AFF_CHARM) && vict->master != ch ){
    OUTPUT_TO_CHAR("They have already a master.\r\n", ch);
    return;
  }
  else if (IS_AFFECTED(vict, AFF_TAMED) && vict->master != ch){
    OUTPUT_TO_CHAR("They have already a master.\r\n", ch);
    return;
  }
  else if ((IS_AFFECTED(vict, AFF_CHARM) || IS_AFFECTED(vict, AFF_TAMED)) && vict->master == ch ){
    OUTPUT_TO_CHAR("You've already tamed them!\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM)){
    OUTPUT_TO_CHAR("Your master might not approve of that!.\r\n", ch);
    return;
  } else if (skill_roll(ch, SKILL_HANDLE_ANIMAL) < 15)  {
    OUTPUT_TO_CHAR("You fail to tame it.\r\n", ch);

    chance = dice(1, 10);

    if (chance <= 3) {
      send_to_char(ch, "The animal becomes angry from your actions and attacks!\r\n");
      hit(vict, ch, TYPE_UNDEFINED);
    }

    return;
  }

 else {
/*
    if (!allowNewFollower(ch, 3))
    {
      return;
    }
*/
    if (vict->master)
      stop_follower(vict);

    add_follower(vict, ch);

    af.type      = SPELL_CHARM;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_TAMED;

 affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);

    act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tames you.", FALSE, ch, 0, vict, TO_VICT);
    act("$n tames $N.", FALSE, ch, 0, vict, TO_NOTVICT);


    if (IS_NPC(vict)) {
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_AGGRESSIVE);
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_SPEC);
}
}

}

ACMD(do_mrest)
{
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k;
	char arg[MAX_STRING_LENGTH];

  one_argument(argument, arg);

 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
     OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return;
}

 if (GET_POS(ch) == POS_RIDING) {
     OUTPUT_TO_CHAR("YPerhaps you should dismount first?\r\n", ch);
    return ;
}

  if (!*arg) {
    OUTPUT_TO_CHAR("Who?!?\r\n", ch);
    return;
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) && !is_abbrev(arg, "followers"))
    OUTPUT_TO_CHAR("That person isn't here.\r\n", ch);
  else if (ch == vict)
    OUTPUT_TO_CHAR("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      OUTPUT_TO_CHAR("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if (vict->master != ch)
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);

      else if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_MOUNTABLE))  {
      OUTPUT_TO_CHAR("They are not a mount!!\r\n", ch);
      return;
     }

      else {
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
        command_interpreter(vict, "rest");
      }
    } else {                    
      
      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM) || IS_AFFECTED(k->follower, AFF_TAMED)) {
            found = TRUE;
            command_interpreter(k->follower, "rest");
          }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}

ACMD(do_mreport)
{
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k;
  struct char_data *l;
	struct follow_type *f;	
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];

  one_argument(argument, arg);

 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
     OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return;
}

if (!IS_AFFECTED(ch, AFF_GROUP)) {
	OUTPUT_TO_CHAR("But you are not a member of any group!\r\n", ch);
	return;
}

  if (!*arg) {
    OUTPUT_TO_CHAR("Who?!?\r\n", ch);
    return;
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) && !is_abbrev(arg, "followers"))
    OUTPUT_TO_CHAR("That person isn't here.\r\n", ch);
  else if (ch == vict)
    OUTPUT_TO_CHAR("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      OUTPUT_TO_CHAR("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if (vict->master != ch)
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);

      else if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_MOUNTABLE))  {
      OUTPUT_TO_CHAR("They are not a mount!!\r\n", ch);
      return;
     }

		  l = (ch->master ? ch->master : ch);

		  for (f = l->followers; f; f = f->next)
		    if (f->follower != ch)
		    {
		      GET_NAME_II(vict, f->follower, chname);
		      sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
		          chname, GET_HIT(vict), GET_MAX_HIT(vict),
		          GET_MOVE(vict), GET_MAX_MOVE(vict));
		      FREE_NAME(chname);
		      CAP(buf);
		      OUTPUT_TO_CHAR(buf, f->follower);
		    }

		  if (l != ch)
		  {
		    GET_NAME_II(vict, l, chname);
		    sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
		        chname, GET_HIT(vict), GET_MAX_HIT(vict),
		        GET_MOVE(vict), GET_MAX_MOVE(vict));
		    FREE_NAME(chname);
		    CAP(buf);
		    OUTPUT_TO_CHAR(buf, l);
		  }
		 
			GET_NAME_II(vict, ch, chname);
			sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
					chname, GET_HIT(vict), GET_MAX_HIT(vict),
					GET_MOVE(vict), GET_MAX_MOVE(vict));
			FREE_NAME(chname);
			CAP(buf);
			OUTPUT_TO_CHAR(buf, ch);		 

		 
    } else {                    
      
      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM) || IS_AFFECTED(k->follower, AFF_TAMED)) {
            found = TRUE;

			  		l = (ch->master ? ch->master : ch);

					  for (f = l->followers; f; f = f->next)
					    if (f->follower != ch)
					    {
					      GET_NAME_II(k->follower, f->follower, chname);
					      sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
					          chname, GET_HIT(k->follower), GET_MAX_HIT(k->follower),
					          GET_MOVE(k->follower), GET_MAX_MOVE(k->follower));
					      FREE_NAME(chname);
					      CAP(buf);
					      OUTPUT_TO_CHAR(buf, f->follower);
					    }

						  if (l != ch)
						  {
						    GET_NAME_II(k->follower, l, chname);
						    sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
						        chname, GET_HIT(k->follower), GET_MAX_HIT(k->follower),
						        GET_MOVE(k->follower), GET_MAX_MOVE(k->follower));
						    FREE_NAME(chname);
						    CAP(buf);
						    OUTPUT_TO_CHAR(buf, l);
						  }
						 
							GET_NAME_II(k->follower, ch, chname);
							sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n",
									chname, GET_HIT(k->follower), GET_MAX_HIT(k->follower),
									GET_MOVE(k->follower), GET_MAX_MOVE(k->follower));
							FREE_NAME(chname);
							CAP(buf);
							OUTPUT_TO_CHAR(buf, ch);
          }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}


ACMD(do_release)
{
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k;
	char arg[MAX_STRING_LENGTH];

  one_argument(argument, arg);

 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
     OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return;
}

 if (GET_POS(ch) == POS_RIDING) {
     OUTPUT_TO_CHAR("Perhaps you should dismount first?\r\n", ch);
    return ;
}

  if (!*arg) {
    OUTPUT_TO_CHAR("Who?!?\r\n", ch);
    return;
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    OUTPUT_TO_CHAR("They aren't here.\r\n", ch);
  else if (ch == vict)
    OUTPUT_TO_CHAR("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      OUTPUT_TO_CHAR("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {

      if (vict->master != ch)
        act("$n is not your mount!", FALSE, ch, 0, 0, TO_ROOM);

      else if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_MOUNTABLE))  {
      OUTPUT_TO_CHAR("They are not a mount!!\r\n", ch);
      return;
     }

      else {
        act("$n releases $N's tether, freeing $M.", FALSE, ch, 0, vict, TO_ROOM);
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
        stop_follower(vict);
        affect_from_char(vict, SPELL_CHARM);
             }
    } else {

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_TAMED)) {
            found = TRUE;
         act("$n releases $N's tether, freeing $M.", FALSE, ch, 0, vict, TO_ROOM);
        stop_follower(vict);
        affect_from_char(vict, SPELL_CHARM);

               }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("You don't seem to own a mount!\r\n", ch);
    }
  }
}



ACMD(do_mstand)
{
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k;
  char arg[MAX_STRING_LENGTH];
	
  one_argument(argument, arg);

  if (!*arg) {
    OUTPUT_TO_CHAR("Who?!?\r\n", ch);
    return;
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) && !is_abbrev(arg, "followers"))
    OUTPUT_TO_CHAR("That person isn't here.\r\n", ch);
  else if (ch == vict)
    OUTPUT_TO_CHAR("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      OUTPUT_TO_CHAR("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {

      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if (vict->master != ch)
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);

      else if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_MOUNTABLE))  {
      OUTPUT_TO_CHAR("They are not a mount!!\r\n", ch);
      return;
     }

      else {
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
        command_interpreter(vict, "stand");
      }
    } else {

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM) || IS_AFFECTED(k->follower, AFF_TAMED)) {
            found = TRUE;
            command_interpreter(k->follower, "stand");
          }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}
/*
ACMD(do_mget)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k;
	char arg[MAX_STRING_LENGTH];
  

  struct obj_data *obj;
  int i;

  

  half_chop(argument, message, name);

 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
     OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return;
}
  if (!*name || !*message)
    OUTPUT_TO_CHAR("Get what from who?\r\n", ch);
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name,
"followers"))
    OUTPUT_TO_CHAR("That person isn't here.\r\n", ch);
  else if (ch == vict)
    OUTPUT_TO_CHAR("You obviously suffer from skitzofrenia.\r\n", ch);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    OUTPUT_TO_CHAR("Your arms are already full!\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      OUTPUT_TO_CHAR("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
        if (vict->master != ch && (vict != ch))
        OUTPUT_TO_CHAR("They are not your mount!\r\n", ch);
      else {
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
        if  (!(obj = get_object_in_equip_vis(vict, message, vict->equipment, &i))) {
      sprintf(buf, "They don't seem to have %s %s.\r\n", AN(message), message);
      OUTPUT_TO_CHAR(buf, ch);
    }

       else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    }
        else if ((IS_CARRYING_W(vict) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(vict)) {
    act("$p: they can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    }

       else {
        perform_remove(vict, i);
        perform_give(vict, ch, obj);
        
      }
    }
  }
   else { 

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM) || IS_AFFECTED(k->follower, AFF_TAMED)) {
            found = TRUE;
             if  (!(obj = get_object_in_equip_vis(k->follower, message, k->follower->equipment, &i))) {
                sprintf(buf, "They don't seem to have %s %s.\r\n", AN(message), message);
                OUTPUT_TO_CHAR(buf, ch);
              }
             else {
              perform_remove(k->follower, i);
              perform_give(k->follower, ch, obj);
           }
          }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}

ACMD(do_mgive)
{
  int dotmode;
  struct char_data *vict;
  struct follow_type *k;
  struct obj_data *obj, *next_obj;
  char buf[MAX_INPUT_LENGTH];
  bool found = FALSE;
  room_rnum org_room;
	char arg[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];

  argument = one_argument(argument, arg);

 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
     OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return;
}
  if (!*arg) {
    OUTPUT_TO_CHAR("Give what to who?\r\n", ch);

  } else if (is_money(arg)) {
     OUTPUT_TO_CHAR("You can't give money to them!.\r\n", ch);

  } else {
    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
   else if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_MOUNTABLE))  {
      OUTPUT_TO_CHAR("They are not a mount!!\r\n", ch);
      return;
   }
   else if ((vict->master != ch) && (vict != ch)) {
        OUTPUT_TO_CHAR("They are not your mount!\r\n", ch);
        return;
    }


    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
        sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        OUTPUT_TO_CHAR(buf, ch);
      } else if (!CAN_WEAR(obj, ITEM_WEAR_ONBACK)) {
        OUTPUT_TO_CHAR("You can't put that on your mount's back!\r\n", ch);
        return;
        }
       else if (GET_EQ(vict, WEAR_ONBACK_1) && GET_EQ(vict, WEAR_ONBACK_2) && GET_EQ(vict, WEAR_ONBACK_3)) {
    OUTPUT_TO_CHAR("Your mount could not carry that much!\r\n", ch);
    return;
  }
      else {
        perform_give(ch, vict, obj);
        perform_wear(vict, obj, WEAR_ONBACK_1);
       }
      } else {
 
       org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM)) {
            found = TRUE;
            if (dotmode == FIND_ALLDOT && !*arg) {
        OUTPUT_TO_CHAR("All of what?\r\n", ch);
        return;
      }
      if (!ch->carrying)
        OUTPUT_TO_CHAR("You don't seem to be holding anything.\r\n", ch);
      else
        for (obj = ch->carrying; obj; obj = next_obj) {
          next_obj = obj->next_content;
          if (CAN_WEAR(obj, ITEM_WEAR_ONBACK) && CAN_SEE_OBJ(ch, obj) &&
              ((dotmode == FIND_ALL || isname(arg, obj->name))))
            perform_give(ch, vict, obj);
            perform_wear(vict, obj, WEAR_ONBACK_1);
            }
          }
      }
      if (found)
        OUTPUT_TO_CHAR(CONFIG_OK, ch);
      else
        OUTPUT_TO_CHAR("Nobody here is a loyal subject of yours!\r\n", ch);
    }
}

}
*/

ACMD(do_run)
{
  int n;

  struct affected_type af2[2];

  if (IS_SET_AR(PRF_FLAGS(ch), PRF_RUN)) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_RUN);
    send_to_char(ch, "You stop running and become fatigued.\r\n");

    af2[0].type = SPELL_AFF_FATIGUED;
    af2[0].location = APPLY_STR;
    af2[0].bitvector = AFF_FATIGUED;
    af2[0].duration = MAX(1, GET_ROUNDS_RUNNING(ch) / 2);
    af2[0].modifier =   -2;

    af2[1].type = SPELL_AFF_FATIGUED;
    af2[1].location = APPLY_DEX;
    af2[1].bitvector = AFF_FATIGUED;
    af2[1].duration = MAX(1, GET_ROUNDS_RUNNING(ch) / 2);
    af2[1].modifier =   -2;

    for (n = 0; n < 2; n++)
      affect_join(ch, af2+n, true, false, false, false);

    GET_ROUNDS_RUNNING(ch) = 0;

  }
  else if (!affected_by_spell(ch, SPELL_AFF_FATIGUED)) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_RUN);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_JOG);
    send_to_char(ch, "You begin to run.\r\n");
  }
  else {
    send_to_char(ch, "You cannot run because you are fatigued.\r\n");
  }

}

ACMD(do_jog)
{
  int n;

  struct affected_type af2[2];

  if (IS_SET_AR(PRF_FLAGS(ch), PRF_JOG)) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_JOG);
    send_to_char(ch, "You stop jogging and become fatigued.\r\n");

    af2[0].type = SPELL_AFF_FATIGUED;
    af2[0].location = APPLY_STR;
    af2[0].bitvector = AFF_FATIGUED;
    af2[0].duration = MAX(1, GET_ROUNDS_RUNNING(ch) / 3);
    af2[0].modifier =   -2;

    af2[1].type = SPELL_AFF_FATIGUED;
    af2[1].location = APPLY_DEX;
    af2[1].bitvector = AFF_FATIGUED;
    af2[1].duration = MAX(1, GET_ROUNDS_RUNNING(ch) / 3);
    af2[1].modifier =   -2;

    for (n = 0; n < 2; n++)
      affect_join(ch, af2+n, true, false, false, false);

    GET_ROUNDS_RUNNING(ch) = 0;

  }
  else if (!affected_by_spell(ch, SPELL_AFF_FATIGUED)) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_RUN);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_JOG);
    send_to_char(ch, "You begin to jog.\r\n");
  }
  else {
    send_to_char(ch, "You cannot jog because you are fatigued.\r\n");
  }

}

