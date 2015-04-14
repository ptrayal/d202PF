/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "mysql/mysql.h"

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "quest.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "constants.h"
#include "feats.h"
#include "deities.h"
#include "screen.h"
#include "player_guilds.h"

/* local functions */
void run_autowiz(void);
void apply_milestone_bonus(struct char_data *ch);
int get_speed(struct char_data *ch);
int group_align_exp_mod(struct char_data *ch);
void do_auto_combat(struct char_data *ch);
void award_rp_exp_char(struct char_data *i);

// External Functions
void calculate_current_weight(struct char_data *ch);
ACMD(do_endturn);
ACMD(do_approach);
ACMD(do_attack);
ACMD(do_endturn);
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int is_crit, int material, int bonus, int spell, int magic);
int get_saving_throw_value(struct char_data *victim, int savetype);
size_t proc_colors(char *txt, size_t 
maxlen, int parse, char **choices);
char *get_blank_clan_name(int clan);
char *get_rank_name(int clan, int rank);
void perform_pc_combat_turn(struct char_data *ch);
int calculate_max_hit(struct char_data *ch);
int is_player_grouped(struct char_data *target, struct char_data *group);
void stop_guard(struct char_data *ch);
char *reduct_desc(struct char_data *victim, struct damreduct_type *reduct);
void convert_coins(struct char_data *ch);
void extract_char(struct char_data *ch);
void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int level, int race);
void update_char_objects(struct char_data *ch);	/* handler.c */
int calc_panalty_exp(struct char_data *ch, int gain);
void update_innate(struct char_data *ch);
void reboot_wizlists(void);
void death_cry(struct char_data *ch);
int strlencolor(char *arg);
void die(struct char_data *ch, struct char_data * killer);
int findslotnum(struct char_data *ch, int spelllvl);

/* extern vars */
extern MYSQL *conn;

extern struct time_info_data time_info;
extern int race_level_adjustment[];
extern int top_of_p_table;

// local vars
int num_online = 0;

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
  return 1;

  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = MAX(1, ability_mod_value(GET_WIS(ch)));
  } else {
    gain = MAX(1, ability_mod_value(GET_WIS(ch)));

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain = gain * 15 / 10;
      break;
    case POS_RESTING:
      gain += (gain / 4);
      break;
    case POS_SITTING:
      gain = gain;
      break;
    case POS_STANDING:
      gain = 25 * gain / 100;
      break;
    case POS_FIGHTING:
      gain = gain * 20 / 100;
      break;
    default:
      gain = 0;
      break;      
    }
  }

  return (gain);
}


/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain = 1;
  int healgain = 0;
  struct obj_data *camp = NULL;

    // fast healer feat

    if (HAS_FEAT(ch, FEAT_FAST_HEALER))
      gain += 2;

    // Con modifier
    gain += ability_mod_value(GET_CON(ch));


    camp = get_obj_in_list_vis(ch, "campfire", NULL, world[IN_ROOM(ch)].contents);

    if (camp && GET_OBJ_VNUM(camp) == 294)
      gain += 1 + MAX(0, ((GET_OBJ_VAL(camp, 0) - 10) / 5));

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += 3;
      break;
    case POS_RESTING:
      gain += 1;
      break;
    case POS_SITTING:
      gain += 1;
      break;
    case POS_STANDING:
      gain = 1;
      break;
    case POS_FIGHTING:
      gain = 0;
      break;
      default:
        gain = 0;
        break;      
    }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TAVERN))
    gain += 2;

  if (!FIGHTING(ch) && HAS_FEAT(ch, FEAT_FAST_HEALING))
    gain += 3 * HAS_FEAT(ch, FEAT_FAST_HEALING);

  if (GET_GUILD(ch) == GUILD_DEVOTIONISTS)
    gain += (GET_GUILD_RANK(ch) + 2) / 4;

  if (FIGHTING(ch))
    gain = 0;
  // Heal skill
  else if (GET_HEAL_AMOUNT(ch) > 0) {
      healgain = MIN(MAX(1, GET_HEAL_ROLL(ch) / 10), GET_HEAL_AMOUNT(ch));
      GET_HEAL_AMOUNT(ch) -= healgain;
      gain += healgain;
  }


  if (GET_POS(ch) >= POS_INCAP || GET_HIT(ch) >= 0)
    return (MAX(0, gain));
  else {
    damage(ch, ch, 1, TYPE_SUFFERING, 0, -1, 0, TYPE_SUFFERING, 1);
    return 0;
  }
}



/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;
  struct obj_data *camp = NULL;

  if (IS_NPC(ch)) 
    /* Neat and fast */
    return GET_LEVEL(ch);

  gain = MAX(1, (get_speed(ch) / 2));
  gain += MAX(0, ability_mod_value(GET_CON(ch)) * 10);

  camp = get_obj_in_list_vis(ch, "campfire", NULL, world[IN_ROOM(ch)].contents);

  if (camp && GET_OBJ_VNUM(camp) == 294)
    gain += MAX(0, (GET_OBJ_VAL(camp, 0) - 10) * 20);

  /* Position calculations    */
  switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += 20;
      break;
    case POS_RESTING:
      gain += 10;
      break;
    case POS_SITTING:
      gain += 10;
      break;
    case POS_STANDING:
      gain = 5;
      break;
    case POS_FIGHTING:
      gain = 5;
      break;
    default:
      break;      
  }

  if (HAS_FEAT(ch, FEAT_ENDURANCE))
    gain += 10;

  gain += MAX(0, get_skill_value(ch, SKILL_ATHLETICS));

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TAVERN))
    gain += 20;

  if (GET_GUILD(ch) == GUILD_DEVOTIONISTS)
    gain += MAX(0, ((GET_GUILD_RANK(ch) + 2) / 4) * 10);

  if (gain < 10)
    gain = 10;

  return gain;
}


/* ki gain pr. game hour */
int ki_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = 5 * MAX(1, ability_mod_value(GET_WIS(ch)) / 2);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain = gain * 15 / 10;
      break;
    case POS_RESTING:
      gain += (gain / 4);
      break;
    case POS_SITTING:
      gain = gain;
      break;
    case POS_STANDING:
      gain = 25 * gain / 100;
      break;
    case POS_FIGHTING:
      gain = gain * 10 / 100;
      break;
      default:
        gain = 0;
        break;      
    }

  }


  return (gain);
}


void set_title(struct char_data *ch, char *title)
{
  if (title == NULL) {
    title = class_desc_str(ch, 2, TRUE);
  }
  

  if (strlencolor(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = strdup(title);
}

void gain_level(struct char_data *ch, int whichclass)
{
  int level = GET_CLASS_LEVEL(ch);
  if (GET_CLASS_LEVEL(ch) == 0)
    level = 0;

  if (whichclass < 0)
    whichclass = GET_CLASS(ch);
  if (GET_CLASS_LEVEL(ch) == 1 && race_list[GET_RACE(ch)].level_adjustment && FALSE) {
   if (level < CONFIG_LEVEL_CAP - 2 ||
       (((GET_EXP(ch) >= (level_exp(level, GET_REAL_RACE(ch)))) ||
      (((GET_EXP(ch) >= (level_exp(level + 1, GET_REAL_RACE(ch))) * 33 / 100)) && (GET_LEVEL_STAGE(ch) == 0)) ||
          (((GET_EXP(ch) >= (level_exp(level + 1, GET_REAL_RACE(ch))) * 66 / 100)) && (GET_LEVEL_STAGE(ch) == 1)))))
	{

		if (GET_EXP(ch) >= level_exp(level + 1, GET_REAL_RACE(ch))) {
		  GET_LEVEL_STAGE(ch) = 0;
		  GET_CHOSEN_CLASS(ch) = CLASS_UNDEFINED;
		  mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced level to level %d.",
			   GET_NAME(ch), GET_LEVEL(ch) + 1);
		  send_to_char(ch, "You rise a level!\r\n");
		  if (raise_class_only(ch, whichclass, 1)) {
			GET_CLASS(ch) = whichclass; /* Now tracks latest class instead of highest */
			advance_level(ch, whichclass);
  			  GET_CLASS_LEVEL(ch) += 1;                        
                          save_char(ch);
		  }
		  else {
			log("Failure in gain_level: raise_class_only failed for %s", GET_NAME(ch));
			send_to_char(ch, "Cannot raise your level. Please contact an administrator.\r\n");
		  }     
		}
		else {
                  send_to_char(ch, "You are not yet ready to advance.\r\n");
		  return;
               }
	}
  }
  else {
    if ((level == 0) || (level < CONFIG_LEVEL_CAP - 2 ||
       (((GET_EXP(ch) >= level_exp(level, GET_REAL_RACE(ch)))) ||
       ((GET_EXP(ch) - level_exp(level, GET_REAL_RACE(ch)) >= ((level_exp(level + 1, GET_REAL_RACE(ch)) - 
	   level_exp(level, GET_REAL_RACE(ch))) * 33 / 100)) && (GET_LEVEL_STAGE(ch) == 0)) ||
       ((GET_EXP(ch) - level_exp(level, GET_REAL_RACE(ch)) >= ((level_exp(level + 1, GET_REAL_RACE(ch)) - 
	   level_exp(level, GET_REAL_RACE(ch))) * 66 / 100)) && (GET_LEVEL_STAGE(ch) == 1))))) 
	{

		if (level == 0 || (GET_EXP(ch) >= level_exp(level + 1, GET_REAL_RACE(ch)))) {
		  GET_LEVEL_STAGE(ch) = 0;
		  GET_CHOSEN_CLASS(ch) = CLASS_UNDEFINED;
		  mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced level to level %d.",
			   GET_NAME(ch), GET_LEVEL(ch) + 1);
		  send_to_char(ch, "You rise a level!\r\n");
		  if (raise_class_only(ch, whichclass, 1)) {
			GET_CLASS(ch) = whichclass; /* Now tracks latest class instead of highest */
			advance_level(ch, whichclass);
//			if (GET_LEVEL(ch) >= 1)
  			  GET_CLASS_LEVEL(ch) += 1;                        
                          save_char(ch);
		  }
		  else {
			log("Failure in gain_level: raise_class_only failed for %s", GET_NAME(ch));
			send_to_char(ch, "Cannot raise your level. Please contact an administrator.\r\n");
		  }     
		}
		else if ((GET_LEVEL_STAGE(ch) < 1)) {
//                  send_to_char(ch, "1\r\n");
                  send_to_char(ch, "You are not yet ready to advance.\r\n");
		  GET_LEVEL_STAGE(ch) = 1;
		  GET_CHOSEN_CLASS(ch) = whichclass;
//		  apply_milestone_bonus(ch);

		}
		else if ((GET_LEVEL_STAGE(ch) < 2) && ((GET_CHOSEN_CLASS(ch) == whichclass) || (GET_CHOSEN_CLASS(ch) == 0)))  {
//                  send_to_char(ch, "2\r\n");
                  send_to_char(ch, "You are not yet ready to advance.\r\n");
			 GET_LEVEL_STAGE(ch) = 2;
//			 apply_milestone_bonus(ch);
		}
		else {
                  send_to_char(ch, "You are not yet ready to advance.\r\n");
		  return;
                }
    }
  }	
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ) {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, ADMLVL_IMMORT, FALSE, "Initiating autowiz.");
      system(buf);
      reboot_wizlists();
    } else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

void gain_exp(struct char_data *ch, int gain)
{
  int level = GET_LEVEL(ch);
  int act_exp = 0;
  struct follow_type *f = NULL;
  int leadership = 0;
  struct char_data *k;
  int num_grouped = 1;
  int num_clanned = 1;
  int exp_buff = 0;
  int total_bonus = 0;
  int exp_chain = 0, leader_bonus = 0, rp_bonus = 0, leadership_bonus = 0, tavern_bonus = 0, alignment_bonus = 0, group_bonus = 0, clan_bonus = 0;
  int exp_mult = 0, deity_bonus = 0;
  int num_deity = 1;

  if (gain > 0)
  gain = (gain * CONFIG_EXP_MULTIPLIER);

  if (!IS_NPC(ch) && GET_LEVEL(ch) < 1)
    return;

  k = ch->master ? ch->master : ch;

  leadership = IS_NPC(k) ? 0 : HAS_FEAT(k, FEAT_LEADERSHIP);
  if (GET_CLAN(ch) != 0 && GET_CLAN(k) == GET_CLAN(ch) && k != ch)
    num_clanned++;

  for (f = k->followers; f; f = f->next) {
    if (!IS_NPC(f->follower) && is_player_grouped(ch, f->follower)) {
      if (HAS_FEAT(f->follower, FEAT_LEADERSHIP))
        leadership = MAX(leadership, HAS_FEAT(f->follower, FEAT_LEADERSHIP));
      num_grouped++;
      if (GET_CLAN(ch) != 0 && GET_CLAN(f->follower) == GET_CLAN(ch))
        num_clanned++;
     if (GET_DEITY(ch) != 0 && GET_DEITY(f->follower) == GET_DEITY(ch))
       num_deity++;
    }
  }
  
 
  if (gain > 0) {
  

  if (ch->exp_chain > 0 && gain > 0) {
        if (ch->exp_chain > 5)
          ch->exp_chain = 5;
	exp_chain =  ((ch->exp_chain * 10));
  }
  if (ch->master == NULL && ch->followers && AFF_FLAGGED(ch, AFF_GROUP) && !IS_NPC(ch->followers->follower)) {
	leader_bonus = 2;
  }
  if (CONFIG_EXP_MULTIPLIER > 1) {
    exp_mult = 1;
    gain *= CONFIG_EXP_MULTIPLIER;
  }
  if (affected_by_spell(ch, SPELL_LEARNING)) {
    exp_buff = 10;
  }
  if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING)) {
    exp_buff = 20;
  }
  if (affected_by_spell(ch, SPELL_GREATER_LEARNING)) {
    exp_buff = 33;
  }
  if (affected_by_spell(ch, SPELL_EPIC_LEARNING)) {
    exp_buff = 50;
  }
  if (num_grouped >= 2) {
    group_bonus += MIN(5, num_grouped);
  }
  if (num_clanned >= 2) {
    clan_bonus += MIN(5, num_clanned);
  }
  if (num_deity >= 2) {
    deity_bonus += MIN(5, num_deity);
  }
  if (get_rp_bonus(ch, RP_EXP) > 0 && gain > 0) {
	rp_bonus = get_rp_bonus(ch, RP_EXP);
  }
  if (leadership > 0 && num_grouped > 1) {
    leadership_bonus = (( ((1 + leadership) * 5)));
  }
  if (GET_TAVERN_EXP_BONUS(ch) > 0 && gain > 0) {
  	tavern_bonus = 10 ;
  }
  if (group_align_exp_mod(ch) > 0 && gain > 0) {
 	alignment_bonus = 15;
  }
  }

  if (AFF_FLAGGED(ch, AFF_SPIRIT))
    gain = 0;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  if (dice(1, 10) == 1 && gain > 0)
    act_exp++;

  if (!(PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF) && GET_CRAFTING_OBJ(ch))) {
  if (gain > 0) {
    total_bonus = exp_chain + leader_bonus + rp_bonus + leadership_bonus + tavern_bonus + alignment_bonus + clan_bonus + group_bonus + exp_buff + deity_bonus;

    send_to_char(ch, "@YEXP GAIN: %d base + %d bonus = %d total %s@M%s%s%s%s%s%s%s%s%s%s%s@Y%s@n\r\n",
                 gain, gain * total_bonus / 100, gain * (100 + total_bonus) / 100, total_bonus ? "(" : "", exp_chain ? " encounter" : "",
                 leader_bonus ? " party-leader" : "", rp_bonus ? " rp-points" : "", leadership_bonus ? " leader-feat" : "",
                 tavern_bonus ? " tavern" : "", alignment_bonus ? " party-alignment" : "", deity_bonus ? " religion" : "",
                 group_bonus ? " group" : "", clan_bonus ? " clan" : "", exp_buff ? " exp-buff" : "", exp_mult ? " promotion" : "",
                 total_bonus ? " )" : "");
  }
  }

  gain *= 100 + total_bonus;
  gain /= 100;

  if (gain > 0) {
    gain = calc_penalty_exp(ch, gain);
    if (GET_CRAFTING_OBJ(ch) == NULL) {
      gain = MIN(CONFIG_MAX_EXP_GAIN, gain);	/* put a cap on the max gain per kill */
    }
    GET_EXP(ch) += gain;
  }



  if (ch->desc && ch->desc->account) {
    if (act_exp) {
      if (!(PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF) && GET_CRAFTING_OBJ(ch))) 
        send_to_char(ch, "@YYou have gained %d gift account experience (see help gift account experience).@n\r\n", act_exp);  
      ch->desc->account->gift_experience += act_exp;
    }
  }

  if (!(PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF) && GET_CRAFTING_OBJ(ch))) {    
    if (GET_EXP(ch) >= level_exp(level + 1, GET_REAL_RACE(ch)))
      send_to_char(ch, "@YYou have earned enough experience to gain a level!@n\r\n");     
  }

  if (gain < 0) {
//    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain);	/* Cap max exp lost per death */
    send_to_char(ch, "@REXP LOSS: %d@n@n\r\n", gain);
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}

void gain_artisan_exp(struct char_data *ch, int gain)
{
  struct follow_type *f = NULL;
  int leadership = 0;
  struct char_data *k;
  int num_grouped = 1;
  int num_clanned = 1;
  int exp_buff = 0;
  int total_bonus = 0;
  int exp_chain = 0, leader_bonus = 0, rp_bonus = 0, leadership_bonus = 0, tavern_bonus = 0, alignment_bonus = 0, group_bonus = 0, clan_bonus = 0;
  int fast_craft = 0;
  int exp_mult = 0;

  if (!IS_NPC(ch) && GET_LEVEL(ch) < 1)
    return;

  k = ch->master ? ch->master : ch;

  leadership = IS_NPC(k) ? 0 : HAS_FEAT(k, FEAT_LEADERSHIP);
  if (GET_CLAN(ch) != 0 && GET_CLAN(k) == GET_CLAN(ch) && k != ch)
    num_clanned++;

  for (f = k->followers; f; f = f->next) {
    if (!IS_NPC(f->follower) && is_player_grouped(ch, f->follower)) {
      if (HAS_FEAT(f->follower, FEAT_LEADERSHIP))
        leadership = MAX(leadership, HAS_FEAT(f->follower, FEAT_LEADERSHIP));
      num_grouped++;
      if (GET_CLAN(ch) != 0 && GET_CLAN(f->follower) == GET_CLAN(ch))
        num_clanned++;
    }
  }
  
 
  if (gain > 0) {
  

  if (ch->exp_chain > 0 && gain > 0) {
        if (ch->exp_chain > 5)
          ch->exp_chain = 5;
	exp_chain =  ((ch->exp_chain * 10));
  }
  if (ch->master == NULL && ch->followers && AFF_FLAGGED(ch, AFF_GROUP) && !IS_NPC(ch->followers->follower)) {
	leader_bonus = 2;
  }
  if (CONFIG_EXP_MULTIPLIER > 1) {
    exp_mult = 1;
    gain *= CONFIG_EXP_MULTIPLIER;
  }
  if (affected_by_spell(ch, SPELL_LEARNING)) {
    exp_buff = 10;
  }
  if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING)) {
    exp_buff = 20;
  }
  if (affected_by_spell(ch, SPELL_GREATER_LEARNING)) {
    exp_buff = 33;
  }
  if (affected_by_spell(ch, SPELL_EPIC_LEARNING)) {
    exp_buff = 50;
  }
  if (HAS_FEAT(ch, FEAT_FAST_CRAFTER)) {
    fast_craft = MAX(1, (10 - HAS_FEAT(ch, FEAT_FAST_CRAFTER)));
  }
  if (num_grouped >= 2) {
    group_bonus += MIN(10, num_grouped * 2);
  }
  if (num_clanned >= 2) {
    clan_bonus += MIN(10, num_clanned * 2);
  }
  if (get_rp_bonus(ch, RP_ART_EXP) > 0 && gain > 0) {
	rp_bonus = get_rp_bonus(ch, RP_ART_EXP);
  }
  if (leadership > 0 && num_grouped > 1) {
    leadership_bonus = (( ((1 + leadership) * 5)));
  }
  if (GET_TAVERN_EXP_BONUS(ch) > 0 && gain > 0) {
  	tavern_bonus = 10 ;
  }
  if (group_align_exp_mod(ch) > 0 && gain > 0) {
 	alignment_bonus = 15;
  }
  }

  if (AFF_FLAGGED(ch, AFF_SPIRIT))
    gain = 0;

  if (!(PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF) && GET_CRAFTING_OBJ(ch))) {
  if (gain > 0) {
    total_bonus = exp_chain + leader_bonus + rp_bonus + leadership_bonus + tavern_bonus + alignment_bonus + clan_bonus + group_bonus + exp_buff;
    total_bonus += fast_craft * 10;

    send_to_char(ch, "@GARTISAN EXP GAIN: %d base + %d bonus = %d total %s@M%s%s%s%s%s%s%s%s%s%s%s@Y%s@n\r\n",
                 gain, gain * total_bonus / 100, gain * (100 + total_bonus) / 100, total_bonus ? "(" : "", exp_chain ? " encounter" : "",
                 leader_bonus ? " party-leader" : "", rp_bonus ? " rp-points" : "", leadership_bonus ? " leader-feat" : "",
                 tavern_bonus ? " tavern" : "", alignment_bonus ? " party-alignment" : "", 
                 group_bonus ? " group" : "", clan_bonus ? " clan" : "", exp_buff ? " exp-buff" : "", 
                 fast_craft ? " fast-crafter" : "", exp_mult ? " promotion" : "",
                 total_bonus ? " )" : "");
  }
  }

  gain *= (100 + total_bonus);
  gain /= 100;

  if (gain > 0) {
    GET_ARTISAN_EXP(ch) += gain;
  }

}

void gain_gold(struct char_data *ch, int gain, int type)
{
  struct follow_type *f = NULL;
  int leadership = 0;
  struct char_data *k;
  int num_grouped = 1;
  int num_clanned = 1;
  int exp_buff = 0;
  int total_bonus = 0;
  int exp_chain = 0, leader_bonus = 0, rp_bonus = 0, leadership_bonus = 0, tavern_bonus = 0, alignment_bonus = 0, group_bonus = 0, clan_bonus = 0;
  int exp_mult = 0;

  if (gain > 0)
  gain = (gain * CONFIG_EXP_MULTIPLIER);

  if (!IS_NPC(ch) && GET_LEVEL(ch) < 1)
    return;

  k = ch->master ? ch->master : ch;

  leadership = IS_NPC(k) ? 0 : HAS_FEAT(k, FEAT_LEADERSHIP);
  if (GET_CLAN(ch) != 0 && GET_CLAN(k) == GET_CLAN(ch) && k != ch)
    num_clanned++;

  for (f = k->followers; f; f = f->next) {
    if (!IS_NPC(f->follower) && is_player_grouped(ch, f->follower)) {
      if (HAS_FEAT(f->follower, FEAT_LEADERSHIP))
        leadership = MAX(leadership, HAS_FEAT(f->follower, FEAT_LEADERSHIP));
      num_grouped++;
      if (GET_CLAN(ch) != 0 && GET_CLAN(f->follower) == GET_CLAN(ch))
        num_clanned++;
    }
  }
  
 
  if (gain > 0) {
  

  if (ch->exp_chain > 0 && gain > 0) {
        if (ch->exp_chain > 5)
          ch->exp_chain = 5;
	exp_chain =  ((ch->exp_chain * 10));
  }
  if (ch->master == NULL && ch->followers && AFF_FLAGGED(ch, AFF_GROUP) && !IS_NPC(ch->followers->follower)) {
	leader_bonus = 2;
  }
  if (CONFIG_EXP_MULTIPLIER > 1) {
    exp_mult = 1;
    gain *= CONFIG_EXP_MULTIPLIER;
  }
  if (affected_by_spell(ch, SPELL_LEARNING)) {
    exp_buff = 10;
  }
  if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING)) {
    exp_buff = 20;
  }
  if (affected_by_spell(ch, SPELL_GREATER_LEARNING)) {
    exp_buff = 33;
  }
  if (affected_by_spell(ch, SPELL_EPIC_LEARNING)) {
    exp_buff = 50;
  }
  if (num_grouped >= 2) {
    group_bonus += MIN(5, num_grouped);
  }
  if (num_clanned >= 2) {
    clan_bonus += MIN(5, num_clanned);
  }
  if (get_rp_bonus(ch, RP_GOLD) > 0 && gain > 0) {
	rp_bonus = get_rp_bonus(ch, RP_GOLD);
  }
  if (leadership > 0 && num_grouped > 1) {
    leadership_bonus = (( ((1 + leadership) * 5)));
  }
  if (GET_TAVERN_EXP_BONUS(ch) > 0 && gain > 0) {
  	tavern_bonus = 10 ;
  }
  if (group_align_exp_mod(ch) > 0 && gain > 0) {
 	alignment_bonus = 15;
  }
  }

  if (AFF_FLAGGED(ch, AFF_SPIRIT))
    gain = 0;

  if (!(PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF) && GET_CRAFTING_OBJ(ch))) {
  if (gain > 0) {
    total_bonus = exp_chain = leader_bonus = leadership_bonus = tavern_bonus = alignment_bonus = clan_bonus = group_bonus = exp_buff = 0;
    total_bonus = rp_bonus;
    send_to_char(ch, "@C%s GAIN: %d base + %d bonus = %d total %s@M%s%s%s%s%s%s%s%s%s%s@Y%s@n\r\n", type == GOLD_ONHAND ? "GOLD" : "BANK",
                 gain, gain * total_bonus / 100, gain * (100 + total_bonus) / 100, total_bonus ? "(" : "", exp_chain ? " encounter" : "",
                 leader_bonus ? " party-leader" : "", rp_bonus ? " rp-points" : "", leadership_bonus ? " leader-feat" : "",
                 tavern_bonus ? " tavern" : "", alignment_bonus ? " party-alignment" : "", 
                 group_bonus ? " group" : "", clan_bonus ? " clan" : "", exp_buff ? " exp-buff" : "", exp_mult ? " promotion" : "",
                 total_bonus ? " )" : "");
  } else {
    send_to_char(ch, "@C%s LOSS: %d@n\r\n", type == GOLD_ONHAND ? "GOLD" : "BANK", -gain);
  }
  }

  gain *= 100 + (int) (total_bonus * 10);
  gain /= 100;

  if (type == GOLD_ONHAND)
    GET_GOLD(ch) += gain;
  else
    GET_BANK_GOLD(ch) += gain;
}


void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  gain = (gain * CONFIG_EXP_MULTIPLIER);

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1 &&
	GET_EXP(ch) >= level_exp(GET_LEVEL(ch) + 1, GET_REAL_RACE(ch))) {
      if (raise_class_only(ch, GET_CLASS(ch), 1)) {
        GET_CLASS_LEVEL(ch) += 1;
        save_char(ch);
        num_levels++;
        advance_level(ch, GET_CLASS(ch));
        is_altered = TRUE;
      } else {
        log("gain_exp_regardless: raise_class_only() failed for %s", GET_NAME(ch));
      }
    }

    if (is_altered) {
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s advanced %d level%s to level %d.", GET_NAME(ch), num_levels,
             num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      //set_title(ch, NULL);
    }
  }
}


void gain_condition(struct char_data *ch, int condition, int value)
{
  if (!ch->desc && !IS_NPC(ch))
    return;

  if (IS_NPC(ch) || (ch->desc && GET_COND(ch, condition) == -1))	/* No change */
    return;

  bool intoxicated;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }

}


void check_idling(struct char_data *ch)
{

  if (++(ch->timer) > CONFIG_IDLE_VOID * 10) {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      stop_guard(ch);
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
      save_char(ch);
      Crash_crashsave(ch, FALSE);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->timer > CONFIG_IDLE_RENT_TIME * 10) {
      if (IN_ROOM(ch) != NOWHERE)
        char_from_room(ch);
      char_to_room(ch, 3);
      add_llog_entry(ch,LAST_IDLEOUT);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      stop_guard(ch);
      if (CONFIG_FREE_RENT)
	Crash_rentsave(ch, GET_LOADROOM(ch));
      else
	Crash_idlesave(ch);
      mudlog(CMP, ADMLVL_GOD, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      extract_char(ch);
    }
  }
}

void do_auto_combat(struct char_data *ch)
{
  if (!FIGHTING(ch) || GET_POS(FIGHTING(ch)) == POS_DEAD || ch->active_turn == 0)
    return;
  if (((ch->timer > 10 && ch->active_turn == 1) || PRF_FLAGGED(ch, PRF_AUTOATTACK)) && FIGHTING(ch)) {
    if ((ch->timer > 10 && ch->active_turn == 1))
      send_to_char(ch, "You have been idle for too long, thus your combat action was taken automatically.\r\n");
    if (ch->combat_cmd) {
      command_interpreter(ch, ch->combat_cmd);
//      free(ch->combat_cmd);
    }
    perform_pc_combat_turn(ch);
    do_endturn(ch, "", 0, 0);
  }
}


/* Update PCs, NPCs, and objects */
void point_update(void)
{

  num_online = 0;
  char query[500];	
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  int n;
  struct affected_type *af, af2[2], af3[2], af4[2];
  int poisonmod = 0;
  int dam;
  char dammes[MAX_STRING_LENGTH];
  struct char_data *vict;
  int fighting = FALSE;  
  struct damreduct_type *reduct, *temp;
  struct char_data *i, *next_char;
  char buf[200];
  char *title;
  int adm = 0;
  char *account_name;
  char align[100];
  int save = 0, dc = 0;
  int fight_found = FALSE;
  char fbuf[100];

  // Open mysql connection
  conn = mysql_init(NULL);
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  sbyte fa_found = FALSE;


  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
    log("Cannot connect to mysql database in point update.");
  }

  if (CONFIG_DFLT_PORT == 4000 || CONFIG_DFLT_PORT == 6070) {
    sprintf(query, "UPDATE player_data SET online = '0'");
    if (mysql_query(conn, query)) {
       log("Cannot set online status to 0");   
    }
  }

  mysql_close(conn);


  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

/*
    if (!IS_NPC(i) && !i->desc) {
      if (IN_ROOM(i) != NOWHERE)
  	    char_from_room(i);
      char_to_room(i, real_room(3));
      if (CONFIG_FREE_RENT)
	    Crash_rentsave(i, 0);
      else
	    Crash_idlesave(i);	
	  mudlog(BRF, ADMLVL_IMMORT, true, "Sending %s to extraction point.", GET_NAME(i));		
	  extract_char(i);
    }
*/

    if (!IS_NPC(i) && (!i->desc || STATE(i->desc) != CON_PLAYING))
      continue;

    if (i->bounty_gem > 0)
      i->bounty_gem--;

    if (GET_PETITION(i) > 0 && GET_PETITION(i) < 50)
      GET_PETITION(i)++;

    if (GET_GATHER_INFO(i) > 0)
      GET_GATHER_INFO(i)--;

    if (!IS_NPC(i) && i->desc)
      num_online++;

      if (GET_STR(i) > 0 && GET_DEX(i) > 0 && GET_INT(i) > 0 &&
          GET_WIS(i) > 0 && GET_CHA(i) > 0 && !affected_by_spell(i, SPELL_HOLD_PERSON) &&
          !affected_by_spell(i, ART_STUNNING_FIST) && !affected_by_spell(i, SPELL_SOUND_BURST) &&
          !affected_by_spell(i, SPELL_PARALYZE)
          )
        REMOVE_BIT_AR(AFF_FLAGS(i), AFF_STUNNED);

    if (!FIGHTING(i)) {
      GET_DAMAGE_TAKEN(i) = 0;
      i->total_defense = 0;
    }

    fight_found = FALSE;

    if (FIGHTING(i)) {
      for (vict = world[IN_ROOM(i)].people; vict; vict = vict->next_in_room) {
        if (vict == FIGHTING(i)) {
          if (!FIGHTING(vict)) {
            sprintf(fbuf, " %s", GET_NAME(vict));
            do_attack(i, buf, 0, 0);
            do_approach(i, "", 0, 0);
            do_approach(i, "", 0, 0);
            do_endturn(i, "", 0, 0);
          }
          fight_found = TRUE;
        }
      }
      if (!fight_found)
        stop_fighting(i);
    }

// MySQL Save

  if (i->desc && i->desc->account && STATE(i->desc) == CON_PLAYING && !IS_NPC(i) && (CONFIG_DFLT_PORT == 4000)) {

    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
      log("Cannot connect to mysql database in point update.");
    }

    sprintf(query, "DELETE FROM player_data WHERE name = '%s'", GET_NAME(i));
    if (mysql_query(conn, query)) {
       log("Cannot set delete %s from player_data table", GET_NAME(i));
    }

    sprintf(query, "DELETE FROM player_data WHERE idnum = '%ld'", GET_IDNUM(i));
    if (mysql_query(conn, query)) {
    }

    adm = GET_ADMLEVEL(i);

    account_name = strdup(i->desc->account->name);


    // Prepare strings

    if (GET_ALIGNMENT(i) == 0 && GET_ETHIC_ALIGNMENT(i) == 0)
      sprintf(align, "True Neutral");
    else
      sprintf(align, "%s %s", GET_ETHIC_ALIGNMENT(i) > 0 ? "Lawful" : (GET_ETHIC_ALIGNMENT(i) < 0 ? "Chaotic" : "Neutral"), 
              GET_ALIGNMENT(i) > 0 ? "Good" : (GET_ALIGNMENT(i) < 0 ? "Evil" : "Neutral"));

    title = strdup(GET_TITLE(i));
    proc_colors(title, sizeof(title), FALSE, COLOR_CHOICES(i));

    sprintf(query, "INSERT INTO player_data " 
    "(idnum,online,name,title,titlenocolor,rp_points,deity,laston,artisan_exp,experience,classes,race,quest_points,clan,clan_rank,web_password,alignment,level,account,adm_level,clan_rank_num) "
    "VALUES ('%ld','1','%s','%s','%s', '%ld','%s', NOW(), '%f', '%d', '%s', '%s', '%d', '%s', '%s', '%s', '%s', '%d', '%s', '%d', '%d')",
    GET_IDNUM(i), GET_NAME(i), GET_TITLE(i), title, adm ? 0 : GET_RP_POINTS(i), deity_list[GET_DEITY(i)].name,  adm ? 0 : get_artisan_exp(i), 
    adm ? 0 : GET_EXP(i),
    class_desc_str(i, 2, 0), race_list[GET_RACE(i)].name, adm ? 0 : GET_QUESTPOINTS(i), get_blank_clan_name(GET_CLAN(i)), 
    get_rank_name(GET_CLAN(i), GET_CLAN_RANK(i)), i->desc->account ? i->desc->account->web_password : "", align, adm ? 0 : GET_CLASS_LEVEL(i),
    account_name ? UNCAP(account_name) : "", adm, (GET_CLAN_RANK(i) > 0) ? GET_CLAN_RANK(i) : 0
    );
    free(title);
    if (mysql_query(conn, query)) {
       log("Cannot insert data for %s into player_data table", GET_NAME(i));
       log(query);

      sprintf(query, "SELECT account FROM player_forum_data WHERE account='%s'", UNCAP(account_name));
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        if ((row = mysql_fetch_row(res)) != NULL) {
          if (row[0] != NULL) {
            fa_found = TRUE;
          } 
        }
      }
      mysql_free_result(res);

      if (!fa_found) {
        sprintf(query, "INSERT INTO player_forum_data (account) VALUES('%s')", UNCAP(account_name));
        if (mysql_query(conn, query)) {
           log("Cannot insert data for %s into player_forum_data table", account_name);
        }
      }
    }

    mysql_close(conn);
  }

  if (!FIGHTING(i) && GET_HIT(i) <= 0) {
    save = d20 + get_saving_throw_value(i, SAVING_FORTITUDE);
    dc = 10;
    if (GET_HIT(i) < 0)
      dc -= GET_HIT(i);
    if ((save + 10) >= dc) {
      GET_HIT(i) = 1;
      update_pos(i);
      send_to_char(i, "You stablize and regain consciousness.\r\n");
      act("$n stablizes and regains consciousness.", FALSE, i, 0, 0, TO_ROOM);
      return;
    } else if ((save + 5) >= dc) {
      GET_HIT(i) = 0;
      update_pos(i);
      send_to_char(i, "You stablize but remain unconscious.\r\n");
      act("$n stablizes but remains unconscious.", FALSE, i, 0, 0, TO_ROOM);
      return;
    } else if (save >= dc) {
      send_to_char(i, "Your condition neither worsens nor improves.\r\n");
      return;
    } else {
      send_to_char(i, "You slide a little bit closer to death.\r\n");
      damage(i, i, 1, TYPE_SUFFERING, 0, -1, 0, TYPE_SUFFERING, 1);
    }
  }
	
    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    GET_MAX_HIT(i) = calculate_max_hit(i);

    if (IS_SET_AR(PRF_FLAGS(i), PRF_RUN)) {
      GET_ROUNDS_RUNNING(i) += 1;

      if (dice(1, 20) + ability_mod_value(GET_CON(i)) + (HAS_FEAT(i, FEAT_ENDURANCE) ? 4 : 0) 
          < 10 + GET_ROUNDS_RUNNING(i)) {

        REMOVE_BIT_AR(PRF_FLAGS(i), PRF_RUN);
        send_to_char(i, "You have become fatigued and are forced to stop running.\r\n");
 
        af3[0].type = SPELL_AFF_FATIGUED;
        af3[0].location = APPLY_STR;
        af3[0].bitvector = AFF_FATIGUED;
        af3[0].duration = MAX(1, GET_ROUNDS_RUNNING(i) / 2);
        af3[0].modifier =   -2;

        af3[1].type = SPELL_AFF_FATIGUED;
        af3[1].location = APPLY_DEX;
        af3[1].bitvector = AFF_FATIGUED;
        af3[1].duration = MAX(1, GET_ROUNDS_RUNNING(i) / 2);
        af3[1].modifier =   -2;

        for (n = 0; n < 2; n++)
          affect_join(i, af3+n, true, false, false, false);

        GET_ROUNDS_RUNNING(i) = 0;

      }
    }

    if (!FIGHTING(i) && !IS_NPC(i)) {
      if (i->fight_over == true)
        i->exp_chain = 0;
      else
        i->fight_over = true;
    }

    if (GET_MARK(i)) {
      if (skill_roll(i, SKILL_HIDE) > skill_roll(GET_MARK(i), SKILL_SPOT) &&
          skill_roll(i, SKILL_MOVE_SILENTLY) > skill_roll(GET_MARK(i), SKILL_LISTEN)) {
        GET_MARK_ROUNDS(i) += 2;
        if (GET_MARK_ROUNDS(i) >= 10)
          send_to_char(i, "Your mark is now ready for assassination.\r\n");
        else
          send_to_char(i, "You continue marking your target for assassination.\r\n");
      }
      else {
        GET_MARK_ROUNDS(i) = 0;
        GET_MARK(i) = NULL;
        send_to_char(i, "You lose your mark.\r\n");
      }
    }

    if (ROOM_FLAGGED(IN_ROOM(i), ROOM_TAVERN))
      GET_TAVERN_EXP_BONUS(i) += 2;
    else if (GET_TAVERN_EXP_BONUS(i) > 0)
      GET_TAVERN_EXP_BONUS(i) -= 1;
    

    for (reduct = i->damreduct; reduct; reduct = reduct->next) {
      if (reduct->duration > 0) 
        reduct->duration--;
      if (reduct->duration == 0) {
        send_to_char(i, "You have lost your damage reduction effect of %s.\r\n", reduct_desc(i, reduct));
        REMOVE_FROM_LIST(reduct, i->damreduct, next);
      }
    }

    if (IS_SET_AR(PRF_FLAGS(i), PRF_JOG)) {
      GET_ROUNDS_RUNNING(i) += 1;

      if (dice(1, 20) + ability_mod_value(GET_CON(i)) + (HAS_FEAT(i, FEAT_ENDURANCE) ? 4 : 0) 
          < 10 + GET_ROUNDS_RUNNING(i)) {

        REMOVE_BIT_AR(PRF_FLAGS(i), PRF_JOG);
        send_to_char(i, "You have become fatigued and are forced to stop jogging.\r\n");
 
        af3[0].type = SPELL_AFF_FATIGUED;
        af3[0].location = APPLY_STR;
        af3[0].bitvector = AFF_FATIGUED;
        af3[0].duration = MAX(1, GET_ROUNDS_RUNNING(i) / (HAS_FEAT(i, FEAT_ENDURANCE) ? 3 : 2));
        af3[0].modifier =   -2;

        af3[1].type = SPELL_AFF_FATIGUED;
        af3[1].location = APPLY_DEX;
        af3[1].bitvector = AFF_FATIGUED;
        af3[0].duration = MAX(1, GET_ROUNDS_RUNNING(i) / (HAS_FEAT(i, FEAT_ENDURANCE) ? 5 : 3));
        af3[1].modifier =   -2;

        for (n = 0; n < 2; n++)
          affect_join(i, af3+n, true, false, false, false);

        GET_ROUNDS_RUNNING(i) = 0;

      }
    }
    if (affected_by_spell(i, SPELL_ENTANGLED))
      affect_from_char(i, SPELL_ENTANGLED);

    if (i->affected) {
      for (af = i->affected; af; af = af->next) {
        if (af->type == SPELL_ENTANGLE) {
          if (dice(1, 20) + ability_mod_value(GET_STR(i)) < 20 && skill_roll(i, SKILL_ESCAPE_ARTIST) < 20) {
            af4[0].type = SPELL_ENTANGLED;
            af4[0].location = APPLY_DEX;
            af4[0].modifier = -4;
            af4[0].bitvector = AFF_NONE;
            af4[0].duration = 2;
            af4[1].type = SPELL_ENTANGLED;
            af4[1].location = APPLY_ACCURACY;
            af4[1].modifier = -2;
            af4[1].bitvector = AFF_NONE;
            af4[1].duration = 2;

            for (n = 0; n < 2; n++)
              affect_join(i, af4+n, true, false, false, false);
            act("You have become entangled.", FALSE, i, 0, 0, TO_CHAR);
            act("$n has become entangled.", FALSE, i, 0, 0, TO_ROOM);
            break;
          }          
        }
      }
    }

  if (GET_FIGHT_BLEEDING_DAMAGE(i) && !FIGHTING(i)) {
    damage(i, i, GET_FIGHT_BLEEDING_DAMAGE(i), 0, SPELL_BLEEDING_DAMAGE, -1, 0, SPELL_BLEEDING_DAMAGE, 1);

    sprintf(buf, "@RYou take @Y%d@R bleeding damage!@n", GET_FIGHT_BLEEDING_DAMAGE(i));
    act(buf, TRUE, i, 0, 0, TO_CHAR);
    sprintf(buf, "@R$n takes @Y%d@R bleeding damage!@n", GET_FIGHT_BLEEDING_DAMAGE(i));
    act(buf, TRUE, i, 0, 0, TO_ROOM);
  }


    if (GET_HIT(i) >= GET_MAX_HIT(i))
      GET_FIGHTING_MAX_LVL(i) = 0;	

    if ((IS_ELF(i) || IS_ORC(i) || IS_HALF_ORC(i) || IS_HALF_ELF(i) || IS_DEEP_GNOME(i) || 
            IS_CENTAUR(i) || IS_OGRE(i)) && !IS_AFFECTED(i, AFF_INFRAVISION)) 
    {
      SET_BIT_AR(AFF_FLAGS(i), AFF_INFRAVISION);
    }

	  if (IS_NPC(i)) {
	  	for (n = 0; n < MAX_SPELL_LEVELS; n++) {
	  		if (GET_SPELL_SLOT(i, n) < findslotnum(i, n) && dice(1, 10) == 1)
	  			SET_SPELL_SLOT(i, n, GET_SPELL_SLOT(i, n) + 1);
	  	}
	  }
	
	  if (affected_by_spell(i, SPELL_POISON_TIMER)) {
	    if (!affected_by_spell(i, SPELL_DELAY_POISON) && !HAS_FEAT(i, FEAT_ESSENCE_OF_UNDEATH) &&
                !HAS_FEAT(i, FEAT_DIAMOND_BODY)) {
        for (af = i->affected; af; af = af->next) {
          if (af->type == SPELL_POISON_TIMER) {
            
            send_to_char(i, "The deadly venom courses through your veins.\r\n");
            
            if ((spell_info[af->type].violent == true) && mag_newsaves(find_savetype(SPELL_POISON), i, i, SPELL_POISON, af->modifier)) {
              if (IS_SET(spell_info[af->type].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
                send_to_char(i, "@g*save*@y You avoid any lasting affects.@n\r\n");
              }
            }
            
            poisonmod = GET_POISON_DAMAGE_AMOUNT(i);
            
            af2[0].type = SPELL_POISON;
            af2[0].location = GET_POISON_DAMAGE_TYPE(i);
            af2[0].bitvector = AFF_POISON;
            af2[0].duration = 480;
            af2[0].modifier = -poisonmod;
            af2[0].level = af->level;
          
            if (GET_POISON_DAMAGE_TYPE(i) == APPLY_CON) {
              af2[1].type = SPELL_POISON;
              af2[1].location = APPLY_HIT;
              af2[1].bitvector = AFF_POISON;
              af2[1].duration = 480;
              af2[1].modifier = ((poisonmod / 2) * GET_LEVEL(i));
              af2[1].level = af->level;
              affect_join(i, &af2[1], false, false, true, false);
            }
					  
            affect_join(i, &af2[0], false, false, true, false);
            if (GET_CON(i) < 1 || GET_MAX_HIT(i) < 1)
            	die(i, i);
            if (GET_STR(i) < 1 || GET_DEX(i) < 1 || GET_INT(i) < 1 ||
                GET_WIS(i) < 1 || GET_CHA(i) < 1)
              SET_BIT_AR(AFF_FLAGS(i), AFF_STUNNED);
          }
        }    	  		
	  	}
	  }

	  if (affected_by_spell(i, SPELL_ACID_ARROW)) {
        for (af = i->affected; af; af = af->next) {
          if (af->type == SPELL_ACID_ARROW) {
              dam = dice(2, 4);
              sprintf(dammes, "@yDeadly acid sears your flesh for @R%d@y points of damage!@n", dam);
              act(dammes, false, i, 0, i, TO_VICT);
              sprintf(dammes, "@yDeadly acid sears the flesh of $N for @R%d@y points of damage!@n", dam);
              act(dammes, false, i, 0, i, TO_ROOM);
              dam = damage(i, i, dam, SPELL_ACID_ARROW, 0, -1, 0, SPELL_ACID_ARROW, 1);
              af->duration--;
              if (af->duration < 1)
              	affect_from_char(i, SPELL_ACID_ARROW);
          }
       }
	  }
	
    calculate_current_weight(i);

    if (!FIGHTING(i))
      i->paralyzed = 0;

    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
      GET_KI(i) = MIN(GET_KI(i) + ki_gain(i), GET_MAX_KI(i));
      
      if (GET_POS(i) == POS_STUNNED) {
        if (damage(i, i, 1, TYPE_SUFFERING, 0, -1, 0, 0, 0) == -1)
          continue;
      }
      update_pos(i);           
      } else if (GET_POS(i) == POS_INCAP) {
        update_pos(i);           
      if (damage(i, i, 1, TYPE_SUFFERING, 0, -1, 0, 0, 0) == -1)
	      continue;
    } else if (GET_POS(i) == POS_MORTALLYW) {
        for (vict = world[IN_ROOM(i)].people; vict; vict = vict->next_in_room)
          if (FIGHTING(vict) == i)
            fighting = TRUE;
        update_pos(i);           
        if (fighting)
          continue;
        if (damage(i, i, 2, TYPE_SUFFERING, 0, -1, 0, 0, 0) == -1)
	        continue;
    }

    if (!IS_NPC(i)) {
      update_char_objects(i);
      update_innate(i);
      if (GET_ADMLEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
	      check_idling(i);
    }
  }



  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if (IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, IN_ROOM(j->carried_by));
	  else if (IN_ROOM(j) != NOWHERE)
	    obj_to_room(jj, IN_ROOM(j));
	  else
	    core_dump();
	}
	extract_obj(j);
      }
    }
    else if (GET_OBJ_TYPE(j) == ITEM_PORTAL) {
      if (GET_OBJ_TIMER(j) > 0) {
          GET_OBJ_TIMER(j)--;

        if (!GET_OBJ_TIMER(j)) {
          act("A glowing portal fades from existence.",
    	   TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
          act("A glowing portal fades from existence.",
    	   TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
          extract_obj(j);
        }
      }
    }
    else if (GET_OBJ_TIMER(j)>0) {
      /* If the timer is set, count it down and at 0, try the trigger */
      /* note to .rej hand-patchers: make this last in your point-update() */
      GET_OBJ_TIMER(j)--; 
      if (!GET_OBJ_TIMER(j))
        timer_otrigger(j);
    }
  }
}

void timed_dt(struct char_data *ch)
{ 
  struct char_data *vict;
  room_rnum rrnum;
  
  if (ch == NULL) {
  /* BY -WELCOR
    first make sure all rooms in the world have thier 'timed'
    value decreased if its not -1.
    */
  
  for (rrnum = 0; rrnum < top_of_world; rrnum++)
    world[rrnum].timed -= (world[rrnum].timed != -1);
  
  for (vict = character_list; vict ;vict = vict->next){
    if (IS_NPC(vict))
      continue;   

    if (IN_ROOM(vict) == NOWHERE)
      continue;
    
    if(!ROOM_FLAGGED(IN_ROOM(vict), ROOM_TIMED_DT))
      continue;
    
    timed_dt(vict);
   }
  return;
  }
  
  /*Called with a non-null ch. let's check the room. */
  
  /*if the room wasn't triggered (i.e timed wasn't set), just set it
    and return again.
  */
  
  if (world[IN_ROOM(ch)].timed < 0) {
    world[IN_ROOM(ch)].timed = rand_number(2, 5);
    return;
  }
  
  /* We know ch is in a dt room with timed >= 0 - see if its the end.
  *
  */
  if (world[IN_ROOM(ch)].timed == 0) {
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
      if (IS_NPC(vict))
        continue;
      if (GET_ADMLEVEL(vict) >= ADMLVL_IMMORT)
        continue;

      /* Skip those alread dead people */
      /* extract char() jest sets the bit*/
      if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
        continue;
             
      log_death_trap(vict);
      death_cry(vict);
      extract_char(vict);
    }
  }
}

void apply_milestone_bonus(struct char_data *ch)
{

  int number;
  int number2;

  number = dice(1, 100);

  if (number <= 15) {
    GET_MAX_HIT(ch) += 1;
    GET_HIT(ch) += 1;
    send_to_char(ch, "@YYou have gained 1 hit point!@n\r\n");
    return;
  }
  else if (number <= 20) {
    GET_MAX_HIT(ch) += 2;
    GET_HIT(ch) += 2;
    send_to_char(ch, "@YYou have gained 2 hit points!@n\r\n");
    return;
  }
  else if (number == 21) {
    GET_MAX_HIT(ch) += 3;
    GET_HIT(ch) += 3;
    send_to_char(ch, "@YYou have gained 3 hit points!@n\r\n");
    return;
  }
  else if (number <= 41) {
    GET_MAX_MOVE(ch) += 1;
    GET_MOVE(ch) += 1;
    send_to_char(ch, "@YYou have gained 1 stamina point!@n\r\n");
    return;
  }
  else if (number <= 51) {
    GET_MAX_MOVE(ch) += 2;
    GET_MOVE(ch) += 2;
    send_to_char(ch, "@YYou have gained 2 stamina points!@n\r\n");
    return;
  }
  else if (number <= 55) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += 1;
    send_to_char(ch, "@YYou have gained 1 skill point!@n\r\n");
    return;
  }
  else if (number <= 58) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += 1;
    send_to_char(ch, "@YYou have gained 2 skill points!@n\r\n");
    return;
  }
  else if (number <= 59) {
    GET_FEAT_POINTS(ch) += 1;
    send_to_char(ch, "@YYou have gained 1 feat point!@n\r\n");
    return;
  }
  else if (number <= 60 && (number2 = dice(1, 100)) <= 50) {
    GET_TRAINS(ch) += 1;
    send_to_char(ch, "@YYou have gained 1 ability training point!@n\r\n");
    return;
  }
  else if (number <= 65) {
    GET_BANK_GOLD(ch) += GET_LEVEL(ch) * 1000;
    send_to_char(ch, "A grateful citizen has rewarded you with %d %s! (check bank)@n\r\n", GET_LEVEL(ch) * 10, MONEY_STRING);
    return;
  }
  else if (number <= 73) {
  	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_10) || AFF_FLAGGED(ch, AFF_EXP_BONUS_25) || AFF_FLAGGED(ch, AFF_EXP_BONUS_33) || AFF_FLAGGED(ch, AFF_EXP_BONUS_50)) {
  		apply_milestone_bonus(ch);  	
  		return;
  	}
  	SET_BIT_AR(AFF_FLAGS(ch), AFF_EXP_BONUS_10);
  	send_to_char(ch, "You gain a modest understanding from your session and will learn slightly more from each encounter for a while.@n\r\n");
  	return;
  }
  else if (number <= 90) {
  	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_10) || AFF_FLAGGED(ch, AFF_EXP_BONUS_25) || AFF_FLAGGED(ch, AFF_EXP_BONUS_33) || AFF_FLAGGED(ch, AFF_EXP_BONUS_50)) {
  		apply_milestone_bonus(ch);
  		return;
  	}
  	SET_BIT_AR(AFF_FLAGS(ch), AFF_EXP_BONUS_25);
  	send_to_char(ch, "You gain a decent understanding from your session and will learn more from each encounter for a while.@n\r\n");
  	return;
  }
  else if (number <= 97) {
  	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_10) || AFF_FLAGGED(ch, AFF_EXP_BONUS_25) || AFF_FLAGGED(ch, AFF_EXP_BONUS_33) || AFF_FLAGGED(ch, AFF_EXP_BONUS_50)) {
  		apply_milestone_bonus(ch);  	
  		return;
  	}
  	SET_BIT_AR(AFF_FLAGS(ch), AFF_EXP_BONUS_33);
  	send_to_char(ch, "You gain a good understanding from your session and will learn significantly more from each encounter for a while.@n\r\n");
  	return;
  }
  else if (number <= 100) {
  	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_10) || AFF_FLAGGED(ch, AFF_EXP_BONUS_25) || AFF_FLAGGED(ch, AFF_EXP_BONUS_33) || AFF_FLAGGED(ch, AFF_EXP_BONUS_50)) {
  		apply_milestone_bonus(ch);  	
  		return;
  	}  	
  	SET_BIT_AR(AFF_FLAGS(ch), AFF_EXP_BONUS_50);  	
  	send_to_char(ch, "You gain a great understanding from your session and will learn much more from each encounter for a while.@n\r\n");
  	return;
  }        
  else
    apply_milestone_bonus(ch);  

  return;
}

int group_align_exp_mod(struct char_data *ch) {

  if (IS_NEUTRAL(ch))
    return 0;

  int unlike = FALSE;
  int opposed = FALSE;
  struct char_data *k;
  struct follow_type *f;
  int tot_members = 0;

  if (ch->master)
    k = ch->master;
  else
    k = ch;

  if (k != ch && !IS_NPC(k) && ((IS_GOOD(ch) && IS_EVIL(k)) || (IS_EVIL(ch) && IS_GOOD(k))))
    opposed = TRUE;
  if (k != ch && !IS_NPC(k) && ((IS_NEUTRAL(ch) && !IS_NEUTRAL(k)) || (!IS_NEUTRAL(ch) && IS_NEUTRAL(k))))
    unlike = TRUE;
  tot_members++;

  for (f = k->followers; f; f = f->next) {
    if (IS_NPC(f->follower))
      continue;
    if (f->follower != ch && ((IS_GOOD(ch) && IS_EVIL(f->follower)) || (IS_EVIL(ch) && IS_GOOD(f->follower))))
      opposed = TRUE;
    if (f->follower != ch && ((IS_NEUTRAL(ch) && !IS_NEUTRAL(f->follower)) || (!IS_NEUTRAL(ch) && IS_NEUTRAL(f->follower))))
      unlike = TRUE;
    tot_members++;
  }

  if (tot_members == 1)
    return 0;

  if (opposed)
    return -25;
  else if (unlike)
    return 0;
  else
    return 15;
}

void award_rp_exp(void)
{
  struct char_data *i, *next_char;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    award_rp_exp_char(i);
  }
}

void award_rp_exp_char(struct char_data *i)
{
  long exp = 0;
    if (GET_RP_EXP(i) > 0) {
      time_t mytime;
      int d, h, m;

      mytime = time(0);

      d = mytime / 86400;
      h = (mytime / 3600) % 24;
      m = (mytime / 60) % 60;

      h -= 5; // Server time zone
      if (h < 0)
        h += 24;

      if (h == 11 || h == 12 || h == 19 || h == 20 || h == 3 || h == 4) {
        GET_RP_EXP(i) *= 2;
      }

      exp = level_exp(GET_LEVEL(i) + 1, GET_REAL_RACE(i)) - level_exp(GET_LEVEL(i), GET_REAL_RACE(i));
      exp *= GET_RP_EXP(i);
      exp /= 25000 + (GET_LEVEL(i) * 1000);
      if (exp < 0) {
        exp += -(exp * 2);
      }
      if (GET_RP_POINTS(i) < 5000)
      send_to_char(i, "\r\n\r\n"
                       "@RYou have just gained experience for role playing.  If you are not intending to role play, please\r\n"
                       "speak using the ooc or chat command.  Anyone caught trying to abuse this system in any way (and you will\r\n"
                       "be caught eventually as everything for which rp exp is awarded is logged and reviewed) will\r\n"
                       "have their account and all characters deleted, and will be banned for at least 6 months.\r\n"
                       "Examples of attempts to abuse the system include, but are not limited to: spamming says or\r\n"
                       "emotes, using the say command repeatedly to communicate out of character (see help rp), doing\r\n"
                       "says or emotes when no one else is around, or listening, etc.\r\n\r\n@n");
      send_to_char(i, "\r\n@YYou have gained experience for role playing. (Please read and obey HELP RULES ROLEPLAY)\r\n");

      if (h == 11 || h == 12 || h == 19 || h == 20 || h == 3 || h == 4) {
        send_to_char(i, "You are role playing during role play hour!  Experience gains are doubled!\r\n");
      } else {
        send_to_char(i, "Next role play hour at hour %d.  Current hour is %d.  Experience rewards doubled during this two-hour period.\r\n",
                     ((h / 8) * 8) + 3, h);
      }

      gain_exp(i, exp);
      int account_exp = (GET_RP_EXP(i) / 5) * (100 + (GET_RP_ACCOUNT_EXP(i) * 5)) / 100;
      if (i->desc && i->desc->account)
        i->desc->account->experience += account_exp;
      send_to_char(i, "\r\n@YYou have gained %d account experience for your role playing.\r\n", account_exp);
      send_to_char(i, "\r\n@YYou have gained %d rp points for your role playing.\r\n", GET_RP_EXP(i));
      award_rp_points(i, GET_RP_POINTS(i), GET_RP_EXP(i));
      GET_RP_EXP(i) = 0;



    }

}

int get_rp_bonus(struct char_data *ch, int type)
{

  if (!ch)
    return 0;

  int num = 0;

  switch (type) {
    case RP_EXP:
      num = GET_RP_POINTS(ch);
      num /= 10000;
      return num;
    case RP_ART_EXP:
      num = GET_RP_POINTS(ch);
      num /= 5000;
      return num;
    case RP_GOLD:
      num = GET_RP_POINTS(ch);
      num /= 10000;
      return num;
    case RP_QUEST:
      num = GET_RP_POINTS(ch);
      num /= 1000;
      return num;
    case RP_CRAFT:
      num = GET_RP_POINTS(ch);
      num /= 10000;
      return num;
    case RP_ACCOUNT:
      num = GET_RP_POINTS(ch);
      num /= 2000;
      return num;
  }

  return 0;

}
