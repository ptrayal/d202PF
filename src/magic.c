/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "pets.h"
#include "player_guilds.h"

/* external variables */
extern int mini_mud;
extern struct spell_info_type spell_info[];
byte object_saving_throws(int material_type, int type);
extern struct deity_info deity_list[NUM_DEITIES];
extern struct pet_data pet_list[NUM_PETS];

/* external functions */
int is_player_grouped(struct char_data *target, struct char_data *group);
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
void die(struct char_data *ch, struct char_data * killer);
void set_auto_mob_stats(struct char_data *mob);
void free_summon(struct char_data *ch);

/* local functions */
int mag_materials(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int spellnum);
void affect_update(void);
void affect_update_violence(void);
int obj_savingthrow(int material, int type);
void do_affectv_tickdown(struct char_data *i);
void do_affect_tickdown(struct char_data *i);

int obj_savingthrow(int material, int type)
{
  int save, rnum;

  save = object_saving_throws(material, type);

  rnum = rand_number(1,100);

  if (rnum < save) {
    return (true);
  }

  return (false);
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct char_data *i;

  for (i = affect_list; i; i = i->next_affect) {
    if (FIGHTING(i))
      continue;
    do_affect_tickdown(i);
  }
}

void do_affect_tickdown(struct char_data *i)
{

  if (!i || IS_NPC(i))
    return;

  struct affected_type *af, *next;
    for (af = i->affected; af; af = next) {
      if (!af || af == NULL)
        break;
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == 0) {
	if (af->type > 0)
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (spell_info[af->type].wear_off_msg)
	      send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
        if (af)
  	  affect_remove(i, af);
      }
    }

}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data *ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (rand_number(0, 2)) {
      case 0:
	send_to_char(ch, "A wart sprouts on your nose.\r\n");
	break;
      case 1:
	send_to_char(ch, "Your hair falls out in clumps.\r\n");
	break;
      case 2:
	send_to_char(ch, "A huge corn develops on your big toe.\r\n");
	break;
      }
    }
    return (false);
  }
  if (extract) {
    if (item0 < 0)
      extract_obj(obj0);
    if (item1 < 0)
      extract_obj(obj1);
    if (item2 < 0)
      extract_obj(obj2);
  }
  if (verbose) {
    send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
    act("A puff of smoke rises from $n's pack.", true, ch, NULL, NULL, TO_ROOM);
  }
  return (true);
}

int find_savetype(int spellnum) {

  int stype;

  if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_FORT)) {
    stype = SAVING_FORTITUDE;
  }
  else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX)) {
    stype = SAVING_REFLEX;
  }
  else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_WILL)) {
    stype = SAVING_WILL;
  }
  else
  	return FALSE;

  return stype;
}

int calc_spell_dc(struct char_data *ch, int spellnum) 
{
  int dc = 0;
  int cast_stat = GET_INT(ch);
  int class = CLASS_WIZARD;

  if (GET_MEM_TYPE(ch) == MEM_TYPE_MAGE)
    class = CLASS_WIZARD;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC)
    class = CLASS_CLERIC;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID)
    class = CLASS_DRUID;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD)
    class = CLASS_BARD;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER)
    class = CLASS_RANGER;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN)
    class = CLASS_PALADIN;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_ASSASSIN)
    class = CLASS_PALADIN;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_SORCERER)
    class = CLASS_SORCERER;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_MUSIC)
    class = CLASS_BARD;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_ART)
    class = CLASS_MONK;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_INNATE)
    class = CLASS_UNDEFINED;
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_FAVORED_SOUL)
    class = CLASS_FAVORED_SOUL;

  if (GET_DIVINE_LEVEL(ch) > GET_ARCANE_LEVEL(ch) && !DOMAIN_FLAGGED(spellnum, DOMAIN_UNDEFINED)) {
		if ((GET_CLASS_RANKS(ch, CLASS_PALADIN) - 3) > GET_CLASS_RANKS(ch, CLASS_CLERIC))
			cast_stat = GET_CHA(ch);
		else
		  cast_stat = GET_WIS(ch);
  }

  dc = spell_info[spellnum].spell_level + (class == CLASS_UNDEFINED ? GET_LEVEL(ch) : GET_CASTER_LEVEL(ch, class)) + ability_mod_value(cast_stat);
  if (ch) {
    if (HAS_SCHOOL_FEAT(ch, SFEAT_SPELL_FOCUS, spell_info[spellnum].school))
      dc += 1;
    if (HAS_SCHOOL_FEAT(ch, SFEAT_GREATER_SPELL_FOCUS, spell_info[spellnum].school))
      dc += 1;
  }

  if (IS_GNOME(ch) && spell_info[spellnum].school == SCHOOL_ENCHANTMENT)
    dc += 1;

  dc += HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC);

  if (spellnum == ART_STUNNING_FIST)
    dc = 10 + (GET_LEVEL(ch) / 2) + ability_mod_value(GET_WIS(ch));

  if (spellnum == ART_QUIVERING_PALM)
    dc = 10 + (GET_LEVEL(ch) / 2) + ability_mod_value(GET_WIS(ch));

  return dc;
}

int mag_newsaves(int savetype, struct char_data *ch, struct char_data *victim, int spellnum, int dc)
{
  int total = 0;
  int diceroll = 0;
  struct affected_type *af;

  total += get_saving_throw_value(victim, savetype);

  total += (diceroll = dice(1, 20));

  if (diceroll == 1)
    return FALSE;
  if (diceroll == 20)
    return TRUE;

  if (spell_info[spellnum].school == SCHOOL_ENCHANTMENT && (IS_ELF(victim) || IS_HALF_ELF(victim)))
    total += 2;

  if (spell_info[spellnum].school == SCHOOL_ILLUSION && IS_GNOME(victim))
    total += 2;

  if (spell_info[spellnum].school == SCHOOL_ENCHANTMENT && HAS_FEAT(victim, FEAT_HONORBOUND))
    total += 2;

  if ((spellnum == SPELL_AFF_TAUNTED || spellnum == SPELL_AFF_TAUNTED || spellnum == SPELL_AFF_TAUNTED) &&
     HAS_FEAT(ch, FEAT_HONORBOUND))
    total += 2;

  if (spellnum == SPELL_POISON && IS_DWARF(victim))
    total += 2;

  if (spellnum == SPELL_FEAR) 
  {
    if (IS_HALFLING(victim))
      total +=2;

    struct char_data *tch;
    for (tch = world[IN_ROOM(victim)].people; tch; tch = tch->next_in_room) 
    {
      if (HAS_FEAT(tch, FEAT_AURA_OF_COURAGE) && is_player_grouped(tch, victim)) 
      {
        total += 4;
        break;
      }
    }
  }


  if (savetype == SAVING_FORTITUDE && affected_by_spell(victim, SPELL_INSPIRE_GREATNESS))
    total += 1;

  if (ch != victim && IS_EVIL(ch) && (affected_by_spell(victim, SPELL_PROT_FROM_EVIL) || affected_by_spell(victim, SPELL_MAGIC_CIRCLE_AGAINST_EVIL)))
    total += 2;

  if (ch != victim && (race_list[GET_RACE(ch)].family == RACE_TYPE_FEY || race_list[GET_RACE(ch)].family == RACE_TYPE_ELF) && HAS_FEAT(victim, FEAT_RESIST_NATURES_LURE))
    total += 4;

  if (affected_by_spell(victim, SPELL_SICKENED))
    total -= 2;

  if (GET_GUILD(victim) == GUILD_DEVOTIONISTS && dice(1, 5) == 1) 
  {
    total += (GET_GUILD_RANK(victim) + 1) / 4;
  }

  if (total >= dc)
    return true;

  if (affected_by_spell(victim, SPELL_COUNTERSONG)) 
  {
    if (ch->affected) 
    {
      for (af = ch->affected; af; af = af->next) 
      {
        if (af->type == SPELL_COUNTERSONG) 
        {
          if (af->modifier >= dc)
            return true;
          else
            return false;
        }
      }
    }
  }

  if (HAS_FEAT(victim, FEAT_SLIPPERY_MIND) && savetype == SAVING_WILL)
    if (total - diceroll + dice(1, 20) > dc)
      return true;

  return false;

}

/*
 * Every spell that does damage comes through here.  this calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data *ch, struct char_data *victim, int spellnum)
{
  int saveHalf = FALSE;
  int dam = 0, dc = 0;
  char dammes[MAX_STRING_LENGTH]={'\0'};
  int numdice = 0;
  int sizedice = 0;
  int dammod = 0;
  int enhance = MAX(0, HAS_FEAT(ch, FEAT_ENHANCE_SPELL) * 5);

  if (victim == NULL || ch == NULL)
    return (0);

  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
    numdice = 1;
    sizedice = 4;
    dammod = 1;
    dam = dice(1, 4) + 1;
    break;

  case SPELL_SCORCHING_RAY:
    numdice = 4;
    sizedice = 6;
    dammod = 0;
    dam = dice(4, 6);
    break;

  case SPELL_MISSILE_SWARM:
    numdice = MIN(10 + enhance, level);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(10 + enhance, level), 6);
    break;

  case SPELL_GREATER_MISSILE_SWARM:
    numdice = MIN(20 + enhance, level);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(20 + enhance, level), 6);
    break;

  case SPELL_FLAME_ARROW:
    numdice = 4;
    sizedice = 6;
    dammod = 0;
    dam = dice(4, 6);
    break;

  case SPELL_FLAMING_SPHERE:
    numdice = 3;
    sizedice = 6;
    dammod = 0;
    dam = dice(3, 6);
    break;

  case SPELL_ACID_ARROW:
    numdice = 2;
    sizedice = 4;
    dammod = 0;
    dam = dice(2, 4);
    break;

  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
    numdice = 1;
    sizedice = 6;
    dammod = 0;
    dam = dice(1, 6);
    break;

  case SPELL_BURNING_HANDS:
    numdice = MIN(MAX(1, level), 5 + enhance);
    sizedice = 4;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 5), 4);
    break;

  case SPELL_SHOCKING_GRASP:
    numdice = 1;
    sizedice = 8;
    dammod = MIN(MAX(1, level), 20);
    dam = dice(1, 8) + MIN(MAX (1, level), 20);
    break;

  case SPELL_LIGHTNING_BOLT:
  case SPELL_FIREBALL:
    numdice = MIN(MAX(1, level), 10 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 10), 6);
    break;

  case SPELL_DRAGON_MOUNT_BREATH:
    numdice = ch->player_specials->mount_cur_hit;
    sizedice = 1;
    dammod = 0;
    dam = numdice;
    break;

  case SPELL_BREATH_WEAPON:
    if (GET_RACE(ch) == RACE_DRAGON) {
      numdice = MAX(1, GET_LEVEL(ch) / 2);
      sizedice = 10;
      dammod = 0;
      dam = dice(MAX(1, GET_LEVEL(ch) / 2), 10);
    }
    else {
      numdice = MAX(1, HAS_FEAT(ch, FEAT_BREATH_WEAPON) * 2);
      sizedice = 8;
      dammod = 0;
      dam = dice(MAX(1, HAS_FEAT(ch, FEAT_BREATH_WEAPON) * 2), 8);
    }
    break;

  case SPELL_DISINTIGRATE:
    numdice = MIN(MAX(2, level * 2), 40 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(2, level * 2), 40), 6);
    break;

  case SPELL_SLAY_LIVING:
    numdice = MIN(MAX(3, level * 3), 45 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(3, 3 * level), 45), 6);
    break;

  case SPELL_FINGER_OF_DEATH:
    numdice = MIN(MAX(3, level * 3), 51 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(3, 3 * level), 51), 6);
    break;

  case SPELL_WAIL_OF_THE_BANSHEE:
    numdice = MIN(MAX(3, level * 3), 60 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(3, 3 * level), 60), 6);
    break;

  case ART_QUIVERING_PALM:
    if (GET_LEVEL(ch) < GET_LEVEL(victim)) {
      numdice = 1;
      sizedice = 1;
      dammod = -1;
      dam = 0;
    }
    numdice = MAX(3, level * 3);
    sizedice = 6;
    dammod = 0;
    dam = dice(MAX(3, 3 * level), 6);
    break;

  case SPELL_ICE_STORM:
    numdice = 5;
    sizedice = 6;
    dammod = 0;
    dam = dice(5, 6);
    break;

  case SPELL_CONE_OF_COLD:
  case SPELL_FLAMESTRIKE:
    numdice = MIN(MAX(1, level), 15 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 15), 6);
    break;

  case SPELL_DELAYED_BLAST_FIREBALL:
    numdice = MIN(MAX(1, level), 20 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 20), 6);
    break;

  case SPELL_METEOR_SWARM_AREA:
    numdice = 6;
    sizedice = 6;
    dammod = 0;
    dam = dice(6, 6);
    break;

  case SPELL_METEOR_SWARM:
    numdice = 2;
    sizedice = 6;
    dammod = 0;
    dam = dice(2, 6);
    break;

  case SPELL_HORRID_WILTING:
  case SPELL_FIRE_STORM:
    numdice = MIN(MAX(1, level), 20 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 20), 6);
    break;

  case SPELL_CHAIN_LIGHTNING:
    numdice = MIN(MAX(1, level), 15 + enhance);
    sizedice = 6;
    dammod = 0;
    dam = dice(MIN(MAX(1, level), 15), 6);
    break;


  case SPELL_COLOR_SPRAY:
    numdice = 1;
    sizedice = 1;
    dammod = 1;
    dam = dice(1, 1) + 1;
    break;

    /* Mostly clerics */
  case SPELL_DISPEL_EVIL:
    numdice = 6;
    sizedice = 8;
    dammod = 6;
    dam = dice(6, 8) + 6;
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
      numdice = GET_HIT(ch) - 1;
      sizedice = 1;
      dammod = 0;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", false, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;

  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
      numdice = 6;
      sizedice = 8;
      dammod = 6;
    if (IS_GOOD(ch)) {
      victim = ch;
      numdice = GET_HIT(ch) - 1;
      sizedice = 1;
      dammod = 0;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", false, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;

  case SPELL_CALL_LIGHTNING_BOLT:
    if (weather_info.sky == SKY_LIGHTNING) {
      numdice = 3;
      sizedice = 10;
      dammod = 0;
      dam = dice(3, 10);
    }
    else {
      numdice = 3;
      sizedice = 6;
      dammod = 0;
      dam = dice(3, 6);
    }
    break;

  case SPELL_HARM:
      numdice = MIN(GET_HIT(victim) - 1, MIN(150 + (enhance * 10), level * 10));
      sizedice = 1;
      dammod = 0;
    dam = MIN(GET_HIT(victim) - 1, MIN(150, level * 10));
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2) {
      dam = 100;
      numdice = 100;
      sizedice = 1;
      dammod = 0;
   }
    else {
      numdice = 1;
      sizedice = 10;
      dammod = 0;
      dam = dice(1, 10);
    }
    break;

    /* Area spells */
  case SPELL_EARTHQUAKE:
    numdice = 2;
    sizedice = 8;
    dammod = level;
    dam = dice(2, 8) + level;
    break;

  case SPELL_INFLICT_MINOR:
    numdice = 4;
    sizedice = 1;
    dammod = 0;
    dam = 4;
    break;
  case SPELL_INFLICT_LIGHT:
    numdice = 1;
    sizedice = 8;
    dammod = MIN(level, 5);
    dam = dice(1, 8) + MIN(level, 5);
    break;
  case SPELL_INFLICT_MODERATE:
    numdice = 2;
    sizedice = 8;
    dammod = MIN(level, 10);
    dam = dice(2, 8) + MIN(level, 10);
    break;
  case SPELL_INFLICT_SERIOUS:
    numdice = 3;
    sizedice = 8;
    dammod = MIN(level, 15);
    dam = dice(3, 8) + MIN(level, 15);
    break;
  case SPELL_INFLICT_CRITIC:
    numdice = 4;
    sizedice = 8;
    dammod = MIN(level, 20);
    dam = dice(4, 8) + MIN(level, 20);
    break;

  case SPELL_ACID_SPLASH:
  case SPELL_RAY_OF_FROST:
    numdice = 1;
    sizedice = 3;
    dammod = 0;
    dam = dice(1, 3);
    break;

  case SPELL_DISRUPT_UNDEAD:
    if (race_list[GET_RACE(victim)].family == RACE_TYPE_UNDEAD) {
      numdice = 1;
      sizedice = 6;
      dammod = 0;
      dam = dice(1, 6);
    }
    else {
      send_to_char(ch, "This magic only affects the undead!\r\n");
      numdice = 1;
      sizedice = 1;
      dammod = -1;
      dam = 0;
    }
    break;

  case SPELL_SOUND_BURST:
        numdice = 1;
        sizedice = 8;
        dammod = 0;
  	dam = dice(1, 8);
  	break;

  case SPELL_SHOUT:
      numdice = 5;
      sizedice = 6;
      dammod = 0;
      dam = dice(5, 6);
    break;

  } /* switch(spellnum) */

  if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
    numdice *= 15;
    numdice /= 10;
  }

  dc = calc_spell_dc(ch, spellnum);

  if (spellnum == SPELL_BREATH_WEAPON) {
    if (GET_RACE(ch) == RACE_DRAGON)
      dc = 10 + ability_mod_value(GET_CON(ch)) + GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE);
    else 
      dc = 10 + ability_mod_value(GET_CON(ch)) + (GET_LEVEL(ch) / 2);
  }

  if (spellnum != SPELL_HARM && spellnum != SPELL_SLAY_LIVING && spellnum != ART_QUIVERING_PALM &&
      spellnum != SPELL_FINGER_OF_DEATH && spellnum != SPELL_WAIL_OF_THE_BANSHEE)
    dammod += MAX(0, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE) * numdice);
  else if (spellnum == SPELL_HARM)
    dammod += MAX(0, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE) * level);

  if ((spellnum == SPELL_SLAY_LIVING || spellnum == ART_QUIVERING_PALM  ||
      spellnum == SPELL_FINGER_OF_DEATH || spellnum == SPELL_WAIL_OF_THE_BANSHEE) &&
        (AFF_FLAGGED(victim, AFF_DEATH_WARD) || (IS_NPC(victim) &&
        (MOB_FLAGGED(victim, MOB_LIEUTENANT) || MOB_FLAGGED(victim, MOB_CAPTAIN) ||
         MOB_FLAGGED(victim, MOB_BOSS)))))
    dam = 0;
  else
    dam = dice(numdice, sizedice) + dammod;

  if (mag_newsaves(find_savetype(spellnum), ch, victim, spellnum, dc) ||
      ((spellnum == SPELL_SLAY_LIVING || spellnum == ART_QUIVERING_PALM ||
        spellnum == SPELL_FINGER_OF_DEATH || spellnum == SPELL_WAIL_OF_THE_BANSHEE) &&
        AFF_FLAGGED(victim, AFF_DEATH_WARD))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any injury.@n\r\n");
      dam = 0;
    } else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_HALF)) {
      if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX) &&
          HAS_FEAT(victim, FEAT_EVASION) &&
          (!GET_EQ(victim, WEAR_BODY) ||
          GET_OBJ_TYPE(GET_EQ(victim, WEAR_BODY)) != ITEM_ARMOR ||
          GET_OBJ_VAL(GET_EQ(victim, WEAR_BODY), VAL_ARMOR_SKILL) < ARMOR_TYPE_MEDIUM)) {
        send_to_char(victim, "@g*save*@y Your evasion ability allows you to avoid ANY injury.@n\r\n");
        dam = 0;
      } else {
        if (spellnum == SPELL_DISINTIGRATE) {
          send_to_char(victim, "@g*save*@y You take partial damage.@n\r\n");
          dam = dice(5, 6);
          numdice = 5;
          sizedice = 6;
          dammod = 0;
        }
        else if (spellnum == SPELL_SLAY_LIVING || spellnum == SPELL_FINGER_OF_DEATH) {
          send_to_char(victim, "@g*save*@y You take partial damage.@n\r\n");
          dam = dice(3, 6) + level;
          numdice = 3;
          sizedice = 6;
          dammod = level;
        }
        else if (spellnum == ART_QUIVERING_PALM || spellnum == SPELL_WAIL_OF_THE_BANSHEE) {
          send_to_char(victim, "@g*save*@y You take no damage.@n\r\n");
          dam = 0;
        }
        else {
          send_to_char(victim, "@g*save*@y You take half damage.@n\r\n");
          saveHalf = TRUE;
          dam = MAX(1, dam / 2);
        }
      }
    }
  } else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_HALF) &&
             IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX) &&
             HAS_FEAT(victim, FEAT_IMPROVED_EVASION) &&
             (!GET_EQ(victim, WEAR_BODY) ||
              GET_OBJ_TYPE(GET_EQ(victim, WEAR_BODY)) != ITEM_ARMOR ||
              GET_OBJ_VAL(GET_EQ(victim, WEAR_BODY), VAL_ARMOR_SKILL) < ARMOR_TYPE_MEDIUM)) {
        send_to_char(victim, "@r*save*@y Your improved evasion prevents full damage even on failure.@n\r\n");
        saveHalf = TRUE;
        dam = MAX(1, dam / 2);
  }

  if (dam > 0) {
    if (PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL) || PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL))
      dam = (numdice * sizedice) + dammod;
    else
      dam = dice(numdice, sizedice) + dammod;
    if (PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL))
      dam *= 2;
    if (saveHalf)
      dam /= 2;
    dam = MAX(dam, 1);
  }

  /* and finally, inflict the damage */
  sprintf(dammes, "@yYour @W%s@y hits $N for @R%d@y points of damage.@n", spell_info[spellnum].name, dam);
  act(dammes, false, ch, 0, victim, TO_CHAR);
  sprintf(dammes, "@y$n's @W%s@y hits you for @R%d@y points of damage.@n", spell_info[spellnum].name, dam);
  act(dammes, false, ch, 0, victim, TO_VICT);
  sprintf(dammes, "@y$n's @W%s@y hits $N for @R%d@y points of damage.@n", spell_info[spellnum].name, dam);
  act(dammes, false, ch, 0, victim, TO_NOTVICT);
 return (damage(ch, victim, dam, spellnum, 0, -1, 0, spellnum, 1));

}

void mag_loop(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum)
{

  int num_times = 1;
  int area = FALSE;
  int i;
  struct char_data *tch, *next_tch;
  int j=0, numtimes=0, roll=0;

  switch (spellnum) {

  case SPELL_METEOR_SWARM:
    mag_loop(level, ch, victim, SPELL_METEOR_SWARM_AREA);
    num_times = 4;
    break;

  case SPELL_METEOR_SWARM_AREA:
    num_times = 4;
    area = TRUE;
    break;

  case SPELL_MAGIC_MISSILE:
    num_times = MIN(5, (level + 1) / 2);
    break;

  case SPELL_SCORCHING_RAY:
    num_times = 1 + ((level - 3) / 4);
    break;

  case SPELL_FLAME_ARROW:
    num_times = MAX(1, level / 4);
    break;

  }

  if (area) {
    for (i = 0; i < num_times; i++) {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     *		  5: grouped pcs
     *		  5: people not fighting the caster's group if caster fighting
     *		  5: people not fighting if caster fighting
     */

        if (tch == ch)
          continue;
        if (ADM_FLAGGED(tch, ADM_NODAMAGE))
          continue;
        if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(tch))
          continue;
        if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
          continue;
        if (FIGHTING(ch) && FIGHTING(tch) && !is_player_grouped(FIGHTING(tch), ch))
          continue;
        if (PRF_FLAGGED(ch, PRF_CONTAINED_AREAS) && !FIGHTING(tch))
          continue;
        if (IS_NPC(ch) && IS_NPC(tch) && !tch->master && !AFF_FLAGGED(ch, AFF_CHARM))
          continue;
        if (IS_NPC(ch) && !IS_NPC(tch) && AFF_FLAGGED(ch, AFF_CHARM) && !is_player_grouped(FIGHTING(tch), ch->master))
          continue;
        if (GET_POS(tch) == POS_DEAD)
          continue;

        if ((tch->master == ch || ch->master == tch || ch->master == tch->master) &&
            AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(tch, AFF_GROUP))
          continue;

        /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
        if (spellnum == SPELL_LIGHTNING_BOLT) {
          roll = dice(1, 100);
          if (roll < 25) {
            numtimes = 0;
          } else if (roll < 68 && !OUTSIDE(ch)) {
            numtimes = 2;
          } else {
            numtimes = 1;
          }
          for (j = 0; j < numtimes; j++) {
            if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && IS_NPC(tch) && !AFF_FLAGGED(tch, AFF_CHARM))
              continue;
            mag_damage(level, ch, tch, spellnum);
          }
          if (numtimes == 0)
            damage(ch, tch, 0, spellnum, 0, -1, 0, spellnum, 1);
        }
        else
          mag_damage(level, ch, tch, spellnum);
      }
    }
    return;
  }

  for (i = 0; i < num_times; i++) {
    if (GET_POS(victim) == POS_DEAD)
      continue;
    if (mag_damage(level, ch, victim, spellnum) == -1)
      break;
  }

  return;

}
/*
 * Every spell that does an affect comes through here.  this determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = false, accum_duration = false;
  const char *to_vict = NULL, *to_room = NULL, *to_char = NULL;
  int i, mod, q=0, roll=0;
  struct damreduct_type *ptr, *reduct, *temp;


  if (victim == NULL || ch == NULL)
    return;

  if (GET_ADMLEVEL(ch) > 0)
    level = MAX(level, GET_LEVEL(ch));

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
    af[i].level = level;
  }

  if ((SINFO.violent == true) && mag_newsaves(find_savetype(spellnum), ch, victim, spellnum, calc_spell_dc(ch, spellnum))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  while (affected_by_spell(victim, spellnum))
    affect_from_char(victim, spellnum);

  switch (spellnum) {

  case SPELL_DEATH_WARD:
    af[0].duration = level * 10;
    af[0].bitvector = AFF_DEATH_WARD;
    accum_duration = false;
    to_vict = "You are warded against instant death spells and effects!";
    to_room = "$n is warded against instant death spells and effects!";
    break;
  case SPELL_PARALYZE:
    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) 
    {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }
    af[0].duration = level/2;
    af[0].bitvector = AFF_STUNNED;
    accum_duration = false;
    to_vict = "You feel your limbs freeze!";
    to_room = "$n suddenly freezes in place!";
    break;
  case ART_STUNNING_FIST:
    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) 
    {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }
    af[0].duration = 1;
    af[0].bitvector = AFF_STUNNED;
    accum_duration = false;
    to_vict = "You are in a stunned daze!";
    to_room = "$n is stunned.";
    break;
  case ART_EMPTY_BODY:
    af[0].duration = GET_KI(ch) / 10;
    af[0].bitvector = AFF_ETHEREAL;
    accum_duration = false;
    to_vict = "You switch to the ethereal plane.";
    to_room = "$n disappears.";
    break;
  case SPELL_RESISTANCE:
    af[0].duration = 12;
    af[0].location = APPLY_ALLSAVES;
    af[0].modifier = 1;
    accum_duration = false;
    to_vict = "You glow briefly with a silvery light.";
    to_room = "$n glows briefly with a silvery light.";
    break;
  case SPELL_DAZE:
    if (GET_LEVEL(victim) < 5) {
      af[0].duration = 2;
      af[0].bitvector = AFF_NEXTNOACTION;
      af[0].location = APPLY_ABILITY;
      af[0].modifier = 1;
      accum_duration = false;
      to_vict = "You are struck dumb by a flash of bright light!";
      to_room = "$n is struck dumb by a flash of bright light!";
    }
    break;
  case SPELL_FLARE:
    if (GET_LEVEL(victim) < 5) {
      af[0].duration = 2;
      af[0].bitvector = AFF_NONE;
      af[0].location = APPLY_HITROLL;
      af[0].modifier = -1;
      accum_duration = false;
      to_vict = "You are disoriented by a flash of bright light!";
      to_room = "$n is disoriented by a flash of bright light!";
    }
    break;

  case SPELL_STONESKIN:

    to_vict = "Your skin hardens to stone giving you damage reduction of 10/adamantine.";

    for (reduct = victim->damreduct; reduct; reduct = reduct->next) {
      if (reduct->spell == SPELL_STONESKIN) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
      if (reduct->spell == SPELL_GREATER_STONESKIN) {
        return;
      }
      if (reduct->spell == SPELL_PREMONITION) {
        return;
      }
    }
    CREATE(ptr, struct damreduct_type, 1);
    ptr->next = victim->damreduct;
    victim->damreduct = ptr;
    ptr->spell = SPELL_STONESKIN;
    ptr->feat = 0;
    ptr->mod = 10;
    ptr->duration = level * 25;
    ptr->max_damage = MIN(150, level * 10);
    for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
      ptr->damstyle[q] = ptr->damstyleval[q] = 0;
    ptr->damstyle[0] = DR_MATERIAL;
    ptr->damstyleval[0] = MATERIAL_ADAMANTINE;
    ptr->damstyle[1] = DR_SPELL;

    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level * 25;
    af[0].bitvector = AFF_NONE;

    break;

  case SPELL_GREATER_STONESKIN:

    to_vict = "Your skin hardens to thick stone giving you damage reduction of 20/adamantine.";

    for (reduct = victim->damreduct; reduct; reduct = reduct->next) {
      if (reduct->spell == SPELL_STONESKIN) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
      if (reduct->spell == SPELL_GREATER_STONESKIN) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
      if (reduct->spell == SPELL_PREMONITION) {
        return;
      }
    }
    CREATE(ptr, struct damreduct_type, 1);
    ptr->next = victim->damreduct;
    victim->damreduct = ptr;
    ptr->spell = SPELL_GREATER_STONESKIN;
    ptr->feat = 0;
    ptr->mod = 20;
    ptr->duration = level * 25;
    ptr->max_damage = MIN(150, level * 10);
    for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
      ptr->damstyle[q] = ptr->damstyleval[q] = 0;
    ptr->damstyle[0] = DR_MATERIAL;
    ptr->damstyleval[0] = MATERIAL_ADAMANTINE;
    ptr->damstyle[1] = DR_SPELL;

    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level * 25;
    af[0].bitvector = AFF_NONE;
    break;

  case SPELL_PREMONITION:

    to_vict = "A shield of dense, but invisible mental energy surrounds you giving you damage reduction 30/adamantine.";

    for (reduct = victim->damreduct; reduct; reduct = reduct->next) {
      if (reduct->spell == SPELL_STONESKIN) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
      if (reduct->spell == SPELL_GREATER_STONESKIN) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
      if (reduct->spell == SPELL_PREMONITION) {
        REMOVE_FROM_LIST(reduct, victim->damreduct, next);
      }
    }
    CREATE(ptr, struct damreduct_type, 1);
    ptr->next = victim->damreduct;
    victim->damreduct = ptr;
    ptr->spell = SPELL_PREMONITION;
    ptr->feat = 0;
    ptr->mod = 30;
    ptr->duration = level * 25;
    ptr->max_damage = level * 10;
    for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
      ptr->damstyle[q] = ptr->damstyleval[q] = 0;
    ptr->damstyle[0] = DR_MATERIAL;
    ptr->damstyleval[0] = MATERIAL_ADAMANTINE;
    ptr->damstyle[1] = DR_SPELL;

    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level * 25;
    af[0].bitvector = AFF_NONE;
    break;

  case SPELL_HASTE:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level;
    af[0].location = APPLY_REFLEX;
    to_vict = "Your speed and reflexes accelerate to an unreal effect.";
    break;

  case SPELL_KEEN_EDGE:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 10;
    af[0].location = APPLY_ABILITY;
    to_vict = "Your bladed weapons gain a keen edge.";
    break;

  case SPELL_WEAPON_OF_IMPACT:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 10;
    af[0].location = APPLY_ABILITY;
    to_vict = "Your blunt weapons gain a force of impact.";
    break;

  case SPELL_LIGHT:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "A globe of magical light winks into existance above your head.";
    break;

  case SPELL_DAYLIGHT:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "An aura of daylight suddenly surrounds you.";
    break;

  case SPELL_FLY:
    af[0].bitvector = AFF_FLYING;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "You gain the magical ability to fly.";
    break;

  case SPELL_LEVITATE:
    af[0].bitvector = AFF_FLYING;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "You gain the magical ability to levitate above the ground.";
    break;

  case SPELL_BLUR:
    if (affected_by_spell(victim, SPELL_FAERIE_FIRE)) {
      send_to_char(victim, "The blur spell fails as you are covered in faerie fire and cannot be concealed.\r\n");
      return;
    }
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 10;
    af[0].location = APPLY_NONE;
    to_vict = "Your body begins to take upon a blurry form making it hard to hit.";
    break;


  case SPELL_COMPREHEND_LANGUAGES:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "Your mind expands to allow understanding of all spoken and written languages";
    break;

  case SPELL_TONGUES:
    af[0].bitvector = AFF_NONE;
    af[0].modifier = 0;
    af[0].duration = level * 20;
    af[0].location = APPLY_NONE;
    to_vict = "Your mind expands to allow speaking, writing and understanding of all languages";
    break;

  case SPELL_ACID_ARROW:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_CHA;
    af[0].duration = level / 3;
    af[0].modifier = -2;
    accum_duration = true;
    to_vict = "Searing acid begins to eat into your flesh!";
    break;

  case SPELL_FLAMING_SPHERE:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].duration = level;
    af[0].modifier = 12 + ((GET_MEM_TYPE(ch) == MEM_TYPE_MAGE) ? ability_mod_value(GET_INT(ch)) : ability_mod_value(GET_WIS(ch)));
    accum_duration = true;
    to_vict = "A flaming sphere appears into being right beneath your feet!";
    to_room = "A flaming sphere appears into being right beneath $n's feet!";
    break;

  case SPELL_CHILL_TOUCH:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_STR;
    af[0].duration = 5;
    af[0].modifier = -1;
    accum_duration = true;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_SHILLELAGH:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].duration = level * 10;
    af[0].modifier = 1;
    to_vict = "Your weapon glows bright green for a short moment.";
    to_room = "$n's weapon glows bright green for a short moment.";
    break;

  case SPELL_CALL_LIGHTNING:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].duration = 100;
    af[0].modifier = 1;
    to_vict = "You gain control over the lightning in the air.";
    to_room = "$n gains control over the lightning in the air.";
    break;

  case SPELL_FLAME_WEAPON:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].duration = level * 10;
    af[0].modifier = 1;
    to_vict = "Your weapons suddenly burst into flame offering extra damage against opponents.";
    to_room = "$n's weapons suddenly burst into flame offering extra damage against opponents.";
    break;

  case SPELL_FLOATING_DISC:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level * 20;
    to_vict = "A magical floating disc appears at your side.";
    to_room = "A magical floating disc appears at $n's side.";
    break;

  case SPELL_MAGE_ARMOR:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_AC_DEFLECTION;
    af[0].modifier = 40;
    af[0].duration = level * 20;
    to_vict = "A semi-transparent globe of force surrounds your body.";
    to_room = "$n is suddenly surrounded by a semi-transparent globe of force.";
    break;

  case SPELL_LEARNING:
    if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING))
      affect_from_char(ch, SPELL_IMPROVED_LEARNING);
    if (affected_by_spell(ch, SPELL_GREATER_LEARNING))
      affect_from_char(ch, SPELL_GREATER_LEARNING);
    if (affected_by_spell(ch, SPELL_EPIC_LEARNING))
      affect_from_char(ch, SPELL_EPIC_LEARNING);
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_FEAT;
    af[0].modifier = 1;
    af[0].duration = 1200;
    to_vict = "Your learning abilities have increased.";
    to_room = "$n's learning abilities have increased.";
    break;

  case SPELL_IMPROVED_LEARNING:
    if (affected_by_spell(ch, SPELL_LEARNING))
      affect_from_char(ch, SPELL_LEARNING);
    if (affected_by_spell(ch, SPELL_GREATER_LEARNING))
      affect_from_char(ch, SPELL_GREATER_LEARNING);
    if (affected_by_spell(ch, SPELL_EPIC_LEARNING))
      affect_from_char(ch, SPELL_EPIC_LEARNING);
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_FEAT;
    af[0].modifier = 1;
    af[0].duration = 1200;
    to_vict = "Your learning abilities have increased a fair bit.";
    to_room = "$n's learning abilities have increased a fair bit.";
    break;

  case SPELL_GREATER_LEARNING:
    if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING))
      affect_from_char(ch, SPELL_IMPROVED_LEARNING);
    if (affected_by_spell(ch, SPELL_LEARNING))
      affect_from_char(ch, SPELL_LEARNING);
    if (affected_by_spell(ch, SPELL_EPIC_LEARNING))
      affect_from_char(ch, SPELL_EPIC_LEARNING);
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_FEAT;
    af[0].modifier = 1;
    af[0].duration = 1200;
    to_vict = "Your learning abilities have increased a lot.";
    to_room = "$n's learning abilities have increased a lot.";
    break;

  case SPELL_EPIC_LEARNING:
    if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING))
      affect_from_char(ch, SPELL_IMPROVED_LEARNING);
    if (affected_by_spell(ch, SPELL_GREATER_LEARNING))
      affect_from_char(ch, SPELL_GREATER_LEARNING);
    if (affected_by_spell(ch, SPELL_LEARNING))
      affect_from_char(ch, SPELL_LEARNING);
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_FEAT;
    af[0].modifier = 1;
    af[0].duration = 1200;
    to_vict = "Your learning abilities have increased tremendously.";
    to_room = "$n's learning abilities have increased tremendously.";
    break;



  case SPELL_EPIC_MAGE_ARMOR:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_AC_ARMOR;
    af[0].modifier = 5 + get_skill_value(ch, SKILL_SPELLCRAFT);
    af[0].duration = 24 * 20;
    af[1].bitvector = AFF_NONE;
    af[1].location = APPLY_AC_DEFLECTION;
    af[1].modifier = 5 + get_skill_value(ch, SKILL_SPELLCRAFT);
    af[1].duration = 24 * 20;
    af[2].bitvector = AFF_NONE;
    af[2].location = APPLY_AC_NATURAL;
    af[2].modifier = 5 + get_skill_value(ch, SKILL_SPELLCRAFT);
    af[2].duration = 24 * 20;
    af[3].bitvector = AFF_NONE;
    af[3].location = APPLY_AC_DODGE;
    af[3].modifier = 5 + get_skill_value(ch, SKILL_SPELLCRAFT);
    af[3].duration = 24 * 20;
    to_vict = "A semi-transparent globe of shimmering force surrounds your body.";
    to_room = "$n is suddenly surrounded by a semi-transparent globe of shimmering force.";
    break;

  case SPELL_BARKSKIN:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_AC_NATURAL;
    af[0].modifier = MIN(50, (((level - 3) / 3) * 10) + 20);
    af[0].duration = level * 10;
    to_vict = "Your skin is covered in hard, tough bark.";
    to_room = "$n's skin is suddenly covered in tough, hard bark.";
    break;

  case SPELL_ENTANGLE:
    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }

    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].modifier = 1;
    af[0].duration = level * 10;
    to_vict = "Entangling plants spring up around you threatening to hold you fast!";
    to_room = "Entangling plants spring up around $n threatening to hold $m fast!";
    break;

  case SPELL_INSPIRE_COURAGE:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_HITROLL;
    af[0].modifier = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
    af[0].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    af[1].bitvector = AFF_NONE;
    af[1].location = APPLY_DAMROLL;
    af[1].modifier = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
    af[1].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    to_vict = "You have inspired to have greater courage and morale.";
    to_room = "$n has been isnpired to have greater courage and morale.";
    break;

  case SPELL_INSPIRE_GREATNESS:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_FEAT;
    af[0].modifier = 1;
    af[0].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    to_vict = "You have inspired to greatness.";
    to_room = "$n has been isnpired to greatness.";
    break;

  case SPELL_INSPIRE_HEROICS:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_AC_DODGE;
    af[0].modifier = 40;
    af[0].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    af[1].bitvector = AFF_NONE;
    af[1].location = APPLY_ALLSAVES;
    af[1].modifier = 4;
    af[1].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    to_vict = "You have inspired to heroics.";
    to_room = "$n has been isnpired to heroics.";
    break;

  case SPELL_INSPIRE_COMPETENCE:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_SKILL;
    af[0].modifier = 2;
    af[0].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;

    to_vict = "You have inspired to have greater competence with your skills.";
    to_room = "$n has been isnpired to have greater competenece with $s skills.";
    break;

  case SPELL_COUNTERSONG:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].modifier = skill_roll(ch, SKILL_PERFORM);
    af[0].duration = HAS_FEAT(ch, FEAT_LINGERING_SONG) ? level * 10 : level * 5;
    to_vict = "You come under the effect of the protective bardic countersong.";
    to_room = "$n comes under the effect of the protective bardic countersong.";
    break;

  case SPELL_SHIELD_OF_FAITH:
    af[0].location = APPLY_AC_DEFLECTION;
    af[0].modifier = 20 + 10 * MIN(level / 6, 3);
    af[0].duration = level * 10;
    to_vict = "A shield of divine energy surrounds your body.";
    to_room = "A shield of divine energy surrounds $n's body.";
    break;

  case SPELL_BLESS_SINGLE:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1;
    af[0].duration = level * 10;

    af[1].location = APPLY_WILL;
    af[1].modifier = 1;
    af[1].duration = level * 10;

    to_vict = "You feel the touch of a divine entity in your soul.";
    to_room = "A flash of divine energy surrounds $n momentarily.";
    break;

  case SPELL_CALM_ANIMAL:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].modifier = GET_WIS(ch);
    af[0].duration = level * 10;

    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_LEVEL;
    af[0].modifier = level;
    af[0].duration = level * 10;

    to_vict = "You feel an affinity with animals that will keep them calm with you.";
    to_room = "$n gains an affinity with animals that will keep them calm with $m.";
    break;

  case SPELL_PRAYER:
    af[0].location = APPLY_NONE;
    af[0].modifier = 0;
    af[0].duration = level;

    to_vict = "A prayer of faith leaves you with a renewed spirit and zealous inner fire.";
    to_room = "A prayer of faith leaves $n with a renewed spirit and zealous inner fire.";

    break;

  case SPELL_DIVINE_FAVOR:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = MIN(3, MAX(1, GET_LEVEL(ch) / 3));
    af[0].duration = 10;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = MIN(3, MAX(1, GET_LEVEL(ch) / 3));
    af[1].duration = 10;

    to_vict = "Your faith is bolstered as the favor of your deity is placed upon you.";
    to_room = "$n's faith is bolstered as the favor of $s deity is placed upon $m.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_NONE;
    af[0].modifier = 0;
    af[0].duration = level;
    af[0].bitvector = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_COURAGE:

    af[0].modifier = 0;
    af[0].duration = level * 20;
    af[0].bitvector = AFF_COURAGE;

    accum_duration = false;

    to_vict = "You feel more courageous and resistant to fear affects.";
    to_room = "$n feels more courageous and resistant to fear affects.";
    break;

  case SPELL_BESTOW_CURSE:

    roll = dice(1, 8);
    /* Roll of 1 = -6 to Str
     * Roll of 2 = -6 to Con
     * Roll of 3 = -6 to Dex
     * Roll of 4 = -6 to Int
     * Roll of 5 = -6 to Wis
     * Roll of 6 = -6 to Cha
     * Roll of 7 = -4 to attacks, saves, skills
     * Roll of 8 = 50% chance to miss attack each round
    */

    if (roll == 1) {
      af[0].bitvector = 0;
      af[0].location = APPLY_STR;
      af[0].modifier = -6;
      af[0].duration = -1;
    }
    else if (roll == 2) {
      af[0].bitvector = 0;
      af[0].location = APPLY_CON;
      af[0].modifier = -6;
      af[0].duration = -1;

    }
    else if (roll == 2) {
      af[0].bitvector = 0;
      af[0].location = APPLY_DEX;
      af[0].modifier = -6;
      af[0].duration = -1;

    }
    else if (roll == 2) {
      af[0].bitvector = 0;
      af[0].location = APPLY_INT;
      af[0].modifier = -6;
      af[0].duration = -1;

    }
    else if (roll == 2) {
      af[0].bitvector = 0;
      af[0].location = APPLY_WIS;
      af[0].modifier = -6;
      af[0].duration = -1;

    }
    else if (roll == 2) {
      af[0].bitvector = 0;
      af[0].location = APPLY_CHA;
      af[0].modifier = -6;
      af[0].duration = -1;

    }
    else if (roll == 2) {
      mag_affects(level, ch, victim, SPELL_BESTOW_CURSE_PENALTIES);
      return;
    }
    else if (roll == 2) {
      mag_affects(level, ch, victim, SPELL_BESTOW_CURSE_DAZE);
      return;
    }

    to_room = "$n briefly glows red!";
    to_vict = "You feel a great curse being laid upon you.";
    break;

  case SPELL_BESTOW_CURSE_PENALTIES:
    af[0].bitvector = 0;
    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = -1;
    to_room = "$n briefly glows red!";
    to_vict = "You feel a great curse being laid upon you.";
    break;

  case SPELL_BESTOW_CURSE_DAZE:
    af[0].bitvector = 0;
    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = -1;
    to_room = "$n briefly glows red!";
    to_vict = "You feel a great curse being laid upon you.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = level * 20;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = true;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_SEE_INVIS:
    af[0].duration = level * 20;
    af[0].bitvector = AFF_DETECT_INVIS;
    to_vict = "Your eyes become more sensative and perceptive to unseen forces.";
    to_room = "$n's eyes become more sensative and perceptive to unseen forces.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = 20 * level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = true;
    to_vict = "Your eyes tingle.";
    to_room = "$n's eyes tingle.";
    break;

  case SPELL_FAERIE_FIRE:
    af[0].location = APPLY_NONE;
    af[0].modifier = 0;
    af[0].duration = 10 * level;
    accum_duration = false;
    to_vict = "Your body flickers with a purplish light.";
    to_room = "$n's body flickers with with a purplish light.";
    break;

  case SPELL_DARKVISION:
    af[0].duration = level * 20;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = true;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_FIRE_SHIELD:

    af[0].duration = level;
    af[0].modifier = MIN(15, level);
    af[0].location = APPLY_ABILITY;
    af[0].bitvector = AFF_NONE;
    to_vict = "You are covered in a wreath of red and violet flames!";
    to_room = "$n is covered in a wreath of red and violent flames!";
    break;

  case SPELL_FREEDOM_OF_MOVEMENT:

    af[0].duration = level * 20;
    af[0].modifier = 1;
    af[0].location = APPLY_ABILITY;
    af[0].bitvector = AFF_NONE;
    to_vict = "You become immune to movement impairing effects.";
    to_room = "$n becomes immune to movement impairing effects.";
    break;


  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    if (affected_by_spell(victim, SPELL_FAERIE_FIRE)) {
      send_to_char(victim, "The inviisibility fails as you are covered in faerie fire and cannot be concealed.\r\n");
      return;
    }

    af[0].duration = level * 10;
    af[0].modifier = 40;
    af[0].location = APPLY_AC_DODGE;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = true;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_GREATER_INVISIBILITY:
    if (!victim)
      victim = ch;

    if (affected_by_spell(victim, SPELL_FAERIE_FIRE)) {
      send_to_char(victim, "The inviisibility fails as you are covered in faerie fire and cannot be concealed.\r\n");
      return;
    }

    af[0].duration = level;
    af[0].modifier = 40;
    af[0].location = APPLY_AC_DODGE;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = false;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_HOLD_PERSON:

    if (!IS_HUMANOID(victim) || GET_CLASS_RANKS(victim, CLASS_DRAGON_DISCIPLE) >= 10)
      return;

    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }

    af[0].location = APPLY_NONE;
    af[0].duration = level;
    af[0].modifier = 0;
    af[0].bitvector = AFF_PARALYZE;

    to_vict = "You suddenly freeze in place losing all ability to move whatsoever.";
    to_room = "$n suddenly freezes in place losing all ability to move whatsoever.";
    break;

  case SPELL_HOLD_MONSTER:

    if (GET_CLASS_RANKS(victim, CLASS_DRAGON_DISCIPLE) >= 10)
      return;

    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }

    af[0].location = APPLY_NONE;
    af[0].duration = level;
    af[0].modifier = 0;
    af[0].bitvector = AFF_PARALYZE;

    to_vict = "You suddenly freeze in place losing all ability to move whatsoever.";
    to_room = "$n suddenly freezes in place losing all ability to move whatsoever.";
    break;

  case SPELL_SLOW:

    if (affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT)) {
      act("@MYou resist the movement impairing effect due to your freedom of movement.@n", FALSE, victim, 0, ch, TO_CHAR);
      act("@M$n resists the movement impairing effect due to $s freedom of movement.@n", FALSE, victim, 0, ch, TO_ROOM);
      return;
    }

    af[0].location = APPLY_REFLEX;
    af[0].duration = level;
    af[0].modifier = -1;
    af[0].bitvector = AFF_NONE;

    to_vict = "You begin to slow down considerably as a magical weight burdens your entire body.";
    to_room = "$n begins to slow down considerably as a magical weight burdens $s entire body.";
    break;

  case SPELL_DELAY_POISON:
    af[0].location = APPLY_NONE;
    af[0].duration = level * 20;
    af[0].modifier = 0;
    af[0].bitvector = AFF_NONE;

    to_vict = "Your consititution is enhanced against the effects of poisons negating future effects completely.";
    to_room = "$n's consititution is enhanced against the effects of poisons negating future effects completely.";
    break;

  case SPELL_SILENCE:

    af[0].location = APPLY_NONE;
    af[0].duration = level * 2;
    af[0].modifier = 0;
    af[0].bitvector = AFF_SILENCE;

    to_vict = "Your vocal cords freeze making them unable to make the slightest sound.";
    to_room = "$n's vocal cords freeze making them unable to make the slightest sound.";
    break;


  case SPELL_POISON:

  	if (affected_by_spell(victim, SPELL_DELAY_POISON) || HAS_FEAT(ch, FEAT_ESSENCE_OF_UNDEATH) || HAS_FEAT(ch, FEAT_DIAMOND_BODY)) {
  		to_vict = "You feel slightly ill for a moment but the feeling quickly passes.";
  		break;
  	}
  	mod = dice(1, 10);

    af[0].location = APPLY_CON;
    af[0].duration = level * 10;
    af[0].modifier = -mod;
    af[0].bitvector = AFF_POISON;

    af[1].location = APPLY_HIT;
    af[1].duration = level * 10;
    af[1].modifier = -(GET_LEVEL(victim) * (mod / 2));
    af[1].bitvector = AFF_POISON;

    af[2].location = APPLY_NONE;
    af[2].type = SPELL_POISON_TIMER;
    af[2].duration = 5;
    af[2].modifier = 14 + level + ability_mod_value(GET_WIS(ch));
    af[2].bitvector = AFF_NONE;

    GET_POISON_DAMAGE_TYPE(victim) = APPLY_CON;
    GET_POISON_DAMAGE_AMOUNT(victim) = dice(1, 10);

    accum_affect = true;

    GET_HIT(victim) -= GET_LEVEL(victim) * (mod / 2);

    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";

    break;

  case SPELL_SOUND_BURST:

    af[0].location = APPLY_NONE;
    af[0].duration = 2;
    af[0].modifier = 0;
    af[0].bitvector = AFF_STUNNED;

    to_vict = "The sonic boom knocks the wind out of you, stunning you for a moment.";
    to_room = "The sonic boom knocks the wind out of $n, stunning $m for a moment.";
    break;


  case SPELL_UNDETECTABLE_ALIGNMENT:

    af[0].location = APPLY_NONE;
    af[0].duration = 24 * 20;
    af[0].modifier = 0;
    af[0].bitvector = AFF_NO_ALIGN;

    to_vict = "Your alignment is now concealed from detection.";
    break;

  case SPELL_DEATH_KNELL:

    if (victim == ch && FIGHTING(ch))
      victim = FIGHTING(ch);

    if (GET_HIT(victim) > GET_MAX_HIT(victim) / 10) {
      send_to_char(ch, "Your death knell causes your victim to shudder a moment, but otherwise has no effect.\r\n");
      break;
    }

    mod = dice(1, 8);

    af[0].location = APPLY_STR;
    af[0].duration = GET_LEVEL(victim) * 20;
    af[0].modifier = 2;
    af[0].bitvector = AFF_NONE;

    af[1].location = APPLY_HIT;
    af[1].duration = GET_LEVEL(victim) * 20;
    af[1].modifier = mod;
    af[1].bitvector = AFF_NONE;

    af[2].location = APPLY_HITROLL;
    af[2].duration = GET_LEVEL(victim) * 20;
    af[2].modifier = 1;
    af[2].bitvector = AFF_NONE;

    GET_HIT(ch) += mod;

    to_char = "Your death knell suffuses you with the lifeforce of your victim.";
    to_room = "$n's death knell suffuses $m with the lifeforce of $s victim.";

    victim = ch;

    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration = level * 10;
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration = false;
    to_vict = "You gain increased invulnerability against evil!";
    to_room = "$n gains increased invulnerability against evil!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = level;
    af[0].bitvector = AFF_SANCTUARY;

    accum_duration = true;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SLEEP_SINGLE:
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;

    if (IS_HALF_ELF(victim) || IS_ELF(victim))
      return;

    if (GET_LEVEL(ch) > (MAX(4, level / 2)))
      return;

    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level;
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", true, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_DEEP_SLUMBER:
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP) || IS_BOSS_MOB(victim))
      return;

    if (IS_HALF_ELF(victim) || IS_ELF(victim))
      return;

    if (GET_LEVEL(ch) > (MAX(10, level)))
      return;

    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].duration = level;
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", true, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

 case SPELL_MAGIC_WEAPON:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].duration = level * 50;
    af[0].modifier = 1;
    to_vict = "Your weapons have been slightly enhanced.";
    to_room = "$n's weapons have been slightly enhanced.";
    break;

 case SPELL_MAGIC_FANG:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_NONE;
    af[0].duration = level * 50;
    af[0].modifier = 1;
    to_vict = "Your natural weapons have been slightly enhanced.";
    to_room = "$n's natural weapons have been slightly enhanced.";
    break;

 case SPELL_MAGIC_VESTMENT:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].duration = level * 50;
    af[0].modifier = MIN(50, (level / 4) * 10);
    to_vict = "Your armor and shield has been slightly enhanced.";
    to_room = "$n's armor and shield has been slightly enhanced.";
    break;

 case SPELL_GREATER_MAGIC_WEAPON:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].duration = level * 50;
    af[0].modifier = MIN(5, level / 4);
    to_vict = "Your weapons have been enhanced.";
    to_room = "$n's weapons have been enhanced.";
    break;

 case SPELL_GREATER_MAGIC_FANG:
    af[0].bitvector = AFF_NONE;
    af[0].location = APPLY_ABILITY;
    af[0].duration = level * 50;
    af[0].modifier = MIN(5, level / 4);
    to_vict = "Your natural weapons have been enhanced.";
    to_room = "$n's natural weapons have been enhanced.";
    break;

 case SPELL_BULL_STRENGTH:
    af[0].location = APPLY_STR;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "You feel stronger!";
    to_room = "$n looks stronger!";
    break;

 case SPELL_FOX_CUNNING:
    af[0].location = APPLY_INT;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "You feel more cunning!";
    to_room = "$n looks more cunning!";
    break;

 case SPELL_CAT_GRACE:
    af[0].location = APPLY_DEX;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "You feel more agile!";
    to_room = "$n looks more agile!";
    break;

  case SPELL_BEARS_ENDURANCE:
    af[0].location = APPLY_CON;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "Your endurance is heightened!";
    to_room = "$n's endurance is heightened!";
    break;

  case SPELL_OWLS_WISDOM:
    af[0].location = APPLY_WIS;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "Your wisdom is heightened!";
    to_room = "$n's wisdom is heightened!";
    break;

  case SPELL_EAGLES_SPLENDOR:
    af[0].location = APPLY_CHA;
    af[0].duration = level * 10;
    af[0].modifier = 4;
    to_vict = "Your charisma is heightened!";
    to_room = "$n's charisma is heightened!";
    break;

  case SPELL_ENLARGE_PERSON:
    af[0].location = APPLY_STR;
    af[0].duration = level * 10;
    af[0].modifier = 2;
    af[1].location = APPLY_DEX;
    af[1].duration = level * 10;
    af[1].modifier = -2;
    to_vict = "You have grown one size category larger!";
    to_room = "$n has grown one size category larger!";
    break;

  case SPELL_AID:
    af[0].location = APPLY_HITROLL;
    af[0].duration = level * 10;
    af[0].modifier = 1;
    af[1].location = APPLY_WILL;
    af[1].duration = level * 10;
    af[1].modifier = 1;
    af[2].location = APPLY_HIT;
    af[2].duration = level * 10;
    af[2].modifier = dice(1, 8) + MIN(1, level);
    to_vict = "You have received the aid of the gods!";
    to_room = "$n has received the aid of the gods!";
    break;

  case SPELL_LESSER_GLOBE_OF_INVUL:
    af[0].location = APPLY_ABILITY;
    af[0].modifier = 1;
    af[0].bitvector = AFF_NONE;
    af[0].duration = level;
    to_vict = "A barely visible globe with a greenish tinge surrounds you.";
    to_room = "A barely visible globe with a greenish tinge surrounds $n.";
    break;



  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    to_room = "$n's awareness improves.";
    af[0].duration = level * 10;
    af[0].bitvector = AFF_SENSE_LIFE;
    break;

  case SPELL_WATERWALK:
    af[0].duration = level * 20;
    af[0].bitvector = AFF_WATERWALK;
    to_vict = "You feel webbing between your toes.";
    to_room = "$n gains webbing between $s toes.";
    break;

  }


  if (PRF_FLAGGED(ch, PRF_EXTEND_SPELL)) {
    for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
      if (af[i].duration > 0) {
        af[i].duration *= 15;
        af[i].duration /= 10;
      }
    }
  }


  if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
    for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
      af[i].modifier *= 15;
      af[i].modifier /= 10;
    }
  }


  /*
   * if this is a mob that has this affect set in its mob file, do not
   * perform the affect.  this prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector) && af[i].bitvector != AFF_NONE && af[i].bitvector != 0) {
	send_to_char(ch, "%s", CONFIG_NOEFFECT);
	return;
      }

  /*
   * if the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    while (affected_by_spell(victim, spellnum))
      affect_from_char(victim, spellnum);
    affect_join(victim, af+i, accum_duration, false, accum_affect, false);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    if (af[i].bitvector || (af[i].location != APPLY_NONE)) {
        affect_join(victim, af+i, accum_duration, false, accum_affect, false);
    }
  }

  if (to_vict != NULL)
    act(to_vict, false, victim, 0, ch, TO_CHAR);
  if (!IS_NPC(ch) && (!FIGHTING(ch) || ch->active_turn != 1)) {
    if (to_room != NULL)
      act(to_room, true, victim, 0, ch, TO_ROOM);
  }
  if (to_char != NULL)
    act(to_char, true, ch, 0, victim, TO_CHAR);

  if (GET_CON(victim) < 1)
  	die(victim, ch);
}


/*
 * this function is used to provide services to mag_groups.  this function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, int spellnum)
{
  switch (spellnum) {
  case SPELL_MASS_HEAL:
    mag_points(level, ch, tch, SPELL_MASS_HEAL);
    mag_unaffects(level, ch, tch, SPELL_HEAL);
    break;
  case SPELL_REMOVE_PARALYSIS:
    mag_unaffects(level, ch, tch, SPELL_REMOVE_PARALYSIS);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_MAGE_ARMOR);
    break;
  case SPELL_MASS_BULLS_STRENGTH:
    mag_affects(level, ch, tch, SPELL_BULL_STRENGTH);
    break;
  case SPELL_MASS_BEARS_ENDURANCE:
    mag_affects(level, ch, tch, SPELL_BEARS_ENDURANCE);
    break;
  case SPELL_MASS_CATS_GRACE:
    mag_affects(level, ch, tch, SPELL_CAT_GRACE);
    break;
  case SPELL_MASS_FOXS_CUNNING:
    mag_affects(level, ch, tch, SPELL_FOX_CUNNING);
    break;
  case SPELL_MASS_OWLS_WISDOM:
    mag_affects(level, ch, tch, SPELL_OWLS_WISDOM);
    break;
  case SPELL_MASS_EAGLES_SPLENDOR:
    mag_affects(level, ch, tch, SPELL_EAGLES_SPLENDOR);
    break;
  case SPELL_MASS_AID:
    mag_affects(level, ch, tch, SPELL_AID);
    break;
  case SPELL_MASS_ENLARGE_PERSON:
    mag_affects(level, ch, tch, SPELL_ENLARGE_PERSON);
    break;
  case SPELL_MAGIC_CIRCLE_AGAINST_EVIL:
    mag_affects(level, ch, tch, SPELL_PROT_FROM_EVIL);
    break;
  case SPELL_LIGHT:
    mag_affects(level, ch, tch, SPELL_LIGHT);
    break;
  case SPELL_DAYLIGHT:
    mag_affects(level, ch, tch, SPELL_DAYLIGHT);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL, NULL);
    break;
  case SPELL_AURA_OF_GOOD:
    mag_affects(level, ch, tch, SPELL_PROT_FROM_EVIL);
    break;
  case SPELL_AURA_OF_COURAGE:
    mag_affects(level, ch, tch, SPELL_COURAGE);
    break;
  case SPELL_BLESS:
    mag_affects(level, ch, tch, SPELL_BLESS_SINGLE);
    break;
  case SPELL_HASTE:
    mag_affects(level, ch, tch, SPELL_HASTE);
    break;
  case SPELL_COUNTERSONG:
    mag_affects(level, ch, tch, SPELL_COUNTERSONG);
    break;
  case SPELL_INSPIRE_COURAGE:
    mag_affects(level, ch, tch, SPELL_INSPIRE_COURAGE);
    break;
  case SPELL_INSPIRE_GREATNESS:
    mag_affects(level, ch, tch, SPELL_INSPIRE_GREATNESS);
    break;
  case SPELL_CALM_ANIMAL:
    mag_affects(level, ch, tch, SPELL_CALM_ANIMAL);
    break;
  case SPELL_PRAYER:
    mag_affects(level, ch, tch, SPELL_PRAYER);
    break;
  case SPELL_LESSER_MASS_REJUVENATE:
    mag_points(level, ch, tch, SPELL_LIGHT_REFRESH);
    break;
  case SPELL_MINOR_MASS_REJUVENATE:
    mag_points(level, ch, tch, SPELL_CRITICAL_REFRESH);
    break;
  case SPELL_MASS_REJUVENATE:
    mag_points(level, ch, tch, SPELL_REJUVENATE);
    break;
  }
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum);
  perform_mag_groups(level, ch, ch, spellnum);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *tch_next;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
      case SPELL_SLEEP:
        mag_affects(level, ch, tch, SPELL_SLEEP_SINGLE);
        break;
      case SPELL_DEEP_SLUMBER:
        mag_affects(level, ch, tch, SPELL_DEEP_SLUMBER);
        break;
      case SPELL_ENTANGLE:
        mag_affects(level, ch, tch, SPELL_ENTANGLE);
        break;
      default:
        break;
    }
  }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  this calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;
  int i=0, numtimes=0, roll=0;


  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
  case SPELL_MASS_HARM:
    spellnum = SPELL_HARM;
    break;
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room ="$n gracefully gestures and the earth begins to shake violently!";
    break;
  }

  if (to_char != NULL)
    act(to_char, false, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, false, ch, 0, 0, TO_ROOM);


  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     *		  5: grouped pcs
     *		  5: people not fighting the caster's group if caster fighting
     *		  5: people not fighting if caster fighting
     */

    if (tch == ch)
      continue;
    if (ADM_FLAGGED(tch, ADM_NODAMAGE))
      continue;
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(tch))
      continue;
    if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (FIGHTING(ch) && FIGHTING(tch) && !is_player_grouped(FIGHTING(tch), ch))
      continue;
    if (PRF_FLAGGED(ch, PRF_CONTAINED_AREAS) && !FIGHTING(tch))
      continue;
    if (IS_NPC(ch) && IS_NPC(tch) && !tch->master && !AFF_FLAGGED(ch, AFF_CHARM))
      continue;
    if (IS_NPC(ch) && !IS_NPC(tch) && AFF_FLAGGED(ch, AFF_CHARM) && !is_player_grouped(FIGHTING(tch), ch->master))
      continue;
    if (GET_POS(tch) == POS_DEAD)
      continue;

    if ((tch->master == ch || ch->master == tch || ch->master == tch->master) &&
        AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(tch, AFF_GROUP))
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    if (spellnum == SPELL_LIGHTNING_BOLT) {
      roll = dice(1, 100);
      if (roll < 25) {
        numtimes = 0;
      } else if (roll < 68 && !OUTSIDE(ch)) {
        numtimes = 2;
      } else {
        numtimes = 1;
      }
      for (i = 0; i < numtimes; i++) {
        if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && IS_NPC(tch) && !AFF_FLAGGED(tch, AFF_CHARM))
          continue;
        mag_damage(level, ch, tch, spellnum);
      }
      if (numtimes == 0)
        damage(ch, tch, 0, spellnum, 0, -1, 0, spellnum, 1);
    }
    else
      mag_damage(level, ch, tch, spellnum);
  }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 */

mob_vnum monsum_list_lg_1[] = { 100, NOBODY };
mob_vnum monsum_list_ng_1[] = { 100, NOBODY };
mob_vnum monsum_list_cg_1[] = { 100, NOBODY };
mob_vnum monsum_list_ln_1[] = { 100, 101, NOBODY };
mob_vnum monsum_list_nn_1[] = { 100, 101, NOBODY };
mob_vnum monsum_list_cn_1[] = { 100, 101, NOBODY };
mob_vnum monsum_list_le_1[] = { 101, NOBODY };
mob_vnum monsum_list_ne_1[] = { 101, NOBODY };
mob_vnum monsum_list_ce_1[] = { 101, NOBODY };

mob_vnum monsum_list_lg_2[] = { 102, NOBODY };
mob_vnum monsum_list_ng_2[] = { 102, NOBODY };
mob_vnum monsum_list_cg_2[] = { 102, NOBODY };
mob_vnum monsum_list_ln_2[] = { 102, 103, NOBODY };
mob_vnum monsum_list_nn_2[] = { 102, 103, NOBODY };
mob_vnum monsum_list_cn_2[] = { 102, 103, NOBODY };
mob_vnum monsum_list_le_2[] = { 103, NOBODY };
mob_vnum monsum_list_ne_2[] = { 103, NOBODY };
mob_vnum monsum_list_ce_2[] = { 103, NOBODY };

mob_vnum monsum_list_lg_3[] = { 104, NOBODY };
mob_vnum monsum_list_ng_3[] = { 104, NOBODY };
mob_vnum monsum_list_cg_3[] = { 104, NOBODY };
mob_vnum monsum_list_ln_3[] = { 104, 105, NOBODY };
mob_vnum monsum_list_nn_3[] = { 104, 105, NOBODY };
mob_vnum monsum_list_cn_3[] = { 104, 105, NOBODY };
mob_vnum monsum_list_le_3[] = { 105, NOBODY };
mob_vnum monsum_list_ne_3[] = { 105, NOBODY };
mob_vnum monsum_list_ce_3[] = { 105, NOBODY };

mob_vnum monsum_list_lg_4[] = { 106, NOBODY };
mob_vnum monsum_list_ng_4[] = { 106, NOBODY };
mob_vnum monsum_list_cg_4[] = { 106, NOBODY };
mob_vnum monsum_list_ln_4[] = { 106, 107, NOBODY };
mob_vnum monsum_list_nn_4[] = { 106, 107, NOBODY };
mob_vnum monsum_list_cn_4[] = { 106, 107, NOBODY };
mob_vnum monsum_list_le_4[] = { 107, NOBODY };
mob_vnum monsum_list_ne_4[] = { 107, NOBODY };
mob_vnum monsum_list_ce_4[] = { 107, NOBODY };

mob_vnum monsum_list_lg_5[] = { 108, NOBODY };
mob_vnum monsum_list_ng_5[] = { 108, NOBODY };
mob_vnum monsum_list_cg_5[] = { 108, NOBODY };
mob_vnum monsum_list_ln_5[] = { 108, 109, NOBODY };
mob_vnum monsum_list_nn_5[] = { 108, 109, NOBODY };
mob_vnum monsum_list_cn_5[] = { 108, 109, NOBODY };
mob_vnum monsum_list_le_5[] = { 109, NOBODY };
mob_vnum monsum_list_ne_5[] = { 109, NOBODY };
mob_vnum monsum_list_ce_5[] = { 109, NOBODY };

mob_vnum monsum_list_lg_6[] = { 111, NOBODY };
mob_vnum monsum_list_ng_6[] = { 111, NOBODY };
mob_vnum monsum_list_cg_6[] = { 111, NOBODY };
mob_vnum monsum_list_ln_6[] = { 111, 112, NOBODY };
mob_vnum monsum_list_nn_6[] = { 111, 112, NOBODY };
mob_vnum monsum_list_cn_6[] = { 111, 112, NOBODY };
mob_vnum monsum_list_le_6[] = { 112, NOBODY };
mob_vnum monsum_list_ne_6[] = { 112, NOBODY };
mob_vnum monsum_list_ce_6[] = { 112, NOBODY };

mob_vnum monsum_list_lg_7[] = { 113, NOBODY };
mob_vnum monsum_list_ng_7[] = { 113, NOBODY };
mob_vnum monsum_list_cg_7[] = { 113, NOBODY };
mob_vnum monsum_list_ln_7[] = { 113, 114, NOBODY };
mob_vnum monsum_list_nn_7[] = { 113, 114, NOBODY };
mob_vnum monsum_list_cn_7[] = { 113, 114, NOBODY };
mob_vnum monsum_list_le_7[] = { 114, NOBODY };
mob_vnum monsum_list_ne_7[] = { 114, NOBODY };
mob_vnum monsum_list_ce_7[] = { 114, NOBODY };

mob_vnum monsum_list_lg_8[] = { 115, NOBODY };
mob_vnum monsum_list_ng_8[] = { 115, NOBODY };
mob_vnum monsum_list_cg_8[] = { 115, NOBODY };
mob_vnum monsum_list_ln_8[] = { 115, 116, NOBODY };
mob_vnum monsum_list_nn_8[] = { 115, 116, NOBODY };
mob_vnum monsum_list_cn_8[] = { 115, 116, NOBODY };
mob_vnum monsum_list_le_8[] = { 116, NOBODY };
mob_vnum monsum_list_ne_8[] = { 116, NOBODY };
mob_vnum monsum_list_ce_8[] = { 116, NOBODY };

mob_vnum monsum_list_lg_9[] = { 117, NOBODY };
mob_vnum monsum_list_ng_9[] = { 117, NOBODY };
mob_vnum monsum_list_cg_9[] = { 117, NOBODY };
mob_vnum monsum_list_ln_9[] = { 117, 118, NOBODY };
mob_vnum monsum_list_nn_9[] = { 117, 118, NOBODY };
mob_vnum monsum_list_cn_9[] = { 117, 118, NOBODY };
mob_vnum monsum_list_le_9[] = { 118, NOBODY };
mob_vnum monsum_list_ne_9[] = { 118, NOBODY };
mob_vnum monsum_list_ce_9[] = { 118, NOBODY };

mob_vnum *monsum_list[9][9] = {
  { monsum_list_lg_1, monsum_list_ng_1, monsum_list_cg_1,
    monsum_list_ln_1, monsum_list_nn_1, monsum_list_cn_1,
    monsum_list_le_1, monsum_list_ne_1, monsum_list_ce_1 },
  { monsum_list_lg_2, monsum_list_ng_2, monsum_list_cg_2,
    monsum_list_ln_2, monsum_list_nn_2, monsum_list_cn_2,
    monsum_list_le_2, monsum_list_ne_2, monsum_list_ce_2 },
  { monsum_list_lg_3, monsum_list_ng_3, monsum_list_cg_3,
    monsum_list_ln_3, monsum_list_nn_3, monsum_list_cn_3,
    monsum_list_le_3, monsum_list_ne_3, monsum_list_ce_3 },
  { monsum_list_lg_4, monsum_list_ng_4, monsum_list_cg_4,
    monsum_list_ln_4, monsum_list_nn_4, monsum_list_cn_4,
    monsum_list_le_4, monsum_list_ne_4, monsum_list_ce_4 },
  { monsum_list_lg_5, monsum_list_ng_5, monsum_list_cg_5,
    monsum_list_ln_5, monsum_list_nn_5, monsum_list_cn_5,
    monsum_list_le_5, monsum_list_ne_5, monsum_list_ce_5 },
  { monsum_list_lg_6, monsum_list_ng_6, monsum_list_cg_6,
    monsum_list_ln_6, monsum_list_nn_6, monsum_list_cn_6,
    monsum_list_le_6, monsum_list_ne_6, monsum_list_ce_6 },
  { monsum_list_lg_7, monsum_list_ng_7, monsum_list_cg_7,
    monsum_list_ln_7, monsum_list_nn_7, monsum_list_cn_7,
    monsum_list_le_7, monsum_list_ne_7, monsum_list_ce_7 },
  { monsum_list_lg_8, monsum_list_ng_8, monsum_list_cg_8,
    monsum_list_ln_8, monsum_list_nn_8, monsum_list_cn_8,
    monsum_list_le_8, monsum_list_ne_8, monsum_list_ce_8 },
  { monsum_list_lg_9, monsum_list_ng_9, monsum_list_cg_9,
    monsum_list_ln_9, monsum_list_nn_9, monsum_list_cn_9,
    monsum_list_le_9, monsum_list_ne_9, monsum_list_ce_9 }
};

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = 
{
  "\r\n",
  "$n animates a corpse!",
  "$n summons extraplanar assistance!",
  "$n summons an undead creature to $s side!"
};

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE	20	/* Only one for now. */
#define MOB_ZOMBIE		30020
#define MOB_AERIALSERVANT	19
#define MOB_DRAGON_KNIGHT	30019

void mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, const char *arg)
{

  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int msg = 0, num = 1, handle_corpse = false, affs = 0, affvs = 0, assist = 0, i, j, count;
  char *buf = NULL, buf2[MAX_INPUT_LENGTH]={'\0'};
  int lev;
  mob_vnum mob_num;
  int corpse_race = 0;
  int mob_level = 0;
  byte celestial = FALSE;

  if (ch == NULL)
    return;

  switch (spellnum) {

  case SPELL_SUMMON_MONSTER_I:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_I:
    mob_num = PET_DOG;
    break;

  case SPELL_SUMMON_MONSTER_II:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_II:
    mob_num = PET_WOLF;
    break;

  case SPELL_SUMMON_MONSTER_III:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_III:
    mob_num = PET_BLACK_BEAR;
    break;

  case SPELL_SUMMON_MONSTER_IV:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_IV:
    mob_num = PET_LION;
    break;

  case SPELL_SUMMON_MONSTER_V:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_V:
    mob_num = PET_GRIFFON;
    break;

  case SPELL_SUMMON_MONSTER_VI:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_VI:
    mob_num = PET_DIRE_LION;
    break;

  case SPELL_SUMMON_MONSTER_VII:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_VII:
    mob_num = PET_MEGARAPTOR;
    break;

  case SPELL_SUMMON_MONSTER_VIII:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_VIII:
    mob_num = PET_TRICERATOPS;
    break;

  case SPELL_SUMMON_MONSTER_IX:
    celestial = TRUE;
  case SPELL_SUMMON_NATURE_IX:
    mob_num = PET_ROC;
    break;

  case SPELL_SUMMON_UNDEAD:
    if (level <= 11)
      mob_num = PET_GHOUL;
    else if (level <= 14)
      mob_num = PET_GHAST;
    else if (level <= 17)
      mob_num = PET_MUMMY;
    else
      mob_num = PET_MOHRG;
    break;

  case SPELL_DRAGON_KNIGHT:
    if (IS_EVIL(ch))
      mob_num = PET_ADULT_RED_DRAGON;
    else
      mob_num = PET_ADULT_SILVER_DRAGON;
    break;

  default:
    send_to_char(ch, "Summons for this spell have not yet been done.  We will add it asap.  Thank you for your patience.\r\n");
    return;  

  }

  if (mob_num <= 0 || mob_num >= NUM_PETS) {
    send_to_char(ch, "Summons for this spell have not yet been done.  We will add it asap.  Thank you for your patience.\r\n");
    return;    
  }

  if (ch->player_specials->summon_num > 0) {
    send_to_char(ch, "You dismiss %s from your service.\r\n", ch->player_specials->summon_desc ? ch->player_specials->summon_desc : "your current summon");
    free_summon(ch);
  }

  // assign the pet stats to the character's summon vars
  ch->player_specials->summon_num = mob_num;
  ch->player_specials->summon_desc = strdup(pet_list[mob_num].desc);
  ch->player_specials->summon_max_hit = pet_list[mob_num].max_hit + ((HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING)) ? pet_list[mob_num].level * 2 : 0);
  ch->player_specials->summon_cur_hit = ch->player_specials->summon_max_hit;
  ch->player_specials->summon_ac = MAX(150 + (MIN(GET_CLASS_LEVEL(ch), pet_list[mob_num].level) * 10 / 2), MIN(GET_CLASS_LEVEL(ch), pet_list[mob_num].level) * 15);
  ch->player_specials->summon_dr = pet_list[mob_num].dr;
  for (i = 0; i < 5; i++) {
      ch->player_specials->summon_attack_to_hit[i] = pet_list[mob_num].attacks_to_hit[i];
      ch->player_specials->summon_attack_ndice[i] = pet_list[mob_num].attacks_ndice[i];
      ch->player_specials->summon_attack_sdice[i] = pet_list[mob_num].attacks_sdice[i];
      ch->player_specials->summon_attack_dammod[i] = pet_list[mob_num].attacks_dammod[i];
  }

  if (celestial)
  {
    if (pet_list[mob_num].level > 3 && pet_list[mob_num].level < 12)
      ch->player_specials->summon_dr = MAX(5, ch->player_specials->summon_dr);
    else if (pet_list[mob_num].level > 11)
      ch->player_specials->summon_dr = MAX(10, ch->player_specials->summon_dr);
    sprintf(buf2, "%s %s", ch->player_specials->summon_desc, (IS_GOOD(ch) || (IS_NEUTRAL(ch) && dice(1, 10) %2 == 0)) ? "(celestial)" : "(fiendish)");
    free(ch->player_specials->summon_desc);
    ch->player_specials->summon_desc = strdup(buf2);
  }

  ch->player_specials->summon_timer = 10 + (level / 3);

  sprintf(buf2, "You summon forth %s!", ch->player_specials->summon_desc);
  act(buf2, true, ch, 0, 0, TO_CHAR);  
  sprintf(buf2, "$n summons forth %s!", ch->player_specials->summon_desc);
  act(buf2, true, ch, 0, 0, TO_ROOM);  

  return;

  send_to_char(ch, "The summon variety of spells is currently disabled as they are causing\r\n"
                   "an inordiante number of crashes.  We are going to reimplement the summon/pet\r\n"
                   "system, but until then, summoning mobs will be disabled.\r\n");
  return;


  if (FIGHTING(ch)) {
    send_to_char(ch, "Using summon spells in battle is currently disabled.  We will fix this asap.  Thanks for your patience.\r\n");
    return;
  }

  lev = spell_info[spellnum].spell_level;

  switch (spellnum) {
  case SPELL_DRAGON_KNIGHT:
    mob_num = MOB_DRAGON_KNIGHT;
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL) {
      send_to_char(ch, "With what corpse?\r\n");
      return;
    }
    if (!IS_CORPSE(obj)) {
      send_to_char(ch, "That's not a corpse!\r\n");
      return;
    }
    corpse_race = GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE_RACE);
    if (race_list[corpse_race].family == RACE_TYPE_CONSTRUCT ||
       race_list[corpse_race].family == RACE_TYPE_OOZE ||
       race_list[corpse_race].family == RACE_TYPE_PLANT ||
       race_list[corpse_race].family == RACE_TYPE_ELEMENTAL) {
       send_to_char(ch, "You can only animate corpses with standard anatomies.\r\n");
       return;
    }
    handle_corpse = true;
    msg = 1;
    mob_num = MOB_ZOMBIE;
    break;

  case SPELL_SUMMON_UNDEAD:
    msg = 1;
    mob_num = MOB_ZOMBIE + MIN(4, ((level - 9) / 3) + 1);
    break;

  case SPELL_SUMMON_GREATER_UNDEAD:
    msg = 1;
    mob_num = MOB_ZOMBIE + 4 + MIN(4, ((level - 14) / 2) + 1);
    break;

  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    
    mob_num = NOBODY;
    affvs = 1;
    assist = 1;
    if (arg) {
      buf = (char *)arg;
      skip_spaces(&buf);
      if (!*buf)
        buf = NULL;
    }
    j = ALIGN_TYPE(ch);
    if (buf) {
      buf = any_one_arg(buf, buf2);
      for (i = lev - 1; i >= 0; i--) {
        for (count = 0; monsum_list[i][j][count] != NOBODY; count++) {
          mob_num = monsum_list[i][j][count] -100 +30000;
          if (real_mobile(mob_num) == NOBODY)
            mob_num = NOBODY;
          else if (!is_name(buf2, mob_proto[real_mobile(mob_num)].name))
            mob_num = NOBODY;
          else
            break;
        }
        if (mob_num != NOBODY)
          break;
      }
      if (mob_num == NOBODY) {
        send_to_char(ch, "That's not a name for a monster you can summon. Summoning something else.\r\n");
      } else {
        log("lev=%d, i=%d, ngen=%d", lev, i, lev - i);
        switch (lev - i) {
        case 1:
          num = 1;
          break;
        case 2:
          num = rand_number(1, 3);
          break;
        default:
          num = rand_number(1, 4) + 1;
          break;
        }
      }
    }
    if (mob_num == NOBODY) {
      num = 1;
      for (count = 0; monsum_list[lev - 1][j][count] != NOBODY; count++);
      if (!count) {
        log("No monsums for spell level %d align %s", lev, alignments[j]);
        return;
      }
      count--;
      mob_num = monsum_list[lev - 1][j][rand_number(0, count)] - 100 + 30000;
    }
    break;

  default:
    return;
  }

  if (num_charmies(ch) > 0) {
      send_to_char(ch, "You can only control one creature at a time.\r\n");
      return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }

  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, virtual))) {
      send_to_char(ch, "You don't quite remember how to summon that creature.\r\n");
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    if (affs)
      mag_affects(level, ch, mob, spellnum);
    if (affvs)
      mag_affectsv(level, ch, mob, spellnum);
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

    if (spellnum == SPELL_ANIMATE_DEAD && mob && obj) {
			char *tmp;
      if (race_list[GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE_RACE)].family != RACE_TYPE_UNDEAD) {
        mob->name = strdup(replace_string(obj->name, "corpse", "zombie"));
        mob->short_descr = strdup(replace_string(obj->short_description, "corpse", "zombie"));
        mob->long_descr = strdup(replace_string(obj->description, "corpse", "zombie"));
        tmp = strdup(replace_string(mob->long_descr, "lying", "standing"));
				free(mob->long_descr);
        mob->long_descr = strdup(replace_string(tmp, "here.", "here.\r\n"));
				free(tmp);
      }
      else {
        mob->name = strdup(replace_string(obj->name, "corpse", ""));
        mob->short_descr = strdup(replace_string(obj->short_description, "the corpse of ", ""));
        mob->long_descr = strdup(replace_string(obj->description, "The corpse of ", ""));
        tmp = strdup(replace_string(mob->long_descr, "lying", "standing"));
				free(mob->long_descr);
        mob->long_descr = strdup(replace_string(tmp, "here.", "here.\r\n"));
				free(tmp);
      }
      mob_level = 8 + ((race_list[corpse_race].size - SIZE_MEDIUM) * 2);
      GET_HITDICE(mob) = mob_level;
      set_auto_mob_stats(mob);    
    }

    act(mag_summon_msgs[msg], false, ch, 0, mob, TO_ROOM);
    load_mtrigger(mob);
    add_follower(mob, ch);
    if (assist && FIGHTING(ch)) {
      set_fighting(mob, FIGHTING(ch));
    }
    mob->master_id = GET_IDNUM(ch);
  }

  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }

}


void mag_points(int level, struct char_data *ch, struct char_data *victim, int spellnum)
{
  int healing = 0, move = 0;
  int hndice = 0, hsdice = 0, hmod = 0, mndice = 0, msdice = 0, mmod = 0;
  int tmp;
  char buf[200]={'\0'};

  if (victim == NULL)
    return;

  if (IS_UNDEAD(victim)) {
    switch(spellnum) {
      case SPELL_CURE_LIGHT:
        mag_damage(level, ch, victim, SPELL_INFLICT_LIGHT);
        return;   
      case SPELL_CURE_MODERATE:
        mag_damage(level, ch, victim, SPELL_INFLICT_MODERATE);
        return;   
      case SPELL_CURE_SERIOUS:
        mag_damage(level, ch, victim, SPELL_INFLICT_SERIOUS);
        return;   
      case SPELL_CURE_CRITIC:
        mag_damage(level, ch, victim, SPELL_INFLICT_CRITIC);
        return;   
      case SPELL_HEAL:
      case SPELL_MASS_HEAL:
        mag_damage(level, ch, victim, SPELL_HARM);
        return;   
    }
  }

  switch (spellnum) {
  case SPELL_MINOR_REFRESH:
    move = dice(10, 8);
    mndice = 10;
    msdice = 8;
    break;
  case SPELL_LIGHT_REFRESH:
    move = dice(50, 8) + MIN(level * 3, 15);
    mndice = 50;
    msdice = 8;
    mmod = MIN(level * 3, 15);
    break;
  case SPELL_MODERATE_REFRESH:
    move = dice(80, 8) + MIN(level * 3, 30);
    mndice = 80;
    msdice = 8;
    mmod = MIN(level * 3, 30);
    break;
  case SPELL_SERIOUS_REFRESH:
    move = dice(120, 8) + MIN(level * 3, 45);
    mndice = 120;
    msdice = 8;
    mmod = MIN(level * 3, 45);
    break;
  case SPELL_CRITICAL_REFRESH:
    move = dice(200, 8) + MIN(level * 3, 60);
    mndice = 200;
    msdice = 8;
    mmod = MIN(level * 3, 60);
    break;
  case SPELL_REJUVENATE:
    move = GET_MAX_MOVE(victim);
    mndice = GET_MAX_MOVE(victim);
    msdice = 1;
    mmod = 0;
    break;
  case SPELL_CURE_MINOR:
    healing = dice(1, 3);
    hndice = 1;
    hsdice = 3; 
    send_to_char(victim, "You feel slightly better.\r\n");
    break;
  case SPELL_CURE_LIGHT:
    healing = dice(1, 8) + MIN(level, 5);
    hndice = 1;
    hsdice = 8;
    hmod = MIN(level, 5);    
    send_to_char(victim, "You feel a little better.\r\n");
    break;
  case SPELL_CURE_MODERATE:
    healing = dice(2, 8) + MIN(level, 10);
    hndice = 2;
    hsdice = 8;
    hmod = MIN(level, 10);
    send_to_char(victim, "You feel better.\r\n");
    break;
  case SPELL_CURE_SERIOUS:
    healing = dice(3, 8) + MIN(level, 15);
    hndice = 3;
    hsdice = 8;
    hmod = MIN(level, 15);
    send_to_char(victim, "You feel a lot better.\r\n");
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(4, 8) + MIN(level, 20);
    hndice = 4;
    hsdice = 8;
    hmod = MIN(level, 20);
    send_to_char(victim, "You feel considerably better!\r\n");
    break;
  case SPELL_HEAL:
    healing = MIN(150, level * 10);
    hndice = MIN(150, level * 10);
    hsdice = 1;
    hmod = 0;
    send_to_char(victim, "A warm feeling floods your body and your wounds are instantly healed.\r\n");
    if (AFF_FLAGGED(ch, AFF_CDEATH)) {
      affect_from_char(ch, ART_QUIVERING_PALM);
      send_to_char(ch, "Your nerves settle slightly\r\n");
    }
    break;
  case SPELL_MASS_HEAL:
    healing = MIN(250, level * 10);
    hndice = MIN(250, level * 10);
    hsdice = 1;
    hmod = 0;
    send_to_char(victim, "A warm feeling floods your body and your wounds are instantly healed.\r\n");
    if (AFF_FLAGGED(ch, AFF_CDEATH)) {
      affect_from_char(ch, ART_QUIVERING_PALM);
      send_to_char(ch, "Your nerves settle slightly\r\n");
    }
    break;
  case SPELL_HEAL_MOUNT:
    healing = MIN(150, level * 10);
    if (victim->player_specials->mounted == MOUNT_MOUNT) {
      victim->player_specials->mount_cur_hit += healing;
      victim->player_specials->mount_cur_hit += MIN(victim->player_specials->mount_cur_hit, victim->player_specials->mount_max_hit);
      sprintf(buf, "You heal $N's mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_CHAR);
      sprintf(buf, "$n heals your mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_VICT);
      sprintf(buf, "$n heals $N's mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
      return;
    } else if (victim->player_specials->mounted == MOUNT_SUMMON) {
      victim->player_specials->summon_cur_hit += healing;
      victim->player_specials->summon_cur_hit += MIN(victim->player_specials->summon_cur_hit, victim->player_specials->summon_max_hit);
      sprintf(buf, "You heal $N's mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_CHAR);
      sprintf(buf, "$n heals your mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_VICT);
      sprintf(buf, "$n heals $N's mount for %d hit points!", healing);
      act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
      return;
    } else {
      send_to_char(ch, "That person does not have a mount.\r\n");
      return;
    }
    break;
  case ART_WHOLENESS_OF_BODY:
    healing = GET_MAX_HIT(victim) - GET_HIT(victim);
    hndice = GET_MAX_HIT(victim) - GET_HIT(victim);
    hsdice = 1;
    hmod = 0;
    tmp = GET_KI(ch) / 2;
    if (tmp > hndice)
      tmp = hndice;
    else {
      hndice = tmp;
    }
    GET_KI(ch) -= tmp * 2;
    send_to_char(ch, "You have healed yourself for %d points of damage.\r\n", healing);
    break;
  }

  if (PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL) || PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL)) {
    if (hndice > 0)
      healing = (hndice * hsdice) + hmod;
    if (mndice > 0)
      move = (mndice * msdice) + mmod;
  }
  else {
    if (hndice > 0)
      healing = dice(hndice, hsdice) + hmod;
    if (mndice > 0)
      move = dice(mndice, msdice) + mmod;
  }

  if (PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL)) {
    healing *= 2;
    move *= 2;
  }
    
  if (HAS_DOMAIN(ch, DOMAIN_HEALING) && hndice > 0) {
    healing *= 15;
    healing /= 10;
  }

  if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
    healing = healing * 15 / 10;
    move = move * 15 / 10;
  }

  if (healing > 0) {
    if (GET_FIGHT_BLEEDING_DAMAGE(victim))    
      send_to_char(victim, "Your bleeding stops.\r\n");
    GET_FIGHT_BLEEDING_DAMAGE(victim) = 0;
  }

  if (PRF_FLAGGED(victim, PRF_PVP))
    healing = MAX(1, healing / 5);

  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  if (healing > 0) {
    if (victim != ch) {
      send_to_char(ch, "You heal @y%s@n for @y%d@n damage.\r\n", GET_NAME(victim), healing);
      send_to_char(victim, "@y%s@n heals you for @y%d@n damage.\r\n", GET_NAME(ch), healing);
    }
    else
      send_to_char(ch, "You heal @yyourself@n for @y%d@n damage.\r\n", healing);
  }
  if (move > 0) {
    if (victim != ch) {
      send_to_char(ch, "You restore @y%d@n moves to @y%s@n.\r\n", move, GET_NAME(victim));
      send_to_char(victim, "@y%s@n restores @y%d@n moves to you.\r\n", GET_NAME(victim), move);
    }
    else
      send_to_char(ch, "You restore @y%d@n moves to @yyourself@n.\r\n", move);
  }
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data *ch, struct char_data *victim, int spellnum)
{
  int spell = 0, msg_not_affected = true;
  const char *to_vict = NULL, *to_room = NULL;
  int spell2 = 0;
  int spell3 = 0;
  int spell4 = 0;
  int found = FALSE;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_HEAL_MOUNT:
    if (!(IS_NPC(victim) && GET_MOB_VNUM(victim) == 199 && victim->master == ch))
      break;
  case SPELL_HEAL:
    /*
     * Heal also restores health, so don't give the "no effect" message
     * if the target isn't afflicted by the 'blindness' spell.
     */
    msg_not_affected = false;
    /* fall-through */
    spell = SPELL_BLINDNESS;
    spell2 = SPELL_POISON;
    to_room = "$n is suddenly healed of all ailments.";
    to_vict = "You are suddenly healed of all ailments.";
    break;
  case SPELL_FAERIE_FIRE:
    spell = SPELL_INVISIBLE;
    spell2 = SPELL_BLUR;
    REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_SNEAK);
    msg_not_affected = false;
    to_vict = "All concealments upon you are nullified.";
    break;
  case SPELL_REMOVE_PARALYSIS:
    spell = SPELL_HOLD_PERSON;
    spell2 = SPELL_HOLD_MONSTER;
    spell3 = SPELL_SLOW;
    msg_not_affected = false;
    to_vict = "Your regain your movement functions!";
    to_room = "$n regains $s movement functions.";
    break;
  case SPELL_REMOVE_BLINDNESS:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_NEUTRALIZE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_BESTOW_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (affected_by_spell(victim, spell))
    found = TRUE;
  if (affected_by_spell(victim, spell2))
    found = TRUE;
  if (affected_by_spell(victim, spell3))
    found = TRUE;
  if (affected_by_spell(victim, spell4))
    found = TRUE;

  affect_from_char(victim, spell);

  affect_from_char(victim, spell2);

  affect_from_char(victim, spell3);

  affect_from_char(victim, spell4);

  if (found) {
    if (to_vict != NULL)
      act(to_vict, false, victim, 0, ch, TO_CHAR);
    if (to_room != NULL)
      act(to_room, true, victim, 0, ch, TO_ROOM);
  }
  else {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  }
}


void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj, int spellnum)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS_OBJECT:
      if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && !OBJ_FLAGGED(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * level)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly with a divine aura.";
      }
      break;
    case SPELL_BLESS_WEAPON:
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && !OBJ_FLAGGED(obj, ITEM_BLESS)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly with a divine aura.";
      }
      break;
    case SPELL_MENDING:
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        GET_OBJ_VAL(obj, VAL_WEAPON_HEALTH) = 100;
      else if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        GET_OBJ_VAL(obj, VAL_ARMOR_HEALTH) = 100;
      else
        GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 100;
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
      to_char = "$p is instantly mended in full.";
      to_room = "$n instantly mends $p with $s spell.";
      break;
    case SPELL_BESTOW_CURSE:
      if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE)--;
	to_char = "$p briefly glows red.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
      GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 1;
      to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_NEUTRALIZE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
        GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, true, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, true, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, true, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data *ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(z, virtual))) {
    send_to_char(ch, "I seem to have goofed.\r\n");
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  add_unique_id(tobj);
  obj_to_char(tobj, ch);
  act("$n creates $p.", false, ch, tobj, 0, TO_ROOM);
  act("You create $p.", false, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

/* affect_update_violence: called from fight.c (causes spells to wear off) */
void affect_update_violence(void)
{
  struct char_data *i;

  for (i = affectv_list; i; i = i->next_affectv) {
    if (FIGHTING(i))
      continue;
    do_affectv_tickdown(i);
  }
}

void do_affectv_tickdown(struct char_data *i)
{
  if (IS_NPC(i))
    return;

  struct affected_type *af, *next;
  int dam;
  int maxdam;

    for (af = i->affectedv; af; af = next) {
      next = af->next;
      if (af->duration >= 1) {
        af->duration--;
        switch (af->type) {
        case ART_EMPTY_BODY:
          if (GET_KI(i) >= 10) {
            GET_KI(i) -= 10;
          } else {
            af->duration = 0; /* Wear it off immediately! No more ki */
          }
        }
      } else if (af->duration == -1) {     /* No action */
        continue;
      }
      if (!af->duration) {
        if ((af->type > 0) && (af->type < SKILL_TABLE_SIZE))
          if (!af->next || (af->next->type != af->type) ||
              (af->next->duration > 0))
            if (spell_info[af->type].wear_off_msg)
              send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
          if (af->bitvector == AFF_SUMMONED) {
            stop_follower(i);
            if (!DEAD(i))
              extract_char(i);
          }
          if (af->type == ART_QUIVERING_PALM) {
            maxdam = GET_HIT(i) + 8;
            dam = GET_MAX_HIT(i) * 3 / 4;
            dam = MIN(dam, maxdam);
            dam = MAX(25, dam);
            log("Creeping death strike doing %d dam", dam);
            damage(i, i, dam, af->type, 0, -1, 0, 0, 1);
          }
        affectv_remove(i, af);
      }
    }

}

void mag_affectsv(int level, struct char_data *ch, struct char_data *victim, int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = false, accum_duration = false;
  const char *to_vict = NULL, *to_room = NULL;
  int i;


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
    af[i].level = level;
  }

  if (mag_newsaves(find_savetype(spellnum), ch, victim, spellnum, calc_spell_dc(ch, spellnum))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  while (affected_by_spell(victim, spellnum))
    affect_from_char(victim, spellnum);


  switch (spellnum) {
  case SPELL_DRAGON_KNIGHT:
  case SPELL_ANIMATE_DEAD:
  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    af[0].duration = MAX(10, level + 1) * 10;
    af[0].bitvector = AFF_SUMMONED;
    accum_duration = false;
    to_vict = "You are summoned to assist $N!";
    to_room = "$n appears, ready for action.";
    break;
  }

  /*
   * if this is a mob that has this affect set in its mob file, do not
   * perform the affect.  this prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (PRF_FLAGGED(ch, PRF_EXTEND_SPELL)) {
    for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
      if (af[i].duration > 0) {
        af[i].duration *= 15;
        af[i].duration /= 10;
      }
    }
  }

  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector)) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
      }
  /*
   * if the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    while (affected_by_spell(victim, spellnum))
      affect_from_char(victim, spellnum);
    affect_join(victim, af+i, accum_duration, false, accum_affect, false);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect_join(victim, af+i, accum_duration, false, accum_affect, false);

  if (to_vict != NULL)
    act(to_vict, false, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, true, victim, 0, ch, TO_ROOM);
}

