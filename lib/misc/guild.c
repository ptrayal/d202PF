/* ************************************************************************
 *   File: Guild.c                                                         *
 *  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
 *                        practicing.                                      *
 *                                                                         *
 * Based on shop.c.  As such, the CircleMud License applies                *
 * Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
 ************************************************************************ */


#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/guild.c,v 1.25 2005/01/03 18:01:09 fnord Exp $");

#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "constants.h"
#include "db.h"
#include "shop.h"
#include "guild.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "screen.h"
#include "gengld.h"
#include "feats.h"


/* extern variables */
extern struct spell_info_type spell_info[];
extern struct time_info_data time_info;
extern const char *trade_letters[];
extern int cmd_say, cmd_tell;
extern const char *class_names[];
extern const int *class_bonus_feats[];

/* extern function prototypes */
ACMD(do_tell);
ACMD(do_say);
int level_exp(int level);
void gain_level(struct char_data *ch, int whichclass);
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg);
int find_feat_num(char *name);
int get_skill_mod(struct char_data *ch, int skillnum);

/* Local variables */
struct guild_data *guild_index;
int spell_sort_info[SKILL_TABLE_SIZE + 1];
int top_guild = -1;
char *guild_customer_string(int guild_nr, int detailed);

const char *how_good(int percent)
{
  if (percent < 0)
    return " error)";
  if (percent == 0)
    return "(@Mnot@n)";
  if (percent <= 10)
    return "(@rawful@n)";
  if (percent <= 20)
    return "(@Rbad@n)";
  if (percent <= 40)
    return "(@ypoor@n)";
  if (percent <= 55)
    return "(@Yaverage@n)";
  if (percent <= 70)
    return "(@gfair@n)";
  if (percent <= 80)
    return "(@Ggood@n)";
  if (percent <= 85)
    return "(@bgreat@n)";
  if (percent <= 100)
    return "(@Bsuperb@n)";

  return "(@rinate@n)";
}

const char *prac_types[] = {
  "spell",
  "skill"
};


int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}


int print_skills_by_type(struct char_data *ch, char *buf, int maxsz, int sktype)
{
  size_t len = 0;
  int i, t, known, nlen, roll = 0;
  char buf2[READ_SIZE];

  for (i = 1; i <= SKILL_TABLE_SIZE; i++) { 
    t = spell_info[i].skilltype;

    if (t != sktype)
      continue;

    if ((t & SKTYPE_SKILL) || (t & SKTYPE_SPELL)) {
      for (nlen = 0, known = 0; nlen < NUM_CLASSES; nlen++)
	if (spell_info[i].can_learn_skill[nlen] > known)
	  known = spell_info[i].can_learn_skill[nlen];
    } else {
      known = 0;
    }

    if (known) {
      if (t & SKTYPE_LANG) {
	nlen = snprintf(buf + len, maxsz - len, "%-20s  (%s)\r\n",
                        spell_info[i].name, GET_SKILL_BASE(ch, i) ? "known" : "unknown");
      } else if (t & SKTYPE_SKILL) {
        if (GET_SKILL_BONUS(ch, i) + (roll = get_skill_mod(ch, i)))
          snprintf(buf2, sizeof(buf2), " (base %d + bonus %d)", GET_SKILL_BASE(ch, i),
                   GET_SKILL_BONUS(ch, i));
        else
          buf2[0] = 0;
        if (spell_info[i].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CROSSCLASS) {
	      nlen = snprintf(buf + len, maxsz - len, "@W%-20s  %d%s@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i), buf2);
        }
        else
   	      nlen = snprintf(buf + len, maxsz - len, "@G%-20s  %d%s@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i), buf2);
        
      }
      else
	nlen = snprintf(buf + len, maxsz - len, "%-20s  unknown type\r\n",
                        spell_info[i].name);
      if (len + nlen >= maxsz || nlen < 0)
        break;
      len += nlen;
    }
  }

  return len;
}

void list_skills(struct char_data *ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  size_t len = 0;
  char buf2[MAX_STRING_LENGTH];

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch, GET_CLASS(ch)), GET_PRACTICES(ch, GET_CLASS(ch)) == 1 ? "" : "s");

  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nYou know the following skills:\r\n");

  len += print_skills_by_type(ch, buf2 + len, sizeof(buf2) - len, SKTYPE_SKILL);

  if (CONFIG_ENABLE_LANGUAGES) {
    len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nand the following languages:\r\n");
    len += print_skills_by_type(ch, buf2 + len, sizeof(buf2) - len, SKTYPE_SKILL | SKTYPE_LANG);
  }
  
  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\n@WWhite@n denotes cross-class skill, @GGreen@n denotes class skill.\r\n");

  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


int is_guild_open(struct char_data *keeper, int guild_nr, int msg)
{
  char buf[200];
  *buf = 0;

  if (GM_OPEN(guild_nr) > time_info.hours &&
    GM_CLOSE(guild_nr) < time_info.hours)
  strlcpy(buf, MSG_TRAINER_NOT_OPEN, sizeof(buf));

  if (!*buf)
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);

  return (FALSE);
}


int is_guild_ok_char(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	char buf[200];

	if (!(CAN_SEE(keeper, ch))) {
		do_say(keeper, MSG_TRAINER_NO_SEE_CH, cmd_say, 0);
		return (FALSE);
	}

	
	if (GET_LEVEL(ch) > GM_MINLVL(guild_nr)) {
				snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), "I am sorry but you have learned all that I can teach you.");
		do_tell(keeper, buf, cmd_tell, 0); 
		return (FALSE);
	}


	if ((IS_GOOD(ch) && NOTRAIN_GOOD(guild_nr)) ||
		 (IS_EVIL(ch) && NOTRAIN_EVIL(guild_nr)) ||
		 (IS_NEUTRAL(ch) && NOTRAIN_NEUTRAL(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if (IS_NPC(ch))
		return (FALSE);

	if ((IS_WIZARD(ch) && NOTRAIN_WIZARD(guild_nr)) ||
		(IS_CLERIC(ch) && NOTRAIN_CLERIC(guild_nr)) ||
		(IS_ROGUE(ch) && NOTRAIN_ROGUE(guild_nr)) ||
		(IS_FIGHTER(ch) && NOTRAIN_FIGHTER(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((!IS_WIZARD(ch) && TRAIN_WIZARD(guild_nr)) ||
	    (!IS_CLERIC(ch) && TRAIN_CLERIC(guild_nr)) ||
	    (!IS_ROGUE(ch) && TRAIN_ROGUE(guild_nr)) ||
	    (!IS_MONK(ch) && TRAIN_MONK(guild_nr)) ||
	    (!IS_PALADIN(ch) && TRAIN_PALADIN(guild_nr)) ||
	    (!IS_FIGHTER(ch) && TRAIN_FIGHTER(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), "I'm sorry, but I don't train members of your profession");
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((IS_HUMAN(ch) && NOTRAIN_HUMAN(guild_nr)) ||
		 (IS_ELF(ch)   && NOTRAIN_ELF(guild_nr)) ||
		 (IS_GNOME(ch) && NOTRAIN_GNOME(guild_nr)) ||
		 (IS_DWARF(ch) && NOTRAIN_DWARF(guild_nr)) ||
		 (IS_HALF_ELF(ch) && NOTRAIN_HALF_ELF(guild_nr)) ||
		 (IS_HALFLING(ch) && NOTRAIN_HALFLING(guild_nr)) ||
		 (IS_DROW_ELF(ch) && NOTRAIN_DROW_ELF(guild_nr)) ||
 		 (IS_ANIMAL(ch) && NOTRAIN_ANIMAL(guild_nr)) ||
		 (IS_CONSTRUCT(ch) && NOTRAIN_CONSTRUCT(guild_nr)) ||
		 (IS_DEMON(ch) && NOTRAIN_DEMON(guild_nr)) ||
 		 (IS_DRAGON(ch) && NOTRAIN_DRAGON(guild_nr)) ||
 		 (IS_FISH(ch) && NOTRAIN_FISH(guild_nr)) ||
 		 (IS_GIANT(ch) && NOTRAIN_GIANT(guild_nr)) ||
 		 (IS_GOBLIN(ch) && NOTRAIN_GOBLIN(guild_nr)) ||
 		 (IS_INSECT(ch) && NOTRAIN_INSECT(guild_nr)) ||
 		 (IS_ORC(ch) && NOTRAIN_ORC(guild_nr)) ||
 		 (IS_SNAKE(ch) && NOTRAIN_SNAKE(guild_nr)) ||
		 (IS_TROLL(ch) && NOTRAIN_TROLL(guild_nr)) ||
 		 (IS_HALF_ORC(ch) && NOTRAIN_HALF_ORC(guild_nr)) ||
 		 (IS_MINOTAUR(ch) && NOTRAIN_MINOTAUR(guild_nr)) ||
 		 (IS_KOBOLD(ch) && NOTRAIN_KOBOLD(guild_nr)) ||
		 (IS_LIZARDFOLK(ch) && NOTRAIN_LIZARDFOLK(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_RACE);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_guild_ok(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	if (is_guild_open(keeper, guild_nr, TRUE))
		return (is_guild_ok_char(keeper, ch, guild_nr));

	return (FALSE);
}


int does_guild_know(int guild_nr, int i)
{
  return ((int)(guild_index[guild_nr].skills[i]));
}


void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < SKILL_TABLE_SIZE; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], SKILL_TABLE_SIZE, sizeof(int), compare_spells);
}


/* this and list skills should probally be combined.  perhaps in the
 * next release?  */
void what_does_guild_know(int guild_nr, struct char_data * ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  char buf2[MAX_STRING_LENGTH];
  int i, sortpos, canknow, j;
  size_t nlen = 0, len = 0;

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch, GET_CLASS(ch)), GET_PRACTICES(ch, GET_CLASS(ch)) == 1 ? "" : "s"); // ???

  nlen = snprintf(buf2 + len, sizeof(buf2) - len, "I can teach you the following skills:\r\n");
  len += nlen;
  
  /* Need to check if trainer can train doesnt do it now ??? */
  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, i) && skill_type(i) == SKTYPE_SKILL) {
      //if (GET_LEVEL(ch) >= spell_info[i].min_level[(int)GET_CLASS(ch)]) {
      for (canknow = 0, j = 0; j < NUM_CLASSES; j++) 
	    if (spell_info[i].can_learn_skill[j] > canknow)
	      canknow = spell_info[i].can_learn_skill[j];
      canknow = highest_skill_value(GET_LEVEL(ch), canknow);
      if (spell_info[i].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CROSSCLASS) {
	      nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@W%-20s  %d@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i));
      }
      else
 	      nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@G%-20s  %d@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i));                               
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  if (CONFIG_ENABLE_LANGUAGES) {
  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nand the following languages:\r\n");

  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, i) && IS_SET(skill_type(i), SKTYPE_LANG)) {
      //if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
        for (canknow = 0, j = 0; j < NUM_CLASSES; j++)
	  if (spell_info[i].can_learn_skill[j] > canknow)
	    canknow = spell_info[i].can_learn_skill[j];
        canknow = highest_skill_value(GET_LEVEL(ch), canknow);
        if (GET_SKILL_BASE(ch, i) < canknow) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, GET_SKILL_BASE(ch, i) ? "known" : "unknown");
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      //}
    }
  }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


void handle_practice(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int skill_num, learntype, pointcost, highest, i;
  char buf[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    what_does_guild_know(guild_nr, ch);
    return;
  }

  if (GET_PRACTICES(ch, GET_CLASS(ch)) <= 0) {
    send_to_char(ch, "You do not seem to be able to practice now.\r\n");
    return;
  }

  skill_num = find_skill_num(argument, SKTYPE_SKILL);

  /****  Does the GM know the skill the player wants to learn?  ****/
  if (!(does_guild_know(guild_nr, skill_num))) {
    snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }

  /**** Can the player learn the skill if the GM knows it?  ****/ 
  if (IS_SET(spell_info[skill_num].skilltype, SKTYPE_SKILL)) {
    for (learntype = 0, i = 0; i < NUM_CLASSES; i++)
      if (spell_info[skill_num].can_learn_skill[i] > learntype)
        learntype = spell_info[skill_num].can_learn_skill[i];
    switch (learntype) {
    case SKLEARN_CANT:
      send_to_char(ch, "You cannot learn that.\r\n");
      return;
    case SKLEARN_CROSSCLASS:
      highest = highest_skill_value(GET_LEVEL(ch), SKLEARN_CROSSCLASS);
      break;
    case SKLEARN_CLASS:
      highest = highest_skill_value(GET_LEVEL(ch), learntype);
      break;
    default:
      log("Unknown SKLEARN type for skill %d in practice", skill_num);
      send_to_char(ch, "You can't learn that.\r\n");
      return;
    }
    if (spell_info[skill_num].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CLASS)
      pointcost = 1;
    else
      pointcost = 2;
    if (GET_PRACTICES(ch, GET_CLASS(ch)) >= pointcost) {
      if (GET_SKILL_BASE(ch, skill_num) >= highest) {
        send_to_char(ch, "You cannot increase that skill again until you progress further.\r\n");
        return;
      } else {
        send_to_char(ch, "You practice for a while...\r\n");
        SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + 1);
        GET_PRACTICES(ch, GET_CLASS(ch)) -= pointcost;
      }
    } else {
      send_to_char(ch, "You need %d skill point%s to increase your skill.\r\n",
             pointcost, (pointcost == 1) ? "" : "s");
    }
  } else {
    send_to_char(ch, "You can't learn that.\r\n");
  }
}


void handle_train(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  skip_spaces(&argument);
  if (!argument || !*argument)
    send_to_char(ch, "Training sessions remaining: %d\r\n"
                 "Stats: strength constitution dexterity intelligence wisdom charisma\r\n",
                 GET_TRAINS(ch));
  else if (!GET_TRAINS(ch))
    send_to_char(ch, "You have no ability training sessions.\r\n");
  else if (!strncasecmp("strength", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.str += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("constitution", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.con += 1;
    /* Give them retroactive hit points for constitution */
    if (! (ch->real_abils.con % 2))
      GET_MAX_HIT(ch) += GET_LEVEL(ch);
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("dexterity", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.dex += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("intelligence", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.intel += 1;
    /* Give extra skill practice, but only for this level */
    if (! (ch->real_abils.intel % 2))
      GET_PRACTICES(ch, GET_CLASS(ch)) += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("wisdom", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.wis += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("charisma", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.cha += 1;
    GET_TRAINS(ch) -= 1;
  } else
    send_to_char(ch, "Stats: strength constitution dexterity intelligence wisdom charisma\r\n");
  affect_total(ch);
  return;
}


void handle_gain(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int whichclass, i;

  skip_spaces(&argument);
  if (CONFIG_ALLOW_MULTICLASS) {
    if (!*argument) {
      send_to_char(ch, "You must specify a class you want to train.\r\n"
                   "You are eligible for these classes:\r\n");
      for (i = 0; i < NUM_CLASSES; i++) {
        if (class_ok_general(ch, i))
          send_to_char(ch, "  %s\r\n", pc_class_types[i]);
      }
      return;
    }
    if ((whichclass = search_block(argument, class_names, FALSE)) < 0) {
      send_to_char(ch, "That is not a class.\r\n");
      return;
    }
  } else {
    whichclass = GET_CLASS(ch);
  }
  if (! class_ok_general(ch, whichclass)) {
    send_to_char(ch, "You cannot progress in that class.\r\n");
    return;
  }
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1 &&
    (((GET_EXP(ch) >= level_exp(GET_LEVEL(ch))) && GET_LEVEL_STAGE(ch) == 4) ||
    ((GET_EXP(ch) - level_exp(GET_LEVEL(ch)) >= ((level_exp(GET_LEVEL(ch) + 1) - level_exp(GET_LEVEL(ch))) * 20 / 100)) &&
            (GET_LEVEL_STAGE(ch) == 0)) ||
    ((GET_EXP(ch) - level_exp(GET_LEVEL(ch)) >= ((level_exp(GET_LEVEL(ch) + 1) - level_exp(GET_LEVEL(ch))) * 40 / 100)) &&
            (GET_LEVEL_STAGE(ch) == 1)) ||
    ((GET_EXP(ch) - level_exp(GET_LEVEL(ch)) >= ((level_exp(GET_LEVEL(ch) + 1) - level_exp(GET_LEVEL(ch))) * 60 / 100)) &&
            (GET_LEVEL_STAGE(ch) == 2)) ||
    ((GET_EXP(ch) - level_exp(GET_LEVEL(ch)) >= ((level_exp(GET_LEVEL(ch) + 1) - level_exp(GET_LEVEL(ch))) * 80 / 100)) &&
            (GET_LEVEL_STAGE(ch) == 3))))
  {
    if (GET_LEVEL_STAGE(ch) != 4 && whichclass != GET_CLASS(ch)) {
      send_to_char(ch, "You must progress in the same class for each milestone of this level.\r\n");
      return;
    }
    else
      gain_level(ch, whichclass);
  } else {
    send_to_char(ch, "You are not yet ready for further advancement.\r\n");
  }
  return;
}

void handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int feat_num, subval, sftype, subfeat;
  int classFeat = false, featMarker = 1, featCounter = 0;
  char *ptr, buf[MAX_STRING_LENGTH];

  if (!*argument) {
    send_to_char(ch, "Which feat would you like to learn?\r\n");
    return;
  }

  if (MAX(GET_FEAT_POINTS(ch), GET_CLASS_FEATS(ch, GET_CLASS(ch))) < 1) {
    send_to_char(ch, "You can't learn any new feats right now.\r\n");
    return;
  }

  if (!GET_FEAT_POINTS(ch) && !GET_CLASS_FEATS(ch, GET_CLASS(ch))) {
    send_to_char(ch, "You do not have any feat points to spend.\r\n");
    return;
  }

  ptr = strchr(argument, ':');
  if (ptr)
    *ptr = 0;
  feat_num = find_feat_num(argument);
  if (ptr)
    *ptr = ':';

  if (HAS_FEAT(ch, feat_num) && !feat_list[feat_num].can_stack) {
    send_to_char(ch, "You already know the %s feat.\r\n", feat_list[feat_num].name);
    return;
  }

  if (!feat_is_available(ch, feat_num, 0, NULL) || !feat_list[feat_num].in_game || !feat_list[feat_num].can_learn) {
    send_to_char(ch, "The %s feat is not available to you at this time.\r\n", argument);
    return;
  }

  sftype = 2;
  switch (feat_num) {
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
  case FEAT_GREATER_WEAPON_FOCUS:
  case FEAT_WEAPON_SPECIALIZATION:
  case FEAT_WEAPON_FOCUS:
  case FEAT_WEAPON_FINESSE:
  case FEAT_IMPROVED_CRITICAL:
    sftype = 1;
  case FEAT_SPELL_FOCUS:
  case FEAT_GREATER_SPELL_FOCUS:
    subfeat = feat_to_subfeat(feat_num);
    if (subfeat == -1) {
      log("guild: Unconfigured subfeat '%s', check feat_to_subfeat()", feat_list[feat_num].name);
      send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return;
    }
    if (!ptr || !*ptr) {
      if (sftype == 2)
        ptr = "spell school";
      else
        ptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "No ':' found. You must specify a %s to improve. Example:\r\n"
                         " learn %s: %s\r\nAvailable %s:\r\n", ptr,
                         feat_list[feat_num].name,
                         (sftype == 2) ? spell_schools[0] : weapon_type[0], ptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          ptr = (char *)spell_schools[subval];
        else
          ptr = (char *)weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", ptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    if (*ptr == ':') ptr++;
    skip_spaces(&ptr);
    if (!ptr || !*ptr) {
      if (sftype == 2)
        ptr = "spell school";
      else
        ptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "No %s found. You must specify a %s to improve.\r\n\r\nExample:\r\n"
                         " learn %s: %s\r\n\r\nAvailable %s:\r\n", ptr, ptr,
                         feat_list[feat_num].name,
                         (sftype == 2) ? spell_schools[0] : weapon_type[0], ptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          ptr = (char *)spell_schools[subval];
        else
          ptr = (char *)weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", ptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    subval = search_block(ptr, (sftype == 2) ? spell_schools : weapon_type, FALSE);
    if (subval == -1) {
      log("bad subval: %s", ptr);
      if (sftype == 2)
        ptr = "spell school";
      else
        ptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "That is not a known %s. Available %s:\r\n",
                         ptr, ptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          ptr = (char *)spell_schools[subval];
        else
          ptr = (char *)weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", ptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    if (!feat_is_available(ch, feat_num, subval, NULL)) {
      send_to_char(ch, "You do not satisfy the prerequisites for that feat.\r\n");
      return;
    }
    if (sftype == 1) {
      if (HAS_COMBAT_FEAT(ch, subfeat, subval)) {
        send_to_char(ch, "You already have that weapon feat.\r\n");
        return;
      }
      SET_COMBAT_FEAT(ch, subfeat, subval);
    } else if (sftype == 2) {
      if (HAS_SCHOOL_FEAT(ch, subfeat, subval)) {
        send_to_char(ch, "You already have that spell school feat.\r\n");
        return;
      }
      SET_SCHOOL_FEAT(ch, subfeat, subval);
    } else {
      log("unknown feat subtype %d in subfeat code", sftype);
      send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return;
    }
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  case FEAT_GREAT_FORTITUDE:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_FORTITUDE) += 2;
    break;
  case FEAT_IRON_WILL:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_WILL) += 2;
    break;
  case FEAT_LIGHTNING_REFLEXES:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_REFLEX) += 2;
    break;
  case FEAT_TOUGHNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_MAX_HIT(ch) += 10;
    break;
  case FEAT_SKILL_FOCUS:
    if (!ptr || !*ptr) {
      send_to_char(ch, "You must specify a skill to improve. Syntax:\r\n  learn skill focus: skill\r\n");
      return;
    }
    if (*ptr == ':') ptr++;
    skip_spaces(&ptr);
    if (!ptr || !*ptr) {
      send_to_char(ch, "You must specify a skill to improve. Syntax:\r\n  learn skill focus: skill\r\n");
      return;
    }
    subval = find_skill_num(ptr, SKTYPE_SKILL);
    if (subval < 0) {
      send_to_char(ch, "I don't recognize that skill.\r\n");
      return;
    }
    SET_SKILL_BONUS(ch, subval, GET_SKILL_BONUS(ch, subval) + 3);
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  case FEAT_SPELL_MASTERY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_SPELL_MASTERY_POINTS(ch) += MAX(1, ability_mod_value(GET_INT(ch)));
    break;
  case FEAT_ACROBATIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_JUMP, GET_SKILL_BONUS(ch, SKILL_JUMP) + 2);
    SET_SKILL_BONUS(ch, SKILL_TUMBLE, GET_SKILL_BONUS(ch, SKILL_TUMBLE) + 2);
    break;
  case FEAT_AGILE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_BALANCE, GET_SKILL_BONUS(ch, SKILL_BALANCE) + 2);
    SET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST, GET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST) + 2);
    break;
  case FEAT_ALERTNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_LISTEN, GET_SKILL_BONUS(ch, SKILL_LISTEN) + 2);
    SET_SKILL_BONUS(ch, SKILL_SPOT, GET_SKILL_BONUS(ch, SKILL_SPOT) + 2);
    break;
  case FEAT_ANIMAL_AFFINITY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HANDLE_ANIMAL, GET_SKILL_BONUS(ch, SKILL_HANDLE_ANIMAL) + 2);
    SET_SKILL_BONUS(ch, SKILL_RIDE, GET_SKILL_BONUS(ch, SKILL_RIDE) + 2);
    break;
  case FEAT_ATHLETIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_CLIMB, GET_SKILL_BONUS(ch, SKILL_CLIMB) + 2);
    SET_SKILL_BONUS(ch, SKILL_SWIM, GET_SKILL_BONUS(ch, SKILL_SWIM) + 2);
    break;
  case FEAT_DECEITFUL:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_DISGUISE, GET_SKILL_BONUS(ch, SKILL_DISGUISE) + 2);
    SET_SKILL_BONUS(ch, SKILL_FORGERY, GET_SKILL_BONUS(ch, SKILL_FORGERY) + 2);
    break;
  case FEAT_DEFT_HANDS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND, GET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND) + 2);
    SET_SKILL_BONUS(ch, SKILL_USE_ROPE, GET_SKILL_BONUS(ch, SKILL_USE_ROPE) + 2);
    break;
  case FEAT_DILIGENT:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_APPRAISE, GET_SKILL_BONUS(ch, SKILL_APPRAISE) + 2);
    SET_SKILL_BONUS(ch, SKILL_DECIPHER_SCRIPT, GET_SKILL_BONUS(ch, SKILL_DECIPHER_SCRIPT) + 2);
    break;
  case FEAT_INVESTIGATOR:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_GATHER_INFORMATION, GET_SKILL_BONUS(ch, SKILL_GATHER_INFORMATION) + 2);
    SET_SKILL_BONUS(ch, SKILL_SEARCH, GET_SKILL_BONUS(ch, SKILL_SEARCH) + 2);
    break;
  case FEAT_MAGICAL_APTITUDE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_SPELLCRAFT, GET_SKILL_BONUS(ch, SKILL_SPELLCRAFT) + 2);
    SET_SKILL_BONUS(ch, SKILL_USE_MAGIC_DEVICE, GET_SKILL_BONUS(ch, SKILL_USE_MAGIC_DEVICE) + 2);
    break;
  case FEAT_NEGOTIATOR:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_DIPLOMACY, GET_SKILL_BONUS(ch, SKILL_DIPLOMACY) + 2);
    SET_SKILL_BONUS(ch, SKILL_SENSE_MOTIVE, GET_SKILL_BONUS(ch, SKILL_SENSE_MOTIVE) + 2);
    break;
  case FEAT_NIMBLE_FINGERS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_DISABLE_DEVICE, GET_SKILL_BONUS(ch, SKILL_DISABLE_DEVICE) + 2);
    SET_SKILL_BONUS(ch, SKILL_OPEN_LOCK, GET_SKILL_BONUS(ch, SKILL_OPEN_LOCK) + 2);
    break;
  case FEAT_PERSUASIVE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_BLUFF, GET_SKILL_BONUS(ch, SKILL_BLUFF) + 2);
    SET_SKILL_BONUS(ch, SKILL_INTIMIDATE, GET_SKILL_BONUS(ch, SKILL_INTIMIDATE) + 2);
    break;
  case FEAT_SELF_SUFFICIENT:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HEAL, GET_SKILL_BONUS(ch, SKILL_HEAL) + 2);
    SET_SKILL_BONUS(ch, SKILL_SURVIVAL, GET_SKILL_BONUS(ch, SKILL_SURVIVAL) + 2);
    break;
  case FEAT_STEALTHY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HIDE, GET_SKILL_BONUS(ch, SKILL_HIDE) + 2);
    SET_SKILL_BONUS(ch, SKILL_MOVE_SILENTLY, GET_SKILL_BONUS(ch, SKILL_MOVE_SILENTLY) + 2);
    break;
  default:
    SET_FEAT(ch, feat_num, TRUE);
    break;
  }
  save_char(ch);

  while (featMarker != 0) {
    featMarker = class_bonus_feats[GET_CLASS(ch)][featCounter];
    if (feat_num == class_bonus_feats[GET_CLASS(ch)][featCounter] && GET_CLASS_FEATS(ch, GET_CLASS(ch)) > 0) {
      GET_CLASS_FEATS(ch, GET_CLASS(ch))--;
      classFeat = true;
    }
    featCounter++;
  }
  if (!classFeat)
    GET_FEAT_POINTS(ch)--;
  send_to_char(ch, "Your training has given you the %s feat!\r\n", feat_list[feat_num].name);

  return;
}


SPECIAL(guild)
{
  char arg[MAX_INPUT_LENGTH];
  int guild_nr, i;
  struct char_data *keeper = (struct char_data *) me;
  struct {
    const char *cmd;
    void (*func)(struct char_data *, int, struct char_data *, char *);
  } guild_cmd_tab[] = {
    { "practice",	handle_practice },
    { "gain",		handle_gain },
    { "train",		handle_train },
    { "learn",		handle_learn },
    { NULL,		NULL }
  };

  for (guild_nr = 0; guild_nr <= top_guild; guild_nr++)
    if (GM_TRAINER(guild_nr) == keeper->nr)
      break;

  if (guild_nr > top_guild)
    return (FALSE);

  if (GM_FUNC(guild_nr))
    if ((GM_FUNC(guild_nr)) (ch, me, cmd, arg))
      return (TRUE);

  /*** Is the GM able to train?    ****/
  if (!AWAKE(keeper))
    return (FALSE);

  for (i = 0; guild_cmd_tab[i].cmd; i++)
    if (CMD_IS(guild_cmd_tab[i].cmd))
      break;

  if (!guild_cmd_tab[i].cmd)
    return (FALSE);

  if (!(is_guild_ok(keeper, ch, guild_nr)))
    return (TRUE);

  (guild_cmd_tab[i].func)(keeper, guild_nr, ch, argument);

  return (TRUE);
}


/**** This function is here just because I'm extremely paranoid.  Take
      it out if you aren't ;)  ****/
void clear_skills(int index)
{
  int i;

  for  (i = 0; i < SKILL_TABLE_SIZE; i++) 
    guild_index[index].skills[i] = 0;
}


/****  This is ripped off of read_line from shop.c.  They could be
 *  combined. But why? ****/

void read_guild_line(FILE * gm_f, char *string, void *data, char *type)
{
	char buf[MAX_STRING_LENGTH];
	
	if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
		fprintf(stderr, "Error in guild #%d, Could not get %s\n", GM_NUM(top_guild), type);
		exit(1);
	}
}


void boot_the_guilds(FILE * gm_f, char *filename, int rec_count)
{
  char *buf, buf2[256], *p;
  int temp, val;
  int done = FALSE;

  snprintf(buf2, sizeof(buf2), "beginning of GM file %s", filename);

  buf = fread_string(gm_f, buf2);
  while (!done) {
    if (*buf == '#') {		/* New Trainer */
      sscanf(buf, "#%d\n", &temp);
      snprintf(buf2, sizeof(buf2), "GM #%d in GM file %s", temp, filename);
      free(buf);		/* Plug memory leak! */
      top_guild++;
      if (!top_guild)
	CREATE(guild_index, struct guild_data, rec_count);
      GM_NUM(top_guild) = temp;

      clear_skills(top_guild);    
      read_guild_line(gm_f, "%d", &temp, "TEMP1");
      while( temp > -1) {
	guild_index[top_guild].skills[(int)temp] = 1;
	read_guild_line(gm_f, "%d", &temp, "TEMP2");
      }
                   
      read_guild_line(gm_f, "%f", &GM_CHARGE(top_guild), "GM_CHARGE");

      guild_index[top_guild].no_such_skill = fread_string(gm_f, buf2);
      guild_index[top_guild].not_enough_gold = fread_string(gm_f, buf2);

      read_guild_line(gm_f, "%d", &GM_MINLVL(top_guild), "GM_MINLVL");
      read_guild_line(gm_f, "%d", &GM_TRAINER(top_guild), "GM_TRAINER");

      GM_TRAINER(top_guild) = real_mobile(GM_TRAINER(top_guild));
      read_guild_line(gm_f, "%d", &GM_WITH_WHO(top_guild)[0], "GM_WITH_WHO");

      read_guild_line(gm_f, "%d", &GM_OPEN(top_guild), "GM_OPEN");
      read_guild_line(gm_f, "%d", &GM_CLOSE(top_guild), "GM_CLOSE");

      GM_FUNC(top_guild) = NULL;
      CREATE(buf, char, READ_SIZE);
      get_line(gm_f, buf);
      if (buf && *buf != '#' && *buf != '$') {
	p = buf;
	for (temp = 1; temp < SW_ARRAY_MAX; temp++) {
	  if (!p || !*p)
	    break;
	  if (sscanf(p, "%d", &val) != 1) {
	    log("SYSERR: Can't parse GM_WITH_WHO line in %s: '%s'", buf2, buf);
	    break;
	  }
	  GM_WITH_WHO(top_guild)[temp] = val;
	  while (isdigit(*p) || *p == '-') {
	    p++;
	  }
	  while (*p && !(isdigit(*p) || *p == '-')) {
	    p++;
	  }
	}
	while (temp < SW_ARRAY_MAX)
	  GM_WITH_WHO(top_guild)[temp++] = 0;
	free(buf);
	buf = fread_string(gm_f, buf2);
      }
    } else {
      if (*buf == '$')		/* EOF */
	done = TRUE;
      free(buf);		/* Plug memory leak! */
    }
  }
}


void assign_the_guilds(void)
{
	int index;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");

	for (index = 0; index <= top_guild; index++) {
		if (GM_TRAINER(index) == NOBODY)
			continue;
		if (mob_index[GM_TRAINER(index)].func)
			GM_FUNC(index) = mob_index[GM_TRAINER(index)].func;
		mob_index[GM_TRAINER(index)].func = guild;
	}
}

char *guild_customer_string(int guild_nr, int detailed)
{
  int gindex = 0, flag = 0, nlen;
  size_t len = 0;
  static char buf[MAX_STRING_LENGTH];

  while (*trade_letters[gindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!IS_SET_AR(GM_WITH_WHO(guild_nr), flag)) {
	nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[gindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (IS_SET_AR(GM_WITH_WHO(guild_nr), flag) ? '_' : *trade_letters[gindex]);
      buf[len] = '\0';

      if (len >= sizeof(buf))
        break;
    }

    gindex++;
    flag += 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}

void list_all_guilds(struct char_data *ch)
{
  const char *list_all_guilds_header =
  "Virtual   G.Master	Charge   Members\r\n"
  "-------------------------------------------------------------\r\n";
  int gm_nr, headerlen = strlen(list_all_guilds_header);
  size_t len = 0;
  char buf[MAX_STRING_LENGTH], buf1[16];

  *buf = '\0';
  for (gm_nr = 0; gm_nr <= top_guild && len < sizeof(buf); gm_nr++) {
  /* New page in page_string() mechanism, print the header again. */
    if (!(gm_nr % (PAGE_LENGTH - 2))) {
    /*
     * If we don't have enough room for the header, or all we have room left
     * for is the header, then don't add it and just quit now.
     */
    if (len + headerlen + 1 >= sizeof(buf))
      break;
    strcpy(buf + len, list_all_guilds_header);	/* strcpy: OK (length checked above) */
    len += headerlen;
    }

    if (GM_TRAINER(gm_nr) == NOBODY)
      strcpy(buf1, "<NONE>");  /* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", mob_index[GM_TRAINER(gm_nr)].vnum);  /* sprintf: OK (for 'buf1 >= 11', 32-bit int) */	

    len += snprintf(buf + len, sizeof(buf) - len, "%6d	%s		%5.2f	%s\r\n",
      GM_NUM(gm_nr), buf1, GM_CHARGE(gm_nr), guild_customer_string(gm_nr, FALSE));
  }

  page_string(ch->desc, buf, TRUE);
}


void list_detailed_guild(struct char_data * ch, int gm_nr)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (GM_TRAINER(gm_nr) < 0)
    strcpy(buf1, "<NONE>");
  else
    sprintf(buf1, "%6d   ", mob_index[GM_TRAINER(gm_nr)].vnum);

  sprintf(buf, " Guild Master: %s\r\n", buf1);
  sprintf(buf, "%s Hours: %4d to %4d,  Surcharge: %5.2f\r\n", buf,
			  GM_OPEN(gm_nr), GM_CLOSE(gm_nr), GM_CHARGE(gm_nr));
  sprintf(buf, "%s Min Level will train: %d\r\n", buf, GM_MINLVL(gm_nr));
  sprintf(buf, "%s Whom will train: %s\r\n", buf, guild_customer_string(gm_nr, TRUE));

   /* now for the REAL reason why someone would want to see a Guild :) */

  sprintf(buf, "%s The GM can teach the following:\r\n", buf);

  *buf2 = '\0';
  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (does_guild_know(gm_nr, i))
      sprintf(buf2, "%s %s \r\n", buf2, spell_info[i].name);
  }
 
  strcat(buf, buf2);

  page_string(ch->desc, buf, 1);
}
  

void show_guild(struct char_data * ch, char *arg)
{
  int gm_nr, gm_num;

  if (!*arg)
    list_all_guilds(ch);
  else {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

    if (gm_num > 0) {
      for (gm_nr = 0; gm_nr <= top_guild; gm_nr++) {
      if (gm_num == GM_NUM(gm_nr))
        break; 
      }

      if (gm_num < 0 || gm_nr > top_guild) {
        send_to_char(ch, "Illegal guild master number.\n\r");
        return;
      }
      list_detailed_guild(ch, gm_nr);
    }
  }
}

/*
 * List all guilds in a zone.                              
 */                                                                           
void list_guilds(struct char_data *ch, zone_rnum rnum, guild_vnum vmin, guild_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  /****************************************************************************/
  /** Store the header for the guild listing.                                **/
  /****************************************************************************/
  send_to_char (ch,
  "Index VNum    Guild Master\r\n"
  "----- ------- ---------------------------------------------\r\n");
  
  for (i = 0; i <= top_guild; i++) {
    if (GM_NUM(i) >= bottom && GM_NUM(i) <= top) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@c%-5d@n]", counter, GM_NUM(i));

      /************************************************************************/
      /** Retrieve the list of rooms for this guild.                         **/
      /************************************************************************/

		send_to_char(ch, " @c[@y%d@c]@y %s@n", 
			(GM_TRAINER(i) == -1) ?
			  -1 : mob_index[GM_TRAINER(i)].vnum,
			(GM_TRAINER(i) == -1) ?
			  "" : mob_proto[GM_TRAINER(i)].short_descr); 
      
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

void destroy_guilds(void)
{
  ssize_t cnt/*, itr*/;

  if (!guild_index)
    return;

  for (cnt = 0; cnt <= top_guild; cnt++) {
    if (guild_index[cnt].no_such_skill)
      free(guild_index[cnt].no_such_skill);
    if (guild_index[cnt].not_enough_gold)
      free(guild_index[cnt].not_enough_gold);

    /*if (guild_index[cnt].type) {
      for (itr = 0; BUY_TYPE(guild_index[cnt].type[itr]) != NOTHING; itr++)
        if (BUY_WORD(guild_index[cnt].type[itr]))
          free(BUY_WORD(guild_index[cnt].type[itr]));
      free(shop_index[cnt].type);
    } */
  }

  free(guild_index);
  guild_index = NULL;
  top_guild = -1;
}


int count_guilds(guild_vnum low, guild_vnum high)
{
  int i, j;
  
  for (i = j = 0; GM_NUM(i) <= high; i++)
    if (GM_NUM(i) >= low)
      j++;
 
  return j;
}


void levelup_parse(struct descriptor_data *d, char *arg)
{
}
