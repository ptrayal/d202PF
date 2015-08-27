/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "pets.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "feats.h"
#include "player_guilds.h"

struct combat_list_type *combat_list;

/* extern functions */
void do_affectv_tickdown(struct char_data *i);
void do_affect_tickdown(struct char_data *i);
void decide_mobile_special_action(struct char_data *ch);
int one_hit(struct char_data *ch, struct char_data *victim, struct obj_data *wielded, int w_type, int calc_base_hit,
            char *damstr, char *critstr, int hitbonus);
void fight_action(struct char_data *ch);
void roll_initiative(struct char_data *ch);
int get_speed(struct char_data *ch);
int compute_base_hit(struct char_data *ch, int weaponmod);
void stop_guard(struct char_data *ch);
void do_summon_attack(struct char_data *ch);
void do_companion_attack(struct char_data *ch);
void do_mount_attack(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data * killer);
void check_killer(struct char_data *ch, struct char_data *vict);
int is_innate_ready(struct char_data *ch, int spellnum);
void add_innate_timer(struct char_data *ch, int spellnum);
void do_pet_attacks(struct char_data *ch);
int get_skill_mod(struct char_data *ch, int skillnum);
int get_size(struct char_data *ch);
int skill_roll(struct char_data *ch, int skillnum);
int has_favored_enemy(struct char_data *ch, struct char_data *victim);
ACMD(do_flee);
ACMD(do_get);
int compute_armor_class(struct char_data *ch, struct char_data *att);
ACMD(do_split);
ACMD(do_sac);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
int is_player_grouped(struct char_data *target, struct char_data *group);
void do_whirlwind_attack(struct char_data *ch, struct char_data *victim);
void do_swarm_of_arrows(struct char_data *ch, struct char_data *victim);
void award_expendable_item(struct char_data *ch, int grade, int type);
void award_magic_weapon(struct char_data *ch, int grade, int moblevel);
int determine_random_weapon_type(void);
void award_magic_armor(struct char_data *ch, int grade, int moblevel);
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel);
void award_special_magic_item(struct char_data *ch);

/* extern variables */
extern struct spell_info_type spell_info[];

#define GRADE_MUNDANE 1
#define GRADE_MINOR   2
#define GRADE_MEDIUM  3
#define GRADE_MAJOR   4

#define TYPE_POTION 1
#define TYPE_SCROLL 2
#define TYPE_WAND   3
#define TYPE_STAFF  4


/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_order);
ACMD(do_flee);
int in_range(struct char_data *ch);
void find_next_combat_target(struct char_data *ch);
void perform_mob_combat_turn(struct char_data *ch);
void perform_pc_combat_turn(struct char_data *ch);
int check_active_turn(struct char_data *ch);
void display_your_turn(struct char_data *ch);
byte can_use_available_actions(struct char_data *ch, byte action);

ACMD(do_guard)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *to_guard;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You stop guarding.\r\n");
    stop_guard(ch);
  }
  else if (!(to_guard = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (to_guard == ch)
    send_to_char(ch, "You can't guard yourself any more than this!\r\n");
  else {

    if (GUARDED_BY(ch) || GUARDING(ch)) {
      stop_guard(ch);
    }

    if (IS_NPC(ch) || IS_NPC(to_guard))
      return;

    GUARDING(ch) = to_guard;
    GUARDED_BY(to_guard) = ch;
    act("You begin to guard $N.", FALSE, ch, 0, to_guard, TO_CHAR);
    act("$n begins to guard you.", FALSE, ch, 0, to_guard, TO_VICT);
  }
}	

ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_notvis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch))
      tch->exp_chain = 0;
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room)
		;
    if (opponent == helpee)
      return;
    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (ch && opponent && !CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
         /* prevent accidental pkill */
    else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent) && !IS_NPC(ch))	
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      if (!ch)
        return;
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      set_fighting(ch, opponent);
    }
  }
}


ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct follow_type *k;

  one_argument(argument, arg);

  if (ch->paralyzed) {
    send_to_char(ch, "You are paralyzed and cannot act!\r\n");
    return;
  }

  if (!*arg)
    send_to_char(ch, "Hit who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "That player is not here.\r\n");
  else if (subcmd != SCMD_MURDER && !IS_NPC(ch) && (!IS_NPC(vict) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_INNOCENT)))) {
    send_to_char(ch, "You must use the murder command to attack that opponent.\r\nOtherwise witnesses could turn you over for justice for killing an innocent.\r\n");
    return;
  }
  else if (subcmd == SCMD_MURDER) {
    if (PRF_FLAGGED(ch, PRF_PVP) && PRF_FLAGGED(vict, PRF_PVP)) {
      if (get_highest_group_level(ch) > get_highest_group_level(vict)) {
        send_to_char(ch, "Your highest group level is %d, while your opponent's highest group level is %d.  You must mentor down to that level to fight.\r\n",
                     get_highest_group_level(ch), get_highest_group_level(vict));
        return;
      }
      send_to_char(ch, "Your pvp timer has been set to 30 minutes.\r\n");
      ch->pvp_timer = 30;
      send_to_char(vict, "Your pvp timer has been set to 30 minutes.\r\n");
      vict->pvp_timer = 30;
    } else if (!PRF_FLAGGED(ch, PRF_PVP)) {
      send_to_char(ch, "You do not have your pvp flag enabled. (type pvp to enable it).\r\n");
      return;
    } else {
      send_to_char(ch, "That person does not have their pvp flag enabled.\r\n");
      return;
    }
  }
  else if (vict == ch) {
    send_to_char(ch, "You hit yourself...OUCH!.\r\n");
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch))
      tch->exp_chain = 0;
    if (!CONFIG_PK_ALLOWED) {
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
	if (subcmd != SCMD_MURDER) {
	  send_to_char(ch, "Use 'murder' to hit another player.\r\n");
	  return;
	} else {
	  check_killer(ch, vict);
	}
      }
      if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if (((GET_POS(ch) == POS_STANDING) || GET_POS(ch) == POS_RIDING) && (vict != FIGHTING(ch))) {
      /* normal attack :( */
      hit(ch, vict, TYPE_UNDEFINED);
    } else
      send_to_char(ch, "You do the best you can!\r\n");
  }
  for (k = ch->followers; k; k=k->next) {
    /* should followers auto-assist master? */
    if ((IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) && !FIGHTING(k->follower) && IN_ROOM(k->follower) == IN_ROOM(ch) &&
       AFF_FLAGGED(k->follower, AFF_GROUP)) ||
       (!IS_NPC(k->follower) && !FIGHTING(k->follower) &&
      (!IS_NPC(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST)) &&
      (IN_ROOM(k->follower) == IN_ROOM(ch))))
      do_assist(k->follower, GET_NAME(ch), 0, 0);
  }

  /* should master auto-assist followers?  */
  if (ch->master && PRF_FLAGGED(ch->master, PRF_AUTOASSIST) &&
    FIGHTING(ch) && (!IS_NPC(ch) || (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))) && !FIGHTING(ch->master) &&
    (IN_ROOM(ch->master) == IN_ROOM(ch)))
    do_assist(ch->master, GET_NAME(ch), 0, 0);

}



ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_INSTANTKILL)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      if (IS_NPC(vict))
        SET_BIT_AR(MOB_FLAGS(vict), MOB_EXTRACT);
      raw_kill(vict, ch);
    }
  }
}



#if 0 /* backstab doesn't exist, sneaky types get automatic sneak attack */
ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), VAL_WEAPON_DAMTYPE) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  percent = roll_skill(ch, SKILL_BACKSTAB);

  if (AWAKE(vict) && percent)
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);

  WAIT_STATE(ch, 2 * CONFIG_PULSE_VIOLENCE);
}
#endif


ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH]={'\0'};

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(ch, "%s", CONFIG_OK);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH]={'\0'};

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
	if (ch && k && k->follower && IN_ROOM(ch) != NOWHERE && IN_ROOM(k->follower) != NOWHERE && IN_ROOM(ch) == IN_ROOM(k->follower))
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(ch, "%s", CONFIG_OK);
      else
	send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}



ACMD(do_flee)
{
  int i, attempt;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
    return;

  for (i = 0; i < 6; i++) {
    attempt = rand_number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch))
      tch->exp_chain  = 0;
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char(ch, "You flee head over heels.\r\n");
        struct char_data *tch = NULL;
        sbyte found = FALSE;
        for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
          if (FIGHTING(tch) == FIGHTING(ch) && tch != ch) {
            found = TRUE;
            break;
          }
        }
        if (!found)
          stop_fighting(FIGHTING(ch));
        stop_fighting(ch);
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}

void perform_disarm(struct char_data *ch, struct char_data *vict, int skillnum) {

  int roll, goal;
  struct obj_data *weap;
  int success = FALSE;
  struct affected_type af;

  weap = GET_EQ(vict, WEAR_WIELD1);
  if (!weap) {
    weap = GET_EQ(vict, WEAR_WIELD2);
    if (!weap) {
      send_to_char(ch, "But your opponent is not wielding a weapon!\r\n");
      return;
    }
  }

  goal = get_saving_throw_value(vict, SAVING_REFLEX) + 10;

  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  struct obj_data *offhand = GET_EQ(ch, WEAR_HOLD);

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
      (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)) ||
      has_weapon_feat(ch, FEAT_WEAPON_FLURRY, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL))))
    goal += 4;
  else if (offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON &&
          (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, GET_OBJ_VAL(offhand, VAL_WEAPON_SKILL)) ||
          has_weapon_feat(ch, FEAT_WEAPON_FLURRY, GET_OBJ_VAL(offhand, VAL_WEAPON_SKILL))))
    goal += 4;
  else if (!wielded && (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED) ||
          has_weapon_feat(ch, FEAT_WEAPON_FLURRY, WEAPON_TYPE_UNARMED)))
    goal += 4;

  roll = d20 + MAX(compute_base_hit(ch, GET_CLASS_LEVEL(ch) / 5), get_skill_value(ch, skillnum));

  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)  
    roll = 1000;
  if (GET_ADMLEVEL(vict) >= ADMLVL_IMMORT) 
    roll = -1000;

  if (roll >= goal) {
    success = TRUE;

    if ((weap = GET_EQ(vict, WEAR_WIELD1))) {

      act("You disarm $p from $N's hand!", FALSE, ch, weap, vict, TO_CHAR);
      act("$n disarms $p from your hand!", FALSE, ch, weap, vict, TO_VICT);
      act("$n disarms $p from $N's hand!", FALSE, ch, weap, vict, TO_NOTVICT);

      af.location = APPLY_ABILITY;
      af.type = SPELL_AFF_DISARMED;
      af.duration = 3 + MAX(0, ability_mod_value(GET_STR(ch)) - ability_mod_value(GET_DEX(vict)));
      af.bitvector = 0;
      af.modifier = 1;

      affect_join(vict, &af, false, false, false, false);		

    } else if ((weap = GET_EQ(vict, WEAR_WIELD2))) {

      act("You disarm $p from $N's hand!", FALSE, ch, weap, vict, TO_CHAR);
      act("$n disarms $p from your hand!", FALSE, ch, weap, vict, TO_VICT);
      act("$n disarms $p from $N's hand!", FALSE, ch, weap, vict, TO_NOTVICT);

      af.location = APPLY_ABILITY;
      af.type = SPELL_AFF_DISARMED;
      af.duration = 3 + MAX(0, ability_mod_value(GET_STR(ch)) - ability_mod_value(GET_DEX(vict)));
      af.bitvector = 0;
      af.modifier = 1;

      affect_join(vict, &af, false, false, false, false);		

    } else {
      log("SYSERR: do_disarm(), should have a weapon to be disarmed, but lost it!");
    }
  } else {
      act("You fail to disarm $N.", FALSE, ch, weap, vict, TO_CHAR);
      act("$n fails to disarm you.", FALSE, ch, weap, vict, TO_VICT);
      act("$n fails to disarm $N.", FALSE, ch, weap, vict, TO_NOTVICT);
      if (HAS_FEAT(vict, FEAT_SPRING_ATTACK) &&
          (GET_TOTAL_AOO(vict) < MAX(1, HAS_FEAT(vict, FEAT_COMBAT_REFLEXES) ? ability_mod_value(GET_DEX(vict)): 1))) {
        do_attack_of_opportunity(vict, ch, "Spring Attack");
      }
  }
}

ACMD(do_disarm)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int success = FALSE;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
    else {
      send_to_char(ch, "Disarm who?\r\n");
      return;
    }
  }

  if (ch->combat_pos != vict->combat_pos) {
    send_to_char(ch, "You are not close enough to attempt a disarm.\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(ch, "Try REMOVE and DROP instead...\r\n");
    return;
  }

  perform_disarm(ch, vict, SKILL_COMBAT_TACTICS);

  if (!GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
    WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);	

  
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;

  if (success && IS_NPC(vict))
    set_fighting(ch, vict);
}




ACMD(do_turn)
{
  struct char_data *vict;
  int turn_level, percent;
  int turn_difference=0, turn_result=0, turn_roll=0;
  int i;
  char buf[MAX_STRING_LENGTH]={'\0'};

  one_argument(argument,buf);

  if (!IS_CLERIC(ch) && !IS_PALADIN(ch))   {
    send_to_char(ch, "You do not possess the favor of your God.\r\n");
    return;
  }
  if (!(vict=get_char_room_vis(ch, buf, NULL)))   {
    send_to_char(ch, "Turn who?\r\n");
    return;
  }
  if (vict==ch)   {
    send_to_char(ch, "How do you plan to turn yourself?\r\n");
    return;
  }
  if (!IS_UNDEAD(vict)) {
    send_to_char(ch, "You can only attempt to turn undead!\r\n");
    return;
  }

  if (CONFIG_ALLOW_MULTICLASS) {
    turn_level = GET_TLEVEL(ch);
    for (i = 0; i < NUM_CLASSES; i++) {
      if (spell_info[ABIL_TURNING].min_level[i] <= GET_CLASS_RANKS(ch, i)) {
        if (i == CLASS_CLERIC)
          turn_level += GET_CLASS_RANKS(ch, i);
        else
          turn_level += MAX(0, GET_CLASS_RANKS(ch, i) - 2);
      }
    }
  } else {
  if (GET_CLASS(ch) != CLASS_CLERIC)
    turn_level = ((GET_LEVEL(ch) - 2) + GET_TLEVEL(ch));
  else
    turn_level = (GET_LEVEL(ch) + GET_TLEVEL(ch));
  }

  percent=roll_skill(ch, ABIL_TURNING);

  if (!percent)   {
    send_to_char(ch, "You lost your concentration!\r\n");
    return;
  }

  turn_difference = (turn_level - GET_LEVEL(vict));
  turn_roll = rand_number(1, 20);

  switch (turn_difference)   {
    case -5:
    case -4:
      if (turn_roll >= 20)
        turn_result=1;
      break;
    case -3:
      if (turn_roll >= 17)
        turn_result=1;
      break;
    case -2:
      if (turn_roll >= 15)
        turn_result=1;
      break;
    case -1:
      if (turn_roll >= 13)
        turn_result=1;
      break;
    case 0:
      if (turn_roll >= 11)
        turn_result=1;
      break;
    case 1:
      if (turn_roll >= 9)
        turn_result=1;
      break;
    case 2:
      if (turn_roll >= 6)
        turn_result=1;
      break;
    case 3:
      if (turn_roll >= 3)
        turn_result=1;
      break;
    case 4:
    case 5:
      if (turn_roll >= 2)
        turn_result=1;
      break;
    default:
      turn_result=0;
      break;
  }

  if (turn_difference <= -6)   
    turn_result=0; 
  else if (turn_difference >= 6)
    turn_result=2;

  switch (turn_result)   {
    case 0:  /* Undead resists turning */
      act("$N blasphemously mocks your faith!",FALSE,ch,0,vict,TO_CHAR);
      act("You blasphemously mock $N and $S faith!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n blasphemously mocks $N and $S faith!", FALSE, vict, 0, ch, TO_NOTVICT);
      hit(vict, ch, TYPE_UNDEFINED);
      break;
    case 1:  /* Undead is turned */
      act("The power of your faith overwhelms $N, who flees!", FALSE, ch, 0, vict, TO_CHAR);
      act("The power of $N's faith overwhelms you! You flee in terror!!!", FALSE, vict, 0, ch, TO_CHAR);
      act("The power of $N's faith overwhelms $n, who flees!", FALSE, vict, 0, ch, TO_NOTVICT);
      do_flee(vict,0,0,0);
      break;
    case 2:  /* Undead is automatically destroyed */
      act("The mighty force of your faith blasts $N out of existence!", FALSE,ch,0,vict, TO_CHAR);
      act("The mighty force of $N's faith blasts you out of existence!",FALSE,vict,0,ch, TO_CHAR);
      act("The mighty force of $N's faith blasts $n out of existence!",FALSE,vict,0,ch, TO_NOTVICT);
      raw_kill(vict, ch);
      break;
  }
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;
}

ACMD(do_lay_hands)
{
  struct char_data *vict;
  int healing;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Whom do you wish to lay hands on?\r\n");
    return;
  }

  if (FIGHTING(ch)) {
    if (!can_use_available_actions(ch, ACTION_MOVE)) {
      send_to_char(ch, "You do not have any available actions to use lay on hands.\r\n");
      return;
    }
  }

  if (is_innate_ready(ch, ABIL_LAY_HANDS) && GET_LAY_HANDS(ch) == 0) {
    GET_LAY_HANDS(ch) = (GET_CLASS_RANKS(ch, CLASS_PALADIN) / 2) + ability_mod_value(GET_CHA(ch));
    send_to_char(ch, "Your lay on hands ability has been restored to full.  You have %d uses left.\r\n", GET_LAY_HANDS(ch));
  }

  if (GET_LAY_HANDS(ch) == 0) {
    send_to_char(ch, "You do not have the energy to lay hands on someone.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  if ((!IS_NPC(ch)) && (!HAS_FEAT(ch, FEAT_LAYHANDS))) {
    send_to_char(ch, "You have no idea how to lay hands to heal someone.\r\n");
    return;
  }

  if (GET_HIT(vict) >= GET_MAX_HIT(vict)) {
    send_to_char(ch, "They are already at maximum health!\r\n");
    return;
  }

  healing = dice(GET_CLASS_RANKS(ch, CLASS_PALADIN) / 2, 6);

  if (PRF_FLAGGED(vict, PRF_PVP))
  healing = MAX(1, healing / 5);

  char buf[200]={'\0'};
  if (vict == ch) {
    sprintf(buf, "@WYou lay hands on yourself for @Y%d@W hit points healed.@n", healing);
    act(buf, FALSE, ch, 0, ch, TO_CHAR);
    sprintf(buf, "@W$m lays hands on $mself for @Y%d@W hit points healed.@n", healing);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  } else {
    sprintf(buf, "@WYou lay hands on $M for @Y%d@W hit points healed.@n", healing);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    sprintf(buf, "@W$m lays hands on you for @Y%d@W hit points healed.@n", healing);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    sprintf(buf, "@W$m lays hands on $M for @Y%d@W hit points healed.@n", healing);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }

  if (IS_NPC(vict) && IS_UNDEAD(vict)) {
    sprintf(buf, "@rYou @ylay hands on @W$M@y for @R%d@y damage dealt.@n", healing);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    sprintf(buf, "@r$m @ylays hands on @Wyou@y for @R%d@y damage dealt.@n", healing);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    sprintf(buf, "@r$m @ylays hands on @W$M @yfor @R%d @ydamage dealt.@n", healing);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);

    if (!FIGHTING(vict))
      set_fighting(vict, ch);
    if (!FIGHTING(ch))
      set_fighting(ch, vict);
    damage(ch, vict, healing, 0, ABIL_LAY_HANDS, -1, 0, ABIL_LAY_HANDS, 1);
  }
  else {
    GET_HIT(vict) += healing;
    if (GET_HIT(vict) > GET_MAX_HIT(vict))
      GET_HIT(vict) = GET_MAX_HIT(vict);  
    if (GET_FIGHT_BLEEDING_DAMAGE(vict))
      send_to_char(vict, "Your bleeding stops.\r\n");
    GET_FIGHT_BLEEDING_DAMAGE(vict) = 0;

  }

  GET_LAY_HANDS(ch) -= 1;
  send_to_char(ch, "You now have %d uses of lay hands left.\r\n", GET_LAY_HANDS(ch));
  /* set the timer until the skill can be used again -Cyric*/
  if (is_innate_ready(ch, ABIL_LAY_HANDS))
    add_innate_timer(ch, ABIL_LAY_HANDS);
}

ACMD(do_trip) {

  struct char_data *vict;
  char arg[MAX_STRING_LENGTH];
  int roll = 0, dc = 0;


  one_argument(argument, arg);

  if (!*arg) {
    if (!(vict = FIGHTING(ch))) {
      send_to_char(ch, "Who would you like to trip?\r\n");
      return;
    }
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person doesn't seem to be here.\r\n");
    return;
  }
  
  if (ch->combat_pos != vict->combat_pos) {
    send_to_char(ch, "You are not close enough to attempt a trip.\r\n");
    return;
  }


  if (!IS_NPC(vict) && (affected_by_spell(vict, SPELL_FLY) || HAS_FEAT(vict, FEAT_WINGS))) {
    send_to_char(ch, "You cannot trip a flying opponent.\r\n");
    return;
  }

  if ((get_size(vict) - get_size(ch)) > 1) {
    send_to_char(ch, "That creature is too large for you to trip.\r\n");
    return;
  }

  if (!can_use_available_actions(ch, ACTION_STANDARD)) {
      return;
  }

  roll = d20 + get_combat_bonus(ch) + (HAS_FEAT(ch, FEAT_IMPROVED_TRIP) ? 4 : 0);
  dc = get_combat_defense(vict) / 10;

  if (race_list[GET_RACE(vict)].family == RACE_TYPE_CENTAUR)
    dc += 4;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_ANIMAL)
    dc += 4;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_DRAGON)
    dc += 4;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_MAGICAL_BEAST)
    dc += 4;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_VERMIN)
    dc += 8;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_PLANT)
    dc += 999;
  if (race_list[GET_RACE(vict)].family == RACE_TYPE_OOZE)
    dc += 999;


  if (roll >= dc) {
    GET_POS(vict) = POS_RESTING;
    SET_BIT_AR(AFF_FLAGS(vict), AFF_NEXTPARTIAL);

    act("You trip $N sending $M sprawling.", false, ch, 0, vict, TO_CHAR);
    act("$n trips you sending you sprawling.", false, ch, 0, vict, TO_VICT);
    act("$n trips $N sending $M sprawling.", false, ch, 0, vict, TO_NOTVICT);    
    if (HAS_FEAT(vict, FEAT_SPRING_ATTACK) &&
          (GET_TOTAL_AOO(vict) < MAX(1, HAS_FEAT(vict, FEAT_COMBAT_REFLEXES) ? ability_mod_value(GET_DEX(vict)): 1))) {
        do_attack_of_opportunity(vict, ch, "Spring Attack");
    }
  }
  else if ((dc - roll) > 10 && dc < 999) {
    GET_POS(ch) = POS_RESTING;
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);

    send_to_char(ch, "ROLL: %d DC: %d\r\n", roll, dc);

    act("$N fails to trip you and you trip $M instead, sending $M sprawling.", false, vict, 0, ch, TO_CHAR);
    act("You fail to trip $n and $e trips you instead, sending you sprawling.", false, vict, 0, ch, TO_VICT);
    act("$N fails to trip $n and $n trips $N instead, sending $M sprawling.", false, vict, 0, ch, TO_NOTVICT);    
    if (HAS_FEAT(ch, FEAT_SPRING_ATTACK) &&
          (GET_TOTAL_AOO(ch) < MAX(1, HAS_FEAT(ch, FEAT_COMBAT_REFLEXES) ? ability_mod_value(GET_DEX(ch)): 1))) {
        do_attack_of_opportunity(vict, ch, "Spring Attack");
    }
  }
  else {
    act("You fail to trip $N.", false, ch, 0, vict, TO_CHAR);
    act("$n fails to trip you.", false, ch, 0, vict, TO_VICT);
    act("$n fails to trip $N.", false, ch, 0, vict, TO_NOTVICT);
  }

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;

  if (!FIGHTING(ch))
    hit(vict, ch, TYPE_UNDEFINED);

  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);

}

ACMD(do_smite) {

  char arg[MAX_STRING_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (GET_SMITE_EVIL(ch) < 1) {
    send_to_char(ch, "You have used all of your smite opportunities for now.\r\n");
    return;
  }

  if (!*arg) {
    if (FIGHTING(ch))
      vict = FIGHTING(ch);
    else {
      send_to_char(ch, "You must specify a target to smite.\r\n");
      return;
    }
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There doesn't seem to be anything here by that description.\r\n");
    return;
  }

  if (HAS_FEAT(ch, FEAT_SMITE_EVIL) == 0) {
    send_to_char(ch, "You do not have the ability to smite evil.\r\n");
    return;
  }

  if (FALSE && IS_GOOD(ch) && !IS_EVIL(vict)) {
    send_to_char(ch, "Your target is not evil.\r\n");
    return;
  }
  
  if (FIGHTING(ch))
    send_to_char(ch, "You prepare to smite your opponent.\r\n");

  GET_SMITE_EVIL(ch) -= 1;

  ch->smiting = vict;

  if (is_innate_ready(ch, ABIL_SMITE_EVIL))
    add_innate_timer(ch, ABIL_SMITE_EVIL);
}

ACMD(do_feint) {
	
  if (!FIGHTING(ch)) {
    send_to_char(ch, "Why feint?  You aren't even fighting anyone.\r\n");
    return;
  }

  if (HAS_FEAT(ch, FEAT_IMPROVED_FEINT)) {
    if (!can_use_available_actions(ch, ACTION_MINOR)) {
      return;
    }
  }
  else if (!can_use_available_actions(ch, ACTION_MOVE)) {
    return;
  }

  struct char_data *vict = FIGHTING(ch);
  
  int dc, roll;

  dc = dice(1, 20) + get_skill_value(vict, SKILL_SENSE_MOTIVE);

  if (!IS_NPC(vict))
  if (has_favored_enemy(vict, ch))
    dc += dice(1, (GET_CLASS_RANKS(vict, CLASS_RANGER) / 5) + 1) * 2;
	
  roll = dice(1, 20) + get_skill_value(ch, SKILL_BLUFF);

  if (!IS_NPC(ch))
    if (has_favored_enemy(ch, vict))
      dc += dice(1, (GET_CLASS_RANKS(ch, CLASS_RANGER) / 5) + 1) * 2;
	
  if (roll >= dc) {
    SET_BIT_AR(AFF_FLAGS(vict), AFF_FLAT_FOOTED_1);
      if (!HAS_FEAT(ch, FEAT_IMPROVED_FEINT))
        SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    act("You feint a strike at $N throwing $M off-guard.", false, ch, 0, vict, TO_CHAR);	
    act("$n feints a strike at you throwing you off-guard.", false, ch, 0, vict, TO_VICT);	
    act("$n feints a strike at $N throwing $M off-guard.", false, ch, 0, vict, TO_NOTVICT);	  	
  }
  else {
    act("You feint a strike at $N with no apparent effect.", false, ch, 0, vict, TO_CHAR);	
    act("$n feints a strike at you but you sense $s true intentions easily.", false, ch, 0, vict, TO_VICT);	
    act("$n feints a strike at $N with no apparent effect.", false, ch, 0, vict, TO_NOTVICT);	  
  }
  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;
}

ACMD(do_expertise) {

  char arg[MAX_STRING_LENGTH];
  int mod;

  one_argument(argument, arg);
	
  if (!*arg) {
      send_to_char(ch, "You must specify a modifier between 1 and 5 to be added to defense and subtracted from offense.@n\r\n");
      return;
  }
  
  mod = atoi(arg);
	
	if (!HAS_FEAT(ch, FEAT_COMBAT_EXPERTISE)) {
		send_to_char(ch, "You don't have the combat expertise feat.@n\r\n");
		return;
	}
	
	if (mod > MIN(5, GET_BAB(ch))) {
		send_to_char(ch, "You cannot set the modifier to be higher then %d.@n\r\n", MIN(5, GET_BAB(ch)));
		return;		
	}

	if (mod > 5 || mod < 0) {
		send_to_char(ch, "The modifier must be between 1 and 5.@n\r\n");
		return;		
	}
	
	GET_EXPERTISE_BONUS(ch) = mod;
	
	send_to_char(ch, "You will now have +%d added to armor class and -%d subtracted from attack roll.@n\r\n", mod, mod);
	
	return;
}

ACMD(do_challenge)
{

  if (!HAS_FEAT(ch, FEAT_COMBAT_CHALLENGE)) {
    send_to_char(ch, "You must have the combat challenge feat in order to issue a challenge.\r\n");
    return;
  }

  struct char_data *vict = NULL;
  char arg[200];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Who do you wish to challenge?\r\n");
    return;
  }

  if (strlen(arg) > 200) {
    arg[199] = '\0';
  }
  
  if (strcmp("all", arg)) {

    if ((HAS_FEAT(ch, FEAT_GREATER_COMBAT_CHALLENGE) || HAS_FEAT(ch, FEAT_EPIC_COMBAT_CHALLENGE)) && !can_use_available_actions(ch, ACTION_MINOR)) {
      return;
    }
    else if (HAS_FEAT(ch, FEAT_IMPROVED_COMBAT_CHALLENGE) && !can_use_available_actions(ch, ACTION_MOVE)) {
      return;
    }
    else if (!can_use_available_actions(ch, ACTION_STANDARD)) {
      return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "There's no one here by that description.\r\n");
      return;
    }

    if (vict == FIGHTING(ch)) {
      send_to_char(ch, "You are already fighting them.\r\n");
      return;
    }

    if (FIGHTING(vict) && !is_player_grouped(FIGHTING(vict), ch)) {
      send_to_char(ch, "No mob stealing please :)\r\n");
      return;
    }

    if (!IS_NPC(vict)) {
      send_to_char(ch, "Sorry, you can't challenge other players.  Free will and all that :)\r\n");
      return;
    }

    ch->challenge = 1;
    set_fighting(vict, ch);
    if (!FIGHTING(ch))
      set_fighting(ch, vict);
    ch->challenge = 0;

    act("You challenge $N to face you in personal combat.@n", true, ch, 0, vict, TO_CHAR);
    act("$n challenges you to face $m in personal combat.@n", true, ch, 0, vict, TO_VICT);
    act("$n challenge $N to face $m in personal combat.@n", true, ch, 0, vict, TO_NOTVICT);
    
  } 
  else if (HAS_FEAT(ch, FEAT_IMPROVED_COMBAT_CHALLENGE)) 
  {

    if (HAS_FEAT(ch, FEAT_EPIC_COMBAT_CHALLENGE) && !can_use_available_actions(ch, ACTION_MINOR)) {
      return;
    }
    else if (HAS_FEAT(ch, FEAT_GREATER_COMBAT_CHALLENGE) && !can_use_available_actions(ch, ACTION_MOVE)) {
      return;
    }
    else if (!can_use_available_actions(ch, ACTION_STANDARD)) {
      return;
    }

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    {
      if (!(CAN_SEE(ch, vict))) {
        continue;
      }

      if (vict == FIGHTING(ch)) {
        continue;
      }

      if (FIGHTING(vict) && !is_player_grouped(FIGHTING(vict), ch)) {
        continue;
      }

      if (!IS_NPC(vict))
        continue;

      set_fighting(vict, ch);
      if (!FIGHTING(ch))
        set_fighting(ch, vict);

      act("You challenge $N to face you in personal combat.@n", true, ch, 0, vict, TO_CHAR);
      act("$n challenges you to face $m in personal combat.@n", true, ch, 0, vict, TO_VICT);
      act("$n challenge $N to face $m in personal combat.@n", true, ch, 0, vict, TO_NOTVICT);

    }
    
  }
  else
  {
    send_to_char(ch, "You must have the improved combat challenge feat in order to challenge all.\r\n");
    return;
  }

}

ACMD(do_pilfer)
{

  if (!FIGHTING(ch)) {
    send_to_char(ch, "You need to be fighting to attempt to pilfer from your enemy.\r\n");
    return;
  }

  if (!can_use_available_actions(ch, ACTION_STANDARD)) {        
    send_to_char(ch, "You have no available actions to pilfer.\r\n");
    return;
  }

  if (!is_innate_ready(ch, SPELL_ACMD_PILFER)) {
    send_to_char(ch, "You are not able to pilfer again until the cooldown timer expires.\r\n");
    return;
  }

  int roll = skill_roll(ch, SKILL_SLEIGHT_OF_HAND);
  int dc = skill_roll(FIGHTING(ch), SKILL_PERCEPTION);

  if (roll >= dc) {

  roll = dice(1, 100);
  int grade = GRADE_MUNDANE;
  int level = GET_LEVEL(FIGHTING(ch));

  if (level >= 20) {
    grade = GRADE_MAJOR;
  }
  else if (level >= 16) {
    if (roll >= 61)
      grade = GRADE_MAJOR;
    else
      grade = GRADE_MEDIUM;
  }
  else if (level >= 12) {
    if (roll >= 81)
      grade = GRADE_MAJOR;
    else if (roll >= 11)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 8) {
    if (roll >= 96)
      grade = GRADE_MAJOR;
    else if (roll >= 31)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 4) {
    if (roll >= 76)
      grade = GRADE_MEDIUM;
    else if (roll >= 16)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }
  else {
    if (roll >= 96)
      grade = GRADE_MEDIUM;
    else if (roll >= 41)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }

    int roll2 = dice(1, 1000);

    if (roll2 == 1)
      award_special_magic_item(ch);
    else if (roll2 <= 200) {
      int gold = GET_LEVEL(FIGHTING(ch)) * MAX(1, (GET_LEVEL(FIGHTING(ch)) / 5)) * MAX(1, (GET_LEVEL(FIGHTING(ch)) / 10)) * 100;
      send_to_char(ch, "You have pilfered a purse containing gems worth %d %s.\r\n", gold, MONEY_STRING);
      GET_GOLD(ch) += gold;
    }
    else if (roll2 <= 400)
      award_expendable_item(ch, grade, TYPE_POTION);
    else if (roll2 <= 500)
      award_expendable_item(ch, grade, TYPE_SCROLL);
    else if (roll2 <= 575)
      award_expendable_item(ch, grade, TYPE_WAND);
    else if (roll2 <= 600)
      award_expendable_item(ch, grade, TYPE_STAFF);
    else if (roll2 <= 700)
      award_magic_weapon(ch, grade, level);
    else if (roll2 <= 900)
      award_misc_magic_item(ch, grade, level);
    else 
      award_magic_armor(ch, grade, level);
  } else {
    send_to_char(ch, "Your mark notices your attempt to steal from them and swats your hand away.\r\n");
    return;
  }

  add_innate_timer(ch, SPELL_ACMD_PILFER);
  send_to_char(ch, "You will be able to attempt another pilfer in 10 minutes.\r\n");

}

ACMD(do_taunt) 
{
	
	struct char_data *victim;
	int success = FALSE;	
	char taunter[MAX_STRING_LENGTH];
	char tauntee[MAX_STRING_LENGTH];
	char others[MAX_STRING_LENGTH];
	char arg[80];
	
	struct affected_type af;
				
	int skillnum;
	
	int dc, roll;
	
	if (subcmd == SCMD_TAUNT) 
  {
          if (!can_use_available_actions(ch, ACTION_MINOR)) 
          {        
            send_to_char(ch, "You have no available actions to taunt.\r\n");
            return;
          }
		skillnum = SKILL_BLUFF;
	}
	else if (subcmd == SCMD_INTIMIDATE) 
  {
          if (!can_use_available_actions(ch, ACTION_MINOR)) 
          {        
            send_to_char(ch, "You have no available actions to intimidate.\r\n");
            return;
          }
		skillnum =  SKILL_INTIMIDATE;
	}
	else if (subcmd == SCMD_DIPLOMACY) 
  {
          if (!can_use_available_actions(ch, ACTION_MINOR)) 
          {        
            send_to_char(ch, "You have no available actions to challenge.\r\n");
            return;
          }
		skillnum = SKILL_DIPLOMACY;
	}
	else 
  {
		send_to_char(ch, "Error with do_taunt.  Please contact a staff member.@n\r\n");
		return;
	}
	
        one_argument(argument, arg);
	
	if (affected_by_spell(ch, SPELL_AFF_TAUNTING)) 
  {
		send_to_char(ch, "You cannot do this more than once a round.\r\n");
		return;
	}
		
		
	
  if (!*arg) {
    if (FIGHTING(ch))
      victim = FIGHTING(ch);
      
    else {
    	send_to_char(ch, "Who are you trying to do this to?@n\r\n");
    	return;
    }
  }
  
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There doesn't seem to be anything here by that description.\r\n");
    return;
  }			

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;
	
	dc = dice(1, 20) + skill_roll(victim, SKILL_SENSE_MOTIVE);

	roll = dice(1, 20) + skill_roll(ch, skillnum);


  // Smaller is better for diplomacy
	if (subcmd == SCMD_DIPLOMACY) {
		roll += (get_size(victim) - get_size(ch)) * 2;
	}
	
	// Bigger is better for intimidate
	if (subcmd == SCMD_INTIMIDATE) {
		roll -= (get_size(victim) - get_size(ch)) * 2;
	}	
	
if (roll >= dc) {

    success = TRUE;
		
    if (subcmd == SCMD_INTIMIDATE) {
      af.location = APPLY_DAMROLL;
      af.type = SPELL_AFF_INTIMIDATED;
      af.duration = 1 + MAX(0, ability_mod_value(GET_CHA(ch)));
      af.bitvector = 0;
      af.modifier = -2;

      affect_join(victim, &af, false, false, false, false);		
    }

    if (subcmd == SCMD_TAUNT) {
      af.location = APPLY_AC;
      af.type = SPELL_AFF_TAUNTED;
      af.duration = 1 + MAX(0, ability_mod_value(GET_CHA(ch)));
      af.bitvector = 0;
      af.modifier = -20;

      affect_join(victim, &af, false, false, false, false);		
    }

    if (subcmd == SCMD_DIPLOMACY) {
      af.location = APPLY_HITROLL;
      af.type = SPELL_AFF_CHALLENGED;
      af.duration = 1 + MAX(0, ability_mod_value(GET_CHA(ch)));
      af.bitvector = 0;
      af.modifier = -2;

      affect_join(victim, &af, false, false, false, false);		
    }

    
}

	if (subcmd == SCMD_TAUNT) {
		sprintf(taunter, "You throw a barrage of nasty insults and mockery at $N.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(tauntee, "$n throws a barrage of nasty insults and mockery at you.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(others, "$n throws a barrage of nasty insults and mockery at $N.@n%s", success ? "" : " @M*resisted*@n");
	}
	else if (subcmd == SCMD_INTIMIDATE) {
		sprintf(taunter, "You call out a number of intimidating threats towards $N.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(tauntee, "$n calls out a number of intimidating threats towards you.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(others, "$n calls out a number of intimidating threats towards $N.@n%s", success ? "" : " @M*resisted*@n");			
	}
	else if (subcmd == SCMD_DIPLOMACY) {
		sprintf(taunter, "You challenge $N to face you in personal combat.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(tauntee, "$n challenges you to face $m in personal combat.@n%s", success ? "" : " @M*resisted*@n");
		sprintf(others, "$n challenges $N to face $m in personal combat.@n%s", success ? "" : " @M*resisted*@n");			
	}
	else {
		send_to_char(ch, "Error with do_taunt.  Please contact a staff member.@n\r\n");
		return;
	}

  act(taunter, FALSE, ch, 0, victim, TO_CHAR);
  act(tauntee, FALSE, ch, 0, victim, TO_VICT);
  act(others, FALSE, ch, 0, victim, TO_NOTVICT);    

  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);  
	
}

ACMD(do_rage) {

  struct affected_type af[5];
  int abil_mod=0, will=0, ac_mod=0, i;

  if (!HAS_FEAT(ch, FEAT_RAGE)) {
      send_to_char(ch, "You do not know how to rage.\r\n");
      return;
  }

  if (GET_RAGE(ch) < 1) {
        if (is_innate_ready(ch, SPELL_AFF_RAGE))
          GET_RAGE(ch) = HAS_FEAT(ch, FEAT_RAGE);
        else {
  	send_to_char(ch, "You have already used up all of your rage opportunities today.\r\n");
  	return;
        }
  }
  
  
  if (HAS_FEAT(ch, FEAT_RAGE)) {
  	abil_mod = 4;
  	will = 2;
  	ac_mod = -20;
  }
  
  if (HAS_FEAT(ch, FEAT_GREATER_RAGE)) {
  	abil_mod = 6;
  	will = 3;
  	ac_mod = -20;  	
  }
  if (HAS_FEAT(ch, FEAT_MIGHTY_RAGE)) {
  	abil_mod = 8;
  	will = 4;
  	ac_mod = -20;  	
  }  
  
  af[0].location = APPLY_STR;
  af[0].type = SPELL_AFF_RAGE;
  af[0].duration =  3 + ability_mod_value(GET_CON(ch)) + (abil_mod / 2);
  af[0].bitvector = AFF_RAGE;
  af[0].modifier = abil_mod;
  
  af[1].location = APPLY_CON;
  af[1].type = SPELL_AFF_RAGE;
  af[1].duration = 3 + ability_mod_value(GET_CON(ch)) + (abil_mod / 2);
  af[1].bitvector = AFF_RAGE;
  af[1].modifier = abil_mod;  

  af[2].location = APPLY_AC;
  af[2].type = SPELL_AFF_RAGE;
  af[2].duration = 3 + ability_mod_value(GET_CON(ch)) + (abil_mod / 2);
  af[2].bitvector = AFF_RAGE;
  af[2].modifier = ac_mod;
  
  af[3].location = APPLY_HIT;
  af[3].type = SPELL_AFF_RAGE;
  af[3].duration = 3 + ability_mod_value(GET_CON(ch)) + (abil_mod / 2);
  af[3].bitvector = AFF_RAGE;
  af[3].modifier = GET_LEVEL(ch) * (abil_mod / 2);
  
  af[4].location = APPLY_WILL;
  af[4].type = SPELL_AFF_RAGE;
  af[4].duration = 3 + ability_mod_value(GET_CON(ch)) + (abil_mod / 2);
  af[4].bitvector = AFF_RAGE;
  af[4].modifier = will;  

  for (i = 0; i < 5; i++) {
    if (HAS_FEAT(ch, FEAT_EXTEND_RAGE))
      af[i].duration += 5;
      af[i].duration *= 5;
    affect_join(ch, af+i, false, false, false, false);
  }

  GET_RAGE(ch) -= 1;
  GET_HIT(ch) += GET_LEVEL(ch) * (abil_mod / 2);

  if (is_innate_ready(ch, SPELL_AFF_RAGE) && GET_RAGE(ch) == 0) {
    add_innate_timer(ch, SPELL_AFF_RAGE);
  }
  
  send_to_char(ch, "You fly into a rage of fury and force, heightening your ability to deal and withstand damage.\r\n");
  act("$n flies into a barbaric rage of fury and force!", false, ch, 0, 0, TO_NOTVICT);
  
  return;
}

ACMD(do_strength_of_honor) {

  struct affected_type af;
  int abil_mod=0;
  
  if (!HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR)) {
    send_to_char(ch, "You do not have the strength of honor ability.\r\n");
    return;
  }

  if (GET_STRENGTH_OF_HONOR(ch) < 1) {
        if (is_innate_ready(ch, SPELL_AFF_STRENGTH_OF_HONOR))
          GET_STRENGTH_OF_HONOR(ch) = HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR);
        else {
  	send_to_char(ch, "You have already used up all of your strength of honor opportunities today.\r\n");
  	return;
        }
  }
  
  if (HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR)) {
  	abil_mod = 4;
  }
  
  if (HAS_FEAT(ch, FEAT_MIGHT_OF_HONOR)) {
  	abil_mod = 6;
  }
  
  af.location = APPLY_STR;
  af.type = SPELL_AFF_STRENGTH_OF_HONOR;
  af.duration = ability_mod_value(GET_CHA(ch)) + 3;
  af.bitvector = AFF_STRENGTH_OF_HONOR;
  af.modifier = abil_mod;

  affect_to_char(ch, &af);

  GET_STRENGTH_OF_HONOR(ch) -= 1;

  if (is_innate_ready(ch, SPELL_AFF_STRENGTH_OF_HONOR) && GET_STRENGTH_OF_HONOR(ch) == 0) {
    add_innate_timer(ch, SPELL_AFF_STRENGTH_OF_HONOR);
  }
  
  send_to_char(ch, "You lift your weapon in a knightly salute and recite your oath of honor.@n\r\n");
  act("$n lifts $s weapon in a knightly salute and powerfully recites $s oath of honor.@n", false, ch, 0, 0, TO_NOTVICT);
  
  return;
}

ACMD(do_defensive_stance) {

  struct affected_type af[5];
  int i;  

  if (!HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE)) {
    send_to_char(ch, "You do not have the defensive stance ability.\r\n");
    return;
  }

  if (GET_DEFENSIVE_STANCE(ch) < 1) {
        if (is_innate_ready(ch, SPELL_AFF_DEFENSIVE_STANCE))
          GET_DEFENSIVE_STANCE(ch) = HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE);
        else {
    	  send_to_char(ch, "You have already used up all of your defensive stance opportunities today.\r\n");
  	  return;
        }
  }
  
  af[0].location = APPLY_STR;
  af[0].type = SPELL_AFF_DEFENSIVE_STANCE;
  af[0].duration = ability_mod_value(GET_CON(ch)) + 5;
  af[0].bitvector = 0;
  af[0].modifier = 2;

  af[1].location = APPLY_CON;
  af[1].type = SPELL_AFF_DEFENSIVE_STANCE;
  af[1].duration = ability_mod_value(GET_CON(ch)) + 5;
  af[1].bitvector = 0;
  af[1].modifier = 4;

  af[2].location = APPLY_AC_DODGE;
  af[2].type = SPELL_AFF_DEFENSIVE_STANCE;
  af[2].duration = ability_mod_value(GET_CON(ch)) + 5;
  af[2].bitvector = 0;
  af[2].modifier = 40;

  af[3].location = APPLY_ALLSAVES;
  af[3].type = SPELL_AFF_DEFENSIVE_STANCE;
  af[3].duration = ability_mod_value(GET_CON(ch)) + 5;
  af[3].bitvector = 0;
  af[3].modifier = 2;

  af[4].location = APPLY_HIT;
  af[4].type = SPELL_AFF_DEFENSIVE_STANCE;
  af[4].duration = ability_mod_value(GET_CON(ch)) + 5;
  af[4].bitvector = 0;
  af[4].modifier = GET_CLASS_LEVEL(ch) * 2;

  GET_HIT(ch) += GET_CLASS_LEVEL(ch) * 2;

  for (i = 0; i < 5; i++) {
    affect_join(ch, af+i, false, false, false, false);
  }

  GET_DEFENSIVE_STANCE(ch) -= 1;

  if (is_innate_ready(ch, SPELL_AFF_DEFENSIVE_STANCE))
    add_innate_timer(ch, SPELL_AFF_DEFENSIVE_STANCE);
  
  send_to_char(ch, "You set your feet and brace your shoulders as you enter a defenstive stance.@n\r\n");
  act("$n sets $s feet and braces $s shoulders as $e enters a defenstive stance.@.@n", false, ch, 0, 0, TO_ROOM);
  
  return;
}

ACMD(do_breath_weapon) {
  char arg[100];
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to use your breath weapon on?\r\n");
      return;
    }
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to use your breath weapon on?\r\n");
      return;
    }
    vict = FIGHTING(ch);
  }

  if (!HAS_FEAT(ch, FEAT_BREATH_WEAPON)) {
    send_to_char(ch, "You do not have the breath weapon ability.\r\n");
    return;
  }

  if (GET_INNATE(ch, SPELL_BREATH_WEAPON) < 1) {
        if (is_innate_ready(ch, SPELL_BREATH_WEAPON))
          GET_INNATE(ch, SPELL_BREATH_WEAPON) = HAS_FEAT(ch, FEAT_BREATH_WEAPON);
        else {
    	  send_to_char(ch, "You have already used up all of your breath weapon opportunities today.\r\n");
  	  return;
        }
  }
  

  GET_INNATE(ch, SPELL_BREATH_WEAPON) -= 1;

  if (is_innate_ready(ch, SPELL_BREATH_WEAPON)) {
    add_innate_timer(ch, SPELL_BREATH_WEAPON);
  }

  call_magic(ch, vict, 0, SPELL_BREATH_WEAPON, GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE), CAST_SPELL, argument);

  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);

  return;
}

ACMD(do_dragonbreath) {
  char arg[100];
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to use your mount's breath weapon on?\r\n");
      return;
    }
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to use your mount's breath weapon on?\r\n");
      return;
    }
    vict = FIGHTING(ch);
  }

  if (!HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH)) {
    send_to_char(ch, "You do not have the dragon mount breath weapon ability.\r\n");
    return;
  }

  if (!IS_MOUNT_DRAGON(ch)) {
    send_to_char(ch, "Your mount is not a dragon and does not have a breath weapon.\r\n");
    return;
  }

  if (!can_use_available_actions(ch, ACTION_MOVE)) {
    return;
  }

  if (GET_INNATE(ch, SPELL_DRAGON_MOUNT_BREATH) < 1) {
    if (is_innate_ready(ch, SPELL_DRAGON_MOUNT_BREATH))
      GET_INNATE(ch, SPELL_DRAGON_MOUNT_BREATH) = HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH);
    else {
      send_to_char(ch, "You have already used up all of your mount's breath weapon opportunities today.\r\n");
      return;
    }
  }
  

  GET_INNATE(ch, SPELL_DRAGON_MOUNT_BREATH) -= 1;

  if (is_innate_ready(ch, SPELL_DRAGON_MOUNT_BREATH)) {
    add_innate_timer(ch, SPELL_DRAGON_MOUNT_BREATH);
  }

  call_magic(ch, vict, 0, SPELL_DRAGON_MOUNT_BREATH, GET_CLASS_RANKS(ch, CLASS_DRAGON_RIDER), CAST_SPELL, argument);

  return;
}

ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (GET_FORM_POS(ch) > FORM_POS_FRONT) {
    send_to_char(ch, "You must be fighting in the front row to rescue someone.\r\n");
    return;
  } 

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }
  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  percent = d20 + get_combat_bonus(ch);
  prob = get_combat_defense(tmp_ch) / 10;

  if (percent < prob) {
    send_to_char(ch, "You fail the rescue!\r\n");
		SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE);
    return;
  }
  send_to_char(ch, "Banzai!  To the rescue...\r\n");
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);	
  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);
  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE);

}

ACMD(do_turn_undead)
{
  struct char_data *victim, *tmp;
  int turned = 0;
  struct affected_type af;
	int maxLevel = 0;
	int maxHitdice = 0;
	int turnCheck = 0;
  long local_gold = 0;
  char local_buf[256];
  struct char_data *tmp_char;
  struct obj_data *corpse_obj, *coin_obj, *next_obj;
	struct follow_type *f = NULL;
	int controlledHitdice = 0;
	int undeadFound = FALSE;

	if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
	  OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
	  return ;
	}
	
  if (!HAS_FEAT(ch, FEAT_TURN_UNDEAD))
  {
    OUTPUT_TO_CHAR("You lack the ability to turn undead beings.\r\n", ch);
    return;
  }

	if (GET_TURN_UNDEAD(ch) < 1) {
	  send_to_char(ch, "You have no turn undead opportunities left.\r\n");
	  if (GET_TURN_UNDEAD(ch) < 1 && is_innate_ready(ch, SPELL_AFF_TURN_UNDEAD)) {
	    GET_TURN_UNDEAD(ch) = MAX(1, 3 + ability_mod_value(GET_CHA(ch)) + (HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2));		
			send_to_char(ch, "YOur turn undead opportunities have been refeshed!\r\n");
		}
		return;
	}	
	
	turnCheck = dice(1, 20) + ability_mod_value(GET_CHA(ch));

        maxLevel = GET_TLEVEL(ch) + GET_CLASS_RANKS(ch, CLASS_CLERIC) + MAX(0, (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2)) +
                   MAX(0, (GET_CLASS_RANKS(ch, CLASS_TEMPLAR) - 1)) + MAX(0, (GET_CLASS_RANKS(ch, CLASS_CHAMPION) - 3)) +
                   MAX(0, (GET_CLASS_RANKS(ch, CLASS_DRAGON_PRIEST) - 2));
	
	if (turnCheck <= 0)
          maxLevel += -4;
	else if (turnCheck <= 3)
          maxLevel += -3;
	else if (turnCheck <= 6)
          maxLevel += -2;
	else if (turnCheck <= 9)
          maxLevel += -1;
	else if (turnCheck <= 12)
          maxLevel += 0;
	else if (turnCheck <= 15)
          maxLevel += 1;
	else if (turnCheck <= 18)
          maxLevel += 2;
	else if (turnCheck <= 21)
          maxLevel += 3;
	else
          maxLevel += 4;
	
	maxHitdice = dice(2, 6) + GET_TLEVEL(ch) + GET_CLASS_RANKS(ch, CLASS_CLERIC) + (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2) 
                     + ability_mod_value(GET_CHA(ch)) + HAS_FEAT(ch, FEAT_EXCEPTIONAL_TURNING) ? dice(1, 10) : 0;	
	
  for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room)
  {
    victim = tmp;
		
    if (!IS_UNDEAD(victim))
      continue;
		else
		  undeadFound = TRUE;


    if (IS_GOOD(ch) || PRF_FLAGGED(ch, PRF_POSITIVE)) {
	    if ((GET_HITDICE(victim) <= maxLevel) && ((maxHitdice - GET_HITDICE(victim)) > 0) && !((affected_by_spell(victim, SPELL_BRAVERY)) || (affected_by_spell(victim, SPELL_IMPERVIOUS_MIND))))
	    {
			  if ((GET_TLEVEL(ch) + GET_CLASS_RANKS(ch, CLASS_CLERIC) + (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2)) 
                               >= GET_HITDICE(victim) * 2) {
				turned = 1;
				
				  maxHitdice -= GET_HITDICE(victim);
				
				  act("$n has been destroyed.", FALSE, victim, 0, 0, TO_ROOM);
				
				  if (ch != victim && (IS_NPC(victim) || victim->desc)) {
			      if (AFF_FLAGGED(ch, AFF_GROUP))
				      group_gain(ch, victim);
			      else
			        solo_gain(ch, victim);
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
			    if (IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
			        PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {
			      generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
			      if (corpse_obj) {
			        for (coin_obj = corpse_obj->contains; coin_obj; coin_obj = next_obj) {
			          next_obj = coin_obj->next_content;
			          if (CAN_SEE_OBJ(ch, coin_obj) && isname("coin", coin_obj->name))
			            extract_obj(coin_obj);
			        }
			        do_split(ch,local_buf,0,0);
			      }
			      /* need to remove the gold from the corpse */
			    } else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {      
			    	do_get(ch, "all.coin corpse", 0, 0);
			    }
			    if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
			      do_get(ch, "all corpse", 0, 0);
			    }
			    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC)) { 
			       do_sac(ch,"corpse",0,0); 
			    } 
		  			
				}
				else {
				  act("$n has been turned.", FALSE, victim, 0, 0, TO_ROOM);
				  maxHitdice -= GET_HITDICE(victim);
		      turned = 1;
		      af.type = SPELL_INSPIRE_FEAR;
		      af.duration = 10;
		      af.modifier = -20;
		      af.location = APPLY_AC;
		      af.bitvector = 0;
		      affect_join(victim, &af, 0, 0, 0, 0);
			}
	    }
	  }
    else {
	    if ((GET_HITDICE(victim) <= maxLevel) && ((maxHitdice - GET_HITDICE(victim)) > 0) && !((affected_by_spell(victim, SPELL_BRAVERY)) || (affected_by_spell(victim, SPELL_IMPERVIOUS_MIND))))
	    {
			  if ((GET_CLASS_RANKS(ch, CLASS_CLERIC) + (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2)) >= GET_HITDICE(victim) * 2) {
		      
					for (f = ch->followers; f; f = f->next) {
					  if (affected_by_spell(f->follower, SPELL_CONTROL_UNDEAD))
						  controlledHitdice += GET_HITDICE(f->follower);
					}
					
					if (controlledHitdice + GET_HITDICE(victim) > GET_CLASS_RANKS(ch, CLASS_CLERIC) + (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2)) {
					  continue;
					}
					
					turned = 1;
				
				  maxHitdice -= GET_HITDICE(victim);
				
		      af.type      = SPELL_CONTROL_UNDEAD;
		      af.location  = APPLY_NONE;
		      af.bitvector = AFF_CHARM;
		      af.modifier  = 0;
		      af.duration  = 500;

		      if (victim->master)
		        stop_follower(victim);

		      add_follower(victim, ch);
		      affect_to_char(victim, &af);

		      act("You have managed to control $N.", FALSE, ch, 0, victim, TO_CHAR);
		      act("You find yourself controlled by $n!", FALSE, ch, 0, victim, TO_VICT);
		      act("$n has managed to gain control over $N.", 1, ch, 0, victim, TO_ROOM);
				
				}
				else {
				  act("$n has been rebuked.", FALSE, victim, 0, 0, TO_ROOM);
				  maxHitdice -= GET_HITDICE(victim);
		      turned = 1;
		      af.type = SPELL_INSPIRE_AWE;
		      af.duration = 10;
		      af.modifier = -20;
		      af.location = APPLY_AC;
		      af.bitvector = 0;
		      affect_join(victim, &af, 0, 0, 0, 0);
				}
	    }
    }
	}

	if (IS_EVIL(ch) || PRF_FLAGGED(ch, PRF_NEGATIVE)) {
		if (turned)
		{
			act("You have managed to rebuke some undead.", 0, ch, 0, 0, TO_CHAR);
			act("$n has managed to rebuke undead.", 0, ch, 0, 0, TO_ROOM);
		} else
		{
			act("You fail to rebuke any undead.", 0, ch, 0, 0, TO_CHAR);
			act("$n tries to rebuke undead, but fails.", 0, ch, 0, 0, TO_ROOM);
		}
	}
	else {
		if (turned)
		{
			act("You have managed to turn some undead.", 0, ch, 0, 0, TO_CHAR);
			act("$n has managed to turn undead.", 0, ch, 0, 0, TO_ROOM);
		} else
		{
			act("You fail to turn any undead.", 0, ch, 0, 0, TO_CHAR);
			act("$n tries to turn undead, but fails.", 0, ch, 0, 0, TO_ROOM);
		}			
	}
	
	if (!undeadFound)
	  send_to_char(ch, "No undead were found.\r\n");
	
	GET_TURN_UNDEAD(ch) -= 1;
	
  if (is_innate_ready(ch, SPELL_AFF_TURN_UNDEAD)) {
    add_innate_timer(ch, SPELL_AFF_TURN_UNDEAD);
  }	

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;
	
  SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE);
  return;
}

ACMD(do_divine_feats)
{
  struct affected_type af;
  char arg1[200];
  

  one_argument(argument, arg1);

  if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING) {
    OUTPUT_TO_CHAR("You are not in a proper position for that!\r\n", ch);
    return ;
  }
	
  if (!HAS_FEAT(ch, FEAT_DIVINE_MIGHT) && !HAS_FEAT(ch, FEAT_DIVINE_SHIELD) && !HAS_FEAT(ch, FEAT_DIVINE_VENGEANCE))
  {
    OUTPUT_TO_CHAR("You lack the ability to use divine feats.\r\n", ch);
    return;
  }

  if (GET_TURN_UNDEAD(ch) < 1) {
    send_to_char(ch, "You have no turn undead opportunities left.  Divine feats use turn undead usages.\r\n");
    if (GET_TURN_UNDEAD(ch) < 1 && is_innate_ready(ch, SPELL_AFF_TURN_UNDEAD)) {
      GET_TURN_UNDEAD(ch) = MAX(1, 3 + ability_mod_value(GET_CHA(ch)) + (HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2));		
 	send_to_char(ch, "YOur turn undead opportunities have been refeshed!  Divine feats use turn undead usages.\r\n");
    }
    return;
  }	
	
  if (!*arg1) {
    send_to_char(ch, "You must specify either 'might', 'shield', or 'vengeance'.\r\n");
    return;
  }

  if (!strcmp(arg1, "might")) {

    if (!HAS_FEAT(ch, FEAT_DIVINE_MIGHT)) {
      send_to_char(ch, "You do not have the divine might feat.\r\n");
      return;
    }

    af.type      = SPELL_FEAT_DIVINE_MIGHT;
    af.location  = APPLY_DAMROLL;
    af.bitvector = 0;
    af.modifier  = MAX(1, ability_mod_value(GET_CHA(ch)));
    af.duration  = MAX(1, ability_mod_value(GET_CHA(ch)));

    affect_to_char(ch, &af);


  } else if (!strcmp(arg1, "shield")) {

    if (!HAS_FEAT(ch, FEAT_DIVINE_SHIELD)) {
      send_to_char(ch, "You do not have the divine might feat.\r\n");
      return;
    }

    af.type      = SPELL_FEAT_DIVINE_SHIELD;
    af.location  = APPLY_AC_DEFLECTION;
    af.bitvector = 0;
    af.modifier  = MAX(1, ability_mod_value(GET_CHA(ch))) * 10;
    af.duration  = MAX(1, ability_mod_value(GET_CHA(ch)));

    affect_to_char(ch, &af);

  } else if (!strcmp(arg1, "vengeance")) {

    if (!HAS_FEAT(ch, FEAT_DIVINE_VENGEANCE)) {
      send_to_char(ch, "You do not have the divine might feat.\r\n");
      return;
    }

    af.type      = SPELL_FEAT_DIVINE_MIGHT;
    af.location  = APPLY_ABILITY;
    af.bitvector = 0;
    af.modifier  = 1;
    af.duration  = MAX(1, ability_mod_value(GET_CHA(ch)));

    affect_to_char(ch, &af);

  } else {
    send_to_char(ch, "You must specify either 'might', 'shield', or 'vengeance'.\r\n");
    return;
  }


	
  GET_TURN_UNDEAD(ch) -= 1;
	
  if (is_innate_ready(ch, SPELL_AFF_TURN_UNDEAD)) {
    add_innate_timer(ch, SPELL_AFF_TURN_UNDEAD);
  }	
	
  SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE);
  return;
}

ACMD(do_kick)
{
  struct char_data *vict;
  int percent, prob;
  char buf[100]={'\0'};
  char arg[100]={'\0'};

  one_argument(argument, arg);

  if (!*arg) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to kick?\r\n");
      return;
    }
  }

  if (!can_use_available_actions(ch, ACTION_MOVE)) {        
    send_to_char(ch, "You have no available actions to do a kick.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (!(FIGHTING(ch))) {
      send_to_char(ch, "Who do you want to kick?\r\n");
      return;
    }
    vict = FIGHTING(ch);
  }

  if (ch->combat_pos != vict->combat_pos) {
    send_to_char(ch, "You are not close enough to attempt a kick.\r\n");
    return;
  }

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;

  if (IN_ROOM(ch) != IN_ROOM(vict))
    return;

  if (HAS_FEAT(vict, FEAT_SPRING_ATTACK) &&
        (GET_TOTAL_AOO(vict) < MAX(1, HAS_FEAT(vict, FEAT_COMBAT_REFLEXES) ? ability_mod_value(GET_DEX(vict)): 1))) {
        do_attack_of_opportunity(vict, ch, "Spring Attack");
  }

  percent = d20 + get_combat_bonus(ch);
  prob = get_combat_defense(vict) / 10;

  if (AFF_FLAGGED(ch, AFF_SPIRIT) || AFF_FLAGGED(vict, AFF_SPIRIT) || prob > percent) {

    act("You try to kick $N but miss.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to kick you but misses.", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to kick $N but misses.", FALSE, ch, 0, vict, TO_NOTVICT);
    
    if (percent <= 5)
      SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);
    return;
  }

  int dam;

  dam = MAX(1, dice(1, 2) + ability_mod_value(GET_STR(ch)));

  sprintf(buf, "@yYou successfully kick $N for @Y%d@R damage.@n", dam);
  act(buf, FALSE, ch, 0, vict, TO_CHAR);
  sprintf(buf, "@R$n successfully kicks you for @Y%d@R damage.@n", dam);
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  sprintf(buf, "@n$n successfully kicks $N for @Y%d@n damage.@n", dam);
  act(buf, FALSE, ch, 0, vict, TO_NOTVICT);

  if (percent <= 5)
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
  WAIT_STATE(ch, CONFIG_PULSE_VIOLENCE * 2);  

  damage(ch, vict, dam, 0, SKILL_KICK, -1, 0, SKILL_KICK, 1);
}
ACMD(do_deathattack)
{

  struct char_data *vict = NULL;
  char arg1[100], arg2[100];
  int death_attack = TRUE;

  if (!HAS_FEAT(ch, FEAT_DEATH_ATTACK)) {
      send_to_char(ch, "You do not know how to perform a death attack.\r\n");
      return;
  }

  if (FIGHTING(ch)) {
    send_to_char(ch, "You cannot perform this action in combat.\r\n");
    return;
  }

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Who would you like to use your death attack on?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person isn't here right now.\r\n");
    return;
  }

  if (*arg2) {
    if (is_abbrev(arg2, "death"))
      death_attack = TRUE;    
    else if (is_abbrev(arg2, "paralysis"))
      death_attack = FALSE;
    else {
      send_to_char(ch, "You must choose either 'death' or 'paralysis' for your death attack type.\r\n");
      return;
    }
  }

  if (death_attack) {
    GET_DEATH_ATTACK(ch) = 1;
  }
  else {
    GET_DEATH_ATTACK(ch) = 2;
  }

  send_to_char(ch, "You creep up on your prey with your intended death attack...\r\n");

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;


  hit(ch, vict, TYPE_UNDEFINED);

  GET_MARK_ROUNDS(ch) = 0;
  GET_MARK(ch) = NULL;

}

ACMD(do_mark)
{
  struct char_data *vict = NULL;
  char arg[100];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Who would you like to mark for assassination?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person isn't here right now.\r\n");
    return;
  }

  act("You begin to mark $N for assassination.", FALSE, ch, 0, vict, TO_CHAR);
  GET_MARK(ch) = vict;

}

ACMD(do_undeathtouch)
{

  if (!HAS_FEAT(ch, FEAT_TOUCH_OF_UNDEATH)) {
      send_to_char(ch, "You do not know how to perform a touch of undeath.\r\n");
      return;
  }

  if (GET_INNATE(ch, SPELL_TOUCH_OF_UNDEATH) < 1) {
        if (is_innate_ready(ch, SPELL_TOUCH_OF_UNDEATH))
          GET_INNATE(ch, SPELL_TOUCH_OF_UNDEATH) = GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER);
        else {
    	  send_to_char(ch, "You have already used up all of your touch of undeath opportunities today.\r\n");
  	  return;
        }
  }
  struct char_data *vict = NULL;
  char arg1[100], arg2[100];
  int undeath_touch = TRUE;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Who would you like to use your touch of undeath on?\r\n");
    return;
  }

  // only works if you have a hand free or wielding a scythe

  if (GET_EQ(ch, WEAR_WIELD1) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0) != WEAPON_TYPE_SCYTHE &&
      ((GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0) != WEAPON_TYPE_SCYTHE) || 
      GET_EQ(ch, WEAR_SHIELD) || GET_OBJ_SIZE(GET_EQ(ch, WEAR_WIELD1)) > get_size(ch))) {
    send_to_char(ch, "You can only use touch of undeath while wielding a scythe or if you have a free hand.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person isn't here right now.\r\n");
    return;
  }

  if (*arg2) {
    if (is_abbrev(arg2, "death"))
      undeath_touch = TRUE;    
    else if (is_abbrev(arg2, "paralysis"))
      undeath_touch = FALSE;
    else {
      send_to_char(ch, "You must choose either 'death' or 'paralysis' for your touch of undeath.\r\n");
      return;
    }
  }

  if (undeath_touch) {
    if (GET_INNATE(ch, SPELL_TOUCH_OF_UNDEATH) < 3) {
        send_to_char(ch, "You don't have enough uses left to do a death touch of undeath.\r\n");
        return;
    }
    GET_UNDEATH_TOUCH(ch) = 1;
  }
  else {
    GET_UNDEATH_TOUCH(ch) = 2;
  }

  send_to_char(ch, "You prepare yourself to deliver a touch of undeath.\r\n");

  struct char_data *tch;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (is_player_grouped(ch, tch) && !IS_NPC(tch) && FIGHTING(tch) == NULL)
      tch->exp_chain = 0;


  hit(ch, vict, TYPE_UNDEFINED);

  GET_INNATE(ch, SPELL_TOUCH_OF_UNDEATH) -= undeath_touch ? 3 : 1;

  if (is_innate_ready(ch, SPELL_TOUCH_OF_UNDEATH) && GET_INNATE(ch, SPELL_TOUCH_OF_UNDEATH) <= 0) {
    add_innate_timer(ch, SPELL_TOUCH_OF_UNDEATH);
  }

}

ACMD(do_attack) {

  if (ch->paralyzed) {
    send_to_char(ch, "You cannot act as you are paralyzed!\r\n");
    return;
  }

  extern int circle_copyover;

  if (circle_copyover && !FIGHTING(ch)) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin combat.\r\n");
    return;
  }


  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg || FIGHTING(ch)) {
    if (!FIGHTING(ch)) {
      send_to_char(ch, "You need to specify who you would like to attack first.\r\n");
    } else if (ch->standard_action_spent) {
      send_to_char(ch, "You have already spent your standard action this round.\r\n");
    } else if (in_range(ch)) {
      if (AFF_FLAGGED(ch, AFF_NEXTNOACTION)) {
        send_to_char(ch, "You can't attack this round after the other actions you've performed.\r\n");
        return;
     }
      if (!PRF_FLAGGED(ch, PRF_AUTOATTACK))
        act("You attack $N!\r\n", false, ch, 0, FIGHTING(ch), TO_CHAR);
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
      ch->standard_action_spent++;
    } else {
      send_to_char(ch, "You are not close enough to attack that target yet\r\n");
      return;
    }
  } else {
    vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);
    if (vict) {
      if (subcmd == SCMD_MURDER) {
        if (PRF_FLAGGED(ch, PRF_PVP) && PRF_FLAGGED(vict, PRF_PVP)) {
          if (get_highest_group_level(ch) > get_highest_group_level(vict)) {
            send_to_char(ch, "Your highest group level is %d, while your opponent's highest group level is %d.  You must mentor down to that level to fight.\r\n",
                         get_highest_group_level(ch), get_highest_group_level(vict));
            return;
          }
          send_to_char(ch, "Your pvp timer has been set to 30 minutes.\r\n");
          ch->pvp_timer = 30;
          send_to_char(vict, "Your pvp timer has been set to 30 minutes.\r\n");
          vict->pvp_timer = 30;
        } else if (!PRF_FLAGGED(ch, PRF_PVP)) {
          send_to_char(ch, "You do not have your pvp flag enabled. (type pvp to enable it).\r\n");
          return;
        } else {
          send_to_char(ch, "That person does not have their pvp flag enabled.\r\n");
          return;
        }
      }

      if (is_player_grouped(vict, ch)) {
        send_to_char(ch, "You cannot attack people in your own group.\r\n");
        return;
      }
      if (!IS_NPC(vict) && !IS_NPC(ch) && subcmd != SCMD_MURDER) {
        send_to_char(ch, "You have to use the murder command to attack a player.\r\n");
        return;
      }
      if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
        send_to_char(ch, "You cannot attack that creature.\r\n");
        return;
      }
      set_fighting(ch, vict);
      if (!FIGHTING(vict))
        set_fighting(vict, ch);
      struct char_data *tch;
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
        if (is_player_grouped(ch, tch) && tch != ch && PRF_FLAGGED(tch, PRF_AUTOASSIST))
          set_fighting(tch, vict);
      ch->standard_action_spent = 0;
    } else {
      send_to_char(ch, "There doesn't seem to be anyone around by that description for you to attack.\r\n");
    }
  }
 
}

int in_range(struct char_data *ch) 
{

  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);

  int distance = 0;
  int range = 0;

  if (FIGHTING(ch)) {
    if ((distance = (ch->combat_pos - FIGHTING(ch)->combat_pos)) > 0) {
      if (wielded) {
        range = weapon_list[GET_OBJ_VAL(wielded, 0)].range;
        if (distance >= (range * 3)) {
          return FALSE;
        }
      }
      else
        return FALSE;
    }
    else if ((distance = (FIGHTING(ch)->combat_pos - ch->combat_pos)) > 0) {
      if (wielded) {
        range = weapon_list[GET_OBJ_VAL(wielded, 0)].range;
        if (distance >= (range * 3)) {
          return FALSE;
        }
      }
      else
        return FALSE;
    }
  }
  else
    return FALSE;

  return TRUE;
}

ACMD(do_approach) {

  if (!check_active_turn(ch)) {
    send_to_char(ch, "It is not your turn in combat yet.\r\n");
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char(ch, "Why approach?  You aren't even fighting anyone.\r\n");
    return;
  }

      if (IS_NPC(FIGHTING(ch)) || IS_NPC(ch) || TRUE) {
        if ((FIGHTING(ch)->combat_pos - ch->combat_pos) > 0) {
          if (ch->full_round_action) {
            if (ch->move_action_spent == 0) {
              send_to_char(ch, "You take a 5-foot step towards your opponent.\r\n");
              ch->combat_pos += MIN(5, FIGHTING(ch)->combat_pos - (ch)->combat_pos);
              ch->move_action_spent = 1;
            } else {
              send_to_char(ch, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (ch->move_action_spent == 0) {
              send_to_char(ch, "You move %d feet towards your opponent.",
                              MIN(get_speed(ch), FIGHTING(ch)->combat_pos - ch->combat_pos));
              ch->combat_pos += MIN(get_speed(ch), FIGHTING(ch)->combat_pos - ch->combat_pos);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              FIGHTING(ch)->combat_pos - ch->combat_pos);
              ch->move_action_spent = 1;
            } else if (ch->standard_action_spent == 0) {
              send_to_char(ch, "You move %d feet towards your opponent.",
                              MIN(get_speed(ch), FIGHTING(ch)->combat_pos - ch->combat_pos));
              ch->combat_pos += MIN(get_speed(ch), FIGHTING(ch)->combat_pos - ch->combat_pos);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              FIGHTING(ch)->combat_pos - ch->combat_pos);
              ch->standard_action_spent = 1;
            } else {
              send_to_char(ch, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
        } else if ((FIGHTING(ch)->combat_pos - ch->combat_pos) < 0) {
          if (ch->full_round_action) {
            if (ch->move_action_spent == 0) {
              send_to_char(ch, "You take a 5-foot step towards your opponent.\r\n");
              ch->combat_pos -= MIN(5, ch->combat_pos - FIGHTING(ch)->combat_pos);
              ch->move_action_spent = 1;
            } else {
              send_to_char(ch, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (ch->move_action_spent == 0) {
              send_to_char(ch, "You move %d feet towards your opponent.",
                              MIN(get_speed(ch), (ch)->combat_pos - FIGHTING(ch)->combat_pos));
              ch->combat_pos -= MIN(get_speed(ch), (ch)->combat_pos - FIGHTING(ch)->combat_pos);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              (ch)->combat_pos - FIGHTING(ch)->combat_pos);
              ch->move_action_spent = 1;
            } else if (ch->standard_action_spent == 0) {
              send_to_char(ch, "You move %d feet towards your opponent.",
                              MIN(get_speed(ch), (ch)->combat_pos - FIGHTING(ch)->combat_pos));
              ch->combat_pos -= MIN(get_speed(ch), (ch)->combat_pos - FIGHTING(ch)->combat_pos);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              (ch)->combat_pos - FIGHTING(ch)->combat_pos);
              ch->standard_action_spent = 1;
            } else {
              send_to_char(ch, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
        } else {
          send_to_char(ch, "You cannot get any closer to your opponent.\r\n");
        }
      } else {
        send_to_char(ch, "Movement is not yet implemented for player-vs-player combat.\r\n");
      }
}

ACMD(do_retreat) {

  if (!check_active_turn(ch)) {
    send_to_char(ch, "It is not your turn in combat yet.\r\n");
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char(ch, "Why retreat?  You aren't even fighting anyone.\r\n");
    return;
  }

  if (IS_NPC(FIGHTING(ch)) || IS_NPC(ch) || TRUE) {
    if ((FIGHTING(ch)->combat_pos - (ch)->combat_pos) >= 0) {
          if (ch->full_round_action) {
            if (can_use_available_actions(ch, ACTION_MOVE)) {        
              send_to_char(ch, "You take a 5-foot step away from your opponent.\r\n");
              ch->combat_pos -= 5;
            } else {
              send_to_char(ch, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (can_use_available_actions(ch, ACTION_MOVE)) {        
              send_to_char(ch, "You move %d feet away from your opponent.",
                              get_speed(ch));
              ch->combat_pos -= get_speed(ch);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              FIGHTING(ch)->combat_pos - ch->combat_pos);
              ch->move_action_spent = 1;
            } else {
              send_to_char(ch, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
    } else {
          if (ch->full_round_action) {
            if (can_use_available_actions(ch, ACTION_MOVE)) {        
              send_to_char(ch, "You take a 5-foot step away from your opponent.\r\n");
              ch->combat_pos += 5;
            } else {
              send_to_char(ch, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (can_use_available_actions(ch, ACTION_MOVE)) {        
              send_to_char(ch, "You move %d feet away from your opponent.",
                              get_speed(ch));
              ch->combat_pos += get_speed(ch);
              send_to_char(ch, "  You are now %d feet away from each other.\r\n",
                              (ch)->combat_pos - FIGHTING(ch)->combat_pos);
            } else {
              send_to_char(ch, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
    }
  } else {
    send_to_char(ch, "Movement is not yet implemented for player-vs-player combat.\r\n");
  }
}

ACMD(do_endturn)
{

  if (!check_active_turn(ch)) {
    send_to_char(ch, "It is not your turn in combat yet.\r\n");
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char(ch, "You are not even fighting.\r\n");
    return;
  }

  if (!ch->standard_action_spent && !AFF_FLAGGED(ch, AFF_NEXTNOACTION) && GET_POS(ch) >= POS_FIGHTING) {
    send_to_char(ch, "@YYou take on the total defense combat stance.\r\n");
    ch->total_defense = 1;
  }

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
  
  ch->active_turn = 0;
  ch->initiative = 0;
  ch->standard_action_spent = 0;
  ch->move_action_spent = 0;
  ch->minor_action_spent = 0;
  ch->full_round_action = 0;

  find_next_combat_target(ch);

}

void find_next_combat_target(struct char_data *ch)
{

  ch->round_num++;

  struct char_data *tch = NULL, *next_tch = NULL, *high = NULL;
  int highest = 0;
  int new_round = FALSE;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;

      if (tch == ch)
        continue;

      if (tch->round_num >= ch->round_num)
        new_round = TRUE;
    }

    if (!new_round) {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        tch->damage_taken_last_round = GET_DAMAGE_DEALT_THIS_ROUND(tch) = 0;
      }
    }

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;

      if (tch == ch)
        continue;

      if (tch->round_num >= ch->round_num)
        continue;

      if (is_player_grouped(ch, tch) || is_player_grouped(FIGHTING(tch), ch) || FIGHTING(ch) == tch || is_player_grouped(FIGHTING(ch), tch)) {
        if (tch->initiative > highest) {
          highest = tch->initiative;
          high = tch;
        }
      }
    }

  if (!high) {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;

      if (is_player_grouped(ch, tch) || is_player_grouped(FIGHTING(tch), ch) || FIGHTING(ch) == tch || is_player_grouped(FIGHTING(ch), tch)) {
        roll_initiative(tch);
      }
    }    
    highest = 0;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;

      if (tch == ch)
        continue;

      if (is_player_grouped(ch, tch) || is_player_grouped(FIGHTING(tch), ch) || FIGHTING(ch) == tch || is_player_grouped(FIGHTING(ch), tch)) {
        if (tch->initiative > highest) {
          highest = tch->initiative;
          high = tch;
        }
      }
    }
    if (!high)
      FIGHTING(ch) = NULL;
  }

  if (!high || ((!FIGHTING(ch) || (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) != ch)) && GET_POS(ch) >= POS_RESTING)) {
  struct char_data *k = NULL, *temp = NULL;
  struct follow_type *f = NULL;
  ubyte found = FALSE;

      for (k = world[IN_ROOM(ch)].people; k; k = temp) {
        temp = k->next_in_room;
        if (FIGHTING(k) == ch || (FIGHTING(k) && is_player_grouped(FIGHTING(k), ch))) {
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
        return;
      }
  }

  if (!high)
    high = ch;

  if (!high)
    return;
  
  display_your_turn(high);
  fight_action(high);
  if (IS_NPC(high)) {
    perform_mob_combat_turn(high);
    find_next_combat_target(high);
    high->initiative = 0;
    high->active_turn = 0;
  } else {
    high->active_turn = 1;
  }

  
}

void perform_mob_combat_turn(struct char_data *ch) 
{

  if (GET_POS(ch) < POS_RESTING)
    return;

  char buf[MAX_INPUT_LENGTH]={'\0'};

  if (!IS_NPC(ch))
    return;

  if (ch->master) {
        if ((FIGHTING(ch)->combat_pos - (ch)->combat_pos) > 0) {
          ch->combat_pos += MIN(FIGHTING(ch)->combat_pos - (ch)->combat_pos, get_speed(ch));
	  sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), (FIGHTING(ch)->combat_pos - (ch)->combat_pos));
          act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
          if ((FIGHTING(ch)->combat_pos - (ch)->combat_pos) > 0) {
            ch->combat_pos += MIN((ch)->combat_pos - FIGHTING(ch)->combat_pos, get_speed(ch));
            sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), (FIGHTING(ch)->combat_pos - (ch)->combat_pos));
            act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          }
          else {
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
          }
        } else if ((FIGHTING(ch)->combat_pos - (ch)->combat_pos) < 0) {
          ch->combat_pos -= MIN((ch)->combat_pos - FIGHTING(ch)->combat_pos, get_speed(ch));
	  sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), ((ch)->combat_pos - FIGHTING(ch)->combat_pos));
          act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
          if (((ch)->combat_pos - FIGHTING(ch)->combat_pos) < 0) {
            ch->combat_pos -= MIN(FIGHTING(ch)->combat_pos - (ch)->combat_pos, get_speed(ch));
            sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), (FIGHTING(ch)->combat_pos - (ch)->combat_pos));
            act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          }
          else {
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
          }
        } else {
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
        }
  }
  else {
        if (FIGHTING(ch) && ((ch)->combat_pos - FIGHTING(ch)->combat_pos) > 0) {
          ch->combat_pos -= MIN((ch)->combat_pos - FIGHTING(ch)->combat_pos, get_speed(ch));
	  sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), ((ch)->combat_pos - FIGHTING(ch)->combat_pos));
          act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
          if (((ch)->combat_pos - FIGHTING(ch)->combat_pos) > 0) {
            ch->combat_pos -= MIN((ch)->combat_pos - FIGHTING(ch)->combat_pos, get_speed(ch));
            sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), ((ch)->combat_pos - FIGHTING(ch)->combat_pos));
            act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          }
          else {
            decide_mobile_special_action(ch);
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
          }
        } else if (FIGHTING(ch) && ((ch)->combat_pos - FIGHTING(ch)->combat_pos) < 0) {
          ch->combat_pos += MIN(FIGHTING(ch)->combat_pos - (ch)->combat_pos, get_speed(ch));
          sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), (FIGHTING(ch)->combat_pos - (ch)->combat_pos));
          act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
          if (((ch)->combat_pos - FIGHTING(ch)->combat_pos) < 0) {
            ch->combat_pos += MIN(FIGHTING(ch)->combat_pos - (ch)->combat_pos, get_speed(ch));
            sprintf(buf, "$n moves %d feet towards $N.  They are now %d feet apart.\r\n",
                  get_speed(ch), (FIGHTING(ch)->combat_pos - (ch)->combat_pos));
            act(buf, TRUE, ch, 0, FIGHTING(ch), TO_ROOM);
          }
          else {
            decide_mobile_special_action(ch);
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
          }
        }
        else {
          decide_mobile_special_action(ch);
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
        }
  }

  ch->parries = 0;
  ch->parried_attacks = 0;
}

void perform_pc_combat_turn(struct char_data *ch) 
{

  struct char_data *target = NULL;

  if (GET_POS(ch) < POS_RESTING)
    return;

  if (IS_NPC(ch))
    return;

  if (FIGHTING(ch)) {
    FIGHTING(ch)->combat_pos = MIN(200, MAX(0, FIGHTING(ch)->combat_pos));
    target = FIGHTING(ch);
  }

  if (!FIGHTING(ch) || GET_POS(FIGHTING(ch)) == POS_DEAD)
    return;

  if (FIGHTING(ch) && !FIGHTING(FIGHTING(ch)))
    return;

  if (FIGHTING(FIGHTING(ch)) && !is_player_grouped(FIGHTING(FIGHTING(ch)), ch))
    return;

  if (in_range(ch)) {
    do_attack(ch, "", 0, 0);
  }
  else {
    do_approach(ch, "", 0, 0);
    if (in_range(ch)) {
      do_attack(ch, "", 0, 0);
    }
    else {
      do_approach(ch, "", 0, 0);
    }
  }
  ch->parries = 0;
  ch->parried_attacks = 0;
}

int check_active_turn(struct char_data *ch) {

  if (ch->active_turn == 1)
    return TRUE;

  struct char_data *tch = NULL, *next_tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    if (tch == ch)
      continue;

    if (is_player_grouped(ch, tch) || is_player_grouped(FIGHTING(tch), ch)) {
      if (tch->active_turn == 1)
        return FALSE;
    }
  }

  ch->active_turn = 1;
  return TRUE;

}

ACMD(do_coupdegrace)
{

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  if (FIGHTING(ch)) {
    vict = FIGHTING(ch);
  }
  else {
    one_argument(argument, arg);
    if (!*arg) {
      send_to_char(ch, "Who do you wish to perform a coup-de-grace against?\r\n");
      return;
    }
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "There doesn't seem to be anyone around here by that name.\r\n");
      return;
    }
  }

  if (GET_POS(ch) >= POS_SITTING) {
    send_to_char(ch, "You can't perform a coup de grace against an opponent who isn't helpless!\r\n");
    return;
  }

  act("You perform a coup de grace against $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n perform a coup de grace against you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n perform a coup de grace against $N.", FALSE, ch, 0, vict, TO_NOTVICT);

  vict->coupdegrace=1;

  one_hit(ch, vict, GET_EQ(ch, WEAR_WIELD), TYPE_UNDEFINED, 9999999, NULL, NULL, 9999999);

}

void display_your_turn(struct char_data *ch)
{

  if (!ch)
    return;

  struct char_data *victim;

  if ((victim = FIGHTING(ch))) {
    if (GET_HIT(victim) < -10 || GET_POS(victim) == POS_DEAD) {
      struct char_data *k = NULL, *temp = NULL;
      struct follow_type *f = NULL;
      ubyte found = FALSE;

      for (k = world[IN_ROOM(ch)].people; k; k = temp) {
        temp = k->next_in_room;
        if (FIGHTING(k) == ch || (FIGHTING(k) && is_player_grouped(FIGHTING(k), ch))) {
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
        return;
      }
      return;
    }
  }

  char buf[200]={'\0'};

  ch->total_defense = 0;
  ch->weapon_supremacy_miss = 0;
  ch->opportunist = 0;

  GET_HIT(ch) += HAS_FEAT(ch, FEAT_FAST_HEALING) * 3;
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch));

  struct char_data *tch, *next_tch;

  if (PRF_FLAGGED(ch, PRF_FIGHT_SPAM) || ch->combat_output == OUTPUT_SPARSE) {
    send_to_char(ch, "@CYou @Wdealt @R%d@W damage last round.  @CYou @Wtook @R%d @wdamage last round.@n\r\n", GET_DAMAGE_DEALT_THIS_ROUND(ch), (ch)->damage_taken_last_round);
    char fbuf[200];
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if ((is_player_grouped(tch, ch) && FIGHTING(tch) && tch != ch) || is_player_grouped(tch, FIGHTING(ch))) {
        sprintf(fbuf, "%s$n @Wdealt @R%d@W damage last round.  %s$n @Wtook @R%d @Wdamage last round.@n", is_player_grouped(tch, ch) ? "@Y" : "@r",
                GET_DAMAGE_DEALT_THIS_ROUND(tch), is_player_grouped(tch, ch) ? "@Y" : "@r",(tch)->damage_taken_last_round);
        act(fbuf, false, tch, 0, ch, TO_VICT);
      }
    }
  }

  if (!PRF_FLAGGED(ch, PRF_AUTOATTACK))
    if (!IS_NPC(ch))
      send_to_char(ch, "@YIt is now your turn in combat.  Common combat commands include: attack, approach, retreat, cast and endturn.@n\r\n");

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;

      if (tch == ch)
        continue;

      if (PRF_FLAGGED(tch, PRF_AUTOATTACK))
        continue;

      if ((is_player_grouped(ch, tch) || is_player_grouped(FIGHTING(tch), ch) || FIGHTING(ch) == tch) && !PRF_FLAGGED(tch, PRF_FIGHT_SPAM)) {
        act("@MIt is now $n's turn in combat.@n", true, ch, 0, tch, TO_VICT);
      }
    }

  do_summon_attack(ch);
  do_companion_attack(ch);
  do_mount_attack(ch);

  if (GET_GUILD(ch) == GUILD_DEVOTIONISTS && dice(1, 20) == 1) {
    GET_HIT(ch) += GET_GUILD_RANK(ch) / 4 * 5;
  }
    

  if (GET_FIGHT_BLEEDING_DAMAGE(ch)) {
    damage(ch, ch, GET_FIGHT_BLEEDING_DAMAGE(ch), 0, SPELL_BLEEDING_DAMAGE, -1, 0, SPELL_BLEEDING_DAMAGE, 1);

    sprintf(buf, "@RYou take @Y%d@R bleeding damage!@n", GET_FIGHT_BLEEDING_DAMAGE(ch));
    act(buf, TRUE, ch, 0, 0, TO_CHAR);
    if (!PRF_FLAGGED(ch, PRF_FIGHT_SPAM)) {
      sprintf(buf, "@R$n takes @Y%d@R bleeding damage!@n", GET_FIGHT_BLEEDING_DAMAGE(ch));
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  if (ch->active_turn == 1)
    do_affect_tickdown(ch);

}

byte can_use_available_actions(struct char_data *ch, byte action)
{
  if (action == ACTION_MINOR) {
    if (ch->minor_action_spent != 0) {
      if (ch->move_action_spent != 0) {
        if (ch->standard_action_spent != 0) {
          send_to_char(ch, "You have already spent your minor, move and standard actions this round.\r\n");
          return FALSE;
        }
        ch->standard_action_spent = 1;
      }
      ch->move_action_spent = 1;
    }
    else {
      ch->minor_action_spent = 1;
    }
  }
  else if (action == ACTION_MOVE) {
    if (ch->move_action_spent != 0) {
      if (ch->standard_action_spent != 0) {
        send_to_char(ch, "You have already spent your move and standard actions this round.\r\n");
        return FALSE;
      }
      ch->standard_action_spent = 1;
    }
    else 
      ch->move_action_spent = 1;
  }
  else {
    if (ch->standard_action_spent != 0) {
      send_to_char(ch, "You have already spent your standard action this round.\r\n");
      return FALSE;
    }
    else
      ch->standard_action_spent = 1;
  }

  return TRUE;
}

ACMD(do_divine_bond)
{

  if (!HAS_FEAT(ch, FEAT_DIVINE_BOND)) {
    send_to_char(ch, "You do not have the paladin feat divine bond.\r\n");
    return;
  }

  send_to_char(ch, "Your divine bond with your weapon gives you the following bonuses:\r\n\r\n");

  send_to_char(ch, "+%d to hit and damage rolls when wielding a weapon.\r\n", 
               MIN(6, 1 + MAX(0, (GET_CLASS_RANKS(ch, CLASS_PALADIN) - 5) / 3)));

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 23)
    send_to_char(ch, "+1d6 holy damage against non-good foes.\r\n");

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 26)
    send_to_char(ch, "+1d6 fire damage.\r\n");

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 29)
    send_to_char(ch, "+2d10 fire damage on critical hits.\r\n");

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 32)
    send_to_char(ch, "+1d6 holy damage when trying to subdue.\r\n");

  if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 35)
    send_to_char(ch, "an extra attack each round with each weapon.\r\n");

  send_to_char(ch, "\r\n");

}

ACMD(do_whirlwind)
{

  struct char_data *vict;

  if (!HAS_FEAT(ch, FEAT_WHIRLWIND_ATTACK)) {
    send_to_char(ch, "You do not have the whirlwind attack feat.\r\n");
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char(ch, "You cannot do a whirlwind attack if you are not fighting.\r\n");
    return;
  }

  if (!can_use_available_actions(ch, ACTION_STANDARD)) {
    send_to_char(ch, "You do not have any available actions to do a whirlwind attack.\r\n");
    return;
  }

  send_to_char(ch, "You perform a whirlwind attack.\r\n");

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {

    if (PRF_FLAGGED(ch, PRF_CONTAINED_AREAS) && (!FIGHTING(vict) || !is_player_grouped(FIGHTING(vict), ch)))
      continue;
    if (is_player_grouped(ch, vict))
      continue;
    do_whirlwind_attack(ch, vict);
  }
}

ACMD(do_swarmofarrows)
{

  struct char_data *vict;

  if (!HAS_FEAT(ch, FEAT_SWARM_OF_ARROWS)) {
    send_to_char(ch, "You do not have the swarm of arrows feat.\r\n");
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char(ch, "You cannot perform a swarm of arrows if you are not fighting.\r\n");
    return;
  }

  if (!can_use_available_actions(ch, ACTION_STANDARD)) {
    send_to_char(ch, "You do not have any available actions to perform a swarm of arrows.\r\n");
    return;
  }

  send_to_char(ch, "You let loose a swarm of arrows.\r\n");

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {

    if (PRF_FLAGGED(ch, PRF_CONTAINED_AREAS) && (!FIGHTING(vict) || !is_player_grouped(FIGHTING(vict), ch)))
      continue;
    if (is_player_grouped(ch, vict))
      continue;
    do_swarm_of_arrows(ch, vict);
  }
}


void award_lockbox_treasure(struct char_data *ch, int level)
{
  int roll = 0, roll2 = 0;

  roll = dice(1, 100);
  int grade = GRADE_MUNDANE;

  if (level >= 20) {
    grade = GRADE_MAJOR;
  }
  else if (level >= 16) {
    if (roll >= 61)
      grade = GRADE_MAJOR;
    else
      grade = GRADE_MEDIUM;
  }
  else if (level >= 12) {
    if (roll >= 81)
      grade = GRADE_MAJOR;
    else if (roll >= 11)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 8) {
    if (roll >= 96)
      grade = GRADE_MAJOR;
    else if (roll >= 31)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 4) {
    if (roll >= 76)
      grade = GRADE_MEDIUM;
    else if (roll >= 16)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }
  else {
    if (roll >= 96)
      grade = GRADE_MEDIUM;
    else if (roll >= 41)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }

    roll2 = dice(1, 1000);

    if (roll2 == 1)
      award_special_magic_item(ch);
    else if (roll2 <= 500) {
      int gold = level * MAX(1, (level / 5)) * MAX(1, (level / 10)) * 100;
      send_to_char(ch, "You have found %d %s!\r\n", gold, MONEY_STRING);
      GET_GOLD(ch) += gold;
    }
    else if (roll2 <= 700)
      award_expendable_item(ch, grade, TYPE_SCROLL);
    else if (roll2 <= 800)
      award_magic_weapon(ch, grade, level);
    else if (roll2 <= 900)
      award_misc_magic_item(ch, grade, level);
    else 
      award_magic_armor(ch, grade, level);
}
