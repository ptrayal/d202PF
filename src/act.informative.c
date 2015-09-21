/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include "feats.h"
#include "player_guilds.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "boards.h"
#include "deities.h"
#include "clan.h"
#include "protocol.h"
#include "modules.h"

#include "version.h"

/* extern variables */
extern MYSQL *conn;
extern int top_of_helpt;
const char *admin_level_names[ADMLVL_IMPL + 4];
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;
extern struct deity_info deity_list[NUM_DEITIES];
extern const char *size_names[];
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *guild_names[];
extern char *class_abbrevs_core[];
extern char *class_abbrevs_dl_aol[];
extern char *race_abbrevs[];
extern const char *material_names[];
extern int show_mob_stacking;
extern int show_obj_stacking;
extern const char *deity_names_fr[];
extern const char *deity_names_dl_aol[];
extern char *pc_race_types[NUM_RACES];

/* extern functions */
int get_average_damage(struct char_data *ch, struct obj_data *wielded);
int get_speed(struct char_data *ch);
void award_expendable_item(struct char_data *ch, int grade, int type);
void award_magic_weapon(struct char_data *ch, int grade, int moblevel);
int determine_random_weapon_type(void);
void award_magic_armor(struct char_data *ch, int grade, int moblevel);
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel);
SPECIAL(questmaster);
bool can_see_map(struct char_data *ch);
void str_and_map(char *str, struct char_data *ch );
ACMD(do_action);
ACMD(do_insult);
int level_exp(int level, int race);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch, struct char_data *att);
struct obj_data *find_vehicle_by_vnum(int vnum);
extern struct obj_data *get_obj_in_list_type(int type, struct obj_data *list);
void view_room_by_rnum(struct char_data * ch, int is_in);
int check_disabled(const struct command_info *command);
char *get_blank_clan_whostring(int clan);
char *current_short_desc(struct char_data *ch);
char *which_desc(struct char_data *ch);
int has_intro(struct char_data *ch, struct char_data *target);
int compute_base_hit(struct char_data *ch, int weaponmod);
char * change_coins(int coins);
int get_spell_resistance(struct char_data *ch);

int get_dam_dice_size(struct char_data *ch, struct obj_data *wielded, int return_mode);
int get_damage_mod(struct char_data *ch, struct obj_data *wielded);

/* local functions */
void FormatNumberOutput(char * String);
char *get_pc_alignment(struct char_data *ch, char *alignment);
int sort_commands_helper(const void *a, const void *b);
void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data *ch, char *arg);
void perform_immort_where(struct char_data *ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void look_at_char_old(struct char_data *i, struct char_data *ch);
void list_one_char(struct char_data *i, struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
ACMD(do_exits);
void look_in_direction(struct char_data *ch, int dir);
void look_in_obj(struct char_data *ch, char *arg);
void look_out_window(struct char_data *ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data *list);
void look_at_target(struct char_data *ch, char *arg, int read);
void search_in_direction(struct char_data * ch, int dir);
ACMD(do_autoexit);
void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode);
void display_spells(struct char_data *ch, struct obj_data *obj);
void display_scroll(struct char_data *ch, struct obj_data *obj);
ACMD(do_scan);
char * attribute_text(int att, char *buf);
char * defense_text(int def, char *buf);
char * offense_text(int off, char *buf);
char * saving_throw_text(int save, char *buf);
char * attribute_text_desc(int att, char *buf);
char * defense_text_desc(int def, char *buf);
char * offense_text_desc(int off, char *buf);
char * saving_throw_text_desc(int save, char *buf);
ACMD(do_autocon);

/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
const char *portal_appearance[] = 
{
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"
};
#define MAX_PORTAL_TYPES        6

/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2


struct help_index_element *find_help(char *keyword)
{
  extern int top_of_helpt;
  int i = 0;
  char new_keyword[MAX_INPUT_LENGTH]={'\0'};

  for (i = 0; i < top_of_helpt; i++)
    if (isname(keyword, help_table[i].keywords))
      return (help_table + i);
  
  for (i = 0; i < strlen(keyword); i++) {
    keyword[i] = toupper(keyword[i]);
    if (i != 0 && keyword[i] == ' ')
      keyword[i] = '-';
  }

  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "class-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "race-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "feat-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "class-ability-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "class-abilities-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "skill-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  sprintf(new_keyword, "spell-%s", keyword);
  for (i = 0; i < top_of_helpt; i++)
    if (is_abbrev(new_keyword, help_table[i].keywords))
      return (help_table + i);

  return NULL;
}


void display_spells(struct char_data *ch, struct obj_data *obj)
{
  int i = 0, j = 0;
  int titleDone = FALSE;

  send_to_char(ch, "The spellbook contains the following spells:\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");
  if (!obj->sbinfo)
    return;
  for (j = 0; j <= 9; j++) 
  {
    titleDone = FALSE;
    for (i=0; i < SPELLBOOK_SIZE; i++) 
    {
      if (obj->sbinfo[i].spellname != 0 && spell_info[obj->sbinfo[i].spellname].class_level[CLASS_WIZARD] == j) 
      {
        if (!titleDone) 
        {
        send_to_char(ch, "\r\n@WSpell Level %d:@n\r\n", j);
        titleDone = TRUE;
        }
        send_to_char(ch, "\t[U10132/*]%-20s		[%2d]\r\n", 
obj->sbinfo[i].spellname <= MAX_SPELLS ? spell_info[obj->sbinfo[i].spellname].name : "Error: Contact Admin"  ,
obj->sbinfo[i].pages ? obj->sbinfo[i].pages : 0);
      }
    }
  }
  return;
}

void display_scroll(struct char_data *ch, struct obj_data *obj)
{
  send_to_char(ch, "The scroll contains the following spell:\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");
  send_to_char(ch, "%-20s\r\n", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
  return;
}

void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    /*  SYSERR_DESC:
     *  Somehow a NULL pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was NULL
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
    return;
  }

  char amount[200]={'\0'};
  char buf2[200]={'\0'};
  char bitbuf[200]={'\0'};
  int found = FALSE;
  int i = 0;

  switch (mode) {
  case SHOW_OBJ_LONG:
    /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
    if (*obj->description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) 
      return;

    send_to_char(ch, "@[2]");
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    send_to_char(ch, "%s@n", obj->description);
    break;

  case SHOW_OBJ_SHORT:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    sprintf(amount, " @GPrice: @Y(%d)@n", GET_OBJ_VAL(obj, 14));

    send_to_char(ch, "%s%s%s", obj->short_description, (GET_OBJ_VAL(obj, 13) > 0) ? " @G(bound)@n" : "", GET_OBJ_VAL(obj, 14) > 0 ? amount : "");
    
    if (GET_OBJ_VAL(obj, 14) > 0) {
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0)) {
        if (!found) {
          found = TRUE;
          send_to_char(ch, "\r\n--- Affects: ");
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
        send_to_char(ch, " %s%s %s%d", bitbuf, buf2,
                     (obj->affected[i].modifier > 0) ? "+" : "", obj->affected[i].modifier);
      }
    }
    }
    break;

  case SHOW_OBJ_ACTION:
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH + 64]={'\0'};

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, true);
      } else
	send_to_char(ch, "It's blank.\r\n");
      return;

    case ITEM_BOARD:
      show_board(GET_OBJ_VNUM(obj),ch);
      break;
      
    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.\r\n");
      break;

    case ITEM_SPELLBOOK:
      send_to_char(ch, "It looks like an arcane tome.\r\n");
      display_spells(ch, obj);
      break;

    case ITEM_SCROLL:
      send_to_char(ch, "It looks like an arcane scroll.\r\n");
      display_scroll(ch, obj);
      break;

    default:
      send_to_char(ch, "You see nothing special..\r\n");
      break;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
      send_to_char(ch, "The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
      send_to_char(ch, "You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
    }
    diag_obj_to_char(obj, ch);
    send_to_char(ch, "It appears to be made of %s and its size is %s.\r\n",
    material_names[GET_OBJ_MATERIAL(obj)], size_names[obj->size]);
    break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
    return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
  send_to_char(ch, "\r\n");
}


int show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  int found = false;

  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
    send_to_char(ch, " ..It has a soft glowing aura!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_HUM)) {
    send_to_char(ch, " ..It emits a faint humming sound!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, " ..It appears to be broken.");
    found++;
  }
  return(found);
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
  struct obj_data *i, *j;
  bool found = false;
  int num;

  for (i = list; i; i = i->next_content) {
    if (i->description == NULL)
      continue;
    if (str_cmp(i->description, "undefined") == 0)
      continue;
    num = 0;
    if (CONFIG_STACK_OBJS) {
      for (j = list; j != i; j = j->next_content)
      {
        if (!j->short_description)
          log("SYSERR:  Null short description for item %d in room %d", GET_OBJ_VNUM(j), world[ch->in_room].number);
        if (!j->short_description)
          log("SYSERR:  Null short description for item %d in room %d", GET_OBJ_VNUM(i), world[ch->in_room].number);
        if (j->short_description && i->short_description &&
            !str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number) && GET_OBJ_VAL(j, 14) == GET_OBJ_VAL(i, 14))
          break;
      }
      if (j!=i)
        continue;
      for (j = i; j; j = j->next_content)
      {
        if (!j->short_description)
          log("SYSERR:  Null short description for item %d in room %d", GET_OBJ_VNUM(j), world[ch->in_room].number);
        if (!j->short_description)
          log("SYSERR:  Null short description for item %d in room %d", GET_OBJ_VNUM(i), world[ch->in_room].number);
        if (j->short_description && i->short_description &&
            !str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number) && GET_OBJ_VAL(j, 14) == GET_OBJ_VAL(i, 14))
          num++;
      }
    }
    if ((CAN_SEE_OBJ(ch, i) && ((*i->description != '.' && *i->short_description != '.' )|| PRF_FLAGGED(ch, PRF_HOLYLIGHT))) || (GET_OBJ_TYPE(i) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
      show_obj_to_char(i, ch, mode);
      found = true;
    }
  } /* Loop through all objects in the list */
  if (!found && show)
  send_to_char(ch, " Nothing.\r\n");
}

void diag_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."                 },
    {  90, "has a few scuffs."                          },
    {  75, "has some small scuffs and scratches."       },
    {  50, "has quite a few scratches."                 },
    {  30, "has some big nasty scrapes and scratches."  },
    {  15, "looks pretty damaged."                      },
    {   0, "is in awful condition."                     },
    {  -1, "is in need of repair."                      },
  };
  int percent, ar_index;
  const char *objs = OBJS(obj, ch);


  if ((GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT) && GET_OBJ_VAL(obj, VAL_ARMOR_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_ARMOR_HEALTH)) / GET_OBJ_VAL(obj, VAL_ARMOR_MAXHEALTH);
  else if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_VAL(obj, VAL_WEAPON_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_WEAPON_HEALTH)) / GET_OBJ_VAL(obj, VAL_WEAPON_MAXHEALTH);
  else if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0;               /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", UPPER(*objs), objs + 1, diagnosis[ar_index].text);
}

void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."			},
    {  90, "has a few scratches."			},
    {  75, "has some small wounds and bruises."		},
    {  50, "has quite a few wounds."			},
    {  30, "has some big nasty wounds and scratches."	},
    {  15, "looks pretty hurt."				},
    {   0, "is in awful condition."			},
    {  -1, "is bleeding awfully from big wounds."	},
  };
  int percent, ar_index;
  const char *pers = PERS(i, ch);

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = 0;		/* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "%c%s %s\r\n", UPPER(*pers), pers + 1, diagnosis[ar_index].text);
}


void look_at_char(struct char_data *i, struct char_data *ch)
{
  char buf[200]={'\0'};

  if (IS_NPC(i)) {
    look_at_char_old(i, ch);
    return;
  }
/*
  char query[400];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  sbyte found = FALSE;

  sprintf(query, "SELECT background_story, personality, background_length, personality_length FROM player_extra_data WHERE name = '%s' AND "
          "background_story != '' AND personality != '' AND background_length != '0' AND personality_length != '0'", CAP(arg));
  mysql_query(conn, query);
  res2 = mysql_use_result(conn);
  if (res2 != NULL) {
    if ((row2 = mysql_fetch_row(res2)) != NULL) {
        found = TRUE;
    }
  }
*/

  // Display title or desc depending on if they have the person's intro
  if (has_intro(ch, i))
    send_to_char(ch, "%s\r\n", GET_TITLE(i));
  else {
    char *tmpdesc;
    send_to_char(ch, "%s\r\n", tmpdesc = which_desc(i));
    free(tmpdesc);
  }

  if (i->player_specials->description)
    send_to_char(ch, "\r\n%s\r\n\r\n", i->player_specials->description);
  else
     act("\r\nYou see nothing special about $m.\r\n", false, i, 0, ch, TO_VICT);


  // display gender and race
  sprintf(buf, "$e appears to be a %s %s.", MAFE(i), RACE(i));
  act(buf, false, i, 0, ch, TO_VICT);

  // if has intro display profession
  if (has_intro(ch, i))
    send_to_char(ch, "Profession: %s\r\n", class_desc_str(i, 2, 0));

  // show religion
  if (has_intro(ch, i))
    send_to_char(ch, "%s is a follower of @Y%s@n.\r\n", GET_NAME(i), deity_list[GET_DEITY(i)].name);
                 
  // show alignment
  /* Is subguild ever set to GUILD_OPERATIVES? /Malar */
  if (has_intro(ch, i))
    send_to_char(ch, "%s's alignment is @Y%s@n.\r\n", GET_NAME(i),
                 (GET_SUBGUILD(i) == GUILD_OPERATIVES && i != ch) ?
                 alignments[FALSE_ALIGN_TYPE(i)] : alignments[ALIGN_TYPE(i)]);

  // show what is in each hand
  sprintf(buf, "$s primary hand holds @C%s@n.", GET_EQ(i, WEAR_WIELD1) ? GET_EQ(i, WEAR_WIELD1)->short_description : "nothing");
  act(buf, false, i, 0, ch, TO_VICT);
  if (GET_EQ(i, WEAR_WIELD2))
    sprintf(buf, "$s secondary hand holds @C%s@n.", GET_EQ(i, WEAR_WIELD2)->short_description);
  else if (GET_EQ(i, WEAR_SHIELD))
    sprintf(buf, "$s secondary hand holds @C%s@n.", GET_EQ(i, WEAR_SHIELD)->short_description);
  else
    sprintf(buf, "$s secondary hand holds @Cnothing@n.");
  act(buf, false, i, 0, ch, TO_VICT);


  //show armor
  char *tmpdesc = NULL;
  send_to_char(ch, "%s's armor consists of:\r\n", has_intro(ch, i) ? GET_NAME(i) : (tmpdesc = which_desc(i)));
  if (GET_EQ(i, WEAR_HEAD))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_HEAD)->short_description);
  if (GET_EQ(i, WEAR_ABOUT))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_ABOUT)->short_description);
  if (GET_EQ(i, WEAR_BODY))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_BODY)->short_description);
  if (GET_EQ(i, WEAR_HANDS))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_HANDS)->short_description);
  if (GET_EQ(i, WEAR_WAIST_1))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_WAIST_1)->short_description);
  if (GET_EQ(i, WEAR_FEET))
    send_to_char(ch, "  @G%s@n\r\n", GET_EQ(i, WEAR_FEET)->short_description);
  if (!GET_EQ(i, WEAR_HEAD) && !GET_EQ(i, WEAR_ABOUT) && !GET_EQ(i, WEAR_BODY) && !GET_EQ(i, WEAR_HANDS) &&
      !GET_EQ(i, WEAR_WAIST_1) && !GET_EQ(i, WEAR_FEET))
    send_to_char(ch, "  @Gnothing@n\r\n");

  //show jewelry
  send_to_char(ch, "%s's jewelry consists of:\r\n", has_intro(ch, i) ? GET_NAME(i) : tmpdesc);
	free(tmpdesc);
  if (GET_EQ(i, WEAR_NECK_1))
    send_to_char(ch, "  @M%s@n\r\n", GET_EQ(i, WEAR_NECK_1)->short_description);
  if (GET_EQ(i, WEAR_FINGER_R))
    send_to_char(ch, "  @M%s@n\r\n", GET_EQ(i, WEAR_FINGER_R)->short_description);
  if (GET_EQ(i, WEAR_FINGER_L))
    send_to_char(ch, "  @M%s@n\r\n", GET_EQ(i, WEAR_FINGER_L)->short_description);
  if (GET_EQ(i, WEAR_WRIST_R))
    send_to_char(ch, "  @M%s@n\r\n", GET_EQ(i, WEAR_WRIST_R)->short_description);
  if (GET_EQ(i, WEAR_WRIST_L))
    send_to_char(ch, "  @M%s@n\r\n", GET_EQ(i, WEAR_WRIST_L)->short_description);
  if (!GET_EQ(i, WEAR_NECK_1) && !GET_EQ(i, WEAR_FINGER_R) && !GET_EQ(i, WEAR_FINGER_L) && !GET_EQ(i, WEAR_WRIST_R) &&
      !GET_EQ(i, WEAR_WRIST_L))
    send_to_char(ch, "  @Mnothing\r\n@n");

  // display power relation
  if (has_intro(ch, i)) {
    if (GET_CLASS_LEVEL(ch) > GET_CLASS_LEVEL(i))
      act("You are $s better.", false, i, 0, ch, TO_VICT);
    if (GET_CLASS_LEVEL(ch) < GET_CLASS_LEVEL(i))
      act("$e is your better.", false, i, 0, ch, TO_VICT);
    if (GET_CLASS_LEVEL(ch) == GET_CLASS_LEVEL(i))
      act("$e is your equal.", false, i, 0, ch, TO_VICT);
  }
  
  send_to_char(ch, "\r\n");

}

void look_at_char_old(struct char_data *i, struct char_data *ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (!ch->desc)
    return;

    if (IS_NPC(i)) {
      if (i->description)
        send_to_char(ch, "%s\r\n", i->description);
      else
        act("You see nothing special about $m.", false, i, 0, ch, TO_VICT);
    }
    else {
      if (i->player_specials->description)
        send_to_char(ch, "%s\r\n", i->player_specials->description);
      else
         act("You see nothing special about $m.", false, i, 0, ch, TO_VICT);
    }

	char *tmpdesc = NULL;
	char *tmpstr = has_intro(ch, i) ? GET_NAME(i) + 1 : (tmpdesc = which_desc(i));
  if (GET_SEX(i) == SEX_NEUTRAL) 
    send_to_char(ch, "%c%s appears to be %s %s.\r\n", UPPER(*tmpstr),
								tmpstr + 1, AN(RACE(i)), RACE(i));
  else
    send_to_char(ch, "%c%s appears to be %s %s %s.\r\n", UPPER(*tmpstr),
								tmpstr + 1, AN(MAFE(i)), MAFE(i), RACE(i));
	free(tmpdesc);

  diag_char_to_char(i, ch);

  found = false;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = true;

  if (found) {
    send_to_char(ch, "\r\n");	/* act() does capitalization. */
    act("$n is using:", false, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(ch, "%25s ", wear_where[j]);
	show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && (IS_ROGUE(ch) || GET_ADMLEVEL(ch))) {
    found = false;
    act("\r\nYou attempt to peek at $s inventory:", false, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) &&
          (ADM_FLAGGED(ch, ADM_SEEINV) || (rand_number(0, 20) < GET_LEVEL(ch)))) {
	show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
	found = true;
      }
    }

    if (!found)
      send_to_char(ch, "You can't see anything.\r\n");
  }
}

ACMD(do_autocon)
{
  if (PRF_FLAGGED(ch, PRF_AUTOCON)) {
	  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
		send_to_char(ch, "Auto-consider has been turned off.\r\n");
		return;
	}
	else {
	  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
		send_to_char(ch, "Auto-consider has been turned on.\r\n");
		return;
	}
}

void list_one_char(struct char_data *i, struct char_data *ch)
{
  char *color = "@W";
  char buf[200]={'\0'};
  int lvl_diff = 0;
	int j = 0;
	
  const char *positions[] = 
  {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i)) 
     send_to_char(ch, "[%d] %s", GET_MOB_VNUM(i), SCRIPT(i) ? "[TRIG] " : "");
  
  if ((IS_NPC(i) && i->long_descr && GET_POS(i) == GET_DEFAULT_POS(i))) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");
			
		if (PRF_FLAGGED(ch, PRF_AUTOCON)) {
		  switch (lvl_diff = (GET_LEVEL(i) - GET_LEVEL(ch) - race_list[GET_RACE(ch)].level_adjustment)) {
			  case 0:
				  color = "@W";
					break;
			  case 1:
				  color = "@m";
					break;
			  case 2:
				  color = "@M";
					break;
			  case 3:
				  color = "@r";
					break;
			  case 4:
				  color = "@R";
					break;
			  case -1:
				  color = "@y";
					break;
			  case -2:
				  color = "@Y";
					break;
				case -3:
				  color = "@g";
					break;
			  case -4:
				  color = "@G";
					break;
			  case -5:
				  color = "@c";
					break;
			  case -6:
				  color = "@C";
					break;
			  case -7:
				  color = "@b";
					break;
			  case -8:
				  color = "@B";
					break;
			  default:
				  if (lvl_diff < -6)
				    color = "@D";
					else if (lvl_diff > 4)
					  color = "@l@R";
					break;				
			}
			send_to_char(ch, "%s(@n", color);

			for (j = 0; j < MIN(5, MAX(1, GET_MAX_HIT(i) / (GET_HITDICE(i) * 10 * (1 + (GET_LEVEL(i) / 5))))); j++) {
	      if (lvl_diff > 0)		
			    send_to_char(ch, "%s+@n", color);
				else if (lvl_diff == 0)
				  send_to_char(ch, "%s=@n", color);
				else
	        send_to_char(ch, "%s-@n", color);				
			}
			send_to_char(ch, "%s) @n", color);
	  
    }

    if (IS_EVIL(i) && (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_EVIL)) && !AFF_FLAGGED(i, AFF_NO_ALIGN)
             && GET_SUBGUILD(i) != GUILD_OPERATIVES)
	    send_to_char(ch, "(@rRed@[3] Aura) ");
    else if (IS_GOOD(i) && (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_GOOD)) && !AFF_FLAGGED(i, AFF_NO_ALIGN)
             && GET_SUBGUILD(i) != GUILD_OPERATIVES)
	    send_to_char(ch, "(@bBlue@[3] Aura) ");
    else if (IS_FALSE_EVIL(i) && (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_EVIL)) && !AFF_FLAGGED(i, AFF_NO_ALIGN)
             && GET_SUBGUILD(i) == GUILD_OPERATIVES)
	    send_to_char(ch, "(@rRed@[3] Aura) ");
    else if (IS_FALSE_GOOD(i) && (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_GOOD)) && !AFF_FLAGGED(i, AFF_NO_ALIGN)
             && GET_SUBGUILD(i) == GUILD_OPERATIVES)
	    send_to_char(ch, "(@bBlue@[3] Aura) ");

    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_PVP))
	    send_to_char(ch, "@n(@GPvP@n) ");

    char *tmpdesc = NULL;
    send_to_char(ch, "%s%s", IS_NPC(i) ? (GET_AUTOQUEST_VNUM(ch) == GET_MOB_VNUM(i) ? "@M(Bounty)@n " : "") : "", IS_NPC(i) ? i->long_descr : (has_intro(ch, i) ? GET_NAME(i) : (tmpdesc = which_desc(i))));
    free(tmpdesc);

    if (!IS_NPC(i) && i->player_specials->summon_num > 0) {
      if (i->player_specials->mounted == MOUNT_SUMMON)
        sprintf(buf, "@W...$e is mounted on %s.@n", i->player_specials->summon_desc);
      else
        sprintf(buf, "@W...$e is followed by %s.@n", i->player_specials->summon_desc);
      act(buf, true, i, 0, ch, TO_VICT);
    }
    if (!IS_NPC(i) && i->player_specials->mount_num > 0) {
      if (i->player_specials->mounted == MOUNT_MOUNT)
        sprintf(buf, "@W...$e is mounted on %s.@n", i->player_specials->mount_desc);
      else
        sprintf(buf, "@W...$e is followed by %s.@n", i->player_specials->mount_desc);
      act(buf, true, i, 0, ch, TO_VICT);
    }
    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", false, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_FIRE_SHIELD))
      act("@R...$e is surrounded by a shield of raging flames!@n", false, i, 0, ch, TO_VICT);
    if (!IS_NPC(i) && HAS_FEAT(i, FEAT_WINGS)) {
      if (GET_CLASS_RANKS(i, CLASS_DRAGON_DISCIPLE) > 0)
        act("...$e has a set of draconic wings protruding from $s back!", false, i, 0, ch, TO_VICT);
      else if (IS_GOOD(i))
        act("...$e has a set of feathered wings protruding from $s back!", false, i, 0, ch, TO_VICT);
      else
        act("...$e has a set of batlike wings protruding from $s back!", false, i, 0, ch, TO_VICT);
    }
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", false, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_FAERIE_FIRE))
      act("@m...$e @mis outlined with purple fire!@m", false, i, 0, ch, TO_VICT);
    if (IS_NPC(i) && mob_index[GET_MOB_RNUM(i)].func == questmaster)
      act("@Y...$e offers a quest. (@Wtype quest list@Y for more details).@n", false, i, 0, ch, TO_VICT);

    return;
  }

  char *tmpdesc = NULL;

  if (IS_NPC(i))
    send_to_char(ch, "%c%s@[3]", UPPER(*i->short_descr), i->short_descr + 1);
  else if (has_intro(ch, i)) {
    send_to_char(ch, "@y%s, %s,", GET_NAME(i), tmpdesc = which_desc(i));
  }
  else {
		tmpdesc = which_desc(i);
    send_to_char(ch, "@y%c%s@n", UPPER(*tmpdesc), tmpdesc + 1);
  }
  free(tmpdesc);
	
  if (GET_POS(i) != POS_FIGHTING)
    send_to_char(ch, "@[3]%s", positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      send_to_char(ch, " is here, fighting ");
      if (FIGHTING(i) == ch)
	send_to_char(ch, "@rYOU!@[3]");
      else {
	if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
	  send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
	else
	  send_to_char(ch,  "someone who has already left!");
      }
    } else			/* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
  }	
	
  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    send_to_char(ch, " (invisible)");
  if (AFF_FLAGGED(i, AFF_ETHEREAL))
    send_to_char(ch, " (ethereal)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    send_to_char(ch, " (hiding)");
  if (!IS_NPC(i) && !i->desc)
    send_to_char(ch, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    send_to_char(ch, " (writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (buildwalk)");

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      send_to_char(ch, " (@rRed@[3] Aura)");
    else if (IS_GOOD(i))
      send_to_char(ch, " (@bBlue@[3] Aura)");
  }
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_PVP))
    send_to_char(ch, "@n(@GPvP@n) ");

  send_to_char(ch, "@n\r\n");

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("@[3]...$e glows with a bright light!@n", false, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_FAERIE_FIRE))
    act("@[3]..$e is outlined with purple fire!@n", false, i, 0, ch, TO_VICT);

    if (!IS_NPC(i) && i->player_specials->summon_num > 0) {
      if (i->player_specials->mounted == MOUNT_SUMMON)
        sprintf(buf, "@W...$e is mounted on %s.@n", i->player_specials->summon_desc);
      else
        sprintf(buf, "@W...$e is followed by %s.@n", i->player_specials->summon_desc);
      act(buf, true, i, 0, ch, TO_VICT);
    }
    if (!IS_NPC(i) && i->player_specials->mount_num > 0) {
      if (i->player_specials->mounted == MOUNT_MOUNT)
        sprintf(buf, "@W...$e is mounted on %s.@n", i->player_specials->mount_desc);
      else
        sprintf(buf, "@W...$e is followed by %s.@n", i->player_specials->mount_desc);
      act(buf, true, i, 0, ch, TO_VICT);
    }
    if (affected_by_spell(i, SPELL_FIRE_SHIELD))
      act("@R...$e is surrounded by a shield of raging flames!@n", false, i, 0, ch, TO_VICT);
    if (!IS_NPC(i) && HAS_FEAT(i, FEAT_WINGS)) {
      if (GET_CLASS_RANKS(i, CLASS_DRAGON_DISCIPLE) > 0)
        act("...$e has a set of draconic wings protruding from $s back!", false, i, 0, ch, TO_VICT);
      else if (IS_GOOD(i))
        act("...$e has a set of feathered wings protruding from $s back!", false, i, 0, ch, TO_VICT);
      else
        act("...$e has a set of batlike wings protruding from $s back!", false, i, 0, ch, TO_VICT);
    }
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", false, i, 0, ch, TO_VICT);
}

void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *j;
	/* I guess this is so you can't spam look to find someone but now it's just a leak
  struct hide_node {
    struct hide_node *next;
    struct char_data *hidden;
  } *hideinfo, *lasthide, *tmphide; */
  int num;
  
  /* hideinfo = lasthide = NULL; */
  
  /* for (i = list; i; i = i->next_in_room) {
    if (AFF_FLAGGED(i, AFF_HIDE) && roll_resisted(i, SKILL_STEALTH, ch, SKILL_PERCEPTION)) {
      CREATE(tmphide, struct hide_node, 1);
      tmphide->next = NULL;
      tmphide->hidden = i;
      if (!lasthide) {
        hideinfo = lasthide = tmphide;
      } else {
        lasthide->next = tmphide;
        lasthide = tmphide;
      }
      continue;
    }
  } */

  for (i = list; i; i = i->next_in_room) {
    if (ch == i) 
      continue;

    /* for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
      if (tmphide->hidden == i)
        break;
    if (tmphide)
      continue; */
    if (AFF_FLAGGED(i, AFF_HIDE) && roll_resisted(i, SKILL_STEALTH, ch, SKILL_PERCEPTION))
			continue;

    if (CAN_SEE(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        for (j = list; j != i; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) {
            /* for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              break; */
          }
     	if (j!=i)
          /* This will be true where we have already found this
	   * mob for an earlier "i".  The continue pops us out of
	   * the main "i" for loop.
	   */
          continue;
 	for (j = i; j; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) {
            /*for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide) */
              num++;
        }
      }
      /* Now show this mob's name and other stuff */
      send_to_char(ch, "@[3]");
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
      list_one_char(i, ch);
    } /* processed a character we can see */
    else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
           !(AFF_FLAGGED(i, AFF_INFRAVISION) || HAS_FEAT(i, FEAT_LOW_LIGHT_VISION)))
      send_to_char(ch, "@[3]You see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}

void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode)
{
  int door, door_found = 0, has_light_obj = false, i;

  if (exit_mode == EXIT_NORMAL) {
    /* Standard behaviour - just list the available exit directions */
    send_to_char(ch, "@c[ Exits: ");
    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (!W_EXIT(target_room, door) ||
           W_EXIT(target_room, door)->to_room == NOWHERE)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED) &&
          !CONFIG_DISP_CLOSED_DOORS)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_SECRET) && 
          EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        send_to_char(ch, "@r(%s)@c ", abbr_dirs[door]);
      else
        send_to_char(ch, "%s ", abbr_dirs[door]);
      door_found++;
    }
    send_to_char(ch, "%s]@n\r\n", door_found ? "" : "None!");
  }
  if (exit_mode == EXIT_COMPLETE) {
    send_to_char(ch, "@cObvious Exits:@n\r\n");
    if (IS_AFFECTED(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't see anything, you're blind!\r\n");
      return;
    }
    if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
      send_to_char(ch, "It is pitch black...\r\n");
      return;
    }

    /* Is the character using a working light source? */
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i))
        if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
          if (GET_OBJ_VAL(GET_EQ(ch, i),VAL_LIGHT_HOURS))
            has_light_obj = true;
    }

    if (affected_by_spell(ch, SPELL_LIGHT))
      has_light_obj = true;

    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (W_EXIT(target_room, door) &&
          W_EXIT(target_room, door)->to_room != NOWHERE) {
        /* We have a door that leads somewhere */
        if (ADM_FLAGGED(ch, ADM_SEESECRET)) {
          /* Immortals see everything */
          door_found++;
          send_to_char(ch, "%-9s - [%5d] %s.\r\n", dirs[door],
                world[W_EXIT(target_room, door)->to_room].number,
                world[W_EXIT(target_room, door)->to_room].name);
          if (IS_SET(W_EXIT(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            send_to_char(ch,"                    The %s%s is %s %s%s.\r\n",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET) ?
                    "secret " : "",
                (W_EXIT(target_room, door)->keyword &&
                 str_cmp(fname(W_EXIT(target_room, door)->keyword), "undefined")) ?
                    fname(W_EXIT(target_room, door)->keyword) : "opening",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "but unlocked",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
        }
        else { /* This is what mortal characters see */
          if (!IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED)) {
            /* And the door is open */
            door_found++;
            send_to_char(ch, "%-9s - %s\r\n", dirs[door],
                IS_DARK(W_EXIT(target_room, door)->to_room) &&
                !CAN_SEE_IN_DARK(ch) && !has_light_obj ?
                "Too dark to tell." :
                world[W_EXIT(target_room, door)->to_room].name);
          } else if (CONFIG_DISP_CLOSED_DOORS &&
              !IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)) {
              /* But we tell them the door is closed */
              door_found++;
              send_to_char(ch, "%-9s - The %s is closed.\r\n", dirs[door],
                  (W_EXIT(target_room, door)->keyword) ?
                  fname(W_EXIT(target_room,door)->keyword) : "opening" );
            }
        }
      }
    }
    if (!door_found)
    send_to_char(ch, " None.\r\n\r\n");
  }
}

/*void do_auto_exits(room_rnum target_room, struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "@c[ Exits: ");

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!W_EXIT(target_room, door) || W_EXIT(target_room, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
      continue;

    send_to_char(ch, "%s ", abbr_dirs[door]);
    slen++;
  }

  send_to_char(ch, "%s]@n\r\n", slen ? "" : "None!");
}*/


ACMD(do_exits)
{
  /* Why duplicate code? */
  do_auto_exits(IN_ROOM(ch), ch, EXIT_COMPLETE);
}

const char *exitlevels[] = {
  "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  char arg[MAX_INPUT_LENGTH]={'\0'};
  int tp = 0;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);


  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n", exitlevels[EXIT_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, false)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case EXIT_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_NORMAL:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_COMPLETE:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
  }
  send_to_char(ch, "Your @rautoexit level@n is now %s.\r\n", exitlevels[EXIT_LEV(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  struct room_data *rm = &world[IN_ROOM(ch)];
  if (!ch->desc)
    return;

  if (rm->number >= 64000 && rm->number <= 64099) {
   char sbuf1[2000]={'\0'};
   if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE) {
    if (ch->travel_type == TRAVEL_TAXI) {
      rm->name = strdup("A Horse-Drawn Carriage");
      sprintf(sbuf1, "This small carriage is pulled by two brown draft horses, led by a weathered old man with smoking a pipe and wearing a long, brown overcoat.\r\n"
                     "The inside of the carriage is big enough for about 6 people, with benches on either side, and a small table in the middle.  An oil based lantern\r\n"
                     "hangs overhead, supplying light, and windows are real glass, and able to be opened or closed, with small rain canopies keeping the weather out for the most part.\r\n"
                     "Judging by how far you've gone so far, you have about %d kilometers to go until you get to %s.\r\n",
                               ch->travel_timer * 10, speeder_locales_dl[ch->travel_locale][0]);
      rm->description = strdup(sbuf1);
    } else if (ch->travel_type == TRAVEL_SHUTTLE) {
      rm->name = strdup("A Small Gnomish Airship");
      sprintf(sbuf1,
"This small airship is of gnomish design, and fortunately for travelers like you, it's one of their rare\r\n"
"inventions that actually works, with almost no cases of crashes and subsequent death.  Faster even than\r\n"
"dragon flight (though infinitely louder), it roars through the skies on a coal-powered engine, that spews\r\n"
"huge clouds of foul-looking black smoke in its wake.  Still, it beats walking or riding most of the time.\r\n"
"The airship itself has seating for about 6 people, on wooden benches that line the railed side of the airship.\r\n"
"The view below is nothing short of amazing.  Judging by how far you've gone so far you should arrive in\r\n"
"about %d minutes an %d seconds to your destination: %s.\r\n",
                               ch->travel_timer / 60, ch->travel_timer % 60, shuttle_locales_dl[ch->travel_locale][0]);
      rm->description = strdup(sbuf1);
    } else if (ch->travel_type == TRAVEL_STARSHIP) {
      rm->name = strdup("A Small Gnomish Airship");
      rm->name = strdup(sbuf1);
      sprintf(sbuf1,
"This small airship is of gnomish design, and fortunately for travelers like you, it's one of their rare\r\n"
"inventions that actually works, with almost no cases of crashes and subsequent death.  Faster even than\r\n"
"dragon flight (though infinitely louder), it roars through the skies on a coal-powered engine, that spews\r\n"
"huge clouds of foul-looking black smoke in its wake.  Still, it beats walking or riding most of the time.\r\n"
"The airship itself has seating for about 6 people, on wooden benches that line the railed side of the airship.\r\n"
"The view below is nothing short of amazing.  Judging by how far you've gone so far you should arrive in\r\n"
"about %d minutes an d%d seconds to your destination: %s.\r\n",
                               ch->travel_timer / 60, ch->travel_timer % 60, shuttle_locales_dl[ch->travel_locale][0]);
      rm->description = strdup(sbuf1);
    }
   }
  }

  if (IS_DARK(target_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND) || affected_by_spell(ch, SPELL_BLINDNESS)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH]={'\0'};
    char buf2[MAX_STRING_LENGTH]={'\0'};

    sprintbitarray(ROOM_FLAGS(target_room), room_bits, RF_ARRAY_MAX, buf);
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    send_to_char(ch, "@[1][%5d] ", GET_ROOM_VNUM(IN_ROOM(ch)));

    send_to_char(ch, "%s%s [ %s] [ %s ]@[0]\r\n",
                     SCRIPT(rm) ? "[TRIG] " : "",
                     world[IN_ROOM(ch)].name, buf, buf2);
  } else {
    char buf2[MAX_STRING_LENGTH]={'\0'};
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    send_to_char(ch, "@[1]%s [ %s ]%s@[0]\r\n", world[target_room].name, buf2, ROOM_FLAGGED(target_room, ROOM_HOUSE) ? "@Y[Items Can be Left Here]@n" 
: "");
  }
  if (ROOM_FLAGGED(target_room, ROOM_PLAYER_SHOP))
    send_to_char(ch, "@WThis is a player shop.  Please see @YHELP PLAYER-SHOPS@W to see how you can interact with it.@n\r\n");
  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(target_room, ROOM_DEATH))
  {
    if(!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOMAP) && can_see_map(ch))
    {
      str_and_map(PRF_FLAGGED(ch, PRF_BRIEFMAP) ? "" : world[IN_ROOM(ch)].description, ch);
    }
    else if (!PRF_FLAGGED(ch, PRF_BRIEFMAP) || ignore_brief)
    {
      send_to_char(ch, "%s", world[target_room].description);
    }
  }
  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(target_room, ch, EXIT_LEV(ch));

  /* now list characters & objects */
  send_to_char(ch, "@Y--- Objects ---- @n\r\n");
  if (ROOM_FLAGGED(target_room, ROOM_PLAYER_SHOP))
    list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_SHORT, false);
  else
    list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, false);
  
  send_to_char(ch, "@Y--- Creatures/People ---- @n\r\n");
  list_char_to_char(world[target_room].people, ch);
}

void look_in_direction(struct char_data *ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else
      send_to_char(ch, "You see nothing special.\r\n");

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
           EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) )
        send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL && !bits)
      send_to_char(ch, "There's nothing inside that!\r\n");
    else if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) && !OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) { 
    if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < 0) {
      /* You can look through the portal to the destination */
      /* where does this lead to? */
      room_rnum portal_dest = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)); 
      if (portal_dest == NOWHERE) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else if (IS_DARK(portal_dest) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else {
       send_to_char(ch, "After seconds of concentration you see the image of %s.\r\n", world[portal_dest].name);
      }
    } else if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
     /* display the appropriate description from the list of descriptions
*/
      send_to_char(ch, "%s\r\n", portal_appearance[GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR)]);
    } else {
      /* We shouldn't really get here, so give a default message */
      send_to_char(ch, "All you can see is the glow of the portal.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else if (GET_OBJ_VAL(obj, VAL_VEHICLE_APPEAR) < 0) {
      /* You can look inside the vehicle */
      /* where does this lead to? */
      room_rnum vehicle_inside = real_room(GET_OBJ_VAL(obj, VAL_VEHICLE_ROOM)); 
      if (vehicle_inside == NOWHERE) {
        send_to_char(ch, "You cannot see inside that.\r\n");
      } else if (IS_DARK(vehicle_inside) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "It is pitch black...\r\n");
      } else {
        send_to_char(ch, "You look inside and see:\r\n");
        look_at_room(vehicle_inside, ch, 0);
      }
    } else {
      send_to_char(ch, "You cannot see inside that.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
    look_out_window(ch, arg);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)&&
             (GET_OBJ_TYPE(obj) != ITEM_PORTAL))     {
    send_to_char(ch, "There's nothing inside that!\r\n");
  } else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) || 
             (GET_OBJ_TYPE(obj) == ITEM_PORTAL)) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	send_to_char(ch, "It is closed.\r\n");
      else {
	send_to_char(ch, "%s", fname(obj->name));
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(ch, " (carried): \r\n");
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(ch, " (here): \r\n");
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(ch, " (used): \r\n");
	  break;
	}

	list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, true);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) <= 0)
	send_to_char(ch, "It is empty.\r\n");
      else {
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) < 0)
        {
          char buf2[MAX_STRING_LENGTH]={'\0'};
	  sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        }
       else if (GET_OBJ_VAL(obj,VAL_DRINKCON_HOWFULL)>GET_OBJ_VAL(obj,VAL_DRINKCON_CAPACITY)) {
	  send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
          char buf2[MAX_STRING_LENGTH]={'\0'};
	  amt = (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) * 3) / GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
	  sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
      }
    }
}


    
char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data *ch, char *arg, int read)
{
  int bits, found = false, j, fnum, i = 0, msg = 1, hidelooker = 0;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH]={'\0'};

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (read) {
    for (obj = ch->carrying; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	found = true;
	break;
      }
    }
    if(!obj) {
      for (obj = world[IN_ROOM(ch)].contents; obj;obj=obj->next_content) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  found = true;
	  break;
	}
      }
    }
    if (obj && found) {
      arg = one_argument(arg, number);
      if (!*number) {
	send_to_char(ch,"Read what?\r\n");
	return;
      }
      
      /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */
      
      if (isname(number, obj->name)) {
	show_board(GET_OBJ_VNUM(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
		 (strchr(number,'.'))) {
	sprintf(arg,"%s %s", number,arg);
	look_at_target(ch, arg, 0);
      } else {
	board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
      }
    }
    else
      look_at_target(ch, arg, 0);	
  } else {
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (AFF_FLAGGED(ch, AFF_HIDE))
        hidelooker = TO_HIDERESIST;
      if (CAN_SEE(found_char, ch))
				act("$n looks at you.", true, ch, 0, found_char, TO_VICT | hidelooker);
      act("$n looks at $N.", true, ch, 0, found_char, TO_NOTVICT | hidelooker);
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, false);
    return;
  } 

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
        if (isname(arg, GET_EQ(ch, j)->name)) {
          if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_WEAPON) {
            send_to_char(ch, "The weapon type of %s is a %s.\r\n", 
              GET_OBJ_SHORT(GET_EQ(ch, j)), 
              weapon_type[(int) GET_OBJ_VAL(GET_EQ(ch, j), VAL_WEAPON_SKILL)]);
          }
          if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SPELLBOOK) {
            display_spells(ch, GET_EQ(ch, j));
          }
          if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SCROLL) {
            display_scroll(ch, GET_EQ(ch, j));
          }
          diag_obj_to_char(GET_EQ(ch, j), ch);
          send_to_char(ch, "It appears to be made of %s.\r\n",
          material_names[GET_OBJ_MATERIAL(GET_EQ(ch, j))]);
        }
	found = true;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if (GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	  send_to_char(ch, "%s", desc);
          if (isname(arg, obj->name)) {
            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
              send_to_char(ch, "The weapon type of %s is a %s.\r\n", 
                GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, 
                VAL_WEAPON_SKILL)]);
            }
            if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
              display_spells(ch, obj);
            }
            if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
              display_scroll(ch, obj);
            }
            diag_obj_to_char(obj, ch);
            send_to_char(ch, "It appears to be made of %s.\r\n",
            material_names[GET_OBJ_MATERIAL(obj)]);
          }
	}
	found = true;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
          send_to_char(ch, "The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
        }
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
        material_names[GET_OBJ_MATERIAL(obj)]);
	}
	found = true;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
    else {
      if (show_obj_modifiers(found_obj, ch))
      send_to_char(ch, "\r\n");
    }
  } else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
  }
}

void look_out_window(struct char_data *ch, char *arg)
{
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  room_rnum target_room = NOWHERE;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits = generic_find(arg,
              FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
              ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
    return;
  }
  } else if (OUTSIDE(ch)) {
      /* yeah, sure stupid */
      send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      if ((GET_OBJ_TYPE(i) == ITEM_WINDOW) &&
           isname("window", i->name)) {
        viewport = i;
      continue;
      }
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
             OBJVAL_FLAGGED(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1) < 0) {
      /* We are looking out of the room */
      if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4) < 0) {
        /* Look for the default "outside" room */
        for (door = 0; door < NUM_OF_DIRS; door++)
          if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room != NOWHERE)
              if (!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
                target_room = EXIT(ch, door)->to_room;
                continue;
              }
      } else {
        target_room = real_room(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4));
      }
    } else {
      /* We are looking out of a vehicle */
      if ( (vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1))) );
        target_room = IN_ROOM(vehicle);
    }
    if (target_room == NOWHERE) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, true, ch, viewport, 0, TO_CHAR);
      else
        send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You can't see anything, you're blind!\r\n");
  else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH]={'\0'}, arg2[MAX_INPUT_LENGTH]={'\0'};

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
	send_to_char(ch, "Read what?\r\n");
      else
	look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "You need to search in a particular direction.\r\n");
      else
      look_at_room(IN_ROOM(ch), ch, 1);
    } else if (is_abbrev(arg, "inside")   && EXIT(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside")   ||
               is_abbrev(arg, "into")       )  { 
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") || 
                is_abbrev(arg, "through") ||
	        is_abbrev(arg, "thru")      ) && 
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") && 
               (subcmd == SCMD_LOOK) && !EXIT(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, false)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, false)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, false)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, false)) >= 0 )) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
      look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
      look_at_target(ch, arg2, 0);
    } else if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
    else
      look_at_target(ch, arg, 0);
  }
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH]={'\0'}, arg[MAX_INPUT_LENGTH]={'\0'};

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg),0);	/* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  char buf[MAX_STRING_LENGTH]={'\0'};

  if (GET_GOLD(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (GET_GOLD(ch) == 1)
    send_to_char(ch, "You have one miserable little gold coin.\r\n");
  else {
    sprintf(buf, "You have %s @ncoins.\r\n", change_coins(GET_GOLD(ch)));
    act(buf, false, ch, 0, 0, TO_CHAR);
  }

}


char *reduct_desc(struct char_data *victim, struct damreduct_type *reduct)
{
  static char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH]={'\0'};
  int len = 0;
  int slash = 0;
  int i = 0;
  int reduction = 0;

  struct obj_data *armor = GET_EQ(victim, WEAR_BODY);
  struct obj_data *shield = GET_EQ(victim, WEAR_SHIELD);

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

  if (reduct && reduct->mod == -1)
    len += snprintf(buf + len, sizeof(buf) - len, "FULL");
  else
    len += snprintf(buf + len, sizeof(buf) - len, "%d", reduction + (reduct ? reduct->mod : 0));
  for (i = 0; i < MAX_DAMREDUCT_MULTI; i++) {
    if (!reduct) {
      break;
    }
    switch (reduct->damstyle[i]) {
    case DR_NONE:
      continue;
    case DR_ADMIN:
      snprintf(buf2, sizeof(buf2), "%s", admin_level_names[reduct->damstyleval[i]]);
      break;
    case DR_MATERIAL:
      snprintf(buf2, sizeof(buf2), "%s", material_names[reduct->damstyleval[i]]);
      break;
    case DR_BONUS:
      snprintf(buf2, sizeof(buf2), "%+d", reduct->damstyleval[i]);
      break;
    case DR_SPELL:
      snprintf(buf2, sizeof(buf2), "%s%s", reduct->damstyleval[i] ? "spell " : "", reduct->damstyleval[i] ? spell_info[reduct->damstyleval[i]].name : "spells");
      break;
    default:
      log("reduct_desc: unknown damstyle %d", reduct->damstyle[i]);
    }
    if (slash++)
      len += snprintf(buf + len, sizeof(buf) - len, " or ");
    else
      len += snprintf(buf + len, sizeof(buf) - len, "/");
    len += snprintf(buf + len, sizeof(buf) - len, "%s", buf2);
  }
  if (!slash)
    len += snprintf(buf + len, sizeof(buf) - len, "/--");

  if (reduct->max_damage > 0)
    len += snprintf(buf + len, sizeof(buf) - len, " %d hp left", reduct->max_damage);

  return buf;
}


ACMD(do_score)
{
  extern  char *  godSelected(struct char_data *ch);
  extern  char *pc_race_types[NUM_RACES];
  char    buf[MAX_STRING_LENGTH]={'\0'};

  /* struct time_info_data playing_time; */
  if (IS_NPC(ch)) 
  {
    return;
  }


  sprintf(buf, "\r\n");


  sprintf(buf, "%s@CName: @R%s\r\n@n", buf, GET_NAME(ch));
  sprintf(buf, "%s@CRanks@W: @R%-12s \r\n@CRace@W: @R%-12s@n",buf, class_desc_str(ch, 2, 0), pc_race_types[(int)GET_RACE(ch)]);

  sprintf(buf,"%s\r\n@CMoney: @W[@R%s@W]", buf, change_coins(GET_GOLD(ch)));

  /*  PDH  2/25/99 - god selection code  */
  sprintf(buf,"%s \r\n@CAlignment: @W[@R%s %s@W]@n @W(@CGod: @R%s@W)@n",
	    buf, 
	    (GET_ETHIC(ch) == 0 && GET_ALIGN(ch) == 0) ? "True" : 
	    (GET_ETHIC(ch) < 0 ? "Chaotic" : (GET_ETHIC(ch) == 0 ? "Neutral" : "Lawful")), 
	    (GET_ALIGN(ch) < 0 ? "Evil" : (GET_ALIGN(ch) == 0 ? "Neutral" : "Good")),
            deity_list[GET_DEITY(ch)].name);


  sprintf(buf, "%s\r\n@CCurrent Hp/Max Hp: @W[@R%5d @W/ @R%5d@W]@n", buf, GET_HIT(ch), GET_MAX_HIT(ch));
  sprintf(buf, "%s\r\n@CCurrent Mv/Max Mv: @W[@R%5d @W/ @R%5d@W]@n", buf, GET_MOVE(ch), GET_MAX_MOVE(ch));

  sprintf(buf, "%s\r\n@CRP Experience Factor: @W[@R%5d@W]@n", buf, 100);

// New exp % code by Gicker
   int int_xp = 0;
   int int_percent;
   float percent = 0.0;
   float xp = 0.0;
   
   if (GET_LEVEL(ch) == 1 && race_list[GET_RACE(ch)].level_adjustment) {
     xp = ((float) GET_EXP(ch)) /
                ((float) level_exp((GET_CLASS_LEVEL(ch) + 1), GET_REAL_RACE(ch)));   
   }
   else {
     xp = (((float) GET_EXP(ch)) - ((float) level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)))) /
                (((float) level_exp((GET_CLASS_LEVEL(ch) + 1), GET_REAL_RACE(ch)) -
                (float) level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch))));
	}

   xp *= (float) 1000.0;
   percent = (int) xp % 10;
   xp /= (float) 10;
    int_xp = MAX(0, (int) xp);
    int_percent = MAX(0, MIN((int) percent, 99));

    sprintf(buf,"%s \r\n@CProgress toward next level: @W[@R%3d.%1d%%@W]@n",
            buf,int_xp,int_percent);

  sprintf(buf, "%s\r\n", buf);

  /* no need to change anything down here...yet */
  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
    {
      sprintf(buf + strlen(buf), "You are fighting %s.\r\n", PERS(FIGHTING(ch), ch));
    }
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }
  
  {
    int drunkValue  = GET_COND(ch, DRUNK);

    if (!IS_NPC(ch))
    {
      if (drunkValue > 10)
        strcat(buf, "You are very intoxicated.\r\n");
      else if (drunkValue > 8)
        strcat(buf, "You are intoxicated.\r\n");
      else if (drunkValue > 5)
        strcat(buf, "You are beginning to feel intoxicated.\r\n");
    }
  }

  send_to_char(ch, "%s", buf);
}
/* end of new_do_score */

ACMD(do_aod_new_score)
{

  int attack = 0, base_attack = 0, offhand = 0, weaponmod = 0, offhandmod = 0;
  char attack_text[MAX_STRING_LENGTH]={'\0'};
  int i = 0, j = 0;
  struct time_info_data playing_time;
  char play_time[MAX_STRING_LENGTH]={'\0'};
  int int_xp = 0;
  int int_percent = 0;
  float percent = 0.0;
  float xp = 0.0;
  char exp_percent[MAX_STRING_LENGTH]={'\0'};
  int grace = 0;
  char hp_text[MAX_STRING_LENGTH]={'\0'}, mv_text[MAX_STRING_LENGTH]={'\0'};
  char coin_text[MAX_STRING_LENGTH]={'\0'};
  char color1[10]={'\0'}, color2[10]={'\0'}, color3[10]={'\0'}, color4[10]={'\0'};
  struct damreduct_type *reduct;
  int showStats = TRUE;
  char enc_text[100]={'\0'}, stat_buf[200]={'\0'}, desc_buf[200]={'\0'};
  struct char_data *rec = ch, *vict;
  char arg[200]={'\0'};

  one_argument(argument, arg);

  if (*arg) {
    if (GET_ADMLEVEL(ch) > 0) {
      if (!(vict = get_char_vis(rec, arg, NULL, FIND_CHAR_WORLD))) {
        send_to_char(rec, "That person does not seem to be online.\r\n");
        return;
      }
    }
    else {
      if (!(vict = get_char_vis(rec, arg, NULL, FIND_CHAR_WORLD))) {
        send_to_char(rec, "That person does not seem to be here.\r\n");
        return;
      }
      if (!IS_NPC(vict) || vict->master != rec) {
        send_to_char(rec, "You do not have permission to view that person's score.\r\n");
        return;
      }
    }
    ch = vict;
  }  
  


  // Set colors
  
  sprintf(color1, "@W");
  sprintf(color2, "@w");
  sprintf(color3, "@c");
  sprintf(color4, "@y");
  
  // Determine number of attacks and their attack values
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD1) && (GET_EQ(ch, WEAR_WIELD1)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD1)) == ITEM_WEAPON)
        weaponmod = GET_EQ(ch, WEAR_WIELD1)->affected[j].modifier;
    }
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD2) && (GET_EQ(ch, WEAR_WIELD2)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
        offhandmod = GET_EQ(ch, WEAR_WIELD2)->affected[j].modifier;
    }  
  
    sprintf(attack_text, "%s", subcmd == SCMD_SCORE_NUMBERS ? color4 : "@G");
    attack = compute_base_hit(ch, weaponmod);
    base_attack = GET_ACCURACY_BASE(ch);
    offhand = compute_base_hit(ch, offhandmod);  
    for (i = 0; i < 4; i++) {
      
      if (i != 0 && base_attack > 0)
        sprintf(attack_text, "%s/", attack_text);
      if (base_attack > 0) {
        sprintf(attack_text, "%s%s%d", attack_text, (attack > 0) ? "+" : "", attack);
        if (i == 0 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON) {
          sprintf(attack_text, "%s/%s%d", attack_text, (offhand > 0) ? "+" : "", offhand);
        }
        if (i == 1 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON && HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING)) {
          offhand = compute_base_hit(ch, offhandmod) - 5;
          sprintf(attack_text, "%s/%s%d", attack_text, (offhand > 0) ? "+" : "", offhand);
        }        
        if (i == 0 && (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS) && GET_CLASS_RANKS(ch, CLASS_MONK) && !GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2))) {
          sprintf(attack_text, "%s/%s%d", attack_text, (attack > 0) ? "+" : "", attack);
        }
      }
      attack -= 5;
      base_attack -= 5;
    }

  // Determine play time
    
  playing_time = *real_time_passed((time(0) - ch->time.logon) + ch->time.played, 0);
  sprintf(play_time, "%d day%s and %d hour%s.",
     playing_time.day, playing_time.day == 1 ? "" : "s",
     playing_time.hours, playing_time.hours == 1 ? "" : "s");    
    
  // Determine exp percent to next level
    
   if (GET_LEVEL(ch) == 1 && race_list[GET_RACE(ch)].level_adjustment) {
     xp = ((float) GET_EXP(ch)) /
                ((float) level_exp((GET_CLASS_LEVEL(ch) + 1), GET_REAL_RACE(ch)));   
   }
   else {
     xp = (((float) GET_EXP(ch)) - ((float) level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)))) /
                (((float) level_exp((GET_CLASS_LEVEL(ch) + 1), GET_REAL_RACE(ch)) -
                (float) level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch))));
  }

  xp *= (float) 1000.0;
  percent = (int) xp % 10;
  xp /= (float) 10;
  int_xp = MAX(0, (int) xp);
  int_percent = MAX(0, MIN((int) percent, 99));

  if (showStats)
    sprintf(exp_percent, "%d Exp %d.%d%% tnl", GET_EXP(ch), int_xp, int_percent);
  else
    sprintf(exp_percent, "%d.%d%% exp to level", int_xp, int_percent);

  
  // Determine divine grace modifier to saving throws

  if (HAS_FEAT(ch, FEAT_DIVINE_GRACE))
    grace = MAX(0, ability_mod_value(GET_CHA(ch)));

  // Determine hit point and mv point sttrings
  sprintf(hp_text, "%d of %d HP %d%%", GET_HIT(ch), GET_MAX_HIT(ch), GET_HIT(ch) * 100 / GET_MAX_HIT(ch));
  sprintf(mv_text, "%d of %d MV %d%%", GET_MOVE(ch), GET_MAX_MOVE(ch), GET_MOVE(ch) * 100 / GET_MAX_MOVE(ch));
    
  // Determine coins string
  sprintf(coin_text, "%s", change_coins(GET_GOLD(ch)));

  sprintf(enc_text, "%lld.%d/%ld.%d (%s)",
          (IS_CARRYING_W(ch)) / 10, (int) (IS_CARRYING_W(ch)) % 10,  CAN_CARRY_W(ch) / 10, (int) CAN_CARRY_W(ch) % 10, 
          IS_LIGHT_LOAD(ch) ? "light" : (IS_MEDIUM_LOAD(ch) ? "medium" : (IS_HEAVY_LOAD(ch) ? "heavy" : "over")));



if (subcmd == SCMD_SCORE_NUMBERS) {  
send_to_char(rec, "\r\n");
send_to_char(rec, "%s================================================================================@n\r\n", color1);
send_to_char(rec, "%s== %sName: %s%-20.20s %sGender: %s%-6.6s %sSize: %s%-6.6s %sDeity: %s%-12.12s %s==@n\r\n", color1, color3, color4, GET_NAME(ch), color3, color4, genders[(int)GET_SEX(ch)], color3, color4, size_names[get_size(ch)], color3, color4, deity_list[GET_DEITY(ch)].name, color1);
send_to_char(rec, "%s== %sTitle: %s%-67.67s %s==@n\r\n", color1, color3, color4, GET_TITLE(ch), color1);
send_to_char(rec, "%s== %sLevel: %s%2d %sRace: %s%-20.20s %sAccount Exp: %s%-8d %sGift: %s%-8d  %s==@n\r\n", color1, color3, color4, GET_LEVEL(ch), color3, color4, pc_race_types[GET_RACE(ch)], color3, color4, IS_NPC(ch) ? 0 : ch->desc->account->experience, color3, color4, IS_NPC(ch) ? 0 : ch->desc->account->gift_experience, color1);
send_to_char(rec, "%s== %sClasses: %s%-66.66s%s==@n\r\n", color1, color3, color4, class_desc_str(ch, 1, 0), color1);
send_to_char(rec, "%s== %sAge: %s%2d%sy %s%2d%sm Height: %s%2d%s\'%s%2d%s\" Weight: %s%3d %slbs. Alignment: %s%-18.18s %s==@n\r\n", color1, color3, 
color4, age(ch)->year, color3, color4, age(ch)->month, color3, color4, (GET_HEIGHT(ch) / 30), color3, color4, ((GET_HEIGHT(ch) % 30) / 5 * 2), 
color3, color4, (GET_WEIGHT(ch) * 22 / 10), color3, color4, alignments[ALIGN_TYPE(ch)], color1);
send_to_char(rec, "%s================================================================================@n\r\n", color1);
send_to_char(rec, "%s== %sStrength     : %s%-15.15s %s== %sHit Points   : %s%-25.25s %s==@n\r\n", color1, color3, color4, attribute_text(GET_STR(ch), desc_buf), color1, color3, color4, hp_text, color1);
send_to_char(rec, "%s== %sConstitution : %s%-15.15s %s== %sMove Points  : %s%-25.25s %s==@n\r\n", color1, color3, color4, attribute_text(GET_CON(ch), desc_buf), color1, color3, color4, mv_text, color1);
send_to_char(rec, "%s== %sDexterity    : %s%-15.15s %s== %sBase Attack  : %s%-25.25s %s==@n\r\n", color1, color3, color4, attribute_text(GET_DEX(ch), desc_buf), color1, color3, color4, offense_text(GET_BAB(ch), desc_buf + 100), color1);
send_to_char(rec, "%s== %sIntelligence : %s%-15.15s %s== %sArmor Class  : %s%-25.25s %s==@n\r\n", color1, color3, color4, attribute_text(GET_INT(ch), desc_buf), color1, color3, color4, defense_text(compute_armor_class(ch, NULL), desc_buf + 100), color1);
send_to_char(rec, "%s== %sWisdom       : %s%-15.15s %s== %sEncumbrance  : %s%-25.25s %s==@n\r\n", color1, color3, color4, attribute_text(GET_WIS(ch), desc_buf), color1, color3, color4, enc_text, color1);
send_to_char(rec, "%s== %sCharisma     : %s%-15.15s %s== %sAttacks      : %s%-27.27s %s==@n\r\n", color1, color3, color4, attribute_text(GET_CHA(ch), desc_buf), color1, color3, color4, attack_text, color1);
send_to_char(rec, "%s== %sReflex       : %s%-15.15s %s== %sPlay Time    : %s%-25.25s %s==@n\r\n", color1, color3, color4, saving_throw_text(get_saving_throw_value(ch, SAVING_REFLEX), desc_buf), color1, color3, color4, play_time, color1);
send_to_char(rec, "%s== %sFortitude    : %s%-15.15s %s== %sMoney        : %s%-25.25s %s==@n\r\n", color1, color3, color4, saving_throw_text(get_saving_throw_value(ch, SAVING_FORTITUDE), desc_buf), color1, color3, color4, coin_text, color1);
send_to_char(rec, "%s== %sWillpower    : %s%-15.15s %s== %sExperience   : %s%-25.25s %s==@n\r\n", color1, color3, color4, saving_throw_text(get_saving_throw_value(ch, SAVING_WILL), desc_buf), color1, color3, color4, exp_percent, color1);
send_to_char(rec, "%s================================================================================@n\r\n", color1);
send_to_char(rec, "%s== %sFeats: %s%1d  %s==   %sClass Feats: %s%1d  %s==  %sSkill Points: %s%2d  %s==  %sAbility Trains: %s%1d %s==@n\r\n", color1, color3, color4, GET_FEAT_POINTS(ch), color1, color3, color4, GET_CLASS_FEATS(ch, GET_CLASS(ch)), color1, color3, color4, GET_PRACTICES(ch, GET_CLASS(ch)), color1, color3, color4, GET_TRAINS(ch), color1);
if (IS_EPIC(ch))
  send_to_char(rec, "%s== %sEpic Feats %s%1d   %sEpic Class Feats: %s%1d                                         %s==@n\r\n", color1, color3, color4, GET_EPIC_FEAT_POINTS(ch), color3, color4, GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch)), color1);
send_to_char(rec, "%s================================================================================@n\r\n", color1);
send_to_char(rec, "%s== %sQuest Points: %s[%6d] %sCompleted Quests %s[%3d]                              %s==@n\r\n", color1,  color3, color4, GET_QUESTPOINTS(ch), color3, color4, GET_NUM_QUESTS(ch),color1);
if (GET_MAX_MANA(ch) > 0)
send_to_char(rec, "%s== %sMetamagic Points: %s%3d\\%3d                                                  %s==@n\r\n", color1,  color3, color4, GET_MANA(ch), GET_MAX_MANA(ch), color1);
if (GET_RESEARCH_TOKENS(ch))
send_to_char(rec, "%s== %sResearch Tokens: %s%2d                                                        %s==@n\r\n", color1,  color3, color4, GET_RESEARCH_TOKENS(ch), color1);
if (get_spell_resistance(ch))
send_to_char(rec, "%s== %sSpell Resistance: %s%2d                                                       %s==@n\r\n", color1,  color3, color4, 
get_spell_resistance(ch), color1);
if (GET_CLASS_RANKS(ch, CLASS_BARD) > 0)
send_to_char(rec, "%s== %sBard Songs: %s%2d                                                             %s==@n\r\n", color1,  color3, color4, 
GET_BARD_SONGS(ch), color1);
  if (ch->damreduct)
    for (reduct = ch->damreduct; reduct; reduct = reduct->next)
      send_to_char(ch, "%s== %sDamage Reduction: %s%-30.30s                           %s==@n\r\n", color1, color3, color4, reduct_desc(ch, reduct), color1);
send_to_char(rec, "%s================================================================================@n\r\n", color1);
send_to_char(rec, "\r\n");
if (ch->mentor_level > 0) {
  send_to_char(rec, "@YYou are mentoring at level %d.\r\n", ch->mentor_level);
  send_to_char(rec, "\r\n");
}
}
else {
  send_to_char(rec, "\r\n");
  send_to_char(rec, "You are @Y%s@n.\r\n", GET_TITLE(ch));
  send_to_char(rec, "@Y%s@n is a level @Y%d@n %s@n\r\n", GET_NAME(ch), GET_CLASS_LEVEL(ch), class_desc_str(ch, 2, 0));
  send_to_char(rec, "You are @Y%s@n.\r\n", current_short_desc(rec));
  send_to_char(rec, "You have @Y%d@n of @Y%d@n maximum health points.\r\n", GET_HIT(ch), GET_MAX_HIT(ch));
  send_to_char(rec, "You have @Y%d@n of @Y%d@n maximum stamina points.\r\n", GET_MOVE(ch), GET_MAX_MOVE(ch));
if (GET_MAX_MANA(ch) > 0)
  send_to_char(rec, "You have @Y%d@n of @Y%d@n maximum metamagic points.\r\n", GET_MANA(ch), GET_MAX_MANA(ch));
if (GET_MAX_KI(rec) > 0 && GET_CLASS_RANKS(ch, CLASS_MONK) > 0)
  send_to_char(rec, "You have @Y%d@n of @Y%d@n maximum ki points.\r\n", GET_KI(ch), GET_MAX_KI(ch));
  send_to_char(rec, "Your alignment is @Y%s@n.\r\n", alignments[ALIGN_TYPE(ch)]);
  if (GET_SUBGUILD(ch) == GUILD_OPERATIVES)
    send_to_char(rec, "Your false alignment is @Y%s@n.\r\n", alignments[FALSE_ALIGN_TYPE(ch)]);
  if (GET_GUILD(ch) != GUILD_UNDEFINED)
    send_to_char(rec, "Your guild is @Y%s@n.\r\n", guild_names[GET_GUILD(ch)]);
  if (GET_SUBGUILD(ch) != GUILD_UNDEFINED)
    send_to_char(rec, "Your subguild is @Y%s@n.\r\n", guild_names[GET_SUBGUILD(ch)]);
  if(GET_CLAN(ch) > CLAN_NONE)
    send_to_char(rec,   "Clan : @Y%s (%s)@n\r\nClan Rank : @Y%s%s@n\r\n",
     get_blank_clan_name(GET_CLAN(ch)), get_clan_name(GET_CLAN(ch)),
     get_rank_name(GET_CLAN(ch), GET_CLAN_RANK(ch)), "@n");
  send_to_char(rec, "%s is a follower of @Y%s@n.\r\n", GET_NAME(ch), deity_list[GET_DEITY(ch)].name);
  send_to_char(rec, "Your strength is @Y%s @M%s@n.\r\n", attribute_text(GET_STR(ch), stat_buf), attribute_text_desc(GET_STR(ch), desc_buf));
  send_to_char(rec, "Your dexterity is @Y%s @M%s@n.\r\n", attribute_text(GET_DEX(ch), stat_buf), attribute_text_desc(GET_DEX(ch), desc_buf));
  send_to_char(rec, "Your constitution is @Y%s @M%s@n.\r\n", attribute_text(GET_CON(ch), stat_buf), attribute_text_desc(GET_CON(ch), desc_buf));
  send_to_char(rec, "Your intelligence is @Y%s @M%s@n.\r\n", attribute_text(GET_INT(ch), stat_buf), attribute_text_desc(GET_INT(ch), desc_buf));
  send_to_char(rec, "Your wisdom is @Y%s @M%s@n.\r\n", attribute_text(GET_WIS(ch), stat_buf), attribute_text_desc(GET_WIS(ch), desc_buf));
  send_to_char(rec, "Your charisma is @Y%s @M%s@n.\r\n", attribute_text(GET_CHA(ch), stat_buf), attribute_text_desc(GET_CHA(ch), desc_buf));
  send_to_char(rec, "Your fortitude is @Y%s @C%s@n.\r\n", 
               saving_throw_text(get_saving_throw_value(ch, SAVING_FORTITUDE), stat_buf), 
               saving_throw_text_desc(get_saving_throw_value(ch, SAVING_FORTITUDE), desc_buf));
  send_to_char(rec, "Your reflexes are @Y%s @C%s@n.\r\n", 
               saving_throw_text(get_saving_throw_value(ch, SAVING_REFLEX), stat_buf),
               saving_throw_text_desc(get_saving_throw_value(ch, SAVING_REFLEX), desc_buf));
  send_to_char(rec, "Your willpower is @Y%s @C%s@n.\r\n", 
               saving_throw_text(get_saving_throw_value(ch, SAVING_WILL), stat_buf),
               saving_throw_text_desc(get_saving_throw_value(ch, SAVING_WILL), desc_buf));
  send_to_char(rec, "Your current movement speed is @Y%d@n feet per round.\r\n", get_speed(ch));
  send_to_char(rec, "Your current base attack bonus is @Y%s @G%s@n.\r\n", 
               offense_text(GET_BAB(ch), stat_buf),
               offense_text_desc(GET_BAB(ch), desc_buf));
  send_to_char(rec, "Your current CMB is @Y%s @G%s @n and your CMD is @Y%d.%d @G%s@n.\r\n",
               offense_text(get_combat_bonus(ch), stat_buf),
               offense_text_desc(get_combat_bonus(ch), desc_buf),
               get_combat_defense(ch) / 10,  get_combat_defense(ch) % 10,
               defense_text_desc(get_combat_defense(ch), desc_buf));

  send_to_char(rec, "Your attacks with your current weapon are @G%s@n.\r\n", attack_text);
  send_to_char(rec, "Your average weapon damage (only) per hit is @G%d@n for your main hand and @G%d@n for your offhand.\r\n", 
               GET_EQ(ch, WEAR_WIELD) ? get_average_damage(ch, GET_EQ(ch, WEAR_WIELD)) : 0,
               (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON) ? get_average_damage(ch, GET_EQ(ch, WEAR_HOLD)) : 0);
  send_to_char(rec, "Your current armor class is @Y%s @G%s@n.\r\n", 
               defense_text(compute_armor_class(ch, NULL), stat_buf),
               defense_text_desc(compute_armor_class(ch, NULL), desc_buf));
  send_to_char(rec, "You have @Y%d@n quest points accumulated.\r\n", GET_QUESTPOINTS(rec));
  send_to_char(rec, "Awarded Account Experience: @Y%d@n / Gift Account Experience: @Y%d@n\r\n", 
               IS_NPC(ch) ? 0 : ch->desc->account->experience,
               IS_NPC(ch) ? 0 : ch->desc->account->gift_experience);
  send_to_char(rec, "You are @Y%d@n years and @Y%d@n months old.\r\n", age(ch)->year, age(ch)->month);
  send_to_char(rec, "You have played for @Y%d@n days, @Y%d@n hours and @Y%d@n minutes.\r\n", (int) rec->time.played / 3600 / 24, 
((int)rec->time.played / 3600) % 24 , (int)((rec->time.played % 3600) / 60));
  if (!IS_NPC(rec)) {
    send_to_char(rec, "Your character was created on @Y%s@n", asctime(localtime(&(rec->time.created))));
  }
  send_to_char(rec, "You have @Y%d@n gold coins and @Y%d@n more in the bank.\r\n", GET_GOLD(ch), GET_BANK_GOLD(ch));
  send_to_char(rec, "You have @Y%d@n experience, which is @Y%d.%d%%@n of what you need for level @Y%d@n.\r\n", GET_EXP(ch), int_xp, int_percent, GET_CLASS_LEVEL(ch) + 1);
if (ch->mentor_level > 0) {
  send_to_char(rec, "@YYou are mentoring at level %d.\r\n", ch->mentor_level);
  send_to_char(rec, "\r\n");
}
  send_to_char(rec, "  See @YSTATS@n for more detailed statistics.\r\n");
  send_to_char(rec, "\r\n");
}
}

ACMD(do_old_score)
{
  struct damreduct_type *reduct;
  int penalty, attack, base_attack, offhand;
//  char penstr[80];
  int grace = 0;
  int i = 0, j = 0;
  int offhandmod = 0, weaponmod = 0;

  if (!GET_ADMLEVEL(ch) && CONFIG_CAMPAIGN != CAMPAIGN_FORGOTTEN_REALMS)
    return;

  if (IS_NPC(ch))
    return;

  if (HAS_FEAT(ch, FEAT_DIVINE_GRACE))
    grace += ability_mod_value(GET_CHA(ch));

  send_to_char(ch, "@r=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=@n\r\n");
  send_to_char(ch, "@rName: @y%s@n\r\n", GET_TITLE(ch));
  send_to_char(ch, "@r=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=@n\r\n");
  send_to_char(ch, "@rClass: @y%s @rRace: @y%s @rLevel: @y%d@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core )[(int)GET_CLASS(ch)], pc_race_types[(int)GET_RACE(ch)], GET_LEVEL(ch));
  if (CONFIG_ALLOW_MULTICLASS && GET_LEVEL(ch) > GET_CLASS_RANKS(ch, GET_CLASS(ch))) {
    send_to_char(ch, "@rRanks: @y%s@n\r\n", class_desc_str(ch, 2, 0));
  }
  if (GET_ADMLEVEL(ch))
    send_to_char(ch, "@rAdmin Level@n: @y%d - %s@n\r\n", GET_ADMLEVEL(ch), admin_level_names[GET_ADMLEVEL(ch)]);
  send_to_char(ch, "@rAlignment@n: %s%s@n (@rE@n-@gG@n: %s%d@n, @yC-L@n: @y%d@n) @rDeity: @y%s@n\r\n", IS_EVIL(ch) ? "@r" : IS_GOOD(ch) ? "@g" : "@y", alignments[ALIGN_TYPE(ch)], IS_EVIL(ch) ? "@r" : IS_GOOD(ch) ? "@g" : "@y", GET_ALIGNMENT(ch), GET_ETHIC_ALIGNMENT(ch), deity_list[GET_DEITY(ch)].name);
  send_to_char(ch, "@rSize@n: @y%s@n @rAge@n: @y%d@n @rGender@n: @y%s@n @rHeight@n: @y%dcm@n @rWeight@n: @y%dkg@n\r\n", size_names[get_size(ch)], GET_AGE(ch), genders[(int)GET_SEX(ch)], GET_HEIGHT(ch), GET_WEIGHT(ch));
  send_to_char(ch, "@rStr@n: [@m%2d@n(@y%+d@n)] @rDex@n: [@m%2d@n(@y%+d@n)] @rHit Points@n : @m%d@n(@y%d@n)\r\n", GET_STR(ch), ability_mod_value(GET_STR(ch)), GET_DEX(ch), ability_mod_value(GET_DEX(ch)), GET_HIT(ch), GET_MAX_HIT(ch));
  send_to_char(ch, "@rCon@n: [@m%2d@n(@y%+d@n)] @rInt@n: [@m%2d@n(@y%+d@n)] @rArmor Class@n: @B%.1f@n\r\n", GET_CON(ch), ability_mod_value(GET_CON(ch)), GET_INT(ch), ability_mod_value(GET_INT(ch)), ((float)compute_armor_class(ch, NULL))/10);
  send_to_char(ch, "@rWis@n: [@m%2d@n(@y%+d@n)] @rCha@n: [@m%2d@n(@y%+d@n)] @rBase Attack Bonus@n: @m%d@n\r\n", GET_WIS(ch), ability_mod_value(GET_WIS(ch)), GET_CHA(ch), ability_mod_value(GET_CHA(ch)), GET_ACCURACY_BASE(ch));
  send_to_char(ch, "@rFortitude@n: [@m%d(@y%+d@n)]  @rReflex@n: [@m%d(@y%+d@n)]  @rWill@n: [@m%d(@y%+d@n)]  @rKi@n: @m%d@n(@y%d@n)\r\n", 
 		GET_SAVE(ch, SAVING_FORTITUDE) + GET_SAVE_MOD(ch, SAVING_FORTITUDE) + grace + ability_mod_value(GET_CON(ch)), GET_SAVE_MOD(ch, SAVING_FORTITUDE) + grace 
		+ ability_mod_value(GET_CON(ch)), GET_SAVE(ch, SAVING_REFLEX) + grace + ability_mod_value(GET_DEX(ch)), 
		GET_SAVE_MOD(ch, SAVING_REFLEX) + grace + ability_mod_value(GET_DEX(ch)), 
		GET_SAVE(ch, SAVING_WILL) + grace + ability_mod_value(GET_WIS(ch)), GET_SAVE_MOD(ch, SAVING_WILL) + grace 
		+ ability_mod_value(GET_WIS(ch)), GET_KI(ch), GET_MAX_KI(ch));
		
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD1) && (GET_EQ(ch, WEAR_WIELD1)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD1)) == ITEM_WEAPON)
        weaponmod = GET_EQ(ch, WEAR_WIELD1)->affected[j].modifier;
	  }
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD2) && (GET_EQ(ch, WEAR_WIELD2)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
        offhandmod = GET_EQ(ch, WEAR_WIELD2)->affected[j].modifier;
	  }
	
  send_to_char(ch, "@rAttacks@n: @y");
    attack = compute_base_hit(ch, weaponmod);
    base_attack = GET_ACCURACY_BASE(ch);
    offhand = compute_base_hit(ch, offhandmod);  
    for (i = 0; i < 4; i++) {
      
    	if (i != 0 && base_attack > 0)
    	  send_to_char(ch, "/");
    	if (base_attack > 0) {
    		send_to_char(ch, "%s%d", (attack > 0) ? "+" : "", attack);
        if (i == 0 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON) {
    	    send_to_char(ch, "/%s%d", (offhand > 0) ? "+" : "", offhand);
        }
        if (i == 1 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON && HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING)) {
					offhand = compute_base_hit(ch, offhandmod) - 5;
    	    send_to_char(ch, "/%s%d", (offhand > 0) ? "+" : "", offhand);
        }        
        if (i == 0 && (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS) && GET_CLASS_RANKS(ch, CLASS_MONK) && !GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2))) {
    	    send_to_char(ch, "/%s%d", (attack > 0) ? "+" : "", attack);
        }
    	}
    	attack -= 5;
    	base_attack -= 5;
    }
    send_to_char(ch, "@n\r\n");
  
		
  penalty = 100 - calc_penalty_exp(ch, 100);
  if (penalty)
    send_to_char(ch, "@rMulti-Class Exp Penalty: @n(@y%d%% penalty@n)\r\n", penalty);
  if (HAS_FEAT(ch, FEAT_RAGE))
  	send_to_char(ch, "@rRage Opportunities Remaining: @y%d@n\r\n", GET_RAGE(ch));
  if (HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR))
  	send_to_char(ch, "@rStrength of Honor Opportunities Remaining: @y%d@n\r\n", GET_STRENGTH_OF_HONOR(ch));  	
  if (HAS_FEAT(ch, FEAT_SMITE_EVIL))
  	send_to_char(ch, "@rSmite Evil Opportunities Remaining: @y%d@n\r\n", GET_SMITE_EVIL(ch));
  if (GET_RESEARCH_TOKENS(ch))
  	send_to_char(ch, "@rSpell Research Tokens: @y%d@n\r\n", GET_RESEARCH_TOKENS(ch));

  if (ch->hit_breakdown[0] || ch->hit_breakdown[1]) {
    send_to_char(ch, "@rBreakdown of your last attack@n:\r\n");
    if (ch->hit_breakdown[0] && ch->hit_breakdown[0][0] && ch->dam_breakdown[0])
      send_to_char(ch, "@rPrimary attack@n: @y%s.@n", ch->hit_breakdown[0]);
      if (ch->dam_breakdown[0])
        send_to_char(ch, "@y dam %s %s@n\r\n", ch->dam_breakdown[0],
                     ch->crit_breakdown[0] ? ch->crit_breakdown[0] : "");
      else
        send_to_char(ch, "\r\n");
      send_to_char(ch, "@rOffhand attack@n: @y%s.@n", ch->hit_breakdown[1]);
      if (ch->dam_breakdown[1])
        send_to_char(ch, "@y dam %s %s@n\r\n", ch->dam_breakdown[1],
                     ch->crit_breakdown[1] ? ch->crit_breakdown[1] : "");
      else
        send_to_char(ch, "\r\n");
  }
  if (IS_ARCANE(ch) && GET_SPELLFAIL(ch))
    send_to_char(ch, "Your armor causes %d%% failure in arcane spells with somatic components.\r\n", GET_SPELLFAIL(ch));

  if (ch->damreduct)
    for (reduct = ch->damreduct; reduct; reduct = reduct->next)
      send_to_char(ch, "@rDamage reduction@n: @g%s@n\r\n", reduct_desc(ch, reduct));

  switch (GET_POS(ch)) {
  case POS_DEAD:
    send_to_char(ch, "You are DEAD!\r\n");
  break;
  case POS_MORTALLYW:
    send_to_char(ch, "You are mortally wounded! You should seek help!\r\n");
  break;
  case POS_INCAP:
    send_to_char(ch, "You are incapacitated, slowly fading away...\r\n");
  break;
  case POS_STUNNED:
    send_to_char(ch, "You are stunned! You can't move!\r\n");
  break;
  case POS_SLEEPING:
    send_to_char(ch, "You are sleeping.\r\n");
  break;
  case POS_RESTING:
    send_to_char(ch, "You are resting.\r\n");
  break;
  case POS_SITTING:
    send_to_char(ch, "You are sitting.\r\n");
  break;
  case POS_FIGHTING:
    send_to_char(ch, "You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
  break;
  case POS_STANDING:
    send_to_char(ch, "You are standing.\r\n");
  break;
  default:
    send_to_char(ch, "You are floating.\r\n");
  break;
  }

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You are intoxicated.\r\n");

  if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    send_to_char(ch, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    send_to_char(ch, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    send_to_char(ch, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    send_to_char(ch, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
    send_to_char(ch, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    send_to_char(ch, "You can see in darkness with infravision.\r\n");
    
  if (HAS_FEAT(ch, FEAT_LOW_LIGHT_VISION))
  	send_to_char(ch, "You can see in the dark with low light vision.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    send_to_char(ch, "You are summonable by other players.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    send_to_char(ch, "You see into the hearts of others.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    send_to_char(ch, "You are sensitive to the magical nature of things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SPIRIT))
    send_to_char(ch, "You have died are are part of the SPIRIT world!\r\n");

  if (AFF_FLAGGED(ch, AFF_ETHEREAL))
    send_to_char(ch, "You are ethereal and cannot interact with normal space!\r\n");

}


ACMD(do_inventory)
{
  send_to_char(ch, "You are carrying:\r\n");
  list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, true);
}

int eq_order[NUM_WEARS] =
{
WEAR_ABOVE     ,
WEAR_LIGHT      ,
WEAR_HEAD       ,
WEAR_FACE      ,
WEAR_NOSE      ,
WEAR_EAR_R     ,
WEAR_EAR_L     ,
WEAR_NECK_1     ,
WEAR_NECK_2     ,
WEAR_BODY       ,
WEAR_SHOULDER  ,
WEAR_ABOUT     ,
WEAR_BACK_1    ,
WEAR_ONBACK_1  ,
WEAR_ONBACK_2  ,
WEAR_ONBACK_3  ,
WEAR_SHEATHED_B1 ,
WEAR_SHEATHED_B2 ,
WEAR_ARMS      ,
WEAR_WRIST_R   ,
WEAR_WRIST_L   ,
WEAR_SHEATHED_WR1,
WEAR_SHEATHED_WR2,
WEAR_HANDS      ,
WEAR_SHEATHED_H , 
WEAR_FINGER_R   ,
WEAR_FINGER_L   ,
WEAR_WAIST_1   ,
WEAR_WAIST_2   ,
WEAR_SHEATHED_WA1,
WEAR_ONBELT    ,
WEAR_LEGS       ,
WEAR_FEET       ,
WEAR_ANKLE_R   ,
WEAR_ANKLE_L   ,
WEAR_SHEATHED_A1 ,
WEAR_SHEATHED_A2 ,
WEAR_BARDING,
WEAR_SHIELD    ,
WEAR_WIELD     ,
WEAR_HOLD      ,
};

#define NUM_EQ_SHORT_EXCEPTIONS 24

int eq_short_exceptions[NUM_EQ_SHORT_EXCEPTIONS] = 
{
WEAR_NECK_2,
WEAR_LEGS,
WEAR_ARMS,
WEAR_FACE,
WEAR_EAR_R,
WEAR_EAR_L,
WEAR_ANKLE_R,
WEAR_ANKLE_L,
WEAR_SHOULDER,
WEAR_NOSE,    
WEAR_ONBELT,  
WEAR_ONBACK_1,
WEAR_ONBACK_2,
WEAR_ONBACK_3,
WEAR_SHEATHED_H,
WEAR_SHEATHED_B1,
WEAR_SHEATHED_B2,
WEAR_SHEATHED_WA1,
WEAR_SHEATHED_WA2,
WEAR_SHEATHED_A1, 
WEAR_SHEATHED_A2, 
WEAR_SHEATHED_WR1,
WEAR_SHEATHED_WR2
};

void perform_equipment_short(struct char_data *ch)
{
  int i, j, exception = FALSE;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS - 1; i++) {
    exception = FALSE;
    for (j = 0; j < NUM_EQ_SHORT_EXCEPTIONS; j++)
      if (eq_short_exceptions[j] == eq_order[i])
        exception = TRUE;  
    if (exception)
      continue;
    if (GET_EQ(ch, eq_order[i])) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, eq_order[i]))) {
        send_to_char(ch, "%-30s", wear_where[eq_order[i]]);
        if (((eq_order[i] == WEAR_WIELD1 || eq_order[i] == WEAR_WIELD2) &&
             (GET_OBJ_TYPE(GET_EQ(ch, eq_order[i])) == ITEM_WEAPON) &&
             !is_proficient_with_weapon(ch, GET_OBJ_VAL(GET_EQ(ch, eq_order[i]), VAL_WEAPON_SKILL))) ||
            (eq_order[i] == WEAR_BODY && !is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, eq_order[i]), VAL_ARMOR_SKILL))))
          send_to_char(ch, "(unskilled) ");
        show_obj_to_char(GET_EQ(ch, eq_order[i]), ch, SHOW_OBJ_SHORT);
      } else {
        send_to_char(ch, "%-30s", wear_where[eq_order[i]]);
        send_to_char(ch, "Something.\r\n");
      }
    } else {
      if (!GET_EQ(ch, eq_order[i])) {
        send_to_char(ch, "%-30s<@Dempty@n>\r\n", wear_where[eq_order[i]]);
      }
    }
  }

}


void perform_equipment_full(struct char_data *ch)
{
  int i;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS - 1; i++) {
    if (GET_EQ(ch, eq_order[i])) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, eq_order[i]))) {
        send_to_char(ch, "%-25s", wear_where[eq_order[i]]);
        if (((eq_order[i] == WEAR_WIELD1 || eq_order[i] == WEAR_WIELD2) &&
             (GET_OBJ_TYPE(GET_EQ(ch, eq_order[i])) == ITEM_WEAPON) &&
             !is_proficient_with_weapon(ch, GET_OBJ_VAL(GET_EQ(ch, eq_order[i]), VAL_WEAPON_SKILL))) ||
            (eq_order[i] == WEAR_BODY && !is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, eq_order[i]), VAL_ARMOR_SKILL))))
          send_to_char(ch, "(unskilled) ");
        show_obj_to_char(GET_EQ(ch, eq_order[i]), ch, SHOW_OBJ_SHORT);
      } else {
        send_to_char(ch, "%-25s", wear_where[eq_order[i]]);
        send_to_char(ch, "Something.\r\n");
      }
    } else {
      if (!GET_EQ(ch, eq_order[i])) {
        send_to_char(ch, "%-25s<@Dempty@n>\r\n", wear_where[eq_order[i]]);
      }
    }
  }

}

ACMD(do_equipment)
{
  char arg[100]={'\0'};
  
  one_argument(argument, arg);

  if (!*arg)
    perform_equipment_short(ch);
  else if (!strcmp(arg, "full")) {
    perform_equipment_full(ch);
  }
  else  {
    send_to_char(ch, "Proper syntax is equipment or equipment full.\r\n");
    return;
  }
}

/*
ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(ch, "%-30s", wear_where[i]);
	if (((i == WEAR_WIELD1 || i == WEAR_WIELD2) &&
             (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON) &&
             !is_proficient_with_weapon(ch, GET_OBJ_VAL(GET_EQ(ch, i), VAL_WEAPON_SKILL))) ||
	    ((GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR || GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR_SUIT)
             && !is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), 9))))
          send_to_char(ch, "(unskilled) ");
	show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);
	found = true;
      } else {
	send_to_char(ch, "%s", wear_where[i]);
	send_to_char(ch, "Something.\r\n");
	found = true;
      }
    }
  }
  if (!found)
    send_to_char(ch, " Nothing.\r\n");
}
*/

ACMD(do_time)
{
  const char *suf;
  int weekday = 0, day = 0;

  /* day in [1..30] */
  day = time_info.day + 1;

  /* 30 days in a month, 6 days a week */
  weekday = day % 6;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
	  (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
	  time_info.hours >= 12 ? "pm" : "am", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[time_info.month], time_info.year);
}


ACMD(do_weather)
{
  const char *sky_look[] = 
  {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch))
    {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due");
    if (ADM_FLAGGED(ch, ADM_KNOWWEATHER))
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                 weather_info.pressure,
                 weather_info.change,
                 weather_info.sky,
                 sky_look[weather_info.sky]);
    }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}


ACMD(do_help)
{
  struct help_index_element *this_help;
  char entry[MAX_STRING_LENGTH]={'\0'};    
  int chk = 0, bot = 0, mid = 0;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) 
  {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) 
  {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  if (!(this_help = find_help(argument))) 
  {
    log("HELP: %s tried to get help on %s", GET_NAME(ch), argument);
    send_to_char(ch, "There is no help on that word.\r\n");
    return;
  } 
  else 
  {
    if (chk > 0)
      bot = mid + 1;
  }
  if (this_help->min_level > GET_LEVEL(ch)) 
  {
    send_to_char(ch, "There is no help on that word.\r\n");
    return;  
  }

  snprintf(entry, sizeof(entry), "@W%s@n\r\n%s", this_help->keywords, this_help->entry);
  
  page_string(ch->desc, entry, 0); 

  if (!strcmp(this_help->keywords, "RULES POLICIES"))
    ch->player_specials->rules_read[0] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-VALUES"))
    ch->player_specials->rules_read[1] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-RESPECT"))
    ch->player_specials->rules_read[2] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-EVIL"))
    ch->player_specials->rules_read[3] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-LANGUAGE"))
    ch->player_specials->rules_read[4] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-BUGS"))
    ch->player_specials->rules_read[5] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-MULTIPLAYING"))
    ch->player_specials->rules_read[6] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-NAMES"))
    ch->player_specials->rules_read[7] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-BOTTING"))
    ch->player_specials->rules_read[8] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-TITLES"))
    ch->player_specials->rules_read[9] = TRUE;
  else if (!strcmp(this_help->keywords, "RULES-DISCLAIMER"))
    ch->player_specials->rules_read[10] = TRUE;

    
}

ACMD(do_nohelps) 
{

  struct help_index_element *this_help;
  int i = 0;
  char buf[100]={'\0'};

  if (!ch->desc)
    return;

  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  send_to_char(ch, "Missing Skill Help Files:\r\n");
  // Skills
  for (i = SKILL_LOW_SKILL; i <= SKILL_HIGH_SKILL; i++) {
    sprintf(buf, "skill %s", spell_info[i].name);
    if (!(this_help = find_help(buf)) && strcmp(spell_info[i].name, "!UNUSED!")) {
      send_to_char(ch, "help skill %s\r\n", spell_info[i].name);
    }
  }
  send_to_char(ch, "\r\n");

  send_to_char(ch, "Missing Spell Help Files:\r\n");
  // Spells
  for (i = 0; i <= TOP_SPELL; i++) {
    sprintf(buf, "spell %s", spell_info[i].name);
    if (!(this_help = find_help(buf)) && strcmp(spell_info[i].name, "!UNUSED!")) {
      send_to_char(ch, "help spell %s\r\n", spell_info[i].name);
    }
  }
  send_to_char(ch, "\r\n");


  send_to_char(ch, "Missing Feat Help Files:\r\n");
  // Feats
  for (i = 0; i <= NUM_FEATS_DEFINED; i++) {
    if (feat_list[i].in_game) {
      sprintf(buf, "feat %s", feat_list[i].name);
      if (!(this_help = find_help(buf)) && strcmp(feat_list[i].name, "Unused Feat")) {
        send_to_char(ch, "help feat %s\r\n", feat_list[i].name);
      }
    }
  }
  send_to_char(ch, "\r\n");

  send_to_char(ch, "Missing Race Help Files:\r\n");
  // Races
  for (i = 0; i < NUM_RACES; i++) {
    sprintf(buf, "race %s", race_list[i].name);
    if (!(this_help = find_help(buf)) && strcmp(race_list[i].name, "undefined") && race_list[i].is_pc == TRUE) {
      send_to_char(ch, "help race %s\r\n", race_list[i].name);
    }
  }
  send_to_char(ch, "\r\n");

  send_to_char(ch, "Missing Class Help Files:\r\n");
  // Classes
  for (i = 0; i <= NUM_CLASSES; i++) {
    sprintf(buf, "class %s", CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol[i] : class_names_core[i] );
    if (!(this_help = find_help(buf)) && (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_in_game_dl_aol[i]: class_in_game_core[i] ) == TRUE) {
      send_to_char(ch, "help class %s\r\n", CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol[i] : class_names_core[i]);
    }
  }
  send_to_char(ch, "\r\n");


}

#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-s] [-o] [-q] [-r] [-z]\r\n"

/* FIXME: This whole thing just needs rewritten. */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH]={'\0'}, buf[MAX_INPUT_LENGTH]={'\0'};
  char mode;
  int low = 0, high = CONFIG_LEVEL_CAP * 2, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0, showrace = 0;
  char *line_color = "@n";
  int tot_players = 0, num_staff_can_see = 0;


  skip_spaces(&argument);
  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) 
  {
    char arg[MAX_INPUT_LENGTH]={'\0'}, buf1[MAX_INPUT_LENGTH]={'\0'};

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      default:
	send_to_char(ch, "%s", WHO_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", WHO_FORMAT);
      return;
    }
  }				/* end while (parser) */

//  if (GET_ADMLEVEL(ch) >= 0)
  send_to_char(ch, "\r\nVisible Staff Members Online\r\n"
                       "----------------------------\r\n");

  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    tot_players++;

    if ((GET_ADMLEVEL(tch) >= ADMLVL_IMMORT)) {
    line_color = "@y";
    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
      continue;
    if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
     if (showrace && !(showrace & (1 << GET_RACE(tch))))
       continue;
    if (short_list && has_intro(ch, tch)) {
      send_to_char(ch, "%s%s%s", line_color, GET_NAME(tch),
	      ((!(++num_staff_can_see % 3)) ? "\r\n" : ""));
    } else if (1) {
      num_staff_can_see++;

//      if (GET_ADMLEVEL(ch) == 0)
//        continue;

       send_to_char(ch, "%s%s %s", 
                    line_color, GET_TITLE(tch), line_color)   ;
      if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_PVP))
        send_to_char(ch, "@n(@GPvP@n) ");
      if (GET_ADMLEVEL(tch) > 0)
        send_to_char(ch, " [%s]", admin_level_names[GET_ADMLEVEL(tch)]);

      if (GET_INVIS_LEV(tch))
	send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
      else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (d->olc)
        send_to_char(ch, " (OLC)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->original)
        send_to_char(ch, " (out of body)");

      if (d->connected == CON_OEDIT)
        send_to_char(ch, " (Object Edit)");
      if (d->connected == CON_MEDIT)
        send_to_char(ch, " (Mobile Edit)");
      if (d->connected == CON_ZEDIT)
        send_to_char(ch, " (Zone Edit)");
      if (d->connected == CON_SEDIT)
        send_to_char(ch, " (Shop Edit)");
      if (d->connected == CON_REDIT)
        send_to_char(ch, " (Room Edit)");
      if (d->connected == CON_TEDIT)
        send_to_char(ch, " (Text Edit)");
      if (d->connected == CON_TRIGEDIT)
        send_to_char(ch, " (Trigger Edit)");
      if (d->connected == CON_AEDIT)
        send_to_char(ch, " (Social Edit)");
      if (d->connected == CON_CEDIT)
        send_to_char(ch, " (Configuration Edit)");
      if (d->connected >= CON_LEVELUP_START && d->connected <= CON_LEVELUP_END)
        send_to_char(ch, " (Levelling Up)");
      if (d->connected >= CON_GEN_DESCS_INTRO && d->connected <= CON_GEN_DESCS_MENU_PARSE)
        send_to_char(ch, " (Setting Descs)");

      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
      send_to_char(ch, " (Buildwalking)");

      if (PRF_FLAGGED(tch, PRF_DEAF))
	send_to_char(ch, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	send_to_char(ch, " (notell)");
      if (PRF_FLAGGED(tch, PRF_NOGOSS))
        send_to_char(ch, " (nogos)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	send_to_char(ch, " (quest)");
      if (GET_CLAN(tch) > CLAN_NONE)
        send_to_char(ch, " (%s@n)", get_clan_name(GET_CLAN(tch)));
      if (PLR_FLAGGED(tch, PLR_THIEF))
	send_to_char(ch, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	send_to_char(ch, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        send_to_char(ch, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_NOWIZ))
        send_to_char(ch, " (nowiz)");
      send_to_char(ch, "@n\r\n");
    }				/* endif shortlist */
  }				/* end of for */
  }
  if (num_staff_can_see == 0)
    send_to_char(ch, "\r\n@WThe Are Currently No Staff Members Available. (HELP PETITION)@n\r\n");
  else if (GET_ADMLEVEL(ch) == 0)
    send_to_char(ch, "\r\n@GThere are Staff Members Available if Required. (HELP PETITION)@n\r\n");

  send_to_char(ch, "\r\nKnown Adventurers Walking in %s\r\n"
                       "----------------------------------\r\n", CampaignWorld[CONFIG_CAMPAIGN]);
  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (GET_ADMLEVEL(tch))
      continue;

    line_color = "@n";
    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
      continue;
    if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
     if (showrace && !(showrace & (1 << GET_RACE(tch))))
       continue;
    num_can_see++;
		char *tmpdesc = NULL;
       send_to_char(ch, "@W(%-12s)@n %s%s %s", has_intro(ch, tch) ? (GET_CLAN(tch) ? get_blank_clan_whostring(GET_CLAN(tch)) : "UnClanned") : "Not Known", 
                    line_color, has_intro(ch, tch) ? GET_TITLE(tch) : (tmpdesc = which_desc(tch)), line_color);

//	  send_to_char(ch, "%s%s", line_color, has_intro(ch, tch) ? (GET_TITLE(tch) ? GET_TITLE(tch) : GET_NAME(tch)) : 
//		             (tmpdesc = which_desc(tch)));
		free(tmpdesc);

      if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_PVP))
        send_to_char(ch, "@n(@GPvP@n) ");

      if (GET_ADMLEVEL(tch))
        send_to_char(ch, " (%s)", admin_level_names[GET_ADMLEVEL(tch)]);

//      if (!IS_APPROVED(tch) && GET_ADMLEVEL(tch) < ADMLVL_IMMORT && GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && *tch->player_specials->short_descr)
//        send_to_char(ch, " (awaiting approval)");
      if (d->connected >= CON_LEVELUP_START && d->connected <= CON_LEVELUP_END)
        send_to_char(ch, " (Levelling Up)");
      if (d->connected >= CON_GEN_DESCS_INTRO && d->connected <= CON_GEN_DESCS_MENU_PARSE)
        send_to_char(ch, " (Setting Descs)");



      if (GET_INVIS_LEV(tch))
	send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
      else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->original)
        send_to_char(ch, " (out of body)");

      if (PRF_FLAGGED(tch, PRF_DEAF))
	send_to_char(ch, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	send_to_char(ch, " (notell)");
      if (PRF_FLAGGED(tch, PRF_NOGOSS))
        send_to_char(ch, " (nogos)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	send_to_char(ch, " (quest)");
      if (PLR_FLAGGED(tch, PLR_THIEF))
	send_to_char(ch, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	send_to_char(ch, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        send_to_char(ch, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_NOWIZ))
        send_to_char(ch, " (nowiz)");
      send_to_char(ch, "@n\r\n");
  }				/* end of for */
  

  send_to_char(ch, "@n");
  if (short_list && (num_can_see % 4))
    send_to_char(ch, "\r\n");
  if (num_can_see == 0)
    send_to_char(ch, "\r\nNone.\r\n");
  else if (!(num_can_see == 1 && GET_ADMLEVEL(ch) == ADMLVL_NONE))
    send_to_char(ch, "\r\n%d characters displayed.\r\n", num_can_see);
  send_to_char(ch, "\r\n%d %s in game in total (not necessarily visible)\r\n", tot_players, tot_players == 1 ? "person" : "people");
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
    char line[200]={'\0'}, line2[220]={'\0'}, idletime[10]={'\0'};
    char state[30], timestr[9], mode;
    char name_search[MAX_INPUT_LENGTH]={'\0'}, host_search[MAX_INPUT_LENGTH]={'\0'};
    struct char_data *tch;
    struct descriptor_data *d;
    int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
    int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
    char buf[MAX_INPUT_LENGTH]={'\0'}, arg[MAX_INPUT_LENGTH]={'\0'};

    host_search[0] = name_search[0] = '\0';

strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
    while (*buf) {
        char buf1[MAX_INPUT_LENGTH]={'\0'};

        half_chop(buf, arg, buf1);
        if (*arg == '-') {
mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
            switch (mode) {
                case 'o':
                case 'k':
                outlaws = 1;
                playing = 1;
strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
                break;
                case 'p':
                playing = 1;
strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
                break;
                case 'd':
                deadweight = 1;
strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
                break;
                case 'l':
                playing = 1;
                half_chop(buf1, arg, buf);
                sscanf(arg, "%d-%d", &low, &high);
                break;
                case 'n':
                playing = 1;
                half_chop(buf1, name_search, buf);
                break;
                case 'h':
                playing = 1;
                half_chop(buf1, host_search, buf);
                break;
                default:
                send_to_char(ch, "%s", USERS_FORMAT);
                return;
}				/* end of switch */

} else {			/* endif */
                send_to_char(ch, "%s", USERS_FORMAT);
                return;
            }
}				/* end while (parser) */
            send_to_char(ch,"Name         State          Idl Login    C Site                Client\r\n"
                            "-------------------------------------------------------------------------\r\n");

            one_argument(argument, arg);

            for (d = descriptor_list; d; d = d->next) {
                if (STATE(d) != CON_PLAYING && playing)
                    continue;
                if (STATE(d) == CON_PLAYING && deadweight)
                    continue;
                if (IS_PLAYING(d)) {
                    if (d->original)
                        tch = d->original;
                    else if (!(tch = d->character))
                        continue;

                    if (*host_search && !strstr(d->host, host_search))
                        continue;
                    if (*name_search && str_cmp(GET_NAME(tch), name_search))
                        continue;
                    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                        continue;
                    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
                        !PLR_FLAGGED(tch, PLR_THIEF))
                        continue;
                    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
                        continue;
                    if (showrace && !(showrace & (1 << GET_RACE(tch))))
                        continue;
                    if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
                        continue;
                }

                strftime(timestr, sizeof(timestr), "%H:%M:%S", localtime(&(d->login_time)));

                if (STATE(d) == CON_PLAYING && d->original)
                    strcpy(state, "Switched");

                else
                    strcpy(state, connected_types[STATE(d)]);

                if (d->character && STATE(d) == CON_PLAYING && GET_ADMLEVEL(d->character) <= GET_ADMLEVEL(ch))
                    sprintf(idletime, "%3d", d->character->timer *
                        SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
                else
                    strcpy(idletime, "");

                sprintf(line, "%-12s %-14s %-3s %-8s %1s ", d->original && d->original->name ? d->original->name : d->character && d->character->name ? d->character->name : "UNDEFINED",
                    state, idletime, timestr, d->comp->state ? d->comp->state == 1 ? "?" : "Y" : "N");

                if (d->host && *d->host)
                    sprintf(line + strlen(line), "[%s]", d->host);
                else
                    strcat(line, "[Hostname unknown]");

                sprintf(line + strlen(line), " | %-15s\r\n", d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);

                if (STATE(d) != CON_PLAYING) {
                    sprintf(line2, "@g%s@n", line);
                    strcpy(line, line2);
                }
                if (STATE(d) != CON_PLAYING ||
                    (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
                    send_to_char(ch, "%s", line);
                num_can_see++;
            }
        }

        send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
    }


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  int patch_num = 0, show_patches = 0;
  char arg[MAX_INPUT_LENGTH]={'\0'};
  const char *version_args[] = {"full", "complete", "\n"}; 
  one_argument(argument, arg);
  
  switch (subcmd) 
  {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    if (!*arg)
      show_patches = false;
    else
      if (search_block(arg, version_args, false ) != -1)
        show_patches = true;
      else
      show_patches = false;
    send_to_char(ch, "We are currently using these versions:\r\n");
    send_to_char(ch, "\t[U10148/*] %s\r\n", circlemud_version);
    send_to_char(ch, "\t[U10148/*] %s\r\n", oasisolc_version);
    send_to_char(ch, "\t[U10148/*] %s\r\n", DG_SCRIPT_VERSION);
    send_to_char(ch, "\t[U10148/*] %s\r\n", CWG_VERSION);
    send_to_char(ch, "\t[U10148/*] %s\r\n", ascii_pfiles_version);
    send_to_char(ch, "\t[U10148/*] KaVir's Protocol Snippet Version %d\r\n", SNIPPET_VERSION);
    // send_to_char(ch, "\t[U10148/*] Campaign Deities %d\r\n", CAMPAIGN_PANTHEON);
    if (show_patches == true) {
      send_to_char(ch, "The following patches have been installed:\r\n");
      for (patch_num = 0; **(patch_list + patch_num) != '\n'; patch_num++)
        send_to_char(ch, "%2d: %s\r\n", patch_num, patch_list[patch_num]);
    }
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    /*  SYSERR_DESC:
     *  General page string function for such things as 'credits', 'news',
     *  'wizlist', 'clear', 'version'.  This occurs when a call is made to
     *  this routine that is not one of the predefined calls.  To correct
     *  it, either a case needs to be added into the function to account for
     *  the subcmd that is being passed to it, or the call to the function
     *  needs to have the correct subcmd put into place.
     */
    return;
  }
}


void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
	continue;
      send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (IN_ROOM(i) == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
	continue;
      if (!isname(arg, i->name))
	continue;
      send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}


void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			        int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (obj->proto_script)
    send_to_char(ch, "[TRIG]");

  if (IN_ROOM(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s\r\n", PERS(obj->carried_by, ch));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s\r\n", PERS(obj->worn_by, ch));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}



void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char(ch, "Players\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (IS_PLAYING(d)) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
	  if (d->original)
	    send_to_char(ch, "%-20s - [%5d] %s (in %s)\r\n",
		GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		world[IN_ROOM(d->character)].name, GET_NAME(d->character));
	  else
	    send_to_char(ch, "%-20s - [%5d] %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->name)) {
	found = 1;
	send_to_char(ch, "M%3d. %-25s - [%5d] %-25s %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name,
		(IS_NPC(i) && i->proto_script) ? "[TRIG]" : "");
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, true);
      }
    if (!found)
      send_to_char(ch, "Couldn't find any such thing.\r\n");
  }
}



ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH]={'\0'};

  one_argument(argument, arg);

  if (ADM_FLAGGED(ch, ADM_FULLWHERE) || GET_ADMLEVEL(ch) == ADMLVL_IMPL)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH]={'\0'};
  size_t i, len = 0, nlen;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 2; i < CONFIG_LEVEL_CAP; i++) {
    if (i == CONFIG_LEVEL_CAP - 1)
      nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : \r\n",
           CONFIG_LEVEL_CAP - 1, level_exp(CONFIG_LEVEL_CAP - 1, GET_REAL_RACE(ch)));
    else
    nlen = snprintf(buf+len, sizeof(buf)-len, "[%2d] %8d-%-8d : \r\n", (int)i,
		level_exp(i, GET_RACE(ch)), level_exp(i+1, GET_REAL_RACE(ch)) - 1);
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  page_string(ch->desc, buf, true);
}



ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  struct char_data *victim;
  int diff = 0;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char(ch, "Would you like to borrow a cross and a shovel?\r\n");
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char(ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(ch, "You would need some luck!\r\n");
  else if (diff <= 2)
    send_to_char(ch, "You would need a lot of luck!\r\n");
  else if (diff <= 3)
    send_to_char(ch, "You would need a lot of luck and great equipment!\r\n");
  else if (diff <= 5)
    send_to_char(ch, "Do you feel lucky, punk?\r\n");
  else if (diff <= 10)
    send_to_char(ch, "Are you mad!?\r\n");
  else if (diff <= 100)
    send_to_char(ch, "You ARE mad!\r\n");
}



ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char(ch, "Diagnose who?\r\n");
  }
}


const char *ctypes[] = 
{
  "off", "on", "\n", NULL
};

char *cchoice_to_str(char *col)
{
  static char buf[READ_SIZE];
  char *s = NULL;
  int i = 0;
  int fg = 0;
  int needfg = 0;
  int bold = 0;

  if (!col) {
    buf[0] = 0;
    return buf;
  }
  while (*col) {
    if (strchr(ANSISTART, *col)) {
      col++;
    } else {
      switch (*col) {
      case ANSISEP:
      case ANSIEND:
        s = NULL;
        break;
      case '0':
        s = NULL;
        break;
      case '1':
        bold = 1;
        s = NULL;
        break;
      case '5':
        s = "blinking";
        break;
      case '7':
        s = "reverse";
        break;
      case '8':
        s = "invisible";
        break;
      case '3':
        col++;
        fg = 1;
        switch (*col) 
        {
        case '0':
          s = bold ? "grey" : "black";
          bold = 0;
          fg = 1;
          break;
        case '1':
          s = "red";
          fg = 1;
          break;
        case '2':
          s = "green";
          fg = 1;
          break;
        case '3':
          s = "yellow";
          fg = 1;
          break;
        case '4':
          s = "blue";
          fg = 1;
          break;
        case '5':
          s = "magenta";
          fg = 1;
          break;
        case '6':
          s = "cyan";
          fg = 1;
          break;
        case '7':
          s = "white";
          fg = 1;
          break;
        case 0:
          s = NULL;
          break;
        }
        break;
      case '4':
        col++;
        switch (*col) 
        {
        case '0':
          s = "on black";
          needfg = 1;
          bold = 0;
          break;
        case '1':
          s = "on red";
          needfg = 1;
          bold = 0;
          break;
        case '2':
          s = "on green";
          needfg = 1;
          bold = 0;
          break;
        case '3':
          s = "on yellow";
          needfg = 1;
          bold = 0;
          break;
        case '4':
          s = "on blue";
          needfg = 1;
          bold = 0;
          break;
        case '5':
          s = "on magenta";
          needfg = 1;
          bold = 0;
          break;
        case '6':
          s = "on cyan";
          needfg = 1;
          bold = 0;
          break;
        case '7':
          s = "on white";
          needfg = 1;
          bold = 0;
          break;
        default:
          s = "underlined";
          break;
        }
        break;
      default:
        s = NULL;
        break;
      }
      if (s) {
        if (needfg && !fg) {
          i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
          fg = 1;
        }
        if (i)
          i += snprintf(buf + i, sizeof(buf) - i, " ");
        if (bold) {
          i += snprintf(buf + i, sizeof(buf) - i, "bright ");
          bold = 0;
        }
        i += snprintf(buf + i, sizeof(buf) - i, "%s", s ? s : "null 1");
        s = NULL;
      }
      col++;
    }
  }
  if (!fg)
    i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
  return buf;
}

int str_to_cchoice(char *str, char *choice)
{
  char buf[MAX_STRING_LENGTH]={'\0'};
  int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0, error = 0;
  int i, len = MAX_INPUT_LENGTH;
  struct 
  {
    char *name; 
    int *ptr;
  } attribs[] = {
    { "bright", &bold },
    { "bold", &bold },
    { "underlined", &uline },
    { "reverse", &rev },
    { "blinking", &blink },
    { "invisible", &invis },
    { NULL, NULL }
  };
  struct {
    char *name;
    int val;
    int bold;
  } colors[] = {
    { "default", -1, 0 },
    { "normal", -1, 0 },
    { "black", 0, 0 },
    { "red", 1, 0 },
    { "green", 2, 0 },
    { "yellow", 3, 0 },
    { "blue", 4, 0 },
    { "magenta", 5, 0 },
    { "cyan", 6, 0 },
    { "white", 7, 0 },
    { "grey", 0, 1 },
    { "gray", 0, 1 },
    { NULL, 0, 0 }
  };
  skip_spaces(&str);
  if (isdigit(*str)) { /* Accept a raw code */
    strcpy(choice, str);
    for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++);
    error = choice[i] != 0;
    choice[i] = 0;
    return error;
  }
  while (*str) {
    str = any_one_arg(str, buf);
    if (!strcmp(buf, "on")) {
      bg = 1;
      continue;
    }
    if (!fg) {
      for (i = 0; attribs[i].name; i++)
        if (!strncmp(attribs[i].name, buf, strlen(buf)))
          break;
      if (attribs[i].name) {
        *(attribs[i].ptr) = 1;
        continue;
      }
    }
    for (i = 0; colors[i].name; i++)
      if (!strncmp(colors[i].name, buf, strlen(buf)))
        break;
    if (!colors[i].name) {
      error = 1;
      continue;
    }
    if (colors[i].val != -1) {
      if (bg == 1) {
        bg = 40 + colors[i].val;
      } else {
        fg = 30 + colors[i].val;
        if (colors[i].bold)
          bold = 1;
      }
    }
  }
  choice[0] = i = 0;
  if (bold)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BOLD);
  if (uline)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_UNDERLINE);
  if (blink)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BLINK);
  if (rev)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_REVERSE);
  if (invis)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_INVIS);
  if (!i)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_NORMAL);
  if (fg && fg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , fg);
  if (bg && bg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , bg);

  return error;
}

char *default_color_choices[NUM_COLOR + 1] = {
/* COLOR_NORMAL */	AA_NORMAL,
/* COLOR_ROOMNAME */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMOBJS */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_ROOMPEOPLE */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_HITYOU */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUHIT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_OTHERHIT */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_CRITICAL */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_HOLLER */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_SHOUT */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_GOSSIP */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_AUCTION */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_CONGRAT */	AA_NORMAL ANSISEPSTR AF_GREEN,
NULL
};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH]={'\0'};
  char buf[MAX_STRING_LENGTH]={'\0'};
  char *p;
  char *col;
  int tp = 0, len = 0;

  if (IS_NPC(ch))
    return;

  p = any_one_arg(argument, arg);

  if (!*arg) {
    len = snprintf(buf, sizeof(buf), "Currently, color is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    if (COLOR_LEV(ch)) {
      len += snprintf(buf + len, sizeof(buf) - len, "\r\nYour color choices:\r\n");
      for (tp = 0; tp < NUM_COLOR; tp++) {
        len += snprintf(buf + len, sizeof(buf) - len, " %2d: %-20s - ", tp,
                        cchoice_names[tp]);
        col = ch->player_specials->color_choices[tp];
        if (!col) {
          len += snprintf(buf + len, sizeof(buf) - len, "(default) ");
          col = default_color_choices[tp];
        }
        len += snprintf(buf + len, sizeof(buf) - len, "%s (%s)\r\n", cchoice_to_str(col), col);
      }
    }
    page_string(ch->desc, buf, true);
    return;
  }
  if (isdigit(*arg)) {
    tp = atoi(arg);
    if (tp < 0 || tp >= NUM_COLOR) {
      send_to_char(ch, "Custom color selection out of range.\r\n");
      return;
    }
    skip_spaces(&p);
    if (!strcmp(p, "default")) {
      if (ch->player_specials->color_choices[tp])
        free(ch->player_specials->color_choices[tp]);
      ch->player_specials->color_choices[tp] = NULL;
      send_to_char(ch, "Using default color for %s\r\n", cchoice_names[tp]);
    } else if (str_to_cchoice(p, buf)) {
      send_to_char(ch, "Invalid color choice.\r\n\r\n"
"Format: code | ( [ attributes ] [ foreground | \"default\" ] [ \"on\" background ] )\r\n"
"Attributes: underlined blink reverse invisible\r\n"
"Foreground colors: black, grey, red, bright red, green, bright green, yellow,\r\n"
"                   bright yellow, blue, bright blue, magenta,\r\n"
"                   bright magenta, cyan, bright cyan, white, bright white\r\n"
"Background colors: black, red, green, yellow, blue, magenta, cyan, white\r\n"
"Examples: red, bright blue on yellow, 1;34;45\r\n");
    } else {
      if (ch->player_specials->color_choices[tp])
        free(ch->player_specials->color_choices[tp]);
      ch->player_specials->color_choices[tp] = strdup(buf);
      send_to_char(ch, "Setting color %d to %s: @[%d]sample@n\r\n", tp, cchoice_to_str(buf), tp);
    }
    return;
  } else if (((tp = search_block(arg, ctypes, false)) == -1)) {
    send_to_char(ch, "Usage: color [ off | on | number [ color choice ] ]\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
      break;
    case C_ON:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
      break;
  }
  send_to_char(ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}


ACMD(do_toggle)
{
  char buf2[4]={'\0'};

  if (IS_NPC(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");	/* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));	/* sprintf: OK */

  if (GET_ADMLEVEL(ch)) {
    send_to_char(ch,
          "      Buildwalk: %-3s    "
          "Clear Screen in OLC: %-3s\r\n",
        ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
        ONOFF(PRF_FLAGGED(ch, PRF_CLS))
    );

    send_to_char(ch,
	  "      No Hassle: %-3s    "
	  "      Holylight: %-3s    "
	  "     Room Flags: %-3s\r\n"
          "  Clan Channels: %-3s\r\n",
	ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
	ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
	ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),
        ONOFF(PRF_FLAGGED(ch, PRF_ALLCTELL))
    );
  }

  send_to_char(ch,
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  "   Mana Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  "     Ki Display: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

          "      Auto Loot: %-3s    "
          "      Auto Gold: %-3s    "
	  "    Color Level: %s\r\n"

          "     Auto Split: %-3s    "
          "       Auto Sac: %-3s    "
          "       Auto Mem: %-3s\r\n"

          "     View Order: %-3s    "
          "    Auto Assist: %-3s    "
	  " Auto Show Exit: %-3s\r\n"

          "    Screenwidth: %-3d    "
          "   Clan Channel: %-3s    "          
	  "        Automap: %-3s\r\n"

          "    Power Sneak: %-3s    "
          "    Bleed Attck: %-3s    "
          "    Knockdown  : %-3s    "

          "    Roblr Gambt: %-3s    "
          "    Taking Ten : %-3s    "
          "    Mount Tank : %-3s    "

          "    Summon Tank: %-3s    "
          "    Divine Bond: %-3s    ",

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPKI)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
	  ctypes[COLOR_LEV(ch)],

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),

          ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),

          GET_SCREEN_WIDTH(ch),
          ONOFF(PRF_FLAGGED(ch, PRF_CLANTALK)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAP)),
          
          ONOFF(PRF_FLAGGED(ch, PRF_POWERFUL_SNEAK)),
          ONOFF(PRF_FLAGGED(ch, PRF_BLEEDING_ATTACK)),
          ONOFF(PRF_FLAGGED(ch, PRF_KNOCKDOWN)),

          ONOFF(PRF_FLAGGED(ch, PRF_ROBILARS_GAMBIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_TAKE_TEN)),
          ONOFF(PRF_FLAGGED(ch, PRF_MOUNT_TANK)),

          ONOFF(PRF_FLAGGED(ch, PRF_SUMMON_TANK)),
          ONOFF(PRF_FLAGGED(ch, PRF_DIVINE_BOND))

          );

          if (CONFIG_ENABLE_COMPRESSION) {
            send_to_char(ch, "    Compression: %-3s\r\n", ONOFF(!PRF_FLAGGED(ch, PRF_NOCOMPRESS)));
          }
}


int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}


void sort_commands(void)
{
  int a = 0, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;	/* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


ACMD(do_commands)
{
    int no = 0, i = 0, cmd_num = 0;
    int wizhelp = 0, socials = 0;
    struct char_data *vict;
    char arg[MAX_INPUT_LENGTH]={'\0'};

    one_argument(argument, arg);

    if (*arg) 
    {
        if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict)) 
        {
            send_to_char(ch, "Who is that?\r\n");
            return;
        }
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) 
        {
            send_to_char(ch, "You can't see the commands of people above your level.\r\n");
            return;
        }
    } 
    else
        vict = ch;

    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    else if (subcmd == SCMD_WIZHELP)
        wizhelp = 1;

    send_to_char(ch, "The following %s%s are available to %s:\r\n",
        wizhelp ? "privileged " : "",
        socials ? "socials" : "commands",
        vict == ch ? "you" : GET_NAME(vict));

/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) 
    {
        i = cmd_sort_info[cmd_num];

        if (complete_cmd_info[i].minimum_level < 0 || GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
            continue;

        if (complete_cmd_info[i].minimum_admlevel < 0 || GET_ADMLEVEL(vict) < complete_cmd_info[i].minimum_admlevel)
            continue;

        if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
            continue;

        if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action || complete_cmd_info[i].command_pointer == do_insult))
            continue;

        if (check_disabled(&complete_cmd_info[i]))
            sprintf(arg, "(%s)", complete_cmd_info[i].command);
        else  
            sprintf(arg, "%s", complete_cmd_info[i].command);

        send_to_char(ch, "%-20s%s", arg, no++ % 4 == 0 ? "\r\n" : "");

    }

    if (no % 5 != 1)
        send_to_char(ch, "\r\n");
}

ACMD(do_whois)
{
  const char *immlevels[ADMLVL_OWNER + 1] = {
  "[Mortal]",          /* lowest admin level */
  "[Immortal]",        /* lowest admin level +1 */
  "[Builder]",         /* lowest admin level +2 */
  "[God]",             /* lowest admin level +3 */
  "[Greater God]",     /* lowest admin level +4 */
  "[Co-Owner]",     /* lowest admin level +5 */
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
  CREATE(victim, struct char_data, 1);
  clear_char(victim);
  CREATE(victim->player_specials, struct player_special_data, 1);
  if (load_char(argument, victim) >= 0) {
    if (GET_ADMLEVEL(victim) >= ADMLVL_IMMORT)
      send_to_char(ch, "%s Level %d %s %s\r\n", immlevels[GET_ADMLEVEL(victim)],
                   GET_LEVEL(victim), GET_NAME(victim), GET_TITLE(victim));
    else 
      send_to_char(ch, "Level %d %s - %s %s\r\n", GET_LEVEL(victim),
                   CLASS_ABBR(victim), GET_NAME(victim), GET_TITLE(victim));
    } else {
    send_to_char(ch, "There is no such player.\r\n"); 
    }
    free(victim); 
  }
}

#define DOOR_DCHIDE(ch, door)           (EXIT(ch, door)->dchide)

void search_in_direction(struct char_data * ch, int dir)
{
  int check=false, skill_lvl, dchide=20;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", true, ch, 0, 0, TO_ROOM);

  /* SEARCHING is allowed untrained */
  skill_lvl = roll_skill(ch, SKILL_PERCEPTION);
  // if (IS_ELF(ch) || IS_DROW_ELF(ch) || IS_HALF_ELF(ch)) 
  //   skill_lvl = skill_lvl + 2;
  
  if (EXIT(ch, dir))
    dchide = DOOR_DCHIDE(ch, dir);

  if (skill_lvl > dchide)
    check = true;

  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description &&
        !EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) &&
             EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
             EXIT(ch, dir)->keyword && (check == true)  )
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname (EXIT(ch, dir)->keyword),
                   (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}

ACMD(do_affstats) 
{
  struct char_data *k = ch;
  char duration[100]={'\0'}, modifier[100]={'\0'}, bitvector[100]={'\0'};
  struct affected_type *aff;

  send_to_char(k, "@y%-25s %-5s %-10s %-25s %-17s\r\n------------------------- ----- ---------- ------------------------- -----------------\r\n@n",
               "Affect Name", "Level", "Duration", "Modifies", "Affects");

  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      sprintf(duration, "%-3d rounds", aff->duration + 1);
      sprintf(modifier, "%+d to %s", aff->modifier, apply_text[(int) aff->location]);
      sprintf(bitvector, "sets %s", affected_bits[aff->bitvector]);
      send_to_char(k, "@w%-25s %-5d %-10s %-25s %-17s\r\n@n", skill_name(aff->type), aff->level, duration, modifier, bitvector);
    }
  }


  if (k->affectedv) {
    for (aff = k->affectedv; aff; aff = aff->next) {
      sprintf(duration, "%-3d rounds", aff->duration + 1);
      sprintf(modifier, "%+d to %s", aff->modifier, apply_text[(int) aff->location]);
      sprintf(bitvector, "sets %s", affected_bits[aff->bitvector]);
      send_to_char(k, "@w%-25s %-10s %-25s %-17s\r\n@n", skill_name(aff->type), duration, modifier, bitvector);
    }
  }

  send_to_char(ch, "\r\n");

}

ACMD(do_affect) 
{
	char buf[MAX_STRING_LENGTH]={'\0'};
	struct damreduct_type *reduct;
	int isAffected = FALSE, len;
	
	len = sprintf(buf, "@n@KYou are affected by the following:@n\r\n\r\n");
	len += sprintf(buf+len, "@WPositive Effects:@n\r\n");
	
	if (affected_by_spell(ch, SPELL_MAGE_ARMOR) || affected_by_spell(ch, SPELL_GROUP_ARMOR)) {
		len += sprintf(buf+len, "@wThe protective, invisible barrier of mage armor surrounds you.\r\n");
		isAffected = TRUE;
	}

	if (affected_by_spell(ch, SPELL_BLESS)) {
		len += sprintf(buf+len, "@wYou have been blessed by divine power.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DETECT_ALIGN)) {
		len += sprintf(buf+len, "@wYou have the ability to detect the alignment of those in your presence.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_SEE_INVIS)) {
		len += sprintf(buf+len, "@wYou have the ability to see invisible beings and objects in your presence.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DETECT_MAGIC)) {
		len += sprintf(buf+len, "@wYou have the ability to detect magical items and effects in your presence.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DETECT_POISON)) {
		len += sprintf(buf+len, "@wYou have the ability to detect poisons and other toxins in your presence.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_INVISIBLE)) {
		len += sprintf(buf+len, "@wYou are invisible to the naked eye and can only be seen by special methods.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_PROT_FROM_EVIL) || affected_by_spell(ch, SPELL_AURA_OF_GOOD)) {
		len += sprintf(buf+len, "@wYou have been imbued with a heightened protection against evil beings.@n\r\n");
		isAffected = TRUE;
	}	
	
	if (affected_by_spell(ch, SPELL_SANCTUARY)) {
		len += sprintf(buf+len, "@wYou are protected by a sanctuary of peace and are protected from many attacks.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_BULL_STRENGTH)) {
		len += sprintf(buf+len, "@wYou have been imbued with the strength of the bull.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_SENSE_LIFE)) {
		len += sprintf(buf+len, "@wYou have the ability to sense the presence of nearby living creatures.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DARKVISION)) {
		len += sprintf(buf+len, "@wYour eyes are able to see in darkvision and suffer no penalities seeing in any non-magical darkness.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_WATERWALK)) {
		len += sprintf(buf+len, "@wYou have the ability to walk on water.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_RESISTANCE)) {
		len += sprintf(buf+len, "@wYour body and mind have been reenforced with extra resistance against negative effects.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_LESSER_GLOBE_OF_INVUL)) {
		len += sprintf(buf+len, "@wYou are protected by a lesser globe of invulnerability giving you immunity to minor magical spells.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_STONESKIN)) {
		len += sprintf(buf+len, "@wYour skin has become as hard as stone giving you protection from physical attacks.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_FIRE_SHIELD)) {
		len += sprintf(buf+len, "@wYou are surrounded by a shield of fire threatening tosear any who attempt a physical attack.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_PASSWALL)) {
		len += sprintf(buf+len, "@wYou have the passwall ability and can pass through unwarded doors without effort.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_COURAGE) || affected_by_spell(ch, SPELL_AURA_OF_COURAGE)) {
		len += sprintf(buf+len, "@wYou are filled with courage bolstering your resistance against natural and magical fear.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DIVINE_FAVOR)) {
		len += sprintf(buf+len, "@wYou have been bestowed with divine favor and have heightened combat abilities as a result.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_SHIELD_OF_FAITH)) {
		len += sprintf(buf+len, "@wYou are protected by a shield of faith and have a heightened deflection defense against physical attacks@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_EAGLES_SPLENDOR)) {
		len += sprintf(buf+len, "@wYou have been given the splendor of the eagle and as a result have a higher charisma than normal.@n\r\n");		
		isAffected = TRUE;
	}
	if (affected_by_spell(ch, SPELL_BEARS_ENDURANCE)) {
		len += sprintf(buf+len, "@wYou have been given the endurance of the bear and as a result have a higher consitution than normal.@n\r\n");		
		isAffected = TRUE;
	}
	if (affected_by_spell(ch, SPELL_OWLS_WISDOM)) {
		len += sprintf(buf+len, "@wYou have been given the wisdom of the owl and as a result have a higher wisdom than normal.@n\r\n");		
		isAffected = TRUE;
	}
	if (affected_by_spell(ch, SPELL_AID)) {
		len += sprintf(buf+len, "@wYout are blessed with the aid of the Gods and have a better accuracy and more health as a result.@n\r\n");		
		isAffected = TRUE;
	}	
	if (affected_by_spell(ch, SPELL_DEATH_KNELL)) {
		len += sprintf(buf+len, "@wYou have absorbed the life force of another being and as a result have heightened strength, accuracy and health.@n\r\n");		
		isAffected = TRUE;
	}	
	if (affected_by_spell(ch, SPELL_UNDETECTABLE_ALIGNMENT)) {
		len += sprintf(buf+len, "@wYour alignment is completely undetectable by all but the most powerful of magic.@n\r\n");		
		isAffected = TRUE;
	}			
	if (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS)) {
		len += sprintf(buf+len, "@wYou are currently fighting using the flurry of blows method\r\n@n");
		isAffected = TRUE;
	}
	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_10)) {
		len += sprintf(buf+len, "@wYou are benefitting from a 10%% bonus to all experience gained.\r\n@n");
		isAffected = TRUE;
	}	
	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_25)) {
		len += sprintf(buf+len, "@wYou are benefitting from a 25%% bonus to all experience gained.\r\n@n");
		isAffected = TRUE;
	}	
	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_33)) {
		len += sprintf(buf+len, "@wYou are benefitting from a 33%% bonus to all experience gained.\r\n@n");
		isAffected = TRUE;
	}	
	if (AFF_FLAGGED(ch, AFF_EXP_BONUS_50)) {
		len += sprintf(buf+len, "@wYou are benefitting from a 50%% bonus to all experience gained.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_COMPREHEND_LANGUAGES)) {
		len += sprintf(buf+len, "@wYou are able to understand all written and spoken languages.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_TONGUES)) {
		len += sprintf(buf+len, "@wYou are able to read, write, speak and understand all languages.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_HASTE)) {
		len += sprintf(buf+len, "@wYou are able to move at an accelerated and hasted speed.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_BLUR)) {
		len += sprintf(buf+len, "@wYour body is blurred and hard to see, causing higher miss chance by opponents.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_AID)) {
		len += sprintf(buf+len, "@wYou have been blessed with the aid of the gods.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_DEATH_KNELL)) {
		len += sprintf(buf+len, "@wYour life force has been infused with that of another.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_FLAME_WEAPON)) {
		len += sprintf(buf+len, "@wYour weapons have been wreathed in flame for extra damage.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_SHILLELAGH)) {
		len += sprintf(buf+len, "@wYour clubs and staves have been enchanted for extra damage.\r\n@n");
		isAffected = TRUE;
	}		
	if (affected_by_spell(ch, SPELL_BARKSKIN)) {
		len += sprintf(buf+len, "@wYou are covered in bark, improving your natural armor class.\r\n@n");
		isAffected = TRUE;
	}		
	for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
		if (reduct->spell == SPELL_STONESKIN) {
			len += sprintf(buf+len, "@wYou are protected by stoneskin offering you damage reduction of %s.\r\n@n", reduct_desc(ch, reduct));
			isAffected = TRUE;
			break;
		} else if (reduct->spell == SPELL_PREMONITION) {
			len += sprintf(buf+len, "@wYou are protected by premonition offering you damage reduction of %s.\r\n@n", reduct_desc(ch, reduct));
			isAffected = TRUE;
			break;
		}
	}
          

	if (!isAffected)
		len += sprintf(buf+len, "@wNothing\r\n");

	isAffected = FALSE;

	len += sprintf(buf+len, "\r\n@RNegative Effects:@n\r\n");	
	
	if (affected_by_spell(ch, SPELL_BLINDNESS)) {
		len += sprintf(buf+len, "@rYou are blind and cannot see.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_CHARM)) {
		len += sprintf(buf+len, "@rYou have been charmed.  !!!READ HELP RULES CHARM!!!@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_CHILL_TOUCH)) {
		len += sprintf(buf+len, "@rYour body has been weakened and numbed by a chill touch.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_COLOR_SPRAY)) {
		len += sprintf(buf+len, "@rYou see only spots of white and red after being dazzled by a color spray.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_BESTOW_CURSE)) {
		len += sprintf(buf+len, "@rYou have been cursed resulting in bad luck and weakened abilities@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_POISON)) {
		len += sprintf(buf+len, "@rYou have been poisoned and are weakened as a result.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_SLEEP) || affected_by_spell(ch, SPELL_SLEEP_SINGLE)) {
		len += sprintf(buf+len, "@rYou are afflicted by a magical slumber and cannot awaken.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_PARALYZE)) {
		len += sprintf(buf+len, "@rYou have been paralyzed and cannot move an inch.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_FAERIE_FIRE)) {
		len += sprintf(buf+len, "@rYou are surrounded by flickering purple faerie fire making you an easier target for enemies.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_DAZE)) {
		len += sprintf(buf+len, "@rYou have been dazed and are unable to act.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_FLARE)) {
		len += sprintf(buf+len, "@rYou have trouble seeing well due to a quick flare of light resulting in decreased accuracy.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_FEAR)) {
		len += sprintf(buf+len, "@rYou are trembling with fear making you unable to act when the terror is at its highest.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_HOLD_MONSTER)) {
		len += sprintf(buf+len, "@rYou are held firm by a magical force and have only the ability to move your eyes and breathe.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_BALEFUL_POLYMORPH)) {
		len += sprintf(buf+len, "@rYou have been polymorphed into a sheep and have no way to defend yourself effectively.@n\r\n");
		isAffected = TRUE;
	}
	
	if (affected_by_spell(ch, SPELL_AFF_STUNNED)) {
		len += sprintf(buf+len, "@rYou have been stunned and cannot act for the time being.@n\r\n");
		isAffected = TRUE;
	}

	if (affected_by_spell(ch, SPELL_SLOW)) {
		len += sprintf(buf+len, "@rYou are slowed down by an unseen magical weight across your entire body.@n\r\n");
		isAffected = TRUE;
	}

	if (!isAffected)
		len += sprintf(buf+len, "@wNothing\r\n");

  send_to_char(ch, "%s", buf);
}

ACMD(do_scan)
{
  struct char_data *i;

  int curDir = 0;

  int curRange = 0;

  int maxRange = 0;

  int found = 0;

  room_rnum inRoom = NOWHERE;



  const char *disMessage[] = {

	"close  : ",

	"nearby : ",

	"nearby : ",

	"distant: ",

	"distant: ",

	"remote : ",

	"remote : "

  };



  // First check if they are awake

  if (GET_POS(ch) < POS_SLEEPING)
  {
    send_to_char(ch, "You can't see anything but stars!\r\n");
	return;
  }

  // Then check if they can see
  if (AFF_FLAGGED(ch, AFF_BLIND))
  {
    send_to_char(ch, "You can't see anything, you're blind!\r\n");
	return;
  }

  // Then check for enough light
  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) 
  {
    send_to_char(ch, "It is pitch black...\r\n");
  }



  // Now we know that they can perform the scan



  // If not standing max range is just one

  if (GET_POS(ch) != POS_STANDING)

	  maxRange = 1;

  // else if they have skill and are standing (able to perform thorough search)

  //   give them a farther search distance based on skill

  // !!this probably needs a better max based on skill range, current max is at

  // !!scan level 10 for seven room distance - Zivilyn (3-16-07)

  else

	maxRange = MAX(7, MAX(1, 1 + (GET_SKILL(ch, SKILL_SURVIVAL) / 4) + 
		       ((IS_ELF(ch) || IS_KENDER(ch)) ? 1 : 0) ));

  

  if (GET_LEVEL(ch) >= ADMLVL_IMMORT)

    maxRange = 7;



  // Set inRoom to room ch is standing in

  inRoom = ch->in_room;



  // Loop through all directions

  for (curDir = 0; curDir < NUM_OF_DIRS; curDir++) 

  {

    // Reset ch's room to the room they are in before search in new dir

	ch->in_room = inRoom;



    // Loop through range to distance

    for (curRange = 0; curRange <= maxRange; curRange++) 

	{

	  // If we are at start of new direction, check for direction

	  

	  

	  if(curRange == 0)

	  {

		  // If no can go, skip

		  if(!CAN_GO(ch, curDir) || (world[ch->in_room].dir_option[curDir]->to_room == inRoom))

			  break;



		  // If can go, print 'looking in' message, set found to 0 and

		  // increase range to first room

		  else

		  {

			  ch->in_room = world[ch->in_room].dir_option[curDir]->to_room;

			  send_to_char(ch, "Looking %s you see:\r\n", dirs[curDir]);

			  found = 0;

		  }

	  }

	  // Else we are in a new room in direction-curDir at range curRange

	  // Look for see-able people in this room, then check for range increase

	  else

	  {

		  // Loop through all people in the room

		  for(i = world[ch->in_room].people; i; i = i->next_in_room)

		  {

			  // Make sure i is not ch, and that ch can see i

			  if(!(ch == i) && CAN_SEE(ch,i))

			  {

				  // If they don't have skill send simple message

				  if(maxRange == 1)

					send_to_char(ch, "%s\r\n", GET_NAME(i));



				  // Else send them a more complex version

				  else

				    send_to_char(ch, "%s%s\r\n", disMessage[curRange-1], GET_NAME(i));

			  }

		  }



		  // Check for continuation in curDir

		  // If no can go, skip

		  if(!CAN_GO(ch, curDir) || (world[ch->in_room].dir_option[curDir]->to_room == inRoom))

		  {

			  // If nothing was found in this direction, print a '-' before continuing

			  if(found == 0)

				  send_to_char(ch, "-\r\n");

			  break;

		  }



		  // Else increase range to next room

		  else

			  ch->in_room = world[ch->in_room].dir_option[curDir]->to_room;

	  }

	}

  }



  // Reset ch's room to saved inRoom

  ch->in_room = inRoom;

}

char *attribute_text(int att, char *buf)
{
  if (TRUE || CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    sprintf(buf, "%-2d [%s%d]", att, ability_mod_value(att) >= 0 ? "+" : "", ability_mod_value(att));
  else
    attribute_text_desc(att, buf);

  return buf;
}

char *attribute_text_desc(int att, char *buf)
{
  if (att <= 3)
    strcpy(buf, "feeble (1)");
  else if (att <= 6)
    strcpy(buf, "very poor (2)");
  else if (att <= 9)
    strcpy(buf, "poor (3)");
  else if (att <= 12)
    strcpy(buf, "typical (4)");
  else if (att <= 15)
    strcpy(buf, "good (5)");
  else if (att <= 18)
    strcpy(buf, "excellent (6)");
  else if (att <= 21)
    strcpy(buf, "remarkable (7)");
  else if (att <= 24)
    strcpy(buf, "incredible (8)");
  else if (att <= 27)
    strcpy(buf, "amazing (9)");
  else if (att <= 30)
    strcpy(buf, "monstrous (10)");
  else if (att <= 33)
    strcpy(buf, "heroic (11)");
  else if (att <= 36)
    strcpy(buf, "epic (12)");
  else if (att <= 39)
    strcpy(buf, "unearthly (13)");
  else
    sprintf(buf, "godly (%d)", (att + 2) / 3);    
  
  return buf;
}

char * offense_text(int off, char *buf)
{
  if (TRUE || CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    sprintf(buf, "%s%-2d", off > 0 ? "+" : "", off);
  else if (off <= 0)
    strcpy(buf, "inept (0)");
  else if (off <= 3)
    strcpy(buf, "novice (1)");
  else if (off <= 6)
    strcpy(buf, "adept (2)");
  else if (off <= 9)
    strcpy(buf, "veteran (3)");
  else if (off <= 12)
    strcpy(buf, "proficient (4)");
  else if (off <= 15)
    strcpy(buf, "expert (5)");
  else if (off <= 18)
    strcpy(buf, "master (6)");
  else if (off <= 21)
    strcpy(buf, "hero (7)");
  else if (off <= 24)
    strcpy(buf, "epic hero (8)");
  else if (off <= 27)
    strcpy(buf, "legend (9)");
  else
    sprintf(buf, "epic legend (%d)", (off + 2) / 3);

  return buf;
  
}

char *defense_text(int def, char *buf)
{
  if (TRUE || CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    sprintf(buf, "%-2d.%1d", def / 10, def % 10);
  else
    defense_text_desc(def, buf);

  return buf;
}

char *offense_text_desc(int off, char *buf)
{
  if (off <= 0)
    strcpy(buf, "inept (0)");
  else if (off <= 3)
    strcpy(buf, "that of a novice (1)");
  else if (off <= 6)
    strcpy(buf, "adept (2)");
  else if (off <= 9)
    strcpy(buf, "that of a veteran (3)");
  else if (off <= 12)
    strcpy(buf, "proficient (4)");
  else if (off <= 15)
    strcpy(buf, "expert (5)");
  else if (off <= 18)
    strcpy(buf, "masterful (6)");
  else if (off <= 21)
    strcpy(buf, "heroic (7)");
  else if (off <= 24)
    strcpy(buf, "epic (8)");
  else if (off <= 27)
    strcpy(buf, "legendary (9)");
  else
    sprintf(buf, "that of an epic legend (%d)", (off + 2) / 3);

  return buf;
  
}

char *defense_text_desc(int def, char *buf)
{
  if (TRUE || def <= 10)
    strcpy(buf, "unprotected (0)");
  else if (def <= 15)
    strcpy(buf, "lightly protected (1)");    
  else if (def <= 20)
    strcpy(buf, "moderately protected (2)");
  else if (def <= 25)
    strcpy(buf, "protected (3)");    
  else if (def <= 30)
    strcpy(buf, "well protected (4)");    
  else if (def <= 35)
    strcpy(buf, "heavily protected (5)");  
  else if (def <= 40)
    strcpy(buf, "barely penetrable (6)");    
  else if (def <= 45)
    strcpy(buf, "almost unpenetrable (7)");
  else if (def <= 50)
    strcpy(buf, "a wall of defense (8)");  
  else if (def <= 55)
    strcpy(buf, "a bulwark of protection (9)");
  else
    sprintf(buf, "a perfect defense (%d)", (def - 6) / 5);

  return buf;
}

char *saving_throw_text(int save, char *buf)
{
  if (TRUE || CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    sprintf(buf, "%s%-2d", save > 0 ? "+" : "", save);
  else
    saving_throw_text_desc(save, buf);

  return buf;
}

char *saving_throw_text_desc(int save, char *buf)
{
  if (save <= 0)
    strcpy(buf, "vulnerable (0)");
  else if (save <= 3)
    strcpy(buf, "poor (1)");
  else if (save <= 6)
    strcpy(buf, "decent (2)");
  else if (save <= 9)
    strcpy(buf, "good (3)");  
  else if (save <= 12)
    strcpy(buf, "great (4)");  
  else if (save <= 15)
    strcpy(buf, "astounding (5)");
  else if (save <= 18)
    strcpy(buf, "incredible (6)");    
  else if (save <= 21)
    strcpy(buf, "amazing (7)");
  else 
    sprintf(buf, "almost immune (%d)", (save + 2) / 3);

  return buf;
  
}

char *get_pc_alignment(struct char_data *ch, char *alignment)
{
  if (IS_ETHIC_NEUTRAL(ch) && IS_NEUTRAL(ch)) {
    sprintf(alignment, "True Neutral");
    return alignment;
  }

  if (IS_LAWFUL(ch))
    sprintf(alignment, "Lawful ");
  else if (IS_ETHIC_NEUTRAL(ch))
    sprintf(alignment, "Neutral ");
  else 
    sprintf(alignment, "Chaotic ");

  if (IS_GOOD(ch))
    sprintf(alignment + strlen(alignment), "Good");
  else if (IS_NEUTRAL(ch))
    sprintf(alignment + strlen(alignment), "Neutral");
  else 
    sprintf(alignment + strlen(alignment), "Evil");

  return alignment;

}
void FormatNumberOutput(char * String)
{
	int	x,c,d,i;
	char Work[20]={'\0'};

	x = strlen(String)-1;

	for (i=1,c=x+(x/3)+1,d=x; c>0; --c,--d,++i)
	{
		if(i%4==0)
		{
			*(Work+(--c)) = ',';
			i=1;
		}
		*(Work+(c-1)) = *(String+d);
	}

	Work[x+(x/3)+1] = '\0';

	strcpy(String, Work);
}

ACMD(do_screenwidth)
{
  char arg2[100]={'\0'};

  one_argument(argument, arg2);

    if (!*arg2)
      send_to_char(ch, "You current screen width is set to %d characters.", GET_SCREEN_WIDTH(ch));
    else if (is_number(arg2)) {
      GET_SCREEN_WIDTH(ch) = MIN(MAX(atoi(arg2), 20), 200);
      send_to_char(ch, "Okay, your screen width is now set to %d characters.", GET_SCREEN_WIDTH(ch));
    } else
      send_to_char(ch, "Please specify a number of characters (40 - 200).");
}


ACMD(do_automap)
{
  char arg[100]={'\0'};

  one_argument(argument, arg);

    if (can_see_map(ch)) {
      if (!*arg) {
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOMAP);
        send_to_char(ch, "Automap turned %s.\r\n", ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAP)));
        return;
      } else if (!strcmp(arg, "on")) {
        SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOMAP);
        send_to_char(ch, "Automap has been turned on.\r\n");
        return;
      } else if (!strcmp(arg, "off")) {
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOMAP);
        send_to_char(ch, "Automap has been turned off.\r\n");
        return;
      } else {
        send_to_char(ch, "Value for automap must either be 'on' or 'off'.\r\n");
        return;
      }
    } else {
      send_to_char(ch, "Sorry, automap is currently disabled.\r\n");
      return;
    }

}

ACMD(do_bounty)
{
  if (GET_AUTOQUEST_VNUM(ch) < 1) {
    send_to_char(ch, "You do not currently have a bounty contract.\r\n");
    return;
  }


  if (GET_AUTOQUEST_KILLNUM(ch) > 0) {
    send_to_char(ch, "You have not yet completed your bounty contract against %s.\r\n"
                     "You still need to kill %d more.\r\n"
                     "Once completed you will receive the following:\r\n"
                     "You will receive %d quest points.\r\n"
                     "%d coins will be deposited into your bank account.\r\n"
                     "You will receive %d experience points.\r\n",
                     GET_AUTOQUEST_DESC(ch), GET_AUTOQUEST_KILLNUM(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch));
    return;
  }

  send_to_char(ch, "You have completed your bounty contract against %s.\r\n"
                   "You need to find a bounty contractor and type 'bounty complete'.  This is likely the same place where you took the bounty.\r\n"
                   "Once turned in you will receive the following:\r\n"
                   "You will receive %d quest points.\r\n"
                   "%d coins will be deposited into your bank account.\r\n"
                   "You will receive %d experience points.\r\n",
                   GET_AUTOQUEST_DESC(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch));

}

ACMD(do_rpsheet)
{

  send_to_char(ch,
    "Role Playing Sheet for %s\r\n"
    "\r\n"
    "RP Points: %ld\r\n"
    "\r\n"
    "RP Experience Bonus  : %d%% bonus to all normal experience gains\r\n"
    "RP Gold Bonus        : %d%% bonus to all gold obtained as treasure or bounties\r\n"
    "RP Account Exp Bonus : %d%% bonus to all account experience obtained from role playing\r\n"
    "RP Quest Point Bonus : %d%% bonus to all quest point gains\r\n"
    "RP Artisan Exp Bonus : %d%% bonus to all artisan experience gains\r\n"
    "RP Crafting Bonus    : %d%% bonus to all critical success rolls when crafting\r\n"
    "\r\n",

    GET_NAME(ch),
    GET_RP_POINTS(ch),
    get_rp_bonus(ch, RP_EXP), //GET_RP_EXP_BONUS(ch),
    get_rp_bonus(ch, RP_GOLD), //GET_RP_GOLD_BONUS(ch),
    get_rp_bonus(ch, RP_ACCOUNT), //GET_RP_ACCOUNT_EXP(ch) * 5,
    get_rp_bonus(ch, RP_QUEST), //GET_RP_QP_BONUS(ch),
    get_rp_bonus(ch, RP_ART_EXP), //GET_RP_ART_EXP_BONUS(ch) / 100,
    get_rp_bonus(ch, RP_CRAFT) //GET_RP_CRAFT_BONUS(ch) / 10, GET_RP_CRAFT_BONUS(ch) % 10
    );

}

ACMD(do_laston) {

}

ACMD(do_post_forums)
{
  send_to_char(ch, "All bugs, ideas and typos should be posted on the forums at http://www.ageofdragons.com/ .\r\n");
}

ACMD(do_gatherinfo)
{

  if (GET_GATHER_INFO(ch) > 0) {
    send_to_char(ch, "You can try to gather information again in %d minutes and %d seconds.\r\n", GET_GATHER_INFO(ch) / 10,
                 (GET_GATHER_INFO(ch) % 10) * 6);
    return;
  }

  char query[400]={'\0'};
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  MYSQL_RES *res2 = NULL;
  MYSQL_ROW row2 = NULL;
  char arg[200]={'\0'};
  sbyte found = FALSE;
  int dc = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You must specify the name of a person you wish to gather information on.\r\n");
    return;
  }

  // Open mysql connection
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
    log("Cannot connect to mysql database in gather info.");
  }

  sprintf(query, "SELECT idnum, title, level, classes, race, alignment, deity, clan, clan_rank FROM player_data WHERE name = '%s'", CAP(arg));
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL) {
    if ((row = mysql_fetch_row(res)) != NULL) {
        found = TRUE;
    }
  }

  if (!found) {
    send_to_char(ch, "You couldn't locate any information about that person.\r\n");
    return;
  }

  dc = 10;
  if ((GET_ALIGNMENT(ch) > 0 && strstr(row[5], "Good")) || (GET_ALIGNMENT(ch) < 0 && strstr(row[5], "Evil")) || 
      (GET_ALIGNMENT(ch) == 0 && strstr(row[5], "Neutral") && !is_abbrev(row[5], "Neutral")))
    dc += 0;
  else
    dc += 5;
  if (!strcmp(race_list[GET_RACE(ch)].name, row[4]))
    dc += 0;
  else
    dc += 5;
  dc += (35 - atoi(row[2])) / 2;
  
  if (!has_intro_idnum(ch, (int) atoi(row[0])) && skill_roll(ch, SKILL_DIPLOMACY) < dc) {
    send_to_char(ch, "You couldn't find any information on that person.  You will not be able to use gather information again for 10 minutes.\r\n");
    GET_GATHER_INFO(ch) = 100;
    save_char(ch);
    return;
  }

  int idnum = atoi(row[0]);

  send_to_char(ch, 
               "Your investigation uncovers information about %s.\r\n"
               "\r\n"
               "Title: %s\r\n"
               "Race: %s\r\n"
               "Rank: %s\r\n"
               "Professions: %s\r\n"
               "Alignment: %s\r\n"
               "Deity: %s\r\n"
               "Clan: %s\r\n"
               "Clan Rank: %s\r\n"
               "\r\n",
               CAP(arg), row[1], CAP(row[4]), row[2], row[3], row[5], row[6], row[7], row[8]);

  found = FALSE;

  mysql_free_result(res);

  sprintf(query, "SELECT background_story, personality, background_length, personality_length FROM player_extra_data WHERE name = '%s' AND "
          "background_story != '' AND personality != '' AND background_length != '0' AND personality_length != '0'", CAP(arg));
  mysql_query(conn, query);
  res2 = mysql_use_result(conn);
  if (res2 != NULL) {
    if ((row2 = mysql_fetch_row(res2)) != NULL) {
        found = TRUE;
    }
  }
   

  if (found && row2[2] != NULL && row2[3] != NULL)
  {
    int bgl = atoi(row2[2]);
    row2[0][MAX(bgl, 0)] = '\0';
    int prl = atoi(row2[3]);
    row2[1][MAX(prl, 0)] = '\0';

    send_to_char(ch, 
                 "@WPersonality:@n\r\n\r\n");

    int len = 0;

    char *temp = strtok(row2[1], " ");
    while (temp != NULL) {
      send_to_char(ch, "%s ", temp);
      len += strlen(temp);
      if (strstr(temp, "\n"))
        len = 0;
      if (len > 70) {
        len = 0;
        send_to_char(ch, "\r\n");
      }
      temp = strtok(NULL, " ");
    }


    send_to_char(ch, 
                 "\r\n"
                 "\r\n"
                 "@WBackground:@n\r\n\r\n");

    len = 0;

    temp = strtok(row2[0], " ");
    while (temp != NULL) {
      send_to_char(ch, "%s ", temp);
      len += strlen(temp);
      if (strstr(temp, "\n"))
        len = 0;
      if (len > 70) {
        len = 0;
        send_to_char(ch, "\r\n");
      }
      temp = strtok(NULL, " ");
    }


    send_to_char(ch, 
                 "\r\n"
                 "\r\n");
  }
  mysql_free_result(res2);

  if (!has_intro_idnum(ch, idnum)) {
    send_to_char(ch, "\r\nYou can try to gather information again in 10 minutes.\r\n");
    GET_GATHER_INFO(ch) = 100;
    save_char(ch);
  }

  mysql_close(conn);

}

ACMD(do_eqstats) 
{

  int i, j, k, exception = FALSE;
  int found = FALSE;
  struct obj_data *obj = NULL;
  char buf2[200]={'\0'}, bitbuf[200]={'\0'};

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS - 1; i++) {
    found = FALSE;
    exception = FALSE;
    for (j = 0; j < NUM_EQ_SHORT_EXCEPTIONS; j++)
      if (eq_short_exceptions[j] == eq_order[i])
        exception = TRUE;
    if (exception)
      continue;
    if (GET_EQ(ch, eq_order[i])) {
      obj = GET_EQ(ch, eq_order[i]);
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, eq_order[i]))) {
        send_to_char(ch, "%-30s", wear_where[eq_order[i]]);
        for (k = 0; k < MAX_OBJ_AFFECT; k++) {
          if ((obj->affected[k].location != APPLY_NONE) &&
              (obj->affected[k].modifier != 0)) {
            if (!found) {
              found = TRUE;
            }
            sprinttype(obj->affected[k].location, apply_types, bitbuf, sizeof(bitbuf));
            switch (obj->affected[k].location) {
            case APPLY_FEAT:
              snprintf(buf2, sizeof(buf2), " (%s)", feat_list[obj->affected[k].specific].name);
              break;
            case APPLY_SKILL:
              snprintf(buf2, sizeof(buf2), " (%s)", spell_info[obj->affected[k].specific].name);
              break;
            default:
              buf2[0] = 0;
              break;
            }
            send_to_char(ch, " %s%s %s%d", bitbuf, buf2,
                         (obj->affected[k].modifier > 0) ? "+" : "", obj->affected[k].modifier);
          }
        }
        send_to_char(ch, "\r\n");
      } else {
        send_to_char(ch, "%-30s", wear_where[eq_order[i]]);
        send_to_char(ch, "Something.\r\n");
      }
    } else {
      if (!GET_EQ(ch, eq_order[i])) {
        send_to_char(ch, "%-30s<@Dempty@n>\r\n", wear_where[eq_order[i]]);
      }
    }
  }



}

ACMD(do_output) 
{
  char one[50]={'\0'}, two[50]={'\0'};
  two_arguments(argument, one, two);
  if (!*one) {
    send_to_char(ch, "Current output options are:\r\n"
                     "-- combat (full | normal | sparse)\r\n"
                );
    return;
  }
  if (strlen(one) > 50) {
    send_to_char(ch, "The command parameters cannot exceed 50 characters.\r\n");
    return;
  }
  if (!*two) {
    send_to_char(ch, "Current output options are:\r\n"
                     "-- combat (full | normal | sparse)\r\n"
                );
    return;
  }
  if (strlen(two) > 50) {
    send_to_char(ch, "The command parameters cannot exceed 50 characters.\r\n");
    return;
  }
  if (is_abbrev(one, "combat")) {
    if (is_abbrev(two, "full")) {
      ch->combat_output = OUTPUT_FULL;
      send_to_char(ch, "Combat output set to 'full'.\r\n");
      return;
    } else if (is_abbrev(two, "normal")) {
      ch->combat_output = OUTPUT_NORMAL;
      send_to_char(ch, "Combat output set to 'normal'.\r\n");
      return;
    } else if (is_abbrev(two, "sparse") || FALSE) {
      ch->combat_output = OUTPUT_SPARSE;
      send_to_char(ch, "Combat output set to 'sparse'.\r\n");
      return;
    } else {
      send_to_char(ch, "Current output options are:\r\n"
                       "-- combat (full | normal | sparse)\r\n"
                  );
      return;
    }
  } else {
    send_to_char(ch, "Current output options are:\r\n"
                     "-- combat (full | normal | sparse)\r\n"
                );
    return;
  }
}
char *get_weapon_dam(struct char_data *ch)
{
  char main_dam_text[200]={'\0'}, off_dam_text[200]={'\0'};

  sprintf(main_dam_text, "%dd%d%s%d", get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD1), 1), get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD1), 2),
          (get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD1)) >= 0) ? "+" : "-", get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD1)));
  if (GET_EQ(ch, WEAR_WIELD1) && IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0)].weaponFlags, WEAPON_FLAG_DOUBLE))
    sprintf(off_dam_text, "%dd%d%s%d", get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD1), 1), get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD1), 2),
          (get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD1)) >= 0) ? "+" : "-", get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD1)));
  else
    sprintf(off_dam_text, "%dd%d%s%d", get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD2), 1), get_dam_dice_size(ch, GET_EQ(ch, WEAR_WIELD2), 2),
          (get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD2)) >= 0) ? "+" : "-", get_damage_mod(ch, GET_EQ(ch, WEAR_WIELD2)));

  char dam_text[400]={'\0'};

  sprintf(dam_text, "%s%s%s", main_dam_text, ((GET_EQ(ch, WEAR_WIELD1) && IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) ||
                              GET_EQ(ch, WEAR_WIELD2)) ? "/" : "", ((GET_EQ(ch, WEAR_WIELD1) && IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) ||
                              GET_EQ(ch, WEAR_WIELD2)) ? off_dam_text : "");

  return strdup(dam_text);
}

char *get_attack_text(struct char_data *ch)
{

  int attack = 0, base_attack = 0, offhand = 0, weaponmod = 0, offhandmod = 0;
  int i = 0, j = 0;
  char attack_text[200]={'\0'};

  // Determine number of attacks and their attack values
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD1) && (GET_EQ(ch, WEAR_WIELD1)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD1)) == ITEM_WEAPON)
        weaponmod = GET_EQ(ch, WEAR_WIELD1)->affected[j].modifier;
    }
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (GET_EQ(ch, WEAR_WIELD2) && (GET_EQ(ch, WEAR_WIELD2)->affected[j].location == APPLY_ACCURACY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
        offhandmod = GET_EQ(ch, WEAR_WIELD2)->affected[j].modifier;
      else if (GET_EQ(ch, WEAR_WIELD1) && IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0)].weaponFlags, WEAPON_FLAG_DOUBLE) && 
              GET_EQ(ch, WEAR_WIELD1)->affected[j].location == APPLY_ACCURACY && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD1)) == ITEM_WEAPON)
        offhandmod = GET_EQ(ch, WEAR_WIELD1)->affected[j].modifier;
    }  
  

    sprintf(attack_text, "(");
    attack = compute_base_hit(ch, weaponmod);
    base_attack = GET_ACCURACY_BASE(ch);
    offhand = compute_base_hit(ch, offhandmod);
    for (i = 0; i < 4; i++) {

      if (i != 0 && base_attack > 0)
        sprintf(attack_text, "%s/", attack_text);
      if (base_attack > 0) {
        sprintf(attack_text, "%s%s%d", attack_text, (attack > 0) ? "+" : "", attack);
        if (i == 0 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON) {
          sprintf(attack_text, "%s/%s%d", attack_text, (offhand > 0) ? "+" : "", offhand);
        }
        if (i == 1 && GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON && HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING)) {
          offhand = compute_base_hit(ch, offhandmod) - 5;
          sprintf(attack_text, "%s/%s%d", attack_text, (offhand > 0) ? "+" : "", offhand);
        }
        if (i == 0 && (AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS) && GET_CLASS_RANKS(ch, CLASS_MONK) && !GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2))) {
          sprintf(attack_text, "%s/%s%d", attack_text, (attack > 0) ? "+" : "", attack);
        }
      }
      attack -= 5;
      base_attack -= 5;
    }
    sprintf(attack_text, "%s)", attack_text);

  return strdup(attack_text);
}

ACMD (do_show_combat)
{
    // For testing initiative calculations.
    int feat = 0;
    int race_mod = 0;
    int misc_mod = 0;
    int initiative_total = 0;
    
    if (HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE))
        feat += 4;
    
    initiative_total = dex_mod_capped(ch) + feat + race_mod + misc_mod;
    
    send_to_char(ch, "\t[B321]\tBInitiative\tn\r\n");
    send_to_char(ch, "\t[B202]\tW[%4s] + [%4s] + [%4s] + [%4s]    %s\tn\r\n", "Dex", "Feat", "Race", "Misc", "Total");
    send_to_char(ch, "[ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] = [ %2d ]\r\n", dex_mod_capped(ch), feat, race_mod, misc_mod, initiative_total);
    send_to_char(ch, "\r\n");

    // Show saving throws portion.
    int fort_class = 0;
    int fort_feat_mod = 0;
    int fort_magic = 0;
    int fort_misc = 0;

    int ref_class = 0;
    int ref_feat_mod = 0;
    int ref_magic = 0;
    int ref_misc = 0;

    int will_class = 0;
    int will_feat_mod = 0;
    int will_magic = 0;
    int will_misc = 0;

    // To do:  Need to work in calculation for FEAT_STEADFAST_DETERMINATION (which uses CON instead of WIS for WILL saves.)

    // Calculate class bonus.
    fort_class = GET_SAVE(ch, SAVING_FORTITUDE);
    ref_class = GET_SAVE(ch, SAVING_REFLEX);
    will_class = GET_SAVE(ch, SAVING_WILL);

    // Calculate feat bonus.
    if (HAS_FEAT(ch, FEAT_GREAT_FORTITUDE))
        fort_feat_mod += 2;
    if (HAS_FEAT(ch, FEAT_LIGHTNING_REFLEXES))
        ref_feat_mod += 2;
    if (HAS_FEAT(ch, FEAT_IRON_WILL))
        will_feat_mod += 2;

    // Calculate magic bonus.
    if (affected_by_spell(ch, SPELL_PRAYER))
        {
            fort_magic += 1;
            ref_magic += 1;
            will_magic += 1;
        }
    if (affected_by_spell(ch, SPELL_BESTOW_CURSE_PENALTIES))
        {
            fort_magic -= 4;
            ref_magic -= 4;
            will_magic -= 4;
        }

    // Calculate misc bonus.
    if (IS_HALFLING(ch))
    {
        fort_misc += 1;
        ref_misc += 1;
        will_misc += 1;
    }
    if (HAS_FEAT(ch, FEAT_DIVINE_GRACE) || HAS_FEAT(ch, FEAT_DARK_BLESSING))
    {    
        fort_misc += ability_mod_value(GET_CHA(ch));
        ref_misc += ability_mod_value(GET_CHA(ch));
        will_misc += ability_mod_value(GET_CHA(ch));
    }

    send_to_char(ch, "\t[B321]\tBSaving Throws\tn\r\n");
    send_to_char(ch, "\t[B202]%11s\tWClass     Stat     Feat     Magic    Misc     Total\tn\r\n", "");
    send_to_char(ch, "%-9s  [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] = [ %2d ]\r\n", "Fortitude", fort_class, ability_mod_value(GET_CON(ch)), fort_feat_mod, fort_magic, fort_misc, get_saving_throw_value(ch, SAVING_FORTITUDE) );
    send_to_char(ch, "%-9s  [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] = [ %2d ]\r\n", "Reflex", ref_class, ability_mod_value(GET_DEX(ch)), ref_feat_mod, ref_magic, ref_misc, get_saving_throw_value(ch, SAVING_REFLEX) );
    send_to_char(ch, "%-9s  [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] + [ %2d ] = [ %2d ]\r\n", "Will", will_class, ability_mod_value(GET_WIS(ch)), will_feat_mod, will_magic, will_misc, get_saving_throw_value(ch, SAVING_WILL) );
}

