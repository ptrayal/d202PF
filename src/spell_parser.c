/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "deities.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "feats.h"

/* extern globals */
extern int spell_lvlmax_table[NUM_CLASSES][21][10];

/* local globals */
struct spell_info_type spell_info[SKILL_TABLE_SIZE];

/* local functions */
void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);
int mag_manacost(struct char_data *ch, int spellnum);
void mag_nextstrike(int level, struct char_data *caster, int spellnum);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);
int sr_check(struct char_data *caster, struct char_data *victim);

int check_active_turn(struct char_data *ch);
ACMD(do_memorize);
int is_innate(struct char_data *ch, int spellnum);
int is_memorized(struct char_data *ch, int spellnum, int single);
int is_innate_ready(struct char_data *ch, int spellnum);
void do_mem_spell(struct char_data *ch, char *arg, int type, int message);
void add_innate_timer(struct char_data *ch, int spellnum);
int get_skill_mod(struct char_data *ch, int snum);
int spell_in_domain(struct char_data *ch, int spellnum);
int is_player_grouped(struct char_data *target, struct char_data *group);

int findslotnum(struct char_data *ch, int spellvl);

/*
 * this arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
  const char *org;
  const char *news;
};


struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

int mag_manacost(struct char_data *ch, int spellnum)
{
  int whichclass, i, min, tval;
  if (CONFIG_ALLOW_MULTICLASS) {
    /* find the cheapest class to cast it */
    min = MAX(SINFO.mana_max - (SINFO.mana_change *
	      (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS_RANKS(ch, GET_CLASS(ch))])),
	     SINFO.mana_min);
    whichclass = GET_CLASS(ch);
    for (i = 0; i < NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i) == 0)
        continue;
      tval = MAX(SINFO.mana_max - (SINFO.mana_change *
	         (GET_CLASS_RANKS(ch, i) - SINFO.min_level[i])),
	         SINFO.mana_min);
      if (tval < min) {
        min = tval;
        whichclass = i;
      }
    }
    return min;
  } else {
  return MAX(SINFO.mana_max - (SINFO.mana_change *
		    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min);
  }
}


int mag_kicost(struct char_data *ch, int spellnum)
{
  int whichclass, i, min, tval;
  if (CONFIG_ALLOW_MULTICLASS) {
    /* find the cheapest class to cast it */
    min = MAX(SINFO.ki_max - (SINFO.ki_change *
              (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS_RANKS(ch, GET_CLASS(ch))])),
             SINFO.ki_min);
    whichclass = GET_CLASS(ch);
    for (i = 0; i < NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i) == 0)
        continue;
      tval = MAX(SINFO.ki_max - (SINFO.ki_change *
                 (GET_CLASS_RANKS(ch, i) - SINFO.min_level[i])),
                 SINFO.ki_min);
      if (tval < min) {
        min = tval;
        whichclass = i;
      }
    }
    return min;
  } else {
  return MAX(SINFO.ki_max - (SINFO.ki_change *
                    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
             SINFO.ki_min);
  }
}


void mag_nextstrike(int level, struct char_data *caster, int spellnum)
{
  if (!caster)
    return;
  if (caster->actq) {
    send_to_char(caster, "You can't perform more than one special attack at a time!");
    return;
  }
  CREATE(caster->actq, struct queued_act, 1);
  caster->actq->level = level;
  caster->actq->spellnum = spellnum;
}


void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
	            struct obj_data *tobj)
{
  char lbuf[256], buf[256], buf1[256], buf2[256];	/* FIXME */
  const char *format = "";

  struct char_data *i;
  int j, ofs = 0;

  *buf = '\0';
  strlcpy(lbuf, skill_name(spellnum), sizeof(lbuf));

  while (lbuf[ofs]) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].news);	/* strcat: BAD */
	ofs += strlen(syls[j].org);
        break;
      }
    }
    /* i.e., we didn't find a match in syls[] */
    if (!*syls[j].org) {
      log("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch) && 
      (!IS_NPC(ch) && (!FIGHTING(ch) || ch->active_turn != 1))) {
    if (tch == ch)
      format = "$n closes $s eyes and utters the words, '%s'.";
    else
      format = "$n stares at $N and utters the words, '%s'.";
  } else if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
    format = "$n stares at $p and utters the words, '%s'.";
  else if (!IS_NPC(ch) && (!FIGHTING(ch) || ch->active_turn != 1))
    format = "$n utters the words, '%s'.";

  snprintf(buf1, sizeof(buf1), format, skill_name(spellnum));
  snprintf(buf2, sizeof(buf2), format, buf);

  if (!(IS_NPC(ch) && (!FIGHTING(ch) || ch->active_turn != 1))) {
    for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
      if (i == ch || i == tch || !i->desc || !AWAKE(i))
        continue;
      /* This should really check spell type vs. target ranks */
      if (GET_CLASS_RANKS(i, GET_CLASS(ch)) || GET_ADMLEVEL(i) >= ADMLVL_IMMORT)
        perform_act(buf1, ch, tobj, tch, i);
      else
        perform_act(buf2, ch, tobj, tch, i);
    }
  }

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
    snprintf(buf1, sizeof(buf1), "$n stares at you and utters the words, '%s'.",
	    GET_CLASS_RANKS(tch, GET_CLASS(ch)) ? skill_name(spellnum) : buf);
    act(buf1, false, ch, NULL, tch, TO_VICT);
  }
}

/*
 * this function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and < SKILL_TABLE_SIZE
 */
const char *skill_name(int num)
{
  if (num > 0 && num < SKILL_TABLE_SIZE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

int find_skill_num(char *name, int sktype)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[256], first2[256], tempbuf[256];

  for (skindex = 1; skindex < SKILL_TABLE_SIZE; skindex++) {
    if (is_abbrev(name, spell_info[skindex].name) && (spell_info[skindex].skilltype & sktype)) {
      return (skindex);
    }

    ok = true;
    strlcpy(tempbuf, spell_info[skindex].name, sizeof(tempbuf));        /* strlcpy: OK */
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = false;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2 && (spell_info[skindex].skilltype & sktype)) {
      return (skindex);
    }
  }

  return (-1);
}


/*
 * this function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * this is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(struct char_data *caster, struct char_data *cvict,
	     struct obj_data *ovict, int spellnum, int level, int casttype, const char *arg)
{

  if (casttype != CAST_POTION && casttype != CAST_SCROLL && casttype != CAST_WAND && casttype != CAST_STAFF) {
    if (IS_NPC(caster))
      level = GET_LEVEL(caster);
    else
      level = MAX(1, GET_SPELLCASTER_LEVEL(caster));
  }


  if (spellnum < 0 || spellnum > SKILL_TABLE_SIZE) {
    send_to_char(caster, "Spell number of out range.  Spell Failed.  Contact a staff member.\r\n");
    return (0);
  }

  if (spellnum == 0)
    return (0);

  if (!cast_wtrigger(caster, cvict, ovict, spellnum) || !cast_otrigger(caster, ovict, spellnum) ||!cast_mtrigger(caster, cvict, spellnum)) {
    send_to_char(caster, "Spell-casting trigger failed.  Spell Failed.  Please contact an imm.\r\n");
    return 0;
  }

  if (IS_SET(SINFO.skilltype, SKTYPE_SPELL) &&
      ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && GET_ADMLEVEL(caster) < ADMLVL_IMPL) {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", false, caster, 0, 0, TO_ROOM);
    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) && GET_ADMLEVEL(caster) < ADMLVL_IMPL &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char(caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.", false, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if (IS_SET(SINFO.routines, MAG_NEXTSTRIKE) && casttype != CAST_STRIKE) {
    mag_nextstrike(level, caster, spellnum);
    return 1;
  }

  if (SINFO.violent == TRUE) {
    if (!sr_check(caster, cvict)) {
      act("@W$N resists your spell.@n", FALSE, caster, 0, cvict, TO_CHAR);
      act("@WYou resist $n's spell.@n", FALSE, caster, 0, cvict, TO_VICT);
      act("@W$N resists $n's spell.@n", FALSE, caster, 0, cvict, TO_NOTVICT);
      return (1);
    }
  }

  if (IS_SET(SINFO.routines, MAG_LOOP))
    mag_loop(level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(level, caster, cvict, spellnum) == -1)
      return (-1);	/* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(level, caster, ovict, spellnum);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, arg);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
    case SPELL_CLAN_RECALL:   MANUAL_SPELL(spell_clan_recall); break;
    case SPELL_CHARM:		MANUAL_SPELL(spell_charm); break;
    case SPELL_CHARM_ANIMAL:	MANUAL_SPELL(spell_charm_animal); break;
    case SPELL_CREATE_WATER:	MANUAL_SPELL(spell_create_water); break;
    case SPELL_DETECT_POISON:	MANUAL_SPELL(spell_detect_poison); break;
    case SPELL_ENCHANT_WEAPON:  MANUAL_SPELL(spell_enchant_weapon); break;
    case SPELL_IDENTIFY:	MANUAL_SPELL(spell_identify); break;
    case SPELL_LOCATE_OBJECT:   MANUAL_SPELL(spell_locate_object); break;
    case SPELL_SUMMON:		MANUAL_SPELL(spell_summon); break;
    case SPELL_WORD_OF_RECALL:  MANUAL_SPELL(spell_recall); break;
    case SPELL_TELEPORT:	MANUAL_SPELL(spell_teleport); break;
    case SPELL_PORTAL:		MANUAL_SPELL(spell_portal); break;
    case SPELL_RAISE_DEAD:	MANUAL_SPELL(spell_raise_dead); break;
    case SPELL_REINCARNATE:	MANUAL_SPELL(spell_raise_dead); break;
    case SPELL_RESURRECTION:	MANUAL_SPELL(spell_resurrection); break;
    case SPELL_TRUE_RESURRECTION: MANUAL_SPELL(spell_true_resurrection); break;
    case ART_ABUNDANT_STEP:	MANUAL_SPELL(art_abundant_step); break;
    case SPELL_DISPEL_MAGIC:    MANUAL_SPELL(spell_dispel_magic); break;
    case SPELL_WISH: MANUAL_SPELL(spell_wish); break;
    case SPELL_WISH_RING: MANUAL_SPELL(spell_wish_ring); break;
    case SPELL_MIRACLE: MANUAL_SPELL(spell_wish); break;
    }

  if (IS_SET(SINFO.routines, MAG_AFFECTSV))
    mag_affectsv(level, caster, cvict, spellnum);

  if (FIGHTING(caster))
    GET_HATE(caster) += spell_info[spellnum].hate;

  return (1);
}

/*
 * mag_objectmagic: this is the entry-point for all magic items.  this should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * for reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
		          char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    if (!IS_SPELLCASTER(ch) && skill_roll(ch, SKILL_USE_MAGIC_DEVICE) < (20 + GET_OBJ_LEVEL(obj))) {
      act("You fail to use $p, though a charge is expended.", true, ch, obj, 0, TO_CHAR);
      act("$n fails to use $p.", false, ch, obj, NULL, TO_ROOM);
      GET_OBJ_VAL(obj, VAL_STAFF_CHARGES)--;
      return;
    }

    act("You tap $p three times on the ground.", false, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, false, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", false, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, VAL_STAFF_CHARGES) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", false, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, VAL_STAFF_CHARGES)--;
      SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, VAL_STAFF_LEVEL) ? GET_OBJ_VAL(obj, VAL_STAFF_LEVEL) : DEFAULT_STAFF_LVL;

      /*
       * Problem : Area/mass spells on staves can cause crashes.
       * Solution: Remove the special nature of area/mass spells on staves.
       * Problem : People like that behavior.
       * Solution: We special case the area/mass spells here.
       */
      if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, VAL_STAFF_SPELL), MAG_MASSES | MAG_AREAS)) {
          GET_SPELLCASTER_LEVEL(ch) = k;
          for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)  {
            if (spell_info[GET_OBJ_VAL(obj, VAL_STAFF_SPELL)].violent == TRUE) {
              if (tch == FIGHTING(ch) || is_player_grouped(FIGHTING(tch), ch)) {
                call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), k, CAST_STAFF, NULL);
              }
            } else {
              call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), k, CAST_STAFF, NULL);
            }
          }
      } else {
	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
	  next_tch = tch->next_in_room;
          if (spell_info[GET_OBJ_VAL(obj, VAL_STAFF_SPELL)].violent == TRUE && is_player_grouped(tch, ch))
            continue;
          call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), k, CAST_STAFF, NULL);
	}
      }
    }
    break;
  case ITEM_WAND:
    if (!IS_SPELLCASTER(ch) && skill_roll(ch, SKILL_USE_MAGIC_DEVICE) < (15 + GET_OBJ_LEVEL(obj))) {
      act("You fail to use $p, though a charge is expended.", true, ch, obj, 0, TO_CHAR);
      act("$n fails to use $p.", false, ch, obj, NULL, TO_ROOM);
      GET_OBJ_VAL(obj, VAL_WAND_CHARGES)--;
      return;
    }
    if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
	act("You point $p at yourself.", false, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", false, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", false, ch, obj, tch, TO_CHAR);
	if (obj->action_description)
	  act(obj->action_description, false, ch, obj, tch, TO_ROOM);
	else
	  act("$n points $p at $N.", true, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", false, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, false, ch, obj, tobj, TO_ROOM);
      else
	act("$n points $p at $P.", true, ch, obj, tobj, TO_ROOM);
    } else if (IS_SET(spell_info[GET_OBJ_VAL(obj, VAL_WAND_SPELL)].routines, MAG_AREAS | MAG_MASSES)) {
      /* Wands with area spells don't need to be pointed. */
      act("You point $p outward.", false, ch, obj, NULL, TO_CHAR);
      act("$n points $p outward.", true, ch, obj, NULL, TO_ROOM);
    } else {
      act("At what should $p be pointed?", false, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, VAL_WAND_CHARGES) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", false, ch, obj, 0, TO_ROOM);
      return;
    }
    GET_OBJ_VAL(obj, VAL_WAND_CHARGES)--;
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);

    GET_SPELLCASTER_LEVEL(ch) = GET_OBJ_VAL(obj, VAL_WAND_LEVEL) ? GET_OBJ_VAL(obj, VAL_WAND_LEVEL) : DEFAULT_WAND_LVL;

    if (GET_OBJ_VAL(obj, VAL_WAND_LEVEL))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, VAL_WAND_SPELL),
		 GET_OBJ_VAL(obj, VAL_WAND_LEVEL), CAST_WAND, NULL);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, VAL_WAND_SPELL),
		 DEFAULT_WAND_LVL, CAST_WAND, NULL);
    break;
  case ITEM_SCROLL:
    if (!IS_SPELLCASTER(ch) && skill_roll(ch, SKILL_USE_MAGIC_DEVICE) < (10 + GET_OBJ_LEVEL(obj))) {
      act("You fail to recite $p which promptly dissolves.", true, ch, obj, 0, TO_CHAR);
      act("$n fails to recite $p.", false, ch, obj, NULL, TO_ROOM);

      if (obj != NULL)
        extract_obj(obj);
      return;
    }
    if (*arg) {
      if (!k) {
	act("There is nothing to here to affect with $p.", false,
	    ch, obj, NULL, TO_CHAR);
	return;
      }
    } else
      tch = ch;

    act("You recite $p which dissolves.", true, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, false, ch, obj, NULL, TO_ROOM);
    else
      act("$n recites $p.", false, ch, obj, NULL, TO_ROOM);

    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
    GET_SPELLCASTER_LEVEL(ch) = GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL), CAST_SCROLL, NULL) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    tch = ch;

  if (!consume_otrigger(obj, ch, OCMD_QUAFF))  /* check trigger */
    return;

    act("You quaff $p.", false, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, false, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", true, ch, obj, NULL, TO_ROOM);

    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
    GET_SPELLCASTER_LEVEL(ch) = GET_OBJ_VAL(obj, VAL_POTION_LEVEL);
    for (i = 1; i <= 3; i++) {
      if (GET_OBJ_VAL(obj, i) < 1 || GET_OBJ_VAL(obj, i) > SKILL_TABLE_SIZE)
        continue;
      if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
          GET_OBJ_VAL(obj, VAL_POTION_LEVEL), CAST_POTION, NULL) <= 0)
	break;
    }

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type %d in mag_objectmagic.",
	GET_OBJ_TYPE(obj));
    break;
  }
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
int cast_spell(struct char_data *ch, struct char_data *tch,
	           struct obj_data *tobj, int spellnum, const char *arg)
{
  int lvl = GET_LEVEL(ch);
  int concentration = 0;

  if (AFF_FLAGGED(ch, AFF_STUNNED)) {
    send_to_char(ch, "You cannot cast spells while stunned!\r\n");
    return (0);
  }

  if (affected_by_spell(ch, SPELL_SILENCE) || AFF_FLAGGED(ch, AFF_SILENCE)) {
  	send_to_char(ch, "You try to cast a spell but have not the ability to speak!\r\n");
  	return (0);
  }

  if (IS_NPC(ch) && dice(1, 20) + GET_LEVEL(ch) < GET_DAMAGE_TAKEN(ch)) {
    act("$n loses $s concentration and could not complete $s casting.", TRUE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  if (IS_NPC(ch) && !GET_SPELL_SLOT(ch, spell_info[spellnum].spell_level)) {
  	send_to_char(ch, "You have no remaining spell slots for that level.\r\n");
  	return (0);
  }

  if (IS_NPC(ch)) {
    SET_SPELL_SLOT(ch, spell_info[spellnum].spell_level, GET_SPELL_SLOT(ch, spell_info[spellnum].spell_level) - 1);
  }

  concentration = GET_SKILL(ch, SKILL_SPELLCRAFT);
  if (HAS_FEAT(ch, FEAT_COMBAT_CASTING) > 0)
    concentration += 4;

  if (!IS_NPC(ch) && dice(1, 20) + concentration < GET_DAMAGE_TAKEN(ch) &&
      GET_MEM_TYPE(ch) != MEM_TYPE_ART && GET_MEM_TYPE(ch) != MEM_TYPE_MUSIC) {
    send_to_char(ch, "You lose your concentration and are unable to complete casting your spell.\r\n");
    act("$n loses his concentration and could not complete $s casting.", TRUE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  if (spellnum < 0 || spellnum >= SKILL_TABLE_SIZE) {
    log("SYSERR: cast_spell trying to call out of range spellnum %d/%d.", spellnum,
	SKILL_TABLE_SIZE);
	  send_to_char(ch, "SYSERR: cast_spell trying to call out of range spellnum %d/%d.", spellnum,
	SKILL_TABLE_SIZE);
    return (0);
  }

  if (!IS_SET(SINFO.skilltype, SKTYPE_SPELL | SKTYPE_ART)) {
    log("SYSERR: cast_spell trying to call nonspell spellnum %d/%d.", spellnum,
	SKILL_TABLE_SIZE);
    send_to_char(ch, "SYSERR: cast_spell trying to call nonspell spellnum %d/%d.", spellnum,
	SKILL_TABLE_SIZE);
    return (0);
  }

  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char(ch, "You dream about great magical powers.\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "You cannot concentrate while resting.\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You can't do this sitting!\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
      break;
    default:
      send_to_char(ch, "You can't do much of anything like this!\r\n");
      break;
    }
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char(ch, "You are afraid you might hurt your master!\r\n");
    return (0);
  }
  if ((tch != ch) && GET_ADMLEVEL(ch) == 0 && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char(ch, "You can only cast this spell upon yourself!\r\n");
    return (0);
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char(ch, "You cannot cast this spell upon yourself!\r\n");
    return (0);
  }
  if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
    if (!ch->master)
      SET_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    else {
      send_to_char(ch, "You can't cast this spell if you're not in a group!\r\n");
      return (0);
    }
  }

  if (IS_SET(SINFO.skilltype, SKTYPE_SPELL)) {
    lvl = GET_SPELLCASTER_LEVEL(ch);
  } else if (IS_SET(SINFO.skilltype, SKTYPE_ART))
    lvl = GET_CLASS_RANKS(ch, CLASS_MONK) / 2;
  else {
    lvl = GET_SPELLCASTER_LEVEL(ch);
  }


  send_to_char(ch, "%s", CONFIG_OK);
  if (IS_SET(SINFO.skilltype, SKTYPE_SPELL)) {
    say_spell(ch, spellnum, tch, tobj);
  }

  if (IS_SET(SINFO.routines, MAG_ACTION_FULL) && FIGHTING(ch)) {
    ch->spell_cast = TRUE;
  }

  return (call_magic(ch, tch, tobj, spellnum, lvl, CAST_SPELL, arg));
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  struct follow_type *f;
  char *s, *t, buffer[25];
  /* char export[256];
  int mana, percent; */
  int ki = 0, lvl = 0;
  sh_int n = 0;
  byte spellKnown = FALSE;
  int spellnum, spellnum_bak, i, target = 0, innate = false;
  char argument2[MAX_STRING_LENGTH];
  char arg[100], dBuf[MAX_STRING_LENGTH];
  int classnum = CLASS_WIZARD;
  int attrib = GET_WIS(ch);
  int count = 0;

  if (FIGHTING(ch) && !check_active_turn(ch)) {
    send_to_char(ch, "It is not your turn in combat yet.\r\n");
    return;
  }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;

  tch = NULL;

  sprintf(argument2, "%s", argument);
  one_argument(argument, arg);
  for( i = 0; i < strlen(argument2); i++)
        dBuf[i] = argument2[i+1];

  if (subcmd == SCMD_PRAY) {
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    classnum = CLASS_CLERIC;
  }
  else if (subcmd == SCMD_INNATE) {
    GET_MEM_TYPE(ch) = MEM_TYPE_INNATE;
    classnum = CLASS_UNDEFINED;
  }
  else if (subcmd == SCMD_INTONE) {
    GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
    classnum = CLASS_PALADIN;
  }
  else if (subcmd == SCMD_CHANT) {
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    classnum = CLASS_DRUID;
  }
  else if (subcmd == SCMD_COMMUNE) {
    GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
    classnum = CLASS_RANGER;
  }
  else if (subcmd == SCMD_SING) {
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    classnum = CLASS_BARD;
    attrib = GET_CHA(ch);
  }
  else if (subcmd == SCMD_MUSIC) {
    GET_MEM_TYPE(ch) = MEM_TYPE_MUSIC;
    classnum = CLASS_BARD;
    attrib = GET_CHA(ch);
  }
  else if (subcmd == SCMD_EVOKE) {
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    classnum = CLASS_SORCERER;
    attrib = GET_CHA(ch);
  }
  else if (subcmd == SCMD_IMBUE) {
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    classnum = CLASS_FAVORED_SOUL;
    attrib = GET_CHA(ch);
  }
  else if (subcmd == SCMD_DUSK) {
    GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
    classnum = CLASS_ASSASSIN;
    attrib = GET_INT(ch);
  }
  else if (subcmd == SCMD_ART) {
    GET_MEM_TYPE(ch) = MEM_TYPE_ART;
    classnum = CLASS_MONK;
    attrib = 999;
  }
  else {
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    classnum = CLASS_WIZARD;
    attrib = GET_INT(ch);
  }

  if (GET_ADMLEVEL(ch) > 0)
    lvl = GET_LEVEL(ch);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_MAGE)
    lvl = GET_CASTER_LEVEL(ch, CLASS_WIZARD);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC)
    lvl = GET_CASTER_LEVEL(ch, CLASS_CLERIC);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID)
    lvl = GET_CASTER_LEVEL(ch, CLASS_DRUID);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN)
    lvl = GET_CASTER_LEVEL(ch, CLASS_PALADIN);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER)
    lvl = GET_CASTER_LEVEL(ch, CLASS_RANGER);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD)
    lvl = GET_CASTER_LEVEL(ch, CLASS_BARD);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_SORCERER)
    lvl = GET_CASTER_LEVEL(ch, CLASS_SORCERER);
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_FAVORED_SOUL)
    lvl = GET_CASTER_LEVEL(ch, CLASS_FAVORED_SOUL);
  else
    lvl = GET_LEVEL(ch);


  GET_SPELLCASTER_LEVEL(ch) = lvl;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");

  if (AFF_FLAGGED(ch, AFF_STUNNED)) {
    send_to_char(ch, "You can't cast spells while stunned!\r\n");
    return;
  }
  if (ch->spell_cast == TRUE && FIGHTING(ch) && !PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
    send_to_char(ch, "You have already cast a spell this round!\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_SILENCE) && !(subcmd == SCMD_INNATE || subcmd == SCMD_ART)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    act("$n moves $s mouth but no sound comes out", TRUE, ch, 0, ch, TO_NOTVICT);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && !(subcmd == SCMD_INNATE || subcmd == SCMD_ART) &&
      !HAS_FEAT(ch, FEAT_NATURAL_SPELL)) {
    send_to_char(ch, "You cannot cast spells in this form.\r\n");
    return;
  }
  if (subcmd == SCMD_CHANT &&
        ((GET_EQ(ch, WEAR_BODY) && is_metal_item(GET_EQ(ch, WEAR_BODY))) ||
        (GET_EQ(ch, WEAR_SHIELD) && is_metal_item(GET_EQ(ch, WEAR_SHIELD))))) {
    send_to_char(ch, "You cannot cast druid spells while wearing metal armor.\r\n");
    return;
  }

  if (s == NULL) {
    if (subcmd == SCMD_ART || subcmd == SCMD_INNATE)
      send_to_char(ch, "Use what ability?\r\n");
    else if (subcmd == SCMD_PRAY)
      send_to_char(ch, "Pray for what spell?\r\n");
    else if (subcmd == SCMD_INTONE)
      send_to_char(ch, "Intone which spell?\r\n");
    else if (subcmd == SCMD_CHANT)
      send_to_char(ch, "Chant which spell?\r\n");
    else if (subcmd == SCMD_COMMUNE)
      send_to_char(ch, "Commune for which spell?\r\n");
    else if (subcmd == SCMD_SING)
      send_to_char(ch, "Sing which spell?\r\n");
    else if (subcmd == SCMD_EVOKE)
      send_to_char(ch, "Evoke which spell?\r\n");
    else if (subcmd == SCMD_IMBUE)
      send_to_char(ch, "Imbue which spell?\r\n");
    else if (subcmd == SCMD_DUSK)
      send_to_char(ch, "Use which assassin spell?\r\n");
    else if (subcmd == SCMD_MUSIC)
      send_to_char(ch, "Play which bard song?\r\n");
    else
      send_to_char(ch, "Cast what spell?\r\n");
    return;
  }

  // Summon Druid / Ranger Check
  if( dBuf[0] != '\0' && strcmp(dBuf, "for companion") == 0)
  {
	  int b_vnum = 4506;
	  struct follow_type *f;
	  struct char_data *mob;
	  char buf[MSL];

	  if (!HAS_FEAT(ch, FEAT_ANIMAL_COMPANION))
	  {
		send_to_char(ch, "You must have the animal companion feat in order to have an animal companion.\r\n");
	    return;
	  }

	  if( ch->summon_type <= 0)
	  {
		  send_to_char(ch, "You need to set your pet stats before summoning it. Please use the @Rpetset@n command to do so.\r\n");
		  return;
	  }

	  for( f = ch->followers; f; f = f->next)
	  {
		  if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM) && GET_MOB_VNUM(f->follower) == b_vnum)
		  {
			  send_to_char(ch, "You already control a companion!\r\n");
			  return;
		  }
	  }
          if (FIGHTING(ch))
          {
              send_to_char(ch, "You can't seem to concentrate enough on the world around you!\r\n");
              return;
          }

	  mob = read_mobile(b_vnum, virtual);
	  if(mob)
	  {
              int hdMod = 0;
		  GET_CLASS(mob) = CLASS_FIGHTER;
		  mob->summon_type = ch->summon_type;
                  mob->new_summon = TRUE;
                  mob->level = MAX(1, calc_summoner_level(ch, CLASS_DRUID) / 2);
                  mob->race_level = MAX(1, calc_summoner_level(ch, CLASS_DRUID) / 2);
		  mob->aff_abils.str = GET_STR(ch) + pet_table[mob->summon_type - 1].str_mod;
		  mob->aff_abils.dex = GET_DEX(ch) + pet_table[mob->summon_type - 1].dex_mod;
		  mob->aff_abils.intel = GET_INT(ch);
		  mob->aff_abils.wis = GET_WIS(ch);
		  mob->aff_abils.con = GET_CON(ch) + pet_table[mob->summon_type - 1].con_mod;
		  mob->aff_abils.cha = GET_CHA(ch);
                  while(calc_summoner_level(ch, CLASS_DRUID) > hdMod)
                      hdMod++;
		  mob->sum_hd = hdMod + pet_table[mob->summon_type - 1].hd_mod;
		  GET_MAX_HIT(mob) = (dice(3, 6) * mob->sum_hd) + ability_mod_value(GET_CON(ch)) + (GET_CLASS_RANKS(mob, CLASS_FIGHTER) / 2) + (get_skill_value(ch, SKILL_HANDLE_ANIMAL) / 4);
		  GET_HIT(mob) = GET_MAX_HIT(mob);
		  GET_MOVE(mob) = GET_MAX_MOVE(mob);
		  mob->name = strdup(ch->sum_name);
	          sprintf(buf, "%s is here\r\n", ch->sum_desc);
		  mob->short_descr = strdup(ch->sum_desc);
		  mob->long_descr = strdup(buf);
                  char_to_room(mob, IN_ROOM(ch));
          IS_CARRYING_W(mob) = 0;
          IS_CARRYING_N(mob) = 0;
          SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
          load_mtrigger(mob);
          add_follower(mob, ch);
          if (FIGHTING(ch))		  
            set_fighting(mob, FIGHTING(ch));          
          mob->master_id = GET_IDNUM(ch);	   	  
		  act("$N comes forth, answering $n's call.", TRUE, ch, 0, mob, TO_ROOM);
	      act("$N comes forth, answering your call.", TRUE, ch, 0, mob, TO_CHAR);
		  return;
	  }
	  

	  return;
  }

  s = strtok(NULL, "'");
  if (s == NULL || !*s) {
    if (subcmd == SCMD_ART || subcmd == SCMD_INNATE)
      send_to_char(ch, "You must enclose the ability name in quotes: '\r\n");
    else
      send_to_char(ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
    return;
  }
  t = strtok(NULL, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s, SKTYPE_SPELL| SKTYPE_ART);

  spellnum_bak = spellnum;

  sprintf(buffer, "%d", spellnum_bak);

  if (is_innate(ch, spellnum))
    innate = true;

  if (spellnum <= 0 )
  {
      send_to_char(ch, "The powers of %s have yet to be unlocked from the weave.\r\n", s);
      return;
  }

  if (spellnum != SPELL_REMOVE_PARALYSIS && ch->paralyzed) {
    send_to_char(ch, "You cannot cast spells with somatic components while paralyzed.\r\n");
    return;
  }

  if (subcmd != SCMD_INNATE && subcmd != SCMD_MUSIC && subcmd != SCMD_ART && attrib - 10 < spell_info[spellnum].class_level[classnum] &&
        !(HAS_FEAT(ch, FEAT_AURA_OF_GOOD) && spellnum == SPELL_AURA_OF_GOOD) &&
        !(HAS_FEAT(ch, FEAT_AURA_OF_COURAGE) && spellnum == SPELL_AURA_OF_COURAGE)) {

    send_to_char(ch, "Your prime requisite ability score is not high enough to cast spells of that level.\r\n");
    send_to_char(ch, "It is also possible you are using the wrong spellcasting command.  Type @YHELP CAST@n for more info.\r\n");
    return;
  }



  if (subcmd != SCMD_MUSIC && subcmd != SCMD_ART && subcmd != SCMD_INNATE) {
    int manacost = spell_info[spellnum].class_level[classnum];
    if (PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL)) {
      manacost += 3;
    }
    if (PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL)) {
      manacost += 7;
    }
    if (PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
      manacost += 4;
    }
    if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
      manacost += 2;
    }
    if (PRF_FLAGGED(ch, PRF_EXTEND_SPELL)) {
      manacost += 1;
    }
    while (GET_MANA(ch) < manacost && manacost > spell_info[spellnum].class_level[classnum]) {
      if (PRF_FLAGGED(ch, PRF_EXTEND_SPELL)) {
        manacost -= 1;
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EXTEND_SPELL);
        continue;
      }
      if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
        manacost -= 2;
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EMPOWER_SPELL);
        continue;
      }
      if (PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL)) {
        manacost -= 3;
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_MAXIMIZE_SPELL);
        continue;
      }
      if (PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
        manacost -= 4;
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUICKEN_SPELL);
        continue;
      }
      if (PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL)) {
        manacost -= 7;
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_INTENSIFY_SPELL);
        continue;
      }
    }
    if (manacost > spell_info[spellnum].class_level[classnum])
      GET_MANA(ch) -= manacost;
  }

  if (!FIGHTING(ch) && spell_info[spellnum].violent && !PRF_FLAGGED(ch, PRF_SPONTANEOUS)) {
    send_to_char(ch, "You must first enter combat by either using the attack or assist commands.\r\n");
    return;
  }

  if (spell_info[spellnum].epic_dc > 0 && spell_info[spellnum].epic_dc > get_skill_value(ch, SKILL_SPELLCRAFT)) {
    send_to_char(ch, "The spell '%s' needs a spellcraft skill of %d, while yours is only %d.\r\n",
                 spell_info[spellnum].name, spell_info[spellnum].epic_dc, get_skill_value(ch, SKILL_SPELLCRAFT));
    return;
  }

  if (spell_info[spellnum].class_level[classnum] == 10) {
    if (!(HAS_FEAT(ch, FEAT_EPIC_SPELLCASTING))) {
      send_to_char(ch, "You do not know how to cast epic spells.\r\n");
      return;
    }
    if (GET_EPIC_SPELLS(ch) < 1) {
      if (is_innate_ready(ch, SPELL_EPIC_SPELLS)) {
        GET_EPIC_SPELLS(ch) = get_skill_value(ch, SKILL_KNOWLEDGE) / 10;
        send_to_char(ch, "Your epic spells have been refreshed to full.\r\n");
      } else {
        send_to_char(ch, "You have used up all of your epic spells for the moment.\r\n");
        return;
      }
    }
    if (spell_info[spellnum].epic_dc > (get_skill_value(ch, SKILL_SPELLCRAFT) + 10)) {
      send_to_char(ch, "Your spellcraft skill isn't high enough to cast that epic spell.\r\n");
      return;
    }
  }
  else if (subcmd == SCMD_INNATE) {
    switch (spellnum) {
      case SPELL_INSPIRE_COURAGE:
        if (!(HAS_FEAT(ch, FEAT_INSPIRE_COURAGE))) {
          send_to_char(ch, "You do not have the ability to inspire courage.\r\n");
          return;
        }
        break;
      case SPELL_INSPIRE_GREATNESS:
        if (!(HAS_FEAT(ch, FEAT_INSPIRE_GREATNESS))) {
          send_to_char(ch, "You do not have the ability to inspire greatness.\r\n");
          return;
        }
        break;
      case SPELL_ANIMATE_DEAD:
        if (!(HAS_FEAT(ch, FEAT_ANIMATE_DEAD))) {
          send_to_char(ch, "You do not have the ability to animate dead.\r\n");
          return;
        }
        break;
      case SPELL_SUMMON_UNDEAD:
        if (!(HAS_FEAT(ch, FEAT_SUMMON_UNDEAD))) {
          send_to_char(ch, "You do not have the ability to summon undead.\r\n");
          return;
        }
        break;
      case SPELL_SUMMON_GREATER_UNDEAD:
        if (!(HAS_FEAT(ch, FEAT_SUMMON_GREATER_UNDEAD))) {
          send_to_char(ch, "You do not have the ability to summon greater undead.\r\n");
          return;
        }
        break;
      case SPELL_INVISIBLE:
        if (GET_RACE(ch) != RACE_DUERGAR) {
          send_to_char(ch, "You do not have the ability to use invisibility.\r\n");
          return;
        }
        break;
      case SPELL_ENLARGE_PERSON:
        if (GET_RACE(ch) != RACE_DUERGAR) {
          send_to_char(ch, "You do not have the ability to use enlarge person.\r\n");
          return;
        }
        break;
      case SPELL_BLINDNESS:
        if (GET_RACE(ch) != RACE_DROW_ELF && GET_RACE(ch) != RACE_SVIRFNEBLIN && GET_RACE(ch) != RACE_TIEFLING) {
          send_to_char(ch, "You do not have the ability to use blindness.\r\n");
          return;
        }
        break;
      case SPELL_FAERIE_FIRE:
        if (GET_RACE(ch) != RACE_DROW_ELF) {
          send_to_char(ch, "You do not have the ability to use faerie fire.\r\n");
          return;
        }
        break;
      case SPELL_BLUR:
         if (GET_RACE(ch) != RACE_SVIRFNEBLIN) {
          send_to_char(ch, "You do not have the ability to use blur.\r\n");
          return;
        }
        break;
      case SPELL_DAYLIGHT:
        if (GET_RACE(ch) != RACE_AASIMAR) {
          send_to_char(ch, "You do not have the ability to use daylight.\r\n");
          return;
        }
        break;
      case SPELL_FLY:
        if (GET_RACE(ch) != RACE_AIR_GENESI) {
          send_to_char(ch, "You do not have the ability to use fly.\r\n");
          return;
        }
        break;
      case SPELL_FIREBALL:
        if (GET_RACE(ch) != RACE_FIRE_GENESI) {
          send_to_char(ch, "You do not have the ability to use fireball.\r\n");
          return;
        }
        break;
      case SPELL_HASTE:
        if (!HAS_FEAT(ch, FEAT_HASTE)) {
          send_to_char(ch, "You do not have the ability to use haste.\r\n");
          return;
        }
        break;
      case SPELL_FLAME_WEAPON:
        if (!HAS_FEAT(ch, FEAT_SACRED_FLAMES)) {
          send_to_char(ch, "You do not have the ability to use flame weapon.\r\n");
          return;
        }
        break;
      case SPELL_ICE_STORM:
        if (GET_RACE(ch) != RACE_WATER_GENESI) {
          send_to_char(ch, "You do not have the ability to use ice storm.\r\n");
          return;
        }
        break;
      case SPELL_STONESKIN:
        if (GET_RACE(ch) != RACE_EARTH_GENESI) {
          send_to_char(ch, "You do not have the ability to use stoneskin.\r\n");
          return;
        }
        break;
      default:
        send_to_char(ch, "You do not have the ability to use that spell innately.\r\n");
        return;

    }
    innate = TRUE;
    if (GET_INNATE(ch, spellnum) < 1) {
      if (is_innate_ready(ch, spellnum)) {
        switch (spellnum) {
          case SPELL_INSPIRE_COURAGE:
            GET_INNATE(ch, spellnum) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
            send_to_char(ch, "Your inspire courage uses have been refreshed to full.\r\n");
            break;
          case SPELL_ANIMATE_DEAD:
            GET_INNATE(ch, spellnum) = HAS_FEAT(ch, FEAT_ANIMATE_DEAD);
            send_to_char(ch, "Your animate dead uses have been refreshed to full.\r\n");
            break;
          case SPELL_SUMMON_UNDEAD:
            GET_INNATE(ch, spellnum) = HAS_FEAT(ch, FEAT_SUMMON_UNDEAD);
            send_to_char(ch, "Your summon undead uses have been refreshed to full.\r\n");
            break;
          case SPELL_SUMMON_GREATER_UNDEAD:
            GET_INNATE(ch, spellnum) = HAS_FEAT(ch, FEAT_SUMMON_GREATER_UNDEAD);
            send_to_char(ch, "Your summon greater undead uses have been refreshed to full.\r\n");
            break;
          case SPELL_INSPIRE_GREATNESS:
            if (is_innate_ready(ch, SPELL_INSPIRE_COURAGE)) {
              GET_INNATE(ch, SPELL_INSPIRE_COURAGE) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
              send_to_char(ch, "Your inspire greatness uses have been refreshed to full.\r\n");
            }
            else {
              send_to_char(ch, "Your inspire courage/greatness uses have been used up.\r\n");
              return;
            }
            break;
          case SPELL_INVISIBLE:
            if (GET_RACE(ch) == RACE_DUERGAR)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your invisibility uses have been refreshed to full.\r\n");
            break;
          case SPELL_ENLARGE_PERSON:
            if (GET_RACE(ch) == RACE_DUERGAR)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your enlarge person uses have been refreshed to full.\r\n");
            break;
          case SPELL_BLINDNESS:
            if (GET_RACE(ch) == RACE_DROW_ELF || GET_RACE(ch) == RACE_SVIRFNEBLIN || GET_RACE(ch) == RACE_TIEFLING)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your blindness uses have been refreshed to full.\r\n");
            break;
          case SPELL_FAERIE_FIRE:
            if (GET_RACE(ch) == RACE_DROW_ELF)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your faerie fire uses have been refreshed to full.\r\n");
            break;
          case SPELL_BLUR:
            if (GET_RACE(ch) == RACE_SVIRFNEBLIN)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your blur uses have been refreshed to full.\r\n");
            break;
          case SPELL_DAYLIGHT:
            if (GET_RACE(ch) == RACE_AASIMAR)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your ball of light uses have been refreshed to full.\r\n");
            break;
          case SPELL_FLY:
            if (GET_RACE(ch) == RACE_AIR_GENESI)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your fly uses have been refreshed to full.\r\n");
            break;
          case SPELL_FIREBALL:
            if (GET_RACE(ch) == RACE_FIRE_GENESI)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your fireball uses have been refreshed to full.\r\n");
            break;
          case SPELL_HASTE:
            if (HAS_FEAT(ch, FEAT_HASTE))
              GET_INNATE(ch, spellnum) = 3;
            send_to_char(ch, "Your haste uses have been refreshed to full.\r\n");
            break;
          case SPELL_FLAME_WEAPON:
            if (HAS_FEAT(ch, FEAT_SACRED_FLAMES))
              GET_INNATE(ch, spellnum) = 3;
            send_to_char(ch, "Your flame weapon (sacred flames) uses have been refreshed to full.\r\n");
            break;
          case SPELL_ICE_STORM:
            if (GET_RACE(ch) == RACE_WATER_GENESI)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your ice storm uses have been refreshed to full.\r\n");
            break;
          case SPELL_STONESKIN:
            if (GET_RACE(ch) == RACE_EARTH_GENESI)
              GET_INNATE(ch, spellnum) = 1;
            send_to_char(ch, "Your stoneskin uses have been refreshed to full.\r\n");
            break;
        }
      }
      else {
        send_to_char(ch, "You do not have any uses left in that ability right now.  See the timers command.\r\n");
        return;
      }
    }
  }
  else if (subcmd == SCMD_MUSIC) {
    if (GET_BARD_SONGS(ch) < 1) {
      if (is_innate_ready(ch, SPELL_BARD_SONGS)) {
        GET_BARD_SONGS(ch) = GET_CLASS_RANKS(ch, CLASS_BARD) + HAS_FEAT(ch, FEAT_EXTRA_MUSIC) ? 4 : 0;
        send_to_char(ch, "Your bard songs have been refreshed to full.\r\n");
      }
      else {
        send_to_char(ch, "You have used up all of your bard songs for the moment.\r\n");
        return;
      }
    }
    if (spell_info[spellnum].bard_song == FALSE) {
      send_to_char(ch, "That is not a valid bard song.  Type bardsongs to see which bard songs are available to you.\r\n");
      return;
    }
    if (!(HAS_FEAT(ch, spell_info[spellnum].bard_feat))) {
      send_to_char(ch, "You do not know bard song.  Type bardsongs to see which bard songs are available to you.\r\n");
      return;
    }
    if (spell_info[spellnum].perform_rank > get_skill_value(ch, SKILL_PERFORM)) {
      send_to_char(ch, "Your perform skill isn't high enough to play that bard song.\r\n");
      return;
    }
  }
  else if (subcmd == SCMD_SING) {
    if (GET_BARD_SPELLS(ch, spell_info[spellnum].class_level[CLASS_BARD]) == 0 && is_innate_ready(ch, SPELL_BARD_SPELLS)) {
      for (count = 0; count <= 6; count++)
        GET_BARD_SPELLS(ch, count) = findslotnum(ch, count);
      send_to_char(ch, "Your bard spells have been refreshed.\r\n");
    }

    if (!knows_spell(ch, spellnum)) {
      send_to_char(ch, "You don't know that spell.\r\n");
      return;
    }

    if (spell_info[spellnum].class_level[CLASS_BARD] == 99) {
      send_to_char(ch, "That is not a valid bard spell.\r\n");
      return;
    }

    if (GET_BARD_SPELLS(ch, spell_info[spellnum].class_level[CLASS_BARD]) < 1) {
      send_to_char(ch, "You do not have any bard spells left to cast at the moment.\r\n");
      return;
    }
  }
  else if (subcmd == SCMD_EVOKE) {
    if (GET_SORCERER_SPELLS(ch, spell_info[spellnum].class_level[CLASS_WIZARD]) == 0 && is_innate_ready(ch, SPELL_SORCERER_SPELLS)) {
      for (count = 0; count <= 9; count++)
        GET_SORCERER_SPELLS(ch, count) = findslotnum(ch, count);
      send_to_char(ch, "Your sorcerer spells have been refreshed.\r\n");
    }

    if (!knows_spell(ch, spellnum)) {
      send_to_char(ch, "You don't know that spell.\r\n");
      return;
    }

    if (spell_info[spellnum].class_level[CLASS_WIZARD] == 99) {
      send_to_char(ch, "That is not a valid sorcerer spell.\r\n");
      return;
    }

    if (GET_SORCERER_SPELLS(ch, spell_info[spellnum].class_level[CLASS_WIZARD]) < 1) {
      send_to_char(ch, "You do not have any %d level sorcerer spells left to cast at the moment.\r\n", spell_info[spellnum].class_level[CLASS_WIZARD]);
      return;
    }
  }
  else if (subcmd == SCMD_IMBUE) {
    if (GET_FAVORED_SOUL_SPELLS(ch, spell_info[spellnum].class_level[CLASS_CLERIC]) == 0 && is_innate_ready(ch, SPELL_FAVORED_SOUL_SPELLS)) {
      for (count = 0; count <= 9; count++)
        GET_FAVORED_SOUL_SPELLS(ch, count) = findslotnum(ch, count);
      send_to_char(ch, "Your favored soul spells have been refreshed.\r\n");
    }

    if (!knows_spell(ch, spellnum)) {
      send_to_char(ch, "You don't know that spell.\r\n");
      return;
    }

    if (spell_info[spellnum].class_level[CLASS_CLERIC] == 99) {
      send_to_char(ch, "That is not a valid favored soul spell.\r\n");
      return;
    }

    if (GET_FAVORED_SOUL_SPELLS(ch, spell_info[spellnum].class_level[CLASS_CLERIC]) < 1) {
      send_to_char(ch, "You do not have any %d level favored soul spells left to cast at the moment.\r\n", 
        spell_info[spellnum].class_level[CLASS_CLERIC]);
      return;
    }
  }
  else if (subcmd == SCMD_DUSK) {
    if (GET_ASSASSIN_SPELLS(ch, spell_info[spellnum].class_level[CLASS_ASSASSIN]) == 0 && is_innate_ready(ch, SPELL_ASSASSIN_SPELLS)) {
      for (count = 1; count <= 4; count++)
        GET_ASSASSIN_SPELLS(ch, count) = findslotnum(ch, count);
      send_to_char(ch, "Your assassin spells have been refreshed.\r\n");
    }

    if (!knows_spell(ch, spellnum)) {
      send_to_char(ch, "You don't know that spell.\r\n");
      return;
    }

    if (spell_info[spellnum].class_level[CLASS_ASSASSIN] == 99) {
      send_to_char(ch, "That is not a valid assassin spell.\r\n");
      return;
    }

    if (GET_ASSASSIN_SPELLS(ch, spell_info[spellnum].class_level[CLASS_ASSASSIN]) < 1) {
      send_to_char(ch, "You do not have any %d level assassin spells left to cast at the moment.\r\n",
                   spell_info[spellnum].class_level[CLASS_ASSASSIN]);
      return;
    }
  }
  else if (subcmd == SCMD_ART) {
    if (!IS_MONK(ch) && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
      send_to_char(ch, "You are not trained in that.\r\n");
      return;
    }

    if ((spellnum < 1) || (spellnum >= SKILL_TABLE_SIZE) || !(SINFO.skilltype & SKTYPE_ART)) {
      send_to_char(ch, "I don't recognize that martial art or ability.\r\n");
      return;
    }

    if (!GET_SKILL(ch, spellnum)) {
      send_to_char(ch, "You do not have that ability.\r\n");
      return;
    }
  } else {
    if (!innate && (!IS_SPELLCASTER(ch)) && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
      send_to_char(ch, "You are not able to cast spells.\r\n");
      return;
    }

    if ((spellnum < 1) || (spellnum >= SKILL_TABLE_SIZE) || !(SINFO.skilltype & SKTYPE_SPELL)) {
      send_to_char(ch, "Cast what?!?\r\n");
      return;
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You cannot do that from this position.\r\n");
      return;
    }

    /* if spell NOT memorized and is not an innate ability */
    if (!innate && ((!IS_NPC(ch) && !is_memorized(ch, spellnum, true)) && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
        !(HAS_FEAT(ch, FEAT_AURA_OF_GOOD) && spellnum == SPELL_AURA_OF_GOOD) &&
        !(HAS_FEAT(ch, FEAT_AURA_OF_COURAGE) && spellnum == SPELL_AURA_OF_COURAGE)) {
      send_to_char(ch, "You do not have that spell memorized!\r\n");
      return;
    }
  }
  if (!innate && (classnum == CLASS_FAVORED_SOUL || classnum == CLASS_SORCERER)) {
    if (spell_info[spellnum].epic_dc > 0)
      spellKnown = TRUE;
    for (n = 0; n < MAX_NUM_KNOWN_SPELLS; n++)
      if (spellnum == ch->player_specials->spells_known[n])
        spellKnown = TRUE;
    if (!spellKnown) {
      send_to_char(ch, "You do not know that spell.\r\n");
      return;
    }
  }

  if (spellnum == SPELL_AURA_OF_GOOD && !HAS_FEAT(ch, FEAT_AURA_OF_GOOD)) {
    send_to_char(ch, "You don't have that ability!\r\n");
    return;
  }

  if (spellnum == SPELL_AURA_OF_COURAGE && !HAS_FEAT(ch, FEAT_AURA_OF_COURAGE)) {
    send_to_char(ch, "You don't have that ability!\r\n");
    return;
  }

  if (subcmd == SCMD_ART) {
    ki = mag_kicost(ch, spellnum);
    if ((ki > 0) && (GET_KI(ch) < ki) && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT)) {
      send_to_char(ch, "You haven't the energy to use that ability!\r\n");
      return;
    }
  }


  if (!innate && PRF_FLAGGED(ch, PRF_SPONTANEOUS) && subcmd == SCMD_CHANT) {
      switch (spell_info[spellnum].class_level[CLASS_DRUID]) {
        case 0:
          break;
        case 1:
          spellnum = SPELL_SUMMON_NATURE_I;
          break;
        case 2:
          spellnum = SPELL_SUMMON_NATURE_II;
          break;
        case 3:
          spellnum = SPELL_SUMMON_NATURE_III;
          break;
        case 4:
          spellnum = SPELL_SUMMON_NATURE_IV;
          break;
        case 5:
          spellnum = SPELL_SUMMON_NATURE_V;
          break;
        case 6:
          spellnum = SPELL_SUMMON_NATURE_VI;
          break;
        case 7:
          spellnum = SPELL_SUMMON_NATURE_VII;
          break;
        case 8:
          spellnum = SPELL_SUMMON_NATURE_VIII;
          break;
        case 9:
          spellnum = SPELL_SUMMON_NATURE_IX;
          break;
        default:
          break;
      }
  }
  if (!innate && PRF_FLAGGED(ch, PRF_SPONTANEOUS) && subcmd == SCMD_PRAY) {

    if (IS_GOOD(ch) || (IS_NEUTRAL(ch) && !PRF_FLAGGED(ch, PRF_NEGATIVE))) {
      switch (spell_info[spellnum].class_level[CLASS_CLERIC]) {
        case 0:
          spellnum = SPELL_CURE_MINOR;
          break;
        case 1:
          spellnum = SPELL_CURE_LIGHT;
          break;
        case 2:
          if (HAS_DOMAIN(ch, DOMAIN_HEALING))
            spellnum = SPELL_CURE_SERIOUS;
          else
          spellnum = SPELL_CURE_MODERATE;
          break;
        case 3:
          spellnum = SPELL_CURE_SERIOUS;
          break;
        case 4:
          spellnum = SPELL_CURE_CRITIC;
          break;
        case 5:
        	if (HAS_DOMAIN(ch, DOMAIN_HEALING))
            spellnum = SPELL_HEAL;
          else
            spellnum = SPELL_CURE_CRITIC;
          break;
        case 6:
          spellnum = SPELL_HEAL;
          break;
        case 7:
          spellnum = SPELL_HEAL;
          break;
        case 8:
          spellnum = SPELL_MASS_HEAL;
          break;
        case 9:
          spellnum = SPELL_MASS_HEAL;
          break;
        default:
          spellnum = SPELL_CURE_MINOR;
          break;
      }
    }
    else {
      switch (spell_info[spellnum].class_level[CLASS_CLERIC]) {
        case 0:
          spellnum = SPELL_INFLICT_MINOR;
          break;
        case 1:
          spellnum = SPELL_INFLICT_LIGHT;
          break;
        case 2:
          spellnum = SPELL_INFLICT_MODERATE;
          break;
        case 3:
          spellnum = SPELL_INFLICT_SERIOUS;
          break;
        case 4:
          spellnum = SPELL_INFLICT_CRITIC;
          break;
        case 5:
          spellnum = SPELL_INFLICT_CRITIC;
          break;
        case 6:
          spellnum = SPELL_HARM;
          break;
        case 7:
          spellnum = SPELL_HARM;
          break;
        case 8:
          spellnum = SPELL_MASS_HARM;
          break;
        case 9:
          spellnum = SPELL_MASS_HARM;
          break;
        default:
          spellnum = SPELL_INFLICT_MINOR;
          break;
      }
    }
  }

  /* Find the target */
  if (t != NULL) {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, t, sizeof(arg));
    one_argument(arg, t);
    skip_spaces(&t);
  }
  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = true;
  } else if (t != NULL && *t) {
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
      if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_ROOM)) != NULL)
	target = true;
    }
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD)) != NULL)
	target = true;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, t, NULL, ch->carrying)) != NULL)
	target = true;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
	if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name)) {
	  tobj = GET_EQ(ch, i);
	  target = true;
	}
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, t, NULL, world[IN_ROOM(ch)].contents)) != NULL)
	target = true;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, t, NULL)) != NULL)
	target = true;
	
  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
	tch = ch;
	target = true;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
	tch = FIGHTING(ch);
	target = true;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent) {
      tch = ch;
      target = true;
    }
    if (!target) {
      send_to_char(ch, "Upon %s should the spell be cast?\r\n",
		IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
      return;
    }
  }

  if (target && (tch == ch) && SINFO.violent) {
    send_to_char(ch, "You shouldn't cast that on yourself -- could be bad for your health!\r\n");
    return;
  }

  if (!target) {
    send_to_char(ch, "Cannot find the target of your spell!\r\n");
    return;
  }

//herehere

  if (FIGHTING(ch) && !ch->active_turn) {
    send_to_char(ch, "You cannot cast a spell until it is your turn in combat.\r\n");
    return;
  }
  else if (FIGHTING(ch) && ch->active_turn) {

  if (HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) || (HAS_FEAT(ch, FEAT_QUICKEN_SPELL) && PRF_FLAGGED(ch, PRF_QUICKEN_SPELL))) {
    if (spell_info[spellnum].class_level[classnum] <= (HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) * 3)) {
      if (!can_use_available_actions(ch, ACTION_MOVE)) {
        send_to_char(ch, "You don't have any actions left to cast this spell.\r\n");
        return;
      }
    }
    else if (IS_SET(SINFO.routines, MAG_ACTION_FULL | MAG_ACTION_FULL) && HAS_FEAT(ch, FEAT_QUICKEN_SPELL) && PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
      if (!can_use_available_actions(ch, ACTION_MOVE)) {
        send_to_char(ch, "You don't have any actions left to cast this spell.\r\n");
        return;
      }
    }
    else {
      if (!can_use_available_actions(ch, ACTION_STANDARD)) {
        send_to_char(ch, "You don't have any actions left to cast this spell.\r\n");
        return;
      }
    }
  }
  else if (IS_SET(SINFO.routines, MAG_ACTION_FULL | MAG_ACTION_FULL) && PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
      if (!can_use_available_actions(ch, ACTION_MOVE)) {
        send_to_char(ch, "You don't have any actions left to cast this spell.\r\n");
        return;
      }
  }
  else {
    if (!can_use_available_actions(ch, ACTION_STANDARD)) {
      send_to_char(ch, "You don't have any actions left to cast this spell.\r\n");
      return;
    }
  }
  }

  if (SINFO.violent && tch && IS_NPC(tch)) {
    if (!FIGHTING(tch))
      set_fighting(tch, ch);
    if (!FIGHTING(ch))
      set_fighting(ch, tch);
  }

  if (!innate && IS_SET(SINFO.comp_flags, MAGCOMP_SOMATIC) && rand_number(1, 100) <= calc_spellfail(ch) &&
      (classnum == CLASS_BARD || classnum == CLASS_WIZARD || classnum == CLASS_SORCERER)) {
    send_to_char(ch, "Your armor interferes with your casting, and you fail!\r\n");
    do_mem_spell(ch, buffer, SCMD_FORGET, 0);
    if (PRF_FLAGGED(ch, PRF_AUTOMEM)) 
      do_mem_spell(ch, buffer, SCMD_MEMORIZE, 0);
    return;
  } else {


    if (cast_spell(ch, tch, tobj, spellnum, t) && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {      
      if (ki > 0) {
        GET_KI(ch) = MAX(0, MIN(GET_MAX_KI(ch), GET_KI(ch) - ki));
        return;
      }
      if (HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) && ch == tch) {
        for (f = ch->followers; f; f = f->next) {
          if (IS_NPC(f->follower) && GET_MOB_VNUM(f->follower) == GET_COMPANION_VNUM(ch)) {
            cast_spell(ch, f->follower, tobj, spellnum, t);
            break;
          }
        }
      }
      if (spell_info[spellnum].class_level[classnum] == 10) {
        GET_EPIC_SPELLS(ch) -= 1;
        if (is_innate_ready(ch, SPELL_EPIC_SPELLS)) {
          add_innate_timer(ch, SPELL_EPIC_SPELLS);
        }
        return;
      }
      else if (subcmd == SCMD_MUSIC) {
        GET_BARD_SONGS(ch) -= 1;
        if (is_innate_ready(ch, SPELL_BARD_SONGS)) {
          add_innate_timer(ch, SPELL_BARD_SONGS);
        }
        return;
      }
      else if (subcmd == SCMD_SING) {
        GET_BARD_SPELLS(ch, spell_info[spellnum].class_level[CLASS_BARD]) -= 1;
        if (is_innate_ready(ch, SPELL_BARD_SPELLS)) {
          send_to_char(ch, "Your bard spells will be refreshed in approximately 10 minutes.\r\n");
          add_innate_timer(ch, SPELL_BARD_SPELLS);
        }
        return;
      }
      else if (subcmd == SCMD_EVOKE) {
        GET_SORCERER_SPELLS(ch, spell_info[spellnum].class_level[CLASS_SORCERER]) -= 1;
        if (is_innate_ready(ch, SPELL_SORCERER_SPELLS)) {
          send_to_char(ch, "Your sorcerer spells will be refreshed in approximately 10 minutes.\r\n");
          add_innate_timer(ch, SPELL_SORCERER_SPELLS);
        }
        return;
      }
      else if (subcmd == SCMD_IMBUE) {
        GET_FAVORED_SOUL_SPELLS(ch, spell_info[spellnum].class_level[CLASS_CLERIC]) -= 1;
        if (is_innate_ready(ch, SPELL_FAVORED_SOUL_SPELLS)) {
          send_to_char(ch, "Your favored soul spells will be refreshed in approximately 10 minutes.\r\n");
          add_innate_timer(ch, SPELL_FAVORED_SOUL_SPELLS);
        }
        return;
      }
      else if (subcmd == SCMD_DUSK) {
        GET_ASSASSIN_SPELLS(ch, spell_info[spellnum].class_level[CLASS_ASSASSIN]) -= 1;
        if (is_innate_ready(ch, SPELL_ASSASSIN_SPELLS)) {
          send_to_char(ch, "Your assassin spells will be refreshed in approximately 10 minutes.\r\n");
          add_innate_timer(ch, SPELL_ASSASSIN_SPELLS);
        }
        return;
      }
      else if (subcmd == SCMD_INNATE) {
        GET_INNATE(ch, spellnum) -= 1;
        if (is_innate_ready(ch, spellnum)) {
          send_to_char(ch, "Your %s ability will be refreshed in approximately 10 minutes.\r\n", spell_info[spellnum].name);
          add_innate_timer(ch, spellnum);
        }
        return;
      }
      if (spell_info[spellnum].class_level[classnum] > 0) {
        do_mem_spell(ch, buffer, SCMD_FORGET, 0);
        if (PRF_FLAGGED(ch, PRF_AUTOMEM)) {
          do_mem_spell(ch, buffer, SCMD_MEMORIZE, 0);
        }
      }
    }
  }

}

void skill_race_class(int spell, int race, int learntype)
{
  int bad = 0;

  if (spell < 0 || spell >= SKILL_TABLE_SIZE) {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, SKILL_TABLE_SIZE);
    return;
  }

  if (race < 0 || race >= NUM_RACES) {
    log("SYSERR: assigning '%s' to illegal race %d/%d.", skill_name(spell),
                race, NUM_RACES - 1);
    bad = 1;
  }

  if (!bad)
    spell_info[spell].race_can_learn[race] = learntype;
}

void spell_level(int spell, int chclass, int level)
{
  int bad = 0;

  if (spell < 0 || spell > SKILL_TABLE_SIZE) {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, SKILL_TABLE_SIZE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
		chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1) {
    log("SYSERR: assigning '%s' to illegal level %d.", skill_name(spell),
		level);
    bad = 1;
  }

  if (!bad)
    spell_info[spell].min_level[chclass] = level;
}

void skill_class(int skill, int chclass, int learntype)
{
  int bad = 0;

  if (skill < 0 || skill > SKILL_TABLE_SIZE) {
    log("SYSERR: attempting assign to illegal skillnum %d/%d", skill, SKILL_TABLE_SIZE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(skill),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (learntype < 0 || learntype > SKLEARN_CLASS) {
    log("SYSERR: assigning skill '%s' illegal learn type %d for class %d.", skill_name(skill),
                learntype, chclass);
    bad = 1;
  }

  if (!bad)
    spell_info[skill].can_learn_skill[chclass] = learntype;
}

int skill_type(int snum)
{
  return spell_info[snum].skilltype;
}

void set_skill_type(int snum, int sktype)
{
  spell_info[snum].skilltype = sktype;
}


void assassin_spell(int spellnum, int level) {
  spell_info[spellnum].class_level[CLASS_ASSASSIN] = level;
}
void epic_spell(int spellnum, int dc) {
  spell_info[spellnum].epic_dc = dc;
}

void elemental_spell(int spellnum) {
  spell_info[spellnum].elemental = TRUE;
}
	
/* Assign the spells on boot up */
void spello(int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos,
int targets, int violent, int routines, int save_flags, int comp_flags, const char *wearoff,
int spell_level, int school, int domain, int mageLevel, int clericLevel, int druidLevel,
int rangerLevel, int paladinLevel, int bardLevel) {

  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = CONFIG_LEVEL_CAP;
  for (i = 0; i < NUM_RACES; i++)
    spell_info[spl].race_can_learn[i] = CONFIG_LEVEL_CAP;
  spell_info[spl].mana_max = max_mana;
  spell_info[spl].mana_min = min_mana;
  spell_info[spl].mana_change = mana_change;
  spell_info[spl].ki_max = 0;
  spell_info[spl].ki_min = 0;
  spell_info[spl].ki_change = 0;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
  spell_info[spl].name = name;
  spell_info[spl].wear_off_msg = wearoff;
  spell_info[spl].skilltype = SKTYPE_SPELL;
  spell_info[spl].flags = 0;
  spell_info[spl].save_flags = save_flags;
  spell_info[spl].comp_flags = comp_flags;
  spell_info[spl].spell_level = spell_level;
  spell_info[spl].school = school;
  spell_info[spl].domain = domain;
  if (IS_SET(spell_info[spl].routines, MAG_DAMAGE))
    spell_info[spl].hate = 0;
   else
    spell_info[spl].hate = (spell_level * 8);

  spell_info[spl].class_level[CLASS_WIZARD] = mageLevel;
  spell_info[spl].class_level[CLASS_SORCERER] = mageLevel;
  spell_info[spl].class_level[CLASS_FAVORED_SOUL] = clericLevel;
  spell_info[spl].class_level[CLASS_CLERIC] = clericLevel;
  spell_info[spl].class_level[CLASS_DRUID] = druidLevel;
  spell_info[spl].class_level[CLASS_PALADIN] = paladinLevel;
  spell_info[spl].class_level[CLASS_RANGER] = rangerLevel;
  spell_info[spl].class_level[CLASS_BARD] = bardLevel;


}


void arto(int spl, const char *name, int max_ki, int min_ki, int ki_change, int minpos, int targets, int violent, int routines, int save_flags, int comp_flags, const char *wearoff)
{
  spello(spl, name, 0, 0, 0, minpos, targets, violent, routines, save_flags, comp_flags, wearoff, 0, 0, 0, 99, 99, 99, 99, 99, 99);
  set_skill_type(spl, SKTYPE_ART);
  spell_info[spl].ki_max = max_ki;
  spell_info[spl].ki_min = min_ki;
  spell_info[spl].ki_change = ki_change;
}


void unused_spell(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    spell_info[spl].min_level[i] = CONFIG_LEVEL_CAP;
    spell_info[spl].can_learn_skill[i] = SKLEARN_CROSSCLASS;
  }
  for (i = 0; i < NUM_RACES; i++)
    spell_info[spl].race_can_learn[i] = SKLEARN_CROSSCLASS;
  spell_info[spl].mana_max = 0;
  spell_info[spl].mana_min = 0;
  spell_info[spl].mana_change = 0;
  spell_info[spl].ki_max = 0;
  spell_info[spl].ki_min = 0;
  spell_info[spl].ki_change = 0;
  spell_info[spl].min_position = 0;
  spell_info[spl].targets = 0;
  spell_info[spl].violent = 0;
  spell_info[spl].routines = 0;
  spell_info[spl].name = unused_spellname;
  spell_info[spl].skilltype = SKTYPE_NONE;
  spell_info[spl].flags = 0;
  spell_info[spl].save_flags = 0;
  spell_info[spl].comp_flags = 0;
  spell_info[spl].spell_level = 0;
  spell_info[spl].school = 0;
  spell_info[spl].domain = 0;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].class_level[i] = 99;
  spell_info[spl].bard_song = FALSE;
  spell_info[spl].perform_rank = 0;
  spell_info[spl].bard_feat = 0;
  spell_info[spl].elemental = FALSE;
}

void bard_song(int spellnum, int rank, int feat) {
  spell_info[spellnum].bard_song = TRUE;
  spell_info[spellnum].perform_rank = rank;
  spell_info[spellnum].bard_feat = feat;

}

void skillo(int skill, const char *name, int flags)
{
  spello(skill, name, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 99, 99, 99, 99, 99, 99);
  spell_info[skill].skilltype = SKTYPE_SKILL;
  spell_info[skill].flags = flags;
}

void artisan_skill(int skill, int type) {
  spell_info[skill].artisan_type = type;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  this
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  true or false, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * spellname: The name of the spell.
 *
 * school: The school of the spell.
 *
 * domain: The domain of the spell.
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    unused_spell(i);
  /* Do not change the loop above. */

  spello(SPELL_CLAN_RECALL, "clan recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
        NULL,
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_ANIMATE_DEAD, "animate dead", 35, 10, 3, POS_FIGHTING,
	TAR_OBJ_ROOM, false,
	MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	3, SCHOOL_NECROMANCY, DOMAIN_DEATH,
	4, 3, 99, 99, 99, 99);

  spello(SPELL_SUMMON_UNDEAD, "summon undead", 35, 10, 3, POS_FIGHTING,
	TAR_IGNORE, false,
	MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	6, SCHOOL_NECROMANCY, DOMAIN_DEATH | DOMAIN_EVIL,
	6, 6, 99, 99, 99, 99);

  spello(SPELL_SUMMON_GREATER_UNDEAD, "summon greater undead", 35, 10, 3, POS_FIGHTING,
	TAR_IGNORE, false,
	MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	8, SCHOOL_NECROMANCY, DOMAIN_DEATH,
	8, 8, 99, 99, 99, 99);

  spello(SPELL_FLOATING_DISC, "floating disc", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your floating disc winks out of existance.",
	1, SCHOOL_CONJURATION, 0,
	1, 99, 99, 99, 99, 99);

  spello(SPELL_MAGE_ARMOR, "mage armor", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less protected.",
	1, SCHOOL_CONJURATION, DOMAIN_SPELL,
	1, 99, 99, 99, 99, 99);
  
  epic_spell(SPELL_EPIC_MAGE_ARMOR, 46);
  spello(SPELL_EPIC_MAGE_ARMOR, "epic mage armor", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The sphere of protection dissipates.",
	10, SCHOOL_CONJURATION, 0,
	10, 10, 10, 99, 99, 99);

  spello(SPELL_HASTE, "haste", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel the magical haste leave you.",
	3, SCHOOL_CONJURATION, DOMAIN_TIME,
	3, 99, 99, 99, 99, 99);

  spello(SPELL_KEEN_EDGE, "keen edge", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your bladed weapons lose their keen edge.",
	3, SCHOOL_TRANSMUTATION, 0,
	3, 99, 99, 99, 99, 99);

  spello(SPELL_WEAPON_OF_IMPACT, "weapon of impact", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your blunt weapons lose their force of impact.",
	3, SCHOOL_TRANSMUTATION, 0,
	3,  3, 99, 99, 99, 3);

  spello(SPELL_FLY, "fly", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your magical ability to fly comes to an end.",
	3, SCHOOL_CONJURATION, 0,
	3, 99, 99, 99, 99, 99);

  spello(SPELL_LEVITATE, "levitate", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your magical ability to levitate comes to an end.",
	2, SCHOOL_CONJURATION, 0,
	2, 99, 99, 99, 99, 99);

  spello(SPELL_BLUR, "blur", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You notice the blurred outline of your body fade away.",
	2, SCHOOL_CONJURATION, 0,
	2, 99, 99, 99, 99, 99);

  spello(SPELL_BLESS, "bless", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel less righteous.",
	1, SCHOOL_ENCHANTMENT, 0,
	99, 1, 99, 99, 1, 99);

  spello(SPELL_PRAYER, "prayer", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The effect of the prayer leaves you with a blessed power.",
	3, 0, 0,
	99, 3, 99, 99, 3, 99);

  spello(SPELL_BEARS_ENDURANCE, "bears endurance", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The endurance of the bear has left you.",
	2, SCHOOL_TRANSMUTATION, 0,
 	2, 2, 2, 2, 99, 99);

  spello(SPELL_MASS_BEARS_ENDURANCE, "mass bears endurance", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The endurance of the bear has left you.",
	6, SCHOOL_TRANSMUTATION, 0,
 	6, 6, 6, 99, 99, 99);

  spello(SPELL_AID, "aid", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The aid of the Gods has left you.",
	2, SCHOOL_UNDEFINED, DOMAIN_GOOD | DOMAIN_LUCK,
        99, 2, 99, 99, 99, 99);

  spello(SPELL_MASS_AID, "mass aid", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The aid of the Gods has left you.",
	3, SCHOOL_UNDEFINED, DOMAIN_GOOD | DOMAIN_LUCK,
        99, 3, 99, 99, 99, 99);

  spello(SPELL_ENLARGE_PERSON, "enlarge person", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You return to your normal size.",
	1, SCHOOL_TRANSMUTATION, DOMAIN_STRENGTH,
        1, 99, 99, 99, 99, 99);

  spello(SPELL_MASS_ENLARGE_PERSON, "mass enlarge person", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You return to your normal size.",
	4, SCHOOL_TRANSMUTATION, 0,
        4, 99, 99, 99, 99, 99);

  spello(SPELL_DELAY_POISON, "delay poison", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now succeptable to poisons once again.",
	2, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
	99, 2, 2, 2, 2, 2);

  spello(SPELL_EAGLES_SPLENDOR, "eagles splendor", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The splendor of the eagle has left you.",
	2, SCHOOL_TRANSMUTATION, 0,
	2, 2, 99, 99, 2, 2);

  spello(SPELL_MASS_EAGLES_SPLENDOR, "mass eagles splendor", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The splendor of the eagle has left you.",
	6, SCHOOL_TRANSMUTATION, 0,
	6, 99, 99, 99, 99, 6);

  spello(SPELL_OWLS_WISDOM, "owls wisdom", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The wisdom of the owl has left you.",
	2, SCHOOL_TRANSMUTATION, 0,
	2, 2, 2, 2, 2, 99);

  spello(SPELL_MASS_OWLS_WISDOM, "mass owls wisdom", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The wisdom of the owl has left you.",
	6, SCHOOL_TRANSMUTATION, 0,
	6, 6, 6, 99, 99, 99);

  spello(SPELL_REMOVE_PARALYSIS, "remove paralysis", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_UNAFFECTS | MAG_GROUPS, 0, 0,
	"You are no longer paralyzed.",
	2, SCHOOL_ENCHANTMENT, 0,
	99, 2, 99, 99, 2, 99);

  spello(SPELL_SILENCE, "silence", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The veil of silence has lifted.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_SPELL,
	99, 2, 99, 99, 99, 2);

  spello(SPELL_UNDETECTABLE_ALIGNMENT, "undetectable alignment", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your alignment is now detectable again.",
	2, SCHOOL_TRANSMUTATION, 0,
	99, 2, 99, 99, 2, 1);

  spello(SPELL_HOLD_PERSON, "hold person", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"The feeling returns to your body again and you are once again able to move freely.",
	2, SCHOOL_ENCHANTMENT, 0,
	3, 2, 99, 99, 99, 2);

  spello(SPELL_HOLD_MONSTER, "hold monster", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"The feeling returns to your body again and you are once again able to move freely.",
	6, SCHOOL_ENCHANTMENT, DOMAIN_LAW,
	5, 99, 99, 99, 99, 4);

  spello(SPELL_SLOW, "slow", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The magical slowness leaves your body and you return to normal speed.",
	3, SCHOOL_ENCHANTMENT, 0,
	3, 99, 99, 99, 99, 99);

  spello(SPELL_DEATH_KNELL, "death knell", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"The life force of the slain has left you..",
	2, SCHOOL_NECROMANCY, DOMAIN_DEATH,
	99, 2, 99, 99, 99, 99);

  spello(SPELL_INFLICT_MODERATE, "inflict moderate", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
	0,
	2, SCHOOL_UNDEFINED, 0,
	99, 2, 99, 99, 99, 99);

  spello(SPELL_SLAY_LIVING, "slay living", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_HALF | MAGSAVE_DEATH, 0,
	0,
	5, SCHOOL_UNDEFINED, 0,
	99, 5, 5, 99, 99, 99);

  spello(SPELL_FINGER_OF_DEATH, "finger of death", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_HALF | MAGSAVE_DEATH, 0,
	0,
	7, SCHOOL_NECROMANCY, 0,
	7, 99, 8, 99, 99, 99);

  spello(SPELL_WAIL_OF_THE_BANSHEE, "wail of the banshee", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE | MAG_AREAS, MAGSAVE_FORT | MAGSAVE_NONE | MAGSAVE_DEATH, 0,
	0,
	9, SCHOOL_NECROMANCY, 0,
	9, 99, 99, 99, 99, 99);

  spello(SPELL_INFLICT_SERIOUS, "inflict serious", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
	0,
	2, SCHOOL_UNDEFINED, 0,
	99, 3, 99, 99, 99, 99);

  spello(SPELL_INFLICT_MINOR, "inflict minor", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
	0,
	0, SCHOOL_UNDEFINED, 0,
	99, 0, 99, 99, 99, 99);


  spello(SPELL_SOUND_BURST, "sound burst", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
	"The effects of the sound burst's shockwave has left you and you are free to act again.",
	2, SCHOOL_TRANSMUTATION, 0,
        99, 2, 99, 99, 99, 2);
  elemental_spell(SPELL_SOUND_BURST);

  spello(SPELL_BLESS_SINGLE, "bless single", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less righteous.",
	10, SCHOOL_ENCHANTMENT, 0,
	99, 99, 99, 99, 99, 99);

  spello(SPELL_DEATH_WARD, "death ward", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel your immunity to death effects fade away.",
	4, SCHOOL_ABJURATION, DOMAIN_DEATH,
	99, 4, 5, 99, 4, 99);

  spello(SPELL_BLESS_OBJECT, "bless object", 35, 5, 3, POS_FIGHTING,
	TAR_OBJ_INV, false,
	MAG_ACTION_FULL | MAG_ALTER_OBJS, 0, 0,
	"You feel less righteous.",
	1, SCHOOL_ENCHANTMENT, 0,
        99, 1, 99, 99, 99, 99);

  spello(SPELL_MENDING, "mending", 35, 5, 3, POS_FIGHTING,
	TAR_OBJ_INV, false,
	MAG_ACTION_FULL | MAG_ALTER_OBJS, 0, 0,
	0,
	0, SCHOOL_TRANSMUTATION, 0,
        0, 0, 0, 99, 99, 0);

  spello(SPELL_BLESS_WEAPON, "bless weapon", 35, 5, 3, POS_FIGHTING,
	TAR_OBJ_INV, false,
	MAG_ACTION_FULL | MAG_ALTER_OBJS, 0, 0,
	"You feel less righteous.",
	1, SCHOOL_ENCHANTMENT, 0,
        99, 99, 99, 99, 1, 99);

  spello(SPELL_SHIELD_OF_FAITH, "shield of faith", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
	"The shield of faith disperses.",
	1, SCHOOL_ABJURATION, 0,
	99, 1, 99, 99, 99, 99);

  spello(SPELL_DIVINE_FAVOR, "divine favor", 35, 5, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
	MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
	"The favor of your deity has left you.",
	1, SCHOOL_EVOCATION, 0,
	99, 1, 99, 99, 1, 99);

  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, true,
	MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
	"You feel a cloak of blindness dissolve.",
	2, SCHOOL_NECROMANCY, DOMAIN_DARKNESS,
	2, 3, 99, 99, 99, 2);

  spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	3, SCHOOL_EVOCATION, 0,
        3, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_FIREBALL);


  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	1, SCHOOL_TRANSMUTATION, DOMAIN_FIRE,
	1, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_BURNING_HANDS);


  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, true,
	MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	"You no longer have control over the lightning of the air.",
	3, SCHOOL_EVOCATION, 0,
	99, 99, 3, 99, 99, 99);
  elemental_spell(SPELL_CALL_LIGHTNING);


  spello(SPELL_INFLICT_CRITIC, "inflict critic", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_HALF, 0,
	NULL,
	4, SCHOOL_NECROMANCY, DOMAIN_DESTRUCTION,
	99, 4, 99, 99, 99, 99);

  spello(SPELL_INFLICT_LIGHT, "inflict light", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_HALF, 0,
	NULL,
	1, SCHOOL_NECROMANCY, DOMAIN_DESTRUCTION,
	99, 1, 99, 99, 99, 99);

  spello(SPELL_CHARM, "charm person", 75, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, true,
	MAG_ACTION_FULL | MAG_MANUAL, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel more self-confident.",
	1, SCHOOL_ENCHANTMENT, 0,
	1, 99, 99, 99, 99, 1);

  spello(SPELL_CHARM_ANIMAL, "charm animal", 75, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, true,
	MAG_ACTION_FULL | MAG_MANUAL, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel more in control of yourself",
	1, SCHOOL_ENCHANTMENT, 0,
	99, 99, 1, 1, 99, 99);

  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
	"You feel your strength return.",
	1, SCHOOL_NECROMANCY, 0,
	1, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_CHILL_TOUCH);


  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	NULL,
	1, SCHOOL_ILLUSION, 0,
	1, 99, 99, 99, 99, 99);

  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
	TAR_IGNORE, false,
	MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	7, SCHOOL_TRANSMUTATION, DOMAIN_AIR,
	7, 7, 7, 99, 99, 99);

  spello(SPELL_CREATE_FOOD, "create food and water", 30, 5, 4, POS_STANDING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_CREATIONS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, 0,
	99, 3, 99, 99, 99, 99);

  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
	TAR_OBJ_INV | TAR_OBJ_EQUIP, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	0, SCHOOL_UNDEFINED, 0,
	99, 99, 99, 99, 99, 99);

  spello(SPELL_REMOVE_BLINDNESS, "remove blindness", 30, 5, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_UNAFFECTS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, 0,
	99, 3, 99, 99, 3, 99);

  spello(SPELL_CURE_CRITIC, "cure critical wounds", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	4, SCHOOL_UNDEFINED, DOMAIN_HEALING,
	99, 4, 5, 99, 99, 4);

  spello(SPELL_CURE_LIGHT, "cure light wounds", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	1, SCHOOL_UNDEFINED, DOMAIN_HEALING,
	99, 1, 1, 2, 1, 1);

  spello(SPELL_CURE_MODERATE, "cure moderate wounds", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	2, SCHOOL_UNDEFINED, DOMAIN_HEALING,
	99, 2, 3, 3, 3, 2);

  spello(SPELL_CURE_SERIOUS, "cure serious wounds", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, DOMAIN_HEALING,
	99, 3, 4, 4, 4, 3);

  spello(SPELL_MINOR_REFRESH, "refresh minor exhaustion", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	0, SCHOOL_UNDEFINED, 0,
	99, 0, 0, 99, 99, 99);

  spello(SPELL_LIGHT_REFRESH, "refresh light exhaustion", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	1, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
	99, 1, 1, 1, 99, 1 );

  spello(SPELL_MODERATE_REFRESH, "refresh moderate exhaustion", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	2, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
	99, 2, 2, 2, 99, 2);

  spello(SPELL_SERIOUS_REFRESH, "refresh serious exhaustion", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
	99, 3, 3, 3, 99, 3);

  spello(SPELL_CRITICAL_REFRESH, "refresh critical exhaustion", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	4, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
        99, 4, 4, 4, 99, 4);

  spello(SPELL_REJUVENATE, "rejuvenate", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
	NULL,
	6, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
        99, 6, 6, 99, 99, 6);

  spello(SPELL_LESSER_MASS_REJUVENATE, "lesser mass rejuvenate", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_POINTS | MAG_GROUPS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
        99, 3, 3, 3, 99, 3);

  spello(SPELL_MINOR_MASS_REJUVENATE, "minor mass rejuvenate", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_POINTS | MAG_GROUPS, 0, 0,
	NULL,
	5, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
        99, 5, 5, 5, 99, 5);

  spello(SPELL_MASS_REJUVENATE, "mass rejuvenate", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_POINTS | MAG_GROUPS, 0, 0,
	NULL,
	7, SCHOOL_UNDEFINED, DOMAIN_TRAVEL,
        99, 7, 7, 99, 99, 99);

  spello(SPELL_BESTOW_CURSE, "bestow curse", 80, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, true,
        MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel your curse fade away.",
	8, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        4, 3, 99, 99, 99, 3);

  spello(SPELL_BESTOW_CURSE_PENALTIES, "bestow curse penalties", 80, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, true,
        MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel your curse fade away.",
	99, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_BESTOW_CURSE_DAZE, "bestow curse daze", 80, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, true,
        MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel your curse fade away.",
	99, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less aware.",
	1, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        99, 1, 99, 2, 99, 99);

  spello(SPELL_SEE_INVIS, "see invisibility", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your eyes stop tingling.",
	2, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        2, 99, 99, 99, 99, 2);

  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The detect magic wears off.",
	0, SCHOOL_UNIVERSAL, DOMAIN_UNDEFINED,
        0, 0, 0, 99, 99, 0);

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	"The detect poison wears off.",
	0, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        0, 0, 0, 99, 1, 99);

  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	NULL,
	5, SCHOOL_UNDEFINED, DOMAIN_GOOD,
        99, 5, 99, 4, 99, 99);

  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	NULL,
	5, SCHOOL_UNDEFINED, DOMAIN_EVIL,
        99, 5, 99, 99, 99, 99);

  spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_FIGHTING,
	TAR_IGNORE, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	8, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION | DOMAIN_EARTH,
        99, 8, 9, 99, 99, 99);

  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
	TAR_OBJ_INV, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	9, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
         99, 99, 99, 99, 99, 99);

  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
        MAG_DAMAGE | MAG_MANUAL, MAGSAVE_FORT | MAGSAVE_NONE, 0, NULL,
	9, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED,
        9, 9, 99, 99, 99, 99);

  spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
	NULL,
	5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FAERIE_FIRE, "faerie fire", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTS | MAG_UNAFFECTS,  0, 0,
	NULL,
	1, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        99, 99, 1, 99, 99, 99);

  spello(SPELL_ENTANGLE, "entangle", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_MASSES, MAGSAVE_NONE, 0,
	"The entangled plants around you crumble to dust.",
	1, SCHOOL_EVOCATION, DOMAIN_PLANT,
        99, 99, 1, 1, 99, 99);

  spello(SPELL_ENTANGLED, "entangled", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_NONE, 0,
	"You are no longer entangled",
	1, SCHOOL_EVOCATION, DOMAIN_PLANT,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FLAMESTRIKE, "flamestrike", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	5, SCHOOL_EVOCATION, DOMAIN_SUN | DOMAIN_WAR,
        99, 5, 4, 99, 99, 99);
  elemental_spell(SPELL_FLAMESTRIKE);


  spello(SPELL_FIRE_STORM, "fire storm", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	7, SCHOOL_EVOCATION, DOMAIN_FIRE,
        99, 8, 7, 99, 99, 99);
  elemental_spell(SPELL_FIRE_STORM);


  spello(SPELL_BREATH_WEAPON, "breath weapon", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	99, SCHOOL_EVOCATION, 0,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_DRAGON_MOUNT_BREATH, "dragon breath", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	99, SCHOOL_EVOCATION, 0,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_ICE_STORM, "ice storm", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	4, SCHOOL_EVOCATION, 0,
        4, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_ICE_STORM);


  spello(SPELL_CONE_OF_COLD, "cone of cold", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	5, SCHOOL_EVOCATION, 0,
        5, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_CONE_OF_COLD);



  spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	6, SCHOOL_EVOCATION, 0,
        6, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_CHAIN_LIGHTNING);


  spello(SPELL_DISINTIGRATE, "disintigrate", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_HALF, 0,
	NULL,
	6, SCHOOL_EVOCATION, 0,
        6, 99, 99, 99, 99, 99);

  spello(SPELL_DELAYED_BLAST_FIREBALL, "delayed blast fireball", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	7, SCHOOL_EVOCATION, 0,
        7, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_DELAYED_BLAST_FIREBALL);


  spello(SPELL_HORRID_WILTING, "horrid wilting", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_FORT | MAGSAVE_HALF, 0,
	NULL,
	8, SCHOOL_EVOCATION, 0,
        8, 99, 99, 99, 99, 99);

  spello(SPELL_METEOR_SWARM, "meteor swarm", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_LOOP, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	9, SCHOOL_EVOCATION, 0,
        9, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_METEOR_SWARM);


  spello(SPELL_METEOR_SWARM_AREA, "meteor swarm", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	99, SCHOOL_EVOCATION, 0,
        99, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_METEOR_SWARM_AREA);


  spello(SPELL_MASS_HEAL, "mass heal", 80, 60, 5, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
	NULL,
	7, SCHOOL_CONJURATION, DOMAIN_HEALING,
        99, 8, 9, 99, 99, 99);

  spello(SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_HALF, 0,
	NULL,
	6, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION,
        99, 6, 7, 99, 99, 99);

  spello(SPELL_MASS_HARM, "mass harm", 75, 45, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
	NULL,
	7, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION,
        99, 8, 9, 99, 99, 99);

  spello(SPELL_HEAL_MOUNT, "heal mount", 60, 40, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS | MAG_UNAFFECTS, 0, 0,
	NULL,
	3, SCHOOL_UNDEFINED, 0,
        99, 99, 99, 99, 3, 99);

  spello(SPELL_HEAL, "heal", 60, 40, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS | MAG_UNAFFECTS, 0, 0,
	NULL,
	6, SCHOOL_UNDEFINED, DOMAIN_HEALING,
        99, 6, 7, 99, 99, 99);

  spello(SPELL_IDENTIFY, "identify", 50, 25, 5, POS_FIGHTING,
	TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	1, SCHOOL_DIVINATION, DOMAIN_MAGIC,
        1, 99, 99, 99, 99, 1);

  spello(SPELL_RAISE_DEAD, "raise dead", 50, 25, 5, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	5, SCHOOL_TRANSMUTATION, 0,
        99, 5, 99, 99, 99, 99);

  spello(SPELL_REINCARNATE, "reincarnate", 50, 25, 5, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	4, SCHOOL_TRANSMUTATION, 0,
        99, 99, 4, 99, 99, 99);

  spello(SPELL_RESURRECTION, "resurrection", 50, 25, 5, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	7, SCHOOL_TRANSMUTATION, 0,
        99, 7, 99, 99, 99, 99);

  spello(SPELL_TRUE_RESURRECTION, "true resurrection", 50, 25, 5, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	9, SCHOOL_TRANSMUTATION, 0,
        99, 9, 99, 99, 99, 99);

  spello(SPELL_DISPEL_MAGIC, "dispel magic", 50, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	3, SCHOOL_TRANSMUTATION, DOMAIN_MAGIC,
        3, 3, 4, 99, 3, 3);

  spello(SPELL_DARKVISION, "darkvision", 25, 10, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your night vision seems to fade.",
	2, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED,
        2, 99, 99, 99, 99, 99);

  spello(SPELL_LIGHT, "ball of light", 25, 10, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The ball of light winks out of existance",
	0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        0, 0, 0, 99, 99, 0);

  spello(SPELL_DAYLIGHT, "daylight", 25, 10, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The aura of daylight fades out of existance",
	3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        3, 3, 3, 99, 3, 3);

  spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
	"You feel yourself exposed.",
	2, SCHOOL_ILLUSION, DOMAIN_TRICKERY,
        2, 99, 99, 99, 99, 2);

  spello(SPELL_FREEDOM_OF_MOVEMENT, "freedom of movement", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are no longer immune to holding effects.",
	4, SCHOOL_ABJURATION, DOMAIN_LUCK,
        4, 4, 4, 4, 99, 4);

  spello(SPELL_FIRE_SHIELD, "fire shield", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The wreath of flames surrounding you winks out of existence.",
	4, SCHOOL_EVOCATION, DOMAIN_FIRE | DOMAIN_SUN | DOMAIN_RETRIBUTION,
        4, 99, 99, 99, 99, 99);

  spello(SPELL_GREATER_INVISIBILITY, "greater invisibility", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
	"You feel yourself exposed.",
	4, SCHOOL_ILLUSION, DOMAIN_TRICKERY,
        4, 99, 99, 99, 99, 4);

  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
	NULL,
	3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        3, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_LIGHTNING_BOLT);


  spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_FIGHTING,
	TAR_OBJ_WORLD, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	3, SCHOOL_DIVINATION, DOMAIN_TRAVEL,
        2, 3, 99, 99, 99, 2);

  spello(SPELL_WISH, "wish", 25, 20, 1, POS_STANDING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	9, SCHOOL_UNIVERSAL, 0,
        9, 99, 99, 99, 99, 99);

  spello(SPELL_WISH_RING, "ring of wishes", 25, 20, 1, POS_STANDING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	9, SCHOOL_UNIVERSAL, 0,
        9, 99, 99, 99, 99, 99);

  spello(SPELL_MIRACLE, "miracle", 25, 20, 1, POS_STANDING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	9, SCHOOL_UNIVERSAL, DOMAIN_LUCK,
        99, 9, 99, 99, 99, 99);		
		
  spello(SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_LOOP, 0, 0,
	NULL,
	1, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        1, 99, 99, 99, 99, 99);

  spello(SPELL_MISSILE_SWARM, "missile swarm", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, 0, 0,
	NULL,
	4, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
	4, 99, 99, 99, 99, 99);

  spello(SPELL_GREATER_MISSILE_SWARM, "greater missile swarm", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_ACTION_FULL | MAG_DAMAGE, 0, 0,
	NULL,
	6, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
	6, 99, 99, 99, 99, 99);

  spello(SPELL_ACID_ARROW, "acid arrow", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, 0, 0,
	NULL,
	2, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        2, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_ACID_ARROW);

  spello(SPELL_ON_FIRE, "on fire", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, 0, 0,
	"You are no longer on fire.",
	99, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_ON_FIRE);

  spello(SPELL_WHIRLWIND, "whirlwind", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, 0, 0,
	"You are no longer caught in a whirlwind",
	99, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_VORTEX, "vortex", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, 0, 0,
	"You are no longer in a vortex",
	99, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);


  spello(SPELL_SCORCHING_RAY, "scorching ray", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_LOOP, 0, 0,
        NULL,
	2, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        2, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_SCORCHING_RAY);


  spello(SPELL_FLAMING_SPHERE, "flaming sphere", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, MAGSAVE_REFLEX | MAGSAVE_NONE, 0,
	NULL,
	2, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        2, 99, 2, 99, 99, 99);
  elemental_spell(SPELL_FLAMING_SPHERE);


  spello(SPELL_FLAME_ARROW, "flame arrow", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_LOOP, 0, 0,
	NULL,
	3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        3, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_FLAME_ARROW);


  spello(SPELL_CALL_LIGHTNING_BOLT, "called lightning bolt", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
	MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
        NULL,
	3, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_CALL_LIGHTNING_BOLT);



  spello(SPELL_PARALYZE, "paralyze", 25, 10, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
	"Your feel that your limbs will move again.",
	10, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        99,99,99,99,99,99);

  spello(SPELL_POISON, "poison", 50, 20, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, true,
        MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
	"Your body finally passes the venoms from your body.",
	4, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED,
        99, 4, 3, 99, 99, 99);


  spello(SPELL_POISON_TIMER, "poison timer", 50, 20, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, true,
	0, 0, 0,
	"You are no longer at risk to more damage from the poison.",
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_PORTAL, "portal", 75, 75, 0, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL,  0, 0,
	NULL,
	8, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        8, 8, 99, 99, 99, 99);

  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less protected from evil.",
	1, SCHOOL_ABJURATION, DOMAIN_GOOD,
        1, 1, 99, 99, 1, 1);

  spello(SPELL_MAGIC_CIRCLE_AGAINST_EVIL, "magic circle against evil", 40, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel less protected from evil.",
	3, SCHOOL_ABJURATION, DOMAIN_LAW,
        3, 3, 99, 99, 3, 99);

  spello(SPELL_AURA_OF_GOOD, "aura of good", 40, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The paladin's aura of good has dissipated.",
	1, SCHOOL_ABJURATION, DOMAIN_UNIVERSAL,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_STUNNED, "stunned", 40, 10, 3, POS_STANDING,
	TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are no longer stunned!",
	999, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_TRIP, "tripped", 40, 10, 3, POS_STANDING,
	TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are no longer stunned!",
	999, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_TRIPPING, "tripping", 40, 10, 3, POS_STANDING,
	TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are no longer stunned!",
	999, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AURA_OF_COURAGE, "aura of courage", 40, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"The paladin's aura of courage has dissipated.",
	1, SCHOOL_ABJURATION, DOMAIN_UNIVERSAL,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, false,
	MAG_ACTION_FULL | MAG_UNAFFECTS | MAG_ALTER_OBJS, 0, 0,
	NULL,
	3, SCHOOL_ABJURATION, DOMAIN_UNDEFINED,
        4, 3, 99, 99, 3, 3);

  spello(SPELL_NEUTRALIZE_POISON, "neutralize poison", 40, 8, 4, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_UNAFFECTS | MAG_ALTER_OBJS, 0, 0,
	NULL,
	4, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 4, 3, 3, 4, 4);

  spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The white aura around your body fades.",
	9, SCHOOL_UNDEFINED, DOMAIN_PROTECTION,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less aware of your surroundings.",
	2, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, 0, 0,
	NULL,
	1, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        1, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_SHOCKING_GRASP);


  spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS | MAG_MASSES, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel less tired.",
	1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED,
        1, 99, 99, 2, 99, 1);

  spello(SPELL_DEEP_SLUMBER, "deep slumber", 40, 25, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS | MAG_MASSES, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel less tired.",
	3, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED,
        3, 99, 99, 99, 99, 3);

  spello(SPELL_SLEEP_SINGLE, "sleep single", 40, 25, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
	"You feel less tired.",
	1, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_MASS_BULLS_STRENGTH, "mass bulls strength", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel the strength of the bull leave you.",
	6, SCHOOL_TRANSMUTATION, DOMAIN_STRENGTH,
        6, 6, 6, 99, 99, 99);

  spello(SPELL_BULL_STRENGTH, "bulls strength", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel the strength of the bull leave you.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_STRENGTH,
        2, 2, 2, 99, 2, 2);

  spello(SPELL_BARKSKIN, "barkskin", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your skin is no longer strengthened with hardness of bark.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_PLANT,
        99, 99, 2, 2, 99, 99);

  spello(SPELL_FLAME_WEAPON, "flame weapon", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You flames surrounding your weapons wink out of existance.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_FIRE,
        99, 99, 2, 99, 99, 99);

  spello(SPELL_SHILLELAGH, "shillelagh", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The shillelagh enhancement to your weapons expires.",
	1, SCHOOL_TRANSMUTATION, 0,
        99, 99, 1, 99, 99, 99);

  spello(SPELL_CALM_ANIMAL, "calm animals", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"Your calming effect on animals has subsided.",
	1, SCHOOL_TRANSMUTATION, DOMAIN_ANIMAL,
        99, 99, 1, 1, 99, 99);

  spello(SPELL_MAGIC_WEAPON, "magic weapon", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your weapon enhancements have expired.",
	1, SCHOOL_TRANSMUTATION, DOMAIN_WAR | DOMAIN_DWARF,
        1, 1, 99, 99, 1, 99);

  spello(SPELL_GREATER_MAGIC_WEAPON, "greater magic weapon", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your greater weapon enhancements have expired.",
	3, SCHOOL_TRANSMUTATION, DOMAIN_DWARF,
        3, 4, 99, 99, 3, 99);

  spello(SPELL_MAGIC_FANG, "magic fang", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your natural weapon enhancements have expired.",
	1, SCHOOL_TRANSMUTATION, DOMAIN_ANIMAL,
        99, 99, 1, 1, 99, 99);

  spello(SPELL_GREATER_MAGIC_FANG, "greater magic fang", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your greater natural weapon enhancements have expired.",
	3, SCHOOL_TRANSMUTATION, DOMAIN_ANIMAL,
        99, 99, 3, 3, 99, 99);

  spello(SPELL_MAGIC_VESTMENT, "magic vestment", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your magic vestment enhancements have expired.",
	3, SCHOOL_TRANSMUTATION, 0,
        99, 3, 99, 99, 3, 99);

  spello(SPELL_FOX_CUNNING, "foxs cunning", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less cunning.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        2, 99, 99, 99, 99, 2);

  spello(SPELL_MASS_FOXS_CUNNING, "mass foxs cunning", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel less cunning.",
	6, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        6, 99, 99, 99, 99, 6);

  spello(SPELL_CAT_GRACE, "cats grace", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less agile.",
	2, SCHOOL_TRANSMUTATION, DOMAIN_ELF,
        2, 99, 2, 2, 99, 2);

  spello(SPELL_MASS_CATS_GRACE, "mass cats grace", 35, 30, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_GROUPS, 0, 0,
	"You feel less agile.",
	6, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED,
        6, 99, 6, 99, 99, 6);

  spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_FIGHTING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	7, SCHOOL_CONJURATION, DOMAIN_TRAVEL,
        7, 7, 7, 99, 99, 99);

  spello(SPELL_TELEPORT, "teleport", 75, 50, 3, POS_FIGHTING,
	TAR_CHAR_WORLD, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	5, SCHOOL_TRANSMUTATION, DOMAIN_TRAVEL,
        5, 99, 99, 99, 99, 99);

  spello(SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your feet seem less buoyant.",
	3, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 3, 99, 3, 99, 99);

  spello(SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
	NULL,
	6, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 6, 8, 99, 99, 99);

  arto(ART_STUNNING_FIST, "stunning fist", 5, 10, 1, POS_FIGHTING,
       TAR_CHAR_ROOM | TAR_NOT_SELF, true,
       MAG_NEXTSTRIKE | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_NONE | MAG_ACTION_FREE,
       0, "You are no longer stunned.");

  spell_level(ART_STUNNING_FIST, CLASS_MONK, 1);

  arto(ART_WHOLENESS_OF_BODY, "wholeness of body", 2, 1, 1, POS_FIGHTING,
       TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0, NULL);

  spell_level(ART_WHOLENESS_OF_BODY, CLASS_MONK, 7);

  arto(ART_ABUNDANT_STEP, "abundant step", 120, 100, 8, POS_FIGHTING,
       TAR_IGNORE, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0, NULL);

  spell_level(ART_ABUNDANT_STEP, CLASS_MONK, 12);

  arto(ART_QUIVERING_PALM, "quivering palm", 190, 180, 1, POS_FIGHTING,
       TAR_CHAR_ROOM | TAR_NOT_SELF, true,
       MAG_ACTION_FREE | MAG_NEXTSTRIKE | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_HALF | MAGSAVE_DEATH, 0, NULL);

  spell_level(ART_QUIVERING_PALM, CLASS_MONK, 15);

  arto(ART_EMPTY_BODY, "empty body", 20, 10, 5, POS_FIGHTING,
       TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_ACTION_FREE, 0, 0, NULL);

  spell_level(ART_EMPTY_BODY, CLASS_MONK, 19);

  spello(SPELL_RESISTANCE, "resistance", 40, 20, 0, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
	0, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL | MAGCOMP_DIVINE_FOCUS,
	NULL, 0, SCHOOL_ABJURATION, DOMAIN_UNDEFINED,
        0, 0, 0, 99, 1, 0);

  spello(SPELL_ACID_SPLASH, "acid splash", 40, 20, 0, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE,
	0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL,
	0, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        0, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_ACID_SPLASH);


  spello(SPELL_DAZE, "daze", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS,
	MAGSAVE_WILL | MAGSAVE_NONE, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL, 0, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED,
        0, 99, 99, 99, 99, 0);

  spello(SPELL_FLARE, "flare", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS,
	MAGSAVE_FORT | MAGSAVE_NONE, MAGCOMP_VERBAL,
	NULL,
	0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        0, 99, 0, 99, 99, 0);

  spello(SPELL_RAY_OF_FROST, "ray of frost", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE,
	0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL,
	0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED,
        0, 99, 99, 99, 99, 99);
  elemental_spell(SPELL_RAY_OF_FROST);


  spello(SPELL_DISRUPT_UNDEAD, "disrupt undead", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE,
	0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL,
	0, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED,
        0, 99, 99, 99, 99, 99);

  spello(SPELL_LESSER_GLOBE_OF_INVUL, "lesser globe of invulnerability", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
	0, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_STONESKIN, "stoneskin", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
	0 , MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	"The shell of hard stone flakes off and crumbles away.",
	6, SCHOOL_ABJURATION, DOMAIN_EARTH | DOMAIN_STRENGTH,
        4, 99, 5, 99, 99, 99);

  spello(SPELL_GREATER_STONESKIN, "greater stoneskin", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
	0 , MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	"The shell of thick, hard stone flakes off and crumbles away.",
	6, SCHOOL_ABJURATION, 0,
        6, 99, 6, 99, 99, 99);

  spello(SPELL_PREMONITION, "premonition", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS,
	0 , MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	"Your mentally enhanced defenses fade away.",
	7, SCHOOL_ABJURATION, DOMAIN_TIME,
        8, 99, 8, 99, 99, 99);

  spello(SPELL_MINOR_CREATION, "minor creation", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_PROTECTION,
        99, 99, 99, 99, 99, 99);


  epic_spell(SPELL_DRAGON_KNIGHT, 38);
  spello(SPELL_DRAGON_KNIGHT, "dragon knight", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	10, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        10, 10, 10, 99, 99, 99);

  spello(SPELL_SUMMON_NATURE_I, "summon nature i", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 1, 99, 99, 1);

  spello(SPELL_SUMMON_NATURE_II, "summon nature ii", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        2, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 2, 99, 99, 2);

  spello(SPELL_SUMMON_NATURE_III, "summon nature iii", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        3, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 3, 99, 99, 3);

  spello(SPELL_SUMMON_NATURE_IV, "summon nature iv", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        4, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 4, 99, 99, 4);

  spello(SPELL_SUMMON_NATURE_V, "summon nature v", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 5, 99, 99, 5);

  spello(SPELL_SUMMON_NATURE_VI, "summon nature vi", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        6, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 6, 99, 99, 6);

  spello(SPELL_SUMMON_NATURE_VII, "summon nature vii", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 7, 99, 99, 99);

  spello(SPELL_SUMMON_NATURE_VIII, "summon nature viii", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        8, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 8, 99, 99, 99);

  spello(SPELL_SUMMON_NATURE_IX, "summon nature ix", 40, 20, 2, POS_FIGHTING,
        TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
        NULL,
        9, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        99, 99, 9, 99, 99, 99);


  spello(SPELL_SUMMON_MONSTER_I, "summon monster i", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        1, 1, 99, 99, 99, 1);

  spello(SPELL_SUMMON_MONSTER_II, "summon monster ii", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	2, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        2, 2, 99, 99, 99, 2);

  spello(SPELL_SUMMON_MONSTER_III, "summon monster iii", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	3, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        3, 3, 99, 99, 99, 3);

  spello(SPELL_SUMMON_MONSTER_IV, "summon monster iv", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	4, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        4, 4, 99, 99, 99, 4);

  spello(SPELL_SUMMON_MONSTER_V, "summon monster v", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        5, 5, 99, 99, 99, 5);

  spello(SPELL_SUMMON_MONSTER_VI, "summon monster vi", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	6, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        6, 6, 99, 99, 99, 6);

  spello(SPELL_SUMMON_MONSTER_VII, "summon monster vii", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        7, 7, 99, 99, 99, 99);

  spello(SPELL_SUMMON_MONSTER_VIII, "summon monster viii", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	8, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        8, 8, 99, 99, 99, 99);

  spello(SPELL_SUMMON_MONSTER_IX, "summon monster ix", 40, 20, 2, POS_FIGHTING,
	TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
	NULL,
	9, SCHOOL_CONJURATION, DOMAIN_UNDEFINED,
        9, 9, 99, 99, 99, 99);

  spello(SPELL_COMPREHEND_LANGUAGES, "comprehend languages", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	1, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        1, 1, 99, 99, 99, 99);

  spello(SPELL_TONGUES, "tongues", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	3, SCHOOL_DIVINATION, DOMAIN_UNDEFINED,
        3, 4, 99, 99, 99, 99);


  spello(SPELL_SHOUT, "shout", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FEAR, "fear", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_CLOUDKILL, "cloudkill", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_MAJOR_CREATION, "major creation", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_ANIMAL_GROWTH, "animal growth", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_BALEFUL_POLYMORPH, "baleful polymorph", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_PASSWALL, "passwall", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	10, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  //  These spells are not castable by players.  They are simply in place for skill
  // affects and feat affects and whatnot.  The spello has been set up so they will
  // have proper wear off messages

  spello(SPELL_AFF_RAGE, "rage", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your raging emotions return to normal.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_ACMD_PILFER, "pilfer", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
        "@nYou are now able to use the @Ypilfer@n command again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_LEARNING, "learning", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your learning abilities have diminished.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_IMPROVED_LEARNING, "improved learning", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your improved learning abilities have diminished.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_GREATER_LEARNING, "greater learning", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your greater learning abilities have diminished.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_EPIC_LEARNING, "epic learning", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your epic learning abilities have diminished.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SICKENED, "sickened", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"the waves of nausea finally pass.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_RESIST_SICKENED, "resist sickened", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your resistance against nausea has passed.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_RESIST_FEAR, "resist fear", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your resistance against fear has passed.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(ABILITY_CALL_MOUNT, "call mount", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You may now call your mount again if desired.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(ABIL_LAY_HANDS, "lay hands", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your lay hands ability has been restored.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_STUNNED, "stunned", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your raging emotions return to normal.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_DEATH_ATTACK, "death attack", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your regain the use of your limbs.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_TOUCH_OF_UNDEATH, "touch of undeath", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your regain the use of your limbs.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_WILD_SHAPE, "wild shape", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your wild shape status expires.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_WILD_SHAPE_ELEMENTAL, "elemental wild shape", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your elemental wild shape status expires.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FEAT_DIVINE_MIGHT, "divine might", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The divine might bolstering your soul departs.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FEAT_DIVINE_SHIELD, "divine shield", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The divine shield bolstering your soul departs.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FEAT_DIVINE_VENGEANCE, "divine vengeance", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"The divine vengeance bolstering your soul departs.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_TAUNTED, "taunted", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You get control of yourself from being taunted and act normally again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_FATIGUED, "fatigued", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are no longer fatigued.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_DISARMED, "disarmed", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You recover your weapons and wield them again in the blink of an eye.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SKILL_HEAL_USED, "heal skill", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use the heal skill again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SKILL_CAMP_USED, "camp skill", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to set a camp again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_BARD_SONGS, "bard songs", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your bard songs again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_EPIC_SPELLS, "epic spells", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your epic spells again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SORCERER_SPELLS, "sorcerer spells", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your sorcerer spells again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_FAVORED_SOUL_SPELLS, "favored soul spells", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your favored soul spells again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_ASSASSIN_SPELLS, "assassin spells", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your assassin spells again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_DEFENSIVE_STANCE, "defensive stance", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You are now able to use your defensive stance again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_INTIMIDATED, "intimidated", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"You feel less intimidated and are able to act normally again.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_CHALLENGED, "challenged", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your prideful distraction from being challenged, has departed.",
	99, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_DEFENSIVE_STANCE, "defensive stance", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your defensive stance ends as you run out of endurance to maintain it.",
	0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_AFF_STRENGTH_OF_HONOR, "strength of honor", 0, 0, 0, POS_DEAD,
	TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	"Your heightened strength returns to normal as your intense feelings of honor return to normal.",
	0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_RACIAL_NATURAL_ARMOR, "racial natural armor", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
        "Your natural racial armor has worn thin and will be back to 100% in a few seconds.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_NATURAL_ARMOR_INCREASE, "natural armor increase", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
        "Your natural armor increase has worn thin and will be back to 100% in a few seconds.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_COUNTERSONG, "countersong", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
        "You have been affected by the bardic countersong giving you an increased resistance with saving throws.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  bard_song(SPELL_COUNTERSONG, 3, FEAT_COUNTERSONG);

  spello(SPELL_INSPIRE_COURAGE, "inspire courage", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
        "Your bardic inspired courage has passed.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  bard_song(SPELL_INSPIRE_COURAGE, 3, FEAT_INSPIRE_COURAGE);

  spello(SPELL_INSPIRE_GREATNESS, "inspire greatness", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
        "Your bardic inspired greatness has passed.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  bard_song(SPELL_INSPIRE_GREATNESS, 12, FEAT_INSPIRE_GREATNESS);

  spello(SPELL_INSPIRE_HEROICS, "inspire heroics", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
        "Your bardic inspired courage has passed.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  bard_song(SPELL_INSPIRE_HEROICS, 18, FEAT_INSPIRE_HEROICS);

  spello(SPELL_INSPIRE_COMPETENCE, "inspire competence", 0, 0, 0, POS_DEAD,
        TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
        "Your bardic inspired courage has passed.",
        0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED,
        99, 99, 99, 99, 99, 99);
  bard_song(SPELL_INSPIRE_COMPETENCE, 3, FEAT_INSPIRE_COMPETENCE);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);

  spello(SPELL_SKILL_HEAL_USED, "heal skill used", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);


  spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);


  spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);


  spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);


  spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);


  /* you might want to name this one something more fitting to your theme -Welcor*/
  spello(SPELL_DG_AFFECT, "Script-inflicted", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, true, 0, 0, 0,
	NULL,
	0, 0, 0,
        99, 99, 99, 99, 99, 99);



  assassin_spell(SPELL_SLEEP, 1);
  assassin_spell(SPELL_INVISIBLE, 2);
  assassin_spell(SPELL_CAT_GRACE, 2);
  assassin_spell(SPELL_FOX_CUNNING, 2);
  assassin_spell(SPELL_MAGIC_CIRCLE_AGAINST_EVIL, 3);
  assassin_spell(SPELL_BLINDNESS, 3);
  assassin_spell(SPELL_DEEP_SLUMBER, 3);
  assassin_spell(SPELL_GREATER_INVISIBILITY, 4);
  assassin_spell(SPELL_POISON, 4);


  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */

  /*
   * skillo does spello and then marks the skill as a new style skill with
   * the appropriate flags.

   */
  skillo(SKILL_ACROBATICS, "Acrobatics", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(SKILL_APPRAISE, "Appraise", SKFLAG_INTMOD);
  skillo(SKILL_BLUFF, "Bluff", SKFLAG_CHAMOD);
  skillo(SKILL_COMBAT_TACTICS, "Combat Tactics", SKFLAG_INTMOD);
  skillo(SKILL_DIPLOMACY, "Diplomacy", SKFLAG_CHAMOD);
  skillo(SKILL_DISABLE_DEVICE, "Disable Device", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN | SKFLAG_ARMORBAD);
  skillo(SKILL_DISGUISE, "Disguise", SKFLAG_CHAMOD);
  skillo(SKILL_ESCAPE_ARTIST, "Escape Artist", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(SKILL_HANDLE_ANIMAL, "Handle Animal", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);
  skillo(SKILL_HEAL, "Heal", SKFLAG_WISMOD | SKFLAG_ARMORBAD);
  skillo(SKILL_INTIMIDATE, "Intimidate", SKFLAG_CHAMOD);
  skillo(SKILL_KNOWLEDGE, "Knowledge", SKFLAG_INTMOD);
  skillo(SKILL_LINGUISTICS, "Linguistics", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(SKILL_PERCEPTION, "Perception", SKFLAG_WISMOD);
  skillo(SKILL_PERFORM, "Perform", SKFLAG_CHAMOD);
  skillo(SKILL_RIDE, "Ride", SKFLAG_DEXMOD | SKFLAG_ARMORBAD);
  skillo(SKILL_SENSE_MOTIVE, "Sense Motive", SKFLAG_WISMOD);
  skillo(SKILL_SLEIGHT_OF_HAND, "Sleight of Hand", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(SKILL_SPELLCRAFT, "Spellcraft", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(SKILL_STEALTH, "Stealth", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(SKILL_SURVIVAL, "Survival", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(SKILL_USE_MAGIC_DEVICE, "Use Magic Device", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);

  // All Artisans get this.
  skillo(SKILL_CRAFTING_THEORY, "Crafting Theory", SKFLAG_INTMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_CRAFTING_THEORY, ARTISAN_TYPE_ALL);

  // Smith artisan skills
  skillo(SKILL_BLACKSMITHING, "Blacksmithing", SKFLAG_STRMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_BLACKSMITHING, ARTISAN_TYPE_SMITH);
  skillo(SKILL_GOLDSMITHING, "Goldsmithing", SKFLAG_INTMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_GOLDSMITHING, ARTISAN_TYPE_SMITH);
  skillo(SKILL_MINING, "Mining", SKFLAG_STRMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_MINING, ARTISAN_TYPE_SMITH);

  // Farmer artisan skills
  skillo(SKILL_FARMING, "Farming", SKFLAG_WISMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_FARMING, ARTISAN_TYPE_FARMER);
  skillo(SKILL_TAILORING, "Tailoring", SKFLAG_DEXMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_TAILORING, ARTISAN_TYPE_FARMER);
  skillo(SKILL_COOKING, "Cooking", SKFLAG_WISMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_COOKING, ARTISAN_TYPE_FARMER);

  // Woodsman artisan skills
  skillo(SKILL_WOODWORKING, "Woodworking", SKFLAG_DEXMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_WOODWORKING, ARTISAN_TYPE_WOODSMAN);
  skillo(SKILL_TANNING, "Tanning", SKFLAG_WISMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_TANNING, ARTISAN_TYPE_WOODSMAN);
  skillo(SKILL_FORESTING, "Foresting", SKFLAG_WISMOD | SKFLAG_CRAFT);
  artisan_skill(SKILL_FORESTING, ARTISAN_TYPE_WOODSMAN);

  // Languages
  skillo(SKILL_LANG_COMMON, "common", 0);
  set_skill_type(SKILL_LANG_COMMON, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_THIEVES_CANT, "thieves cant", 0);
  set_skill_type(SKILL_LANG_THIEVES_CANT, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_DRUIDIC, "druidic", 0);
  set_skill_type(SKILL_LANG_DRUIDIC, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_ABYSSAL, "abyssal", 0);
  set_skill_type(SKILL_LANG_ABYSSAL, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_ELVEN, "elven", 0);
  set_skill_type(SKILL_LANG_ELVEN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_GNOME, "gnome", 0);
  set_skill_type(SKILL_LANG_GNOME, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_DWARVEN, "dwarven", 0);
  set_skill_type(SKILL_LANG_DWARVEN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_CELESTIAL, "celestial", 0);
  set_skill_type(SKILL_LANG_CELESTIAL, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_DRACONIC, "draconic", 0);
  set_skill_type(SKILL_LANG_DRACONIC, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_GOBLIN, "goblin", 0);
  set_skill_type(SKILL_LANG_GOBLIN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_GIANT, "giant", 0);
  set_skill_type(SKILL_LANG_GIANT, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_AKLO, "aklo", 0);
  set_skill_type(SKILL_LANG_AKLO, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_AQUAN, "aquan", 0);
  set_skill_type(SKILL_LANG_AQUAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_AURAN, "auran", 0);
  set_skill_type(SKILL_LANG_AURAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_GNOLL, "gnoll", 0);
  set_skill_type(SKILL_LANG_GNOLL, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_IGNAN, "ignan", 0);
  set_skill_type(SKILL_LANG_IGNAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_INFERNAL, "infernal", 0);
  set_skill_type(SKILL_LANG_INFERNAL, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_ORCISH, "orc", 0);
  set_skill_type(SKILL_LANG_ORCISH, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_HALFLING, "halfling", 0);
  set_skill_type(SKILL_LANG_HALFLING, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_UNDERCOMMON, "undercommon", 0);
  set_skill_type(SKILL_LANG_UNDERCOMMON, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_SYLVAN, "sylvan", 0);
  set_skill_type(SKILL_LANG_SYLVAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_TERRAN, "terran", 0);
  set_skill_type(SKILL_LANG_TERRAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_ABOLETH, "aboleth", 0);
  set_skill_type(SKILL_LANG_ABOLETH, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_DROW_SIGN, "drow sign language", 0);
  set_skill_type(SKILL_LANG_DROW_SIGN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_BOGGARD, "boggard", 0);
  set_skill_type(SKILL_LANG_BOGGARD, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_SPHINX, "sphinx", 0);
  set_skill_type(SKILL_LANG_SPHINX, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_STRIX, "strix", 0);
  set_skill_type(SKILL_LANG_STRIX, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_CYCLOPS, "cyclops", 0);
  set_skill_type(SKILL_LANG_CYCLOPS, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_GRIPPLI, "grippli", 0);
  set_skill_type(SKILL_LANG_GRIPPLI, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_TENGU, "tengu", 0);
  set_skill_type(SKILL_LANG_TENGU, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_PROTEAN, "protean", 0);
  set_skill_type(SKILL_LANG_PROTEAN, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_DARK_FOLKS, "dark folks", 0);
  set_skill_type(SKILL_LANG_DARK_FOLKS, SKTYPE_SKILL | SKTYPE_LANG);

  skillo(SKILL_LANG_TREANT, "treant", 0);
  set_skill_type(SKILL_LANG_TREANT, SKTYPE_SKILL | SKTYPE_LANG);

}

ACMD(do_spells) 
{

  int i = 0, j, k = 0, n = 0;
  sbyte spellKnown = FALSE, spellInRepetoire = FALSE;
  char arg[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  int count = 0;
  int spellLevel = -1;
  int tempClass = GET_CLASS(ch);
  int currentClass = -1;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    currentClass = tempClass;
  }
  else {
    for (i = 0; i <= NUM_CLASSES; i++) {
      if (is_abbrev(arg, (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? class_names_core : class_names_dl_aol)[i])) {
        currentClass = i;
        break;
      }
    }
    if (currentClass < 0) {
      send_to_char(ch, "The class '%s' does not exist.\r\n", arg);
      return;
    }
  }

  GET_CLASS(ch) = currentClass;

  if (GET_CLASS_RANKS(ch, GET_CLASS(ch)) < 1 && GET_ADMLEVEL(ch) < 1 && !ch->levelup) {
    send_to_char(ch, "You don't have any levels in that class.\r\n");
    GET_CLASS(ch) = tempClass;
    return;
  }

  if (*arg2)
    spellLevel = atoi(arg2);

  if (currentClass == CLASS_WIZARD) {
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
  }
  else if (currentClass == CLASS_CLERIC) {
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
  }
  else if (currentClass == CLASS_PALADIN) {
    GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
  }
  else if (currentClass == CLASS_DRUID) {
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
  }
  else if (currentClass == CLASS_RANGER) {
    GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
  }
  else if (currentClass == CLASS_BARD) {
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
  }
  else if (currentClass == CLASS_SORCERER) {
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
  }
  else if (currentClass == CLASS_FAVORED_SOUL) {
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
  }
  else if (currentClass == CLASS_ASSASSIN) {
    GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
  }

  if (findslotnum(ch, 1) < 1  && GET_ADMLEVEL(ch) < 1) {
    send_to_char(ch, "The '%s' class has no spellcasting ability.\r\n", class_names_dl_aol[currentClass]);
    GET_CLASS(ch) = tempClass;
    return;
  }

  send_to_char(ch, "@WBelow is a list of spells available to you:@n\r\n\r\n");

  for (i = 0; i <= MAX_SPELL_LEVELS; i++) {
    if (GET_ADMLEVEL(ch) < 1 && findslotnum(ch, i) < 1)
      continue;
    if (spellLevel == -1 || spellLevel == i) {
      send_to_char(ch, "@WLevel @R%d@W %s Spells@n:\r\n", i, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[currentClass]);
      for (j = 0; j <= MAX_SPELLS; j++) {
        if (spell_info[j].class_level[currentClass] == i && (findslotnum(ch, i) > 0 || GET_ADMLEVEL(ch) > 1)
	  &&  (spell_info[j].spell_level < 10) && !strstr(spell_info[j].name, "UNUSED")) {
          if (!ch->levelup && (currentClass == CLASS_FAVORED_SOUL || currentClass == CLASS_SORCERER)) {
            spellKnown = FALSE;
            for (k = 0; k < MAX_NUM_KNOWN_SPELLS; k++) {
              if (ch->player_specials->spells_known[k] == j) {
                spellKnown = TRUE;
              }
            }
            if (!spellKnown)
              continue;
          }
          

          spellInRepetoire = FALSE;
          if (currentClass == CLASS_FAVORED_SOUL || currentClass == CLASS_SORCERER) {
            for (n = 0; n < MAX_NUM_KNOWN_SPELLS; n++) {
              if (ch->player_specials->spells_known[n] == j) {
                spellInRepetoire = TRUE;
                break;
              }
            }
          }

          send_to_char(ch, "%s%-25s%s@n", spellInRepetoire ? "* " : "  ", spell_info[j].name, " ");
	  count++;
          if (count % 3 == 0)
            send_to_char(ch, "\r\n");
        }
        else
      	  continue;
      }
      if (count % 3 != 0)
        send_to_char(ch, "\r\n");
      send_to_char(ch, "\r\n");

      count = 0;
    }
  }

  GET_CLASS(ch) = tempClass;

  return;
}

int get_skill_mod(struct char_data *ch, int snum) {

	int roll = 0;

  if (IS_SET(spell_info[snum].flags, SKFLAG_STRMOD))
    roll += ability_mod_value(GET_STR(ch));
  if (IS_SET(spell_info[snum].flags, SKFLAG_DEXMOD))
    roll += dex_mod_capped(ch);
  if (IS_SET(spell_info[snum].flags, SKFLAG_CONMOD))
    roll += ability_mod_value(GET_CON(ch));
  if (IS_SET(spell_info[snum].flags, SKFLAG_INTMOD))
    roll += ability_mod_value(GET_INT(ch));
  if (IS_SET(spell_info[snum].flags, SKFLAG_WISMOD))
    roll += ability_mod_value(GET_WIS(ch));
  if (IS_SET(spell_info[snum].flags, SKFLAG_CHAMOD))
    roll += ability_mod_value(GET_CHA(ch));
  if (IS_SET(spell_info[snum].flags, SKFLAG_ARMORALL))
    roll -= GET_ARMORCHECKALL(ch);
  else if (IS_SET(spell_info[snum].flags, SKFLAG_ARMORBAD))
    roll -= GET_ARMORCHECK(ch);

	return roll;
}

int max_innate_spells[21][10] = {
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{ 4,  2, -1, -1, -1, -1, -1, -1, -1, -1},
{ 5,  2, -1, -1, -1, -1, -1, -1, -1, -1},
{ 5,  3, -1, -1, -1, -1, -1, -1, -1, -1},
{ 6,  3,  1, -1, -1, -1, -1, -1, -1, -1},
{ 6,  4,  2, -1, -1, -1, -1, -1, -1, -1},
{ 7,  4,  3,  1, -1, -1, -1, -1, -1, -1},
{ 7,  5,  3,  2, -1, -1, -1, -1, -1, -1},
{ 8,  5,  3,  2,  1, -1, -1, -1, -1, -1},
{ 8,  5,  4,  3,  2, -1, -1, -1, -1, -1},
{ 9,  5,  4,  3,  2,  1, -1, -1, -1, -1},
{ 9,  5,  5,  4,  3,  2, -1, -1, -1, -1},
{ 9,  5,  5,  4,  3,  2,  1, -1, -1, -1},
{ 9,  5,  5,  4,  4,  3,  2, -1, -1, -1},
{ 9,  5,  5,  4,  4,  3,  2,  1, -1, -1},
{ 9,  5,  5,  4,  4,  4,  3,  2, -1, -1},
{ 9,  5,  5,  4,  4,  4,  3,  2,  1, -1},
{ 9,  5,  5,  4,  4,  4,  3,  3,  2, -1},
{ 9,  5,  5,  4,  4,  4,  3,  3,  2,  1},
{ 9,  5,  5,  4,  4,  4,  3,  3,  3,  2},
{ 9,  5,  5,  4,  4,  4,  3,  3,  3,  3},
};

ACMD(do_learnspell) {

  int spells_known[10];
  int i = 0;

  for (i = 0; i < 10; i++)
    spells_known[i] = 0;

  for (i = 1; i <= MAX_SPELLS; i++) {
    if (GET_SKILL_BASE(ch, i))
      spells_known[spell_info[i].class_level[GET_CLASS(ch)]] += 1;
  }

  for (i = 1;i <= MAX_SPELLS; i++) {
    if (is_abbrev(spell_info[i].name, argument)) {
      if (!GET_SKILL_BASE(ch, i)) {
        if (spells_known[spell_info[i].class_level[GET_CLASS(ch)]] <
            max_innate_spells[GET_CLASS_RANKS(ch,
            GET_CLASS(ch))][spells_known[spell_info[i].class_level[GET_CLASS(ch)]]]) {
          GET_SKILL_BASE(ch, i) = 1;
          send_to_char(ch, "You have learned the spell '%s'.\r\n", spell_info[i].name);
          return;
        }
      }
    }
  }

}

int knows_spell(struct char_data *ch, int spellnum)
{

  return spell_in_domain(ch, spellnum);

}

int get_spell_resistance(struct char_data *ch)
{

  int sr = 0;

  if (!ch || ch == NULL)
    return 0;

  if (HAS_FEAT(ch, FEAT_DIAMOND_SOUL)) {
    sr = MAX(sr, 10 + GET_CLASS_RANKS(ch, CLASS_MONK));
  }

  if (GET_RACE(ch) == RACE_DROW_ELF || GET_RACE(ch) == RACE_DEEP_GNOME)
    sr = MAX(sr, 11 + GET_LEVEL(ch));

  return sr;
}


int sr_check(struct char_data *caster, struct char_data *victim) {

  int casterLevel = GET_LEVEL(caster);


  if (!IS_NPC(caster) && GET_MEM_TYPE(caster)) 
  {
    switch (GET_MEM_TYPE(caster)) 
    {
      case MEM_TYPE_MAGE:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_WIZARD);
        break;
      case MEM_TYPE_CLERIC:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_CLERIC);
        break;
      case MEM_TYPE_PALADIN:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_PALADIN);
        break;
      case MEM_TYPE_DRUID:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_DRUID);
        break;
      case MEM_TYPE_RANGER:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_RANGER);
        break;
      case MEM_TYPE_BARD:
        casterLevel = GET_CASTER_LEVEL(caster, CLASS_BARD);
        break;
      default:
        casterLevel = GET_LEVEL(caster);
        break;
    }
  }
  else 
  {
    casterLevel = GET_LEVEL(caster);
  }
  if (HAS_FEAT(caster, FEAT_SPELL_PENETRATION))
    casterLevel += 2;
  if (IS_ELF(caster))
    casterLevel += 2;

  if ((dice(1, 20) + casterLevel) > get_spell_resistance(victim))
    return TRUE;

  return FALSE;
}
