/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "protocol.h"

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  for instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD  0x030100 /* Major/Minor/Patchlevel - MMmmPP */
/*
 * if you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ  1 /* TRUE/FALSE aren't defined yet. */

/* CWG Version String */

#define CWG_VERSION "CWG Rasputin - 1.2.28b"

/* Only one campaign should ever be defined at any given time.  Having more than one of
*    these set will prevent your mud from operating correctly.
*/

#define CAMPAIGN_NONE             0  // Do not use.  Mostly for record keeping
#define CAMPAIGN_FORGOTTEN_REALMS 1
#define CAMPAIGN_DRAGONLANCE      2
#define CAMPAIGN_GOLARION         3

#define NUM_CAMPAIGNS 3

#define CAMPAIGN_STAR_WARS 99

// MySQL Stuff
#define MYSQL_SERVER      "mud.themudhost.net"
// #define MYSQL_SERVER      "localhost"
#define MYSQL_DB          "uriel_sql"
#define MYSQL_PASSWD      "pepsic0l@"
#define MYSQL_USER        "uriel"

typedef int ch_ret;
typedef int obj_ret;

/* preamble *************************************************************/
/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: this will likely be unconditionally unsigned later.
 */
#define CIRCLE_UNSIGNED_INDEX 1 /* 0 = signed, 1 = unsigned */
#if CIRCLE_UNSIGNED_INDEX
# define IDXTYPE  ush_int
# define NOWHERE  ((IDXTYPE)~0)
# define NOTHING  ((IDXTYPE)~0)
# define NOBODY   ((IDXTYPE)~0)
#else
# define IDXTYPE  sh_int
# define NOWHERE  (-1)  /* nil reference for rooms  */
# define NOTHING  (-1)  /* nil reference for objects  */
# define NOBODY   (-1)  /* nil reference for mobiles  */
#endif
#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)
#define SG_MIN    2 /* Skill gain check must be less than this
           number in order to be successful.
           IE: 1% of a skill gain */
/* room-related defines *************************************************/
/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NORTHWEST      6
#define NORTHEAST      7
#define SOUTHEAST      8
#define SOUTHWEST      9
#define INDIR         10
#define OUTDIR        11

#define ALIGNMENT_GOOD    500
#define ALIGNMENT_NEUTRAL 0
#define ALIGNMENT_EVIL    -500

#define ETHOS_LAWFUL    500
#define ETHOS_NEUTRAL   0
#define ETHOS_CHAOTIC   -500

#define NUM_ALIGNMENTS    9 /* (# ALIGNMENTS) * (# ETHOS) */

#define MAX_ASSM 20

#define NUM_RULES 12

#define MAX_NUM_KNOWN_SPELLS 500

/*******************  Config macros *********************/
extern struct config_data config_info;

#define CONFIG_CONFFILE         config_info.CONFFILE

#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
#define CONFIG_IDLE_VOID        config_info.play.idle_void
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
#define CONFIG_LEVEL_CAP	config_info.play.level_cap
#define CONFIG_STACK_MOBS	config_info.play.stack_mobs
#define CONFIG_STACK_OBJS	config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING	config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION	config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL	config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION	config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES	config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE	config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER	config_info.play.exp_multiplier
/* Map/Automap options */
#define CONFIG_MAP             config_info.play.map_option
#define CONFIG_MAP_SIZE        config_info.play.map_size
#define CONFIG_MINIMAP_SIZE    config_info.play.minimap_size
#define CONFIG_CAMPAIGN         config_info.play.campaign


  /** Crash Saves **/
#define CONFIG_FREE_RENT        config_info.csd.free_rent
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

  /** Room Numbers **/
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3

  /** Game Operation **/
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
#define CONFIG_MENU             config_info.operation.MENU
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
#define CONFIG_START_MESSG      config_info.operation.START_MESSG

  /** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

  /** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS	config_info.advance.allow_multiclass

  /** For tick system **/
#define CONFIG_PULSE_VIOLENCE	config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE	config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE	config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE	config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD	config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY	config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE	config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE	config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT	config_info.ticks.pulse_current

  /** Character Creation Method **/
#define CONFIG_CREATION_METHOD	config_info.creation.method

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(0)   /* Dark			*/
#define ROOM_DEATH		(1)   /* Death trap		*/
#define ROOM_NOMOB		(2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(3)   /* Indoors			*/
#define ROOM_PEACEFUL		(4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		(9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		(13)  /* (R) The door to a house	*/
#define ROOM_OLC		(14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(15)  /* (R) breath-first srch mrk	*/
#define ROOM_PKILL              (16)
#define ROOM_NORECALL           (17)
#define ROOM_GUARDED            (18)
#define ROOM_PULSE_DAMAGE       (19)
#define ROOM_NO_OOC		(20)   /* NO ooc communication     */
#define ROOM_SALTWATER_FISH     (21)  /* Player can fish here     */
#define ROOM_FRESHWATER_FISH    (22)  /* Player can fish here too */
#define ROOM_NODIG		(23)
#define ROOM_NOBURY             (24)
#define ROOM_SPEC               (25)
#define ROOM_PLAYER_SHOP        (26)
#define ROOM_NOVEHICLE           27  /* Requires a vehicle to pass       */
#define ROOM_UNDERGROUND         28  /* Room is below ground      */
#define ROOM_CURRENT             29  /* Room move with random currents */
#define ROOM_TIMED_DT            30  /* Room has a timed death trap    */
#define ROOM_WORLDMAP            31   /* World-map style maps here */
#define ROOM_MINE                32  // can mine ores for crafting
#define ROOM_MINE_ABUNDANT       33  // mining roll +10
#define ROOM_MINE_RICH           34  // mining roll +25
#define ROOM_TAVERN              35 // healing bonus and exp bonus
#define ROOM_FOREST              36  // can forest wood for crafting
#define ROOM_FOREST_ABUNDANT     37  // foresting roll +10
#define ROOM_FOREST_RICH         38  // foresting roll +25
#define ROOM_FARM                39  // can farm cloth & food for crafting
#define ROOM_FARM_ABUNDANT       40  // farming roll +10
#define ROOM_FARM_RICH           41  // farming roll +25
#define ROOM_HUNTING             42  // can hunt food and hides for crafting
#define ROOM_HUNTING_ABUNDANT    43  // hunting roll +10
#define ROOM_HUNTING_RICH        44  // hunting roll +25
#define ROOM_WEAPON_MARKET       45
#define ROOM_ARMOR_MARKET        46
#define ROOM_TRINKETS_MARKET     47
#define ROOM_STIMS_MARKET        48


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR   (1 << 0)   /* Exit is a door    */
#define EX_CLOSED   (1 << 1)   /* The door is closed  */
#define EX_LOCKED   (1 << 2)   /* The door is locked  */
#define EX_PICKPROOF    (1 << 3)   /* Lock can't be picked  */
#define EX_SECRET   (1 << 4)   /* The door is hidden        */
#define EX_PICKED   (1 << 5)   // The door has been picked
#define NUM_EXIT_FLAGS 6

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0       /* Indoors     */
#define SECT_CITY            1       /* In a city     */
#define SECT_FIELD           2       /* In a field    */
#define SECT_FOREST          3       /* In a forest   */
#define SECT_HILLS           4       /* In the hills    */
#define SECT_MOUNTAIN        5       /* On a mountain   */
#define SECT_WATER_SWIM      6       /* Swimmable water   */
#define SECT_WATER_NOSWIM    7       /* Water - need a boat */
#define SECT_UNDERWATER      8       /* Underwater    */
#define SECT_FLYING          9       /* Wheee!      */
#define SECT_ROAD            10      // Any road
#define SECT_CAVE            11      // Any Cave
#define SECT_DESERT          12      // Desert

/* char and mob-related defines *****************************************/
/* PC classes */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */

#define CLASS_UNDEFINED               -1

#define CLASS_JEDI			0  // changed to force adept, left in for legacy support
#define CLASS_FORCE_ADEPT               0
#define CLASS_NOBLE			1
#define CLASS_SQUAD_LEADER              1
#define CLASS_SCOUNDREL			2
#define CLASS_SCOUT			3
#define CLASS_SOLDIER			4
#define CLASS_TRADER			5
#define CLASS_JEDI_KNIGHT		6
#define CLASS_SITH_APPRENTICE		7
#define CLASS_GUNSLINGER		8
#define CLASS_BOUNTY_HUNTER		9
#define CLASS_FORCE_DISCIPLE		10
#define CLASS_TERAS_KASI_MASTER         11
#define CLASS_SITH_SORCERER             12
#define CLASS_SITH_WARRIOR              13
#define CLASS_JEDI_SAGE                 14
#define CLASS_JEDI_WARDEN               15
#define CLASS_SITH_LORD                 16
#define CLASS_JEDI_MASTER               17

#define CLASS_WIZARD                   0
#define CLASS_CLERIC                   1
#define CLASS_ROGUE                    2
#define CLASS_FIGHTER                  3
#define CLASS_MONK                     4
#define CLASS_PALADIN                  5
#define CLASS_BARBARIAN                6
#define CLASS_BARD                     7
#define CLASS_RANGER                   8
#define CLASS_DRUID                    9
#define CLASS_PURPLE_DRAGON_KNIGHT     10 /* NOW AN EXPANSION*/
#define CLASS_KNIGHT                   10 /* NOW AN EXPANSION*/
#define CLASS_KNIGHT_OF_THE_CROWN      10 /* NOW AN EXPANSION*/
#define CLASS_PURPLE_DRAGON_TEMPLAR    11
#define CLASS_TEMPLAR                  11
#define CLASS_KNIGHT_OF_THE_SWORD      11
#define CLASS_PURPLE_DRAGON_LORD       12
#define CLASS_LORD                     12
#define CLASS_CHAMPION                 12
#define CLASS_KNIGHT_OF_THE_ROSE       12
#define CLASS_ZHENTARIM_KNIGHT         13
#define CLASS_KNIGHT_OF_THE_LILY       13
#define CLASS_ZHENTARIM_PRIEST         14
#define CLASS_KNIGHT_OF_THE_SKULL      14
#define CLASS_DRAGON_PRIEST            14
#define CLASS_ZHENTARIM_WIZARD         15
#define CLASS_KNIGHT_OF_THE_THORN      15
#define CLASS_EXPANSION_ONE_A          16
#define CLASS_WIZARD_OF_HIGH_SORCERY   16
#define CLASS_DUELIST                  17
#define CLASS_GLADIATOR                18
#define CLASS_MYSTIC_THEURGE           19
#define CLASS_SORCERER                 20
#define CLASS_CLASSLESS                21
#define CLASS_DWARVEN_DEFENDER         22
#define CLASS_WEAPON_MASTER            23
#define CLASS_DRAGON_DISCIPLE          24
#define CLASS_ARCANE_ARCHER            25
#define CLASS_INVISIBLE_BLADE          26
#define CLASS_NPC_EXPERT               27
#define CLASS_NPC_ADEPT                27
#define CLASS_NPC_COMMONER             27
#define CLASS_NPC_ARISTOCRAT           27
#define CLASS_NPC_WARRIOR              27
#define CLASS_ASSASSIN                 28
#define CLASS_BLACKGUARD               29
#define CLASS_ARTISAN                  30
#define CLASS_ARCANE_TRICKSTER         31
#define CLASS_FAVORED_SOUL             32
#define CLASS_ELDRITCH_KNIGHT          33
#define CLASS_DEATH_MASTER             34
#define CLASS_SACRED_FIST              35
#define CLASS_DRAGON_RIDER             36
#define CLASS_EXTRA_J                  37
#define CLASS_EXTRA_K                  38
#define CLASS_EXTRA_L                  39
#define CLASS_EXTRA_M                  40
#define CLASS_EXTRA_N                  41
#define CLASS_EXTRA_O                  42
#define CLASS_EXTRA_P                  43
#define CLASS_EXTRA_Q                  44
#define CLASS_EXTRA_R                  45
#define CLASS_EXTRA_S                  46
#define CLASS_EXTRA_T                  47
#define CLASS_EXTRA_U                  48
#define CLASS_EXTRA_V                  49
#define CLASS_EXTRA_W                  50
#define CLASS_EXTRA_X                  51
#define CLASS_EXTRA_Y                  52
#define CLASS_EXTRA_Z                  53
#define CLASS_EXTRA_AA                 54
#define CLASS_EXTRA_BB                 55

#define NUM_CLASSES                 58  /* This must be the number of classes!! */
#define TOP_PC_CLASS                (CLASS_DRAGON_RIDER + 1)
#define CLASS_PROGRESSION	       54

/* Clan ranks */
#define CLAN_UNDEFINED     -1
#define CLAN_NONE           0

#define NUM_CLAN_RANKS      6 /* 0,1,2... leader is top number */
#define CLAN_LEADER         (NUM_CLAN_RANKS-1)
#define CLAN_ADVISOR        (CLAN_LEADER-1)
#define MAX_CLAN_APPLICANTS 20
#define NUM_CLAN_GUARDS     2

#define NUM_STARTROOMS      3

#define RACE_UNDEFINED             -1
#define RACE_SPIRIT                 0
#define RACE_HUMAN                  1
#define RACE_DWARF                  2
#define RACE_ELF                    3
#define RACE_HALFLING               4
#define RACE_GNOME                  5
#define RACE_HALF_ELF               6
#define RACE_HALF_ORC               7
#define RACE_ZOMBIE                 8
#define RACE_ANIMAL                 9
#define RACE_WOLF                   10
#define RACE_GREAT_CAT              11
#define RACE_MONSTROUS_HUMANOID     12
#define RACE_GIANT                  13
#define RACE_MANDRAGORA             14
#define RACE_PLANT                  14
#define RACE_OOZE                   15
#define RACE_ELEMENTAL              16
#define RACE_OUTSIDER               17
#define RACE_MAGICAL_BEAST          18
#define RACE_STIRGE                 18
#define RACE_TINY_BEAST             18
#define RACE_SMALL_BEAST            19
#define RACE_MEDIUM_BEAST           20
#define RACE_LARGE_BEAST            21
#define RACE_HUGE_BEAST             22
#define RACE_COLOSSAL_BEAST         23
#define RACE_GARGANTUAN_BEAST       24
#define RACE_UNDEAD                 25
#define RACE_SKELETON               26
#define RACE_EXPANSION_1            27
#define RACE_EXPANSION_2            28
#define RACE_EXPANSION_3            29
#define RACE_MINOTAUR               30
#define RACE_EXPANSION_4            31
#define RACE_EXPANSION_5            32
#define RACE_EXPANSION_6            33
#define RACE_EXPANSION_7            34
#define RACE_EXPANSION_8            35
#define RACE_EXPANSION_10           36
#define RACE_EXPANSION_11           37
#define RACE_EXPANSION_12           38
#define RACE_RAT                    39
#define RACE_HORSE                  40
#define RACE_EXPANSION_13           41
#define RACE_HUMAN_NORTHERNER       42 // pc forgotten realms
#define RACE_HUMAN_ICE_BARBARIAN    43 // pc forgotten realms
#define RACE_HUMAN_CORMYR           44 // pc forgotten realms
#define RACE_HUMAN_AMN              45
#define RACE_HUMAN_ANAUROCH         46
#define RACE_HUMAN_CHULT            47
#define RACE_HUMAN_DALELANDS        48
#define RACE_HUMAN_HORDELANDS       49
#define RACE_HUMAN_LANTAN           50
#define RACE_HUMAN_MULHORAND        51
#define RACE_HUMAN_RASHEMAN         52
#define RACE_HUMAN_SEMBIA           53
#define RACE_HUMAN_TETHYR           54
#define RACE_HUMAN_THAY             55
#define RACE_HUMAN_WATERDEEP        56
#define RACE_MOON_ELF               57 // pc forgotten realms
#define RACE_DROW_ELF               58
#define RACE_SUN_ELF                59
#define RACE_WILD_ELF               60
#define RACE_WOOD_ELF               61
#define RACE_HALF_DROW              62
#define RACE_SHIELD_DWARF           67 // pc forgotten realms
#define RACE_GOLD_DWARF             68
#define RACE_GRAY_DWARF             69
#define RACE_GOBLIN                 70
#define RACE_LIGHTFOOT_HALFLING     77 // pc forgotten realms
#define RACE_GHOSTWISE_HALFLING     78 // pc forgotten realms
#define RACE_STRONGHEART_HALFLING   79 // pc forgotten realms
#define RACE_ROCK_GNOME             82 // pc forgotten realms
#define RACE_DEEP_GNOME             83
#define RACE_EXPANSION_14           84
#define RACE_AASIMAR                85
#define RACE_TIEFLING               86
#define RACE_AIR_GENESI             87
#define RACE_EARTH_GENESI           88
#define RACE_FIRE_GENESI            89
#define RACE_WATER_GENESI           90
#define RACE_OGRE                   91
#define RACE_HALF_OGRE              92
#define RACE_ORC                    93
#define RACE_CENTAUR                99
#define RACE_CONSTRUCT              100
#define RACE_DRAGON                 101
#define RACE_EXPANSION_15           102
#define RACE_EXPANSION_16           103
#define RACE_EXPANSION_17           104
#define RACE_HALF_DRAGON            105
#define RACE_TROLL                  106
#define RACE_DINOSAUR               107
#define RACE_FEY                    108
#define RACE_MEDIUM_FIRE_ELEMENTAL  109
#define RACE_MEDIUM_EARTH_ELEMENTAL 110
#define RACE_MEDIUM_AIR_ELEMENTAL   111
#define RACE_MEDIUM_WATER_ELEMENTAL 112
#define RACE_FIRE_ELEMENTAL         109
#define RACE_EARTH_ELEMENTAL        110
#define RACE_AIR_ELEMENTAL          111
#define RACE_WATER_ELEMENTAL        112
#define RACE_HUGE_FIRE_ELEMENTAL    113
#define RACE_HUGE_EARTH_ELEMENTAL   114
#define RACE_HUGE_AIR_ELEMENTAL     115
#define RACE_HUGE_WATER_ELEMENTAL   116
#define RACE_APE                    117
#define RACE_BOAR                   118
#define RACE_CHEETAH                119
#define RACE_CROCODILE              120
#define RACE_GIANT_CROCODILE        121
#define RACE_HYENA                  122
#define RACE_LEOPARD                123
#define RACE_RHINOCEROS             124
#define RACE_WOLVERINE              125
#define RACE_MEDIUM_VIPER           126
#define RACE_LARGE_VIPER            127
#define RACE_HUGE_VIPER             128
#define RACE_CONSTRICTOR_SNAKE      129
#define RACE_GIANT_CONSTRICTOR_SNAKE 130
#define RACE_TIGER                  131
#define RACE_BLACK_BEAR             132
#define RACE_BROWN_BEAR             133
#define RACE_POLAR_BEAR             134
#define RACE_LION                   135
#define RACE_ELEPHANT               136
#define RACE_EAGLE                  137
#define RACE_GHOUL                  138
#define RACE_GHAST                  139
#define RACE_MUMMY                  140
#define RACE_MOHRG                  141
#define RACE_SMALL_FIRE_ELEMENTAL    142
#define RACE_SMALL_EARTH_ELEMENTAL   143
#define RACE_SMALL_AIR_ELEMENTAL     144
#define RACE_SMALL_WATER_ELEMENTAL   145
#define RACE_LARGE_FIRE_ELEMENTAL    146
#define RACE_LARGE_EARTH_ELEMENTAL   147
#define RACE_LARGE_AIR_ELEMENTAL     148
#define RACE_LARGE_WATER_ELEMENTAL   149
#define RACE_BLINK_DOG               150
#define RACE_SHOCKER_LIZARD          151
#define RACE_OWLBEAR                 152
#define RACE_SHAMBLING_MOUND         153
#define RACE_TREANT                  154
#define RACE_MYCANOID                155
#define RACE_MIRALUKA                156
#define RACE_RATTATAKI               157
#define RACE_DUERGAR                RACE_GRAY_DWARF
#define RACE_SVIRFNEBLIN            RACE_DEEP_GNOME

// 1 higher than the last race define

#define NUM_RACES 158


#define RACE_TYPE_UNDEFINED          0
#define RACE_TYPE_HUMANOID           1
#define RACE_TYPE_DROID              2
#define RACE_TYPE_BEAST              3

#define RACE_TYPE_HUMAN              1
#define RACE_TYPE_ELF                2
#define RACE_TYPE_DWARF              3
#define RACE_TYPE_HALFLING           4
#define RACE_TYPE_KENDER             4
#define RACE_TYPE_GNOME              5
#define RACE_TYPE_ANIMAL             6
#define RACE_TYPE_UNDEAD             7
#define RACE_TYPE_MONSTROUS_HUMANOID 8
#define RACE_TYPE_GIANT              9
#define RACE_TYPE_PLANT              10
#define RACE_TYPE_OOZE               11
#define RACE_TYPE_ELEMENTAL          12
#define RACE_TYPE_OUTSIDER           13
#define RACE_TYPE_MAGICAL_BEAST      14
#define RACE_TYPE_MINOTAUR           15
#define RACE_TYPE_DRAGON             16
#define RACE_TYPE_CONSTRUCT          17
#define RACE_TYPE_HALF_ELF           18
#define RACE_TYPE_ORC                19
#define RACE_TYPE_DRACONIAN          20
#define RACE_TYPE_GOBLINOID          21
#define RACE_TYPE_VERMIN             22
#define RACE_TYPE_CENTAUR            23
#define RACE_TYPE_FEY                23

#define NUM_RACE_TYPES               23



#define COIN_UNDEFINED      0
#define COIN_ADAMANTINE     1
#define COIN_MITHRIL      2
#define COIN_STEEL      3
#define COIN_BRONZE     4
#define COIN_COPPER     5

#define OPERAND_NONE      0
#define OPERAND_ADD     1
#define OPERAND_SUBTRACT    2

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define SIZE_UNDEFINED  -1
#define SIZE_FINE 0
#define SIZE_DIMINUTIVE 1
#define SIZE_TINY 2
#define SIZE_SMALL  3
#define SIZE_MEDIUM 4
#define SIZE_LARGE  5
#define SIZE_HUGE 6
#define SIZE_GARGANTUAN 7
#define SIZE_COLOSSAL 8
#define NUM_SIZES         9
#define WIELD_NONE        0
#define WIELD_LIGHT       1
#define WIELD_ONEHAND     2
#define WIELD_TWOHAND     3

#define WIELD_TYPE_MAIN   0
#define WIELD_TYPE_OFF    1

/* Number of weapon types */

/* Critical hit types */
#define CRIT_X2   0
#define CRIT_X3   1
#define CRIT_X4   2
#define CRIT_X5   3
#define MAX_CRIT_TYPE CRIT_X5

/* Clans --gan, 07/27/2001 */

#define CLAN_KOS         1
#define CLAN_CONCLAVE    2
#define CLAN_HOLY_ORDER  3
#define CLAN_BLACKBLADE  4

#define CLAN_MAX_CLAN    15
#define CLAN_MAX_LEVEL   101

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2
#define NUM_SEX       3

/* Positions */

#define POS_DEAD       0        /* dead                 */
#define POS_MORTALLYW  1        /* mortally wounded     */
#define POS_INCAP      2        /* incapacitated        */
#define POS_STUNNED    3        /* stunned              */
#define POS_SLEEPING   4        /* sleeping             */
#define POS_RESTING    5        /* resting              */
#define POS_SITTING    6        /* sitting              */
#define POS_FIGHTING   7        /* fighting             */
#define POS_STANDING   8        /* standing             */
#define POS_DIGGING    9
#define POS_FISHING    10       /*  fishing            */
#define POS_RIDING     11        /* riding             */

/* Player flags: used by char_data.act */

#define PLR_KILLER	0   /* Player is a player-killer		*/
#define PLR_THIEF	1   /* Player is a player-thief		*/
#define PLR_FROZEN	2   /* Player is frozen			*/
#define PLR_DONTSET     3   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	4   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	5   /* Player is writing mail		*/
#define PLR_CRASH	6   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	7   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	8   /* Player not allowed to shout/goss	*/
#define PLR_RABBIT	9   /* Player not allowed to set title	*/
#define PLR_DELETED	10  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	11  /* Player uses nonstandard loadroom	*/
#define PLR_WOLF	12  /* Player shouldn't be on wizlist	*/
#define PLR_BEAR	13  /* Player shouldn't be deleted	*/
#define PLR_COURIER	14  /* Player is either morphed/disguised */
#define PLR_CAT		15  /* Player is cryo-saved (purge prog)	*/
#define PLR_BIRD	16  /* Player is test char only          */
#define PLR_IMMCHAR     17  /* Player will have some imm notifications*/
#define PLR_FREERENT    18  /* Player will have free rent        */
#define PLR_SUBDUING    19  /* Player is in subdual mode         */
#define PLR_PRISONER    20  /* Player is a prisoner              */
#define PLR_FISHING     21  /* Player has a line in the water   */
#define PLR_FISH_ON     22  /* Player has a fish on their line  */
#define PLR_DIGGING     23  /* Player is digging   */
#define PLR_DIG_ON      24  /* Player has hit something   */
#define PLR_FIRE_ON     25
#define PLR_MAGE	26
#define PLR_CLERIC	27
#define PLR_MONK	28
#define PLR_BARD	29
#define PLR_BEGGAR	30
#define PLR_KNIGHT	31
#define PLR_NOTITLE 32  /* Player not allowed to set title  */
#define PLR_NOWIZLIST 33  /* Player shouldn't be on wizlist   */
#define PLR_NODELETE  34  /* Player shouldn't be deleted      */
#define PLR_INVSTART  35  /* Player should enter game wizinvis*/
#define PLR_CRYO  36  /* Player is cryo-saved (purge prog)*/
#define PLR_NOTDEADYET  37  /* (R) Player being extracted.      */
#define PLR_AGEMID_G  38  /* Player has had pos of middle age */
#define PLR_AGEMID_B  39  /* Player has had neg of middle age */
#define PLR_AGEOLD_G  40  /* Player has had pos of old age  */
#define PLR_AGEOLD_B  41  /* Player has had neg of old age  */
#define PLR_AGEVEN_G  42  /* Player has had pos of venerable age  */
#define PLR_AGEVEN_B 43  /* Player has had neg of venerable age  */
#define PLR_OLDAGE  44  /* Player is dead of old age  */
#define PLR_SUICIDE 45  // Player is deleting

/* Mobile flags: used by char_data.act */
#define MOB_SPEC         (0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (15) /* Mob can't be slept		*/
#define MOB_NOBASH	 (16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (17) /* Mob can't be blinded		*/
#define MOB_HUNTERKILLER (18) /* Pkiller hunter/killer             */
#define MOB_USE_SPELLS   (19) /* Mob can cast spells               */
#define MOB_DBL_ATTACK   (20) /* Mob gets another attack           */
#define MOB_TRPL_ATTACK  (21) /* Mob gets another attack           */
#define MOB_QUAD_ATTACK  (22) /* Mob gets another attack           */
#define MOB_SENTRY       (23) /* Mob will block players            */
#define MOB_MOUNTABLE    (24) /* Mob may be mounted for riding     */
#define MOB_QUEST	 (25) /* Mob is used for quests		*/
#define MOB_NOKILL    26 /* Mob can't be killed               */
#define MOB_NOTDEADYET    27 /* (R) Mob being extracted.          */
#define MOB_EXTRACT      (28) // Mob is set for extraction
#define MOB_LIEUTENANT   29   // Mob is a lieutenant with 2x hp and 4x xp
#define MOB_CAPTAIN      30   // Mob is a captain with 5x hp and 10x xp
#define MOB_BOSS         31   // Mob is a boss with 10x hp and 20x hp
#define MOB_TRAINALL     32   // If guildmaster will train all classes
#define MOB_INNOCENT     33   // must use the murder command to kill, will have penalties
#define MOB_GUILDMASTER  34   // mob is a guild master
#define MOB_NO_AUTOGOLD  35   // mob won't have it's gold automatically set
#define MOB_CUSTOM_STATS 36   // mob doesn't have their stats automatically set
#define MOB_CRYSTAL_VENDOR 37 // mob is a shopkeeper who sells crystals and essences
#define MOB_FINAL_BOSS 38
#define MOB_BLOCK_N  39
#define MOB_BLOCK_E  40
#define MOB_BLOCK_S  41
#define MOB_BLOCK_W  42
#define MOB_BLOCK_NE 43
#define MOB_BLOCK_SE 44
#define MOB_BLOCK_SW 45
#define MOB_BLOCK_NW 46
#define MOB_BLOCK_U  47
#define MOB_BLOCK_D  48
#define MOB_BLOCK_RACE 49
#define MOB_BLOCK_CLASS 50
#define MOB_BLOCK_LEVEL 51
#define MOB_BLOCK_ALIGN 52
#define MOB_BLOCK_RACE_FAMILY 53
#define MOB_RANGED_ATTACKER 54
#define MOB_RANDOM_RACE 55
#define MOB_RANDOM_GENDER 56
#define MOB_MOLD_SELLER 57
#define MOB_BLOCK_ALLIANCE_GOOD 58
#define MOB_BLOCK_ALLIANCE_NEUTRAL 59
#define MOB_BLOCK_ALLIANCE_EVIL 60

// Mob Feats
#define MFEAT_WEAPON_FOCUS      1
#define MFEAT_GREATER_WEAPON_FOCUS    2
#define MFEAT_WEAPON_SPECIALIZATION   3
#define MFEAT_GREATER_WEAPON_SPECIALIZATION 4
#define MFEAT_DODGE       5
#define MFEAT_IRON_WILL       6
#define MFEAT_LIGHTNING_REFLEXES    7
#define MFEAT_GREAT_FORTITUDE     8
#define MFEAT_IMPROVED_TWO_WEAPON_FIGHTING  9
#define MFEAT_GREATER_TWO_WEAPON_FIGHTING 10
#define MFEAT_IMPROVED_DISARM     11
#define MFEAT_EVASION       12
#define MFEAT_IMPROVED_EVASION      13
#define MFEAT_COMBAT_REFLEXES     14
#define MFEAT_IMPROVED_INITIATIVE   15
#define MFEAT_IMPROVED_CRITICAL     16
#define MFEAT_SNEAK_ATTACK      17

#define ACTION_STANDARD 1
#define ACTION_MOVE     2
#define ACTION_MINOR    3

#define RP_EXP     1
#define RP_ART_EXP 2
#define RP_GOLD    3
#define RP_QUEST   4
#define RP_CRAFT   5
#define RP_ACCOUNT 6

#define GOLD_ONHAND 1
#define GOLD_BANK   2

/*  flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(3)  /* Can't receive tells		*/
#define PRF_DISPHP	(4)  /* Display hit points in prompt	*/
#define PRF_DETECT	(5)
#define PRF_DISPMOVE	(6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(8)  /* Aggr mobs won't attack		*/
#define PRF_RUN 	(9)  // Player is running
#define PRF_JOG         (10)
#define PRF_NOREPEAT	(11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(12) /* Can see in dark			*/
#define PRF_COLOR_1	(13) /* Color (low bit)			*/
#define PRF_COLOR_2	(14) /* Color (high bit)			*/
#define PRF_NOWIZ	(15) /* Can't hear wizline			*/
#define PRF_LOG1	(16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(17) /* On-line System Log (high bit)	*/
#define PRF_NOTSELF     (18)
#define PRF_DISGUISE    (19)
#define PRF_HUNTED      (20)
#define PRF_ROOMFLAGS	(21) /* Can see room flags (ROOM_x)	*/
#define PRF_AFK		(22) /* AFK flag :)			*/
#define PRF_UNUSED_7    (23)
#define PRF_QUEST       (24)
#define PRF_LEVEL_FLAGS (25) /* so imm\imps can see level eq flags */
#define PRF_INTRO	(26) /* PC for intro toggle                */
#define PRF_NO_OOC	(27) /* No ooc chatter			*/
#define PRF_NO_NEWBIE   (28) /* Don't hear the newbie line         */
#define PRF_NO_CHAT     (29) /* The chat line is wiz/immchar only  */
#define PRF_TIRED       (30) /* After stance  */
#define PRF_DISPMANA    31  /* Display mana points in prompt       */
#define PRF_SUMMONABLE  32 /* Can be summoned       */
#define PRF_COLOR       33 /* Color         */
#define PRF_SPARE       34 /* Used to be second color bit   */
#define PRF_NOAUCT      35 /* Can't hear auction channel    */
#define PRF_NOGOSS      36 /* Can't hear gossip channel     */
#define PRF_NOGRATZ     37 /* Can't hear grats channel      */
#define PRF_DISPAUTO    38 /* Show prompt HP, MP, MV when < 30%.  */
#define PRF_CLS         39 /* Clear screen in OasisOLC      */
#define PRF_BUILDWALK   40 /* Build new rooms when walking    */
#define PRF_AUTOLOOT    41 /* Loot everything from a corpse   */
#define PRF_AUTOGOLD    42 /* Loot gold from a corpse     */
#define PRF_AUTOSPLIT   43 /* Split gold with group     */
#define PRF_FULL_EXIT   44 /* Shows full autoexit details   */
#define PRF_AUTOSAC     45 /* Sacrifice a corpse      */
#define PRF_AUTOMEM     46 /* Memorize spells       */
#define PRF_VIEWORDER   47 /* if you want to see the newest first   */
#define PRF_NOCOMPRESS  48 /* If you want to force MCCP2 off            */
#define PRF_AUTOASSIST  49 /* Auto-assist toggle                        */
#define PRF_DISPKI      50 /* Display ki points in prompt     */
#define PRF_DISPEXP     51
#define PRF_DISPGOLD    52
#define PRF_DISPEXITS   53
#define PRF_SPONSOR_CROWN 54
#define PRF_SPONSOR_PALADIN 55
#define PRF_AUTOCON     56 // Autoconsider turned on
#define PRF_POSITIVE    57 // Neutral cleric uses positive energy
#define PRF_NEGATIVE    58 //Neutral cleric uses negative energy
#define PRF_HAGGLE      59 // Player would like to haggle his store purchases
#define PRF_AUTOMAP     60   /* Show map at the side of room descs */
#define PRF_SPONTANEOUS 61 // Cleric using spontaneous spellcasting
#define PRF_ANONYMOUS   62  // player is anonymous
#define PRF_AUTOFEINT   63  // player feints automatically each round of combat
#define PRF_MAXIMIZE_SPELL 64 // player will maximize spells
#define PRF_EXTEND_SPELL 65 // player will extend spells
#define PRF_CRAFTING_BRIEF 66 // crafting progress won't be shown
#define PRF_NOHINTS     67  // player does not want to see hints
#define PRF_CONTAINED_AREAS 68  // area spells are contained to those you are fighting only
#define PRF_LFG         69  // player is looking for group
#define PRF_CLANTALK    (70) /* Can't hear clan channel            */
#define PRF_ALLCTELL    (71) /* Can't hear all clan channels(imm)  */
#define PRF_QUICKEN_SPELL 72 // can cast a spell as a free action
#define PRF_AUTOATTACK 73
#define PRF_POWERFUL_SNEAK 74
#define PRF_BLEEDING_ATTACK 75
#define PRF_KNOCKDOWN 76
#define PRF_ROBILARS_GAMBIT 77
#define PRF_TAKE_TEN 78
#define PRF_SUMMON_TANK 79
#define PRF_MOUNT_TANK 80
#define PRF_DIVINE_BOND 81
#define PRF_EMPOWER_SPELL 82
#define PRF_INTENSIFY_SPELL 83
#define PRF_COMPANION_TANK 84
#define PRF_BRIEFMAP 85
#define PRF_PARRY 86
#define PRF_PVP 87
#define PRF_FIGHT_SPAM 88
#define PRF_AUTO_FIRE 89
#define PRF_BURST_FIRE 90
#define PRF_DOUBLE_ATTACK 91
#define PRF_TRIPLE_ATTACK 92
#define PRF_MIGHTY_SWING 93
#define PRF_QUICKSLOTS 94
#define PRF_NEGATE_ENERGY 95
#define PRF_FORCE_HEAL 96
#define PRF_WHIRLWIND 97
#define PRF_OVERCHARGE 98
#define PRF_DJEM_SO 99
#define PRF_WICKED_STRIKE 100
#define PRF_QUEST_REPEAT 101
#define PRF_NOINFO 102

/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF        0       /* Autoexit off                     */
#define EXIT_NORMAL     1       /* Brief display (stock behaviour)  */
#define EXIT_NA         2       /* Not implemented - do not use     */
#define EXIT_COMPLETE   3       /* Full display                     */
#define _exitlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch),PRF_AUTOEXIT) ? 1 : 0 ) + (PRF_FLAGGED((ch),PRF_FULL_EXIT) ? 2 : 0 ) : 0 )
#define EXIT_LEV(ch) (_exitlevel(ch))

/* Affect bits: used in char_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */

#define AFF_INVISIBLE         (1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (10)	   /* Char can see in dark	*/
#define AFF_POISON            (11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (13)	   /* Char protected from good  */
#define AFF_SLEEP             (14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (15)	   /* Char can't be tracked	*/
#define AFF_FLIGHT            (16)
#define AFF_HASTE	      (17)	   /* Char moves rapidly        */
#define AFF_SNEAK             (18)	   /* Char can move quietly	*/
#define AFF_HIDE              (19)	   /* Char is hidden		*/
#define AFF_STANCE	      (20)	   /* Room for future expansion	*/
#define AFF_CHARM             (21)	   /* Char is charmed		*/
#define AFF_POLYMORPH         (22)
#define AFF_TIRED             (23)    /* Undead Timer              */
#define AFF_WATERBREATH       (24)    /* Underwater Breathing      */
#define AFF_STONESKIN         (25)    /* Stoneskin                 */
#define AFF_DETECT_DISGUISE   (26)    /* Detect Disguise      */
#define AFF_KNOCKOUT          (27)    /* Knockout     */
#define AFF_MAGIC_VEST        (28)    /* Magical Vestment          */
#define AFF_CONCEAL_ALIGN     (29)    /* Cannot tell align         */
#define AFF_TAMED	      (30)	   /* (R) Char has been tamed   */
#define AFF_JAWSTRIKE         (31)
#define AFF_DONTUSE           32    /* DON'T USE!    */
#define AFF_UNDEAD            33   /* Char is undead    */
#define AFF_PARALYZE          34   /* Char is paralized   */
#define AFF_NONE              35   /* Room for future expansion */
#define AFF_FLYING            36   /* Char is flying          */
#define AFF_ANGELIC           37  /* Char is an angelic being  */
#define AFF_ETHEREAL          38   /* Char is ethereal          */
#define AFF_MAGICONLY         39   /* Char only hurt by magic   */
#define AFF_NEXTPARTIAL       40   /* Next action cannot be full*/
#define AFF_NEXTNOACTION      41   /* Next action cannot attack (took full action between rounds) */
#define AFF_STUNNED           42   /* Char is stunned   */
#define AFF_UNUSED30          43   /* Char is stunned   */
#define AFF_CDEATH            44   /* Char is undergoing creeping death */
#define AFF_SPIRIT            45   /* Char has no body          */
#define AFF_SUMMONED          46   /* Char is summoned (i.e. transient */
#define AFF_CELESTIAL         47   /* Char is celestial         */
#define AFF_FIENDISH          48   /* Char is fiendish          */
#define AFF_ILLITERATE        49   /* Character cannot read or write */
#define AFF_WEARING_SUIT      50   /* Character is wearing a full suit of armor */
#define AFF_ANTI_EQUIPPED     51   /* Character has an anti-flagged item equipped */
#define AFF_COURAGE           52   /* Character inspired by aura of courage */
#define AFF_TRIPPING          53   // Character is attempting to trip opponent
#define AFF_SMITING           54   // Character is smiting opponent
#define AFF_FLURRY_OF_BLOWS   55  // Character is attacking using flurry of blows
#define AFF_FLAT_FOOTED_1     56    // Character is caught flat footed
#define AFF_FLAT_FOOTED_2     57    // Character is performing a feint with his weapon
#define AFF_AOO               58    // Character is receiving an attack of opportunity
#define AFF_SNEAK_ATTACK      59    // Character just did a sneak attack
#define AFF_NO_DEITY          60    // Character has no deity
#define AFF_RAGE              61    // Character is raging
#define AFF_FATIGUED          62		// Character is fatigued
#define AFF_EXP_BONUS_10      63    // +10% to exp
#define AFF_EXP_BONUS_25      64    // +25% to exp
#define AFF_EXP_BONUS_33      65    // +33% to exp
#define AFF_EXP_BONUS_50      66    // +50% to exp
#define AFF_STRENGTH_OF_HONOR 67    // Charactcer affected by strength of honor
#define AFF_NO_ALIGN          68    // Character's alignment can't be detected
#define AFF_SILENCE           69    // Character is silenced
#define AFF_BLIND             70    /* (R) Char is blind		*/
#define AFF_ENCUMBERANCE_LIGHT 71   // Light encumberance load
#define AFF_ENCUMBERANCE_MEDIUM 72  // Medium encumberance load
#define AFF_ENCUMBERANCE_HEAVY 73   // Heavy encumberance load
#define AFF_DOING_AOO         74    // character is performing an attack of opportunity
#define AFF_RAPID_SHOT        75    // character is using rapid shot feat
#define AFF_DEATH_WARD        76    // character cannot be affected by death affects
#define AFF_KEEN_WEAPON       77    // character's slashing/piercing weapons have extra critical chances
#define AFF_IMPACT_WEAPON     78    // character's bludgeoning weapons have extra critical chances
#define AFF_DISGUISED         79    // character is disguised
#define AFF_WILD_SHAPE        80    // character is wild shaped
#define AFF_TRUE_SIGHT        81    // character has true sight
#define AFF_TANKING           82    // character is tanking
#define AFF_RAPID_STRIKE      83    // character is using rapid strike

/* Modes of connectedness: used by descriptor_data.state */

#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12             /* Changing passwd: get old     */
#define CON_CHPWD_GETNEW 13             /* Changing passwd: get new     */
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_OEDIT	 17		/*. OLC mode - object edit     .*/
#define CON_REDIT	 18		/*. OLC mode - room edit       .*/
#define CON_ZEDIT	 19		/*. OLC mode - zone info edit  .*/
#define CON_MEDIT	 20		/*. OLC mode - mobile edit `   .*/
#define CON_SEDIT	 21		/*. OLC mode - shop edit       .*/
#define CON_TRIGEDIT	 22		/*. OLC mode - trigger edit    .*/
#define CON_QRACE        23             /* Race?                        */
#define CON_QANSI	 24		/* ANSI colors YES? no		*/
/*  PDH 12/ 2/98 - char gen  */
#define CON_QSHORT_D     25             /* short description            */
#define CON_QONELINE_D   26             /* one line description         */
#define CON_LDESC        27             /* long (paragraph) description */
#define CON_QABILITY     28             /* ability scores               */
#define CON_ALIAS        29             /* alias list                   */
#define CON_AFTER_DESC   30             /* after char complete          */
#define CON_ALIGNMENT    31             /* static alignment selection   */
#define CON_GODSELECT    32             /* cleric/paladin god selection */
#define CON_TEDIT  34 /* OLC mode - text editor   */
#define CON_CEDIT  35 /* OLC mode - config editor   */
#define CON_ASSEDIT      36     /* OLC mode - Assemblies                */
#define CON_AEDIT        37 /* OLC mode - social (action) edit      */
#define CON_RACE_HELP    39 /* Race Help        */
#define CON_CLASS_HELP   40 /* Class Help         */
#define CON_GEDIT  41 /* oLC mode - guild editor    */
#define CON_QROLLSTATS   42 /* Reroll stats       */
#define CON_IEDIT        43 /* OLC mode - individual edit   */
#define CON_LEVELUP  44 /* Level up menu      */
#define CON_QSTATS   45 /* Assign starting stats          */
#define CON_HEDIT  46   /* Using Help Editor      */
#define CON_QALIGNMENT   47 /* Assign Alignment     */
#define CON_ALIGNMENT_HELP 48 /* Alignment Help     */
#define CON_SDESC 49  /* Short description      */
#define CON_QLDESCC 50  /* Long description     */
#define CON_DDESC 51  /* Detailed description     */
#define CON_KEYW  52  /* Keywords       */
#define CON_ACCOUNT_NAME       53
#define CON_ACCOUNT_NAME_CNFRM 54
#define CON_ACCOUNT_MENU       55
#define CON_ACCOUNT_NAME       53
#define CON_ACCOUNT_DELETE 56
#define CON_QUIT_GAME_EARLY 57

#define CON_GEN_DESCS_INTRO 58           /* Introduction text for generic short descs */
#define CON_GEN_DESCS_DESCRIPTORS_1 59 /* Set descriptor 1 for generic short descs */
#define CON_GEN_DESCS_DESCRIPTORS_2 60 /* Set descriptor 2 for generic short descs */
#define CON_GEN_DESCS_ADJECTIVES_1 61  /* Set adjective 1 for generic short descs */
#define CON_GEN_DESCS_ADJECTIVES_2 62  /* Set adjective 2 for generic short descs */
#define CON_GEN_DESCS_MENU 63          /* Generic short desc menu */
#define CON_GEN_DESCS_MENU_PARSE 64    /* Decide what to do from generic short desc menu choice */


#define CON_RACE_HUMAN         65
#define CON_RACE_ELF           66
#define CON_RACE_DWARF         67
#define CON_RACE_CONFIRM       68

#define CON_QEDIT              69     /* OLC mode - quest edit                */

#define CON_CLANEDIT     70     /* OLC mode - clan editor               */

#define CON_LEVELUP_START		71	
#define CON_LEVELUP_SPELLS   		72
#define CON_LEVELUP_CLASSES 		73
#define CON_LEVELUP_SKILLS_MENU 	74
#define CON_LEVELUP_SKILLS 		75
#define CON_LEVELUP_SKILLS_CONFIRM 	76
#define CON_LEVELUP_FEATS			77
#define CON_LEVELUP_FEATS_CONFIRM	78
#define CON_LEVELUP_FEATS_WEAPONS	79
#define CON_LEVELUP_FEATS_SKILLS	80
#define CON_LEVELUP_TRAINS			81
#define CON_LEVELUP_TRAINS_CONFIRM	82
#define CON_LEVELUP_SUMMARY			83
#define CON_LEVELUP_FEATS_PROCESS	84
#define CON_LEVELUP_SPELLS_CONFIRM	85
#define CON_LEVELUP_END			85

#define CON_SELECT_COMBAT_ACTION	86
#define CON_CUSTOM_COMBAT_ACTION	87
#define CON_PARSE_COMBAT_ACTION		88
#define CON_PETSET		89	/* OLC mode - pet builder	*/
#define CON_EMAIL			90
#define CON_NOTE_WRITE			91
#define CON_NEWMAIL			92

#define CON_DISCONNECT   999             /* in game disconnection        */

/* Colors that the player can define */
#define COLOR_NORMAL      0
#define COLOR_ROOMNAME      1
#define COLOR_ROOMOBJS      2
#define COLOR_ROOMPEOPLE    3
#define COLOR_HITYOU      4
#define COLOR_YOUHIT      5
#define COLOR_OTHERHIT      6
#define COLOR_CRITICAL      7
#define COLOR_HOLLER      8
#define COLOR_SHOUT     9
#define COLOR_GOSSIP      10
#define COLOR_AUCTION     11
#define COLOR_CONGRAT     12
#define NUM_COLOR     13

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_IMPLANT_ONE   1
#define WEAR_IMPLANT_TWO   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST_1   13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD       17
#define WEAR_FACE      18
#define WEAR_EAR_R     19
#define WEAR_EAR_L     20
#define WEAR_ANKLE_R   21
#define WEAR_ANKLE_L   22
#define WEAR_ABOVE     23
#define WEAR_BACK_1    24
#define WEAR_SHOULDER  25
#define WEAR_NOSE      26
#define WEAR_ONBELT    27
#define WEAR_ONBACK_1  28
#define WEAR_ONBACK_2  29
#define WEAR_ONBACK_3  30
#define WEAR_WAIST_2   31
#define WEAR_SHEATHED_H   32
#define WEAR_SHEATHED_B1 33
#define WEAR_SHEATHED_B2 34
#define WEAR_SHEATHED_WA1 35
#define WEAR_SHEATHED_WA2 36
#define WEAR_SHEATHED_A1 37
#define WEAR_SHEATHED_A2 38
#define WEAR_SHEATHED_WR1 39
#define WEAR_SHEATHED_WR2 40
#define WEAR_BARDING 41

#define WEAR_WIELD1     16
#define WEAR_WIELD2     17
#define WEAR_UNUSED 41
#define WEAR_UNUSED0 42
#define WEAR_UNUSED1 43

#define WEAR_WAIST 31
#define WEAR_BACKPACK  28
#define WEAR_WINGS 29
#define WEAR_MASK 18

#define NUM_WEARS      42      /* This must be the # of eq positions!! */

#define SPELL_LEVEL_0     0
#define SPELL_LEVEL_1     1
#define SPELL_LEVEL_2     2
#define SPELL_LEVEL_3     3
#define SPELL_LEVEL_4     4
#define SPELL_LEVEL_5     5
#define SPELL_LEVEL_6     6
#define SPELL_LEVEL_7     7
#define SPELL_LEVEL_8     8
#define SPELL_LEVEL_9     9
#define MAX_SPELL_LEVEL   10                    /* how many spell levels */
#define MAX_MEM          (MAX_SPELL_LEVEL * 10) /* how many total spells */

#define DOMAIN_UNDEFINED    0
#define DOMAIN_AIR          1
#define DOMAIN_ANIMAL       2
#define DOMAIN_CHAOS        3
#define DOMAIN_DEATH        4
#define DOMAIN_DESTRUCTION  5
#define DOMAIN_EARTH        6
#define DOMAIN_EVIL         7
#define DOMAIN_FIRE         8
#define DOMAIN_GOOD         9
#define DOMAIN_HEALING      10
#define DOMAIN_KNOWLEDGE    11
#define DOMAIN_LAW          12
#define DOMAIN_LUCK         13
#define DOMAIN_MAGIC        14
#define DOMAIN_PLANT        15
#define DOMAIN_PROTECTION   16
#define DOMAIN_STRENGTH     17
#define DOMAIN_SUN          18
#define DOMAIN_TRAVEL       19
#define DOMAIN_TRICKERY     20
#define DOMAIN_UNIVERSAL    21
#define DOMAIN_WAR          22
#define DOMAIN_WATER        23
#define DOMAIN_ARTIFICE     24
#define DOMAIN_CHARM        25
#define DOMAIN_COMMUNITY    26
#define DOMAIN_CREATION     27
#define DOMAIN_DARKNESS     28
#define DOMAIN_GLORY        29
#define DOMAIN_LIBERATION   30
#define DOMAIN_MADNESS      31
#define DOMAIN_NOBILITY     32
#define DOMAIN_REPOSE       33
#define DOMAIN_RUNE         34
#define DOMAIN_SCALYKIND    35
#define DOMAIN_WEATHER      36
#define DOMAIN_MEDITATION   37
#define DOMAIN_FORGE        38
#define DOMAIN_PASSION      39
#define DOMAIN_INSIGHT      40
#define DOMAIN_TREACHERY    41
#define DOMAIN_STORM        42
#define DOMAIN_PESTILENCE   43
#define DOMAIN_SUFFERING    44
#define DOMAIN_RETRIBUTION  45
#define DOMAIN_PLANNING     46
#define DOMAIN_CRAFT        47
#define DOMAIN_DWARF        48
#define DOMAIN_TIME         49
#define DOMAIN_FAMILY       50
#define DOMAIN_MOON         51
#define DOMAIN_DROW         52
#define DOMAIN_ELF          53
#define DOMAIN_CAVERN       54
#define DOMAIN_ILLUSION     55
#define DOMAIN_SPELL        56
#define DOMAIN_HATRED       57
#define DOMAIN_TYRANNY      58
#define DOMAIN_FATE         59
#define DOMAIN_RENEWAL      60
#define DOMAIN_METAL        61
#define DOMAIN_OCEAN        62
#define DOMAIN_MOBILITY     63
#define DOMAIN_PORTAL       64
#define DOMAIN_TRADE        65
#define DOMAIN_UNDEATH      66
#define DOMAIN_MENTALISM    67
#define DOMAIN_GNOME        68
#define DOMAIN_HALFLING     69
#define DOMAIN_ORC          70
#define DOMAIN_SPIDER       71
#define DOMAIN_SLIME        72
#define DOMAIN_MEDIATION    73

#define NUM_DOMAINS         74

#define DOMAIN_CLERIC	    1000
#define DOMAIN_PALADIN      1001
#define DOMAIN_RANGER       1002
#define DOMAIN_DRUID        1003
#define DOMAIN_BARD         1004


#define SCHOOL_UNDEFINED  1
#define SCHOOL_ABJURATION 2
#define SCHOOL_CONJURATION  3
#define SCHOOL_DIVINATION 4
#define SCHOOL_ENCHANTMENT  6
#define SCHOOL_EVOCATION  7
#define SCHOOL_ILLUSION   8
#define SCHOOL_NECROMANCY 9
#define SCHOOL_TRANSMUTATION  10
#define SCHOOL_UNIVERSAL  11
#define NUM_SCHOOLS   12

#define NUM_DEITIES_DL_AOL               22
#define NUM_DEITIES_FR                   11

/* Combat feats that apply to a specific weapon type */
#define CFEAT_IMPROVED_CRITICAL     0
#define CFEAT_WEAPON_FINESSE      1
#define CFEAT_WEAPON_FOCUS      2
#define CFEAT_WEAPON_SPECIALIZATION   3
#define CFEAT_GREATER_WEAPON_FOCUS    4
#define CFEAT_GREATER_WEAPON_SPECIALIZATION 5
#define CFEAT_IMPROVED_WEAPON_FINESSE 6
#define CFEAT_SKILL_FOCUS 7
#define CFEAT_WEAPON_PROFICIENCY_EXOTIC 8
#define CFEAT_MONKEY_GRIP 9
#define CFEAT_FAVORED_ENEMY 10
#define CFEAT_EPIC_SKILL_FOCUS 11
#define CFEAT_CRITICAL_FOCUS 12
#define CFEAT_WEAPON_MASTERY 13
#define CFEAT_WEAPON_FLURRY 14
#define CFEAT_WEAPON_SUPREMACY 15
#define CFEAT_TRIPLE_CRIT 16
#define CFEAT_MAX       16

/* Spell feats that apply to a specific school of spells */
#define SFEAT_SPELL_FOCUS     0
#define SFEAT_GREATER_SPELL_FOCUS   1
#define SFEAT_MAX       1

// Skill feats that apply to a specific skill
#define SKFEAT_SKILL_FOCUS 0
#define SKFEAT_MAX 1

/* object-related defines ********************************************/
/* Item types: used by obj_data.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_SALVE      6               /* Item is a salve              */
#define ITEM_BELT       7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_BOTTLE    14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_SHOVEL    22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_FIRE      24                /* Item for flying             */
#define ITEM_AQUALUNG  25
#define ITEM_SHEATH    26
#define ITEM_RAW       27
#define ITEM_PORTAL    28
#define ITEM_POLE      29
#define ITEM_FIREWOOD  30
#define ITEM_FIREWEAPON 31   /* Unimplemented    */
#define ITEM_MISSILE   32   /* Unimplemented    */
#define ITEM_TRAP      33   /* Unimplemented    */
#define ITEM_BOAT      34   /* Item is a boat   */
#define ITEM_VEHICLE   35               /* Item is a vehicle            */
#define ITEM_HATCH     36               /* Item is a vehicle hatch      */
#define ITEM_WINDOW    37               /* Item is a vehicle window     */
#define ITEM_CONTROL   38               /* Item is a vehicle control    */
#define ITEM_SPELLBOOK 39               /* Item is a spellbook          */
#define ITEM_BOARD     40               /* Item is a message board  */
#define ITEM_MATERIAL  41   /* Item is a crafting material  */
#define ITEM_ARMOR_SUIT 42    /* Item is a full suit of armor */
#define ITEM_HEALING_KIT 43   // Kit used with the heal skill
#define ITEM_HARVEST_NODE 44  // Used for harvesting materials for crafting
#define ITEM_CRYSTAL   45

/* Take/Wear flags: used by obj_data.wear_flags */
#define ITEM_WEAR_TAKE		(0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1)  /* Can be worn on finger	*/
#define ITEM_WEAR_IMPLANT	(1)  /* Can be worn as implant	*/
#define ITEM_WEAR_NECK		(2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(14) /* Can be held		*/
#define ITEM_WEAR_FACE          (15) /* new position face          */
#define ITEM_WEAR_EAR           (16) /* new position ear           */
#define ITEM_WEAR_ANKLE         (17) /* new position ankle         */
#define ITEM_WEAR_ABOVE         (18) /* new position above head    */
#define ITEM_WEAR_BACK          (19) /* new position back          */
#define ITEM_WEAR_SHOULDER      (20) /* new position sholders      */
#define ITEM_WEAR_NOSE          (21) /* new position nose          */
#define ITEM_WEAR_SPARE_1       (22) /* new position sheathed      */
#define ITEM_WEAR_SHEATHED_B    (23) /* new position for sheath obj*/
#define ITEM_WEAR_ONBELT        (24) /* new position for sheath obj*/
#define ITEM_WEAR_ONBACK        (25)
#define ITEM_WEAR_SHEATHED_WA   (26)
#define ITEM_WEAR_SHEATHED_WR   (27)
#define ITEM_WEAR_SHEATHED_A    (28)
#define ITEM_WEAR_SHEATHED_H    (29)
#define ITEM_WEAR_BARDING       (30)

#define ITEM_WEAR_PACK  19
#define ITEM_WEAR_WINGS 19
#define ITEM_WEAR_MASK 15

/* Extra object flags: used by obj_data.extra_flags */


#define ITEM_GLOW          (0)	/* Item is glowing		*/
#define ITEM_HIDDEN        (1)     /* Item is hiding               */
#define ITEM_NORENT        (2)	/* Item cannot be rented	*/
#define ITEM_NO_REMOVE     (3)     /* Item cannot be removed       */
#define ITEM_NOINVIS	   (4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (5)	/* Item is invisible		*/
#define ITEM_MAGIC         (6)	/* Item is magical		*/
#define ITEM_NODROP        (7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (8)	/* Item is blessed		*/
#define ITEM_ANTI_GOOD     (9)	/* Not usable by good people	*/
#define ITEM_ANTI_EVIL     (10)	/* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL  (11)	/* Not usable by neutral people	*/
#define ITEM_ANTI_MAGE     (12)    /* Not usable by mages          */
#define ITEM_ANTI_CLERIC   (13)    /* Not usable by clerics        */
#define ITEM_ANTI_ROGUE    (14)    /* Not usable by rogues         */
#define ITEM_ANTI_WARRIOR  (15)    /* Not usable by warriors       */
#define ITEM_NOSELL	   (16)	/* Shopkeepers won't touch it	*/
#define ITEM_ANTI_BARBAR   (17)	/* Not usable by barbarians     */
#define ITEM_ANTI_PALADIN  (18)	/* Not usable by paladins       */
#define ITEM_ANTI_DWARF    (19)	/* Not usable by dwarves        */
#define ITEM_ANTI_ELF      (20)	/* Not usable by elves          */
#define ITEM_ANTI_GNOME    (21)	/* Not usable by gnomes         */
#define ITEM_ANTI_HUMAN    (22)	/* Not usable by humans         */
#define ITEM_HUM             23
#define ITEM_UNIQUE_SAVE     24 /* unique object save           */
#define ITEM_BROKEN          25 /* Item is broken hands         */
#define ITEM_UNBREAKABLE     26 /* Item is unbreakable          */
#define ITEM_DOUBLE          27 /* Double weapon                */
#define ITEM_MOBITEM         28 // Item can be worn by mobs
#define ITEM_IDENTIFIED      29 // Item has been identified
#define ITEM_NOLORE          30 // Item cannot be identified by lore skills
#define ITEM_BLACKMARKET     31 // Item can only be sold on the blackmarket
#define ITEM_QUEST           32 /* Item is a quest item         */
#define ITEM_MOLD            33
#define ITEM_EXTRACT         34 // these items will not be transferred to mob corpses
#define ITEM_ATTUNE          35
#define ITEM_FORCE_PVP       36
#define ITEM_DEATH_DROP      37
#define ITEM_RARE            38
#define ITEM_LEGENDARY       39
#define ITEM_MYTHICAL        40


#define ITEM_ANTI_KNIGHT    127	/* Not usable by knights        */
#define ITEM_ANTI_RANGER   (127)	/* Not usable by rangers        */
#define ITEM_ALLOW_MONK    (127)    /* item usable by monks         */
#define ITEM_ANTI_DRUID    (127)	/* Not usable by druids         */
#define ITEM_ANTI_HALFELF  (127)	/* Not usable by halfelves      */
#define ITEM_ANTI_KENDER   (127)	/* Not usable by kender         */
#define ITEM_ANTI_MINOTAUR (127)	/* Not usable by minnies        */
#define ITEM_TWO_HANDED    (127)    /*  Two-handed weapon           */
#define ITEM_CREATE        (127)
#define ITEM_NODONATE       127  /* Item cannot be donated       */
#define ITEM_ANTI_WIZARD     127 /* Not usable by mages          */
#define ITEM_ANTI_FIGHTER    127 /* Not usable by warriors       */
#define ITEM_2H              127 /* Requires two free hands      */
#define ITEM_ANTI_BARD       127 /* Not usable by bards          */
#define ITEM_ANTI_MONK       127 /* Not usable by monks          */
#define ITEM_ANTI_BARBARIAN  127 /* Not usable by barbarians     */
#define ITEM_ANTI_SORCERER   127 /* Not usable by sorcerers      */
#define ITEM_ONLY_WIZARD     127 /* Only usable by mages         */
#define ITEM_ONLY_CLERIC     127 /* Only usable by clerics       */
#define ITEM_ONLY_ROGUE      127 /* Only usable by thieves       */
#define ITEM_ONLY_FIGHTER    127 /* Only usable by warriors      */
#define ITEM_ONLY_DRUID      127 /* Only usable by druids        */
#define ITEM_ONLY_BARD       127 /* Only usable by bards         */
#define ITEM_ONLY_RANGER     127 /* Only usable by rangers       */
#define ITEM_ONLY_PALADIN    127 /* Only usable by paladins      */
#define ITEM_ONLY_HUMAN      127 /* Only usable by humans        */
#define ITEM_ONLY_DWARF      127 /* Only usable by dwarves       */
#define ITEM_ONLY_ELF        127 /* Only usable by elves         */
#define ITEM_ONLY_GNOME      127 /* Only usable by gnomes        */
#define ITEM_ONLY_MONK       127 /* Only usable by monks         */
#define ITEM_ONLY_BARBARIAN  127 /* Only usable by barbarians    */
#define ITEM_ONLY_SORCERER   127 /* Only usable by sorcerers     */
#define ITEM_ONLY_KNIGHT     127 // Only usable by knights
#define ITEM_ONLY_KENDER     127 // Only usable by kender
#define ITEM_ONLY_MINOTAUR   127 // Only usable by minotaurs
#define ITEM_ONLY_HALFELF    127 // Only usable by half elves

// Item Lore Types

#define ITEM_LORE_ARCANE    0
#define ITEM_LORE_RELIGIOUS 1
#define ITEM_LORE_PLANAR    2

#define MAX_ITEM_LORE_TYPES 3

/* Modifier constants used with obj affects ('A' fields) */

#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_UNDEFINED        12
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Deflection AC	*/
#define APPLY_AC_DEFLECTION    17       // Repeat of APPLY_AC for compatibility
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_UNUSED           20
#define APPLY_AC_NATURAL       21
#define APPLY_AC_ARMOR         22
#define APPLY_AC_SHIELD        23
#define APPLY_AC_DODGE         24
#define APPLY_QUESTPOINTS      25       /* Apply quest points i hope	*/
#define APPLY_LIGHT            26       /* Apply to light source        */
#define APPLY_MANA             27 /* Apply to max mana    */
#define APPLY_ACCURACY         28 /* Apply to accuracy    */
#define APPLY_DAMAGE           29 /* Apply to damage    */
#define APPLY_RACE             30       /* Apply to race                */
#define APPLY_TURN_LEVEL       31       /* Apply to turn undead         */
#define APPLY_SPELL_LVL_0      32       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_1      33       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_2      34       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_3      35       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_4      36       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_5      37       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_6      38       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_7      39       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_8      40       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_9      41       /* Apply to spell cast per day  */
#define APPLY_KI               42 /* Apply to max ki    */
#define APPLY_FORTITUDE        43 /* Apply to fortitue save */
#define APPLY_REFLEX           44 /* Apply to reflex save   */
#define APPLY_WILL             45 /* Apply to will save   */
#define APPLY_SKILL            46       /* Apply to a specific skill    */
#define APPLY_FEAT             47       /* Apply to a specific feat     */
#define APPLY_ALLSAVES         48       /* Apply to all 3 save types  */
#define APPLY_RESISTANCE       49       /* Apply to all 3 save types  */
#define APPLY_ABILITY          50 /* Apply adds an ability  */
#define APPLY_CARRY_WEIGHT     51 /* Adds to strength for purpose of carrying weight */

#define APPLY_SAVING_PARA      43	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       44	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     43	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    44	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     45	/* Apply to save throw: spells	*/

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)  /* Container can be closed  */
#define CONT_PICKPROOF      (1 << 1)  /* Container is pickproof */
#define CONT_CLOSED         (1 << 2)  /* Container is closed    */
#define CONT_LOCKED         (1 << 3)  /* Container is locked    */
#define CONT_PICKED         (1 << 4)  // Container has been picked

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15

#define MATERIAL_UNDEFINED      0
#define MATERIAL_COTTON         1
#define MATERIAL_LEATHER        2
#define MATERIAL_GLASS          3
#define MATERIAL_GOLD           4
#define MATERIAL_ORGANIC        5
#define MATERIAL_PAPER          6
#define MATERIAL_STEEL          7
#define MATERIAL_WOOD           8
#define MATERIAL_BONE           9
#define MATERIAL_CRYSTAL        10
#define MATERIAL_ETHER          11
#define MATERIAL_ADAMANTINE     12
#define MATERIAL_MITHRIL        13
#define MATERIAL_IRON           14
#define MATERIAL_CURRENCY       15
#define MATERIAL_COPPER         16
#define MATERIAL_CERAMIC        17
#define MATERIAL_SATIN          18
#define MATERIAL_SILK           19
#define MATERIAL_BURLAP         20
#define MATERIAL_VELVET         21
#define MATERIAL_PLATINUM       22
#define MATERIAL_OBSIDIAN       23
#define MATERIAL_WOOL           24
#define MATERIAL_ONYX           25
#define MATERIAL_IVORY          26
#define MATERIAL_BRASS          27
#define MATERIAL_MARBLE         28
#define MATERIAL_BRONZE         29
#define MATERIAL_PEWTER         30
#define MATERIAL_RUBY           31
#define MATERIAL_SAPPHIRE       32
#define MATERIAL_EMERALD        33
#define MATERIAL_GEMSTONE       34
#define MATERIAL_GRANITE        35
#define MATERIAL_STONE          36
#define MATERIAL_ENERGY         37
#define MATERIAL_HEMP           38
#define MATERIAL_DIAMOND        39
#define MATERIAL_EARTH          40
#define MATERIAL_SILVER         41
#define MATERIAL_ALCHEMICAL_SILVER 42
#define MATERIAL_COLD_IRON      43
#define MATERIAL_DARKWOOD       44
#define MATERIAL_DRAGONHIDE     45

#define NUM_MATERIALS           45

#define NUM_CRAFTS              17

/* other miscellaneous defines *******************************************/

// mounted states

#define MOUNT_NONE 0
#define MOUNT_SUMMON 1
#define MOUNT_MOUNT 2
#define MOUNT_COMPANION 3

// weapon head types

#define HEAD_TYPE_UNDEFINED 0
#define HEAD_TYPE_BLADE     1
#define HEAD_TYPE_HEAD      2
#define HEAD_TYPE_POINT     3
#define HEAD_TYPE_BOW       4
#define HEAD_TYPE_POUCH     5
#define HEAD_TYPE_CORD      6
#define HEAD_TYPE_MESH      7
#define HEAD_TYPE_CHAIN     8
#define HEAD_TYPE_FIST      9
#define HEAD_TYPE_BARREL    10
#define HEAD_TYPE_ENERGY    11

// weapon handle types

#define HANDLE_TYPE_UNDEFINED 0
#define HANDLE_TYPE_SHAFT     1
#define HANDLE_TYPE_HILT      2
#define HANDLE_TYPE_STRAP     3
#define HANDLE_TYPE_STRING    4
#define HANDLE_TYPE_GRIP      5
#define HANDLE_TYPE_HANDLE    6
#define HANDLE_TYPE_GLOVE     7

// damage types

#define DAMAGE_TYPE_BLUDGEONING        (1 << 0)
#define DAMAGE_TYPE_SLASHING           (1 << 1)
#define DAMAGE_TYPE_PIERCING           (1 << 2)
#define DAMAGE_TYPE_ENERGY_BLASTERS    (1 << 3)
#define DAMAGE_TYPE_ENERGY_RIFLES      (1 << 4)
#define DAMAGE_TYPE_ENERGY_LIGHTSABERS (1 << 5)
#define DAMAGE_TYPE_SUBDUAL            (1 << 6)

#define NUM_DAMAGE_TYPES               3
#define NUM_SW_DAMAGE_TYPES             3

// weapon flags

#define WEAPON_FLAG_SIMPLE      (1 << 0)
#define WEAPON_FLAG_MARTIAL     (1 << 1)
#define WEAPON_FLAG_EXOTIC      (1 << 2)
#define WEAPON_FLAG_RANGED      (1 << 3)
#define WEAPON_FLAG_THROWN      (1 << 4)
#define WEAPON_FLAG_REACH       (1 << 5)
#define WEAPON_FLAG_ENTANGLE    (1 << 6)
#define WEAPON_FLAG_TRIP        (1 << 7)
#define WEAPON_FLAG_DOUBLE      (1 << 8)
#define WEAPON_FLAG_DISARM      (1 << 9)
#define WEAPON_FLAG_SUBDUAL     (1 << 10)
#define WEAPON_FLAG_SLOW_RELOAD (1 << 11)
#define WEAPON_FLAG_BALANCED    (1 << 12)
#define WEAPON_FLAG_CHARGE      (1 << 13)
#define WEAPON_FLAG_REPEATING   (1 << 14)
#define WEAPON_FLAG_TWO_HANDED  (1 << 15)

#define NUM_WEAPON_FLAGS        16

// weapon families

#define WEAPON_FAMILY_MONK             0
#define WEAPON_FAMILY_SMALL_BLADE      1
#define WEAPON_FAMILY_CLUB             2
#define WEAPON_FAMILY_FLAIL            3
#define WEAPON_FAMILY_SPEAR            4
#define WEAPON_FAMILY_DOUBLE           5
#define WEAPON_FAMILY_CROSSBOW         6
#define WEAPON_FAMILY_THROWN           7
#define WEAPON_FAMILY_AXE              8
#define WEAPON_FAMILY_HAMMER           9
#define WEAPON_FAMILY_PICK             10
#define WEAPON_FAMILY_MEDIUM_BLADE     11
#define WEAPON_FAMILY_LARGE_BLADE      12
#define WEAPON_FAMILY_POLEARM          13
#define WEAPON_FAMILY_BOW              14
#define WEAPON_FAMILY_WHIP             15
#define WEAPON_FAMILY_VIBROBLADES      16
#define WEAPON_FAMILY_BLASTERS         17
#define WEAPON_FAMILY_RIFLES           18
#define WEAPON_FAMILY_CARBINES         19
#define WEAPON_FAMILY_LIGHTSABERS      20

#define NUM_WEAPON_FAMILIES            21

#define MAX_BAG_ROWS 10000

#define ARTISAN_TYPE_UNDEFINED  0
#define ARTISAN_TYPE_SMITH      1
#define ARTISAN_TYPE_FARMER     2
#define ARTISAN_TYPE_WOODSMAN   3
#define ARTISAN_TYPE_ARMORTECH  4
#define ARTISAN_TYPE_WEAPONTECH 5
#define ARTISAN_TYPE_TINKERING  6
#define ARTISAN_TYPE_ROBOTICS   7
#define ARTISAN_TYPE_ALL        8

// Planets

#define PLANET_NONE			0
#define PLANET_CORUSCANT		1
#define PLANET_KORRIBAN			2
#define PLANET_TATOOINE			3
#define PLANET_KASHYYYK			4
#define PLANET_DANTOOINE		5
#define PLANET_CZERKA_STATION		6

#define NUM_PLANETS			7

// Level Ranges

#define LVL_RANGE_UNDEFINED    (1 << 0)
#define LVL_RANGE_1_2          (1 << 1)
#define LVL_RANGE_3_4          (1 << 2)
#define LVL_RANGE_5_6          (1 << 3)
#define LVL_RANGE_7_8          (1 << 4)
#define LVL_RANGE_9_10         (1 << 5)
#define LVL_RANGE_11_12        (1 << 6)
#define LVL_RANGE_13_14        (1 << 7)
#define LVL_RANGE_15_16        (1 << 8)
#define LVL_RANGE_17_18        (1 << 9)
#define LVL_RANGE_19_20        (1 << 10)
#define LVL_RANGE_21_22        (1 << 11)
#define LVL_RANGE_23_24        (1 << 12)
#define LVL_RANGE_25_26        (1 << 13)
#define LVL_RANGE_27_28        (1 << 14)
#define LVL_RANGE_29_30        (1 << 15)
#define LVL_RANGE_31_32        (1 << 16)
#define LVL_RANGE_32_34        (1 << 17)
#define LVL_RANGE_35_36        (1 << 18)
#define LVL_RANGE_37_38        (1 << 19)
#define LVL_RANGE_39_40        (1 << 20)
#define LVL_RANGE_41_42        (1 << 21)
#define LVL_RANGE_43_44        (1 << 22)
#define LVL_RANGE_45_46        (1 << 23)
#define LVL_RANGE_47_48        (1 << 24)
#define LVL_RANGE_49_50        (1 << 25)
#define LVL_RANGE_51_52        (1 << 26)
#define LVL_RANGE_53_54        (1 << 27)
#define LVL_RANGE_55_56        (1 << 28)
#define LVL_RANGE_57_58        (1 << 29)
#define LVL_RANGE_59_60        (1 << 30)

#define NUM_CURSE_WORDS 15

// Zone States

#define ZONE_STATE_CLOSED      (1 << 0)
#define ZONE_STATE_WORKING     (1 << 1)
#define ZONE_STATE_OPEN        (1 << 2)

// Animal Companion Types

#define COMPANION_TYPE_NONE             0
#define COMPANION_TYPE_BEAR             1
#define COMPANION_TYPE_WOLF             2
#define COMPANION_TYPE_PANTHER          3
#define COMPANION_TYPE_LION             4
#define COMPANION_TYPE_TIGER            5
#define COMPANION_TYPE_HAWK             6
#define COMPANION_TYPE_BOAR             7
#define COMPANION_TYPE_EAGLE            8
#define COMPANION_TYPE_OWL              9
#define COMPANION_TYPE_DOG              10
#define COMPANION_TYPE_PYTHON           11
#define COMPANION_TYPE_STAG             12
#define COMPANION_TYPE_COYOTE           13
#define COMPANION_TYPE_BADGER           14
#define COMPANION_TYPE_SNAKE            15

#define NUM_COMPANION_TYPES             16

#define TRAVEL_NONE		0
#define TRAVEL_TAXI		1
#define TRAVEL_SPEEDER		2	
#define TRAVEL_SHUTTLE		3
#define TRAVEL_STARSHIP 	4

#define MOB_TYPE_MOB        0
#define MOB_TYPE_SUMMON     1
#define MOB_TYPE_MOUNT      2
#define MOB_TYPE_COMPANION  3

#define NUM_CRAFT_TYPES   96
#define NUM_ESSENCE_TYPES 31

/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2

/* Sun state for weather_data */
#define SUN_DARK  0
#define SUN_RISE  1
#define SUN_LIGHT 2
#define SUN_SET   3

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS 0
#define SKY_CLOUDY  1
#define SKY_RAINING 2
#define SKY_LIGHTNING 3
/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5
/* for the 128bits */
#define RF_ARRAY_MAX    4
#define PM_ARRAY_MAX    4
#define PR_ARRAY_MAX    4
#define AF_ARRAY_MAX    4
#define TW_ARRAY_MAX    4
#define EF_ARRAY_MAX    4
#define AD_ARRAY_MAX  4
#define FT_ARRAY_MAX  4
#define MF_ARRAY_MAX  4
/* other #defined constants **********************************************/
/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
#define ADMLVL_NONE   0
#define ADMLVL_IMMORT   1
#define ADMLVL_BUILDER          2
#define ADMLVL_GOD    3
#define ADMLVL_GRGOD    4
#define ADMLVL_ADMIN   4
#define ADMLVL_IMPL   4
#define ADMLVL_OWNER 5
/* First character level that forces epic levels */
#define LVL_EPICSTART   21
/*
 * ADM flags - define admin privs for chars
 */
#define ADM_TELLALL   0 /* Can use 'tell all' to broadcast GOD */
#define ADM_SEEINV    1 /* Sees other chars inventory IMM */
#define ADM_SEESECRET   2 /* Sees secret doors IMM */
#define ADM_KNOWWEATHER   3 /* Knows details of weather GOD */
#define ADM_FULLWHERE   4 /* Full output of 'where' command IMM */
#define ADM_MONEY     5 /* Char has a bottomless wallet GOD */
#define ADM_EATANYTHING   6 /* Char can eat anything GOD */
#define ADM_NOPOISON    7 /* Char can't be poisoned IMM */
#define ADM_WALKANYWHERE  8 /* Char has unrestricted walking IMM */
#define ADM_NOKEYS    9 /* Char needs no keys for locks GOD */
#define ADM_INSTANTKILL   10  /* "kill" command is instant IMPL */
#define ADM_NOSTEAL   11  /* Char cannot be stolen from IMM */
#define ADM_TRANSALL    12  /* Can use 'trans all' GRGOD */
#define ADM_SWITCHMORTAL  13  /* Can 'switch' to a mortal PC body IMPL */
#define ADM_FORCEMASS   14  /* Can force rooms or all GRGOD */
#define ADM_ALLHOUSES   15  /* Can enter any house GRGOD */
#define ADM_NODAMAGE    16  /* Cannot be damaged IMM */
#define ADM_ALLSHOPS    17  /* Can use all shops GOD */
#define ADM_CEDIT   18  /* Can use cedit IMPL */
/* Level of the 'freeze' command */
#define ADMLVL_FREEZE ADMLVL_GRGOD
#define NUM_OF_DIRS 12  /* number of directions in a room (nsewud) */
/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC  100000    /* 10 passes per second */
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC    * PASSES_PER_SEC
#define PULSE_ZONE  (CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE    (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define PULSE_AUCTION	(20 RL_SEC)
#define PULSE_AUTOSAVE  (CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD (CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY  (CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE (CONFIG_PULSE_SANITY * 60 RL_SEC)   /* 5 mins */
#define PULSE_TIMESAVE  (CONFIG_PULSE_TIMESAVE * 60 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT (CONFIG_PULSE_CURRENT RL_SEC)
#define PULSE_CRAFTING (10 RL_SEC)
#define PULSE_D20_ROUND (6 RL_SEC)


/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (24 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       500          /* Max length of prompt        */
#define GARBAGE_SPACE   32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE   1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE    (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)
#define HISTORY_SIZE    5 /* Keep last 5 commands. */
#define MAX_STRING_LENGTH 64000
#define MSL MAX_STRING_LENGTH
#define MAX_INPUT_LENGTH  1024 /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH  1024 /* Max size of *raw* input */
#define MAX_MESSAGES    60
#define MAX_NAME_LENGTH   20
#define MAX_PWD_LENGTH    30
#define MAX_TITLE_LENGTH  54
#define HOST_LENGTH   30
#define EXDSCR_LENGTH   800
#define MAX_TONGUE    3
#define MAX_SKILLS    1000
#define MAX_AFFECT    32
#define MAX_OBJ_AFFECT    6
#define MAX_NOTE_LENGTH   100000  /* arbitrary */
#define SKILL_TABLE_SIZE  1000
#define SPELLBOOK_SIZE    50
#define MAX_FEATS         750
/* define the largest set of commands for a trigger */
#define MAX_CMD_LENGTH          16384 /* 16k should be plenty and then some */
/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  if you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  if you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif
/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char   sbyte;
typedef unsigned char   ubyte;
typedef signed short int  sh_int;
typedef unsigned short int  ush_int;
#if !defined(__cplusplus) /* Anyone know a portable method? */
typedef char      bool;
#endif
#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)  /* Hm, sysdep.h? */
typedef signed char     byte;
#endif
/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE trig_vnum;
typedef IDXTYPE guild_vnum;
typedef IDXTYPE qst_vnum;
/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE trig_rnum;
typedef IDXTYPE guild_rnum;
typedef IDXTYPE qst_rnum;
/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int bitvector_t;
/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char *keyword;                 /* Keyword in look/examine          */
   char *description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};
/* object-related structures ******************************************/
#define NUM_OBJ_VAL_POSITIONS 16
#define VAL_ALL_HEALTH                4
#define VAL_ALL_MAXHEALTH             5
#define VAL_ALL_MATERIAL              7
/*
 * Uses for generic object values on specific object types
 * Please use these instead of numbers to prevent overlaps.
 */
#define VAL_LIGHT_UNUSED1             0
#define VAL_LIGHT_UNUSED2             1
#define VAL_LIGHT_HOURS               2
#define VAL_LIGHT_UNUSED4             3
#define VAL_LIGHT_HEALTH              4
#define VAL_LIGHT_MAXHEALTH           5
#define VAL_LIGHT_UNUSED7             6
#define VAL_LIGHT_MATERIAL            7
#define VAL_SCROLL_LEVEL              0
#define VAL_SCROLL_SPELL1             1
#define VAL_SCROLL_SPELL2             2
#define VAL_SCROLL_SPELL3             3
#define VAL_SCROLL_HEALTH             4
#define VAL_SCROLL_MAXHEALTH          5
#define VAL_SCROLL_UNUSED7            6
#define VAL_SCROLL_MATERIAL           7
#define VAL_WAND_LEVEL                0
#define VAL_WAND_MAXCHARGES           1
#define VAL_WAND_CHARGES              2
#define VAL_WAND_SPELL                3
#define VAL_WAND_HEALTH               4
#define VAL_WAND_MAXHEALTH            5
#define VAL_WAND_UNUSED7              6
#define VAL_WAND_MATERIAL             7
#define VAL_STAFF_LEVEL               0
#define VAL_STAFF_MAXCHARGES          1
#define VAL_STAFF_CHARGES             2
#define VAL_STAFF_SPELL               3
#define VAL_STAFF_HEALTH              4
#define VAL_STAFF_MAXHEALTH           5
#define VAL_STAFF_UNUSED7             6
#define VAL_STAFF_MATERIAL            7
#define VAL_WEAPON_SKILL              0
#define VAL_WEAPON_DAMDICE            1
#define VAL_WEAPON_DAMSIZE            2
#define VAL_WEAPON_DAMTYPE            3
#define VAL_WEAPON_HEALTH             4
#define VAL_WEAPON_MAXHEALTH          5
#define VAL_WEAPON_CRITTYPE           6
#define VAL_WEAPON_MATERIAL           7
#define VAL_WEAPON_CRITRANGE          8
#define VAL_FIREWEAPON_UNUSED1        0
#define VAL_FIREWEAPON_UNUSED2        1
#define VAL_FIREWEAPON_UNUSED3        2
#define VAL_FIREWEAPON_UNUSED4        3
#define VAL_FIREWEAPON_HEALTH         4
#define VAL_FIREWEAPON_MAXHEALTH      5
#define VAL_FIREWEAPON_UNUSED7        6
#define VAL_FIREWEAPON_MATERIAL       7
#define VAL_MISSILE_UNUSED1           0
#define VAL_MISSILE_UNUSED2           1
#define VAL_MISSILE_UNUSED3           2
#define VAL_MISSILE_UNUSED4           3
#define VAL_MISSILE_HEALTH            4
#define VAL_MISSILE_MAXHEALTH         5
#define VAL_MISSILE_UNUSED7           6
#define VAL_MISSILE_MATERIAL          7
#define VAL_TREASURE_UNUSED1          0
#define VAL_TREASURE_UNUSED2          1
#define VAL_TREASURE_UNUSED3          2
#define VAL_TREASURE_UNUSED4          3
#define VAL_TREASURE_HEALTH           4
#define VAL_TREASURE_MAXHEALTH        5
#define VAL_TREASURE_UNUSED7          6
#define VAL_TREASURE_MATERIAL         7
#define VAL_ARMOR_APPLYAC             0
#define VAL_ARMOR_SKILL               1
#define VAL_ARMOR_MAXDEXMOD           2
#define VAL_ARMOR_CHECK               3
#define VAL_ARMOR_HEALTH              4
#define VAL_ARMOR_MAXHEALTH           5
#define VAL_ARMOR_SPELLFAIL           6
#define VAL_ARMOR_MATERIAL            7
#define VAL_POTION_LEVEL              0
#define VAL_POTION_SPELL1             1
#define VAL_POTION_SPELL2             2
#define VAL_POTION_SPELL3             3
#define VAL_POTION_HEALTH             4
#define VAL_POTION_MAXHEALTH          5
#define VAL_POTION_UNUSED7            6
#define VAL_POTION_MATERIAL           7
#define VAL_WORN_UNUSED1              0
#define VAL_WORN_UNUSED2              1
#define VAL_WORN_UNUSED3              2
#define VAL_WORN_UNUSED4              3
#define VAL_WORN_HEALTH               4
#define VAL_WORN_MAXHEALTH            5
#define VAL_WORN_UNUSED7              6
#define VAL_WORN_MATERIAL             7
#define VAL_OTHER_UNUSED1             0
#define VAL_OTHER_UNUSED2             1
#define VAL_OTHER_UNUSED3             2
#define VAL_OTHER_UNUSED4             3
#define VAL_OTHER_HEALTH              4
#define VAL_OTHER_MAXHEALTH           5
#define VAL_OTHER_UNUSED7             6
#define VAL_OTHER_MATERIAL            7
#define VAL_TRASH_UNUSED1             0
#define VAL_TRASH_UNUSED2             1
#define VAL_TRASH_UNUSED3             2
#define VAL_TRASH_UNUSED4             3
#define VAL_TRASH_HEALTH              4
#define VAL_TRASH_MAXHEALTH           5
#define VAL_TRASH_UNUSED7             6
#define VAL_TRASH_MATERIAL            7
#define VAL_TRAP_SPELL                0
#define VAL_TRAP_HITPOINTS            1
#define VAL_TRAP_UNUSED3              2
#define VAL_TRAP_UNUSED4              3
#define VAL_TRAP_HEALTH               4
#define VAL_TRAP_MAXHEALTH            5
#define VAL_TRAP_UNUSED7              6
#define VAL_TRAP_MATERIAL             7
#define VAL_CONTAINER_CAPACITY        0
#define VAL_CONTAINER_FLAGS           1
#define VAL_CONTAINER_KEY             2
#define VAL_CONTAINER_CORPSE          3
#define VAL_CONTAINER_HEALTH          4
#define VAL_CONTAINER_MAXHEALTH       5
#define VAL_CONTAINER_CORPSE_RACE     6
#define VAL_CONTAINER_MATERIAL        7
#define VAL_CONTAINER_OWNER           8
#define VAL_NOTE_LANGUAGE             0
#define VAL_NOTE_UNUSED2              1
#define VAL_NOTE_UNUSED3              2
#define VAL_NOTE_UNUSED4              3
#define VAL_NOTE_HEALTH               4
#define VAL_NOTE_MAXHEALTH            5
#define VAL_NOTE_UNUSED7              6
#define VAL_NOTE_MATERIAL             7
#define VAL_DRINKCON_CAPACITY         0
#define VAL_DRINKCON_HOWFULL          1
#define VAL_DRINKCON_LIQUID           2
#define VAL_DRINKCON_POISON           3
#define VAL_DRINKCON_HEALTH           4
#define VAL_DRINKCON_MAXHEALTH        5
#define VAL_DRINKCON_UNUSED7          6
#define VAL_DRINKCON_MATERIAL         7
#define VAL_KEY_UNUSED1               0
#define VAL_KEY_UNUSED2               1
#define VAL_KEY_KEYCODE               2
#define VAL_KEY_UNUSED4               3
#define VAL_KEY_HEALTH                4
#define VAL_KEY_MAXHEALTH             5
#define VAL_KEY_UNUSED7               6
#define VAL_KEY_MATERIAL              7
#define VAL_FOOD_FOODVAL              0
#define VAL_FOOD_UNUSED2              1
#define VAL_FOOD_UNUSED3              2
#define VAL_FOOD_POISON               3
#define VAL_FOOD_HEALTH               4
#define VAL_FOOD_MAXHEALTH            5
#define VAL_FOOD_UNUSED7              6
#define VAL_FOOD_MATERIAL             7
#define VAL_MONEY_SIZE                0
#define VAL_MONEY_UNUSED2             1
#define VAL_MONEY_UNUSED3             2
#define VAL_MONEY_UNUSED4             3
#define VAL_MONEY_HEALTH              4
#define VAL_MONEY_MAXHEALTH           5
#define VAL_MONEY_UNUSED7             6
#define VAL_MONEY_MATERIAL            7
#define VAL_PEN_UNUSED1               0
#define VAL_PEN_UNUSED2               1
#define VAL_PEN_UNUSED3               2
#define VAL_PEN_UNUSED4               3
#define VAL_PEN_HEALTH                4
#define VAL_PEN_MAXHEALTH             5
#define VAL_PEN_UNUSED7               6
#define VAL_PEN_MATERIAL              7
#define VAL_BOAT_UNUSED1              0
#define VAL_BOAT_UNUSED2              1
#define VAL_BOAT_UNUSED3              2
#define VAL_BOAT_UNUSED4              3
#define VAL_BOAT_HEALTH               4
#define VAL_BOAT_MAXHEALTH            5
#define VAL_BOAT_UNUSED7              6
#define VAL_BOAT_MATERIAL             7
#define VAL_FOUNTAIN_CAPACITY         0
#define VAL_FOUNTAIN_HOWFULL          1
#define VAL_FOUNTAIN_LIQUID           2
#define VAL_FOUNTAIN_POISON           3
#define VAL_FOUNTAIN_HEALTH           4
#define VAL_FOUNTAIN_MAXHEALTH        5
#define VAL_FOUNTAIN_UNUSED7          6
#define VAL_FOUNTAIN_MATERIAL         7
#define VAL_VEHICLE_ROOM              0
#define VAL_VEHICLE_UNUSED2           1
#define VAL_VEHICLE_UNUSED3           2
#define VAL_VEHICLE_APPEAR            3
#define VAL_VEHICLE_HEALTH            4
#define VAL_VEHICLE_MAXHEALTH         5
#define VAL_VEHICLE_UNUSED7           6
#define VAL_VEHICLE_MATERIAL          7
#define VAL_HATCH_DEST                0
#define VAL_HATCH_FLAGS               1
#define VAL_HATCH_DCSKILL             2
#define VAL_HATCH_DCMOVE              3
#define VAL_HATCH_HEALTH              4
#define VAL_HATCH_MAXHEALTH           5
#define VAL_HATCH_UNUSED7             6
#define VAL_HATCH_MATERIAL            7
#define VAL_HATCH_DCLOCK              8
#define VAL_HATCH_DCHIDE              9
#define VAL_WINDOW_UNUSED1            0
#define VAL_WINDOW_UNUSED2            1
#define VAL_WINDOW_UNUSED3            2
#define VAL_WINDOW_UNUSED4            3
#define VAL_WINDOW_HEALTH             4
#define VAL_WINDOW_MAXHEALTH          5
#define VAL_WINDOW_UNUSED7            6
#define VAL_WINDOW_MATERIAL           7
#define VAL_CONTROL_UNUSED1           0
#define VAL_CONTROL_UNUSED2           1
#define VAL_CONTROL_UNUSED3           2
#define VAL_CONTROL_UNUSED4           3
#define VAL_CONTROL_HEALTH            4
#define VAL_CONTROL_MAXHEALTH         5
#define VAL_CONTROL_UNUSED7           6
#define VAL_CONTROL_MATERIAL          7
#define VAL_PORTAL_DEST               0
#define VAL_PORTAL_DCSKILL            1
#define VAL_PORTAL_DCMOVE             2
#define VAL_PORTAL_APPEAR             3
#define VAL_PORTAL_HEALTH             4
#define VAL_PORTAL_MAXHEALTH          5
#define VAL_PORTAL_UNUSED7            6
#define VAL_PORTAL_MATERIAL           7
#define VAL_PORTAL_DCLOCK             8
#define VAL_PORTAL_DCHIDE             9
#define VAL_BOARD_READ                0
#define VAL_BOARD_WRITE               1
#define VAL_BOARD_ERASE               2
#define VAL_BOARD_UNUSED4             3
#define VAL_BOARD_HEALTH              4
#define VAL_BOARD_MAXHEALTH           5
#define VAL_BOARD_UNUSED7             6
#define VAL_BOARD_MATERIAL            7
#define VAL_DOOR_DCLOCK               8
#define VAL_DOOR_DCHIDE               9

#define MAX_FORM_POSITIONS            18
#define FORM_POS_FRONT                ((MAX_FORM_POSITIONS / 3) - 1)
#define FORM_POS_MIDDLE               ((MAX_FORM_POSITIONS / 3 * 2) - 1)
#define FORM_POS_BACK                 ((MAX_FORM_POSITIONS) - 1)

#define MAX_SHORT_DESC_LENGTH           51
#define MAX_LONG_DESC_LENGTH            81
#define MAX_DESCRIPTION_LENGTH          650

#define MAX_BACKGROUND_LENGTH           8100
#define MIN_BACKGROUND_LENGTH            810

#define MAX_INTROS                       50

#define MAX_HELP_KEYWORDS       75
#define MAX_HELP_ENTRY         (MAX_STRING_LENGTH * 4)

#define MAX_COMPLETED_QUESTS    1024

#define FEAT_LAST_FEAT		367
#define NUM_FEATS_DEFINED	368

#define MAX_UNLOCKED_RACES   100
#define MAX_UNLOCKED_CLASSES 100



/* AUCTIONING STATES */
#define AUC_NULL_STATE		0   /* not doing anything */
#define AUC_OFFERING		1   /* object has been offfered */
#define AUC_GOING_ONCE		2	/* object is going once! */
#define AUC_GOING_TWICE		3	/* object is going twice! */
#define AUC_LAST_CALL		4	/* last call for the object! */
#define AUC_SOLD		5

/* AUCTION CANCEL STATES */
#define AUC_NORMAL_CANCEL	6	/* normal cancellation of auction */
#define AUC_QUIT_CANCEL		7	/* auction canclled because player quit */
#define AUC_WIZ_CANCEL		8	/* auction cancelled by a god */

/* OTHER JUNK */
#define AUC_STAT		9
#define AUC_BID			10


/* Map options (settable in cedit) */
#define MAP_OFF      0
#define MAP_ON       1
#define MAP_IMM_ONLY 2

#define NUM_NO_AFFECT_EQ 16

struct obj_affected_type {
   int location;       /* Which ability to change (APPLY_XXX) */
   int specific;       /* Some locations have parameters      */
   int modifier;       /* How much it changes by              */
};
struct obj_spellbook_spell {
   ush_int spellname; /* Which spell is written */
   ubyte pages;   /* How many pages does it take up */
};
struct object_flags {
  sbyte skin_data[4]; // Objects dropped from skinning a creature
};

struct xtm
{
    unsigned int year, mon, day, hour, min, sec;
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;  /* Where in data-base     */
   room_rnum in_room;   /* In what room -1 when conta/carr  */
   int  value[NUM_OBJ_VAL_POSITIONS*2];   /* Values of the item (see list)    */
   byte type_flag;      /* Type of item                        */
   ubyte  level;           /* Minimum level of object.            */
   int  wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
   int  extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
   ush_int  weight;         /* Weigt what else                     */
   unsigned int  cost;           /* Value when sold (gp.)               */
   ush_int  cost_per_day;   /* Cost to keep pr. real day           */
   ush_int  timer;          /* Timer for object                    */
   int  bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
   byte  size;           /* Size class of object                */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */
   char *name;                    /* Title of object :get etc.        */
   char *description;     /* When in room                     */
   char *short_description;       /* when worn/carry/in cont.         */
   char *action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;   /* Worn by?           */
   ubyte worn_on;      /* Worn where?          */
   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */
   long id;                       /* used by DG triggers              */
   time_t generation;             /* creation time for dupe check     */
   unsigned long long unique_id;  /* random bits for dupe check       */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */
   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
   struct obj_data *next_auction; /* For the object list              */
   struct obj_spellbook_spell *sbinfo;  /* For spellbook info */
   struct object_flags *obj_flags;
   sbyte skin_data[4]; // Objects dropped from skinning a creature
   sbyte item_lore_type;
   long int date_sold;
   struct obj_data *next_in_bank; // for the item bank
};
/* ======================================================================= */
/* room-related structures ************************************************/
struct room_direction_data {
   char *general_description;       /* When look DIR.     */
   char *keyword;   /* for open/close     */
   sh_int /*bitvector_t*/ exit_info;  /* Exit info      */
   obj_vnum key;    /* Key's number (-1 for no key)   */
   room_rnum to_room;   /* Where direction leads (NOWHERE)  */
   ubyte dclock;      /* DC to pick the lock      */
   ubyte dchide;      /* DC to find hidden      */
   ush_int dcskill;     /* Skill req. to move through exit  */
   ubyte dcmove;      /* DC for skill to move through exit  */
   ubyte failsavetype;    /* Saving Throw type on skill fail  */
   ubyte dcfailsave;    /* DC to save against on fail   */
   unsigned int failroom;    /* Room # to put char in when fail > 5  */
   unsigned int totalfailroom;   /* Room # if char fails save < 5  */
};
/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;    /* Rooms number (vnum)          */
   zone_rnum zone;              /* Room zone (for resetting)          */
   ubyte  sector_type;            /* sector type (move/hide)            */
   char *name;                  /* Rooms name 'You are ...'           */
   char *description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int room_flags[RF_ARRAY_MAX];   /* DEATH,DARK ... etc */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */
   ubyte light;                  /* Number of lightsources in room     */
   SPECIAL(*func);
   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
   int timed;                   /* For timed Dt's                     */
   struct affected_type *affected;
};
/* ====================================================================== */
/* char-related structures ************************************************/
/* memory structure for characters */
struct memory_rec_struct {
   long id;
   struct memory_rec_struct *next;
};
typedef struct memory_rec_struct memory_rec;
/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};
/* These data contain information about a players time data */
struct time_data {
   time_t birth;  /* This represents the characters current age        */
   time_t created;  /* This does not change                              */
   time_t maxage; /* This represents death by natural causes           */
   time_t logon;  /* Time of the last logon (used to calculate played) */
   time_t played; /* This is the total accumulated time played in secs */
};
/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  ubyte level;    /* max level for this time limit  */
  int days;   /* time limit in days     */
};
/* Char's abilities. */
struct abil_data {
   sbyte str;            /* New stats can go over 18 freely, no more /xx */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};
/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. this structure
 * can be changed freely.
 */
struct player_special_data {
  char *poofin;     /* Description on arrival of a god.     */
  char *poofout;    /* Description upon a god's exit.       */
  struct alias_data *aliases; /* Character's aliases                  */
  long last_tell;   /* idnum of last tell from              */
  void *last_olc_targ;    /* olc control                          */
  byte last_olc_mode;    /* olc control                          */
  char *host;     /* host of last logon                   */
  byte spell_level[MAX_SPELL_LEVEL];
        /* bonus to number of spells memorized */
  sh_int wimp_level;   /* Below this # of hit points, flee!  */
  byte freeze_level;    /* Level of god who froze char, if any  */
  byte invis_level;   /* level of invisibility    */
  room_vnum load_room;    /* Which room to place char in    */
  int pref[PR_ARRAY_MAX]; /* preference flags for PC's.   */
  sbyte bad_pws;    /* number of bad password attemps */
  sbyte conditions[3];    /* Drunk, full, thirsty     */
  byte skill_points;   /* Skill points earned from race HD */
  byte class_skill_points[NUM_CLASSES];
        /* Skill points earned from a class */
  sh_int olc_zone;     /* Zone where OLC is permitted    */
  sh_int speaking;     /* Language currently speaking    */
  byte tlevel;     /* Turning level      */
  byte ability_trains;   /* How many stat points can you train?  */
  byte feat_points;    /* How many general feats you can take  */
  byte epic_feat_points;   /* How many epic feats you can take */
  byte class_feat_points[NUM_CLASSES];
        /* How many class feats you can take  */
  byte epic_class_feat_points[NUM_CLASSES];
        /* How many epic class feats    */
  byte domain[NUM_DOMAINS];
  byte school[NUM_SCHOOLS];
  byte diety;
  byte spell_mastery_points;
  char *color_choices[NUM_COLOR];
  byte page_length;
        /* Choices for custom colors    */
  byte form_pos;
  byte form_total[MAX_FORM_POSITIONS];

  sbyte rules_read[NUM_RULES];

  char *background_1;   // Used to store background
  char *background_2;   // Used to store background
  char *background_3;   // Used to store background
  char *background_4;   // Used to store background
  char *background_5;   // Used to store background
  char *background_6;   // Used to store background
  char *background_7;   // Used to store background
  char *background_8;   // Used to store background

  sh_int intro_list[MAX_INTROS][1];                   // Stores pfilepos of chars known
  sh_int intros_given;
  sh_int intros_received;

  sh_int alignment;
  sh_int alignment_ethic;

  byte chosen_class;
  sbyte level_stage;

  byte smite_evil; // Number of smite evil uses
  sh_int enmity;   // Hate accrued by character
  sbyte expertise_mod;  // modifier from the expertise feat
  sh_int deity; // Deity the character follows
  byte domain_one; // Cleric's domain one
  byte domain_two; // Cleric's domain two
  sbyte class_sponsor[NUM_CLASSES]; // Whether a character is sponsored in a class or not.
  byte research_tokens;

  sbyte extract; //Should this character be extracted?

  char *keywords;
  char *short_descr;
  char *description;

  unsigned int companion_vnum;
  unsigned int familiar_vnum;
  unsigned int pet_vnum;
  unsigned int mount_vnum;

  char *name_dis;

  byte turn_undead;

  int ticks_passed;

  struct obj_data *craftingObject;
  byte craftingType;
  byte craftingProgress;

  int accountNumber;

  byte poisonDamageAmount;
  byte poisonDamageType;

  char *irda_short_descr_one;
  char *irda_description_one;
  char *irda_keywords_one;
  char *irda_name_one;
  char *irda_title_one;

  char *irda_short_descr_two;
  char *irda_description_two;
  char *irda_keywords_two;
  char *irda_name_two;
  char *irda_title_two;

  byte irda_shape_status;

  byte sdesc_descriptor_1;
  byte sdesc_descriptor_2;
  byte sdesc_adjective_1;
  byte sdesc_adjective_2;

  byte disguise_gender;

  byte disguise_race;

  byte disguise_descriptor_1;
  byte disguise_descriptor_2;
  byte disguise_adjective_1;
  byte disguise_adjective_2;

  sbyte approved;

  sh_int RKit;

  int temp_load_room;

  room_rnum recall_room;

  byte mem_type;

  sh_int tavern_exp_bonus_rounds;

  char *account_name;

  sbyte skill_focus[100];

   qst_vnum *completed_quests;           /* Quests completed              */
   sh_int    num_completed_quests;          /* Number completed              */
   int    current_quest;                 /* vnum of current quest         */
   sh_int    quest_time;                    /* time left on current quest    */
   byte    quest_counter;                 /* Count of targets left to get  */

   byte companion_type;		// type of animal companion is
   char *companion_name;	// animal companion's name
   char *companions_desc;	// companion describer

  sbyte death_attack;               // character's death attack type
  sh_int death_attack_rounds;        // number of rounds player has been trying for death attack
  sh_int mark_rounds;                // number of rounds a character has marked their opponent for
  struct char_data *mark_target;  // person the character is marking for assassination

  // Guild Info

  int guild;
  byte guild_rank;
  int guild_exp;
  int subguild;
  int false_ethos;
  int false_alignment;

  byte defensive_stance;

  byte caster_level;

  byte innate_abilities[700];

  sbyte epic_dodge;

  char *lfg_string;

  byte breath_weapon;

  // Mage Memorizations

  int memcursor;    /* points to the next free slot in spellmem */
  int spellmem[MAX_MEM];  /* Spell slots old system, kept for compatibility */

  // Cleric Memorizations

  int memcursor_c;    /* points to the next free slot in spellmem */
  int spellmem_c[MAX_MEM];  /* Spell slots old system, kept for compatibility */

  // Paladin Memorizations

  int memcursor_p;    /* points to the next free slot in spellmem */
  int spellmem_p[MAX_MEM];  /* Spell slots old system, kept for compatibility */

  // Druid Memorizations

  int memcursor_d;    /* points to the next free slot in spellmem */
  int spellmem_d[MAX_MEM];  /* Spell slots old system, kept for compatibility */
  struct memorize_node *memorized_d;

  // RANGER Memorizations

  int memcursor_r;    /* points to the next free slot in spellmem */
  int spellmem_r[MAX_MEM];  /* Spell slots old system, kept for compatibility */
  struct memorize_node *memorized_r;

  // Bard Memorizations

  int memcursor_b;    /* points to the next free slot in spellmem */
  int spellmem_b[MAX_MEM];  /* Spell slots old system, kept for compatibility */
  struct memorize_node *memorized_b;


  byte mounted_attacks_avoided;

  sh_int rounds_running;

  char login_messages[MAX_STRING_LENGTH];

  byte screen_width;          /* How wide a players page is */

  sh_int questpoints;             /* A players questpoints earned */

  byte heal_roll;		// a person's heal roll from heal skill
  byte heal_amount;              // a person's heal amount
  sbyte heal_used;

  sbyte camp_used;

  byte stat_points;		// Number of stat points when creating a character

  byte crafting_type;
  sh_int crafting_ticks;
  struct obj_data *crafting_object;
  byte crafting_exp_mult;

  byte rooms_visited[65555];  // number of times room has been visited.
  unsigned int num_of_rooms_visited;   // the number of rooms that have been visited in total

  byte bonus_levels[NUM_CLASSES];
  byte bonus_levels_arcane;
  byte bonus_levels_divine;

  byte bard_songs;
  byte bard_spells[7];
  byte sorcerer_spells[10];
  byte favored_soul_spells[10];
  byte assassin_spells[5];
  byte bard_spells_to_learn[7];
  byte sorcerer_spells_to_learn[10];
  byte bard_spells_known[7];
  byte sorcerer_spells_known[10];

  double artisan_experience;

  short int wishlist[10][2];

  int spells_known[MAX_NUM_KNOWN_SPELLS];
  int spell_slots[10];

  int summon_num;
  char *summon_desc;
  int summon_attack_to_hit[5];
  int summon_attack_ndice[5];
  int summon_attack_sdice[5];
  int summon_attack_dammod[5];
  int summon_max_hit;
  int summon_cur_hit;
  int summon_ac;
  int summon_dr;
  int summon_timer;

  int mount_num;
  char *mount_desc;
  int mount_attack_to_hit[5];
  int mount_attack_ndice[5];
  int mount_attack_sdice[5];
  int mount_attack_dammod[5];
  int mount_max_hit;
  int mount_cur_hit;
  int mount_ac;
  int mount_dr;

  int companion_num;
  char *companion_desc;
  int companion_attack_to_hit[5];
  int companion_attack_ndice[5];
  int companion_attack_sdice[5];
  int companion_attack_dammod[5];
  int companion_max_hit;
  int companion_cur_hit;
  int companion_ac;
  int companion_dr;

  byte mounted;
  int mount;

  byte lay_hands;

  char *note_cat;
  char *note_subj;
  char *note;

};
/* this can be used for skills that can be used per-day */
struct memorize_node {
   ush_int    timer;      /* how many ticks till memorized */
   ush_int    spell;      /* the spell number */
   struct   memorize_node *next;  /* link to the next node */
   ubyte    class;      /* The class under which the spell is memorized */
};
struct innate_node {
   ush_int timer;
   ush_int spellnum;
   struct innate_node *next;
};
/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;      /* List of attackers to remember         */
   ubyte attack_type;        /* The Attack Type Bitvector for NPC's     */
   ubyte default_pos;        /* Default position for NPC                */
   ubyte damnodice;          /* The number of damage dice's         */
   ubyte damsizedice;        /* The size of the damage dice's           */
   ubyte newitem;             /* Check if mob has new inv item       */
   ubyte spell_slots[9];
   sbyte skin_data[4];  // Items dropped when corpse is skinned
};
/* An affect structure. */
struct affected_type {
   int type;          /* The type of spell that caused this      */
   int duration;      /* For how long its effects will last      */
   int modifier;         /* This is added to apropriate ability     */
   ubyte location;         /* Tells which ability to change(APPLY_XXX)*/
   ubyte specific;         /* Some locations have parameters          */
   long /*bitvector_t*/ bitvector; /* Tells which bits to set (AFF_XXX) */
   struct affected_type *next;
   ubyte level;            // level at which the effect is cast
};
#define MAX_DAMREDUCT_MULTI 5
#define DR_NONE     0
#define DR_ADMIN    1
#define DR_MATERIAL   2
#define DR_BONUS    3
#define DR_SPELL    4
#define DR_MAGICAL    5
#define NUM_DR_STYLES   6
struct damreduct_type {
  int duration;
  sh_int max_damage;
  sh_int spell;
  sh_int feat;
  struct obj_data *eq;
  byte mod;
  int damstyle[MAX_DAMREDUCT_MULTI];
  int damstyleval[MAX_DAMREDUCT_MULTI];
  struct damreduct_type *next;
};
/* Queued spell entry */
struct queued_act {
   ubyte level;
   ush_int spellnum;
};
/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};
#define LEVELTYPE_CLASS 1
#define LEVELTYPE_RACE  2
struct level_learn_entry {
  struct level_learn_entry *next;
  ubyte location;
  ubyte specific;
  byte value;
};
struct levelup_data {
  struct levelup_data *next;  /* Form a linked list     */
  struct levelup_data *prev;  /* Form a linked list     */
  byte type;      /* LEVELTYPE_ value     */
  byte spec;      /* Specific class or race   */
  byte level;     /* Level ir HD # for that class or race */
  byte hp_roll;     /* Straight die-roll value with no mods */
  byte mana_roll;   /* Straight die-roll value with no mods */
  byte ki_roll;     /* Straight die-roll value with no mods */
  byte move_roll;   /* Straight die-roll value with no mods */
  byte accuracy;    /* Hit accuracy change      */
  byte fort;      /* Fortitude change     */
  byte reflex;      /* Reflex change      */
  byte will;      /* Will change        */
  byte add_skill;   /* Total added skill points   */
  byte add_gen_feats;   /* General feat points      */
  byte add_epic_feats;    /* General epic feat points   */
  byte add_class_feats;   /* Class feat points      */
  byte add_class_epic_feats;  /* Epic class feat points   */
  struct level_learn_entry *skills; /* Head of linked list    */
  struct level_learn_entry *feats;  /* Head of linked list    */
  struct level_data *level_extra;
};

#define MAX_CHARS_PER_ACCOUNT         100

struct account_data {

        int id;
        char *name;
        char password[MAX_PWD_LENGTH+1];
        sbyte bad_password_count;
        char *character_names[MAX_CHARS_PER_ACCOUNT];
        ush_int experience;
        ush_int gift_experience;
        sbyte level;
        int account_flags;
        time_t last_login;
        sbyte read_rules;
        char * websiteAccount;
        byte polls[100];
        char * web_password;
        int classes[MAX_UNLOCKED_CLASSES];
        int races[MAX_UNLOCKED_RACES];
        char *email;
        int surveys[4];
        struct obj_data *item_bank;
        int item_bank_size;
        char *ignored[MAX_CHARS_PER_ACCOUNT];
};


struct player_data {

  char * keywords;
  char * short_descr;
  char * description;

};

/* ================== Structure for player/non-player ===================== */
struct char_data {
  int pfilepos;     /* playerfile pos     */
  mob_rnum nr;      /* Mob's rnum       */
  room_rnum in_room;    /* Location (real room number)    */
  room_rnum was_in_room;  /* location for linkdead people   */
  int wait;     /* wait for how many loops    */
  char passwd[MAX_PWD_LENGTH+1];
        /* character's password     */
  char *name;     /* PC / NPC s name (kill ...  )   */
  char *short_descr;    /* for NPC 'actions'      */
  char *long_descr;   /* for 'look'       */
  char *description;    /* Extra descriptions                   */
  char *title;      /* PC / NPC's title                     */
  byte size;     /* Size class of char                   */
  byte sex;     /* PC / NPC's sex                       */
  ubyte race;      /* PC / NPC's race                      */
  ubyte race_level;   /* PC / NPC's racial level / hit dice   */
  ubyte level_adj;    /* PC level adjustment                  */
  byte chclass;     /* Last class taken                     */
  ubyte chclasses[NUM_CLASSES]; /* Ranks in all classes        */
  ubyte epicclasses[NUM_CLASSES]; /* Ranks in all epic classes */
  struct levelup_data *level_info;
        /* Info on gained levels */
  ubyte level;      /* PC / NPC's level                     */
  ubyte admlevel;     /* PC / NPC's admin level               */
  int admflags[AD_ARRAY_MAX]; /* Bitvector for admin privs    */
  sh_int hometown;    /* PC s Hometown (zone)                 */
  struct time_data time;  /* PC's AGE in days     */
  int weight;     /* PC / NPC's weight                    */
  int height;     /* PC / NPC's height                    */
  struct abil_data real_abils;  /* Abilities without modifiers   */
  struct abil_data aff_abils; /* Abils with spells/stones/etc  */
  struct player_special_data *player_specials;
        /* PC specials        */
  struct mob_special_data mob_specials;
        /* NPC specials       */
  struct affected_type *affected;
        /* affected by what spells    */
  struct affected_type *affectedv;
        /* affected by what combat spells */
  struct damreduct_type *damreduct;
        /* damage resistances     */
  struct queued_act *actq;  /* queued spells / other actions  */
  struct obj_data *equipment[NUM_WEARS];
        /* Equipment array      */
  struct obj_data *carrying;  /* Head of list       */
  char *hit_breakdown[2]; /* description of last attack roll breakdowns */
  char *dam_breakdown[2]; /* description of last damage roll breakdowns */
  char *crit_breakdown[2];  /* description of last damage roll breakdowns */
  struct descriptor_data *desc; /* NULL for mobiles     */
  long id;      /* used by DG triggers      */
  struct trig_proto_list *proto_script;
        /* list of default triggers   */
  struct script_data *script; /* script info for the object   */
  struct script_memory *memory; /* for mob memory triggers    */
  struct char_data *next_in_room;
        /* For room->people - list    */
  struct char_data *next; /* For either monster or ppl-list */
  struct char_data *prev; /* For either monster or ppl-list */
  struct char_data *next_fighting;
        /* For fighting list      */
  struct char_data *next_affect;/* For affect wearoff     */
  struct char_data *next_affectv;
        /* For round based affect wearoff */
  struct follow_type *followers;/* List of chars followers    */
  struct char_data *master; /* Who is char following?   */
  long pref;				/* unique session id */
  char *hostname;			/* hostname copy */
  long master_id;
  struct memorize_node *memorized;
  struct memorize_node *memorized_c;
  struct memorize_node *memorized_p;
  struct innate_node *innate;
  struct char_data *fighting; /* Opponent       */
  struct char_data *hunting;  /* Char hunted by this char   */
  byte position;    /* Standing, fighting, sleeping, etc. */
  long long carry_weight;   /* Carried weight     */
  sh_int carry_items;   /* Number of items carried    */
  int timer;      /* Timer for update     */
  byte feats[MAX_FEATS + 1];  /* Feats (booleans and counters)  */
  int combat_feats[CFEAT_MAX+1][FT_ARRAY_MAX];
        /* One bitvector array per CFEAT_ type  */
  int school_feats[SFEAT_MAX+1];/* One bitvector array per CFEAT_ type  */
  byte skills[SKILL_TABLE_SIZE + 1];
        /* array of skills/spells/arts/etc  */
  byte skillmods[SKILL_TABLE_SIZE + 1];
        /* array of skill mods      */
  sh_int alignment;    /* +-1000 for alignment good vs. evil */
  sh_int alignment_ethic;    /* +-1000 for alignment law vs. chaos */
  long idnum;     /* player's idnum; -1 for mobiles */
  int act[PM_ARRAY_MAX];  /* act flag for NPC's; player flag for PC's */
  int affected_by[AF_ARRAY_MAX];/* Bitvector for current affects  */
  sh_int saving_throw[3]; /* Saving throw       */
  sh_int apply_saving_throw[3]; /* Saving throw bonuses     */
  ubyte powerattack;    /* Setting for power attack level */
  sh_int mana;
  sh_int max_mana;    /* Max mana for PC/NPC      */
  int hit;
  int max_hit;   /* Max hit for PC/NPC     */
  int move;
  int max_move;    /* Max move for PC/NPC      */
  int ki;
  int max_ki;    /* Max ki for PC/NPC      */
  int armor;     /* Internally stored *10    */
  int gold;     /* Money carried      */
  int bank_gold;    /* Gold the char has in a bank account  */
  int exp;      /* The experience of the player   */
  byte accuracy;     /* Base hit accuracy      */
  byte accuracy_mod;   /* Any bonus or penalty to the accuracy */
  byte damage_mod;   /* Any bonus or penalty to the damage */
  byte spellfail;   /* Total spell failure %                 */
  byte armorcheck;    /* Total armorcheck penalty with proficiency forgiveness */
  byte armorcheckall;   /* Total armorcheck penalty regardless of proficiency */

  byte mob_feats[MF_ARRAY_MAX];
  sh_int damage_taken;     // The amount of damage taken last round in combat
  byte attacks_of_opportunity; // The number of attacks of opportunity the char has made per round
  byte arrows_deflected;  // The number of arrows deflected this round
  byte rage;								// The number of rage uses left
  byte strength_of_honor;	// The number of strength of honor uses left
  byte fight_precise_attack;
  byte fight_sneak_attack;
  byte fight_spring_attack;
  byte fight_critical_hit;
  sh_int fight_damage_reduction;
  sh_int fight_damage_reduction_actual;
  byte fight_death_attack;
  byte fight_touch_of_undeath;
  sh_int fight_damage_done;
  byte fight_number_of_attacks;
  byte fight_number_of_hits;
  sbyte fight_message_printed;
  int clan;           /* PC / NPC's clan                      */
  int rank;           /* PC / NPC's clan rank                 */
  struct char_data *guarding;
  struct char_data *guarded_by;
  ubyte exp_chain;
  sbyte fight_over;
  ubyte epic_spells;
  sbyte free;
  sh_int hp_bonus;
  ubyte wish_str;
  ubyte wish_con;
  ubyte wish_dex;
  ubyte wish_int;
  ubyte wish_wis;
  ubyte wish_cha;
  sbyte spell_cast;
  ubyte autoquest_killnum;
  unsigned int autoquest_vnum;
  ubyte autoquest_qp;
  unsigned int autoquest_exp;
  unsigned int autoquest_gold;
  char * autoquest_desc;
  ubyte autocquest_makenum;
  unsigned int autocquest_vnum;
  ubyte autocquest_qp;
  unsigned int autocquest_exp;
  unsigned int autocquest_gold;
  char * autocquest_desc;
  int autocquest_material;
  byte damage_reduction_feats;
  byte fast_healing_feats;
  byte armor_skin_feats;
  byte sneak_attack_feats;
  unsigned int rp_exp;
  char *account_name;
  ubyte disguise_race;
  ubyte disguise_sex;
  ubyte disguise_dsc1;
  ubyte disguise_dsc2;
  ubyte disguise_adj1;
  ubyte disguise_adj2;
  ubyte disguise_roll;
  ubyte disguise_seen;
  ubyte touch_of_undeath;

  ush_int stat_mob_kills;

  long unsigned int rp_points;
  ush_int rp_exp_bonus;
  ush_int rp_gold_bonus;
  ush_int rp_account_exp;
  ush_int rp_qp_bonus;
  ush_int rp_craft_bonus;
  ush_int rp_art_exp_bonus;
  int summon_type;
  bool new_summon;
  char *sum_name;
  char *sum_desc;
  int sum_hd;
  struct level_data *levelup;
  sh_int combat_pos;
  ush_int in_combat;
  ubyte fight_state;
  ubyte standard_action_spent;
  ubyte move_action_spent;
  ubyte minor_action_spent;
  ubyte full_round_action;
  struct char_data *top_of_initiative;
  byte active_turn;
  byte initiative;
  sbyte end_turn;
  sbyte rolled_initiative;
  int round_num;
  byte coupdegrace;
  time_t boot_time;
  struct char_data *riding;	/* Who are they riding? (DAK) */
  struct char_data *ridden_by; /* Who is riding them? (DAK) */

  sbyte stat_points_given;

  ubyte max_fighting_level;
  sbyte total_defense;

  char *combat_cmd;
  byte petition;

  byte bleeding_damage;
  byte bleeding_attack;

  struct char_data *smiting;
  byte bounty_gem;

  byte gather_info;
  int extra_account_exp;
  sbyte weapon_supremacy_miss;

  byte crafting_repeat;

  sbyte challenge;

  long int last_kill_rnum;

  sbyte opportunist;
  sbyte sneak_opp;
  byte imp_sneak_attack_feats;

  sbyte dead;

  byte synth_value;

  unsigned int craft_vnum;
  sh_int craft_times;

  sh_int carry_strength_mod;

  byte artisan_type;

  byte mentor_level;

  byte trains_spent;
  byte trains_unspent;
  struct event * pause_combat;
  byte paralyzed;
  byte parries;
  byte parried_attacks;
  byte less_attacks;
  byte pvp_timer;
  sbyte pvp_death;

  sh_int dam_round_taken;
  sh_int dam_round_dealt;

  sbyte kill_incap_spam[12];

  sbyte milestone;

  byte stim_cooldown;

  sh_int damage_taken_last_round;

  byte combat_output;

  byte att_roll;
  byte opp_def;

  sbyte new_parry;

  byte battle_strike;
  byte dark_rage;

  sbyte force_grip_stun;
  sbyte force_grip;
  sh_int force_grip_rounds;

  byte riposte;

  byte condition;

  sbyte setstats_not_avail;

  sbyte reloaded;

  ubyte milestone_hp;

  sbyte wield_type;

  char *quickslots[2][10];

  sh_int milestone_mv;

  unsigned int bacta;

  room_rnum destination;
  sh_int travel_timer;
  byte travel_type;
  byte travel_locale;

  sbyte restringing;

  byte flat_footed;

  sh_int post_thirty_advancement;

  struct ship_data *ship1;

  ush_int reputation;

  byte pilot_faction;

  sbyte ambush;

  sbyte knockdown;

  char *new_mail_receiver;
  char *new_mail_subject;
  char *new_mail_content;

  sbyte scavenge;

  int advancement_points;

  int crafting_level;

  sbyte djem;

  byte evasive_fighting;

  sbyte natural_20;

  byte dazed;
  byte surge_amount;
  byte surge_ticks;

  int mob_killed_type;

  sbyte aim;

  byte cover_fire;

  sbyte hunters_target;

  byte familiar_foe;

  char *rp_pose;

  sh_int wishes;

  long apprentice_id;
  time_t apprentice_cancel_timestamp;

  sh_int hard_mode;
};

struct pettable_data
{
	int class_required;
	char *name;
	int required_level;
	int type;
	int str_mod;
	int dex_mod;
	int con_mod;
	int hd_mod;
        int c_type;
};


struct coin_data
{
  sbyte adamantine;
  sbyte mithril;
  sbyte steel;
  sbyte bronze;
  sbyte copper;
};


/* ====================================================================== */
/* descriptor-related structures ******************************************/
struct txt_block {
   char *text;
   int aliased;
   struct txt_block *next;
};
struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};
struct compr {
    int state; /* 0 - off. 1 - waiting for response. 2 - compress2 on */
#ifdef HAVE_ZLIB_H
    Bytef *buff_out;
    int total_out; /* size of input buffer */
    int size_out; /* size of data in output buffer */
    Bytef *buff_in;
    int total_in; /* size of input buffer */
    int size_in; /* size of data in input buffer */
    z_streamp stream;
#endif /* HAVE_ZLIB_H */
};

struct descriptor_data {
   socket_t descriptor; /* file descriptor for socket   */
   char host[HOST_LENGTH+1];  /* hostname       */
   byte bad_pws;    /* number of bad pw attemps this login  */
   byte idle_tics;    /* tics idle at password prompt   */
   ush_int  connected;    /* mode of 'connectedness'    */
   int  desc_num;   /* unique num assigned to desc    */
   time_t login_time;   /* when the person connected    */
   char *showstr_head;    /* for keeping track of an internal str */
   char **showstr_vector; /* for paging through texts   */
   ubyte  showstr_count;    /* number of pages to page through  */
   ubyte  showstr_page;   /* which page are we currently showing? */
   char **str;      /* for the modify-str system    */
   char *backstr;   /* backup string for modify-str system  */
   size_t max_str;          /* maximum size of string in modify-str */
   long mail_to;    /* name for mail system     */
   sbyte  has_prompt;   /* is the user at a prompt?             */
   char inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input   */
   char last_input[MAX_INPUT_LENGTH]; /* the last input     */
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer   */
   char *output;    /* ptr to the current output buffer */
   char **history;    /* History of commands, for ! mostly. */
   int  history_pos;    /* Circular array position.   */
   int  bufptr;     /* ptr to end of current output   */
   int  bufspace;   /* space left in the output buffer  */
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;    /* q of unprocessed input   */
   struct char_data *character; /* linked to char     */
   struct char_data *original;  /* original char if switched    */
   struct descriptor_data *snooping; /* Who is this char snooping */
   struct descriptor_data *snoop_by; /* And who is snooping this char */
   struct descriptor_data *next; /* link to next descriptor   */
   struct oasis_olc_data *olc;   /* OLC info                            */
   struct compr *comp;                /* compression info */
   struct account_data *account;
   byte copyover;
   protocol_t *pProtocol;
};

/* other miscellaneous structures ***************************************/
struct msg_type {
   char *attacker_msg;  /* message to attacker */
   char *victim_msg;    /* message to victim   */
   char *room_msg;      /* message to room     */
};
struct message_type {
   struct msg_type die_msg; /* messages when death      */
   struct msg_type miss_msg;  /* messages when miss     */
   struct msg_type hit_msg; /* messages when hit      */
   struct msg_type god_msg; /* messages when hit on god   */
   struct message_type *next; /* to next messages of this kind. */
};
struct message_list {
   int  a_type;     /* Attack type        */
   int  number_of_attacks;  /* How many attack messages to chose from. */
   struct message_type *msg;  /* List of messages.      */
};
/* used in the socials */
struct social_messg {
  int act_nr;
  char *command;               /* holds copy of activating command */
  char *sort_as;              /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
  int hide;                   /* ? */
  int min_victim_position;    /* Position of victim */
  int min_char_position;      /* Position of char */
  int min_level_char;          /* Minimum level of socialing char */
  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;
  /* An argument was there, and a victim was found */
  char *char_found;
  char *others_found;
  char *vict_found;
  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found;
  char *others_body_found;
  char *vict_body_found;
  /* An argument was there, but no victim was found */
  char *not_found;
  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;
  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found;
  char *others_obj_found;
};
struct weather_data {
   int  pressure; /* How is the pressure ( Mb ) */
   int  change; /* How fast and what way does it change. */
   int  sky;  /* How is the sky. */
   int  sunlight; /* And how much sun. */
};
/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
   mob_vnum vnum; /* virtual number of this mob/obj   */
   unsigned int    number; /* number of existing units of this mob/obj */
   SPECIAL(*func);
   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};
/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  unsigned int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};
struct guild_info_type {
  ubyte pc_class;
  room_vnum guild_room;
  ubyte direction;
};
/*
 * Config structs
 *
 */

 /*
 * The game configuration structure used for configurating the game play
 * variables.
 */
struct game_data {
  sbyte pk_allowed;         /* Is player killing allowed?     */
  sbyte pt_allowed;         /* Is player thieving allowed?    */
  ubyte level_can_shout;    /* Level player must be to shout.   */
  ush_int holler_move_cost;   /* Cost to holler in move points.   */
  sbyte tunnel_size;        /* Number of people allowed in a tunnel.*/
  unsigned int max_exp_gain;       /* Maximum experience gainable per kill.*/
  unsigned int max_exp_loss;       /* Maximum experience losable per death.*/
  ush_int max_npc_corpse_time;/* Num tics before NPC corpses decompose*/
  ush_int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
  ush_int idle_void;          /* Num tics before PC sent to void(idle)*/
  ush_int idle_rent_time;     /* Num tics before PC is autorented.    */
  ubyte idle_max_level;     /* Level of players immune to idle.     */
  sbyte dts_are_dumps;      /* Should items in dt's be junked?    */
  sbyte load_into_inventory;/* Objects load in immortals inventory. */
  sbyte track_through_doors;/* Track through doors while closed?    */
  ubyte level_cap;          /* You cannot level to this level       */
  sbyte stack_mobs;   /* Turn mob stacking on                 */
  sbyte stack_objs;   /* Turn obj stacking on                 */
  sbyte mob_fighting;       /* Allow mobs to attack other mobs.     */
  char *OK;               /* When player receives 'Okay.' text.   */
  char *NOPERSON;         /* 'No-one by that name here.'    */
  char *NOEFFECT;         /* 'Nothing seems to happen.'           */
  sbyte disp_closed_doors;  /* Display closed doors in autoexit?    */
  sbyte reroll_player;      /* Players can reroll stats on creation */
  ubyte initial_points;	  /* Initial points pool size		  */
  sbyte enable_compression; /* Enable MCCP2 stream compression      */
  sbyte enable_languages;   /* Enable spoken languages              */
  sbyte all_items_unique;   /* Treat all items as unique      */
  float exp_multiplier;     /* Experience gain  multiplier    */
  sbyte map_option;         /* MAP_ON, MAP_OFF or MAP_IMM_ONLY      */
  ubyte map_size;           /* Default size for map command         */
  ubyte minimap_size;       /* Default size for mini-map (automap)  */
  sbyte campaign;            /* Campaign setting to use */
};
/*
 * The rent and crashsave options.
 */
struct crash_save_data {
  sbyte free_rent;          /* Should the MUD allow rent for free?  */
  ush_int max_obj_save;       /* Max items players can rent.          */
  ush_int min_rent_cost;      /* surcharge on top of item costs.    */
  sbyte auto_save;          /* Does the game automatically save ppl?*/
  int autosave_time;      /* if auto_save=TRUE, how often?        */
  int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
  int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};
/*
 * The room numbers.
 */
struct room_numbers {
  room_vnum mortal_start_room;  /* vnum of room that mortals enter at.  */
  room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
  room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1;    /* vnum of donation room #1.            */
  room_vnum donation_room_2;    /* vnum of donation room #2.            */
  room_vnum donation_room_3;    /* vnum of donation room #3.          */
};
/*
 * The game operational constants.
 */
struct game_operation {
  ush_int DFLT_PORT;        /* The default port to run the game.  */
  char *DFLT_IP;            /* Bind to all interfaces.      */
  char *DFLT_DIR;           /* The default directory (lib).   */
  char *LOGNAME;            /* The file to log messages to.   */
  ush_int max_playing;          /* Maximum number of players allowed. */
  unsigned int max_filesize;         /* Maximum size of misc files.    */
  sbyte max_bad_pws;          /* Maximum number of pword attempts.  */
  sbyte siteok_everyone;      /* Everyone from all sites are SITEOK.*/
  sbyte nameserver_is_slow;   /* Is the nameserver slow or fast?    */
  sbyte use_new_socials;      /* Use new or old socials file ?      */
  sbyte auto_save_olc;        /* Does OLC save to disk right away ? */
  char *MENU;               /* The MAIN MENU.       */
  char *WELC_MESSG;     /* The welcome message.     */
  char *START_MESSG;        /* The start msg for new characters.  */
};
/*
 * The Autowizard options.
 */
struct autowiz_data {
  sbyte use_autowiz;        /* Use the autowiz feature?   */
  ubyte min_wizlist_lev;    /* Minimun level to show on wizlist.  */
};
/* This is for the tick system.
 *
 */

struct tick_data {
  int pulse_violence;
  int pulse_mobile;
  int pulse_zone;
  int pulse_autosave;
  int pulse_idlepwd;
  int pulse_sanity;
  int pulse_usage;
  int pulse_timesave;
  int pulse_current;
};
/*
 * The character advancement (leveling) options.
 */
struct advance_data {
  sbyte allow_multiclass; /* Allow advancement in multiple classes     */
};
/*
 * The new character creation method options.
 */
struct creation_data {
  byte method; /* What method to use for new character creation */
};
/*
 * The main configuration structure;
 */
struct config_data {
  char                   *CONFFILE; /* config file path  */
  struct game_data       play;    /* play related config   */
  struct crash_save_data csd;   /* rent and save related */
  struct room_numbers    room_nums; /* room numbers          */
  struct game_operation  operation; /* basic operation       */
  struct autowiz_data    autowiz; /* autowiz related stuff */
  struct advance_data    advance;   /* char advancement stuff */
  struct tick_data       ticks;   /* game tick stuff   */
  struct creation_data   creation;  /* char creation method  */
};
/*
 * Data about character aging
 */
struct aging_data {
  int adult;    /* Adulthood */
  int classdice[3][2];  /* Dice info for starting age based on class age type */
  int middle;   /* Middle age */
  int old;    /* Old age */
  int venerable;  /* Venerable age */
  int maxdice[2]; /* For roll to determine natural death beyond venerable */
};

struct weapon_table
{

  char *name;
  sbyte numDice;
  ubyte diceSize;
  sbyte critRange;
  sbyte critMult;
  ush_int weaponFlags;
  ush_int cost;
  ush_int damageTypes;
  ush_int weight;
  ubyte range;
  ush_int weaponFamily;
  byte size;
  ubyte material;
  ubyte handle_type;
  ubyte head_type;
  ubyte stunNumDice;
  ubyte stunSizeDice;
  ubyte availability;
  ush_int ammo;
};

struct armor_table
{
  char *name;
  ubyte armorType;
  ush_int cost;
  ubyte armorBonus;
  ubyte dexBonus;
  byte armorCheck;
  ubyte spellFail;
  ubyte thirtyFoot;
  ubyte twentyFoot;
  ush_int weight;
  ubyte material;
  ubyte availability;
  ubyte fort_bonus;
};

struct pet_data {

  char *desc;
  int level;
  int max_hit;
  int ac;
  int dr;
  int attacks_to_hit[5];
  int attacks_ndice[5];
  int attacks_sdice[5];
  int attacks_dammod[5];
  int mount;
  int size;
  int speed;
  int skill;
  int cost;
  int flying;

};

struct race_data {

  char *name;
  char *abbrev;
  char *type;
  ubyte family;
  sbyte genders[NUM_SEX];
  char *menu_display;
  byte ability_mods[6];
  ush_int height[NUM_SEX];
  ush_int weight[NUM_SEX];
  byte size;
  int body_parts[NUM_WEARS];
  sbyte alignments[9];
  sbyte is_pc;
  byte favored_class[NUM_SEX];
  ush_int language;
  ubyte level_adjustment;
};

// Item Availability
#define AVAILABILITY_NORMAL     0
#define AVAILABILITY_LICENSED   1
#define AVAILABILITY_RESTRICTED 2
#define AVAILABILITY_MILITARY   3
#define AVAILABILITY_RARE       4

// Output types

#define OUTPUT_NORMAL 0
#define OUTPUT_FULL   1
#define OUTPUT_SPARSE 2

// Below are the various weapon type defines

#define WEAPON_TYPE_UNDEFINED       0
#define WEAPON_TYPE_UNARMED         1
// Old Fantasy Types
#define WEAPON_TYPE_DAGGER          2
#define WEAPON_TYPE_LIGHT_MACE      3
#define WEAPON_TYPE_SICKLE          4
#define WEAPON_TYPE_CLUB            5
#define WEAPON_TYPE_HEAVY_MACE      6
#define WEAPON_TYPE_MORNINGSTAR     7
#define WEAPON_TYPE_SHORTSPEAR      8
#define WEAPON_TYPE_LONGSPEAR       9
#define WEAPON_TYPE_QUARTERSTAFF    10
#define WEAPON_TYPE_SPEAR           11
//RANGE D
#define WEAPON_TYPE_HEAVY_CROSSBOW  12
#define WEAPON_TYPE_LIGHT_CROSSBOW  13
#define WEAPON_TYPE_DART            14
#define WEAPON_TYPE_JAVELIN         15
#define WEAPON_TYPE_SLING           16
//MARTIAL WEAPONS
//MELE E
#define WEAPON_TYPE_THROWING_AXE    17
#define WEAPON_TYPE_LIGHT_HAMMER    18
#define WEAPON_TYPE_HAND_AXE        19
#define WEAPON_TYPE_KUKRI           20
#define WEAPON_TYPE_LIGHT_PICK      21
#define WEAPON_TYPE_SAP             22
#define WEAPON_TYPE_SHORT_SWORD     23
#define WEAPON_TYPE_BATTLE_AXE      24
#define WEAPON_TYPE_FLAIL           25
#define WEAPON_TYPE_LONG_SWORD      26
#define WEAPON_TYPE_HEAVY_PICK      27
#define WEAPON_TYPE_RAPIER          28
#define WEAPON_TYPE_SCIMITAR        29
#define WEAPON_TYPE_TRIDENT         30
#define WEAPON_TYPE_WARHAMMER       31
#define WEAPON_TYPE_FALCHION        32
#define WEAPON_TYPE_GLAIVE          33
#define WEAPON_TYPE_GREAT_AXE       34
#define WEAPON_TYPE_GREAT_CLUB      35
#define WEAPON_TYPE_HEAVY_FLAIL     36
#define WEAPON_TYPE_GREAT_SWORD     37
#define WEAPON_TYPE_GUISARME        38
#define WEAPON_TYPE_HALBERD         39
#define WEAPON_TYPE_LANCE           40
#define WEAPON_TYPE_RANSEUR         41
#define WEAPON_TYPE_SCYTHE          42
//RANGED
#define WEAPON_TYPE_LONG_BOW        43
#define WEAPON_TYPE_SHORT_BOW       44
#define WEAPON_TYPE_COMPOSITE_LONGBOW   45
#define WEAPON_TYPE_COMPOSITE_SHORTBOW  46
//EXOTIC WEAPONS
//MELEE
#define WEAPON_TYPE_KAMA            47
#define WEAPON_TYPE_NUNCHAKU        48
#define WEAPON_TYPE_SAI             49
#define WEAPON_TYPE_SIANGHAM        50
#define WEAPON_TYPE_BASTARD_SWORD   51
#define WEAPON_TYPE_DWARVEN_WAR_AXE 52
#define WEAPON_TYPE_WHIP            53
#define WEAPON_TYPE_SPIKED_CHAIN    54
//DOUBLE WEAPONS
#define WEAPON_TYPE_DOUBLE_AXE      55
#define WEAPON_TYPE_DIRE_FLAIL      56
#define WEAPON_TYPE_HOOKED_HAMMER   57
#define WEAPON_TYPE_2_BLADED_SWORD  58
#define WEAPON_TYPE_DWARVEN_URGOSH  59
//RANGED WEAPONS
#define WEAPON_TYPE_HAND_CROSSBOW   60
#define WEAPON_TYPE_HEAVY_REP_XBOW  61
#define WEAPON_TYPE_LIGHT_REP_XBOW  62
#define WEAPON_TYPE_BOLA            63
#define WEAPON_TYPE_NET             64
#define WEAPON_TYPE_SHURIKEN        65

//New Weapons
#define WEAPON_TYPE_FULLBLADE       66
#define WEAPON_TYPE_KHOPESH         67
#define WEAPON_TYPE_CURVE_BLADE     68
#define WEAPON_TYPE_GREATBOW        69

#define MAX_WEAPON_TYPES    69
#define NUM_WEAPON_TYPES MAX_WEAPON_TYPES
#define NUM_SW_WEAPON_TYPES 12

#define MIN_WEAPON_DAMAGE_TYPES     101
#define WEAPON_DAMAGE_TYPE_SLASHING 101
#define WEAPON_DAMAGE_TYPE_BLUDGEONING 102
#define WEAPON_DAMAGE_TYPE_PIERCING 103
#define MAX_WEAPON_DAMAGE_TYPES    103
#define MAX_FANTASY_WEAPON_DAMAGE_TYPES 103

// Below are the various armor type defines

#define ARMOR_TYPE_NONE     0
#define ARMOR_TYPE_LIGHT    1
#define ARMOR_TYPE_MEDIUM   2
#define ARMOR_TYPE_HEAVY    3
#define ARMOR_TYPE_SHIELD   4

#define MAX_ARMOR_TYPES     4

#define SPEC_ARMOR_TYPE_UNDEFINED 0
#define SPEC_ARMOR_TYPE_CLOTHING  0
#define SPEC_ARMOR_TYPE_BLAST_VEST 1
#define SPEC_ARMOR_TYPE_PADDED_FLIGHT_SUIT 2
#define SPEC_ARMOR_TYPE_COMBAT_JUMPSUIT 3
#define SPEC_ARMOR_TYPE_ARMORED_FLIGHT_SUIT 4
#define SPEC_ARMOR_TYPE_TROOPER_ARMOR 5
#define SPEC_ARMOR_TYPE_CEREMONIAL 6
#define SPEC_ARMOR_TYPE_POWER_SUIT 7
#define SPEC_ARMOR_TYPE_BATTLE_ARMOR 8
#define SPEC_ARMOR_TYPE_ARMORED_SPACE_SUIT 9
#define SPEC_ARMOR_TYPE_HEAVY_BATTLE_ARMOR 10
#define SPEC_ARMOR_TYPE_SHIELD_GENERATOR 11

// Old Fantasy Types
#define SPEC_ARMOR_TYPE_PADDED    1
#define SPEC_ARMOR_TYPE_LEATHER   2
#define SPEC_ARMOR_TYPE_STUDDED_LEATHER 3
#define SPEC_ARMOR_TYPE_LIGHT_CHAIN 4
#define SPEC_ARMOR_TYPE_HIDE    5
#define SPEC_ARMOR_TYPE_SCALE   6
#define SPEC_ARMOR_TYPE_CHAINMAIL 7
#define SPEC_ARMOR_TYPE_PIECEMEAL 8
#define SPEC_ARMOR_TYPE_SPLINT    9
#define SPEC_ARMOR_TYPE_BANDED    10
#define SPEC_ARMOR_TYPE_HALF_PLATE  11
#define SPEC_ARMOR_TYPE_FULL_PLATE  12
#define SPEC_ARMOR_TYPE_BUCKLER   13
#define SPEC_ARMOR_TYPE_SMALL_SHIELD  14
#define SPEC_ARMOR_TYPE_LARGE_SHIELD  15
#define SPEC_ARMOR_TYPE_TOWER_SHIELD  16

#define NUM_SPEC_ARMOR_TYPES   17
#define NUM_SW_SPEC_ARMOR_TYPES   12




/* Below is the structure for a feat */

struct feat_info {

  char *name;   // The name of the feat to be displayed to players
  sbyte in_game;    // TRUE or FALSE, is the feat in the game yet?
  sbyte can_learn;  // TRUE or FALSE, can the feat be learned or is it an automatic feat?
  sbyte can_stack;  // TRUE or FALSE, can the feat be learned more than once?
  char *prerequisites;
  char *description;
  sbyte epic;
  sbyte combat_feat;
};


struct auction_house_data {
  struct obj_data *obj;
  struct auction_house_data *next;
  int price;
  char *seller;
  long idnum;
  time_t date_sold;
  int active;
};

struct deity_info {

  char *name;
  int ethos;
  int alignment;
  ubyte domains[6];
  ubyte favored_weapon;
  sbyte pantheon;
  char *portfolio;
  char *description;

};

/* clan-related structures ************************************************/
struct clan_type {
  int  number;            /* clan's UNIQUE ID Number      */
  char *name;             /* No color name of clan (string)  */
  char *applicants[MAX_CLAN_APPLICANTS];/* Pointer to strings            */
  char *leadersname;      /* Leader's (Player's) Name     */
  char *rank_name[NUM_CLAN_RANKS]; /* Rank names                      */
  char *member_look_str;  /* The clan's colored name      */
  room_vnum clan_entr_room;    /* VNUM of clan Entrance Room */
  room_vnum clan_recall;           /* VNUM of clan recall room        */
  mob_rnum guard[NUM_CLAN_GUARDS]; /* RNUM of clan guard              */
  ubyte  direction;         /* Direction of clan entrance   */
  sbyte  pkill;             /* TRUE if pkill desired        */
  long  clan_gold;         /* clan gold                    */
  obj_vnum clan_eq[NUM_CLAN_RANKS];/* clan equipment                  */
  struct clan_type *next;
};

/* ====================================================================== */

// Structure for levelup data
struct level_data {
        struct level_data *next;
        struct level_data *prev;
	ubyte level;
	ubyte class;
	ubyte feats[NUM_FEATS_DEFINED];
	int combat_feats[CFEAT_MAX+1][FT_ARRAY_MAX];
	ubyte trains[6];
	ubyte skills[SKILL_TABLE_SIZE+1];
	ubyte feat_points;
	ubyte practices;
	ubyte num_trains;
	unsigned short int tempFeat;
	ubyte num_class_feats;
	ubyte epic_feat_points;
	ubyte num_epic_class_feats;
	unsigned short int feat_weapons[NUM_FEATS_DEFINED];
	unsigned short int feat_skills[NUM_FEATS_DEFINED];
        sh_int spells_known[MAX_NUM_KNOWN_SPELLS];
        byte spell_slots[10];
};

struct combat_list_type {

  struct char_data *ch;
  int initiative;
  int round;
  struct combat_list_type *next;
  struct combat_list_type *prev;

};

struct pause_event {
  struct char_data *ch;
};

struct fightsort_elem {
  struct char_data *ch;
  int init;
  int dex;
};

#define SHIP_COMP_SHIELD_GENERATOR 		0
#define SHIP_COMP_SHIELD_BATTERY		1
#define SHIP_COMP_TURRET_1			2
#define SHIP_COMP_TURRET_2			3
#define SHIP_COMP_TURRET_3			4
#define SHIP_COMP_TURRET_4			5
#define SHIP_COMP_TURRET_5			6
#define SHIP_COMP_ARMOR				7
#define SHIP_COMP_WEAPON_1			8
#define SHIP_COMP_WEAPON_2			9
#define SHIP_COMP_WEAPON_BATTERY_1		10
#define SHIP_COMP_WEAPON_BATTERY_2		11
#define SHIP_COMP_TURRET_BATTERY_1		12
#define SHIP_COMP_TURRET_BATTERY_2		13
#define SHIP_COMP_TURRET_BATTERY_3		14
#define SHIP_COMP_TURRET_BATTERY_4		15
#define SHIP_COMP_TURRET_BATTERY_5		16
#define SHIP_COMP_SHIP_COMPUTER			17
#define SHIP_COMP_TARGETTING_COMPUTER           18

#define NUM_SHIP_COMPONENT_TYPES		19


struct ship_data {

  int ship_type;
  char *name;
  struct char_data *owner;
  struct char_data *pilot;
  int max_turrets;
  struct char_data *turret1;
  struct char_data *turret2;
  struct char_data *turret3;
  struct char_data *turret4;
  struct char_data *turret5;
  int ship_hp;
  int ship_max_hp;
  int ship_shields;
  int ship_max_shields;
  int ship_components[NUM_SHIP_COMPONENT_TYPES];
  struct ship_data *docked;
  int ship_landed;
  room_rnum room1;
  room_rnum room2;
  room_rnum room3;
  room_rnum room4;
  room_rnum room5;
  room_rnum room6;
  room_rnum room7;
  room_rnum room8;
  room_rnum room9;
  room_rnum room10;
  int ship_speed;
};
