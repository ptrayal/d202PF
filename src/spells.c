/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "feats.h"

SVNHEADER("$Id: spells.c 65 2009-04-01 13:06:57Z gicker $");


/* external variables */
extern struct clan_type *clan_info;
extern room_rnum r_mortal_start_room;
extern int mini_mud;
extern obj_vnum portal_object;
extern const char *apply_types[];

/* external functions */
int num_rooms_between(room_rnum src, room_rnum target);
void Crash_crashsave(struct char_data *ch, int backup);
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
void name_to_drinkcon(struct obj_data *obj, int type);
int level_exp(int level, int race);
void name_from_drinkcon(struct obj_data *obj);
int compute_armor_class(struct char_data *ch, struct char_data *att);
char *which_desc(struct char_data *ch);
int has_intro(struct char_data *ch, struct char_data *target);
ACMD(do_raise);
int mob_exp_by_level(int level);
int is_player_grouped(struct char_data *target, struct char_data *group);

ASPELL(spell_raise_dead)
{

  struct char_data *vict = victim;

  if (!victim)
    return;

  if (ch == victim) {
    send_to_char(ch, "You cannot raise yourself from the dead.\r\n");
    return;
  } 

  if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "But they aren't even dead!\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_SPIRIT);
  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ETHEREAL);

  GET_HIT(victim) = 1;
  GET_MOVE(victim) = 1;

  char_from_room(vict);
  char_to_room(vict, IN_ROOM(ch));
  look_at_room(IN_ROOM(vict), vict, 0);

  act("You raise $N from the dead!", FALSE, ch, 0, victim, TO_CHAR);
  act("$n raises you from the dead!", FALSE, ch, 0, victim, TO_VICT);
  act("$n raises $N from the dead!", FALSE, ch, 0, victim, TO_NOTVICT);

}

ASPELL(spell_resurrection)
{

  struct char_data *vict = victim;

  if (!victim)
    return;

  if (ch == victim) {
    send_to_char(ch, "You cannot raise yourself from the dead.\r\n");
    return;
  } 

  if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "But they aren't even dead!\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_SPIRIT);
  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ETHEREAL);

  GET_HIT(victim) = GET_MAX_HIT(ch);
  GET_MOVE(victim) = GET_MAX_MOVE(ch);

  char_from_room(vict);
  char_to_room(vict, IN_ROOM(ch));
  look_at_room(IN_ROOM(vict), vict, 0);

  act("You resurrect $N from the dead!", FALSE, ch, 0, victim, TO_CHAR);
  act("$n resurrects you from the dead!", FALSE, ch, 0, victim, TO_VICT);
  act("$n resurrects $N from the dead!", FALSE, ch, 0, victim, TO_NOTVICT);

}

ASPELL(spell_true_resurrection)
{

  struct char_data *vict = victim;

  if (!victim)
    return;

  if (ch == victim) {
    send_to_char(ch, "You cannot raise yourself from the dead.\r\n");
    return;
  } 

  if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "But they aren't even dead!\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_SPIRIT);
  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ETHEREAL);

  GET_HIT(victim) = GET_MAX_HIT(ch);
  GET_MOVE(victim) = GET_MAX_MOVE(ch);


  char_from_room(vict);
  char_to_room(vict, IN_ROOM(ch));
  look_at_room(IN_ROOM(vict), vict, 0);

  act("You resurrect $N from the dead!", FALSE, ch, 0, victim, TO_CHAR);
  act("$n resurrects you from the dead!", FALSE, ch, 0, victim, TO_VICT);
  act("$n resurrects $N from the dead!", FALSE, ch, 0, victim, TO_NOTVICT);

  gain_exp(victim, (mob_exp_by_level(GET_LEVEL(ch)) * 125 / 10));

}

ASPELL(spell_dispel_magic)
{

  struct affected_type *af = NULL;
  int violent = TRUE;
  char buf[100];
  int found = FALSE;

  if (is_player_grouped(ch, victim))
    violent = FALSE;

  for (af = victim->affected; af; af = af->next) {
    if (spell_info[af->type].violent != violent && af->level > 0) {
      if ((dice(1, 20) + MIN(10, level)) > (11 + af->level)) {
        sprintf(buf, "$N's %s affect has been dispelled by $n!", spell_info[af->type].name);
        act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
        if (victim != ch) {
          sprintf(buf, "Your %s affect has been dispelled by $n!", spell_info[af->type].name);
          act(buf, FALSE, ch, 0, victim, TO_VICT);
          sprintf(buf, "$N's %s affect has been dispelled!", spell_info[af->type].name);
          act(buf, FALSE, ch, 0, victim, TO_CHAR);
        }
        else {
          sprintf(buf, "Your %s affect has been dispelled!", spell_info[af->type].name);
          act(buf, FALSE, ch, 0, victim, TO_CHAR);
        }
        affect_remove(victim, af);
        found = TRUE;
      }
    }
  }

  if (!found) {
    if (victim != ch) {
      act("The bright light of @Ryour@n @Ydispel magic@n surrounds @r$N@n without effect.", FALSE, ch, 0, victim, TO_CHAR);
      act("The bright light of @R$n's@n @Ydispel magic@n surrounds @ryou@n without effect.", FALSE, ch, 0, victim, TO_VICT);
    }
    else {
      act("The bright light of @Ryour@n @Ydispel magic@n surrounds @ryou@n without effect.", FALSE, ch, 0, victim, TO_CHAR);   
    }
    act("The bright light of @R$n's@n @Ydispel magic@n surrounds @r$N@n without effect.", FALSE, ch, 0, victim, TO_NOTVICT);
  } 

}


/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) != LIQ_WATER) && (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) = LIQ_WATER;
	GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", false, ch, obj, 0, TO_CHAR);
      }
    }
  }
}


ASPELL(spell_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (GET_RECALL(victim) <= 1) // Simply sanity check to avoid a crash
    GET_RECALL(victim) = CONFIG_MORTAL_START;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL)) {
    OUTPUT_TO_CHAR("A magical shroud prevents recall!\r\n", victim);
    return;
  }

  OUTPUT_TO_CHAR("Recalling.\r\n", victim);
  act("$n recalls.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(GET_RECALL(victim)));
  act("$n appears in a swirling mist.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
  return;
}


ASPELL(spell_teleport)
{

  if (ch == NULL || victim == NULL)
    return;

  room_rnum to_room;
  int times_visited = ch->player_specials->rooms_visited[world[IN_ROOM(victim)].number];
  int chance = 0;

  if (times_visited <= 0)
      chance = 0;
  else if (times_visited <= 1)
      chance = 10;
  else if (times_visited <= 3)
      chance = 25;
  else if (times_visited <= 5)
      chance = 50;
  else if (times_visited <= 10)
      chance = 75;
  else if (times_visited <= 15)
      chance = 90;
  else
      chance = 95;

  if (dice(1, 100) <= chance)
    to_room = IN_ROOM(victim);
  else {
    do {
      to_room = rand_number(0, top_of_world);
    } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM) ||
           zone_table[world[to_room].zone].zone_status < 2);
  }

  if (chance == 0) {
    send_to_char(ch, "You cannot teleport somewhere you have never been.\r\n");
    return;
  }

  if (num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) <= 0 || 
      num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) > (100 *
       get_skill_value(ch, SKILL_SPELLCRAFT))) {
    send_to_char(ch, "You cannot master the weave well enough to teleport that distant.\r\n");
    return;
  }

  if (zone_table[world[to_room].zone].zone_status == 3) {
    send_to_char(ch, "You cannot teleport into dungeons.\r\n");
    return;
  }

  act("$n slowly fades out of existence and is gone.",
      false, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n slowly fades into existence.", false, ch, 0, 0, TO_ROOM);
  look_at_room(to_room, ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

    if (num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) <= 0 ||
      num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) > (100 * 
      get_skill_value(ch, SKILL_SPELLCRAFT))) {
    send_to_char(ch, "You cannot master the weave well enough to summon someone that distant.\r\n");
    return;
  }

  if (!CONFIG_PK_ALLOWED) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  false, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
			char *tmpdesc = NULL;
      send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      has_intro(victim, ch) ? GET_NAME(ch) : (tmpdesc = which_desc(ch)), world[IN_ROOM(ch)].name,
	      (ch->sex == SEX_MALE) ? "He" : "She");
			free(tmpdesc);
			tmpdesc = NULL;

      send_to_char(ch, "You failed because %s has summon protection on.\r\n", has_intro(ch, victim) ? GET_NAME(victim) : (tmpdesc = which_desc(victim)));
			free(tmpdesc);
      mudlog(BRF, ADMLVL_IMMORT, true, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_newsaves(find_savetype(SPELL_SUMMON), ch, victim, SPELL_SUMMON, calc_spell_dc(ch, SPELL_SUMMON)))) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  act("$n disappears suddenly.", true, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, IN_ROOM(ch));

  act("$n arrives suddenly.", true, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", false, ch, 0, victim, TO_VICT);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);

}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  /*
   * FIXME: this is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
  if (!obj) {
    send_to_char(ch, "You sense nothing.\r\n");
    return;
  }
  
  strlcpy(name, fname(obj->name), sizeof(name));
  j = level / 2;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;

    send_to_char(ch, "%c%s", UPPER(*i->short_description), (i->short_description)+1);

    if (i->carried_by)
      send_to_char(ch, " is being carried by %s.\r\n", PERS(i->carried_by, ch));
    else if (IN_ROOM(i) != NOWHERE)
      send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
    else if (i->in_obj)
      send_to_char(ch, " is in %s.\r\n", i->in_obj->short_description);
    else if (i->worn_by)
      send_to_char(ch, " is being worn by %s.\r\n", PERS(i->worn_by, ch));
    else
      send_to_char(ch, "'s location is uncertain.\r\n");

    j--;
  }

  if (j == level / 2)
    send_to_char(ch, "You sense nothing.\r\n");
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char(ch, "You like yourself even better!\r\n");
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char(ch, "You fail because SUMMON protection is on!\r\n");
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "Your victim resists!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You can't have any followers of your own!\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM))
    send_to_char(ch, "You fail.\r\n");
  else if (!IS_HUMANOID(victim))
    send_to_char(ch, "You can only charm humanoids.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "You fail - shouldn't be doing it anyway.\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
  else if (mag_newsaves(find_savetype(SPELL_CHARM), ch, victim, SPELL_CHARM, calc_spell_dc(ch, SPELL_CHARM)))
    send_to_char(ch, "Your victim resists!\r\n");
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);
    victim->master_id = GET_IDNUM(ch);

    af.type = SPELL_CHARM;
    af.duration = 24 * 2;
    if (GET_CHA(ch))
      af.duration *= GET_CHA(ch);
    if (GET_INT(victim))
      af.duration /= GET_INT(victim);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", false, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim))
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
}

ASPELL(spell_charm_animal)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char(ch, "You like yourself even better!\r\n");
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char(ch, "You fail because SUMMON protection is on!\r\n");
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "Your victim resists!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You can't have any followers of your own!\r\n");
  else if (!IS_ANIMAL(victim))
    send_to_char(ch, "You can only charm animals with this spell.\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM))
    send_to_char(ch, "You fail.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "You fail - shouldn't be doing it anyway.\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
  else if (mag_newsaves(find_savetype(SPELL_CHARM_ANIMAL), ch, victim, SPELL_CHARM_ANIMAL, calc_spell_dc(ch, SPELL_CHARM_ANIMAL)))
    send_to_char(ch, "Your victim resists!\r\n");
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);
    victim->master_id = GET_IDNUM(ch);

    af.type = SPELL_CHARM_ANIMAL;
    af.duration = 24 * 2;
    if (GET_CHA(ch))
      af.duration *= GET_CHA(ch);
    if (GET_INT(victim))
      af.duration /= GET_INT(victim);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", false, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim))
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
}



ASPELL(spell_identify)
{
  int i, found, spellnum = 0;
  size_t len;
  char buf[200];
  char keyw[100];

  if (obj) {
    char bitbuf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);
	
    sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
    send_to_char(ch, "You feel informed:\r\nObject '%s', Item type: %s\r\n", obj->short_description, bitbuf);

    if (GET_OBJ_PERM(obj)) {
      sprintbitarray(GET_OBJ_PERM(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
      send_to_char(ch, "Item will give you following abilities:  %s\r\n", bitbuf);
    }

    if (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL) {
      send_to_char(ch, "Item will imbue a crafted item with the following property: %s\r\n", apply_types[GET_OBJ_VAL(obj, 0)]);
      if (GET_OBJ_VAL(obj, 0) == APPLY_FEAT)
        send_to_char(ch, "Item will bestow or improve the following feat: '%s'\r\n", feat_list[GET_OBJ_VAL(obj, 1)].name);
      if (GET_OBJ_VAL(obj, 0) == APPLY_SKILL)
        send_to_char(ch, "Item will improve the following skill: '%s'\r\n", spell_info[GET_OBJ_VAL(obj, 1)].name);
    }

    sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, AF_ARRAY_MAX, bitbuf);
    send_to_char(ch, "Item is: %s\r\n", bitbuf);

    send_to_char(ch, "Weight: %d, Value: %d, Rent: %d, %s Level: %d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), 
                 (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL  || IS_POWER_ESSENCE(obj)) ? "Max" : "Min", GET_OBJ_LEVEL(obj));

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_CRYSTAL:

      sprintf(buf, "@wa crystal of@y %s@n max level@y %d@n", apply_types[GET_OBJ_VAL(obj, 0)], GET_OBJ_LEVEL(obj));
      obj->name = strdup(buf);
      obj->short_description = strdup(buf);
      sprintf(buf, "@wA crystal of@y %s@n max level@y %d@n lies here.", apply_types[GET_OBJ_VAL(obj, 0)], GET_OBJ_LEVEL(obj));
      obj->description = strdup(buf);
      break;

    case ITEM_SCROLL:

      sprintf(buf, "@wa scroll of @y%s@n", spell_info[GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)].name);
      obj->name = strdup(buf);
      obj->short_description = strdup(buf);
      sprintf(buf, "A scroll of %s lies here.", spell_info[GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)].name);
      obj->description = strdup(buf);
      

      len = i = 0;

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) >= 1) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3)));
        if (i >= 0)
          len += i;
      }

      send_to_char(ch, "This %s casts: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)], bitbuf);
      break;

    case ITEM_POTION:
      spellnum = GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1);

      sprintf(keyw, "%s", spell_info[spellnum].name);

      for (i = 0; i < strlen(keyw); i++)
        if (keyw[i] == ' ')
          keyw[i] = '-';

      sprintf(buf, "potion-%s", keyw);
      obj->name = strdup(buf);
      sprintf(buf, "@wa potion of @y%s@n", spell_info[spellnum].name);
      obj->name = strdup(buf);
      obj->short_description = strdup(buf);
      sprintf(buf, "A potion of %s lies here.", spell_info[spellnum].name);
      obj->description = strdup(buf);
      
      len = i = 0;

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) >= 1) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3)));
        if (i >= 0)
          len += i;
      }

      send_to_char(ch, "This %s casts: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)], bitbuf);
      break;

    case ITEM_WAND:
      spellnum = GET_OBJ_VAL(obj, VAL_WAND_SPELL);

      sprintf(keyw, "%s", spell_info[spellnum].name);

      for (i = 0; i < strlen(keyw); i++)
        if (keyw[i] == ' ')
          keyw[i] = '-';

      sprintf(buf, "wand-%s", keyw);
      obj->name = strdup(buf);
      sprintf(buf, "@wa wand of @y%s@n", spell_info[spellnum].name);
      obj->name = strdup(buf);
      obj->short_description = strdup(buf);
      sprintf(buf, "A wand of %s lies here.", spell_info[spellnum].name);
      obj->description = strdup(buf);
      

      send_to_char(ch, "This %s casts: %s\r\nIt has %d maximum charge%s and %d remaining.\r\n",
		item_types[(int) GET_OBJ_TYPE(obj)], skill_name(GET_OBJ_VAL(obj, VAL_WAND_SPELL)),
		GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES), GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES) == 1 ? "" : "s", GET_OBJ_VAL(obj, VAL_WAND_CHARGES));
      break;

    case ITEM_STAFF:
      spellnum = GET_OBJ_VAL(obj, VAL_WAND_SPELL);

      sprintf(keyw, "%s", spell_info[spellnum].name);

      for (i = 0; i < strlen(keyw); i++)
        if (keyw[i] == ' ')
          keyw[i] = '-';

      sprintf(buf, "staff-%s", keyw);
      obj->name = strdup(buf);
      sprintf(buf, "@wa staff of @y%s@n", spell_info[spellnum].name);
      obj->name = strdup(buf);
      obj->short_description = strdup(buf);
      sprintf(buf, "A staff of %s lies here.", spell_info[spellnum].name);
      obj->description = strdup(buf);
      

      send_to_char(ch, "This %s casts: %s\r\nIt has %d maximum charge%s and %d remaining.\r\n",
		item_types[(int) GET_OBJ_TYPE(obj)], skill_name(GET_OBJ_VAL(obj, VAL_WAND_SPELL)),
		GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES), GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES) == 1 ? "" : "s", GET_OBJ_VAL(obj, VAL_WAND_CHARGES));
      break;
    case ITEM_WEAPON:
      send_to_char(ch, "Damage Dice is '%dD%d' with a threat range of %d-20 and a critical multiplier of x%d.\r\n",
		GET_OBJ_VAL(obj, VAL_WEAPON_DAMDICE), GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE), 
                20 - weapon_list[GET_OBJ_VAL(obj, 0)].critRange, 2 + weapon_list[GET_OBJ_VAL(obj, 0)].critMult);
      break;
    case ITEM_ARMOR:
      send_to_char(ch, "AC-apply is %.1f\r\n", ((float)GET_OBJ_VAL(obj, VAL_ARMOR_APPLYAC)) / 10);
      break;
    }
    found = false;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char(ch, "Can affect you as :\r\n");
	  found = true;
	}
	sprinttype(obj->affected[i].location, apply_types, bitbuf, sizeof(bitbuf));
        switch (obj->affected[i].location) {
        case APPLY_FEAT:
          snprintf(buf2, sizeof(buf2), " (%s)", feat_list[obj->affected[i].specific].name);
          break;
        case APPLY_SKILL:
          snprintf(buf2, sizeof(buf2), " (%s)", spell_info[obj->affected[i].specific].name);
          break;
        default:
          buf2[0] = 0;
          break;
        }
	send_to_char(ch, "   Affects: %s%s By %d\r\n", bitbuf, buf2,
                     obj->affected[i].modifier);
      }
    }
  }
  if (GET_OBJ_VNUM(obj) == 30188) {
    send_to_char(ch, "It will grant %d more wishes.\r\n", GET_OBJ_VAL(obj, 0));
  }
}



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
      return;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].location == APPLY_NONE) {
      obj->affected[i].location = APPLY_ACCURACY;
      obj->affected[i].modifier = 1 + (level >= 18);
      break;
    }
  }

  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].location == APPLY_NONE) {
      obj->affected[i].location = APPLY_DAMAGE;
      obj->affected[i].modifier = 1 + (level >= 20);
      break;
    }
  }

  if (IS_GOOD(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    act("$p glows blue.", false, ch, obj, 0, TO_CHAR);
  } else if (IS_EVIL(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
    act("$p glows red.", false, ch, obj, 0, TO_CHAR);
  } else
    act("$p glows yellow.", false, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char(ch, "You can sense poison in your blood.\r\n");
      else
        send_to_char(ch, "You feel healthy.\r\n");
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", false, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", false, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, VAL_FOOD_POISON))
	act("You sense that $p has been contaminated.",false,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", false, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char(ch, "You sense that it should not be consumed.\r\n");
    }
  }
}

ASPELL(spell_portal)
{
  room_rnum to_room;
  int times_visited = ch->player_specials->rooms_visited[world[IN_ROOM(victim)].number];
  int chance = 0;
  struct obj_data *portal, *tportal;

  if (ch == NULL || victim == NULL)
    return;

  do {
    to_room = rand_number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM) ||
           zone_table[world[to_room].zone].zone_status < 2);

  if (times_visited <= 0)
      chance = 0;
  else
      chance = 1;


  if (chance)
    to_room = IN_ROOM(victim);

    if (num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) <= 0 ||
      num_rooms_between(IN_ROOM(ch), IN_ROOM(victim)) > (100 * 
      + get_skill_value(ch, SKILL_SPELLCRAFT))) {
    send_to_char(ch, "You cannot master the weave well enough to create a portal that distant.\r\n");
    return;
  }

  if (zone_table[world[to_room].zone].zone_status == 3) {
    send_to_char(ch, "You cannot portal into dungeons.\r\n");
    return;
  }

  /* create the portal */
  portal = read_object(portal_object, virtual);
  GET_OBJ_VAL(portal, VAL_PORTAL_DEST) = GET_ROOM_VNUM(to_room);
  GET_OBJ_VAL(portal, VAL_PORTAL_HEALTH) = 100;
  GET_OBJ_VAL(portal, VAL_PORTAL_MAXHEALTH) = 100;
  GET_OBJ_TIMER(portal) = 2000;
  add_unique_id(portal);
  obj_to_room(portal, IN_ROOM(ch));
  act("$n opens a portal in thin air.",
       true, ch, 0, 0, TO_ROOM);
  act("You open a portal out of thin air.",
       true, ch, 0, 0, TO_CHAR);
  /* create the portal at the other end */
  tportal = read_object(portal_object, virtual);
  GET_OBJ_VAL(tportal, VAL_PORTAL_DEST) = GET_ROOM_VNUM(IN_ROOM(ch));
  GET_OBJ_VAL(tportal, VAL_PORTAL_HEALTH) = 100;
  GET_OBJ_VAL(tportal, VAL_PORTAL_MAXHEALTH) = 100;
  GET_OBJ_TIMER(tportal) = 2000;
  add_unique_id(portal);
  obj_to_room(tportal, to_room);
  act("A shimmering portal appears out of thin air.",
       true, victim, 0, 0, TO_ROOM);
  act("A shimmering portal opens here for you.",
       true, victim, 0, 0, TO_CHAR);
}


ASPELL(art_abundant_step)
{
  int steps, i=0, rep, max;
  room_rnum r, nextroom;
  char buf[MAX_INPUT_LENGTH];
  const char *p;

  steps = 0;
  r = IN_ROOM(ch);
  p = arg;
  max = 10 + GET_CLASS_RANKS(ch, CLASS_MONK) / 2;

  while (p && *p && !isdigit(*p) && !isalpha(*p)) p++;

  if (!p || !*p) {
    send_to_char(ch, "You must give directions from your current location. Examples:\r\n"
                 "  w w nw n e\r\n"
                 "  2w nw n e\r\n");
    return;
  }

  while (*p) {
    while (*p && !isdigit(*p) && !isalpha(*p)) p++;
    if (isdigit(*p)) {
      rep = atoi(p);
      while (isdigit(*p)) p++;
    } else
      rep = 1;
    if (isalpha(*p)) {
      for (i = 0; isalpha(*p); i++, p++) buf[i] = LOWER(*p);
      buf[i] = 0;
      for (i = 1; complete_cmd_info[i].command_pointer == do_move && strcmp(complete_cmd_info[i].sort_as, buf); i++);
      if (complete_cmd_info[i].command_pointer == do_move) {
        i = complete_cmd_info[i].subcmd - 1;
      } else
        i = -1;
    }
    if (i > -1)
      while (rep--) {
        if (++steps > max)
          break;
        nextroom = W_EXIT(r, i)->to_room;
        if (nextroom == NOWHERE)
          break;
        r = nextroom;
      }
    if (steps > max)
      break;
  }

  if (zone_table[world[r].zone].zone_status == 3) {
    send_to_char(ch, "You cannot teleport into dungeons.\r\n");
    return;
  }

  send_to_char(ch, "Your will bends reality as you travel through the ethereal plane.\r\n");
  act("$n is suddenly absent.", true, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, r);

  act("$n is suddenly present.", true, ch, 0, 0, TO_ROOM);

  look_at_room(IN_ROOM(ch), ch, 0);

  return;
}


int roll_skill(const struct char_data *ch, int snum)
{
  return skill_roll((struct char_data *)ch, snum);
}

int roll_resisted(const struct char_data *actor, int sact, const struct char_data *resistor, int sres)
{
  return roll_skill(actor, sact) >= roll_skill(resistor, sres);
}


ASPELL(spell_clan_recall)
{
  struct clan_type *cptr;
  if (victim == NULL)
    return;
                                                                                   
  if (IS_NPC(victim))
    if (!(victim->master == ch))
      return;
                                                                                   
  for (cptr = clan_info; cptr->number != GET_CLAN(victim); cptr = cptr->next);
                                                                                   
  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
                                                                                   
  if(!IS_NPC(victim)) {
    if(GET_CLAN(victim) > 0)
      char_to_room(victim, real_room(cptr->clan_recall));
    else
      char_to_room(victim, r_mortal_start_room);
  } 

  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
/* uncomment if you use dg scripts 
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
*/
  return;
}

ASPELL(spell_wish)
{
  if (!ch || !victim) {
    send_to_char(ch, "You need to specify a target.\r\n");
    return;
  }

  if (GET_GOLD(ch) < 100000) {
    send_to_char(ch, "You need 100,000 gold on hand to give a wish or miracle.\r\n");
    return;
  }

  if (GET_EXP(ch) < 500000) {
    send_to_char(ch, "You need at least 500,000 experience points to cast a wish.\r\n");
    return;
  }

  GET_GOLD(ch) -= 100000;
  gain_exp(ch, -500000);
  GET_WISHES(victim)++;

  char buf[200];

  if (ch != victim) {
    sprintf(buf, "You grant $N a wish for a cost of 100,000 gold coins and 500,000 experience points.");
    act(buf, false, ch, 0, victim, TO_CHAR);
    act("$n grants you a wish!  Use the wish command to spend it.", false, ch, 0, victim, TO_VICT);
  } else {
    send_to_char(ch, "You gain a wish for a cost of 100,000 gold coins and 500,000 experience points.\r\nUse the wish command to use your wish.\r\n");
  }
}

ASPELL(spell_wish_ring)
{
  if (!ch || !victim) {
    send_to_char(ch, "You need to specify a target.\r\n");
    return;
  }

  GET_WISHES(victim)++;

  char buf[200];

  if (ch != victim) {
    sprintf(buf, "You grant $N a wish for a cost of 100,000 gold coins and 500,000 experience points.");
    act(buf, false, ch, 0, victim, TO_CHAR);
    act("$n grants you a wish!  Use the wish command to spend it.", false, ch, 0, victim, TO_VICT);
  } else {
    send_to_char(ch, "You gain a wish!  Use the wish command to use your wish.\r\n");
  }
}

ACMD(do_wish)
{

  if (GET_WISHES(ch) < 1) {
    send_to_char(ch, "You do not have any wishes to spend.\r\n");
    return;
  }

  char result[100];
  struct char_data *victim = ch;

  one_argument(argument, result);

  if (!*result) {
    send_to_char(ch, "Please wish for one of the following: strength, intelligence, wisdom, dexterity, constitution, charisma\r\n");
    send_to_char(ch, "You currrently have %d wishes to spend.\r\n", GET_WISHES(ch));
    return;
  }
  
  if (!strcmp(result, "strength")) {
	if (GET_WISH_STR(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.str++;
	GET_WISH_STR(victim)++;
	send_to_char(ch, "Your wish for greater strength was granted.\r\n");
	send_to_char(victim, "A wish was made for greater strength and feel your muscles permanently surge and grow.\r\n");
  }
  else if (!strcmp(result, "dexterity")) {
	if (GET_WISH_DEX(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.dex++;
	GET_WISH_DEX(victim)++;
	send_to_char(ch, "Your wish for greater dexterity was granted.\r\n");
	send_to_char(victim, "A wish was made for greater dexterity and feel your reflexes permanently surge and grow.\r\n");
  }  
  else if (!strcmp(result, "constitution")) {
	if (GET_WISH_CON(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.con++;
	GET_WISH_CON(victim)++;
	send_to_char(ch, "Your wish for greater constitution was granted.\r\n");
	send_to_char(victim, "A wish was made for greater constitution and feel your hardiness permanently surge and grow.\r\n");
  }
  else if (!strcmp(result, "intelligence")) {
	if (GET_WISH_INT(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.intel++;
	GET_WISH_INT(victim)++;
	send_to_char(ch, "Your wish for greater intelligence was granted.\r\n");
	send_to_char(victim, "A wish was made for greater intelligence and feel your intellect permanently surge and grow.\r\n");
  }  
  else if (!strcmp(result, "wisdom")) {
	if (GET_WISH_WIS(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.wis++;
	GET_WISH_WIS(victim)++;
	send_to_char(ch, "Your wish for greater wisdom was granted.\r\n");
	send_to_char(victim, "A wish was made for greater wisdom and feel your intuition permanently surge and grow.\r\n");
  }
  else if (!strcmp(result, "charisma")) {
	if (GET_WISH_CHA(victim) >= 5) {
	  send_to_char(ch, "The target has already raised this as much as they can by using wishes.\r\n");
	  return;
	}
	victim->real_abils.cha++;
	GET_WISH_CHA(victim)++;
	send_to_char(ch, "Your wish for greater charisma was granted.\r\n");
	send_to_char(victim, "A wish was made for greater charisma and feel yourself become more personable permanently.\r\n");
  } else {
    send_to_char(ch, "Please wish for one of the following: strength, intelligence, wisdom, dexterity, constitution, charisma\r\n");
    send_to_char(ch, "You currrently have %d wishes to spend.\r\n", GET_WISHES(ch));
    return;
  }
  affect_total(ch);  
  GET_WISHES(ch)--;
  save_char(ch);
}


