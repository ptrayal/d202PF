/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "spec_procs.h"
#include "feats.h"
#include "oasis.h"
#include "pets.h"
#include "house.h"
#include "player_guilds.h"
#include "grid.h"

/*   external vars  */
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];
extern struct guild_info_type guild_info[];
extern const char *size_names[];
extern int mining_nodes;
extern int farming_nodes;
extern int hunting_nodes;
extern int foresting_nodes;
extern int orphans;
extern int lockboxes;


/* extern functions */
int is_innate_ready(struct char_data *ch, int spellnum);
ACMD(do_fix);
int get_license_fee(struct obj_data *obj, struct char_data *ch);
int valid_crafting_descs(int vnum, char *string);
char *list_crafting_descs(int vnum);
void scaleup_dam(int *num, int *size);
void scaledown_dam(int *num, int *size);
int level_exp(int level, int race);
int stats_disp_menu(struct descriptor_data *d);
ACMD(do_spells);
ACMD(do_drop);
void award_lockbox_treasure(struct char_data *ch, int level);
void determine_treasure(struct char_data *ch, struct char_data *mob);
ACMD(do_tell);
ACMD(do_gen_door);
ACMD(do_kick);
ACMD(do_say);
ACMD(do_action);
int art_level_exp(int level);
ACMD(do_trip);
void update_encumberance(struct char_data *ch);
ACMD(do_value);
ACMD(do_feint);
void gain_level(struct char_data *ch, int whichclass);
int findslotnum(struct char_data *ch, int spelllvl);
int spell_in_book(struct obj_data *obj, int spellnum);
char * change_coins(int coins);
void prune_crlf(char *txt);
ASPELL(spell_identify);
void convert_coins(struct char_data *ch);
void set_armor_values(struct obj_data *obj, int type);
void set_weapon_values(struct obj_data *obj, int type);
int apply_gold_cost[NUM_APPLIES+1];
int set_object_level(struct obj_data *obj);
void award_lockbox_treasure(struct char_data *ch, int level);
int combat_skill_roll(struct char_data *ch, int skillnum);
ACMD(do_set_stats);

/* local functions */
SPECIAL(crafting_quest);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(questmaster);
SPECIAL(mayor);
void npc_steal(struct char_data *ch, struct char_data *victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(wizard);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(bounty_contractor);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(library_small);
SPECIAL(library_medium);
SPECIAL(library_large);
int wizard_cast_buff(struct char_data *ch);
int wizard_cast_spell(struct char_data *ch, struct char_data *vict);
int cleric_cast_buff(struct char_data *ch);
int cleric_cast_spell(struct char_data *ch, struct char_data *vict);
int cleric_cast_cure(struct char_data *ch);
SPECIAL(postmaster);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(no_spec);
SPECIAL(cleric);
SPECIAL(fighter);
int fighter_perform_action(struct char_data *ch, struct char_data *vict);
SPECIAL(rogue);
int rogue_perform_action(struct char_data *ch, struct char_data *vict);
SPECIAL(respec);
SPECIAL(stable);
SPECIAL(identify_mob);
SPECIAL(auto_equip_newbie);
SPECIAL(read_rules);
/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(read_rules)
{
    int i = 0;

    if (CMD_IS("north") && GET_CLASS_LEVEL(ch) == 0 && ch->desc && ch->desc->account && ch->desc->account->read_rules == 0) {
        for (i = 0; i < NUM_RULES; i++) 
        {
            if (ch->player_specials->rules_read[i] == FALSE) 
            {
                send_to_char(ch, "You must read all of the rules before you are able to move on.  Type @Yhelp rules@n to begin.\r\n");
                return 1;
            }
        }
    }
    if (ch->desc && ch->desc->account)
    {
        ch->desc->account->read_rules = 1;
    }

    return 0;
}

SPECIAL(enforce_chargen)
{

  if ((CMD_IS("west") || CMD_IS("east")) && GET_CLASS_LEVEL(ch) == 0) {
    send_to_char(ch, "You must complete character generation first.  You can begin this by typing 'north'.\r\n");
    return 1;
  }

  return 0;
}

SPECIAL(buy_items)
{
  struct obj_data *obj;
  char buf[100]={'\0'};
  char arg2[100]={'\0'};
  int is_weapon = FALSE;
  int is_armor = FALSE;
  int i = 0;

  if (IS_NPC(ch) || (!CMD_IS("buy") && !CMD_IS("list") && !CMD_IS("sell")))
    return 0;

  if (CMD_IS("buy"))
  {
    skip_spaces(&argument);

    if (!*argument)
    {
      send_to_char(ch, "Type list to see what is available or specify what you would like to buy.\r\n");
      return 1;
    }

    for (i = 0; i <= (NUM_WEAPON_TYPES); i++) {
      if (is_abbrev(argument, weapon_list[i].name)) {
        is_weapon = TRUE;
        break;
      }
    }

    if (!is_weapon) {
      for (i = 0; i < (NUM_SPEC_ARMOR_TYPES); i++) {
        if (is_abbrev(argument, armor_list[i].name)) {
          is_armor = TRUE;
          break;
        }
      }
    }

    CREATE(obj, struct obj_data, 1);


    if (is_weapon && GET_GOLD(ch) >= weapon_list[i].cost) {

      obj = read_object(30018, VIRTUAL);
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

      GET_GOLD(ch) -= weapon_list[i].cost;

      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);

      sprintf(buf, "%s %s", a_or_an(weapon_list[i].name), weapon_list[i].name);
      obj->short_description = strdup(buf);
      sprintf(buf, "%s %s lies here.", a_or_an(weapon_list[i].name), weapon_list[i].name);
      obj->description = strdup(buf);
      sprintf(buf, "%s %s", weapon_list[i].name, material_names[weapon_list[i].material]);
      obj->name = strdup(buf);
      GET_OBJ_TYPE(obj) = ITEM_WEAPON;
      set_weapon_values(obj, i);
    }
    else if (is_armor && GET_GOLD(ch) >= armor_list[i].cost) {

      GET_GOLD(ch) -= armor_list[i].cost;

      if (armor_list[i].armorType == ARMOR_TYPE_SHIELD) {

        obj = read_object(30013, VIRTUAL);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);


        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_SHIELD);

        sprintf(buf, "%s %s", a_or_an(armor_list[i].name), armor_list[i].name);
        obj->short_description = strdup(buf);
        sprintf(buf, "%s %s lies here.", a_or_an(armor_list[i].name), armor_list[i].name);
        obj->description = strdup(buf);
        sprintf(buf, "%s %s", armor_list[i].name, material_names[armor_list[i].material]);
        obj->name = strdup(buf);
      }
      else {

        obj = read_object(30000, VIRTUAL);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY);

        sprintf(buf, "a suit of %s",  armor_list[i].name);
        obj->short_description = strdup(buf);
        sprintf(buf, "A suit of %s lies here.", armor_list[i].name);
        obj->description = strdup(buf);
        sprintf(buf, "%s %s", armor_list[i].name, material_names[armor_list[i].material]);
        obj->name = strdup(buf);
      }
      GET_OBJ_TYPE(obj) = ITEM_ARMOR;
      set_armor_values(obj, i);
    }
    else if (!is_armor && !is_weapon) {
      send_to_char(ch, "That is not a proper item name.  Please type list to see what is available.\r\n");
      return 1;
    }
    else {
      send_to_char(ch, "You need %d more gold to purchase that item.\r\n", (is_weapon ? weapon_list[i].cost : armor_list[i].cost) - GET_GOLD(ch));
      return 1;
    }
    obj_to_char(obj, ch);
    send_to_char(ch, "You purchase %s for %d %s.\r\n", obj->short_description, GET_OBJ_COST(obj), MONEY_STRING);
    return 1;
  }
  else if (CMD_IS("sell"))
  {

    if (GET_CLASS_LEVEL(ch) > 1 && GET_ADMLEVEL(ch) == 0)
      return 0;

    skip_spaces(&argument);

    if (!*argument) {
      send_to_char(ch, "What do you want to sell?\r\n");
      return 1;
    }

    if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
      send_to_char(ch, "There is nothing in your inventory by the name of %s.\r\n", argument);
      return 1;
    }

    GET_GOLD(ch) += GET_OBJ_COST(obj) / 4;
    send_to_char(ch, "You are reimbursed %d %s for selling %s.\r\n", GET_OBJ_COST(obj) / 4, MONEY_STRING, obj->short_description);
    obj_from_char(obj);
    extract_obj(obj);
    return 1;
  }
  else {
    two_arguments(argument, buf, arg2);

    if (!*buf) {
      send_to_char(ch, "Would you like to list armor or weapons? list <armor|weapons> <simple|martial|exotic|light|medium|heavy|shields>\r\n");
      return 1;
    }

    if (!*arg2) {
      send_to_char(ch, "What subtype would you like to list? list <armor|weapons> <simple|martial|exotic|light|medium|heavy|shields>\r\n");
      return 1;
    }

    if (is_abbrev(buf, "weapons")) {
      if (is_abbrev(arg2, "simple")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Simple Weapon Type", "Item Cost");
        for (i = 0; i <= (NUM_WEAPON_TYPES);i++)
          if (IS_SET(weapon_list[i].weaponFlags, WEAPON_FLAG_SIMPLE))
            send_to_char(ch, "%-20s %-10d\r\n", weapon_list[i].name, weapon_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else if (is_abbrev(arg2, "martial")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Martial Weapon Type", "Item Cost");
        for (i = 0; i <= (NUM_WEAPON_TYPES);i++)
          if (IS_SET(weapon_list[i].weaponFlags, WEAPON_FLAG_MARTIAL))
            send_to_char(ch, "%-20s %-10d\r\n", weapon_list[i].name, weapon_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else if (is_abbrev(arg2, "exotic")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Exotic Weapon Type", "Item Cost");
        for (i = 0; i <= (NUM_WEAPON_TYPES);i++)
          if (IS_SET(weapon_list[i].weaponFlags, WEAPON_FLAG_EXOTIC))
            send_to_char(ch, "%-20s %-10d\r\n", weapon_list[i].name, weapon_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else {
        send_to_char(ch, "What subtype would you like to list? list weapons <simple|martial|exotic>\r\n");
        return 1;
      }
    }
    else if (is_abbrev(buf, "armor")) {
      if (is_abbrev(arg2, "light")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Light Armor Type", "Item Cost");
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES;i++)
          if (armor_list[i].armorType == ARMOR_TYPE_LIGHT)
            send_to_char(ch, "%-20s %-10d\r\n", armor_list[i].name, armor_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else if (is_abbrev(arg2, "medium")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Medium Armor Type", "Item Cost");
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES;i++)
          if (armor_list[i].armorType == ARMOR_TYPE_MEDIUM)
            send_to_char(ch, "%-20s %-10d\r\n", armor_list[i].name, armor_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else if (is_abbrev(arg2, "heavy")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Heavy Armor Type", "Item Cost");
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES;i++)
          if (armor_list[i].armorType == ARMOR_TYPE_HEAVY)
            send_to_char(ch, "%-20s %-10d\r\n", armor_list[i].name, armor_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else if (is_abbrev(arg2, "shields")) {
        send_to_char(ch, "%-20s %-10s\r\n-------------------- ----------\r\n", "Shield Type", "Item Cost");
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES;i++)
          if (armor_list[i].armorType == ARMOR_TYPE_SHIELD)
            send_to_char(ch, "%-20s %-10d\r\n", armor_list[i].name, armor_list[i].cost);
        send_to_char(ch, "\r\nTo buy something from this list type @Ybuy <item name>@n\r\n");
        return 1;
      }
      else {
        send_to_char(ch, "What subtype would you like to list? list armor <light|medium|heavy|shields>\r\n");
        return 1;
      }
    }
    else {
      send_to_char(ch, "Would you like to list armor or weapons? list <armor|weapons> <simple|martial|exotic|light|medium|heavy|shields>\r\n");
      return 1;
    }

  }

  return 1;
}

SPECIAL(set_descs)
{
  struct descriptor_data *d = ch->desc;

  if (IS_NPC(ch) || (!CMD_IS("setdescs") && !CMD_IS("north")))
    return 0;

  if (GET_CLASS_LEVEL(ch) > 0 && GET_ADMLEVEL(ch) == 0)
    return 0;

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot set your descriptions until after it occurs.  The wait should not be long.\r\n");
    return 1;
  }

  if (CMD_IS("north")) {
    if (!GET_PC_DESCRIPTOR_1(ch))
      send_to_char(ch, "You must set your description before you can proceed.\r\n");
    else {
      char_from_room(ch);
      char_to_room(ch, real_room(30019));
      look_at_room(IN_ROOM(ch), ch, 0);
    }
    return 1;
  }

  SEND_TO_Q("Now you are required to enter some descriptions in order to make your character\r\n", d);
  SEND_TO_Q("playable in our description-based game.  The first descriptions you enter are\r\n", d);
  SEND_TO_Q("mandatory at this stage.  They are based on a series of features you choose for\r\n", d);
  SEND_TO_Q("your character from lists.  The second set of descriptions are optional at\r\n", d);
  SEND_TO_Q("this point, though they need to be entered at some stage.  The second set of\r\n", d);
  SEND_TO_Q("descriptions will be your permanent descriptions and are completely custom-written\r\n", d);
  SEND_TO_Q("by you, but they need to be approved by a staff member.  Until the second set are\r\n", d);
  SEND_TO_Q("written and approved, you will use the first set, the ones you are about to do now,\r\n", d);
  SEND_TO_Q("and until the second set are written and approved, you will only be able to level\r\n", d);
  SEND_TO_Q("up to level 10.  This is usually about 15-30+ hours of game play so it should\r\n", d);
  SEND_TO_Q("be plenty of time for you to come up with the descriptions and have an immortal\r\n", d);
  SEND_TO_Q("approve them.  Don't forget as well, that you are required to write at least a few\r\n", d);
  SEND_TO_Q("sentences of a character background or personality profile to be sent to the staff\r\n", d);
  SEND_TO_Q("for approval as well.\r\n", d);
  SEND_TO_Q("\r\n", d);
  SEND_TO_Q("So without further ado, let's get ready to enter your descriptions.  You will first\r\n", d);
  SEND_TO_Q("choose an initial descrtiptor type from the list below.  Then you will choose a word\r\n", d);
  SEND_TO_Q("or phrase to describe it.  At this point you can accept your description or customize\r\n", d);
  SEND_TO_Q("it further with a second descriptor and describing word/phrase.  Try to keep your\r\n", d);
  SEND_TO_Q("description somewhat short as this will be used in place of your name in any situation\r\n", d);
  SEND_TO_Q("where someone has not been told your name using the introduce command.\r\n", d);
  SEND_TO_Q("\r\n", d);
  SEND_TO_Q("@Y(Press Enter to Continue)@n\r\n", d);

  STATE(d) = CON_GEN_DESCS_INTRO;

  return 1;
}

SPECIAL(set_stats)
{

  if (IS_NPC(ch) || (!CMD_IS("setstats") && !CMD_IS("north")))
    return 0;

  if (CMD_IS("setstats") && GET_ROOM_VNUM(IN_ROOM(ch)) == 30003)
  {
    do_set_stats(ch, argument, 0, 0);
    return 1;
  }

  if (GET_CLASS_LEVEL(ch) > 0)
    return 0;

  if (ch->real_abils.str + ch->real_abils.intel + ch->real_abils.wis + ch->real_abils.con + ch->real_abils.dex + ch->real_abils.cha == 60 && ch->stat_points_given == FALSE)
  {
    GET_STAT_POINTS(ch) = 20;
    ch->stat_points_given = TRUE;
  }

  if (CMD_IS("north"))
  {
    if (ch->real_abils.str + ch->real_abils.intel + ch->real_abils.wis + ch->real_abils.con + ch->real_abils.dex + ch->real_abils.cha == 60)
      send_to_char(ch, "You must set your ability scores before you can proceed.\r\n");
    else {
      char_from_room(ch);
      char_to_room(ch, real_room(30004));
      look_at_room(IN_ROOM(ch), ch, 0);
    }
    return 1;
  }

  char arg1[100]={'\0'};
  char arg2[100]={'\0'};

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
  {
    send_to_char(ch, "Which stat do you want to adjust?\r\n");
    return 1;
  }
  if (!*arg2 && !is_abbrev("show", arg1) && !is_abbrev("reset", arg1))
  {
    send_to_char(ch, "What do you wish to change the stat to?\r\n");
    return 1;
  }

  if (is_abbrev(arg1, "reset"))
  {
    send_to_char(ch, "You reset your stats back to the default values.\r\n");
    GET_STAT_POINTS(ch) = 20;
    ch->real_abils.str = 10;
    ch->real_abils.dex = 10;
    ch->real_abils.con = 10;
    ch->real_abils.intel = 10;
    ch->real_abils.wis = 10;
    ch->real_abils.cha = 10;
    return 1;
  }
  else if (is_abbrev(arg1, "show"))
  {
    send_to_char(ch, "Here are your current stats:\r\n");
    send_to_char(ch, "Strength     : %d\r\n", ch->real_abils.str);
    send_to_char(ch, "Dexterity    : %d\r\n", ch->real_abils.dex);
    send_to_char(ch, "Constitution : %d\r\n", ch->real_abils.con);
    send_to_char(ch, "Intelligence : %d\r\n", ch->real_abils.intel);
    send_to_char(ch, "Wisdom       : %d\r\n", ch->real_abils.wis);
    send_to_char(ch, "Charisma     : %d\r\n", ch->real_abils.cha);
    send_to_char(ch, "Stat Points  : %d\r\n", GET_STAT_POINTS(ch));
    return 1;
  }
  if (GET_STAT_POINTS(ch) == 0)
  {
    send_to_char(ch, "You need to type @Ysetstats reset@n if you want to change your stats after using all of your points.\r\n");
    return 1;
  }
  if (is_abbrev(arg1, "strength"))
  {
    ch->real_abils.str = stat_assign_stat(ch->real_abils.str, arg2, ch);
  }
  else if (is_abbrev(arg1, "dexterity"))
  {
    ch->real_abils.dex = stat_assign_stat(ch->real_abils.dex, arg2, ch);
  }
  else if (is_abbrev(arg1, "constitution"))
  {
    ch->real_abils.con = stat_assign_stat(ch->real_abils.con, arg2, ch);
  }
  else if (is_abbrev(arg1, "intelligence"))
  {
    ch->real_abils.intel = stat_assign_stat(ch->real_abils.intel, arg2, ch);
  }
  else if (is_abbrev(arg1, "wisdom"))
  {
    ch->real_abils.wis = stat_assign_stat(ch->real_abils.wis, arg2, ch);
  }
  else if (is_abbrev(arg1, "charisma"))
  {
    ch->real_abils.cha = stat_assign_stat(ch->real_abils.cha, arg2, ch);
  }
  else
  {
    send_to_char(ch, "That is not a valid ability score.\r\n");
  }

  send_to_char(ch, "Strength     : %d\r\n", ch->real_abils.str);
  send_to_char(ch, "Dexterity    : %d\r\n", ch->real_abils.dex);
  send_to_char(ch, "Constitution : %d\r\n", ch->real_abils.con);
  send_to_char(ch, "Intelligence : %d\r\n", ch->real_abils.intel);
  send_to_char(ch, "Wisdom       : %d\r\n", ch->real_abils.wis);
  send_to_char(ch, "Charisma     : %d\r\n", ch->real_abils.cha);
  send_to_char(ch, "Stat Points  : %d\r\n", GET_STAT_POINTS(ch));


  return 1;

}

SPECIAL(select_race)
{
    int i = 0;

    GRID_DATA *grid;
    GRID_ROW *row;

    if (IS_NPC(ch) || (!CMD_IS("setrace") && !CMD_IS("listraces") && !CMD_IS("north")))
        return 0;

    if (GET_CLASS_LEVEL(ch) > 0 && GET_RACE(ch) != RACE_SPIRIT && GET_ADMLEVEL(ch) == 0)
        return 0;

    if (CMD_IS("north"))
    {
        if (GET_RACE(ch) == RACE_SPIRIT)
            send_to_char(ch, "You must choose a race before you can proceed.\r\n");
        else
        {
            char_from_room(ch);
            char_to_room(ch, real_room(30002));
            look_at_room(IN_ROOM(ch), ch, 0);
        }
        return 1;
    }

    if (CMD_IS("setrace"))
    {

        skip_spaces(&argument);

        if (!*argument)
        {
            send_to_char(ch, "Which race would you like to become? Type listraces for a list of available races.\r\n");
            return 1;
        }

        for (i = 0; i < NUM_RACES; i++)
        {
            if (is_abbrev(argument, "undefined"))
            {
                send_to_char(ch, "That is not a valid race.  Type listraces for a list of available races.\r\n");
                return 1;
            }
            else if (race_list[i].name && is_abbrev(argument, race_list[i].name) && race_list[i].is_pc)
            {
                if (race_list[i].is_pc && race_list[i].level_adjustment > 0)
                {
                    if (ch->desc && ch->desc->account)
                    {
                        if (has_unlocked_race(ch, i))
                        {
                            send_to_char(ch, "You have changed your race to %s.  For more information type help race %s\r\n", race_list[i].name, race_list[i].name);
                            GET_REAL_RACE(ch) = i;
                            return 1;
                        }
                        else
                        {
                            send_to_char(ch, "You have not unlocked that race.  See @YHELP ACCOUNT EXPERIENCE@n.\r\n");
                            return 1;
                        }
                    }
                    else
                    {
                        return 1;
                    }
                }

                send_to_char(ch, "You have changed your race to %s.  For more information type help race %s\r\n", race_list[i].name, race_list[i].name);
                GET_REAL_RACE(ch) = i;
                return 1;
            }
        }

        send_to_char(ch, "That is not a valid race.  Type listraces for a list of available races.\r\n");
        return 1;
    }
    else
    {
        // send_to_char(ch, "%-20s %-3s %-3s %-3s %-3s %-3s %-3s %-9s %-16s\r\n-------------------- --- --- --- --- --- --- --------- ----------------\r\n", "Race Name",
        //              "Str", "Con", "Dex", "Int", "Wis", "Cha", "Level Adj", "Account Exp Cost");
        grid = create_grid(75);
        row = create_row(grid);
        row_append_cell(row, 20, "Race Name");
        row_append_cell(row, 6, "STR");
        row_append_cell(row, 6, "CON");
        row_append_cell(row, 6, "DEX");
        row_append_cell(row, 6, "INT");
        row_append_cell(row, 6, "WIS");
        row_append_cell(row, 6, "CHA");
        row_append_cell(row, 19, "Account Cost");

        for (i = 0; i < NUM_RACES; i++)
        {
            if (race_list[i].is_pc)
            {
                row = create_row(grid);
                row_append_cell(row, 20, "%s", race_list[i].type);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[0]);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[1]);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[4]);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[2]);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[3]);
                row_append_cell(row, 6, "%-3d", race_list[i].ability_mods[5]);
                row_append_cell(row, 19, "%-16d", level_exp(race_list[i].level_adjustment + 1, RACE_SPIRIT));
                // send_to_char(ch, "%-20s %-3d %-3d %-3d %-3d %-3d %-3d %-9d %-16d\r\n", race_list[i].type,
                //              race_list[i].ability_mods[0],
                //              race_list[i].ability_mods[1],
                //              race_list[i].ability_mods[4], race_list[i].ability_mods[2], race_list[i].ability_mods[3], race_list[i].ability_mods[5],
                //              race_list[i].level_adjustment, level_exp(race_list[i].level_adjustment + 1, RACE_SPIRIT));
            }
        }
        grid_to_char(grid, ch, TRUE);
        // send_to_char(ch, "\r\n");
        return 1;
    }
}

SPECIAL(select_align)
{
  if (IS_NPC(ch) || (!CMD_IS("setalign") && !CMD_IS("setethos")))
    return 0;

  if (GET_CLASS_LEVEL(ch) > 0 && GET_ADMLEVEL(ch) == 0)
    return 0;

  if (CMD_IS("setalign"))
  {

    skip_spaces(&argument);

    if (!*argument)
    {
      send_to_char(ch, "Which alignment would you like to become? Choose between good, neutral and evil.\r\n");
      return 1;
    }

    if (!strcmp(argument, "good"))
    {
      send_to_char(ch, "You have set your alignment to good.\r\n");
      GET_ALIGNMENT(ch) = 500;
      return 1;
    }
    if (!strcmp(argument, "evil"))
    {
/* Uncomment the following if you wish to make evil a disallowed alignment
      send_to_char(ch,
      "For the time being, evil is not allowed as an alignment.  We are working on\r\n"
      "a new system, as well as supporting zones, to allow players to play characters\r\n"
      "on the side of the necro kings.  We will be having separate chat channels\r\n"
      "for both sides, and neither side will be able to communicate with the other\r\n"
      "in any way.  They will only be able to interact using combat and\r\n"
      "other non-communicative game commands.\r\n"
      "\r\n"
      "In the mean time, please make a character that is either good or neutral\r\n"
      "and who fights on the side of the free kingdoms.  We are also going to be\r\n"
      "redoing the guild system so that the guilds properly reflect the various\r\n"
      "factions and alignments.\r\n"
      "\r\n"
      "Once we have the good/evil system ready and the zones created, we will allow\r\n"
      "people to remake characters who are good or neutral as evil characters\r\n"
      "instead, provided the basic character build idea stays the same.\r\n"
      "Since we are still in development, we will waive the need to choose a new\r\n"
      "name and identity for the player, though if they keep the same name, it\r\n"
      "is expected they will come up with some in-character reason for their fall\r\n"
      "to darkness.\r\n"
      "\r\n"
      "Once evil role playing is allowed, there will be rules put in place to make\r\n"
      "sure that the evil role playing stays within the boundaries of reasonable\r\n"
      "morality.  That is to say we don't want to see sadistic behavior or any kind\r\n"
      "of role playing that would only be allowed in an R-Rated horror movie.\r\n"
      "There have been many villains played in A and PG Rated movies without having to\r\n"
      "lose any kind of the feeling of the epic struggle between good and evil, so we\r\n"
      "expect people to keep their dark fantasies in check and hold to boundaries\r\n"
      "of reasonable morality in their evil role play.  Yes, it seems like a contradiction\r\n"
      "but I think the intent is clear.  If it is not please speak with Seraphime\r\n"
      "in a polite, calm and respectful manner and we will try to resolve things and\r\n"
      "rephrase the contents on this help file if necessary.\r\n"
      "\r\n"
      "In any case, good or evil, if your role playing is bothering anyone and they\r\n"
      "ask you out of character to stop, you must do so without creating a scene or\r\n"
      "causing contention by talking poorly about them behind their backs.  People who\r\n"
      "cause trouble in this manner will be forced to leave the mud.  We are trying to\r\n"
      "accomodate our players and potential players by allowing evil alignments, but\r\n"
      "we are still not going to let things come to a point in any situation where\r\n"
      "an otherwise peaceful player or staff member becomes uncomfortable because of\r\n"
      "immoral behavior (ic or ooc) by another person.\r\n"
      "\r\n"
      "At this time, please choose either a good or neutral alignment.\r\n");
*/
      send_to_char(ch, "You have set your alignment to evil.\r\n");
      GET_ALIGNMENT(ch) = -500;
      return 1;
    }
    if (!strcmp(argument, "neutral"))
    {
      send_to_char(ch, "You have set your alignment to neutral.\r\n");
      GET_ALIGNMENT(ch) = 0;
      return 1;
    }
    else
    {
      send_to_char(ch, "That is not a valid alignment.  Choose either good, neutral or evil.\r\n");
      return 1;
    }
  }

  if (CMD_IS("setethos"))
  {

    skip_spaces(&argument);

    if (!*argument)
    {
      send_to_char(ch, "Which ethos would you like to become? Choose between lawful, neutral and chaotic.\r\n");
      return 1;
    }

    if (!strcmp(argument, "lawful")) {
      send_to_char(ch, "You have set your ethos to lawful.\r\n");
      GET_ETHOS(ch) = 500;
      return 1;
    }
    if (!strcmp(argument, "chaotic")) {
      send_to_char(ch, "You have set your ethos to chaotic.\r\n");
      GET_ETHOS(ch) = -500;
      return 1;
    }
    if (!strcmp(argument, "neutral")) {
      send_to_char(ch, "You have set your ethos to neutral.\r\n");
      GET_ETHOS(ch) = 0;
      return 1;
    }
    else {
      send_to_char(ch, "That is not a valid ethos, choose between lawful, neutral and chaotic.\r\n");
      return 1;
    }
  }
  return 0;
}


SPECIAL(identify_mob)
{

    struct obj_data *obj;
    char arg[MAX_STRING_LENGTH]={'\0'};

    if (!CMD_IS("identify"))
        return FALSE;

    one_argument(argument, arg);

    if (!*arg)
    {
        send_to_char(ch, "What would you like to have identified?\r\n");
        return TRUE;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    {
        send_to_char(ch, "You don't seem to have a %s in your inventory.\r\n", argument);
        return TRUE;
    }

    int cost = GET_OBJ_TYPE(obj) == ITEM_POTION ? GET_OBJ_LEVEL(obj) * 3 : GET_OBJ_LEVEL(obj) * 10;

    if (GET_GOLD(ch) < cost)
    {
        if (GET_BANK_GOLD(ch) < cost)
        {

            send_to_char(ch, "You must have %d %s on hand or in the bank to identify an item.\r\n", cost, MONEY_STRING);
            return TRUE;
        }
        GET_BANK_GOLD(ch) -= cost;
    }
    else
    {
        GET_GOLD(ch) -= cost;
    }

    send_to_char(ch, "Your item has been identified!\r\n");

    convert_coins(ch);

    spell_identify(20, ch, ch, obj, arg);

    return TRUE;
}

SPECIAL(identify_kit)
{

  struct obj_data *obj;
  char arg[MAX_STRING_LENGTH]={'\0'};

  if (!CMD_IS("identify") && !CMD_IS("idcost"))
    return FALSE;

  one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char(ch, "What would you like to have identified?\r\n");
    return TRUE;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    send_to_char(ch, "You don't seem to have a %s in your inventory.\r\n", argument);
    return TRUE;
  }

  int cost = 0;

  switch(GET_OBJ_TYPE(obj))
  {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    cost = 10 * GET_OBJ_LEVEL(obj);
    break;
    default:
    cost = 50 * GET_OBJ_LEVEL(obj);
    break;
  }

  if (CMD_IS("idcost"))
  {
    send_to_char(ch, "It would cost %d %s to identify that item.\r\n", cost, MONEY_STRING);
    return TRUE;
  }

  int bank = 0;

  if (GET_GOLD(ch) < cost)
  {
    if ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) < cost)
    {
      send_to_char(ch, "You do not have enough money on hand and in the bank to identify that item.\r\n");
      return TRUE;
    }
    bank = cost - GET_GOLD(ch);
    GET_GOLD(ch) = 0;
    GET_BANK_GOLD(ch) -= bank;
    send_to_char(ch, "You empty your coin purse into the gnomish machine.  The remainder was withdrawn from your bank for a total cost of %d.\r\n", cost);
  }
  else
  {
    GET_GOLD(ch) -= cost;
    send_to_char(ch, "You dump %d %s into the gnomish machine.\r\n", cost, MONEY_STRING);
  }

  send_to_char(ch, "\r\nThe gnomish machine begins to hum and whir, and finally begins to shake violently before a huge puff of black smoke emerges.\r\n\r\n");
  send_to_char(ch, "Your item has been identified!\r\n");

  spell_identify(20, ch, ch, obj, arg);

  return TRUE;
}

SPECIAL(research)
{

  if (!CMD_IS("research") && !CMD_IS("study") && !CMD_IS("theorize") && !CMD_IS("identify"))
    return 0;

  if (CMD_IS("identify")) {
    struct obj_data *obj;
    char arg[MAX_STRING_LENGTH]={'\0'};

    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "What would you like to have identified?\r\n");
      return TRUE;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't seem to have a %s in your inventory.\r\n", argument);
       return TRUE;
    }

    int cost = GET_OBJ_TYPE(obj) == ITEM_POTION ? GET_OBJ_LEVEL(obj) * 3 : GET_OBJ_LEVEL(obj) * 10;

    cost *= 2;

    if (GET_GOLD(ch) < cost) {
      if (GET_BANK_GOLD(ch) < cost) {
        send_to_char(ch, "You must have %d %s on hand or in the bank to identify an item.\r\n", cost, MONEY_STRING);
        return TRUE;
      }
      GET_BANK_GOLD(ch) -= cost;
    } else {
      GET_GOLD(ch) -= cost;
    }

    send_to_char(ch, "Your item has been identified!\r\n");

    convert_coins(ch);

    spell_identify(20, ch, ch, obj, arg);

    return TRUE;
  }
  if (CMD_IS("study")) {

    if (!is_innate_ready(ch, SPELL_THEORY_TO_PRACTICE)) {
      send_to_char(ch, "Your cooldown on this ability has not yet expired.  See @Ytimers@n command.\r\n");
      return 1;
    }

    int sr = combat_skill_roll(ch, SKILL_KNOWLEDGE);
    int cost = GET_CLASS_RANKS(ch, CLASS_ARTISAN) * GET_CLASS_RANKS(ch, CLASS_ARTISAN) * 10;
    if (!HAS_FEAT(ch, FEAT_THEORY_TO_PRACTICE)) {
      send_to_char(ch, "You must have the theory to practice feat to be able to train in libraries.\r\n");
      return TRUE;
    }
    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "You do not have enough %s on hand to pay for the research materials.\r\n", MONEY_STRING);
      return TRUE;
    }
    if (sr == 0) {
      send_to_char(ch, "You did not learn anything new during your study.\r\n");
      return TRUE;
    }
    gain_gold(ch, -cost, GOLD_ONHAND);
//    int exp = mob_exp_by_level(GET_CLASS_RANKS(ch, CLASS_ARTISAN) + sr);
    int exp = cost * 6;
    send_to_char(ch, "You have learned some new techniques during your study.\r\n");
    gain_exp(ch, exp);
    add_innate_timer(ch, SPELL_THEORY_TO_PRACTICE);
    return TRUE;
  }

  if (CMD_IS("theorize")) {

    if (!is_innate_ready(ch, SPELL_THEORY_TO_PRACTICE)) {
      send_to_char(ch, "Your cooldown on this ability has not yet expired.  See @Ytimers@n command.\r\n");
      return 1;
    }

    int sr = combat_skill_roll(ch, SKILL_KNOWLEDGE);
    int cost = GET_CLASS_RANKS(ch, CLASS_ARTISAN) * GET_CLASS_RANKS(ch, CLASS_ARTISAN) * 25;
    if (!HAS_FEAT(ch, FEAT_THEORY_TO_PRACTICE)) {
      send_to_char(ch, "You must have the theory to practice feat to be able to train in libraries.\r\n");
      return TRUE;
    }
    if (GET_ARTISAN_EXP(ch) < cost) {
      send_to_char(ch, "You do not have enough artisan experience to theorize.\r\n");
      return TRUE;
    }
    if (sr == 0) {
      send_to_char(ch, "You did not learn anything new during your study.\r\n");
      return TRUE;
    }
    GET_ARTISAN_EXP(ch) -= cost;
    int exp = mob_exp_by_level(GET_CLASS_RANKS(ch, CLASS_ARTISAN) + sr);
    send_to_char(ch, "You have learned some new techniques during your theorizing.\r\n");
    gain_exp(ch, exp);
    add_innate_timer(ch, SPELL_THEORY_TO_PRACTICE);
    return TRUE;
  }

  return 1;
  skip_spaces(&argument);
/*
  if (is_abbrev(argument, "list")) {
    list_force_powers(ch);
    return 1;
  }
*/
  int i;

  for (i = 0; i <= TOP_SPELL; i++) {
    if (i == SPELL_DARK_HEALING) continue;
    if (is_abbrev(argument, spell_info[i].name) && spell_info[i].class_level[CLASS_JEDI] == 0)
      break;
  }

  if (i > TOP_SPELL) {
    send_to_char(ch, "There is no force power by that name.\r\n");
    return 1;
  }

  int j;

//  int num_toks = HAS_REAL_FEAT(ch, FEAT_FORCE_TRAINING) * MAX(1, 1 + ability_mod_value(ch->real_abils.wis));
  int num_toks = 0;

  for (j = 0; j < MAX_NUM_KNOWN_SPELLS; j++) {
    if (ch->player_specials->spells_known[j] == i) {
      send_to_char(ch, "You already know that force power.\r\n");
      return 1;
    }
    if (ch->player_specials->spells_known[j] > 0)
      num_toks--;
  }

  if (num_toks <= 0) {
    send_to_char(ch, "You do not have any force power tokens left.\r\n");
    return 1;
  }

  for (j = 0; j < MAX_NUM_KNOWN_SPELLS; j++)
    if (ch->player_specials->spells_known[j] == 0)
      break;

  ch->player_specials->spells_known[j] = i;

  send_to_char(ch, "You have learned the force power: '%s'.  You have %d force tokens left.\r\n", spell_info[i].name, num_toks-1);

  return 1;
}

SPECIAL(library_small)
{
  struct obj_data *obj;
  int i, j, cost =10;
  int spellBookFull = TRUE;
  int researchCheck = FALSE;
  bool found = FALSE;

  if (!CMD_IS("research"))
    return (FALSE);

  skip_spaces(&argument);

  for (i = 0; i < MAX_SPELLS; i++) {
    if (spell_info[i].name != NULL && is_abbrev(argument, spell_info[i].name))
    {
      if (spell_info[i].school != SCHOOL_UNDEFINED)
      {
        GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
        if (findslotnum(ch, spell_info[i].class_level[CLASS_WIZARD]) != -1)
        {
          if (spell_info[i].spell_level <= 2)
          {
            for (j = 1; j < spell_info[i].class_level[CLASS_WIZARD]; j++)
              cost *= 3 ;
            if (GET_GOLD(ch) >= cost || GET_RESEARCH_TOKENS(ch) > 0)
            {
              for (obj = ch->carrying; obj && !found; obj = obj->next_content)
              {
                if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
                {
                  found = TRUE;
                  if (spell_in_book(obj, i))
                  {
                    send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n", spell_info[i].name);
                    return TRUE;
                  }
                  if (!obj->sbinfo)
                  {
                    CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
                    memset((char *) obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
                  }
                  for (j=0; j < SPELLBOOK_SIZE; j++)
                  {
                    if (obj->sbinfo[j].spellname == 0)
                    {
                      spellBookFull = FALSE;
                      break;
                    }
                    else
                    {
                      continue;
                    }
                  }
                  researchCheck = ((dice(1, 20) + ability_mod_value(GET_INT(ch))) > (dice(1, 20) + spell_info[i].spell_level));
                  if (!spellBookFull && ((researchCheck) || (GET_RESEARCH_TOKENS(ch) > 0)))
                  {
                    obj->sbinfo[j].spellname = i;
                    obj->sbinfo[j].pages = MAX(1, spell_info[i].class_level[CLASS_WIZARD] * 2);
                    send_to_char(ch, "Your research is successful and you scribe the spell '%s' into your spellbook, which takes up %d pages.\r\n", spell_info[i].name, obj->sbinfo[j].pages);
                    if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE))
                      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
                    if (GET_RESEARCH_TOKENS(ch) < 1)
                    {
                      GET_GOLD(ch) -= cost / 100;
                      send_to_char(ch, "You were charged %s %s for this research session.\r\n", change_coins(cost), MONEY_STRING);
                    }
                    else
                    {
                      GET_RESEARCH_TOKENS(ch) -= 1;
                      send_to_char(ch, "You used one of your research sessions to learn this spell leaving you with %d.\r\n", GET_RESEARCH_TOKENS(ch));
                    }
                    return TRUE;
                  }
                  else if (spellBookFull)
                  {
                    send_to_char(ch, "Your spellbooks are full, you must buy a new one if you wish to research a new spell.\r\n");
                    return TRUE;
                  }
                  else
                  {
                    GET_GOLD(ch) -= cost / 500;
                    send_to_char(ch, "Your research failed, and the cost in materials was %s %s.\r\n", change_coins(cost / 5), MONEY_STRING);
                    return TRUE;
                  }
                }
              }
              if (!found)
              {
                send_to_char(ch, "You do not have a spellbook to record your spell in.\r\n");
                return TRUE;
              }
            }
            else
            {
              send_to_char(ch, "You need to have at least %s %s to do the research for that spell.\r\n", change_coins(cost), MONEY_STRING);
              return TRUE;
            }
          }
          else
          {
            send_to_char(ch, "This library does not have the resources to research that spell.\r\n");
            return TRUE;
          }
        }
        else
        {
          send_to_char(ch, "The references for that spell are far beyond your understanding.\r\n");
          return TRUE;
        }
        GET_MEM_TYPE(ch) = 0;
      }
      else
      {
        send_to_char(ch, "The library has no references whatsoever for that spell.\r\n");
        return TRUE;
      }
    }
  }

  send_to_char(ch, "The library has no references for that spell.\r\n");
  return TRUE;

}

SPECIAL(library_medium)
{
struct obj_data *obj;
int i, j, cost =10;
int spellBookFull = TRUE;
int researchCheck = FALSE;
bool found = FALSE;

if (!CMD_IS("research"))
return (FALSE);

skip_spaces(&argument);

for (i = 0; i < MAX_SPELLS; i++) {
	if (spell_info[i].name != NULL && is_abbrev(argument, spell_info[i].name)) {
	if (spell_info[i].school != SCHOOL_UNDEFINED) {
                GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
		if (findslotnum(ch, spell_info[i].class_level[CLASS_WIZARD]) != -1) {
			if (spell_info[i].spell_level <= 6) {
			for (j = 1; j < spell_info[i].class_level[CLASS_WIZARD]; j++)
				cost *= 3 ;
			if (GET_GOLD(ch) >= cost || GET_RESEARCH_TOKENS(ch) > 0) {
				for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
					if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
						found = TRUE;
						if (spell_in_book(obj, i)) {
							send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n", spell_info[i].name);
							return TRUE;
						}
						if (!obj->sbinfo) {
							CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
							memset((char *) obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
						}
						for (j=0; j < SPELLBOOK_SIZE; j++) {
							if (obj->sbinfo[j].spellname == 0) {
								spellBookFull = FALSE;
								break;
							} else {
								continue;
							}
						}
						researchCheck = ((dice(1, 20) + ability_mod_value(GET_INT(ch))) > (dice(1, 20) + spell_info[i].spell_level));
						if (!spellBookFull && ((researchCheck) || (GET_RESEARCH_TOKENS(ch) > 0))) {
							obj->sbinfo[j].spellname = i;
							obj->sbinfo[j].pages = MAX(1, spell_info[i].class_level[CLASS_WIZARD] * 2);
							send_to_char(ch, "Your research is successful and you scribe the spell '%s' into your spellbook, which takes up %d pages.\r\n", spell_info[i].name, obj->sbinfo[j].pages);
							if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE))
								SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
							if (GET_RESEARCH_TOKENS(ch) < 1) {
								GET_GOLD(ch) -= cost / 100;
								send_to_char(ch, "You were charged %s %s for this research session.\r\n", change_coins(cost), MONEY_STRING);
							}
							else {
								GET_RESEARCH_TOKENS(ch) -= 1;
								send_to_char(ch, "You used one of your research sessions to learn this spell leaving you with %d.\r\n", GET_RESEARCH_TOKENS(ch));
							}
							return TRUE;
						}
						else if (spellBookFull) {
							send_to_char(ch, "Your spellbooks are full, you must buy a new one if you wish to research a new spell.\r\n");
							return TRUE;
						}
						else {
							GET_GOLD(ch) -= cost / 500;
							send_to_char(ch, "Your research failed, and the cost in materials was %s %s.\r\n", change_coins(cost / 5), MONEY_STRING);
							return TRUE;
						}
					}
				}
				if (!found) {
				  send_to_char(ch, "You do not have a spellbook to record your spell in.\r\n");
				  return TRUE;
				}
			}
			else {
				send_to_char(ch, "You need to have at least %s %s to do the research for that spell.\r\n", change_coins(cost), MONEY_STRING);
				return TRUE;
			}
		}
		else {
			send_to_char(ch, "This library does not have the resources to research that spell.\r\n");
			return TRUE;
		}
	}
	else {
		send_to_char(ch, "The references for that spell are far beyond your understanding.\r\n");
		return TRUE;
	}
		GET_MEM_TYPE(ch) = 0;
}
else {
	send_to_char(ch, "The library has no references whatsoever for that spell.\r\n");
	return TRUE;
}
}
}

send_to_char(ch, "The library has no references for that spell.\r\n");
return TRUE;

}

SPECIAL(library_large)
{
struct obj_data *obj;
int i, j, cost =10;
int spellBookFull = TRUE;
int researchCheck = FALSE;
bool found = FALSE;

if (!CMD_IS("research"))
return (FALSE);

skip_spaces(&argument);

for (i = 0; i < MAX_SPELLS; i++) {
	if (spell_info[i].name != NULL && is_abbrev(argument, spell_info[i].name)) {
	if (spell_info[i].school != SCHOOL_UNDEFINED) {
                GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
		if (findslotnum(ch, spell_info[i].class_level[CLASS_WIZARD]) != -1) {
			if (spell_info[i].spell_level <= 9) {
			for (j = 1; j < spell_info[i].class_level[CLASS_WIZARD]; j++)
				cost *= 3 ;
			if (GET_GOLD(ch) >= cost || GET_RESEARCH_TOKENS(ch) > 0) {
				for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
					if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
						found = TRUE;
						if (spell_in_book(obj, i)) {
							send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n", spell_info[i].name);
							return TRUE;
						}
						if (!obj->sbinfo) {
							CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
							memset((char *) obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
						}
						for (j=0; j < SPELLBOOK_SIZE; j++) {
							if (obj->sbinfo[j].spellname == 0) {
								spellBookFull = FALSE;
								break;
							} else {
								continue;
							}
						}
						researchCheck = ((dice(1, 20) + ability_mod_value(GET_INT(ch))) > (dice(1, 20) + spell_info[i].spell_level));
						if (!spellBookFull && ((researchCheck) || (GET_RESEARCH_TOKENS(ch) > 0))) {
							obj->sbinfo[j].spellname = i;
							obj->sbinfo[j].pages = MAX(1, spell_info[i].class_level[CLASS_WIZARD] * 2);
							send_to_char(ch, "Your research is successful and you scribe the spell '%s' into your spellbook, which takes up %d pages.\r\n", spell_info[i].name, obj->sbinfo[j].pages);
							if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE))
								SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
							if (GET_RESEARCH_TOKENS(ch) < 1) {
								GET_GOLD(ch) -= cost / 100;
								send_to_char(ch, "You were charged %s %s for this research session.\r\n", change_coins(cost), MONEY_STRING);
							}
							else {
								GET_RESEARCH_TOKENS(ch) -= 1;
								send_to_char(ch, "You used one of your research sessions to learn this spell leaving you with %d.\r\n", GET_RESEARCH_TOKENS(ch));
							}
							return TRUE;
						}
						else if (spellBookFull) {
							send_to_char(ch, "Your spellbooks are full, you must buy a new one if you wish to research a new spell.\r\n");
							return TRUE;
						}
						else {
							GET_GOLD(ch) -= cost / 500;
							send_to_char(ch, "Your research failed, and the cost in materials was %s %s.\r\n", change_coins(cost / 5), MONEY_STRING);
							return TRUE;
						}
					}
				}
				if (!found) {
				  send_to_char(ch, "You do not have a spellbook to record your spell in.\r\n");
				  return TRUE;
				}
			}
			else {
				send_to_char(ch, "You need to have at least %s %s to do the research for that spell.\r\n", change_coins(cost), MONEY_STRING);
				return TRUE;
			}
		}
		else {
			send_to_char(ch, "This library does not have the resources to research that spell.\r\n");
			return TRUE;
		}
	}
	else {
		send_to_char(ch, "The references for that spell are far beyond your understanding.\r\n");
		return TRUE;
	}
		GET_MEM_TYPE(ch) = 0;
}
else {
	send_to_char(ch, "The library has no references whatsoever for that spell.\r\n");
	return TRUE;
}
}
}

send_to_char(ch, "The library has no references for that spell.\r\n");
return TRUE;

}

SPECIAL(dump)
{
struct obj_data *k;
int value = 0;

for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
extract_obj(k);
}

if (!CMD_IS("drop"))
return (FALSE);

do_drop(ch, argument, cmd, SCMD_DROP);

for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
extract_obj(k);
}

if (value) {
send_to_char(ch, "You are awarded for outstanding performance.\r\n");
act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

if (GET_CLASS_LEVEL(ch) < 3)
gain_exp(ch, value);
else
GET_GOLD(ch) += value;
}
return (TRUE);
}
/*
SPECIAL(respec)
{

char className[MAX_STRING_LENGTH];
char startFresh[MAX_STRING_LENGTH];
int i = 0;
int beginAtZero = FALSE;

return FALSE;

if(!CMD_IS("respec"))
return FALSE;

two_arguments(argument, className, startFresh);

if (!*className) {
send_to_char(ch, "Please enter the name of the class you wish to respec to.  If you wish to begin at level 1 follow the class name with the word 'new'\r\n"
			"The following classes are available:\r\n");
	for (i = 0; i < NUM_CLASSES; i++) {

	}
}

}
*/
SPECIAL(no_spec)
{

return FALSE;

}

SPECIAL(mayor)
{
char actbuf[MAX_INPUT_LENGTH]={'\0'};

const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

static const char *path = NULL;
static int path_index;
static bool move = FALSE;

if (!move) {
if (time_info.hours == 6) {
move = TRUE;
path = open_path;
path_index = 0;
} else if (time_info.hours == 20) {
move = TRUE;
path = close_path;
path_index = 0;
}
}
if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
(GET_POS(ch) == POS_FIGHTING))
return (FALSE);

switch (path[path_index]) {
case '0':
case '1':
case '2':
case '3':
perform_move(ch, path[path_index] - '0', 1);
break;

case 'W':
GET_POS(ch) = POS_STANDING;
act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'S':
GET_POS(ch) = POS_SLEEPING;
act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'a':
act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'b':
act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
break;

case 'c':
act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
break;

case 'd':
act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'e':
act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'E':
act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
break;

case 'O':
do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);	/* strcpy: OK */
do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);	/* strcpy: OK */
break;

case 'C':
do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);	/* strcpy: OK */
do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);	/* strcpy: OK */
break;

case '.':
move = FALSE;
break;

}

path_index++;
return (FALSE);
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data *ch, struct char_data *victim)
{
int gold;

if (IS_NPC(victim))
return;
if (ADM_FLAGGED(victim, ADM_NOSTEAL))
return;
if (!CAN_SEE(ch, victim))
return;

if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
} else {
/* Steal some gold coins */
gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
if (gold > 0) {
GET_GOLD(ch) += gold;
GET_GOLD(victim) -= gold;
}
}
}


/*
* Quite lethal to low-level characters.
*/
SPECIAL(snake)
{
if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
return (FALSE);

if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
return (FALSE);

act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL, NULL);
return (TRUE);
}


SPECIAL(thief)
{
struct char_data *cons;

if (cmd || GET_POS(ch) != POS_STANDING)
return (FALSE);

for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
if (!IS_NPC(cons) && !ADM_FLAGGED(cons, ADM_NOSTEAL) && !rand_number(0, 4)) {
npc_steal(ch, cons);
return (TRUE);
}

return (FALSE);
}


SPECIAL(magic_user_orig)
{
struct char_data *vict;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return (FALSE);

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
vict = FIGHTING(ch);

/* Hm...didn't pick anyone...I'll wait a round. */
if (vict == NULL)
return (TRUE);

if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
cast_spell(ch, vict, NULL, SPELL_POISON, NULL);

if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
cast_spell(ch, vict, NULL, SPELL_BLINDNESS, NULL);

if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0) {
if (IS_EVIL(ch))
cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN, NULL);
else if (IS_GOOD(ch))
cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
}

if (rand_number(0, 4))
return (TRUE);

switch (GET_LEVEL(ch)) {
case 4:
case 5:
cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
break;
case 6:
case 7:
cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
break;
case 8:
case 9:
cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
break;
case 10:
case 11:
cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
break;
case 12:
case 13:
cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
break;
case 14:
case 15:
case 16:
case 17:
cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, NULL);
break;
default:
cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
break;
}
return (TRUE);

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
int i;
struct char_data *guard = (struct char_data *)me;
const char *buf = "The guard humiliates you, and blocks your way.\r\n";
const char *buf2 = "The guard humiliates $n, and blocks $s way.";

if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
return (FALSE);

if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
return (FALSE);

for (i = 0; guild_info[i].guild_room != NOWHERE; i++) {
/* Wrong guild or not trying to enter. */
if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room || cmd != guild_info[i].direction)
continue;

/* Allow the people of the guild through. */
if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class)
continue;

send_to_char(ch, "%s", buf);
act(buf2, FALSE, ch, 0, 0, TO_ROOM);
return (TRUE);
}

return (FALSE);
}



SPECIAL(puff)
{
char actbuf[MAX_INPUT_LENGTH]={'\0'};

if (cmd)
return (FALSE);

switch (rand_number(0, 60)) {
case 0:
do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);	/* strcpy: OK */
return (TRUE);
case 1:
do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);	/* strcpy: OK */
return (TRUE);
case 2:
do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);	/* strcpy: OK */
return (TRUE);
case 3:
do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);	/* strcpy: OK */
return (TRUE);
default:
return (FALSE);
}
}



SPECIAL(fido)
{
struct obj_data *i, *temp, *next_obj;

if (cmd || !AWAKE(ch))
return (FALSE);

for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
if (!IS_CORPSE(i))
continue;

act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
for (temp = i->contains; temp; temp = next_obj) {
next_obj = temp->next_content;
obj_from_obj(temp);
obj_to_room(temp, IN_ROOM(ch));
}
extract_obj(i);
return (TRUE);
}

return (FALSE);
}



SPECIAL(janitor)
{
struct obj_data *i;

if (cmd || !AWAKE(ch))
return (FALSE);

for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
continue;
if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
continue;
act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
obj_from_room(i);
obj_to_char(i, ch);
return (TRUE);
}

return (FALSE);
}


SPECIAL(cityguard)
{
struct char_data *tch, *evil, *spittle;
int max_evil, min_cha;

if (cmd || !AWAKE(ch) || FIGHTING(ch))
return (FALSE);

max_evil = 1000;
min_cha = 6;
spittle = evil = NULL;

for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
if (!CAN_SEE(ch, tch))
continue;

if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
hit(ch, tch, TYPE_UNDEFINED);
return (TRUE);
}

if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
hit(ch, tch, TYPE_UNDEFINED);
return (TRUE);
}

if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
max_evil = GET_ALIGNMENT(tch);
evil = tch;
}

if (GET_CHA(tch) < min_cha) {
spittle = tch;
min_cha = GET_CHA(tch);
}
}

if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
hit(ch, evil, TYPE_UNDEFINED);
return (TRUE);
}

/* Reward the socially inept. */
if (spittle && !rand_number(0, 9)) {
static int spit_social;

if (!spit_social)
spit_social = find_command("spit");

if (spit_social > 0) {
char spitbuf[MAX_NAME_LENGTH + 1]={'\0'};

strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));	/* strncpy: OK */
spitbuf[sizeof(spitbuf) - 1] = '\0';

do_action(ch, spitbuf, spit_social, 0);
return (TRUE);
}
}

return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 50)

SPECIAL(pet_shops)
{
char buf[MAX_STRING_LENGTH]={'\0'}, pet_name[256]={'\0'};
room_rnum pet_room;
struct char_data *pet;

/* Gross. */
pet_room = IN_ROOM(ch) + 1;

if (CMD_IS("list")) {
send_to_char(ch, "Available pets are:\r\n");
for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
/* No, you can't have the Implementor as a pet if he's in there. */
if (!IS_NPC(pet))
	continue;
send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
}
return (TRUE);
} else if (CMD_IS("buy")) {

two_arguments(argument, buf, pet_name);

if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
send_to_char(ch, "There is no such pet!\r\n");
return (TRUE);
}
if (GET_GOLD(ch) < PET_PRICE(pet) && !(HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) && GET_COMPANION_VNUM(ch) == 0)) {
send_to_char(ch, "You don't have enough gold!\r\n");
return (TRUE);
}
if (!(HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) && GET_COMPANION_VNUM(ch) == 0))
  GET_GOLD(ch) -= PET_PRICE(pet);


pet = read_mobile(GET_MOB_RNUM(pet), REAL);
GET_EXP(pet) = 0;
SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);


if (HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) && GET_COMPANION_VNUM(ch) == 0) {
  GET_COMPANION_VNUM(ch) = GET_MOB_VNUM(pet);
  send_to_char(ch, "%s has become your animal companion.\r\n", pet->short_descr);
}

if (*pet_name) {
snprintf(buf, sizeof(buf), "%s %s", pet->name, pet_name);
/* free(pet->name); don't free the prototype! */
pet->name = strdup(buf);

snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	pet->description, pet_name);
/* free(pet->description); don't free the prototype! */
pet->description = strdup(buf);
}
char_to_room(pet, IN_ROOM(ch));
add_follower(pet, ch);
	SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
pet->master_id = GET_IDNUM(ch);

	if (MOB_FLAGGED(pet, MOB_MOUNTABLE))
	GET_MOUNT_VNUM(ch) = GET_MOB_VNUM(pet);
	else
	GET_PET_VNUM(ch) = GET_MOB_VNUM(pet);

/* Be certain that pets can't get/carry/use/wield/wear items */
IS_CARRYING_W(pet) = 0;
IS_CARRYING_N(pet) = 0;

send_to_char(ch, "May you enjoy your pet.\r\n");
act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

return (TRUE);
}

/* All commands except list and buy */
return (FALSE);
}
/*
SPECIAL(stable)
{

int horses[] = {1636, 3910, 4564, 4565, 6034, 6036, 8518, 12119, 12187, 12382, 19023};

if (CMD_IS("buy")) {

return TRUE;
}
else if (CMD_IS("sell")) {
return TRUE;
}
return FALSE;
}
*/
/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(item_bank) {
/*
  struct obj_data *obj;
  skip_spaces(&argument);

  if (CMD_IS("list")) {
    send_to_char(ch, "The contents of your item bank are as follows:\r\n");
    for (obj = ch->desc->account->item_bank; obj; obj = obj->next_in_bank) {
      send_to_char(ch, "-- %s\r\n", obj->short_description);
    }
    return 1;
  } else if (CMD_IS("deposit")) {
    if (!*argument) {
      send_to_char(ch, "What do you wish to deposit into your item bank?\r\n");
      return 1;
    }
    int i = 0;
    for (obj = ch->desc->account->item_bank; obj; obj = obj->next_in_bank)
      i++;
    if (i >= (20 + MAX(0, ch->desc->account->item_bank_size))) {
      send_to_char(ch, "You do not have any more space in your item bank.\r\n");
      return 1;
    }
    if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
      send_to_char(ch, "You don't seem to have anything by that description in your inventory.\r\n");
      return 1;
    }
    int cost = MAX(5, GET_OBJ_COST(obj) * 2 / 100);
    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "It costs %d credits to store this item, and you only have %d on hand.\r\n", cost, GET_GOLD(ch));
      return 1;
    }
    obj_from_char(obj);
    obj->next_in_bank = ch->desc->account->item_bank;
    ch->desc->account->item_bank = obj;
    send_to_char(ch, "You deposit %s into your item bank at a cost of %d credits.\r\n", obj->short_description, cost);
    GET_GOLD(ch) -= cost;
    save_char(ch);
    Crash_crashsave(ch, 2);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if (!*argument) {
      send_to_char(ch, "What do you wish to withdraw from your item bank?\r\n");
      return 1;
    }
    if (!(obj = get_obj_in_item_bank(ch, argument, NULL, ch->desc->account->item_bank))) {
      send_to_char(ch, "You don't seem to have anything by that description in your item bank.\r\n");
      return 1;
    }
    obj_to_char(obj, ch);
    struct obj_data *temp;
    REMOVE_FROM_LIST(obj, ch->desc->account->item_bank, next_in_bank);
    send_to_char(ch, "You withdraw %s into your item bank.\r\n", obj->short_description);
    save_char(ch);
    Crash_crashsave(ch, 2);
    return 1;
  } else {
    return 0;
  }
*/
  return 0;
}

SPECIAL(bank)
{
    int amount;

    if (CMD_IS("balance"))
    {
        if (GET_BANK_GOLD(ch) > 0)
            send_to_char(ch, "Your current balance is %d %s.\r\n", GET_BANK_GOLD(ch), MONEY_STRING);
        else
            send_to_char(ch, "You currently have no money deposited.\r\n");
        return (TRUE);
    } else if (CMD_IS("deposit"))
    {
        if ((amount = atoi(argument)) <= 0)
        {
            send_to_char(ch, "How much do you want to deposit?\r\n");
            return (TRUE);
        }
        if (GET_GOLD(ch) < amount)
        {
            send_to_char(ch, "You don't have that many %s!\r\n", MONEY_STRING);
            return (TRUE);
        }
        if (AFF_FLAGGED(ch, AFF_SPIRIT))
        {
            send_to_char(ch, "You can't use the bank when you're dead!\r\n");
            return 1;
        }

        GET_GOLD(ch) -= amount;
        GET_BANK_GOLD(ch) += amount * 19 / 20;
        send_to_char(ch, "You deposit %d %s.\r\n", amount * 19 / 20, MONEY_STRING);
        send_to_char(ch, "%d %s were taken by the bank for taxes and service fees.\r\n", amount / 20, MONEY_STRING);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    } else if (CMD_IS("invest"))
    {
        if (!HAS_FEAT(ch, FEAT_FINANCIAL_EXPERT))
        {
            send_to_char(ch, "Only characters with the financial expert feat can invest %s at the bank.\r\n", MONEY_STRING);
            return 1;
        }
        if (!is_innate_ready(ch, SPELL_FINANCIAL_EXPERT))
        {
            send_to_char(ch, "Your cooldown on this ability has not yet expired.  See @Ytimers@n command.\r\n");
            return 1;
        }
        if (AFF_FLAGGED(ch, AFF_SPIRIT))
        {
            send_to_char(ch, "You can't use the bank when you're dead!\r\n");
            return 1;
        }
        if (GET_BANK_GOLD(ch) == 0)
        {
            send_to_char(ch, "You do not have any money in your bank to invest.\r\n");
            return 1;
        }

        amount = GET_BANK_GOLD(ch);

        add_innate_timer(ch, SPELL_FINANCIAL_EXPERT);

        int skill_roll = 0;

        skill_roll = combat_skill_roll(ch, SKILL_KNOWLEDGE);

        amount *= skill_roll;
        amount /= 1000;

        gain_gold(ch, amount, GOLD_BANK);
        send_to_char(ch, "You make an investment.  Your bank balance increases by %d %s to a new balance of %d.\r\n",
            amount, MONEY_STRING, GET_BANK_GOLD(ch));
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    } else if (CMD_IS("withdraw"))
    {
        if ((amount = atoi(argument)) <= 0)
        {
            send_to_char(ch, "How much do you want to withdraw?\r\n");
            return (TRUE);
        }
        if (GET_BANK_GOLD(ch) < amount)
        {
            send_to_char(ch, "You don't have that many %s deposited!\r\n", MONEY_STRING);
            return (TRUE);
        }
        GET_GOLD(ch) += amount;
        GET_BANK_GOLD(ch) -= amount;
        send_to_char(ch, "You withdraw %d %s.\r\n", amount, MONEY_STRING);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    } else
    return (FALSE);
}


SPECIAL(cleric_marduk)
{
int tmp, num_used = 0;
struct char_data *vict;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

num_used = 12;

tmp = rand_number(1, 10);

if ( (tmp == 7 ) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
tmp = rand_number(1, num_used);
if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
	cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
	return TRUE;
}
if ((tmp == 2) && ( (GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
	cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
	return TRUE;
}
if ((tmp == 3) && (GET_LEVEL(ch) > 4 )) {
	cast_spell(ch, vict, NULL, SPELL_BESTOW_CURSE, NULL);
	return TRUE;
}
if ((tmp == 4) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
	cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
	return TRUE;
}
if ((tmp == 5) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
	cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
	return TRUE;
}
if ((tmp == 6) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
	cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
	return TRUE;
}
if (tmp == 7) {
	cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
	return TRUE;
}
if ((tmp == 8) && (GET_LEVEL(ch) > 6 ) && (!IS_UNDEAD(vict))) {
	cast_spell(ch, vict, NULL, SPELL_POISON, NULL);
	return TRUE;
}
if (tmp == 9 && GET_LEVEL(ch) > 8) {
	cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
	return TRUE;
}
if ((tmp == 10) && (GET_LEVEL(ch) > 10)) {
	cast_spell(ch, vict, NULL, SPELL_HARM, NULL);
	return TRUE;
}
if (tmp == 11) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
	return TRUE;
}
if (tmp == 12 && GET_LEVEL(ch) > 8) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
	return TRUE;
}
}
return FALSE;
}


SPECIAL(cleric_ao)
{
int tmp, num_used = 0;
struct char_data *vict;
if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

num_used = 8;

tmp = rand_number(1, 10);

if ( (tmp == 7 ) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
tmp = rand_number(1, num_used);
if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
return TRUE;
}
if ((tmp == 2) && ( (GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
return TRUE;
}
if ((tmp == 3) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
return TRUE;
}
if ((tmp == 4) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
return TRUE;
}
if ((tmp == 5) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
return TRUE;
}
if (tmp == 6) {
cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
return TRUE;
}
if (tmp == 7 && GET_LEVEL(ch) > 8) {
cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
return TRUE;
}
if (tmp == 8 && GET_LEVEL(ch) > 10) {
cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
return TRUE;
}
if (tmp == 9) {
cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
return TRUE;
}
if (tmp == 10 && GET_LEVEL(ch) > 8) {
cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
return TRUE;
}
}
return FALSE;
}


SPECIAL(dziak)
{
int tmp, num_used = 0;
struct char_data *vict;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;
/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

num_used = 9;

tmp = rand_number(3, 10);

if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
tmp = rand_number(1, num_used);

if (tmp == 2 || tmp == 1) {
cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
return TRUE;
}
if (tmp == 3) {
cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
return TRUE;
}
if (tmp == 4) {
cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
return TRUE;
}
if (tmp == 5) {
cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
return TRUE;
}
if (tmp == 6) {
cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
return TRUE;
}
if (tmp == 7) {
cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
return TRUE;
}
if ((tmp == 8) && (IS_GOOD(vict))) {
cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
return TRUE;
}
if (tmp == 9) {
cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
return TRUE;
}
}
return FALSE;
}


SPECIAL(azimer)
{
int tmp, num_used = 0;
struct char_data *vict;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

num_used = 8;

tmp = rand_number(3, 10);

if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
tmp = rand_number(1, num_used);

if (tmp == 2 || tmp == 1) {
cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
return TRUE;
}
if (tmp == 3) {
cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
return TRUE;
}
if (tmp == 4) {
cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
return TRUE;
}
if (tmp == 5) {
cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
return TRUE;
}
if (tmp == 6) {
cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
return TRUE;
}
if (tmp == 7) {
cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
return TRUE;
}
if ((tmp == 8) && (IS_GOOD(vict))) {
cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
return TRUE;
}
}
return FALSE;
}


SPECIAL(lyrzaxyn)
{
int tmp, num_used = 0;
struct char_data *vict;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

num_used = 8;

tmp = rand_number(3, 10);

if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
tmp = rand_number(1, num_used);

if (tmp == 2 || tmp == 1) {
cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
return TRUE;
}
if (tmp == 3) {
cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
return TRUE;
}
if (tmp == 4) {
cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
return TRUE;
}
if (tmp == 5) {
cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
return TRUE;
}
if (tmp == 6) {
cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
return TRUE;
}
if (tmp == 7) {
cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
return TRUE;
}
if ((tmp == 8) && (IS_GOOD(vict))) {
cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
return TRUE;
}
}
return FALSE;
}


SPECIAL(wizard)
{
struct char_data *vict;
if (!FIGHTING(ch))
return (wizard_cast_buff(ch));

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

return (wizard_cast_spell(ch, vict));

return FALSE;
}

SPECIAL(cleric)
{
    struct char_data *vict;

    if (!FIGHTING(ch))
    {
        if (GET_MAX_HIT(ch) != GET_HIT(ch))
        {
            return (cleric_cast_cure(ch));
        }
    }
    return (cleric_cast_buff(ch));

    if (cmd || GET_POS(ch) != POS_FIGHTING)
    {
        return FALSE;
    }

/* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    {
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
        {
            break;
        }
    }

/* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
    {
        vict = FIGHTING(ch);
    }

    if (dice(1, 100) < 51 && (GET_MAX_HIT(ch) != GET_HIT(ch)))
    {
        return (cleric_cast_cure(ch));
    }
    else
    {
        return (cleric_cast_spell(ch, vict));
    }

    return FALSE;
}

int wizard_cast_buff(struct char_data *ch)
{
	struct char_data *vict = ch;
int num_buffs = 0, buffnum = 0;

num_buffs = 4;

buffnum = dice(1, num_buffs * 2);

if (buffnum == 1 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_WIZARD) {
if (GET_LEVEL(ch) >= 1) {
	if (!affected_by_spell(ch, SPELL_MAGE_ARMOR)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_MAGE_ARMOR].spell_level)) {
	cast_spell(ch, ch, NULL, SPELL_MAGE_ARMOR, NULL);
	return true;
	}
	wizard_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 2 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_WIZARD) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_SEE_INVIS)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_SEE_INVIS].spell_level)) {
	cast_spell(ch, ch, NULL, SPELL_SEE_INVIS, NULL);
	return true;
	}
	wizard_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 3 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_WIZARD) {
if (GET_LEVEL(ch) >= 1) {
	if (!affected_by_spell(ch, SPELL_PROT_FROM_EVIL)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_PROT_FROM_EVIL].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_PROT_FROM_EVIL, NULL);
	return true;
	}
	wizard_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 4 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_WIZARD) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_BULL_STRENGTH)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BULL_STRENGTH].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BULL_STRENGTH, NULL);
	return true;
	}
	wizard_cast_buff(ch);
	}
	else
	return false;
}
}

return false;
}

int cleric_cast_buff(struct char_data *ch)
{
	struct char_data *vict = ch;
int num_buffs = 0, buffnum = 0;

num_buffs = 9;

buffnum = dice(1, num_buffs * 2);

if (buffnum == 1 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 1) {
	if (!affected_by_spell(ch, SPELL_SHIELD_OF_FAITH)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_SHIELD_OF_FAITH].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_SHIELD_OF_FAITH, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 2 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 1) {
	if (!affected_by_spell(ch, SPELL_DIVINE_FAVOR)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_DIVINE_FAVOR].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_DIVINE_FAVOR, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 3 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_SEE_INVIS)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_SEE_INVIS].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_SEE_INVIS, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 4 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 1) {
	if (!affected_by_spell(ch, SPELL_PROT_FROM_EVIL)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_PROT_FROM_EVIL].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_PROT_FROM_EVIL, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 5 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_BULL_STRENGTH)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BULL_STRENGTH].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BULL_STRENGTH, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 7 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_OWLS_WISDOM)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_OWLS_WISDOM].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_OWLS_WISDOM, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 8 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_BEARS_ENDURANCE)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BEARS_ENDURANCE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BEARS_ENDURANCE, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 9 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 3) {
	if (!affected_by_spell(ch, SPELL_AID)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_AID].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_AID, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}
else if (buffnum == 6 && IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_CLERIC) {
if (GET_LEVEL(ch) >= 7) {
	if (!affected_by_spell(ch, SPELL_SENSE_LIFE)) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_SENSE_LIFE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_SENSE_LIFE, NULL);
	return true;
	}
	cleric_cast_buff(ch);
	}
	else
	return false;
}
}

return false;
}

int cleric_cast_cure(struct char_data *ch)
{

int num_cures = 5;
int tmp = dice(1, (num_cures * 3));

if (tmp >= (num_cures * 2))
	return FALSE;

switch ((tmp + 1) / 2) {

case 1:
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_CURE_LIGHT].spell_level)) {
cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
return TRUE;
}
cleric_cast_cure(ch);
break;

case 2:
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_CURE_MODERATE].spell_level)) {
cast_spell(ch, ch, NULL, SPELL_CURE_MODERATE, NULL);
return TRUE;
}
cleric_cast_cure(ch);
break;

case 3:
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_CURE_SERIOUS].spell_level)) {
cast_spell(ch, ch, NULL, SPELL_CURE_SERIOUS, NULL);
return TRUE;
}
cleric_cast_cure(ch);
break;

case 4:
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_CURE_CRITIC].spell_level)) {
cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
return TRUE;
}
cleric_cast_cure(ch);
break;

case 5:
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_HEAL].spell_level)) {
cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
return TRUE;
}
cleric_cast_cure(ch);
break;
}

return FALSE;;
}

int wizard_cast_spell(struct char_data *ch, struct char_data *vict)
{

int num_used = 0, tmp = 0;

num_used = 8;

tmp = rand_number(1, num_used + (num_used / 4));

if ((tmp == 1)) {
if (GET_LEVEL(ch) >= 1) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_CHILL_TOUCH].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 2)) {
if (GET_LEVEL(ch) >= 1) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BURNING_HANDS].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 3)) {
if (GET_LEVEL(ch) >= 1) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_MAGIC_MISSILE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 4)) {
if (GET_LEVEL(ch) >= 5) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_LIGHTNING_BOLT].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 5)) {
if (GET_LEVEL(ch) >= 5) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_FIREBALL].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 6)) {
if (GET_LEVEL(ch) >= 5) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BLINDNESS].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BLINDNESS, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 7)) {
if (GET_LEVEL(ch) >= 7) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BESTOW_CURSE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BESTOW_CURSE, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
}
else
return false;
}
else if (tmp == 8) {
	if (GET_LEVEL(ch) >= 3) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_ACID_ARROW].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_ACID_ARROW, NULL);
	return true;
}
wizard_cast_spell(ch, vict);
	}
	else
		return FALSE;
}
else
return false;

return false;
}

int cleric_cast_spell(struct char_data *ch, struct char_data *vict)
{

int num_used = 0, tmp = 0;

num_used = 12;

tmp = rand_number(1, num_used + 1);

if ((tmp == 1)) {
if (GET_LEVEL(ch) >= 1) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_INFLICT_LIGHT].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 2)) {
if (GET_LEVEL(ch) >= 3) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BLINDNESS].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BLINDNESS, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 3)) {
if (GET_LEVEL(ch) >= 7) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_POISON].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_POISON, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 4)) {
if (GET_LEVEL(ch) >= 7) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_INFLICT_CRITIC].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 5)) {
if (GET_LEVEL(ch) >= 3) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_INFLICT_MODERATE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_MODERATE, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 6)) {
if (GET_LEVEL(ch) >= 9 && !IS_EVIL(ch) && !IS_GOOD(FIGHTING(ch))) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_DISPEL_EVIL].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 7)) {
if (GET_LEVEL(ch) >= 9 && !IS_GOOD(ch) && !IS_EVIL(FIGHTING(ch))) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_DISPEL_GOOD].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 8)) {
if (GET_LEVEL(ch) >= 15) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_BESTOW_CURSE].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_BESTOW_CURSE, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 9)) {
if (GET_LEVEL(ch) >= 11) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_HARM].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_HARM, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 10)) {
if (GET_LEVEL(ch) >= 13) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_MASS_HARM].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_MASS_HARM, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 11)) {
if (GET_LEVEL(ch) >= 1) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_INFLICT_MINOR].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_MINOR, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}
else if ((tmp == 12)) {
if (GET_LEVEL(ch) >= 7) {
	if (GET_SPELL_SLOT(ch, spell_info[SPELL_INFLICT_SERIOUS].spell_level)) {
	cast_spell(ch, vict, NULL, SPELL_INFLICT_SERIOUS, NULL);
	return true;
}
cleric_cast_spell(ch, vict);
}
else
return false;
}


return false;
}

SPECIAL(fighter) {

struct char_data *vict;

if (!FIGHTING(ch))
return FALSE;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

return fighter_perform_action(ch, vict);

return FALSE;
}

int fighter_perform_action(struct char_data *ch, struct char_data *vict)
{

int act_used = 0, act = 0, num = 0;
char buf[MAX_STRING_LENGTH]={'\0'};

act_used = 2;

act = rand_number(1, act_used + 1);

if ((act == 1)) {
  if (GET_LEVEL(ch) >= 2 && ((num) = (MAX(0, MIN(5, (int) GET_LEVEL(ch) - (int) GET_LEVEL(vict)))))) {
	SET_FEAT(ch, FEAT_POWER_ATTACK, 1);
	sprintf(buf, "%d", num);
  do_value(ch, buf, 1, 0);
  return true;
}
else if ((act == 2)) {
  if (GET_LEVEL(ch) >= 1) {
    do_kick(ch, 0, 0, 0);
    return true;
  }
}
else
  return false;
}

return false;
}

SPECIAL(rogue) {

struct char_data *vict;

if (!FIGHTING(ch))
return FALSE;

if (cmd || GET_POS(ch) != POS_FIGHTING)
return FALSE;

/* pseudo-randomly choose someone in the room who is fighting me */
for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
if (FIGHTING(vict) == ch && !rand_number(0, 4))
break;

/* if I didn't pick any of those, then just slam the guy I'm fighting */
if (vict == NULL)
vict = FIGHTING(ch);

return rogue_perform_action(ch, vict);

return FALSE;
}

int rogue_perform_action(struct char_data *ch, struct char_data *vict)
{

int act_used = 0, act = 0;

act_used = 2;

act = rand_number(1, act_used + 1);

if ((act == 1)) {
  if (GET_LEVEL(ch) >= 1) {
    SET_FEAT(ch, FEAT_SNEAK_ATTACK, (GET_LEVEL(ch) + 1) / 2);
    do_feint(ch, 0, 0, 0);
    return true;
  }
  else
    return false;
}
else if ((act == 2)) {
  if (GET_LEVEL(ch) >= 1) {
    do_kick(ch, 0, 0, 0);
    return true;
  }
}
else
  return false;

return false;
}


SPECIAL(enchant_mob)
{

  char arg[100]={'\0'};
  char arg2[100]={'\0'};
  char arg3[100]={'\0'};
  int i = 0, j = 0, cost = 0, mod = 0;
  struct obj_data *obj = NULL;

  if (!CMD_IS("enchant") && !CMD_IS("value"))
    return 0;

 one_argument(one_argument(one_argument(argument, arg), arg2), arg3);

  if (!*arg) {
    send_to_char(ch, "What would you like to enchant?\r\n");
    return 1;
  }

  if (!*arg2) {
    send_to_char(ch, "Please choose from the following enchant types for your item:\r\n\r\n");
    for (i = 0; i < NUM_APPLIES+1; i++){
     if (apply_gold_cost[i] == 0)
        continue;
      send_to_char(ch, "%s\r\n", apply_text[i]);
    }
    return 1;
  }

  if (!*arg3) {
    send_to_char(ch, "How much to you want to enchant it by?\r\n");
    return 1;
  }

  if (!(obj = (get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) {
    send_to_char(ch, "You don't seem to have that item.\r\n");
    return 1;
  }

  for (i = 0; i < NUM_APPLIES; i++) {
    if (is_abbrev(arg2, apply_text[i]))
      break;
  }

  if (i == NUM_APPLIES) {
    send_to_char(ch, "That is not a valid enchantment type.\r\n");
    return 1;
  }

  mod = atoi(arg3);

  if (mod < 1) {
    send_to_char(ch, "The enchantment modifier must be greater than zero.\r\n");
    return 1;
  }

  if (mod > 1000) {
    send_to_char(ch, "The enchantment modifier must be less than 1000.\r\n");
    return 1;
  }

  for (j = 0; j < mod; j++)
    cost += apply_gold_cost[i] * (j + 1) * MAX(1, j);

  if (CMD_IS("value")) {
    send_to_char(ch, "That enchantment would cost %d %s.\r\n", cost, MONEY_STRING);
    return 1;
  }

  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "It would cost %d for that enchantment.  You only have %d\r\n",
                 cost, GET_GOLD(ch));
    return 1;
  }



  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (obj->affected[j].location == i)
      break;
  }

  if (j == MAX_OBJ_AFFECT) {
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      if (obj->affected[j].location == APPLY_NONE)
        break;
    }
  }

  if (j == MAX_OBJ_AFFECT) {
    send_to_char(ch, "That item cannot be enchanted any furhter.\r\n");
	return 1;
  }

  if (i == APPLY_DAMROLL || i == APPLY_AC_ARMOR || i == APPLY_AC_SHIELD) {
    send_to_char(ch, "This type of enchantment is not allowed.\r\n");
    return 1;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
    if  ((i == APPLY_DAMROLL)) {
      send_to_char(ch, "You can only enchant weapons with that enchantment.\r\n");
      return 1;
    }
  }


  GET_GOLD(ch) -= cost;
  obj->affected[j].location = i;
  obj->affected[j].modifier = mod;
  obj->affected[j].specific = 0;

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  GET_OBJ_COST(obj) = 250 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);

  send_to_char(ch, "You have successfully enchanted %s with +%d to %s.\r\n", obj->short_description, mod, apply_text[i]);
  return 1;
}


SPECIAL(buff_mob)
{

  int i = 0, cost = 0;
  char buf[100]={'\0'};

  if (!CMD_IS("buff"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What spell would you like to be buffed with?\r\n");
    return 1;
  }

  for (i = 0; i <= TOP_SPELL; i++) {
    if (is_abbrev(argument, spell_info[i].name) && strcmp(spell_info[i].name, "!UNUSED!"))
      break;
  }

  if (i > TOP_SPELL) {
    send_to_char(ch, "That is not a valid spell to request a buff for.\r\n");
    return 1;
  }

  if (spell_info[i].violent == TRUE) {
    send_to_char(ch, "I'm pretty sure you don't want to have that cast on you.\r\n");
    return 1;
  }

  if (spell_info[i].class_level[GET_CLASS((struct char_data *) me)] == 99) {
    sprintf(buf, "%s I cannot cast that spell.", GET_NAME(ch));
    do_tell(me, buf, 0, 0);
    return 1;
  }

  if (!IS_SET(spell_info[i].targets, TAR_CHAR_ROOM) || IS_SET(spell_info[i].targets, TAR_SELF_ONLY)) {
    sprintf(buf, "%s I cannot cast that spell on you.", GET_NAME(ch));
    do_tell(me, buf, 0, 0);
    return 1;
  }


  cost = ((spell_info[i].spell_level * (spell_info[i].spell_level - 1)) + 2) * 50;

  if (GET_CLASS_LEVEL(ch) <= 5)
    cost /= 10;

  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You do not have enough gold to purchase that buff.  You need %d total.\r\n", cost);
    return 1;
  }

  GET_GOLD(ch) -= cost;

  send_to_char(ch, "That cost you %d %s.\r\n", cost, MONEY_STRING);
  call_magic((struct char_data *) me, ch, 0, i, 60, CAST_SPELL, NULL);

  return 1;
}

SPECIAL(crafting_station)
{

  if (!CMD_IS("resize") && !CMD_IS("create") && !CMD_IS("checkcraft") && !CMD_IS("restring") && !CMD_IS("augment") &&
      !CMD_IS("convert") && !CMD_IS("supplyorder"))
    return 0;

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You cannot craft anything until you've made some room in your inventory.\r\n");
    return 1;
  }

  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until your current task ends.\r\n");
    return 1;
  }

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any crafting actions.\r\n");
    return 1;
  }


  struct obj_data *station = (struct obj_data *) me;
  int rand = 0;
  int i = 0, j = 0;
  struct obj_data *o = NULL;
  int pattern_vnum = 0;
  struct obj_data *essence_one = NULL;
  int material = 0;
  int material_amount = 0;
  int material_level = 0;
  struct obj_data *created = NULL;
  int bonus = 0;
  int station_obj_vnum = GET_OBJ_VNUM(station);
  char buf[200]={'\0'};
  int skill = 0;
  int craft_type = SCMD_CRAFT;
  struct obj_data *orig = NULL, *crystal = NULL;
  int newsize = 0;
  int sz=0, lvl=0, ndice=0, diesize=0;
  int val = 0;
  int mn = 0;
  int crystal_level = 0;
  int crystal_bonus = 0;
  unsigned int mat_vnum = 0;
  sbyte mat_freebie = FALSE;
  struct obj_data *freebie = NULL;

  skip_spaces(&argument);


  if (!*argument && !CMD_IS("checkcraft") && !CMD_IS("augment") && !CMD_IS("supplyorder")) {
    if (CMD_IS("create") || CMD_IS("restring"))
      send_to_char(ch, "Please provide an item description containing the material and item name in the string.\r\n");
    else if (CMD_IS("resize"))
      send_to_char(ch, "What would you like the new size to be? (fine|diminutive|tiny|small|medium|large|huge|gargantuan|colossal)\r\n");
    return 1;
  }

  if (!station->contains) {
    if (CMD_IS("augment"))
      send_to_char(ch, "You must place at least two %ss of the same type into the station in order to augment a %s.\r\n", "crystal", "crystal");
    if (CMD_IS("supplyorder")) {
      if (GET_AUTOCQUEST_MATERIAL(ch))
        send_to_char(ch, "You must place 2 units of %s or a similar type of material (all the same type) into the station to continue.\r\n", material_names[GET_AUTOCQUEST_MATERIAL(ch)]);
      else
        send_to_char(ch, "You do not have a supply order active right now.\r\n");
    }
    if (CMD_IS("create"))
      send_to_char(ch, "You must place an item to use as the mold pattern, a %s and your crafting resource materials\r\nin the station and then type 'create <optional item description>'\r\n", "crystal");
    if (CMD_IS("restring"))
      send_to_char(ch, "You must place the item to restring and in the crafting station.\r\n");
    else if (CMD_IS("resize"))
      send_to_char(ch, "You must place the original item plus enough material in the station to resize it.\r\n");
    else if (CMD_IS("checkcraft"))
      send_to_char(ch, "You must place a an item to use as the mold pattern, a %s and your crafting resource materials\r\nin the station and then type 'checkcraft'\r\n", "crystal");
    return 1;
  }

  for (o = station->contains; o != NULL; o = o->next_content) {
    for (i = 0; i < NUM_CRAFT_TYPES; i++) {
      if (o && (GET_OBJ_VNUM(o) == craft_pattern_vnums(i) || (GET_OBJ_VNUM(o) >= 30200 && GET_OBJ_VNUM(o) <= 30299))) {
        orig = o;
        pattern_vnum = GET_OBJ_VNUM(o);
      }
    }
    if (o && GET_OBJ_TYPE(o) == ITEM_CRYSTAL) {
      orig = crystal = o;
      val = GET_OBJ_VAL(o, 0);
      crystal_level = crystal_bonus = bonus = GET_OBJ_LEVEL(o);

    }
    else if (o && GET_OBJ_TYPE(o) == ITEM_MATERIAL  && !IS_ESSENCE(o)) {
      if (GET_OBJ_VAL(o, 0) >= 2) {
        send_to_char(ch, "%s is a bundled item, which must first be unbundled before you can use it to craft.\r\n", o->short_description);
        return 1;
      }
      if (material != 0 && GET_OBJ_MATERIAL(o) != material)
        material_amount = 0;
      material = GET_OBJ_MATERIAL(o);
      mat_vnum = GET_OBJ_VNUM(o);
      material_level = GET_OBJ_LEVEL(o);
      material_amount++;
      ch->player_specials->crafting_exp_mult = 10 + MIN(60, GET_OBJ_LEVEL(o));
      if (dice(1, 100) < ((GET_GUILD(ch) != GUILD_ARTISANS) ? 0 : ((GET_GUILD_RANK(ch) + 2) / 4 * 5)) && !mat_freebie) {
        freebie = o;
        mat_freebie = TRUE;
      }
    }
    else if (CMD_IS("resize") || CMD_IS("restring"))
      orig = o;
  }

  if (orig && GET_OBJ_TYPE(orig) == ITEM_CONTAINER) {
    if (orig->contains) {
      send_to_char(ch, "You cannot restring bags that have items in them.\r\n");
      return 1;
    }
  }

  if (CMD_IS("augment")) {
    if (orig) {
      GET_OBJ_LEVEL(orig) = crystal_level + crystal_bonus;
      if (GET_OBJ_LEVEL(orig) <= crystal_level) {
        send_to_char(ch, "There is no possibility to augment any %ss in the station with that combination of items.\r\n", "crystal");
        return TRUE;
      }
      sprintf(buf, "@wa %s of@y %s@n max level@y %d@n", "crystal", apply_types[GET_OBJ_VAL(orig, 0)], GET_OBJ_LEVEL(orig));
      orig->name = strdup(buf);
      orig->short_description = strdup(buf);
      sprintf(buf, "@wA %s of@y %s@n max level@y %d@n lies here.", "crystal", apply_types[GET_OBJ_VAL(orig, 0)], GET_OBJ_LEVEL(orig));
      orig->description = strdup(buf);
    }
    else {
      send_to_char(ch, "No %s was detected in the crafting station to be augmented.", "crystal");
      return TRUE;
    }
  }

  if (CMD_IS("convert")) {
    if (material_amount > 0) {
      if ((material_amount % 100) != 0) {
        send_to_char(ch, "You must convert materials in multiple of 10 units.\r\n");
        return TRUE;
      }
    }
    else {
      send_to_char(ch, "There is no material in the station.\r\n");
      return TRUE;
    }
    if (convert_material_vnum(mat_vnum) > 0)
      orig = read_object(convert_material_vnum(mat_vnum), VIRTUAL);
    else {
      send_to_char(ch, "You do not have a valid material in the crafting station.\r\n");
      return TRUE;
    }
  }

  if (CMD_IS("supplyorder")) {
    if (GET_AUTOCQUEST_VNUM(ch) == 0) {
      send_to_char(ch, "You do not have a supply order active.\r\n");
      return 1;
    }
    if (GET_AUTOCQUEST_MAKENUM(ch) == 0) {
      send_to_char(ch, "You have completed your supply order, go turn it in.\r\n");
      return 1;
    }
    if (material_amount < 2) {
      send_to_char(ch, "You must place 2 units of %s or a similar type of material (all the same type) into the station to continue.\r\n", material_names[GET_AUTOCQUEST_MATERIAL(ch)]);
      return 1;
    }

    int obj_level = 0;

    craft_type = SCMD_CRAFT;
    if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_TINKERING)
      obj_level = get_skill_value(ch, SKILL_TINKERING);
    if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_WEAPONTECH)
      obj_level = get_skill_value(ch, SKILL_WEAPONTECH);
    if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_ARMORTECH)
      obj_level = get_skill_value(ch, SKILL_ARMORTECH);


    orig = read_object(30084, VIRTUAL);

    orig->name = strdup(GET_AUTOCQUEST_DESC(ch));
    orig->description = strdup(GET_AUTOCQUEST_DESC(ch));
    orig->short_description = strdup(GET_AUTOCQUEST_DESC(ch));

    GET_OBJ_LEVEL(orig) = obj_level;

    if (material_level > 1)
      GET_AUTOCQUEST_GOLD(ch) += (100 + (GET_ARTISAN_LEVEL(ch) * 10 * MAX(1, GET_ARTISAN_LEVEL(ch) - 1) * 2)) * material_level / 10;
  }

  if (orig) {
    CREATE(created, struct obj_data, 1);

    created->name = strdup(orig->name);
    created->description = strdup(orig->description);
    created->short_description = strdup(orig->short_description);
    created->ex_description = NULL;
    for (i = 0; i < 4; i++)
      created->wear_flags[i] = orig->wear_flags[i];
    for (i = 0; i < 4; i++)
      created->extra_flags[i] = orig->extra_flags[i];
    for (i = 0; i < 4; i++)
      created->bitvector[i] = orig->bitvector[i];
    created->weight = orig->weight;
    created->size = orig->size;
    for (i = 0; i < 4; i++) {
      created->affected[i+2].location = orig->affected[i].location;
      created->affected[i+2].modifier = orig->affected[i].modifier;
      created->affected[i+2].specific = orig->affected[i].specific;
    }
    for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      created->value[i] = orig->value[i];
    created->action_description = orig->action_description;
    created->ex_description = orig->ex_description;
    created->script = orig->script;
    created->sbinfo = orig->sbinfo;
    created->obj_flags = orig->obj_flags;
    created->proto_script = orig->proto_script;
    GET_OBJ_MATERIAL(created) = GET_OBJ_MATERIAL(orig);
    created->item_number = orig->item_number;
    created->in_room = orig->in_room;
    created->type_flag = orig->type_flag;
    if (CMD_IS("augment") || CMD_IS("supplyorder") || CMD_IS("restring"))
      GET_OBJ_LEVEL(created) = GET_OBJ_LEVEL(orig);
    GET_OBJ_COST(created) = GET_OBJ_COST(orig);
  }
  else {
    send_to_char(ch, "You must put a mold item in the crafting station first.\r\n"
                     "This is either a mundane item bought from one of the crafting vendors, or a crafted item\r\n"
                     "created from such.\r\n");
    return 1;
  }

  if (!val) {
    val = APPLY_NONE;
  }


  if (material == MATERIAL_MITHRIL)
    material_amount *= 2;

  if (!created && CMD_IS("create")) {
    send_to_char(ch, "You did not insert a valid object to use as the pattern.\r\nPlease insert a mundane item to be used to make the mold.\r\n");
    return 1;
  }

  if (CMD_IS("create") || CMD_IS("checkcraft")) {

  int mats_needed = MAX(5, GET_OBJ_WEIGHT(created) / 50);
  if ((HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? material_amount * 2 : material_amount) < mats_needed) {
    send_to_char(ch, "You do not have enough materials to make that item.  You need %d more units of the same type.\r\n",
                 mats_needed - (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? material_amount * 2 : material_amount));
    return 1;
  }

  if (val == APPLY_AC_DODGE && !CAN_WEAR(created, ITEM_WEAR_FEET)) {
      send_to_char(ch, "You can only imbue boots with an ac dodge enhancement.\r\n");
      return 1;
  }

  if (val == APPLY_HITROLL && !CAN_WEAR(created, ITEM_WEAR_HANDS)) {
      send_to_char(ch, "You can only imbue gauntlets or gloves with a hitroll enhancement.\r\n");
      return 1;
  }


  GET_OBJ_MATERIAL(created) = material;

  if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
    if ((weapon_list[GET_OBJ_VAL(created, 0)].material == MATERIAL_STEEL || weapon_list[GET_OBJ_VAL(created, 0)].material == MATERIAL_ENERGY) && !IS_HARD_METAL(material)) {
      send_to_char(ch, "You cannot make that item type with that material.\r\n");
      return 1;
    }
    if (weapon_list[GET_OBJ_VAL(created, 0)].material == MATERIAL_WOOD && !IS_WOOD(material)) {
      send_to_char(ch, "You cannot make that item type with that material.\r\n");
      return 1;
    }
    if (weapon_list[GET_OBJ_VAL(created, 0)].material == MATERIAL_COTTON && !IS_CLOTH(material)) {
      send_to_char(ch, "You cannot make that item type with that material.\r\n");
      return 1;
    }
    if (val == APPLY_AC_ARMOR || val == APPLY_AC_SHIELD) {
      send_to_char(ch, "You cannot imbue a weapon with armor or shield ac bonuses.\r\n");
      return 1;
    }

    skill = SKILL_WEAPONTECH;

    mn = 0;

    while (created->affected[mn].modifier != 0 && mn < 6) {
      if (val == created->affected[mn].location) {
        if (bonus > created->affected[mn].modifier)
          break;
        else {
          send_to_char(ch, "This item already has this property, but you need to add more microchips to make it more powerful.\r\n");
          return 1;
        }
      }
        mn++;
    }


    if (mn > 5) {
      send_to_char(ch, "You cannot add additional properties to this object.\r\n");
      return 1;
    }

  }
  else if (GET_OBJ_TYPE(created) == ITEM_ARMOR || GET_OBJ_TYPE(created) == ITEM_ARMOR_SUIT) {

    for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++) {
      if (GET_OBJ_VAL(created, 0) == armor_list[i].armorBonus &&
          GET_OBJ_VAL(created, 2) == armor_list[i].dexBonus)
          break;
    }

    i = GET_OBJ_VAL(created, 9);

    if (armor_list[i].material == MATERIAL_STEEL && !IS_HARD_METAL(material) && !IS_WOOD(material) &&
        ((HAS_FEAT(ch, FEAT_BONE_ARMOR) && material != MATERIAL_BONE) || !HAS_FEAT(ch, FEAT_BONE_ARMOR))) {
      send_to_char(ch, "You cannot make that item type with that material.\r\n");
      return 1;
    }
    if (armor_list[i].material == MATERIAL_LEATHER && !IS_LEATHER(material)) {
      send_to_char(ch, "You cannot make that item type with that material.\r\n");
      return 1;
    }
    if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
      send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
      return 1;
    }
    if (val == APPLY_FEAT && feat_list[GET_OBJ_VAL(crystal, 1)].combat_feat) {
      send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
      return 1;
    }
    if (val == APPLY_AC_ARMOR && i >= SPEC_ARMOR_TYPE_BUCKLER && i <= SPEC_ARMOR_TYPE_TOWER_SHIELD) {
      send_to_char(ch, "You cannot imbue shields with armor bonuses.\r\n");
      return 1;
    }
    if (val == APPLY_AC_SHIELD && i < SPEC_ARMOR_TYPE_BUCKLER) {
      send_to_char(ch, "You cannot imbue armor with shield bonuses.\r\n");
      return 1;
    }


    if (IS_HARD_METAL(material))
      skill = SKILL_ARMORTECH;
    else if (IS_LEATHER(material))
      skill = SKILL_ARMORTECH;
    else
      skill = SKILL_TINKERING;

    mn = 0;

    while (created->affected[mn].modifier != 0 && mn < 6) {
      if (val == created->affected[mn].location) {
        if ( (bonus) > created->affected[mn].modifier)
          break;
        else {
          send_to_char(ch, "This item already has this property, but you need to add more microchips to make it more powerful.\r\n");
          return 1;
        }
      }
        mn++;
    }

    if (mn > 5) {
      send_to_char(ch, "You cannot add additional properties to this object.  If attempting to enhance an existing property, you will need more powerful microchips.\r\n");
      return 1;
    }

  }
  else {
    if (IS_SET_AR(GET_OBJ_WEAR(created), ITEM_WEAR_BODY)) {
      if (!IS_CLOTH(material)) {
        send_to_char(ch, "You cannot make that item type with that material.\r\n");
        return 1;
      }
      if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
        send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
        return 1;
      }
      if (val == APPLY_FEAT && feat_list[GET_OBJ_VAL(crystal, 1)].combat_feat) {
        send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
        return 1;
      }
      if (val == APPLY_AC_SHIELD) {
        send_to_char(ch, "You cannot imbue clothing with shield bonuses.\r\n");
        return 1;
      }

      mn = 0;

      while (created->affected[mn].modifier != 0 && mn < 6) {
        if (val == created->affected[mn].location) {
          if ( (bonus) > created->affected[mn].modifier)
            break;
          else {
            send_to_char(ch, "This item already has this property, but you need to add more microchips to make it more powerful.\r\n");
            return 1;
          }
        }
        mn++;
      }

      if (mn > 5) {
        send_to_char(ch, "You cannot add additional properties to this object.  If attempting to enhance an existing property, you will need more powerful microchips.\r\n");
        return 1;
      }

      skill = SKILL_TINKERING;
    }
    else {
      if (!valid_misc_item_material_type(created, material)) {
        send_to_char(ch, "You cannot make that item type with that material.\r\n");
        return 1;
      }

      if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
        send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
        return 1;
      }
      if (val == APPLY_FEAT && feat_list[GET_OBJ_VAL(crystal, 1)].combat_feat) {
        send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
        return 1;
      }
      if (val == APPLY_AC_SHIELD || val == APPLY_AC_ARMOR) {
        send_to_char(ch, "You cannot imbue miscellaneous items with armor or shield bonuses.\r\n");
        return 1;
      }

      mn = 0;

      while (created->affected[mn].modifier != 0 && mn < 6) {
        if (val == created->affected[mn].location) {
          if ( (bonus) > created->affected[mn].modifier)
            break;
          else {
            send_to_char(ch, "This item already has this property, but you need to add more microchips to make it more powerful.\r\n");
            return 1;
          }
        }
        mn++;
      }

      if (mn > 5) {
        send_to_char(ch, "You cannot add additional properties to this object.  If attempting to enhance an existing property, you will need more powerful microchips.\r\n");
        return 1;
      }

      if (CAN_WEAR(created, ITEM_WEAR_FINGER) || CAN_WEAR(created, ITEM_WEAR_ABOVE))
        skill = SKILL_TANNING;
      else
        skill = SKILL_TINKERING;

    }
  }

  if (get_skill_value(ch, skill) < GET_OBJ_LEVEL(created)) {
    send_to_char(ch, "Your skill in %s is too low to create that item.\r\n", spell_info[skill].name);
    return 1;
  }


  if (!strstr(argument, material_names[material]) && !CMD_IS("checkcraft")) {
    send_to_char(ch, "You must include the material name, '%s', in the object description somewhere.\r\n", material_names[material]);
    return 1;
  }

  for (i = 0; i < NUM_CRAFT_TYPES; i++)
    if (pattern_vnum == craft_pattern_vnums(i))
      break;

  int minbonus = 0;


  if (bonus > 0) {
  if (val >= APPLY_SPELL_LVL_0 && val <= APPLY_SPELL_LVL_9)
    bonus = MAX(1, bonus / 3);
  if ((val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR))
    bonus *= 10;
  if (val == APPLY_HIT)
    bonus *= 5;
  if (val == APPLY_KI)
    bonus *= 5;
  if (val == APPLY_MOVE)
    bonus *= 100;

  minbonus = 1;

  if ((val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR))
    minbonus = 10;
  if (val == APPLY_HIT)
    minbonus = 5;
  if (val == APPLY_KI)
    minbonus = 5;
  if (val == APPLY_MOVE)
    minbonus = 100;

  }

      mn = 0;

      while (created->affected[mn].modifier != 0 && mn < 6) {
        if (val == created->affected[mn].location) {
          if ( (bonus) > created->affected[mn].modifier)
            break;
          else {
            send_to_char(ch, "This item already has this property, but you need to add more microchips to make it more powerful.\r\n");
            return 1;
          }
        }
        mn++;
      }

    if (mn > 5) {
      send_to_char(ch, "You cannot add additional properties to this object.\r\n");
      return 1;
    }





  if (val != APPLY_NONE) {
    created->affected[mn].location = val;
    created->affected[mn].modifier = bonus;
    if (val == APPLY_FEAT || val == APPLY_SKILL) {
      created->affected[mn].specific = GET_OBJ_VAL(crystal, 1);;
    }

    if (val == APPLY_ACCURACY) {
        created->affected[mn+1].location = APPLY_DAMAGE;
        created->affected[mn+1].modifier = bonus;
        if ((crystal_level % 5) >= 2)
          created->affected[mn].modifier++;

    }
    else if (val == APPLY_DAMAGE) {
        created->affected[mn+1].location = APPLY_ACCURACY;
        created->affected[mn+1].modifier = bonus;
        if ((crystal_level % 5) >= 2)
          created->affected[mn].modifier++;
    }
  }

  SET_BIT_AR(GET_OBJ_EXTRA(created), ITEM_UNIQUE_SAVE);
  GET_OBJ_LEVEL(created) = set_object_level(created);
  GET_OBJ_COST(created) = 100 + GET_OBJ_LEVEL(created) * 50 * MAX(1, GET_OBJ_LEVEL(created) - 1) + GET_OBJ_COST(created);

  while (val && bonus != minbonus && GET_OBJ_LEVEL(created) > GET_OBJ_LEVEL(crystal)) {
    if ((val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR))
      created->affected[mn].modifier -= 10;
    else if (val == APPLY_HIT)
      created->affected[mn].modifier -= 5;
    else if (val == APPLY_KI)
      created->affected[mn].modifier -= 5;
    else if (val == APPLY_MOVE)
      created->affected[mn].modifier -= 100;
    else if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
      created->affected[mn].modifier -= 1;
      created->affected[mn+1].modifier -= 1;
    }
    else
      created->affected[mn].modifier -= 1;
    GET_OBJ_LEVEL(created) = set_object_level(created);
    GET_OBJ_COST(created) = 100 + GET_OBJ_LEVEL(created) * 50 * MAX(1, GET_OBJ_LEVEL(created) - 1) + GET_OBJ_COST(created);
  }

  while (val && GET_OBJ_LEVEL(created) >= CONFIG_LEVEL_CAP) {
    if ((val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR))
      created->affected[mn].modifier -= 10;
    else if (val == APPLY_HIT)
      created->affected[mn].modifier -= 5;
    else if (val == APPLY_KI)
      created->affected[mn].modifier -= 5;
    else if (val == APPLY_MOVE)
      created->affected[mn].modifier -= 100;
    else if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
      created->affected[mn].modifier -= 1;
      created->affected[mn+1].modifier -= 1;
    }
    else
      created->affected[mn].modifier -= 1;
    GET_OBJ_LEVEL(created) = set_object_level(created);
    GET_OBJ_COST(created) = 100 + GET_OBJ_LEVEL(created) * 50 * MAX(1, GET_OBJ_LEVEL(created) - 1) + GET_OBJ_COST(created);
  }

  if ((CMD_IS("create") || CMD_IS("checkcraft")) && ch->crafting_level > 0 && ch->crafting_level <= get_skill_value(ch, skill)) {
    send_to_char(ch, "You will be making an item of level %d.\r\n", ch->crafting_level);
    int icl = 0;
    while (val && GET_OBJ_LEVEL(created) > ch->crafting_level) {
      if ((val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR))
        created->affected[mn].modifier -= 10;
      else if (val == APPLY_HIT)
        created->affected[mn].modifier -= 5;
      else if (val == APPLY_KI)
        created->affected[mn].modifier -= 5;
      else if (val == APPLY_MOVE)
        created->affected[mn].modifier -= 100;
      else if (val == APPLY_ACCURACY || val == APPLY_DAMAGE) {
        if (icl % 2 == 0)
          created->affected[mn].modifier -= 1;
        else
          created->affected[mn+1].modifier -= 1;
      }
      else
        created->affected[mn].modifier -= 1;
      GET_OBJ_LEVEL(created) = set_object_level(created);
      GET_OBJ_COST(created) = 100 + GET_OBJ_LEVEL(created) * 50 * MAX(1, GET_OBJ_LEVEL(created) - 1) + GET_OBJ_COST(created);
    }
    if (CMD_IS("create")) {
      send_to_char(ch, "Your crafting level has been turned off.  Type clevel (level) to re-set it.\r\n");
      ch->crafting_level = 0;
    }
  }



  craft_type = SCMD_CRAFT;

  if (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING))
  {
    GET_OBJ_WEIGHT(created) /= 2;
  }

  // Code for adding Branding Mark.
  if (HAS_FEAT(ch, FEAT_BRANDING))
  {
    char buf_brand[MAX_STRING_LENGTH]={'\0'};

    sprintf(buf_brand, "This also bears the mark of %s", ch->name);

    created->description = strdup(buf_brand);
  }

  if (HAS_FEAT(ch, FEAT_MASTERWORK_CRAFTING)) {
    if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      for (j = 0; j < 6; j++)
        if (created->affected[j].location == APPLY_ACCURACY)
          created->affected[j].modifier += 1;
    }
    if (GET_OBJ_TYPE(created) == ITEM_ARMOR_SUIT || GET_OBJ_TYPE(created) == ITEM_ARMOR) {
      GET_OBJ_VAL(created, VAL_ARMOR_SKILL) -= 1;
    }
  }

  if (HAS_FEAT(ch, FEAT_DWARVEN_CRAFTING)) {
    if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      for (j = 0; j < 6; j++)
        if (created->affected[j].location == APPLY_DAMAGE)
          created->affected[j].modifier += 1;
    }
    if (GET_OBJ_TYPE(created) == ITEM_ARMOR_SUIT || GET_OBJ_TYPE(created) == ITEM_ARMOR) {
      GET_OBJ_VAL(created, 2) += 1;
    }
  }

  if (HAS_FEAT(ch, FEAT_DRACONIC_CRAFTING)) {
    if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      for (j = 0; j < 6; j++)
        if (created->affected[j].location == APPLY_DAMAGE)
          created->affected[j].modifier += 1;
    }
    if (created->affected[mn].location == APPLY_AC_DEFLECTION ||
        created->affected[mn].location == APPLY_AC_NATURAL ||
        created->affected[mn].location == APPLY_AC_ARMOR ||
        (created->affected[mn].location == APPLY_AC_SHIELD) ||
        created->affected[mn].location == APPLY_AC_DODGE)
        created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_HIT)
      created->affected[mn].modifier += 5;
    else if (created->affected[mn].location == APPLY_KI)
      created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_MOVE)
      created->affected[mn].modifier += 100;
    else if (created->affected[mn].location == APPLY_SPELL_LVL_0 ||
        created->affected[mn].location == APPLY_SPELL_LVL_1 ||
        created->affected[mn].location == APPLY_SPELL_LVL_2 ||
        created->affected[mn].location == APPLY_SPELL_LVL_3 ||
        created->affected[mn].location == APPLY_SPELL_LVL_4 ||
        created->affected[mn].location == APPLY_SPELL_LVL_5 ||
        created->affected[mn].location == APPLY_SPELL_LVL_6 ||
        created->affected[mn].location == APPLY_SPELL_LVL_7 ||
        created->affected[mn].location == APPLY_SPELL_LVL_8 ||
        created->affected[mn].location == APPLY_SPELL_LVL_9)
      created->affected[mn].modifier += 1;
    else if (created->affected[mn].location == APPLY_ACCURACY) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_DAMAGE;
      created->affected[mn+1].modifier += 1;
    }
    else if (created->affected[mn].location == APPLY_DAMAGE) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_ACCURACY;
      created->affected[mn+1].modifier += 1;
    }
    else
      created->affected[mn].modifier += 1;

  }

  int mnt = mn;
  mn = 0;
  while (crystal && created->affected[mn].modifier <= 0) {
    if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      created->affected[mn+1].location = APPLY_DAMAGE;
      created->affected[mn+1].modifier += 1;
    }
    if (created->affected[mn].location == APPLY_AC_DEFLECTION ||
        created->affected[mn].location == APPLY_AC_NATURAL ||
        created->affected[mn].location == APPLY_AC_ARMOR ||
        (created->affected[mn].location == APPLY_AC_SHIELD)||
        created->affected[mn].location == APPLY_AC_DODGE)
        created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_HIT)
      created->affected[mn].modifier += 5;
    else if (created->affected[mn].location == APPLY_KI)
      created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_MOVE)
      created->affected[mn].modifier += 100;
    else if (created->affected[mn].location == APPLY_SPELL_LVL_0 ||
        created->affected[mn].location == APPLY_SPELL_LVL_1 ||
        created->affected[mn].location == APPLY_SPELL_LVL_2 ||
        created->affected[mn].location == APPLY_SPELL_LVL_3 ||
        created->affected[mn].location == APPLY_SPELL_LVL_4 ||
        created->affected[mn].location == APPLY_SPELL_LVL_5 ||
        created->affected[mn].location == APPLY_SPELL_LVL_6 ||
        created->affected[mn].location == APPLY_SPELL_LVL_7 ||
        created->affected[mn].location == APPLY_SPELL_LVL_8 ||
        created->affected[mn].location == APPLY_SPELL_LVL_9)
      created->affected[mn].modifier += 1;
    else if (created->affected[mn].location == APPLY_ACCURACY) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_DAMAGE;
      created->affected[mn+1].modifier += 1;
    }
    else if (created->affected[mn].location == APPLY_DAMAGE) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_ACCURACY;
      created->affected[mn+1].modifier += 1;
    }
    else
      created->affected[mn].modifier += 1;
  }

  mn = mnt;

  rand = dice(1, 100);
  int x = 0;
  while ((rand <= (1 + get_rp_bonus(ch, RP_CRAFT) + ((GET_GUILD(ch) != GUILD_ARTISANS) ? 0 : (GET_GUILD_RANK(ch) / 4)))) ||
         (crystal && GET_OBJ_VAL(crystal, 2) > 0)) {
    if (GET_OBJ_VAL(crystal, 2) > 0)
      GET_OBJ_VAL(crystal, 2)--;
    x++;
    rand = dice(1, 100);
    if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      created->affected[mn+1].location = APPLY_DAMAGE;
      created->affected[mn+1].modifier += 1;
    }
    if (created->affected[mn].location == APPLY_AC_DEFLECTION ||
        created->affected[mn].location == APPLY_AC_NATURAL ||
        created->affected[mn].location == APPLY_AC_ARMOR ||
        (created->affected[mn].location == APPLY_AC_SHIELD)||
        created->affected[mn].location == APPLY_AC_DODGE)
        created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_HIT)
      created->affected[mn].modifier += 5;
    else if (created->affected[mn].location == APPLY_KI)
      created->affected[mn].modifier += 10;
    else if (created->affected[mn].location == APPLY_MOVE)
      created->affected[mn].modifier += 100;
    else if (created->affected[mn].location == APPLY_SPELL_LVL_0 ||
        created->affected[mn].location == APPLY_SPELL_LVL_1 ||
        created->affected[mn].location == APPLY_SPELL_LVL_2 ||
        created->affected[mn].location == APPLY_SPELL_LVL_3 ||
        created->affected[mn].location == APPLY_SPELL_LVL_4 ||
        created->affected[mn].location == APPLY_SPELL_LVL_5 ||
        created->affected[mn].location == APPLY_SPELL_LVL_6 ||
        created->affected[mn].location == APPLY_SPELL_LVL_7 ||
        created->affected[mn].location == APPLY_SPELL_LVL_8 ||
        created->affected[mn].location == APPLY_SPELL_LVL_9)
      created->affected[mn].modifier += 1;
    else if (created->affected[mn].location == APPLY_ACCURACY) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_DAMAGE;
      created->affected[mn+1].modifier += 1;
    }
    else if (created->affected[mn].location == APPLY_DAMAGE) {
      created->affected[mn].modifier += 1;
      created->affected[mn+1].location = APPLY_ACCURACY;
      created->affected[mn+1].modifier += 1;
    }
    else
      created->affected[mn].modifier += 1;
  }
  if (x > 0)
    send_to_char(ch, "@l@WYou have received a critical success on your craft! (+%d)@n\r\n", x);

  if (essence_one && IS_ESSENCE(essence_one))
    SET_BIT_AR(GET_OBJ_EXTRA(created), ITEM_MAGIC);

  SET_BIT_AR(GET_OBJ_EXTRA(created), ITEM_IDENTIFIED);

  }
  else if (CMD_IS("resize")) {

    if (is_abbrev(argument, "fine"))
      newsize = SIZE_FINE;
    else if (is_abbrev(argument, "diminutive"))
      newsize = SIZE_DIMINUTIVE;
    else if (is_abbrev(argument, "tiny"))
      newsize = SIZE_TINY;
    else if (is_abbrev(argument, "small"))
      newsize = SIZE_SMALL;
    else if (is_abbrev(argument, "medium"))
      newsize = SIZE_MEDIUM;
    else if (is_abbrev(argument, "large"))
      newsize = SIZE_LARGE;
    else if (is_abbrev(argument, "huge"))
      newsize = SIZE_HUGE;
    else if (is_abbrev(argument, "gargantuan"))
      newsize = SIZE_GARGANTUAN;
    else if (is_abbrev(argument, "colossal"))
      newsize = SIZE_COLOSSAL;
    else {
      send_to_char(ch, "That is not a valid size: (fine|diminutive|tiny|small|medium|large|huge|gargantuan|colossal)\r\n");
      return 1;
    }

    if (material_amount < (5 * ((newsize - orig->size > 0) ? (((newsize - orig->size - 1) * 100) + 50) : 0) / 100)) {
      send_to_char(ch, "You need to put in some more %s to resize %s.\r\n", material_names[GET_OBJ_MATERIAL(orig)], orig->short_description);
      return 1;
    }

    if (GET_OBJ_MATERIAL(orig) != material && (newsize - orig->size) > 0) {
      send_to_char(ch, "You need to put in some %s to resize %s.\r\n", material_names[GET_OBJ_MATERIAL(orig)], orig->short_description);
      return 1;
    }

    for (i = 0; i < newsize - orig->size; i++) {
      GET_OBJ_WEIGHT(orig) *= 15;
      GET_OBJ_WEIGHT(orig) /= 10;
    }

    for (i = 0; i < orig->size - newsize; i++) {
      GET_OBJ_WEIGHT(orig) /= 2;
    }

    send_to_char(ch, "You resize %s from %s to %s.\r\n", created->short_description, size_names[orig->size], size_names[newsize]);
    act("$n resizes $p.", FALSE, ch, created, 0, TO_ROOM);

    orig->size = newsize;

    if (GET_OBJ_TYPE(orig) == ITEM_WEAPON) {
        ndice = GET_OBJ_VAL(orig, VAL_WEAPON_DAMDICE);
        diesize = GET_OBJ_VAL(orig, VAL_WEAPON_DAMSIZE);

        sz = orig->size - weapon_list[GET_OBJ_VAL(orig, 0)].size + SIZE_MEDIUM;

        if (sz < SIZE_MEDIUM)
          for (lvl = sz; lvl < SIZE_MEDIUM; lvl++)
            scaledown_dam(&ndice, &diesize);
        else if (sz > SIZE_MEDIUM)
          for (lvl = sz; lvl > SIZE_MEDIUM; lvl--)
            scaleup_dam(&ndice, &diesize);

        GET_OBJ_VAL(orig, VAL_WEAPON_DAMDICE) = ndice;
        GET_OBJ_VAL(orig, VAL_WEAPON_DAMSIZE) = diesize;

    }

    craft_type = SCMD_RESIZE;

    obj_from_obj(orig);

    obj_to_char(orig, ch);

  }

  int cost = GET_OBJ_LEVEL(created) * GET_OBJ_LEVEL(created) * 100 / 3;

  if (CMD_IS("create") || CMD_IS("restring"))
  {
    created->name = strdup(argument);
    created->short_description = strdup(argument);
    sprintf(buf, "%s lies here.", CAP(argument));
    created->description = strdup(buf);
    if (GET_OBJ_TYPE(created) == ITEM_ARMOR) {
      GET_OBJ_COST(created) = armor_list[GET_OBJ_VAL(created, 9)].cost;
      GET_OBJ_COST(created) += cost;
    }
    else if (GET_OBJ_TYPE(created) == ITEM_WEAPON) {
      GET_OBJ_COST(created) = weapon_list[GET_OBJ_VAL(created, 0)].cost;
      GET_OBJ_COST(created) += cost;
    } else {
      GET_OBJ_COST(created) = cost;
    }
  }

  int num_chips = 0;

  if (CMD_IS("create") || CMD_IS("checkcraft")) {
    for (o = station->contains; o != NULL; o = o->next_content) {
      if (o && GET_OBJ_TYPE(o) == ITEM_CRYSTAL) {
        num_chips++;
      }
    }
    if (num_chips > 1) {
      send_to_char(ch, "You can only use one %s per craft.\r\n", "crystal");
      return 1;
    }
    if (GET_OBJ_TYPE(created) == ITEM_ARMOR)
      GET_OBJ_VAL(created, 2) = armor_list[GET_OBJ_VAL(created, 9)].dexBonus;
  }

  if (CMD_IS("checkcraft")) {
    send_to_char(ch, "This crafting session will create the following item:\r\n\r\n");
    spell_identify(20, ch, ch, created, NULL);
    send_to_char(ch, "It will make use of your %s skill, which has a value of %d.\r\n", spell_info[skill].name, get_skill_value(ch, skill));
    send_to_char(ch, "This crafting session will take 60 seconds.\r\n");
    send_to_char(ch, "You need %d %s on hand or in the bank to make this item.\r\n", cost, MONEY_STRING);
    return 1;
  }

  if (CMD_IS("restring")) {
    send_to_char(ch, "You put the item into the crafting station and wait for it to transform into %s.\r\n", created->short_description);
  }

  if (CMD_IS("create") || CMD_IS("restring") || CMD_IS("augment") || CMD_IS("convert") || CMD_IS("supplyorder")) {

    if (CMD_IS("restring")) {
      if (created->ex_description) {
        send_to_char(ch, "You cannot restring items with extra descriptions.\r\n");
        return 1;
      }
    }


    int skilltype = SKILL_TINKERING;

    switch (GET_OBJ_TYPE(created)) {
      case ITEM_WEAPON:
        skilltype = SKILL_WEAPONTECH;
        break;
      case ITEM_ARMOR:
        skilltype = SKILL_ARMORTECH;
        break;
      default:
        if (CAN_WEAR(created, ITEM_WEAR_FINGER) || CAN_WEAR(created, ITEM_WEAR_ABOVE))
          skilltype = SKILL_TANNING;
        else
          skilltype = SKILL_TINKERING;
        break;
    }

    if (CMD_IS("create") || CMD_IS("restring") || CMD_IS("augment") || CMD_IS("convert")) {
      if (CMD_IS("create") && GET_OBJ_LEVEL(created) > get_skill_value(ch, skilltype)) {
        send_to_char(ch, "The item level is %d but your %s skill is only %d.\r\n", GET_OBJ_LEVEL(created), spell_info[skilltype].name, get_skill_value(ch, skilltype));
        return 1;
      }
      if (GET_GOLD(ch) < cost && !CMD_IS("supplyorder")) {
      send_to_char(ch, "You need %d %s on hand for supplies to make this item.\r\n", cost, MONEY_STRING);
      return 1;
      }
      send_to_char(ch, "It cost you %d %s in supplies to create this item.\r\n", cost, MONEY_STRING);
      GET_GOLD(ch) -= cost;
    }

    if (CMD_IS("supplyorder"))
      cost = 0;

    REMOVE_BIT_AR(GET_OBJ_EXTRA(created), ITEM_MOLD);
    if (CMD_IS("restring"))
      ch->restringing = TRUE;
    GET_CRAFTING_TYPE(ch) = craft_type;
    GET_CRAFTING_TICKS(ch) = GET_ADMLEVEL(ch) ? 1 : 15;
    GET_CRAFTING_TICKS(ch) -= MAX(10, HAS_FEAT(ch, FEAT_FAST_CRAFTER));
    GET_CRAFTING_OBJ(ch) = created;

    if (CMD_IS("convert")) {
      ch->craft_vnum = mat_vnum;
      ch->craft_times = MAX(0, (material_amount / 100) - 1);
    }

    obj_from_obj(created);

    if (CMD_IS("create")) {
      send_to_char(ch, "You begin to craft %s.\r\n", created->short_description);
      act("$n begins to craft $p.", FALSE, ch, created, 0, TO_ROOM);
    }
    if (CMD_IS("supplyorder")) {
      send_to_char(ch, "You begin a supply order for %s.\r\n", created->short_description);
      act("$n begins a supply order for $p.", FALSE, ch, created, 0, TO_ROOM);
    }

    if (CMD_IS("augment")) {
      send_to_char(ch, "You begin to augment %s.\r\n", created->short_description);
      act("$n begins to craft $p.", FALSE, ch, created, 0, TO_ROOM);
    }
  }

  if (freebie) {
    send_to_char(ch, "One of %s was refunded to your due to your artisan guild bonus.\r\n", freebie->short_description);
    obj_from_obj(freebie);
    obj_to_char(freebie, ch);
  }


  station_obj_vnum = GET_OBJ_VNUM(station);

  obj_from_room(station);

  extract_obj(station);

  station = read_object(station_obj_vnum, VIRTUAL);

  obj_to_room(station, IN_ROOM(ch));

  return 1;

}

// SPECIAL(buy_potion)
// {
//     struct obj_data *obj = NULL;
//     char buf[100] = {'\0'};
//     char keywords[100] = {'\0'};
//     int i = 0, j = 0, spelllevel = 0, cost = 0;

//     if (!CMD_IS("buy"))
//     {
//         return 0;
//     }

//     skip_spaces(&argument);

//     for (i = 0; i <= TOP_SPELL; i++)
//     {
//         if (!strcmp(spell_info[i].name, argument))
//         {
//             break;
//         }
//     }

//     if (i > TOP_SPELL)
//     {
//         send_to_char(ch, "That isn't a valid spell.\r\n");
//         return 1;
//     }

//     if (spell_info[i].violent == TRUE)
//     {
//         send_to_char(ch, "You cannot put that spell in a potion.\r\n");
//         return 1;
//     }

//     if (!IS_SET(spell_info[i].targets, TAR_CHAR_ROOM))
//     {
//         send_to_char(ch, "You cannot put that spell in a potion.\r\n");
//         return 1;
//     }

//     spelllevel = spell_info[i].spell_level;

//     if (spelllevel > 9)
//     {
//         send_to_char(ch, "You cannot buy a potion for that spell.\r\n");
//         return 1;
//     }

//     cost = MAX(1, spelllevel) * 25 * (MAX(1, spelllevel * MAX(2, spelllevel / 2)));

//     if (cost > GET_GOLD(ch))
//     {
//         send_to_char(ch, "That potion costs %d gold, you only have %d on hand.\r\n", cost, GET_GOLD(ch));
//         return 1;
//     }

//     obj = read_object(30099, VIRTUAL);

//     if (!obj)
//     {
//         return 0;
//     }

//     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

//     GET_OBJ_COST(obj) = cost;

//     GET_OBJ_MATERIAL(obj) = MATERIAL_GLASS;

//     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

//     GET_OBJ_COST(obj) = spelllevel * 25 * (MAX(1, spelllevel * MAX(2, spelllevel / 2)));

//     GET_OBJ_VAL(obj, 1) = i;

//     GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

//     GET_OBJ_LEVEL(obj) = GET_OBJ_VAL(obj, VAL_POTION_LEVEL);

//     GET_OBJ_VAL(obj, VAL_POTION_LEVEL) = 20;

//     snprintf(keywords, sizeof(keywords), "potion-%s", spell_info[i].name);

//     for (j = 0; j < strlen(keywords); j++)
//     {
//         if (keywords[j] == ' ')
//         {
//             keywords[j] = '-';
//         }
//     }

//     snprintf(buf, sizeof(buf), "vial potion %s", keywords);
//     obj->name = strdup(buf);
//     snprintf(buf, sizeof(buf), "a potion of %s", spell_info[i].name);
//     obj->short_description = strdup(buf);
//     snprintf(buf, sizeof(buf), "A potion of %s lies here.", spell_info[i].name);
//     obj->description = strdup(buf);

//     obj_to_char(obj, ch);

//     GET_GOLD(ch) -= cost;

//     send_to_char(ch, "You purchase %s for %d gold.\r\n", obj->short_description, cost);

//     return 1;
// }

/*
SPECIAL(use_credit) {

  if (!CMD_IS("use")) {
    return 0;
  }

  char arg[200];

  one_arguments(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax is use credit <option>\r\nValid options are:\r\n:"
    return 1;
  }

  if (strcmp(arg, "credit")) {

    return 1;
  }

  GET_CREDITS(ch) += GET_OBJ_VAL((struct obj_data *) me, 0);

  send_to_char(ch, "You have gained %d credits!\r\n", GET_OBJ_VAL((struct obj_data *) me, 0));

  return 1;
};
*/

SPECIAL(bounty_contractor)
{

  if (!CMD_IS("bounty")) {
    return 0;
  }

  char arg[200]={'\0'};
  char arg2[200]={'\0'};

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Type 'bounty new' for a new bounty, 'bounty complete' to finish your bounty and receive your reward or 'bounty quit' to quit your bounty.\r\n");
    if (GET_AUTOQUEST_VNUM(ch) > 0) {
      send_to_char(ch, "You have not yet completed your bounty contract against %s.\r\n"
                       "You still need to kill %d more.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d experience points.\r\n",
                       GET_AUTOQUEST_DESC(ch), GET_AUTOQUEST_KILLNUM(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch));
    }
    return 1;
  }

  if (!strcmp(arg, "new")) {

    if (GET_OBJ_VNUM((struct obj_data *)me) == (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? 2237 : (60100)) && ch->bounty_gem > 0 && GET_CLASS_RANKS(ch, CLASS_BOUNTY_HUNTER) == 0) {
      send_to_char(ch, "You must wait %d minutes and %d seconds longer to use your gem.\r\n", ch->bounty_gem / 10, (ch->bounty_gem % 10) * 6);
      return 1;
    }


    if (GET_AUTOQUEST_VNUM(ch) > 0 && GET_AUTOQUEST_KILLNUM(ch) < 1) {
      send_to_char(ch, "You can't take a new bounty until you've handed in the one you've completed.\r\n");
      return 1;
    }

  int count = 0;

  GET_AUTOQUEST_VNUM(ch) = 0;
  GET_AUTOQUEST_KILLNUM(ch) = 0;
  GET_AUTOQUEST_QP(ch) = 0;
  GET_AUTOQUEST_EXP(ch) = 0;
  GET_AUTOQUEST_GOLD(ch) = 0;
  GET_AUTOQUEST_DESC(ch) = strdup("nothing");

  struct char_data *i = NULL;
  struct char_data *next_char = NULL;
  struct char_data *vict = NULL;
  int level = 0;

  if (!*arg2) {
    level = GET_LEVEL(ch);
  }
  else if ((vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
    level = GET_LEVEL(vict);
  }
  else {
    level = atoi(arg2);
  }

  if (level < 1 || level > (GET_LEVEL(ch) + 5)) {
    send_to_char(ch, "You must choose a level between 1 and your total class levels or specify a specific mob.\r\n");
    return 1;
  }

  if (!vict) {
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    if (!IS_NPC(i) || MOB_FLAGGED(i, MOB_NOKILL) || AFF_FLAGGED(i, AFF_CHARM) || i->master || MOB_FLAGGED(i, MOB_INNOCENT) ||
        GET_LEVEL(i) > (level + 3) || GET_LEVEL(i) < (level - 3) ||
        zone_table[world[IN_ROOM(i)].zone].zone_status < 2)
        continue;

    count++;
  }

  int rand = dice(1, count);
  count = 0;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    if (!IS_NPC(i) || MOB_FLAGGED(i, MOB_NOKILL) || AFF_FLAGGED(i, AFF_CHARM) || i->master || MOB_FLAGGED(i, MOB_INNOCENT) ||
        GET_LEVEL(i) > (level + 3) || GET_LEVEL(i) < (level - 3) ||
        zone_table[world[IN_ROOM(i)].zone].zone_status < 2)
        continue;

    count++;

    if (count == rand) {
      vict = i;
      break;
    }
  }
  }

  if (!vict) {
    send_to_char(ch, "Hmm, I don't think I have anything for you right now, ask me again in a few moments.\r\n");
    return 1;
  }

  count = 0;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    if (!IS_NPC(i) || MOB_FLAGGED(i, MOB_NOKILL) || AFF_FLAGGED(i, AFF_CHARM) || i->master || MOB_FLAGGED(i, MOB_INNOCENT) ||
        GET_LEVEL(i) > (level + 3) || GET_LEVEL(i) < (level - 3) ||
        zone_table[world[IN_ROOM(i)].zone].zone_status < 2)
        continue;

    if (GET_MOB_VNUM(i) == GET_MOB_VNUM(vict))
      count++;
  }

//  if (count > 1)
//    count = dice(1, count);

  count = MIN(10, count);

  if (count < 1) {
    send_to_char(ch, "Hmm, I don't think I have anything for you right now, ask me again in a few moments.\r\n");
    return 1;
  }

  GET_AUTOQUEST_VNUM(ch) = GET_MOB_VNUM(vict);
	free(GET_AUTOQUEST_DESC(ch));
	/* Supposed to be strdup()ed */
  GET_AUTOQUEST_DESC(ch) = which_desc(vict);
  GET_AUTOQUEST_KILLNUM(ch) = count;
  GET_AUTOQUEST_QP(ch) = MAX(1, GET_LEVEL(vict) * count / 5);
  GET_AUTOQUEST_EXP(ch) = mob_exp_by_level(MIN(GET_LEVEL(vict), GET_LEVEL(ch))) * count;
  GET_AUTOQUEST_GOLD(ch) = (100 + MIN(GET_LEVEL(ch), GET_LEVEL(vict)) * 10 * MAX(1, MIN(GET_LEVEL(ch), GET_LEVEL(vict)) - 1) * count) / 10;

  if (MOB_FLAGGED(vict, MOB_BOSS)) {
    GET_AUTOQUEST_QP(ch) = GET_AUTOQUEST_QP(ch) * 7 / 2;
    GET_AUTOQUEST_EXP(ch) = GET_AUTOQUEST_EXP(ch) * 7 / 2;
    GET_AUTOQUEST_GOLD(ch) = GET_AUTOQUEST_GOLD(ch) * 7 / 2;
  }
  else if (MOB_FLAGGED(vict, MOB_FINAL_BOSS)) {
    GET_AUTOQUEST_QP(ch) = GET_AUTOQUEST_QP(ch) * 9 / 2;
    GET_AUTOQUEST_EXP(ch) = GET_AUTOQUEST_EXP(ch) * 9 / 2;
    GET_AUTOQUEST_GOLD(ch) = GET_AUTOQUEST_GOLD(ch) * 9 / 2;
  }
  else if (MOB_FLAGGED(vict, MOB_CAPTAIN)) {
    GET_AUTOQUEST_QP(ch) = GET_AUTOQUEST_QP(ch) * 5 / 2;
    GET_AUTOQUEST_EXP(ch) = GET_AUTOQUEST_EXP(ch) * 5 / 2;
    GET_AUTOQUEST_GOLD(ch) = GET_AUTOQUEST_GOLD(ch) * 5 / 2;
  }
  else if (MOB_FLAGGED(vict, MOB_LIEUTENANT)) {
    GET_AUTOQUEST_QP(ch) = GET_AUTOQUEST_QP(ch) * 3 / 2;
    GET_AUTOQUEST_EXP(ch) = GET_AUTOQUEST_EXP(ch) * 3 / 2;
    GET_AUTOQUEST_GOLD(ch) = GET_AUTOQUEST_GOLD(ch) * 3 / 2;
  }

  if (HAS_FEAT(ch, FEAT_RUTHLESS_NEGOTIATOR)) {
    GET_AUTOQUEST_QP(ch) = GET_AUTOQUEST_QP(ch) * 2;
    GET_AUTOQUEST_EXP(ch) = GET_AUTOQUEST_EXP(ch) * 2;
    GET_AUTOQUEST_GOLD(ch) = GET_AUTOQUEST_GOLD(ch) * 2;
  }

	char *tmpdesc = which_desc(vict);
  send_to_char(ch, "You have been commissioned for a bounty to kill %s.  We expect you to kill at least %d before you can collect your reward.  Good luck.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation points.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d experience points.\r\n",

                   tmpdesc, GET_AUTOQUEST_KILLNUM(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch));
	free(tmpdesc);

    if (GET_OBJ_VNUM((struct obj_data *)me) == 2237) {
      ch->bounty_gem = 50;
      send_to_char(ch, "You will be able to use your bounty gem again in 5 minutes.\r\n");
    }
  } // end bounty new

  else if (!strcmp(arg, "complete")) {

    if (GET_AUTOQUEST_VNUM(ch) > 0 && GET_AUTOQUEST_KILLNUM(ch) < 1) {

      send_to_char(ch, "You have completed your bounty contract against %s.\r\n"
                       "You receive %d reputation points.\r\n"
                       "%d credits have been deposited into your bank account.\r\n"
                       "You receive %d experience points.\r\n",
                       GET_AUTOQUEST_DESC(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch) / 3);

      GET_QUESTPOINTS(ch) += GET_AUTOQUEST_QP(ch);
      GET_REPUTATION(ch) += GET_AUTOQUEST_QP(ch);
//      GET_BANK_GOLD(ch) += GET_AUTOQUEST_GOLD(ch);
      gain_gold(ch, GET_AUTOQUEST_GOLD(ch), GOLD_BANK);
      gain_exp(ch, GET_AUTOQUEST_EXP(ch));

      GET_AUTOQUEST_VNUM(ch) = 0;
			free(GET_AUTOQUEST_DESC(ch));
      GET_AUTOQUEST_DESC(ch) = strdup("nothing");
      GET_AUTOQUEST_KILLNUM(ch) = 0;
      GET_AUTOQUEST_QP(ch) = 0;
      GET_AUTOQUEST_EXP(ch) = 0;
      GET_AUTOQUEST_GOLD(ch) = 0;
    }
    else {
      send_to_char(ch, "You have not yet completed your bounty contract against %s.\r\n"
                       "You still need to kill %d more.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation points.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d experience points.\r\n",
                       GET_AUTOQUEST_DESC(ch), GET_AUTOQUEST_KILLNUM(ch), GET_AUTOQUEST_QP(ch), GET_AUTOQUEST_GOLD(ch), GET_AUTOQUEST_EXP(ch));
    }
  }  // end bounty complete

  else if (!strcmp(arg, "quit")) {

    send_to_char(ch, "You abandon your bounty contract to kill %d %s.\r\n", GET_AUTOQUEST_KILLNUM(ch), GET_AUTOQUEST_DESC(ch));

    GET_AUTOQUEST_VNUM(ch) = 0;
		free(GET_AUTOQUEST_DESC(ch));
    GET_AUTOQUEST_DESC(ch) = strdup("nothing");
    GET_AUTOQUEST_KILLNUM(ch) = 0;
    GET_AUTOQUEST_QP(ch) = 0;
    GET_AUTOQUEST_EXP(ch) = 0;
    GET_AUTOQUEST_GOLD(ch) = 0;
  } // end bount quit

  else {
    send_to_char(ch, "Type 'bounty new' for a new bounty, 'bounty complete' to finish your bounty and receive your reward or 'bounty quit' to quit your bounty.\r\n");
  }

  return 1;
};

SPECIAL(harvest)
{

  if (!CMD_IS("harvest")) {
    return 0;
  }

  struct obj_data *node = NULL;
  struct obj_data *tobj = NULL;
  struct obj_data *next_obj = NULL;
  int material = 0;
  int obj_vnum = 0;
  int skillnum = 0;
  struct obj_data *obj = NULL;
  int scmd = 0;
  int race = 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What would you like to harvest?\r\n");
    return 1;
  }

  if (!(node = get_obj_in_list_vis(ch, argument, NULL, world[IN_ROOM(ch)].contents))) {
    send_to_char(ch, "There's nothing here by that description.\r\n");
    return 1;
  }

   if (IS_CORPSE(node)) {
    race = GET_OBJ_VAL(node, VAL_CONTAINER_CORPSE_RACE);
    scmd = SCMD_HUNT;
    if (race_list[race].family != RACE_TYPE_ANIMAL && race_list[race].family != RACE_TYPE_MAGICAL_BEAST &&
        race_list[race].family != RACE_TYPE_DRAGON) {
      if (IS_HUMANOID_RACE(race)) {
        obj_vnum = 64026;
      }
      else {
        send_to_char(ch, "There's nothing harvestable from that.\r\n");
        return 1;
      }
    }
    else if (race_list[race].family != RACE_TYPE_DRAGON) {
      obj_vnum = 64014;
    }
    else {
      obj_vnum = 64025;
    }
  }
  else if (GET_OBJ_VNUM(node) != 64099) {
    send_to_char(ch, "That item cannot be harvested.\r\n");
    return 1;
  }

  material = GET_OBJ_MATERIAL(node);

  if (!obj_vnum) {
    switch (material) {

      case MATERIAL_STEEL:
        obj_vnum = 64000;
        scmd = SCMD_MINE;
        break;
      case MATERIAL_COLD_IRON:
        obj_vnum = 64002;
        scmd = SCMD_MINE;
        break;
      case MATERIAL_MITHRIL:
        obj_vnum = 64007;
        scmd = SCMD_MINE;
        break;
      case MATERIAL_ADAMANTINE:
        obj_vnum = 64010;
        scmd = SCMD_MINE;
        break;

      case MATERIAL_SILVER:
        obj_vnum = 64004;
        scmd = SCMD_MINE;
        break;
      case MATERIAL_GOLD:
        obj_vnum = 64005;
        scmd = SCMD_MINE;
        break;

      case MATERIAL_LEATHER:
        obj_vnum = 64014;
        scmd = SCMD_HUNT;
        break;
      case MATERIAL_DRAGONHIDE:
        obj_vnum = 64025;
        scmd = SCMD_HUNT;
        break;

      case MATERIAL_WOOD:
        obj_vnum = 64015;
        scmd = SCMD_FOREST;
        break;
      case MATERIAL_DARKWOOD:
        obj_vnum = 64016;
        scmd = SCMD_FOREST;
        break;

      case MATERIAL_HEMP:
        obj_vnum = 64020;
        scmd = SCMD_FARM;
        break;
      case MATERIAL_COTTON:
        obj_vnum = 64017;
        scmd = SCMD_FARM;
        break;
      case MATERIAL_WOOL:
        obj_vnum = 64024;
        scmd = SCMD_FARM;
        break;
      case MATERIAL_VELVET:
        obj_vnum = 64021;
        scmd = SCMD_FARM;
        break;
      case MATERIAL_SATIN:
        obj_vnum = 64022;
        scmd = SCMD_FARM;
        break;
      case MATERIAL_SILK:
        obj_vnum = 64023;
        scmd = SCMD_FARM;
        break;
    }
  }

  if (!obj_vnum) {
    send_to_char(ch, "You cannot harvest that item.  Please notify a staff member and give them code 1A and vnum %d.\r\n", GET_OBJ_VNUM(node));
    return 1;
  }

  obj = read_object(obj_vnum, VIRTUAL);

  if (!obj) {
    send_to_char(ch, "You cannot harvest that item.  Please notify a staff member and give them code 1B and vnum %d.\r\n", GET_OBJ_VNUM(node));
    return 1;
  }

  switch (scmd) {
    case SCMD_MINE:
      skillnum = SKILL_MINING;
      break;
    case SCMD_FOREST:
      skillnum = SKILL_FORESTING;
      break;
    case SCMD_HUNT:
      skillnum = SKILL_HUNTING;
      break;
    case SCMD_FARM:
      skillnum = SKILL_FARMING;
      break;
  }

  if (get_skill_value(ch, skillnum) < GET_OBJ_LEVEL(obj)) {
    send_to_char(ch, "You are not skilled enough in %s to harvest %s.\r\n", spell_info[skillnum].name, obj->short_description);
    return 1;
  }

  if ((IS_CORPSE(node) && dice(1, 5) == 5) || !IS_CORPSE(node)) {
    GET_CRAFTING_OBJ(ch) = obj;
    GET_CRAFTING_TYPE(ch) = scmd;
    GET_CRAFTING_TICKS(ch) = GET_ADMLEVEL(ch) ? 0 : 1;
    act("You begin to harvest $p.", true, ch, obj, 0, TO_CHAR);
    act("$n begins to harvest $p.", true, ch, obj, 0, TO_ROOM);
  }
  else {
    act("The corpse is too damaged to harvest.", true, ch, obj, 0, TO_CHAR);
  }

  if (!race)
    GET_OBJ_VAL(node, 0)--;

  if (GET_OBJ_VAL(node, 0) <= 0 || IS_CORPSE(node)) {
    if (!IS_CORPSE(node)) {
      switch (scmd) {
        case SCMD_MINE:
          mining_nodes--;
          break;
        case SCMD_FOREST:
          foresting_nodes--;
          break;
        case SCMD_HUNT:
          hunting_nodes--;
          break;
        case SCMD_FARM:
          farming_nodes--;
          break;
      }
    }
    else {
      for (tobj = node->contains; tobj; tobj = next_obj) {
        next_obj = tobj->next_content;
        obj_from_obj(tobj);
        obj_to_room(tobj, IN_ROOM(ch));
      }
    }
    obj_from_room(node);
    extract_obj(node);
  }

  return 1;
}

SPECIAL(start_room)
{

  if (!CMD_IS("north"))
    return 0;

  if (GET_CLASS_LEVEL(ch) > 0) {
      char_to_room(ch, real_room(29519));
      look_at_room(IN_ROOM(ch), ch, 0);
      return 1;
  }

  return 0;

}

SPECIAL(ring_of_wishes)
{

  if (!CMD_IS("wish"))
    return 0;

  if (GET_OBJ_VAL((struct obj_data *)me, 0) == 0) {
    send_to_char(ch, "Seems like all of the wishes have already been used up. :(\r\n");
    return 1;
  }

  call_magic(ch, ch, 0, SPELL_WISH_RING, 20, CAST_SPELL, *argument ? argument : NULL);

  GET_OBJ_VAL((struct obj_data *)me, 0)--;

  return 1;

}

SPECIAL(mount_shop)
{

  if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("barding") && !CMD_IS("companion"))
    return 0;

  int i = 0, j = 0;
  int avg_dam = 0;
  int num_hit = 0;
  float dam = 0.0;

  if (CMD_IS("list")) {
    send_to_char(ch, "The following mounts are for sale:\r\n\r\n");
    send_to_char(ch, "#   %-25.25s %-3s %-3s %-6s %-2s %-3s %-3s %-5s %-s7\r\n"
                 "--- ------------------------- --- --- ------ -- --- --- ----- -------\r\n",
                 "Name", "Lvl", "HP", "DamScr", "AC", "Spd", "Fly", "Skill", "Cost");

    for (i = 1; i < NUM_PETS; i++) {
      if (pet_list[i].cost > 0) {
        num_hit = 0;
        avg_dam = 0;
        dam = 0;
        for (j = 0; j < 5; j++) {
          if (pet_list[i].attacks_ndice[j] > 0) {
            num_hit++;
            avg_dam = 100 * ((pet_list[i].attacks_ndice[j] * pet_list[i].attacks_sdice[j]) + pet_list[i].attacks_dammod[j]);
            avg_dam *= (5 + GET_LEVEL(ch) + pet_list[i].attacks_to_hit[j]) * 5;
            avg_dam /= 100;
            dam += avg_dam;
          }
        }
        dam /= 100;
        send_to_char(ch, "%2d) %-25.25s %-3d %-3d %-6.2f %-2d %-3d %-3s %-5d %-7d\r\n",
                     i, pet_list[i].desc, pet_list[i].level, pet_list[i].max_hit,
                     dam, pet_list[i].ac, pet_list[i].speed, pet_list[i].flying ? "Yes" : "No",
                     pet_list[i].skill, pet_list[i].cost);
      }
    }
    send_to_char(ch, "\r\nType buy <#> to purchase a mount.\r\n");
//    send_to_char(ch, "\r\nType companion <#> to get an animal companion.\r\n(You must have more druid and ranger(-4) ranks than the companion level to take). \r\n");
  }
  else if (CMD_IS("buy") || CMD_IS("companion")) {
    char arg[200]={'\0'};
    one_argument(argument, arg);

    int mob_num = 0;

    if (!*arg) {
      send_to_char(ch, "You need to specify which mount/companion you wish to buy/get.  Type list to see what is available.\r\n");
      return 1;
    }

    mob_num = atoi(arg);

    if (mob_num <= 0 || mob_num >= NUM_PETS || pet_list[mob_num].cost == 0) {
      send_to_char(ch, "That mount is not available for sale.\r\n");
      return 1;
    }

    if (CMD_IS("buy")) {
      if (GET_GOLD(ch) < pet_list[mob_num].cost) {
        send_to_char(ch, "You do not have enough gold to purchase that mount.\r\n");
        return 1;
      }

      if (get_skill_value(ch, SKILL_HANDLE_ANIMAL) < pet_list[mob_num].skill) {
        send_to_char(ch, "You do not have a high enough handle animal skill to get that mount.\r\n");
        return 1;
      }

      if (get_skill_value(ch, SKILL_RIDE) < pet_list[mob_num].skill) {
        send_to_char(ch, "You do not have a high enough ride skill to get that mount.\r\n");
        return 1;
      }

      if (GET_CLASS_LEVEL(ch) < pet_list[mob_num].skill) {
        send_to_char(ch, "You are not high enough level to get that mount.\r\n");
        return 1;
      }
      GET_GOLD(ch) -= pet_list[mob_num].cost;
      ch->player_specials->mount = mob_num;
      save_char(ch);
      send_to_char(ch, "You just bought yourself %s for %d %s!  You may call your mount using the callmount command.\r\n",
                   pet_list[mob_num].desc, pet_list[mob_num].cost, MONEY_STRING);
    } else {
      if (((MAX(0, GET_CLASS_RANKS(ch, CLASS_RANGER) - 4)) + GET_CLASS_RANKS(ch, CLASS_DRUID)) < pet_list[mob_num].level) {
        send_to_char(ch, "Your total druid and ranger(-4) ranks must be equal to or greater than the level of the mob picked.\r\n");
        return 1;
      }
      ch->player_specials->companion_num = mob_num;
      save_char(ch);
      send_to_char(ch, "You just selected for yourself %s as an animal companion!\r\nYou may call your animal companion using the callcompanion command.\r\n",
                   pet_list[mob_num].desc);
    }
  } else {
    struct obj_data * obj;
    char arg[200]={'\0'};
    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "Which item in your inventory do you want to make into barding?\r\n");
      return 1;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't seem to have an item by that description.\r\n");
      return 1;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_ARMOR && !CAN_WEAR(obj, ITEM_WEAR_BODY)) {
      send_to_char(ch, "Only body armor can be converted into barding.\r\n");
      return 1;
    }

    int cost = 100 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1) + armor_list[GET_OBJ_VAL(obj, 0)].cost;
    cost /= 10;

    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "You only have %d %s on hand, but you need %d to convert that armor to barding.\r\n", GET_GOLD(ch), MONEY_STRING, cost);
      return 1;
    }

    GET_GOLD(ch) -= cost;

    REMOVE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY);
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BARDING);

    send_to_char(ch, "You have converted %s into barding for your mount for %d %s.\r\n", obj->short_description, cost, MONEY_STRING);
  }
  return 1;
}
char highscorename[200]={'\0'};
int highscore = 0;

#define SKILL_DARTS SKILL_ACROBATICS

SPECIAL(dartboard)
{
  struct char_data *to;
  int dart1 = 0, dart2 = 0, dart3 = 0, score = 0;
  char buf[400]={'\0'};
  if (!*highscorename) strcpy(highscorename, "Nobody");

  if (CMD_IS("playdarts")) {
      dart1 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_value(ch, SKILL_DARTS);
      dart2 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_value(ch, SKILL_DARTS);
      dart3 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_value(ch, SKILL_DARTS);
      if (affected_by_spell(ch, SPELL_BLESS)) {
        dart1 += 5;
        dart2 += 5;
        dart3 += 5;
      }
      score = dart1 + dart2 + dart3;
      if ((score > highscore) && (GET_ADMLEVEL(ch) == 0)) {
        highscore = score;
        strcpy(highscorename, GET_NAME(ch));
      }
      sprintf(buf, "You throw a dart and score %d!\r\n", dart1);
      send_to_char(ch, "%s", buf);
      to = world[IN_ROOM(ch)].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf, "%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart1);
          send_to_char(to, "%s", buf);
        }
      }

      sprintf(buf, "You throw a dart and score %d!\r\n", dart2);
      send_to_char(ch, "%s", buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart2);
          send_to_char(to, "%s", buf);
        }
      }

      sprintf(buf, "You throw a dart and score %d!\r\n", dart3);
      send_to_char(ch, "%s", buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart3);
          send_to_char(to, "%s", buf);
        }
      }

      sprintf(buf, "Your total score for this game is %d.\r\n\r\n", score);
      send_to_char(ch, "%s", buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"The total score for %s this game is %d!\r\n", PERS(ch, to), score);
          send_to_char(to, "%s", buf);
        }
      }

      sprintf(buf, "The highest score is %d held by %s.\r\n\r\n", highscore, highscorename);
      send_to_char(ch, "%s", buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"The highest score is held by %s at %d.\r\n", highscorename, highscore);
          send_to_char(to, "%s", buf);
        }
      }


      if (dice(1, 100) <= 1) {
        GET_SKILL_BASE(ch, SKILL_DARTS)++;
        send_to_char(ch, "Your skill at the game of darts has improved to %d!\r\n", GET_SKILL_BASE(ch, SKILL_DARTS));
      }
      return (1);
  }
  else
    return (0);
}

SPECIAL(crafting_quest) {

  if (!CMD_IS("supplyorder")) {
    return 0;
  }

  char arg[200]={'\0'};
  char arg2[200]={'\0'};

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Type 'supplyorder new' for a new supply order, 'supplyorder complete' to finish your supply order and\r\n"
                     "receive your reward or 'supplyorder quit' to quit your supply order.\r\n");
    if (GET_AUTOCQUEST_VNUM(ch) > 0) {
      send_to_char(ch, "You have not yet completed your supply order for %s.\r\n"
                       "You still need to make %d more.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation points.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d artisan experience points.\r\n",
                       GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));
    }
    return 1;
  }

  if (!strcmp(arg, "new")) {

    if (GET_AUTOCQUEST_VNUM(ch) > 0 && GET_AUTOCQUEST_MAKENUM(ch) < 1) {
      send_to_char(ch, "You can't take a new supply order until you've handed in the one you've completed.\r\n");
      return 1;
    }

  GET_AUTOCQUEST_VNUM(ch) = 30084;
  GET_AUTOCQUEST_MAKENUM(ch) = 0;
  GET_AUTOCQUEST_QP(ch) = 0;
  GET_AUTOCQUEST_EXP(ch) = 0;
  GET_AUTOCQUEST_GOLD(ch) = 0;
  GET_AUTOCQUEST_DESC(ch) = strdup("nothing");

  int atype = GET_ARTISAN_TYPE(ch);
  int itype = -1;
  char desc[500]={'\0'};
  int roll = 0;

  switch (atype) {
    case ARTISAN_TYPE_ARMORTECH:
        itype = dice(1, NUM_SPEC_ARMOR_TYPES)-1;
        sprintf(desc, "%s", armor_list[itype].name);
        GET_AUTOCQUEST_MATERIAL(ch) = armor_list[itype].material;
      break;
    case ARTISAN_TYPE_WEAPONTECH:
        itype = dice(1, NUM_WEAPON_TYPES)-1;
        sprintf(desc, "%s", weapon_list[itype].name);
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      break;
    case ARTISAN_TYPE_TINKERING:
      if ((roll = dice(1, 7)) == 1) {
        sprintf(desc, "a necklace");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_COPPER;
      } else if (roll == 2) {
        sprintf(desc, "a bracer");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      } else if (roll == 3) {
        sprintf(desc, "a stim injection");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      } else if (roll == 4) {
        sprintf(desc, "a cape");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_HEMP;
      } else if (roll == 5) {
        sprintf(desc, "a belt");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_LEATHER;
      } else if (roll == 6) {
        sprintf(desc, "a pair of gloves");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_HEMP;
      } else {
        sprintf(desc, "a pair of boots");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_LEATHER;
      }
      break;
    case ARTISAN_TYPE_ROBOTICS:
      if ((roll = dice(1, 2)) == 1) {
        sprintf(desc, "a hover droid");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      } else {
        sprintf(desc, "an implant");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      }
      break;
  }


  GET_AUTOCQUEST_DESC(ch) = strdup(desc);
  GET_AUTOCQUEST_MAKENUM(ch) = 5;
  GET_AUTOCQUEST_QP(ch) = MAX(1, GET_ARTISAN_LEVEL(ch));
  GET_AUTOCQUEST_EXP(ch) = (art_level_exp(GET_ARTISAN_LEVEL(ch) + 1) - art_level_exp(GET_ARTISAN_LEVEL(ch))) / (5 + (GET_ARTISAN_LEVEL(ch) / 4)) * 5;
  GET_AUTOCQUEST_GOLD(ch) = (100 + (GET_ARTISAN_LEVEL(ch) * 10 * MAX(1, GET_ARTISAN_LEVEL(ch) - 1) * 2));

  GET_AUTOCQUEST_QP(ch) *= (GET_GUILD(ch) != GUILD_ARTISANS) ? 100 : (100 + ((GET_GUILD_RANK(ch) + 1) / 4 * 5));
  GET_AUTOCQUEST_QP(ch) /= 100;
  GET_AUTOCQUEST_EXP(ch) *= (GET_GUILD(ch) != GUILD_ARTISANS) ? 100 : (100 + ((GET_GUILD_RANK(ch) + 1) / 4 * 5));
  GET_AUTOCQUEST_EXP(ch) /= 100;
  GET_AUTOCQUEST_GOLD(ch) *= (GET_GUILD(ch) != GUILD_ARTISANS) ? 100 : (100 + ((GET_GUILD_RANK(ch) + 1) / 4 * 5));
  GET_AUTOCQUEST_GOLD(ch) /= 100;

  send_to_char(ch, "You have been commissioned for a supply order to make %s.  We expect you to make %d before you can collect your reward.  Good luck.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation points.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d artisan experience points.\r\n",

                   desc, GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));

  } // end bounty new

  else if (!strcmp(arg, "complete")) {

    if (GET_AUTOCQUEST_VNUM(ch) > 0 && GET_AUTOCQUEST_MAKENUM(ch) < 1) {

      send_to_char(ch, "You have completed your supply order contract for %s.\r\n"
                       "You receive %d reputation points.\r\n"
                       "%d credits have been deposited into your bank account.\r\n"
                       "You receive %d experience points.\r\n",
                       GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));

      GET_QUESTPOINTS(ch) += GET_AUTOCQUEST_QP(ch);
      GET_REPUTATION(ch) += GET_AUTOQUEST_QP(ch);
      gain_gold(ch, GET_AUTOCQUEST_GOLD(ch), GOLD_BANK);
      gain_artisan_exp(ch, GET_AUTOCQUEST_EXP(ch));

      GET_AUTOCQUEST_VNUM(ch) = 0;
      free(GET_AUTOCQUEST_DESC(ch));
      GET_AUTOCQUEST_DESC(ch) = strdup("nothing");
      GET_AUTOCQUEST_MAKENUM(ch) = 0;
      GET_AUTOCQUEST_QP(ch) = 0;
      GET_AUTOCQUEST_EXP(ch) = 0;
      GET_AUTOCQUEST_GOLD(ch) = 0;
    }
    else {
      send_to_char(ch, "You have not yet completed your supply order for %s.\r\n"
                       "You still need to make %d more.\r\n"
                       "Once completed you will receive the following:\r\n"
                       "You will receive %d reputation points.\r\n"
                       "%d credits will be deposited into your bank account.\r\n"
                       "You will receive %d experience points.\r\n",
                       GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));
    }
  }  // end supplyorder complete

  else if (!strcmp(arg, "quit")) {

    send_to_char(ch, "You abandon your supply order to make %d %s.\r\n", GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_DESC(ch));

    GET_AUTOCQUEST_VNUM(ch) = 0;
    free(GET_AUTOCQUEST_DESC(ch));
    GET_AUTOCQUEST_DESC(ch) = strdup("nothing");
    GET_AUTOCQUEST_MAKENUM(ch) = 0;
    GET_AUTOCQUEST_QP(ch) = 0;
    GET_AUTOCQUEST_EXP(ch) = 0;
    GET_AUTOCQUEST_GOLD(ch) = 0;
  } // end supplyorder quit

  else {
    send_to_char(ch, "Type 'supplyorder new' for a new supply order, 'supplyorder complete' to finish your supply order and\r\n"
                     "receive your reward or 'supplyorder quit' to quit your supply order.\r\n");
  }

  return 1;
};

SPECIAL(item_seller)
{

  if (!CMD_IS("item"))
    return 0;

  char arg[200]={'\0'}, arg2[200]={'\0'};

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item list <weapons|armor|other>'.\r\n");
    return 1;
  }

  if (is_abbrev(arg, "buy")) {
    if (!*arg2) {
      send_to_char(ch, "Please specify the vnum of the item you wish to buy.  You may obtain the vnum from the 'item list' command.\r\n");
      return 1;
    }

    int vnum = atoi(arg2);

    if (vnum < 30000 || vnum > 300099) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return 1;
    }

    struct obj_data *obj = read_object(vnum, VIRTUAL);

    if (!obj) {
      send_to_char(ch, "There was an error buying your item.  Please inform a staff member with error code ITM_BUY_001.\r\n");
      return 1;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WORN) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return 1;
    }

    int cost = GET_OBJ_COST(obj);

    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "That item costs %d %s and you only have %d on hand.\r\n", cost, MONEY_STRING, GET_GOLD(ch));
      return 1;
    }

    GET_GOLD(ch) -= cost;

    obj_to_char(obj, ch);

    send_to_char(ch, "You purchase %s for %d %s.\r\n", obj->short_description, cost, MONEY_STRING);
    return 1;
  }
  else if (is_abbrev(arg, "list")) {

    if (!*arg2) {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      return 1;
    }

    int type = ITEM_OTHER;

    if (is_abbrev(arg2, "armor"))
      type = ITEM_ARMOR;
    else if (is_abbrev(arg2, "weapon"))
      type = ITEM_WEAPON;
    else if (is_abbrev(arg2, "other"))
      type = ITEM_WORN;
    else {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      return 1;
    }

    struct obj_data *obj = NULL;
    int i = 0;

    send_to_char(ch, "%-5s %-6s %-25s\r\n----- ------ -------------------------\r\n", "VNUM", "COST", "ITEM");

    for (i = 30000; i < 30100; i++) {
      if (obj)
        extract_obj(obj);
      obj = read_object(i, VIRTUAL);
      if (!obj)
        continue;
      if (GET_OBJ_TYPE(obj) != type)
        continue;
      send_to_char(ch, "%-5d %-6d %-25s\r\n", i, GET_OBJ_COST(obj), obj->short_description);
    }
    send_to_char(ch, "\r\n");
    return 1;
  }
  else {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item list <weapons|armor|other>'.\r\n");
    return 1;
  }

  return 1;

}

SPECIAL(mold_seller)
{

  if (!CMD_IS("item"))
    return 0;

  char arg[200]={'\0'}, arg2[200]={'\0'};

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item list <weapons|armor|other>'.\r\n");
    return 1;
  }

  if (is_abbrev(arg, "buy"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify the vnum of the item you wish to buy.  You may obtain the vnum from the 'item list' command.\r\n");
      return 1;
    }

    int vnum = atoi(arg2);

    if (vnum < 30000 || vnum > 300099)
    {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return 1;
    }

    struct obj_data *obj = read_object(vnum, VIRTUAL);

    if (!obj)
    {
      send_to_char(ch, "There was an error buying your item.  Please inform a staff member with error code ITM_BUY_001.\r\n");
      return 1;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WORN) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return 1;
    }

    int cost = GET_OBJ_COST(obj);

    if (GET_GOLD(ch) < cost)
    {
      send_to_char(ch, "That item costs %d %s and you only have %d on hand.\r\n", cost, MONEY_STRING, GET_GOLD(ch));
      return 1;
    }

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);

    GET_GOLD(ch) -= cost;

    obj_to_char(obj, ch);

    send_to_char(ch, "You purchase %s for %d %s.\r\n", obj->short_description, cost, MONEY_STRING);
    return 1;
  }
  else if (is_abbrev(arg, "list"))
  {

    if (!*arg2)
    {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      return 1;
    }

    int type = ITEM_OTHER;

    if (is_abbrev(arg2, "armor"))
      type = ITEM_ARMOR;
    else if (is_abbrev(arg2, "weapon"))
      type = ITEM_WEAPON;
    else if (is_abbrev(arg2, "other"))
      type = ITEM_WORN;
    else {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      return 1;
    }

    struct obj_data *obj = NULL;
    int i = 0;

    send_to_char(ch, "%-5s %-6s %-25s\r\n----- ------ -------------------------\r\n", "VNUM", "COST", "ITEM");

    for (i = 30000; i < 30100; i++)
    {
      if (obj)
        extract_obj(obj);
      obj = read_object(i, VIRTUAL);
      if (!obj)
        continue;
      if (GET_OBJ_TYPE(obj) != type)
        continue;
      send_to_char(ch, "%-5d %-6d %-25s\r\n", i, GET_OBJ_COST(obj), obj->short_description);
    }
    send_to_char(ch, "\r\n");
    return 1;
  }
  else
  {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item list <weapons|armor|other>'.\r\n");
    return 1;
  }

  return 1;

}

SPECIAL(player_shop)
{

  if (!CMD_IS("shop")) {
    return 0;
  }

  char arg[200]={'\0'}, arg2[200]={'\0'}, arg3[200]={'\0'};
  struct obj_data *obj = NULL;
  int cost = 0;

  one_argument(one_argument(one_argument(argument, arg), arg2), arg3);

  if (!*arg) {
    send_to_char(ch, "What would you like to do?  (buy | sell | try)\r\n");
    return 1;
  }

  if (is_abbrev(arg, "buy")) {
    if (!*arg2) {
      send_to_char(ch, "What would you like to buy? (separate keywords with commas)\r\n");
      return 1;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "That item doesn't seem to be here.\r\n");
      return 1;
    }
    if (GET_IDNUM(ch) == GET_OBJ_VAL(obj, 12)) {
      send_to_char(ch, "You can't buy your own item, just @Yget@n it if you don't want to sell it.\r\n");
      return 1;
    }

    cost = GET_OBJ_VAL(obj, 14);

    if (cost <= 0) {
      send_to_char(ch, "That item is not for sale.\r\n");
      return 1;
    }

    if (cost > GET_GOLD(ch)) {
      send_to_char(ch, "You do not have enough gold to buy that.  It costs %d and you have %d on hand.\r\n", cost, GET_GOLD(ch));
      return 1;
    }

    GET_OBJ_VAL(obj, 14) = 0;
    GET_GOLD(ch) -= cost;
    obj_from_room(obj);
    obj_to_char(obj, ch);

    send_to_char(ch, "You have purchased %s for %d %s.\r\n", obj->short_description, cost, MONEY_STRING);

    struct obj_data *money;

    money = read_object(64096, VIRTUAL);

    GET_OBJ_VAL(money, 0) = cost;
    GET_OBJ_VAL(money, 12) = GET_OBJ_VAL(obj, 12);

    SET_BIT_AR(GET_OBJ_EXTRA(money), ITEM_UNIQUE_SAVE);
    SET_BIT_AR(GET_OBJ_EXTRA(money), ITEM_IDENTIFIED);

    obj_to_room(money, IN_ROOM(ch));

    return 1;
  }
  else if (is_abbrev(arg, "sell")) {
    if (!*arg2) {
      send_to_char(ch, "What would you like to sell? (separate keywords with commas)\r\n");
      return 1;
    }
    if (!*arg3) {
      send_to_char(ch, "How much would you like to sell it for?\r\n");
      return 1;
    }

    cost = atoi(arg3);

    if (cost < 0) {
      send_to_char(ch, "You must sell it for at least 1 credit or set the price at zero to remove it from sale.\r\n");
      return 1;
    }

    int obj_dotmode = find_all_dots(arg2);
    sbyte found = 0;
    struct obj_data *next_obj = NULL;

    if (obj_dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return 1;
      }
      else {
        GET_OBJ_VAL(obj, 14) = cost;
        GET_OBJ_VAL(obj, 12) = GET_IDNUM(ch);
        found = 1;
      }
    } else {
      for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (CAN_SEE_OBJ(ch, obj) && (obj_dotmode == FIND_ALL || isname(arg2, obj->name))) {
          found = 1;
          GET_OBJ_VAL(obj, 14) = cost;
          GET_OBJ_VAL(obj, 12) = GET_IDNUM(ch);
        }
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
        send_to_char(ch, "You don't seem to have any %ss.\r\n", arg2);
      else
        send_to_char(ch, "You don't seem to have any %ss.\r\n", arg2);
      return 1;
    }
    send_to_char(ch, "You set %s to a cost of %d.\r\n", arg2, cost);
  }
  else if (is_abbrev(arg, "try")) {
    if (!*arg2) {
      send_to_char(ch, "What would you like to try out?  (separate keywords with commas)\r\n");
      return 1;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "That item doesn't seem to be here.\r\n");
      return 1;
    }
    spell_identify(100, ch, ch, obj, NULL);
  }
  else {
    send_to_char(ch, "What would you like to do?  (buy | sell | try)\r\n");
    return 1;
  }

  return 1;
}


struct spec_list spec_names[] = {
/* BEGIN HERE */
{"No Spec Proc", no_spec},
{"Postmaster Mob", postmaster},
{"Receptionist Mob", receptionist},
{"Cryogenicist Mob", cryogenicist},
{"Guild Mob", guild},
{"Identifier Mob", identify_mob},
{"Questmaster", questmaster},
{"Bounty Contractor", bounty_contractor},
{"Crafting Quests", crafting_quest},
{"Item Seller", item_seller},


/* STOP HERE */
{"Unknown; exists", NULL} /* Terminator */
};

/* Get the name of a special proc. */
char *get_spec_name(SPECIAL(func))
{
    int i=0;

    if (!func)
    {
        return "None";
    }

    for (; spec_names[i].func && spec_names[i].func!=func; i++);

    return spec_names[i].name;
}

/* Get a pointer to the function when you have a name */
proctype get_spec_proc(char *name)
{
    int i=0;

    for (; spec_names[i].func && str_cmp(name, spec_names[i].name); i++);

    return spec_names[i].func;
}

/* Show all specprocs */
/* Don't ask me; I haven't got the foggiest idea why I put this in.
Debugging maybe... :] */
void list_spec_procs(struct char_data *ch)
{
    int i=0;

    for(; spec_names[i].func; i++)
    {
        send_to_char(ch, "%s", spec_names[i].name);
        if (i%4==3)
        {
          send_to_char(ch, "\r\n");
      }
      else
      {
          send_to_char(ch, "\t\t");
      }
  }
  send_to_char(ch, "\r\n");
}


SPECIAL(emporium)
{

  if (!CMD_IS("emporium"))
    return 0;

  char arg[200]={'\0'}, arg2[200]={'\0'}, arg3[200]={'\0'}, arg4[200]={'\0'};
  int bt = 0, bonus = 0;

  one_argument(one_argument(one_argument(one_argument(argument, arg), arg2), arg3), arg4);

  if (!*arg) {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum> <bonustype> <bonus>' or 'item list <weapons|armor|other> <bonustype> <bonus>'.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
    return 1;
  }

  if (is_abbrev(arg, "buy")) {
    if (!*arg2) {
      send_to_char(ch, "Please specify the vnum of the item you wish to buy.  You may obtain the vnum from the 'item list' command.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    int vnum = atoi(arg2);

    if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) || vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return 1;
    }

    struct obj_data *obj = read_object(vnum, VIRTUAL);

    if (!obj) {
      send_to_char(ch, "There was an error buying your item.  Please inform a staff member with error code ITM_BUY_001.\r\n");
      return 1;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WORN) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (!*arg3) {
      send_to_char(ch, "%s", list_bonus_types());
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, "%s", list_bonus_types());
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (bt == APPLY_ACCURACY && GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
      send_to_char(ch, "Only weapons can be given that bonus.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (bt == APPLY_AC_ARMOR && !CAN_WEAR(obj, ITEM_WEAR_BODY)) {
      send_to_char(ch, "Only body armor can be given that bonus.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (bt == APPLY_AC_SHIELD && !CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
      send_to_char(ch, "Only shields can be given that bonus.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if ((bonus = atoi(arg4)) > 100) {
      send_to_char(ch, "The bonus must be less than 100.\r\n");
      send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    obj->affected[0].location = bt;
    obj->affected[0].modifier = atoi(arg4);

    if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION || bt == APPLY_AC_NATURAL || bt == APPLY_AC_ARMOR))
      obj->affected[0].modifier *= 10;

    if (bt == APPLY_ACCURACY) {
      obj->affected[1].location = APPLY_DAMAGE;
      obj->affected[1].modifier = atoi(arg4);
    }


    GET_OBJ_LEVEL(obj) = set_object_level(obj);
    GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

    int cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);


    spell_identify(20, ch, ch, obj, NULL);

    if (GET_OBJ_LEVEL(obj) > GET_CLASS_LEVEL(ch)) {
      send_to_char(ch, "You cannot buy an item whose level is greater than yours.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (GET_QUESTPOINTS(ch) < cost) {
      send_to_char(ch, "That item costs %d reputation points and you only have %d.\r\n", cost, GET_QUESTPOINTS(ch));
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    GET_QUESTPOINTS(ch) -= cost;

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
    char buf[200]={'\0'};
    sprintf(buf, "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
    obj->short_description = strdup(buf);
    obj->name = strdup(buf);
    sprintf(buf, "%s +%d %s lies here.", CAP(obj->short_description), bonus, get_bonus_type(arg3));
    obj->description = strdup(buf);

    obj_to_char(obj, ch);

    send_to_char(ch, "You purchase %s for %d reputation points.\r\n", obj->short_description, cost);
    return 1;
  }
  else if (is_abbrev(arg, "list")) {

    if (!*arg2) {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    int type = ITEM_OTHER;

    if (is_abbrev(arg2, "armor"))
      type = ITEM_ARMOR;
    else if (is_abbrev(arg2, "weapon"))
      type = ITEM_WEAPON;
    else if (is_abbrev(arg2, "other"))
      type = ITEM_WORN;
    else {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (!*arg3) {
      send_to_char(ch, "%s", list_bonus_types());
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, "%s", list_bonus_types());
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
      return 1;
    }

    char buf[100]={'\0'};
    int cost = 0;

    struct obj_data *obj = NULL;
    int i = 0;
    int vnum = 0;

    send_to_char(ch, "%-5s %-6s %-7s %-35s\r\n----- ------ -------------------------\r\n", "VNUM", "COST", "MIN-LVL", "ITEM");

    for (i = 30000; i < 30299; i++) {
        vnum = i;
      if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) || vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
        continue;
      }
      if (obj)
        extract_obj(obj);
      obj = read_object(i, VIRTUAL);
      if (!obj)
        continue;
      if (GET_OBJ_TYPE(obj) != type)
        continue;
      obj->affected[0].location = bt;
      obj->affected[0].modifier = atoi(arg4);

    if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION || bt == APPLY_AC_NATURAL || bt == APPLY_AC_ARMOR))
      {
        obj->affected[0].modifier *= 10;
      }

      if (bt == APPLY_ACCURACY) {
        obj->affected[1].location = APPLY_DAMAGE;
        obj->affected[1].modifier = atoi(arg4);
      }

      GET_OBJ_LEVEL(obj) = set_object_level(obj);
      GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

      cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);

      sprintf(buf, "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
      send_to_char(ch, "%-5d %-6d %d %-35s\r\n", i, cost, GET_OBJ_LEVEL(obj), buf);
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
    return 1;
  }
  else {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item list <weapons|armor|other>'.\r\n");
    send_to_char(ch, "@YPlease note that bonuses of the same type @RDO NOT@Y stack in d20 rules.\r\n");
    return 1;
  }

  return 1;

}

SPECIAL(bacta_merchant) {
/*
  if (!CMD_IS("buy"))
    return 0;


  if (CMD_IS("buy")) {

    char arg[200];

    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "How much bacta do you want to buy in units?\r\n");
      return 1;
    }

    int amt = atoi(arg);

    if (amt <= 0) {
      send_to_char(ch, "How much bacta do you want to buy in units?\r\n");
      return 1;
    }

    if ((IS_CARRYING_W(ch) + amt) > max_carry_weight(ch)) {
      send_to_char(ch, "You cannot carry that much bacta, it would be too heavy.!\r\n");
      return 1;
    }

    int cost = amt * 25;
    cost /= 10;

    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "You do not have enough credits on hand to buy that much bacta.  It would cost %d and you have %d.\r\n", cost, GET_GOLD(ch));
      return 1;
    }

    GET_BACTA(ch) += amt;
    GET_GOLD(ch) -= cost;
    IS_CARRYING_W(ch) += amt;
    update_encumberance(ch);
    send_to_char(ch, "You have bought %d units of bacta for %d credits.\r\n", amt, cost);
    return 1;
  }
*/
  return 0;
}

SPECIAL(lockbox)
{
  struct obj_data *lb = (struct obj_data *) me;
  struct obj_data *obj = NULL;
  char buffer[200]={'\0'};
  int roll = skill_roll(ch, SKILL_DISABLE_DEVICE);
  int level = MAX(1, roll - 14);

  if (!CMD_IS("slice") && !CMD_IS("open") && !CMD_IS("pick") && !CMD_IS("unlock"))
  {
    return 0;
  }

  skip_spaces(&argument);

  if (!*argument)
    return 0;

  if ((CMD_IS("open") || CMD_IS("pick") || CMD_IS("unlock")) && !is_abbrev(argument, "chest"))
  {
    return 0;
  }

  if (lb != (obj = get_obj_in_list_vis(ch, argument, NULL, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "That is not a treasure chest and cannot be opened.\r\n");
    return 1;
  }

  if (!lb)
    return 0;

  if (roll < 15)
  {
    sprintf(buffer, "As you try to open the treasure chest, the trap activates and an explosion is heard, with black smoke escaping the seams of the chest.");
    act(buffer, FALSE, ch, 0, 0, TO_CHAR);
    sprintf(buffer, "As $n tries to open the treasure chest, the trap activates and an explosion is heard, with black smoke escaping the seams of the chest.");
    act(buffer, FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(lb);
    extract_obj(lb);
    lb = obj = NULL;
    lockboxes--;
    return 1;
  }

  level = MIN(level, 30);
  award_lockbox_treasure(ch, level);
  obj_from_room(lb);
  extract_obj(lb);
  lb = obj = NULL;
  lockboxes--;
  return 1;
}

SPECIAL(orphan)
{

  struct obj_data *lb = (struct obj_data *) me;
  struct obj_data *obj = NULL;
  char arg[200]={'\0'}, arg2[200]={'\0'};
  char buf[200]={'\0'};
  int roll = skill_roll(ch, SKILL_DIPLOMACY);
  int reward = 0;

  if (!CMD_IS("send"))
  {
    return 0;
  }

  if (!lb)
    return 0;

  two_arguments(argument, arg, arg2);

  if (!*arg)
  {
    if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | dragonarmies)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | zhentarim)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_GOLARION)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | redmantis)\r\n");
    return 1;
  }
  if (!*arg2)
  {
    if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | dragonarmies)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | zhentarim)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_GOLARION)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | redmantis)\r\n");
    return 1;
  }

  if (lb != (obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "That is not an orphan and cannot be assisted.\r\n");
    return 1;
  }

  if (is_abbrev(arg2, "foster-home"))
  {
    sprintf(buf, "You send $p off to a loving foster home.");
    act(buf, FALSE, ch, lb, 0, TO_CHAR);
    sprintf(buf, "$n sends $p off to a loving foster home.");
    act(buf, FALSE, ch, lb, 0, TO_ROOM);
    obj_from_room(lb);
    extract_obj(lb);
    lb = obj = NULL;
    orphans--;
  }
  else if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE && is_abbrev(arg2, "dragonarmies"))
  {
    sprintf(buf, "You send $p off to the Dragonarmy Academy.");
    act(buf, FALSE, ch, lb, 0, TO_CHAR);
    sprintf(buf, "$n sends $p off to the Dragonarmy Academy.");
    act(buf, FALSE, ch, lb, 0, TO_ROOM);
    obj_from_room(lb);
    extract_obj(lb);
    lb = obj = NULL;
    orphans--;
  }
  else if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS && is_abbrev(arg2, "zhentarim"))
  {
    sprintf(buf, "You send $p off to the Zhentarim Academy.");
    act(buf, FALSE, ch, lb, 0, TO_CHAR);
    sprintf(buf, "$n sends $p off to the Zhentarim Academy.");
    act(buf, FALSE, ch, lb, 0, TO_ROOM);
    obj_from_room(lb);
    extract_obj(lb);
    lb = obj = NULL;
    orphans--;

  }
  else if (CONFIG_CAMPAIGN == CAMPAIGN_GOLARION && is_abbrev(arg2, "redmantis"))
  {
    sprintf(buf, "You send $p off to to join the Red Mantis.");
    act(buf, FALSE, ch, lb, 0, TO_CHAR);
    sprintf(buf, "$n sends $p off to join the Red Mantis.");
    act(buf, FALSE, ch, lb, 0, TO_ROOM);
    obj_from_room(lb);
    extract_obj(lb);
    lb = obj = NULL;
    orphans--;

  }
  else
  {
    if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | dragonarmies)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | zhentarim)\r\n");
    if (CONFIG_CAMPAIGN == CAMPAIGN_GOLARION)
    send_to_char(ch, "Who are you looking to send, and where to? (foster-home | redmantis)\r\n");
    return 1;
  }

  if (roll > 70)
  {
    reward = 1500;
  }
  else if (roll > 60)
  {
    reward = 1000;
  }
  else if (roll > 50)
  {
    reward = 750;
  }
  else if (roll > 40)
  {
    reward = 500;
  }
  else if (roll > 30)
  {
    reward = 300;
  }
  else if (roll > 25)
  {
    reward = 200;
  }
  else if (roll > 20)
  {
    reward = 100;
  }
  else if (roll > 15)
  {
    reward = 50;
  }
  else {
    reward = 0;
  }

  if (!reward) {
    send_to_char(ch, "There is no reward for your action.\r\n");
  } else {
    send_to_char(ch, "You are rewarded %d %s for your action.\r\n", reward, MONEY_STRING);
  }

  send_to_char(ch, "Your reputation increased by %d for your action.\r\n", GET_LEVEL(ch));

  GET_QUESTPOINTS(ch) += GET_LEVEL(ch);
  GET_REPUTATION(ch) += GET_LEVEL(ch);

  return 1;
}

SPECIAL(repair_mob)
{

  if (!CMD_IS("repair"))
  {
    return 0;
  }

  struct char_data *mob = (struct char_data *) me;

  if (!mob)
    return 0;

  struct obj_data *obj;

  skip_spaces(&argument);

  if (is_abbrev(argument, "all")) {
    int cost = 0;

    int i = 0;

    for (i = 0; i < NUM_WEARS; i++) {

      obj = GET_EQ(ch, i);
      if (!obj) continue;

      if (GET_OBJ_VAL(obj, 4) >= 100)
        continue;

      cost = GET_OBJ_COST(obj) * 6 / 100;

      cost = cost * (100 - GET_OBJ_VAL(obj, 4)) / 100;

      if (GET_GOLD(ch) < cost) {
        send_to_char(ch, "You did not have enough %s on hand to repair %s\r\n", MONEY_STRING, obj->short_description);
        continue;
      }

      GET_GOLD(ch) -= cost;

      obj_to_char(unequip_char(ch, i), mob);
//      obj_from_char(obj);
//      obj_to_char(obj, mob);

      SET_SKILL(mob, SKILL_MECHANICS, 100);

      do_fix(mob, obj->name, 0, 0);

      obj_from_char(obj);
      obj_to_char(obj, ch);
      equip_char(ch, obj, i);

      send_to_char(ch, "It cost you %d %s to have %s repaired.\r\n", cost, MONEY_STRING, obj->short_description);
    }
  } else {

  if (!*argument) {
    send_to_char(ch, "What do you want to repair?\r\n");
    return 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
    send_to_char(ch, "There is nothing in your inventory by the name of %s.\r\n", argument);
    return 1;
  }

    if (GET_OBJ_VAL(obj, 4) >= 100) {
      send_to_char(ch, "That item is not in need of repair.\r\n");
      return 1;
    }

    int cost = GET_OBJ_COST(obj) * 6 / 100;

    cost = cost * (100 - GET_OBJ_VAL(obj, 4)) / 100;

  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You don't have enough %s.  You need %d and have %d.\r\n", MONEY_STRING, cost, GET_GOLD(ch));
    return 1;
  }

  GET_GOLD(ch) -= cost;

  obj_from_char(obj);
  obj_to_char(obj, mob);

  SET_SKILL(mob, SKILL_MECHANICS, 100);

  do_fix(mob, argument, 0, 0);

  obj_from_char(obj);
  obj_to_char(obj, ch);

  send_to_char(ch, "It cost you %d %s to have %s repaired.\r\n", cost, MONEY_STRING, obj->short_description);
  }

  return 1;
}

SPECIAL(license_mob)
{
  return 0;

}
