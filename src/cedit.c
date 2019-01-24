/************************************************************************
 * OasisOLC - Game configuration / cedit.c                   v2.0	*
 * Copyright 2002-2003 Kip Potter   (kip_potter@hotmail.com)            *
 * A graphical in-game game configuration utility for OasisOLC.         *
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modules.h"


/******************************************************************************/
/** External Functions                                                       **/
/******************************************************************************/
void free_config(struct config_data *data);

/******************************************************************************/
/** Internal Macros                                                          **/
/******************************************************************************/
#define NO 0
#define YES 1

#define CHECK_VAR(var)  ((var == YES) ? "Yes" : "No")
#define TOGGLE_VAR(var)	if (var == YES) { var = NO; } else { var = YES; }


/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
void cedit_disp_menu(struct descriptor_data *d);
void cedit_setup(struct descriptor_data *d);
void cedit_save_to_disk( void );
int  save_config( IDXTYPE nowhere );
void reassign_rooms(void);

/******************************************************************************/
/** Routines                                                                 **/
/******************************************************************************/
ACMD(do_oasis_cedit)
{
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH]={'\0'};
  
  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  one_argument(argument, buf1);
  
  if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && !ADM_FLAGGED(ch, ADM_CEDIT)) {
    send_to_char(ch, "You can't modify the game configuration.\r\n");
    return;
  }
  
  d = ch->desc;
  
  if (!*buf1) {
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZONE(d) = 0;
    cedit_setup(d);
    STATE(d) = CON_CEDIT;
    act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
    
    mudlog(BRF, ADMLVL_IMMORT, TRUE, 
      "OLC: %s starts editing the game configuration.", GET_NAME(ch));
    return;
  } else if (str_cmp("save", buf1) != 0) {
    send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
    return;
  }
  
  send_to_char(ch, "Saving the game configuration.\r\n");
  mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, 
    "OLC: %s saves the game configuration.", GET_NAME(ch));
  
  cedit_save_to_disk();
}

/*-------------------------------------------------------------------*/

void cedit_setup(struct descriptor_data *d)
{
  /****************************************************************************/
  /** Create the config_data struct.                                         **/
  /****************************************************************************/
  CREATE(OLC_CONFIG(d), struct config_data, 1);
  
  /****************************************************************************/
  /** Copy the current configuration from the config_info to this one.       **/
  /****************************************************************************/
  /****************************************************************************/
  /** Copy the game play options from the configuration info struct.         **/
  /****************************************************************************/
  OLC_CONFIG(d)->play.pk_allowed          = CONFIG_PK_ALLOWED;
  OLC_CONFIG(d)->play.pt_allowed          = CONFIG_PT_ALLOWED;
  OLC_CONFIG(d)->play.level_can_shout     = CONFIG_LEVEL_CAN_SHOUT;
  OLC_CONFIG(d)->play.holler_move_cost    = CONFIG_HOLLER_MOVE_COST;
  OLC_CONFIG(d)->play.tunnel_size         = CONFIG_TUNNEL_SIZE;
  OLC_CONFIG(d)->play.max_exp_gain        = CONFIG_MAX_EXP_GAIN;
  OLC_CONFIG(d)->play.max_exp_loss        = CONFIG_MAX_EXP_LOSS;
  OLC_CONFIG(d)->play.max_npc_corpse_time = CONFIG_MAX_NPC_CORPSE_TIME;
  OLC_CONFIG(d)->play.max_pc_corpse_time  = CONFIG_MAX_PC_CORPSE_TIME;
  OLC_CONFIG(d)->play.idle_void           = CONFIG_IDLE_VOID;
  OLC_CONFIG(d)->play.idle_rent_time      = CONFIG_IDLE_RENT_TIME;
  OLC_CONFIG(d)->play.idle_max_level      = CONFIG_IDLE_MAX_LEVEL;
  OLC_CONFIG(d)->play.dts_are_dumps       = CONFIG_DTS_ARE_DUMPS;
  OLC_CONFIG(d)->play.load_into_inventory = CONFIG_LOAD_INVENTORY;
  OLC_CONFIG(d)->play.track_through_doors = CONFIG_TRACK_T_DOORS;
  OLC_CONFIG(d)->play.level_cap           = CONFIG_LEVEL_CAP;
  OLC_CONFIG(d)->play.stack_mobs	  = CONFIG_STACK_MOBS;
  OLC_CONFIG(d)->play.stack_objs	  = CONFIG_STACK_OBJS;
  OLC_CONFIG(d)->play.mob_fighting        = CONFIG_MOB_FIGHTING;
  OLC_CONFIG(d)->play.disp_closed_doors   = CONFIG_DISP_CLOSED_DOORS;
  OLC_CONFIG(d)->play.campaign            = CONFIG_CAMPAIGN;
  OLC_CONFIG(d)->play.map_option          = CONFIG_MAP;
  OLC_CONFIG(d)->play.map_size            = CONFIG_MAP_SIZE;
  OLC_CONFIG(d)->play.minimap_size        = CONFIG_MINIMAP_SIZE;
  OLC_CONFIG(d)->play.reroll_player       = CONFIG_REROLL_PLAYER_CREATION;
  OLC_CONFIG(d)->play.initial_points	  = CONFIG_INITIAL_POINTS_POOL;
  OLC_CONFIG(d)->play.enable_compression  = CONFIG_ENABLE_COMPRESSION;
  OLC_CONFIG(d)->play.enable_languages    = CONFIG_ENABLE_LANGUAGES;
  OLC_CONFIG(d)->play.all_items_unique    = CONFIG_ALL_ITEMS_UNIQUE;
  OLC_CONFIG(d)->play.exp_multiplier      = CONFIG_EXP_MULTIPLIER;
  
  /****************************************************************************/
  /** Crash Saves                                                            **/
  /****************************************************************************/
  OLC_CONFIG(d)->csd.free_rent            = CONFIG_FREE_RENT;
  OLC_CONFIG(d)->csd.max_obj_save         = CONFIG_MAX_OBJ_SAVE;
  OLC_CONFIG(d)->csd.min_rent_cost        = CONFIG_MIN_RENT_COST;
  OLC_CONFIG(d)->csd.auto_save            = CONFIG_AUTO_SAVE;
  OLC_CONFIG(d)->csd.autosave_time        = CONFIG_AUTOSAVE_TIME;
  OLC_CONFIG(d)->csd.crash_file_timeout   = CONFIG_CRASH_TIMEOUT;
  OLC_CONFIG(d)->csd.rent_file_timeout    = CONFIG_RENT_TIMEOUT;
  
  /****************************************************************************/
  /** Room Numbers                                                           **/
  /****************************************************************************/
  OLC_CONFIG(d)->room_nums.mortal_start_room = CONFIG_MORTAL_START;
  OLC_CONFIG(d)->room_nums.immort_start_room = CONFIG_IMMORTAL_START;
  OLC_CONFIG(d)->room_nums.frozen_start_room = CONFIG_FROZEN_START;
  OLC_CONFIG(d)->room_nums.donation_room_1   = CONFIG_DON_ROOM_1;
  OLC_CONFIG(d)->room_nums.donation_room_2   = CONFIG_DON_ROOM_2;
  OLC_CONFIG(d)->room_nums.donation_room_3   = CONFIG_DON_ROOM_3;
  
  /****************************************************************************/
  /** Game Operation                                                         **/
  /****************************************************************************/
  OLC_CONFIG(d)->operation.DFLT_PORT          = CONFIG_DFLT_PORT;
  OLC_CONFIG(d)->operation.max_playing        = CONFIG_MAX_PLAYING;
  OLC_CONFIG(d)->operation.max_filesize       = CONFIG_MAX_FILESIZE;
  OLC_CONFIG(d)->operation.max_bad_pws        = CONFIG_MAX_BAD_PWS;
  OLC_CONFIG(d)->operation.siteok_everyone    = CONFIG_SITEOK_ALL;
  OLC_CONFIG(d)->operation.use_new_socials    = CONFIG_NEW_SOCIALS;
  OLC_CONFIG(d)->operation.auto_save_olc      = CONFIG_OLC_SAVE;
  OLC_CONFIG(d)->operation.nameserver_is_slow = CONFIG_NS_IS_SLOW;
  
  /****************************************************************************/
  /** Autowiz                                                                **/
  /****************************************************************************/
  OLC_CONFIG(d)->autowiz.use_autowiz          = CONFIG_USE_AUTOWIZ;
  OLC_CONFIG(d)->autowiz.min_wizlist_lev      = CONFIG_MIN_WIZLIST_LEV;
  
  /****************************************************************************/
  /** Character advancement                                                  **/
  /****************************************************************************/
  OLC_CONFIG(d)->advance.allow_multiclass   = CONFIG_ALLOW_MULTICLASS;
  
  
  /****************************************************************************/
  /** Allocate space for the strings.                                        **/
  /****************************************************************************/
  OLC_CONFIG(d)->play.OK       = str_udup(CONFIG_OK);
  OLC_CONFIG(d)->play.NOPERSON = str_udup(CONFIG_NOPERSON);
  OLC_CONFIG(d)->play.NOEFFECT = str_udup(CONFIG_NOEFFECT);
  
  if (CONFIG_DFLT_IP)
    OLC_CONFIG(d)->operation.DFLT_IP     = strdup(CONFIG_DFLT_IP);
  else
    OLC_CONFIG(d)->operation.DFLT_IP     = NULL;
  
  if (CONFIG_DFLT_DIR)
    OLC_CONFIG(d)->operation.DFLT_DIR    = strdup(CONFIG_DFLT_DIR);
  else
    OLC_CONFIG(d)->operation.DFLT_DIR    = NULL;

  if (CONFIG_LOGNAME)
    OLC_CONFIG(d)->operation.LOGNAME     = strdup(CONFIG_LOGNAME);
  else
    OLC_CONFIG(d)->operation.LOGNAME     = NULL;
  
  if (CONFIG_MENU)
    OLC_CONFIG(d)->operation.MENU        = strdup(CONFIG_MENU);
  else
    OLC_CONFIG(d)->operation.MENU        = NULL;
  
  if (CONFIG_WELC_MESSG)
    OLC_CONFIG(d)->operation.WELC_MESSG  = strdup(CONFIG_WELC_MESSG);
  else
    OLC_CONFIG(d)->operation.WELC_MESSG  = NULL;
  
  if (CONFIG_START_MESSG)
    OLC_CONFIG(d)->operation.START_MESSG = strdup(CONFIG_START_MESSG);
  else
    OLC_CONFIG(d)->operation.START_MESSG = NULL;
  
  cedit_disp_menu(d);

  /****************************************************************************/
  /** Tick system                                                            **/
  /****************************************************************************/
  OLC_CONFIG(d)->ticks.pulse_violence     = CONFIG_PULSE_VIOLENCE;
  OLC_CONFIG(d)->ticks.pulse_mobile       = CONFIG_PULSE_MOBILE;
  OLC_CONFIG(d)->ticks.pulse_zone         = CONFIG_PULSE_ZONE;
  OLC_CONFIG(d)->ticks.pulse_autosave     = CONFIG_PULSE_AUTOSAVE;
  OLC_CONFIG(d)->ticks.pulse_idlepwd      = CONFIG_PULSE_IDLEPWD;
  OLC_CONFIG(d)->ticks.pulse_sanity       = CONFIG_PULSE_SANITY;
  OLC_CONFIG(d)->ticks.pulse_usage        = CONFIG_PULSE_USAGE;
  OLC_CONFIG(d)->ticks.pulse_timesave     = CONFIG_PULSE_TIMESAVE;
  OLC_CONFIG(d)->ticks.pulse_current      = CONFIG_PULSE_CURRENT;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/
  OLC_CONFIG(d)->creation.method   = CONFIG_CREATION_METHOD;
}

/******************************************************************************/

void cedit_save_internally(struct descriptor_data *d)
{
  /* see if we need to reassign spec procs on rooms */
  int reassign = (CONFIG_DTS_ARE_DUMPS != OLC_CONFIG(d)->play.dts_are_dumps);
  /****************************************************************************/
  /** Copy the data back from the descriptor to the config_info structure.   **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED          = OLC_CONFIG(d)->play.pk_allowed;
  CONFIG_PT_ALLOWED          = OLC_CONFIG(d)->play.pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT     = OLC_CONFIG(d)->play.level_can_shout;
  CONFIG_HOLLER_MOVE_COST    = OLC_CONFIG(d)->play.holler_move_cost;
  CONFIG_TUNNEL_SIZE         = OLC_CONFIG(d)->play.tunnel_size;
  CONFIG_MAX_EXP_GAIN        = OLC_CONFIG(d)->play.max_exp_gain;
  CONFIG_MAX_EXP_LOSS        = OLC_CONFIG(d)->play.max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME = OLC_CONFIG(d)->play.max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME  = OLC_CONFIG(d)->play.max_pc_corpse_time;
  CONFIG_IDLE_VOID           = OLC_CONFIG(d)->play.idle_void;
  CONFIG_IDLE_RENT_TIME      = OLC_CONFIG(d)->play.idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL      = OLC_CONFIG(d)->play.idle_max_level;
  CONFIG_DTS_ARE_DUMPS       = OLC_CONFIG(d)->play.dts_are_dumps;
  CONFIG_LOAD_INVENTORY      = OLC_CONFIG(d)->play.load_into_inventory;
  CONFIG_TRACK_T_DOORS       = OLC_CONFIG(d)->play.track_through_doors;
  CONFIG_LEVEL_CAP           = OLC_CONFIG(d)->play.level_cap;
  CONFIG_STACK_MOBS	     = OLC_CONFIG(d)->play.stack_mobs;
  CONFIG_STACK_OBJS	     = OLC_CONFIG(d)->play.stack_objs;
  CONFIG_MOB_FIGHTING        = OLC_CONFIG(d)->play.mob_fighting;
  CONFIG_DISP_CLOSED_DOORS   = OLC_CONFIG(d)->play.disp_closed_doors;
  CONFIG_CAMPAIGN            = OLC_CONFIG(d)->play.campaign;
  CONFIG_MAP                 = OLC_CONFIG(d)->play.map_option;
  CONFIG_MAP_SIZE            = OLC_CONFIG(d)->play.map_size;
  CONFIG_MINIMAP_SIZE        = OLC_CONFIG(d)->play.minimap_size;
  CONFIG_REROLL_PLAYER_CREATION = OLC_CONFIG(d)->play.reroll_player;
  CONFIG_INITIAL_POINTS_POOL	= OLC_CONFIG(d)->play.initial_points;
  CONFIG_ENABLE_COMPRESSION = OLC_CONFIG(d)->play.enable_compression;
  CONFIG_ENABLE_LANGUAGES = OLC_CONFIG(d)->play.enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE = OLC_CONFIG(d)->play.all_items_unique;
  CONFIG_EXP_MULTIPLIER   = OLC_CONFIG(d)->play.exp_multiplier;
  
  /****************************************************************************/
  /** Crash Saves                                                            **/
  /****************************************************************************/
  CONFIG_FREE_RENT            = OLC_CONFIG(d)->csd.free_rent;
  CONFIG_MAX_OBJ_SAVE         = OLC_CONFIG(d)->csd.max_obj_save;
  CONFIG_MIN_RENT_COST        = OLC_CONFIG(d)->csd.min_rent_cost;
  CONFIG_AUTO_SAVE            = OLC_CONFIG(d)->csd.auto_save;
  CONFIG_AUTOSAVE_TIME        = OLC_CONFIG(d)->csd.autosave_time;
  CONFIG_CRASH_TIMEOUT   = OLC_CONFIG(d)->csd.crash_file_timeout;
  CONFIG_RENT_TIMEOUT    = OLC_CONFIG(d)->csd.rent_file_timeout;
  
  /****************************************************************************/
  /** Room Numbers                                                           **/
  /****************************************************************************/
  CONFIG_MORTAL_START = OLC_CONFIG(d)->room_nums.mortal_start_room;
  CONFIG_IMMORTAL_START = OLC_CONFIG(d)->room_nums.immort_start_room;
  CONFIG_FROZEN_START = OLC_CONFIG(d)->room_nums.frozen_start_room;
  CONFIG_DON_ROOM_1   = OLC_CONFIG(d)->room_nums.donation_room_1;
  CONFIG_DON_ROOM_2   = OLC_CONFIG(d)->room_nums.donation_room_2;
  CONFIG_DON_ROOM_3   = OLC_CONFIG(d)->room_nums.donation_room_3;
  
  /****************************************************************************/
  /** Game Operation                                                         **/
  /****************************************************************************/
  CONFIG_DFLT_PORT          = OLC_CONFIG(d)->operation.DFLT_PORT;
  CONFIG_MAX_PLAYING        = OLC_CONFIG(d)->operation.max_playing;
  CONFIG_MAX_FILESIZE       = OLC_CONFIG(d)->operation.max_filesize;
  CONFIG_MAX_BAD_PWS        = OLC_CONFIG(d)->operation.max_bad_pws;
  CONFIG_SITEOK_ALL    = OLC_CONFIG(d)->operation.siteok_everyone;
  CONFIG_NEW_SOCIALS        = OLC_CONFIG(d)->operation.use_new_socials;  
  CONFIG_NS_IS_SLOW = OLC_CONFIG(d)->operation.nameserver_is_slow;
  CONFIG_OLC_SAVE           = OLC_CONFIG(d)->operation.auto_save_olc;
  
  /****************************************************************************/
  /** Autowiz                                                                **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ          = OLC_CONFIG(d)->autowiz.use_autowiz;
  CONFIG_MIN_WIZLIST_LEV      = OLC_CONFIG(d)->autowiz.min_wizlist_lev;
  
  /****************************************************************************/
  /** Character advancement                                                  **/
  /****************************************************************************/
  CONFIG_ALLOW_MULTICLASS		= OLC_CONFIG(d)->advance.allow_multiclass;
  
  /****************************************************************************/
  /** Allocate space for the strings.                                        **/
  /****************************************************************************/
  if (CONFIG_OK)
    free(CONFIG_OK);
  CONFIG_OK       = str_udup(OLC_CONFIG(d)->play.OK);
  
  if (CONFIG_NOPERSON)
    free(CONFIG_NOPERSON);
  CONFIG_NOPERSON = str_udup(OLC_CONFIG(d)->play.NOPERSON);
  
  if (CONFIG_NOEFFECT)
    free(CONFIG_NOEFFECT);
  CONFIG_NOEFFECT = str_udup(OLC_CONFIG(d)->play.NOEFFECT);
  
  if (CONFIG_DFLT_IP)
    free(CONFIG_DFLT_IP);
  if (OLC_CONFIG(d)->operation.DFLT_IP)
    CONFIG_DFLT_IP     = strdup(OLC_CONFIG(d)->operation.DFLT_IP);
  else
    CONFIG_DFLT_IP     = NULL;
  
  
  if (CONFIG_DFLT_DIR)
    free(CONFIG_DFLT_DIR);
  if (OLC_CONFIG(d)->operation.DFLT_DIR)
    CONFIG_DFLT_DIR    = strdup(OLC_CONFIG(d)->operation.DFLT_DIR);
  else
    CONFIG_DFLT_DIR    = NULL;

  if (CONFIG_LOGNAME)
    free(CONFIG_LOGNAME);
  if (OLC_CONFIG(d)->operation.LOGNAME)
    CONFIG_LOGNAME     = strdup(OLC_CONFIG(d)->operation.LOGNAME);
  else
    CONFIG_LOGNAME     = NULL;
  
  if (CONFIG_MENU)
    free(CONFIG_MENU);
  if (OLC_CONFIG(d)->operation.MENU)
    CONFIG_MENU        = strdup(OLC_CONFIG(d)->operation.MENU);
  else
    CONFIG_MENU        = NULL;
  
  if (CONFIG_WELC_MESSG)
    free(CONFIG_WELC_MESSG);
  if (OLC_CONFIG(d)->operation.WELC_MESSG)
    CONFIG_WELC_MESSG  = strdup(OLC_CONFIG(d)->operation.WELC_MESSG);
  else
    CONFIG_WELC_MESSG  = NULL;
  
  if (CONFIG_START_MESSG)
    free(CONFIG_START_MESSG);
  if (OLC_CONFIG(d)->operation.START_MESSG)
    CONFIG_START_MESSG = strdup(OLC_CONFIG(d)->operation.START_MESSG);
  else
    CONFIG_START_MESSG = NULL;
  
  /* if we changed the dts to/from dumps, reassign - Welcor */
  if (reassign)
    reassign_rooms();

  add_to_save_list(NOWHERE, SL_CFG);

  /****************************************************************************/
  /** Tick system                                                            **/
  /****************************************************************************/

  CONFIG_PULSE_VIOLENCE		= OLC_CONFIG(d)->ticks.pulse_violence;
  CONFIG_PULSE_VIOLENCE       = OLC_CONFIG(d)->ticks.pulse_violence;
  CONFIG_PULSE_MOBILE         = OLC_CONFIG(d)->ticks.pulse_mobile;
  CONFIG_PULSE_ZONE           = OLC_CONFIG(d)->ticks.pulse_zone;
  CONFIG_PULSE_AUTOSAVE       = OLC_CONFIG(d)->ticks.pulse_autosave;
  CONFIG_PULSE_IDLEPWD        = OLC_CONFIG(d)->ticks.pulse_idlepwd;
  CONFIG_PULSE_SANITY         = OLC_CONFIG(d)->ticks.pulse_sanity;
  CONFIG_PULSE_USAGE          = OLC_CONFIG(d)->ticks.pulse_usage;
  CONFIG_PULSE_TIMESAVE       = OLC_CONFIG(d)->ticks.pulse_timesave;
  CONFIG_PULSE_CURRENT        = OLC_CONFIG(d)->ticks.pulse_current;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/

  CONFIG_CREATION_METHOD      = OLC_CONFIG(d)->creation.method;
}


/******************************************************************************/

void cedit_save_to_disk( void )
{
  /****************************************************************************/
  /** Just call save_config and get it over with.                            **/
  /****************************************************************************/
  save_config( NOWHERE );
}

/******************************************************************************/

int save_config( IDXTYPE nowhere )
{
  FILE *fl;
  char buf[MAX_STRING_LENGTH]={'\0'};
  
  if (!(fl = fopen(CONFIG_CONFFILE, "w"))) {
    log("SYSERR: save_config: %s", strerror(errno));
    return (FALSE);
  }
  
  fprintf(fl, 
    "* This file is autogenerated by OasisOLC (CEdit).\n"
    "* Please note the following information about this file's format.\n"
    "*\n"
    "* - If variable is a yes/no or true/false based variable, use 1's and 0's\n"
    "*   where YES or TRUE = 1 and NO or FALSE = 0.\n"
    "* - Variable names in this file are case-insensitive.  Variable values\n"
    "*   are not case-insensitive.\n"
    "* -----------------------------------------------------------------------\n"
    "* Lines starting with * are comments, and are not parsed.\n"
    "* -----------------------------------------------------------------------\n\n"
    "* [ Game Play Options ]\n"
  );
    
  fprintf(fl, "* Is player killing allowed on the mud?\n"
              "pk_allowed = %d\n\n", CONFIG_PK_ALLOWED);
  fprintf(fl, "* Is player thieving allowed on the mud?\n"
  	      "pt_allowed = %d\n\n", CONFIG_PT_ALLOWED);
  fprintf(fl, "* What is the minimum level a player can shout/gossip/etc?\n"
              "level_can_shout = %d\n\n", CONFIG_LEVEL_CAN_SHOUT);
  fprintf(fl, "* How many movement points does shouting cost the player?\n"
  	      "holler_move_cost = %d\n\n", CONFIG_HOLLER_MOVE_COST);
  fprintf(fl, "* How many players can fit in a tunnel?\n"
              "tunnel_size = %d\n\n", CONFIG_TUNNEL_SIZE);
  fprintf(fl, "* Maximum experience gainable per kill?\n"
              "max_exp_gain = %d\n\n", CONFIG_MAX_EXP_GAIN);
  fprintf(fl, "* Maximum experience loseable per death?\n"
              "max_exp_loss = %d\n\n", CONFIG_MAX_EXP_LOSS);
  fprintf(fl, "* Number of tics before NPC corpses decompose.\n"
              "max_npc_corpse_time = %d\n\n", CONFIG_MAX_NPC_CORPSE_TIME);
  fprintf(fl, "* Number of tics before PC corpses decompose.\n"
              "max_pc_corpse_time = %d\n\n", CONFIG_MAX_PC_CORPSE_TIME);
  fprintf(fl, "* Number of tics before a PC is sent to the void.\n"
              "idle_void = %d\n\n", CONFIG_IDLE_VOID);
  fprintf(fl, "* Number of tics before a PC is autorented.\n"
              "idle_rent_time = %d\n\n", CONFIG_IDLE_RENT_TIME);
  fprintf(fl, "* Admin level and above of players whom are immune to idle penalties.\n"
              "idle_max_level = %d\n\n", CONFIG_IDLE_MAX_LEVEL);
  fprintf(fl, "* Should the items in death traps be junked automatically?\n"
              "dts_are_dumps = %d\n\n", CONFIG_DTS_ARE_DUMPS);
  fprintf(fl, "* When an immortal loads an object, should it load into their inventory?\n"
              "load_into_inventory = %d\n\n", CONFIG_LOAD_INVENTORY);
  fprintf(fl, "* Should PC's be able to track through hidden or closed doors?\n"
              "track_through_doors = %d\n\n", CONFIG_TRACK_T_DOORS);
  fprintf(fl, "* What is the level that cannot be attained?\n"
              "level_cap = %d\n\n", CONFIG_LEVEL_CAP);
  fprintf(fl, "* Stack mobiles when showing contents of rooms?\n"
              "stack_mobs = %d\n\n", CONFIG_STACK_MOBS);
  fprintf(fl, "* Stack objects when showing contents of rooms?\n"
              "stack_objs = %d\n\n", CONFIG_STACK_OBJS);
  fprintf(fl, "* Allow aggressive mobs to attack other mobs?\n"
              "mob_fighting = %d\n\n", CONFIG_MOB_FIGHTING);
  fprintf(fl, "* Should closed doors be shown on autoexit / exit?\n"
              "disp_closed_doors = %d\n\n", CONFIG_DISP_CLOSED_DOORS);
  fprintf(fl, "* What campaign setting is running on the mud?\n"
              "campaign_setting = %d\n\n", CONFIG_CAMPAIGN);
  fprintf(fl, "* Who can use the map functions? 0=off, 1=on, 2=imm_only\n"
              "map_option = %d\n\n", CONFIG_MAP);
  fprintf(fl, "* Default size of map shown by 'map' command\n"
              "default_map_size = %d\n\n", CONFIG_MAP_SIZE);
  fprintf(fl, "* Default minimap size shown to the right of room descriptions\n"
              "default_minimap_size = %d\n\n", CONFIG_MINIMAP_SIZE);
  fprintf(fl, "* Should players be able to reroll stats at creation?\n"
              "reroll_stats = %d\n\n", CONFIG_REROLL_PLAYER_CREATION);
  fprintf(fl, "* How many points in players initial points pool if using point pool creation?\n"
  	      "initial_points = %d\n\n", CONFIG_INITIAL_POINTS_POOL);
  fprintf(fl, "* Should compression be used if the client supports it?\n"
              "compression = %d\n\n", CONFIG_ENABLE_COMPRESSION);
  fprintf(fl, "* Should spoken languages be used?\n"
              "enable_languages = %d\n\n", CONFIG_ENABLE_LANGUAGES);
  fprintf(fl, "* Should all items be treated as unique?\n"
              "all_items_unique = %d\n\n", CONFIG_ALL_ITEMS_UNIQUE);
  fprintf(fl, "* Amount of in game experience multiplier.\n"
              "exp_multiplier = %.2f\n\n", CONFIG_EXP_MULTIPLIER);
              
              
  strcpy(buf, CONFIG_OK);
  strip_cr(buf);
  
  fprintf(fl, "* Text sent to players when OK is all that is needed.\n"
              "ok = %s\n\n", buf);
              
  strcpy(buf, CONFIG_NOPERSON);
  strip_cr(buf);            
              
  fprintf(fl, "* Text sent to players when noone is available.\n"
              "noperson = %s\n\n", buf);
              
  strcpy(buf, CONFIG_NOEFFECT);
  strip_cr(buf);
  
  fprintf(fl, "* Text sent to players when an effect fails.\n"
              "noeffect = %s\n", buf);
  
  
  
  /************************************************************************
   ** RENT / CRASHSAVE OPTIONS					         **
   ************************************************************************/
  fprintf(fl, "\n\n\n* [ Rent/Crashsave Options ]\n");
  
  fprintf(fl, "* Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,\n"
              "* your objects are saved at no cost, as in Merc-type MUDs.)\n"
              "free_rent = %d\n\n", CONFIG_FREE_RENT);
  
  fprintf(fl, "* Maximum number of items players are allowed to rent.\n"
   	      "max_obj_save = %d\n\n", CONFIG_MAX_OBJ_SAVE);
 
  fprintf(fl, "* Receptionist's surcharge on top of item costs.\n"
              "min_rent_cost = %d\n\n", CONFIG_MIN_RENT_COST);
 
  fprintf(fl, "* Should the game automatically save people?\n"
              "auto_save = %d\n\n", CONFIG_AUTO_SAVE);
  
  fprintf(fl, "* If auto_save = 1, how often (in minutes) should the game save people's objects?\n"
              "autosave_time = %d\n\n", CONFIG_AUTOSAVE_TIME);
  
  fprintf(fl, "* Lifetime of crashfiles and force-rent (idlesave) files in days.\n"
              "crash_file_timeout = %d\n\n", CONFIG_CRASH_TIMEOUT);
  
  fprintf(fl, "* Lifetime of normal rent files in days.\n"
              "rent_file_timeout = %d\n\n", CONFIG_RENT_TIMEOUT);
  
  
  /************************************************************************
   ** ROOM NUMBERS						  	 **
   ************************************************************************/
  fprintf(fl, "\n\n\n* [ Room Numbers ]\n");
  
  fprintf(fl, "* The virtual number of the room that mortals should enter at.\n"
  	      "mortal_start_room = %d\n\n", CONFIG_MORTAL_START);
  
  fprintf(fl, "* The virtual number of the room that immorts should enter at.\n"
              "immort_start_room = %d\n\n", CONFIG_IMMORTAL_START);
  
  fprintf(fl, "* The virtual number of the room that frozen people should enter at.\n"
	      "frozen_start_room = %d\n\n", CONFIG_FROZEN_START);

  fprintf(fl, "* The virtual numbers of the donation rooms.  Note: Add donation rooms\n"
              "* sequentially (1 & 2 before 3). If you don't, you might not be able to\n"
              "* donate. Use -1 for 'no such room'.\n"
              "donation_room_1 = %d\n"
              "donation_room_2 = %d\n"
              "donation_room_3 = %d\n\n",
              CONFIG_DON_ROOM_1 != NOWHERE ? CONFIG_DON_ROOM_1 : -1,
              CONFIG_DON_ROOM_2 != NOWHERE ? CONFIG_DON_ROOM_2 : -1,
              CONFIG_DON_ROOM_3 != NOWHERE ? CONFIG_DON_ROOM_3 : -1);
  
  
  fprintf(fl, "\n\n\n* [ Game Operation Options ]\n");
  
  fprintf(fl, "* This is the default port on which the game should run if no port is\n"
              "* given on the command-line.  NOTE WELL: If you're using the\n"
              "* 'autorun' script, the port number there will override this setting.\n"
              "* Change the PORT= line in autorun instead of (or in addition to)\n"
              "* changing this.\n"
              "DFLT_PORT = %d\n\n",
              CONFIG_DFLT_PORT);
  
  if (CONFIG_DFLT_IP) {
    strcpy(buf, CONFIG_DFLT_IP);
    strip_cr(buf);
  
    fprintf(fl, "* IP address to which the MUD should bind.\nDFLT_IP = %s\n\n", buf);
  }
  
  if (CONFIG_DFLT_DIR) {
    strcpy(buf, CONFIG_DFLT_DIR);
    strip_cr(buf);
    
    fprintf(fl, "* default directory to use as data directory.\n"
                "DFLT_DIR = %s\n\n", buf);
  }
  
  if (CONFIG_LOGNAME) {
    strcpy(buf, CONFIG_LOGNAME);
    strip_cr(buf);
    
    fprintf(fl, "* What file to log messages to (ex: 'log/syslog').\n"
                "LOGNAME = %s\n\n", buf);
  }
  
  fprintf(fl, "* Maximum number of players allowed before game starts to turn people away.\n"
              "max_playing = %d\n\n",
              CONFIG_MAX_PLAYING);
              
  fprintf(fl, "* Maximum size of bug, typo, and idea files in bytes (to prevent bombing).\n"
              "max_filesize = %d\n\n",
              CONFIG_MAX_FILESIZE);
  
  fprintf(fl, "* Maximum number of password attempts before disconnection.\n"
              "max_bad_pws = %d\n\n",
              CONFIG_MAX_BAD_PWS);
  
  fprintf(fl, "* Is the site ok for everyone except those that are banned?\n"
              "siteok_everyone = %d\n\n",
              CONFIG_SITEOK_ALL);

  fprintf(fl, "* If you want to use the original social file format\n"
              "* and disable Aedit, set to 0, otherwise, 1.\n"
              "use_new_socials = %d\n\n",
              CONFIG_NEW_SOCIALS);
  
  fprintf(fl, "* If the nameserver is fast, set to 0, otherwise, 1.\n"
              "nameserver_is_slow = %d\n\n",
              CONFIG_NS_IS_SLOW);

  fprintf(fl, "* Should OLC autosave to disk (1) or save internally (0).\n"
              "auto_save_olc = %d\n\n",
              CONFIG_OLC_SAVE);
  
  if (CONFIG_MENU) {
    strcpy(buf, CONFIG_MENU);
    strip_cr(buf);
    
    fprintf(fl, "* The entrance/exit menu.\n"
                "MENU = \n%s~\n\n", buf);
  }
  
  if (CONFIG_WELC_MESSG) {
    strcpy(buf, CONFIG_WELC_MESSG);
    strip_cr(buf);

    fprintf(fl, "* The welcome message.\nWELC_MESSG = \n%s~\n\n", buf);    
  }
  
  if (CONFIG_START_MESSG) {
    strcpy(buf, CONFIG_START_MESSG);
    strip_cr(buf);
    
    fprintf(fl, "* NEWBIE start message.\n"
                "START_MESSG = \n%s~\n\n", buf);
  }
  
  fprintf(fl, "\n\n\n* [ Autowiz Options ]\n");
  
  fprintf(fl, "* Should the game automatically create a new wizlist/immlist every time\n"
              "* someone immorts, or is promoted to a higher (or lower) god level?\n"
              "use_autowiz = %d\n\n",
              CONFIG_USE_AUTOWIZ);
  
  fprintf(fl, "* If yes, what is the lowest level which should be on the wizlist?\n"
              "min_wizlist_lev = %d\n\n",
              CONFIG_MIN_WIZLIST_LEV);
              
  fprintf(fl, "\n\n\n* [ Character Advancement Options ]\n");
  
  fprintf(fl, "* Should characters be allowed to advance in multiple classes?\n"
              "allow_multiclass = %d\n\n",
              CONFIG_ALLOW_MULTICLASS);
  
  fprintf(fl, "\n\n\n* [ Game Speed Options ]\n");

  fprintf(fl, "*Speed of Violence system. 0 - 10 (10) being the slowest.\n"
              "pulse_viol = %d\n\n",
              CONFIG_PULSE_VIOLENCE);

  fprintf(fl, "*Speed of Mobile actions. 0 - 20 (20) being the slowest.\n"
              "pulse_mobile = %d\n\n",
              CONFIG_PULSE_MOBILE);

  fprintf(fl, "*Zone updates. 0 - 20 (20) being the slowest.\n"
              "pulse_zone = %d\n\n",
              CONFIG_PULSE_ZONE);

  fprintf(fl, "*Autosave. 0 - 100 (100) being the slowest.\n"
              "pulse_autosave = %d\n\n",
              CONFIG_PULSE_AUTOSAVE);

  fprintf(fl, "*Idling passwords. 0 - 30 (30) being the slowest.\n"
              "pulse_idlepwd = %d\n\n",
              CONFIG_PULSE_IDLEPWD);

  fprintf(fl, "*Sanity check time.. 0 - 50 (50) being the slowest.\n"
              "pulse_sanity = %d\n\n",
              CONFIG_PULSE_SANITY);

  fprintf(fl, "*Usage time.  0 - 10 (10) being the slowest in minutes.\n"
              "pulse_usage = %d\n\n",
              CONFIG_PULSE_USAGE);

  fprintf(fl, "*Timesaving. 0 - 50 (50) being the slowest in minutes.\n"
              "pulse_timesave = %d\n\n",
              CONFIG_PULSE_TIMESAVE);

  fprintf(fl, "*Current update time. 0 - 30 (30) being the slowest.\n"
              "pulse_current = %d\n\n",
              CONFIG_PULSE_CURRENT);

  fprintf(fl, "*Character creation method.\n"
              "method = %d\n\n",
              CONFIG_CREATION_METHOD);
  fclose(fl);
  
  if (in_save_list(NOWHERE, SL_CFG))
    remove_from_save_list(NOWHERE, SL_CFG);
  
  return (TRUE);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * the main menu 
 */
void cedit_disp_menu(struct descriptor_data *d)
{
  clear_screen(d);

  /*
   * Menu header
   */
  write_to_output(d, 
  	  "OasisOLC MUD Configuration Editor\r\n"
          "@WA@B) @CAutowiz Options\r\n"
  	  "@WC@B) @CCrashsave/Rent Options\r\n"
          "@WD@B) @CaDvancement Options\r\n"
  	  "@WG@B) @CGame Play Options\r\n"
  	  "@WN@B) @CNew Character Creation Options\r\n"
          "@WO@B) @COperation Options\r\n"
  	  "@WR@B) @CRoom Numbers\r\n"
          "@WT@B) @CGame Tick Options\r\n"
          "@WQ@B) @CQuit\r\n"
          "@WEnter your choice : @n"
          );

  OLC_MODE(d) = CEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

void cedit_disp_game_play_options(struct descriptor_data *d)
{
  int m_opt;
  clear_screen(d);

  m_opt = OLC_CONFIG(d)->play.map_option;
  
  write_to_output(d, "\r\n\r\n"
        "@WA@B) @CPlayer Killing Allowed      : @c%s\r\n"
        "@WB@B) @CPlayer Thieving Allowed     : @c%s\r\n"
        "@WC@B) @CMinimum Level To Shout      : @c%d\r\n"
        "@WD@B) @CHoller Move Cost            : @c%d\r\n"
        "@WE@B) @CTunnel Size                 : @c%d\r\n"
        "@WF@B) @CMaximum Experience Gain     : @c%d\r\n"
        "@WG@B) @CMaximum Experience Loss     : @c%d\r\n"
        "@WH@B) @CMax Time for NPC Corpse     : @c%d\r\n"
        "@WI@B) @CMax Time for PC Corpse      : @c%d\r\n"
        "@WJ@B) @CTics before PC sent to void : @c%d\r\n"
        "@WK@B) @CTics before PC is autosaved : @c%d\r\n"
        "@WL@B) @CLevel Immune To IDLE        : @c%d\r\n"
        "@WM@B) @CDeath Traps Junk Items      : @c%s\r\n"
        "@WN@B) @CObjects Load Into Inventory : @c%s\r\n"
        "@WO@B) @CTrack Through Doors         : @c%s\r\n"
        "@WP@B) @CDisplay Closed Doors        : @c%s\r\n"
        "@WR@B) @CUnattainable Level          : @c%d\r\n"
        "@WS@B) @CTreat all Objects as Unique : @c%s\r\n"
        "@WT@B) @CExperience multiplier       : @c%.2f\r\n"
	"@WU@B) @CAllow MCCP2 compression     : @c%s\r\n"
	"@WV@B) @CAllow spoken languages      : @c%s\r\n"
	"@WW@B) @CCampaign Setting            : @c%s\r\n"
	"@W1@B) @CStack Mobiles in room descs : @c%s\r\n"
	"@W2@B) @CStack Objects in room descs : @c%s\r\n"
	"@W3@B) @CAllow mobs to fight mobs    : @c%s\r\n"
        "@W4@B) @COK Message Text             : @c%s"
        "@W5@B) @CNOPERSON Message Text       : @c%s"
        "@W6@B) @CNOEFFECT Message Text       : @c%s"
        "@W7@B) @CMap/Automap Option          : @c%s\r\n"
        "@W8@B) @CDefault map size            : @c%d\r\n"
        "@W9@B) @CDefault minimap size        : @c%d\r\n"
        "@WQ@B) @CExit To The Main Menu\r\n"
        "@WEnter your choice : @n",
        CHECK_VAR(OLC_CONFIG(d)->play.pk_allowed),
        CHECK_VAR(OLC_CONFIG(d)->play.pt_allowed),
        OLC_CONFIG(d)->play.level_can_shout,
        OLC_CONFIG(d)->play.holler_move_cost,
        OLC_CONFIG(d)->play.tunnel_size,
        OLC_CONFIG(d)->play.max_exp_gain,
        OLC_CONFIG(d)->play.max_exp_loss,
        OLC_CONFIG(d)->play.max_npc_corpse_time,
        OLC_CONFIG(d)->play.max_pc_corpse_time,

        OLC_CONFIG(d)->play.idle_void,
        OLC_CONFIG(d)->play.idle_rent_time,
        OLC_CONFIG(d)->play.idle_max_level,
        CHECK_VAR(OLC_CONFIG(d)->play.dts_are_dumps),
        CHECK_VAR(OLC_CONFIG(d)->play.load_into_inventory),
        CHECK_VAR(OLC_CONFIG(d)->play.track_through_doors),
        CHECK_VAR(OLC_CONFIG(d)->play.disp_closed_doors),
        OLC_CONFIG(d)->play.level_cap,
	CHECK_VAR(OLC_CONFIG(d)->play.all_items_unique),
	OLC_CONFIG(d)->play.exp_multiplier,
	CHECK_VAR(OLC_CONFIG(d)->play.enable_compression),
	CHECK_VAR(OLC_CONFIG(d)->play.enable_languages),
        // OLC_CONFIG(d)->play.campaign == CAMPAIGN_FORGOTTEN_REALMS ? "Forgotten Realms" : "Dragonlance: Age of Legends",
  CampaignWorld [OLC_CONFIG (d)->play.campaign],
	CHECK_VAR(OLC_CONFIG(d)->play.stack_mobs),
	CHECK_VAR(OLC_CONFIG(d)->play.stack_objs),
        CHECK_VAR(OLC_CONFIG(d)->play.mob_fighting),
        OLC_CONFIG(d)->play.OK,
        OLC_CONFIG(d)->play.NOPERSON,
        OLC_CONFIG(d)->play.NOEFFECT,
        m_opt == 0 ? "Off" : (m_opt == 1 ? "On" : (m_opt == 2 ? "Imm-Only" : "Invalid!")),
        OLC_CONFIG(d)->play.map_size,
        OLC_CONFIG(d)->play.minimap_size
        );

  OLC_MODE(d) = CEDIT_GAME_OPTIONS_MENU;
}

/*-------------------------------------------------------------------*/

void cedit_disp_crash_save_options(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
  	"@WA@B) @CFree Rent          : @c%s\r\n"
  	"@WB@B) @CMax Objects Saved  : @c%d\r\n"
  	"@WC@B) @CMinimum Rent Cost  : @c%d\r\n"
  	"@WD@B) @CAuto Save          : @c%s\r\n"
  	"@WE@B) @CAuto Save Time     : @c%d minute(s)\r\n"
  	"@WF@B) @CCrash File Timeout : @c%d day(s)\r\n"
  	"@WG@B) @CRent File Timeout  : @c%d day(s)\r\n"
  	"@WQ@B) @CExit To The Main Menu\r\n"
  	"@WEnter your choice : @n",
  	CHECK_VAR(OLC_CONFIG(d)->csd.free_rent),
  	OLC_CONFIG(d)->csd.max_obj_save,
  	OLC_CONFIG(d)->csd.min_rent_cost,
  	CHECK_VAR(OLC_CONFIG(d)->csd.auto_save),
  	OLC_CONFIG(d)->csd.autosave_time,
  	OLC_CONFIG(d)->csd.crash_file_timeout,
  	OLC_CONFIG(d)->csd.rent_file_timeout
  	);
  
  OLC_MODE(d) = CEDIT_CRASHSAVE_OPTIONS_MENU;
}

/*-------------------------------------------------------------------*/

void cedit_disp_room_numbers(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
  	"@WA@B) @CMortal Start Room   : @c%d\r\n"
  	"@WB@B) @CImmortal Start Room : @c%d\r\n"
  	"@WC@B) @CFrozen Start Room   : @c%d\r\n"
  	"@W1@B) @CDonation Room #1    : @c%d\r\n"
  	"@W2@B) @CDonation Room #2    : @c%d\r\n"
  	"@W3@B) @CDonation Room #3    : @c%d\r\n"
  	"@WQ@B) @CExit To The Main Menu\r\n"
  	"@WEnter your choice : @n",
  	OLC_CONFIG(d)->room_nums.mortal_start_room,
  	OLC_CONFIG(d)->room_nums.immort_start_room,
  	OLC_CONFIG(d)->room_nums.frozen_start_room,
  	OLC_CONFIG(d)->room_nums.donation_room_1,
  	OLC_CONFIG(d)->room_nums.donation_room_2,
  	OLC_CONFIG(d)->room_nums.donation_room_3
  	);
  
  OLC_MODE(d) = CEDIT_ROOM_NUMBERS_MENU;
}


/*-------------------------------------------------------------------*/

void cedit_disp_operation_options(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
  	"@WA@B) @CDefault Port : @c%d\r\n"
  	"@WB@B) @CDefault IP   : @c%s\r\n"
        "@WC@B) @CDefault Directory   : @c%s\r\n"
        "@WD@B) @CLogfile Name        : @c%s\r\n"
        "@WE@B) @CMax Players         : @c%d\r\n"
        "@WF@B) @CMax Filesize        : @c%d\r\n"
        "@WG@B) @CMax Bad Pws         : @c%d\r\n"
        "@WH@B) @CSite Ok Everyone    : @c%s\r\n"
        "@WI@B) @CName Server Is Slow : @c%s\r\n"
        "@WJ@B) @CUse new socials file: @c%s\r\n"
        "@WK@B) @COLC autosave to disk: @c%s\r\n"
        "@WL@B) @CMain Menu           : \r\n@n%s@n\r\n"
        "@WM@B) @CWelcome Message     : \r\n@n%s@n\r\n"
        "@WN@B) @CStart Message       : \r\n@n%s@n\r\n"
    	"@WQ@B) @CExit To The Main Menu\r\n"
    	"@WEnter your choice : @n",
    OLC_CONFIG(d)->operation.DFLT_PORT,
    OLC_CONFIG(d)->operation.DFLT_IP ? OLC_CONFIG(d)->operation.DFLT_IP : "<None>",
    OLC_CONFIG(d)->operation.DFLT_DIR ? OLC_CONFIG(d)->operation.DFLT_DIR : "<None>",
    OLC_CONFIG(d)->operation.LOGNAME ? OLC_CONFIG(d)->operation.LOGNAME : "<None>",
    OLC_CONFIG(d)->operation.max_playing,
    OLC_CONFIG(d)->operation.max_filesize,
    OLC_CONFIG(d)->operation.max_bad_pws,
    YESNO(OLC_CONFIG(d)->operation.siteok_everyone),
    YESNO(OLC_CONFIG(d)->operation.nameserver_is_slow),
    YESNO(OLC_CONFIG(d)->operation.use_new_socials),
    YESNO(OLC_CONFIG(d)->operation.auto_save_olc),
    OLC_CONFIG(d)->operation.MENU ? OLC_CONFIG(d)->operation.MENU : "<None>",
    OLC_CONFIG(d)->operation.WELC_MESSG ? OLC_CONFIG(d)->operation.WELC_MESSG : "<None>",
    OLC_CONFIG(d)->operation.START_MESSG ? OLC_CONFIG(d)->operation.START_MESSG : "<None>"
    );
  
  OLC_MODE(d) = CEDIT_OPERATION_OPTIONS_MENU;
}
  	

/*-------------------------------------------------------------------*/

void cedit_disp_autowiz_options(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
    "@WA@B) @CUse the autowiz        : @c%s\r\n"
    "@WB@B) @CMinimum wizlist level  : @c%d\r\n"
    "@WQ@B) @CExit To The Main Menu\r\n"
    "@WEnter your choice : @n",
    CHECK_VAR(OLC_CONFIG(d)->autowiz.use_autowiz),
    OLC_CONFIG(d)->autowiz.min_wizlist_lev
    );
  
  OLC_MODE(d) = CEDIT_AUTOWIZ_OPTIONS_MENU;
}


/*-------------------------------------------------------------------*/

void cedit_disp_advance_options(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
    "@WA@B) @CAllow multiclass       : @c%s\r\n"
    "@WQ@B) @CExit To The Main Menu\r\n"
    "@WEnter your choice : @n",
    CHECK_VAR(OLC_CONFIG(d)->advance.allow_multiclass)
    );
  
  OLC_MODE(d) = CEDIT_ADVANCE_OPTIONS_MENU;
}

/*-------------------------------------------------------------------*/

void cedit_disp_ticks_menu(struct descriptor_data *d)
{
  clear_screen(d);

  write_to_output(d, "\r\n\r\n"
    "@WA@B) @CPulse Violence Time  : @c%d\r\n"
    "@WB@B) @CMobile Action Time   : @c%d\r\n"
    "@WC@B) @CZone Time            : @c%d\r\n"
    "@WD@B) @CAutosave Time        : @c%d\r\n"
    "@WE@B) @CIdle Password Time   : @c%d\r\n"
    "@WF@B) @CSanity Check Time    : @c%d\r\n"
    "@WG@B) @CUsage Check Time     : @c%d\r\n"
    "@WH@B) @CTimesave Time        : @c%d\r\n"
    "@WI@B) @CCurrents Update Time : @c%d\r\n"
    "@WQ@B) @CExit To The Main Menu\r\n"
    "@WEnter your choice : @n",
    OLC_CONFIG(d)->ticks.pulse_violence,
    OLC_CONFIG(d)->ticks.pulse_mobile,
    OLC_CONFIG(d)->ticks.pulse_zone,
    OLC_CONFIG(d)->ticks.pulse_autosave,
    OLC_CONFIG(d)->ticks.pulse_idlepwd,
    OLC_CONFIG(d)->ticks.pulse_sanity,
    OLC_CONFIG(d)->ticks.pulse_usage,
    OLC_CONFIG(d)->ticks.pulse_timesave,
    OLC_CONFIG(d)->ticks.pulse_current
    );

   OLC_MODE(d) = CEDIT_TICKS_OPTIONS_MENU;
}

/*-------------------------------------------------------------------*/

void cedit_disp_creation_options(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d, "\r\n\r\n"
    "@WA@B) @CCharacter Creation Method : @c%d \r\n"
    "   %s\r\n"
    "@WB@B) @CPlayers can reroll stats on creation : @c%s\r\n"
    "@WC@B) @CNumber of points in initial points pool for pool generation methods : @c%d \r\n"
    "@WQ@B) @CExit To The Main Menu\r\n"
    "@WEnter your choice : @n",
    OLC_CONFIG(d)->creation.method,
    creation_methods[(int)OLC_CONFIG(d)->creation.method],
    CHECK_VAR(OLC_CONFIG(d)->play.reroll_player),
    OLC_CONFIG(d)->play.initial_points);
  OLC_MODE(d) = CEDIT_CREATION_OPTIONS_MENU;  
}

void cedit_disp_creation_menu(struct descriptor_data *d)
{
  int i = 0;
  char buffer1[MAX_INPUT_LENGTH]={'\0'};

  clear_screen(d);
  for (i = 0; i < NUM_CREATION_METHODS; i++) 
  {
    snprintf(buffer1, sizeof(buffer1), "@W%d@B) @C%s@n\r\n", i, creation_methods[i]);
    write_to_output(d, "%s", buffer1);
  }
  write_to_output(d, "Choose character creation type: ");
  OLC_MODE(d) = CEDIT_CREATION_MENU;  
}



/*-------------------------------------------------------------------*/
/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void cedit_parse(struct descriptor_data *d, char *arg)
{
  char *oldtext = NULL;
  
  switch (OLC_MODE(d)) {
    case CEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          cedit_save_internally(d);
          mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, 
                 "OLC: %s modifies the game configuration.", GET_NAME(d->character));
          cleanup_olc(d, CLEANUP_CONFIG);
	  if (CONFIG_AUTO_SAVE) {
	    cedit_save_to_disk();
	    write_to_output(d, "Game configuration saved to disk.\r\n");
	  } else
          write_to_output(d, "Game configuration saved to memory.\r\n");
          return;
        case 'n':
        case 'N':
          write_to_output(d, "Game configuration not saved to memory.\r\n");
          cleanup_olc(d, CLEANUP_CONFIG);
          return;
        default :
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
          write_to_output(d, "Do you wish to save the configuration? (y/n) : ");
          return;
      }
      
/*-------------------------------------------------------------------*/
      
    case CEDIT_MAIN_MENU:
      switch (*arg) {
        case 'g':
        case 'G':
          cedit_disp_game_play_options(d);
          OLC_MODE(d) = CEDIT_GAME_OPTIONS_MENU;
          break;
          
        case 'c':
        case 'C':
          cedit_disp_crash_save_options(d);
          OLC_MODE(d) = CEDIT_CRASHSAVE_OPTIONS_MENU;
          break;
          
        case 'r':
        case 'R':
          cedit_disp_room_numbers(d);
          OLC_MODE(d) = CEDIT_ROOM_NUMBERS_MENU;
          break;
        
        case 'o':
        case 'O':
          cedit_disp_operation_options(d);
          OLC_MODE(d) = CEDIT_OPERATION_OPTIONS_MENU;
          break;
          
        case 'a':
        case 'A':
          cedit_disp_autowiz_options(d);
          OLC_MODE(d) = CEDIT_AUTOWIZ_OPTIONS_MENU;
          break;
        
        case 'd':
        case 'D':
          cedit_disp_advance_options(d);
          OLC_MODE(d) = CEDIT_ADVANCE_OPTIONS_MENU;
          break;
        
        case 't':
        case 'T':
          cedit_disp_ticks_menu(d);
          OLC_MODE(d) = CEDIT_TICKS_OPTIONS_MENU;
          break;

        case 'q':
        case 'Q':
          write_to_output(d, "Do you wish to save the configuration? (y/n) : ");
          OLC_MODE(d) = CEDIT_CONFIRM_SAVESTRING;
          break;
          
        case 'n':
        case 'N':
          cedit_disp_creation_options(d);
          OLC_MODE(d) = CEDIT_CREATION_OPTIONS_MENU;
          break;
        
        default:
          write_to_output(d, "That is an invalid choice!\r\n");
          cedit_disp_menu(d);
          break;
      }
      break;
          

/*-------------------------------------------------------------------*/

    case CEDIT_GAME_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          TOGGLE_VAR(OLC_CONFIG(d)->play.pk_allowed);
          break;
          
        case 'b':
        case 'B':
          TOGGLE_VAR(OLC_CONFIG(d)->play.pt_allowed);
          break;
          
        case 'c':
        case 'C':
          write_to_output(d, "Enter the minimum level a player must be to shout, gossip, etc : ");
          OLC_MODE(d) = CEDIT_LEVEL_CAN_SHOUT;
          return;
          
        case 'd':
        case 'D':
          write_to_output(d, "Enter the amount it costs (in move points) to holler : ");
          OLC_MODE(d) = CEDIT_HOLLER_MOVE_COST;
          return;
        
        case 'e':
        case 'E':
          write_to_output(d, "Enter the maximum number of people allowed in a tunnel : ");
          OLC_MODE(d) = CEDIT_TUNNEL_SIZE;
          return;
        
        case 'f':
        case 'F':
          write_to_output(d, "Enter the maximum gain of experience per kill for players : ");
          OLC_MODE(d) = CEDIT_MAX_EXP_GAIN;
          return;
          
        case 'g':
        case 'G':
          write_to_output(d, "Enter the maximum loss of experience per death for players : ");
          OLC_MODE(d) = CEDIT_MAX_EXP_LOSS;
          return;
        
        case 'h':
        case 'H':
          write_to_output(d, "Enter the number of tics before NPC corpses decompose : ");
          OLC_MODE(d) = CEDIT_MAX_NPC_CORPSE_TIME;
          return;
        
        case 'i':
        case 'I':
          write_to_output(d, "Enter the number of tics before PC corpses decompose : ");
          OLC_MODE(d) = CEDIT_MAX_PC_CORPSE_TIME;
          return;
        
        case 'j':
        case 'J':
          write_to_output(d, "Enter the number of tics before PC's are sent to the void (idle) : ");
          OLC_MODE(d) = CEDIT_IDLE_VOID;
          return;
        
        case 'k':
        case 'K':
          write_to_output(d, "Enter the number of tics before PC's are automatically rented and forced to quit : ");
          OLC_MODE(d) = CEDIT_IDLE_RENT_TIME;
          return;
        
        case 'l':
        case 'L':
          write_to_output(d, "Enter the level a player must be to become immune to IDLE : ");
          OLC_MODE(d) = CEDIT_IDLE_MAX_LEVEL;
          return;
        
        case 'm':
        case 'M':
          TOGGLE_VAR(OLC_CONFIG(d)->play.dts_are_dumps);
          break;
        
        case 'n':
        case 'N':
          TOGGLE_VAR(OLC_CONFIG(d)->play.load_into_inventory);
          break;
        
        case 'o':
        case 'O':
          TOGGLE_VAR(OLC_CONFIG(d)->play.track_through_doors);
          break;
        
        case 'p':
        case 'P':
          TOGGLE_VAR(OLC_CONFIG(d)->play.disp_closed_doors);
          break;

        case 'r':
        case 'R':
          write_to_output(d, "Enter the number a character cannot level to : ");
          OLC_MODE(d) = CEDIT_LEVEL_CAP;
          return;
        
	case 's':
	case 'S':
	  TOGGLE_VAR(OLC_CONFIG(d)->play.all_items_unique);
	  break;
        
        case 't':
        case 'T':
          write_to_output(d, "Enter the multiplier a player will recieve to experience gained : ");
          OLC_MODE(d) = CEDIT_EXP_MULTIPLIER;
          return;

	case 'U':
	case 'u':
	  TOGGLE_VAR(OLC_CONFIG(d)->play.enable_compression);
	  break;

	case 'V':
	case 'v':        
	  TOGGLE_VAR(OLC_CONFIG(d)->play.enable_languages);
	  break;

	case 'W':
	case 'w':        
          write_to_output(d, " 1) Forgotten Realms\r\n"" 2) Dragonlance\r\n"" 3) Pathfinder\r\n""Enter the campaign setting this game should be running : ");
          OLC_MODE(d) = CEDIT_CAMPAIGN;
	  return;
        

        case '1':
	  TOGGLE_VAR(OLC_CONFIG(d)->play.stack_mobs);
	  break;

	case '2':
	  TOGGLE_VAR(OLC_CONFIG(d)->play.stack_objs);
	  break;
        
        case '3':
          TOGGLE_VAR(OLC_CONFIG(d)->play.mob_fighting);
          break;
        
        case '4':
          write_to_output(d, "Enter the OK message : ");
          OLC_MODE(d) = CEDIT_OK;
          return;
        
        case '5':
          write_to_output(d, "Enter the NOPERSON message : ");
          OLC_MODE(d) = CEDIT_NOPERSON;
          return;
        
        case '6':
          write_to_output(d, "Enter the NOEFFECT message : ");
          OLC_MODE(d) = CEDIT_NOEFFECT;
          return;

        case '7':
          write_to_output(d, "1) Disable maps\r\n");
          write_to_output(d, "2) Enable Maps\r\n");
          write_to_output(d, "3) Maps for Immortals only\r\n");
          write_to_output(d, "Enter choice: ");
          OLC_MODE(d) = CEDIT_MAP_OPTION;
          return;

        case '8':
          write_to_output(d, "Enter default map size (1-12) : ");
          OLC_MODE(d) = CEDIT_MAP_SIZE;
          return;

        case '9':
          write_to_output(d, "Enter default mini-map size (1-12) : ");
          OLC_MODE(d) = CEDIT_MINIMAP_SIZE;
          return;
        
        case 'q':
        case 'Q':
          cedit_disp_menu(d);
          return;
          
        default:
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
          cedit_disp_game_play_options(d);
      }
      
      cedit_disp_game_play_options(d);
      return;
 
 /*-------------------------------------------------------------------*/
    
    case CEDIT_CRASHSAVE_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          TOGGLE_VAR(OLC_CONFIG(d)->csd.free_rent);
          break;
        
        case 'b':
        case 'B':
          write_to_output(d, "Enter the maximum number of items players can rent : ");
          OLC_MODE(d) = CEDIT_MAX_OBJ_SAVE;
          return;
        
        case 'c':
        case 'C':
          write_to_output(d, "Enter the surcharge on top of item costs : ");
          OLC_MODE(d) = CEDIT_MIN_RENT_COST;
          return;
        
        case 'd':
        case 'D':
          TOGGLE_VAR(OLC_CONFIG(d)->csd.auto_save);
          break;
        
        case 'e':
        case 'E':
          write_to_output(d, "Enter how often (in minutes) should the MUD save players : ");
          OLC_MODE(d) = CEDIT_AUTOSAVE_TIME;
          return;
        
        case 'f':
        case 'F':
          write_to_output(d, "Enter the lifetime of crash and idlesave files (days) : ");
          OLC_MODE(d) = CEDIT_CRASH_FILE_TIMEOUT;
          return;
        
        case 'g':
        case 'G':
          write_to_output(d, "Enter the lifetime of normal rent files (days) : ");
          OLC_MODE(d) = CEDIT_RENT_FILE_TIMEOUT;
          return;
        
        case 'q':
        case 'Q':
          cedit_disp_menu(d);
          return;
        
        default:
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
        }
        
        cedit_disp_crash_save_options(d);
        return;
 
 /*-------------------------------------------------------------------*/
 
    case CEDIT_ROOM_NUMBERS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          write_to_output(d, "Enter the room's vnum where mortals should load into : ");
          OLC_MODE(d) = CEDIT_MORTAL_START_ROOM;
          return;
       
        case 'b':
        case 'B':
          write_to_output(d, "Enter the room's vnum where immortals should load into : ");
          OLC_MODE(d) = CEDIT_IMMORT_START_ROOM;
          return;
       
        case 'c':
        case 'C':
        write_to_output(d, "Enter the room's vnum where frozen people should load into : ");
        OLC_MODE(d) = CEDIT_FROZEN_START_ROOM;
        return;
        
      case '1':
        write_to_output(d, "Enter the vnum for donation room #1 : ");
        OLC_MODE(d) = CEDIT_DONATION_ROOM_1;
        return;
        
      case '2':
        write_to_output(d, "Enter the vnum for donation room #2 : ");
        OLC_MODE(d) = CEDIT_DONATION_ROOM_2;
        return;
        
      case '3':
        write_to_output(d, "Enter the vnum for donation room #3 : ");
        OLC_MODE(d) = CEDIT_DONATION_ROOM_3;
        return;
        
      case 'q':
      case 'Q':
        cedit_disp_menu(d);
        return;
        
      default:
        write_to_output(d, "\r\nThat is an invalid choice!\r\n");
    }
   
    cedit_disp_room_numbers(d);
    return;
   
 /*-------------------------------------------------------------------*/
 
     case CEDIT_OPERATION_OPTIONS_MENU:
       switch (*arg) {
         case 'a':
         case 'A':
           write_to_output(d, "Enter the default port number : ");
           OLC_MODE(d) = CEDIT_DFLT_PORT;
           return;
           
         case 'b':
         case 'B':
           write_to_output(d, "Enter the default IP Address : ");
           OLC_MODE(d) = CEDIT_DFLT_IP;
           return;
         
         case 'c':
         case 'C':
           write_to_output(d, "Enter the default directory : ");
           OLC_MODE(d) = CEDIT_DFLT_DIR;
           return;
         
         case 'd':
         case 'D':
           write_to_output(d, "Enter the name of the logfile : ");
           OLC_MODE(d) = CEDIT_LOGNAME;
           return;
         
         case 'e':
         case 'E':
           write_to_output(d, "Enter the maximum number of players : ");
           OLC_MODE(d) = CEDIT_MAX_PLAYING;
           return;
         
         case 'f':
         case 'F':
           write_to_output(d, "Enter the maximum size of the logs : ");
           OLC_MODE(d) = CEDIT_MAX_FILESIZE;
           return;
         
         case 'g':
         case 'G':
           write_to_output(d, "Enter the maximum number of password attempts : ");
           OLC_MODE(d) = CEDIT_MAX_BAD_PWS;
           return;
         
         case 'h':
         case 'H':
           TOGGLE_VAR(OLC_CONFIG(d)->operation.siteok_everyone);
           break;
       
         case 'i':
         case 'I':
           TOGGLE_VAR(OLC_CONFIG(d)->operation.nameserver_is_slow);
           break;
         
         case 'j':
         case 'J':
           TOGGLE_VAR(OLC_CONFIG(d)->operation.use_new_socials);
           send_to_char(d->character, 
              "Please note that using the stock social file will disable AEDIT.\r\n");
           break;

         case 'k':
         case 'K':
           TOGGLE_VAR(OLC_CONFIG(d)->operation.auto_save_olc);
           break;

         case 'l':
         case 'L':
           OLC_MODE(d) = CEDIT_MENU;
           clear_screen(d);
           send_editor_help(d);
           write_to_output(d, "Enter the new MENU :\r\n\r\n");
           
           if (OLC_CONFIG(d)->operation.MENU) {
             write_to_output(d, "%s", OLC_CONFIG(d)->operation.MENU);
             oldtext = strdup(OLC_CONFIG(d)->operation.MENU);
           }
           
           string_write(d, &OLC_CONFIG(d)->operation.MENU, MAX_INPUT_LENGTH, 0, oldtext);
           return;
         
         case 'm':
         case 'M':
           OLC_MODE(d) = CEDIT_WELC_MESSG;
           clear_screen(d);
           send_editor_help(d);
           write_to_output(d, "Enter the new welcome message :\r\n\r\n");
           
           if (OLC_CONFIG(d)->operation.WELC_MESSG) {
             write_to_output(d, "%s", OLC_CONFIG(d)->operation.WELC_MESSG);
             oldtext = str_udup(OLC_CONFIG(d)->operation.WELC_MESSG);
           }
           
           string_write(d, &OLC_CONFIG(d)->operation.WELC_MESSG, MAX_INPUT_LENGTH, 0, oldtext);
           return;
         
         case 'n':
         case 'N':
           OLC_MODE(d) = CEDIT_START_MESSG;
           clear_screen(d);
           send_editor_help(d);
           write_to_output(d, "Enter the new newbie start message :\r\n\r\n");
           
           if (OLC_CONFIG(d)->operation.START_MESSG) {
             write_to_output(d, "%s", OLC_CONFIG(d)->operation.START_MESSG);
             oldtext = strdup(OLC_CONFIG(d)->operation.START_MESSG);
           }
           
           string_write(d, &OLC_CONFIG(d)->operation.START_MESSG, MAX_INPUT_LENGTH, 0, oldtext);
           return;
         
         case 'q':
         case 'Q':
           cedit_disp_menu(d);
           return;
         
         default:
           write_to_output(d, "\r\nThat is an invalid choice!\r\n");
      }
   
   cedit_disp_operation_options(d);
   return;
   
 /*-------------------------------------------------------------------*/
 
    case CEDIT_AUTOWIZ_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          TOGGLE_VAR(OLC_CONFIG(d)->autowiz.use_autowiz);
          break;
        
        case 'b':
        case 'B':
          write_to_output(d, "Enter the minimum level for players to appear on the wizlist : ");
          OLC_MODE(d) = CEDIT_MIN_WIZLIST_LEV;
          return;
        
        case 'q':
        case 'Q':
          cedit_disp_menu(d);
          return;
        
        default:
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
      }
      
      cedit_disp_autowiz_options(d);
      return;
    
 /*-------------------------------------------------------------------*/

    case CEDIT_ADVANCE_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          TOGGLE_VAR(OLC_CONFIG(d)->advance.allow_multiclass);
          break;
        
        case 'q':
        case 'Q':
          cedit_disp_menu(d);
          return;
        
        default:
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
      }
      
      cedit_disp_advance_options(d);
      return;
    
 /*------------------------------------------------------------------*/

    case CEDIT_TICKS_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          write_to_output(d, "Enter the Speed of the violence system (1 - 10) 10 = Slowest : ");
          OLC_MODE(d) = CEDIT_PULSE_VIOLENCE;
          return;

        case 'b':
        case 'B':
          write_to_output(d, "Enter the Speed of the Mobile Actions (1 - 20) 20 = Slowest : ");
          OLC_MODE(d) = CEDIT_PULSE_MOBILE;
          return;

        case 'c':
        case 'C':
          write_to_output(d, "Enter Zone update time : ");
          OLC_MODE(d) = CEDIT_PULSE_ZONE;
          return;

        case 'd':
        case 'D':
          write_to_output(d, "Enter time for autosaving : ");
          OLC_MODE(d) = CEDIT_PULSE_AUTOSAVE;
          return;

        case 'e':
        case 'E':
          write_to_output(d, "Enter the time to kill connection waiting for password : ");
          OLC_MODE(d) = CEDIT_PULSE_IDLEPWD;
          return;

        case 'f':
        case 'F':
          write_to_output(d, "Enter the maximum size of the logs : ");
          OLC_MODE(d) = CEDIT_PULSE_SANITY;
          return;

        case 'g':
        case 'G':
          write_to_output(d, "Enter the maximum number of password attempts : ");
          OLC_MODE(d) = CEDIT_PULSE_USAGE;
          return;

        case 'h':
        case 'H':
          write_to_output(d, "Enter timesave : ");
          OLC_MODE(d) = CEDIT_PULSE_TIMESAVE;
          return;

        case 'i':
        case 'I':
          write_to_output(d, "Enter Current update time : ");
          OLC_MODE(d) = CEDIT_PULSE_CURRENT;
          return;

       case 'q':
       case 'Q':
         cedit_disp_menu(d);
         return;

       default:
         write_to_output(d, "\r\nThat is an invalid choice!\r\n");
     }


    cedit_disp_ticks_menu(d);
    return;

 /*-------------------------------------------------------------------*/

    case CEDIT_LEVEL_CAN_SHOUT:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the minimum level a player must be to shout, gossip, etc : ");
      } else {
        OLC_CONFIG(d)->play.level_can_shout = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_HOLLER_MOVE_COST:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the amount it costs (in move points) to holler : ");
      } else {
        OLC_CONFIG(d)->play.holler_move_cost = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_TUNNEL_SIZE:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the maximum number of people allowed in a tunnel : ");
      } else {
        OLC_CONFIG(d)->play.tunnel_size = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_MAX_EXP_GAIN:
      if (*arg)
        OLC_CONFIG(d)->play.max_exp_gain = atoi(arg);
      
      cedit_disp_game_play_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAX_EXP_LOSS:
      if (*arg)
        OLC_CONFIG(d)->play.max_exp_loss = atoi(arg);
      
      cedit_disp_game_play_options(d);
      break;


/*-------------------------------------------------------------------*/

    case CEDIT_MAX_NPC_CORPSE_TIME:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the number of tics before NPC corpses decompose : ");
      } else {
        OLC_CONFIG(d)->play.max_npc_corpse_time = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAX_PC_CORPSE_TIME:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the number of tics before PC corpses decompose : ");
        } else {
          OLC_CONFIG(d)->play.max_pc_corpse_time = atoi(arg);
          cedit_disp_game_play_options(d);
        }
        break;

/*-------------------------------------------------------------------*/

    case CEDIT_IDLE_VOID:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the number of tics before PC's are sent to the void (idle) : ");
      } else {
        OLC_CONFIG(d)->play.idle_void = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_IDLE_RENT_TIME:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the number of tics before PC's are automatically rented and forced to quit : ");
      } else {
        OLC_CONFIG(d)->play.idle_rent_time = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/
    
    case CEDIT_IDLE_MAX_LEVEL:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the level a player must be to become immune to IDLE : ");
      } else {
        OLC_CONFIG(d)->play.idle_max_level = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_LEVEL_CAP:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the level a player cannot reach : ");
      } else {
        OLC_CONFIG(d)->play.level_cap = atoi(arg);
        cedit_disp_game_play_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_OK:
      if (!genolc_checkstring(d, arg))
        break;
          
      if (OLC_CONFIG(d)->play.OK)
        free(OLC_CONFIG(d)->play.OK);
          
      OLC_CONFIG(d)->play.OK = str_udup(arg);
      strcat(OLC_CONFIG(d)->play.OK, "\r\n");
      
      cedit_disp_game_play_options(d);
      break;
      
/*-------------------------------------------------------------------*/
        
    case CEDIT_NOPERSON:
      if (!genolc_checkstring(d, arg))
        break;
          
      if (OLC_CONFIG(d)->play.NOPERSON)
        free(OLC_CONFIG(d)->play.NOPERSON);
          
      OLC_CONFIG(d)->play.NOPERSON = str_udup(arg);
      strcat(OLC_CONFIG(d)->play.NOPERSON, "\r\n");
      
      cedit_disp_game_play_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_NOEFFECT:
      if (!genolc_checkstring(d, arg))
        break;
          
      if (OLC_CONFIG(d)->play.NOEFFECT)
        free(OLC_CONFIG(d)->play.NOEFFECT);
          
      OLC_CONFIG(d)->play.NOEFFECT = str_udup(arg);
      strcat(OLC_CONFIG(d)->play.NOEFFECT, "\r\n");
      
      cedit_disp_game_play_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAP_OPTION:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Select 1, 2 or 3 (0 to cancel) :");
      } else {
        if ((atoi(arg) >= 1) && (atoi(arg) <= 3))
          OLC_CONFIG(d)->play.map_option = (atoi(arg) - 1);
        cedit_disp_game_play_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAP_SIZE:
      if (!*arg) {
		/* User just pressed return - restore to default */
        OLC_CONFIG(d)->play.map_size = 6;
        cedit_disp_game_play_options(d);
      } else {
        OLC_CONFIG(d)->play.map_size = MIN(MAX((atoi(arg)), 1), 12);
        cedit_disp_game_play_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MINIMAP_SIZE:
      if (!*arg) {
		/* User just pressed return - restore to default */
        OLC_CONFIG(d)->play.minimap_size = 2;
        cedit_disp_game_play_options(d);
      } else {
        OLC_CONFIG(d)->play.minimap_size = MIN(MAX((atoi(arg)), 1), 12);
        cedit_disp_game_play_options(d);
      }
      break;


/*-------------------------------------------------------------------*/

    case CEDIT_MAX_OBJ_SAVE:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the maximum objects a player can save : ");
        } else {
          OLC_CONFIG(d)->csd.max_obj_save = atoi(arg);
          cedit_disp_crash_save_options(d);
        }
        break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_MIN_RENT_COST:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the minimum amount it costs to rent : ");
      } else {
        OLC_CONFIG(d)->csd.min_rent_cost = atoi(arg);
        cedit_disp_crash_save_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_AUTOSAVE_TIME:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the interval for player's being autosaved : ");
      } else {
        OLC_CONFIG(d)->csd.autosave_time = atoi(arg);
        cedit_disp_crash_save_options(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_CRASH_FILE_TIMEOUT:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the lifetime of crash and idlesave files (days) : ");
      } else {
        OLC_CONFIG(d)->csd.crash_file_timeout = atoi(arg);
        cedit_disp_crash_save_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_RENT_FILE_TIMEOUT:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the lifetime of rent files (days) : ");
      } else {
        OLC_CONFIG(d)->csd.rent_file_timeout = atoi(arg);
        cedit_disp_crash_save_options(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_MORTAL_START_ROOM:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the room's vnum where mortals should load into : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the room's vnum where mortals should load into : ");
      } else {
        OLC_CONFIG(d)->room_nums.mortal_start_room = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;
    
/*-------------------------------------------------------------------*/

    case CEDIT_IMMORT_START_ROOM:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the room's vnum where immortals should load into : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the room's vnum where immortals should load into : ");
      } else {
        OLC_CONFIG(d)->room_nums.immort_start_room = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;
      
/*-------------------------------------------------------------------*/

    case CEDIT_FROZEN_START_ROOM:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the room's vnum where frozen people should load into : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the room's vnum where frozen people should load into : ");
      } else {
        OLC_CONFIG(d)->room_nums.frozen_start_room = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_DONATION_ROOM_1:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the vnum for donation room #1 : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the vnum for donation room #1 : ");
      } else {
        OLC_CONFIG(d)->room_nums.donation_room_1 = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_DONATION_ROOM_2:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the vnum for donation room #2 : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the vnum for donation room #2 : ");
      } else {
        OLC_CONFIG(d)->room_nums.donation_room_2 = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_DONATION_ROOM_3:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the vnum for donation room #3 : ");
      } else if (real_room(atoi(arg)) == NOWHERE) {
        write_to_output(d,
          "That room doesn't exist!\r\n"
          "Enter the vnum for donation room #3 : ");
      } else {
        OLC_CONFIG(d)->room_nums.donation_room_3 = atoi(arg);
        cedit_disp_room_numbers(d);
      }
      break;
      
/*-------------------------------------------------------------------*/
    
    case CEDIT_DFLT_PORT:
      OLC_CONFIG(d)->operation.DFLT_PORT = atoi(arg);
      cedit_disp_operation_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_DFLT_IP:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the default ip address : ");
      } else {
        OLC_CONFIG(d)->operation.DFLT_IP = str_udup(arg);
        cedit_disp_operation_options(d);
      }
      break;

/*-------------------------------------------------------------------*/
    
    case CEDIT_DFLT_DIR:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the default directory : ");
      } else {
        OLC_CONFIG(d)->operation.DFLT_DIR = str_udup(arg);
        cedit_disp_operation_options(d);
      }
      break;
    
/*-------------------------------------------------------------------*/

    case CEDIT_LOGNAME:
      if (!*arg) {
        write_to_output(d,
          "That is an invalid choice!\r\n"
          "Enter the name of the logfile : ");
      } else {
        OLC_CONFIG(d)->operation.LOGNAME = str_udup(arg);
        cedit_disp_operation_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAX_PLAYING:
      OLC_CONFIG(d)->operation.max_playing = atoi(arg);
      cedit_disp_operation_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAX_FILESIZE:
      OLC_CONFIG(d)->operation.max_filesize = atoi(arg);
      cedit_disp_operation_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MAX_BAD_PWS:
      OLC_CONFIG(d)->operation.max_bad_pws = atoi(arg);
      cedit_disp_operation_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_MIN_WIZLIST_LEV:
      if (atoi(arg) > ADMLVL_IMPL) {
        write_to_output(d, 
          "The minimum wizlist level can't be greater than %d.\r\n"
          "Enter the minimum level for players to appear on the wizlist : ", ADMLVL_IMPL);
      } else {
        OLC_CONFIG(d)->autowiz.min_wizlist_lev = atoi(arg);
        cedit_disp_autowiz_options(d);
      }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_PULSE_VIOLENCE:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 10){
        write_to_output(d, "Please enter a number between 0 - 10.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_violence = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_MOBILE:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 20){
        write_to_output(d, "Please enter a number between 0 - 20.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_mobile = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_IDLEPWD:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 30){
        write_to_output(d, "Please enter a number between 0 - 30.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_idlepwd = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_ZONE:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 20){
        write_to_output(d, "Please enter a number between 0 - 20.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_zone = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_AUTOSAVE:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 100){
        write_to_output(d, "Please enter a number between 0 - 100.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_autosave = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_CURRENT:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 30){
        write_to_output(d, "Please enter a number between 0 - 30.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_current = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_SANITY:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 50){
        write_to_output(d, "Please enter a number between 0 - 50.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_sanity = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/
    case CEDIT_PULSE_USAGE:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 10){
        write_to_output(d, "Please enter a number between 0 - 10.\r\n");
        cedit_disp_ticks_menu(d);
      } else {
      OLC_CONFIG(d)->ticks.pulse_usage = atoi(arg);
      cedit_disp_ticks_menu(d);
     }
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_EXP_MULTIPLIER:
      if (*arg)
        OLC_CONFIG(d)->play.exp_multiplier = atof(arg);
      
      cedit_disp_game_play_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_CAMPAIGN:
      if (*arg)
        OLC_CONFIG(d)->play.campaign = atoi(arg);
      
      cedit_disp_game_play_options(d);
      break;

/*-------------------------------------------------------------------*/

    case CEDIT_CREATION_OPTIONS_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          cedit_disp_creation_menu(d);
          OLC_MODE(d) = CEDIT_CREATION_MENU;
          break;

        case 'b':
        case 'B':
	  TOGGLE_VAR(OLC_CONFIG(d)->play.reroll_player);
          cedit_disp_creation_options(d);
	  break;
        case 'q':
        case 'Q':
          cedit_disp_menu(d);
          return;
        
        default:
          write_to_output(d, "\r\nThat is an invalid choice!\r\n");
          cedit_disp_creation_menu(d);
      }
      return;
    
 /*------------------------------------------------------------------*/

    case CEDIT_CREATION_MENU:
      if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > NUM_CREATION_METHODS -1){
        write_to_output(d, "Please enter a number between 0 - %d.\r\n", NUM_CREATION_METHODS -1);
        cedit_disp_creation_menu(d);
      } else {
      OLC_CONFIG(d)->creation.method = atoi(arg);
      //OLC_CONFIG(d)->ticks.pulse_usage = atoi(arg);
      cedit_disp_creation_options(d);
      }
      break;

 /*------------------------------------------------------------------*/

    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_CONFIG);
      mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: OLC: cedit_parse(): Reached default case!");
      write_to_output(d, "Oops...\r\n");
      break;
  }
}

/*
 * End of parse_cedit()  
 */
void reassign_rooms(void)
{ 
  void assign_rooms(void);
  int i;

  /* remove old funcs */
  for (i = 0; i < top_of_world; i++)
    world[i].func = NULL;
        
  /* reassign spec_procs */
  assign_rooms();
}

void cedit_string_cleanup(struct descriptor_data *d, int terminator)
{
    switch (OLC_MODE(d)) 
    {
        case CEDIT_MENU:
        case CEDIT_WELC_MESSG:
        case CEDIT_START_MESSG:
        cedit_disp_operation_options(d);
        break;
    }
}
