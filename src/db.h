/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_TRG	6
#define DB_BOOT_GLD	7
#define DB_BOOT_QST     8

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_ETC		":etc:"
#define LIB_PLRTEXT	":plrtext:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_PLRVARS	":plrvars:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_PLRFILES	":plrfiles:"
#define LIB_ACTFILES    ":actfiles:"
#define LIB_HOUSE	":house:"
#define LIB_AUCTION	":auction:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_PLRVARS	"plrvars/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_PLRFILES	"plrfiles/"
#define LIB_ACTFILES    "actfiles/"
#define LIB_HOUSE	"house/"
#define LIB_AUCTION	"auction/"
#define SLASH		"/"
#else
#error "Unknown path components."
#endif

#define SUF_OBJS	"new"
#define SUF_OLD_OBJS	"old"
#define SUF_TEXT	"text"
#define SUF_ALIAS	"alias"
#define SUF_MEM	        "mem"
#define SUF_PLR	        "plr"
#define SUF_PET		"pet"
#define SUF_PET_NEW "pet.new"
#define SUF_ACT         "act"
#define SUF_BAK         "bak"
#define SUF_HOUSE       "house"

#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"    /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "/.killscript"  /* autorun: shut mud down       */
#define PAUSE_FILE      "/pause"        /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	/* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE	"::.killscript"	/* autorun: shut mud down	*/
#define PAUSE_FILE	"::pause"	/* autorun: don't restart mud	*/
#else
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#endif

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	LIB_WORLD"wld"SLASH	/* room definitions	*/
#define MOB_PREFIX	LIB_WORLD"mob"SLASH	/* monster prototypes	*/
#define OBJ_PREFIX	LIB_WORLD"obj"SLASH	/* object prototypes	*/
#define ZON_PREFIX	LIB_WORLD"zon"SLASH	/* zon defs & command tables */
#define SHP_PREFIX	LIB_WORLD"shp"SLASH	/* shop definitions	*/
#define HLP_PREFIX	LIB_TEXT"help"SLASH	/* for HELP <keyword>	*/
#define TRG_PREFIX	LIB_WORLD"trg"SLASH	/* trigger files	*/
#define GLD_PREFIX	LIB_WORLD"gld"SLASH	/* guild files		*/
#define QST_PREFIX      LIB_WORLD"qst"SLASH     /* quest files          */

#define HELP_FILE       "help.hlp"
#define CREDITS_FILE	LIB_TEXT"credits" /* for the 'credits' command	*/
#define MOTD_FILE	LIB_TEXT"motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	LIB_TEXT"imotd"	/* messages of the day / immort	*/
#define GREETINGS_FILE	LIB_TEXT"greetings"	/* The opening screen.	*/
#define GREETANSI_FILE	LIB_TEXT"greetansi"	/* The opening screen.	*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP"screen"	/* for HELP <CR>	*/
#define CONTEXT_HELP_FILE LIB_TEXT"contexthelp"	/* The opening screen.	*/
#define INFO_FILE	LIB_TEXT"info"		/* for INFO		*/
#define WIZLIST_FILE	LIB_TEXT"wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE	LIB_TEXT"immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE	LIB_TEXT"background"/* for the background story	*/
#define POLICIES_FILE	LIB_TEXT"policies"  /* player policies/rules	*/
#define HANDBOOK_FILE	LIB_TEXT"handbook"  /* handbook for new immorts	*/

#define IDEA_FILE	LIB_MISC"ideas"	   /* for the 'idea'-command	*/
#define TYPO_FILE	LIB_MISC"typos"	   /*         'typo'		*/
#define BUG_FILE	LIB_MISC"bugs"	   /*         'bug'		*/
#define NEWS_FILE	LIB_MISC"news"	   /* for the 'news' command	*/
#define MESS_FILE	LIB_MISC"messages" /* damage messages		*/
#define SOCMESS_FILE	LIB_MISC"socials"  /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC"socials.new"  /* messages for social acts with aedit patch*/
#define XNAME_FILE	LIB_MISC"xnames"   /* invalid name substrings	*/

#define CLAN_FILE       LIB_ETC"clans"     /* the clan file */
#define CONFIG_FILE     LIB_ETC"config"      /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE	LIB_ETC"players"   /* the player database	*/
#define MAIL_FILE	LIB_ETC"mail"   /* for the mudmail system	*/
#define BAN_FILE	LIB_ETC"badsites"  /* for the siteban system	*/
#define HCONTROL_FILE	LIB_ETC"hcontrol"  /* for the house system	*/
#define TIME_FILE	LIB_ETC"time"	   /* for calendar system	*/
#define ASSEMBLIES_FILE LIB_ETC"assemblies" /* Assemblies engine 	*/
#define LEVEL_CONFIG	LIB_ETC"levels"	   /* set various level values  */
#define AUCTION_FILE    LIB_ETC"auction"   /* for the auction house */

#define PREF_FILE	LIB_ETC"pref"	  /* next pref number		*/
#define LAST_FILE	LIB_ETC"last"

/* new bitvector data for use in player_index_element */
#define PINDEX_DELETED		(1 << 0)	/* deleted player	*/
#define PINDEX_NODELETE		(1 << 1)	/* protected player	*/
#define PINDEX_SELFDELETE	(1 << 2)	/* player is selfdeleting*/
#define PINDEX_NOWIZLIST	(1 << 3)	/* Player shouldn't be on wizlist*/


/* public procedures in db.c */
void	boot_db(void);
void	destroy_db(void);
int	create_entry(char *name);
void	zone_update(void);
char	*fread_string(FILE *fl, const char *error);
long	get_id_by_name(const char *name);
char	*get_name_by_id(long id);
void	save_mud_time(struct time_info_data *when);
void	free_extra_descriptions(struct extra_descr_data *edesc);
void	free_text_files(void);
void	free_player_index(void);
void	free_help_table(void);
void load_disabled(void);
void save_disabled(void);
void free_disabled(void);

zone_rnum real_zone(zone_vnum vnum);
room_rnum real_room(room_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);

int	load_char(const char *name, struct char_data *ch);
void	load_char_pets(struct char_data *ch);
void	save_char(struct char_data *ch);
void	save_char_pets(struct char_data *ch);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);
void	save_player_index(void);
long  get_ptable_by_name(const char *name);
void    current_update(void);
void	read_level_data(struct char_data *ch, FILE *fl);
void	write_level_data(struct char_data *ch, FILE *fl);

int parse_mobile_from_file(FILE *mob_f, struct char_data *ch);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
struct obj_data *read_object(obj_vnum nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate);
void add_unique_id(struct obj_data *obj);
void check_unique_id(struct obj_data *obj);
char *sprintuniques(int low, int high);

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int  arg4;		/* room_max  default 0			*/
   int  arg5;           /* percentages variable                 */
   int line;		/* line number this command appears on  */
   char *sarg1;		/* string argument                      */
   char *sarg2;		/* string argument                      */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *
        *  'V': Assign a variable *
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   char *builders;          /* namelist of builders allowed to    */
                            /* modify this zone.		  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   room_vnum bot;           /* starting room number for this zone */
   room_vnum top;           /* upper limit for rooms in this zone */

   int	reset_mode;         /* conditions for reset (see below)   */
   zone_vnum number;	    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */
   int timeline;
   int plane;
   int planet;
   int continent;
   int country;
   int region;
   int city;
   int population;
   int races;
   int government;
   int trade_materials;
   int trade_services;
   int trade_items;
   int languages;
   int alignments;
   int zone_status;
   int level_range;
   int spare; 

   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};


/* Added level, flags, and last, primarily for pfile autocleaning.  You
   can also use them to keep online statistics, and can add race, class,
   etc if you like.
*/
struct player_index_element {
   char	*name;
   long id;
   int level;
   int admlevel;
   int flags;
   time_t last;
};


struct help_index_element {
   char *keywords;
   int min_level;
   char *entry;
};


/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};


/* global buffering system */

#ifndef __DB_C__
extern struct config_data config_info;

extern struct room_data *world;
extern room_rnum top_of_world;

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct char_data *affect_list;
extern struct char_data *affectv_list;
extern struct player_special_data dummy_mob;
extern struct pettable_data pet_table[];

extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern mob_rnum top_of_mobt;

extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern obj_rnum top_of_objt;

extern struct htree_node *room_htree;
extern struct htree_node *mob_htree;
extern struct htree_node *obj_htree;

extern struct social_messg *soc_mess_list;
extern int top_of_socialt;

extern struct index_data **trig_index;
extern struct trig_data *trigger_list;
extern int top_of_trigt;
extern long max_mob_id;
extern long max_obj_id;
extern int dg_owner_purged;

#endif /* __DB_C__ */

void strip_string(char *buffer);
int read_xap_objects(FILE *fl,struct char_data *ch);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE    "disabled.cmds"  /* disabled commands */
#define END_MARKER    "END" /* for load_disabled() and save_disabled() */

typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first; /* interpreter.c */

/* one disabled command */
struct disabled_data {
       DISABLED_DATA *next;                /* pointer to next node          */
       struct command_info const *command; /* pointer to the command struct */
       char *disabled_by;                  /* name of disabler              */
       sh_int level;                       /* level of disabler             */
       int subcmd;                         /* the subcmd, if any            */
};

int get_new_pref();
