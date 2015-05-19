#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: spec_procs.c 62 2009-03-25 23:06:34Z gicker $");

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
#include "dg_scripts.h"

// Local Functions
void enter_taxi(struct char_data *ch, int locale, int type, int here);
int get_travel_time(struct char_data *ch, int speed, int locale, int here);
int get_distance(struct char_data *ch, int locale, int here);

// External Functions
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
int is_player_grouped(struct char_data *target, struct char_data *group);

// Local Functions
int valid_shuttle_travel(int here, int i);

char *speeder_locales_dl[][5] = {
  {"palanthas",                      "16801", "50",   "Solamnia", "good alignment starting city", },
  {"vingaard keep",                  "16805", "50",   "Solamnia", "levels 13-20"},
  {"thelgaard keep",                 "16806", "200",  "Solamnia", "levels 21-24"},
  {"caergoth",                       "16807", "250",  "Solamnia", "levels 8-16, boat to Abanasinia"},
  {"solace",                         "16808", "50",   "Abanasinia", "city for neutral or unfactioned people of any alignment"},
  {"que-shu village",                "16812", "50",   "Abanasinia", "level 18 npcs"},  
  {"xak tsaroth",                    "16813", "50",   "Abanasinia", "level 30 npcs, boat to sanction (taman busuk)"},  
  {"fireside tavern",                "16814", "50",   "Abanasinia",  "level 12 npcs"},  
  {"new sea docks",                  "16815", "100",  "Abanasinia",  "boat to solamnia"},  
  {"qualinost",                      "16816", "200",  "Abanasinia", "level 20 npcs"},  
  {"pax tharkas",                    "16817", "200",  "Abanasinia", "halfway between solace and tarsis"},  
  {"tarsis",                         "16818", "250",  "Abanasinia", "levels 18-25"},  
  {"sanction",                       "16819", "50",   "Taman Busuk",  "evil alignment starting city"},  
  {"neraka",                         "16823", "50",   "Taman Busuk", "levels 14-25"},  
  {"city of morning dew",            "16824", "200",  "Taman Busuk", "level 40 npcs"},  
  {"plains of dust",                 "16825", "250",  "Taman Busuk", "near the onyx obelisk epic level zone"},  
//  {"MUD START ROOM",                 "30036", "0",    "Coruscant", "MUD START ROOM"},
//  {"MUD START ROOM",                 "30037", "0",    "Korriban",  "MUD START ROOM"},
  {"always the last item",           "0",     "0",    "Nowhere", "nothing"}
};

char *speeder_locales_sw[][5] = {
  {"republic training center",       "130",   "5",    "Coruscant", "levels 1-4", },
  {"galactic marketplace",           "150",   "5",    "Coruscant", "shops, bank, services"},
  {"black sun district",             "140",   "20",   "Coruscant", "levels 5-8"},
  {"jedi temple",                    "239",   "5",    "Coruscant", "quests, force training"},
  {"coruscant spaceport",            "295",   "20",   "Coruscant", "travel to other planets"},
  {"mos eisley spaceport",           "600",   "50",   "Tatooine",  "travel to other planets"},  
  {"sand people camp delta",         "603",   "50",   "Tatooine",  "levels 9-12"},  
  {"ruined sith temple",             "700",   "100",  "Tatooine",  "levels 14-16"},  
  {"rogue mandalorian bunker",       "764",   "300",  "Tatooine",  "levels 18-22"},  
  {"korriban spaceport",             "907",   "20",   "Korriban",  "levels 14-16"},  
  {"sith temple",                    "930",   "5",    "Korriban",  "quests, force training, levels 1-7, 12-25"},  
  {"desert village",                 "1103",  "5",    "Korriban",  "levels 1-15"},  
  {"khoonda outpost",                "1305",  "250",    "Dantooine",  "shops, bank, services"},  
  {"hunting grounds",                "1400",  "500",  "Dantooine", "levels 25-30"},  
  {"rwookrrorro starport",           "421",   "5",    "Kashyyyk",  "starport"},
  {"kashyyyk markets",               "476",   "5",    "Kashyyyk",  "shops and services"},
  {"upper forest of kashyyyk",       "541",   "20",   "Kashyyyk",  "levels 1-10"},
  {"MUD START ROOM",                 "30036", "0",    "Coruscant", "MUD START ROOM"},
  {"MUD START ROOM",                 "30037", "0",    "Korriban",  "MUD START ROOM"},
  {"always the last item",           "0",     "0",    "Nowhere", "nothing"}
};

char *shuttle_locales_dl[][7] = {
  {"Solamnia ",                      "16804", "500",   "Whitestone Council", "home of the knights of solamnia & forces of whitestone", "380", "155"},
  {"Taman Busuk",                    "16822",   "500",      "Dragonarmies",     "home of the dragonarmies & knights of takhisis", "575", "225"},
  {"Abanasinia",                     "16811", "500",      "Free People of Ansalon",  "home of the qualinesti, tarsis, solace and plainsmen tribes", "342", "410"},
//  {"MUD START ROOM",                 "30038", "0",     "Neutral Space",  "MUD START ROOM", "150", "150"},
  {"always the last item",           "0",     "0",     "Nowhere", "nothing", "0", "0"}
};

char *shuttle_locales_sw[][7] = {
  {"Coruscant",                      "310",   "50",    "Republic Space", "republic starting area", "75", "185"},
  {"Korriban",                       "901",   "50",    "Sith Space",     "sith/empire starting area", "55", "320"},
  {"Tatooine",                       "602",   "100",   "Neutral Space",  "levels 9-12", "185", "190"},
  {"Dantooine",                      "1300",  "250",   "Neutral Space",  "levels 25-30", "40", "260"},
  {"Kashyyyk",                       "400",   "50",    "Neutral Space",  "levels 1-30",  "105", "243"},
  {"Czerka Space Station",           "1700",  "50",    "Neutral Space",  "auction house, social center", "65", "268"},
  {"MUD START ROOM",                 "30038", "0",     "Neutral Space",  "MUD START ROOM", "150", "150"},
  {"always the last item",           "0",     "0",     "Nowhere", "nothing", "0", "0"}
};

ACMD(do_speeder) {


  if (subcmd == SCMD_SPEEDER) {
    send_to_char(ch, "%s aren't implemented yet.  You'll have to use the %s command for now.\r\n", "Caravans", "carriage");
    return;
  }

  // taxi command

  skip_spaces(&argument);

  int i = 0;
  sbyte found = FALSE;
  while (atoi((speeder_locales_dl)[i][1]) != 0) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1])) {
      found = TRUE;
      break; 
    }
    i++;
  }

  if (!found) {
    send_to_char(ch, "You are not at a valid %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "speeder pad" : "carriage stand");
    return;
  }

  int here = i;

  if (!*argument) {
    found = FALSE;
    i = 0;
    send_to_char(ch, "Available %s Destinations:\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "Taxi" : "Carriage");
    send_to_char(ch, "@Y%-30s %4s (%s)\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "Taxi Destination:" : "Carriage Destination", "Cost", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++) 
      send_to_char(ch, "~");
    send_to_char(ch, "@n\r\n");
    while (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) != 0) {
      if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) == 30036) {i++; continue; }
      if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) == 30037) {i++; continue; }
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) && ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][3] == ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][3]) {
        found = TRUE;
        send_to_char(ch, "%-30s %4s (%s)\r\n", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][0],  ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2], ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][4]);
      }
      i++;
    }

    if (found) {
      send_to_char(ch, "\r\nTo take a %s, type @Y%s <name of destination>@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage");
      return;
    } else {
      send_to_char(ch, "There are no available destinations from this %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi pad" : "carriage stand");
    }
    return;
  } else {
    i = 0;
    found = FALSE;
    while (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][1]) && ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][3] == ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][3]) {
//        send_to_char(ch, "%s\r\n", argument);
//        send_to_char(ch, "%s\r\n", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][0]);
        if (is_abbrev(argument, ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][0])) {
          found = TRUE;
          if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][1]) != 30036 && atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][1]) != 30037) {
          if (GET_GOLD(ch) < atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2])) {
            if (GET_BANK_GOLD(ch) < atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2])) {
              send_to_char(ch, "A friendly passerby pays for your %s, as you don't have enough %s on hand or in your bank.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
            } else {
              send_to_char(ch, "You %s to withdraw the fee of %s %s from your bank account.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "swipe your credstick" : "give a bank note", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2], (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
              GET_BANK_GOLD(ch) -= atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2]);
            }
          } else {
              send_to_char(ch, "You %s to withdraw the fee of %s %s from your bank account.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "swipe your credstick" : "give a bank note", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2], (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
            GET_GOLD(ch) -= atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[i][2]);
          }
          }
          enter_taxi(ch, i, TRAVEL_TAXI, here);
          return;
        }
      }
      i++;
    }
    if (!found) {
      send_to_char(ch, "There is no %s destination %s this %s by that name.  Type @Y%s@n by itself to see a list of destinations.\r\n",
	  (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "on" : "in",
	  (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "planet" : "nation", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage"
	  );
      return;
    }
  }
}

ACMD(do_shuttle) {


  // shuttle command

  skip_spaces(&argument);

  int i = 0;
  sbyte found = FALSE;
  while (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) != 0) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1])) {
      found = TRUE;
      break; 
    }
    i++;
  }

  if (!found) {
    send_to_char(ch, "You are not at a valid %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "spaceport" : "airship tower");
    return;
  }

  int here = i;

  if (!*argument) {
    found = FALSE;
    i = 0;
    send_to_char(ch, "Available %s Destinations:\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "Shuttle" : "Airship");
    send_to_char(ch, "@Y%-30s %4s %10s %10s (%s)\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "Shuttle Destination:" : "Airship Destination:", "Cost", "Distance", "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++) 
      send_to_char(ch, "~");
    send_to_char(ch, "@n\r\n");
    while (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) != 0) {
      if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) == 30036) {i++; continue; }
      if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) == 30037) {i++; continue; }
      if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) == 30038) {i++; continue; }
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) && valid_shuttle_travel(here, i)) {
        found = TRUE;
        send_to_char(ch, "%-30s %4s %10d %10d (%s)\r\n", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][0],  ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2], get_distance(ch, i, here), get_travel_time(ch, 10, i, here), ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][4]);
      }
      i++;
    }

    if (found) {
      send_to_char(ch, "\r\nTo take %s, type @Y%s <name of destination>@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "a shuttle" : "an airship", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship");
      return;
    } else {
      send_to_char(ch, "There are no available destinations from this %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "spaceport" : "airship tower");
    }
    return;
  } else {
    i = 0;
    found = FALSE;
    while (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1]) && valid_shuttle_travel(here, i)) {
        if (is_abbrev(argument, ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][0])) {
          found = TRUE;
          if (atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][1]) != 30036 && atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][1]) != 30037 && atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[here][1]) != 30038) {
          if (GET_GOLD(ch) < atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2])) {
            if (GET_BANK_GOLD(ch) < atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2])) {
              send_to_char(ch, "A friendly passerby pays for your %s, as you don't have enough %s on hand or in your bank.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
            } else {
              send_to_char(ch, "You %s to withdraw the fee of %s %s from your bank account.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "swipe your credstick" : "give a bank note", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2], (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
              GET_BANK_GOLD(ch) -= atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2]);
            }
          } else {
              send_to_char(ch, "You %s to withdraw the fee of %s %s from your bank account.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "swipe your credstick" : "give a bank note", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2], (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "credits" : "gold");
            GET_GOLD(ch) -= atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][2]);
          }
          }
          room_rnum to_room = NOWHERE;
          if ((to_room = find_target_room(ch, ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[i][1])) == NOWHERE) {
            send_to_char(ch, "There is an error with that destination.  Please report on the forums.\r\n");
            return;
          }
          enter_taxi(ch, i, TRAVEL_SHUTTLE, here);
/*
          char_from_room(ch);
          char_to_room(ch, to_room);
          entry_memory_mtrigger(ch);
          greet_mtrigger(ch, -1);
          greet_memory_mtrigger(ch);
*/
          return;
        }
      }
      i++;
    }
    if (!found) {
      send_to_char(ch, "There is no %s destination by that name.  Type @Y%s@n by itself to see a list of destinations.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" :"airship", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship");
      return;
    }
  }
}

int valid_shuttle_travel(int here, int i)
{
  if (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) {
	  if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Republic Space")) {
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Neutral Space"))
		  return TRUE;
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Republic Space")) 
		  return TRUE;
	  }
	  else if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Sith Space")) {
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Neutral Space"))
		  return TRUE;
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Sith Space")) 
		  return TRUE;
	  } else {
		return TRUE;
	  }
  } else if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE) {
	if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Solamnia")) {
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Abanasinia"))
		  return TRUE;
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Abanasinia")) 
		  return TRUE;
	  }
	  else if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Taman Busuk")) {
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Abanasinia"))
		  return TRUE;
		if (!strcmp(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][3], "Taman Busuk")) 
		  return TRUE;
	  } else {
		return TRUE;
	  }
  }
   return FALSE;
}

void enter_taxi(struct char_data *ch, int locale, int type, int here)
{
  int cnt = 0, found = FALSE;

  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (world[cnt].number < 64000 || world[cnt].number > 64099)
      continue;
    if (world[cnt].people)
      continue;
    found = FALSE;
    break;
  }

  room_rnum to_room = NOWHERE;
  if ((to_room = find_target_room(ch, (type == TRAVEL_SHUTTLE || type == TRAVEL_STARSHIP) ? ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][1] : ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[locale][1])) == NOWHERE) {
    send_to_char(ch, "There is an error with that destination.  Please report on the forums.\r\n");
    return;
  }
  room_rnum taxi = cnt;

  if (taxi == NOWHERE) {
    if (type != TRAVEL_STARSHIP)
      send_to_char(ch, "There are no %s available currently.\r\n", type == TRAVEL_SHUTTLE ? "shuttles" :"taxis");
    else
      send_to_char(ch, "%s", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "The docking lanes are clogged with traffic right now, you will have to wait until the traffic subsides to take off.\r\n" : "Some mechanical errors are preventing you from being able to leave right now. Try again in a few moments.\r\n");
    return;
  }

  struct follow_type *f = NULL;;
  struct char_data *tch = NULL;

  for (f = ch->followers; f; f = f->next) {
    tch = f->follower;
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (!is_player_grouped(ch, tch))
      continue;
    if (FIGHTING(tch))
      continue;
    if (GET_POS(tch) < POS_STANDING)
      continue;
    if (type == TRAVEL_TAXI) {
      act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n hops into a taxi and speeds off into the horizon." : "$n boards a carriage which heads off into the distance.",
           false, tch, 0, 0, TO_ROOM);
      send_to_char(tch, "Your group leader ushers you into a nearby %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage");
      send_to_char(tch, "You hop into a %s and %s off towards the %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "speed" : "head", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[locale][0]);
    } else if (type == TRAVEL_SHUTTLE) {
      act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n boards a shuttle which promptly takes off into the atmosphere." : 
	  "$n boards an airship which flies off into the skyline.",
           false, tch, 0, 0, TO_ROOM);
      send_to_char(tch, "Your group leader ushers you into a nearby %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship");
      send_to_char(tch, "You board the %s and %s, %s towards %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "launch" : "take off", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "entering hyperspace" : "flying off into the skyline", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][0]);
    } else if (type == TRAVEL_STARSHIP) {
//      sprintf(buf, "$n boards a %s which promptly takes off into the atmosphere.", ship_types[ch->ship1->ship_type][1]);   
//      act(buf, false, tch, 0, 0, TO_ROOM);
//      send_to_char(tch, "Your group leader ushers you into $s docked %s.\r\n", ship_types[ch->ship1->ship_type][1]);
//      send_to_char(tch, "You board $n's %s and launch, entering hyperspace towards %s.\r\n\r\n", ship_types[ch->ship1->ship_type][1], ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][0]);
    }
    char_from_room(tch);
    char_to_room(tch, taxi);
    tch->destination = to_room;
    if (type == TRAVEL_STARSHIP)
      tch->travel_timer = get_travel_time(tch, tch->ship1->ship_speed, locale, here);
    else if (type == TRAVEL_SHUTTLE)
      tch->travel_timer = get_travel_time(tch, 10, locale, here);
    else
      tch->travel_timer = 30;
    tch->travel_type = type;
    tch->travel_locale = locale;
    look_at_room(taxi, tch, 0);
    entry_memory_mtrigger(tch);
    greet_mtrigger(tch, -1);
    greet_memory_mtrigger(tch);
    save_char(tch);
  }


  if (type == TRAVEL_TAXI) {
	  act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n hops into a taxi and speeds off into the horizon." : "$n boards a carriage which heads off into the distance.",
		   false, ch, 0, 0, TO_ROOM);
	  send_to_char(ch, "Your group leader ushers you into a nearby %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage");
	  send_to_char(ch, "You hop into a %s and %s off towards the %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "speed" : "head", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[locale][0]);
  } else if (type == TRAVEL_SHUTTLE) {
	  act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n boards a shuttle which promptly takes off into the atmosphere." : 
	  "$n boards an airship which flies off into the skyline.",
		   false, ch, 0, 0, TO_ROOM);
	  send_to_char(ch, "Your group leader ushers you into a nearby %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship");
	  send_to_char(ch, "You board the %s and %s, %s towards %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "launch" : "take off", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "entering hyperspace" : "flying off into the skyline", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][0]);
  } else if (type == TRAVEL_STARSHIP) {
//	  sprintf(buf, "$n boards a %s which promptly takes off into the atmosphere.", ship_types[ch->ship1->ship_type][1]);   
//	  act(buf, false, ch, 0, 0, TO_ROOM);
//	  send_to_char(ch, "Your group leader ushers you into $s docked %s.\r\n", ship_types[ch->ship1->ship_type][1]);
//	  send_to_char(ch, "You board $n's %s and launch, entering hyperspace towards %s.\r\n\r\n", ship_types[ch->ship1->ship_type][1], ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][0]);
  }
  char_from_room(ch);
  char_to_room(ch, taxi);
  ch->destination = to_room;
  if (type == TRAVEL_STARSHIP)
    ch->travel_timer = get_travel_time(ch, ch->ship1->ship_speed, locale, here);
  else if (type == TRAVEL_SHUTTLE)
    ch->travel_timer = get_travel_time(ch, 10, locale, here);
  else
    ch->travel_timer = 30;
  ch->travel_type = type;
  ch->travel_locale = locale;
  look_at_room(taxi, ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  save_char(ch);
}

void travel_tickdown(void)
{

  struct char_data *ch = NULL;
  room_rnum to_room = NOWHERE;

  for (ch = character_list; ch; ch = ch->next) {

    if (IS_NPC(ch) || !ch->desc)
      continue;

    if (world[IN_ROOM(ch)].number < 64000 || world[IN_ROOM(ch)].number > 64099)
      continue;

    if (ch->destination == 0 || ch->destination == NOWHERE) {
      if ((to_room = find_target_room(ch, strdup("30000"))) == NOWHERE) {
        send_to_char(ch, "You are stuck in the %s due to a system error.  You will need to recall.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage");
        continue;
      }
      char_from_room(ch);
      char_to_room(ch, to_room);
      look_at_room(to_room, ch, 0);
      entry_memory_mtrigger(ch);
      greet_mtrigger(ch, -1);
      greet_memory_mtrigger(ch);
      save_char(ch);      

      continue;
    } else {
      ch->travel_timer--;
      if (ch->travel_timer < 1) {
        if ((to_room = find_target_room(ch, (ch->travel_type == TRAVEL_SHUTTLE || ch->travel_type == TRAVEL_STARSHIP) ? ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[ch->travel_locale][1] 
             :((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[ch->travel_locale][1])) == NOWHERE) {
          if ((to_room = find_target_room(ch, strdup("30000"))) == NOWHERE) {
            send_to_char(ch, "You are stuck in the %s due to a system error.  You will need to recall.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage");
            continue;
          }
          char_from_room(ch);
          char_to_room(ch, to_room);
          look_at_room(to_room, ch, 0);
          entry_memory_mtrigger(ch);
          greet_mtrigger(ch, -1);
          greet_memory_mtrigger(ch);
          save_char(ch);      
        }
        char_from_room(ch);
        char_to_room(ch, to_room);
        look_at_room(to_room, ch, 0);
        entry_memory_mtrigger(ch);
        greet_mtrigger(ch, -1);
        greet_memory_mtrigger(ch);
        save_char(ch);      
        if (ch->travel_type == TRAVEL_TAXI) {
          act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n hops out of a taxi speeder that lands on the pad." : "$n disembarks a horse-drawn carriage that grinds to a halt before you.", false, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You hop out of your %s arriving at the %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "taxi" : "carriage", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? speeder_locales_sw : speeder_locales_dl)[ch->travel_locale][0]);
        }
        else if (ch->travel_type == TRAVEL_TAXI) {
          act((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "$n disembarks a shuttle that just landed here." : "$n disembarks an airship that just landed here.", false, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You disembark your %s arriving at %s.\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? "shuttle" : "airship", ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[ch->travel_locale][0]);
        }
        else if (ch->travel_type == TRAVEL_STARSHIP) {
//          sprintf(sbuf1, "$n disembarks a %s that just landed here.", ch->master ? ship_types[ch->master->ship1->ship_type][1] : ship_types[ch->ship1->ship_type][1]);
//          act(sbuf1, false, ch, 0, 0, TO_ROOM);
//          send_to_char(ch, "You disembark a %s arriving at %s.\r\n\r\n", ch->master ? ship_types[ch->master->ship1->ship_type][1] : ship_types[ch->ship1->ship_type][1], ((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[ch->travel_locale][0]);
        }
        ch->destination = NOWHERE;
        ch->travel_timer = 0;
        ch->travel_type = 0;
        ch->travel_locale = 0;
      }
      continue;
    }
  }
}

int get_distance(struct char_data *ch, int locale, int here)
{
  int xf, xt, yf, yt;
  xf = atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][5]);
  xt = atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][5]);
  yf = atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[here][6]);
  yt = atoi(((CONFIG_CAMPAIGN == CAMPAIGN_STAR_WARS) ? shuttle_locales_sw : shuttle_locales_dl)[locale][6]);

  int dx, dy;

  if (xf > xt)
    dx = xf - xt;
  else
    dx = xt - xf;

  if (yf > yt)
    dy = yf - yt;
  else
    dy = yt - yf;

  int distance = dx + dy;

  return distance;
}

int get_travel_time(struct char_data *ch, int speed, int locale, int here)
{
  int distance = get_distance(ch, locale, here);

  distance *= 10;

  if (speed == 0) speed = 10;

  distance /= speed;

  return distance;
}
