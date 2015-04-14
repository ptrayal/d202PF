/* 
************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: mobact.c 57 2009-03-24 00:15:02Z gicker $");

#include "structs.h"
#include "feats.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "shop.h"

/* external globals */
extern int no_specials;
extern int top_shop;
extern struct shop_data *shop_index;

/* external functions */
ACMD(do_action);
ACMD(do_taunt);
ACMD(do_kick);
ACMD(do_disarm);
ACMD(do_trip);
void hunt_victim(struct char_data *ch);
int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int ok_shop_room(int shop_nr, room_vnum room);
SPECIAL(shop_keeper);
void npc_steal(struct char_data *ch, struct char_data *victim);
int wizard_cast_buff(struct char_data *ch);
int wizard_cast_spell(struct char_data *ch, struct char_data *vict);
int cleric_cast_buff(struct char_data *ch);
int cleric_cast_spell(struct char_data *ch, struct char_data *vict);
int cleric_cast_cure(struct char_data *ch);
int fighter_perform_action(struct char_data *ch, struct char_data *vict);
int rogue_perform_action(struct char_data *ch, struct char_data *vict);

/* local functions */
void decide_mobile_special_action(struct char_data *ch);
void mobile_activity(void);
void clearMemory(struct char_data *ch);
bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found = FALSE, max;
  memory_rec *names;
  int spell_mod = 0, max_hit_dice = 0;
  struct affected_type *af = NULL;
  int spell_cast = FALSE;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    spell_cast = FALSE;

    if (!ch)
      continue;

 if (ch && !IS_MOB(ch))
      continue;

    /* Examine call for special procedure */
 if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) 
      {
          log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
               GET_NAME(ch), GET_MOB_VNUM(ch));
          REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
     } 
     else 
     {
        char actbuf[MAX_INPUT_LENGTH] = "";
        if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
       continue;         /* go to next char */
   }
}

    /* If the mob has no specproc, do the default actions */

if (!AWAKE(ch))
    continue;

if (IN_ROOM(ch) == NOWHERE)
    continue;

if (FIGHTING(ch))
    continue;

decide_mobile_special_action(ch);


    /* hunt a victim, if applicable */
hunt_victim(ch);

    /* Scavenger (picking up objects) */
if (IS_HUMANOID(ch) && !FIGHTING(ch) && AWAKE(ch) && MOB_FLAGGED(ch, MOB_SCAVENGER))
 if (world[IN_ROOM(ch)].contents && !rand_number(0, 10)) {
     max = 1;
     best_obj = NULL;
     for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
       if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
         best_obj = obj;
         max = GET_OBJ_COST(obj);
    }
    if (best_obj != NULL) {
       perform_get_from_room(ch, best_obj);
  }
}

    /* Mob Movement */
if (!FIGHTING(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
     ((door = (dice(1, 100) - 1)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
     !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
     !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
     (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
      (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) {
 if (dice(1, 10) == 1)
   perform_move(ch, door, 1);
}

    /* Aggressive Mobs */
if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) || MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN)) {
 found = FALSE;
 for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
     if (!CAN_SEE(ch, vict) || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
       continue;

  if (!CONFIG_MOB_FIGHTING && IS_NPC(vict))
     continue;
if (vict == ch)
  continue;
if (GET_MOB_VNUM(ch) == GET_MOB_VNUM(vict))
  continue;
if (GET_RACE(ch) == GET_RACE(vict))
  continue;
if (!ok_damage_shopkeeper(ch, vict))
     continue;
if ((GET_LEVEL(vict) - GET_LEVEL(ch)) > 3 && IS_NPC(vict))
  continue;
if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
  continue;

if (GET_FORM_POS(vict) > FORM_POS_FRONT)
     continue;

if (MOB_FLAGGED(ch, MOB_AGGRESSIVE  ) ||
   (MOB_FLAGGED(ch, MOB_AGGR_EVIL   ) && IS_EVIL(vict)) ||
   (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
   (MOB_FLAGGED(ch, MOB_AGGR_GOOD   ) && IS_GOOD(vict))) {

          /* Can a master successfully control the charmed monster? */
     if (aggressive_mob_on_a_leash(ch, ch->master, vict))
       continue;

  if (affected_by_spell(vict, SPELL_CALM_ANIMAL)) {
       if (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL) {
         if (!ch->affected)
           continue;
      for (af = ch->affected; af; af = af->next) {
           if (af->type == SPELL_CALM_ANIMAL && af->location == APPLY_ABILITY)
             spell_mod = af->modifier;
        if (af->type == SPELL_CALM_ANIMAL && af->location == APPLY_LEVEL)
             max_hit_dice = af->modifier;
   }
   if (GET_LEVEL(ch) <= max_hit_dice && (GET_LEVEL(ch) < 10 || mag_newsaves(SAVING_WILL, vict, ch, SPELL_CALM_ANIMAL, 11 + spell_mod)))
      continue;
}
}

hit(ch, vict, TYPE_UNDEFINED);
found = TRUE;
}
}
}

if (!found) {
    /* Aggressive Mobs */
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) || MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN)) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
          if (!CAN_SEE(ch, vict) || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
            continue;

       if (!CONFIG_MOB_FIGHTING && IS_NPC(vict))
          continue;
     if (vict == ch)
       continue;
  if (GET_MOB_VNUM(ch) == GET_MOB_VNUM(vict))
       continue;
  if (GET_RACE(ch) == GET_RACE(vict))
       continue;
  if (!ok_damage_shopkeeper(ch, vict))
     continue;
if ((GET_LEVEL(vict) - GET_LEVEL(ch)) > 3 && IS_NPC(vict))
  continue;
if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
  continue;

if (MOB_FLAGGED(ch, MOB_AGGRESSIVE  ) ||
   (MOB_FLAGGED(ch, MOB_AGGR_EVIL   ) && IS_EVIL(vict)) ||
   (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
   (MOB_FLAGGED(ch, MOB_AGGR_GOOD   ) && IS_GOOD(vict))) {

          /* Can a master successfully control the charmed monster? */
     if (aggressive_mob_on_a_leash(ch, ch->master, vict))
       continue;

  if (affected_by_spell(vict, SPELL_CALM_ANIMAL)) {
       if (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL) {
         if (!ch->affected)
           continue;
      for (af = ch->affected; af; af = af->next) {
           if (af->type == SPELL_CALM_ANIMAL && af->location == APPLY_ABILITY)
             spell_mod = af->modifier;
        if (af->type == SPELL_CALM_ANIMAL && af->location == APPLY_LEVEL)
             max_hit_dice = af->modifier;
   }
   if (GET_LEVEL(ch) <= max_hit_dice && (GET_LEVEL(ch) < 10 || mag_newsaves(SAVING_WILL, vict, ch, SPELL_CALM_ANIMAL, 11 + spell_mod)))
      continue;
}
}

hit(ch, vict, TYPE_UNDEFINED);
found = TRUE;
}
}
}
}

    /* Mob Memory */
if (!FIGHTING(ch) && MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch) && !FIGHTING(ch)) {
 found = FALSE;
 for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
     if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
       continue;

  for (names = MEMORY(ch); names && !found; names = names->next) {
       if (names->id != GET_IDNUM(vict))
            continue;

          /* Can a master successfully control the charmed monster? */
       if (aggressive_mob_on_a_leash(ch, ch->master, vict))
            continue;

       found = TRUE;
       act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
       hit(ch, vict, TYPE_UNDEFINED);
  }
}
}

    /*
     * Charmed Mob Rebellion
     *
     * In order to rebel, there need to be more charmed monsters
     * than the person can feasibly control at a time.  Then the
     * mobiles have a chance based on the charisma of their leader.
     *
     * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc.
     */
     if (!FIGHTING(ch) && 
        AFF_FLAGGED(ch, AFF_CHARM) && ch->master && num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3) {
      if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
        if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
          hit(ch, ch->master, TYPE_UNDEFINED);
     stop_follower(ch);
}
}

    /* Helper Mobs */
if (!FIGHTING(ch) && MOB_FLAGGED(ch, MOB_HELPER) && 
   !AFF_FLAGGED(ch, AFF_BLIND) &&
   !AFF_FLAGGED(ch, AFF_CHARM)) {
 found = FALSE;
for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
     if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
       continue;
  if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
       continue;

  act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
  hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
  found = TRUE;
}
}

    /* Add new mobile actions here */

if (IS_ROGUE(ch)) {
 int shop_nr;
 found = FALSE;
      /* Is there a shopkeeper around? */
 for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
   if (GET_MOB_SPEC(vict) == shop_keeper) {
          /* Ok, vict is a shop keeper.  Which shop is his? */
     for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
       if (SHOP_KEEPER(shop_nr) == vict->nr)
         break;
    if (shop_nr <= top_shop)
            /* Is the shopkeeper in his shop? */
       if (ok_shop_room(shop_nr, IN_ROOM(vict)))
              /* Does the shopkeeper prevent stealing? */
         if (!SHOP_ALLOW_STEAL(shop_nr))
           found = TRUE;
 }
      /* Note: found will be true if there the character is in a shop where
       * the shopkeeper present who doesn't allow stealing.  Don't bother 
       * running the next loop, since we can't steal from anyone anyway. 
       */
  }
  for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
   if (vict == ch)
     continue;
if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
     continue;
if (!IS_HUMANOID(vict))
     continue;
if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL))
     continue;
if (GET_MOB_VNUM(ch) == GET_MOB_VNUM(vict))
     continue;
if (skill_roll(ch, SKILL_SLEIGHT_OF_HAND) > skill_roll(vict, SKILL_PERCEPTION) && dice(1, 10) == 1 && GET_MOB_SPEC(ch) == NULL) {
     npc_steal(ch, vict);
     found = TRUE;
}
}
}

  }                 /* end for() */
}



/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
     memory_rec *tmp;
     bool present = FALSE;

     if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
          return;

     for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
          if (tmp->id == GET_IDNUM(victim))
           present = TRUE;

     if (!present) 
     {
         CREATE(tmp, memory_rec, 1);
         tmp->next = MEMORY(ch);
         tmp->id = GET_IDNUM(victim);
         MEMORY(ch) = tmp;
    }
}


/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
     memory_rec *curr, *prev = NULL;

     if (!(curr = MEMORY(ch)))
          return;

     while (curr && curr->id != GET_IDNUM(victim)) 
     {
          prev = curr;
          curr = curr->next;
     }

     if (!curr)
    return;              /* person wasn't there at all. */

          if (curr == MEMORY(ch))
               MEMORY(ch) = curr->next;
          else
               prev->next = curr->next;

          free(curr);
     }


/* erase ch's memory */
     void clearMemory(struct char_data *ch)
     {
          memory_rec *curr, *next;

          curr = MEMORY(ch);

          while (curr) 
          {
               next = curr->next;
               free(curr);
               curr = next;
          }

          MEMORY(ch) = NULL;
     }


/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
     static int snarl_cmd;
     int dieroll;

     if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
          return (FALSE);

     if (!snarl_cmd)
          snarl_cmd = find_command("snarl");

/* Sit. Down boy! HEEEEeeeel! */
     dieroll = rand_number(1, 20);
     if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
          if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
               char victbuf[MAX_NAME_LENGTH + 1];

strncpy(victbuf, GET_NAME(attack), sizeof(victbuf));  /* strncpy: OK */
               victbuf[sizeof(victbuf) - 1] = '\0';

               do_action(slave, victbuf, snarl_cmd, 0);
          }

/* Success! But for how long? Hehe. */
          return (TRUE);
     }

/* So sorry, now you're a player killer... Tsk tsk. */
     return (FALSE);
}

void decide_mobile_special_action(struct char_data *ch) 
{

     int roll = 0;
     int spell_cast = FALSE;
     int spell_found = FALSE;
     int sp = 0;
     byte no_combat_act = FALSE;

     struct char_data * vict = FIGHTING(ch);

     if (ch && vict && IN_ROOM(ch) != IN_ROOM(vict))
          return;

     if (ch->active_turn == 1 || FIGHTING(ch)) 
     {
          if (GET_RACE(ch) == RACE_DRAGON && dice(1, 3) == 1 && FIGHTING(ch)) 
          {
               mag_areas(GET_LEVEL(ch), ch, SPELL_BREATH_WEAPON);
               return;
          }

          while (!spell_cast) 
          {
               if (!IS_SPELLCASTER(ch)) 
               {
                    spell_cast = FALSE;
                    break;
               }
               else if (!FIGHTING(ch) && dice(1, 3) == 1) 
               {
                    for (sp = 0; sp <= TOP_SPELL; sp++) {
                         if (spell_info[sp].violent == FALSE && IS_SET(spell_info[sp].targets, TAR_CHAR_ROOM) && 
                              IS_SET(spell_info[sp].routines, MAG_AFFECTS) && 
                              GET_SPELL_SLOT(ch, spell_info[sp].class_level[GET_CLASS(ch)]) > 0 &&
                              ((GET_LEVEL(ch) / 2) + 1) >= spell_info[sp].class_level[GET_CLASS(ch)] ) {
                              if (sp == SPELL_INVISIBLE || sp == SPELL_GREATER_INVISIBILITY)
                                   continue;
                              spell_found = TRUE;
                              if (!affected_by_spell(ch, sp) && dice(1, 500) == 1) 
                              {
                                   cast_spell(ch, ch, NULL, sp, NULL);
                                   spell_cast = TRUE;
                                   break;
                              }
                         }
                    }
                    if (!spell_found) 
                    {
                         spell_cast = TRUE;
                         break;
                    }     
               }
               else if (vict && !no_combat_act) 
               {
                    if (GET_SPELL_SLOT(ch, 1) == 0 && GET_SPELL_SLOT(ch, 2) == 0 && GET_SPELL_SLOT(ch, 3) == 0 &&
                         GET_SPELL_SLOT(ch, 4) == 0 && GET_SPELL_SLOT(ch, 5) == 0 && GET_SPELL_SLOT(ch, 6) == 0 &&
                         GET_SPELL_SLOT(ch, 7) == 0 && GET_SPELL_SLOT(ch, 8) == 0 && GET_SPELL_SLOT(ch, 9) == 0)
                         spell_cast = TRUE;
                    int num_checks = 0;
                    while (spell_cast == FALSE) 
                    {
                         num_checks++;
                         if (num_checks > (TOP_SPELL * 10))
                              break;
                         for (sp = 0; sp <= TOP_SPELL; sp++) 
                         {
                              if ( dice(1, TOP_SPELL) != 1)
                                   continue;
                              if (spell_info[sp].violent == TRUE && IS_SET(spell_info[sp].targets, TAR_CHAR_ROOM) &&
                                   (IS_SET(spell_info[sp].routines, MAG_AFFECTS) || 
                                        IS_SET(spell_info[sp].routines, MAG_DAMAGE) ||
                                        IS_SET(spell_info[sp].routines, MAG_AREAS)) && 
                                   GET_SPELL_SLOT(ch, spell_info[sp].class_level[GET_CLASS(ch)]) > 0 &&
                                   ((GET_LEVEL(ch) / 2) + 1) >= spell_info[sp].class_level[GET_CLASS(ch)] ) 
                              {
                                   spell_found = TRUE;
                                   if (!IS_SET(spell_info[sp].save_flags, MAGSAVE_DEATH) || (IS_SET(spell_info[sp].save_flags, MAGSAVE_DEATH))) 
                                   {
                                        cast_spell(ch, vict, NULL, sp, NULL);
                                        spell_cast = TRUE;
                                        break;
                                   }
                              }
                         }
                    }
                    if (!spell_found) 
                    {
                         spell_cast = TRUE;
                         break;
                    }
               }
               else 
               {
                    spell_cast = TRUE;
                    break;
               }
          }


          if (!no_combat_act && !spell_cast && vict && FIGHTING(ch) && dice(1, 10) <= 3) 
          {
               if (!FIGHTING(ch))
                    set_fighting(ch, vict);
               if (!FIGHTING(vict))
                    set_fighting(vict, ch);
               if (IS_HUMANOID(ch)) {
                    roll = dice (1, 12);
                    if (roll == 1) 
                    {
                         do_taunt(ch, "", 0, SCMD_TAUNT);
                    }
                    else if (roll == 2) 
                    {
                         do_taunt(ch, "", 0, SCMD_INTIMIDATE);
                    }
                    else if (roll == 3) 
                    {
                         do_taunt(ch, "", 0, SCMD_DIPLOMACY);
                    }
                    else if (roll >= 4 && roll <= 8) 
                    {
                         do_kick(ch, "", 0, 0);
                    }
                    else if (roll == 9)
                    {
                         do_disarm(ch, "", 0, 0);
                    }
                    else if (roll >= 10 && roll <= 12) 
                    {
                         do_trip(ch, "", 0, 0);
                    }
               }
               else 
               {
                    do_trip(ch, "", 0, 0);
               }
          }
     }



}
