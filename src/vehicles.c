/*************************************************************************
*  File: vehicles.c                                    Part of CircleMUD *
*  Usage: Vechicle related code						 *
*									 *
*  All rights reserved.  See license.doc for complete information.	 *
*									 *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University*
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
*  Vehicle.c written by Chris Jacobson <fear@athenet.net>		 *
*************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "handler.h"
#include "spells.h"
#include "feats.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "screen.h"
#include "house.h"
#include "constants.h"


struct obj_data *get_obj_in_list_type(int type, struct obj_data *list);
ACMD(do_look);

#ifndef EXITN
#  define EXITN(room, door)		(world[room].dir_option[door])
#endif

struct obj_data *find_vehicle_by_vnum(int vnum) 
{
  extern struct obj_data * object_list;
  struct obj_data * i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_TYPE(i) == ITEM_VEHICLE)
      if (GET_OBJ_VNUM(i) == vnum)
        return i;
    
  return 0;
}

/* Search the given list for an object type, and return a ptr to that obj*/
struct obj_data *get_obj_in_list_type(int type, struct obj_data *list) 
{
  struct obj_data * i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_TYPE(i) == type)
      return i;

  return NULL;
}

/* Search the player's room, inventory and equipment for a control */
struct obj_data *find_control(struct char_data *ch)
{
  struct obj_data *controls, *obj;
  int    j;

  controls = get_obj_in_list_type(ITEM_CONTROL, world[IN_ROOM(ch)].contents);
  if (!controls)
    for (obj = ch->carrying; obj && !controls; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_TYPE(obj) == ITEM_CONTROL)
        controls = obj;
  if (!controls)
    for (j = 0; j < NUM_WEARS && !controls; j++)
        if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)) && 
          GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_CONTROL)
          controls = GET_EQ(ch, j);
  return controls;
}

/* Drive our vehicle into another vehicle */
 void drive_into_vehicle(struct char_data *ch, struct obj_data *vehicle, char *arg)
 {
  struct obj_data *vehicle_in_out;
  int is_in, is_going_to;
  char   buf[MAX_INPUT_LENGTH]={'\0'};

  if (!*arg) {
    send_to_char(ch, "Drive into what?\r\n");
  } else if (!(vehicle_in_out = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(vehicle)].contents)) ) {
    send_to_char(ch, "Nothing here by that name!\r\n");
  } else if (GET_OBJ_TYPE(vehicle_in_out) != ITEM_VEHICLE) {
    send_to_char(ch, "That's not a vehicle.\r\n");
  } else if (vehicle == vehicle_in_out) {
    send_to_char(ch, "My, we are in a clever mood today, aren't we.\r\n");
  } else {
    is_going_to = real_room(GET_OBJ_VAL(vehicle_in_out, 0));
    if (!IS_SET_AR(ROOM_FLAGS(is_going_to), ROOM_NOVEHICLE)) {
      send_to_char(ch, "That vehicle can't carry other vehicles.");
    } else {
      sprintf(buf, "%s enters %s.\n\r", vehicle->short_description, 
        vehicle_in_out->short_description);
      send_to_room(IN_ROOM(vehicle), "%s", buf);

      obj_from_room(vehicle);
      obj_to_room(vehicle, is_going_to);
      is_in = IN_ROOM(vehicle);
      if (ch->desc != NULL)
        look_at_room(is_in, ch, 0);
      sprintf(buf, "%s enters.\r\n", vehicle->short_description);
      send_to_room(is_in, "%s", buf);
    }
  }
}

/* Drive our vehicle out of another vehicle */
void drive_outof_vehicle(struct char_data *ch, struct obj_data *vehicle)
{
    struct obj_data *hatch, *vehicle_in_out;
    char   buf[MAX_INPUT_LENGTH]={'\0'};

    if ( !(hatch = get_obj_in_list_type(ITEM_HATCH,world[IN_ROOM(vehicle)].contents)) ) 
    {
        send_to_char(ch, "Nowhere to drive out of.\r\n");
    } 
    else if (!(vehicle_in_out = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0)))) 
    {
        send_to_char(ch, "You can't drive out anywhere!\r\n");
    } 
    else 
    {
        sprintf(buf, "%s exits %s.\r\n", vehicle->short_description, vehicle_in_out->short_description);
        send_to_room(IN_ROOM(vehicle), "%s", buf);

        obj_from_room(vehicle);
        obj_to_room(vehicle, IN_ROOM(vehicle_in_out));

        if (ch->desc != NULL)
        {
            look_at_room(IN_ROOM(vehicle), ch, 0);
        }

        sprintf(buf, "%s drives out of %s.\r\n", vehicle->short_description,vehicle_in_out->short_description);
        send_to_room(IN_ROOM(vehicle), "%s", buf);
    }
}

/* Drive out vehicle in a certain direction */
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle, int dir)
{
  char   buf[MAX_INPUT_LENGTH]={'\0'};

  if (!EXIT(vehicle, dir) || EXIT(vehicle, dir)->to_room == NOWHERE) {
        /* But there is no exit that way */
        send_to_char(ch, "Alas, you cannot go that way...\r\n");
      } else if (IS_SET(EXIT(vehicle, dir)->exit_info, EX_CLOSED)) {
        /* But the door is closed */
        if (EXIT(vehicle, dir)->keyword)
          send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(vehicle,           dir)->keyword));
        else
          send_to_char(ch, "It seems to be closed.\r\n");

      } else if (IS_SET_AR(ROOM_FLAGS(EXIT(vehicle,dir)->to_room),ROOM_NOVEHICLE)) {
        /* But the vehicle can't go that way*/
        send_to_char(ch, "The vehicle can't manage that terrain.\r\n");
        } else {
          /* But nothing!  Let's go that way! */
          int was_in, is_in;

          sprintf(buf, "%s leaves %s.\n\r", vehicle->short_description, dirs[dir]);
          send_to_room(IN_ROOM(vehicle), "%s", buf);

          was_in = IN_ROOM(vehicle);
          obj_from_room(vehicle);
          obj_to_room(vehicle, world[was_in].dir_option[dir]->to_room);

          is_in = IN_ROOM(vehicle);

          if (ch->desc != NULL)
      look_at_room(is_in, ch, 0);
          sprintf(buf, "%s enters from the %s.\r\n",
                  vehicle->short_description, dirs[rev_dir[dir]]);
          send_to_room(is_in, "%s", buf);
      }
}

ACMD(do_drive) 
{
  int    dir;
  struct obj_data *vehicle, *controls;

  if (GET_POS(ch) < POS_SLEEPING) {
    send_to_char(ch, "You can't see anything but stars!\r\n");
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see anything, you're blind!\r\n");
  } else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
  } else if (!(controls = find_control(ch) )) {
    send_to_char(ch,"You have no idea how to drive anything here.\r\n");
  } else if (invalid_align(ch, controls) ||
             invalid_class(ch, controls) ||
             invalid_race(ch, controls)) {
    act("You are zapped by $p and instantly step away from it.", FALSE, ch, controls, 0, TO_CHAR);
    act("$n is zapped by $p and instantly steps away from it.", FALSE, ch, controls, 0, TO_ROOM);
  } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0))) ) {
    send_to_char(ch, "You can't find anything to drive.\r\n");
  } else {
    char   arg[MAX_INPUT_LENGTH]={'\0'}, arg2[MAX_INPUT_LENGTH]={'\0'};

    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2); 
    if (!*arg) {
      send_to_char(ch, "Drive, yes, but where?\r\n");
    } else if (is_abbrev(arg, "into")   ||
               is_abbrev(arg, "inside") ||
	       is_abbrev(arg, "onto")    )  {
      /* Driving into another vehicle */
      drive_into_vehicle(ch, vehicle, arg2);
    } else if (is_abbrev(arg, "outside") && !EXIT(vehicle, OUTDIR)) {
      drive_outof_vehicle(ch, vehicle);
    } else if ((dir = search_block(arg, dirs, FALSE)) >= 0 ) {
      /* Drive in a direction... */
      drive_in_direction(ch, vehicle, dir);
    } else {
      send_to_char(ch, "Thats not a valid direction.\r\n");
    }
  }
}
