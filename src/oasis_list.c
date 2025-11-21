/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "shop.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "guild.h"
#include "quest.h"

/******************************************************************************/
/** External Variables                                                       **/
/******************************************************************************/
extern struct shop_data *shop_index;
extern int top_shop;
extern struct index_data **trig_index;
extern int top_of_trigt;
extern struct guild_data *guild_index;
extern int top_guild;
extern int count_guilds(guild_vnum low, guild_vnum high);
extern int count_shops(shop_vnum low, shop_vnum high);

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
void list_rooms(struct char_data *ch  , zone_rnum rnum, room_vnum vmin, room_vnum vmax);
void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin , mob_vnum vmax );
void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin , obj_vnum vmax );
void list_shops(struct char_data *ch  , zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);
void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);
void list_zones(struct char_data *ch);
void print_zone(struct char_data *ch, zone_vnum vnum);
void list_guilds(struct char_data *ch  , zone_rnum rnum, guild_vnum vmin, guild_vnum vmax);


/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list) 
{
    zone_rnum rzone = NOWHERE;
    room_rnum vmin = NOWHERE;
    room_rnum vmax = NOWHERE;
    char smin[MAX_INPUT_LENGTH] = "";
    char smax[MAX_INPUT_LENGTH] = "";

    two_arguments(argument, smin, smax);

    if (subcmd == SCMD_OASIS_ZLIST) 
    { 
/* special case */
        if (*smin && is_number(smin)) 
        {
            print_zone(ch, atoi(smin));
        }
        else
        {
            list_zones(ch);  
        }
        return;
    }

    if (!*smin || *smin == '.') 
    {
        rzone = world[IN_ROOM(ch)].zone;
    } 
    else if (!*smax) 
    {
        rzone = real_zone(atoi(smin));

        if (rzone == NOWHERE) 
        {
            send_to_char(ch, "Sorry, there's no zone with that number\r\n");
            return;
        }
    } 
    else 
    {
        /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
        vmin = atoi(smin);
        vmax = atoi(smax);

        if (vmin > vmax) 
        {
            send_to_char(ch, "List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
            return;
        }
    }

    switch (subcmd) 
    {
        case SCMD_OASIS_RLIST: list_rooms(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_OLIST: list_objects(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_MLIST: list_mobiles(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_TLIST: list_triggers(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_SLIST: list_shops(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_GLIST: list_guilds(ch, rzone, vmin, vmax); break;
        case SCMD_OASIS_QLIST: list_quests(ch, rzone, vmin, vmax); break;
        default: 
        send_to_char(ch, "You can't list that!\r\n");
        mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
    }
}

ACMD(do_oasis_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  char arg[MAX_INPUT_LENGTH]={'\0'};

  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!*arg || !strcmp(arg, ".")) {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  } else {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE) {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
    if (GET_ROOM_VNUM(nr) >= first) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
      if (world[nr].dir_option[j]) {
        to_room = world[nr].dir_option[j]->to_room;
        if (to_room != NOWHERE && (zrnum != world[to_room].zone))
          send_to_char(ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
            zone_table[world[to_room].zone].number,
            zone_table[world[to_room].zone].name,
            GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
      }
      }
    }
  }
}

/******************************************************************************/
/** Helper Functions                                                         **/
/******************************************************************************/


/*
 * List all rooms in a zone.                              
 */                                                                           
void list_rooms(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax)
{
  int i, j, bottom, top, counter = 0;
  /*
   * Expect a minimum / maximum number if the rnum for the zone is NOWHERE. 
   */
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char (ch,
  "Index VNum    Room Name                                Exits\r\n"
  "----- ------- ---------------------------------------- -----\r\n");
  
  for (i = 0; i <= top_of_world; i++) {
    
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((world[i].number >= bottom) && (world[i].number <= top)) {
      counter++;
      
        send_to_char(ch, "%4d) [@g%-5d@n] @c%-40.40s@n %s",
                          counter, world[i].number, world[i].name,
                          world[i].proto_script ? "[TRIG] " : "");

      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (W_EXIT(i, j) == NULL)
          continue;
        if (W_EXIT(i, j)->to_room == NOWHERE)
          continue;
          
        if (world[W_EXIT(i, j)->to_room].zone != world[i].zone) 
          send_to_char(ch, "(@y%d@n)", world[W_EXIT(i, j)->to_room].number);
     
      }
    
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "No rooms found for zone/range specified.\r\n"); 
}

/*
 * List all mobiles in a zone.                              
 */                                                                           
void list_mobiles(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char(ch,
  "Index VNum    Mobile Name                                   Level\r\n"
  "----- ------- --------------------------------------------- -----\r\n");
  
  for (i = 0; i <= top_of_mobt; i++) {
    if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@g%-5d@n] @c%-44.44s @y[%4d]@n%s\r\n",
                   counter, mob_index[i].vnum, mob_proto[i].short_descr, 
                   mob_proto[i].level + mob_proto[i].level_adj + mob_proto[i].race_level, 
                   mob_proto[i].proto_script ? " [TRIG]" : "");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/*
 * List all objects in a zone.                              
 */                                                                           
void list_objects(struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char(ch,
  "Index VNum    Object Name                                  Object Type\r\n"
  "----- ------- -------------------------------------------- ----------------\r\n");
  
  for (i = 0; i <= top_of_objt; i++) {
    if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@g%-5d@n] @c%-44.44s @y[%s]@n%s\r\n",  
                   counter, obj_index[i].vnum, obj_proto[i].short_description,
                   item_types[obj_proto[i].type_flag],
                   obj_proto[i].proto_script ? " [TRIG]" : "");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}


/*
 * List all shops in a zone.                              
 */                                                                           
void list_shops(struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax)
{
  int i, j, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  /****************************************************************************/
  /** Store the header for the shop listing.                                 **/
  /****************************************************************************/
  send_to_char (ch,
  "Index VNum    Shop Room(s)\r\n"
  "----- ------- ---------------------------------------------\r\n");
  
  for (i = 0; i <= top_shop; i++) {
    if (SHOP_NUM(i) >= bottom && SHOP_NUM(i) <= top) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@g%-5d@n]", counter, SHOP_NUM(i));

      /************************************************************************/
      /** Retrieve the list of rooms for this shop.                          **/
      /************************************************************************/
      
      for (j = 0; SHOP_ROOM(i, j) != NOWHERE; j++)
        send_to_char(ch, "%s@c[@y%d@c]@n",
          ((j > 0) && (j % 8 == 0)) ? "\r\n              " : " ", SHOP_ROOM(i, j));

      if (j == 0)
        send_to_char(ch, "@cNone.@n");
      
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/*
 * List all zones in the world (sort of like 'show zones').                              
 */                                                                           
void list_zones(struct char_data *ch)
{
  int i;
  
  send_to_char(ch,
  "VNum  Zone Name                      Builder(s)\r\n"
  "----- ------------------------------ --------------------------------------\r\n");
  
  for (i = 0; i <= top_of_zone_table; i++)
    send_to_char(ch, "[@g%3d@n] @c%-30.30s @y%-1s@n\r\n",
                 zone_table[i].number, zone_table[i].name,
                 zone_table[i].builders ? zone_table[i].builders : "None.");
}



/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(struct char_data *ch, zone_vnum vnum)
{
  zone_rnum rnum;
  int size_rooms, size_objects, size_mobiles, size_quests, i;
  int size_guilds, size_shops;
  room_vnum top, bottom;
  int largest_table;
  
  if ((rnum = real_zone(vnum)) == NOWHERE) {
    send_to_char(ch, "Zone #%d does not exist in the database.\r\n", vnum);
    return;
  }
  
  /****************************************************************************/
  /** Locate the largest of the three, top_of_world, top_of_mobt, or         **/
  /** top_of_objt.                                                           **/
  /****************************************************************************/
  if (top_of_world >= top_of_objt && top_of_world >= top_of_mobt)
    largest_table = top_of_world;
  else if (top_of_objt >= top_of_mobt && top_of_objt >= top_of_world)
    largest_table = top_of_objt;
  else
    largest_table = top_of_mobt;
  
  /****************************************************************************/
  /** Initialize some of the variables.                                      **/
  /****************************************************************************/
  size_rooms   = 0;
  size_objects = 0;
  size_mobiles = 0;
  size_quests  = 0;
  top          = zone_table[rnum].top;
  bottom       = zone_table[rnum].bot;
  size_guilds  = 0;
  size_shops  = 0;
  
  for (i = 0; i <= largest_table; i++) {
    if (i <= top_of_world)
      if (world[i].zone == rnum)
        size_rooms++;
    
    if (i <= top_of_objt)
      if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top)
        size_objects++;
    
    if (i <= top_of_mobt)
      if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top)
        size_mobiles++;

    size_quests = count_quests(bottom, top);
    size_guilds = count_guilds(bottom, top);
    size_shops = count_shops(bottom, top);
  }
  
  /****************************************************************************/
  /** Display all of the zone information at once.                           **/
  /****************************************************************************/
  send_to_char(ch,
    "@gVirtual Number = @c%d\r\n"
    "@gName of zone   = @c%s\r\n"
    "@gBuilders       = @c%s\r\n"
    "@gLifespan       = @c%d\r\n"
    "@gAge            = @c%d\r\n"
    "@gBottom of Zone = @c%d\r\n"
    "@gTop of Zone    = @c%d\r\n"
    "@gReset Mode     = @c%s\r\n"
    "@gSize\r\n"
    "@g   Rooms       = @c%d\r\n"
    "@g   Guilds      = @c%d\r\n"
    "@g   Objects     = @c%d\r\n"
    "@g   Mobiles     = @c%d\r\n"
    "@g   Quests      = @c%d\r\n"
    "@g   Shops       = @c%d@n\r\n",
    zone_table[rnum].number, zone_table[rnum].name,
    zone_table[rnum].builders, zone_table[rnum].lifespan,
    zone_table[rnum].age, zone_table[rnum].bot, zone_table[rnum].top,
    zone_table[rnum].reset_mode ? ((zone_table[rnum].reset_mode == 1) ?
    "Reset when no players are in zone." : "Normal reset.") : "Never reset",
    size_rooms, size_guilds, size_objects, size_mobiles, (int) size_quests, size_shops);
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax)
{
  int i, bottom, top, counter = 0;
  char trgtypes[256]={'\0'};

  /** Expect a minimum / maximum number if the rnum for the zone is NOWHERE. **/
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }


  /** Store the header for the room listing. **/
  send_to_char (ch,
  "Index VNum    Trigger Name                        Type\r\n"
  "----- ------- -------------------------------------------------------\r\n");


  /** Loop through the world and find each room. **/
  for (i = 0; i < top_of_trigt; i++) {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((trig_index[i]->vnum >= bottom) && (trig_index[i]->vnum <= top)) {
      counter++;

      send_to_char(ch, "%4d) [@g%5d@n] @c%-45.45s ",
        counter, trig_index[i]->vnum, trig_index[i]->proto->name);

      if (trig_index[i]->proto->attach_type == OBJ_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "obj @y%s@n\r\n", trgtypes);
      } else if (trig_index[i]->proto->attach_type==WLD_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "wld @y%s@n\r\n", trgtypes);
      } else {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "mob @y%s@n\r\n", trgtypes);
      }

    }
  }

  if (counter == 0)
    send_to_char(ch, "No triggers found for zone #%d\r\n", zone_table[rnum].number);
}
