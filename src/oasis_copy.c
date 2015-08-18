/******************************************************************************/
/** OasisOLC - InGame OLC Copying                                      v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "spells.h"
#include "feats.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "genolc.h"
#include "genzon.h"
#include "genwld.h"
#include "oasis.h"
#include "improved-edit.h"
#include "constants.h"


/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
ACMD(do_dig);
ACMD(do_room_copy);
room_vnum redit_find_new_vnum(zone_rnum zone);
int buildwalk(struct char_data *ch, int dir);

void room_copy_existing(int source_num, int real_num);


/******************************************************************************/
/** Commands                                                                 **/
/******************************************************************************/
ACMD(do_dig)
{
  char sdir[MAX_INPUT_LENGTH]={'\0'}, sroom[MAX_INPUT_LENGTH]={'\0'}, *new_room_name;
  room_vnum rvnum = NOWHERE;
  room_rnum rrnum = NOWHERE;
  zone_rnum zone;
  int dir = 0, rawvnum;
  struct descriptor_data *d = ch->desc; /* will save us some typing */
  
  /* Grab the room's name (if available). */
  new_room_name = two_arguments(argument, sdir, sroom);
  skip_spaces(&new_room_name);
  
  /* Can't dig if we don't know where to go. */
  if (!*sdir || !*sroom) {
    send_to_char(ch, "Format: dig <direction> <room> - to create an exit\r\n"
                     "        dig <direction> -1     - to delete an exit\r\n");
    return;
  }

  rawvnum = atoi(sroom);
  if (rawvnum == -1)
    rvnum = NOWHERE;
  else
    rvnum = (room_vnum)rawvnum;
  rrnum = real_room(rvnum);  
  if ((dir = search_block(sdir, abbr_dirs, FALSE)) < 0)
  dir = search_block(sdir, dirs, FALSE);
  zone = world[IN_ROOM(ch)].zone;

  if (dir < 0) {
    send_to_char(ch, "Can not create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == NOWHERE) || !can_edit_zone(ch, zone)) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    return;
  }
  /*
   * Lets not allow digging to limbo. 
   * After all, it'd just get us more errors on 'show errors'
   */
  if (rvnum == 0) {
   send_to_char(ch, "The target exists, but you can't dig to limbo!\r\n");
   return;
  }
  /*
   * target room == -1 removes the exit 
   */
  if (rvnum == NOTHING) {
    if (W_EXIT(IN_ROOM(ch), dir)) {
      /* free the old pointers, if any */
      if (W_EXIT(IN_ROOM(ch), dir)->general_description)
        free(W_EXIT(IN_ROOM(ch), dir)->general_description);
      if (W_EXIT(IN_ROOM(ch), dir)->keyword)
        free(W_EXIT(IN_ROOM(ch), dir)->keyword);
      free(W_EXIT(IN_ROOM(ch), dir));
      W_EXIT(IN_ROOM(ch), dir) = NULL;
      add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
      send_to_char(ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n", dirs[dir]);
    return;
  }  
  /*
   * Can't dig in a direction, if it's already a door. 
   */
  if (W_EXIT(IN_ROOM(ch), dir)) {
      send_to_char(ch, "There already is an exit to the %s.\r\n", dirs[dir]);
      return;
  }
  
  /* Make sure that the builder has access to the zone he's linking to. */
  zone = real_zone_by_thing(rvnum);  
  if (zone == NOWHERE) {
    send_to_char(ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(ch, zone)) {
    send_to_char(ch, "You do not have permission to edit room #%d.\r\n", rvnum);
    return;
  }
  /*
   * Now we know the builder is allowed to make the link 
   */
  /* If the room doesn't exist, create it.*/
  if (rrnum == NOWHERE) {
    /*
     * Give the descriptor an olc struct.
     * This way we can let redit_save_internally handle the room adding.
     */
    if (d->olc) {
      mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone;
    OLC_NUM(d) = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);
    
    
    /* Copy the room's name. */
    if (*new_room_name)
     OLC_ROOM(d)->name = strdup(new_room_name);
    else
     OLC_ROOM(d)->name = strdup("An unfinished room");
    
    /* Copy the room's description.*/
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->zone = OLC_ZNUM(d);
    OLC_ROOM(d)->number = NOWHERE;
    
    /*
     * Save the new room to memory.
     * redit_save_internally handles adding the room in the right place, etc.
     */
    redit_save_internally(d);
    OLC_VAL(d) = 0;
    
    send_to_char(ch, "New room (%d) created.\r\n", rvnum);
    cleanup_olc(d, CLEANUP_STRUCTS);
    /* 
     * update rrnum to the correct room rnum after adding the room 
     */
    rrnum = real_room(rvnum);
  }

  /*
   * Now dig.
   */
  CREATE(W_EXIT(IN_ROOM(ch), dir), struct room_direction_data, 1);
  W_EXIT(IN_ROOM(ch), dir)->general_description = NULL;
  W_EXIT(IN_ROOM(ch), dir)->keyword = NULL;
  W_EXIT(IN_ROOM(ch), dir)->to_room = rrnum;
  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
  
  send_to_char(ch, "You make an exit %s to room %d (%s).\r\n", 
                   dirs[dir], rvnum, world[rrnum].name);

  /* 
   * check if we can dig from there to here. 
   */
  if (W_EXIT(rrnum, rev_dir[dir])) 
    send_to_char(ch, "Can not dig from %d to here. The target room already has an exit to the %s.\r\n",
                     rvnum, dirs[rev_dir[dir]]);
  else {
    CREATE(W_EXIT(rrnum, rev_dir[dir]), struct room_direction_data, 1);
    W_EXIT(rrnum, rev_dir[dir])->general_description = NULL;
    W_EXIT(rrnum, rev_dir[dir])->keyword = NULL;
    W_EXIT(rrnum, rev_dir[dir])->to_room = IN_ROOM(ch);
    add_to_save_list(zone_table[world[rrnum].zone].number, SL_WLD);
  }
}

ACMD(do_room_copy)
{
   struct descriptor_data *d;
   struct room_data *room_src, *room_dst;
   int room_num, buf_num;
   zone_rnum dst_zone;
   char buf[MAX_INPUT_LENGTH]={'\0'};
     
   one_argument(argument, buf);
   
   if (!*buf) {
     send_to_char(ch, "Usage: rclone <target room>\r\n");
     return;
   }
   buf_num = atoi(buf);

   if ((dst_zone = real_zone_by_thing(buf_num)) == NOWHERE) {
     send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
     return;
   }
    
   if (!can_edit_zone(ch, dst_zone) ||
       !can_edit_zone(ch, world[IN_ROOM(ch)].zone) ) {
     send_to_char(ch, "You may only copy rooms within your designated zone(s)!\r\n");
     return;
   }
      
   for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_REDIT) {
       if (d->olc && OLC_NUM(d) == buf_num) {
         send_to_char(ch, "That room is currently being edited by %s.\r\n",
             PERS(d->character, ch));
         return;
       }
     }
   }

   if (real_room(buf_num) == NOWHERE) {
     send_to_char(ch, "Target room does not exist.\r\n");
     return;
   }
  
   room_src = &world[IN_ROOM(ch)];
   room_dst = &world[real_room(buf_num)];
   room_num = room_src->number;
   dst_zone = room_src->zone;

   send_to_char(ch, "Cloning room....\r\n");
   room_copy_existing(IN_ROOM(ch), real_room(buf_num));
   room_src->number = room_num;
   room_src->zone = dst_zone;

  add_to_save_list(real_zone_by_thing(room_num), SL_WLD);
  redit_save_to_disk(real_zone_by_thing(room_num));
  send_to_char(ch, "Room %d cloned to present room.\r\nAll Done.\r\n", buf_num);
}


/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
room_vnum redit_find_new_vnum(zone_rnum zone) 
{
  room_vnum vnum = genolc_zone_bottom(zone);
  room_rnum rnum = real_room(vnum);

  if (rnum == NOWHERE) 
    return NOWHERE;

  for(;;) {
    if (vnum > zone_table[zone].top)
      return(NOWHERE);
    if (rnum > top_of_world || world[rnum].number > vnum)
      break;
    rnum++;
    vnum++;
  }
  return(vnum);
}

int buildwalk(struct char_data *ch, int dir)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  room_vnum vnum;
  room_rnum rnum;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK) &&
      GET_ADMLEVEL(ch) >= ADMLVL_BUILDER) {

    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You do not have build permissions in this zone.\r\n");
    } else if ((vnum = redit_find_new_vnum(world[IN_ROOM(ch)].zone)) == NOWHERE)
      send_to_char(ch, "No free vnums are available in this zone!\r\n");
    else {
      struct descriptor_data *d = ch->desc;
      /*
       * Give the descriptor an olc struct.
       * This way we can let redit_save_internally handle the room adding.
       */
      if (d->olc) {
        mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = world[IN_ROOM(ch)].zone;
      OLC_NUM(d) = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

      OLC_ROOM(d)->name = strdup("New BuildWalk Room");

      sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
      OLC_ROOM(d)->description = strdup(buf);
      OLC_ROOM(d)->zone = OLC_ZNUM(d);
      OLC_ROOM(d)->number = NOWHERE;

      /*
       * Save the new room to memory.
       * redit_save_internally handles adding the room in the right place, etc.
       */
      redit_save_internally(d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      rnum = real_room(vnum);
      CREATE(EXIT(ch, dir), struct room_direction_data, 1);
      EXIT(ch, dir)->to_room = rnum;
      CREATE(world[rnum].dir_option[rev_dir[dir]], struct room_direction_data, 1);
      world[rnum].dir_option[rev_dir[dir]]->to_room = IN_ROOM(ch);

      /* Report room creation to user */
      send_to_char(ch, "@yRoom #%d created by BuildWalk.@n\r\n", vnum);
      cleanup_olc(d, CLEANUP_STRUCTS);

      return (1);

    }
  }

  return(0);
}

void room_copy_existing(int source_num, int real_num)
{
  struct room_data *room;

  room = &world[source_num];

  /*
   * Allocate space for all strings.
   */
  room->name = str_udup(world[real_num].name);
  room->description = str_udup(world[real_num].description);

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

}
