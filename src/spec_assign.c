/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"


/* external globals */
extern int mini_mud;

/* external functions */
SPECIAL(auto_equip_newbie);
SPECIAL(azimer);
SPECIAL(bacta_merchant);
SPECIAL(bank);
SPECIAL(bounty_contractor);
SPECIAL(buff_mob);
SPECIAL(buy_items);
// SPECIAL(buy_potion);
SPECIAL(cityguard);
SPECIAL(cleric_ao);
SPECIAL(cleric_marduk);
SPECIAL(crafting_quest);
SPECIAL(crafting_station);
SPECIAL(cryogenicist);
SPECIAL(dump);
SPECIAL(dziak);
SPECIAL(emporium);
SPECIAL(enchant_mob);
SPECIAL(enforce_chargen);
SPECIAL(fido);
SPECIAL(guild);
SPECIAL(guild_guard);
SPECIAL(harvest);
SPECIAL(identify_kit);
SPECIAL(identify_mob);
SPECIAL(item_bank);
SPECIAL(item_seller);
SPECIAL(janitor);
SPECIAL(library_large);
SPECIAL(library_medium);
SPECIAL(library_small);
SPECIAL(license_mob);
SPECIAL(lockbox);
SPECIAL(lyrzaxyn);
SPECIAL(mayor);
SPECIAL(mold_seller);
SPECIAL(mount_shop);
SPECIAL(orphan);
SPECIAL(pet_shops);
SPECIAL(player_shop);
SPECIAL(postmaster);
SPECIAL(puff);
SPECIAL(read_rules);
SPECIAL(receptionist);
SPECIAL(repair_mob);
SPECIAL(research);
SPECIAL(ring_of_wishes);
SPECIAL(select_align);
SPECIAL(select_race);
SPECIAL(set_descs);
SPECIAL(set_stats);
SPECIAL(snake);
SPECIAL(start_room);
SPECIAL(thief);
SPECIAL(wizard);


/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
    mob_rnum rnum;

    if ((rnum = real_mobile(mob)) != NOBODY) 
    {
        mob_index[rnum].func = fname;
    }
    else if (!mini_mud)
    {
        log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
    }
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
    obj_rnum rnum;

    if ((rnum = real_object(obj)) != NOTHING)
    {
        obj_index[rnum].func = fname;
    }
    else if (!mini_mud)
    {
        log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
    }
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
    room_rnum rnum;

    if ((rnum = real_room(room)) != NOWHERE)
    {
        world[rnum].func = fname;
    }
    else if (!mini_mud)
    {
        log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
    }
}

void UNASSIGNROOM(room_vnum room)
{
    room_rnum rnum;

    if ((rnum = real_room(room)) != NOWHERE)
    {
        world[rnum].func = NULL;
    }
    else if (!mini_mud)
    {
        log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
    }
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  ASSIGNMOB(3044, mold_seller);

  // if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE) 
  // {
  //   ASSIGNMOB(2427, crafting_quest);
  //   ASSIGNMOB(2428, mold_seller);
  //   ASSIGNMOB(16800, repair_mob);
  // }
}



/* assign special procedures to objects */
void assign_objects(void)
{

    ASSIGNOBJ(199, harvest);
    ASSIGNOBJ(64013, crafting_station);
    ASSIGNOBJ(30195, crafting_station);
    ASSIGNOBJ(30188, ring_of_wishes);
    ASSIGNOBJ(64098, orphan);
    ASSIGNOBJ(64097, lockbox);
    // ASSIGNOBJ(2237, bounty_contractor);
    // ASSIGNOBJ(7024, identify_kit);

}


/* assign special procedures to rooms */
void assign_rooms(void)
{
    room_rnum i;

    switch (CONFIG_CAMPAIGN)
    {

    case CAMPAIGN_FORGOTTEN_REALMS:
        // Character generation
        ASSIGNROOM(30000, enforce_chargen);
        ASSIGNROOM(30001, select_race);
        ASSIGNROOM(30002, select_align);
        ASSIGNROOM(30003, set_stats);
        ASSIGNROOM(30004, set_descs);
        ASSIGNROOM(30015, read_rules);
        ASSIGNROOM(30034, buy_items);
        ASSIGNROOM(30022, library_small); // Character Generation
        break;

    case CAMPAIGN_GOLARION:
        // Character generation
        ASSIGNROOM(30000, enforce_chargen);
        ASSIGNROOM(30001, select_race);
        ASSIGNROOM(30002, select_align);
        ASSIGNROOM(30003, set_stats);
        ASSIGNROOM(30004, set_descs);
        ASSIGNROOM(30015, read_rules);
        ASSIGNROOM(30034, buy_items);
        ASSIGNROOM(30022, library_small); // Character Generation
        break;

    case CAMPAIGN_DRAGONLANCE:


        ASSIGNROOM(30000, enforce_chargen);
        ASSIGNROOM(30001, select_race);
        ASSIGNROOM(30002, select_align);
        ASSIGNROOM(30003, set_stats);
        ASSIGNROOM(30004, set_descs);
        ASSIGNROOM(30015, read_rules);
        ASSIGNROOM(30034, item_seller);
        ASSIGNROOM(30022, library_small); // Character Generation
        ASSIGNROOM(30099, mount_shop);    // Palanthas Stables
        ASSIGNROOM(2340, library_large);  // Palanthas
        ASSIGNROOM(2314, bank);           // Palanthas
        ASSIGNROOM(7067, identify_mob);   // Palanthas
        ASSIGNROOM(1250, emporium);       // Palanthas

        // Palanthas
        ASSIGNROOM(2340, library_large);
        ASSIGNROOM(16800, item_bank);
        ASSIGNROOM(16826, license_mob);

        ASSIGNROOM(59089, player_shop); // Unassigned
        ASSIGNROOM(59091, player_shop); // Unassigned
        ASSIGNROOM(59095, player_shop); // Unassigned
        ASSIGNROOM(59097, player_shop); // Unassigned
        ASSIGNROOM(59101, player_shop); // Unassigned
        ASSIGNROOM(59103, player_shop); // Unassigned
        ASSIGNROOM(59105, player_shop); // Unassigned
        ASSIGNROOM(59108, player_shop); // Unassigned
        ASSIGNROOM(59110, player_shop); // Unassigned
        ASSIGNROOM(59114, player_shop); // Unassigned
        ASSIGNROOM(59116, player_shop); // Unassigned
        ASSIGNROOM(59120, player_shop); // Unassigned
        ASSIGNROOM(59122, player_shop); // Unassigned
        ASSIGNROOM(59124, player_shop); // Unassigned


        break;
    }
    // Player Shops

    if (CONFIG_DTS_ARE_DUMPS)
        for (i = 0; i <= top_of_world; i++)
        {
            if (ROOM_FLAGGED(i, ROOM_DEATH))
            {
                world[i].func = dump;
            }
        }
}
