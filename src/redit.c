/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: redit.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "handler.h"
#include "spells.h"
#include "feats.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "constants.h"
#include "spec_procs.h"
/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *exit_bits[];
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct spec_list spec_names[];

/*------------------------------------------------------------------------*/

void redit_disp_spec_proc(struct descriptor_data *d);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void UNASSIGNROOM(room_vnum room);

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

ACMD(do_oasis_redit)
{
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int number = NOWHERE, save = 0, real_num;
  struct descriptor_data *d;
  
  /* Parse any arguments. */
  buf3 = two_arguments(argument, buf1, buf2);
  
  if (!*buf1)
    number = GET_ROOM_VNUM(IN_ROOM(ch));
  else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    
    save = TRUE;
      
    if (is_number(buf2))
      number = atoi(buf2);
    /* olc_zone is of type ush_int
    else if (GET_OLC_ZONE(ch) >= 0) { */
      zone_rnum zlok;
        
      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    /*}*/
      
    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }
  
  /*
   * If a numeric argument was given (like a room number), get it.
   */
  if (number == NOWHERE)
    number = atoi(buf1);
  
  /* Check to make sure the room isn't already being edited. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_REDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That room is currently being edited by %s.\r\n", 
          PERS(d->character, ch));
        return;
      }
    }
  }
  
  /* Retrieve the player's descriptor. */
  d = ch->desc;
  
  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_oasis_redit: Player already had olc structure.");
    free(d->olc);
  }
  
  /* Create the OLC structure. */
  CREATE(d->olc, struct oasis_olc_data, 1);
  
  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  /* Make sure the builder is allowed to modify this zone. */
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    mudlog(CMP, ADMLVL_IMPL, TRUE, "OLC: %s tried to edit zone %d allowed zone %d",
               GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
    
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  if (save) {
    send_to_char(ch, "Saving all rooms in zone %d.\r\n", zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves room info for zone %d.", GET_NAME(ch), 
zone_table[OLC_ZNUM(d)].number);
    
    /* Save the rooms. */
    save_rooms(OLC_ZNUM(d));
    
    /* Free the olc data from the descriptor. */
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  OLC_NUM(d) = number;
  
  if ((real_num = real_room(number)) != NOWHERE)
    redit_setup_existing(d, real_num);
  else
    redit_setup_new(d);
  
  STATE(d) = CON_REDIT;
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  
  mudlog(CMP, ADMLVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

void redit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_ROOM(d), struct room_data, 1);

  OLC_ROOM(d)->name = strdup("An unfinished room");
  OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
  OLC_ROOM(d)->number = NOWHERE;
  OLC_ITEM_TYPE(d) = WLD_TRIGGER;
  OLC_ROOM(d)->proto_script = OLC_SCRIPT(d) = NULL;

  redit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct room_data *room;
  int counter;

  /*
   * Build a copy of the room for editing.
   */
  CREATE(room, struct room_data, 1);

  *room = world[real_num];
  /*
   * Allocate space for all strings.
   */
  room->name = str_udup(world[real_num].name);
  room->description = str_udup(world[real_num].description);

  /*
   * Exits - We allocate only if necessary.
   */
  for (counter = 0; counter < NUM_OF_DIRS; counter++) {
    if (world[real_num].dir_option[counter]) {
      CREATE(room->dir_option[counter], struct room_direction_data, 1);

      /*
       * Copy the numbers over.
       */
      *room->dir_option[counter] = *world[real_num].dir_option[counter];
      /*
       * Allocate the strings.
       */
      if (world[real_num].dir_option[counter]->general_description)
        room->dir_option[counter]->general_description = strdup(world[real_num].dir_option[counter]->general_description);
      if (world[real_num].dir_option[counter]->keyword)
        room->dir_option[counter]->keyword = strdup(world[real_num].dir_option[counter]->keyword);
    }
  }

  /*
   * Extra descriptions, if necessary.
   */
  if (world[real_num].ex_description) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    CREATE(temp, struct extra_descr_data, 1);

    room->ex_description = temp;
    for (tdesc = world[real_num].ex_description; tdesc; tdesc = tdesc->next) {
      temp->keyword = strdup(tdesc->keyword);
      temp->description = strdup(tdesc->description);
      if (tdesc->next) {
	CREATE(temp2, struct extra_descr_data, 1);
	temp->next = temp2;
	temp = temp2;
      } else
	temp->next = NULL;
    }
  }
  /*
   * Attach copy of room to player's descriptor.
   */
  OLC_ROOM(d) = room;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = WLD_TRIGGER;

  dg_olc_script_copy(d);
  room->proto_script = NULL;
  SCRIPT(room) = NULL; 

  redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void redit_save_internally(struct descriptor_data *d)
{
  int j, room_num, new_room = FALSE;
  struct descriptor_data *dsc;

  if (OLC_ROOM(d)->number == NOWHERE) {
    new_room = TRUE;
    OLC_ROOM(d)->number = OLC_NUM(d);
  }
  /* FIXME: Why is this not set elsewhere? */
  OLC_ROOM(d)->zone = OLC_ZNUM(d);

  if ((room_num = add_room(OLC_ROOM(d))) == NOWHERE) {
    write_to_output(d, "Something went wrong...\r\n");
    log("SYSERR: redit_save_internally: Something failed! (%d)", room_num);
    return;
  }

  /* Update triggers */  
  /* Free old proto list */
  if (world[room_num].proto_script &&
      world[room_num].proto_script != OLC_SCRIPT(d)) 
    free_proto_script(&world[room_num], WLD_TRIGGER); 
       
  world[room_num].proto_script = OLC_SCRIPT(d);
  assign_triggers(&world[room_num], WLD_TRIGGER);
  /* end trigger update */  
  
  /* Don't adjust numbers on a room update. */
  if (!new_room)
    return;

  /* Idea contributed by C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (dsc == d)
      continue;

    if (STATE(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
          case 'T':
          case 'V':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            /* Fall through */
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
          }
    } else if (STATE(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (OLC_ROOM(dsc)->dir_option[j])
          if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
            OLC_ROOM(dsc)->dir_option[j]->to_room++;
    }
  }
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(zone_vnum zone_num)
{
  save_rooms(zone_num);		/* :) */
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room)
{
  /* Free the strings (Mythran). */
  free_room_strings(room);

  if (SCRIPT(room))
    extract_script(room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);
  
  /* Free the room. */
  free(room);	/* XXX ? */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  clear_screen(d);
  write_to_output(d,
	  "@g1@n) Keyword: @y%s\r\n"
	  "@g2@n) Description:\r\n@y%s\r\n"
	  "@g3@n) Goto next description: ",

	  extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	  extra_desc->description ? extra_desc->description : "<NONE>"
	  );

  write_to_output(d, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
  write_to_output(d, "Enter choice (0 to quit) : ");
  OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d)
{
  char door_buf[40];
  /*
   * if exit doesn't exist, alloc/create it 
   */
  if (OLC_EXIT(d) == NULL)
    CREATE(OLC_EXIT(d), struct room_direction_data, 1);

  sprintbit(OLC_EXIT(d)->exit_info, exit_bits, door_buf, sizeof(door_buf));

  clear_screen(d);
  write_to_output(d,
	  "@g1@n) Exit to     		: @c%d\r\n"
	  "@g2@n) Description 		:-\r\n@y%s\r\n"
	  "@g3@n) Door name   		: @y%s\r\n"
	  "@g4@n) Key         		: @c%d\r\n"
	  "@g5@n) Door flags  		: @c%s\r\n"
	  "@g6@n) Purge exit.\r\n"
	  "@g7@n) DC Lock		: @c%d\r\n"
 	  "@g8@n) DC Hide     		: @c%d\r\n"
	  "@g9@n) DC Skill		: @c%s\r\n"
	  "@gA@n) DC Move		: @c%d\r\n"
	  "@gB@n) Skill Fail Save Type		: @c%d\r\n"
	  "@gC@n) DC Skill Save		: @c%d\r\n"
	  "@gD@n) Minor Fail Dest. Room	: @c%d\r\n"
	  "@gE@n) Major Fail Dest. Room	: @c%d@n\r\n"
	  "Enter choice, 0 to quit : ",

	  OLC_EXIT(d)->to_room != NOWHERE ? world[OLC_EXIT(d)->to_room].number : -1,
	  OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
	  OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
	  OLC_EXIT(d)->key != NOTHING ? OLC_EXIT(d)->key : -1,
	  door_buf, OLC_EXIT(d)->dclock, OLC_EXIT(d)->dchide,
          OLC_EXIT(d)->dcskill != 0 ? spell_info[OLC_EXIT(d)->dcskill].name : "<NONE>",
          OLC_EXIT(d)->dcmove, OLC_EXIT(d)->failsavetype, 
          OLC_EXIT(d)->dcfailsave, OLC_EXIT(d)->failroom, 
          OLC_EXIT(d)->totalfailroom
	  );

  OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d)
{
  char bits[MAX_INPUT_LENGTH];
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_EXIT_FLAGS; counter++) {
    write_to_output(d, "@g%2d@n) %-20.20s%s",
                    counter + 1,
                    exit_bits[counter],
                    !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(OLC_EXIT(d)->exit_info, exit_bits, bits, sizeof(bits));
  write_to_output(d, "\r\nExit flags: @c%s@n\r\n"
                     "Enter exit flags, 0 to quit : ",
                     bits);
  OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
}

/*
 * For room flags.
 */
void redit_disp_flag_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_FLAGS; counter++) {
    write_to_output(d, "@g%2d@n) %-20.20s%s", counter + 1,
		room_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(OLC_ROOM(d)->room_flags, room_bits, RF_ARRAY_MAX, bits);
  write_to_output(d, "\r\nRoom flags: @c%s@n\r\n"
	  "Enter room flags, 0 to quit : ", bits);
  OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
void redit_disp_sector_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
    write_to_output(d, "@g%2d@n) %-20.20s%s", counter,
		sector_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter sector type : ");
  OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct room_data *room;

  room_rnum rnum = NOWHERE;

  rnum = real_room(OLC_ROOM(d)->number);

  clear_screen(d);
  room = OLC_ROOM(d);

  sprintbitarray(room->room_flags, room_bits, RF_ARRAY_MAX, buf1);
  sprinttype(room->sector_type, sector_types, buf2, sizeof(buf2));
  write_to_output(d,
	  "-- Room number : [@c%d@n]  	Room zone: [@c%d@n]\r\n"
	  "@g1@n) Name        : @y%s\r\n"
	  "@g2@n) Description :\r\n@y%s"
	  "@g3@n) Room flags  : @c%s\r\n"
	  "@g4@n) Sector type : @c%s\r\n"
	  "@g5@n) Exit north  : [@c%6d@n],  @gB@n) Exit northwest : [@c%6d@n]\r\n"
	  "@g6@n) Exit east   : [@c%6d@n],  @gC@n) Exit northeast : [@c%6d@n]\r\n"
	  "@g7@n) Exit south  : [@c%6d@n],  @gD@n) Exit southeast : [@c%6d@n]\r\n"
	  "@g8@n) Exit west   : [@c%6d@n],  @gE@n) Exit southwest : [@c%6d@n]\r\n"
	  "@g9@n) Exit up     : [@c%6d@n],  @gF@n) Exit in        : [@c%6d@n]\r\n"
	  "@gA@n) Exit down   : [@c%6d@n],  @gG@n) Exit out       : [@c%6d@n]\r\n"
	  "@gH@n) Extra descriptions menu\r\n"
	  "@gX@n) Delete Room\r\n"
	  "@gS@n) Script      : @c%s\r\n"
          "@gV@n) Spec Proc   : @c%s\r\n"
	  "@gQ@n) Quit\r\n"
	  "Enter choice : ",

	  OLC_NUM(d), zone_table[OLC_ZNUM(d)].number, room->name,
	  room->description, buf1, buf2,
	  room->dir_option[NORTH] && room->dir_option[NORTH]->to_room != NOWHERE ?
	  world[room->dir_option[NORTH]->to_room].number : -1,
	  room->dir_option[NORTHWEST] && room->dir_option[NORTHWEST]->to_room != NOWHERE ?
	  world[room->dir_option[NORTHWEST]->to_room].number : -1,
	  room->dir_option[EAST] && room->dir_option[EAST]->to_room != NOWHERE ?
	  world[room->dir_option[EAST]->to_room].number : -1,
	  room->dir_option[NORTHEAST] && room->dir_option[NORTHEAST]->to_room != NOWHERE ?
	  world[room->dir_option[NORTHEAST]->to_room].number : -1,
	  room->dir_option[SOUTH] && room->dir_option[SOUTH]->to_room != NOWHERE ?
	  world[room->dir_option[SOUTH]->to_room].number : -1,
	  room->dir_option[SOUTHEAST] && room->dir_option[SOUTHEAST]->to_room != NOWHERE ?
	  world[room->dir_option[SOUTHEAST]->to_room].number : -1,
	  room->dir_option[WEST] && room->dir_option[WEST]->to_room != NOWHERE ?
	  world[room->dir_option[WEST]->to_room].number : -1,
	  room->dir_option[SOUTHWEST] && room->dir_option[SOUTHWEST]->to_room != NOWHERE ?
	  world[room->dir_option[SOUTHWEST]->to_room].number : -1,
	  room->dir_option[UP] && room->dir_option[UP]->to_room != NOWHERE ? 
	  world[room->dir_option[UP]->to_room].number : -1,
	  room->dir_option[INDIR] && room->dir_option[INDIR]->to_room != NOWHERE ?
	  world[room->dir_option[INDIR]->to_room].number : -1,
	  room->dir_option[DOWN] && room->dir_option[DOWN]->to_room != NOWHERE ?
	  world[room->dir_option[DOWN]->to_room].number : -1,
	  room->dir_option[OUTDIR] && room->dir_option[OUTDIR]->to_room != NOWHERE ?
	  world[room->dir_option[OUTDIR]->to_room].number : -1,
          OLC_SCRIPT(d) ? "Set" : "Not Set",
          rnum != NOWHERE ? ((GET_ROOM_SPEC(rnum) != NULL )? "Set" : "Not Set") : "Not Set"
	  );

  OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg)
{
  int number;
  char *oldtext = NULL;
  room_rnum rnum = NOWHERE;

  switch (OLC_MODE(d)) {
  case REDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      redit_save_internally(d);
      mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
	"OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE) {
	redit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
	write_to_output(d, "Room saved to disk.\r\n");
      } else
        write_to_output(d, "Room saved to memory.\r\n");
      /*
       * Do NOT free strings! Just the room structure. 
       */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /*
       * Free everything up, including strings, etc.
       */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save this room? : ");
      break;
    }
    return;

  case REDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Something has been modified. */
        write_to_output(d, "Do you wish to save this room? : ");
        OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      write_to_output(d, "Enter room name:-\r\n] ");
      OLC_MODE(d) = REDIT_NAME;
      break;
    case '2':
      OLC_MODE(d) = REDIT_DESC;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter room description:\r\n\r\n");

      if (OLC_ROOM(d)->description) {
	write_to_output(d, "%s", OLC_ROOM(d)->description);
	oldtext = strdup(OLC_ROOM(d)->description);
      }
      string_write(d, &OLC_ROOM(d)->description, MAX_ROOM_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '3':
      redit_disp_flag_menu(d);
      break;
    case '4':
      redit_disp_sector_menu(d);
      break;
    case '5':
      OLC_VAL(d) = NORTH;
      redit_disp_exit_menu(d);
      break;
    case '6':
      OLC_VAL(d) = EAST;
      redit_disp_exit_menu(d);
      break;
    case '7':
      OLC_VAL(d) = SOUTH;
      redit_disp_exit_menu(d);
      break;
    case '8':
      OLC_VAL(d) = WEST;
      redit_disp_exit_menu(d);
      break;
    case '9':
      OLC_VAL(d) = UP;
      redit_disp_exit_menu(d);
      break;
    case 'a':
    case 'A':
      OLC_VAL(d) = DOWN;
      redit_disp_exit_menu(d);
      break;
    case 'b':
    case 'B':
      OLC_VAL(d) = NORTHWEST;
      redit_disp_exit_menu(d);
      break;
    case 'c':
    case 'C':
      OLC_VAL(d) = NORTHEAST;
      redit_disp_exit_menu(d);
      break;
    case 'd':
    case 'D':
      OLC_VAL(d) = SOUTHEAST;
      redit_disp_exit_menu(d);
      break;
    case 'e':
    case 'E':
      OLC_VAL(d) = SOUTHWEST;
      redit_disp_exit_menu(d);
      break;
    case 'f':
    case 'F':
      OLC_VAL(d) = INDIR;
      redit_disp_exit_menu(d);
      break;
    case 'g':
    case 'G':
      OLC_VAL(d) = OUTDIR;
      redit_disp_exit_menu(d);
      break; 
    case 'h':
    case 'H':
      /*
       * If the extra description doesn't exist.
       */
      if (!OLC_ROOM(d)->ex_description)
	CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
      OLC_DESC(d) = OLC_ROOM(d)->ex_description;
      redit_disp_extradesc_menu(d);
      break;
    case 'x':
    case 'X':
      /* Delete the room, prompt first. */
      write_to_output(d, "Are you sure you want to delete this room? ");
      OLC_MODE(d) = REDIT_DELETE;
      break;
      
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = REDIT_SPEC_PROC;
      redit_disp_spec_proc(d);
      return;
    default:
      write_to_output(d, "Invalid choice!");
      redit_disp_menu(d);
      break;
    }
    return;
    

  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;

  case REDIT_NAME:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_ROOM(d)->name)
      free(OLC_ROOM(d)->name);
    arg[MAX_ROOM_NAME - 1] = '\0';
    OLC_ROOM(d)->name = str_udup(arg);
    break;

  case REDIT_DESC:
    /*
     * We will NEVER get here, we hope.
     */
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: Reached REDIT_DESC case in parse_redit().");
    write_to_output(d, "Oops, in REDIT_DESC.\r\n");
    break;

  case REDIT_FLAGS:
    number = atoi(arg);
    if (number < 0 || number > NUM_ROOM_FLAGS) {
      write_to_output(d, "That is not a valid choice!\r\n");
      redit_disp_flag_menu(d);
    } else if (number == 0)
      break;
    else {
      /*
       * Toggle the bit.
       */
      TOGGLE_BIT_AR(OLC_ROOM(d)->room_flags, number - 1); 
      redit_disp_flag_menu(d);
    }
    return;

  case REDIT_SECTOR:
    number = atoi(arg);
    if (number < 0 || number >= NUM_ROOM_SECTORS) {
      write_to_output(d, "Invalid choice!");
      redit_disp_sector_menu(d);
      return;
    }
    OLC_ROOM(d)->sector_type = number;
    break;

  case REDIT_EXIT_MENU:
    switch (*arg) {
    case '0':
      break;
    case '1':
      OLC_MODE(d) = REDIT_EXIT_NUMBER;
      write_to_output(d, "Exit to room number : ");
      return;
    case '2':
      OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter exit description:\r\n\r\n");
      if (OLC_EXIT(d)->general_description) {
	write_to_output(d, "%s", OLC_EXIT(d)->general_description);
	oldtext = strdup(OLC_EXIT(d)->general_description);
      }
      string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC, 0, oldtext);
      return;
    case '3':
      OLC_MODE(d) = REDIT_EXIT_KEYWORD;
      write_to_output(d, "Enter keywords : ");
      return;
    case '4':
      OLC_MODE(d) = REDIT_EXIT_KEY;
      write_to_output(d, "Enter key number : ");
      return;
    case '5':
      OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
      redit_disp_exit_flag_menu(d);
      return;
    case '6':
      /*
       * Delete an exit.
       */
      if (OLC_EXIT(d)->keyword)
	free(OLC_EXIT(d)->keyword);
      if (OLC_EXIT(d)->general_description)
	free(OLC_EXIT(d)->general_description);
      if (OLC_EXIT(d))
	free(OLC_EXIT(d));
      OLC_EXIT(d) = NULL;
      break;
    case '7':
      OLC_MODE(d) = REDIT_EXIT_DCLOCK;
      write_to_output(d, "Enter lock DC number : ");
      return;
    case '8':
      OLC_MODE(d) = REDIT_EXIT_DCHIDE;
      write_to_output(d, "Enter door search DC number : ");
      return;
    case '9':
      OLC_MODE(d) = REDIT_EXIT_DCSKILL;
      write_to_output(d, "Enter skill to be checked to pass through exit : ");
      return;
    case 'a':
    case 'A':
      OLC_MODE(d) = REDIT_EXIT_DCMOVE;
      write_to_output(d, "Enter DC for skill required to pass through exit : ");
      return;
    case 'b':
    case 'B':
      OLC_MODE(d) = REDIT_EXIT_SAVETYPE;
      write_to_output(d, "Enter the SAVE TYPE for failed skill checks : ");
      return;
    case 'c':
    case 'C':
      OLC_MODE(d) = REDIT_EXIT_DCSAVE;
      write_to_output(d, "Enter the DC to beat for SAVE from failed skill checks : ");
      return;
    case 'd':
    case 'D':
      OLC_MODE(d) = REDIT_EXIT_FAILROOM;
      write_to_output(d, "Enter the room to send play to for minor save failure : ");
      return;
    case 'e':
    case 'E':
      OLC_MODE(d) = REDIT_EXIT_TOTALFAILROOM;
      write_to_output(d, "Enter the room to send play to for major save failure : ");
      return;
    default:
      write_to_output(d, "Try again : ");
      return;
    }
    break;

  case REDIT_EXIT_NUMBER:
    if ((number = atoi(arg)) != -1)
      if ((number = real_room(number)) == NOWHERE) {
	write_to_output(d, "That room does not exist, try again : ");
	return;
      }
    OLC_EXIT(d)->to_room = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DESCRIPTION:
    /*
     * We should NEVER get here, hopefully.
     */
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: Reached REDIT_EXIT_DESC case in parse_redit");
    write_to_output(d, "Oops, in REDIT_EXIT_DESCRIPTION.\r\n");
    break;

  case REDIT_EXIT_KEYWORD:
    if (OLC_EXIT(d)->keyword)
      free(OLC_EXIT(d)->keyword);
    OLC_EXIT(d)->keyword = str_udup(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_KEY:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->key = NOTHING;
    else
      OLC_EXIT(d)->key = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DOORFLAGS:
    number = atoi(arg);
    if (number < 0 || number > NUM_EXIT_FLAGS) {
      write_to_output(d, "That's not a valid choice!\r\n");
      redit_disp_exit_flag_menu(d);
    } else if (number == 0) {
      redit_disp_exit_menu(d);
    } else {
      /* Toggle the bit. */
      TOGGLE_BIT(OLC_EXIT(d)->exit_info, 1 << (number - 1));
      redit_disp_exit_flag_menu(d);
    }
    return;

  case REDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg)) {
      if (OLC_DESC(d)->keyword)
        free(OLC_DESC(d)->keyword);
      OLC_DESC(d)->keyword = str_udup(arg);
    }
    redit_disp_extradesc_menu(d);
    return;

  case REDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) {
    case 0:
      /*
       * If something got left out, delete the extra description
       * when backing out to the menu.
       */
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	struct extra_descr_data *temp;
	if (OLC_DESC(d)->keyword)
	  free(OLC_DESC(d)->keyword);
	if (OLC_DESC(d)->description)
	  free(OLC_DESC(d)->description);

	/*
	 * Clean up pointers.
	 */
	REMOVE_FROM_LIST(OLC_DESC(d), OLC_ROOM(d)->ex_description, next);
	free(OLC_DESC(d));
      }
      break;
    case 1:
      OLC_MODE(d) = REDIT_EXTRADESC_KEY;
      write_to_output(d, "Enter keywords, separated by spaces : ");
      return;
    case 2:
      OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter extra description:\r\n\r\n");
      if (OLC_DESC(d)->description) {
	write_to_output(d, "%s", OLC_DESC(d)->description);
	oldtext = strdup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      return;
    case 3:
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	write_to_output(d, "You can't edit the next extra description without completing this one.\r\n");
	redit_disp_extradesc_menu(d);
      } else {
	struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else {
	  /*
	   * Make new extra description and attach at end.
	   */
	  CREATE(new_extra, struct extra_descr_data, 1);
	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = new_extra;
	}
	redit_disp_extradesc_menu(d);
      }
      return;
    }
    break;
  
  case REDIT_DELETE:
    if (*arg == 'y' || *arg == 'Y') {
      if (delete_room(real_room(OLC_ROOM(d)->number)))
        write_to_output(d, "Room deleted.\r\n");
      else
        write_to_output(d, "Couldn't delete the room!\r\n");
      
      cleanup_olc(d, CLEANUP_ALL);
      return;
    } else if (*arg == 'n' || *arg == 'N') {
      redit_disp_menu(d);
      OLC_MODE(d) = REDIT_MAIN_MENU;      
      return;
    } else
      write_to_output(d, "Please answer 'Y' or 'N': ");
    
    break;  
  
  case REDIT_SPEC_PROC:
    if ((rnum = real_room(OLC_ROOM(d)->number)) != NOWHERE) {
      number = atoi(arg);
      if (number == 0) {
        UNASSIGNROOM(OLC_NUM(d));
        REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_SPEC);
        break;
      }
      write_to_output(d, "Test 4: Number: %d Spec: %s\r\n", number, spec_names[LIMIT(number, MIN_ROOM_SPECS, MAX_ROOM_SPECS)].name);
      ASSIGNROOM(OLC_ROOM(d)->number, get_spec_proc(spec_names[LIMIT(number, MIN_ROOM_SPECS, MAX_ROOM_SPECS)].name));
      world[rnum].func  = get_spec_proc(spec_names[LIMIT(number, MIN_ROOM_SPECS, MAX_ROOM_SPECS)].name);
      SET_BIT_AR(ROOM_FLAGS(rnum), ROOM_SPEC);

    }
    break;

  case REDIT_EXIT_DCLOCK:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->dclock = (ubyte)NOTHING;
    else
      OLC_EXIT(d)->dclock = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DCHIDE:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->dchide = (ubyte)NOTHING;
    else
      OLC_EXIT(d)->dchide = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DCSKILL:
    number = find_skill_num(arg, SKTYPE_SKILL);
    if (number < 1)
      OLC_EXIT(d)->dcskill = 0;
    else
      OLC_EXIT(d)->dcskill = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DCMOVE:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->dcmove = (ubyte)NOTHING;
    else
      OLC_EXIT(d)->dcmove = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_SAVETYPE:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->failsavetype = (ubyte)NOTHING;
    else
      OLC_EXIT(d)->failsavetype = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DCSAVE:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->dcfailsave = (ubyte)NOTHING;
    else
      OLC_EXIT(d)->dcfailsave = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_FAILROOM:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->failroom = NOWHERE;
    else
      OLC_EXIT(d)->failroom = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_TOTALFAILROOM:
    number = atoi(arg);
    if (number < 0)
      OLC_EXIT(d)->totalfailroom = NOWHERE;
    else
      OLC_EXIT(d)->totalfailroom = number;
    redit_disp_exit_menu(d);
    return;

  default:
    /*
     * We should never get here.
     */
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: Reached default case in parse_redit");
    break;
  }
  /*
   * If we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  redit_disp_menu(d);
}

void redit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case REDIT_DESC:
    redit_disp_menu(d);
    break;
  case REDIT_EXIT_DESCRIPTION:
    redit_disp_exit_menu(d);
    break;
  case REDIT_EXTRADESC_DESCRIPTION:
    redit_disp_extradesc_menu(d);
    break;
  }
}

void redit_disp_spec_proc(struct descriptor_data *d)
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
  write_to_output(d, "Please select a spec proc to apply to this room (room_ spec procs only please) : ");

  return;
}

