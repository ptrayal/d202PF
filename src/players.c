/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "mysql/mysql.h"

#include "conf.h"
#include "sysdep.h"

#include "screen.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "pfdefaults.h"
#include "feats.h"
#include "dg_scripts.h"
#include "comm.h"
#include "genmob.h"
#include "constants.h"
#include "spells.h"
#include "interpreter.h"
#include "quest.h"
#include "deities.h"

#define LOAD_HIT	0
#define LOAD_MANA	1
#define LOAD_MOVE	2
#define LOAD_KI		3


/* local functions */
void build_player_index(void);
void save_etext(struct char_data *ch);
int sprintascii(char *out, bitvector_t bits);
void tag_argument(char *argument, char *tag);
void load_affects(FILE *fl, struct char_data *ch, int violence);
void load_skills(FILE *fl, struct char_data *ch, bool mods);
void load_quests(FILE *fl, struct char_data *ch);
void load_feats(FILE *fl, struct char_data *ch);
void load_skill_focus(FILE *fl, struct char_data *ch);
void load_intros(FILE *fl, struct char_data *ch);
void load_HMVS(struct char_data *ch, const char *line, int mode);
void write_new_pet_data(struct char_data *ch);
void load_new_pet_data(struct char_data *ch);


/* external fuctions */
size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices);
bitvector_t asciiflag_conv(char *flag);
void innate_add(struct char_data * ch, int innate, int timer);
void memorize_add(struct char_data * ch, int spellnum, int timer);
void save_char_vars(struct char_data *ch);
time_t birth_age(struct char_data *ch);
time_t max_age(struct char_data *ch);

/* 'global' vars */
struct player_index_element *player_table = NULL;	/* index to plr file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */
extern struct deity_info deity_list[NUM_DEITIES];

extern int num_online;

/* external ASCII Player Files vars */
extern struct pclean_criteria_data pclean_criteria[];

/* ASCII Player Files - set this TRUE if you want poofin/poofout
   strings saved in the pfiles
 */
#define ASCII_SAVE_POOFS  FALSE

extern MYSQL *conn;


/*************************************************************************
*  stuff related to the player index					 *
*************************************************************************/


/* new version to build player index for ASCII Player Files */
/* generate index table for the player file */

/* new version to build player index for ASCII Player Files */
/* generate index table for the player file */
void build_player_index(void)
{
  int rec_count = 0, i = 0;
  FILE *plr_index;
  char index_name[40]={'\0'}, line[256]={'\0'}, bits[64]={'\0'};
  char arg2[80]={'\0'};

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r"))) {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);
  for (i = 0; i < rec_count; i++) {
    get_line(plr_index, line);
    player_table[i].admlevel = ADMLVL_NONE; /* In case they're not in the index yet */
    sscanf(line, "%ld %s %d %s %ld %d", &player_table[i].id, arg2,
      &player_table[i].level, bits, (long int*)&player_table[i].last, &player_table[i].admlevel);
    CREATE(player_table[i].name, char, strlen(arg2) + 1);
    strcpy(player_table[i].name, arg2);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}




/*
void build_player_index(void)
{
  int rec_count = 0, i;
  FILE *plr_index;
  char index_name[40], line[256], bits[64];
  char arg2[80];

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r"))) {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);
  for (i = 0; i < rec_count; i++) {
    get_line(plr_index, line);
    if (*line == '~')
      break;
    player_table[i].admlevel = ADMLVL_NONE; // In case they're not in the index yet 
    sscanf(line, "%ld %s %d %s %d %d", &player_table[i].id, arg2,
      &player_table[i].level, bits, 
 (int *) player_table[i].last, 
 &player_table[i].admlevel);
    CREATE(player_table[i].name, char, strlen(arg2) + 1);
    strcpy(player_table[i].name, arg2);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}
*/

/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
  int i = 0, pos = 0 ;

  if (top_of_p_table == -1) {	/* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
	/* Nothing */;

  /* clear the bitflag in case we have garbage data */
  player_table[pos].flags = 0;

  return (pos);
}


/* This function necessary to save a seperate ASCII player index */
void save_player_index(void)
{
  int i = 0;
  char index_name[50]={'\0'}, bits[64]={'\0'};
  FILE *index_file;

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name) {
      sprintascii(bits, player_table[i].flags);
      fprintf(index_file, "%ld %s %d %s %ld %d\n", player_table[i].id,
              player_table[i].name, player_table[i].level, *bits ? bits : "0",
              (long int)player_table[i].last, player_table[i].admlevel);
    }
  fprintf(index_file, "~\n");

  fclose(index_file);
}


void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++)
    if (player_table[tp].name)
      free(player_table[tp].name);

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}


long get_ptable_by_name(const char *name)
{
  int i = 0;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (i);

  return (-1);
}


long get_id_by_name(const char *name)
{
  int i = 0;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}


char *get_name_by_id(long id)
{
  int i = 0;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}


void load_follower_from_file(FILE *fl, struct char_data *ch)
{
  int nr = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};
  struct char_data *newch;

  if (!get_line(fl, line))
    return;

  if (line[0] != '#' || !line[1])
    return;

  nr = atoi(line + 1);
  newch = create_char();
  newch->nr = real_mobile(nr);

  if (!parse_mobile_from_file(fl, newch)) {
    free(newch);
  } else {
    add_follower(newch, ch);
    newch->master_id = GET_IDNUM(ch);
    GET_POS(newch) = POS_STANDING;
  }
}


void load_damreduce(struct char_data *ch, FILE *fl)
{
  struct damreduct_type *ptr;
  sh_int spell, feat;
  int mod, duration, max_damage;

  char buf[READ_SIZE]={'\0'};
  int i = 0;

  if (!get_line(fl, buf)) {
    log("Empty get_line reading damage reduction for %s", GET_NAME(ch));
    return;
  }
  if (sscanf(buf, "%hd %hd %d %d %d", &spell, &feat, &mod, &duration, &max_damage) != 5) {
    log("Error reading damage reduction for %s", GET_NAME(ch));
    return;
  }
  
  CREATE(ptr, struct damreduct_type, 1);
  ptr->next = ch->damreduct;
  ch->damreduct = ptr;
  ptr->spell = spell;
  ptr->feat = feat;
  ptr->mod = mod;
  ptr->duration = duration;
  ptr->max_damage = max_damage;
  for (i = 0; i < MAX_DAMREDUCT_MULTI; i++)
    ptr->damstyle[i] = ptr->damstyleval[i] = 0;
  i = 0;
  while (get_line(fl, buf)) {
    if (!strcmp(buf, "end"))
      break;
    if (sscanf(buf, "%d %d", ptr->damstyle + i, ptr->damstyleval + i) != 2)
      log("Bad damage reduction style line for %s", GET_NAME(ch));
    else
      i++;
  }

  return;
}



/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


#define NUM_OF_SAVE_THROWS	3

/* new load_char reads ASCII Player Files */
/* Load a char, TRUE if loaded, FALSE if not */
int load_char(const char *name, struct char_data *ch)
{
  int id, i, num = 0, num2 = 0, num3 = 0, num4 = 0;
  FILE *fl;
  char fname[READ_SIZE]={'\0'};
  char buf[128]={'\0'}, buf2[128]={'\0'}, line[MAX_INPUT_LENGTH + 1]={'\0'}, tag[6]={'\0'};
  char f1[128]={'\0'}, f2[128]={'\0'}, f3[128]={'\0'}, f4[128]={'\0'};
//  char *s1;

  if ((id = get_ptable_by_name(name)) < 0)
    return (-1);
  else {
    if (!get_filename(fname, sizeof(fname), PLR_FILE, player_table[id].name))
      return (-1);
    if (!(fl = fopen(fname, "r"))) {
      mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open player file %s", fname);
      return (-1);
    }

    /* character initializations */
    /* initializations necessary to keep some things straight */
    ch->affected = NULL;
    ch->affectedv = NULL;
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
      SET_SKILL(ch, i, 0);
      SET_SKILL_BONUS(ch, i, 0);
    }
    GET_SEX(ch) = PFDEF_SEX;
    ch->size = PFDEF_SIZE;
    GET_CRAFTING_TYPE(ch) = PFDEF_CRAFTING_TYPE;
    GET_CLASS(ch) = PFDEF_CLASS;
    for (i = 0; i < MAX_NUM_KNOWN_SPELLS; i++) {
      ch->player_specials->spells_known[i] = 0;
    }
    for (i = 0; i < 10; i++)
      ch->player_specials->spell_slots[i] = 0;

    for (i = 0; i < NUM_CLASSES; i++)
      GET_CLASS_SPONSOR(ch, i) = FALSE;
    GET_CLASS(ch) = PFDEF_CLASS;
    for (i = 0; i < NUM_CLASSES; i++) {
      GET_CLASS_NONEPIC(ch, i) = 0;
      GET_CLASS_EPIC(ch, i) = 0;
    }
    for (i = 0; i < 10; i++) {
    	ch->player_specials->wishlist[i][0] = 0;
		ch->player_specials->wishlist[i][1] = 0;
    }
    GET_HEAL_ROLL(ch) = PFDEF_HEAL_ROLL;
    GET_HEAL_AMOUNT(ch) = PFDEF_HEAL_AMOUNT;
    GET_HEAL_USED(ch) = PFDEF_HEAL_USED;
    GET_CAMP_USED(ch) = 0;
    GET_RESEARCH_TOKENS(ch) = 0;
    GET_REAL_RACE(ch) = PFDEF_RACE;
    GET_DISGUISE_RACE(ch) = PFDEF_RACE;
    GET_CLAN(ch) = PFDEF_CLAN;
    GET_CLAN_RANK(ch) = PFDEF_CLANRANK;
    GET_CARRY_STR_MOD(ch) = 0;
    ch->player_specials->account_name = NULL;
    ch->player_specials->short_descr = NULL;
    ch->player_specials->keywords = NULL;
    ch->player_specials->description = NULL;
    ch->player_specials->background_1 = NULL;
    ch->player_specials->background_2 = NULL;
    ch->player_specials->background_3 = NULL;
    ch->player_specials->background_4 = NULL;
    ch->player_specials->background_5 = NULL;
    ch->player_specials->background_6 = NULL;
    ch->player_specials->background_7 = NULL;
    ch->player_specials->background_8 = NULL;
    GET_ADMLEVEL(ch) = PFDEF_LEVEL;
    GET_CLASS_LEVEL(ch) = PFDEF_LEVEL;
    GET_HITDICE(ch) = PFDEF_LEVEL;
    GET_LEVEL_ADJ(ch) = PFDEF_LEVEL;
    GET_HOME(ch) = PFDEF_HOMETOWN;
    GET_HEIGHT(ch) = PFDEF_HEIGHT;
    GET_WEIGHT(ch) = PFDEF_WEIGHT;
    GET_ALIGNMENT(ch) = PFDEF_ALIGNMENT;
    GET_ETHIC_ALIGNMENT(ch) = PFDEF_ETHIC_ALIGNMENT;
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = PFDEF_AFFFLAGS;
    for (i = 0; i < PM_ARRAY_MAX; i++)
      PLR_FLAGS(ch)[i] = PFDEF_PLRFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
      PRF_FLAGS(ch)[i] = PFDEF_PREFFLAGS;
    for (i = 0; i < AD_ARRAY_MAX; i++)
      ADM_FLAGS(ch)[i] = 0;
    for (i = 0; i < NUM_OF_SAVE_THROWS; i++) {
      GET_SAVE_MOD(ch, i) = PFDEF_SAVETHROW;
      GET_SAVE_BASE(ch, i) = PFDEF_SAVETHROW;
    }

    GET_AUTOCQUEST_VNUM(ch) = 0;
    GET_AUTOCQUEST_DESC(ch) = strdup("nothing");
    GET_AUTOCQUEST_MAKENUM(ch) = 5;
    GET_AUTOCQUEST_QP(ch) = 0;
    GET_AUTOCQUEST_EXP(ch) = 0;
    GET_AUTOCQUEST_GOLD(ch) = 0;

//    ch->levelup->class = 0;
    GET_QUEST_COUNTER(ch) = PFDEF_QUESTCOUNT;
    GET_QUEST(ch) = PFDEF_CURRQUEST;
    GET_NUM_QUESTS(ch) = PFDEF_COMPQUESTS;
    GET_TURN_UNDEAD(ch) = 0;
    GET_PREF(ch) = get_new_pref();
    GET_STRENGTH_OF_HONOR(ch) = 0;
    GET_RAGE(ch) = 0;
    GET_DEFENSIVE_STANCE(ch) = 0;
    GET_SMITE_EVIL(ch) = 0;
    GET_MOUNT_VNUM(ch) = 0;
    GET_PET_VNUM(ch) = 0;
    GET_FAMILIAR_VNUM(ch) = 0;
    GET_COMPANION_VNUM(ch) = 0;
    GET_COMPANION_NAME(ch) = NULL;
    GET_EXPERTISE_BONUS(ch) = 0;
    GET_ENMITY(ch) = 0;
    GET_TOTAL_AOO(ch) = 0;
    GET_LOADROOM(ch) = 30000;
    GET_SPELLCASTER_LEVEL(ch) = 0;
    ch->stat_points_given = 0;
    GET_LAY_HANDS(ch) = 0;
    GET_RECALL(ch) = PFDEF_RECALLROOM;
    GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
    GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
    GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
    GET_POWERATTACK(ch) = PFDEF_POWERATT;
    GET_COND(ch, FULL) = PFDEF_HUNGER;
    ch->player_specials->crafting_exp_mult = 1;
    GET_COND(ch, THIRST) = PFDEF_THIRST;
    GET_COND(ch, DRUNK) = PFDEF_DRUNK;
    GET_BAD_PWS(ch) = PFDEF_BADPWS;
    GET_RACE_PRACTICES(ch) = PFDEF_PRACTICES;
    for (i = 0; i < NUM_CLASSES; i++)
      GET_PRACTICES(ch, i) = PFDEF_PRACTICES;
    GET_GOLD(ch) = PFDEF_GOLD;
    GET_TAVERN_EXP_BONUS(ch) = 0;
    GET_BANK_GOLD(ch) = PFDEF_BANK;
    GET_EXP(ch) = PFDEF_EXP;
    GET_ARTISAN_EXP(ch) = 0;
    GET_ARTISAN_TYPE(ch) = 0;
    GET_SCREEN_WIDTH(ch) = PFDEF_SCREENWIDTH;
    GET_QUESTPOINTS(ch) = PFDEF_QUESTPOINTS;
    GET_ACCOUNT_NUM(ch) = PFDEF_ACCT_NUM;
    GET_ACCOUNT_NAME(ch) = NULL;
    GET_ACCURACY_MOD(ch) = PFDEF_ACCURACY;
    GET_ACCURACY_BASE(ch) = PFDEF_ACCURACY;
    GET_DAMAGE_MOD(ch) = PFDEF_DAMAGE;
    GET_ARMOR(ch) = PFDEF_AC;
    ch->real_abils.str = PFDEF_STR;
    ch->real_abils.dex = PFDEF_DEX;
    ch->real_abils.intel = PFDEF_INT;
    ch->real_abils.wis = PFDEF_WIS;
    ch->real_abils.con = PFDEF_CON;
    ch->real_abils.cha = PFDEF_CHA;
	GET_IRDA_SHAPE_STATUS(ch) = PFDEF_IRDA_SHAPE_STATUS;
    GET_HIT(ch) = PFDEF_HIT;
    GET_MAX_HIT(ch) = PFDEF_MAXHIT;
    GET_EXTRA_ACC_EXP(ch) = 0;
    ch->bounty_gem = 0;
    GET_GATHER_INFO(ch) = 0;
    GET_MANA(ch) = PFDEF_MANA;
    GET_MAX_MANA(ch) = PFDEF_MAXMANA;
    GET_MOVE(ch) = PFDEF_MOVE;
    GET_MAX_MOVE(ch) = PFDEF_MAXMOVE;
    GET_KI(ch) = PFDEF_KI;
    GET_MAX_KI(ch) = PFDEF_MAXKI;
    SPEAKING(ch) = PFDEF_SPEAKING;
    GET_EPIC_SPELLS(ch) = 0;
    GET_BARD_SONGS(ch) = 0;
    ch->mentor_level = 0;
    for (i = 0; i < 7; i++)
      GET_BARD_SPELLS(ch, i) = 0;
    for (i = 0; i < 7; i++)
      ch->player_specials->bard_spells_to_learn[i] = 0;
    for (i = 0; i < 10; i++)
      GET_FAVORED_SOUL_SPELLS(ch, i) = 0;
    GET_FIGHT_BLEEDING_DAMAGE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GET_OLC_ZONE(ch) = PFDEF_OLC;
    GET_HOST(ch) = NULL;
    GUARDING(ch) = NULL;
    GUARDED_BY(ch) = NULL;
    ch->exp_chain = 0;
    ch->fight_over = 0;
    ch->player_specials->epic_dodge = FALSE;
    GET_LFG_STRING(ch) = NULL;
    GET_HP_BONUS(ch) = 0;
    GET_INTROS_GIVEN(ch) = 0;
    GET_INTROS_RECEIVED(ch) = 0;
    ch->spell_cast = FALSE;
    GET_WISH_STR(ch) = 0;
    GET_WISH_CON(ch) = 0;
    GET_WISH_DEX(ch) = 0;
    GET_WISH_INT(ch) = 0;
    GET_WISH_WIS(ch) = 0;
    GET_WISH_CHA(ch) = 0;
    ch->trains_spent = 0;
    ch->trains_unspent = 0;

    GET_AUTOQUEST_VNUM(ch) = 0;
    GET_AUTOQUEST_KILLNUM(ch) = 0;
    GET_AUTOQUEST_QP(ch) = 0;
    GET_AUTOQUEST_EXP(ch) = 0;
    GET_AUTOQUEST_GOLD(ch) = 0;
    GET_AUTOQUEST_DESC(ch) = strdup("nothing");

    ch->damage_reduction_feats = 0;
    ch->armor_skin_feats = 0;
    ch->fast_healing_feats = 0;

    GET_RP_EXP(ch) = 0;
    GET_RP_POINTS(ch) = 0;
    GET_RP_EXP_BONUS(ch) = 0;
    GET_RP_ART_EXP_BONUS(ch) = 0;
    GET_RP_GOLD_BONUS(ch) = 0;
    GET_RP_ACCOUNT_EXP(ch) = 0;
    GET_RP_QP_BONUS(ch) = 0;
    GET_RP_CRAFT_BONUS(ch) = 0;


    GET_STAT_MOB_KILLS(ch) = 0;

    for (i = 0; i < (700); i++)
      GET_INNATE(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM_C(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM_P(ch, i) = 0;
    for(i = 0; i < MAX_SPELL_LEVEL; i++)
      GET_SPELL_LEVEL(ch, i) = 0;
    for (i = SKILL_LOW_SKILL; i <= SKILL_HIGH_SKILL; i++)
      ch->player_specials->skill_focus[i-SKILL_LOW_SKILL] = 0;
    GET_MEMCURSOR(ch) = 0;
    GET_MEMCURSOR_C(ch) = 0;
    GET_MEMCURSOR_P(ch) = 0;
    ch->damreduct = NULL;
    ch->time.birth = ch->time.created = ch->time.maxage = 0;
    ch->followers = NULL;
    GET_PAGE_LENGTH(ch) = PFDEF_PAGELENGTH;
    GET_TICKS_PASSED(ch) = 0;
    IS_APPROVED(ch) = 0;
    GET_PC_DESCRIPTOR_1(ch) = 0;
    GET_PC_DESCRIPTOR_2(ch) = 0;
    GET_PC_ADJECTIVE_1(ch) = 0;
    GET_PC_ADJECTIVE_2(ch) = 0;

    ch->player_specials->irda_keywords_one = NULL;
    ch->player_specials->irda_keywords_two = NULL;
    ch->player_specials->irda_name_one = NULL;
    ch->player_specials->irda_name_two = NULL;
    ch->player_specials->irda_short_descr_one = NULL;
    ch->player_specials->irda_short_descr_two = NULL;
    ch->player_specials->irda_title_one = NULL;
    ch->player_specials->irda_title_two = NULL;

    ch->player_specials->RKit = 0;
    GET_DEITY(ch) = PFDEF_DEITY;  
    for (i = 0; i < NUM_COLOR; i++)
      ch->player_specials->color_choices[i] = NULL;
    for (i = 0; i < 65555; i++)
      ch->player_specials->rooms_visited[i] = 0;
    ch->player_specials->num_of_rooms_visited = 0;
    for (i = 0; i < NUM_CLASSES; i++)
      ch->player_specials->bonus_levels[i] = 0;
    GET_MARK(ch) = NULL;
    GET_MARK_ROUNDS(ch) = 0;
    GET_DEATH_ATTACK(ch) = 0;
    GET_FALSE_ETHOS(ch) = 0;
    GET_FALSE_ALIGNMENT(ch) = 0;
    GET_GUILD(ch) = -1;
    GET_SUBGUILD(ch) = -1;
    GET_GUILD_RANK(ch) = 1;
    GET_GUILD_EXP(ch) = 0;
    for (i = 0; i < NUM_RULES; i++)
      ch->player_specials->rules_read[i] = FALSE;
	ch->sum_name = NULL; //"Invalid";
	ch->sum_desc = NULL; //"Invalid";
	ch->summon_type = -1;

    while (get_line(fl, line)) {
      tag_argument(line, tag);

      switch (*tag) {
      case 'A':
             if (!strcmp(tag, "Ac  "))  GET_ARMOR(ch)           = atoi(line);
        else if (!strcmp(tag, "Acc "))  GET_ACCURACY_MOD(ch)    = atoi(line);
        else if (!strcmp(tag, "AccB"))  GET_ACCURACY_BASE(ch)   = atoi(line);
        else if (!strcmp(tag, "Acct"))  {
          ch->player_specials->account_name = strdup(line);
          if (ch->desc && ch->desc->account == NULL) {
           CREATE(ch->desc->account, struct account_data, 1);
           for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
             ch->desc->account->character_names[i] = NULL;

            load_account(ch->player_specials->account_name, ch->desc->account);
          }
        }
        else if (!strcmp(tag, "Act ")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            PLR_FLAGS(ch)[0] = asciiflag_conv(f1);
            PLR_FLAGS(ch)[1] = asciiflag_conv(f2);
            PLR_FLAGS(ch)[2] = asciiflag_conv(f3);
            PLR_FLAGS(ch)[3] = asciiflag_conv(f4);
        } else if (!strcmp(tag, "ActN"))  GET_ACCOUNT_NAME(ch)  = strdup(line);
        else if (!strcmp(tag, "ArXp"))  GET_ARTISAN_EXP(ch)   = atof(line);
        else if (!strcmp(tag, "ArTy"))  GET_ARTISAN_TYPE(ch)   = atoi(line);
        else if (!strcmp(tag, "Aff ")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            AFF_FLAGS(ch)[0] = asciiflag_conv(f1);
            AFF_FLAGS(ch)[1] = asciiflag_conv(f2);
            AFF_FLAGS(ch)[2] = asciiflag_conv(f3);
            AFF_FLAGS(ch)[3] = asciiflag_conv(f4);
        } else if (!strcmp(tag, "Affs"))  load_affects(fl, ch, 0);
        else if (!strcmp(tag, "Affv"))  load_affects(fl, ch, 1);
        else if (!strcmp(tag, "AdmL"))  GET_ADMLEVEL(ch)	= atoi(line);
        else if (!strcmp(tag, "AdmF")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
          ADM_FLAGS(ch)[0] = asciiflag_conv(f1);
          ADM_FLAGS(ch)[1] = asciiflag_conv(f2);
          ADM_FLAGS(ch)[2] = asciiflag_conv(f3);
          ADM_FLAGS(ch)[3] = asciiflag_conv(f4);
        } else if (!strcmp(tag, "Alin"))  GET_ALIGNMENT(ch)       = atoi(line);
        else if (!strcmp(tag, "Alis"))  ch->player_specials->keywords      = strdup(line);
        else if (!strcmp(tag, "Appr"))  IS_APPROVED(ch) = atoi(line);
        else if (!strcmp(tag, "AQVN"))  GET_AUTOQUEST_VNUM(ch) = atoi(line);
        else if (!strcmp(tag, "AQKN"))  GET_AUTOQUEST_KILLNUM(ch) = atoi(line);
        else if (!strcmp(tag, "AQQP"))  GET_AUTOQUEST_QP(ch) = atoi(line);
        else if (!strcmp(tag, "AQEX"))  GET_AUTOQUEST_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "AQGP"))  GET_AUTOQUEST_GOLD(ch) = atoi(line);
        else if (!strcmp(tag, "AQDS")) {
					free(GET_AUTOQUEST_DESC(ch));
					GET_AUTOQUEST_DESC(ch) = strdup(line);
				}
        else if (!strcmp(tag, "ACVN"))  GET_AUTOCQUEST_VNUM(ch) = atoi(line);
        else if (!strcmp(tag, "ACKN"))  GET_AUTOCQUEST_MAKENUM(ch) = atoi(line);
        else if (!strcmp(tag, "ACQP"))  GET_AUTOCQUEST_QP(ch) = atoi(line);
        else if (!strcmp(tag, "ACEX"))  GET_AUTOCQUEST_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "ACGP"))  GET_AUTOCQUEST_GOLD(ch) = atoi(line);
        else if (!strcmp(tag, "ACMT"))  GET_AUTOCQUEST_MATERIAL(ch) = atoi(line);
        else if (!strcmp(tag, "ACDS")) {
					free(GET_AUTOCQUEST_DESC(ch));
					GET_AUTOCQUEST_DESC(ch) = strdup(line);
				}
      break;

      case 'B':
             if (!strcmp(tag, "Badp"))  GET_BAD_PWS(ch)         = atoi(line);
        else if (!strcmp(tag, "Bank"))  GET_BANK_GOLD(ch)       = atoi(line);
        else if (!strcmp(tag, "BG1 "))  ch->player_specials->background_1 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG2 "))  ch->player_specials->background_2 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG3 "))  ch->player_specials->background_3 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG4 "))  ch->player_specials->background_4 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG5 "))  ch->player_specials->background_5 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG6 "))  ch->player_specials->background_6 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG7 "))  ch->player_specials->background_7 = fread_string(fl, buf2);
        else if (!strcmp(tag, "BG8 "))  ch->player_specials->background_8 = fread_string(fl, buf2);
        else if (!strcmp(tag, "Bled"))  GET_FIGHT_BLEEDING_DAMAGE(ch) = atoi(line);
        else if (!strcmp(tag, "BLvA"))  ch->player_specials->bonus_levels_arcane = atoi(line);
        else if (!strcmp(tag, "BLvD"))  ch->player_specials->bonus_levels_divine = atoi(line);
        else if (!strcmp(tag, "BLev"))  {
          do {
            get_line(fl, line);
            if (!strcmp(line, "EndBonusLevels\n"))
              break;
             sscanf(line, "%d %d", &num, &num2);
           if (num >= 0 && num < NUM_CLASSES)
             ch->player_specials->bonus_levels[num] = num2;
          } while (num != -1);
        }
        else if (!strcmp(tag, "Boot")) ch->boot_time = atoi(line);
        else if (!strcmp(tag, "Boun")) ch->bounty_gem = atoi(line);
        else if (!strcmp(tag, "BSng")) GET_BARD_SONGS(ch) = atoi(line);
        else if (!strcmp(tag, "BSp0")) GET_BARD_SPELLS(ch, 0) = atoi(line);
        else if (!strcmp(tag, "BSp1")) GET_BARD_SPELLS(ch, 1) = atoi(line);
        else if (!strcmp(tag, "BSp2")) GET_BARD_SPELLS(ch, 2) = atoi(line);
        else if (!strcmp(tag, "BSp3")) GET_BARD_SPELLS(ch, 3) = atoi(line);
        else if (!strcmp(tag, "BSp4")) GET_BARD_SPELLS(ch, 4) = atoi(line);
        else if (!strcmp(tag, "BSp5")) GET_BARD_SPELLS(ch, 5) = atoi(line);
        else if (!strcmp(tag, "BSp6")) GET_BARD_SPELLS(ch, 6) = atoi(line);
        else if (!strcmp(tag, "Bmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM_B(ch, GET_MEMCURSOR_B(ch)) = num2;
                GET_MEMCURSOR_B(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "Brth"))  ch->time.birth          = atol(line);
      break;

      case 'C':
             if (!strcmp(tag, "Cha "))  ch->real_abils.cha      = atoi(line);
        else if (!strcmp(tag, "Camp"))  GET_CAMP_USED(ch)         = atoi(line);
        else if (!strcmp(tag, "COut"))  (ch)->combat_output     = atoi(line);
        else if (!strcmp(tag, "CSpn")) {
          do {
          get_line(fl, line);
            if (!strcmp(line, "NoSponsor\n"))
              break;
            sscanf(line, "%d", &num);
            GET_CLASS_SPONSOR(ch, num) = TRUE;
          } while (num != -1 && atoi(strtok(line, "\n")) < NUM_CLASSES);
        }
        else if (!strcmp(tag, "CNum"))   GET_COMPANION_VNUM(ch)  = atoi(line);
        else if (!strcmp(tag, "CbFt")) {
          sscanf(line, "%d %s %s %s %s", &num, f1, f2, f3, f4);
          if (num < 0 || num > CFEAT_MAX) {
            log("load_char: %s combat feat record out of range: %s", GET_NAME(ch), line);
            break;
          }
          ch->combat_feats[num][0] = asciiflag_conv(f1);
          ch->combat_feats[num][1] = asciiflag_conv(f2);
          ch->combat_feats[num][2] = asciiflag_conv(f3);
          ch->combat_feats[num][3] = asciiflag_conv(f4);
        }
        else if (!strcmp(tag, "Clan"))  GET_CLAN(ch)           = atoi(line);
        else if (!strcmp(tag, "Clas"))  GET_CLASS(ch)           = atoi(line);
        else if (!strcmp(tag, "Cmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM_C(ch, GET_MEMCURSOR_C(ch)) = num2;
                GET_MEMCURSOR_C(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "Colr"))  {
          sscanf(line, "%d %s", &num, buf2);
          ch->player_specials->color_choices[num] = strdup(buf2);
        }
        else if (!strcmp(tag, "Con "))  ch->real_abils.con      = atoi(line);
        else if (!strcmp(tag, "Crtd"))  ch->time.created        = atol(line);
        else if (!strcmp(tag, "CStr"))  GET_CARRY_STR_MOD(ch)   = atoi(line); 
        
      break;

      case 'D':
             if (!strcmp(tag, "Desc"))  ch->player_specials->description  = fread_string(fl, buf2);
        else if (!strcmp(tag, "DfSt"))  GET_DEFENSIVE_STANCE(ch) = atoi(line);
        else if (!strcmp(tag, "Dex "))  ch->real_abils.dex      = atoi(line);
        else if (!strcmp(tag, "Dom1"))  GET_DOMAIN_ONE(ch) = atoi(line);
        else if (!strcmp(tag, "Dom2"))  GET_DOMAIN_TWO(ch) = atoi(line);
        else if (!strcmp(tag, "Drnk"))  GET_COND(ch, DRUNK)     = atoi(line);
        else if (!strcmp(tag, "Damg"))  GET_DAMAGE_MOD(ch)          = atoi(line);
        else if (!strcmp(tag, "DRac"))  GET_DISGUISE_RACE(ch)          = atoi(line);
        else if (!strcmp(tag, "DmRd"))  load_damreduce(ch, fl);
        else if (!strcmp(tag, "Dmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM_D(ch, GET_MEMCURSOR_D(ch)) = num2;
                GET_MEMCURSOR_D(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }

      break;

      case 'E':
             if (!strcmp(tag, "Exp "))  GET_EXP(ch)             = atoi(line);
        else if (!strcmp(tag, "ExAE"))  GET_EXTRA_ACC_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "Eali"))  GET_ETHIC_ALIGNMENT(ch) = atoi(line);
        else if (!strcmp(tag, "Ecls"))  { sscanf(line, "%d=%d", &num, &num2);
                                          GET_CLASS_EPIC(ch, num) = num2; }
        else if (!strcmp(tag, "ESpl")) GET_EPIC_SPELLS(ch) = atoi(line);
        else if (!strcmp(tag, "EFDR")) ch->damage_reduction_feats = atoi(line);
        else if (!strcmp(tag, "EFFH")) ch->fast_healing_feats = atoi(line);
        else if (!strcmp(tag, "EFAS")) ch->armor_skin_feats = atoi(line);
    
      break;

      case 'F':
             if (!strcmp(tag, "Frez"))  GET_FREEZE_LEV(ch)      = atoi(line);
        else if (!strcmp(tag, "FAln"))  GET_FALSE_ALIGNMENT(ch)     = atoi(line);
        else if (!strcmp(tag, "FEth"))  GET_FALSE_ETHOS(ch)     = atoi(line); 
        else if (!strcmp(tag, "Feat"))  load_feats(fl, ch);
        else if (!strcmp(tag, "Fpnt"))  GET_FEAT_POINTS(ch)     = atoi(line);
        else if (!strcmp(tag, "FEpc"))  GET_EPIC_FEAT_POINTS(ch)= atoi(line);
        else if (!strcmp(tag, "FCls"))  { sscanf(line, "%d %d", &num, &num2);
          GET_CLASS_FEATS(ch, num) = num2; }
        else if (!strcmp(tag, "FECl"))  { sscanf(line, "%d %d", &num, &num2);
          GET_EPIC_CLASS_FEATS(ch, num) = num2; }
        else if (!strcmp(tag, "FNum")) GET_FAMILIAR_VNUM(ch) = atoi(line);
        else if (!strcmp(tag, "FSp0")) GET_FAVORED_SOUL_SPELLS(ch, 0) = atoi(line);
        else if (!strcmp(tag, "FSp1")) GET_FAVORED_SOUL_SPELLS(ch, 1) = atoi(line);
        else if (!strcmp(tag, "FSp2")) GET_FAVORED_SOUL_SPELLS(ch, 2) = atoi(line);
        else if (!strcmp(tag, "FSp3")) GET_FAVORED_SOUL_SPELLS(ch, 3) = atoi(line);
        else if (!strcmp(tag, "FSp4")) GET_FAVORED_SOUL_SPELLS(ch, 4) = atoi(line);
        else if (!strcmp(tag, "FSp5")) GET_FAVORED_SOUL_SPELLS(ch, 5) = atoi(line);
        else if (!strcmp(tag, "FSp6")) GET_FAVORED_SOUL_SPELLS(ch, 6) = atoi(line);
        else if (!strcmp(tag, "FSp7")) GET_FAVORED_SOUL_SPELLS(ch, 7) = atoi(line);
        else if (!strcmp(tag, "FSp8")) GET_FAVORED_SOUL_SPELLS(ch, 8) = atoi(line);
        else if (!strcmp(tag, "FSp9")) GET_FAVORED_SOUL_SPELLS(ch, 9) = atoi(line);
      break;

      case 'G':
             if (!strcmp(tag, "God "))  { GET_DEITY(ch)           = atoi(line);}
        else if (!strcmp(tag, "Gath"))  { GET_GATHER_INFO(ch)     = atoi(line);}
        else if (!strcmp(tag, "Gold"))  { GET_GOLD(ch)            = atoi(line);}
        else if (!strcmp(tag, "GSD1"))  GET_PC_DESCRIPTOR_1(ch)   = atoi(line);
        else if (!strcmp(tag, "GSD2"))  GET_PC_DESCRIPTOR_2(ch)   = atoi(line);
        else if (!strcmp(tag, "GSA1"))  GET_PC_ADJECTIVE_1(ch)   = atoi(line);
        else if (!strcmp(tag, "GSA2"))  GET_PC_ADJECTIVE_2(ch)   = atoi(line);
        else if (!strcmp(tag, "Gild"))  GET_GUILD(ch)   = atoi(line);
        else if (!strcmp(tag, "GSub"))  GET_SUBGUILD(ch)   = atoi(line);
        else if (!strcmp(tag, "GRnk"))  GET_GUILD_RANK(ch)   = atoi(line);
        else if (!strcmp(tag, "GExp"))  GET_GUILD_EXP(ch)   = atoi(line);
      break;

      case 'H':
             if (!strcmp(tag, "Hit "))  load_HMVS(ch, line, LOAD_HIT);
        else if (!strcmp(tag, "Heal"))  GET_HEAL_USED(ch)         = atoi(line);
        else if (!strcmp(tag, "HAmt"))  GET_HEAL_AMOUNT(ch)         = atoi(line);
        else if (!strcmp(tag, "HPBn"))  GET_HP_BONUS(ch)         = atoi(line);
        else if (!strcmp(tag, "HRol"))  GET_HEAL_ROLL(ch)         = atoi(line);
        else if (!strcmp(tag, "HitD"))  GET_HITDICE(ch)         = atoi(line);
        else if (!strcmp(tag, "Hite"))  GET_HEIGHT(ch)          = atoi(line);
        else if (!strcmp(tag, "Home"))  GET_HOME(ch)            = atoi(line);
        else if (!strcmp(tag, "Host")) {
          if (GET_HOST(ch))
            free(GET_HOST(ch));
          GET_HOST(ch) = strdup(line);
        }
        else if (!strcmp(tag, "Hung"))  GET_COND(ch, FULL)      = atoi(line);
      break;

      case 'I':
             if (!strcmp(tag, "Id  "))  GET_IDNUM(ch)           = atol(line);
        else if (!strcmp(tag, "Int "))  ch->real_abils.intel    = atoi(line);
        else if (!strcmp(tag, "Invs"))  GET_INVIS_LEV(ch)       = atoi(line);
        else if (!strcmp(tag, "Intr"))  load_intros(fl, ch);
        else if (!strcmp(tag, "IGiv"))  GET_INTROS_GIVEN(ch)    = atoi(line);
        else if (!strcmp(tag, "IRec"))  GET_INTROS_RECEIVED(ch) = atoi(line);
        else if (!strcmp(tag, "ISAF"))  ch->imp_sneak_attack_feats	= atoi(line);
        else if (!strcmp(tag, "InAb")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if(num2 != -1)
              GET_INNATE(ch, num2) = num3;
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "Inna")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if(num2 != -1)
              innate_add(ch, num2, num3);
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "IDs1")) ch->player_specials->irda_description_one = fread_string(fl, buf2);
        else if (!strcmp(tag, "IDs2")) ch->player_specials->irda_description_two = fread_string(fl, buf2);
        else if (!strcmp(tag, "IKw1")) ch->player_specials->irda_keywords_one = strdup(line);
        else if (!strcmp(tag, "IKw2")) ch->player_specials->irda_keywords_two = strdup(line);
        else if (!strcmp(tag, "INa1")) ch->player_specials->irda_name_one = strdup(line);
        else if (!strcmp(tag, "INa2")) ch->player_specials->irda_name_two = strdup(line);
        else if (!strcmp(tag, "ISD1")) ch->player_specials->irda_short_descr_one = strdup(line);
        else if (!strcmp(tag, "ISD2")) ch->player_specials->irda_short_descr_two = strdup(line);
        else if (!strcmp(tag, "ISSt")) GET_IRDA_SHAPE_STATUS(ch) = atoi(line);
        else if (!strcmp(tag, "ITi1")) GET_IRDA_TITLE_1(ch) = strdup(line);
        else if (!strcmp(tag, "ITi2")) GET_IRDA_TITLE_2(ch) = strdup(line);
      break;

      case 'K':
             if (!strcmp(tag, "Ki  "))  load_HMVS(ch, line, LOAD_KI);
        else if (!strcmp(tag, "KeyW"))  ch->player_specials->keywords = strdup(line);
      break;

      case 'L':
             if (!strcmp(tag, "Last"))  ch->time.logon          = atol(line);
        else if (!strcmp(tag, "LayH"))  GET_LAY_HANDS(ch)       = atoi(line);
        else if (!strcmp(tag, "Lern"))  GET_PRACTICES(ch, GET_CLASS(ch)) = atoi(line);
        else if (!strcmp(tag, "Levl"))  GET_CLASS_LEVEL(ch)     = atoi(line);
        else if (!strcmp(tag, "LevD"))  read_level_data(ch, fl);
        else if (!strcmp(tag, "LFG "))  GET_LFG_STRING(ch) = strdup(line);
        else if (!strcmp(tag, "LStg"))  GET_LEVEL_STAGE(ch)   = atoi(line);
        else if (!strcmp(tag, "LvlA"))  GET_LEVEL_ADJ(ch)     = atoi(line);
        else if (!strcmp(tag, "LDsc"))  GET_LONG_DESC(ch)  = strdup(line);
/*
        else if (!strcmp(tag, "LvFt"))  {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if(num2 != -1) {
              struct level_data *lvl = ch->level_info->level_extra;
              for (lvl; lvl; lvl = lvl->next) {
                if (lvl->level == num2) {

                }
              } 
            }
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "LvSk"))  read_level_skills(ch, fl);
        else if (!strcmp(tag, "LvSp"))  read_level_spells(ch, fl);
        else if (!strcmp(tag, "LvTr"))  read_level_trains(ch, fl);
*/
      break;

      case 'M':
             if (!strcmp(tag, "Mana"))  load_HMVS(ch, line, LOAD_MANA);
        else if (!strcmp(tag, "Mmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM(ch, GET_MEMCURSOR(ch)) = num2;
                GET_MEMCURSOR(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }

        else if (!strcmp(tag, "Move"))  load_HMVS(ch, line, LOAD_MOVE);
        else if (!strcmp(tag, "Mcls"))  { sscanf(line, "%d=%d", &num, &num2);
                                          GET_CLASS_NONEPIC(ch, num) = num2; }
        else if (!strcmp(tag, "MxAg"))  ch->time.maxage         = atol(line);

        else if (!strcmp(tag, "MNum")) GET_MOUNT_VNUM(ch)   = atoi(line);
        else if (!strcmp(tag, "Mntd"))  ch->player_specials->mounted = atoi(line);
        else if (!strcmp(tag, "Ment"))  ch->mentor_level = atoi(line);
        else if (!strcmp(tag, "Mnt "))  ch->player_specials->mount = atoi(line);
	else if (!strcmp(tag, "MtNm"))  ch->player_specials->mount_num = atoi(line);
	else if (!strcmp(tag, "MtDs"))  ch->player_specials->mount_desc = strdup(line);
	else if (!strcmp(tag, "MtCH"))  ch->player_specials->mount_cur_hit = atoi(line);
	else if (!strcmp(tag, "MtMH"))  ch->player_specials->mount_max_hit = atoi(line);
	else if (!strcmp(tag, "MtAC"))  ch->player_specials->mount_ac = atoi(line);
	else if (!strcmp(tag, "MtDR"))  ch->player_specials->mount_dr = atoi(line);
        else if (!strcmp(tag, "MtAt")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d %d %d", &num, &num2, &num3, &num4);
            if (num == -1)
              break;
            ch->player_specials->mount_attack_to_hit[i] = num;
            ch->player_specials->mount_attack_ndice[i] = num2;
            ch->player_specials->mount_attack_sdice[i] = num3;
            ch->player_specials->mount_attack_dammod[i] = num4;
            i++;
          } while (num != -1);
        }       
      break;

      case 'N':
             if (!strcmp(tag, "Name"))  GET_PC_NAME(ch)         = strdup(line);
      break;

      case 'O':
             if (!strcmp(tag, "Olc "))  GET_OLC_ZONE(ch) = atoi(line);
      break;

      case 'P':
             if (!strcmp(tag, "Page"))  GET_PAGE_LENGTH(ch) = atoi(line);
        else if (!strcmp(tag, "Pass"))  strcpy(GET_PASSWD(ch), line);
        else if (!strcmp(tag, "Plyd"))  ch->time.played          = atol(line);
#ifdef ASCII_SAVE_POOFS
        else if (!strcmp(tag, "PfIn"))  POOFIN(ch)               = strdup(line);
        else if (!strcmp(tag, "PfOt"))  POOFOUT(ch)              = strdup(line);
#endif
        else if (!strcmp(tag, "Pmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM_P(ch, GET_MEMCURSOR_P(ch)) = num2;
                GET_MEMCURSOR_P(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "PNum")) GET_PET_VNUM(ch)          = atoi(line);
        else if (!strcmp(tag, "PoAm"))  GET_POISON_DAMAGE_AMOUNT(ch) = atoi(line);
        else if (!strcmp(tag, "PoTy"))  GET_POISON_DAMAGE_TYPE(ch) = atoi(line);
        else if (!strcmp(tag, "Pref")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
          PRF_FLAGS(ch)[0] = asciiflag_conv(f1);
          PRF_FLAGS(ch)[1] = asciiflag_conv(f2);
          PRF_FLAGS(ch)[2] = asciiflag_conv(f3);
          PRF_FLAGS(ch)[3] = asciiflag_conv(f4);
        }
        else if (!strcmp(tag, "PwrA"))  GET_POWERATTACK(ch)      = atoi(line);
        else if (!strcmp(tag, "PrfL"))  GET_PREF(ch)             = atoi(line);
        else if (!strcmp(tag, "PvPT"))  (ch)->pvp_timer          = atoi(line);
      break;

      case 'Q':
        if (!strcmp(tag, "Qpnt")) GET_QUESTPOINTS(ch) = atoi(line); /* Backward compatibility */
        else if (!strcmp(tag, "Qcur")) GET_QUEST(ch) = atoi(line);
        else if (!strcmp(tag, "Qcnt")) GET_QUEST_COUNTER(ch) = atoi(line);
        else if (!strcmp(tag, "Qest")) load_quests(fl, ch);
        break;

      case 'R':
             if (!strcmp(tag, "Race"))  GET_REAL_RACE(ch)            = atoi(line);
        else if (!strcmp(tag, "Rage"))  GET_RAGE(ch)  = atoi(line);
        else if (!strcmp(tag, "Recl"))  GET_RECALL(ch)          = atoi(line);
        else if (!strcmp(tag, "RKit"))  ch->player_specials->RKit = atoi(line);
        else if (!strcmp(tag, "Room"))  GET_LOADROOM(ch)        = atoi(line);
        else if (!strcmp(tag, "Rank"))  GET_CLAN_RANK(ch) = atoi(line);
        else if (!strcmp(tag, "RPEx"))  GET_RP_EXP_BONUS(ch) = atoi(line);
        else if (!strcmp(tag, "RPAE"))  GET_RP_ART_EXP_BONUS(ch) = atoi(line);
        else if (!strcmp(tag, "RPGp"))  GET_RP_GOLD_BONUS(ch) = atoi(line);
        else if (!strcmp(tag, "RPAE"))  GET_RP_ACCOUNT_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "RPQP"))  GET_RP_QP_BONUS(ch) = atoi(line);
        else if (!strcmp(tag, "RPCr"))  GET_RP_CRAFT_BONUS(ch) = atoi(line);
        else if (!strcmp(tag, "RPPt"))  GET_RP_POINTS(ch) = atoi(line);

        else if (!strcmp(tag, "RVNm"))  ch->player_specials->num_of_rooms_visited = atoi(line);
        else if (!strcmp(tag, "RmVs")) {
          for (i = 0; i < 65555; i++) {
            get_line(fl, line);
            sscanf(line, "%d", &num);
            ch->player_specials->rooms_visited[i] = num;
          }
        }
        else if (!strcmp(tag, "RVis")) {
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num, &num2);
            if (num == -1)
              break;
            ch->player_specials->rooms_visited[num] = num2;
          } while (num != -1);
        }
        else if (!strcmp(tag, "Rmem")) {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if (num2 != -1) {
              /* if memorized add to arrays */
              if (num3 == 0) {
                GET_SPELLMEM_R(ch, GET_MEMCURSOR_R(ch)) = num2;
                GET_MEMCURSOR_R(ch)++;
                /* otherwise add to in-progress linked-list */
              } else {
              GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
              memorize_add(ch, num2, num3);
              GET_MEM_TYPE(ch) = 0;
              }
            }
            num++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "RTok"))  GET_RESEARCH_TOKENS(ch) = atoi(line);
      break;

      case 'S':
             if (!strcmp(tag, "Sex "))  GET_SEX(ch)             = atoi(line);
        else if (!strcmp(tag, "ScrW"))  GET_SCREEN_WIDTH(ch) = atoi(line);
        else if (!strcmp(tag, "Skil"))  load_skills(fl, ch, FALSE);
        else if (!strcmp(tag, "Size"))  ch->size  = atoi(line);
        else if (!strcmp(tag, "SklB"))  load_skills(fl, ch, TRUE);
        else if (!strcmp(tag, "SklF"))  load_skill_focus(fl, ch);
        else if (!strcmp(tag, "SkRc"))  GET_RACE_PRACTICES(ch)	= atoi(line);
        else if (!strcmp(tag, "SnAF"))  ch->sneak_attack_feats	= atoi(line);
        else if (!strcmp(tag, "Stat"))	GET_STAT_POINTS(ch)	= atoi(line);
        else if (!strcmp(tag, "StGv"))	(ch)->stat_points_given	= atoi(line);
        else if (!strcmp(tag, "StMK"))	GET_STAT_MOB_KILLS(ch)	= atoi(line);
        else if (!strcmp(tag, "SDsc"))  GET_PC_SDESC(ch)	= strdup(line);
        else if (!strcmp(tag, "SkCl")) {
          sscanf(line, "%d %d", &num2, &num3);
          GET_PRACTICES(ch, num2) = num3;
        }
        else if (!strcmp(tag, "Spek"))  SPEAKING(ch)  = atoi(line);
        else if (!strcmp(tag, "SpKn")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d", &num);
            ch->player_specials->spells_known[i] = num;
            i++;
          } while (num != -2);
        }
        else if (!strcmp(tag, "SclF")) {
          sscanf(line, "%d %s", &num, f1);
          if (num < 0 || num > SFEAT_MAX) {
            log("load_char: %s school feat record out of range: %s", GET_NAME(ch), line);
            break;
          }
          ch->school_feats[num] = asciiflag_conv(f1);
        }
        else if (!strcmp(tag, "Str "))  ch->real_abils.str = atoi(line);
        else if (!strcmp(tag, "StrH"))  GET_STRENGTH_OF_HONOR(ch) = atoi(line);
	else if (!strcmp(tag, "SmNm"))  ch->player_specials->summon_num = atoi(line);
	else if (!strcmp(tag, "SmDs"))  ch->player_specials->summon_desc = strdup(line);
	else if (!strcmp(tag, "SmCH"))  ch->player_specials->summon_cur_hit = atoi(line);
	else if (!strcmp(tag, "SmMH"))  ch->player_specials->summon_max_hit = atoi(line);
	else if (!strcmp(tag, "SmAC"))  ch->player_specials->summon_ac = atoi(line);
	else if (!strcmp(tag, "SmDR"))  ch->player_specials->summon_dr = atoi(line);
	else if (!strcmp(tag, "SmTm"))  ch->player_specials->summon_timer = atoi(line);
        else if (!strcmp(tag, "SmAt")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d %d %d", &num, &num2, &num3, &num4);
            if (num == -1)
              break;
            ch->player_specials->summon_attack_to_hit[i] = num;
            ch->player_specials->summon_attack_ndice[i] = num2;
            ch->player_specials->summon_attack_sdice[i] = num3;
            ch->player_specials->summon_attack_dammod[i] = num4;
            i++;
          } while (num != -1);
        }       
	else if (!strcmp(tag, "SumN"))  ch->sum_name = fread_string(fl, buf2);
	else if (!strcmp(tag, "SumD"))  ch->sum_desc = fread_string(fl, buf2);
	else if (!strcmp(tag, "SumT"))  ch->summon_type = atoi(line);
      break;

      case 'T':
             if (!strcmp(tag, "TLRm"))  GET_TEMP_LOADROOM(ch)   = atoi(line);
        else if (!strcmp(tag, "Thir"))  GET_COND(ch, THIRST)    = atoi(line);
        else if (!strcmp(tag, "Thr1"))  GET_SAVE_MOD(ch, 0)     = atoi(line);
        else if (!strcmp(tag, "Thr2"))  GET_SAVE_MOD(ch, 1)     = atoi(line);
        else if (!strcmp(tag, "Thr3"))  GET_SAVE_MOD(ch, 2)     = atoi(line);
        else if (!strcmp(tag, "Thr4") || !strcmp(tag, "Thr5")); /* Discard extra saves */
        else if (!strcmp(tag, "ThB1"))  GET_SAVE_BASE(ch, 0)    = atoi(line);
        else if (!strcmp(tag, "ThB2"))  GET_SAVE_BASE(ch, 1)    = atoi(line);
        else if (!strcmp(tag, "ThB3"))  GET_SAVE_BASE(ch, 2)    = atoi(line);
        else if (!strcmp(tag, "Titl"))  GET_TITLE(ch)           = strdup(line);
        else if (!strcmp(tag, "Trns"))  GET_TRAINS(ch)          = atoi(line);
        else if (!strcmp(tag, "TrSp"))  ch->trains_spent        = atoi(line);
        else if (!strcmp(tag, "TrUS"))  ch->trains_unspent      = atoi(line);
        else if (!strcmp(tag, "Turn"))  GET_TURN_UNDEAD(ch)     = atoi(line);
      break;

      case 'W':
             if (!strcmp(tag, "Wate"))  GET_WEIGHT(ch)          = atoi(line);
        else if (!strcmp(tag, "Wimp"))  GET_WIMP_LEV(ch)        = atoi(line);
        else if (!strcmp(tag, "Wis "))  ch->real_abils.wis      = atoi(line);
        else if (!strcmp(tag, "Wish"))  GET_WISHES(ch)          = atoi(line);
        else if (!strcmp(tag, "WLst")) {
        	i = 0;
        	do {
        	  get_line(fl, line);
        	  sscanf(line, "%d %d", &num, &num2);
        	  ch->player_specials->wishlist[i][0] = num;
        	  ch->player_specials->wishlist[i][1] = num2;
        	  i++;
        	} while (num != -1);
        }
        else if (!strcmp(tag, "WStr"))  GET_WISH_STR(ch)        = atoi(line);
        else if (!strcmp(tag, "WCon"))  GET_WISH_CON(ch)        = atoi(line);
        else if (!strcmp(tag, "WDex"))  GET_WISH_DEX(ch)        = atoi(line);
        else if (!strcmp(tag, "WInt"))  GET_WISH_INT(ch)        = atoi(line);
        else if (!strcmp(tag, "WWis"))  GET_WISH_WIS(ch)        = atoi(line);
        else if (!strcmp(tag, "WCha"))  GET_WISH_CHA(ch)        = atoi(line);
      break;

      default:
        sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
      }
    }
  }

  if (GET_CLASS_RANKS(ch, CLASS_ROGUE) && !HAS_FEAT(ch, FEAT_SNEAK_ATTACK)) {
    log("Loading rogue '%s' without sneak attack ranks, fixing", GET_NAME(ch));
    SET_FEAT(ch, FEAT_SNEAK_ATTACK, (GET_CLASS_RANKS(ch, CLASS_ROGUE) + 1) / 2);
  }

  if (! ch->time.created) {
    log("No creation timestamp for user %s, using current time", GET_NAME(ch));
    ch->time.created = time(0);
  }

  if (! ch->time.birth) {
    log("No birthday for user %s, using standard starting age determination", GET_NAME(ch));
    ch->time.birth = time(0) - birth_age(ch);
  }

  if (! ch->time.maxage) {
    log("No max age for user %s, using standard max age determination", GET_NAME(ch));
    ch->time.maxage = ch->time.birth + max_age(ch);
  }
  
  if (GET_RAGE(ch) == 0 && !affected_by_spell(ch, SPELL_AFF_RAGE))
  	GET_RAGE(ch) = HAS_FEAT(ch, FEAT_RAGE);

  if (GET_DEFENSIVE_STANCE(ch) == 0 && !affected_by_spell(ch, SPELL_AFF_DEFENSIVE_STANCE))
  	GET_DEFENSIVE_STANCE(ch) = HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE);

  if (GET_STRENGTH_OF_HONOR(ch) == 0 && !affected_by_spell(ch, SPELL_AFF_STRENGTH_OF_HONOR))
  	GET_STRENGTH_OF_HONOR(ch) = HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR);  	
  	
  if (GET_SMITE_EVIL(ch) == 0 && !affected_by_spell(ch, ABIL_SMITE_EVIL))
  	GET_SMITE_EVIL(ch) = HAS_FEAT(ch, FEAT_SMITE_EVIL);
  		
	if (GET_TURN_UNDEAD(ch) < 1 && !affected_by_spell(ch, SPELL_AFF_TURN_UNDEAD))
	  GET_TURN_UNDEAD(ch) = MAX(1, 3 + ability_mod_value(GET_CHA(ch)) + (HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2));
  	
  affect_total(ch);

  /* initialization for imms */
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    for (i = 1; i <= SKILL_TABLE_SIZE; i++)
      SET_SKILL(ch, i, 100);
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }

  fclose(fl);
  return(id);
}


/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
   it's part of obuild).  Remove this if you get a
   multiple definition error or if it you want to use a
   substitute
*/
void kill_ems(char *str)
{
  char *ptr1, *ptr2, *tmp;

  tmp = str;
  ptr1 = str;
  ptr2 = str;

  while (*ptr1) {
    if ((*(ptr2++) = *(ptr1++)) == '\r')
      if (*ptr1 == '\r')
	ptr1++;
  }
  *ptr2 = '\0';
}

void write_new_pet_data(struct char_data *ch)
{
	struct follow_type *foll;
	char fname[40]={'\0'};
	FILE *fl;

	if( IS_NPC(ch) || GET_PFILEPOS(ch) < 0) return;
	if (!get_filename(fname, sizeof(fname), PET_FILE_NEW, GET_NAME(ch))) return;
	if (!(fl = fopen(fname, "w")))
	{
	    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open new pet file %s for write", fname);
		return;
	}

	for( foll = ch->followers; foll; foll = foll->next)
	{
		if( foll->follower && IS_NPC(foll->follower) && AFF_FLAGGED(foll->follower, AFF_CHARM) && foll->follower->new_summon == true)
		{
			if( IN_ROOM(foll->follower) == IN_ROOM(ch) || IN_ROOM(ch) == 1)
			{
				write_mobile_record(GET_MOB_VNUM(foll->follower), foll->follower, fl);
			}
		}
	}


}

void save_char_pets(struct char_data *ch)
{
  struct follow_type *foll;
  char fname[40]={'\0'};
  FILE *fl;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  if (!get_filename(fname, sizeof(fname), PET_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open pet file %s for write", fname);
    return;
  }

  for (foll = ch->followers; foll; foll = foll->next) {
    if (foll->follower && IS_NPC(foll->follower) && AFF_FLAGGED(foll->follower, AFF_CHARM) && foll->follower->new_summon == false) {
      if (IN_ROOM(foll->follower) == IN_ROOM(ch) || IN_ROOM(ch) == 1 /* Idle void */)
        write_mobile_record(GET_MOB_VNUM(foll->follower), foll->follower, fl);
    }
  }
  fprintf(fl, "$\n");

  fclose(fl);
}


void load_char_pets(struct char_data *ch)
{
  char fname[40]={'\0'};
  FILE *fl;
  room_rnum load_room;
  struct follow_type *foll;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  if (!get_filename(fname, sizeof(fname), PET_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "r")))
    return;

  while (!feof(fl))
    load_follower_from_file(fl, ch);

  load_room = IN_ROOM(ch);
  for (foll = ch->followers; foll; foll = foll->next) {
    char_to_room(foll->follower, load_room);
    act("You are joined by $N.", FALSE, ch, 0, foll->follower, TO_CHAR);
  }
}


/*
 * write the vital data of a player to the player file
 *
 * And that's it! No more fudging around with the load room.
 */
/* This is the ASCII Player Files save routine */
void save_char(struct char_data * ch)
{
  FILE *fl;
  char fname[40]={'\0'}, tmpname[40]={'\0'}, bakname[40]={'\0'}, buf[MAX_STRING_LENGTH]={'\0'};
  int i = 0, j = 0, id = 0, save_index = FALSE;
  struct affected_type *aff, tmp_aff[MAX_AFFECT], tmp_affv[MAX_AFFECT];
  struct damreduct_type *reduct;
  struct obj_data *char_eq[NUM_WEARS];
  char fbuf1[MAX_STRING_LENGTH]={'\0'}, fbuf2[MAX_STRING_LENGTH]={'\0'};
  char fbuf3[MAX_STRING_LENGTH]={'\0'}, fbuf4[MAX_STRING_LENGTH]={'\0'};
  struct memorize_node *mem, *next;
  struct innate_node *inn = NULL, *next_inn = NULL;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  /*
   * If ch->desc is not null, then we need to update some session data
   * before saving.
   */
  if (ch->desc) {
    if (ch->desc->host && *ch->desc->host) {
      if (!GET_HOST(ch))
        GET_HOST(ch) = strdup(ch->desc->host);
      else if (GET_HOST(ch) && !strcmp(GET_HOST(ch), ch->desc->host)) {
        free(GET_HOST(ch));
      GET_HOST(ch) = strdup(ch->desc->host);
      }
    }

    /*
     * We only update the time.played and time.logon if the character
     * is playing.
     */
    if (STATE(ch->desc) == CON_PLAYING) {
      ch->time.played += time(0) - ch->time.logon;
      ch->time.logon = time(0);
    }
  }

  if (!get_filename(fname, sizeof(fname), PLR_FILE, GET_NAME(ch)))
    return;
  snprintf(tmpname, sizeof(tmpname), "%s.new", fname);
  snprintf(bakname, sizeof(bakname), "%s.bak", fname);
  if (!(fl = fopen(tmpname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open temporary player file %s for write", tmpname);
    return;
  }

  /* remove affects from eq and spells (from char_to_store) */
  /* Unaffect everything a character can be affected by */

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      char_eq[i] = unequip_char(ch, i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    } else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_aff[i] = *aff;
      tmp_aff[i].next = 0;
      aff = aff->next;
    } else {
      tmp_aff[i].type = 0;	/* Zero signifies not used */
      tmp_aff[i].duration = 0;
      tmp_aff[i].modifier = 0;
      tmp_aff[i].specific = 0;
      tmp_aff[i].location = 0;
      tmp_aff[i].bitvector = 0;
      tmp_aff[i].next = 0;
    }
  }

  for (aff = ch->affectedv, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_affv[i] = *aff;
      tmp_affv[i].next = 0;
      aff = aff->next;
    } else {
      tmp_affv[i].type = 0;      /* Zero signifies not used */
      tmp_affv[i].duration = 0;
      tmp_affv[i].modifier = 0;
      tmp_affv[i].location = 0;
      tmp_affv[i].specific = 0;
      tmp_affv[i].bitvector = 0;
      tmp_affv[i].next = 0;
    }
  }

  save_char_vars(ch);


  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  while (ch->affectedv)
    affectv_remove(ch, ch->affectedv);

  if ((i >= MAX_AFFECT) && aff && aff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  /* end char_to_store code */


  if (GET_NAME(ch))				fprintf(fl, "Name: %s\n", GET_NAME(ch));
  if (GET_IRDA_NAME_1(ch)) fprintf(fl, "INa1: %s\n", GET_IRDA_NAME_1(ch));
  if (GET_IRDA_NAME_2(ch)) fprintf(fl, "INa2: %s\n", GET_IRDA_NAME_2(ch));
  if (GET_PASSWD(ch))				fprintf(fl, "Pass: %s\n", GET_PASSWD(ch));
  if (ch->desc && ch->desc->account && ch->desc->account->name) {
    fprintf(fl, "Acct: %s\n", ch->desc->account->name);
    fprintf(fl, "ActN: %s\n", ch->desc->account->name);
  }
  if (GET_TITLE(ch))				fprintf(fl, "Titl: %s\n", GET_TITLE(ch));
  if (GET_IRDA_TITLE_1(ch)) fprintf(fl, "ITi1: %s\n", GET_IRDA_TITLE_1(ch));
  if (GET_IRDA_TITLE_2(ch)) fprintf(fl, "ITi2: %s\n", GET_IRDA_TITLE_2(ch));  
  if (ch->player_specials->short_descr)				fprintf(fl, "SDsc: %s\n", ch->player_specials->short_descr);
  if (ch->player_specials->irda_short_descr_one)				fprintf(fl, "ISD1: %s\n", ch->player_specials->irda_short_descr_one);  
  if (ch->player_specials->irda_short_descr_two)				fprintf(fl, "ISD2: %s\n", ch->player_specials->irda_short_descr_two);    
  if (ch->player_specials->keywords)				fprintf(fl, "KeyW: %s\n", ch->player_specials->keywords);
  if (ch->player_specials->irda_keywords_one)				fprintf(fl, "IKw1: %s\n", ch->player_specials->irda_keywords_one);  
  if (ch->player_specials->irda_keywords_two)				fprintf(fl, "IKw2: %s\n", ch->player_specials->irda_keywords_two);    
  if (GET_LONG_DESC(ch))			fprintf(fl, "LDsc: %s\n", GET_LONG_DESC(ch));  
  if (GET_TRAINS(ch))				fprintf(fl, "Trns: %d\n", GET_TRAINS(ch));
  if ((ch)->trains_unspent)			fprintf(fl, "TrUS: %d\n", (ch)->trains_unspent);
  if (ch->player_specials->RKit)                fprintf(fl, "RKit: %d\n", ch->player_specials->RKit);
  if (ch->player_specials->description && *ch->player_specials->description) {
    strcpy(buf, ch->player_specials->description);
    kill_ems(buf);
    fprintf(fl, "Desc:\n%s~\n", buf);
  }
  if (ch->player_specials->irda_description_one && *ch->player_specials->irda_description_one) {
    strcpy(buf, ch->player_specials->irda_description_one);
    kill_ems(buf);
    fprintf(fl, "IDs1:\n%s~\n", buf);
  }  
  if (ch->player_specials->irda_description_two && *ch->player_specials->irda_description_two) {
    strcpy(buf, ch->player_specials->irda_description_two);
    kill_ems(buf);
    fprintf(fl, "IDs2:\n%s~\n", buf);
  }    
#ifdef ASCII_SAVE_POOFS
  if (POOFIN(ch))				fprintf(fl, "PfIn: %s\n", POOFIN(ch));
  if (POOFOUT(ch))				fprintf(fl, "PfOt: %s\n", POOFOUT(ch));
#endif
  if (GET_SEX(ch)	   != PFDEF_SEX)	fprintf(fl, "Sex : %d\n", GET_SEX(ch)); 
  if (ch->size		   != PFDEF_SIZE)	fprintf(fl, "Size: %d\n", ch->size); 
  if (GET_CLASS(ch)	   != PFDEF_CLASS)	fprintf(fl, "Clas: %d\n", GET_CLASS(ch)); 
  if (GET_REAL_RACE(ch)	   != PFDEF_RACE)	fprintf(fl, "Race: %d\n", GET_REAL_RACE(ch));
  if (GET_DISGUISE_RACE(ch))	                fprintf(fl, "DRac: %d\n", GET_DISGUISE_RACE(ch));
  if (GET_RAGE(ch)     != 0)        fprintf(fl, "Rage: %d\n", GET_RAGE(ch));
  if (GET_DEFENSIVE_STANCE(ch) != 0)        fprintf(fl, "DfSt: %d\n", GET_DEFENSIVE_STANCE(ch));
  if (GET_RESEARCH_TOKENS(ch) != 0) fprintf(fl, "RTok: %d\n", GET_RESEARCH_TOKENS(ch));
  if (GET_TURN_UNDEAD(ch) != 0)     fprintf(fl, "Turn: %d\n", GET_TURN_UNDEAD(ch));
	if (GET_STRENGTH_OF_HONOR(ch) != 0) fprintf(fl, "StrH: %d\n", GET_STRENGTH_OF_HONOR(ch));
  if (GET_ADMLEVEL(ch)	   != PFDEF_LEVEL)	fprintf(fl, "AdmL: %d\n", GET_ADMLEVEL(ch));
  if (GET_CLASS_LEVEL(ch)  != PFDEF_LEVEL)	fprintf(fl, "Levl: %d\n", GET_CLASS_LEVEL(ch));
  if (GET_LAY_HANDS(ch) != 0)		fprintf(fl, "LayH: %d\n", GET_LAY_HANDS(ch));
  if (GET_IRDA_SHAPE_STATUS(ch) != PFDEF_IRDA_SHAPE_STATUS) fprintf(fl, "ISSt: %d\n", GET_IRDA_SHAPE_STATUS(ch));
  if (GET_LEVEL_STAGE(ch))			fprintf(fl, "LStg: %d\n", GET_LEVEL_STAGE(ch));
  if (GET_HITDICE(ch)      != PFDEF_LEVEL)	fprintf(fl, "HitD: %d\n", GET_HITDICE(ch));
  if (GET_LEVEL_ADJ(ch)    != PFDEF_LEVEL)	fprintf(fl, "LvlA: %d\n", GET_LEVEL_ADJ(ch));
  if (GET_HOME(ch)	   != PFDEF_HOMETOWN)	fprintf(fl, "Home: %d\n", GET_HOME(ch));
  if (GET_COMPANION_VNUM(ch) != 0) fprintf(fl, "CNum: %X\n", GET_COMPANION_VNUM(ch));
  if (GET_FAMILIAR_VNUM(ch) != 0) fprintf(fl, "FNum: %d\n", GET_FAMILIAR_VNUM(ch));
  if (GET_MOUNT_VNUM(ch) != 0) fprintf(fl, "MNum: %d\n", GET_MOUNT_VNUM(ch));
  if (GET_PET_VNUM(ch) != 0) fprintf(fl, "PNum: %d\n", GET_PET_VNUM(ch));
  if (GET_PC_DESCRIPTOR_1(ch) != 0) fprintf(fl, "GSD1: %d\n", GET_PC_DESCRIPTOR_1(ch));
  if (GET_PC_DESCRIPTOR_2(ch) != 0) fprintf(fl, "GSD2: %d\n", GET_PC_DESCRIPTOR_2(ch));
  if (GET_PC_ADJECTIVE_1(ch) != 0) fprintf(fl, "GSA1: %d\n", GET_PC_ADJECTIVE_1(ch));
  if (GET_PC_ADJECTIVE_2(ch) != 0) fprintf(fl, "GSA2: %d\n", GET_PC_ADJECTIVE_2(ch));
  if (IS_APPROVED(ch) != 0) fprintf(fl, "Appr: %d\n", IS_APPROVED(ch));
  if (GET_HEAL_ROLL(ch) != PFDEF_HEAL_ROLL) fprintf(fl, "HRol: %d\n", GET_HEAL_ROLL(ch));
  if (GET_HP_BONUS(ch) != 0) fprintf(fl, "HPBn: %d\n", GET_HP_BONUS(ch));
  if (GET_HEAL_AMOUNT(ch) != PFDEF_HEAL_AMOUNT) fprintf(fl, "HAmt: %d\n", GET_HEAL_AMOUNT(ch));
  if (GET_HEAL_USED(ch) != PFDEF_HEAL_USED) fprintf(fl, "Heal: %d\n", GET_HEAL_USED(ch));
  if (GET_CAMP_USED(ch) != 0) fprintf(fl, "Camp: %d\n", GET_CAMP_USED(ch));
  if ((ch)->combat_output != 0) fprintf(fl, "COut: %d\n", (ch)->combat_output);
  if (GET_GUILD(ch) != -1) fprintf(fl, "Gild: %d\n", GET_GUILD(ch));
  if (GET_GUILD_RANK(ch) != 1) fprintf(fl, "GRnk: %d\n", GET_GUILD_RANK(ch));
  if (GET_GUILD_EXP(ch) != 0) fprintf(fl, "GExp: %d\n", GET_GUILD_EXP(ch));
  if (GET_SUBGUILD(ch) != -1) fprintf(fl, "GSub: %d\n", GET_SUBGUILD(ch));
  fprintf(fl, "FAln: %d\n", GET_FALSE_ALIGNMENT(ch));
  fprintf(fl, "FEth: %d\n", GET_FALSE_ETHOS(ch));
  if(GET_CLAN(ch) != PFDEF_CLAN) fprintf(fl, "Clan: %d\n", GET_CLAN(ch));
  if(GET_CLAN_RANK(ch) != PFDEF_CLANRANK) fprintf(fl, "Rank: %d\n", GET_CLAN_RANK(ch));
  if (GET_RP_POINTS(ch)) fprintf(fl, "RPPt: %ld\n", GET_RP_POINTS(ch));
  if (GET_RP_EXP_BONUS(ch)) fprintf(fl, "RPEx: %d\n", GET_RP_EXP_BONUS(ch));
  if (GET_RP_ART_EXP_BONUS(ch)) fprintf(fl, "RPAE: %d\n", GET_RP_ART_EXP_BONUS(ch));
  if (GET_RP_GOLD_BONUS(ch)) fprintf(fl, "RPGp: %d\n", GET_RP_GOLD_BONUS(ch));
  if (GET_RP_ACCOUNT_EXP(ch)) fprintf(fl, "RPAE: %d\n", GET_RP_ACCOUNT_EXP(ch));
  if (GET_RP_QP_BONUS(ch)) fprintf(fl, "RPQP: %d\n", GET_RP_QP_BONUS(ch));
  if (GET_RP_CRAFT_BONUS(ch)) fprintf(fl, "RPCr: %d\n", GET_RP_CRAFT_BONUS(ch));
  fprintf(fl, "Stat: %d\n", GET_STAT_POINTS(ch));
  fprintf(fl, "StGv: %d\n", (ch)->stat_points_given);

  fprintf(fl, "StMK: %d\n", GET_STAT_MOB_KILLS(ch));


  for (i = 0; i < NUM_CLASSES; i++) {
    if(GET_CLASS_RANKS(ch, i))	fprintf(fl, "Mcls: %d=%d\n", i, GET_CLASS_NONEPIC(ch, i));
    if(GET_CLASS_EPIC(ch, i))	fprintf(fl, "Ecls: %d=%d\n", i, GET_CLASS_EPIC(ch, i));
    if(GET_CLASS_FEATS(ch, i))	fprintf(fl, "FCls: %d %d\n", i, GET_CLASS_FEATS(ch, i));
    if(GET_EPIC_CLASS_FEATS(ch, i))
      fprintf(fl, "FECl: %d %d\n", i, GET_EPIC_CLASS_FEATS(ch, i));
  }
  for (i = 0; i <= CFEAT_MAX; i++) {
    sprintascii(fbuf1, ch->combat_feats[i][0]);
    sprintascii(fbuf2, ch->combat_feats[i][1]);
    sprintascii(fbuf3, ch->combat_feats[i][2]);
    sprintascii(fbuf4, ch->combat_feats[i][3]);
    fprintf(fl, "CbFt: %d %s %s %s %s\n", i, fbuf1, fbuf2, fbuf3, fbuf4);
  }
  for (i = 0; i <= SFEAT_MAX; i++) {
    sprintascii(fbuf1, ch->combat_feats[i][0]);
    fprintf(fl, "SclF: %d %s\n", i, fbuf1);
  }

  if (GET_CARRY_STR_MOD(ch) > 0) fprintf(fl, "CStr: %d\n", GET_CARRY_STR_MOD(ch));

  fprintf(fl, "Id  : %ld\n", GET_IDNUM(ch));
  fprintf(fl, "Brth: %ld\n", (long int)ch->time.birth);
  fprintf(fl, "Crtd: %ld\n", (long int)ch->time.created);
  fprintf(fl, "MxAg: %ld\n", (long int)ch->time.maxage);
  fprintf(fl, "Plyd: %ld\n", (long int)ch->time.played);
  fprintf(fl, "Last: %ld\n", (long int)ch->time.logon);

  if (GET_HOST(ch))				fprintf(fl, "Host: %s\n", GET_HOST(ch));
  if (GET_PREF(ch))				fprintf(fl, "PrfL: %ld\n", GET_PREF(ch));
  if (ch->pvp_timer)                            fprintf(fl, "PvPT: %d\n", ch->pvp_timer);
  if (GET_HEIGHT(ch)	   != PFDEF_HEIGHT)	fprintf(fl, "Hite: %d\n", GET_HEIGHT(ch));
  if (GET_WEIGHT(ch)	   != PFDEF_HEIGHT)	fprintf(fl, "Wate: %d\n", GET_WEIGHT(ch));
  if (GET_ALIGNMENT(ch)	   != PFDEF_ALIGNMENT)	fprintf(fl, "Alin: %d\n", GET_ALIGNMENT(ch));
  if (GET_ETHIC_ALIGNMENT(ch)	   != PFDEF_ETHIC_ALIGNMENT)	fprintf(fl, "Eali: %d\n", GET_ETHIC_ALIGNMENT(ch));
  fprintf(fl, "ExAE: %d\n", GET_EXTRA_ACC_EXP(ch));

  sprintascii(fbuf1, PLR_FLAGS(ch)[0]);
  sprintascii(fbuf2, PLR_FLAGS(ch)[1]);
  sprintascii(fbuf3, PLR_FLAGS(ch)[2]);
  sprintascii(fbuf4, PLR_FLAGS(ch)[3]);
  fprintf(fl, "Act : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, AFF_FLAGS(ch)[0]);
  sprintascii(fbuf2, AFF_FLAGS(ch)[1]);
  sprintascii(fbuf3, AFF_FLAGS(ch)[2]);
  sprintascii(fbuf4, AFF_FLAGS(ch)[3]);
  fprintf(fl, "Aff : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, PRF_FLAGS(ch)[0]);
  sprintascii(fbuf2, PRF_FLAGS(ch)[1]);
  sprintascii(fbuf3, PRF_FLAGS(ch)[2]);
  sprintascii(fbuf4, PRF_FLAGS(ch)[3]);
  fprintf(fl, "Pref: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, ADM_FLAGS(ch)[0]);
  sprintascii(fbuf2, ADM_FLAGS(ch)[1]);
  sprintascii(fbuf3, ADM_FLAGS(ch)[2]);
  sprintascii(fbuf4, ADM_FLAGS(ch)[3]);
  fprintf(fl, "AdmF: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);

  if (GET_SAVE_BASE(ch, 0)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB1: %d\n", GET_SAVE_BASE(ch, 0));
  if (GET_SAVE_BASE(ch, 1)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB2: %d\n", GET_SAVE_BASE(ch, 1));
  if (GET_SAVE_BASE(ch, 2)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB3: %d\n", GET_SAVE_BASE(ch, 2));
  if (GET_SAVE_MOD(ch, 0)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr1: %d\n", GET_SAVE_MOD(ch, 0));
  if (GET_SAVE_MOD(ch, 1)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr2: %d\n", GET_SAVE_MOD(ch, 1));
  if (GET_SAVE_MOD(ch, 2)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr3: %d\n", GET_SAVE_MOD(ch, 2));

  if (GET_WISH_STR(ch))				fprintf(fl, "WStr: %d\n", GET_WISH_STR(ch));
  if (GET_WISH_CON(ch))				fprintf(fl, "WCon: %d\n", GET_WISH_CON(ch));
  if (GET_WISH_DEX(ch))				fprintf(fl, "WDex: %d\n", GET_WISH_DEX(ch));
  if (GET_WISH_INT(ch))				fprintf(fl, "WInt: %d\n", GET_WISH_INT(ch));
  if (GET_WISH_WIS(ch))				fprintf(fl, "WWis: %d\n", GET_WISH_WIS(ch));
  if (GET_WISH_CHA(ch))				fprintf(fl, "WCha: %d\n", GET_WISH_CHA(ch));

  if (GET_WIMP_LEV(ch)	   != PFDEF_WIMPLEV)	fprintf(fl, "Wimp: %d\n", GET_WIMP_LEV(ch));
  if (GET_POWERATTACK(ch)  != PFDEF_POWERATT)	fprintf(fl, "PwrA: %d\n", GET_POWERATTACK(ch));
  if (GET_FREEZE_LEV(ch)   != PFDEF_FREEZELEV)	fprintf(fl, "Frez: %d\n", GET_FREEZE_LEV(ch));
  if (GET_FEAT_POINTS(ch) != PFDEF_FEAT_POINTS) fprintf(fl, "Fpnt: %d\n", GET_FEAT_POINTS(ch));
  if (GET_EPIC_FEAT_POINTS(ch) != PFDEF_FEAT_POINTS)
    fprintf(fl, "FEpc: %d\n", GET_EPIC_FEAT_POINTS(ch));
  if (GET_INVIS_LEV(ch)	   != PFDEF_INVISLEV)	fprintf(fl, "Invs: %d\n", GET_INVIS_LEV(ch));
  if (GET_TEMP_LOADROOM(ch))	                fprintf(fl, "TLRm: %d\n", GET_TEMP_LOADROOM(ch));
  if (GET_LOADROOM(ch))                         fprintf(fl, "Room: %d\n", GET_LOADROOM(ch));
  if(GET_RECALL(ch) != PFDEF_RECALLROOM)        fprintf(fl, "Recl: %d\n", GET_RECALL(ch));

  if (GET_BAD_PWS(ch)	   != PFDEF_BADPWS)	fprintf(fl, "Badp: %d\n", GET_BAD_PWS(ch));

  if (GET_RACE_PRACTICES(ch)!= PFDEF_PRACTICES)	fprintf(fl, "SkRc: %d\n", GET_RACE_PRACTICES(ch));
  if (ch->sneak_attack_feats > 0)               fprintf(fl, "SnAF: %d\n", ch->sneak_attack_feats);
  if (ch->imp_sneak_attack_feats > 0)           fprintf(fl, "ISAF: %d\n", ch->imp_sneak_attack_feats);
  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_PRACTICES(ch, i)!= PFDEF_PRACTICES)
      fprintf(fl, "SkCl: %d %d\n", i, GET_PRACTICES(ch, i));

  if (GET_COND(ch, FULL)   != PFDEF_HUNGER && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Hung: %d\n", GET_COND(ch, FULL));
  if (GET_COND(ch, THIRST) != PFDEF_THIRST && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Thir: %d\n", GET_COND(ch, THIRST));
  if (GET_COND(ch, DRUNK)  != PFDEF_DRUNK  && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Drnk: %d\n", GET_COND(ch, DRUNK));

  if (GET_HIT(ch)	   != PFDEF_HIT  || GET_MAX_HIT(ch)  != PFDEF_MAXHIT)
    fprintf(fl, "Hit : %d/%d\n", GET_HIT(ch),  GET_MAX_HIT(ch));
  if (GET_MANA(ch)	   != PFDEF_MANA || GET_MAX_MANA(ch) != PFDEF_MAXMANA)
    fprintf(fl, "Mana: %d/%d\n", GET_MANA(ch), GET_MAX_MANA(ch));
  if (GET_MOVE(ch)	   != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    fprintf(fl, "Move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));
  if (GET_KI(ch)	   != PFDEF_KI || GET_MAX_KI(ch) != PFDEF_MAXKI)
    fprintf(fl, "Ki  : %d/%d\n", GET_KI(ch), GET_MAX_KI(ch));

  if (GET_STR(ch)	   != PFDEF_STR)        fprintf(fl, "Str : %d\n", GET_STR(ch));
  if (GET_INT(ch)	   != PFDEF_INT)	fprintf(fl, "Int : %d\n", GET_INT(ch));
  if (GET_WIS(ch)	   != PFDEF_WIS)	fprintf(fl, "Wis : %d\n", GET_WIS(ch));
  if (GET_DEX(ch)	   != PFDEF_DEX)	fprintf(fl, "Dex : %d\n", GET_DEX(ch));
  if (GET_CON(ch)	   != PFDEF_CON)	fprintf(fl, "Con : %d\n", GET_CON(ch));
  if (GET_CHA(ch)	   != PFDEF_CHA)	fprintf(fl, "Cha : %d\n", GET_CHA(ch));

  if (GET_ARMOR(ch)	   != PFDEF_AC)		fprintf(fl, "Ac  : %d\n", GET_ARMOR(ch));
  if (GET_GOLD(ch)	   != PFDEF_GOLD)	fprintf(fl, "Gold: %d\n", GET_GOLD(ch));
  if (GET_GATHER_INFO(ch)  != PFDEF_GOLD)	fprintf(fl, "Gath: %d\n", GET_GATHER_INFO(ch));
  fprintf(fl, "God : %d\n", GET_DEITY(ch));
  if (GET_DOMAIN_ONE(ch) != 0)				fprintf(fl, "Dom1: %d\n", GET_DOMAIN_ONE(ch));
  if (GET_DOMAIN_TWO(ch) != 0)				fprintf(fl, "Dom2: %d\n", GET_DOMAIN_TWO(ch));  	
  if (GET_BANK_GOLD(ch)	   != PFDEF_BANK)	fprintf(fl, "Bank: %d\n", GET_BANK_GOLD(ch));
  if (GET_EXP(ch)	   != PFDEF_EXP)	fprintf(fl, "Exp : %d\n", GET_EXP(ch));
  if (GET_ARTISAN_EXP(ch)  > 0) 	        fprintf(fl, "ArXp: %12.0f\n", GET_ARTISAN_EXP(ch));
  if (GET_ARTISAN_TYPE(ch)  > 0) 	        fprintf(fl, "ArTy: %d\n", GET_ARTISAN_TYPE(ch));
  if (GET_ACCURACY_MOD(ch) != PFDEF_ACCURACY)	fprintf(fl, "Acc : %d\n", GET_ACCURACY_MOD(ch));
  if (GET_ACCURACY_BASE(ch)!= PFDEF_ACCURACY)	fprintf(fl, "AccB: %d\n", GET_ACCURACY_BASE(ch));
  if (GET_DAMAGE_MOD(ch)   != PFDEF_DAMAGE)	fprintf(fl, "Damg: %d\n", GET_DAMAGE_MOD(ch));
  if (SPEAKING(ch)	   != PFDEF_SPEAKING)	fprintf(fl, "Spek: %d\n", SPEAKING(ch));
  fprintf(fl, "Olc : %d\n", GET_OLC_ZONE(ch));
  if (GET_PAGE_LENGTH(ch)  != PFDEF_PAGELENGTH)	fprintf(fl, "Page: %d\n", GET_PAGE_LENGTH(ch));
  if (GET_SCREEN_WIDTH(ch) != PFDEF_SCREENWIDTH) fprintf(fl, "ScrW: %d\n", GET_SCREEN_WIDTH(ch));
  if (GET_QUEST_COUNTER(ch)!= PFDEF_QUESTCOUNT)  fprintf(fl, "Qcnt: %d\n", GET_QUEST_COUNTER(ch));
  if (GET_NUM_QUESTS(ch)   != PFDEF_COMPQUESTS) {
    fprintf(fl, "Qest:\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
      fprintf(fl, "%d\n", ch->player_specials->completed_quests[i]);
    fprintf(fl, "%d\n", NOTHING);
  }
  if (GET_WISHES(ch)	   != 0)		fprintf(fl, "Wish: %d\n", GET_WISHES(ch));
  if (GET_QUEST(ch)        != PFDEF_CURRQUEST)  fprintf(fl, "Qcur: %d\n", GET_QUEST(ch));
  if (GET_QUESTPOINTS(ch)  != PFDEF_QUESTPOINTS) fprintf(fl, "Qpnt: %d\n", GET_QUESTPOINTS(ch));

  if (GET_INTROS_GIVEN(ch)    != 0)  fprintf(fl, "IGiv: %d\n", GET_INTROS_GIVEN(ch));
  if (GET_INTROS_RECEIVED(ch) != 0)  fprintf(fl, "IRec: %d\n", GET_INTROS_RECEIVED(ch));
                                     fprintf(fl, "Bled: %d\n", GET_FIGHT_BLEEDING_DAMAGE(ch));
  if (ch->player_specials->bonus_levels_arcane) fprintf(fl, "BLvA: %d\n", ch->player_specials->bonus_levels_arcane);
  if (ch->player_specials->bonus_levels_divine) fprintf(fl, "BLvD: %d\n", ch->player_specials->bonus_levels_divine);
  if (ch->player_specials->num_of_rooms_visited != 0) fprintf(fl, "RVNm: %d\n", ch->player_specials->num_of_rooms_visited);
  if (GET_LFG_STRING(ch) != NULL) fprintf(fl, "LFG : %s\n", GET_LFG_STRING(ch));
  if (GET_EPIC_SPELLS(ch)) fprintf(fl, "ESpl: %d\n", GET_EPIC_SPELLS(ch));
  if (GET_BARD_SONGS(ch)) fprintf(fl, "BSng: %d\n", GET_BARD_SONGS(ch));
  if (ch->boot_time) fprintf(fl, "Boot: %d\n", (int)ch->boot_time);
  if (ch->bounty_gem) fprintf(fl, "Boun: %d\n", (int)ch->bounty_gem);
  if (ch->damage_reduction_feats)		fprintf(fl, "EFDR: %d\n", ch->damage_reduction_feats);
  if (ch->fast_healing_feats)			fprintf(fl, "EFDR: %d\n", ch->fast_healing_feats);
  if (ch->armor_skin_feats)			fprintf(fl, "EFDR: %d\n", ch->armor_skin_feats);
  if (GET_AUTOQUEST_VNUM(ch)) 		fprintf(fl, "AQVN: %d\n", GET_AUTOQUEST_VNUM(ch));
  if (GET_AUTOQUEST_KILLNUM(ch))	fprintf(fl, "AQKN: %d\n", GET_AUTOQUEST_KILLNUM(ch));
  if (GET_AUTOQUEST_QP(ch)) 		fprintf(fl, "AQQP: %d\n", GET_AUTOQUEST_QP(ch));
  if (GET_AUTOQUEST_EXP(ch)) 		fprintf(fl, "AQEX: %d\n", GET_AUTOQUEST_EXP(ch));
  if (GET_AUTOQUEST_GOLD(ch)) 		fprintf(fl, "AQGP: %d\n", GET_AUTOQUEST_GOLD(ch));
  if (GET_AUTOQUEST_DESC(ch)) 		fprintf(fl, "AQDS: %s\n", GET_AUTOQUEST_DESC(ch));
  if (GET_AUTOCQUEST_VNUM(ch)) 		fprintf(fl, "ACVN: %d\n", GET_AUTOCQUEST_VNUM(ch));
  if (GET_AUTOCQUEST_MAKENUM(ch))	fprintf(fl, "ACKN: %d\n", GET_AUTOCQUEST_MAKENUM(ch));
  if (GET_AUTOCQUEST_QP(ch)) 		fprintf(fl, "ACQP: %d\n", GET_AUTOCQUEST_QP(ch));
  if (GET_AUTOCQUEST_EXP(ch)) 		fprintf(fl, "ACEX: %d\n", GET_AUTOCQUEST_EXP(ch));
  if (GET_AUTOCQUEST_GOLD(ch)) 		fprintf(fl, "ACGP: %d\n", GET_AUTOCQUEST_GOLD(ch));
  if (GET_AUTOCQUEST_MATERIAL(ch))	fprintf(fl, "ACMT: %d\n", GET_AUTOCQUEST_MATERIAL(ch));
  if (GET_AUTOCQUEST_DESC(ch)) 		fprintf(fl, "ACDS: %s\n", GET_AUTOCQUEST_DESC(ch));
  if (GET_BARD_SPELLS(ch, 0)) fprintf(fl, "BSp0: %d\n", GET_BARD_SPELLS(ch, 0));
  if (GET_BARD_SPELLS(ch, 1)) fprintf(fl, "BSp1: %d\n", GET_BARD_SPELLS(ch, 1));
  if (GET_BARD_SPELLS(ch, 2)) fprintf(fl, "BSp2: %d\n", GET_BARD_SPELLS(ch, 2));
  if (GET_BARD_SPELLS(ch, 3)) fprintf(fl, "BSp3: %d\n", GET_BARD_SPELLS(ch, 3));
  if (GET_BARD_SPELLS(ch, 4)) fprintf(fl, "BSp4: %d\n", GET_BARD_SPELLS(ch, 4));
  if (GET_BARD_SPELLS(ch, 5)) fprintf(fl, "BSp5: %d\n", GET_BARD_SPELLS(ch, 5));
  if (GET_BARD_SPELLS(ch, 6)) fprintf(fl, "BSp6: %d\n", GET_BARD_SPELLS(ch, 6));
  if (GET_FAVORED_SOUL_SPELLS(ch, 0)) fprintf(fl, "FSp0: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 0));
  if (GET_FAVORED_SOUL_SPELLS(ch, 1)) fprintf(fl, "FSp1: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 1));
  if (GET_FAVORED_SOUL_SPELLS(ch, 2)) fprintf(fl, "FSp2: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 2));
  if (GET_FAVORED_SOUL_SPELLS(ch, 3)) fprintf(fl, "FSp3: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 3));
  if (GET_FAVORED_SOUL_SPELLS(ch, 4)) fprintf(fl, "FSp4: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 4));
  if (GET_FAVORED_SOUL_SPELLS(ch, 5)) fprintf(fl, "FSp5: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 5));
  if (GET_FAVORED_SOUL_SPELLS(ch, 6)) fprintf(fl, "FSp6: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 6));
  if (GET_FAVORED_SOUL_SPELLS(ch, 7)) fprintf(fl, "FSp7: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 7));
  if (GET_FAVORED_SOUL_SPELLS(ch, 8)) fprintf(fl, "FSp8: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 8));
  if (GET_FAVORED_SOUL_SPELLS(ch, 9)) fprintf(fl, "FSp9: %d\n", GET_FAVORED_SOUL_SPELLS(ch, 9));


  
  fprintf(fl, "WLst:\n");
  for (i = 0; i < 10; i++) {
    fprintf(fl, "%d %d\n", ch->player_specials->wishlist[i][0], ch->player_specials->wishlist[i][1]);
  }
  fprintf(fl, "-1 -1\n");

  fprintf(fl, "SpKn:\n");
  for (i = 0; i < MAX_NUM_KNOWN_SPELLS; i++) {
    fprintf(fl, "%d\n", ch->player_specials->spells_known[i]);
  }
  fprintf(fl, "-2\n");

  fprintf(fl, "BLev:\n");
  for (i = 0; i < NUM_CLASSES; i++)
    if (ch->player_specials->bonus_levels[i] > 0)
      fprintf(fl, "%d %d\n", i, ch->player_specials->bonus_levels[i]);
  fprintf(fl, "-1 -1\n");


  /* Save skills */
  if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "Skil:\n");
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
     if (GET_SKILL_BASE(ch, i))
	fprintf(fl, "%d %d\n", i, GET_SKILL_BASE(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  // Save Innate Abils
  fprintf(fl, "InAb:\n");
  for (i = 0; i <= (MAX_SPELLS+100); i++) {
    if (GET_INNATE(ch, i) > 0) {
      fprintf(fl, "%d %d\n", i, GET_INNATE(ch, i));
    }
  }
  fprintf(fl, "-1 -1\n");

  // Save Skill Foci
  fprintf(fl, "SklF:\n");
  for (i = SKILL_LOW_SKILL; i <= SKILL_HIGH_SKILL; i++) {
    for (j = 0; j < ch->player_specials->skill_focus[i-SKILL_LOW_SKILL]; j++)
      fprintf(fl, "%d\n", i);
  }
  fprintf(fl, "0\n");

  /* Save skill bonuses */
  if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "SklB:\n");
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
     if (GET_SKILL_BONUS(ch, i))
	fprintf(fl, "%d %d\n", i, GET_SKILL_BONUS(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  /* Save feats */
  fprintf(fl, "Feat:\n");
  for (i = 1; i <= NUM_FEATS_DEFINED; i++) {
    if (HAS_FEAT(ch, i))
      fprintf(fl, "%d %d\n", i, HAS_FEAT(ch, i));
  }
  fprintf(fl, "0 0\n");

  /* Save Sponsors in Classes */
  fprintf(fl, "CSpn:\n");
  for (i = 0; i < NUM_CLASSES; i++) {
  	if (GET_CLASS_SPONSOR(ch, i) == TRUE) {
  		fprintf(fl, "%d\n", i);
  	}
  }
  fprintf(fl, "-1\n");

  /* Save affects */
  fprintf(fl, "Affs:\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    aff = &tmp_aff[i];
    if (aff->type)
      fprintf(fl, "%d %d %d %d %d %d %d\n", aff->type, aff->duration,
      aff->modifier, aff->location, (int)aff->bitvector, aff->specific, aff->level);
  }
  fprintf(fl, "0 0 0 0 0 0\n");
  fprintf(fl, "Affv:\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    aff = &tmp_affv[i];
    if (aff->type)
      fprintf(fl, "%d %d %d %d %d %d %d\n", aff->type, aff->duration,
        aff->modifier, aff->location, (int)aff->bitvector, aff->specific, aff->level);
  }
  fprintf(fl, "0 0 0 0 0 0\n");

  /* wizard memorize */
  fprintf(fl, "Mmem:\n");
  for (i = 0; i < GET_MEMCURSOR(ch); i++) {
    if (GET_SPELLMEM(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM(ch, i), 0);
  }
  for (mem = ch->memorized; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");

  /* cleric memorize */
  fprintf(fl, "Cmem:\n");
  for (i = 0; i < GET_MEMCURSOR_C(ch); i++) {
    if (GET_SPELLMEM_C(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM_C(ch, i), 0);
  }
  for (mem = ch->memorized_c; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");


  /* paladin memorize */
  fprintf(fl, "Pmem:\n");
  for (i = 0; i < GET_MEMCURSOR_P(ch); i++) {
    if (GET_SPELLMEM_P(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM_P(ch, i), 0);
  }
  for (mem = ch->memorized_p; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");

  /* druid memorize */
  fprintf(fl, "Dmem:\n");
  for (i = 0; i < GET_MEMCURSOR_D(ch); i++) {
    if (GET_SPELLMEM_D(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM_D(ch, i), 0);
  }
  for (mem = ch->player_specials->memorized_d; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");

  /* ranger memorize */
  fprintf(fl, "Rmem:\n");
  for (i = 0; i < GET_MEMCURSOR_R(ch); i++) {
    if (GET_SPELLMEM_R(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM_R(ch, i), 0);
  }
  for (mem = ch->player_specials->memorized_r; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");

  /* paladin memorize */
  fprintf(fl, "Bmem:\n");
  for (i = 0; i < GET_MEMCURSOR_B(ch); i++) {
    if (GET_SPELLMEM_B(ch, i) > 0)
      fprintf(fl, "%d %d\n", GET_SPELLMEM_B(ch, i), 0);
  }
  for (mem = ch->player_specials->memorized_b; mem; mem = next) {
    next = mem->next;
    fprintf(fl, "%d %d\n", mem->spell, mem->timer);
  }
  fprintf(fl, "-1 0\n");

  /* Pet Variables for the New System */
	if (ch->sum_name != NULL) fprintf(fl, "SumN:\n%s~\n", ch->sum_name);
	if (ch->sum_desc != NULL) fprintf(fl, "SumD:\n%s~\n", ch->sum_desc);
  fprintf(fl, "SumT: %i\n", ch->summon_type);

  if (ch->player_specials->summon_num > 0) {
    fprintf(fl, "SmNm: %d\n", ch->player_specials->summon_num);  
    fprintf(fl, "SmDs: %s\n", ch->player_specials->summon_desc);  
    fprintf(fl, "SmCH: %d\n", ch->player_specials->summon_cur_hit);  
    fprintf(fl, "SmMH: %d\n", ch->player_specials->summon_max_hit);  
    fprintf(fl, "SmAC: %d\n", ch->player_specials->summon_ac);  
    fprintf(fl, "SmDR: %d\n", ch->player_specials->summon_dr);  
    fprintf(fl, "SmTm: %d\n", ch->player_specials->summon_timer);  
    fprintf(fl, "SmAt:\n");  
    for (i = 0; i < 5; i++)
      fprintf(fl, "%d %d %d %d\n", ch->player_specials->summon_attack_to_hit[i], ch->player_specials->summon_attack_ndice[i], 
              ch->player_specials->summon_attack_sdice[i], ch->player_specials->summon_attack_dammod[i]);  
    fprintf(fl, "-1 -1 -1 -1\n");
  }

  fprintf(fl, "Ment: %d\n", ch->mentor_level);

  fprintf(fl, "Mntd: %d\n", ch->player_specials->mounted);  
  fprintf(fl, "Mnt : %d\n", ch->player_specials->mount);  

  if (ch->player_specials->mount_num > 0) {
    fprintf(fl, "MtNm: %d\n", ch->player_specials->mount_num);  
    fprintf(fl, "MtDs: %s\n", ch->player_specials->mount_desc);  
    fprintf(fl, "MtCH: %d\n", ch->player_specials->mount_cur_hit);  
    fprintf(fl, "MtMH: %d\n", ch->player_specials->mount_max_hit);  
    fprintf(fl, "MtAC: %d\n", ch->player_specials->mount_ac);  
    fprintf(fl, "MtDR: %d\n", ch->player_specials->mount_dr);  
    fprintf(fl, "MtAt:\n");  
    for (i = 0; i < 5; i++)
      fprintf(fl, "%d %d %d %d\n", ch->player_specials->mount_attack_to_hit[i], ch->player_specials->mount_attack_ndice[i], 
              ch->player_specials->mount_attack_sdice[i], ch->player_specials->mount_attack_dammod[i]);  
    fprintf(fl, "-1 -1 -1 -1\n");
  }
  fprintf(fl, "Inna:\n");
  for (inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    fprintf(fl, "%d %d\n", inn->spellnum, inn->timer);
  }
  fprintf(fl, "-1 0\n");

  fprintf(fl, "LevD:\n");
  write_level_data(ch, fl);

  for (i = 0; i < NUM_COLOR; i++)
    if (ch->player_specials->color_choices[i]) {
      fprintf(fl, "Colr: %d %s\r\n", i, ch->player_specials->color_choices[i]);
    }

  if (ch->damreduct)
    for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
      fprintf(fl, "DmRd:\n%hd %hd %d %d %d\n", reduct->spell, reduct->feat, reduct->mod, reduct->duration, reduct->max_damage);
      for (i = 0; i < MAX_DAMREDUCT_MULTI; i++)
        if (reduct->damstyle[i])
          fprintf(fl, "%d %d\n", reduct->damstyle[i], reduct->damstyleval[i]);
      fprintf(fl, "end\n");
    }

  fprintf(fl, "Intr:\n");
  for (i = 0; i < MAX_INTROS; i++)
    fprintf(fl, "%d\n", ch->player_specials->intro_list[i][0]);
  fprintf(fl, "-1\n");

  // Save character to account if necessary

  if (ch->desc && ch->desc->account) {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
      if (ch->desc->account->character_names[i] != NULL && 
          !strcmp(ch->desc->account->character_names[i], GET_NAME(ch)))
        break;
      if (ch->desc->account->character_names[i] == NULL)
        break;
    }
  
    if (i != MAX_CHARS_PER_ACCOUNT && !IS_SET_AR(PLR_FLAGS(ch), PLR_DELETED))
      ch->desc->account->character_names[i] = strdup(GET_NAME(ch));
    save_account(ch->desc->account);
  }


  fprintf(fl, "RVis:\n");
  for (i = 0; i < 65555; i++)
    if (ch->player_specials->rooms_visited[i] > 0) 
      fprintf(fl, "%d %d\n", i, ch->player_specials->rooms_visited[i]);
  fprintf(fl, "-1 -1\n");

  fclose(fl);

  if (rename(fname, bakname) != 0 && errno != ENOENT) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't backup player file %s, '%s'", tmpname, strerror(errno));
    send_to_char(ch, "There was an error while saving. Copy this line and send to an imm. %s 0x01\r\n", GET_NAME(ch));
    return;
  }

  if (rename(tmpname, fname) != 0) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't move temporary player file %s, '%s'", tmpname, strerror(errno));
    send_to_char(ch, "There was an error while saving. Copy this line and send to an imm. %s 0x02\r\n", GET_NAME(ch));
    return;
  }

  /* more char_to_store code to restore affects */

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].type)
      affect_to_char(ch, &tmp_aff[i]);
  }
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_affv[i].type)
      affectv_to_char(ch, &tmp_affv[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
#ifndef NO_EXTRANEOUS_TRIGGERS
        if (wear_otrigger(char_eq[i], ch, i))
#endif
      equip_char(ch, char_eq[i], i);
#ifndef NO_EXTRANEOUS_TRIGGERS
          else
          obj_to_char(char_eq[i], ch);
#endif
  }

  /* end char_to_store code */
 
  if ((id = get_ptable_by_name(GET_NAME(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != GET_LEVEL(ch)) {
    save_index = TRUE;
    player_table[id].level = GET_LEVEL(ch);
  }
  if (player_table[id].admlevel != GET_ADMLEVEL(ch)) {
    save_index = TRUE;
    player_table[id].admlevel = GET_ADMLEVEL(ch);
  }
  if (player_table[id].last != ch->time.logon) {
    save_index = TRUE;
    player_table[id].last = ch->time.logon;
  }
  i = player_table[id].flags;
  if (PLR_FLAGGED(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (PLR_FLAGGED(ch, PLR_FROZEN) || PLR_FLAGGED(ch, PLR_NOWIZLIST))
    SET_BIT(player_table[id].flags, PINDEX_NOWIZLIST);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NOWIZLIST);

  if (player_table[id].flags != i || save_index)
    save_player_index();

}



void save_etext(struct char_data *ch)
{
/* this will be really cool soon */
}


/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i = 0;

  for (i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  
  while (*tmp == ':' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}


void load_affects(FILE *fl, struct char_data *ch, int violence)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0, num7 = 0, i = 0, retval = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};
  struct affected_type af;

  i = 0;
  do {
    get_line(fl, line);
    num = num2 = num3 = num4 = num5 = num6 = 0;
    if ((retval = sscanf(line, "%d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6, &num7) == 6)) {
      if (num != 0) {
        af.type = num;
        af.duration = num2;
        af.modifier = num3;
        af.location = num4;
        af.bitvector = num5;
        af.specific = num6;
    
        if (violence)
          affect_to_char(ch, &af);
        else
          affect_to_char(ch, &af);
        i++;
      }
    }
    else {
      if (num != 0) {
        af.type = num;
        af.duration = num2;
        af.modifier = num3;
        af.location = num4;
        af.bitvector = num5;
        af.specific = num6;
        af.level = num7;
    
        if (violence)
          affect_to_char(ch, &af);
        else
          affect_to_char(ch, &af);
        i++;
      }
    }
  } while (num != 0);
}


void load_skills(FILE *fl, struct char_data *ch, bool mods)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};

  do {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0) {
      if (mods)
        SET_SKILL_BONUS(ch, num, num2);
      else
        SET_SKILL(ch, num, num2);
    }
  } while (num != 0);
}

void load_feats(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};

  do {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
      if (num != 0)
	SET_FEAT(ch, num, num2);
  } while (num != 0);
}

void load_skill_focus(FILE *fl, struct char_data *ch)
{
  int num = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};

  do {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != 0)
      ch->player_specials->skill_focus[num-SKILL_LOW_SKILL] += 1;
  } while (num != 0);
}

void load_intros(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};

  do {
    get_line(fl, line);
    sscanf(line, "%d", &num);
      if (num != -1)
	ch->player_specials->intro_list[i][0] = num;
    i++;
  } while (num != -1);
}

void load_HMVS(struct char_data *ch, const char *line, int mode)
{
  int num = 0, num2 = 0;

  sscanf(line, "%d/%d", &num, &num2);

  switch (mode) {
  case LOAD_HIT:
    GET_HIT(ch) = num;
    GET_MAX_HIT(ch) = num2;
    break;

  case LOAD_MANA:
    GET_MANA(ch) = num;
    GET_MAX_MANA(ch) = num2;
    break;

  case LOAD_MOVE:
    GET_MOVE(ch) = num;
    GET_MAX_MOVE(ch) = num2;
    break;

  case LOAD_KI:
    GET_KI(ch) = num;
    GET_MAX_KI(ch) = num2;
    break;
  }
}


/*************************************************************************
*  stuff related to the player file cleanup system			 *
*************************************************************************/


/*
 * remove_player() removes all files associated with a player who is
 * self-deleted, deleted by an immortal, or deleted by the auto-wipe
 * system (if enabled).
 */ 
void remove_player(int pfilepos)
{
  char fname[40]={'\0'};
  int i = 0;
  struct tm *info;
  char buffer[80]={'\0'};

  time(&player_table[pfilepos].last);
  info = localtime(&player_table[pfilepos].last);
  strftime(buffer, 80, "%b-%d-%Y", info);

  if (!*player_table[pfilepos].name)
    return;

  /* Unlink all player-owned files */
  for (i = 0; i < MAX_FILES; i++) {
    if (get_filename(fname, sizeof(fname), i, player_table[pfilepos].name))
      unlink(fname);
  }

  log("PCLEAN: %s Lev: %d Last: %s", player_table[pfilepos].name, player_table[pfilepos].level, buffer);
  player_table[pfilepos].name[0] = '\0';
  save_player_index();
}


void clean_pfiles(void)
{
  int i, ci;

  for (i = 0; i <= top_of_p_table; i++) {
    /*
     * We only want to go further if the player isn't protected
     * from deletion and hasn't already been deleted.
     */
    if (!IS_SET(player_table[i].flags, PINDEX_NODELETE) &&
        *player_table[i].name) {
      /*
       * If the player is already flagged for deletion, then go
       * ahead and get rid of him.
       */
      if (IS_SET(player_table[i].flags, PINDEX_DELETED)) {
	remove_player(i);
      } else {
        /*
         * Now we check to see if the player has overstayed his
         * welcome based on level.
         */
	for (ci = 0; pclean_criteria[ci].level != (ubyte)-1; ci++) {
	  if (player_table[i].level <= pclean_criteria[ci].level &&
	      ((time(0) - player_table[i].last) >
	       (pclean_criteria[ci].days * SECS_PER_REAL_DAY))) {
	    remove_player(i);
	    break;
	  }
	}
	/*
         * If we got this far and the players hasn't been kicked out,
         * then he can stay a little while longer.
         */
      }
    }
  }
  /*
   * After everything is done, we should rebuild player_index and
   * remove the entries of the players that were just deleted.
   */
}

void load_quests(FILE *fl, struct char_data *ch)
{
  int num = NOTHING;
  char line[MAX_INPUT_LENGTH + 1]={'\0'};

  do {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != NOTHING)
      add_completed_quest(ch, num);
 } while (num != NOTHING);
}

void init_respec_char(struct char_data *ch)
{
  int i = 0;

    /* character initializations */
    /* initializations necessary to keep some things straight */
    ch->affected = NULL;
    ch->affectedv = NULL;
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
      SET_SKILL(ch, i, 0);
      SET_SKILL_BONUS(ch, i, 0);
    }
    ch->size = PFDEF_SIZE;
    GET_CRAFTING_TYPE(ch) = PFDEF_CRAFTING_TYPE;
    GET_CLASS(ch) = CLASS_ARTISAN;
    for (i = 0; i < MAX_NUM_KNOWN_SPELLS; i++) {
      ch->player_specials->spells_known[i] = 0;
    }
    for (i = 0; i < 10; i++)
      ch->player_specials->spell_slots[i] = 0;

    for (i = 0; i < NUM_CLASSES; i++)
      GET_CLASS_SPONSOR(ch, i) = FALSE;
    GET_CLASS(ch) = PFDEF_CLASS;
    for (i = 0; i < NUM_CLASSES; i++) {
      GET_CLASS_NONEPIC(ch, i) = 0;
      GET_CLASS_EPIC(ch, i) = 0;
    }
    for (i = 0; i < 10; i++) {
    	ch->player_specials->wishlist[i][0] = 0;
		ch->player_specials->wishlist[i][1] = 0;
    }
    GET_HEAL_ROLL(ch) = PFDEF_HEAL_ROLL;
    GET_HEAL_AMOUNT(ch) = PFDEF_HEAL_AMOUNT;
    GET_HEAL_USED(ch) = PFDEF_HEAL_USED;
    GET_CAMP_USED(ch) = 0;
    GET_RESEARCH_TOKENS(ch) = 0;
    GET_DISGUISE_RACE(ch) = PFDEF_RACE;
    GET_CARRY_STR_MOD(ch) = 0;
    GET_CLASS_LEVEL(ch) = PFDEF_LEVEL;
    GET_HITDICE(ch) = PFDEF_LEVEL;
    GET_HOME(ch) = PFDEF_HOMETOWN;
    for (i = 0; i < NUM_OF_SAVE_THROWS; i++) {
      GET_SAVE_MOD(ch, i) = PFDEF_SAVETHROW;
      GET_SAVE_BASE(ch, i) = PFDEF_SAVETHROW;
    }

    GET_ARMOR(ch) = 0;
    GET_TURN_UNDEAD(ch) = 0;
    GET_STRENGTH_OF_HONOR(ch) = 0;
    GET_DEFENSIVE_STANCE(ch) = 0;
    GET_SMITE_EVIL(ch) = 0;
    GET_MOUNT_VNUM(ch) = 0;
    GET_PET_VNUM(ch) = 0;
    GET_FAMILIAR_VNUM(ch) = 0;
    GET_COMPANION_VNUM(ch) = 0;
    GET_COMPANION_NAME(ch) = NULL;
    GET_EXPERTISE_BONUS(ch) = 0;
    GET_ENMITY(ch) = 0;
    GET_TOTAL_AOO(ch) = 0;
    GET_LOADROOM(ch) = PFDEF_LOADROOM;
    GET_SPELLCASTER_LEVEL(ch) = 0;
    ch->stat_points_given = 0;
    GET_ACCURACY_MOD(ch) = PFDEF_ACCURACY;
    GET_ACCURACY_BASE(ch) = PFDEF_ACCURACY;
    GET_DAMAGE_MOD(ch) = PFDEF_DAMAGE;
    GET_LAY_HANDS(ch) = 0;
    GET_RECALL(ch) = PFDEF_RECALLROOM;
    GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
    GET_POWERATTACK(ch) = PFDEF_POWERATT;
    GET_RACE_PRACTICES(ch) = PFDEF_PRACTICES;
    for (i = 0; i < NUM_CLASSES; i++)
      GET_PRACTICES(ch, i) = PFDEF_PRACTICES;
	GET_IRDA_SHAPE_STATUS(ch) = PFDEF_IRDA_SHAPE_STATUS;
    GET_MANA(ch) = PFDEF_MANA;
    GET_MAX_MANA(ch) = PFDEF_MAXMANA;
    GET_MOVE(ch) = PFDEF_MOVE;
    GET_MAX_MOVE(ch) = PFDEF_MAXMOVE;
    GET_KI(ch) = PFDEF_KI;
    GET_MAX_KI(ch) = PFDEF_MAXKI;
    GET_EPIC_SPELLS(ch) = 0;
    GET_BARD_SONGS(ch) = 0;
    ch->mentor_level = 0;
    for (i = 0; i < 7; i++)
      GET_BARD_SPELLS(ch, i) = 0;
    for (i = 0; i < 7; i++)
      ch->player_specials->bard_spells_to_learn[i] = 0;
    for (i = 0; i < 10; i++)
      GET_FAVORED_SOUL_SPELLS(ch, i) = 0;
    GET_FIGHT_BLEEDING_DAMAGE(ch) = 0;
    GET_FIGHT_PRECISE_ATTACK(ch) = 0;
    GET_FIGHT_DAMAGE_REDUCTION(ch) = 0;
    GET_FIGHT_MESSAGE_PRINTED(ch) = 0;
    GET_FIGHT_SNEAK_ATTACK(ch) = 0;
    GET_FIGHT_CRITICAL_HIT(ch) = 0;
    GET_FIGHT_DEATH_ATTACK(ch) = 0;
    GET_FIGHT_DAMAGE_DONE(ch) = 0;
    GET_FIGHT_NUMBER_OF_ATTACKS(ch) = 0;
    GET_FIGHT_NUMBER_OF_HITS(ch) = 0;
    GUARDING(ch) = NULL;
    GUARDED_BY(ch) = NULL;
    ch->exp_chain = 0;
    ch->fight_over = 0;
    ch->player_specials->epic_dodge = FALSE;
    GET_LFG_STRING(ch) = NULL;
    GET_HP_BONUS(ch) = 0;
    ch->spell_cast = FALSE;

    ch->damage_reduction_feats = 0;
    ch->armor_skin_feats = 0;
    ch->fast_healing_feats = 0;

    GET_WISH_STR(ch) = 0;
    GET_WISH_CON(ch) = 0;
    GET_WISH_DEX(ch) = 0;
    GET_WISH_INT(ch) = 0;
    GET_WISH_WIS(ch) = 0;
    GET_WISH_CHA(ch) = 0;

    GET_STAT_MOB_KILLS(ch) = 0;

    for (i = 0; i < (700); i++)
      GET_INNATE(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM_C(ch, i) = 0;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM_P(ch, i) = 0;
    for(i = 0; i < MAX_SPELL_LEVEL; i++)
      GET_SPELL_LEVEL(ch, i) = 0;
    for (i = SKILL_LOW_SKILL; i <= SKILL_HIGH_SKILL; i++)
      ch->player_specials->skill_focus[i-SKILL_LOW_SKILL] = 0;
    GET_MEMCURSOR(ch) = 0;
    GET_MEMCURSOR_C(ch) = 0;
    GET_MEMCURSOR_P(ch) = 0;
    ch->damreduct = NULL;

    GET_MARK(ch) = NULL;
    GET_MARK_ROUNDS(ch) = 0;
    GET_DEATH_ATTACK(ch) = 0;
    GET_FALSE_ETHOS(ch) = 0;
    GET_FALSE_ALIGNMENT(ch) = 0;
    GET_GUILD(ch) = -1;
    GET_SUBGUILD(ch) = -1;
    GET_GUILD_RANK(ch) = 1;
    GET_GUILD_EXP(ch) = 0;
    for (i = 0; i < NUM_RULES; i++)
    ch->player_specials->rules_read[i] = FALSE;
    ch->sum_name = NULL; //"Invalid";
    ch->sum_desc = NULL; //"Invalid";
    ch->summon_type = -1;
    ch->player_specials->companion_num = 0;
    ch->trains_spent = 0;
    ch->trains_unspent = 0;
    for (i = 0; i < NUM_CLASSES; i++)
      ch->player_specials->bonus_levels[i] = 0;
    ch->player_specials->bonus_levels_arcane = 0;
    ch->player_specials->bonus_levels_divine = 0;
    for (i = 0; i < (MAX_FEATS+1); i++)
      ch->feats[i] = 0;
     int j = 0;
    for (i = 0; i < (CFEAT_MAX+1); i++) {
      for (j=0; j < FT_ARRAY_MAX; j++) {
        ch->combat_feats[i][j] = 0;
      }
    }
    for (i = 0; i < (SFEAT_MAX+1); i++)
      ch->school_feats[i] = 0;
}
