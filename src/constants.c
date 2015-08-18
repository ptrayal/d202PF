/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"	/* alias_data */
#include "spells.h"
#include "oasis.h"

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.1";

cpp_extern const char *oasisolc_version =
	"OasisOLC 2.0.6";

cpp_extern const char *ascii_pfiles_version =
  "ASCII Player Files 3.0.1";

const char *patch_list[] =
{
  "Races Support                        version 3.1",
  "Xapobjs                              version 1.2",
  "EZColor                              version 2.2",
  "Spoken Language Code                 version 2.1",
  "Copyover                             version 1.2",
  "128bit Support                       version 1.4",
  "Assembly Edit Code                   version 1.1",
  "Whois Command                        version 1.0",
  "Weapon Skill/Skill Progression       version 1.3",
  "Race/Class Restriction               version 1.0",
  "Vehicle Code                         version 2.0  (2003/10/01)",
  "Compare Object Code                  version 1.1",
  "Object Damage/Material Types         version 1.3",
  "Container Patch                      version 1.1",
  "Percentage Zone Loads                version 1.2",
  "Patch list                           version 1.3  (2003/08/27)",
  "Portal Object/Spell                  version 1.3  (2003/09/13)",
  "Mobile stacking                      version 3.1  (2003/08/30)",
  "Object stacking                      version 3.1  (2003/08/30)",
  "Stacking in cedit                    version 1.1  (2003/08/30)",
  "Fixtake in extra descs               version 1.1  (2003/06/21)",
  "Exits                                version 3.2  (2003/10/22)",
  "Seelight                             version 1.1  (2003/06/22)",
  "Manual Color                         version 3.1  (2003/04/15)",
  "Command Disable                      version 1.1  (2003/12/09)",
  "Reroll Player Stats w/ cedit         version 1.1  (2004/05/23)",
  "Spell Memorization Patch             version 2.0  (2003/07/15)",
  "Dynamic Boards                       version 2.4  (2004/04/23)",
  "MCCP2                                version cwg1.0  (2004/09/05)",
  "Dupecheck                            version 1.0  (2003/12/07)",
  "Auto-Assist                          version 1.0  (2004/9/06)",
  "Remove color question                version 1.0  (2004/9/27)",
  "Room currents                        version 1.0  (2004/9/27)",
  "Timed Deathtraps                     version 1.0  (2004/9/27)",
  "Starting Stats Edit                  version 1.0  (1999/2/24)",
  "\n",
  NULL
};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */

/* Alignments */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
const char *alignments[] = 
{
  "Lawful Good",
  "Neutral Good",
  "Chaotic Good",
  "Lawful Neutral",
  "True Neutral",
  "Chaotic Neutral",
  "Lawful Evil",
  "Neutral Evil",
  "Chaotic Evil",
  "\n",
  NULL
};

/* Armor Types */
const char *armor_type[] = 
{
  "Undefined",
  "Light",
  "Medium",
  "Heavy",
  "Shield",
  "\n",
  NULL
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *crit_type[] =
{
  "x2",
  "x3",
  "x4",
  "\n",
  NULL
};

/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "northeast",
  "southeast",
  "southwest",
  "inside",
  "outside",
  "\n",
  NULL
};
const char *abbr_dirs[] =
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "in",
  "out",
  "\n",
  NULL
};

/* ROOM_x */
const char *room_bits[] = 
{
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HOUSE_CRASH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "PKILL",
  "NORECALL",
  "GUARDED",
  "PULE_DAM",
  "NO_OOC",
  "SALTWATER_FISH",
  "FRESHWATER_FISH",
  "NODIG",
  "NOBURY",
  "SPEC_PROC",
  "PLAYER_SHOP",
  "VEHICLE",
  "UNDERGROUND",
  "CURRENT",
  "TIMED_DT",
  "WORLDMAP",
  "MINE",
  "MINE_ABUNDANT",
  "MINE_RICH",
  "TAVERN",
  "FOREST",
  "FOREST_ABUNDANT",
  "FOREST_RICH",
  "FARM",
  "FARM_ABUNDANT",
  "FARM_RICH",
  "HUNTING",
  "HUNTING_ABUNDANT",
  "HUNTING_RICH",
  "\n",
  NULL
};


/* EX_x */
const char *exit_bits[] = 
{
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "SECRET",
  "\n",
  NULL
};


/* SECT_ */
const char *sector_types[] = 
{
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "Road",
  "Cave",
  "\n",
  NULL
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n",
  NULL
};


/* POS_x */
const char *position_types[] = 
{
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "Digging",
  "Fishing",
  "Riding",
  "\n",
  NULL
};


/* PLR_x */
const char *player_bits[] = 
{
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CRASH",
  "SITEOK",
  "NOSHOUT",
  "RABIIT",
  "DELETED",
  "LOAD_ROOM",
  "WOLF",
  "BEAR",
  "COURIER",
  "CAT",
  "BIRD",
  "IMMCHAR",
  "FREE_RENT",
  "SUBDUING",
  "PRISONER",
  "FISHING",
  "FISH_ON",
  "DIGGING",
  "DIG_ON",
  "FIRE_ON",
  "MAGE",
  "CLERIC",
  "MONK",
  "BARD",
  "BEGGAR",
  "KNIGHT"
  "NOTITLE",
  "NO_WIZLIST",
  "NO_DELETE",
  "INVIS_START",
  "CRYO",
  "NOTDEADYET",
  "AGE_MID_G",
  "AGE_MID_B",
  "AGE_OLD_G",
  "AGE_OLD_B",
  "AGE_VEN_G",
  "AGE_VEN_B",
  "OLD_AGE",
  "SUICIDE",
  "\n"
};


/* MOB_x */
const char *action_bits[] = 
{
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "HUNTERKILLER",
  "USE_SPELLS",
  "DBL_ATTACK",
  "TRPL_ATTACK",
  "QUAD_ATTACK",
  "SENTRY",
  "MOUNTABLE",
  "QUEST",
  "NOKILL",
  "NOTDEADYET",
  "EXTRACTING",
  "LIEUTENANT",
  "CAPTAIN",
  "BOSS",
  "TRAIN_ALL",
  "INNOCENT",
  "GUILDMASTER",
  "NO_AUTO_GOLD",
  "CUSTOM_STATS",
  "SELLS_CRYSTALS",
  "FINAL_BOSS",
  "BLOCK_N",
  "BLOCK_E",
  "BLOCK_S",
  "BLOCK_W",
  "BLOCK_NE",
  "BLOCK_SE",
  "BLOCK_SW",
  "BLOCK_NW",
  "BLOCK_U",
  "BLOCK_D",
  "BLOCK_RACE",
  "BLOCK_CLASS",
  "BLOCK_LEVEL",
  "BLOCK_ALIGN",
  "BLOCK_RACE_FAM",
  "RANGED_ATTK",
  "RANDOM_RACE",
  "RANDOM_GENDER",
  "MOLD_SELLER",
  "BLOCK_ALLIANCE_G",
  "BLOCK_ALLIANCE_N",
  "BLOCK_ALLIANCE_E",
  "\n"
};


/* PRF_x */
const char *preference_bits[] = 
{
  "BRIEF",
  "COMPACT",
  "DEAF",
  "NO_TELL",
  "DISP_HP",
  "DETECT",
  "DISP_MOVE",
  "AUTOEX",
  "NO_HASS",
  "RUNNING",
  "JOGGING",
  "NO_REP",
  "HOLYLIGHT",
  "COLOR_1",
  "COLOR_2",
  "NO_WIZ",
  "LOG_1",
  "LOG_2",
  "NOT_SELF",
  "DISGUISE",
  "HUNTED",
  "ROOMFLAGS",
  "AFK",
  "UNUSED_7",
  "QUEST",
  "LEVEL_FLAGS",
  "INTRO",
  "NO_OOC",
  "NO_NEWBIE",
  "NO_CHAT",
  "TIRED",
  "DISP_MANA",
  "SUMMONABLE",
  "COLOR",
  "SPARE",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "DISP_AUTO",
  "CLS",
  "BUILDWALK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "FULL_AUTOEXIT",
  "AUTOSAC",
  "AUTOMEM",
  "VIEWORDER",
  "NO_COMPRESS",
  "AUTOASSIST",
  "DISP_KI",
  "DISP_EXP",
  "DISP_GOLD",
  "DISP_EXIT",
  "SPONSORED-CROWN",
  "SPONSORED-PALADIN",
  "AUTO-CONSIDER",
  "POSITIVE",
  "NEGATIVE",
  "HAGGLE",
  "AUTOMAP",
  "SPONTANEOUS",
  "ANONYMOUS",
  "AUTOFEINT",
  "MAXIMIZE_SPELL",
  "EXTEND_SPELL",
  "CRAFTING_BRIEF",
  "NOHINTS",
  "CONTAIN",
  "LFG",
  "CLANTALK",
  "ALLCTELL",
  "QUICKEN_SPELL",
  "AUTOATTACK",
  "PWR_SNEAK",
  "BLEED_ATK",
  "KNOCKDOWN",
  "ROBILARS",
  "TAKE_TEN",
  "SUMMON_TANK",
  "MOUNT_TANK",
  "DIVINE_BOND",
  "EMPOWER_SPELL",
  "INTENSIFY_SPELL",
  "COMPANION_TANK",
  "BRIEFMAP",
  "PARRY",
  "PVP",
  "FIGHT_SPAM_OFF",
  "\n"
};


/* AFF_x */
/* Many are taken from the SRD under OGL, see ../doc/srd.txt for information */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "FLIGHT",
  "HASTE",
  "SNEAK",
  "HIDE",
  "STANCE",
  "CHARM",
  "POLYMORPH",
  "TIRED",
  "WATERBREATH",
  "STONESKIN",
  "DETECT_DISGUISE",
  "KNOCKOUT",
  "MAGIC_VEST",
  "CONCEAL_ALIGN",
  "TAMED",
  "JAWSTRIKE",
  "DONTUSE",
  "UNDEAD",
  "PARALYZED",
  "",
  "FLYING",
  "ANGELIC",
  "ETHEREAL",
  "MAGICONLY",
  "NEXTPARTIAL",
  "NEXTNOACT",
  "STUNNED",
  "UNUSED_30",
  "CREEPING_DEATH",
  "SPIRIT",
  "SUMMONED",
  "CELESTIAL",
  "FIENDISH",
  "ILLITERATE",
  "WEARING_SUIT",
  "ANTI_EQUIPPED",
  "COURAGE",
  "TRIPPING",
  "SMITING",
  "FLURRY_OF_BLOWS",
  "FLAT_FOOTED1",
  "FLAT_FOOTED2",
  "ATTACK_OF_OPPORTUNITY",
  "SNEAK_ATTACK",
  "NO_DEITY",
  "RAGING",
  "FATIGUED",
  "+10%_EXP",
  "+25%_EXP",
  "+33%_EXP",
  "+50%_EXP",
  "STR_OF_HONOR",
  "NO_ALIGN",
  "SILENCED",
  "BLINDED",
  "LIGHT_LOAD",
  "MEDIUM_LOAD",
  "HEAVY_LOAD",
  "DOING_AOO",
  "RAPID_SHOT",
  "DEATH_WARD",
  "KEEN_WEAPON",
  "IMPACT_WEAPON",
  "DISGUISED",
  "WILD_SHAPED",
  "TRUE_SIGHT",
  "TANKING",
  "\n"
};


/* CON_x */
const char *connected_types[] = 
{
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Trigger edit",
  "Race Select",
  "Color Select",
  "Create SDesc",
  "Create LDesc",
  "Create DDesc",
  "Create Aliases",
  "Edit DDesc",
  "Select Alignment",
  "Select Deity",
  "Text Edit",
  "Config edit",
  "Assembly Edit",
  "Social Edit",
  "Race Help",
  "Class Help",
  "Guild edit",
  "Reroll stats",
  "Object Respec",
  "Level Up",
  "Assign Stats",
  "Help Edit",
  "Assign Alignment",
  "Alignment Help",
  "Help Edit",
  "Long Description",
  "Detailed Description",
  "Keywords",
  "Get Account Name",
  "Confirm Account Name",
  "Account Menu",
  "Account Characters",
  "Quit Comments",
  "Account Menu",
  "Gen Desc Desc 1",
  "Gen Desc Desc 2",
  "Gen Desc Adj. 1",
  "Gen Desc Adj. 2",
  "Gen Desc Menu 1",
  "Gen Desc Menu 2",
  "Human Race Select",
  "Elf Race Select",
  "Dwarf Race Select",
  "Confirm Race",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "Select Combat Action",
  "Custom Combat Action",
  "Parse Combat Action",
  "Note Write",
  "Mailing",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *wear_where[] = 
{
  "<Used as light>",
  "<Worn on right finger>",
  "<Worn on left finger>",
  "<First worn around neck>",
  "<Worn around neck>",
  "<Worn on body>",
  "<Worn on head>",
  "<Worn on legs>",
  "<Worn on feet>",
  "<Worn on hands>",
  "<Worn on arms>",
  "<Worn as shield>",
  "<Worn about body>",
  "<Worn around waist>",
  "<Worn around right wrist>",
  "<Worn around left wrist>",
  "<Wielded in main hand>",
  "<Used in off-hand>",
  "<Worn over face>",
  "<Worn in ear>",
  "<Worn in ear>",
  "<Worn on ankle>",
  "<Worn on ankle>",
  "<Hovering above head>",
  "<Worn on back>",
  "<Worn on shoulder>",
  "<Worn in nose>",
  "<Worn on belt>",
  "<Worn on back>",
  "<Worn on back>",
  "<Worn on back>",
  "<Worn on waist>",
  "<Sheathed on hand>",
  "<Sheathed on back>",
  "<Sheathed on back>",
  "<Sheathed on waist>",
  "<Sheathed on waist>",
  "<Sheathed on arm>",
  "<Sheathed on arm>",
  "<Sheathed on wrist>",
  "<Sheathed on wrist>",
  "<Mount's barding>",
  "<Used in off-hand>",
  "<Report if seen>",
  "<Report if seen>",
  "<Report if seen>",
  "<Mount's barding>",
  "\n"
};


/* WEAR_x - for stat */
const char *equipment_types[] = 
{
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around neck",
  "Worn around neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn as mask",
  "Worn in ear",
  "Worn in ear",
  "Worn on ankle",
  "Worn on ankle",
  "Hovering above head",
  "Worn on back",
  "Worn on shoulder",
  "Worn in nose",
  "Worn on belt",
  "Worn on back",
  "Worn on back",
  "Worn on back",
  "Worn on waist",
  "Sheathed on hand",
  "Sheathed on back",
  "Sheathed on back",
  "Sheathed on waist",
  "Sheathed on waist",
  "Sheathed on arm",
  "Sheathed on arm",
  "Sheathed on wrist",
  "Mount's Barding",
  "Used in off-hand",
  "Report if seen",
  "Report if seen",
  "Mount's Barding",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = 
{
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "SALVE",
  "BELT",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "BOTTLE",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "SHOVEL",
  "FOUNTAIN",
  "FIRE",
  "AQUALUNG",
  "SHEATH",
  "RAW",
  "PORTAL",
  "POLE",
  "FIREWOOD",
  "FIRE_WEAPON",
  "MISSILE",
  "TRAP",
  "BOAT",
  "VEHICLE",
  "HATCH",
  "WINDOW",
  "CONTROL",
  "SPELLBOOK",
  "BOARD",
  "MATERIAL",
  "ARMOR_SUIT",
  "HEALING_KIT",
  "HARVEST_NODE",
  "CRYSTAL",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = 
{
"TAKE",
"FINGER",
"NECK",
"BODY",
"HEAD",
"LEGS",
"FEET",
"HANDS",
"ARMS",
"SHIELD",
"ABOUT",
"WAIST",
"WRIST",
"WIELD",
"HOLD",
"FACE",
"EAR",
"ANKLE",
"ABOVE",
"BACK",
"SHOULDER",
"NOSE",
"SPARE_1",
"SHEATHED_B",
"ONBELT",
"ONBACK",
"SHEATHED_WA",
"SHEATHED_WR",
"SHEATHED_A",
"SHEATHED_H",
"BARDING",
"\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = 
{
  "GLOW",
  "HIDDEN",
  "NO_RENT",
  "NO_REMOVE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_ROGUE",
  "ANTI_FIGHTER",
  "NO_SELL",
  "ANTI_BARBAR",
  "ANTI_PALADIN",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_GNOME",
  "ANTI_HUMAN",
  "HUM",
  "UNIQUE_SAVE",
  "BROKEN",
  "UNBREAKABLE",
  "DOUBLE",
  "MOB_USABLE",
  "IDENTIFIED",
  "NO_LORE",
  "BLACKMARKET",
  "QUEST_ITEM",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = 
{
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "UNDEFINED",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "AC_DEFLECTION",
  "HITROLL",
  "DAMROLL",
  "UNUSED",
  "AC_NATURAL",
  "AC_ARMOR",
  "AC_SHIELD",
  "AC_DODGE",
  "QUESTPOINTS",
  "LIGHT",
  "MANA",
  "WEAPON_HITROLL",
  "WEAPON_DAMROLL",
  "RACE",
  "TURN_LEVEL",
  "SPELL_LEVEL_0",
  "SPELL_LEVEL_1",
  "SPELL_LEVEL_2",
  "SPELL_LEVEL_3",
  "SPELL_LEVEL_4",
  "SPELL_LEVEL_5",
  "SPELL_LEVEL_6",
  "SPELL_LEVEL_7",
  "SPELL_LEVEL_8",
  "SPELL_LEVEL_9",
  "MAX_KI",
  "FORTITUDE",
  "REFLEX",
  "WILL",
  "SKILL",
  "FEAT",
  "ALL_SAVES",
  "ABILITY",
  "ABILITY",
  "CARRY_WEIGHT",
  "\n"
};

char *apply_text[NUM_APPLIES+1] = 
{
  "Nothing",
  "Strength",
  "Dexterity",
  "Intelligence",
  "Wisdom",
  "Constitution",
  "Charisma",
  "Nothing",
  "Nothing",
  "Character-Age",
  "Character-Weight",
  "Character-Height",
  "Nothing",
  "Maximum-HP",
  "Maximum-Stamina",
  "Nothing",
  "Nothing",
  "Deflection-AC",
  "Attack-Bonus",
  "Damage",
  "Dodge-AC",
  "Natural-AC",
  "Armor-AC",
  "Shield-AC",
  "Nothing",
  "Nothing",
  "Nothing",
  "Nothing", // Mana
  "Nothing",
  "Nothing",
  "Nothing",
  "Turn-Undead-Level",
  "Level-0-Spell-Slots",
  "Level-1-Spell-Slots",
  "Level-2-Spell-Slots",
  "Level-3-Spell-Slots",
  "Level-4-Spell-Slots",
  "Level-5-Spell-Slots",
  "Level-6-Spell-Slots",
  "Level-7-Spell-Slots",
  "Level-8-Spell-Slots",
  "Level-9-Spell-Slots",
  "Nothing",
  "Fortitude-Save",
  "Reflex-Save",
  "Willpower-Save",
  "Nothing",
  "Nothing",
  "All-Saving-Throws",
  "Nothing",
  "\n"
};

int apply_gold_cost[NUM_APPLIES+1] = {
  0,    // none
  1000, // strength
  1000, // dexterity
  1000, // int
  1000, // wis
  1000, // con
  1000, // cha
  0,
  0,
  100, // age
  50,  // weight
  50,  // height
  0,
  5, // hp
  2, // stamina
  0,
  0,
  5, // deflect
  500, // attack
  500, // damage
  10, // dodge
  4, // natural
  4, // armor
  4, // shield
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  100, // turn undead
  200, // 0
  500, // 1
  1000, // 2
  2500, // 3
  4000, // 4
  7500, // 5
  10000, // 6
  15000, // 7
  20000, // 8
  35000, // 9
  40000, // 10
  0,
  750, // fort
  750, // ref
  750, // will
  0,
  0,
  2250, // all saves
  0,
  0,
};


/* CONT_x */
const char *container_bits[] = 
{
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/* MATERIAL_ */
const char *material_names[] = 
{
   "undefined",
   "cotton",
   "leather",
   "glass",
   "gold",
   "organic",
   "paper",
   "steel",
   "wood",
   "bone",
   "crystal",
   "ether",
   "adamantine",
   "mithril",
   "iron",
   "currency",
   "copper",
   "ceramic",
   "satin",
   "silk",
   "burlap",
   "velvet",
   "platinum",
   "obsidian",
   "wool",
   "onyx",
   "ivory",
   "brass",
   "marble",
   "bronze",
   "pewter",
   "ruby",
   "sapphire",
   "emerald",
   "gemstone",
   "granite",
   "stone",
   "energy",
   "hemp",
   "diamond",
   "earth",
   "silver",
   "alchemical silver",
   "cold iron",
   "darkwood",
   "dragonhide",
   "\n"
};


/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *domains[] = 
{
  "",
  "Air",
  "Animal",
  "Chaos",
  "Death",
  "Destruction",
  "Earth",
  "Evil",
  "Fire",
  "Good",
  "Healing",
  "Knowledge",
  "Law",
  "Luck",
  "Magic",
  "Plant",
  "Protection",
  "Strength",
  "Sun",
  "Travel",
  "Trickery",
  "War",
  "Water",
  "Artiface",
  "Charm",
  "Community",
  "Creation",
  "Darkness",
  "Glory",
  "Liberation",
  "Madness",
  "Nobility",
  "Repose",
  "Rune",
  "Scalykind",
  "Weather",
  "Meditation",
  "Forge",
  "Passion",
  "Insight",
  "Treachery",
  "Storm",
  "Pestilence",
  "Suffering",
  "Retribution",
  "Planning",
  "Craft",
  "Dwarf",
  "Time",
  "Family",
  "Moon",
  "Drow",
  "Elf",
  "Cavern",
  "Illusion",
  "Spell",
  "Hatred",
  "Tyranny",
  "Fate",
  "Renewal",
  "Metal",
  "Ocean",
  "Mobility",
  "Portal",
  "Trade",
  "Undeath",
  "Mentalism",
  "Gnome",
  "Halfling",
  "Orc",
  "Spider",
  "Slime",
  "Mediation",
  "\n"
};

/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *schools[] = 
{
  "Undefined",
  "Abjuration",
  "Conjuration",
  "Divination",
  "Enchantment",
  "Evocation",
  "Illusion",
  "Necromancy",
  "Transmutation",
  "Universal",
  "\n",
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* mob trigger types */
const char *trig_types[] = 
{
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};


/* obj trigger types */
const char *otrig_types[] = 
{
  "Global",
  "Random",
  "Command",
  "UNUSED",
  "UNUSED",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED",
  "Remove",
  "UNUSED",
  "Load",
  "UNUSED",
  "Cast",
  "Leave",
  "UNUSED",
  "Consume",
  "Time",
  "\n"
};


/* wld trigger types */
const char *wtrig_types[] = 
{
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};


/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *size_names[] = 
{
  "fine",
  "diminutive",
  "tiny",
  "small",
  "medium",
  "large",
  "huge",
  "gargantuan",
  "colossal",
  "\n"
};


int rev_dir[] =
{
  /* North */ SOUTH,
  /* East  */ WEST,
  /* South */ NORTH,
  /* West  */ EAST,
  /* Up    */ DOWN,
  /* Down  */ UP,
  /* NW    */ SOUTHEAST,
  /* NE    */ SOUTHWEST,
  /* SE    */ NORTHWEST,
  /* SW    */ NORTHEAST,
  /* In    */ OUTDIR,
  /* Out   */ INDIR
};

int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  3,	/* Field      */
  4,	/* Forest     */
  5,	/* Hills      */
  6,	/* Mountains  */
  5,	/* Swimming   */
  2,	/* Unswimable */
  6,     /* Underwater */
  2,	/* Flying     */
  2     // road
};

/* Not used in sprinttype(). */
const char *weekdays[] = 
{
  "the Day of the Moon",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = 
{
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Old Forces",
  "Month of the Spring",
  "Month of Nature",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *wield_names[] = 
{
  "if you were bigger",
  "with ease",
  "one-handed",
  "two-handed",
  "\n"
};


const char *admin_level_names[ADMLVL_IMPL + 4] = 
{
  "Mortal",
  "Junior Staff",
  "Regular Staff",
  "Senior Staff",
  "Administrator",
  "Co-Owner",
  "\n",
  NULL
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
struct aging_data racial_aging_data[NUM_RACES] = {

/*                   adult	start1	start2	start3	middle	old	vener.	maxdice	*/
/* HUMAN        */ { 15,	{{1,4}, {1,6}, {2,6}},	35,	53,	70,	{2,20}	},
/* ELF          */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* GNOME        */ { 40,	{{4,6}, {6,6}, {9,6}},	100,	150,	200,	{3,100}	},
/* DWARF        */ { 40,	{{3,6}, {5,6}, {7,6}},	125,	188,	250,	{2,100}	},
/* HALFLING     */ { 20,	{{2,4}, {3,6}, {4,6}},	50,	75,	100,	{5,20}	}
};

int race_ages[NUM_RACE_TYPES+1] =
{
0,   // Undefined
18,  // Human
110, // Elf
40,   // dwarf
20,  // kender/halfing
40,  // gnome
3,   // animal
100, // undead
18,  // monstrous humanoid
18,  // giant
3,   // plant
1,   // ooze
100, // elemental
100, // outsider
18,  // magical beats
18,  // minotaur
1000,// dragon
10,  // construct
35,  // half elf
18,  // orc
200, // draconian
18,  // goblinoid
1,   // vermin
110 // fey
};


/* Administrative flags */
const char *admin_flag_names[] = 
{
  "TellAll",
  "SeeInventory",
  "SeeSecret",
  "KnowWeather",
  "FullWhere",
  "Money",
  "EatAnything",
  "NoPoison",
  "WalkAnywhere",
  "NoKeys",
  "InstantKill",
  "NoSteal",
  "TransAll",
  "SwitchMortal",
  "ForceMass",
  "AllHouses",
  "NoDamage",
  "AllShops",
  "CEDIT",
  "\n"
};


const char *spell_schools[] = 
{
  "abjuration",
  "conjuration",
  "divination",
  "enchantment",
  "evocation",
  "illusion",
  "necromancy",
  "transmutation",
  "universal",
  "\n"
};


const char *cchoice_names[NUM_COLOR + 1] = 
{
  "normal",
  "room names",
  "room objects",
  "room people",
  "someone hits you",
  "you hit someone",
  "other hit",
  "critical hit",
  "holler",
  "shout",
  "gossip channel",
  "auction channel",
  "congratulations",
  "\n"
};


const char *dr_style_names[NUM_DR_STYLES + 1] = 
{
  "NONE",
  "admin",
  "weapon material",
  "weapon bonus",
  "spell",
  "magic",
  "\n"
};


/* --- End of constants arrays. --- */

/*
 * Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared.
 */
size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

const char *creation_methods[] =
{
  "[Standard - Random rolls, ordered assignment]",
  "[Player Assigned - Random rolls, player adjust to taste]",
  "[Points Pool - Player assigns scores from points pool]",
  "[Racial Template - All races start from template values, adjusted by class]",
  "[Class Template - All classes start from template values, adjusted by race]",
  "\n"
};


const char *armor_types[] =
{

  "Undefined",
  "Padded",
  "Leather",
  "Studded Leather",
  "Light Chainmail",
  "Hide",
  "Scale",
  "Chainmail",
  "Piecemeal",
  "Splint",
  "Banded",
  "Half-Plate",
  "Full-Plate",
  "Buckler",
  "Small Shield",
  "Large Shield",
  "Tower Shield",
  "\r\n"
};


const char *death_message = {
"                       _,.-------.,_\r\n"
"                   ,;~\'             \'~;, \r\n"
"                 ,;                     ;,\r\n"
"                ;                         ;\r\n"
"               ,\'                         \',\r\n"
"              ,;                           ;,\r\n"
"              ; ;      .           .      ; ;\r\n"
"              | ;   ______       ______   ; | \r\n"
"              |  \'/~\"     ~\" . \"~     \"~\\\'  |\r\n"
"              |  ~  ,-~~~^~, | ,~^~~~-,  ~  |\r\n"
"               |   |        }:{        |   | \r\n"
"               |   l       / | \\       !   |\r\n"
"               .~  (__,.--\" .^. \"--.,__)  ~. \r\n"
"               |     ---;\' / | \\ \';---     |  \r\n"
"                \\__.       \\/^\\/       .__/  \r\n"
"                 V| \\                 / |V  \r\n"
"                  | |T~\\___!___!___/~T| |  \r\n"
"                  | |`IIII_I_I_I_IIII\'| |  \r\n"
"                  |  \\,III I I I III,/  |  \r\n"
"                   \\   `~~~~~~~~~~\'    /\r\n"
"                     \\   .       .   / \r\n"
"                       \\.    ^    ./   \r\n"
"                         ^~~~^~~~^   \r\n"
"\r\n"
"                Y O U   H A V E   D I E D!\r\n"
"\r\n"
"    @RT Y P E   R E S U R R E C T   T O  R E S P A W N@n   O R\r\n"
"\r\n"
"   W A I T   F O R   S O M E O N E   T O   R A I S E   Y O U.\r\n"
"\r\n"
};

const char *deity_names_fr[] = {
	"None",
	"\n"
};

const char *deity_names_dl_aol[] = {
	"None",
	"Paladine",
	"Kiri-Jolith",
	"Mishakal",
	"Habbakuk",
	"Majere",
	"Branchala",
	"Solinari",
	"Gilean",
	"Reorx",
	"Shinare",
	"Zivilyn",
	"Chislev",
	"Sirrion",
	"Lunitari",
	"Takhisis",
	"Sargonass",
	"Hiddukel",
	"Chemosh",
	"Morgion",
	"Zeboim",
	"Nuitari",
	"\n"
};

const char *domain_names[] =
{
  "",
  "Air",
  "Animal",
  "Chaos",
  "Death",
  "Destruction",
  "Earth",
  "Evil",
  "Fire",
  "Good",
  "Healing",
  "Knowledge",
  "Law",
  "Luck",
  "Magic",
  "Plant",
  "Protection",
  "Strength",
  "Sun",
  "Travel",
  "Trickery",
  "Universal",
  "War",
  "Water",
  "Artifice",
  "Charm",
  "Community",
  "Creation",
  "Darkness",
  "Glory",
  "Liberation",
  "Madness",
  "Nobility",
  "Repose",
  "Rune",
  "Scalykind",
  "Weather",
  "Meditation",
  "Forge",
  "Passion",
  "Insight",
  "Treachery",
  "Storm",
  "Pestilence",
  "Suffering",
  "Retribution",
  "Planning",
  "Craft",
  "Dwarf",
  "Time",
  "Family",
  "Moon",
  "Drow",
  "Elf",
  "Cavern",
  "Illusion",
  "Spell",
  "Hatred",
  "Tyranny",
  "Fate",
  "Renewal",
  "Metal",
  "Ocean",
  "Mobility",
  "Portal",
  "Trade",
  "Undeath",
  "Mentalism",
  "Gnome",
  "Halfling",
  "Orc",
  "Spider",
  "Slime",
  "Mediation",
  "\n"
};

char * level_ranges[] = 
{

  "Any",
  "1-2",
  "3-4",
  "5-6",
  "7-8",
  "9-10",
  "11-12",
  "13-14",
  "15-16",
  "17-18",
  "19-20",
  "21-22",
  "23-24",
  "25-26",
  "27-28",
  "29-30",
  "31-32",
  "33-34",
  "35-36",
  "37-38",
  "39-40",
  "41-42",
  "43-44",
  "45-46",
  "47-48",
  "49-50",
  "51-52",
  "53-54",
  "55-56",
  "57-58",
  "59-60",
  "\n"
};

char * zone_states[] = 
{
  "Closed",
  "Working",
  "Open",
  "Dungeon",
  "\n"
};

char *handle_types[] = 
{
  "handle",
  "shaft",
  "hilt",
  "strap",
  "string",
  "grip",
  "handle",
  "glove"
};

char *head_types[] = 
{
  "headed",
  "bladed",
  "headed",
  "pointed",
  "bowed",
  "strapped",
  "lengthed",
  "meshed",
  "chained",
  "fisted"
};

int no_affect_eq[NUM_NO_AFFECT_EQ] =
{
ITEM_WEAR_LEGS,
ITEM_WEAR_ARMS,
ITEM_WEAR_FACE,
ITEM_WEAR_EAR ,
ITEM_WEAR_ANKLE,
ITEM_WEAR_BACK ,
ITEM_WEAR_SHOULDER,
ITEM_WEAR_NOSE ,
ITEM_WEAR_SPARE_1 ,
ITEM_WEAR_SHEATHED_B ,
ITEM_WEAR_ONBELT,
ITEM_WEAR_ONBACK ,
ITEM_WEAR_SHEATHED_WA,
ITEM_WEAR_SHEATHED_WR,
ITEM_WEAR_SHEATHED_A,
ITEM_WEAR_SHEATHED_H
};

/* Constants for Assemblies    *****************************************/
const char *AssemblyTypes[MAX_ASSM+1] = 
{
  "tan",
  "bake",
  "brew",
  "assemble",
  "craft",
  "fletch",
  "knit",
  "mix",
  "thatch",
  "weave",
  "forge",
  "mine",
  "disenchant",
  "synthesize",
  "hunt",
  "farm",
  "forest",
  "\n"
};

const char *craft_names[NUM_CRAFTS] = 
{
  "bracer",
  "bracelet",
  "armband",
  "medallion",
  "necklace",
  "pendant",
  "chain",
  "gorget",
  "choker",
  "ring",
  "band",
  "ioun stone",
  "gloves",
  "gauntlets",
  "boots",
  "sollerets",
  "\n"
};

int craft_materials[NUM_CRAFTS] = {
  MATERIAL_STEEL,
  MATERIAL_GOLD,
  MATERIAL_LEATHER,
  MATERIAL_GOLD,
  MATERIAL_GOLD,
  MATERIAL_GOLD,
  MATERIAL_GOLD,
  MATERIAL_LEATHER,
  MATERIAL_GOLD,
  MATERIAL_GOLD,
  MATERIAL_GOLD,
  MATERIAL_DIAMOND,
  MATERIAL_LEATHER,
  MATERIAL_LEATHER,
  MATERIAL_LEATHER,
  MATERIAL_LEATHER,
  0
};

int craft_wears[NUM_CRAFTS] = {

  ITEM_WEAR_WRIST,
  ITEM_WEAR_WRIST,
  ITEM_WEAR_WRIST,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_FINGER,
  ITEM_WEAR_FINGER,
  ITEM_WEAR_ABOVE,
  ITEM_WEAR_HANDS,
  ITEM_WEAR_HANDS,
  ITEM_WEAR_FEET,
  ITEM_WEAR_FEET,
  0
};

int craft_skills[NUM_CRAFTS] = {
  SKILL_TANNING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_GOLDSMITHING,
  SKILL_TANNING,
  SKILL_TANNING,
  SKILL_TANNING,
  SKILL_TANNING,
  0
};

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

const char *companion_types[] = 
{
  "none",
  "bear",
  "wolf",
  "panther",
  "lion",
  "tiger",
  "hawk",
  "boar",
  "eagle",
  "owl",
  "dog",
  "python",
  "stag",
  "coyote",
  "badger",
  "\n"
};

int craft_pattern_vnums_real[NUM_CRAFT_TYPES] = {
200,
201,
202,
203,
204,
205,
206,
207,
208,
209,
210,
211,
212,
213,
216,
217,
218,
219,
220,
221,
222,
223,
224,
225,
226,
227,
228,
229,
230,
231,
232,
233,
234,
235,
236,
237,
238,
239,
240,
241,
242,
243,
244,
245,
246,
247,
248,
249,
250,
251,
252,
253,
254,
255,
256,
257,
258,
259,
260,
261,
262,
263,
264,
265,
266,
267,
268,
269,
270,
271,
272,
273,
274,
275,
276,
277,
278,
279,
280,
281,
282,
283,
295,
285,
286,
287,
288,
289,
290,
291,
292,
293,
300,
301,
302,
-1
};

const char *craft_pattern_descs[] = 
{
"padded armor",
"leather armor",
"studded leather armor",
"light chain armor",
"hide armor",
"scale mail armor",
"chainmail armor",
"piecemeal armor",
"splint mail armor",
"banded mail armor",
"half plate armor",
"plate mail armor",
"small shield",
"buckler",
"heavy shield",
"tower shield",
"dagger",
"light mace",
"sickle",
"club",
"half spear",
"heavy mace",
"morningstar",
"quarterstaff",
"short spear",
"light crossbow",
"sling",
"heavy crossbow",
"light hammer",
"hand axe",
"light lance",
"light pick",
"short sword",
"battle axe",
"light flail",
"heavy lance",
"long sword",
"heavy pick",
"rapier",
"scimitar",
"warhammer",
"falchion",
"heavy flail",
"glaive",
"great club",
"maul",
"great sword",
"guisarme",
"halberd",
"long spear",
"scythe",
"short bow",
"long bow",
"bastard sword",
"dart",
"javelin",
"throwing axe",
"kukri",
"blackjack",
"trident",
"ranseur",
"great axe",
"kama",
"double bladed battle axe",
"sai",
"saingham",
"dwarven war axe",
"whip",
"spiked chain",
"dire flail",
"hooked hammer",
"two-bladed sword",
"dwarven urgosh",
"hand crossbow",
"heavy repeating crossbow",
"light repeating crossbow",
"bola",
"weighted net",
"shuriken",
"nunchaku",
"composite long bow",
"composite short bow",
"clothing",
"ring",
"medallion",
"bracer",
"cloak",
"belt",
"gloves",
"boots",
"helmet",
"knuckles",
"fullblade",
"khopesh",
"curve blade",
"\n"
,NULL
};

int essence_vnums[NUM_ESSENCE_TYPES] = {
64100,
64101,
64102,
64103,
64104,
64105,
64106,
64107,
64108,
64109,
64110,
64111,
64112,
64113,
64114,
64115,
64116,
64117,
64118,
64119,
64120,
64121,
64122,
64123,
64124,
64125,
64126,
64127,
64128,
64129,
-1
};

char *curse_words[] =
{
  "fuck",
  " shit",
  "shit ",
  " ass ",
  "wtf",
  "roflmao",
  " ffs",
  "omg",
  "oh my god",
  " cunt ",
  " piss",
  "bitch",
  "beyotch",
  "asshole",
  "stfu"
  "badass",
  NULL
};

char *race_types[] =
{
"none",
"human",
"elf",
"dwarf",
"halfling",
"gnome",
"animal",
"undead",
"monstrous-humanoid",
"giant",
"plant",
"ooze",
"elemental",
"outsider",
"magical-beast",
"minotaur",
"dragon",
"construct",
"half-elf",
"orc",
"draconian",
"goblinoid",
"vermin",
"fey",
"\n"
,NULL
};


char *weapon_damage_types[] =
{
  "slashing",
  "bludgeoning",
  "piercing",
  "\n",
  NULL
};


