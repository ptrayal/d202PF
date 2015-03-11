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

SVNHEADER("$Id: guild.c 55 2009-03-20 17:58:56Z pladow $");

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
extern int race_level_adjustment[];
extern struct spell_info_type spell_info[];
extern struct time_info_data time_info;
extern const char *trade_letters[];
extern int cmd_say, cmd_tell;
extern const int *class_bonus_feats[];
extern char *race_names[NUM_RACES];

/* extern function prototypes */
ACMD(do_tell);
ACMD(do_say);
int art_level_exp(int level);
int level_exp(int level, int race);
void gain_level(struct char_data *ch, int whichclass);
void advance_level(struct char_data *ch, int whichclass);
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg);
int find_feat_num(char *name);
int get_skill_mod(struct char_data *ch, int skillnum);
int get_skill_value(struct char_data *ch, int skillnum);


// Local functions
void show_skills(struct char_data *ch);
int compare_skills(const void *x, const void *y);
int do_handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument, int manual);

/* Local variables */
struct guild_data *guild_index;
int spell_sort_info[SKILL_TABLE_SIZE + 1];
int top_guild = -1;
char *guild_customer_string(int guild_nr, int detailed);
void sort_skills(void);
int sorted_skill_list[SKILL_HIGH_SKILL - SKILL_LOW_SKILL + 1];
int sorted_language_list[SKILL_LANG_HIGH - SKILL_LANG_LOW + 1];

ACMD(do_show_sorted_lists) {


  show_skills(ch);
  return;
}

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

int compare_skills(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a+SKILL_LOW_SKILL].name, spell_info[b+SKILL_LOW_SKILL].name);
}



void show_skills(struct char_data *ch)
{

  int i;
  int count = 0;

    send_to_char(ch, "\r\n@WSkill Points: @Y%d@n\r\n\r\n", GET_PRACTICES(ch, GET_CLASS(ch)));
    send_to_char(ch, "\r\n@WClass Skills:@n\r\n\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if ((spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <= 
          SKILL_HIGH_SKILL) && (spell_info[spell_sort_info[i]].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CLASS) &&
          !IS_SET(spell_info[spell_sort_info[i]].flags, SKFLAG_CRAFT)) {
        send_to_char(ch, "%-30s: %2d [%2d] ", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]),
                     get_skill_value(ch, spell_sort_info[i]));
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");   
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@WCross-Class Skills:@n\r\n\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if ((spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <= 
          SKILL_HIGH_SKILL) && (spell_info[spell_sort_info[i]].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CROSSCLASS) &&
          !IS_SET(spell_info[spell_sort_info[i]].flags, SKFLAG_CRAFT)) {
        send_to_char(ch, "%-30s: %2d [%2d] ", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]),
                     get_skill_value(ch, spell_sort_info[i]));
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");   
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@WLanguages:@n (@Wbold white@n if known)\r\n\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if ((spell_info[spell_sort_info[i]].skilltype == (SKTYPE_SKILL + SKTYPE_LANG) && spell_sort_info[i] >= SKILL_LANG_LOW && spell_sort_info[i] <=
          SKILL_LANG_HIGH)) {
        send_to_char(ch, "%s%-38s%s ", GET_SKILL(ch, spell_sort_info[i]) ? "@W" : "", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, 
                     spell_sort_info[i]) ? "@n" : "");
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

  send_to_char(ch, "Format is <skill name> <base skill value> [<modified skill value>]\r\n\r\n");

}

void display_levelup_skills(struct char_data *ch, int langs)
{

  int i;
  int count = 0;

    send_to_char(ch, "\r\n@WSkill Points: @Y%d@n\r\n\r\n", ch->levelup->practices);
    send_to_char(ch, "\r\n@WSkills:@n\r\n\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if ((spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <=
          SKILL_HIGH_SKILL) && (spell_info[spell_sort_info[i]].can_learn_skill[ch->levelup->class] == SKLEARN_CLASS || spell_info[spell_sort_info[i]].can_learn_skill[ch->levelup->class] == SKLEARN_CROSSCLASS) &&
          !IS_SET(spell_info[spell_sort_info[i]].flags, SKFLAG_CRAFT)) {
        send_to_char(ch, "%s%2d) %-20s: %2d [%2d] @n", spell_info[spell_sort_info[i]].can_learn_skill[ch->levelup->class] == SKLEARN_CLASS ? "@y" : "@r", 
spell_sort_info[i], spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]),
        		GET_SKILL(ch, spell_sort_info[i]) + ch->levelup->skills[spell_sort_info[i]]);
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    if (langs) {

    send_to_char(ch, "@WLanguages:@n\r\n\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if ((spell_info[spell_sort_info[i]].skilltype == (SKTYPE_SKILL + SKTYPE_LANG) && spell_sort_info[i] >= SKILL_LANG_LOW && spell_sort_info[i] <=
          SKILL_LANG_HIGH)) {
        send_to_char(ch, "%s%2d) %-20s%s @n",  (GET_SKILL(ch, spell_sort_info[i]) + ch->levelup->skills[spell_sort_info[i]]) ? "@y" : "@r", spell_sort_info[i], 
spell_info[spell_sort_info[i]].name, GET_SKILL(ch,
                     spell_sort_info[i]) ? "@n" : "");
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    }

  send_to_char(ch, "Format is <skill name> <base skill ranks> [<with levelup skill ranks>].\r\nClass Skill in @yyellow@n, Cross-Class in @rred@n.");
  if (langs)
	  send_to_char(ch, "Language known in @yyellow@n, unknown in @rred@n.");
  send_to_char(ch, "\r\nWhat skill would you like to raise? (-1 to end)");

}

ACMD(do_artisan) {

  char arg[200];

  one_argument(argument, arg);

  if (GET_ARTISAN_TYPE(ch) == 0) {
    if (!*arg) {
      send_to_char(ch, "You must select an artisan type from the following list:\r\n"
                       "smith (blacksmithing, goldsmithing & mining)\r\n"
                       "farmer (tailoring, cooking & farming)\r\n"
                       "woodsman (tanning, woodworking & foresting)\r\n"
                       "\r\n"
                       "Type 'artisan <artisan type>' to select your crafting field.  Once chosen this cannot be changed.\r\n");
      return;
    }
    else {
      if (!strcmp(arg, "smith")) {
        send_to_char(ch, "You have chosen the artisan profession of 'smith' giving you access to the blacksmithing, goldsmithing and mining skills.\r\n");
        GET_ARTISAN_TYPE(ch) = ARTISAN_TYPE_SMITH;
        return;
      }
      else if (!strcmp(arg, "farmer")) {
        send_to_char(ch, "You have chosen the artisan profession of 'farmer' giving you access to the tailoring, cooking and farming skills.\r\n");
        GET_ARTISAN_TYPE(ch) = ARTISAN_TYPE_FARMER;
        return;
      }
      else if (!strcmp(arg, "woodsman")) {
        send_to_char(ch, "You have chosen the artisan profession of 'woodsman' giving you access to the tanning, woodworking and foresting skills.\r\n");
        GET_ARTISAN_TYPE(ch) = ARTISAN_TYPE_WOODSMAN;
        return;
      }
      else {
        send_to_char(ch, "You must select an artisan type from the following list:\r\n"
                         "smith (blacksmithing, goldsmithing & mining)\r\n"
                         "farmer (tailoring, cooking & farming)\r\n"
                         "woodsman (tanning, woodworking & foresting)\r\n"
                         "\r\n"
                         "Type 'artisan <artisan type>' to select your crafting field.  Once chosen this cannot be changed.\r\n");
        return;
      }
    }
  }

  argument = strdup("nothing");

  int i = 0;

  send_to_char(ch, "@WCrafting Skills:@n\r\n\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
    if ((spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <= 
        SKILL_HIGH_SKILL) && IS_SET(spell_info[spell_sort_info[i]].flags, SKFLAG_CRAFT)) {
      if (spell_info[spell_sort_info[i]].artisan_type != GET_ARTISAN_TYPE(ch) && spell_info[spell_sort_info[i]].artisan_type != ARTISAN_TYPE_ALL)
        continue;
      send_to_char(ch, "%-30s: %2d [%2d] %d Artisan Exp to Increase", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]),
                   get_skill_value(ch, spell_sort_info[i]), art_level_exp(GET_SKILL(ch, spell_sort_info[i]) + 1));
    }
    else
      continue;
    send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "\r\n");

  send_to_char(ch, "Artisan Class Level: %-2d\r\n\r\n%10d Artisan Experience Required for Next Artisan Level.\r\n\r\n", GET_CLASS_RANKS(ch, CLASS_ARTISAN), 
               art_level_exp(GET_CLASS_RANKS(ch, CLASS_ARTISAN) + 1));

  send_to_char(ch, "Current Artisan Experience = %12.0f.\r\n\r\n", GET_ARTISAN_EXP(ch));

  send_to_char(ch, "Total Artisan Experience = %12.0f.\r\n\r\n", get_artisan_exp(ch));

  if (argument)
    free(argument);
  
}

int print_skills_by_type(struct char_data *ch, char *buf, int maxsz, int sktype)
{
  size_t len = 0;
  int i, t, known, nlen;
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
	nlen = snprintf(buf + len, maxsz - len, "%-30s  (%s)\r\n",
                        spell_info[i].name, GET_SKILL_BASE(ch, i) ? "known" : "unknown");
      } else if (t & SKTYPE_SKILL) {
        
        snprintf(buf2, sizeof(buf2), " (base %d + bonus %d)", GET_SKILL_BASE(ch, i),
                   (GET_SKILL_BONUS(ch, i) + get_skill_mod(ch, i)));
        if (spell_info[i].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CROSSCLASS) {
	      nlen = snprintf(buf + len, maxsz - len, "@W%-20s  %d%s@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i) + GET_SKILL_BONUS(ch, i) + get_skill_mod(ch, i), buf2);
        }
        else
   	      nlen = snprintf(buf + len, maxsz - len, "@G%-20s  %d%s@n\r\n",
                        spell_info[i].name, GET_SKILL(ch, i) + GET_SKILL_BONUS(ch, i) + get_skill_mod(ch, i), buf2);
        
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

  show_skills(ch);

  return;

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
#if defined(CAMPAIGN_FORGOTTEN_REALMS)
		 (IS_HALFLING(ch) && NOTRAIN_HALFLING(guild_nr)) ||
#endif
#if defined(CAMPAIGN_DRAGONLANCE)
		 (IS_KENDER(ch) && NOTRAIN_HALFLING(guild_nr)) ||
#endif
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
		 (IS_LIZARDFOLK(ch) && NOTRAIN_LIZARDFOLK(guild_nr))
		  ) {
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


void sort_skills(void)
{
  return;

  int i=0, j=0, k=0, temp=0;
  char wordA[100], wordB[100];

  for (i = SKILL_LOW_SKILL; i <= SKILL_HIGH_SKILL; i++)
    sorted_skill_list[i-SKILL_LOW_SKILL] = i;

  qsort(&sorted_skill_list[1], SKILL_HIGH_SKILL - SKILL_LOW_SKILL, sizeof(int), compare_skills);

  return;

  for (i = SKILL_LOW_SKILL; i < SKILL_HIGH_SKILL; i++) {
    for (j = SKILL_LANG_LOW; j < SKILL_LANG_HIGH; j++) {
      sprintf(wordA, "%s", spell_info[j].name);
      sprintf(wordB, "%s", spell_info[j+1].name);
      for (k=0; k < strlen(wordA); k++)
        wordA[k] = tolower(wordA[k]);
      for (k=0; k < strlen(wordB); k++)
        wordB[k] = tolower(wordB[k]);
      if (strcmp(wordA, wordB) > 0) {
        temp = sorted_skill_list[j-SKILL_LOW_SKILL];
        sorted_skill_list[j-SKILL_LOW_SKILL] = sorted_skill_list[j-SKILL_LOW_SKILL+1];
        sorted_skill_list[j-SKILL_LOW_SKILL+1] = temp;
      }
    }
  }

}

void sort_languages(void)
{

  int i=0, j=0, k=0, temp=0;
  char wordA[100], wordB[100];

  for (i = SKILL_LANG_LOW; i <= SKILL_LANG_HIGH; i++)
    sorted_skill_list[i-SKILL_LANG_LOW] = i;

  for (i = SKILL_LANG_LOW; i <= SKILL_LANG_HIGH; i++) {
    for (j = SKILL_LANG_LOW; j <= SKILL_LANG_HIGH; j++) {
      sprintf(wordA, "%s", spell_info[i].name);
      sprintf(wordB, "%s", spell_info[j].name);
      for (k=0; k < strlen(wordA); k++)
        wordA[k] = tolower(wordA[k]);
      for (k=0; k < strlen(wordB); k++)
        wordB[k] = tolower(wordB[k]);
      if (strcmp(wordA, wordB) > 0) {
        temp = i;
        sorted_skill_list[i-SKILL_LANG_LOW] = j;
        sorted_skill_list[j-SKILL_LANG_LOW] = temp;
      }
    }
  }

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

  int i;
  int count = 0;

    send_to_char(ch, "\r\n@WSkill Points: @Y%d@n\r\n\r\n", GET_PRACTICES(ch, GET_CLASS(ch)));

    send_to_char(ch, "\r\n@WClass Skills:@n\r\n\r\n");

    for (i = 0, count = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if (does_guild_know(guild_nr, spell_sort_info[i]) && 
          (spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <= 
          SKILL_HIGH_SKILL) && (spell_info[spell_sort_info[i]].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CLASS)) {
        send_to_char(ch, "%-30s: %2d [%2d] ", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]),
                     get_skill_value(ch, spell_sort_info[i]));
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");   
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@WCross-Class Skills:@n\r\n\r\n");

    for (i = 0, count = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if (does_guild_know(guild_nr, spell_sort_info[i]) && 
          (spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <= 
          SKILL_HIGH_SKILL) && (spell_info[spell_sort_info[i]].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CROSSCLASS)) {
        send_to_char(ch, "%-30s: %2d   ", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, spell_sort_info[i]));
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");   
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@WLanguages:@n (@Wbold white@n if known)\r\n\r\n");

    for (i = 0, count = 0; i < SKILL_TABLE_SIZE + 1; i++) {
      if (does_guild_know(guild_nr, spell_sort_info[i]) && 
          (spell_info[spell_sort_info[i]].skilltype == (SKTYPE_SKILL + SKTYPE_LANG) && spell_sort_info[i] >= SKILL_LANG_LOW && spell_sort_info[i] <=
          SKILL_LANG_HIGH)) {
        send_to_char(ch, "%s%-38s%s ", GET_SKILL(ch, spell_sort_info[i]) ? "@W" : "", spell_info[spell_sort_info[i]].name, GET_SKILL(ch, 
                     spell_sort_info[i]) ? "@n" : "");
      }
      else
        continue;
      if (count % 2 == 1 )
        send_to_char(ch, "\r\n");
      count++;
    }

    if (count % 2 == 1 )
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

  send_to_char(ch, "Format is <skill name> <base skill value> [<modified skill value>]\r\n\r\n");

  return;

/*  Old code

  const char *overflow = "\r\n**OVERFLOW**\r\n";
  char buf2[MAX_STRING_LENGTH];
  int i, sortpos, canknow, j;
  size_t nlen = 0, len = 0;

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch, GET_CLASS(ch)), GET_PRACTICES(ch, GET_CLASS(ch)) == 1 ? "" : "s"); // ???

  nlen = snprintf(buf2 + len, sizeof(buf2) - len, "I can teach you the following skills:\r\n");
  len += nlen;
  
  // Need to check if trainer can train doesnt do it now ??? 
  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; // spell_sort_info[sortpos]; 
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
    i = sortpos; // spell_sort_info[sortpos]; 
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
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); // strcpy: OK 

  page_string(ch->desc, buf2, TRUE);
*/
}


void handle_practice(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int skill_num, learntype, pointcost, highest, i;
  long exp_reimb = 0;

  skip_spaces(&argument);

  if (!*argument) {
    show_skills(ch);
    return;
  }

  skill_num = find_skill_num(argument, SKTYPE_SKILL);

  if (!IS_SET(spell_info[skill_num].flags, SKFLAG_CRAFT)) {
    if (GET_PRACTICES(ch, GET_CLASS(ch)) <= 0) {
      send_to_char(ch, "You do not seem to be able to practice now.\r\n");
      return;
    }
  }
  else {
    if (spell_info[skill_num].artisan_type != GET_ARTISAN_TYPE(ch) && spell_info[skill_num].artisan_type != ARTISAN_TYPE_ALL) {
      send_to_char(ch, "You cannot practice that skill as it is outside of your area of expertise.\r\n");
      if (GET_SKILL_BASE(ch, skill_num) > 0) {
        while (GET_SKILL_BASE(ch, skill_num) > 2) {
          GET_ARTISAN_EXP(ch) += art_level_exp(GET_SKILL(ch, skill_num));
          exp_reimb += art_level_exp(GET_SKILL(ch, skill_num));
          GET_SKILL_BASE(ch, skill_num)--;
        }
        if (exp_reimb > 0)
          send_to_char(ch, "You have been reimbursed %ld artisan experience.\r\n", exp_reimb);
      }
      return;
    }

    if (GET_SKILL(ch, skill_num) - 3 > GET_CLASS_RANKS(ch, CLASS_ARTISAN)) {
      send_to_char(ch, "You must raise your artisan level before you may increase that skill.\r\n");
      return;
    }  
    if (GET_ARTISAN_EXP(ch) < (art_level_exp(GET_SKILL(ch, skill_num) + 1))) {
      send_to_char(ch, "You do not have enough artisan experience to raise this skill another rank.\r\n");
      return;
    }
    send_to_char(ch, "You practice for a while...\r\n");
    GET_ARTISAN_EXP(ch) = GET_ARTISAN_EXP(ch) - (art_level_exp(GET_SKILL(ch, skill_num) + 1));
    SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + 1);
    return;
  }


  /****  Does the GM know the skill the player wants to learn?  ****/

  if (skill_num == SKILL_LANG_THIEVES_CANT && !IS_ROGUE(ch)) {
    send_to_char(ch, "You need at least 1 rank in the rogue class to learn thieves cant.\r\n");    
    return;
 }

  /**** Can the player learn the skill if the GM knows it?  ****/ 
  if (IS_SET(spell_info[skill_num].skilltype, SKTYPE_SKILL)) {
    for (learntype = 0, i = 0; i < NUM_CLASSES; i++)
//      if (spell_info[skill_num].can_learn_skill[i] > learntype)
      if (GET_CLASS(ch) == i)
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
    if (spell_info[skill_num].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CLASS ||
        HAS_FEAT(ch, FEAT_ABLE_LEARNER))
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
  int whichclass;
  int level = GET_LEVEL(ch);
  int i = 0, class_ok;

  if (GET_CLASS_LEVEL(ch) == 0)
    level = 0;

  skip_spaces(&argument);
  if (CONFIG_ALLOW_MULTICLASS) {
    if (!*argument) {
      send_to_char(ch, "You must specify a class you want to train.  Type gain classes for a list\r\n");
      return;
    }
    if (is_abbrev(argument, "classes")) {
      send_to_char(ch, "%-25s %-10s\r\n------------------------- ----------\r\n", "Class Name", "Prestige?");
      for (i = 0; i < NUM_CLASSES; i++) {
        if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS) {
          if (class_in_game_fr[i])
            send_to_char(ch, "%-25s %-10s\r\n", class_names_fr[i], prestige_classes_fr[i] ? "Yes" : "No");
        }
        else  {
          if (class_in_game_dl_aol[i])
            send_to_char(ch, "%-25s %-10s\r\n", class_names_dl_aol[i], prestige_classes_dl_aol[i] ? "Yes" : "No");
        }
      }
      return;
    }
    if ((whichclass = search_block(argument, (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? class_names_fr : class_names_dl_aol), FALSE)) < 0) {
      send_to_char(ch, "That is not a class.\r\n");
      return;
    }
  } else {
     whichclass = GET_CLASS(ch);
  }
  if (whichclass == CLASS_ARTISAN) {
    if (GET_CLASS_RANKS(ch, CLASS_ARTISAN) > 0 && GET_ARTISAN_EXP(ch) < art_level_exp(GET_CLASS_RANKS(ch, CLASS_ARTISAN) + 1)) {
      send_to_char(ch, "You do not have enough artisan experience to gain another level in artisan.\r\n");
      return;
    }
    if (GET_CLASS_RANKS(ch, CLASS_ARTISAN) > 0)
      GET_ARTISAN_EXP(ch) -= art_level_exp(GET_CLASS_RANKS(ch, CLASS_ARTISAN) + 1);
    advance_level(ch, CLASS_ARTISAN);
    send_to_char(ch, "You have raised a level in the artisan class, making your new artisan level %d.\r\n", GET_CLASS_RANKS(ch, CLASS_ARTISAN));
    return;
  }
  else {
    send_to_char(ch, "You must use the levelup comnmand to gain levels now, except with the artisan class.\r\n");
    return;
  }
	class_ok = class_ok_general(ch, whichclass);
  if (!class_ok) {
    send_to_char(ch, "You cannot progress in that class.\r\n");
    return;
  }
  else if (class_ok == -1) {
    send_to_char(ch, "Your race is not eligible for that class.\r\n");
    return;
  }
  else if (class_ok == -2) {
    send_to_char(ch, "Your alignment is not eligible for that class.\r\n");
    return;
  }
  else if (class_ok == -3) {
    send_to_char(ch, "You have reached the maximum allowable level for that class.\r\n");
    return;
  }
  else if (class_ok == -4) {
    send_to_char(ch, "That class is not available to be chosen as it is not yet fully implemented.\r\n");
    return;
  }
  else if (class_ok == -5) {
    send_to_char(ch, "You are only allowed a maximum of 3 classes total.\r\n");
    return;
  }
  else if (GET_PRACTICES(ch, GET_CLASS(ch)) > 0 && GET_ADMLEVEL(ch) < 1 && whichclass != GET_CLASS(ch)) {
    send_to_char(ch, "You have to spend your skill points before you can gain another level.\r\n");
    return;
  }
  if (level == 0 || (level < CONFIG_LEVEL_CAP && GET_EXP(ch) >= level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch)))) {
	gain_level(ch, whichclass);
        GET_LEVEL_STAGE(ch) = 0;
  } else {
    send_to_char(ch, "You are not yet ready for further advancement.\r\n");
  }
  return;
}

#define FEAT_TYPE_NORMAL		1
#define FEAT_TYPE_NORMAL_CLASS		2
#define FEAT_TYPE_EPIC			3
#define FEAT_TYPE_EPIC_CLASS		4

void handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument) {
	int x = 0;
	x = do_handle_learn(keeper, guild_nr, ch, argument, TRUE);
}

int do_handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument, int manual)
{
  struct damreduct_type *dptr, *reduct, *temp;
  int feat_num, subval, sftype, subfeat, q=0;
  char *ptr, buf[MAX_STRING_LENGTH];
  int feat_type = FEAT_TYPE_NORMAL;
  int feat_points, epic_feat_points, class_feat_points, epic_class_feat_points;

  if (manual) {
	  feat_points = GET_FEAT_POINTS(ch);
	  epic_feat_points = GET_EPIC_FEAT_POINTS(ch);
	  class_feat_points = GET_CLASS_FEATS(ch, GET_CLASS(ch));
	  epic_class_feat_points = GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch));
  }
  else {
	  feat_points = ch->levelup->feat_points;
	  epic_feat_points = ch->levelup->epic_feat_points;
	  class_feat_points = ch->levelup->num_class_feats;
	  epic_class_feat_points = ch->levelup->num_epic_class_feats;
  }


  if (argument == NULL || !*argument) {
    
//    send_to_char(ch, "Which feat would you like to learn?\r\n");
    return 0;
  }

  if (is_abbrev(argument, "favored enemy")) {
    if (manual) send_to_char(ch, "Please use the favoredenemy command to choose your favored enemies.\r\n");
    return 0;
  }

  ptr = strchr(argument, ':');
  if (ptr)
    *ptr = 0;
  feat_num = find_feat_num(argument);
  if (ptr)
    *ptr = ':';


  if (HAS_FEAT(ch, feat_num) && !feat_list[feat_num].can_stack) {
    if (manual) send_to_char(ch, "You already know the %s feat.\r\n", feat_list[feat_num].name);
    return 0;
  }

  if (!feat_is_available(ch, feat_num, 0, NULL) || !feat_list[feat_num].in_game || !feat_list[feat_num].can_learn) {
    if (manual) send_to_char(ch, "The %s feat is not available to you at this time.\r\n", argument);
    return 0;
  }

  // determine which type of feat it is for purposes of checking whether they have enough feats or class feats

  if (feat_list[feat_num].epic == TRUE) {
    if (is_class_feat(feat_num, GET_CLASS(ch))) 
      feat_type = FEAT_TYPE_EPIC_CLASS;
    else
      feat_type = FEAT_TYPE_EPIC;
  }
  else {
    if (is_class_feat(feat_num, GET_CLASS(ch))) 
      feat_type = FEAT_TYPE_NORMAL_CLASS;
    else
      feat_type = FEAT_TYPE_NORMAL;
  }

  // if it's an epic feat, make sure they have an epic feat point

  if (feat_type == FEAT_TYPE_EPIC && epic_feat_points < 1) {
    if (manual) send_to_char(ch, "This is an epic feat and you do not have any epic feat points remaining.\r\n");
    return 0;
  }

  // if it's an epic class feat, make sure they have an epic feat point or an epic class feat point

  if (feat_type == FEAT_TYPE_EPIC_CLASS && epic_feat_points < 1 && epic_class_feat_points < 1) {
    if (manual) send_to_char(ch, "This is an epic class feat and you do not have any epic feat points or epic class feat points remaining.\r\n");
    return 0;
  }

  // if it's a normal feat, make sure they have a normal feat point

  if (feat_type == FEAT_TYPE_NORMAL && feat_points < 1) {
    if (manual) send_to_char(ch, "This is a normal feat and you do not have any normal feat points remaining.\r\n");
    return 0;
  }

  // if it's a normal class feat, make sure they have a normal feat point or a normal class feat point

  if (feat_type == FEAT_TYPE_NORMAL_CLASS && feat_points < 1 && class_feat_points < 1) {
    if (manual) send_to_char(ch, "This is a normal class feat and you do not have any normal feat points or normal class feat points remaining.\r\n");
    return 0;
  }
  
  sftype = 2;
  switch (feat_num) {
  case FEAT_FAVORED_ENEMY:
    if (manual) send_to_char(ch, "Please use the favoredenemy command to choose your favored enemies.\r\n");
    return 0;
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
  case FEAT_GREATER_WEAPON_FOCUS:
  case FEAT_WEAPON_SPECIALIZATION:
  case FEAT_WEAPON_FOCUS:
  case FEAT_IMPROVED_CRITICAL:
  case FEAT_POWER_CRITICAL:
  case FEAT_SKILL_FOCUS:
  case FEAT_EPIC_SKILL_FOCUS:
  case FEAT_IMPROVED_WEAPON_FINESSE:
  case FEAT_MONKEY_GRIP:
  case FEAT_EXOTIC_WEAPON_PROFICIENCY:
  case FEAT_WEAPON_MASTERY:
  case FEAT_WEAPON_FLURRY:
  case FEAT_WEAPON_SUPREMACY:

    if (feat_num != FEAT_SKILL_FOCUS && feat_num != FEAT_EPIC_SKILL_FOCUS)
      sftype = 1;
    else
      sftype = 3;

  case FEAT_SPELL_FOCUS:
  case FEAT_GREATER_SPELL_FOCUS:
    subfeat = feat_to_subfeat(feat_num);
    if (subfeat == -1) {
      log("guild: Unconfigured subfeat '%s', check feat_to_subfeat()", feat_list[feat_num].name);
      if (manual) send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return 0;
    }
    if (!ptr || !*ptr) {
      if (manual) send_to_char(ch, "No ':' found. You must specify a something to improve. Example:\r\nlearn weapon focus: long sword\r\n");
      return 0;
    }
    if (*ptr == ':') ptr++;
    skip_spaces(&ptr);
    if (!ptr || !*ptr) {
      if (manual) send_to_char(ch, "No ':' found. You must specify a something to improve. Example:\r\nlearn weapon focus: long sword\r\n");
      return 0;
    }
    if (sftype == 1) {
      for (subval = 1; subval < MAX_WEAPON_TYPES + 1; subval++)
        if (is_abbrev(ptr, weapon_type[subval]))
          break;
      if (subval > MAX_WEAPON_TYPES + 1) {
        if (manual) send_to_char(ch, "That is not a weapon type.\r\n");
        return 0;
      }
    }
    else if (sftype == 3) {
      for (subval = SKILL_LOW_SKILL; subval <= SKILL_HIGH_SKILL; subval++) {
        if (is_abbrev(ptr, spell_info[subval].name) && strcmp(spell_info[subval].name, "!UNUSED!")) {
          if (manual) send_to_char(ch, "Attempting to learnskill focus for %s.\r\n", spell_info[subval].name);
          break;
        }
        if (subval > SKILL_HIGH_SKILL) {
          if (manual) send_to_char(ch, "That is not a valid skill.\r\n");
          return 0;
        }
      }
    }
    else  {
      subval = search_block(ptr, spell_schools, FALSE);
    }

    if (subval == -1) {
      log("bad subval: %s", ptr);
      if (sftype == 2)
        ptr = "spell school";
      else if (sftype == 3)
        ptr = "skill";
      else
        ptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "That is not a known %s. Available %s:\r\n",
                         ptr, ptr);
      for (subval = (sftype == 3 ?  RACE_HUMAN : 1); subval <= (sftype == 2 ? NUM_SCHOOLS : (sftype == 3 ? NUM_RACES : MAX_WEAPON_TYPES + 1)); subval++) {
        if (sftype == 2)
          ptr = (char *)spell_schools[subval];
        else if (sftype == 3)
          ptr = (char *)spell_info[subval].name;
        else
          ptr = (char *)weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", ptr);
      }
      page_string(ch->desc, buf, TRUE);
      return 0;
    }
    if (!feat_is_available(ch, feat_num, subval, NULL)) {
      if (manual) send_to_char(ch, "You do not satisfy the prerequisites for that feat.\r\n");
      return 0;
    }
    if (sftype == 1) {
      if (HAS_COMBAT_FEAT(ch, subfeat, subval)) {
        if (manual) send_to_char(ch, "You already have that weapon feat.\r\n");
        return 0;
      }
      SET_COMBAT_FEAT(ch, subfeat, subval);
    } else if (sftype == 2) {
      if (HAS_SCHOOL_FEAT(ch, subfeat, subval)) {
        if (manual) send_to_char(ch, "You already have that spell school feat.\r\n");
        return 0;
      }
      SET_SCHOOL_FEAT(ch, subfeat, subval);
    } 
    else if (sftype == 3) {
      if (ch->player_specials->skill_focus[subval-SKILL_LOW_SKILL] > 0 && feat_num == FEAT_SKILL_FOCUS) {
        if (manual) send_to_char(ch, "You already have that skill focus feat.\r\n");
        return 0;
      }
      if (ch->player_specials->skill_focus[subval-SKILL_LOW_SKILL] > 1 && feat_num == FEAT_EPIC_SKILL_FOCUS) {
        if (manual) send_to_char(ch, "You already have that epic skill focus feat.\r\n");
        return 0;
      }
      ch->player_specials->skill_focus[subval-SKILL_LOW_SKILL] += 1;
    } else {
      log("unknown feat subtype %d in subfeat code", sftype);
      if (manual) send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return 0;
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
    GET_MAX_HIT(ch) += GET_LEVEL(ch);
    break;
  case FEAT_ENDURANCE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_MAX_MOVE(ch) += GET_LEVEL(ch) * 10;
    break;
  case FEAT_EXTRA_RAGE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_SPELL_MASTERY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_SPELL_MASTERY_POINTS(ch) += MAX(1, ability_mod_value(GET_INT(ch)));
    break;
  case FEAT_ACROBATIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_ATHLETICS, GET_SKILL_BONUS(ch, SKILL_ATHLETICS) + 2);
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
    SET_SKILL_BONUS(ch, SKILL_ATHLETICS, GET_SKILL_BONUS(ch, SKILL_ATHLETICS) + 2);
    SET_SKILL_BONUS(ch, SKILL_ATHLETICS, GET_SKILL_BONUS(ch, SKILL_ATHLETICS) + 2);
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
  case FEAT_DAMAGE_REDUCTION:
    if (ch->damage_reduction_feats == 5) {
      if (manual) send_to_char(ch, "You can only take the damage reduction feat 5 times.\r\n");
      return 0;
    }
    ch->damage_reduction_feats++;
    subval = HAS_FEAT(ch, feat_num) + 3;
    SET_FEAT(ch, feat_num, subval);
    for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
      if (reduct->feat == FEAT_DAMAGE_REDUCTION) {
        REMOVE_FROM_LIST(reduct, ch->damreduct, next);
      }
    }
    CREATE(dptr, struct damreduct_type, 1);
    dptr->next = ch->damreduct;
    ch->damreduct = dptr;
    dptr->spell = 0;
    dptr->feat = FEAT_DAMAGE_REDUCTION;
    dptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION);
    dptr->duration = -1;
    dptr->max_damage = -1;
    for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
      dptr->damstyle[q] = dptr->damstyleval[q] = 0;
    dptr->damstyle[0] = DR_NONE;
    break;
  case FEAT_FAST_HEALING:
    if (ch->fast_healing_feats == 5) {
      if (manual) send_to_char(ch, "You can only take the fast healing feat 5 times.\r\n");
      return 0;
    }
    ch->fast_healing_feats++;
    break;
  case FEAT_ARMOR_SKIN:
    if (ch->armor_skin_feats == 5) {
      if (manual) send_to_char(ch, "You can only take the armor skin feat 5 times.\r\n");
      return 0;
    }
    ch->armor_skin_feats++;
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_GREAT_STRENGTH:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.str++;
    break;
  case FEAT_GREAT_DEXTERITY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.dex++;
    break;
  case FEAT_GREAT_CONSTITUTION:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.con++;
    break;
  case FEAT_GREAT_INTELLIGENCE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.intel++;
    break;
  case FEAT_GREAT_WISDOM:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.wis++;
    break;
  case FEAT_GREAT_CHARISMA:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    ch->real_abils.cha++;
    break;
  default:
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  }
  save_char(ch);

  // reduce the appropriate feat point type based on what the feat type was set as above.  This simulatyes spendinng the feat

  if (feat_type == FEAT_TYPE_EPIC) {
    epic_feat_points--;
  }
  else if (feat_type == FEAT_TYPE_EPIC_CLASS) {
    if (epic_class_feat_points > 0)
      epic_class_feat_points--;
    else
      epic_feat_points--;
  }
  else if (feat_type == FEAT_TYPE_NORMAL) {
    feat_points--;
  }
  else if (feat_type == FEAT_TYPE_NORMAL_CLASS) {
    if (class_feat_points > 0)
      class_feat_points--;
    else
      feat_points--;
  }

  if (manual) {
	  GET_FEAT_POINTS(ch) = feat_points;
	  GET_EPIC_FEAT_POINTS(ch) = epic_feat_points;
	  GET_CLASS_FEATS(ch, GET_CLASS(ch)) = class_feat_points;
	  GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch)) = epic_class_feat_points;
  }
  else {
	  ch->levelup->feat_points = feat_points;
	  ch->levelup->epic_feat_points = epic_feat_points;
	  ch->levelup->num_class_feats = class_feat_points;
	  ch->levelup->num_epic_class_feats = epic_class_feat_points;
  }

  if (manual) send_to_char(ch, "Your training has given you the %s feat!\r\n", feat_list[feat_num].name);

  return 1;
}


ACMD(do_gain)
{

  handle_gain(NULL, 0, ch, argument);

}

ACMD(do_practice_skills)
{

  handle_practice(NULL, 0, ch, argument);

}

SPECIAL(guild)
{
//  char arg[MAX_INPUT_LENGTH];
  int i;
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

  for (i = 0; guild_cmd_tab[i].cmd; i++)
    if (CMD_IS(guild_cmd_tab[i].cmd))
      break;

  if (!guild_cmd_tab[i].cmd)
    return (FALSE);

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
