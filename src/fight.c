/* 
************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "quest.h"
#include "genmob.h"
#include "pets.h"
#include "player_guilds.h"
#include "dg_event.h"

SVNHEADER("$Id: fight.c 57 2009-03-24 00:15:02Z gicker $");

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

static int fightsort_compare(const void *a1, const void *b1);

/* External structures */
extern struct race_data race_list[NUM_RACES];
extern struct message_list fight_messages[MAX_MESSAGES];
extern const char *death_message;
extern struct weapon_table weapon_list[];
extern struct pet_data pet_list[NUM_PETS];

extern int race_level_adjustment[];

/* External procedures */
int has_weapon_feat(struct char_data *ch, int i, int j);
void find_next_combat_target(struct char_data *ch);
void perform_mob_combat_turn(struct char_data *ch);
void do_affect_tickdown(struct char_data *i);
int get_speed(struct char_data *ch);
void display_combat_menu(struct descriptor_data *d);
SPECIAL(harvest);
int sr_check(struct char_data *caster, struct char_data *victim);
int is_player_grouped(struct char_data *target, struct char_data *group);
void display_your_turn(struct char_data *ch);
struct char_data *find_treasure_recipient(struct char_data *killer);
char *reduct_desc(struct char_data *victim, struct damreduct_type *reduct);
void determine_treasure(struct char_data *ch, struct char_data *mob);
void determine_crafting_component_treasure(struct char_data *ch, struct char_data *mob);
char *fread_action(FILE *fl, int nr);
ACMD(do_feint);
ACMD(do_flee);
ACMD(do_get);
ACMD(do_split);
ACMD(do_sac);
ACMD(do_trip);
int backstab_dice(struct char_data *ch);
int num_attacks(struct char_data *ch, int offhand);
int num_parry_attacks(struct char_data *ch, int offhand);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int obj_savingthrow(int material, int type);
void perform_remove(struct char_data *ch, int pos);
void Crash_rentsave(struct char_data *ch, int cost);
char *which_desc(struct char_data *ch);
int base_hit(int hit_type, int chclass, int level);
int get_skill_value(struct char_data *ch, int skillnum);
int skill_roll(struct char_data *ch, int skillnum);
ACMD(do_stand);
int has_intro(struct char_data *ch, struct char_data *target);
int get_skill_value(struct char_data *ch, int skillnum);
int has_favored_enemy(struct char_data *ch, struct char_data *victim);
int mob_exp_by_level(int level);
int highest_armor_type(struct char_data *ch);
void dismount_char(struct char_data *ch);
void do_auto_combat(struct char_data *ch);

/* local functions */
EVENTFUNC(pause_combat);
void do_attack_of_opportunity(struct char_data *ch, struct char_data *victim, char *type);
int roll_initiative(struct char_data *ch);
void fight_action(struct char_data *ch);
void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type, int is_crit, int is_reduc);
void appear(struct char_data *ch);
void load_messages(void);
void free_messages(void);
void free_messages_type(struct msg_type *msg);
void check_killer(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch);
void change_alignment(struct char_data *ch, struct char_data *victim);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data *ch, struct char_data *victim);
int get_combat_pos_by_sector(struct char_data *ch);
void solo_gain(struct char_data *ch, struct char_data *victim);
char *replace_weap_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(struct char_data *ch, struct char_data *att);
void do_summon_attack(struct char_data *ch);
void do_companion_attack(struct char_data *ch);
void do_mount_attack(struct char_data *ch);
int compute_summon_armor_class(struct char_data *ch, struct char_data *att);
int compute_companion_armor_class(struct char_data *ch, struct char_data *att);
int compute_mount_armor_class(struct char_data *ch, struct char_data *att);
int compute_base_hit(struct char_data *ch, int weaponmod);
int compute_summon_base_hit(struct char_data *ch);
int compute_companion_base_hit(struct char_data *ch);
int compute_mount_base_hit(struct char_data *ch);
struct char_data *find_next_victim(struct char_data *ch);
void free_summon(struct char_data *ch);
void free_companion(struct char_data *ch);
void do_mob_special_attacks(struct char_data *ch, int type);
void free_mount(struct char_data *ch);
void find_victim(struct char_data *ch);
int get_touch_ac(struct char_data *ch);
int trip_roll(struct char_data *ch, int defending);
void assign_skin_value(struct char_data *ch, struct obj_data *corpse);
void sort_initiative(void);


#define BASEHIT_MANUAL 0
#define BASEHIT_LOW     1
#define BASEHIT_MEDIUM  2
#define BASEHIT_HIGH    3


/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"stun", "stuns"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"shoot", "shoots"},
  {"cleave", "cleaves"},
  {"claw", "claws"},
  {"freeze", "freezes"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"burn", "burns"},
  {"gore", "gores"},
  {"batter", "batters"}

};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  if (affected_by_spell(ch, SPELL_GREATER_INVISIBILITY))
    affect_from_char(ch, SPELL_GREATER_INVISIBILITY);

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  act("$n slowly fades into existence.", false, ch, 0, 0, TO_ROOM);
}


int class_armor_bonus(struct char_data *ch)
{
  int i = 0;
  int ranks = 0;
  struct obj_data *armor = GET_EQ(ch, WEAR_BODY);

  //Summons Have a Special Armor Class Calculation
  if(ch->new_summon == true)
  {
      ranks = 10;
      switch(pet_table[ch->summon_type - 1].c_type)
      {
          case COMPANION_TYPE_BEAR:
              i = 10; break;
          case COMPANION_TYPE_BADGER:
              i = 5; break;
          case COMPANION_TYPE_SNAKE:
              i = 4; break;
          case COMPANION_TYPE_LION:
              i = 6; break;
          case COMPANION_TYPE_PANTHER:
              i = 5; break;
          case COMPANION_TYPE_DOG:
              i = 4; break;
          default:
              i = 0; break;
      }
      return ((ch->level / 2) + i + ranks);
  }


  if ((ranks = (GET_CLASS_RANKS(ch, CLASS_MONK) + GET_CLASS_RANKS(ch, CLASS_SACRED_FIST))) > 0) {
    if (armor) {
      if ((GET_OBJ_TYPE(armor) != ITEM_ARMOR) || GET_OBJ_VAL(armor, 0) == 0) {
        i += ability_mod_value(GET_WIS(ch));
        i += ranks / 5;
        if (i > 0)
          return i * 10;
      }
      else return 0;
    }
    i += ability_mod_value(GET_WIS(ch));
    i += ranks / 5;
    if (i > 0)
      return i * 10;
  }

  return 0;
}

#define BASE_AC 100

int compute_armor_class(struct char_data *ch, struct char_data *att)
{
  int dexBonus = 0;
  struct obj_data *armor, *shield;
  struct affected_type *af;
  int armorclass = GET_ARMOR(ch);
  int armorExists = false;
  int dexCap = 99999;
	struct char_data *k;
	struct follow_type *f;  
  int mod = 0, i = 0;
  armor = GET_EQ(ch, WEAR_BODY);
  shield = GET_EQ(ch, WEAR_SHIELD);

  if (!ch)
    return 0;
/*
  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      armor = NULL;
      shield = NULL;
  }
*/
  if (armor && (GET_OBJ_TYPE(armor) == ITEM_ARMOR || GET_OBJ_TYPE(armor) == ITEM_ARMOR_SUIT)) {
    dexCap = GET_OBJ_VAL(armor, 2);
    armorExists = true;
  }
  if (shield && GET_OBJ_TYPE(shield) == ITEM_ARMOR) {
    dexCap = MIN(dexCap, GET_OBJ_VAL(shield, 2));
    armorExists = true;
  }

  if (armor && affected_by_spell(ch, SPELL_MAGIC_VESTMENT)) {
    for (af = ch->affected; af; af = af->next)
      if (af->type == SPELL_MAGIC_VESTMENT) {
        mod = af->modifier;
        break;
      }

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (armor->affected[i].location == APPLY_AC_ARMOR)
        break;

    if (armor->affected[i].modifier <= 200 && armor->affected[i].modifier > 0)
      mod = mod - armor->affected[i].modifier;
    

    armorclass += MAX(0, mod);    
  }

  if (armor && GET_OBJ_MATERIAL(armor) == MATERIAL_DRAGONHIDE);
    armorclass += 10;
  if (shield && GET_OBJ_MATERIAL(shield) == MATERIAL_DRAGONHIDE);
    armorclass += 10;


  if (ch->total_defense)
    armorclass += 40;


  if (FIGHTING(ch) && ch->smiting && FIGHTING(ch) == ch->smiting)
    armorclass += ability_mod_value(GET_CHA(ch)) * 10;

  if (HAS_FEAT(ch, FEAT_ARMOR_SKIN))
    armorclass += 10 * HAS_FEAT(ch, FEAT_ARMOR_SKIN);

  if (shield && affected_by_spell(ch, SPELL_MAGIC_VESTMENT)) {
    for (af = ch->affected; af; af = af->next)
      if (af->type == SPELL_MAGIC_VESTMENT) {
        mod = af->modifier;
        break;
      }

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (shield->affected[i].location == APPLY_AC_SHIELD)
        break;

    if (shield->affected[i].modifier > 0 && shield->affected[i].modifier <= 200)
      mod = mod - shield->affected[i].modifier;
    
    armorclass += MAX(0, mod);    
  }

  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  struct obj_data *offhand = GET_EQ(ch, WEAR_HOLD);

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
      (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
      has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    armorclass += 10;
  else if (offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON &&
          (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(offhand, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(offhand, VAL_WEAPON_SKILL))))
    armorclass += 10;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED)))
    armorclass += 10;


  armorclass += HAS_FEAT(ch, FEAT_NATURAL_ARMOR_INCREASE) * 10;

  armorclass += (SIZE_MEDIUM - get_size(ch)) * 10;

  armorclass += BASE_AC;
  
  if (GET_CLASS_RANKS(ch, CLASS_DWARVEN_DEFENDER))
    armorclass += (GET_CLASS_RANKS(ch, CLASS_DWARVEN_DEFENDER) + 2) / 3 * 10;

  if (highest_armor_type(ch) < ARMOR_TYPE_LIGHT)
    armorclass += class_armor_bonus(ch);

  if (affected_by_spell(ch, SPELL_HASTE))
    armorclass += 10;

  if (affected_by_spell(ch, SPELL_SLOW))
    armorclass -= 10;

  if (affected_by_spell(ch, SPELL_AFF_DEFENSIVE_STANCE))
    armorclass += 40;

  if (IS_AFFECTED(ch, AFF_STUNNED))
    armorclass -= 20;
    
  if (affected_by_spell(ch, SPELL_PRAYER))
    armorclass += 10;

  armorclass += GET_EXPERTISE_BONUS(ch) * 10;

      switch (GET_RACE(ch)) {
      case RACE_OGRE:
      case RACE_MINOTAUR:
        armorclass += 50;
        break;
      case RACE_CENTAUR:
        armorclass += 30;
        break;
      case RACE_HALF_OGRE:
        armorclass += 40;
        break;
      }

    
  if (!IS_AFFECTED(ch, AFF_STUNNED)) {
    dexBonus = ability_mod_value(GET_DEX(ch));
    if (armor) 
      if (dexBonus > dexCap) // cap the dex bonus based on armor
        dexBonus = dexCap;
    dexBonus = MIN(9, dexBonus);
    dexBonus *= 10;
    armorclass += dexBonus;
    
    if (HAS_FEAT(ch, FEAT_DODGE))
      armorclass += 10;
  
  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	armorclass = armorclass;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	armorclass += 10;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	armorclass += 10;
      	  break;
        }
      }
    }
  }  
  
  if (HAS_FEAT(ch, FEAT_CANNY_DEFENSE) && ((armor && GET_OBJ_TYPE(armor) != ITEM_ARMOR) || !armor)) {
  	armorclass += MAX(0, ability_mod_value(GET_INT(ch))) * 10;
  }

  if (HAS_FEAT(ch, FEAT_TWO_WEAPON_DEFENSE) && ((GET_EQ(ch, WEAR_HOLD) &&
  	  GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON) || (GET_EQ(ch, WEAR_WIELD) && 
          IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 0)].weaponFlags, WEAPON_FLAG_DOUBLE))))
  	  armorclass += 10;
  
    if (armor) {
      if (GET_OBJ_TYPE(armor) == ITEM_WORN || highest_armor_type(ch) <= ARMOR_TYPE_LIGHT)  // Light armor or less
        armorclass += get_skill_value(ch, SKILL_TUMBLE);
     
    }
    else
      armorclass += get_skill_value(ch, SKILL_TUMBLE);      
  }

  if (ch->mentor_level > 0) {

    armorclass -= BASE_AC;

    armorclass -= (SIZE_MEDIUM - get_size(ch)) * 10;

    if (armor && GET_OBJ_MATERIAL(armor) == MATERIAL_DRAGONHIDE);
      armorclass -= 10;
    if (shield && GET_OBJ_MATERIAL(shield) == MATERIAL_DRAGONHIDE);
      armorclass -= 10;


    if (ch->total_defense)
      armorclass -= 40;

    if (armor)
      armorclass -= GET_OBJ_VAL(armor, 0);
    if (shield)
      armorclass -= GET_OBJ_VAL(shield, 0);

    armorclass = armorclass * ch->mentor_level / GET_CLASS_LEVEL(ch);

    armorclass += (SIZE_MEDIUM - get_size(ch)) * 10;

    armorclass += BASE_AC;

    if (armor)
      armorclass += GET_OBJ_VAL(armor, 0);
    if (shield)
      armorclass += GET_OBJ_VAL(shield, 0);


    if (armor && GET_OBJ_MATERIAL(armor) == MATERIAL_DRAGONHIDE);
      armorclass += 10;
    if (shield && GET_OBJ_MATERIAL(shield) == MATERIAL_DRAGONHIDE);
      armorclass += 10;


    if (ch->total_defense)
      armorclass += 40;

  }
  
  if (IS_NPC(ch) && GET_HITDICE(ch) <= 3) {
    switch (GET_HITDICE(ch)) {
      case 1:
        armorclass = MIN(120, armorclass);
        break;
      case 2:
        armorclass = MIN(140, armorclass);
        break;
      case 3:
        armorclass = MIN(160, armorclass);
        break;
    }
  } else if (IS_NPC(ch) && GET_HITDICE(ch) <= 10) {
    armorclass = armorclass * 4 / 5;
  }

  if (GET_POS(ch) < POS_SLEEPING)
    return 0;

  if (!att)
    return armorclass;

  if (IS_EVIL(att) && IS_SET_AR(AFF_FLAGS(ch), AFF_PROTECT_EVIL))
    armorclass += 20;

  return armorclass;
}

int compute_summon_armor_class(struct char_data *ch, struct char_data *att)
{
  if (IS_NPC(ch) || ch->player_specials->summon_num == 0)
    return 0;

  int armorclass = ch->player_specials->summon_ac;
  struct char_data *k;
  struct follow_type *f;  
  int i = 0;

  struct obj_data *barding = GET_EQ(ch, WEAR_BARDING);

  if (barding) {
    armorclass += GET_OBJ_VAL(barding, 0);
    for (i = 0; i < 6; i ++) {
      if (barding->affected[i].location == APPLY_AC_ARMOR) {
        armorclass += barding->affected[i].modifier;
        break;
      }
    }
  }


  if (affected_by_spell(ch, SPELL_HASTE))
    armorclass += 10;

  if (affected_by_spell(ch, SPELL_SLOW))
    armorclass -= 10;

  if (affected_by_spell(ch, SPELL_PRAYER))
    armorclass += 10;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	armorclass = armorclass;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	armorclass += 10;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	armorclass += 10;
      	  break;
        }
      }
    }
  }  
  
  if (att && IS_EVIL(att) && IS_SET_AR(AFF_FLAGS(ch), AFF_PROTECT_EVIL))
    armorclass += 20;

  return armorclass;
}

int compute_companion_armor_class(struct char_data *ch, struct char_data *att)
{
  if (IS_NPC(ch) || ch->player_specials->companion_num == 0)
    return 0;

  int armorclass = ch->player_specials->companion_ac;
  struct char_data *k;
  struct follow_type *f;  
  int i = 0;

  struct obj_data *barding = GET_EQ(ch, WEAR_BARDING);

  if (barding) {
    armorclass += GET_OBJ_VAL(barding, 0);
    for (i = 0; i < 6; i ++) {
      if (barding->affected[i].location == APPLY_AC_ARMOR) {
        armorclass += barding->affected[i].modifier;
        break;
      }
    }
  }


  if (affected_by_spell(ch, SPELL_HASTE))
    armorclass += 10;

  if (affected_by_spell(ch, SPELL_SLOW))
    armorclass -= 10;

  if (affected_by_spell(ch, SPELL_PRAYER))
    armorclass += 10;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	armorclass = armorclass;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	armorclass += 10;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	armorclass += 10;
      	  break;
        }
      }
    }
  }  
  
  if (att && IS_EVIL(att) && IS_SET_AR(AFF_FLAGS(ch), AFF_PROTECT_EVIL))
    armorclass += 20;

  return armorclass;
}

int compute_mount_armor_class(struct char_data *ch, struct char_data *att)
{
  if (IS_NPC(ch) || ch->player_specials->mount_num == 0)
    return 0;

  int armorclass = ch->player_specials->mount_ac;
  struct char_data *k;
  struct follow_type *f;  
  int i = 0;

  struct obj_data *barding = GET_EQ(ch, WEAR_BARDING);

  if (barding) {
    armorclass += GET_OBJ_VAL(barding, 0);
    for (i = 0; i < 6; i ++) {
      if (barding->affected[i].location == APPLY_AC_ARMOR) {
        armorclass += barding->affected[i].modifier;
        break;
      }
    }
  }

  if (affected_by_spell(ch, SPELL_HASTE))
    armorclass += 10;

  if (affected_by_spell(ch, SPELL_SLOW))
    armorclass -= 10;

  if (affected_by_spell(ch, SPELL_PRAYER))
    armorclass += 10;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	armorclass = armorclass;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	armorclass += 10;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	armorclass += 10;
      	  break;
        }
      }
    }
  }  
  
  if (att && IS_EVIL(att) && IS_SET_AR(AFF_FLAGS(ch), AFF_PROTECT_EVIL))
    armorclass += 20;

  return armorclass;
}


void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];
  char * trash = NULL;

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  trash = fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
    trash = fgets(chk, 128, fl);
  }


  while (*chk == 'M') {
    trash = fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    trash = fgets(chk, 128, fl);

    while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
      trash = fgets(chk, 128, fl);
    }
  }

  fclose(fl);
}


void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;

  if (!IS_NPC(victim) && (rand_number(1, 20) > 10 + (ush_int) GET_HIT(victim)) && GET_HIT(victim) <= 0)
    GET_HIT(victim) = 1;
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, ADMLVL_IMMORT, true, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    return;
  }

  struct char_data *tank;

  for (tank = world[IN_ROOM(vict)].people; tank; tank = tank->next_in_room) {
    if (is_player_grouped(vict, tank) && AFF_FLAGGED(tank, AFF_TANKING)) {
      vict = tank;
      break;
    }
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  if (!ch)
    return;
  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;
  struct char_data *temp = NULL;
  REMOVE_FROM_LIST(ch, combat_list, next_fighting);

  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  if (AFF_FLAGGED(ch, AFF_NEXTNOACTION))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
  update_pos(ch);
  ch->smiting = NULL;
  ch->initiative = 0;
  ch->in_combat = 0;
  ch->active_turn = 0;
  ch->standard_action_spent = 0;
  ch->move_action_spent = 0;
  ch->minor_action_spent = 0;
  ch->full_round_action = 0;
  ch->round_num = 0;
}

void assign_skin_value(struct char_data *ch, struct obj_data *corpse)
{ 
//int i;
/* DON'T pass  PC to this routine! */
/*
for(i = 0; i < 4; i++)
 corpse->obj_flags->skin_data[i] = 
 ch->mob_specials.skin_data[i];
*/
return;
}

void make_corpse(struct char_data *ch)
{
  if (!ch) 
    return;

  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse = NULL, *o;
  struct obj_data *money;
  int i, x, y;
  int race = 0;

  //corpse = create_obj();
  if (CONFIG_DFLT_PORT == 4000 || CONFIG_DFLT_PORT == 4010)
    corpse = read_object(199, VIRTUAL);
  else if (CONFIG_DFLT_PORT == 6070)
    corpse = read_object(199, VIRTUAL);

  //corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
	char *tmpdesc = NULL, *tmpstr = IS_NPC(ch) ? ch->short_descr :      which_desc(ch);
  snprintf(buf2, sizeof(buf2), "corpse %s", tmpstr);
  corpse->name = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", tmpstr);
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", tmpstr);
  corpse->short_description = strdup(buf2);
	free(tmpdesc);

  if (IS_NPC(ch))
    assign_skin_value(ch, corpse);
	
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for(x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CAPACITY) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = 1;	/* corpse identifier */
  GET_OBJ_VAL(corpse, VAL_CONTAINER_OWNER) = GET_PFILEPOS(ch);	/* corpse identifier */
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE_RACE) = race = GET_RACE(ch);
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = (ush_int)100000;
	GET_OBJ_LEVEL(corpse) = GET_LEVEL(ch);
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_UNIQUE_SAVE);

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer gold */
  if ((GET_GOLD(ch) > 0) && !IS_NPC(ch)) {
    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (IS_NPC(ch) || ch->desc) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, IN_ROOM(ch));

  if (!IS_NPC(ch))
    Crash_rentsave(ch, 0);
}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
  return;
}



void death_cry(struct char_data *ch)
{
  if (!ch)
    return;

  act("Your blood freezes as you hear $n's death cry.", false, ch, 0, 0, TO_ROOM);
/*
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
*/
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{
  struct char_data *k, *temp;
  struct follow_type *f;
  long local_gold = 0;
  char local_buf[256];
  struct char_data *tmp_char;
  struct obj_data *corpse_obj;

  if (FIGHTING(ch))
    stop_fighting(ch);

  if (affected_by_spell(killer, SPELL_TRIP))
    affect_from_char(killer, SPELL_TRIP);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  while (ch->affectedv)
    affectv_remove(ch, ch->affectedv);

  /* To make ordinary commands work in scripts.  welcor*/  
  GET_POS(ch) = POS_STANDING; 
  
  if (killer) {
    struct char_data *tch;
    for (tch = world[IN_ROOM(killer)].people; tch; tch = tch->next_in_room) {
      if (IS_NPC(tch) || !is_player_grouped(tch, killer))
        continue;
      if (IS_NPC(ch) && GET_AUTOQUEST_VNUM(tch) == GET_MOB_VNUM(ch) && GET_AUTOQUEST_KILLNUM(tch) > 0) {
        GET_AUTOQUEST_KILLNUM(tch)--;
        if (GET_AUTOQUEST_KILLNUM(tch) == 0)
          send_to_char(tch, "@MYou have completed your bounty contract.  Return to a bounty contractor to get your reward.@n\r\n");
        else
          send_to_char(tch, "@MYou have completed a portion of your bounty contract.  You now have %d left to kill.@n\r\n", 
                   GET_AUTOQUEST_KILLNUM(tch));
      }
    }
    if (death_mtrigger(ch, killer))
      if (IS_NPC(ch) && GET_ID(ch) != killer->last_kill_rnum)
        death_cry(ch);
  } else if ((IS_NPC(ch) && GET_ID(ch) != killer->last_kill_rnum))
    death_cry(ch);

  if (killer) {
    if ((IS_NPC(ch) && GET_ID(ch) != killer->last_kill_rnum)) {
      autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
    }
  }

  update_pos(ch);

  if (IS_NPC(ch) && killer) {
    if ((IS_NPC(ch) && GET_ID(ch) != killer->last_kill_rnum)) {
    if (!AFF_FLAGGED(ch, AFF_CHARM)) {
      determine_treasure(find_treasure_recipient(killer), ch);
      determine_crafting_component_treasure(find_treasure_recipient(killer), ch);
      if (!killer->master && HAS_FEAT(killer, FEAT_SCAVENGE)) {
        determine_treasure(killer, ch);
        determine_crafting_component_treasure(killer, ch);
      }
      else if (killer->master && (HAS_FEAT(killer->master, FEAT_SCAVENGE))) {
        determine_treasure(killer->master, ch);
        determine_crafting_component_treasure(killer->master, ch);
      }
      if (killer->master)
        GET_STAT_MOB_KILLS(killer->master)++;
      else
        GET_STAT_MOB_KILLS(killer)++;

      for (f = killer->master ? killer->master->followers : killer->followers; f; f = f->next) {
        GET_STAT_MOB_KILLS(f->follower)++;
        if (HAS_FEAT(f->follower, FEAT_SCAVENGE)) {
          determine_treasure(f->follower, ch);
          determine_crafting_component_treasure(f->follower, ch);
        }
      }
    }
    }
    make_corpse(ch);
    extract_char(ch);

    if (FIGHTING(ch))
      stop_fighting(ch);

    for (k = world[IN_ROOM(ch)].people; k; k = temp) {
      temp = k->next_in_room;
      if (FIGHTING(k) == ch) {
        stop_fighting(k);
        if (!IS_NPC(k))
          GET_DAMAGE_TAKEN(k) = 0;
      }
    }

    /* Cant determine GET_GOLD on corpse, so do now and store */
  if (IS_NPC(ch)) {
    local_gold = GET_GOLD(ch);
    sprintf(local_buf,"%ld", (long)local_gold);
  }

  if (killer) {
    if (IS_AFFECTED(killer, AFF_GROUP) && (local_gold > 0) &&
        PRF_FLAGGED(killer, PRF_AUTOSPLIT) ) {
      generic_find("corpse", FIND_OBJ_ROOM, killer, &tmp_char, &corpse_obj);
      if (corpse_obj) {
        do_get(killer, "all.coin corpse", 0, 0);
        do_split(killer, local_buf,0,0);
      }
      /* need to remove the gold from the corpse */
    } else if (!IS_NPC(killer) && (killer != ch) && PRF_FLAGGED(killer, PRF_AUTOGOLD)) {
      do_get(killer, "all.coin corpse", 0, 0);
    }
    if (!IS_NPC(killer) && (ch != killer) && PRF_FLAGGED(killer, PRF_AUTOLOOT)) {
      do_get(killer, "all corpse", 0, 0);
    }
    if (IS_NPC(ch) && !IS_NPC(killer) && PRF_FLAGGED(killer, PRF_AUTOSAC)) { 
       do_sac(killer,"corpse",0,0); 
    } 
  }

  } else if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    /* Something killed your spirit. Doh! */
    extract_char(ch);
  } else {
    if (FIGHTING(ch))
      stop_fighting(ch);

    for (k = world[IN_ROOM(ch)].people; k; k = temp) {
      temp = k->next_in_room;
      if (FIGHTING(k) == ch) {
        stop_fighting(k);
        if (!IS_NPC(k))
          GET_DAMAGE_TAKEN(k) = 0;
      }
    }
    /* we can't forget the hunters either... */
    for (temp = character_list; temp; temp = temp->next)
      if (HUNTING(temp) == ch)
        HUNTING(temp) = NULL;

    /* Already set in hit() and damage() functions */
    SET_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);
    GET_HIT(ch) = 1;
    update_pos(ch);
    save_char(ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }

  if (ch && killer) {
    char_from_room(ch);
    char_to_room(ch, real_room(0));
  }
  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }
  if (ch && killer) {
    char_from_room(ch);
    char_to_room(ch, IN_ROOM(killer));
  }

  if (killer)
    killer->last_kill_rnum = GET_ID(ch);
  
}



void die(struct char_data * ch, struct char_data * killer)
{

  if (ch && ch->dead)
    return;

  ch->dead = TRUE;

  if (killer) {
    if (ch == killer->smiting)
      killer->smiting = NULL;
    GET_FIGHT_BLEEDING(killer) = 0;
    GET_FIGHT_BLEEDING_DAMAGE(killer) = 0;
    GET_FIGHT_PRECISE_ATTACK(killer) = 0;
    GET_FIGHT_SNEAK_ATTACK(killer) = 0;
    GET_FIGHT_SPRING_ATTACK(killer) = 0;
    GET_FIGHT_CRITICAL_HIT(killer) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(killer) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(killer) = 0;
    GET_FIGHT_DEATH_ATTACK(killer) = 0;
    GET_FIGHT_UNDEATH_TOUCH(killer) = 0;
    GET_FIGHT_DAMAGE_DONE(killer) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(killer) = 0;
    GET_FIGHT_NUMBER_OF_HITS(killer) = 0;
    GET_FIGHT_MESSAGE_PRINTED(killer) = FALSE;
  }

  struct char_data *tch;
 
  if (!(ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_PVP))) {
    if (GET_LEVEL(ch) > 5)
      gain_exp(ch, -(mob_exp_by_level(GET_LEVEL(ch)) * 25));
  }
  else
    ch->pvp_death = 1;
  if (!IS_NPC(ch)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
  }
  if ((IS_NPC(ch) && GET_ID(ch) != killer->last_kill_rnum))
    send_to_char(ch, "@w%s@n", death_message);
  raw_kill(ch, killer);

  if (killer && IN_ROOM(killer) != NOWHERE) {
    for (tch = world[IN_ROOM(killer)].people; tch; tch = tch->next_in_room)
      if (is_player_grouped(killer, tch) && !IS_NPC(tch))
        tch->exp_chain++;
  }
}



void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_PVP))
    return;

  int share = base;
  int mult = 100;
  int pc_grouped = FALSE;
  struct follow_type *f;
  struct char_data *k;
  int char_lev = 0;
  int accexp = 0;

  if (victim && IS_NPC(victim) && GET_ID(victim) == ch->last_kill_rnum)
    return;

  if (victim && victim->dead)
    return;

  k = ch->master ? ch->master : ch;

  char_lev = GET_LEVEL(k);
  if (k->mentor_level > 0)
    char_lev = k->mentor_level;

  int highest_level = char_lev;

  for (f = k->followers; f; f = f->next)
    if (!IS_NPC(f->follower)) {
      pc_grouped = TRUE;
      char_lev = GET_LEVEL(f->follower);
      if (f->follower->mentor_level > 0)
        char_lev = f->follower->mentor_level;
      if (char_lev > highest_level)
        highest_level = char_lev;
      break;
    }      

  share = MIN(CONFIG_MAX_EXP_GAIN, share);

  share *= 2;

  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_FINAL_BOSS)) {
    mult += 2500;
    if (ch->mentor_level > 0) {
      accexp = MAX(1, (GET_CLASS_LEVEL(ch) - ch->mentor_level) / 2);
      accexp *= 3;
    }
  }
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_BOSS)) {
    mult += 1000;
    if (ch->mentor_level > 0) {
      accexp = MAX(1, (GET_CLASS_LEVEL(ch) - ch->mentor_level) / 2);
      accexp *= 5;
      accexp /= 2;
    }
  }
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_CAPTAIN)) {
    mult += 500;
    if (ch->mentor_level > 0) {
      accexp = MAX(1, (GET_CLASS_LEVEL(ch) - ch->mentor_level) / 2);
      accexp *= 2;
    }
  }
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_LIEUTENANT)) {
    mult += 250;
    if (ch->mentor_level > 0) {
      accexp = MAX(1, (GET_CLASS_LEVEL(ch) - ch->mentor_level) / 2);
      accexp *= 3;
      accexp /= 2;
    }
  } else {
      accexp = MAX(1, (GET_CLASS_LEVEL(ch) - ch->mentor_level) / 2);
  }

  if (IS_UNDEAD(victim)) {
    mult *= 125;
    mult /= 100;
  }

  share *= mult;
  share /= 100;


  if (pc_grouped && share > mob_exp_by_level(GET_LEVEL(ch)) && GET_LEVEL(victim) > GET_LEVEL(ch) &&
      (GET_LEVEL(ch) + 5) < highest_level) {
    share = mob_exp_by_level(GET_LEVEL(ch));
    send_to_char(ch, "@RYour exp has been capped to prevent power levelling.@n\r\n");
  }

  if (ch->mentor_level > 0 && FALSE) {
    int percent = 10000 * share / level_exp(ch->mentor_level, GET_REAL_RACE(ch));
    share = level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)) * percent / 10000;
    share = MAX(share, mob_exp_by_level(GET_LEVEL(victim) + 5));
  }

  if (ch->mentor_level > 0 && ch->desc && ch->desc->account) {
    accexp = MAX(1, accexp);
    send_to_char(ch, "You gain %d account experience for mentoring.\r\n", accexp);
    ch->desc->account->experience += MAX(1, accexp);
  }

  gain_exp(ch, share);
}

void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_levels=0, tot_members = 0, base=0, tot_gain=0, tot_accounts = 0;
  struct char_data *k;
  struct follow_type *f;
  struct follow_type *g;
  int officer = false;
  int officerLevel = 0;
  int maxLevel = 0;
  int amount = 0;
  int mult = 100;

  if (!(k = ch->master))
    k = ch;
    
  if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch))) {
    tot_levels = GET_LEVEL(k);
    tot_members = 1;
  } else {
    tot_levels = 0;
    tot_members = 1;
  }

  if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
    officer = true;
    officerLevel = GET_LEVEL(ch);
  }

  maxLevel = GET_LEVEL(k) + race_list[GET_RACE(k)].level_adjustment;

  if (affected_by_spell(k, SPELL_TRIP))
    affect_from_char(k, SPELL_TRIP);

  tot_accounts = tot_members;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
      if (GET_LEVEL(f->follower) > maxLevel) 
        maxLevel = GET_LEVEL(f->follower) + race_list[GET_RACE(f->follower)].level_adjustment; {
      }
      if (affected_by_spell(f->follower, SPELL_TRIP))
        affect_from_char(f->follower, SPELL_TRIP);
      tot_levels += GET_LEVEL(f->follower);
      tot_members++;
      for (g = k->followers; g; g = g->next) {
        if (g->follower->desc && g->follower->desc->account &&
            f->follower->desc && f->follower->desc->account &&
            k->desc && k->desc->account &&
            (g->follower == f->follower || !strcmp(g->follower->desc->account->name, f->follower->desc->account->name) ||
             !strcmp(g->follower->desc->account->name, k->desc->account->name)))
          continue;
        if (g->follower->desc == NULL || f->follower->desc == NULL || k->desc == NULL)
          continue;
        if (!strcmp(GET_HOST(f->follower), GET_HOST(g->follower)))
          continue;
        tot_accounts++;
      }
    }

    
    if (!AFF_FLAGGED(ch, AFF_GROUP) || tot_members == 1) {
      solo_gain(ch, victim);
      return;
    }

  tot_gain = (mob_exp_by_level(GET_LEVEL(victim)));

  mult += (officer * 10);

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = 0;

  tot_gain *= mult;
  tot_gain /= 100;

  tot_gain *= 120;
  tot_gain /= 100;

  base = MAX(1, tot_gain);

  if (AFF_FLAGGED(victim, AFF_CHARM))
    base = 0;  

  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch)) {
//    amount = MIN(base, ((tot_levels / tot_members < 5) ? mob_exp_by_level(GET_LEVEL(k) + 2) * mult / 100 : 
//             mob_exp_by_level(GET_LEVEL(k) + 2) * mult * GET_LEVEL(k) / 100 / tot_levels));
    amount = base;
    perform_group_gain(k, amount, victim);
  }

  for (f = k->followers; f; f = f->next) {
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
//      amount = MIN(base, ((tot_levels / tot_members < 5) ? mob_exp_by_level(GET_LEVEL(f->follower) + 2) * mult / 100 : 
//               mob_exp_by_level(GET_LEVEL(f->follower) + 2) * mult * GET_LEVEL(f->follower) / 100 / tot_levels));
      amount = base;
      perform_group_gain(f->follower, amount, victim);
    }
  }
}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
  if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_PVP))
    return;

  if (victim && IS_NPC(victim) && GET_ID(victim) == ch->last_kill_rnum)
    return;

  if (victim && victim->dead)
    return;

  int tot_gain;
  int exp;
  int levelVict = GET_LEVEL(victim);
  int levelCh = GET_LEVEL(ch) + race_list[GET_RACE(victim)].level_adjustment;
  int maxLevel;
  int mult = 100;

  maxLevel = MAX(levelCh, GET_FIGHTING_MAX_LVL(victim));

  tot_gain = mob_exp_by_level(levelVict) * 120 / 100;

  tot_gain *= 2;


  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_FINAL_BOSS))
    mult += 2500;
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_BOSS))
    mult += 1000;
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_CAPTAIN))
    mult += 500;
  else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_LIEUTENANT))
    mult += 250;

  if (IS_UNDEAD(victim)) {
    mult *= 125;
    mult /= 100;
  }

  tot_gain *= mult;

  tot_gain /= 100;

  exp = tot_gain;

  exp = MAX(exp, 1);

  if (!IS_NPC(victim))
    exp = 0;

  if (AFF_FLAGGED(victim, AFF_CHARM))
    exp = 0;


  if (affected_by_spell(ch, SPELL_TRIP))
    affect_from_char(ch, SPELL_TRIP);

  gain_exp(ch, exp);
}


char *replace_weap_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[MAX_STRING_LENGTH];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}



/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type, int is_crit, int is_reduc)
{
  char buf[MAX_STRING_LENGTH];
  int msgnum;
  int is_sneak = FALSE, is_precise = FALSE;
  
  if (is_crit == 999 || is_crit == 777 || is_crit == 444 || is_crit == 111) {
  	is_sneak = TRUE;
  }
  if (is_crit == 888 || is_crit == 777 || is_crit == 555 || is_crit == 444) {
  	is_precise = TRUE;
  }  
  if (is_crit == 111 || is_crit == 555 || is_crit == 444) {
  	is_crit = FALSE;
  }
  	


  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "@W$n #W $N but misses with ",	/* 0: 0     */
      "@WYou #w $N but miss with ",
      "@W$n #W you but misses with "
    },

    {
      "@y$n #W $N with ",	/* 1: 1..2  */
      "@yYou #w $N with ",
      "@R$n #W you with "
    },

    {
      "@[6]$n barely #W $N@[6].@n",		/* 2: 3..4  */
      "@[5]You barely #w $N@[5].@n",
      "@[4]$n@[4] barely #W you.@n"
    },

    {
      "@[6]$n #W $N@[6].@n",			/* 3: 5..6  */
      "@[5]You #w $N@[5].@n",
      "@[4]$n@[4] #W you.@n"
    },

    {
      "@[6]$n #W $N@[6] hard.@n",			/* 4: 7..10  */
      "@[5]You #w $N@[5] hard.@n",
      "@[4]$n@[4] #W you hard.@n"
    },

    {
      "@[6]$n #W $N@[6] very hard.@n",		/* 5: 11..14  */
      "@[5]You #w $N@[5] very hard.@n",
      "@[4]$n@[4] #W you very hard.@n"
    },

    {
      "@[6]$n #W $N@[6] extremely hard.@n",	/* 6: 15..19  */
      "@[5]You #w $N@[5] extremely hard.@n",
      "@[4]$n@[4] #W you extremely hard.@n"
    },

    {
      "@[6]$n massacres $N@[6] to small fragments with $s #w.@n",	/* 7: 19..23 */
      "@[5]You massacre $N@[5] to small fragments with your #w.@n",
      "@[4]$n@[4] massacres you to small fragments with $s #w.@n"
    },

    {
      "@[6]$n OBLITERATES $N@[6] with $s deadly #w!!@n",	/* 8: > 23   */
      "@[5]You OBLITERATE $N@[5] with your deadly #w!!@n",
      "@[4]$n@[4] OBLITERATES you with $s deadly #w!!@n"
    }
  };


  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 99999999) msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  sprintf(buf, "%s", replace_weap_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural));
  sprintf(buf, "%s (rolled @Y%d@n vs defense @Y%d@n", buf, ch->att_roll, ch->opp_def);
  if (dam)
    sprintf(buf, "%s for @R%d@n damage)@n", buf, dam);
  else
    sprintf(buf, "%s)", buf);
  if (ch->riposte == TRUE)
    sprintf(buf, "%s @M*riposte*@n", buf);
  if (ch->new_parry == TRUE)
    sprintf(buf, "%s @M*deflected*@n", buf);
  if (GET_POS(victim) < POS_RESTING)
    sprintf(buf, "%s @M*unconscious*@n", buf);
  if (AFF_FLAGGED(ch, AFF_SMITING) && IS_GOOD(ch)) {
    sprintf(buf, "%s @M*smite evil*@n", buf);
  }
  if (dam > 0 && affected_by_spell(ch, SPELL_FLAME_WEAPON)) {
    sprintf(buf, "%s @M*fire damage*@n", buf);
  }
  if (is_precise)
    sprintf(buf, "%s @M*precise strike*@n", buf);    	
  if (is_sneak)
    sprintf(buf, "%s @M*sneak attack*@n", buf);  
  if (is_crit)
    sprintf(buf, "%s @M*critical hit*@n", buf);
  if (is_reduc)
    sprintf(buf, "%s @M*reduced*@n", buf);
  if (GET_DEATH_ATTACK(ch))
    sprintf(buf, "%s @M*death attack*@n", buf);


  /* damage message to damager */
  sprintf(buf, "%s", replace_weap_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural));
  if (ch->wield_type == WIELD_TYPE_MAIN) {
    if (GET_EQ(ch, WEAR_WIELD1))
      sprintf(buf, "%s%s.@n", buf, GET_EQ(ch, WEAR_WIELD1)->short_description);
    else if (IS_NPC(ch))
      sprintf(buf, "%s$s weapon.@n ", buf);
    else 
      sprintf(buf, "%s$s fists.@n ", buf);
      
  } else {
    if (GET_EQ(ch, WEAR_WIELD2))
      sprintf(buf, "%s%s.@n ", buf, GET_EQ(ch, WEAR_WIELD2)->short_description);
  }
  sprintf(buf, "%s (rolled @Y%d@n vs defense @Y%d@n", buf, ch->att_roll, ch->opp_def);
  if (dam)
    sprintf(buf, "%s for @R%d@n damage)@n", buf, dam);
  else
    sprintf(buf, "%s)", buf);
  if (ch->riposte == TRUE)
    sprintf(buf, "%s @M*riposte*@n", buf);
  if (ch->new_parry == TRUE)
    sprintf(buf, "%s @M*deflected*@n", buf);
  if (GET_POS(victim) < POS_RESTING)
    sprintf(buf, "%s @M*unconscious*@n", buf);
  if (AFF_FLAGGED(ch, AFF_SMITING) && IS_GOOD(ch)) {
    sprintf(buf, "%s @M*smite evil*@n", buf);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SMITING);
  }
  if (dam > 0 && affected_by_spell(ch, SPELL_FLAME_WEAPON)) {
    sprintf(buf, "%s @M*fire damage*@n", buf);
  }
  if (is_precise)
    sprintf(buf, "%s @M*precise strike*@n", buf);    	  
  if (is_sneak)
    sprintf(buf, "%s @M*sneak attack*@n", buf);    
  if (is_crit)
    sprintf(buf, "%s @M*critical hit*@n", buf);
  if (is_reduc)
    sprintf(buf, "%s @M*reduced*@n", buf);
  if (GET_DEATH_ATTACK(ch))
    sprintf(buf, "%s @M*death attack*@n", buf);
  if (ch->combat_output == OUTPUT_FULL)
    act(buf, false, ch, NULL, victim, TO_CHAR | TO_SLEEP);


  /* damage message to damagee */
  sprintf(buf, "%s", replace_weap_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural));
  if (ch->wield_type == WIELD_TYPE_MAIN) {
    if (GET_EQ(ch, WEAR_WIELD1))
      sprintf(buf, "%s%s.@n ", buf, GET_EQ(ch, WEAR_WIELD1)->short_description);
    else if (IS_NPC(ch))
      sprintf(buf, "%s$s weapon.@n ", buf);
    else
      sprintf(buf, "%s$s fists.@n ", buf);
      
  } else {
    if (GET_EQ(ch, WEAR_WIELD2))
      sprintf(buf, "%s%s.@n ", buf, GET_EQ(ch, WEAR_WIELD2)->short_description);
  }
  sprintf(buf, "%s (rolled @Y%d@n vs defense @Y%d@n", buf, ch->att_roll, ch->opp_def);
  if (dam)
    sprintf(buf, "%s for @R%d@n damage)@n", buf, dam);
  else
    sprintf(buf, "%s)", buf);
  if (ch->riposte == TRUE)
    sprintf(buf, "%s @M*riposte*@n", buf);
  if (ch->new_parry == TRUE)
    sprintf(buf, "%s @M*deflected*@n", buf);
  if (GET_POS(victim) < POS_RESTING)
    sprintf(buf, "%s @M*unconscious*@n", buf);
  if (AFF_FLAGGED(ch, AFF_SMITING) && IS_GOOD(ch)) {
    sprintf(buf, "%s @M*smite evil*@n", buf);
  }
  if (dam > 0 && affected_by_spell(ch, SPELL_FLAME_WEAPON)) {
    sprintf(buf, "%s @M*fire damage*@n", buf);
  }
  if (is_precise)
    sprintf(buf, "%s @M*precise strike*@n", buf);    
  if (is_sneak)
    sprintf(buf, "%s @M*sneak attack*@n", buf);    
  if (is_crit)
    sprintf(buf, "%s @M*critical hit*@n", buf);
  if (is_reduc)
    sprintf(buf, "%s @M*reduced*@n", buf);
  if (GET_DEATH_ATTACK(ch))
    sprintf(buf, "%s @M*death attack*@n", buf);
  if (victim->combat_output == OUTPUT_FULL)
    act(buf, false, ch, NULL, victim, TO_VICT | TO_SLEEP);


  if (is_precise)
    GET_FIGHT_PRECISE_ATTACK(ch)++;
  if (is_sneak) {
    if (PRF_FLAGGED(ch, PRF_BLEEDING_ATTACK)) {
      GET_FIGHT_BLEEDING(ch)++;
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_BLEEDING_ATTACK);
    }
    else
      GET_FIGHT_SNEAK_ATTACK(ch)++;
  }
  if (is_crit)
    GET_FIGHT_CRITICAL_HIT(ch)++;
  if (GET_DEATH_ATTACK(ch))
    GET_FIGHT_DEATH_ATTACK(ch)++;
  if (GET_UNDEATH_TOUCH(ch))
    GET_FIGHT_UNDEATH_TOUCH(ch)++;

  GET_DEATH_ATTACK(ch) = 0;
  GET_UNDEATH_TOUCH(ch) = 0;
  ch->new_parry = FALSE;
}




/*
 *  message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype, int is_reduc)
{
  int i, j, nr;
  struct message_type *msg;
  char buf[MAX_STRING_LENGTH];

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD1);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && ADM_FLAGGED(vict, ADM_NODAMAGE)) {
	act(msg->god_msg.attacker_msg, false, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, false, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, false, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->die_msg.attacker_msg,
                     is_reduc ? " @[7]*reduced*" : "");
            act(buf, false, ch, weap, vict, TO_CHAR);
          }

          snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->die_msg.victim_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, false, ch, weap, vict, TO_VICT | TO_SLEEP);

          snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->die_msg.room_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, false, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
            snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->hit_msg.attacker_msg,
                     is_reduc ? " @[7]*reduced*" : "");
	    act(buf, false, ch, weap, vict, TO_CHAR);
          }

          snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->hit_msg.victim_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, false, ch, weap, vict, TO_VICT | TO_SLEEP);

          snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->hit_msg.room_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, false, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
          snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->miss_msg.attacker_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, false, ch, weap, vict, TO_CHAR);
        }

        snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->miss_msg.victim_msg,
                 is_reduc ? " @[7]*reduced*" : "");
	act(buf, false, ch, weap, vict, TO_VICT | TO_SLEEP);

        snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->miss_msg.room_msg,
                 is_reduc ? " @[7]*reduced*" : "");
	act(buf, false, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

void damage_object(struct char_data *ch, struct char_data *victim)
{
  /* function needs to do two things, attacker's weapon could take damage
     and the attackee could take damage to their armor. */

  struct obj_data *object = NULL;

  int dnum, rnum, snum, wnum;

  object = GET_EQ(ch, WEAR_WIELD1);

  snum = 90;

  rnum = rand_number(1, 101);

  if (object && GET_OBJ_TYPE(object) == ITEM_WEAPON) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) &&
          !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
	dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) = GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(ch, "@CYour %s has broken beyond use!@n\r\n", object->short_description);
          perform_remove(ch, WEAR_WIELD1);
        }
      }
    }
  }

  object = GET_EQ(ch, WEAR_WIELD2);

  snum = 90;

  rnum = rand_number(1, 101);

  if (object && GET_OBJ_TYPE(object) == ITEM_WEAPON) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) &&
          !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
	dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) = GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(ch, "@CYour %s has broken beyond use!@n\r\n", object->short_description);
          perform_remove(ch, WEAR_WIELD1);
        }
      }
    }
  }

  snum = GET_DEX(victim);

  object = GET_EQ(victim, WEAR_BODY);

  rnum = rand_number(1, 20);

  if (rand_number(1,100) < 10) {
    if (object && dice(1, 3) != 3) {
      if (rnum > snum  || TRUE) {
        if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
            !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
   	  dnum = dice(1, 3);
          GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) = GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) - dnum;
          if (GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) < 0) {
            TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
            send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
            perform_remove(victim, WEAR_BODY);
          }
        }
      }
    }
  }

  rnum = dice(1, 101);
  snum = 90;

  wnum = rand_number(0, NUM_WEARS-1);
  object = GET_EQ(victim, wnum);
  if (object) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
        !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
        dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_ALL_HEALTH) = GET_OBJ_VAL(object, VAL_ALL_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_ALL_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
          perform_remove(victim, wnum);
        }
      }
    }
  }


  object = GET_EQ(victim, WEAR_SHIELD);

  rnum = rand_number(1, 20);

  if (rand_number(1,100) < 10) {
    if (object) {
      if (rnum > snum || TRUE) {
        if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
            !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
   	  dnum = dice(1, 3);
          GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) = GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) - dnum;
          if (GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) < 0) {
            TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
            send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
            perform_remove(victim, WEAR_SHIELD);
          }
        }
      }
    }
  }

  return;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int is_crit, int material, int bonus, int spell, int magic)
{
  int reduction = 0, rtest, passed;
  struct damreduct_type *reduct, *temp;
  struct obj_data *armor = GET_EQ(victim, WEAR_BODY);
  struct obj_data *shield = GET_EQ(victim, WEAR_SHIELD);

  if (ch->mentor_level > 0) {
    dam = MAX(dam ? 5 : 0, dam * ch->mentor_level / GET_CLASS_LEVEL(ch));
  }

  /* peaceful rooms */
  if (GET_MOB_VNUM(ch) != DG_CASTER_PROXY && GET_ADMLEVEL(ch) < ADMLVL_IMPL &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return (0);

  /* Mobs with a NOKILL flag */
  if (MOB_FLAGGED(victim, MOB_NOKILL)) {
    send_to_char(ch, "But they are not to be killed!\r\n");
    return (0);
  }

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  GET_FIGHTING_MAX_LVL(victim) = MAX(GET_FIGHTING_MAX_LVL(victim), GET_LEVEL(ch));  

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (affected_by_spell(ch, SPELL_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
    appear(ch);

  if (IS_NPC(victim) || ((victim->player_specials->summon_num == 0 || !PRF_FLAGGED(victim, PRF_SUMMON_TANK)) && 
      ((victim->player_specials->mount_num == 0) || !PRF_FLAGGED(victim, PRF_MOUNT_TANK)))) {

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (victim != ch && AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;
 /* Calculate Damage for the New Pet System */
  if(ch->new_summon == TRUE && !IS_NPC(ch->master) && dam > 0)
  {
      dam = (ch->level / 2) + ability_mod_value(GET_STR(ch)) + dice(1, 6) + (get_skill_value(ch->master, SKILL_HANDLE_ANIMAL) / 4);
  }
  /* Check for PK if this is not a PK MUD */
  if (!CONFIG_PK_ALLOWED) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }
  if (dam && GET_CLASS_RANKS(victim, CLASS_MONK) > 19) {
    if (!spell && !magic)
      reduction = 10;
  }
  
  if (AFF_FLAGGED(victim, AFF_CELESTIAL) || AFF_FLAGGED(victim, AFF_FIENDISH)) {
  	if (GET_LEVEL(victim) > 11 && !spell && !magic)
  		reduction = MAX(reduction, 10);
  	else if (GET_LEVEL(victim) > 3 && !spell && !magic)
  		reduction = MAX(reduction, 5);
  }
  
  if (dam && attacktype != TYPE_SUFFERING) {
    for (reduct = victim->damreduct; reduct && reduction > -1; reduct = reduct->next) {
      passed = 0;
      if (reduct->mod > reduction || reduct->mod == -1) {
        for (rtest = 0; !passed && rtest < MAX_DAMREDUCT_MULTI; rtest++) {
          if (reduct->damstyle[rtest] != DR_NONE) {
            switch (reduct->damstyle[rtest]) {
            case DR_ADMIN:
              if (GET_ADMLEVEL(ch) >= reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_MATERIAL:
              if (material == reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_BONUS:
              if (bonus >= reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_SPELL:
              if (!reduct->damstyleval[rtest] && spell > 0)
                passed = 1;
              if (spell == reduct->damstyleval[rtest] && spell)
                passed = 1;
              break;
            case DR_MAGICAL:
              if (magic)
                passed = 1;
              break;
            default:
              log("Unknown DR exception type %d", reduct->damstyle[rtest]);
              continue;
            }
          }
        }
        if (!passed) {
          if (reduct->mod == -1) /* Special - Full reduction */
            reduction = dam;
          else
            reduction = MAX(reduction, reduct->mod);
          if (reduct->max_damage != -1) {
            reduct->max_damage -= MIN(dam, reduction);
            if (reduct->max_damage <= 0) {
              send_to_char(victim, "Your damage reduction effect %s has worn off.\r\n", reduct_desc(victim, reduct));
              REMOVE_FROM_LIST(reduct, victim->damreduct, next);
            }
          }
        }
      }
    }
  }

  if (spell > 0 && HAS_FEAT(victim, FEAT_ENERGY_RESISTANCE))
    reduction += (HAS_FEAT(victim, FEAT_ENERGY_RESISTANCE) * 3);

  if (GET_GUILD(victim) == GUILD_DEVOTIONISTS && dice(1, 10) == 1)
    reduction += (GET_GUILD_RANK(victim) + 3) / 4;

  if (armor) {
    switch (armor_list[GET_OBJ_VAL(armor, 9)].armorType) {
      case ARMOR_TYPE_LIGHT:
        if (GET_OBJ_MATERIAL(armor) == MATERIAL_ADAMANTINE)
          reduction += 1;
        if (HAS_FEAT(victim, FEAT_ARMOR_SPECIALIZATION_LIGHT))
          reduction += 2;
        break;
      case ARMOR_TYPE_MEDIUM:
        if (GET_OBJ_MATERIAL(armor) == MATERIAL_ADAMANTINE)
          reduction += 2;
        if (HAS_FEAT(victim, FEAT_ARMOR_SPECIALIZATION_MEDIUM))
          reduction += 2;
        break;
      case ARMOR_TYPE_HEAVY:
        if (GET_OBJ_MATERIAL(armor) == MATERIAL_ADAMANTINE)
          reduction += 3;
        if (HAS_FEAT(victim, FEAT_ARMOR_SPECIALIZATION_HEAVY))
          reduction += 2;
        break;
    }
  }

  if (shield && GET_OBJ_MATERIAL(shield) == MATERIAL_ADAMANTINE)
    reduction += 1;

  if (dam == 0)
    reduction = 0;

  if (FIGHTING(ch) && ch->smiting && FIGHTING(ch) == ch->smiting)
    reduction = 0;

  if (reduction > 0) {
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) += MIN(dam, reduction);
    dam -= reduction;
  }

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(dam, 0);
  if (AFF_FLAGGED(ch, AFF_SPIRIT) || AFF_FLAGGED(victim, AFF_SPIRIT))
    dam = 0;

  if (victim->coupdegrace) {
    if (!mag_newsaves(SAVING_FORTITUDE, ch, victim, 0, dam))
      dam += 99999999999;
    victim->coupdegrace = 0;
  }

  if (dam >= 10 && PRF_FLAGGED(ch, PRF_KNOCKDOWN) && HAS_FEAT(ch, FEAT_KNOCKDOWN)) {
    do_trip(ch,GET_NAME(victim),0,0);
  }

  if (!IS_NPC(ch) && !IS_NPC(victim) && ch != victim) {
    if (dam > 0) {
      dam = MAX(1, dam / 5);
    }
  }

  GET_DAMAGE_DEALT_THIS_ROUND(ch) += dam;
  GET_DAMAGE_TAKEN_THIS_ROUND(victim) += dam;
  victim->damage_taken_last_round += dam;

  GET_HIT(victim) -= dam;

  GET_HATE(ch) = (GET_HATE(ch) * 80 / 100) + (dam / 2);

  if (GET_POS(victim) == POS_SLEEPING) {
    REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_SLEEP);
    GET_POS(victim) = POS_RESTING;
    do_stand(victim, 0, 0, 0);
  }

  update_pos(victim);

  }  // end of check to see if the person has a summon available
  else if (victim->player_specials->summon_num > 0 && PRF_FLAGGED(victim, PRF_SUMMON_TANK))  { 
    if (victim->player_specials->summon_dr > 0)
      dam -= victim->player_specials->summon_dr;
    victim->player_specials->summon_cur_hit -= dam;

  }  // end of check to see if the person has a summon available
  else if (victim->player_specials->mount_num > 0 && PRF_FLAGGED(victim, PRF_MOUNT_TANK))  { 
    if (victim->player_specials->mount_dr > 0)
      dam -= victim->player_specials->mount_dr;
    victim->player_specials->mount_cur_hit -= dam;

  }  // end of check to see if the person has a mount available

  GET_DAMAGE_TAKEN(victim) += dam;
  GET_FIGHT_DAMAGE_DONE(ch) += dam;

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * if we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. if we are attacking with a weapon: if this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (attacktype) {
    if (!IS_WEAPON(attacktype))
      skill_message(dam, ch, victim, attacktype, reduction > 0);
    else {
      dam_message(dam, ch, victim, attacktype, is_crit, reduction > 0);
    }
  }
  /* Use send_to_char -- act() doesn't send message if you are DEAD. */

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", false, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
//  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
//    stop_fighting(victim);

  if (spell > 0) {

  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", true, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", true, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", true, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    if ((IS_NPC(victim) && GET_ID(victim) != ch->last_kill_rnum))
      act("$n is dead!  R.I.P.", false, victim, 0, 0, TO_ROOM);
    break;

  default:			/* >= POSITION SLEEPING */
    if (GET_FIGHT_DAMAGE_DONE(ch) > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      send_to_char(victim, "@[4]You wish that your wounds would stop BLEEDING so much!@n\r\n");
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY) && IN_ROOM(ch) == IN_ROOM(victim))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0 &&
        IN_ROOM(ch) == IN_ROOM(victim)) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD && !AFF_FLAGGED(victim, AFF_SPIRIT)) {
/* This will get set in raw_kill() */
#if 0
    SET_BIT_AR(AFF_FLAGS(victim), AFF_SPIRIT);
#endif
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    send_to_char(victim, "@w%s@n", death_message);

    die(victim, ch);

  struct char_data *k = NULL, *temp = NULL;
  struct follow_type *f = NULL;
  ubyte found = FALSE;

      for (k = world[IN_ROOM(ch)].people; k; k = temp) {
        temp = k->next_in_room;
        if (FIGHTING(k) == ch) {
          found = TRUE;
        }
      }
      if (!found) {
        stop_fighting(ch);
        GET_POS(ch) = POS_STANDING;
        update_pos(ch);
        for (f = ch->followers; f; f = f->next) {
          found = FALSE;
          for (k = world[IN_ROOM(f->follower)].people; k; k = temp) {
            temp = k->next_in_room;
            if (FIGHTING(k) == f->follower) {
              found = TRUE;
            }
          }
          if (!found) {
            stop_fighting(f->follower);
            GET_POS(f->follower) = POS_STANDING;
            update_pos(f->follower);
          }
        }
      }

    return -1;
    }
  }

  return (dam);
}


/*
 * Calculate the hit bonus of the attacker.
 */
int compute_base_hit(struct char_data *ch, int weaponmod)
{
  struct obj_data *wielded, *offhand, *armor, *shield;
  struct char_data *k;
  struct follow_type *f;

  if (affected_by_spell(ch, SPELL_AFF_DISARMED)) {
    wielded = NULL;
    offhand = NULL;
  }
  else {
    wielded = GET_EQ(ch, WEAR_WIELD);
    offhand = GET_EQ(ch, WEAR_HOLD);
  }
  armor   = GET_EQ(ch, WEAR_BODY);
  shield  = GET_EQ(ch, WEAR_SHIELD);

/*
  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      wielded = NULL;
      offhand = NULL;
      weaponmod = 0;
      armor = NULL;
      shield = NULL;
  }
*/
  int armor_check_penalty = 0;
  int i;
	
  int calc_bonus;

  calc_bonus = GET_BAB(ch) + get_size_bonus(get_size(ch));

  if (wielded && IS_BOW(wielded))
    calc_bonus += HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC);

  if (wielded && HAS_FEAT(ch, FEAT_DIVINE_BOND))
    calc_bonus += 1 + MIN(6, MAX(0, (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 5) / 3));

  // flanking bonus
  if (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) != ch)
    calc_bonus +=2;

  calc_bonus += GET_ACCURACY_MOD(ch);

  if (RIDING(ch) && FIGHTING(ch) && get_size(RIDING(ch)) > get_size(FIGHTING(ch)))
    calc_bonus += 1;
  
  calc_bonus -= GET_EXPERTISE_BONUS(ch);  

  if (FIGHTING(ch) && ch->smiting && FIGHTING(ch) == ch->smiting)
    calc_bonus += ability_mod_value(GET_CHA(ch));

  if (affected_by_spell(ch, SPELL_HASTE))
    calc_bonus += 1;

  if (has_daylight(ch) && GET_RACE(ch) == RACE_GRAY_DWARF)
    calc_bonus -= 2;

 if (has_daylight(ch) && GET_RACE(ch) == RACE_DROW_ELF)
    calc_bonus -= 1;

  if (wielded && (((offhand || shield) && GET_OBJ_SIZE(wielded) > get_size(ch)) || IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) )
    calc_bonus -= 2;

  if (affected_by_spell(ch, SPELL_PRAYER))
    calc_bonus += 1;

  int distance = 0;
  int range = 0;

  if (FIGHTING(ch)) {
    if ((distance = (ch->combat_pos - FIGHTING(ch)->combat_pos)) > 0) {
      if (wielded) {
        range = weapon_list[GET_OBJ_VAL(wielded, 0)].range;
        if (distance >= (range * 3))
          calc_bonus -= 4;
        else if (distance >= (range * 2))
          calc_bonus -= 2;
      }
    }
    else if ((distance = (FIGHTING(ch)->combat_pos - ch->combat_pos)) > 0) {
      if (wielded) {
        range = weapon_list[GET_OBJ_VAL(wielded, 0)].range;
        if (distance >= (range * 3))
          calc_bonus -= 4;
        else if (distance >= (range * 2))
          calc_bonus -= 2;
      }
    }
  }

  if (affected_by_spell(ch, SPELL_INSPIRE_GREATNESS))
    calc_bonus += 2;


  if (affected_by_spell(ch, SPELL_MAGIC_WEAPON))
    weaponmod = MAX(weaponmod, 1);

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON)) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_WEAPON)
        weaponmod = MAX(weaponmod, af->modifier);
    }
  }

  if (affected_by_spell(ch, SPELL_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE)))
    weaponmod = MAX(weaponmod, 1);

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponmod = MAX(weaponmod, af->modifier);
    }
  }


  if (wielded && (GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_CLUB || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_QUARTERSTAFF ||
      GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_GREAT_CLUB) && affected_by_spell(ch, SPELL_SHILLELAGH)) {
    weaponmod = MAX(weaponmod, 1);
  }

  calc_bonus += weaponmod;

  if (FIGHTING(ch) && HAS_FEAT(FIGHTING(ch), FEAT_ROBILARS_GAMBIT) && PRF_FLAGGED(FIGHTING(ch), PRF_ROBILARS_GAMBIT))
    calc_bonus += 4;

  if (affected_by_spell(ch, SPELL_BESTOW_CURSE_PENALTIES))
    calc_bonus -= 4;

  if ((!wielded && HAS_FEAT(ch, FEAT_WEAPON_FINESSE)) ||
  	(wielded && (weapon_list[GET_OBJ_VAL(wielded, 0)].size < get_size(ch) ||
         IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) && HAS_FEAT(ch, FEAT_WEAPON_FINESSE))) {
  	 calc_bonus += ability_mod_value(GET_DEX(ch));
  }
  else if (wielded && IS_RANGED_WEAPON(wielded)) {
     calc_bonus += ability_mod_value(GET_DEX(ch));
  }
  else
    calc_bonus += ability_mod_value(GET_STR(ch));

  if (affected_by_spell(ch, SPELL_SICKENED))
    calc_bonus -= 2;

  if (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS) && GET_CLASS_RANKS(ch, CLASS_MONK) && 
      ((GET_EQ(ch, WEAR_WIELD) && IS_MONK_WEAPON(GET_EQ(ch, WEAR_WIELD))) ||
      (!GET_EQ(ch, WEAR_HOLD) && !GET_EQ(ch, WEAR_HOLD)))) {
      calc_bonus -= HAS_FEAT(ch, FEAT_GREATER_FLURRY) ? 1 : 2;
  }  

  if ((!wielded || (wielded && IS_MONK_WEAPON(wielded))) && (!armor || (armor && (GET_OBJ_TYPE(armor) == ITEM_WORN || GET_OBJ_VAL(armor, 0) == 0))) && !shield) {
    calc_bonus += base_hit(BASEHIT_HIGH, CLASS_FIGHTER, GET_CLASS_RANKS(ch, CLASS_MONK)) - 
                  base_hit(BASEHIT_MEDIUM, CLASS_MONK, GET_CLASS_RANKS(ch, CLASS_MONK)) +
                  base_hit(BASEHIT_HIGH, CLASS_FIGHTER, GET_CLASS_RANKS(ch, CLASS_SACRED_FIST)) - 
                  base_hit(BASEHIT_MEDIUM, CLASS_MONK, GET_CLASS_RANKS(ch, CLASS_SACRED_FIST));
  }

  if (HAS_FEAT(ch, FEAT_POINT_BLANK_SHOT) && wielded && IS_RANGED_WEAPON(wielded))
    calc_bonus += 1;

  if (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) == ch && !HAS_FEAT(ch, FEAT_PRECISE_SHOT) && wielded && IS_RANGED_WEAPON(wielded))
    calc_bonus -= 4;

  if (!HAS_FEAT(ch, FEAT_IMPROVED_PRECISE_SHOT) && wielded && IS_RANGED_WEAPON(wielded))
    calc_bonus += 1;

  if (AFF_FLAGGED(ch, AFF_RAPID_SHOT) && wielded && IS_RANGED_WEAPON(wielded))
    calc_bonus -= 2;

  calc_bonus += HAS_FEAT(ch, FEAT_EPIC_PROWESS);
  
  if (wielded && ((offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON) || IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE))) {
  	if (HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING))
  		calc_bonus -= 4;
  	else
  		calc_bonus -= 6;
  	if ((offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON && (GET_OBJ_SIZE(offhand) < get_size(ch))) || 
            (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && 
             IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)))
  		calc_bonus += 2;
  }    

  if (HAS_FEAT(ch, FEAT_POWER_ATTACK) && GET_POWERATTACK(ch) && GET_STR(ch) > 12)
    calc_bonus -= GET_POWERATTACK(ch);

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
    !is_proficient_with_weapon(ch, GET_OBJ_VAL(wielded, 0)))
    calc_bonus -= 4; /* Lack of proficiency yields less accuracy */

  calc_bonus -= GET_ARMORCHECK(ch);

    if (wielded) {
      if (HAS_WEAPON_MASTERY(ch, wielded) && HAS_FEAT(ch, FEAT_SUPERIOR_WEAPON_FOCUS))
        calc_bonus++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        calc_bonus++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_GREATER_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        calc_bonus++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        calc_bonus++;
    } else {
      if ((HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_UNARMED) && HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE) && HAS_FEAT(ch, FEAT_SUPERIOR_WEAPON_FOCUS)) ||
          (has_weapon_feat(ch, FEAT_WEAPON_FOCUS, WEAPON_TYPE_UNARMED) && HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE) && HAS_FEAT(ch, FEAT_SUPERIOR_WEAPON_FOCUS)))
        calc_bonus++;     
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_FOCUS, WEAPON_TYPE_UNARMED))
        calc_bonus++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_GREATER_WEAPON_FOCUS, WEAPON_TYPE_UNARMED))
        calc_bonus++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED))
        calc_bonus++;
    }

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	calc_bonus = calc_bonus;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	calc_bonus++;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	calc_bonus++;
      	  break;
        }
      }
    }
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR ||
        GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR_SUIT))
      if (!is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), 9)) &&
          GET_OBJ_VAL(GET_EQ(ch, i), 3) > armor_check_penalty)
      armor_check_penalty = GET_OBJ_VAL(GET_EQ(ch, i), 3);
  }

  calc_bonus -= armor_check_penalty;

  if (ch->mentor_level > 0) {
    calc_bonus = calc_bonus * ch->mentor_level / GET_CLASS_LEVEL(ch);
  }

  return calc_bonus;
}

int compute_summon_base_hit(struct char_data *ch)
{
  if (IS_NPC(ch) || ch->player_specials->summon_num == 0)
   return 0;


  struct char_data *k;
  struct follow_type *f;
  int calc_bonus = 0;
  int weaponmod = 0;


  if (affected_by_spell(ch, SPELL_PRAYER))
    calc_bonus += 1;

  if (affected_by_spell(ch, SPELL_MAGIC_FANG))
    weaponmod = 1;

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponmod = MAX(weaponmod, af->modifier);
    }
  }

  calc_bonus += weaponmod;

  return 0;

  if (affected_by_spell(ch, SPELL_HASTE))
    calc_bonus += 1;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	calc_bonus = calc_bonus;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	calc_bonus++;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	calc_bonus++;
      	  break;
        }
      }
    }
  }

  return calc_bonus;
}

int compute_companion_base_hit(struct char_data *ch)
{
  if (IS_NPC(ch) || ch->player_specials->companion_num == 0)
   return 0;


  struct char_data *k;
  struct follow_type *f;
  int calc_bonus = 0;
  int weaponmod = 0;

  calc_bonus += MAX(0, ((MAX(0, GET_CLASS_RANKS(ch, CLASS_RANGER) - 4)) + GET_CLASS_RANKS(ch, CLASS_DRUID)) - pet_list[ch->player_specials->companion_num].level);

  if (affected_by_spell(ch, SPELL_PRAYER))
    calc_bonus += 1;

  if (affected_by_spell(ch, SPELL_MAGIC_FANG))
    weaponmod = 1;

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponmod = MAX(weaponmod, af->modifier);
    }
  }

  calc_bonus += weaponmod;

  return 0;

  if (affected_by_spell(ch, SPELL_HASTE))
    calc_bonus += 1;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	calc_bonus = calc_bonus;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	calc_bonus++;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	calc_bonus++;
      	  break;
        }
      }
    }
  }

  return calc_bonus;
}

int compute_mount_base_hit(struct char_data *ch)
{
  if (IS_NPC(ch) || ch->player_specials->mount_num == 0)
   return 0;


  struct char_data *k;
  struct follow_type *f;
  int calc_bonus = 0;
  int weaponmod = 0;


  if (affected_by_spell(ch, SPELL_PRAYER))
    calc_bonus += 1;

  if (affected_by_spell(ch, SPELL_MAGIC_FANG))
    weaponmod = 1;

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponmod = MAX(weaponmod, af->modifier);
    }
  }

  calc_bonus += weaponmod;

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) < 5 && GET_CLASS_RANKS(ch, CLASS_RANGER) < 4 && GET_CLASS_RANKS(ch, CLASS_DRUID) < 1)
    return 0;

  if (affected_by_spell(ch, SPELL_HASTE))
    calc_bonus += 1;

  if (!(k = ch->master))
    k = ch;

  if (k == ch && !(k->followers)) {
  	// In this case nothing changes
  	calc_bonus = calc_bonus;
  }
  else if (HAS_FEAT(k, FEAT_LEADERSHIP)) {
  	calc_bonus++;
  }
  else {
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
        if (HAS_FEAT(f->follower, FEAT_LEADERSHIP)) {
        	calc_bonus++;
      	  break;
        }
      }
    }
  }

  return calc_bonus;
}


/*
 * Find next appropriate target for someone who kills their current target
 */
struct char_data *find_next_victim(struct char_data *ch)
{
  struct char_data *victim;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return NULL;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room) {
    if (!victim)
      return NULL;
    if (GET_POS(victim) == POS_FIGHTING && FIGHTING(victim) == ch) {
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) == AFF_FLAGGED(ch, AFF_ETHEREAL)) {
        if (GET_FORM_POS(victim) > FORM_POS_FRONT && IS_NPC(ch) && AFF_FLAGGED(victim, AFF_GROUP) && 
	  ((victim->master && AFF_FLAGGED(victim->master, AFF_GROUP)) || 
	  (victim->followers && AFF_FLAGGED(victim->followers->follower, AFF_GROUP))))
        {
          if (victim->master == ch || victim->master == ch->master)
            continue;
          if (GET_FORM_POS(victim) > FORM_POS_MIDDLE && rand_number(1, 20) > 18)
	    return victim;
          else if (GET_FORM_POS(victim) > FORM_POS_FRONT && rand_number(1, 20) > 14)
            return victim;
          else if (GET_FORM_POS(victim) <= FORM_POS_FRONT)
            return victim;
          else
            continue;
        }
        else if (victim->master != ch && victim->master != ch->master)
          return victim;
      }
    }
  }

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    return NULL;
  }

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room) {
    if (!victim)
      return NULL;
    if (IS_NPC(victim) && GET_POS(victim) == POS_FIGHTING &&
        ((ch->master && FIGHTING(victim) && FIGHTING(victim) == ch->master) ||
         (FIGHTING(victim) && FIGHTING(victim)->master && FIGHTING(victim)->master == ch) ||
         (FIGHTING(victim) && ch->master && FIGHTING(victim)->master && FIGHTING(victim)->master == ch->master) ) ) {
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) == AFF_FLAGGED(ch, AFF_ETHEREAL)) {
        if (GET_FORM_POS(victim) > FORM_POS_FRONT && IS_NPC(ch) && AFF_FLAGGED(victim, AFF_GROUP) &&
	  ((victim->master && AFF_FLAGGED(victim->master, AFF_GROUP)) || 
	  (victim->followers && AFF_FLAGGED(victim->followers->follower, AFF_GROUP))))
          {
            if (victim->master == ch || victim->master == ch->master)
              continue;
            if (GET_FORM_POS(victim) > FORM_POS_MIDDLE && rand_number(1, 20) > 18)
	      return victim;
            else if (GET_FORM_POS(victim) > FORM_POS_FRONT && rand_number(1, 20) > 14)
              return victim;
            else if (GET_FORM_POS(victim) <= FORM_POS_FRONT)
              return victim;
            else
              continue;
          }
        else if (victim->master != ch && victim->master != ch->master)
          return victim;
      }
    }
  }

  return NULL;

}


int dam_dice_scaling_table[][6] = {
/* old  down    up */
{1, 2,	1, 1,	1, 3},
{1, 3,	1, 2,	1, 4},
{1, 4,	1, 3,	1, 6},
{1, 6,	1, 4,	1, 8},
{1, 8,  1, 6,   2, 6},
{1,10,	1, 8,	2, 8},
{2, 6,	1,10,	3, 6},
{2, 8,	2, 6,	3, 8},
{2,10,	2, 8,	4, 8},
{3, 6,  2, 6,   4, 6},
{4, 6,  3, 6,   6, 6},
{3, 8,  2, 8,   4, 8},
{4, 8,  3, 8,   6, 8},
{1, 12, 1, 10,  3, 6},
{6, 6,  4, 6,   8, 6},
{0, 0,	0, 0,	0, 0}
};


void scaleup_dam(int *num, int *size)
{
  int i = 0;
  for (i = 0; dam_dice_scaling_table[i][0]; i++)
    if (dam_dice_scaling_table[i][0] == *num &&
        dam_dice_scaling_table[i][1] == *size) {
      *num = dam_dice_scaling_table[i][4];
      *size = dam_dice_scaling_table[i][5];
      return;
    }
  log("scaleup_dam: No dam_dice_scaling_table entry for %dd%d in fight.c", *num, *size);
}


void scaledown_dam(int *num, int *size)
{
  int i = 0;
  for (i = 0; dam_dice_scaling_table[i][0]; i++)
    if (dam_dice_scaling_table[i][0] == *num &&
        dam_dice_scaling_table[i][1] == *size) {
      *num = dam_dice_scaling_table[i][2];
      *size = dam_dice_scaling_table[i][3];
      return;
    }
  log("scaledown_dam: No dam_dice_scaling_table entry for %dd%d in fight.c", *num, *size);
}


int bare_hand_damage(struct char_data *ch, int code)
{
  int num, size, lvl, sz;
  int scale = 1;

  lvl = GET_CLASS_RANKS(ch, CLASS_MONK) + GET_CLASS_RANKS(ch, CLASS_SACRED_FIST);    

  if (IS_NPC(ch)) 
  {
    lvl = MAX(1, GET_LEVEL(ch) - 3);
  }

  if (!lvl) {

    num = 1;
    size = 3;
  } 
  else {
    num = 1;
    switch (lvl) 
    {
      case 0:
      case 1:
      case 2:
      case 3:
      size = 6;
      break;
      case 4:
      case 5:
      case 6:
      case 7:
      size = 8;
      break;
      case 8:
      case 9:
      case 10:
      case 11:
      size = 10;
      break;
      case 12:
      case 13:
      case 14:
      case 15:
      size = 6;
      num = 2;
      break;
      case 16:
      case 17:
      case 18:
      case 19:
      size = 8;
      num = 2;
      break;
      default:
      size = 10;
      num = 2;
      break;
    }
  }

if (!lvl && HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
{
  num = 1;
  size = 6;
}

if (scale) 
{
  sz = get_size(ch);
  if (HAS_FEAT(ch, FEAT_IMPROVED_NATURAL_WEAPON))
    sz++;
  if (sz < SIZE_MEDIUM)
    for (lvl = sz; lvl < SIZE_MEDIUM; lvl++)
      scaledown_dam(&num, &size);
    else if (sz > SIZE_MEDIUM)
      for (lvl = sz; lvl > SIZE_MEDIUM; lvl--)
        scaleup_dam(&num, &size);
    }

    if (code == 0)
      return dice(num, size);
    else if (code == 1)
      return num;
    else
      return size;
  }


int crit_range_extension(struct char_data *ch, struct obj_data *weap)
{
  int ext = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_CRITRANGE) + 1 : 1; /* include 20 */
  int tp = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_SKILL) : WEAPON_TYPE_UNARMED;
  int mult = 1;
  int imp_crit = FALSE;

  if (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_CRITICAL, tp) ||
      has_weapon_feat(ch, FEAT_IMPROVED_CRITICAL, tp))
    imp_crit = TRUE;

  if (AFF_FLAGGED(ch, AFF_KEEN_WEAPON)) {
    if (weap) {
      if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
        imp_crit = TRUE;
      else if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_PIERCING))
        imp_crit = TRUE;
    }
    else if (IS_NPC(ch)) {
      switch (GET_ATTACK(ch) + TYPE_HIT) {
        case TYPE_SLASH:
        case TYPE_BITE:
        case TYPE_SHOOT:
        case TYPE_CLEAVE:
        case TYPE_CLAW:
        case TYPE_LASH:
        case TYPE_THRASH:
        case TYPE_PIERCE:
        case TYPE_GORE:
          imp_crit = TRUE;
          break;
      }
    }
    else {
      if (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
        imp_crit = TRUE;
    }
  }

  if (AFF_FLAGGED(ch, AFF_IMPACT_WEAPON)) {
    if (weap) {
      if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
        imp_crit = TRUE;
    }
    else if (IS_NPC(ch)) {
      switch (GET_ATTACK(ch) + TYPE_HIT) {
        case TYPE_HIT:
        case TYPE_STUN:
        case TYPE_BLUDGEON:
        case TYPE_BLAST:
        case TYPE_PUNCH:
        case TYPE_BATTER:
          imp_crit = TRUE;
          break;
      }
    }
    else {
      if (!HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
        imp_crit = TRUE;
    }
  }

  if (imp_crit)
    mult++;

  if (HAS_WEAPON_MASTERY(ch, weap) && HAS_FEAT(ch, FEAT_KI_CRITICAL))
    mult++;

  return (ext * mult) - 1; /* difference from 20 */

}

int one_hit(struct char_data *ch, struct char_data *victim, struct obj_data *wielded, int w_type, int calc_base_hit, 
            char *damstr, char *critstr, int hitbonus)
{
  calc_base_hit *= 10;
  int victim_ac, dam = 0, diceroll;
  int is_crit = false, range, damtimes, strmod, ndice, diesize = 0, sneak = 0;
  struct affected_type af2;
  struct affected_type *fsaf = NULL;
  int fsdam = 0;
  char fsbuf[100];
  struct obj_data *armor;
  int sneakdam = 0, precisedam = 0, dexCap = 9999;
  int weaponDamMod = 0;
  int j=0;
  int magic = FALSE;
  int roll = 0;
  struct char_data *tmp_ch = ch;
  struct affected_type af;

  if (!tmp_ch && victim)
    tmp_ch = FIGHTING(victim);


  if (!victim)
    return -1;

  GET_FIGHT_NUMBER_OF_ATTACKS(ch)++;

  if (!wielded || wielded == GET_EQ(ch, WEAR_WIELD1))
    ch->wield_type = WIELD_TYPE_MAIN;
  else
    ch->wield_type = WIELD_TYPE_OFF;

  if (affected_by_spell(ch, SPELL_AFF_DISARMED))
    wielded = NULL;

  if (HAS_FEAT(victim, FEAT_DEFLECT_ARROWS) && (!GET_EQ(victim, WEAR_WIELD1) || !GET_EQ(victim, WEAR_WIELD2))
  	  && (wielded && (WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED) || WEAPON_FLAGGED(wielded, WEAPON_FLAG_THROWN)))
    && (GET_TOTAL_ARROWS_DEFLECTED(victim) == 0)) {
    return -2;
  }

  if (damstr && *damstr)
    damstr = NULL; /* Only build a damstr if we don't have one yet */

  /* Calculate the raw armor including magic armor. */
  if (!IS_NPC(victim) && victim->player_specials->summon_num > 0 && PRF_FLAGGED(victim, PRF_SUMMON_TANK))
    victim_ac = compute_summon_armor_class(victim, ch);
  else if (!IS_NPC(victim) && victim->player_specials->mount_num > 0 && PRF_FLAGGED(victim, PRF_MOUNT_TANK))
    victim_ac = compute_mount_armor_class(victim, ch);
  else if (!IS_NPC(victim) && victim->player_specials->companion_num > 0 && PRF_FLAGGED(victim, PRF_COMPANION_TANK))
    victim_ac = compute_companion_armor_class(victim, ch);
  else 
    victim_ac = compute_armor_class(victim, ch);

  if (AFF_FLAGGED(victim, AFF_AOO) && HAS_FEAT(victim, FEAT_MOBILITY)) {
  	victim_ac += 4;
  	REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_AOO);
        armor = GET_EQ(ch, WEAR_BODY);
        if (skill_roll(ch, SKILL_TUMBLE) >= 15) {
          if (armor) {
            if (GET_OBJ_TYPE(armor) == ITEM_WORN || highest_armor_type(ch) <= ARMOR_TYPE_LIGHT)  // Light armor or less
              return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);          
           }
           else {
              return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);          
           } 
        }
  }


  /* roll the die and take your chances... */
  diceroll = dice(1, 200);

  armor = GET_EQ(ch, WEAR_BODY);
  if (armor && GET_OBJ_TYPE(armor) == ITEM_ARMOR) {
    dexCap = GET_OBJ_VAL(armor, 2);
  }
    
  /*
   * Decide whether this is a hit or a miss.
   *
   *  Victim asleep = hit, otherwise:
   *     1   = Automatic miss.
   *   2..19 = Checked vs. AC.
   *    20   = Automatic hit.
   */


  if (PRF_FLAGGED(ch, PRF_TAKE_TEN)) {
    if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
        has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))) {
      diceroll = 100;
    }
    else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED) ||
        has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED))) {
      diceroll = 100;
    }
  }

  if (victim_ac > (diceroll + calc_base_hit)) {
    if ((diceroll + calc_base_hit + 50) >= victim_ac) {
      if (wielded && ch->weapon_supremacy_miss == 0 && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))) {
        calc_base_hit += 50;
        ch->weapon_supremacy_miss = 1;
      }
      else if (!wielded && ch->weapon_supremacy_miss == 0 && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_SUPREMACY, WEAPON_TYPE_UNARMED))) {
        calc_base_hit += 50;
        ch->weapon_supremacy_miss = 1;
      }
    }
  }

  if (diceroll >= 191 || !AWAKE(victim)) {
    is_crit = true; 
    dam = true;
  }
  else if (AFF_FLAGGED(ch, AFF_SMITING)) {
    calc_base_hit += 10 * MAX(0, ability_mod_value(GET_CHA(ch)));
    dam = (diceroll + calc_base_hit) >= victim_ac;
  }
  else if (diceroll <= 5)
    dam = false;
  else
   dam = (diceroll + calc_base_hit) >= victim_ac;

  if (dice(1, 10) <= MIN(5, HAS_FEAT(victim, FEAT_SELF_CONCEALMENT))) {
    dam = false;
    is_crit = false;
  }

ch->att_roll = (diceroll + calc_base_hit) / 10;
  if ((ch->att_roll % 10) != 0 && (diceroll + calc_base_hit) > victim_ac) ch->att_roll += 1;
  if (ch->att_roll <= 0) ch->att_roll = 1;
  ch->opp_def = (victim_ac / 10);

  int parry = 0;
  int parried = FALSE;

  struct char_data *pchar = victim->master ? victim->master : victim;
  struct follow_type *fol;


  if (dam && !IS_NPC(pchar) && HAS_FEAT(pchar, FEAT_PARRY) && pchar->parries < num_parry_attacks(pchar, FALSE) && PRF_FLAGGED(pchar, PRF_PARRY)) {
    parry = d20 + compute_base_hit(pchar, 0);
    parry -= (pchar == victim) ? 0 : 4;
    parry -= 5 * pchar->parries;
    parry -= MAX(0, get_size(ch) - get_size(pchar)) * 4;
    parry *= 10;
    if (parry > calc_base_hit && !parried) {
      ch->new_parry = TRUE;
      parried = TRUE;
      dam = false;
      is_crit = false;
      ch->parried_attacks++;
      if (HAS_FEAT(pchar, FEAT_RIPOSTE) &&
          (GET_TOTAL_AOO(pchar) < MAX(1, HAS_FEAT(pchar, FEAT_COMBAT_REFLEXES) ? 1+ability_mod_value(GET_DEX(pchar)): 1))) {
          pchar->riposte = TRUE;
          do_attack_of_opportunity(pchar, ch, "Riposte");
          pchar->riposte = FALSE;
      }

    }
    pchar->parries++;
  }

  for (fol = pchar->followers; fol; fol=fol->next) {
    pchar = fol->follower;
    if (dam && !IS_NPC(pchar) && HAS_FEAT(pchar, FEAT_PARRY) && pchar->parries < num_parry_attacks(pchar, FALSE) && PRF_FLAGGED(pchar, PRF_PARRY)) {
      parry = d20 + compute_base_hit(pchar, 0);
      parry -= (pchar == victim) ? 0 : 4;
      parry -= 5 * pchar->parries;
      parry -= MAX(0, get_size(ch) - get_size(pchar)) * 4;
      parry *= 10;
      if (parry > calc_base_hit && !parried) {
        ch->new_parry = TRUE;
        parried = TRUE;
        dam = false;
        is_crit = false;
        ch->parried_attacks++;
      }
      pchar->parries++;
    }
  }



  if (!IS_NPC(victim) && victim->player_specials->mount_num > 0 && PRF_FLAGGED(victim, PRF_MOUNT_TANK) && ch->player_specials->mounted == MOUNT_MOUNT) {
    if (HAS_FEAT(victim, FEAT_MOUNTED_COMBAT) && (skill_roll(victim, SKILL_RIDE) + 10) > (diceroll + calc_base_hit))
      dam = false;
  }

  if (!IS_NPC(victim) && victim->player_specials->summon_num > 0 && PRF_FLAGGED(victim, PRF_SUMMON_TANK) && ch->player_specials->mounted == MOUNT_SUMMON) {
    if (HAS_FEAT(victim, FEAT_MOUNTED_COMBAT) && (skill_roll(victim, SKILL_RIDE) + 10) > (diceroll + calc_base_hit))
      dam = false;
  }

  if (!IS_NPC(victim) && victim->player_specials->companion_num > 0 && PRF_FLAGGED(victim, PRF_COMPANION_TANK) && ch->player_specials->mounted == MOUNT_COMPANION) {
    if (HAS_FEAT(victim, FEAT_MOUNTED_COMBAT) && (skill_roll(victim, SKILL_RIDE) + 10) > (diceroll + calc_base_hit))
      dam = false;
  }

  if (ch && victim && GET_POS(ch) > POS_DEAD && GET_POS(victim) > POS_DEAD && !IS_NPC(victim) &&
      PRF_FLAGGED(victim, PRF_ROBILARS_GAMBIT) && HAS_FEAT(victim, FEAT_ROBILARS_GAMBIT) &&
          (GET_TOTAL_AOO(victim) < MAX(1, HAS_FEAT(victim, FEAT_COMBAT_REFLEXES) ? 1+ability_mod_value(GET_DEX(victim)): 1))) {
    do_attack_of_opportunity(victim, ch, "Robilars");
  }

  struct char_data *xch;

  for (xch = world[IN_ROOM(ch)].people; xch; xch = xch->next_in_room) {
    if (HAS_FEAT(xch, FEAT_OPPORTUNIST) && is_player_grouped(xch, ch) && xch->opportunist == 0 && 
    (GET_TOTAL_AOO(xch) < MAX(1, HAS_FEAT(xch, FEAT_COMBAT_REFLEXES) ? 1+ability_mod_value(GET_DEX(xch)): 1))) {
      if (GET_HIT(victim) > -10 && xch != ch) {
        do_attack_of_opportunity(xch, victim, "Opportunist");
        xch->opportunist = 1;
      }
    }
  }

  if (victim->coupdegrace == 1) {
    is_crit = TRUE;;
    sneak = TRUE;
  }

  if (dice(1, 100) < (HAS_FEAT(ch, FEAT_IMPROVED_SNEAK_ATTACK) * 5))
    sneak = TRUE;

  if (!is_crit) {
  if (RIDING(victim) && HAS_FEAT(victim, FEAT_MOUNTED_COMBAT) && GET_MOUNTED_ATTACKS_AVOIDED(victim) == 0) {
    if ((skill_roll(victim, SKILL_RIDE) * 10 )> diceroll + calc_base_hit) {
      act("$n uses $s mount to deflect $N's attack", FALSE, victim, 0, ch, TO_NOTVICT);
      act("You use your mount to deflect $N's attack", FALSE, victim, 0, ch, TO_CHAR);
      act("$n uses $s mount to deflect your attack", FALSE, victim, 0, ch, TO_VICT);
      GET_MOUNTED_ATTACKS_AVOIDED(victim) += 1;
      return -1;
    }
  }

  if (!CAN_SEE(ch, victim) && !HAS_FEAT(ch, FEAT_BLIND_FIGHT)) {
    if (dice(1, 100) <= 50) {
      act("@y$n cannot seem to find $s target and ends up attacking empty air.@n", FALSE, ch, 0, victim, TO_NOTVICT);
      act("@yYou cannot seem to find your target and you end up attacking empty air.@n", FALSE, ch, 0, victim, TO_CHAR);
      act("@y$n cannot seem to find you and ends up attacking empty air.@n", FALSE, ch, 0, victim, TO_VICT);
      return 0;
    }
  }

    if (affected_by_spell(victim, SPELL_BLUR)) {
      if (dice(1, 100) <= 20) {
        return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);          
      }
    }
  }

  if (!dam && !is_crit) {
    /* the attacker missed the victim */
    if (ch->actq) {
      free(ch->actq);
      ch->actq = 0;
    }
    return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);    
  } else {

    if (!is_crit && HAS_FEAT(ch, FEAT_EPIC_DODGE) && ch->player_specials->epic_dodge == FALSE) {
      return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);          
    }


    /* okay, we know the guy has been hit.  now calculate damage. */

    GET_FIGHT_NUMBER_OF_HITS(ch)++;

    damage_object(ch, victim);
    damage_object(victim, ch);

    strmod = ability_mod_value(GET_STR(ch));
    
    // If the character is wielding a light weapon and they have the improved weapon
    // finesse feat, make their strength mod equivalent to the characters armor-
    // capped dexterity modifier.
    if (wielded && (GET_OBJ_SIZE(wielded) < get_size(ch) || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_RAPIER ||
                 IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) && 
    	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)) || 
          has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)))) {
      strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
    }
    if (!wielded &&
    	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED) ||
    	  has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED))) {
      strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
    }

    range = 0;

    diceroll += 10 * (MAX(0, range = (crit_range_extension(ch, wielded))));

    if (wielded && wielded == GET_EQ(ch, WEAR_WIELD1) &&
        !GET_EQ(ch, WEAR_WIELD2) &&
        wield_type(get_size(ch), wielded) >= WIELD_ONEHAND &&
        !WEAPON_FLAGGED(wielded, WEAPON_FLAG_DOUBLE) && !WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED) && 
        !WEAPON_FLAGGED(wielded, WEAPON_FLAG_THROWN))
      strmod = strmod * 3 / 2;

    if (wielded && (WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED)) && !IS_BOW(wielded) && !IS_THROWN_WEAPON(wielded)) {
      if (!GET_CLASS_RANKS(ch, CLASS_RANGER) || 
          (weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFamily == WEAPON_FAMILY_CROSSBOW))
      	strmod = 0;
    }

    if (wielded && IS_RANGED_WEAPON(wielded)) {
      if (!IS_COMPOSITE_BOW(wielded) && !IS_THROWN_WEAPON(wielded)) {
       strmod = MAX(strmod, 2);
      }
    }
	
    if (w_type == TYPE_BITE && (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE)))
	  strmod /= 2;
	  
    if (w_type == TYPE_GORE && GET_RACE(ch) == RACE_MINOTAUR)
	  strmod /= 2;

    if (w_type == TYPE_BATTER && GET_RACE(ch) == RACE_CENTAUR)
      strmod /= 2;
      
    is_crit = 0;

    damtimes = 1;

    if (diceroll >= 196 || is_crit) {
      is_crit = FALSE;
      diceroll = rand_number(1, 200);
      if (wielded && IS_SET_AR(GET_OBJ_EXTRA(wielded), ITEM_BLESS) && victim && GET_ALIGN(victim) < -250)
        diceroll = 1000;
      if (((wielded && HAS_COMBAT_FEAT(ch, CFEAT_POWER_CRITICAL, GET_OBJ_VAL(wielded, 0))) ||
          (!wielded && HAS_COMBAT_FEAT(ch, CFEAT_POWER_CRITICAL, WEAPON_TYPE_UNARMED))) ||
          ((wielded && has_weapon_feat(ch, FEAT_POWER_CRITICAL, GET_OBJ_VAL(wielded, 0))) ||
          (!wielded && has_weapon_feat(ch, FEAT_POWER_CRITICAL, WEAPON_TYPE_UNARMED))))
        diceroll += 40;
      if ((diceroll + calc_base_hit) >= victim_ac) { /* It's a critical */
      is_crit = 1;
      if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
        diceroll = GET_OBJ_VAL(wielded, VAL_WEAPON_CRITTYPE);
      } else {
        diceroll = 0; /* default to crit type 0 (x2) */
      }
      switch (diceroll) {
      case CRIT_X5:
        damtimes = 5;
        break;
      case CRIT_X4:
        damtimes = 4;
        break;
      case CRIT_X3:
        damtimes = 3;
        break;
      case CRIT_X2:
        damtimes = 2;
        break;
      default:
        break;
      }
      if (wielded)
        damtimes = 2 + weapon_list[GET_OBJ_VAL(wielded, 0)].critMult;
      if (HAS_WEAPON_MASTERY(ch, wielded) && HAS_FEAT(ch, FEAT_INCREASED_MULTIPLIER))
        damtimes++;
      if (critstr && !*critstr) {
        if (range)
          sprintf(critstr, "(%d-20x%d)", 20 - range, damtimes);
        else
          sprintf(critstr, "(x%d)", damtimes);
      }
    }
  }

  if (!affected_by_spell(ch, SPELL_AFF_DISARMED) && 
        wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && GET_OBJ_VAL(wielded, 0) != WEAPON_TYPE_UNARMED) {
      if ((GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_CLUB || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_QUARTERSTAFF) &&
          affected_by_spell(ch, SPELL_SHILLELAGH)) {
        ndice = 2;
        diesize = 6;
      }
      else if ((GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_GREAT_CLUB) &&
          affected_by_spell(ch, SPELL_SHILLELAGH)) {
        ndice = 3;
        diesize = 8;
      }
      else {
        ndice = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMDICE);
        diesize = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMSIZE);
      }
    } else {
      if (IS_NPC(ch)) {
          ndice = MAX(1, GET_DAMAGE_MOD(ch) * (49 + dice(1, 101)) / 100);
          diesize = 1;
      }
      else {
        if ((w_type == TYPE_BITE && (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))) ||
          (w_type == TYPE_GORE && GET_RACE(ch) == RACE_MINOTAUR) ||
          (w_type == TYPE_BATTER && GET_RACE(ch) == RACE_CENTAUR)) {
          if (w_type == TYPE_BITE && (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))) {
            ndice = 1;
            diesize = 6;
          }
          if (w_type == TYPE_GORE && GET_RACE(ch) == RACE_MINOTAUR) {
            ndice = 1;
            diesize = 6;
          }
          if (w_type == TYPE_BATTER && GET_RACE(ch) == RACE_CENTAUR) {
            ndice = 1;
            diesize = 6;
          }
        }
        else {
          ndice = bare_hand_damage(ch, 1);
          diesize = bare_hand_damage(ch, 2);
        }
      }
    }
    
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (wielded && (wielded->affected[j].location == APPLY_DAMAGE) && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
        weaponDamMod = wielded->affected[j].modifier;
     }  

  if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    weaponDamMod += 2;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED)))
    weaponDamMod += 2;

  if (affected_by_spell(ch, SPELL_SICKENED))
    weaponDamMod -= 2;

  if (wielded && HAS_FEAT(ch, FEAT_DIVINE_BOND)) {
    weaponDamMod += 1 + MIN(6, MAX(0, (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 5) / 3));
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 23)
      if (HAS_FEAT(ch, FEAT_ENHANCE_ARROW_ALIGNED) && ((IS_GOOD(ch) && !IS_GOOD(victim)) || (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
        (IS_EVIL(ch) && !IS_EVIL(victim))))
        weaponDamMod += dice(1, 6);    
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 26)
        weaponDamMod += dice(1, 6);    
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 32)
        weaponDamMod += dice(1, 6);    
  }

  if (wielded && IS_BOW(wielded)) {
    weaponDamMod += HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC);
    if (HAS_FEAT(ch, FEAT_ENHANCE_ARROW_ELEMENTAL))
      weaponDamMod += dice(1, 6);    
    if (HAS_FEAT(ch, FEAT_ENHANCE_ARROW_ALIGNED) && ((IS_GOOD(ch) && !IS_GOOD(victim)) || (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
        (IS_EVIL(ch) && !IS_EVIL(victim))))
      weaponDamMod += dice(1, 6);    
  }

  if (wielded && (GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_CLUB || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_QUARTERSTAFF ||
      GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_GREAT_CLUB) && affected_by_spell(ch, SPELL_SHILLELAGH)) {
    weaponDamMod = MAX(weaponDamMod, 1);
    hitbonus = MAX(hitbonus, 1);  
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    weaponDamMod = MAX(weaponDamMod, 1);
    hitbonus = MAX(hitbonus, 1);  
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponDamMod = MAX(weaponDamMod, af->modifier);
        hitbonus = MAX(hitbonus, af->modifier);  
        magic = TRUE;
    }
  }

  if (affected_by_spell(ch, SPELL_MAGIC_WEAPON)) {
    weaponDamMod = MAX(weaponDamMod, 1);
    hitbonus = MAX(hitbonus, 1);  
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON)) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_WEAPON)
        weaponDamMod = MAX(weaponDamMod, af->modifier);
        hitbonus = MAX(hitbonus, af->modifier);  
        magic = TRUE;
    }
  }

  if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_ADAMANTINE)
    weaponDamMod += 1;
  
  if (ch->sneak_opp == TRUE)
    sneak = TRUE;

  if (sneak || (GET_POS(ch) != POS_FIGHTING && GET_POS(victim) != POS_FIGHTING) 
          || (victim && ch && !CAN_SEE(victim, ch))
          || (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) != ch) 
    	  || AFF_FLAGGED(victim, AFF_FLAT_FOOTED_1)
    	  || AFF_FLAGGED(victim, AFF_FLAT_FOOTED_2)
          || dice(1, 5) <= HAS_FEAT(ch, FEAT_SELF_CONCEALMENT)
          || HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) >= 5)    	  
      sneak = backstab_dice(ch);
    else
      sneak = 0;

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = strmod;
    dam += GET_DAMAGE_MOD(ch);
    if (AFF_FLAGGED(ch, AFF_SMITING))
      dam += (GET_CLASS_RANKS(ch, CLASS_PALADIN) + GET_CLASS_RANKS(ch, CLASS_TEMPLAR) +
              GET_CLASS_RANKS(ch, CLASS_CHAMPION)) * (1 + HAS_FEAT(ch, FEAT_GREAT_SMITING));
    if (HAS_FEAT(ch, FEAT_POWER_ATTACK) &&
        GET_POWERATTACK(ch) && GET_STR(ch) > 12) {
      dam += GET_POWERATTACK(ch);
      if (wielded && !GET_EQ(ch, WEAR_WIELD2) && !GET_EQ(ch, WEAR_SHIELD) && GET_OBJ_SIZE(wielded) > get_size(ch))
        dam += GET_POWERATTACK(ch);      
    }

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
    } else {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
    }

    if (IS_UNDEAD(victim) && affected_by_spell(ch, SPELL_FEAT_DIVINE_VENGEANCE))
      dam += dice(2, 6);

    if (has_favored_enemy(ch, victim))
      dam += dice(1, (GET_CLASS_RANKS(ch, CLASS_RANGER) / 5) + 1) * 2;

    if (affected_by_spell(ch, SPELL_PRAYER))
      dam += 1;

    if (affected_by_spell(ch, SPELL_FLAME_WEAPON) && wielded) {
      dam += dice(1, 8);
    }
    if (GET_RACE(ch) == RACE_SMALL_FIRE_ELEMENTAL)
      dam += dice(1, 6);
    if (GET_RACE(ch) == RACE_MEDIUM_FIRE_ELEMENTAL)
      dam += dice(1, 8);
    if (GET_RACE(ch) == RACE_LARGE_FIRE_ELEMENTAL)
      dam += dice(1, 10);
    if (GET_RACE(ch) == RACE_HUGE_FIRE_ELEMENTAL)
      dam += dice(2, 6);

    if (victim && HAS_FEAT(victim, FEAT_ROBILARS_GAMBIT) && PRF_FLAGGED(victim, PRF_ROBILARS_GAMBIT))
    weaponDamMod += 4;


    if (damstr)
      sprintf(damstr, "%dd%d+%d", ndice, diesize, dam);

   dam += weaponDamMod * damtimes;

    if (FIGHTING(ch) && ch->smiting && FIGHTING(ch) == ch->smiting)
      dam += GET_CLASS_RANKS(ch, CLASS_PALADIN);

    roll = dice(1, 20);
    while (damtimes--) {
      if (HAS_WEAPON_MASTERY(ch, wielded) && HAS_FEAT(ch, FEAT_KI_DAMAGE) && roll <= (2 + (HAS_FEAT(ch, FEAT_KI_DAMAGE))))
        dam += diesize;
      else
        dam += dice(ndice, diesize);
    }

    if (sneak) {
      if (HAS_FEAT(ch, FEAT_SNEAK_ATTACK)) {
        if (HAS_FEAT(ch, FEAT_POWERFUL_SNEAK) && PRF_FLAGGED(ch, PRF_POWERFUL_SNEAK))
          sneakdam = min_dice(HAS_FEAT(ch, FEAT_SNEAK_ATTACK), 6, 2);
        else 
          sneakdam = dice(HAS_FEAT(ch, FEAT_SNEAK_ATTACK), 6);
        if (HAS_FEAT(ch, FEAT_BLEEDING_ATTACK) && PRF_FLAGGED(ch, PRF_BLEEDING_ATTACK)) {
          GET_FIGHT_BLEEDING_DAMAGE(victim) = HAS_FEAT(ch, FEAT_SNEAK_ATTACK);
          sneakdam = 0;
        }
      }

      if (wielded && (GET_OBJ_SIZE(wielded) <= get_size(ch) || IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_BALANCED)) 
          && HAS_FEAT(ch, FEAT_PRECISE_STRIKE) && IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_PIERCING) && GET_EQ(ch, WEAR_HOLD) != wielded)
      	precisedam = GET_CLASS_RANKS(ch, CLASS_DUELIST);      
      if (is_crit && sneakdam)
        is_crit = 999;
      else if (is_crit && precisedam)
       	is_crit = 888;
      else if (is_crit && precisedam && sneakdam)
       	is_crit = 777;
      else if (!is_crit && precisedam)
       	is_crit = 555;
      else if (!is_crit && precisedam && sneakdam)
       	is_crit = 444;
      else if (!is_crit && sneakdam)
       	is_crit = 111;
      if (damstr && sneakdam)
          sprintf(damstr, "%s (+%dd6 sneak)", damstr, sneak);
      if (damstr && precisedam)
          sprintf(damstr, "%s (+%dd6 precise)", damstr, HAS_FEAT(ch, FEAT_PRECISE_STRIKE)); 
        	
      dam += sneakdam;
      dam += precisedam;
    }

    // Checks for death attack
    if (GET_DEATH_ATTACK(ch) == 1 || GET_UNDEATH_TOUCH(ch) == 1) {
      if (!AFF_FLAGGED(victim, AFF_DEATH_WARD) && 
          (!IS_NPC(victim) || (!MOB_FLAGGED(victim, MOB_LIEUTENANT) && !MOB_FLAGGED(victim, MOB_CAPTAIN) && !MOB_FLAGGED(victim, MOB_BOSS))) &&
          (GET_DEATH_ATTACK(ch) == 1 ? 
         !mag_newsaves(SAVING_FORTITUDE, ch, victim, 0, 4 + MIN(10, GET_MARK_ROUNDS(ch)) + GET_CLASS_RANKS(ch, CLASS_ASSASSIN) +  
          ability_mod_value(GET_INT(ch))) :
          !mag_newsaves(SAVING_FORTITUDE, ch, victim, 0, 10 + GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER) + ability_mod_value(GET_CHA(ch))))) {
        dam = GET_MAX_HIT(victim) * 10;
      }
      else {
        act("@r*save* @yYou resisted $n's death attack.", FALSE, ch, 0, victim, TO_VICT);
        GET_DEATH_ATTACK(ch) = 0;
      }
    }
    else if (GET_DEATH_ATTACK(ch) == 2 || GET_UNDEATH_TOUCH(ch) == 2) {
      if (GET_DEATH_ATTACK(ch) == 2 ? 
          !mag_newsaves(SAVING_FORTITUDE, ch, victim, 0, 4 + MIN(10, GET_MARK_ROUNDS(ch)) + GET_CLASS_RANKS(ch, CLASS_ASSASSIN) +  
          ability_mod_value(GET_INT(ch))) :
          !mag_newsaves(SAVING_FORTITUDE, ch, victim, 0, 10 + GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER) + ability_mod_value(GET_CHA(ch)))) {
	af2.type = (GET_DEATH_ATTACK(ch) == 2 ? SPELL_DEATH_ATTACK : SPELL_TOUCH_OF_UNDEATH);
        af2.location = APPLY_ABILITY;
        af2.modifier = -1;
        af2.duration = dice(1, 6) + (GET_DEATH_ATTACK(ch) == 2 ? GET_CLASS_RANKS(ch, CLASS_ASSASSIN) : GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER));
        af2.bitvector = AFF_STUNNED;

        affect_join(victim, &af2, false, false, true, false);

      }
      else {
        act("@r*save* @yYou resisted $n's death attack.", FALSE, ch, 0, victim, TO_VICT);
        GET_DEATH_ATTACK(ch) = 0;
      }

    }

   if (wielded && IS_BOW(wielded))    
     if (is_crit && HAS_FEAT(ch, FEAT_ENHANCE_ARROW_ELEMENTAL_BURST))
       dam += dice(2, 10);
   if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 29 && is_crit)
       dam += dice(2, 10);

    
    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);        

    magic = wielded ? OBJ_FLAGGED(wielded, ITEM_MAGIC) : FALSE;

    if (wielded && IS_SET_AR(GET_OBJ_EXTRA(wielded), ITEM_BLESS) && victim && GET_ALIGN(victim) < -250) {
      hitbonus = MAX(hitbonus, 1);
      magic = TRUE;
    }

    if (affected_by_spell(victim, SPELL_FIRE_SHIELD) && sr_check(victim, ch)) {
      for (fsaf = victim->affected; fsaf; fsaf = fsaf->next)
        if (fsaf->type == SPELL_FIRE_SHIELD) {
          fsdam = dice(1, 6) + fsaf->modifier;
          sprintf(fsbuf, "$n is enveloped in the flames of $N's fire shield for @R%d@n damage!", fsdam);
          act(fsbuf, FALSE, ch, 0, victim, TO_NOTVICT);
          sprintf(fsbuf, "@RYou are enveloped in the flames of $N's fire shield for @Y%d@R damage!@n", fsdam);
          act(fsbuf, FALSE, ch, 0, victim, TO_CHAR);
          sprintf(fsbuf, "@y$n is enveloped in the flames of your fire shield for @R%d@y damage!@n", fsdam);
          act(fsbuf, FALSE, ch, 0, victim, TO_VICT);
          damage(victim, ch, fsdam, SPELL_FIRE_SHIELD, 0, -1, 0, SPELL_FIRE_SHIELD, 1);
          break;
        }
    }

    if ((GET_RACE(ch) == RACE_SMALL_FIRE_ELEMENTAL || GET_RACE(ch) == RACE_MEDIUM_FIRE_ELEMENTAL || 
        GET_RACE(ch) == RACE_LARGE_FIRE_ELEMENTAL || GET_RACE(ch) == RACE_HUGE_FIRE_ELEMENTAL) &&
        !affected_by_spell(victim, SPELL_ON_FIRE)) {
      if (!mag_newsaves(SAVING_REFLEX, ch, victim, SPELL_ON_FIRE, (get_size(ch) * 2) + 12)) {
      
	af.type = SPELL_ON_FIRE;
        af.location = APPLY_ABILITY;
        af.modifier = 1;
        af.duration = dice(1, 4);
        af.bitvector = AFF_NONE;

        affect_join(victim, &af, false, false, true, false);

        act("$N's attack sets you on fire!", FALSE, victim, 0, ch, TO_CHAR);
        act("Your attack sets $n on fire!", FALSE, victim, 0, ch, TO_VICT);
        act("$N's attack sets $n on fire!", FALSE, victim, 0, ch, TO_NOTVICT);
      }
    }


    dam = damage(ch, victim, dam, w_type, is_crit,
                 wielded ?
                   GET_OBJ_VAL(wielded, VAL_WEAPON_MATERIAL) :
                   (GET_CLASS_RANKS(ch, CLASS_MONK) > 15) ?
                     MATERIAL_ADAMANTINE :
                     MATERIAL_ORGANIC,
                 hitbonus, 0,
                 wielded ?
                   magic :
                   GET_CLASS_RANKS(ch, CLASS_MONK) >= 4);


    do_mob_special_attacks(ch, MOB_TYPE_MOB);


    if (ch->actq) {
      call_magic(ch, victim, NULL, ch->actq->spellnum,
                 ch->actq->level, CAST_STRIKE, NULL);
      free(ch->actq);
      ch->actq = 0;
    }

    return dam;
  }
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded;
  int w_type, calc_base_hit, weap, lastweap, notfirst, cleave, hitbonus, i;
  int status, attacks, fullextra = 0;
  room_rnum loc;
  char buf[MAX_INPUT_LENGTH];
  int tot_attacks = 0;
  int weapon_flurry = 0;
  int hitmod = 0;
  char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200], bleedbuf[200];
  char parrybuf[100];
  long local_gold = 0;
  char local_buf[256];


  if (!ch || !victim)
    return;

  int rnum = 0;

  ch->damage_taken_last_round = 0;

  if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)))
    rnum = GET_ID(FIGHTING(ch));

  if (AFF_FLAGGED(ch, AFF_SPIRIT) && !IS_NPC(ch)) {
    send_to_char(ch, "You can't fight when you're dead!  Type 'resurrect' or wait for someone to raise you to return to life.\r\n");
    return;
  }

  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return;

//    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
//		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return;			/* -je, 7/7/92 */
  }



  if (PRF_FLAGGED(ch, PRF_AUTOFEINT))
    do_feint(ch, NULL, 0, 0);


  GET_FIGHTING_MAX_LVL(victim) = MAX(GET_FIGHTING_MAX_LVL(victim), GET_LEVEL(ch));

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  loc = IN_ROOM(victim);

  /* Do some sanity checking, in case someone flees, etc. */
  if (IN_ROOM(ch) != IN_ROOM(victim)) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  if (!AFF_FLAGGED(ch, AFF_NEXTNOACTION)) {

  if (AFF_FLAGGED(ch, AFF_NEXTPARTIAL) || GET_POS(ch) != POS_FIGHTING)
    lastweap = WEAR_WIELD1;
  else
    lastweap = WEAR_WIELD2;

  cleave = 0;
  if (!GET_EQ(ch, WEAR_WIELD1) && !IS_NPC(ch) && GET_FORM_POS(ch) > FORM_POS_FRONT) {
    send_to_char(ch, "You must be in the front row to fight hand-to-hand\r\n");
    return;
  }

  for (weap = WEAR_WIELD1; weap <= lastweap; weap++) {
    wielded = GET_EQ(ch, weap);
//    if (AFF_FLAGGED(ch, AFF_WILD_SHAPE))
//        wielded = NULL;
    
  if (wielded && !IS_NPC(ch)) {
    if (GET_FORM_POS(ch) > FORM_POS_FRONT) {
      if (GET_FORM_POS(ch) < FORM_POS_MIDDLE) {
        if (!(IS_RANGED_WEAPON(wielded) || IS_LONG_WEAPON(wielded) || 
              IS_THROWN_WEAPON(wielded))) {
          send_to_char(ch, "You are too far away from the enemy to attack it with this weapon\r\n");
          return;
        }
      }
      else {
        if (!(IS_RANGED_WEAPON(wielded) || IS_THROWN_WEAPON(wielded))) {
          send_to_char(ch, "You are too far away from the enemy to attack it with this weapon\r\n");          
          return;
        }
      }
    }  
  }


    /* Find the weapon type (for display purposes only) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      w_type = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMTYPE) + TYPE_HIT;
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (wielded->affected[i].location == APPLY_ACCURACY)
          hitmod += wielded->affected[i].modifier;
    } else {
      if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
      else
      w_type = TYPE_HIT;
    }

    if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_ADAMANTINE)
      hitmod += 1;

    /* Calculate chance of hit. Lower THAC0 is better for attacker. */
    calc_base_hit = compute_base_hit(ch, hitmod);

    if (weap != WEAR_WIELD1) {
      if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON) {
        break;
      }
    } 

    if (GET_POS(ch) != POS_FIGHTING) { // initial attack
      attacks = 1;
    } else {
	  if (GET_RACE(ch) == RACE_MINOTAUR ||
	      GET_RACE(ch) == RACE_CENTAUR ||
	      HAS_FEAT(ch, FEAT_CLAWS_AND_BITE)) {
                int calc_base_hit_racial;
		int racial_attack_type = TYPE_BITE;


	     if (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
        {
		  racial_attack_type = TYPE_BITE;
                  calc_base_hit_racial = compute_base_hit(ch, hitmod) - 5;
	          status = one_hit(ch, victim, NULL, racial_attack_type, calc_base_hit, NULL, NULL, ability_mod_value(GET_STR(ch)) / 2);
             } 
             if (GET_RACE(ch) == RACE_MINOTAUR) {
		  racial_attack_type = TYPE_GORE;
                  calc_base_hit_racial = compute_base_hit(ch, hitmod) - 5;
	          status = one_hit(ch, victim, NULL, racial_attack_type, calc_base_hit, NULL, NULL, ability_mod_value(GET_STR(ch)) / 2);
             } 
             if (GET_RACE(ch) == RACE_CENTAUR) {
		  racial_attack_type = TYPE_BATTER;
                  calc_base_hit_racial = compute_base_hit(ch, hitmod) - 5;
	          status = one_hit(ch, victim, NULL, racial_attack_type, calc_base_hit, NULL, NULL, ability_mod_value(GET_STR(ch)) / 2);
             } 
  	  }

      attacks = num_attacks(ch, weap != WEAR_WIELD1);
    }


    if (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS) && GET_CLASS_RANKS(ch, CLASS_MONK) && 
      ((GET_EQ(ch, WEAR_WIELD) && IS_MONK_WEAPON(GET_EQ(ch, WEAR_WIELD))) ||
        (!GET_EQ(ch, WEAR_HOLD) && !GET_EQ(ch, WEAR_HOLD)))) {
      if (GET_CLASS_RANKS(ch, CLASS_MONK) >= 15)
        fullextra = 3;
      else if (GET_CLASS_RANKS(ch, CLASS_MONK) > 10)
        fullextra = 2;
      else
        fullextra = 1;
      if (GET_EQ(ch, WEAR_WIELD2) == wielded && wielded)
        fullextra = 0;
    }

    if (wielded && GET_EQ(ch, WEAR_WIELD1) == wielded) {
      if (AFF_FLAGGED(ch, AFF_RAPID_SHOT) && wielded && IS_RANGED_WEAPON(wielded))
        fullextra = 1;
      if (AFF_FLAGGED(ch, AFF_RAPID_SHOT) && wielded && IS_RANGED_WEAPON(wielded) && HAS_FEAT(ch, FEAT_MANYSHOT))
        fullextra = 2;
    }

    

  if (wielded && IS_RANGED_WEAPON(wielded) && IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_REPEATING))
    fullextra++;

  if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    fullextra++;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED)))
    fullextra++;

  if (affected_by_spell(ch, SPELL_HASTE))
    fullextra += 1;

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 35)
    fullextra += 1;

    if (wielded) {
      hitbonus = 0;
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (wielded->affected[i].location == APPLY_ACCURACY)
          hitbonus += wielded->affected[i].modifier;
    } else {
      hitbonus = HAS_FEAT(ch, FEAT_KI_STRIKE);
    }

    sprintf(buf, ", %d attack%s, ", attacks + fullextra, (attacks + fullextra) == 1 ? "" : "s");

    
    tot_attacks = attacks + fullextra;


    if (!FIGHTING(ch)) {
    	if ((skill_roll(ch, SKILL_STEALTH) <= skill_roll(victim, SKILL_SPOT)) || 
    		  (skill_roll(ch, SKILL_STEALTH) <= skill_roll(victim, SKILL_LISTEN))) {
    		  tot_attacks = 1;
      }
    }


    int opp = FALSE;

    /*
     * if something (casting a spell, doing some other partial action) has
     * marked the attacker NEXTPARTIAL, they cannot do a full attack for
     * one combat round.
     */
    if (AFF_FLAGGED(ch, AFF_NEXTPARTIAL)) {
      attacks = 1;
      tot_attacks = 1;
      fullextra = 0;
      if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
        weapon_flurry = 1;
      else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED) ||
           has_weapon_feat(ch, FEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED)))
        weapon_flurry = 1;

      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    }


  if ((tot_attacks + fullextra) > 1) {
    ch->full_round_action = 1;
  }

  if (AFF_FLAGGED(ch, AFF_DOING_AOO)) {
    attacks = 1 ;
    tot_attacks = 1;
    fullextra = 0;
    GET_TOTAL_AOO(ch)++;
  }

  if (!opp) {
    notfirst = 0;
    do {
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) != AFF_FLAGGED(ch, AFF_ETHEREAL))
        victim = find_next_victim(ch);
      if (!victim)
        return;        
      if (victim && GET_FORM_POS(victim) > FORM_POS_FRONT && IS_NPC(ch) && AFF_FLAGGED(victim, AFF_GROUP) && (victim->master || 
	  (victim->followers && AFF_FLAGGED(victim->followers->follower, AFF_GROUP))))
        victim = find_next_victim(ch);

        status = one_hit(ch, victim, wielded, w_type, calc_base_hit, NULL, NULL, hitbonus);
        if (weapon_flurry > 0)
          one_hit(ch, victim, wielded, w_type, calc_base_hit, NULL, NULL, hitbonus - 5);
        sprintf(buf, "%s%+d", notfirst++ ? "/" : "", calc_base_hit);
        /* check if the victim has a hitprcnt trigger */
        if (status > -1)
          hitprcnt_mtrigger(victim);
        else if (status == -2) {
          act("$n deflects an incoming projectile, sending it veering off harmlessly.", false, ch, NULL, victim, TO_NOTVICT);
          act("You deflect an incoming projectile, sending it veering off harmlessly!", false, ch, NULL, victim, TO_CHAR);
          act("$n deflects your projectile sending it veering off harmlessly!", false, ch, NULL, victim, TO_VICT);
        }          
        else {
          victim = find_next_victim(ch);
          if (victim) {
            status = 0;
            if (HAS_FEAT(ch, FEAT_GREAT_CLEAVE) ||
                (!cleave++ && HAS_FEAT(ch, FEAT_CLEAVE))) {
              act("$n follows through and attacks $N!", false, ch, NULL, victim, TO_NOTVICT);
              act("You follow through and attack $N!", false, ch, NULL, victim, TO_CHAR);
              act("$n follows through and attacks YOU!", false, ch, NULL, victim, TO_VICT | TO_SLEEP);
              status = one_hit(ch, victim, wielded, w_type, calc_base_hit, NULL, NULL, hitbonus);
  
              /* check if the victim has a hitprcnt trigger */
              if (status > -1)
                hitprcnt_mtrigger(victim);
              else {
                victim = find_next_victim(ch);
                if (victim)
                  status = 0;
              }
            }
          } else {
            return;
          }
        }

      if (!fullextra) {
        calc_base_hit -= 5;
        attacks--;
        tot_attacks--;
        weapon_flurry--;
      } else {
        fullextra--;
        tot_attacks--;
        weapon_flurry--;
      }

    } while (status >= 0 && loc == IN_ROOM(victim) && tot_attacks > 0) ;
  } // if (!opp)
  } // for wield1; wield2;

  if (GET_FIGHT_NUMBER_OF_ATTACKS(ch) > 0) {
      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(bleedbuf, " @M(Bleeding @W%d@M)@n", GET_FIGHT_BLEEDING_DAMAGE(victim));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@rYou@y hit @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_SUMMON_TANK)) ? 
              FIGHTING(ch)->player_specials->summon_desc : 
              ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_MOUNT_TANK)) ? 
              FIGHTING(ch)->player_specials->mount_desc 
              : ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 &&
              PRF_FLAGGED(FIGHTING(ch), PRF_COMPANION_TANK)) ?
              FIGHTING(ch)->player_specials->companion_desc : "$N")),
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W$n@y hits @R%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_SUMMON_TANK)) ? 
              FIGHTING(ch)->player_specials->summon_desc : 
              ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_MOUNT_TANK)) ? 
              FIGHTING(ch)->player_specials->mount_desc  
              :  
              ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_COMPANION_TANK)) ? 
              FIGHTING(ch)->player_specials->companion_desc  :
              "You")),
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W$n@y hits @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s",  
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_SUMMON_TANK)) ? 
              FIGHTING(ch)->player_specials->summon_desc : 
              ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_MOUNT_TANK)) ? 
              FIGHTING(ch)->player_specials->mount_desc  : 
              ((FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && 
              PRF_FLAGGED(FIGHTING(ch), PRF_COMPANION_TANK)) ? 
              FIGHTING(ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_NOTVICT);

    if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && FIGHTING(ch)->player_specials->summon_cur_hit <= 0) {
      char sbuf[200];
      sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->summon_desc);
      act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
      fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
      free_summon(FIGHTING(ch));
    }

    if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && FIGHTING(ch)->player_specials->companion_cur_hit <= 0) {
      char sbuf[200];
      sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->companion_desc);
      act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
      fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
      free_companion(FIGHTING(ch));
    }

    if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && 
        FIGHTING(ch)->player_specials->mount_cur_hit <= 0) {
      char sbuf[200];
      sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->mount_desc);
      act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
      fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
      free_mount(FIGHTING(ch));
    }

  }

  } // !AFF_FLAGGED AFF_NEXTNOACTION

  if (AFF_FLAGGED(ch, AFF_NEXTNOACTION)) {  
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
  }

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD && !AFF_FLAGGED(victim, AFF_SPIRIT)) {
/* This will get set in raw_kill() */
#if 0
    SET_BIT_AR(AFF_FLAGS(victim), AFF_SPIRIT);
#endif
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
        }
      }

      if (!IS_NPC(victim)) {
        mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
        if (MOB_FLAGGED(ch, MOB_MEMORY))
   	forget(ch, victim);
      }
      /* Cant determine GET_GOLD on corpse, so do now and store */
      if (IS_NPC(victim)) {
        local_gold = GET_GOLD(victim);
        sprintf(local_buf,"%ld", (long)local_gold);
      }
      send_to_char(victim, "@w%s@n", death_message);

      die(victim, ch);

      struct char_data *k = NULL, *temp = NULL;
      struct follow_type *f = NULL;
      ubyte found = FALSE;

      for (k = world[IN_ROOM(ch)].people; k; k = temp) {
        temp = k->next_in_room;
        if (FIGHTING(k) == ch) {
          found = TRUE;
        }
      }
      if (!found) {
        stop_fighting(ch);
        GET_POS(ch) = POS_STANDING;
        update_pos(ch);
        for (f = ch->followers; f; f = f->next) {
          found = FALSE;
          for (k = world[IN_ROOM(f->follower)].people; k; k = temp) {
            temp = k->next_in_room;
            if (FIGHTING(k) == f->follower) {
              found = TRUE;
            }
          }
          if (!found) {
            stop_fighting(f->follower);
            GET_POS(f->follower) = POS_STANDING;
            update_pos(f->follower);
          }
        }
      }

    return;
    }
//  }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

}

struct fightsort_elem *fightsort_table = NULL;
int fightsort_table_size = 0;

void free_fightsort()
{
  if (fightsort_table)
    free(fightsort_table);
  fightsort_table_size = 0;
}

/* control the fights going on.  Called every round from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  struct follow_type *k;
  int i, j;
  ACMD(do_assist);

  i = 0;
  for (ch = combat_list; ch; ch = ch->next_fighting) {
    if (i >= fightsort_table_size) {
      if (fightsort_table) {
        RECREATE(fightsort_table, struct fightsort_elem, fightsort_table_size + 128);
        fightsort_table_size += 128;
      } else {
        CREATE(fightsort_table, struct fightsort_elem, 128);
        fightsort_table_size = 128;
      }
    }
    fightsort_table[i].ch = ch;
    fightsort_table[i].init = roll_initiative(ch);
    fightsort_table[i].dex = GET_DEX(ch);
    i++;
  }

  // sort into initiative order 
  qsort(fightsort_table, i, sizeof(struct fightsort_elem), fightsort_compare);

  // lowest initiative is at the bottom, and we're constructing combat_list
  // backwards 
  combat_list = NULL;
  for (j = 0; j < i; j++) {
    fightsort_table[j].ch->next_fighting = combat_list;
    combat_list = fightsort_table[j].ch;
  }
  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (IS_AFFECTED(ch, AFF_PARALYZE)) {
      send_to_char(ch, "Your muscles won't respond!\r\n");
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

    for (k = ch->followers; k; k=k->next) {
      /* should followers auto-assist master? */
      if (!IS_NPC(k->follower) && !FIGHTING(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
        (IN_ROOM(k->follower) == IN_ROOM(ch)))
        do_assist(k->follower, GET_NAME(ch), 0, 0);
    }

    /* should master auto-assist followers?  */
    if (ch->master && PRF_FLAGGED(ch->master, PRF_AUTOASSIST) && 
      FIGHTING(ch) && !FIGHTING(ch->master) && 
      (IN_ROOM(ch->master) == IN_ROOM(ch)) && !IS_NPC(ch->master)) 
      do_assist(ch->master, GET_NAME(ch), 0, 0); 

    if (FIGHTING(ch) && AFF_FLAGGED(FIGHTING(ch), AFF_SPIRIT)) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
    if (FIGHTING(ch) && AFF_FLAGGED(FIGHTING(ch), AFF_ETHEREAL)) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
    else if (can_use_available_actions(ch, ACTION_STANDARD)) {
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    }
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }
    ch->standard_action_spent = 0;
    ch->move_action_spent = 0;
    ch->parries = 0;
    ch->total_defense = 0;
    ch->weapon_supremacy_miss = 0;
    ch->opportunist = 0;
    GET_TOTAL_AOO(ch) = 0;
    ch->parried_attacks = 0;
    do_affect_tickdown(ch);


    GET_HIT(ch) += HAS_FEAT(ch, FEAT_FAST_HEALING) * 3;
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch));

    do_summon_attack(ch);
    do_companion_attack(ch);
    do_mount_attack(ch);

  }
}




void tickdown_pvp_timer(void)
{
  struct char_data *ch = NULL;

  for (ch = character_list; ch; ch = ch->next) {
  
    if (!ch->desc)
      continue;

    if (ch->pvp_timer > 0) {
      ch->pvp_timer--;
      if (ch->pvp_timer == 0)
        send_to_char(ch, "Your pvp timer has expired.  You may now turn off your pvp flag if you wish.\r\n");
    }
  }
}

void fight_action(struct char_data *ch)
{

  struct affected_type *af;
  struct affected_type af2[2];
  int n;
  int dam;
  char dammes[MAX_STRING_LENGTH];  
  int can_hit = TRUE;

  if (ch == NULL)
    return;

  ACMD(do_assist);

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    GET_DAMAGE_TAKEN(ch) = 0;
    GET_MOUNTED_ATTACKS_AVOIDED(ch) = 0;
    if (affected_by_spell(ch, SPELL_DEATH_ATTACK)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_DEATH_ATTACK) {
          af->duration -= 1;
          if (af->duration < 1)
            affect_from_char(ch, SPELL_DEATH_ATTACK);
          break;
        }
      }
      send_to_char(ch, "Your muscles won't respond!\r\n");
      can_hit = FALSE;
    }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK_ATTACK);
    if (!IS_NPC(ch)) {
      ch->player_specials->epic_dodge = FALSE;
    }

    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_TOTAL_AOO(ch) = 0;
    ch->spell_cast = FALSE;

    if (HAS_FEAT(ch, FEAT_FAST_HEALING)) {
      GET_HIT(ch) += HAS_FEAT(ch, FEAT_FAST_HEALING) * 3;
      if (GET_HIT(ch) > GET_MAX_HIT(ch))
        GET_HIT(ch) = GET_MAX_HIT(ch);
    }


    if (affected_by_spell(ch, SPELL_BESTOW_CURSE_DAZE)) {
      send_to_char(ch, "The curse you are under overcomes you, preventing you from being able to act.\r\n");
    }

    if (AFF_FLAGGED(ch, AFF_SNEAK_ATTACK))
    	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK_ATTACK);
    
    if (affected_by_spell(ch, SPELL_AFF_TAUNTED)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_AFF_TAUNTED)
          af->duration--;
        if (af->duration < 1)
          affect_from_char(ch, SPELL_AFF_TAUNTED);
      }
    }

    if (affected_by_spell(ch, SPELL_AFF_DISARMED)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_AFF_DISARMED)
          af->duration--;
        if (af->duration < 1)
          affect_from_char(ch, SPELL_AFF_DISARMED);
      }
    }


    if (FIGHTING(ch) && affected_by_spell(ch, SPELL_CALL_LIGHTNING)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_CALL_LIGHTNING) {
          cast_spell(ch, FIGHTING(ch), NULL, SPELL_CALL_LIGHTNING_BOLT, NULL);
          af->duration -= 10;
          if (af->duration < 1)
            affect_from_char(ch, SPELL_CALL_LIGHTNING);
          break;
        }
      }
    }
	  if (affected_by_spell(ch, SPELL_ACID_ARROW)) {
        for (af = ch->affected; af; af = af->next) {
          if (af->type == SPELL_ACID_ARROW) {
              dam = dice(2, 4);
              sprintf(dammes, "@yDeadly acid sears your flesh for @R%d@y points of damage!@n", dam);
              act(dammes, false, ch, 0, ch, TO_VICT);
              sprintf(dammes, "@yDeadly acid sears the flesh of $N for @R%d@y points of damage!@n", dam);
              act(dammes, false, ch, 0, ch, TO_ROOM);
              dam = damage(ch, ch, dam, SPELL_ACID_ARROW, 0, -1, 0, SPELL_ACID_ARROW, 1);
             af->duration--;
             if (af->duration < 1)
               affect_from_char(ch, SPELL_ACID_ARROW);
          }
        }
     }

     if (affected_by_spell(ch, SPELL_ON_FIRE)) {
        for (af = ch->affected; af; af = af->next) {
          if (af->type == SPELL_ON_FIRE) {
              dam = dice(1, 6);
              sprintf(dammes, "@ySearing fire burns you for @R%d@y points of damage!@n", dam);
              act(dammes, false, ch, 0, ch, TO_VICT);
              sprintf(dammes, "@ySearing fire burns $N for @R%d@y points of damage!@n", dam);
              act(dammes, false, ch, 0, ch, TO_ROOM);
              dam = damage(ch, ch, dam, SPELL_ON_FIRE, 0, -1, 0, SPELL_ON_FIRE, 1);
             af->duration--;
             if (af->duration < 1)
               affect_from_char(ch, SPELL_ON_FIRE);
          }
        }
     }

     if (affected_by_spell(ch, SPELL_FLAMING_SPHERE)) {
        for (af = ch->affected; af; af = af->next) {
          if (af->type == SPELL_FLAMING_SPHERE) {
              if (!mag_newsaves(SAVING_REFLEX, ch, ch, 0, af->modifier)) {
                dam = dice(3, 6);
                sprintf(dammes, "@yThe flaming sphere burns you for @R%d@y points of damage!@n", dam);
                act(dammes, false, ch, 0, ch, TO_VICT);
                sprintf(dammes, "@yA flaming sphere burns $N for @R%d@y points of damage!@n", dam);
                act(dammes, false, ch, 0, ch, TO_ROOM);
                dam = damage(ch, ch, dam, SPELL_FLAMING_SPHERE, 0, -1, 0, SPELL_FLAMING_SPHERE, 1);
              }
             af->duration--;
             if (af->duration < 1)
               affect_from_char(ch, SPELL_FLAMING_SPHERE);
          }
        }
     }
    
    if (affected_by_spell(ch, SPELL_AFF_RAGE) || affected_by_spell(ch, SPELL_AFF_DEFENSIVE_STANCE)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_AFF_RAGE || af->type == SPELL_AFF_DEFENSIVE_STANCE)
          af->duration--;
        if (af->duration < 1) {
          if (af->type == SPELL_AFF_RAGE)
            affect_from_char(ch, SPELL_AFF_RAGE);
          else 
            affect_from_char(ch, SPELL_AFF_DEFENSIVE_STANCE);

          
          if (!HAS_FEAT(ch, FEAT_TIRELESS_RAGE) && af->type == SPELL_AFF_RAGE) {          
          af2[0].type = SPELL_AFF_FATIGUED;
          af2[0].location = APPLY_STR;
          af2[0].bitvector = AFF_FATIGUED;
          af2[0].duration = 2;
          af2[0].modifier =   -2;                                     

          af2[1].type = SPELL_AFF_FATIGUED;
          af2[1].location = APPLY_DEX;
          af2[1].bitvector = AFF_FATIGUED;
          af2[1].duration = 2;
          af2[1].modifier =   -2;    
          
          for (n = 0; n < 2; n++)
            affect_join(ch, af2+n, true, false, false, false);               
          }
        }
      }
    }   
     
    if (affected_by_spell(ch, SPELL_AFF_STRENGTH_OF_HONOR)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_AFF_STRENGTH_OF_HONOR)
          af->duration--;
        if (af->duration < 1) {
          affect_from_char(ch, SPELL_AFF_STRENGTH_OF_HONOR);
        }
      }
    }

    if (affected_by_spell(ch, SPELL_INSPIRE_FEAR)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_INSPIRE_FEAR) {
          af->duration--; 
          if (af->duration < 1) {
            affect_from_char(ch, SPELL_INSPIRE_FEAR);
          }
          if (dice(1, 100) <= 50) {
	    act("$n quakes in fear and is unable to act.", TRUE,ch, 0, 0, TO_ROOM);
            act("You quake in fear and are unable to act.", FALSE, ch, 0, 0, TO_CHAR);    
            can_hit = FALSE;
	  }
        }
      }
    }   	

    if (affected_by_spell(ch, SPELL_INSPIRE_AWE)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_INSPIRE_AWE) {
          af->duration--;
          if (af->duration < 1) {
            affect_from_char(ch, SPELL_INSPIRE_AWE);
          }
  	  if (dice(1, 100) <= 50) {
	    act("$N is frozen in awe and is unable to act.", TRUE, 0, 0, ch, TO_NOTVICT);
            act("You are frozen in awe and are unable to act.", FALSE, ch, 0, 0, TO_CHAR);
            SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
            can_hit = FALSE;
  	  }
        }
      }
    }   				
    
    if (affected_by_spell(ch, SPELL_SOUND_BURST)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_SOUND_BURST)
          af->duration--;
        if (af->duration < 1) {
          affect_from_char(ch, SPELL_SOUND_BURST);
        }
      }
    }         
     
    if (affected_by_spell(ch, SPELL_AFF_TAUNTING)) {
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_AFF_TAUNTING)
          af->duration--;
        if (af->duration < 1) {
          affect_from_char(ch, SPELL_AFF_TAUNTING);
        }
      }
    }        

    if (AFF_FLAGGED(ch, AFF_FLAT_FOOTED_2))
    	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED_2);

    if (AFF_FLAGGED(ch, AFF_FLAT_FOOTED_1)) {
    	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED_1);
    	SET_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED_2);
    }

    if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING) {
      do_stand(ch, 0, 0, 0);
      GET_POS(ch) = POS_FIGHTING;
      SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    }

    if (GET_POS(ch) <= POS_SLEEPING) {
      act("You are unconscious and unable to act!", true, ch, 0, 0, TO_CHAR | TO_SLEEP);
      act("$n is unconscious and unable to act!", true, ch, 0, 0, TO_ROOM);
      can_hit = FALSE;
    }
/*
    for (k = ch->followers; k; k=k->next) {
      if ((IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) && !FIGHTING(k->follower) && IN_ROOM(k->follower) == IN_ROOM(ch) &&
         AFF_FLAGGED(k->follower, AFF_GROUP)) || 
         (!IS_NPC(k->follower) && !FIGHTING(k->follower) && 
        (!IS_NPC(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST)) &&
        (IN_ROOM(k->follower) == IN_ROOM(ch))))
    }

    if (ch->master && PRF_FLAGGED(ch->master, PRF_AUTOASSIST) && 
      FIGHTING(ch) && (!IS_NPC(ch) || (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))) && !FIGHTING(ch->master) && 
      (IN_ROOM(ch->master) == IN_ROOM(ch))) 
      do_assist(ch->master, GET_NAME(ch), 0, 0); 
*/
/*
    if (ch && FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && 
       GUARDED_BY(FIGHTING(ch)) && IN_ROOM(FIGHTING(ch)) == IN_ROOM(GUARDED_BY(FIGHTING(ch))) &&
      skill_roll(GUARDED_BY(FIGHTING(ch)), SKILL_COMBAT_TACTICS) >= 10 + GET_LEVEL(ch))
      FIGHTING(ch) = GUARDED_BY(FIGHTING(ch));
*/
    if (affected_by_spell(ch, SPELL_HOLD_PERSON)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_HOLD_PERSON)
          af->duration--;
        if (af->duration < 1) {
          affect_from_char(ch, SPELL_HOLD_PERSON);
        }
      }
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }   

    if (affected_by_spell(ch, SPELL_HOLD_MONSTER)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_HOLD_MONSTER)
          af->duration--;
        if (af->duration < 1) {
          affect_from_char(ch, SPELL_HOLD_MONSTER);
        }
      }
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }       

    if (affected_by_spell(ch, SPELL_DEATH_ATTACK)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_DEATH_ATTACK) {
          af->duration -= 1;
          if (af->duration < 1)
            affect_from_char(ch, SPELL_DEATH_ATTACK);
          break;
        }
      }
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }

    if (affected_by_spell(ch, SPELL_TOUCH_OF_UNDEATH)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == SPELL_TOUCH_OF_UNDEATH) {
          af->duration -= 1;
          if (af->duration < 1)
            affect_from_char(ch, SPELL_TOUCH_OF_UNDEATH);
          break;
        }
      }
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }

    if (affected_by_spell(ch, ART_STUNNING_FIST)) {
      can_hit = FALSE;
      for (af = ch->affected; af; af = af->next) {
        if (af->type == ART_STUNNING_FIST) {
          af->duration -= 1;
          if (af->duration < 1)
            affect_from_char(ch, ART_STUNNING_FIST);
          break;
        }
      }
        send_to_char(ch, "Your muscles won't respond!\r\n");
    }

    if (IS_AFFECTED(ch, AFF_PARALYZE)) {
      can_hit = FALSE;
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }
    
    if (affected_by_spell(ch, SPELL_AFF_STUNNED) && !affected_by_spell(ch, ART_STUNNING_FIST)) {
      can_hit = FALSE;
      affect_from_char(ch, SPELL_AFF_STUNNED);
      send_to_char(ch, "Your muscles won't respond!\r\n");
    }

  if (!can_hit) {
    ch->active_turn = 0;
    if (!IS_NPC(ch)) {
      struct pause_event *pause;
      CREATE(pause, struct pause_event, 1);
      pause->ch = ch;
      ch->paralyzed = 1;
      send_to_char(ch, "@MYou are unable to act.  Combat will resume in 3 seconds.  You may attempt to remove paralysis if you can.@n\r\n");
      ch->pause_combat = event_create(pause_combat, pause, 3000);
    } else {
      find_next_combat_target(ch);
    }
  } else {
    ch->paralyzed = 0;
  }

}

void find_victim(struct char_data *ch)
{  
  struct char_data *k, *m;
  struct char_data *target;
  struct follow_type *f;     
         
  k = (FIGHTING(ch));
    
  target = k;

  if (k->master)
    m = k->master;
  else
    m = k;

  // find the number of members in the group 
  for (f = m->followers; f; f = f->next)
    if (GET_HATE(f->follower) > GET_HATE(target))
    	target = f->follower;  

  if (IS_NPC(ch))
    FIGHTING(ch) = target;

  log("SYSERR: find_victim reached end of loop without finding a victim");
  return;  
}

int trip_roll(struct char_data *ch, int defending) {

  int roll = 0;

  roll += rand_number(1, 20);

  roll += defending ? ability_mod_value(MAX(GET_STR(ch), GET_DEX(ch))) : ability_mod_value(GET_STR(ch));

  roll += 4 * (get_size(ch) - SIZE_MEDIUM);

  if (defending) {
/*
    switch(GET_RACE(ch)) {

    case RACE_ANIMAL:
    case RACE_DRAGON:
	case RACE_PLANT:
	case RACE_MAGICAL_BEAST:
      roll += 4;
      break;
	case RACE_VERMIN:
	case RACE_CONSTRUCT:
	  roll += 8;
	case RACE_OOZE:
	  roll += 99;	  
    default:
      break;
    }
*/	
	if (IS_DWARF(ch))
	  roll += 2;

  }

  return roll;
}
int get_touch_ac(struct char_data *ch) {

  int ac;

  ac = 100;

  if (ch->total_defense)
    ac += 40;

  ac += ability_mod_value(GET_DEX(ch)) * 10;

  if (HAS_FEAT(ch, FEAT_DODGE))
    ac += 10;

  if (FIGHTING(ch) && ch->smiting && FIGHTING(ch) == ch->smiting)
    ac += ability_mod_value(GET_CHA(ch)) * 10;


  int i = 0, j = 0;
  int dodge_ac = 0, def_ac = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      for (j = 0; j < 6; j++) {
        if (GET_EQ(ch, i)->affected[j].location == APPLY_AC_DODGE) {
          dodge_ac = MAX(dodge_ac, GET_EQ(ch, i)->affected[j].modifier);
        }
        if (GET_EQ(ch, i)->affected[j].location == APPLY_AC_DEFLECTION) {
          dodge_ac = MAX(def_ac, GET_EQ(ch, i)->affected[j].modifier);
        }
      }
    }
  }

  struct affected_type *af;

  for (af = ch->affected; af; af = af->next) {
    if (af->location == APPLY_AC_DODGE) {
      dodge_ac = MAX(dodge_ac, af->modifier);
    }
    if (af->location == APPLY_AC_DEFLECTION) {
      dodge_ac = MAX(def_ac, af->modifier);
    }
  }

  ac += (dodge_ac + def_ac);

  ac += (SIZE_MEDIUM - get_size(ch)) * 10;

  if (affected_by_spell(ch, SPELL_PRAYER))
    ac += 10;

  if (ch->mentor_level > 0) {
    ac = ac * ch->mentor_level / GET_CLASS_LEVEL(ch);
  }

  return ac;

}

int roll_initiative(struct char_data *ch) 
{
    ch->initiative = rand_number(1, 20) + dex_mod_capped(ch) + 4 * HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE);
    ch->initiative += 2 * HAS_FEAT(ch, FEAT_IMPROVED_REACTION);
    ch->initiative += HAS_FEAT(ch, FEAT_HEROIC_INITIATIVE);

  return ch->initiative;
}

int get_combat_pos_by_sector(struct char_data *ch)
{

  if (!ch || !IN_ROOM(ch))
    return 100;

  int sun = 100;
  int sky = 100;
  int sect = 100;
  int distance = 100;

  switch (weather_info.sunlight) {
    case SUN_RISE:
    case SUN_SET:
      sun = 50;
      break;
    case SUN_DARK:
      sun = 0;
      break;
  }

  if (!OUTSIDE(ch))
    sun = 100;

  switch(weather_info.sunlight) {
    case SKY_CLOUDY:
      sky = 75;
      break;
    case SKY_LIGHTNING:
      sky = 60;
      break;
    case SKY_RAINING:
      sky = 40;
      break;
  }

  if (!OUTSIDE(ch))
    sky = 100;

  switch (SECT(IN_ROOM(ch))) {
    case SECT_INSIDE:
    case SECT_CITY:
      sect = dice(1, 10) * 10;
      break;

    case SECT_MOUNTAIN:
      sect = dice(4, 10) * 10;
      break;

    case SECT_FOREST: 
      sect = dice(3, 6) * 10;
      break;

    case SECT_CAVE:
      sect = 60;
      break;
    case SECT_FIELD:
    case SECT_ROAD:
    case SECT_FLYING:
      sect = dice(6, 6) * 40;
      break;

    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_UNDERWATER:
      sect = dice(4, 8) * 10;
      break;

    case SECT_HILLS:
      sect = dice(2, 10) * 10;
      break;

    default:
      sect = 300;
      break;    
  }

  distance = MAX(0, sect * sky * sun / 10000);

  if (sun == 0) {
    if (has_daylight(ch))
      distance = MIN(sect, 60);
    else if (has_light(ch))
      distance = MIN(sect, 30);      
  }
  if (has_light(ch) && HAS_LOW_LIGHT_VIS(ch))
    distance *= 2;
  if (HAS_DARKVISION(ch))
    distance = MAX(distance, MIN(sect, 60));

  return distance;
}

void do_attack_of_opportunity(struct char_data *ch, struct char_data *victim, char *type)
{

  if (victim) {
    if (victim->dead)
      return;
    if (GET_HIT(victim) < -10) {
      die(victim, ch);
      return;
    }
    if (GET_POS(victim) == POS_DEAD) {
      die(victim, ch);
      return;
    }
  }
  else {
    return;
  }

  GET_TOTAL_AOO(ch)++;


    int vict_hitmod = 0;
    struct obj_data *vict_wield = GET_EQ(ch, WEAR_WIELD);
    int i = 0;
    int w_type = TYPE_HIT;
    if (vict_wield) {
      w_type = GET_OBJ_VAL(vict_wield, VAL_WEAPON_DAMTYPE) + TYPE_HIT;

      for (i = 0; i < 6; i++)
        if (vict_wield->affected[i].location == APPLY_ACCURACY) {
          vict_hitmod = vict_wield->affected[i].modifier;
        }
    }

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    if (HAS_FEAT(ch, FEAT_SNEAK_ATTACK_OF_OPPORTUNITY))
      ch->sneak_opp = TRUE;
    one_hit(ch, victim, vict_wield, w_type, compute_base_hit(ch, vict_hitmod), NULL, NULL, vict_hitmod);
    ch->sneak_opp = FALSE;

    char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200], bleedbuf[100];	
    char parrybuf[100];

    if (GET_FIGHT_NUMBER_OF_ATTACKS(ch) > 0) {
      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(bleedbuf, " @M(Bleeding @W%d@M)@n", GET_FIGHT_BLEEDING_DAMAGE(victim));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@rYou@y %s @W%s@y with your @Yattack of opportunity@y for @R%d@y damage.%s%s%s%s%s%s%s%s@M(%s)@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hit" : "miss", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "", 
              type);
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W$n@y %s @R%s@n with $s @Yattack of opportunity@y for @R%d@y damage.%s%s%s%s%s%s%s%s@M(%s)@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "You")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "", 
              type);
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W$n@y %s @W%s@y with $s @Yattack of opportunity@y for @R%d@y damage.%s%s%s%s%s%s%s%s@M(%s)@n",  
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "", 
              type);
      fight_output(fbuf, ch, victim, TO_NOTVICT);

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && FIGHTING(ch)->player_specials->summon_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->summon_desc);
        act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_summon(FIGHTING(ch));
      }

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && FIGHTING(ch)->player_specials->companion_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->companion_desc);
        act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_companion(FIGHTING(ch));
     }

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && FIGHTING(ch)->player_specials->mount_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->mount_desc);
        act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_mount(FIGHTING(ch));
      }

    }

  int local_gold = 0;
  char local_buf[200];


  if (victim && victim->dead)
    return;

  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
}

void sort_initiative(void)
{
  struct char_data *current, *cur, *maximum, *first;
  struct char_data *current_previous, *current_next, *max_previous, *max_next;

  for(current = character_list; current->next != NULL; current = current->next) {
     maximum = current;
    for(cur = current ; cur != NULL; cur = cur->next)
    {
      if(maximum->initiative < cur->initiative) 
      {
        maximum = cur;
      }
    }
    if(maximum != current)
    {

      // Initialize them
      current_next = current->next;
      max_previous = maximum->prev;
      max_next = maximum->next;
      current_previous = current->prev;

      if(current_previous == NULL)
      {
        // Change the First Node
        first = maximum;
      }
      if(current->next == maximum)
      {
        // Nodes are Adjacent
        maximum->prev = current_previous;
        maximum->next = current;
 
        current->prev = maximum;
        current->next = max_next;

        if(max_next)
        {
          max_next->prev = current;
        }
        if(current_previous)
        {
          current_previous->next = maximum;
        }
      }
      else
      {
        maximum->prev = current_previous;
        maximum->next = current_next;
 
        current->prev = max_previous;
        current->next = max_next;

        if(current_next)
        {
          current_next->prev = maximum;
        }
        if(max_previous)
        {
          max_previous->next = current;
        }
        if(max_next)
        {
          max_next->prev = current;
        }
        if(current_previous)
        {
          current_previous->next = maximum;
        }
      }
      current = maximum;
    }
  }
}

// 0 = MOB
// 1 = summon
// 2 = mount
// 3 = companion

void do_mob_special_attacks(struct char_data *ch, int type)
{
  struct char_data *victim = FIGHTING(ch);
  struct affected_type af;
  char buf[400];

    if (victim && ((GET_RACE(ch) == RACE_GHOUL || GET_RACE(ch) == RACE_GHAST || GET_RACE(ch) == RACE_MUMMY || GET_RACE(ch) == RACE_MOHRG) || 
        (type == MOB_TYPE_SUMMON && (ch->player_specials->summon_num == PET_GHOUL || ch->player_specials->summon_num == PET_MOHRG ||
        ch->player_specials->summon_num == PET_GHAST || ch->player_specials->summon_num == PET_MUMMY )))) {
      if (!mag_newsaves(SAVING_FORTITUDE, ch, victim, SPELL_PARALYZE, ((type == MOB_TYPE_SUMMON && ch->player_specials->summon_num == PET_MUMMY)
          || GET_RACE(ch) == RACE_MUMMY) ? 16 : (((type == MOB_TYPE_SUMMON && ch->player_specials->summon_num == PET_MOHRG) || 
          GET_RACE(ch) == RACE_MOHRG) ? 17: 12)) &&
          !affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT) && !IS_ELF(victim)) {      
	af.type = SPELL_PARALYZE;
        af.location = APPLY_FEAT;
        af.modifier = 1;
        af.duration = 
                      ((type == MOB_TYPE_SUMMON && ch->player_specials->summon_num == PET_MOHRG) || GET_RACE(ch) == RACE_MOHRG) ? dice(1, 4) * 10 : 
                      dice(1, 4) + 1;
        af.bitvector = AFF_PARALYZE;

        affect_join(victim, &af, false, false, true, false);

        sprintf(buf, "You freeze in your tracks as %s's hit paralyzes you.", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_CHAR);
        sprintf(buf, "$n freezes in $s tracks as %s's hit paralyzes $m.", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_ROOM);
      }
    }

    if (victim && (GET_RACE(ch) == RACE_GHAST || (type == MOB_TYPE_SUMMON && (ch->player_specials->summon_num == PET_GHAST)))) {
      if (!mag_newsaves(SAVING_FORTITUDE, ch, victim, SPELL_SICKENED, 12) &&
          !affected_by_spell(victim, SPELL_RESIST_SICKENED)) {      
	af.type = SPELL_SICKENED;
        af.location = APPLY_FEAT;
        af.modifier = 1;
        af.duration = (dice(1, 6) + 4) * 10;
        af.bitvector = AFF_NONE;

        affect_join(victim, &af, false, false, true, false);

        sprintf(buf, "You double over in nausea as %s's hit sickens you.", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_CHAR);
        sprintf(buf, "$n doubles of in nausea as %s's hit sickens $m.", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_ROOM);
      }
      else if (!affected_by_spell(victim, SPELL_RESIST_SICKENED)) {
	af.type = SPELL_RESIST_SICKENED;
        af.location = APPLY_FEAT;
        af.modifier = 1;
        af.duration = (dice(1, 6) + 4) * 10;
        af.bitvector = AFF_NONE;

        affect_join(victim, &af, false, false, true, false);
      }
    }

    if (victim && (GET_RACE(ch) == RACE_MUMMY || (type == MOB_TYPE_SUMMON && (ch->player_specials->summon_num == PET_MUMMY)))) {
      if (!mag_newsaves(SAVING_FORTITUDE, ch, victim, SPELL_FEAR, 12) &&
          !affected_by_spell(victim, SPELL_RESIST_FEAR)) {      
	af.type = SPELL_FEAR;
        af.location = APPLY_FEAT;
        af.modifier = 1;
        af.duration = dice(1, 4);
        af.bitvector = AFF_NONE;

        affect_join(victim, &af, false, false, true, false);

        sprintf(buf, "You freeze in your tracks as %s paralyzes you with fear .", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_CHAR);
        sprintf(buf, "$n freezes in $s tracks as %s paralyzes $m with fear.", (type == MOB_TYPE_SUMMON) ? 
            ch->player_specials->summon_desc : (has_intro(victim, ch) ? GET_NAME(ch) : which_desc(ch)));
        act(buf, FALSE, victim, 0, ch, TO_ROOM);
      }
      else if (!affected_by_spell(victim, SPELL_RESIST_FEAR)) {
	af.type = SPELL_RESIST_FEAR;
        af.location = APPLY_FEAT;
        af.modifier = 1;
        af.duration = dice(1, 4);
        af.bitvector = AFF_NONE;

        affect_join(victim, &af, false, false, true, false);
      }
    }

}

void do_swarm_of_arrows(struct char_data *ch, struct char_data *victim)
{

  if (victim) {
    if (GET_HIT(victim) < -10) {
      die(victim, ch);
      return;
    }
    if (GET_POS(victim) == POS_DEAD) {
      die(victim, ch);
      return;
    }
  }
  else {
    return;
  }


    int vict_hitmod = 0;
    struct obj_data *vict_wield = GET_EQ(ch, WEAR_WIELD);
    int i = 0;
    int w_type = TYPE_HIT;
    if (vict_wield) {
      w_type = GET_OBJ_VAL(vict_wield, VAL_WEAPON_DAMTYPE) + TYPE_HIT;

      for (i = 0; i < 6; i++)
        if (vict_wield->affected[i].location == APPLY_ACCURACY) {
          vict_hitmod = vict_wield->affected[i].modifier;
        }
    }

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    one_hit(ch, victim, vict_wield, w_type, compute_base_hit(ch, vict_hitmod), NULL, NULL, vict_hitmod);

    char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200], bleedbuf[100];	
    char parrybuf[100];

    if (GET_FIGHT_NUMBER_OF_ATTACKS(ch) > 0) {
      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(bleedbuf, " @M(Bleeding @W%d@M)@n", GET_FIGHT_BLEEDING_DAMAGE(victim));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@rYou@y %s @W%s@y with your @Ywhirlwind@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hit" : "miss", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : ""
              );
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W$n@y %s @R%s@n with $s @Ywhirlwind attack@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "You")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : ""
              );
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W$n@y %s @W%s@y with $s @Ywhirlwind attack@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n",  
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : ""
              );
      fight_output(fbuf, ch, victim, TO_NOTVICT);

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && FIGHTING(ch)->player_specials->summon_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->summon_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_summon(FIGHTING(ch));
      }

     if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && FIGHTING(ch)->player_specials->companion_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->companion_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_companion(FIGHTING(ch));
      }

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && FIGHTING(ch)->player_specials->mount_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->mount_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_mount(FIGHTING(ch));
      }

    }

  int local_gold = 0;
  char local_buf[200];


  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
}

void do_whirlwind_attack(struct char_data *ch, struct char_data *victim)
{

  if (victim) {
    if (GET_HIT(victim) < -10) {
      die(victim, ch);
      return;
    }
    if (GET_POS(victim) == POS_DEAD) {
      die(victim, ch);
      return;
    }
  }
  else {
    return;
  }


    int vict_hitmod = 0;
    struct obj_data *vict_wield = GET_EQ(ch, WEAR_WIELD);
    int i = 0;
    int w_type = TYPE_HIT;
    if (vict_wield) {
      w_type = GET_OBJ_VAL(vict_wield, VAL_WEAPON_DAMTYPE) + TYPE_HIT;

      for (i = 0; i < 6; i++)
        if (vict_wield->affected[i].location == APPLY_ACCURACY) {
          vict_hitmod = vict_wield->affected[i].modifier;
        }
    }

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    one_hit(ch, victim, vict_wield, w_type, compute_base_hit(ch, vict_hitmod), NULL, NULL, vict_hitmod);

    char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200], bleedbuf[100];	
    char parrybuf[100];

    if (GET_FIGHT_NUMBER_OF_ATTACKS(ch) > 0) {
      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(bleedbuf, " @M(Bleeding @W%d@M)@n", GET_FIGHT_BLEEDING_DAMAGE(victim));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@rYou@y %s @W%s@y with your @Ywhirlwind@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hit" : "miss", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              (ch)->parried_attacks > 0 ? parrybuf : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : ""
              );
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W$n@y %s @R%s@n with $s @Ywhirlwind attack@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n", 
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "You")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : ""
              );
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W$n@y %s @W%s@y with $s @Ywhirlwind attack@y for @R%d@y damage.%s%s%s%s%s%s%s%s@n",  
              GET_FIGHT_NUMBER_OF_HITS(ch) ? "hits" : "misses", 

              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0 && PRF_FLAGGED(ch, PRF_SUMMON_TANK)) ? 
              (ch)->player_specials->summon_desc : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0 && PRF_FLAGGED(ch, PRF_MOUNT_TANK)) ? 
              (ch)->player_specials->mount_desc  : 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0 && PRF_FLAGGED(ch, PRF_COMPANION_TANK)) ? 
              (ch)->player_specials->companion_desc  
              : "$N")),
              GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : ""
              );
      fight_output(fbuf, ch, victim, TO_NOTVICT);

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0 && FIGHTING(ch)->player_specials->summon_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->summon_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_summon(FIGHTING(ch));
      }

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0 && FIGHTING(ch)->player_specials->companion_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->companion_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_companion(FIGHTING(ch));
      }

      if (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->mount_num > 0 && FIGHTING(ch)->player_specials->mount_cur_hit <= 0) {
        char sbuf[200];
        sprintf(sbuf, "%s suffers a lethal blow and falls dead!", FIGHTING(ch)->player_specials->mount_desc);
         act(sbuf, true, FIGHTING(ch), 0, 0, TO_CHAR);
        fight_output(sbuf, FIGHTING(ch), 0, TO_ROOM);
        free_mount(FIGHTING(ch));
      }

    }

  int local_gold = 0;
  char local_buf[200];

  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
}

void do_summon_attack(struct char_data *ch)
{

  char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200];
  char parrybuf[100];

  int rnum = 0;

  if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)))
    rnum = GET_ID(FIGHTING(ch));

  struct char_data *victim = FIGHTING(ch);

  if (!victim)
    return;

  if (!IS_NPC(ch) && ch->player_specials->summon_num > 0 && FIGHTING(ch) && GET_POS(FIGHTING(ch)) > POS_DEAD && GET_HIT(FIGHTING(ch)) >= -10 &&
      (rnum == 0 || rnum != ch->last_kill_rnum)) {

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    int si = 0, sj = 0;
    int satt = 0, sattmod = 0;
    int sac;
    sbyte scrit = FALSE;

    while (ch->player_specials->summon_attack_ndice[si] > 0 && FIGHTING(ch)) {

      GET_FIGHT_NUMBER_OF_ATTACKS(ch)++;
      scrit = FALSE;

      satt = dice(1, 200);
      if (satt >= 196) {
        scrit = TRUE;
        GET_FIGHT_CRITICAL_HIT(ch)++;
      }

      sattmod = compute_summon_base_hit(ch);
      sattmod += ch->player_specials->summon_attack_to_hit[si];
      sattmod += (HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING)) ? 2 : 0;      
      sattmod *= 10;
      satt += sattmod;
      sac = compute_armor_class(FIGHTING(ch), ch);
//      send_to_char(ch, "Summon Attack %d.  Attack Roll: %d vs Armor Class %d\r\n", si+1, satt, sac);
      int ndice = 0, sdice = 0, dammod = 0, damdone = 0;
      if (scrit || satt > sac) {
        GET_FIGHT_NUMBER_OF_HITS(ch)++;
        for (sj = 0; sj < (1 + scrit); sj++) {

          if (!FIGHTING(ch))
            continue;

          ndice = ch->player_specials->summon_attack_ndice[si];
          sdice = ch->player_specials->summon_attack_sdice[si];
          dammod = ch->player_specials->summon_attack_dammod[si];
          damdone = dice(ndice, sdice) + dammod + ((HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING)) ? 2 : 0);
          damage(ch, FIGHTING(ch), damdone, TYPE_HIT, 0, -1, 0, TYPE_HIT, 1);
          do_mob_special_attacks(ch, MOB_TYPE_SUMMON);
        }
      }

      si++;
      if (!FIGHTING(ch) || GET_POS(FIGHTING(ch)) == POS_DEAD || GET_HIT(FIGHTING(ch)) < -10)
        break;
    }

      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@r%s@y hit @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0) ? 
              (ch)->player_specials->summon_desc 
              : "You",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W%s@y hits @R%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0) ? 
              (ch)->player_specials->summon_desc 
              : "$n",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "You",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W%s@y hits @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s",  
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->summon_num > 0) ? 
              (ch)->player_specials->summon_desc 
              : "$n",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_NOTVICT);
    
  }  
  int local_gold = 0;
  char local_buf[200];


  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

  ch->player_specials->summon_timer--;
  if (ch->player_specials->summon_num > 0 && ch->player_specials->summon_timer <= 0 && ch) {
    char sbuf[200];
    sprintf(sbuf, "Your pet, %s disappears in a flash of light as the conjuration spell expires!", ch->player_specials->summon_desc);
    act(sbuf, true, ch, 0, 0, TO_CHAR);
    sprintf(sbuf, "$n's pet, %s disappears in a flash of light as the conjuration spell expires!",ch->player_specials->summon_desc);
    act(sbuf, true, ch, 0, 0, TO_ROOM);
    free_summon(ch);
  }
}

void do_mount_attack(struct char_data *ch)
{
  char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200];
  char parrybuf[100];

  int rnum = 0;

  if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)))
    rnum = GET_ID(FIGHTING(ch));

  struct char_data *victim = FIGHTING(ch);

  if (!victim)
    return;

  if (!IS_NPC(ch) && ch->player_specials->mount_num > 0 && FIGHTING(ch) && GET_POS(FIGHTING(ch)) > POS_DEAD && GET_HIT(FIGHTING(ch)) >= -10 &&
      (rnum == 0 || rnum != ch->last_kill_rnum)) {

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    int si = 0, sj = 0;
    int satt = 0, sattmod = 0;
    int sac = 0;
    sbyte scrit = FALSE;

    while (ch->player_specials->mount_attack_ndice[si] > 0 && FIGHTING(ch)) {

      GET_FIGHT_NUMBER_OF_ATTACKS(ch)++;
      scrit = FALSE;

      satt = dice(1, 200);
      if (satt >= 196) {
        scrit = TRUE;
        GET_FIGHT_CRITICAL_HIT(ch)++;
      }

      sattmod = compute_mount_base_hit(ch);
      sattmod += ch->player_specials->mount_attack_to_hit[si];
      sattmod += GET_CLASS_RANKS(ch, CLASS_PALADIN) > 4 ? ((GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2) / 3)  : 0;
      if (IS_MOUNT_DRAGON(ch))
        sattmod += HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST);
      sattmod *= 10;
      satt += sattmod;
      sac = compute_armor_class(FIGHTING(ch), ch);
//      send_to_char(ch, "Mount Attack %d.  Attack Roll: %d vs Armor Class %d\r\n", si+1, satt, sac);
      int ndice = 0, sdice = 0, dammod = 0, damdone = 0;
      if (scrit || satt > sac) {
        GET_FIGHT_NUMBER_OF_HITS(ch)++;
        for (sj = 0; sj < (1 + scrit); sj++) {

          if (!FIGHTING(ch))
            continue;

          ndice = ch->player_specials->mount_attack_ndice[si];
          sdice = ch->player_specials->mount_attack_sdice[si];
          dammod = ch->player_specials->mount_attack_dammod[si];
          dammod += GET_CLASS_RANKS(ch, CLASS_PALADIN) > 4 ? ((GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2) / 3)  : 0;
          if (IS_MOUNT_DRAGON(ch))
            dammod += HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST);
          damdone = dice(ndice, sdice) + dammod;
          damage(ch, FIGHTING(ch), damdone, TYPE_HIT, 0, -1, 0, TYPE_HIT, 1);
        }
      }

      si++;
      if (!FIGHTING(ch) || GET_POS(FIGHTING(ch)) == POS_DEAD || GET_HIT(FIGHTING(ch)) < -10)
        break;
    }

      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@r%s@y hit @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0) ? 
              (ch)->player_specials->mount_desc  
              : "You"),
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W%s@y hits @R%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0) ? 
              (ch)->player_specials->mount_desc  
              : "$n"),
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "You",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W%s@y hits @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s",  
              (((ch) && !IS_NPC((ch)) && (ch)->player_specials->mount_num > 0) ? 
              (ch)->player_specials->mount_desc  
              : "$n"),
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->summon_num > 0) ? 
              FIGHTING(ch)->player_specials->summon_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_NOTVICT);
    
  }  
  int local_gold = 0;
  char local_buf[200];


  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

}

void do_companion_attack(struct char_data *ch)
{

  char critbuf[100], sneakbuf[100], precisebuf[100], reductbuf[100], fbuf[200];
  char parrybuf[100];

  int rnum = 0;

  if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)))
    rnum = GET_ID(FIGHTING(ch));

  struct char_data *victim = FIGHTING(ch);

  if (!victim)
    return;

  if (!IS_NPC(ch) && ch->player_specials->companion_num > 0 && FIGHTING(ch) && GET_POS(FIGHTING(ch)) > POS_DEAD && GET_HIT(FIGHTING(ch)) >= -10 &&
      (rnum == 0 || rnum != ch->last_kill_rnum)) {

    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

    int si = 0, sj = 0;
    int satt = 0, sattmod = 0;
    int sac;
    sbyte scrit = FALSE;

    while (ch->player_specials->companion_attack_ndice[si] > 0 && FIGHTING(ch)) {

      GET_FIGHT_NUMBER_OF_ATTACKS(ch)++;
      scrit = FALSE;

      satt = dice(1, 200);
      if (satt >= 196) {
        scrit = TRUE;
        GET_FIGHT_CRITICAL_HIT(ch)++;
      }

      sattmod = compute_companion_base_hit(ch);
      sattmod += ch->player_specials->companion_attack_to_hit[si];
      sattmod *= 10;
      satt += sattmod;
      sac = compute_armor_class(FIGHTING(ch), ch);
//      send_to_char(ch, "Companion Attack %d.  Attack Roll: %d vs Armor Class %d\r\n", si+1, satt, sac);
      int ndice = 0, sdice = 0, dammod = 0, damdone = 0;
      if (scrit || satt > sac) {
        GET_FIGHT_NUMBER_OF_HITS(ch)++;
        for (sj = 0; sj < (1 + scrit); sj++) {

          if (!FIGHTING(ch))
            continue;

          ndice = ch->player_specials->companion_attack_ndice[si];
          sdice = ch->player_specials->companion_attack_sdice[si];
          dammod = ch->player_specials->companion_attack_dammod[si];
          damdone = dice(ndice, sdice) + dammod;
          damage(ch, FIGHTING(ch), damdone, TYPE_HIT, 0, -1, 0, TYPE_HIT, 1);
          do_mob_special_attacks(ch, MOB_TYPE_COMPANION);
        }
      }

      si++;
      if (!FIGHTING(ch) || GET_POS(FIGHTING(ch)) == POS_DEAD || GET_HIT(FIGHTING(ch)) < -10)
        break;
    }

      sprintf(critbuf, " @M(Critical Hit @Wx%d@M)@n", GET_FIGHT_CRITICAL_HIT(ch));
      sprintf(sneakbuf, " @M(Sneak Attack @Wx%d@M)@n", GET_FIGHT_SNEAK_ATTACK(ch));
      sprintf(precisebuf, " @M(Precise Attack @Wx%d@M)@n", GET_FIGHT_PRECISE_ATTACK(ch));
      sprintf(reductbuf, " @M(Damage This Round Reduced by @W%d@M)@n", GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch));
      sprintf(parrybuf, " @M(@W%d@M Attacks Parried)@n", ch->parried_attacks);

      sprintf(fbuf, "@r%s@y hit @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0) ? 
              (ch)->player_specials->companion_desc 
              : "You",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0) ? 
              FIGHTING(ch)->player_specials->companion_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", 
              GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_CHAR);

      sprintf(fbuf, "@W%s@y hits @R%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s", 
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0) ? 
              (ch)->player_specials->companion_desc 
              : "$n",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0) ? 
              FIGHTING(ch)->player_specials->companion_desc : "You",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_VICT);

      sprintf(fbuf, "@W%s@y hits @W%s@y with @Y%d@y of @Y%d@y attacks for @R%d@y damage.%s%s%s%s%s%s%s%s",  
              ((ch) && !IS_NPC((ch)) && (ch)->player_specials->companion_num > 0) ? 
              (ch)->player_specials->companion_desc 
              : "$n",
              (FIGHTING(ch) && !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->player_specials->companion_num > 0) ? 
              FIGHTING(ch)->player_specials->companion_desc : "$N",
              GET_FIGHT_NUMBER_OF_HITS(ch), 
              GET_FIGHT_NUMBER_OF_ATTACKS(ch), GET_FIGHT_DAMAGE_DONE(ch), GET_FIGHT_CRITICAL_HIT(ch) > 0 ? critbuf : "", 
              GET_FIGHT_SNEAK_ATTACK(ch) > 0 ? sneakbuf : "", GET_FIGHT_PRECISE_ATTACK(ch) > 0 ? precisebuf : "",
              GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) > 0 ? reductbuf : "", GET_FIGHT_DEATH_ATTACK(ch) > 0 ? " @M(Death Attack)@n" : "",
              GET_FIGHT_SPRING_ATTACK(ch) > 0 ? "@M(Spring Attack)@n" : "", GET_FIGHT_UNDEATH_TOUCH(ch) > 0 ? " @M(Touch of Undeath)@n" : "",
              (ch)->parried_attacks > 0 ? parrybuf : "");
      fight_output(fbuf, ch, victim, TO_NOTVICT);
    
  }  
  int local_gold = 0;
  char local_buf[200];


  /* Uh oh.  Victim died. */
  if ((GET_POS(victim) == POS_DEAD || GET_HIT(victim) < -10)) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
        ch = ch->master;
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch->master, victim);
        else
          solo_gain(ch->master, victim);
      }
      else {
        if (AFF_FLAGGED(ch, AFF_GROUP))
          group_gain(ch, victim);
        else
          solo_gain(ch, victim);
      }
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, true, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;
    GET_FIGHT_BLEEDING(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
  }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DOING_AOO);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AOO);
    GET_FIGHT_BLEEDING(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_SPRING_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_UNDEATH_TOUCH(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = FALSE;

}

EVENTFUNC(pause_combat)
{

  struct pause_event *pause = (struct pause_event *) event_obj;
  struct char_data *ch;

  ch = pause->ch;
  ch->active_turn = 0;
  find_next_combat_target(ch);

  /* kill this event */
  ch->pause_combat = NULL;
  free(event_obj);
  return 0;

}

int get_average_damage(struct char_data *ch, struct obj_data *wielded)
{
  int strmod = 0;

   strmod = ability_mod_value(GET_STR(ch));
    
    // If the character is wielding a light weapon and they have the improved weapon
    // finesse feat, make their strength mod equivalent to the characters armor-
    // capped dexterity modifier.
    if (wielded && (GET_OBJ_SIZE(wielded) < get_size(ch) || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_RAPIER ||
                 IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) && 
    	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)) || 
          has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)))) {
      strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
    }
    if (!wielded &&
    	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED) ||
    	  has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED))) {
      strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
    }

    if (wielded && wielded == GET_EQ(ch, WEAR_WIELD1) &&
        !GET_EQ(ch, WEAR_WIELD2) &&
        wield_type(get_size(ch), wielded) >= WIELD_ONEHAND &&
        !WEAPON_FLAGGED(wielded, WEAPON_FLAG_DOUBLE) && !WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED) && 
        !WEAPON_FLAGGED(wielded, WEAPON_FLAG_THROWN))
      strmod = strmod * 3 / 2;

    if (wielded && (WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED)) && !IS_BOW(wielded) && !IS_THROWN_WEAPON(wielded)) {
      if (!GET_CLASS_RANKS(ch, CLASS_RANGER) || 
          (weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFamily == WEAPON_FAMILY_CROSSBOW))
      	strmod = 0;
    }

    if (wielded && IS_RANGED_WEAPON(wielded)) {
      if (!IS_COMPOSITE_BOW(wielded) && !IS_THROWN_WEAPON(wielded)) {
       strmod = MAX(strmod, 2);
      }
    }
	
  int ndice = 0, diesize = 0;

  if (!affected_by_spell(ch, SPELL_AFF_DISARMED) && 
        wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && GET_OBJ_VAL(wielded, 0) != WEAPON_TYPE_UNARMED) {
      if ((GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_CLUB || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_QUARTERSTAFF) &&
          affected_by_spell(ch, SPELL_SHILLELAGH)) {
        ndice = 2;
        diesize = 6;
      }
      else if ((GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_GREAT_CLUB) &&
          affected_by_spell(ch, SPELL_SHILLELAGH)) {
        ndice = 3;
        diesize = 8;
      }
      else {
        ndice = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMDICE);
        diesize = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMSIZE);
      }
    } else {
      if (IS_NPC(ch)) {
          ndice = MAX(1, GET_DAMAGE_MOD(ch) * (49 + dice(1, 101)) / 100);
          diesize = 1;
      }
      else {
          ndice = bare_hand_damage(ch, 1);
          diesize = bare_hand_damage(ch, 2);
      }
    }

    int j = 0;

    int weaponDamMod = 0;
    
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (wielded && (wielded->affected[j].location == APPLY_DAMAGE) && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
        weaponDamMod = wielded->affected[j].modifier;
     }  

  if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    weaponDamMod += 2;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED)))
    weaponDamMod += 2;

  if (affected_by_spell(ch, SPELL_SICKENED))
    weaponDamMod -= 2;

  if (wielded && HAS_FEAT(ch, FEAT_DIVINE_BOND)) {
    weaponDamMod += 1 + MIN(6, MAX(0, (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 5) / 3));
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 26)
        weaponDamMod += 3;    
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 32)
        weaponDamMod += 3;    
  }

  if (wielded && IS_BOW(wielded)) {
    weaponDamMod += HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC);
    if (HAS_FEAT(ch, FEAT_ENHANCE_ARROW_ELEMENTAL))
      weaponDamMod += 3;    
  }

  int magic = FALSE;

  if (wielded && (GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_CLUB || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_QUARTERSTAFF ||
      GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_GREAT_CLUB) && affected_by_spell(ch, SPELL_SHILLELAGH)) {
    weaponDamMod = MAX(weaponDamMod, 1);
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    weaponDamMod = MAX(weaponDamMod, 1);
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_FANG) && ((!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) || AFF_FLAGGED(ch, AFF_WILD_SHAPE))) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_FANG)
        weaponDamMod = MAX(weaponDamMod, af->modifier);
        magic = TRUE;
    }
  }

  if (affected_by_spell(ch, SPELL_MAGIC_WEAPON)) {
    weaponDamMod = MAX(weaponDamMod, 1);
    magic = TRUE;
  }

  if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON)) {
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next) {
      if (af->type == SPELL_GREATER_MAGIC_WEAPON)
        weaponDamMod = MAX(weaponDamMod, af->modifier);
        magic = TRUE;
    }
  }

  if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_ADAMANTINE)
    weaponDamMod += 1;
  
    /* Start with the damage bonuses: the damroll and strength apply */
    int dam = strmod;
    dam += GET_DAMAGE_MOD(ch);
    if (AFF_FLAGGED(ch, AFF_SMITING))
      dam += (GET_CLASS_RANKS(ch, CLASS_PALADIN) + GET_CLASS_RANKS(ch, CLASS_TEMPLAR) +
              GET_CLASS_RANKS(ch, CLASS_CHAMPION)) * (1 + HAS_FEAT(ch, FEAT_GREAT_SMITING));
    if (HAS_FEAT(ch, FEAT_POWER_ATTACK) &&
        GET_POWERATTACK(ch) && GET_STR(ch) > 12) {
      dam += GET_POWERATTACK(ch);
      if (wielded && !GET_EQ(ch, WEAR_WIELD2) && !GET_EQ(ch, WEAR_SHIELD) && GET_OBJ_SIZE(wielded) > get_size(ch))
        dam += GET_POWERATTACK(ch);      
    }

    if (wielded && (GET_OBJ_SIZE(wielded) <= get_size(ch) || IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_BALANCED)) 
        && HAS_FEAT(ch, FEAT_PRECISE_STRIKE) && IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_PIERCING) && GET_EQ(ch, WEAR_HOLD) != wielded)
      dam += GET_CLASS_RANKS(ch, CLASS_DUELIST);      


    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
    } else {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
    }

    if (affected_by_spell(ch, SPELL_PRAYER))
      dam += 1;

    if (affected_by_spell(ch, SPELL_FLAME_WEAPON) && wielded) {
      dam += 4;
    }
    if (GET_RACE(ch) == RACE_SMALL_FIRE_ELEMENTAL)
      dam += 3;
    if (GET_RACE(ch) == RACE_MEDIUM_FIRE_ELEMENTAL)
      dam += 4;
    if (GET_RACE(ch) == RACE_LARGE_FIRE_ELEMENTAL)
      dam += 5;
    if (GET_RACE(ch) == RACE_HUGE_FIRE_ELEMENTAL)
      dam += 7;

   dam += weaponDamMod;

    dam += (int) (ndice * (float) (diesize / 2));

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);        

    magic = wielded ? OBJ_FLAGGED(wielded, ITEM_MAGIC) : FALSE;

    return dam;
}
static int fightsort_compare(const void *a1, const void *b1)
{
  const struct fightsort_elem *a = (const struct fightsort_elem *) a1;
  const struct fightsort_elem *b = (const struct fightsort_elem *) b1;
  if (a->init < b->init)
    return -1;
  if (a->init > b->init)
    return 1;
  if (a->dex < b->dex)
    return -1;
  if (a->dex > b->dex)
    return 1;
  return 0;
}

int get_dam_dice_size(struct char_data *ch, struct obj_data *wielded, int return_mode) {
  int ndice = 0, diesize = 0;
  if (!affected_by_spell(ch, SPELL_AFF_DISARMED) &&
        wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    ndice = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMDICE);
    diesize = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMSIZE);
  } else {
    if (IS_NPC(ch)) {
      ndice = MAX(1, GET_DAMAGE_MOD(ch) * (49 + dice(1, 101)) / 100);
      diesize = 1;
    }
    else {
      ndice = bare_hand_damage(ch, 1);
      diesize = bare_hand_damage(ch, 2);
    }
  }
  if (return_mode == 1)
    return ndice;
  else
    return diesize;
}


int get_damage_mod(struct char_data *ch, struct obj_data *wielded) {
  int j = 0;
  int dam = 0;
  int weaponDamMod = 0;
  int strmod = 0;
  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (wielded && (wielded->affected[j].location == APPLY_DAMAGE) && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      weaponDamMod = wielded->affected[j].modifier;
   }  
  if (wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    weaponDamMod += 2;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_MASTERY, WEAPON_TYPE_UNARMED)))
    weaponDamMod += 2;
  if (affected_by_spell(ch, SPELL_SICKENED))
    weaponDamMod -= 2;
  if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_ADAMANTINE)
    weaponDamMod += 1;
  if (wielded && HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE) && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
      has_weapon_feat(ch, FEAT_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
  strmod = ability_mod_value(GET_STR(ch));
    
  // If the character is wielding a light weapon and they have the improved weapon
  // finesse feat, make their strength mod equivalent to the characters armor-
  // capped dexterity modifier.
  if (wielded && (GET_OBJ_SIZE(wielded) < get_size(ch) || GET_OBJ_VAL(wielded, 0) == WEAPON_TYPE_RAPIER ||
               IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) &&
  	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)) ||
        has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, GET_OBJ_VAL(wielded, 0)))) {
    strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
  }
  if (!wielded &&
  	  (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED) ||
  	  has_weapon_feat(ch, FEAT_IMPROVED_WEAPON_FINESSE, WEAPON_TYPE_UNARMED))) {
    strmod = MAX(strmod, ability_mod_value(GET_DEX(ch)) / 2) ;
  }
  if (wielded && wielded == GET_EQ(ch, WEAR_WIELD1) &&
      !GET_EQ(ch, WEAR_WIELD2) &&
      !GET_EQ(ch, WEAR_SHIELD) &&
      !WEAPON_FLAGGED(wielded, WEAPON_FLAG_DOUBLE) && !WEAPON_FLAGGED(wielded, WEAPON_FLAG_RANGED) &&
      !WEAPON_FLAGGED(wielded, WEAPON_FLAG_THROWN))
      strmod = strmod * 2;
  if (wielded && IS_RANGED_WEAPON(wielded)) {
    strmod = 0;
  }
	
  /* Start with the damage bonuses: the damroll and strength apply */
  dam = strmod;
  if (HAS_FEAT(ch, FEAT_POWER_ATTACK) &&
      GET_POWERATTACK(ch) && GET_STR(ch) > 12) {
    dam += GET_POWERATTACK(ch);
    if (wielded && !GET_EQ(ch, WEAR_WIELD2) && !GET_EQ(ch, WEAR_SHIELD) && (GET_OBJ_SIZE(wielded) > get_size(ch)))
      dam += GET_POWERATTACK(ch);
  }
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
        GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
      dam += 2;
    else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
        GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
      dam += 2;
    if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
        GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
      dam += 2;
    else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
        GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
      dam += 2;
  } else {
    if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
        WEAPON_TYPE_UNARMED))
      dam += 2;
    else if (has_weapon_feat(ch, FEAT_WEAPON_SPECIALIZATION,
        WEAPON_TYPE_UNARMED))
      dam += 2;
    if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
        WEAPON_TYPE_UNARMED))
      dam += 2;
    else if (has_weapon_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION,
        WEAPON_TYPE_UNARMED))
      dam += 2;
  }
  if (affected_by_spell(ch, SPELL_FLAME_WEAPON) && wielded) {
    dam += dice(1, 8);
  }
  dam += weaponDamMod;
  if (ch->mentor_level > 0) {
    dam *= ch->mentor_level;
    dam /= GET_CLASS_LEVEL(ch);
  }
  if (GET_GUILD(ch) == GUILD_FIGHTERS && (!wielded ||!IS_RANGED_WEAPON(wielded)) && FIGHTING(ch))
    if (dice(1, 100) < (20 + (2 * (GET_GUILD_RANK(ch)) / 4)))
      dam += 5;
  if (GET_GUILD(ch) == GUILD_ARCHERS && (wielded && IS_RANGED_WEAPON(wielded)) && FIGHTING(ch))
    if (dice(1, 100) < (20 + (2 * (GET_GUILD_RANK(ch)+1) / 4)))
      dam += 5;
  if (affected_by_spell(ch, SPELL_AFF_INTIMIDATED))
    dam -= 5;
  return dam;
}
