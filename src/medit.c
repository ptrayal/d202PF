/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: medit.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "genshp.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "screen.h"
#include "spec_procs.h"

/*-------------------------------------------------------------------*/

void set_auto_mob_stats(struct char_data *mob);
void advance_mob_level(struct char_data *ch, int whichclass);
void medit_disp_spec_proc(struct descriptor_data *d);
proctype get_spec_proc(char *name);
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void medit_skin_data(struct descriptor_data *d, int i);

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern mob_rnum top_of_mobt;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
extern struct spec_list spec_names[];

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ACMD(do_oasis_medit)
{
  int number = NOBODY, save = 0, real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  
  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  buf3 = two_arguments(argument, buf1, buf2);
  
  if (!*buf1) {
    send_to_char(ch, "Specify a mobile VNUM to edit.\r\n");
    return;
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    
    save = TRUE;
    
    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0) {
      zone_rnum zlok;
      
      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }
    
    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }
  
  /****************************************************************************/
  /** If a numeric argument was given (like a room number), get it.          **/
  /****************************************************************************/
  if (number == NOBODY)
    number = atoi(buf1);
  
  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                  **/
  /****************************************************************************/
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_MEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That mobile is currently being edited by %s.\r\n",
          GET_NAME(d->character));
        return;
      }
    }
  }
  
  d = ch->desc;
  
  /****************************************************************************/
  /** Give descriptor an OLC structure.                                      **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE,
      "SYSERR: do_oasis_medit: Player already had olc structure.");
    free(d->olc);
  }
  
  CREATE(d->olc, struct oasis_olc_data, 1);
  
  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    mudlog(CMP, ADMLVL_IMPL, TRUE, "OLC: %s tried to edit zone %d allowed zone %d",
      GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  /****************************************************************************/
  /** If save is TRUE, save the mobiles.                                     **/
  /****************************************************************************/
  if (save) {
    send_to_char(ch, "Saving all mobiles in zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves mobile info for zone %d.",
      GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    
    /**************************************************************************/
    /** Save the mobiles.                                                    **/
    /**************************************************************************/
    save_mobiles(OLC_ZNUM(d));
    
    /**************************************************************************/
    /** Free the olc structure stored in the descriptor.                     **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  OLC_NUM(d) = number;
  
  /****************************************************************************/
  /** If this is a new mobile, setup a new one, otherwise, setup the         **/
  /** existing mobile.                                                       **/
  /****************************************************************************/
  if ((real_num = real_mobile(number)) == NOBODY)
    medit_setup_new(d);
  else
    medit_setup_existing(d, real_num);
  
  STATE(d) = CON_MEDIT;
  
  /****************************************************************************/
  /** Display the OLC messages to the players in the same room as the        **/
  /** builder and also log it.                                               **/
  /****************************************************************************/
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  
  mudlog(CMP, ADMLVL_IMMORT, TRUE,"OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

void medit_save_to_disk(zone_vnum foo)
{
  save_mobiles(real_zone(foo));
}

void medit_setup_new(struct descriptor_data *d)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.  
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  GET_MOB_RNUM(mob) = NOBODY;
  /*
   * Set up some default strings.
   */
  GET_ALIAS(mob) = strdup("mob unfinished");
  GET_SDESC(mob) = strdup("the unfinished mob");
  GET_LDESC(mob) = strdup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = strdup("It looks unfinished.\r\n");
  SCRIPT(mob) = NULL;
  mob->proto_script = OLC_SCRIPT(d) = NULL;

  OLC_MOB(d) = mob;
  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d) = FALSE;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure. 
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

  OLC_MOB(d) = mob;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
  dg_olc_script_copy(d);
  /*
   * The edited mob must not have a script.
   * It will be assigned to the updated mob later, after editing.
   */
  SCRIPT(mob) = NULL;
  OLC_MOB(d)->proto_script = NULL;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob)
{
  int i;
  
  clear_char(mob);

  GET_HIT(mob) = GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = GET_MAX_KI(mob) = 100;
  GET_NDD(mob) = GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;
  
 for(i=0; i < 4; i++)
    mob->mob_specials.skin_data[i] = 0;   

  mob->real_abils.str = mob->real_abils.intel = mob->real_abils.wis = 11;
  mob->real_abils.dex = mob->real_abils.con = mob->real_abils.cha = 11;
  mob->aff_abils = mob->real_abils;

  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d)
{
  int i;
  mob_rnum new_rnum;
  struct descriptor_data *dsc;
  struct char_data *mob;

  i = (real_mobile(OLC_NUM(d)) == NOBODY);

  if ((new_rnum = add_mobile(OLC_MOB(d), OLC_NUM(d))) == NOBODY) {
    log("medit_save_internally: add_mobile failed.");
    return;
  }



  /* Update triggers */
  /* Free old proto list  */
  if (mob_proto[new_rnum].proto_script &&
      mob_proto[new_rnum].proto_script != OLC_SCRIPT(d)) 
    free_proto_script(&mob_proto[new_rnum], MOB_TRIGGER);   

  mob_proto[new_rnum].proto_script = OLC_SCRIPT(d);

  /* this takes care of the mobs currently in-game */
  for (mob = character_list; mob; mob = mob->next) {
    if (GET_MOB_RNUM(mob) != new_rnum) 
      continue;
    
    /* remove any old scripts */
    if (SCRIPT(mob)) 
      extract_script(mob, MOB_TRIGGER);

    free_proto_script(mob, MOB_TRIGGER);
    copy_proto_script(&mob_proto[new_rnum], mob, MOB_TRIGGER);
    assign_triggers(mob, MOB_TRIGGER);
  }
  /* end trigger update */  

  if (!i)	/* Only renumber on new mobiles. */
    return;

  /*
   * Update keepers in shops being edited and other mobs being edited.
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (STATE(dsc) == CON_SEDIT)
      S_KEEPER(OLC_SHOP(dsc)) += (S_KEEPER(OLC_SHOP(dsc)) >= new_rnum);
    else if (STATE(dsc) == CON_MEDIT)
      GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
  }

  /*
   * Update other people in zedit too. From: C.Raehl 4/27/99
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        if (OLC_ZONE(dsc)->cmd[i].command == 'M')
          if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
            OLC_ZONE(dsc)->cmd[i].arg1++;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d)
{
  int i;

  clear_screen(d);

  for (i = 0; *position_types[i] != '\n'; i++) {
    write_to_output(d, "@g%2d@n) %s\r\n", i, position_types[i]);
  }
  write_to_output(d, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d)
{
  int i;

  clear_screen(d);

  for (i = 0; i < NUM_GENDERS; i++) {
    write_to_output(d, "@g%2d@n) %s\r\n", i, genders[i]);
  }
  write_to_output(d, "Enter gender number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;

  clear_screen(d);

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    write_to_output(d, "@g%2d@n) %s\r\n", i, attack_hit_text[i].singular);
  }
  write_to_output(d, "Enter attack type : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d)
{
  int i, columns = 0;
  char flags[MAX_STRING_LENGTH];
  
  clear_screen(d);
  for (i = 0; i < NUM_MOB_FLAGS; i++) {
    write_to_output(d, "@g%2d@n) %-20.20s  %s", i + 1, action_bits[i],
		!(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(MOB_FLAGS(OLC_MOB(d)), action_bits, AF_ARRAY_MAX, flags);
  write_to_output(d, "\r\nCurrent flags : @c%s@n\r\nEnter mob flags (0 to quit) : ",
		  flags);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d)
{
  int i, columns = 0;
  char flags[MAX_STRING_LENGTH];

  clear_screen(d);
  for (i = 0; i < NUM_AFF_FLAGS; i++) {
    write_to_output(d, "@g%2d@n) %-20.20s  %s", i + 1, affected_bits[i+1],
                    !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(AFF_FLAGS(OLC_MOB(d)), affected_bits, AF_ARRAY_MAX, flags);
  write_to_output(d, "\r\nCurrent flags   : @c%s@n\r\nEnter aff flags (0 to quit) : ",
                  flags);
}

/*-------------------------------------------------------------------*/

/*
 * Display class menu.
 */
void medit_disp_class(struct descriptor_data *d)
{
  int i;
  char buf[MAX_INPUT_LENGTH];

  clear_screen(d);
  for (i = 0; i < NUM_CLASSES; i++) {
    sprintf(buf, "@g%2d@n) %s\r\n", i, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
    write_to_output(d, buf);
  }
  write_to_output(d, "Enter class number : ");
}
/*-------------------------------------------------------------------*/
/*
 * Display race menu.
 */
void medit_disp_race(struct descriptor_data *d)
{
  int i, columns = 0;
  char buf[MAX_INPUT_LENGTH];

  clear_screen(d);
  for (i = 0; i < NUM_RACES; i++) {
    if (race_list[i].family == RACE_TYPE_UNDEFINED)
	      continue;
    sprintf(buf, "@g%2d@n) %-20.20s  %s", i , pc_race_types[i],
                        !(++columns % 2) ? "\r\n" : "");
    write_to_output(d, buf);
  }
  write_to_output(d, "Enter race number : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display size menu.
 */
void medit_disp_size(struct descriptor_data *d)
{
  int i, columns = 0;
  char buf[MAX_INPUT_LENGTH];

  clear_screen(d);
  for (i = -1; i < NUM_SIZES; i++) {
    sprintf(buf, "@g%2d@n) %-20.20s  %s", i ,
            (i == SIZE_UNDEFINED) ? "DEFAULT" : size_names[i],
                        !(++columns % 2) ? "\r\n" : "");
    write_to_output(d, buf);
  }
  write_to_output(d, "Enter size number (-1 for default): ");
}

void medit_skin_data(struct descriptor_data *d, int i)
 {
  char buf[MAX_STRING_LENGTH];
 
sprintf(buf, "Enter a VNUM for object (slot number %i of 4 slots) to be loaded when this mob's\r\n"
          , i);    
 send_to_char(d->character, buf);
 send_to_char(d->character, "corpse is skinned: ");
 return; 
 }    

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d)
{
  struct char_data *mob;
  char flags[MAX_STRING_LENGTH], flag2[MAX_STRING_LENGTH];

  mob = OLC_MOB(d);
  clear_screen(d);

  write_to_output(d,
  "-- Mob Number:  [@c%d@n]\r\n"
  "@g1@n) Sex: @y%-7.7s@n	         @g2@n) Alias: @y%s\r\n"
  "@g3@n) S-Desc: @y%s\r\n"
  "@g4@n) L-Desc:-\r\n@y%s"
  "@g5@n) D-Desc:-\r\n@y%s"
  "@g6@n) Level:       [@c%4d@n],  @g7@n) Alignment:    [@c%4d@n]\r\n"
  "@g8@n) Hitroll:     [@c%4d@n],  @g9@n) Damroll:      [@c%4d@n]\r\n"
  "@gA@n) NumDamDice:  [@c%4d@n],  @gB@n) SizeDamDice:  [@c%4d@n]\r\n"
  "@gC@n) Hit Points:  [@c%4d@n],   @gD@n) Stamina: [@c%5d@n]\r\n"
  "@gE@n) Armor Class: [@c%4d@n],  @gF@n) Exp:     [@c%9d@n],  @gG@n) Gold:  [@c%8d@n]\r\n"
  "@gH@n) Str: [@c%2d@n]  @gI@n) Con: [@c%2d@n]  @gJ@n) Dex: [@c%2d@n]\r\n"
  "@gK@n) Int: [@c%2d@n]  @gL@n) Wis: [@c%2d@n]  @gM@n) Cha: [@c%2d@n]\r\n",

	  OLC_NUM(d), genders[(int)GET_SEX(mob)], GET_ALIAS(mob),
	  GET_SDESC(mob), GET_LDESC(mob), GET_DDESC(mob), GET_HITDICE(mob),
	  GET_ALIGNMENT(mob), GET_ACCURACY_MOD(mob), GET_DAMAGE_MOD(mob),
	  GET_NDD(mob), GET_SDD(mob), GET_MAX_HIT(mob),
	  GET_MAX_MOVE(mob), GET_ARMOR(mob), GET_EXP(mob), GET_GOLD(mob),
          GET_STR(mob), GET_CON(mob), GET_DEX(mob),
          GET_INT(mob), GET_WIS(mob), GET_CHA(mob)
	  );
  sprintbitarray(MOB_FLAGS(mob), action_bits, AF_ARRAY_MAX, flags);
  sprintbitarray(AFF_FLAGS(mob), affected_bits, AF_ARRAY_MAX, flag2);
  write_to_output(d,
	  "@gN@n) Position  : @y%s\r\n"
	  "@gO@n) Default   : @y%s\r\n"
	  "@gP@n) Attack    : @y%s\r\n"
	  "@gR@n) NPC Flags : @c%s\r\n"
	  "@gS@n) AFF Flags : @c%s\r\n"
          "@gT@n) Class     : @y%s\r\n"
          "@gU@n) Race      : @y%s\r\n"
	  "@gV@n) Spec Proc : @y%s\r\n"
          "@gW@n) Script    : @c%s\r\n"
          "@gX@n) Size      : @y%s\r\n"
	  "@gQ@n) Quit\r\n"
	  "Enter choice : ",

	  position_types[(int)GET_POS(mob)],
	  position_types[(int)GET_DEFAULT_POS(mob)],
	  attack_hit_text[(int)GET_ATTACK(mob)].singular,
	  flags, flag2, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[(int)GET_CLASS(mob)],
          race_list[(int)GET_RACE(mob)].name,
	  GET_MOB_SPEC(mob) ? "Set." : "Not Set",
          OLC_SCRIPT(d) ?"Set.":"Not Set.", size_names[get_size(mob)]
	  );

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg)
{
  int i = -1;
  char *oldtext = NULL;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
    i = atoi(arg);
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
      write_to_output(d, "Field must be numerical, try again : ");
      return;
    }
  } else {	/* String response. */
    if (!genolc_checkstring(d, arg))
      return;
  }
  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*
     * Ensure mob has MOB_ISNPC set or things will go pear shaped.
     */
    SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the mob in memory and to disk.
       */
      medit_save_internally(d);
      mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
	"OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE) {
	medit_save_to_disk(zone_table[real_zone_by_thing(OLC_NUM(d))].number);
	write_to_output(d, "Mobile saved to disk.\r\n");
      } else
        write_to_output(d, "Mobile saved to memory.\r\n");
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save the mobile? : ");
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {	/* Anything been changed? */
	write_to_output(d, "Do you wish to save the changes to the mobile? (y/n) : ");
	OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_ALIAS;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_S_DESC;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_D_DESC;
      send_editor_help(d);
      write_to_output(d, "Enter mob description:\r\n\r\n");
      if (OLC_MOB(d)->description) {
	write_to_output(d, "%s", OLC_MOB(d)->description);
	oldtext = strdup(OLC_MOB(d)->description);
      }
      string_write(d, &OLC_MOB(d)->description, MAX_MOB_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      return;
    case '6':
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      i++;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_ACCURACY;
      i++;
      break;
    case '9':
      OLC_MODE(d) = MEDIT_DAMAGE;
      i++;
      break;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_NDD;
      i++;
      break;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_SDD;
      i++;
      break;
			
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;

    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_MOVE_POINTS;
      i++;
      break;

    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;

    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;

    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;

    case 'h':
    case 'H':
      OLC_MODE(d) = MEDIT_STRENGTH;
      i++;
      break;

    case 'i':
    case 'I':
      OLC_MODE(d) = MEDIT_CONSTITUTION;
      i++;
      break;

    case 'j':
    case 'J':
      OLC_MODE(d) = MEDIT_DEXTERITY;
      i++;
      break;

    case 'k':
    case 'K':
      OLC_MODE(d) = MEDIT_INTELLIGENCE;
      i++;
      break;

    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_WISDOM;
      i++;
      break;

    case 'm':
    case 'M':
      OLC_MODE(d) = MEDIT_CHARISMA;
      i++;
      break;


    case 'n':
    case 'N':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case 'o':
    case 'O':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_ATTACK;
      medit_disp_attack_types(d);
      return;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 's':
    case 'S':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 't':
    case 'T':
      OLC_MODE(d) = MEDIT_CLASS;
      medit_disp_class(d);
      return;
    case 'u':
    case 'U':
      OLC_MODE(d) = MEDIT_RACE;
      medit_disp_race(d);
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = MEDIT_SPEC_PROC;
      medit_disp_spec_proc(d);
      return;
    case 'w':
    case 'W':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'x':
    case 'X':
      OLC_MODE(d) = MEDIT_SIZE;
      medit_disp_size(d);
      return;
    default:
      medit_disp_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else if (i == -1)
      write_to_output(d, "\r\nEnter new text :\r\n] ");
    else
      write_to_output(d, "Oops...\r\n");
    return;
/*-------------------------------------------------------------------*/
  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    smash_tilde(arg);
    if (GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    smash_tilde(arg);
    if (GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    smash_tilde(arg);
    if (GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    if (arg && *arg) {
      char buf[MAX_INPUT_LENGTH];
      snprintf(buf, sizeof(buf), "%s\r\n", arg);
      GET_LDESC(OLC_MOB(d)) = strdup(buf);
    } else
      GET_LDESC(OLC_MOB(d)) = strdup("undefined");

    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached D_DESC case!");
    write_to_output(d, "Oops...\r\n");
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_MOB_FLAGS)
      TOGGLE_BIT_AR(MOB_FLAGS(OLC_MOB(d)), i - 1);
    medit_disp_mob_flags(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_AFF_FLAGS)
      TOGGLE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), i);
    /* Remove unwanted bits right away. */
    REMOVE_BIT_AR(AFF_FLAGS(OLC_MOB(d)),
       AFF_CHARM | AFF_POISON | AFF_GROUP | AFF_SLEEP);
    medit_disp_aff_flags(d);
    return;
/*-------------------------------------------------------------------*/

/*
 * Numerical responses.
 */

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = LIMIT(i, 0, NUM_GENDERS - 1);
    break;

  case MEDIT_ACCURACY:
    GET_ACCURACY_MOD(OLC_MOB(d)) = LIMIT(i, 0, 50);
    break;

  case MEDIT_DAMAGE:
    GET_DAMAGE_MOD(OLC_MOB(d)) = LIMIT(i, 0, 50);
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = LIMIT(i, 1, 30);
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = LIMIT(i, 6, 127);
    break;

  case MEDIT_STRENGTH:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_CONSTITUTION:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_DEXTERITY:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_INTELLIGENCE:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_WISDOM:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_CHARISMA:
    GET_STR(OLC_MOB(d)) = LIMIT(i, 3, 99);
    break;

  case MEDIT_NUM_HP_DICE:
    GET_MAX_HIT(OLC_MOB(d)) = LIMIT(i, 1, 30000);
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MAX_MANA(OLC_MOB(d)) = LIMIT(i, 4, 1000);
    break;

  case MEDIT_ADD_HP:
    GET_MAX_MOVE(OLC_MOB(d)) = LIMIT(i, 0, 30000);
    break;

  case MEDIT_AC:
    GET_ARMOR(OLC_MOB(d)) = LIMIT(i, -1000, 1000);
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = LIMIT(i, 0, MAX_MOB_EXP);
    break;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = LIMIT(i, 0, MAX_MOB_GOLD);
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = LIMIT(i, 0, NUM_ATTACK_TYPES - 1);
    break;

  case MEDIT_LEVEL:
    GET_HITDICE(OLC_MOB(d)) = MIN(MAX(1, i), 70);
    if (!MOB_FLAGGED(OLC_MOB(d), MOB_CUSTOM_STATS)) {
      GET_MAX_HIT(OLC_MOB(d)) = 0;
      set_auto_mob_stats(OLC_MOB(d));
    }
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = LIMIT(i, -1000, 1000);
    break;

  case MEDIT_CLASS:
    GET_CLASS(OLC_MOB(d)) = LIMIT(i, 0, NUM_CLASSES);
    break;

  case MEDIT_RACE:
    if (race_list[i].family == RACE_TYPE_UNDEFINED) {
      send_to_char(d->character, "That is not a valid race, pleas choose again.\r\n");
      return;
    }
    
    GET_REAL_RACE(OLC_MOB(d)) = LIMIT(i, 0, NUM_RACES);
    break;

  case MEDIT_SPEC_PROC:
    if (i == 0) {
      mob_index[(OLC_MOB(d))->nr].func  = NULL;
      REMOVE_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_SPEC);
      break;
    }
    ASSIGNMOB(GET_MOB_VNUM(OLC_MOB(d)), get_spec_proc(spec_names[LIMIT(i, MIN_MOB_SPECS, MAX_MOB_SPECS)].name));
    mob_index[(OLC_MOB(d))->nr].func  = get_spec_proc(spec_names[LIMIT(i, MIN_MOB_SPECS, MAX_MOB_SPECS)].name);
    SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_SPEC);
    break;

  case MEDIT_SIZE:
    OLC_MOB(d)->size = LIMIT(i, -1, NUM_SIZES - 1);
    break;

  case MEDIT_SKIN_DATA0:  
//     OLC_MOB(d)->mob_specials.skin_data[0] = MAX(0, atoi(arg));
//     medit_skin_data(d, 1);
     OLC_MODE(d) = MEDIT_SKIN_DATA1;
     return;
     break;
   case MEDIT_SKIN_DATA1:  
//     OLC_MOB(d)->mob_specials.skin_data[1] = MAX(0, atoi(arg));
//     medit_skin_data(d, 2);
     OLC_MODE(d) = MEDIT_SKIN_DATA2;
     return;
     break;
   case MEDIT_SKIN_DATA2:  
//     OLC_MOB(d)->mob_specials.skin_data[2] = MAX(0, atoi(arg));
//     medit_skin_data(d,3);
     OLC_MODE(d) = MEDIT_SKIN_DATA3;
     return;
     break;
   case MEDIT_SKIN_DATA3:  
//     OLC_MOB(d)->mob_specials.skin_data[3] = MAX(0, atoi(arg));
     break;	
	
/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

  OLC_VAL(d) = TRUE;
  medit_disp_menu(d);
}

void medit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {

  case MEDIT_D_DESC:
  default:
     medit_disp_menu(d);
     break;
  }
}

void medit_disp_spec_proc(struct descriptor_data *d)
{
  int i = 0;

  write_to_output(d, "\r\n");

  for(; spec_names[i].func; i++) {
    write_to_output(d, "%d) %-25s", i, spec_names[i].name);
    if (i%3==2)
      write_to_output(d, "\r\n");
    else
      write_to_output(d, "\t\t");
  }
  write_to_output(d, "Please select a spec proc to apply to this mobile (mob_ spec procs only please) : ");

  return;
}
