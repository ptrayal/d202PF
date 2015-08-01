/*
* 
************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "mysql/mysql.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "feats.h"
#include "pets.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "shop.h"
#include "guild.h"
#include "sqlcommon.h"
#include "char_descs.h"
#include "quest.h"
#include "pfdefaults.h"
#include "deities.h"
#include "polls.h"


/* local global variables */
DISABLED_DATA *disabled_first = NULL;

/* external variables */
extern MYSQL *conn;
extern struct clan_type *clan_info;
extern struct race_data race_list[NUM_RACES];
extern char *weapon_damage_types[];
extern room_rnum r_mortal_start_room;
extern struct poll_data poll_list[NUM_POLLS];
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern struct deity_info deity_list[NUM_DEITIES];
extern char *motd;
extern char *imotd;
extern char *background;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int selfdelete_fastwipe;
extern int xap_objs;
extern char *GREETANSI;
extern char *GREETINGS;
extern char *ANSIQUESTION;
extern char *pc_race_types[NUM_RACES];
extern char *race_names[NUM_RACES];
extern const char compress_offer[];
extern struct pet_data pet_list[NUM_PETS];
extern const char *alignments[];
extern int spell_sort_info[SKILL_TABLE_SIZE + 1];
extern int feat_sort_info[MAX_FEATS + 1];
extern int sorcerer_spells_known[22][10];

int pref_temp=0;

/* external functions */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int is_crit, int material, int bonus, int spell, int magic);
int get_saving_throw_value(struct 
char_data *victim, int savetype);
int get_speed(struct char_data *ch);
void display_combat_menu(struct descriptor_data *d);
void fight_action(struct char_data *ch, struct char_data *start);
int level_exp(int level, int race);
void combine_accounts(void);
struct help_index_element *find_help(char *keyword);
char *current_short_desc(struct char_data *ch);
void short_desc_descriptors_menu(struct char_data *ch);
void short_desc_adjectives_menu(struct char_data *ch, int which_type);
int count_adjective_types(int which_desc);
void do_start(struct char_data *ch);
int parse_class(struct char_data *ch, char arg);
int parse_race(struct char_data *ch, char *arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
void read_saved_vars(struct char_data *ch);
void remove_player(int pfilepos);
void assemblies_parse(struct descriptor_data *d, char *arg);
void free_mount(struct char_data *ch);
extern void assedit_parse(struct descriptor_data *d, char *arg);
int class_ok_race[NUM_RACES][NUM_CLASSES];
int class_ok_general(struct char_data *ch, int whichclass);
int race_ok_gender[NUM_SEX][NUM_RACES];
void gedit_disp_menu(struct descriptor_data *d);
void gedit_parse(struct descriptor_data *d, char *arg);
void cedit_creation(struct char_data *ch);
void display_alignments(struct descriptor_data *d);
void display_alignment_help(struct descriptor_data *d);
size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices);
void parse_alignment(struct char_data *ch, char choice);
void convert_coins(struct char_data *ch);
int num_levelup_class_feats(struct char_data *ch, int whichclass, int ranks);
void do_advance_level(struct char_data *ch, int whichclass, int manual);
int num_levelup_practices(struct char_data *ch, int whichclass);
void do_handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument, int manual);
void display_levelup_skills(struct char_data *ch, int langs);
void reset_artisan_experience(struct char_data *vict);
void display_levelup_feats(struct char_data *ch);
int do_class_ok_general(struct char_data *ch, int whichclass, int show_text);
void display_levelup_weapons(struct char_data *ch);
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg);
int handle_levelup_feat_points(struct char_data *ch, int feat_num, int return_val);
int has_combat_feat(struct char_data *ch, int i, int j);
void display_levelup_trains(struct char_data *ch);
char *get_blank_clan_name(int clan);
char *get_rank_name(int clan, int rank);
void note_display_unread(struct char_data *ch);


/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int reserved_word(char *argument);
int _parse_name(char *arg, char *name);
int check_disabled(const struct command_info *command);
void disp_sdesc_menu(struct descriptor_data *d);
int parse_sdesc(char *arg, struct char_data *ch);
void display_levelup_classes(struct descriptor_data *d);
void init_levelup(struct char_data *ch);
void display_levelup_summary(struct char_data *ch);
void display_levelup_changes(struct char_data *ch, int apply_changes);
void process_add_feat(struct char_data *ch, int feat_num);
// nothing

/* prototypes for all do_x functions. */
ACMD(do_wish);
ACMD(do_whirlwind);
ACMD(do_swarmofarrows);
ACMD(do_divine_bond);
ACMD(do_gatherinfo);
ACMD(do_poll);
ACMD(do_pilfer);
ACMD(do_gain);
ACMD(do_practice_skills);
ACMD(do_petition);
ACMD(do_classabilities);
ACMD(do_set_stats);
ACMD(do_challenge);
ACMD(do_post_forums);
ACMD(do_combatcmd);
ACMD(do_coupdegrace);
ACMD(do_attack);
ACMD(do_approach);
ACMD(do_rebind);
ACMD(do_retreat);
ACMD(do_resetartisan);
ACMD(do_respec);
ACMD(do_endturn);
ACMD(do_levelup);
ACMD(do_account);
ACMD(do_wishlist);
ACMD(do_rpsheet);
ACMD(do_wildshape);
ACMD(do_undeathtouch);
ACMD(do_bounty);
ACMD(do_disguise);
ACMD(do_favoredenemy);
ACMD(do_breath_weapon);
ACMD(do_clan);
ACMD(do_show_clan);
ACMD(do_chooseguild);
ACMD(do_guildexp);
ACMD(do_guildbonuses);
ACMD(do_guildscore);
ACMD(do_listguilds);
ACMD(do_showguild);
ACMD(do_guard);
ACMD(do_guildchat);
ACMD(do_subguildchat);
ACMD(do_guildwho);
ACMD(do_accexp);
ACMD(do_subguildwho);
ACMD(do_setheight);
ACMD(do_setweight);
ACMD(do_setfalseethos);
ACMD(do_setfalsealign);
ACMD(do_playerguilds);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_affect);
ACMD(do_output);
ACMD(do_affstats);
ACMD(do_aedit);
ACMD(do_alias);
ACMD(do_approve);
ACMD(do_dragonbreath);
ACMD(do_artisan);
ACMD(do_websiteaccount);
ACMD(do_assemble);
ACMD(do_assedit);
ACMD(do_assist);
ACMD(do_astat);
ACMD(do_at);
ACMD(do_auction);
ACMD(do_autoexit);
ACMD(do_autofeint);
ACMD(do_autocon);
ACMD(do_automap);
ACMD(do_awardexp);
ACMD(do_backup);
ACMD(do_ban);
ACMD(do_bid);
ACMD(do_binditem);
ACMD(do_bonuslevels);
ACMD(do_break);
ACMD(do_buck);
ACMD(do_call);
ACMD(do_callset);
ACMD(do_setcamp);
ACMD(do_cast);
ACMD(do_chown);
ACMD(do_new_mail);
ACMD(do_color);
ACMD(do_compare);
ACMD(do_copyover);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_contained_areas);
ACMD(do_credits);
ACMD(do_craftingbrief);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_deathattack);
ACMD(do_defensive_stance);
ACMD(do_devote);
ACMD(do_diagnose);
ACMD(do_disable);
ACMD(do_disenchant);
ACMD(do_dismiss_mob);
ACMD(do_divide);
ACMD(do_divine_feats);
ACMD(do_synthesize);
ACMD(do_dig);
ACMD(do_disarm);
ACMD(do_display);
ACMD(do_dismount);
ACMD(do_hpay);
ACMD(do_domain);
ACMD(do_drink);
ACMD(do_drive);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_edit);		/* Mainly intended as a test function. */
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_eqstats);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_expertise);
ACMD(do_feats);
ACMD(do_featset);
ACMD(do_feint);
ACMD(do_file);
ACMD(do_fix);
ACMD(do_flee);
ACMD(do_callmount);
ACMD(do_callcompanion);
ACMD(do_rerollheightandweight);
ACMD(do_flurry);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_form);
ACMD(do_shuttle);
ACMD(do_speeder);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_gift);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_greet);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_haggle);
ACMD(do_harvest);
ACMD(do_harvest_new);
ACMD(do_heal);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hedit);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_intro);
ACMD(do_iedit);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_jog);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_languages);
ACMD(do_lay_hands);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_lfg);
ACMD(do_load);
ACMD(do_loadmagic);
ACMD(do_loadcrystal);
ACMD(do_look);
ACMD(do_lore);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_nohelps);
ACMD(do_nohints);
ACMD(do_nosuicide);
ACMD(do_not_here);
ACMD(do_mail);
ACMD(do_map);
ACMD(do_mark);
ACMD(do_mentor);
ACMD(do_memorize);
ACMD(do_metamagic);
ACMD(do_mount);
ACMD(do_mreport);
ACMD(do_mrest);
ACMD(do_mstand);
ACMD(do_note);
ACMD(do_oasis);
ACMD(do_ofind);
ACMD(do_olc);
ACMD(do_old_score);
ACMD(do_osay);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pagelength);
ACMD(do_parry);
ACMD(do_peace);
ACMD(do_petset);
ACMD(do_players);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quest);
ACMD(do_quit);
ACMD(do_rage);
ACMD(do_raise);
ACMD(do_random);
ACMD(do_rapidshot);
ACMD(do_reboot);
ACMD(do_recall);
ACMD(do_reimburse);
ACMD(do_remove);
ACMD(do_rescue);
ACMD(do_reply);
ACMD(do_reform);
ACMD(do_release);
ACMD(do_report);
ACMD(do_respond);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_resurrect);
ACMD(do_return);
ACMD(do_review);
ACMD(do_room_copy);
ACMD(do_run);
ACMD(do_sac);
ACMD(do_save);
ACMD(do_saveall);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_aod_new_score);
ACMD(do_screenwidth);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_setactive);
ACMD(do_setaffinity);
ACMD(do_set);
ACMD(do_show);
ACMD(do_show_combat);
ACMD(do_show_sorted_lists);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillcheck);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_smite);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spells);
ACMD(do_spellup);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_spontaneous);
ACMD(do_sponsor);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_strength_of_honor);
ACMD(do_summon);
ACMD(do_companion);
ACMD(do_suicide);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tame);
ACMD(do_taunt);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_test);
ACMD(do_time);
ACMD(do_timers);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_trip);
ACMD(do_turn);
ACMD(do_turn_undead);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);
ACMD(do_wield);
ACMD(do_value);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);

/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mzoneecho);
ACMD(do_vdelete);
ACMD(do_mfollow);
ACMD(do_dig);

struct command_info *complete_cmd_info;

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (for example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, ADMLVL_NONE	, 0 },     /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NORTH },
  { "east"     , "e"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_EAST },
  { "south"    , "s"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SOUTH },
  { "west"     , "w"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_WEST },
  { "up"       , "u"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_UP },
  { "down"     , "d"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_DOWN },
  { "northwest", "northw"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "nw"       , "nw"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "northeast", "northe"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "ne"       , "ne"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "southeast", "southe"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "se"       , "se"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "southwest", "southw"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "sw"       , "sw"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },

  /* now, the main list */
  { "art"      , "a"		, POS_RESTING , do_cast     , 0, ADMLVL_NONE	, SCMD_ART },
  { "account"  , "account"      , POS_DEAD    , do_account  , 0, ADMLVL_NONE    , 0 },
  { "accexp"   , "accexp"       , POS_DEAD    , do_accexp   , 0, ADMLVL_NONE    , 0 },
  { "at"       , "at"		, POS_DEAD    , do_at       , 1, ADMLVL_GOD	, 0 },
  { "addnews"  , "add" 	        , POS_DEAD    , do_gen_write, 1, ADMLVL_GOD     , SCMD_NEWS },
  { "advance"  , "adv"		, POS_DEAD    , do_advance  , 1, ADMLVL_GOD	, 0 },
  { "aedit"    , "aed"	 	, POS_DEAD    , do_oasis    , 1, ADMLVL_GOD	, SCMD_OASIS_AEDIT },
  { "airship"  , "airship"      , POS_RESTING , do_shuttle  , 0, ADMLVL_NONE    , 0 },
  { "alias"    , "ali"		, POS_DEAD    , do_alias    , 0, ADMLVL_NONE	, 0 },
  { "allctell" , "allc"		, POS_DEAD    , do_gen_tog    , 1, ADMLVL_IMPL	, SCMD_ALLCTELL },
  { "affects"  , "aff"          , POS_DEAD    , do_affstats , 0, ADMLVL_NONE    , 0 },
  { "affstats" , "affs"         , POS_DEAD    , do_affstats , 0, ADMLVL_NONE    , 0 },
  { "afk"      , "afk"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AFK },
  { "approach" , "appr"         , POS_FIGHTING, do_approach , 0, ADMLVL_NONE    , 0 },
  { "approve"  , "appro"        , POS_DEAD    , do_approve  , 1, ADMLVL_IMMORT  , SCMD_APPROVE },
  { "artisan"  , "art"		, POS_DEAD    , do_artisan  , 0, ADMLVL_NONE	, 0 },
  { "assist"   , "ass"		, POS_FIGHTING, do_assist   , 0, ADMLVL_NONE	, 0 },
  { "assemble" , "asse"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_ASSEMBLE },
  { "assedit"  , "assed"	, POS_STANDING, do_assedit  , 1, ADMLVL_BUILDER	, 0},
  { "astat"    , "ast"		, POS_DEAD    , do_astat    , 1, ADMLVL_GOD	, 0 },
  { "ask"      , "ask"		, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_ASK },
  { "attack"   , "att"          , POS_FIGHTING, do_hit   , 0, ADMLVL_NONE    , SCMD_HIT },
  { "auction"  , "auction"      , POS_RESTING , do_auction  , 0, ADMLVL_NONE    , 0 },
  { "auctalk"  , "auctalk"      , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE    , SCMD_AUCTION },
  { "augment"  , "augment"      , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "auto"     , "auto"         , POS_DEAD    , do_toggle   , 0, ADMLVL_NONE    , 0 },
  { "autoassist", "autoass"     , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOASSIST },
  { "autoattack", "autoatt"     , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOATTACK },
  { "autocon"  , "autoco"       , POS_DEAD    , do_autocon  , 0, ADMLVL_NONE    , 0 },
  { "autoexit" , "autoex"	, POS_DEAD    , do_autoexit , 0, ADMLVL_NONE	, 0 },
  { "autofeint", "autofeint"    , POS_DEAD    , do_autofeint, 0, ADMLVL_NONE    , 0 },
  { "autogold" , "autogo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOGOLD },
  { "autoloot" , "autolo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOLOOT },
  { "automap"  , "automa"       , POS_DEAD    , do_automap  , 0, ADMLVL_NONE    , 0 },
  { "automem"  , "autome"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOMEM },
  { "autosac"  , "autosa"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOSAC },
  { "autosplit", "autosp"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOSPLIT },
  { "awardexp" , "awarde"       , POS_DEAD    , do_awardexp , 1, ADMLVL_GRGOD   , 0 },

  { "backstab" , "ba"		, POS_FIGHTING, do_kill	    , 0, ADMLVL_NONE	, 0 }, /* part of normal fighting */
  { "backup"   , "back"         , POS_DEAD    , do_backup   , 0, ADMLVL_NONE    , 0 },
  { "bake"     , "bak"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_BAKE },
  { "ban"      , "ban"		, POS_DEAD    , do_ban      , 1, ADMLVL_GRGOD	, 0 },
  { "balance"  , "bal"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "barding"  , "barding"      , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "bid"      , "bid"          , POS_RESTING , do_bid      , 0, ADMLVL_NONE    , 0 },
  { "binditem" , "bindi"        , POS_RESTING , do_binditem , 0, ADMLVL_NONE    , 0 },
  { "bleedingattack" , "bleed"  , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_BLEEDING_ATTACK }, 
  { "buildwalk", "buildwalk"	, POS_STANDING, do_gen_tog, 1, ADMLVL_BUILDER	, SCMD_BUILDWALK },
  { "bonuslevels", "bonusl"     , POS_DEAD    , do_bonuslevels, 0, ADMLVL_NONE  , 0 },
  { "bounty"   , "bount"        , POS_RESTING , do_bounty   , 0, ADMLVL_NONE    , 0 },
  { "breathweapon", "breathw"   , POS_FIGHTING, do_breath_weapon, 0, ADMLVL_NONE, 0 },
  { "break"    , "break"	, POS_STANDING, do_break    , 1, ADMLVL_IMMORT	, 0 },
  { "brew"     , "brew"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_BREW },
  { "brief"    , "br"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_BRIEF },
  { "briefmap" , "briefm"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_BRIEFMAP },
  { "buy"      , "bu"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "buff"     , "buff"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "bug"      , "bug"		, POS_DEAD    , do_post_forums, 0, ADMLVL_NONE	, SCMD_BUG },
  { "buck"     , "buck"         , POS_STANDING, do_buck     , 0, ADMLVL_NONE    , 0 },

  { "cast"     , "c"		, POS_RESTING , do_cast     , 0, ADMLVL_NONE	, SCMD_CAST },
  { "call"     , "call"         , POS_RESTING , do_call     , 0, ADMLVL_NONE    , 0 },
  { "callset"  , "calls"        , POS_RESTING , do_callset  , 0, ADMLVL_NONE    , 0 },
  { "callcompanion", "callcomp" , POS_RESTING , do_callcompanion, 0, ADMLVL_NONE    , 0 },
  { "callmount", "callmount"    , POS_RESTING , do_callmount, 0, ADMLVL_NONE    , 0 },
  { "camp"     , "camp"         , POS_RESTING , do_setcamp  , 0, ADMLVL_NONE    , 0 },
  { "carriage" , "carriage"     , POS_RESTING , do_speeder  , 0, ADMLVL_NONE    , SCMD_TAXI },
  { "cedit"    , "cedit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_IMPL	, SCMD_OASIS_CEDIT },
  { "chat"     , "ch"           , POS_DEAD    , do_gen_comm , 0, ADMLVL_NONE    , SCMD_GOSSIP},
  { "check"    , "che"		, POS_STANDING, do_mail     , 0, ADMLVL_NONE	, 0 },
  { "checkcraft", "chechc"	, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "chant"    , "cha"          , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_CHANT },
  { "challenge", "chal"         , POS_FIGHTING, do_challenge, 0, ADMLVL_NONE, SCMD_DIPLOMACY },
  { "chooseguild", "chooseguild", POS_DEAD    , do_chooseguild, 0, ADMLVL_NONE  , 0 },
  { "chown"    , "cho"		, POS_DEAD    , do_chown    , 1, ADMLVL_GRGOD	, 0 },
  { "clear"    , "cle"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "close"    , "cl"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_CLOSE },
  { "classabilities", "class"   , POS_DEAD    , do_classabilities, 0, ADMLVL_NONE, 0 },
  { "cls"      , "cls"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "clsolc"   , "clsolc"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMMORT	, SCMD_CLS },
  { "coins"    , "co"     , POS_FIGHTING, do_gold     , 0, ADMLVL_NONE  , 0 },
  { "commune"  , "com"          , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_COMMUNE },
  { "combatcmd", "comb"         , POS_RESTING , do_combatcmd, 0, ADMLVL_NONE    , 0 },
  { "companion", "comp"         , POS_RESTING , do_companion, 0, ADMLVL_NONE    , 0 },
  { "consider" , "con"		, POS_RESTING , do_consider , 0, ADMLVL_NONE	, 0 },
  { "contain"  , "contain"	, POS_DEAD    , do_contained_areas, 0, ADMLVL_NONE, 0 },
  { "convert"  , "convert"      , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "color"    , "col"		, POS_DEAD    , do_color    , 0, ADMLVL_NONE	, 0 },
  { "compare"  , "comp"		, POS_RESTING , do_compare  , 0, ADMLVL_NONE	, 0 },
  { "commands" , "com"		, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_COMMANDS },
  { "compact"  , "compact"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_COMPACT },
  { "copyover" , "copyover"	, POS_DEAD    , do_copyover , 1, ADMLVL_GRGOD	, 0 },
  { "coupdegrace", "coup"       , POS_FIGHTING, do_coupdegrace, 0, ADMLVL_NONE  , 0 },
  { "craft"    , "craft"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_CRAFT },
  { "craftingbrief", "craftingb", POS_DEAD    , do_craftingbrief, 0, ADMLVL_NONE, 0 },
  { "create"   , "create"       , POS_STANDING, do_not_here , 0, ADMLVL_NONE    , 0 },
  { "credits"  , "cred"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CREDITS },

  { "caccept"  , "cacc"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_ACCEPT },
  { "capply"   , "capp"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_APPLY },
  { "clanedit" , "claned"  , POS_DEAD    , do_oasis    , 1, ADMLVL_GRGOD, SCMD_OASIS_CLANEDIT },
  { "cbalance" , "cbal"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_BALANCE },
  { "cdemote"  , "cdem"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_DEMOTE},
  { "cdeposit" , "cdep"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_DEPOSIT },
  { "cdismiss" , "cdis"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_DISMISS },
  { "clans"    , "cla"     , POS_SLEEPING, do_show_clan, 0, ADMLVL_NONE, 0 },
  { "clantalk" , "clant"   , POS_SLEEPING, do_gen_tog  , 0, ADMLVL_NONE, SCMD_CLANTALK },
  { "ctalk"    , "ct"      , POS_SLEEPING, do_gen_tog  , 0, ADMLVL_NONE, SCMD_CLANTALK },
  { "csay"     , "csa"     , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_TELL },
  { "ctell"    , "cte"     , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_TELL },
  { "cwho"     , "cwh"     , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_WHO },
  { "cpromote" , "cprom"   , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_PROMOTE },
  { "cresign"  , "cres"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_RESIGN },
  { "crevoke"  , "crev"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_REVOKE },
  { "creject"  , "crej"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_REJECT },
  { "cwithdraw", "cwit"    , POS_SLEEPING, do_clan     , 0, ADMLVL_NONE, SCMD_CLAN_WITHDRAW_GOLD },


  { "date"     , "da"		, POS_DEAD    , do_date     , 1, ADMLVL_NONE	, SCMD_DATE },
  { "dc"       , "dc"		, POS_DEAD    , do_dc       , 1, ADMLVL_GOD	, 0 },
  { "deathattack", "deathatt"   , POS_STANDING, do_deathattack, 0, ADMLVL_NONE  , 0 },
  { "defensivestance", "defensive", POS_FIGHTING, do_defensive_stance, 0, ADMLVL_NONE, 0 },
  { "deposit"  , "depo"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "destroy"  , "des"		, POS_SLEEPING, do_sac      , 0, ADMLVL_NONE	, 0 },
  { "devote"   , "dev"          , POS_FIGHTING, do_devote   , 0, ADMLVL_NONE    , 0 },
  { "diagnose" , "diag"		, POS_RESTING , do_diagnose , 0, ADMLVL_NONE	, 0 },
  { "disable"  , "disa"		, POS_DEAD    , do_disable  , 1, ADMLVL_GRGOD	, 0 },
  { "disenchant", "disench"     , POS_DEAD    , do_disenchant, 0, ADMLVL_NONE   , SCMD_DISENCHANT },
  { "disguise" , "disguise"     , POS_RESTING , do_disguise , 0, ADMLVL_NONE    , 0 },
  { "dismiss"  , "dismiss"      , POS_RESTING , do_dismiss_mob, 0, ADMLVL_NONE    , 0 },
  { "dig"      , "dig"		, POS_DEAD    , do_dig      , 1, ADMLVL_BUILDER	, 0 },
//  { "disarm"   , "dis"	, POS_STANDING, do_disarm   , 0, 0 },
  { "display"  , "disp"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "dismount" , "dism"         , POS_STANDING, do_dismount , 0, ADMLVL_NONE    , 0 },
  { "divide"   , "divide"       , POS_DEAD    , do_divide   , 0, ADMLVL_NONE , 0 },
  { "divine"   , "divine"       , POS_DEAD    , do_divine_feats, 0, ADMLVL_NONE , 0 },
  { "divinebond", "divineb"     , POS_DEAD    , do_divine_bond, 0, ADMLVL_NONE , 0 },
  { "domains"  , "domains"      , POS_DEAD    , do_domain   , 0, ADMLVL_NONE    , 0 },
//  { "donate"   , "don"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DONATE },
  { "dragonbreath", "dragonbr"  , POS_FIGHTING, do_dragonbreath, 0, ADMLVL_NONE , 0 },
  { "drink"    , "dri"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_DRINK },
  { "drive"    , "drive"	, POS_STANDING, do_drive    , 0, ADMLVL_NONE	, 0 },
  { "drop"     , "dro"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DROP },
  { "dusk"     , "dusk"         , POS_FIGHTING, do_cast     , 0, ADMLVL_NONE    , SCMD_DUSK },

  { "eat"      , "ea"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_EAT },
  { "echo"     , "ec"		, POS_SLEEPING, do_echo     , 1, ADMLVL_GOD	, SCMD_ECHO },
  { "emote"    , "em"		, POS_RESTING , do_echo     , 0, ADMLVL_NONE	, SCMD_EMOTE },
  { "emporium" , "emporium"     , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { ":"        , ":"		, POS_RESTING, do_echo      , 0, ADMLVL_NONE	, SCMD_EMOTE },
  { "endturn"  , "end"          , POS_FIGHTING, do_endturn  , 0, ADMLVL_NONE    , 0 },
  { "enter"    , "ent"		, POS_STANDING, do_enter    , 0, ADMLVL_NONE	, 0 },
  { "enchant"  , "ench"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "equipment", "eq"		, POS_SLEEPING, do_equipment, 0, ADMLVL_NONE	, 0 },
  { "eqstats"  , "eqstats"	, POS_SLEEPING, do_eqstats  , 0, ADMLVL_NONE	, 0 },
  { "evoke"    , "ev"           , POS_FIGHTING, do_cast     , 0, ADMLVL_NONE    , SCMD_EVOKE },
  { "exits"    , "ex"		, POS_RESTING , do_exits    , 0, ADMLVL_NONE	, 0 },
  { "examine"  , "exa"		, POS_RESTING , do_examine  , 0, ADMLVL_NONE	, 0 },
  { "expertise", "exp"    , POS_FIGHTING, do_expertise, 0, ADMLVL_NONE  , 0  },
  { "edit"     , "edit"		, POS_DEAD    , do_edit     , 1, ADMLVL_IMPL	, 0 },      /* Testing! */

  { "falsealign", "falsea"      , POS_DEAD,     do_setfalsealign, 0, ADMLVL_NONE, 0 },
  { "falseethos", "falsee"      , POS_DEAD,     do_setfalseethos, 0, ADMLVL_NONE, 0 },
//  { "farm"     , "farm"         , POS_STANDING, do_harvest  , 0, ADMLVL_NONE    , SCMD_FARM },
  { "favoredenemy", "favoreden" , POS_DEAD    , do_favoredenemy, 0, ADMLVL_NONE , 0 },
  { "feats"    , "fea"		, POS_DEAD    , do_feats    , 0, ADMLVL_NONE	, 0 },
  { "featset"  , "featset"      , POS_DEAD    , do_featset  , 1, ADMLVL_GRGOD	, 0 },
  { "feint"    , "fei"    , POS_FIGHTING, do_feint    , 0, ADMLVL_NONE,  0 },
  { "fightspam", "fightspam"    , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_FIGHT_SPAM },
  { "fill"     , "fil"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_FILL },
  { "file"     , "fi"		, POS_SLEEPING, do_file     , 1, ADMLVL_GRGOD	, 0 },
  { "fix"      , "fix"		, POS_STANDING, do_fix      , 0, ADMLVL_NONE	, 0 },
  { "flee"     , "fl"		, POS_FIGHTING, do_flee     , 0, ADMLVL_NONE	, 0 },
  { "fletch"   , "fletch"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_FLETCH },
  { "flurry"   , "flu"    , POS_DEAD    , do_flurry  , 0, ADMLVL_NONE   , 0 },
  { "follow"   , "fol"		, POS_RESTING , do_follow   , 0, ADMLVL_NONE	, 0 },
  { "force"    , "force"	, POS_SLEEPING, do_force    , 1, ADMLVL_GOD	, 0 },
//  { "forest"   , "forest"       , POS_STANDING, do_harvest  , 0, ADMLVL_NONE    , SCMD_FOREST },
  { "forge"    , "for"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_FORGE },
  { "forget"   , "forget"	, POS_RESTING, do_memorize  , 0, ADMLVL_NONE	, SCMD_FORGET },
  { "form"     , "form"         , POS_RESTING, do_form      , 0, ADMLVL_NONE    , 0 },
  { "freeze"   , "freeze"	, POS_DEAD    , do_wizutil  , 1, ADMLVL_FREEZE	, SCMD_FREEZE },

  { "gain"     , "ga"		, POS_RESTING , do_gain , 0, ADMLVL_NONE	, 0 },
  { "gatherinfo", "gatheri"     , POS_RESTING , do_gatherinfo, 0, ADMLVL_NONE   , 0 },
  { "get"      , "get"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "gecho"    , "gecho"	, POS_DEAD    , do_gecho    , 1, ADMLVL_GOD	, 0 },
  { "gedit"    , "gedit"      	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_GEDIT },
  { "gemote"   , "gem"	 	, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GEMOTE },
  { "glist"    , "glist"	, POS_SLEEPING, do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_GLIST },
  { "give"     , "gi"		, POS_RESTING , do_give     , 0, ADMLVL_NONE	, 0 },
  { "gift"     , "gift"         , POS_DEAD    , do_gift     , 0, ADMLVL_NONE    , 0 },
  { "goto"     , "go"		, POS_SLEEPING, do_goto     , 1, ADMLVL_IMMORT	, 0 },
  { "gold"     , "gol"		, POS_RESTING , do_gold     , 0, ADMLVL_NONE	, 0 },
  { "broadcast", "broadcast"     , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GOSSIP },
  { "group"    , "gro"		, POS_SLEEPING , do_group    , 0, ADMLVL_NONE	, 0 },
  { "greet"    , "gre"          , POS_RESTING , do_greet    , 0, ADMLVL_NONE    , 0 },
  { "grab"     , "grab"		, POS_RESTING , do_grab     , 0, ADMLVL_NONE	, 0 },
  { "newbie"   , "newbie"	, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GRATZ },
  { "gsay"     , "gsay"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },
  { "gtell"    , "gt"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },
  { "guildchat", "gu"           , POS_DEAD    , do_guildchat, 0, ADMLVL_NONE    , 0 },
  { "guildexp", "guilde"        , POS_DEAD    , do_guildexp , 0, ADMLVL_NONE    , 0 },
  { "guildbonuses", "guildb"    , POS_DEAD    , do_guildbonuses, 0, ADMLVL_NONE , 0 },
  { "guildscore", "guildsc"     , POS_DEAD    , do_guildscore, 0, ADMLVL_NONE   , 0 },
  { "guard"    , "gua"          , POS_FIGHTING, do_guard    , 0, ADMLVL_NONE    , 0 },
  { "guildwho" , "guildwho"     , POS_DEAD    , do_guildwho , 0, ADMLVL_NONE    , 0 },
  { "help"     , "he"		, POS_DEAD    , do_help     , 0, ADMLVL_NONE	, 0 },
  { "hedit"    , "hedit"        , POS_DEAD,     do_oasis    , 1, ADMLVL_BUILDER , SCMD_OASIS_HEDIT },
  { "haggle"   , "hag"          , POS_RESTING , do_haggle   , 0, ADMLVL_NONE    , 0 },
  { "handbook" , "handb"	, POS_DEAD    , do_gen_ps   , 1, ADMLVL_IMMORT	, SCMD_HANDBOOK },
  { "harvest"  , "harvest"      , POS_STANDING, do_harvest_new , 0, ADMLVL_NONE    , 0 },
  { "hcontrol" , "hcontrol"	, POS_DEAD    , do_hcontrol , 1, ADMLVL_GOD	, 0 },
  { "hpay"     , "hpay"         , POS_DEAD    , do_hpay     , 0, ADMLVL_NONE    , 0 },
  { "heal"     , "heal"         , POS_STANDING, do_heal     , 0, ADMLVL_NONE    , 0 },
  { "hide"     , "hide"		, POS_RESTING , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_HIDE },
  { "hit"      , "hit"		, POS_FIGHTING, do_hit      , 0, ADMLVL_NONE	, SCMD_HIT },
  { "hold"     , "hold"		, POS_RESTING , do_grab     , 0, ADMLVL_NONE	, 0 },
  { "holler"   , "holler"	, POS_RESTING , do_gen_comm , 0, ADMLVL_NONE	, SCMD_HOLLER },
  { "holylight", "holy"		, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMMORT	, SCMD_HOLYLIGHT },
  { "house"    , "house"	, POS_RESTING , do_house    , 0, ADMLVL_NONE	, 0 },
//  { "hunt"     , "hunt"         , POS_STANDING, do_harvest  , 0, ADMLVL_NONE    , SCMD_HUNT },

  { "i"        , "i"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "identify" , "id"           , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "idcost"   , "idcost"       , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "idea"     , "ide"		, POS_DEAD    , do_post_forums, 0, ADMLVL_NONE	, SCMD_IDEA },
  { "imbue"    , "imb"           , POS_FIGHTING, do_cast     , 0, ADMLVL_NONE    , SCMD_IMBUE },
  { "imotd"    , "imotd"	, POS_DEAD    , do_gen_ps   , 1, ADMLVL_IMMORT	, SCMD_IMOTD },
  { "immlist"  , "imm"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_IMMLIST },
  { "inside"   , "in"		, POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_IN },
  { "innate"   , "inn"          , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_INNATE },
  { "intone"   , "int"          , POS_RESTING, do_cast     , 0, ADMLVL_NONE     , SCMD_INTONE },
  { "intro"    , "intr"         , POS_RESTING , do_intro    , 0, ADMLVL_NONE    , 0 },
  { "intimidate", "inti"  , POS_FIGHTING, do_taunt    , 0, ADMLVL_NONE  , SCMD_INTIMIDATE },
  { "info"     , "info"		, POS_SLEEPING, do_gen_ps   , 0, ADMLVL_NONE	, SCMD_INFO },
  { "insult"   , "insult"	, POS_RESTING , do_insult   , 0, ADMLVL_NONE	, 0 },
  { "inventory", "inv"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "iedit"    , "ie"   	, POS_DEAD    , do_iedit    , 1, ADMLVL_GOD	, 0 },
  { "invis"    , "invi"		, POS_DEAD    , do_invis    , 1, ADMLVL_IMMORT	, 0 },
  { "item"     , "item"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },

  { "jog"      , "jo"           , POS_STANDING, do_jog      , 0, ADMLVL_NONE    , 0 },
  { "junk"     , "junk"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_JUNK },

  { "kit"      , "k"          , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "kick"     , "ki"           , POS_FIGHTING, do_kick     , 0, ADMLVL_NONE    , 0 },
  { "kill"     , "kill"		, POS_FIGHTING, do_hit     , 0, ADMLVL_NONE	, 0 },
  { "knit"     , "knit"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_KNIT },
  { "knockdown", "knockd"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_KNOCKDOWN },

  { "look"     , "l"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_LOOK },
  { "lore"     , "lor"          , POS_RESTING , do_lore     , 0, ADMLVL_NONE    , 0 },
  { "languages", "lang"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "last"     , "last"		, POS_DEAD    , do_last     , 1, ADMLVL_GOD	, 0 },
  { "layhands" , "lay"		, POS_FIGHTING, do_lay_hands, 0, ADMLVL_NONE	, 0 },
//  { "learn"    , "lear"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "leave"    , "lea"		, POS_STANDING, do_leave    , 0, ADMLVL_NONE	, 0 },
  { "levelup"  , "levelu"       , POS_RESTING , do_levelup  , 0, ADMLVL_NONE    , 0 },
  { "levels"   , "levels"	, POS_DEAD    , do_levels   , 0, ADMLVL_NONE	, 0 },
  { "lfg"      , "lfg"		, POS_DEAD    , do_lfg      , 0, ADMLVL_NONE	, 0 },
  { "list"     , "lis"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "listclasses","listclasses" , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "listcomponents", "listcomp", POS_DEAD    , do_assemble , 0, ADMLVL_NONE    , SCMD_LIST_COMPONENTS }, 
  { "listguilds", "listguilds"  , POS_DEAD    , do_listguilds, 0, ADMLVL_NONE   , 0 },
  { "listraces", "listraces"    , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "links"    , "lin"		, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_LINKS },
  { "lock"     , "loc"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_LOCK },
  { "load"     , "load"		, POS_DEAD    , do_load     , 1, ADMLVL_GOD	, 0 },
  { "loadcrystal", "loadc"        , POS_DEAD    , do_loadcrystal, 1, ADMLVL_GRGOD   , 0 },
  { "loadmagic", "loadm"        , POS_DEAD    , do_loadmagic, 1, ADMLVL_GRGOD   , 0 },

  { "motd"     , "motd"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_MOTD },
  { "mail"     , "mail"		, POS_STANDING, do_new_mail     , 0, ADMLVL_NONE	, 0 },
  { "map"      , "map"          , POS_STANDING, do_map      , 0, ADMLVL_NONE    , 0 },
  { "mark"     , "mark"         , POS_RESTING , do_mark     , 0, ADMLVL_NONE    , 0 },
//  { "mine"     , "mine"         , POS_STANDING, do_harvest  , 0, ADMLVL_NONE    , SCMD_MINE },
  { "mix"      , "mix"		, POS_STANDING, do_assemble , 0, ADMLVL_NONE	, SCMD_MIX },
  { "medit"    , "medit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_MEDIT },
  { "memorize" , "mem"		, POS_RESTING , do_memorize , 0, ADMLVL_NONE	, SCMD_MEMORIZE },
  { "memwhen"  , "memwhe"    	, POS_RESTING , do_memorize , 0, ADMLVL_NONE	, SCMD_WHEN_SLOT },
  { "mentor"   , "ment"         , POS_SLEEPING, do_mentor   , 0, ADMLVL_NONE    , 0 },
  { "metamagic", "meta"         , POS_SLEEPING, do_metamagic, 0, ADMLVL_NONE    , 0 },
  { "mlist"    , "mlist"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_MLIST },
  { "mount"    , "mou"          , POS_FIGHTING, do_mount    , 0, ADMLVL_NONE    , 0 },
  { "mreport"  , "mrep"   , POS_RESTING , do_mreport  , 0, ADMLVL_NONE  , 0 },
  { "mrest"    , "mres"   , POS_RESTING , do_mrest    , 0, ADMLVL_NONE  , 0 },
  { "mstand"   , "mst"    , POS_RESTING , do_mstand   , 0, ADMLVL_NONE  , 0 },
  { "music"    , "mus"          , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_MUSIC },
  { "mute"     , "mute"		, POS_DEAD    , do_wizutil  , 1, ADMLVL_GOD	, SCMD_SQUELCH },
  { "murder"   , "murder"	, POS_FIGHTING, do_attack   , 0, ADMLVL_NONE	, SCMD_MURDER },

  { "news"     , "news"		, POS_SLEEPING, do_file     , 0, ADMLVL_NONE	, SCMD_NEWS },
  { "nochat"   , "nochat"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGOSSIP },
  { "nocompress","nocompress"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOCOMPRESS },
  { "nohassle" , "nohassle"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMMORT	, SCMD_NOHASSLE },
  { "nohelps"  , "nohelps"      , POS_DEAD    , do_nohelps  , 0, ADMLVL_IMMORT  , 0 },
  { "nohints"  , "nohints"      , POS_DEAD    , do_nohints  , 0, ADMLVL_NONE    , 0 },
  { "nonewbie"  , "nonewbie"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGRATZ },
  { "norepeat" , "norepeat"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOREPEAT },
  { "noshout"  , "noshout"	, POS_SLEEPING, do_gen_tog  , 0, ADMLVL_NONE	, SCMD_DEAF },
  { "nosuicide", "nosuicide"    , POS_DEAD    , do_nosuicide, 0, ADMLVL_NONE    , 0 },
  { "nosummon" , "nosummon"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOSUMMON },
  { "note"     , "note"		, POS_SLEEPING, do_note     , 0, ADMLVL_NONE	, 0 },
  { "notell"   , "notell"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOTELL },
  { "notitle"  , "notitle"	, POS_DEAD    , do_wizutil  , 1, ADMLVL_GOD	, SCMD_NOTITLE },
  { "nowiz"    , "nowiz"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOWIZ },

  { "order"    , "ord"		, POS_RESTING , do_order    , 0, ADMLVL_NONE	, 0 },
  { "offer"    , "off"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "ofind"    , "ofi"	    , POS_DEAD	  , do_ofind	, 0, ADMLVL_GOD	    , 0 },
  { "open"     , "ope"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_OPEN },
  { "olc"      , "olc"		, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OLC_SAVEINFO },
  { "olist"    , "olist"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_OEDIT },
  { "ooc"      , "ooc"          , POS_DEAD    , do_osay     , 0, ADMLVL_NONE    , 0 },
  { "output"    , "outp"     	, POS_DEAD    , do_output   , 0, ADMLVL_NONE	, 0 },
  { "outside"  , "outs"     	, POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_OUT },

  { "put"      , "put"		, POS_RESTING , do_put      , 0, ADMLVL_NONE	, 0 },
  { "page"     , "pag"		, POS_DEAD    , do_page     , 1, ADMLVL_NONE	, 0 },
  { "pardon"   , "pardon"	, POS_DEAD    , do_wizutil  , 1, ADMLVL_GOD	, SCMD_PARDON },
  { "pagelength", "pagel"	, POS_DEAD    , do_pagelength, 0, 0 },
  { "parry"    , "parry"        , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_PARRY },
  { "peace"    , "pea"		, POS_DEAD    , do_peace    , 1, ADMLVL_GOD	, 0 },
  { "pet"      , "pet"          , POS_DEAD    , do_action   , 0, ADMLVL_NONE    , 0 },
  { "petition" , "peti"         , POS_DEAD    , do_petition , 0, ADMLVL_NONE    , 0 },
  { "petset"   , "pets"		, POS_DEAD	  , do_petset   , 0, ADMLVL_NONE	, 0 },
  { "pick"     , "pi"		, POS_STANDING, do_gen_door , 0, ADMLVL_NONE	, SCMD_PICK },
  { "pilfer"   , "pil"          , POS_FIGHTING, do_pilfer   , 0, ADMLVL_NONE    , 0 },
  { "pilot"    , "pilot"	, POS_STANDING, do_drive    , 0, ADMLVL_NONE	, 0 },
  { "players"  , "play"		, POS_DEAD    , do_players  , 1, ADMLVL_GOD	, 0 },
  { "playerguilds", "playerguilds", POS_DEAD  , do_playerguilds, 1, ADMLVL_GOD	, 0 },
  { "poll"     , "poll"         , POS_DEAD    , do_poll     , 0, ADMLVL_NONE    , 0 },
  { "policy"   , "poli"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_POLICIES },
  { "poofin"   , "poofi"	, POS_DEAD    , do_poofset  , 1, ADMLVL_IMMORT	, SCMD_POOFIN },
  { "poofout"  , "poofo"	, POS_DEAD    , do_poofset  , 1, ADMLVL_IMMORT	, SCMD_POOFOUT },
  { "pour"     , "pour"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_POUR },
  { "powerattack", "pow"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_POWERATT },
  { "powerfulsneak", "powerfuls", POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_POWERFUL_SNEAK },
  { "pray"     , "pra"          , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_PRAY },
  { "prompt"   , "pro"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "practice" , "pra"		, POS_RESTING , do_practice_skills , 0, ADMLVL_NONE	, 0 },
  { "purge"    , "purge"	, POS_DEAD    , do_purge    , 1, ADMLVL_GOD	, 0 },
  { "pvp"      , "pvp"          , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_PVP },

  { "qedit"    , "qedit"        , POS_DEAD    , do_oasis_qedit, 1, ADMLVL_BUILDER, 0 },
  { "qlist"    , "qlist"        , POS_DEAD    , do_oasis_list, 1, ADMLVL_BUILDER, SCMD_OASIS_QLIST },
  { "quaff"    , "qua"		, POS_RESTING , do_use      , 0, ADMLVL_NONE	, SCMD_QUAFF },
  { "qecho"    , "que"		, POS_DEAD    , do_qcomm    , 1, ADMLVL_GOD	, SCMD_QECHO },
  { "quest"    , "que"		, POS_DEAD    , do_quest    , 0, ADMLVL_NONE	, SCMD_QUEST },
  { "qui"      , "qui"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, 0 },
  { "quit"     , "quit"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, SCMD_QUIT },
  { "qsay"     , "qsay"		, POS_RESTING , do_qcomm    , 0, ADMLVL_NONE	, SCMD_QSAY },

  { "rest"     , "re"		, POS_RESTING , do_rest     , 0, ADMLVL_NONE	, 0 },
  { "rebind"   , "reb"          , POS_DEAD    , do_rebind   , 0, ADMLVL_GOD     , 0 },
  { "respec"   , "respec"       , POS_DEAD    , do_respec   , 1, ADMLVL_NONE    , 0 },
  { "restring" , "restr"	, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "recall"   , "rec"          , POS_RESTING , do_recall   , 0, ADMLVL_NONE    , 0 },
  { "reimburse","reimb"         , POS_DEAD    , do_reimburse, 1, ADMLVL_GOD     , 0 },
  { "reject"   , "rej"          , POS_DEAD    , do_approve  , 1, ADMLVL_IMMORT  , SCMD_REJECT },
  { "research" , "research", POS_RESTING, do_not_here , 0, ADMLVL_NONE, 0 },
  { "review"   , "rev"          , POS_DEAD    , do_review   , 1, ADMLVL_IMMORT  , 0 },
  { "rage"     , "rage"   , POS_FIGHTING, do_rage     , 0, ADMLVL_NONE, 0 },
  { "raise"    , "rai"		, POS_STANDING, do_raise    , 1, ADMLVL_GOD	, 0 },
  { "random"   , "ran"    , POS_FIGHTING, do_random   , 0, ADMLVL_NONE , 0 },
  { "rapidshot", "rapids"       , POS_DEAD    , do_rapidshot, 0, ADMLVL_NONE    , 0 },
  { "reply"    , "rep"		, POS_SLEEPING, do_reply    , 0, ADMLVL_NONE	, 0 },
  { "read"     , "rea"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_READ },
  { "reform"   , "ref"          , POS_RESTING , do_reform   , 0, ADMLVL_NONE    , 0 },
  { "reload"   , "reload"	, POS_DEAD    , do_reboot   , 1, ADMLVL_GOD	, 0 },
  { "recite"   , "reci"		, POS_RESTING , do_use      , 0, ADMLVL_NONE	, SCMD_RECITE },
  { "receive"  , "rece"		, POS_STANDING, do_mail     , 0, ADMLVL_NONE	, 0 },
  { "release"  , "rel"    , POS_RESTING , do_release  , 0, ADMLVL_NONE  , 0 },
  { "remove"   , "rem"		, POS_RESTING , do_remove   , 0, ADMLVL_NONE	, 0 },
  { "rent"     , "rent"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "report"   , "repo"		, POS_RESTING , do_report   , 0, ADMLVL_NONE	, 0 },
//  { "reroll"   , "rero"		, POS_DEAD    , do_wizutil  , 1, ADMLVL_IMPL	, SCMD_REROLL },
  { "rerollheightandweight", "rerollh", POS_DEAD, do_rerollheightandweight, 0, ADMLVL_NONE, 0 },
  { "rescue"   , "res"          , POS_FIGHTING, do_rescue   , 0, ADMLVL_NONE    , 0 },
  { "resetartisanexp", "resetart", POS_DEAD   , do_resetartisan, 1, ADMLVL_GOD , 0 },
  { "resize"   , "resi"         , POS_STANDING, do_not_here , 0, ADMLVL_NONE    , 0 },
  { "respond"  , "resp" 	, POS_RESTING,  do_respond  , 0, ADMLVL_NONE	, 0 },
  { "restore"  , "resto"	, POS_DEAD    , do_restore  , 1, ADMLVL_GOD	, 0 },
  { "resurrect", "resu"		, POS_DEAD    , do_resurrect, 0, ADMLVL_NONE	, 0 },
  { "retreat"  , "retr"         , POS_FIGHTING, do_retreat  , 0, ADMLVL_NONE    , 0 },
  { "return"   , "retu"		, POS_DEAD    , do_return   , 0, ADMLVL_NONE	, 0 },
  { "redit"    , "redit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_REDIT },
  { "rlist"    , "rlist"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_RLIST },
  { "robilarsgambit", "robilarsg", POS_DEAD   , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_ROBILARS_GAMBIT },
  { "roomflags", "roomf"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMMORT	, SCMD_ROOMFLAGS },
  { "rpsheet"  , "rpsheet"      , POS_DEAD    , do_rpsheet  , 0, ADMLVL_NONE    , 0 },
  { "rclone"   , "rclone"  	, POS_DEAD    , do_room_copy, 1, ADMLVL_BUILDER	, 0 },
  { "run"      , "ru"           , POS_STANDING, do_run      , 0, ADMLVL_NONE    , 0 },
  { "rsay"     , "rsay"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "rgsay"     , "rsay"		, POS_RESTING , do_gsay      , 0, ADMLVL_NONE	, SCMD_RP_GSAY },
  { "rpsay"    , "rpsay"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "rpgsay"    , "rpsay"		, POS_RESTING , do_gsay      , 0, ADMLVL_NONE	, SCMD_RP_GSAY },
  { "say"      , "sa"		, POS_RESTING , do_osay      , 0, ADMLVL_NONE	, 0 },
  { "'"        , "'"		, POS_RESTING , do_osay      , 0, ADMLVL_NONE	, 0 },
  { "save"     , "sav"		, POS_SLEEPING, do_save     , 0, ADMLVL_NONE	, 0 },
  { "saveall"  , "saveall"	, POS_DEAD    , do_saveall  , 1, ADMLVL_BUILDER	, 0},
  { "score"    , "sc"		, POS_DEAD    , do_aod_new_score    , 0, ADMLVL_NONE, SCMD_SCORE_TEXT },
  { "scan"     , "sca"     , POS_RESTING , do_scan     , 0, 0              , 0},
  { "screenwidth", "screenw"    , POS_DEAD    , do_screenwidth, 0, ADMLVL_NONE  , 0 },
  { "scribe"   , "scr"		, POS_RESTING , do_scribe   , 0, ADMLVL_NONE	, 0 },
  { "search"   , "sea"		, POS_STANDING, do_look     , 0, ADMLVL_NONE	, SCMD_SEARCH },
  { "sell"     , "sell"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "sedit"    , "sedit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_SEDIT },
  { "send"     , "send"		, POS_SLEEPING, do_send     , 1, ADMLVL_GOD	, 0 },
  { "set"      , "set"		, POS_DEAD    , do_set      , 1, ADMLVL_GOD	, 0 },
  { "setaffinity", "setaffinity", POS_DEAD    , do_setaffinity, 0, ADMLVL_NONE  , 0 },
//  { "setactive", "setact"       , POS_DEAD    , do_setactive, 0, ADMLVL_NONE    , 0 },
  { "setrace"  , "setrace"      , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "setalign" , "setalign"     , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "setethos" , "setethos"     , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "setstats" , "setstats"     , POS_DEAD    , do_set_stats , 0, ADMLVL_NONE    , 0 },
  { "setdescs" , "setdescs"     , POS_DEAD    , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "setheight", "setheight"    , POS_DEAD    , do_setheight, 0, ADMLVL_NONE    , 0 },
  { "setweight", "setweight"    , POS_DEAD    , do_setweight, 0, ADMLVL_NONE    , 0 },
  { "shout"    , "sho"		, POS_RESTING , do_gen_comm , 0, ADMLVL_NONE	, SCMD_SHOUT },
  { "shop"     , "shop"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "show"     , "show"		, POS_DEAD    , do_show     , 1, ADMLVL_IMMORT	, 0 },
  { "showcombat", "showcombat", POS_DEAD, do_show_combat, 0, ADMLVL_NONE, 0 },
  { "showguild", "showguild"    , POS_DEAD    , do_showguild, 0, ADMLVL_NONE    , 0 },
  { "shutdow"  , "shutdow"	, POS_DEAD    , do_shutdown , 1, ADMLVL_IMPL	, 0 },
  { "shutdown" , "shutdown"	, POS_DEAD    , do_shutdown , 1, ADMLVL_IMPL	, SCMD_SHUTDOWN },
  { "sing"     , "si"           , POS_RESTING , do_cast     , 0, ADMLVL_NONE    , SCMD_SING },
  { "sip"      , "sip"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_SIP },
  { "sit"      , "sit"		, POS_RESTING , do_sit      , 0, ADMLVL_NONE	, 0 },
  { "skills"  , "skill"        , POS_RESTING , do_show_sorted_lists, 0, ADMLVL_NONE , 0 },
  { "skillcheck", "skillcheck"  , POS_RESTING , do_skillcheck, 0, ADMLVL_NONE   , 0 },
  { "skillset" , "skillset"	, POS_SLEEPING, do_skillset , 1, ADMLVL_GRGOD	, 0 },
  
  { "sleep"    , "sl"		, POS_SLEEPING, do_sleep    , 0, ADMLVL_NONE	, 0 },
  { "slist"    , "slist"	, POS_SLEEPING, do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_SLIST },
  { "slowns"   , "slowns"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMPL	, SCMD_SLOWNS },
  { "smite"    , "smite"         , POS_FIGHTING, do_smite    , 0, ADMLVL_NONE    , 0 },
  { "sneak"    , "sneak"	, POS_STANDING, do_gen_tog  , 0, ADMLVL_NONE	, SCMD_SNEAK },
  { "snoop"    , "snoop"	, POS_DEAD    , do_snoop    , 1, ADMLVL_GOD	, 0 },
  { "socials"  , "socials"	, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_SOCIALS },
  { "split"    , "split"	, POS_SITTING , do_split    , 0, ADMLVL_NONE	, 0 },
  { "speak"    , "spe"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "spells"   , "spel"         , POS_DEAD    , do_spells   , 0, ADMLVL_NONE    , 0 },
  { "spellup"  , "spellup"      , POS_DEAD    , do_spellup  , 1, ADMLVL_GOD     , 0 },
  { "spontaneous", "spont"      , POS_DEAD    , do_spontaneous, 0, ADMLVL_NONE  , 0 },
  { "sponsor"  , "spons" , POS_DEAD    , do_sponsor  , 0, ADMLVL_NONE, 0 },
  { "stand"    , "st"		, POS_RESTING , do_stand    , 0, ADMLVL_NONE	, 0 },
  { "stat"     , "stat"		, POS_DEAD    , do_stat     , 1, ADMLVL_IMMORT	, 0 },
  { "statistics", "stati"       , POS_DEAD    , do_aod_new_score, 0, ADMLVL_NONE    , SCMD_SCORE_NUMBERS },
  { "stats"    , "stats"        , POS_DEAD    , do_aod_new_score, 0, ADMLVL_NONE    , SCMD_SCORE_NUMBERS },
  { "steal"    , "ste"		, POS_STANDING, do_steal    , 0, ADMLVL_NONE	, 0 },
  { "stop"     , "stop"		, POS_RESTING , do_memorize , 0, ADMLVL_NONE	, SCMD_STOP },
  { "strengthofhonor", "strengthofhonor", POS_FIGHTING, do_strength_of_honor, 0, ADMLVL_NONE, 0 },
  { "subguildchat", "subgu"     , POS_DEAD    , do_subguildchat, 0, ADMLVL_NONE , 0 },
  { "subguildwho", "subguildwho", POS_DEAD    , do_subguildwho, 0, ADMLVL_NONE  , 0 },
  { "summon"   , "sum"          , POS_RESTING , do_summon   , 0, ADMLVL_NONE    , 0 },
  { "suicide"  , "suicide"      , POS_DEAD    , do_suicide  , 0, ADMLVL_NONE    , 0 },
  { "supplyorder", "supplyo"    , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "swarmofarrows", "swarmofa" , POS_FIGHTING, do_swarmofarrows, 0, ADMLVL_NONE, 0 },
  { "switch"   , "switch"	, POS_DEAD    , do_switch   , 1, ADMLVL_GOD	, 0 },
  { "synthesize", "synth"       , POS_DEAD    , do_synthesize, 0, ADMLVL_NONE  , 0 },
  { "syslog"   , "syslog"	, POS_DEAD    , do_syslog   , 0, ADMLVL_NONE	, 0 },

  { "tell"     , "t"		, POS_DEAD    , do_tell     , 0, ADMLVL_NONE	, 0 },
  { "take"     , "ta"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "taketen"  , "taket"	, POS_RESTING , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_TAKE_TEN },
  { "tame"     , "tam"          , POS_RESTING , do_tame     , 0, ADMLVL_NONE    , 0 },
  { "tan"     , "tan"		, POS_STANDING, do_assemble , 0, ADMLVL_NONE	, SCMD_TAN },
  { "tank"     , "tank"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_TANK },
  { "tanksummon", "tanksum"     , POS_RESTING , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_SUMMON_TANK },
  { "tankmount", "tankmou"      , POS_RESTING , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_MOUNT_TANK },
  { "taste"    , "tas"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_TASTE },
  { "taunt"    , "tau"    , POS_FIGHTING , do_taunt    , 0, ADMLVL_NONE  , SCMD_TAUNT },
  { "teleport" , "tele"		, POS_DEAD    , do_teleport , 1, ADMLVL_GOD	, 0 },
  { "tedit"    , "tedit"	, POS_DEAD    , do_tedit    , 1, ADMLVL_GRGOD	, 0 },  
  { "test"	   , "test"	    , POS_DEAD    , do_test     , 0, ADMLVL_IMPL    , 0 },
  { "thatch"   , "thatch"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_THATCH },
  { "thaw"     , "thaw"		, POS_DEAD    , do_wizutil  , 1, ADMLVL_FREEZE	, SCMD_THAW },
  { "title"    , "title"	, POS_DEAD    , do_title    , 0, ADMLVL_NONE	, 0 },
  { "time"     , "time"		, POS_DEAD    , do_time     , 0, ADMLVL_NONE	, 0 },
  { "timers"   , "timers"	, POS_DEAD    , do_timers   , 0, ADMLVL_NONE	, 0 },
  { "toggle"   , "toggle"	, POS_DEAD    , do_toggle   , 0, ADMLVL_NONE	, 0 },
  { "track"    , "track"	, POS_STANDING, do_track    , 0, ADMLVL_NONE	, 0 },
  { "trackthru", "trackthru"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_IMPL	, SCMD_TRACK },
  { "train"    , "train"	, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "transfer" , "transfer"	, POS_SLEEPING, do_trans    , 1, ADMLVL_GOD	, 0 },
  { "trigedit" , "trigedit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_TRIGEDIT},
  { "trip"     , "trip"         , POS_FIGHTING, do_trip     , 0, ADMLVL_NONE    , 0 },
  { "try"      , "try"          , POS_STANDING, do_not_here , 0, ADMLVL_NONE    , 0 },
  { "turn"     , "turn"		, POS_FIGHTING, do_turn_undead, 0, ADMLVL_NONE	, 0},
  { "typo"     , "typo"		, POS_DEAD    , do_post_forums, 0, ADMLVL_NONE	, SCMD_TYPO },

  { "unlock"   , "unlock"	, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_UNLOCK },
  { "ungroup"  , "ungroup"	, POS_DEAD    , do_ungroup  , 0, ADMLVL_NONE	, 0 },
  { "undeathtouch", "undeathtouch", POS_FIGHTING, do_undeathtouch, 0, ADMLVL_NONE, 0 }, 
  { "unban"    , "unban"	, POS_DEAD    , do_unban    , 1, ADMLVL_GRGOD	, 0 },
  { "unaffect" , "unaffect"	, POS_DEAD    , do_wizutil  , 1, ADMLVL_GOD	, SCMD_UNAFFECT },
  { "uptime"   , "uptime"	, POS_DEAD    , do_date     , 1, ADMLVL_IMMORT	, SCMD_UPTIME },
  { "use"      , "use"		, POS_SITTING , do_use      , 0, ADMLVL_NONE	, SCMD_USE },
  { "users"    , "users"	, POS_DEAD    , do_users    , 1, ADMLVL_IMMORT	, 0 },

  { "value"    , "val"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "version"  , "ver"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_VERSION },
  { "vieworder", "view" 	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_VIEWORDER },
  { "visible"  , "vis"		, POS_RESTING , do_visible  , 0, ADMLVL_NONE	, 0 },
  { "vnum"     , "vnum"		, POS_DEAD    , do_vnum     , 1, ADMLVL_BUILDER	, 0 },
  { "vstat"    , "vstat"	, POS_DEAD    , do_vstat    , 1, ADMLVL_GOD	, 0 },

  { "wake"     , "wa"		, POS_SLEEPING, do_wake     , 0, ADMLVL_NONE	, 0 },
  { "wear"     , "wea"		, POS_RESTING , do_wear     , 0, ADMLVL_NONE	, 0 },
  { "weather"  , "weather"	, POS_RESTING , do_weather  , 0, ADMLVL_NONE	, 0 },
  { "weave"    , "weave"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_WEAVE },
  { "websitepassword", "websitepass", POS_DEAD   , do_websiteaccount, 0, ADMLVL_NONE, 0 },
  { "webpassword", "webpass", POS_DEAD   , do_websiteaccount, 0, ADMLVL_NONE, 0 },
  { "who"      , "who"		, POS_DEAD    , do_who      , 0, ADMLVL_NONE	, 0 },
  { "whoami"   , "whoami"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WHOAMI },
//  { "whois"    , "whois"	, POS_DEAD    , do_whois    , 0, ADMLVL_NONE	, 0 },
  { "where"    , "where"	, POS_RESTING , do_where    , 0, ADMLVL_NONE	, 0 },
  { "whirlwind", "whirlw"       , POS_FIGHTING, do_whirlwind, 0, ADMLVL_NONE    , 0 },
  { "whisper"  , "whisper"	, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_WHISPER },
  { "wield"    , "wie"		, POS_RESTING , do_wield    , 0, ADMLVL_NONE	, 0 },
  { "wildshape", "wildshape"    , POS_RESTING , do_wildshape, 0, ADMLVL_NONE    , 0 },
  { "wimpy"    , "wimpy"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_WIMPY },
  { "wish"     , "wish"         , POS_DEAD    , do_wish, 0, ADMLVL_NONE    , 0 },
  { "wishlist" , "wishl"    , POS_DEAD    , do_wishlist , 0, ADMLVL_NONE    , 0 },
  { "withdraw" , "withdraw"	, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "wiznet"   , "wiz"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_NONE	, 0 },
  { ";"        , ";"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_NONE	, 0 },
  { "wizhelp"  , "wizhelp"	, POS_SLEEPING, do_commands , 1, ADMLVL_IMMORT	, SCMD_WIZHELP },
  { "wizlist"  , "wizlist"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "wizlock"  , "wizlock"	, POS_DEAD    , do_wizlock  , 1, ADMLVL_IMPL	, 0 },
  { "wizupdate", "wizupdate"    , POS_DEAD    , do_wizupdate, 1, ADMLVL_GRGOD	, 0 },
  { "write"    , "write"	, POS_STANDING, do_write    , 0, ADMLVL_NONE	, 0 },


  { "zreset"   , "zreset"	, POS_DEAD    , do_zreset   , 0, ADMLVL_BUILDER	, 0 },
  { "zedit"    , "zedit"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_ZEDIT },
  { "zlist"    , "zlist"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_ZLIST },

  /* DG trigger commands */
  { "attach"   , "attach"	, POS_DEAD    , do_attach   , 1, ADMLVL_BUILDER	, 0 },
  { "detach"   , "detach"	, POS_DEAD    , do_detach   , 1, ADMLVL_BUILDER	, 0 },
  { "tlist"    , "tlist"	, POS_DEAD    , do_oasis    , 1, ADMLVL_BUILDER	, SCMD_OASIS_TLIST },
  { "tstat"    , "tstat"	, POS_DEAD    , do_tstat    , 1, ADMLVL_BUILDER	, 0 },
  { "masound"  , "masound"	, POS_DEAD    , do_masound  , -1, ADMLVL_NONE	, 0 },
  { "mkill"    , "mkill"	, POS_STANDING, do_mkill    , -1, ADMLVL_NONE	, 0 },
  { "mjunk"    , "mjunk"	, POS_SITTING , do_mjunk    , -1, ADMLVL_NONE	, 0 },
  { "mdamage"  , "mdamage"	, POS_DEAD    , do_mdamage  , -1, ADMLVL_NONE	, 0 },
  { "mdoor"    , "mdoor"	, POS_DEAD    , do_mdoor    , -1, ADMLVL_NONE	, 0 },
  { "mecho"    , "mecho"	, POS_DEAD    , do_mecho    , -1, ADMLVL_NONE	, 0 },
  { "mechoaround", "mechoaround", POS_DEAD    , do_mechoaround, -1, ADMLVL_NONE	, 0 },
  { "msend"    , "msend"	, POS_DEAD    , do_msend    , -1, ADMLVL_NONE	, 0 },
  { "mload"    , "mload"	, POS_DEAD    , do_mload    , -1, ADMLVL_NONE	, 0 },
  { "mpurge"   , "mpurge"	, POS_DEAD    , do_mpurge   , -1, ADMLVL_NONE	, 0 },
  { "mgoto"    , "mgoto"	, POS_DEAD    , do_mgoto    , -1, ADMLVL_NONE	, 0 },
  { "mat"      , "mat"		, POS_DEAD    , do_mat      , -1, ADMLVL_NONE	, 0 },
  { "mteleport", "mteleport"	, POS_DEAD    , do_mteleport, -1, ADMLVL_NONE	, 0 },
  { "mforce"   , "mforce"	, POS_DEAD    , do_mforce   , -1, ADMLVL_NONE	, 0 },
  { "mhunt"    , "mhunt"	, POS_DEAD    , do_mhunt    , -1, ADMLVL_NONE	, 0 },
  { "mremember", "mremember"	, POS_DEAD    , do_mremember, -1, ADMLVL_NONE	, 0 },
  { "mforget"  , "mforget"	, POS_DEAD    , do_mforget  , -1, ADMLVL_NONE	, 0 },
  { "mtransform", "mtransform"	, POS_DEAD    , do_mtransform, -1, ADMLVL_NONE	, 0 },
  { "mzoneecho", "mzoneecho"	, POS_DEAD    , do_mzoneecho, -1, ADMLVL_NONE	, 0 },
  { "vdelete"  , "vdelete"	, POS_DEAD    , do_vdelete  , 0, ADMLVL_BUILDER	, 0 },
  { "mfollow"  , "mfollow"	, POS_DEAD    , do_mfollow  , -1, ADMLVL_NONE	, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, ADMLVL_NONE	, 0 } };	/* this must be last */

struct command_info *complete_cmd_info;

const char *fill[] =
{
  "in",
  "inside",
  "into",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/*
 * this is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd = 0, length = 0;
  char *line;
  char arg[MAX_INPUT_LENGTH]={'\0'};
  struct char_data *k = NULL, *temp = NULL;
  ubyte found = FALSE;
  struct follow_type *f = NULL;
  int save = 0, dc = 0;

  /* REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE); */

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. 
   */

  /* otherwise, find the command */
  {
  int cont;                                            /* continue the command checks */
  cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
  if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
  if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
  if (cont) return;                                    /* yes, command trigger took over */
  }
  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++) {
    if (!strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level &&
          GET_ADMLEVEL(ch) >= complete_cmd_info[cmd].minimum_admlevel)
        break;
  }

  if (*complete_cmd_info[cmd].command == '\n') {
   // if (command_unique_exit(ch, arg))
   //   return;
   // else
    send_to_char(ch, "Huh?!?\r\n");
  }
  else if (check_disabled(&complete_cmd_info[cmd]))    /* is it disabled? */
      send_to_char(ch, "This command has been temporarily disabled.\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n"); 

  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_admlevel >= ADMLVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position) {
    switch (GET_POS(ch)) {
    case POS_DEAD:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_STUNNED:
      if (FIGHTING(ch)) {
        save = d20 + get_saving_throw_value(ch, SAVING_FORTITUDE);
        dc = 10;
        if (GET_HIT(ch) < 0)
          dc -= GET_HIT(ch);
        if ((save + 10) >= dc) {
          GET_HIT(ch) = 1;
          update_pos(ch);
          send_to_char(ch, "You stablize and regain consciousness.\r\n");
          act("$n stablizes and regains consciousness.", FALSE, ch, 0, 0, TO_ROOM);
          return;
        } else if ((save + 5) >= dc) {
          GET_HIT(ch) = 0;
          update_pos(ch);
          send_to_char(ch, "You stablize but remain unconscious.\r\n");
          act("$n stablizes but remains unconscious.", FALSE, ch, 0, 0, TO_ROOM);
          return;
        } else if (save >= dc) {
          send_to_char(ch, "Your condition neither worsens nor improves.\r\n");
          return;
        } else {
          send_to_char(ch, "You slide a little bit closer to death.\r\n");
          damage(ch, ch, 1, TYPE_SUFFERING, 0, -1, 0, TYPE_SUFFERING, 1);
        }
      }
      break;
    }
      
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      for (k = world[IN_ROOM(ch)].people; k; k = temp) {
        temp = k->next_in_room;
        if (FIGHTING(k) == ch) {
          found = TRUE;          
        }
      }
      if (!found) {
        stop_fighting(ch);
        GET_POS(ch) = POS_STANDING;
        update_pos(ch);
        for (f = ch->followers; f; f = f->next) {
          found = FALSE;
          for (k = world[IN_ROOM(f->follower)].people; k; k = temp) {
            temp = k->next_in_room;
            if (FIGHTING(k) == f->follower) {
              found = TRUE;          
            }
          }
          if (!found) {
            stop_fighting(f->follower);
            GET_POS(f->follower) = POS_STANDING;
            update_pos(f->follower);
          }
        }
        command_interpreter(ch, argument);
        return;
      }
      else {
        send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      }
      break;
    }
  } else if (no_specials || !special(ch, cmd, line)) {
    ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
    return;
  }
  /* it's not a 'real' command, so it's a social */


}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH]={'\0'};
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char(ch, "Alias added.\r\n");
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH]={'\0'}, buf[MAX_RAW_INPUT_LENGTH]={'\0'};	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH]={'\0'}, *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  int i = 0, l = 0;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int search_block_2(char *arg, char **list, int exact)
{
  int i = 0, l = 0 ;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}


int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  if you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, true) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, true) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003.
 */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

void display_name_policy(struct descriptor_data *d) {

write_to_output(d,
"\r\n"
"Before confirming this name, please ensure it follows our name guidlines.\r\n"
"\r\n"
"1) No modern names.  If a name wasn't in popular use in medeival times\r\n"
"then it is not acceptable.  For example Alexander would be acceptable\r\n"
"whereas Betsy would not be.\r\n"
"\r\n"
"2) No names from existing fictional works.  That includes forgotten realms\r\n"
"characters.  Do not use names from novels, movies, comics, or what have you.\r\n"
"Please be original with your names.\r\n"
"\r\n"
"3) No modern day nouns as names.  It's ok to call your character something\r\n"
"like Scimitar or Knife, because these are possible nicknames people from\r\n"
"the underworld might call themselves or each other.  However in such circumstances\r\n"
"it is expected for you to invent a real name for your character following\r\n"
"these guidelines for role playing purposes.  It would not be acceptable to\r\n"
"call your character Bazooka or Bomber because Bazookas and Bombs don't\r\n"
"exist in a medeival setting.\r\n"
"\r\n"
"4) No compound names.  Depending on your race, please do not use compound\r\n"
"names, such as madkiller.  Something like firestar might be acceptable for\r\n"
"a wild elf or a barbarian, because both fire and star are common things in\r\n"
"a medeival fantasy world.  It is thinkable that someone from these cultures\r\n"
"might name their child this.  Madkiller on the other hand is something\r\n"
"no one would name their child, and is unlikely someone would name themselves\r\n"
"either.\r\n"
"\r\n"
"In general we ask that you simply make an attempt to use a name that is\r\n"
"original and that make sense in a fantasy medeival world.  If you create\r\n"
"a character with an invalid name you will be asked to change it.  If you\r\n"
"comply with cooperation we will reimburse your character's gear and\r\n"
"levels.  Otherwise we will be forced to unfortunately, delete you.\r\n"
"\r\n"
"If you need help, a good name generator can be found at:\r\n"
"@Whttp://www.seventhsanctum.com/index-name.php@n\r\n"
"\r\n"
"Thanks for your cooperation in this matter.  It will help enhance the\r\n"
"role play of the game and the fun for all involved.\r\n"
"\r\n");
}

void display_race_confirm(struct descriptor_data *d)
{
  struct help_index_element *this_help;
  int x;
  char help_string[100]={'\0'};

    sprintf(help_string, "%s", race_list[GET_RACE(d->character)].name);
    for (x = 0; x < strlen(help_string); x++)
      if (help_string[x] == ' ')
        help_string[x] = '-';
    this_help = find_help(help_string);

    if (this_help)
      write_to_output(d, "\r\n@W%s@n\r\n%s\r\n", this_help->keywords, this_help->entry);


    write_to_output(d, "\r\n\r\nWould you like to keep this race as your own and continue? ( Y | N ) ");

}

void display_races(struct descriptor_data *d)
{
write_to_output(d, 
"Please select your race from the list below.  You may be brought to\r\n"
"an extended race menu filled with subraces depending on the race you\r\n"
"choose.  Once you choose a race you will be presented with a description\r\n"
"of that race and ask if you would like to accept that race as your own\r\n"
"or if you would like to select again.\r\n"
"\r\n"
"H) Human (The Most Popuated of all Races)\r\n"
"E) Elf (Arrogant yet Noble Forest Dwellers)\r\n"
"D) Dwarf (Gruff and Rugged Mountain Dwellers)\r\n"
"G) Gnome (Fast Speaking Tinkerers and Inventors)\r\n"
"K) Kender (Cheerful and Ever-Optomistic \"Handlers\")\r\n"
"M) Minotaur (Warlike and Seafaring Half-Man Half-Bull Creatures)\r\n"
"\r\n"
"Please select a race above: ");

}

void display_human_races(struct descriptor_data *d)
{
write_to_output(d, 
"Humans are the most plentiful of all races on the face of Ansalon.  Their short lifespans\r\n"
"are overcome with a zeal for life and ambition unequalled by perhaps any other race.\r\n"
"They come in all shapes and sizes, skin tones, hair and eye colors.  They live to be\r\n"
"about 75 years on average, during which they usually manage to accomplish as much or\r\n"
"more than the other, longer lived races, in their own extended lifespans.  As you choose\r\n"
"a human subraces you are actually choosing their home nation and/or way of life.\r\n"
"\r\n"
"S) Solamnic (Knights and Chivalrous Folk)\r\n"
"E) Ergothian (Culture-Rich Folk and Sea Farers)\r\n"
"I) Istarian (Citizens of the Dominant Empire, Land of the Kingpriest)\r\n"
"B) Baliforite (Sea Farers and Farmers)\r\n"
"K) Kharolisian (Rugged Mountain Dwellers)\r\n"
"N) Nordmaarian (Tropical Island Folk)\r\n"
"P) Plains Barbarian (Plains Nomads)\r\n"
"M) Mountain Barbarian (Mountain Nomads)\r\n"
"D) Desert Barbarian (Desert Nomads)\r\n"
"F) Ice Folk (Glacial Island Folk)\r\n"
"\r\n"
"Please select a human subrace: ");

}

void display_elf_races(struct descriptor_data *d)
{
write_to_output(d, 
"Elves are one of the first races to set foot on Krynn.  Championed by the Gods of Light,\r\n"
"they have ever been guardians of what is right and true.  However, their isolationist\r\n"
"ways often prevent them from making a great impact on the worlds outside their own.\r\n"
"They stand about 5 foot to 5 foot and a half in height, are slender and have varying\r\n"
"skin tones and hair colors depending on their home nation.  Elves are the longest living\r\n"
"of all of the mortal humanoid races.\r\n"
"\r\n"
"S) Silvanesti Elves (High Elves, Very Isolated from other Races)\r\n"
"Q) Qualinesti Elves (Normal Elves, Trades with Non-Elves but still Arrogant)\r\n"
"\r\n"
"Please select an elven subrace: ");

}

void display_dwarf_races(struct descriptor_data *d)
{
write_to_output(d, 
"Dwarves are the chosen race of Reorx, God of Craftsmanship and Creation.  They are a\r\n"
"short, bearded people with gruff and stubborn mannerisms.  Even the females are known\r\n"
"to sport beards and indeed the beard is a dwarf's most prized possession.  Dwarves\r\n"
"are skilled craftsmen, and most live underground in huge subterranean fortresses and\r\n"
"cities.  Dwarves group themselves together by clan, and each clan has its own strengths\r\n"
"and weaknesses.\r\n"
"\r\n"
"H) Hylar (High Dwarves, the Ruling Class)\r\n"
"D) Daewar (Fervently Religious, the Priesthood Class)\r\n"
"K) Klar (Half-Crazed and Eccentric Warrior Class)\r\n"
"T) Theiwar (Albino Deep Dwarves, Sneaky and Deceiving)\r\n"
"R) Daergar (Deep Dwarves, Mining Class)\r\n"
"W) Dewar (Deep Dwarves, Evil and Treacherous)\r\n"
"Z) Zhakar (Dwarves of the Zhakar Region, Afflicted with a Leperous Curse)\r\n"
"N) Neidar (Hill Dwarves, They Live Above Ground and Trade with Non-Dwarves Often)\r\n"
"A) Aghar (Gully Dwarves, Stupid and Cowardly, They are Smaller and Live off Refuse)\r\n"
"\r\n"
"Please select a dwarven subrace: ");

}

void display_classes(struct descriptor_data *d)
{
  send_to_char(d->character, "\r\n");
  send_to_char(d->character, "Please select a class to begin your character's life as.  This is his first class, though\r\n");
  send_to_char(d->character, "once you enter the game and begin to gain levels you may gain levels some of the other\r\n");
  send_to_char(d->character, "classes you see here as well as some advanced classes, of which more can be learned about\r\n");
  send_to_char(d->character, "in the game.\r\n");
  send_to_char(d->character, "\r\n");
  send_to_char(d->character, "Your class will determine the number of skill points you receive and how much each skill\r\n");
  send_to_char(d->character, "costs to learn, as well as your hit points, attack bonus, what weapons and armor you\r\n");
  send_to_char(d->character, "start off knowing how to use, and what special abilities you will start with.  As you\r\n");
  send_to_char(d->character, "progress in each class you will learn new abilities pertaining to that class.  Or you\r\n");
  send_to_char(d->character, "may split yourlevels among multiple class and be able to do many different things, at\r\n");
  send_to_char(d->character, "a lesser degree of ability than if you specialized.\r\n");
  send_to_char(d->character, "\r\n");
  send_to_char(d->character, "To select a class type the class name.  To learn more about a class type help followed\r\n");
  send_to_char(d->character, "by the class name.\r\n");
  send_to_char(d->character, "\r\n");
  send_to_char(d->character, "C@Wlasses: paladin fighter rogue mage cleric monk barbarian@n\r\n");
  send_to_char(d->character, "\r\n");
  send_to_char(d->character, "What class would you like to start as? ");

}

void display_races_help(struct descriptor_data *d)
{
  int x;

  send_to_char(d->character, "\r\n@YRace HELP menu:\r\n@G---------------------------\r\n@n");
  for (x = 0; x < NUM_RACES; x++)
    if (race_ok_gender[(int)GET_SEX(d->character)][x])
      send_to_char(d->character, "%2d) %s\r\n", x+1, pc_race_types[x]);

      send_to_char(d->character, "\n@BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Race #: @n");
}

void display_classes_help(struct descriptor_data *d)
{
  int x;

  send_to_char(d->character, "\r\n@YClass HELP menu:\r\n@G--------------------------\r\n@n");
  for (x = 0; x < NUM_CLASSES; x++)
    if (class_ok_general(d->character, x))
      send_to_char(d->character, "%2d) %s\r\n", x+1, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[x]);

      send_to_char(d->character, "\n@BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Class #: @n");
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/*
 * return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  if (arg2 != temp)
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* This function needs to die. */
int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg) && *arg != '-')
      return (1);

  if (!i)
    return (1);

  return (0);
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;
  char query[MAX_INPUT_LENGTH]={'\0'};

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      pref_temp=GET_PREF(k->character);
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */
      pref_temp=GET_PREF(k->character);

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  /* stupid-case rule for setting hostname on char:
	any time you set the char's pref .. */
  

  if (!target)
    return 0;

/*
  if (!target) {
    GET_HOST(d->character) = strdup(d->host); 
    GET_PREF(d->character)=  get_new_pref();
    return 0;
  } else {
//
//    if(GET_HOST(target)) {
//      free(GET_HOST(target));
//    }
    GET_HOST(target) = strdup(d->host);
    GET_PREF(target)=pref_temp;
    add_llog_entry(target,LAST_RECONNECT);
  }
*/
  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->timer = 0;
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  if (!AFF_FLAGGED(d->character, AFF_WILD_SHAPE))
    REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_DISGUISED);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:

    // Open mysql connection
    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
      log("Cannot connect to mysql database in enter player game.");
    }

    if (!d->copyover && CONFIG_DFLT_PORT == 9080) {
      ch = d->character;
     sprintf(query, "INSERT INTO player_logins (player_name, last_login, account_name) VALUES('%s',NOW(), '%s')", GET_NAME(ch), ch->desc ? (ch->desc->account ?ch->desc->account->name : "") : "");
      if (mysql_query(conn, query)) {
         log("Cannot set last_login for %s in player_logins", GET_NAME(ch));
      }
    }

    mysql_close(conn);

    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", true, d->character, 0, 0, TO_ROOM);
    if (!PRF_FLAGGED(d->character, PRF_ANONYMOUS))
      mudlog(NRM, MAX(ADMLVL_NONE, GET_INVIS_LEV(d->character)), true, "%s has reconnected.", GET_NAME(d->character));
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), true, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	true, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), true,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), true, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
    int load_result;
    IDXTYPE load_room = CONFIG_MORTAL_START;
    struct char_data *check;
    struct clan_type *cptr;
    struct char_data *ch = d->character;

  REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_FIGHT_SPAM);
  if (d->character->combat_output == OUTPUT_SPARSE)
    d->character->combat_output = OUTPUT_NORMAL;

  if (GET_SKILL_BASE(ch, 809) > GET_SKILL_BASE(ch, 819)) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += GET_SKILL_BASE(ch, 819);
    GET_SKILL_BASE(ch, 819) = 0;
  }

  if (GET_SKILL_BASE(ch, 819) > GET_SKILL_BASE(ch, 809)) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += GET_SKILL_BASE(ch, 809);
    SET_SKILL(ch, 809, GET_SKILL_BASE(ch, 819));
    GET_SKILL_BASE(ch, 819) = 0;
  }

  if (GET_SKILL_BASE(ch, 816) > GET_SKILL_BASE(ch, 828)) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += GET_SKILL_BASE(ch, 828);
    GET_SKILL_BASE(ch, 828) = 0;
  }

  if (GET_SKILL_BASE(ch, 828) > GET_SKILL_BASE(ch, 816)) {
    GET_PRACTICES(ch, GET_CLASS(ch)) += GET_SKILL_BASE(ch, 816);
    SET_SKILL(ch, 816, GET_SKILL_BASE(ch, 828));
    GET_SKILL_BASE(ch, 828) = 0;
  }

  if (ch->player_specials->mount > 0 && ch->player_specials->mount_num > 0 && 
      (get_skill_value(ch, SKILL_RIDE) < pet_list[ch->player_specials->mount_num].level ||
       get_skill_value(ch, SKILL_HANDLE_ANIMAL) < pet_list[ch->player_specials->mount_num].level)) {
    ch->player_specials->mount = 0;
    GET_BANK_GOLD(ch) += pet_list[ch->player_specials->mount_num].level * 1000;
    send_to_char(ch, "@l@RYou have had your mount taken as it is too high for your ride and handle animal skills.  You were reimbursed %d coins in your bank.\r\n",
                 pet_list[ch->player_specials->mount_num].level * 1000);
    if (ch->player_specials->mount_num > 0)
      free_mount(ch);
  }

  if (!GET_ARTISAN_TYPE(ch)) {
    reset_artisan_experience(ch);
  }

  // MySQL Save

  char query[MAX_INPUT_LENGTH]={'\0'};

  // Open mysql connection
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
    log("Cannot connect to mysql database in enter player game.");
  }

  if (!d->copyover && CONFIG_DFLT_PORT == 9080) {
     sprintf(query, "INSERT INTO player_logins (player_name, last_login, account_name) VALUES('%s',NOW(), '%s')", GET_NAME(ch), ch->desc ? (ch->desc->account ? ch->desc->account->name : "") : "");
    if (mysql_query(conn, query)) {
       log("Cannot set last_login for %s in player_logins", GET_NAME(ch));
    }  
  }

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  struct char_data *i = ch;

  if (i->desc && i->desc->account && !IS_NPC(i) && GET_ADMLEVEL(i) == 0 && (CONFIG_DFLT_PORT == 9080)) {

    char account_name[255]={'\0'};

    sprintf(account_name, "%s", i->desc->account->name);

    sprintf(query, "SELECT forum_exp FROM player_forum_data WHERE account='%s'", UNCAP(account_name));
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL) {
      if ((row = mysql_fetch_row(res)) != NULL) {
        if (atoi(row[0]) > 0) {
          i->desc->account->experience += atoi(row[0]);
          send_to_char(i, "You have gained %d account experience for your web forum posts.\r\n", atoi(row[0]));
        }
      }
    }
    mysql_free_result(res);    

    sprintf(query, "UPDATE player_forum_data SET forum_exp=0 WHERE account='%s'", UNCAP(account_name));
    if (mysql_query(conn, query)) {
       log("Cannot set forum_exp to 0 for %s from player_extra_data table", account_name);
    }
   
    sprintf(query, "DELETE FROM player_data WHERE name = '%s'", GET_NAME(i));
    if (mysql_query(conn, query)) {
       log("Cannot set delete %s from player_data table", GET_NAME(i));
    }

    sprintf(query, "SELECT extra_account_exp FROM player_extra_data WHERE name='%s'", GET_NAME(i));
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL) {
      if ((row = mysql_fetch_row(res)) != NULL) {
        if (atoi(row[0]) > GET_EXTRA_ACC_EXP(i)) {
        int acc_exp = atoi(row[0]) - GET_EXTRA_ACC_EXP(i);
        if (i->desc && i->desc->account)
          i->desc->account->experience += acc_exp * 3;
          send_to_char(i, "You have gained %d account experience for modifying your character background and personality online!\r\n", acc_exp);
          int k = 0;
          for (k = 0; k < ((acc_exp) / 500); k++)
            gain_exp(i, level_exp(GET_CLASS_LEVEL(i) + k, GET_REAL_RACE(i)));
          gain_exp(i, level_exp(GET_CLASS_LEVEL(i) + k, GET_REAL_RACE(i)) * (acc_exp % 500) / 500);
          GET_EXTRA_ACC_EXP(i) = atoi(row[0]);
        }
      }
    }      
    mysql_free_result(res);

  }

      convert_coins(d->character);
    
      reset_char(d->character);
      read_aliases(d->character);

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_ADMLEVEL(d->character);
      if (!AFF_FLAGGED(d->character, AFF_WILD_SHAPE))
        REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_DISGUISED);

      /* If you have the copyover patch installed, the following goes in enter_player_game as well */
      /* Check for new clans for leader */
      if (GET_CLAN(d->character) == PFDEF_CLAN) {
        for (cptr = clan_info; cptr; cptr = cptr->next) {
          if (!strcmp(GET_NAME(d->character), cptr->leadersname))
            GET_CLAN(d->character) = cptr->number;
        }
      }
                                                                                                         
      /* can't do an 'else' here, cuz they might have a clan now. */
      if (GET_CLAN(d->character) != PFDEF_CLAN) {
        /* Now check to see if person's clan still exists */
        for (cptr = clan_info; cptr && cptr->number != GET_CLAN(d->character); cptr = cptr->next);
                                                                                                         
        if (cptr == NULL) {  /* Clan no longer exists */
          GET_CLAN(d->character) = PFDEF_CLAN;
          GET_CLAN_RANK(d->character) = PFDEF_CLANRANK;
          GET_HOME(d->character) = 1;
        } else {  /* Was there a change of leadership? */
          if (!strcmp(GET_NAME(d->character), cptr->leadersname))
            GET_CLAN_RANK(d->character) = CLAN_LEADER;
        }
      }

      /*
       * We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE.
       */

      load_room = real_room(GET_LOADROOM(d->character));

//      send_to_char(ch, "LOADROOM: %d, REAL ROOM: %d\r\n", GET_LOADROOM(ch), load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_ADMLEVEL(d->character))
	  load_room = real_room(CONFIG_IMMORTAL_START);
	else
	  load_room = real_room(CONFIG_MORTAL_START);
      }

      if (GET_LEVEL(d->character) < 1 && GET_ADMLEVEL(d->character) > 0)
        GET_CLASS_LEVEL(d->character) = 1;

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = real_room(CONFIG_FROZEN_START);

      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character, FALSE);
      if (d->character->player_specials->host) {
        free(d->character->player_specials->host);
        d->character->player_specials->host = NULL;
      }
      d->character->player_specials->host = strdup(d->host);
      GET_ID(d->character) = GET_IDNUM(d->character);
      /* find_char helper */
      add_to_lookup_table(GET_ID(d->character), (void *)d->character);
      read_saved_vars(d->character);
      load_char_pets(d->character);
      for (check = character_list; check; check = check->next)
        if (!check->master && IS_NPC(check) && check->master_id == GET_IDNUM(d->character) &&
            AFF_FLAGGED(check, AFF_CHARM) && !circle_follow(check, d->character))
          add_follower(check, d->character);
      save_char(d->character);

    //combine_accounts();


  if (CONFIG_DFLT_PORT == 9080 || CONFIG_DFLT_PORT == 6070) {

    char query[200]={'\0'};

    sprintf(query, "SELECT a.gameaccount, b.account_exp, b.forum_user_id FROM jos_chronoforms_user_details a, mud_web_info b WHERE a.cf_user_id = "
                   "b.forum_user_id AND a.gameaccount = '%s'", d->account ? d->account->name : "Unused");
    mysql_query(conn, query);

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    MYSQL_RES *res2 = NULL;
    MYSQL_ROW row2 = NULL;
    char gameaccount[255]={'\0'};
    int accExp = 0;
    int userId = 0;

    res = mysql_use_result(conn);

    if (res != NULL) {

      while ((row = mysql_fetch_row(res)) != NULL) {
      sprintf(gameaccount, "%s", row[0]);
      accExp = atoi(row[1]);
      userId = atoi(row[2]);
      mysql_free_result(res);
      if (!strcmp(gameaccount, d->account->name)) {
          if (accExp > 0) {
            if (d->account) {
              sprintf(query, "SELECT username FROM jos_users WHERE id = '%d'", userId);
              mysql_query(conn, query);
              res2 = mysql_use_result(conn);
              if (res2 != NULL) {
                while ((row2 = mysql_fetch_row(res2)) != NULL) {
                  if (d->account->websiteAccount && !strcmp(d->account->websiteAccount, row2[0])) {
                    mysql_free_result(res2);
                    d->account->experience += accExp;
                    sprintf(query, "UPDATE mud_web_info SET account_exp = '0' WHERE forum_user_id = '%d'", userId);
                    mysql_query(conn, query);
                    send_to_char(d->character, "@l@W\r\nYou have been awarded with %d account experience for your activity on the forums.\r\n\r\n", accExp);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  mysql_close(conn);

  note_display_unread(d->character);

MXPSendTag( d, "<VERSION>" ); 

  return load_result;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  struct help_index_element *this_help;
  char arg1[50]={'\0'}, arg2[50]={'\0'}, buf[100]={'\0'};
  char argument[MAX_INPUT_LENGTH]={'\0'};
  sh_int count = 0;
  int i = 0, j = 0;
  int load_result = 0;	/* Overloaded variable */
  int player_i = 0, total = 0;
	char *tmpdesc;
  byte class = 0;
  byte found = FALSE;
  struct char_data *ch = d->character;
  char query[MAX_INPUT_LENGTH]={'\0'};
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  /* OasisOLC states */
  struct 
  {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_IEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_CEDIT, cedit_parse },
    { CON_AEDIT, aedit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_ASSEDIT, assedit_parse },
    { CON_GEDIT, gedit_parse },
    { CON_LEVELUP, levelup_parse },
    { CON_HEDIT, hedit_parse },
    { CON_QEDIT, qedit_parse },
    { CON_CLANEDIT, clanedit_parse },
	{ CON_PETSET, petset_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
      CREATE(d->character->levelup, struct level_data, 1);
    }

  /*
   * Quick check for the OLC states.
   */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {

  case CON_ACCOUNT_NAME:
    if (d->account == NULL) {
      CREATE(d->account, struct account_data, 1);
      d->account->name = NULL;
      for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
        d->account->character_names[i] = NULL;

    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      char buf[MAX_INPUT_LENGTH]={'\0'}, tmp_name[MAX_INPUT_LENGTH]={'\0'};

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "@RInvalid account name, please try another@W.@n\r\n@YName@W:@n ");
	return;
      }

      if ((load_account(tmp_name, d->account)) > -1) {
	if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "@RInvalid account name, please try another@W.@n\r\n@YName@W:@n ");
	}
        write_to_output(d, "@YPassword@W:@n ");
        ProtocolNoEcho( d, true );
        d->idle_tics = 0;
        STATE(d) = CON_PASSWORD;
        ProtocolNoEcho( d, true );
      }
      else {
	if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "@RInvalid account name, please try another@W.@n\r\n@YName@W:@n ");
	}
	CREATE(d->account->name, char, strlen(tmp_name) + 1);
	strcpy(d->account->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

        write_to_output(d, "@YDid I get that right@W,@C %s @W(@MY@W/@MN@W)?@n", tmp_name);
	STATE(d) = CON_ACCOUNT_NAME_CNFRM;
      }
    }
    break;

  case CON_ACCOUNT_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, ADMLVL_GOD, true, "Request for new account %s denied from [%s] (siteban)", d->account->name, d->host);
	write_to_output(d, "Your site (domain/IP address) has been denied access from making new characters.\r\nPlease contact"
			   "seraphime@d20mud.com if you feel this should not apply to you.\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	write_to_output(d, "The game is currently closed to making new account.  Please check our website at "
	                   "http://forgottenrealms.d20mud.com/\r\nfor more information.\r\n");
	mudlog(NRM, ADMLVL_GOD, true, "Request for new account %s denied from [%s] (wizlock)", d->account->name, d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      write_to_output(d, "@YPassword@W:@n ");
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "@ROkay, what IS it, then@W?@n ");
      free(d->account->name);
      d->account->name = NULL;
      STATE(d) = CON_ACCOUNT_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;


  case CON_ACCOUNT_MENU:
    ProtocolNoEcho( d, false );

    d->character = NULL;

    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
      CREATE(d->character->levelup, struct level_data, 1);

    }

    if (!strcmp(arg, "create")) {
      // create a new character
      write_to_output(d, "What will your character be called? Name: ");
      STATE(d) = CON_GET_NAME;
      return;
    }
    else {
      if (atoi(arg) < 1 || atoi(arg) > (MAX_CHARS_PER_ACCOUNT)) {
        write_to_output(d, "The number must be between 1 and %d.\r\n", MAX_CHARS_PER_ACCOUNT);
        return;
      }
      if (d->account->character_names[atoi(arg) - 1] == NULL) {
        SEND_TO_Q("That character doesn't exist.  Please choose another.  Your Choice: ", d);
        return;
      }
      if ((player_i = load_char(d->account->character_names[atoi(arg) - 1], d->character)) > -1) {
        GET_PFILEPOS(d->character) = player_i;
	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
          show_account_menu(d);
          SEND_TO_Q("This character has been deleted.  Please speak to an immortal about having the deleted flag removed.\r\n", d);
          return;
	} else {
            GET_PFILEPOS(d->character) = player_i;
            if (GET_LEVEL(d->character) < circle_restrict) {
            write_to_output(d, "The game is temporarily open for staff members only.  Please refer to the website for more "
			   "intormation.\r\n@Whttp://forgottenrealms.d20mud.com/\r\n");
            STATE(d) = CON_CLOSE;
            mudlog(NRM, ADMLVL_GOD, true, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
            return;
          }

	  /* undo it just in case they are set */
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
          if (!AFF_FLAGGED(d->character, AFF_WILD_SHAPE))
            REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_DISGUISED);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
	  d->idle_tics = 0;
          SEND_TO_Q("Character loaded.  Press enter to continue.\r\n", d);
          /* check and make sure no other copies of this player are logged in */
          if (perform_dupe_check(d))
	    return;

          if (GET_ADMLEVEL(d->character))
            write_to_output(d, "%s", imotd);
          else
            write_to_output(d, "%s", motd);

          if (GET_INVIS_LEV(d->character)) 
            mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), true, 
            "%s [%s] has connected. (invis %d)", GET_NAME(d->character), d->host, 
            GET_INVIS_LEV(d->character));
          else {
            if (!PRF_FLAGGED(d->character, PRF_ANONYMOUS))
              mudlog(BRF, ADMLVL_NONE, true, "%s has connected.", GET_NAME(d->character));
            mudlog(BRF, ADMLVL_IMMORT, true, "%s [%s] has connected.", GET_NAME(d->character), d->host);
          }
          STATE(d) = CON_RMOTD;
	}
      }
    }

    break;

  case CON_GET_NAME:		/* wait for input of name */
    if (!*arg) {
      SEND_TO_Q("Please provide a name for your character.  Name: ", d);
      return;
    }
    else {
      char buf[MAX_INPUT_LENGTH]={'\0'}, tmp_name[MAX_INPUT_LENGTH]={'\0'};

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "@RInvalid name, please try another@W.@n\r\n@YName@W:@n ");
	return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1) {

        GET_PFILEPOS(d->character) = player_i;

        if (PLR_FLAGGED(d->character, PLR_DELETED)) {

          /* make sure old files are removed so the new player doesn't get
             the deleted player's equipment (this should probably be a
             stock behavior)
          */
          if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
            remove_player(player_i);

          /* We get a false positive from the original deleted character. */
          free_char(d->character);
          /* Check for multiple creations... */
          if (!Valid_Name(tmp_name)) {
            write_to_output(d, "@YInvalid name@n, please try @Canother.@n\r\nName: ");
            return;
          }
          CREATE(d->character, struct char_data, 1);
          clear_char(d->character);
          CREATE(d->character->player_specials, struct player_special_data, 1);
	  CREATE(d->character->levelup, struct level_data, 1);

          d->character->desc = d;
          CREATE(d->character->name, char, strlen(tmp_name) + 1);
          strcpy(d->character->name, CAP(tmp_name));    /* strcpy: OK (size checked above) */
          GET_PFILEPOS(d->character) = player_i;

          write_to_output(d, "@YDid I get that right@W,@C %s @W(@MY@W/@MN@W)?@n", tmp_name);
          STATE(d) = CON_NAME_CNFRM;
          return;
        }
        else {
          SEND_TO_Q("That character already exists.  Please choose another name.  Name: ", d);
          return;
        }
      } else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "@RInvalid name, please try another@W.@n\r\n@YName@W:@n ");
	}
	CREATE(d->character->name, char, strlen(tmp_name) + 1);
	strcpy(d->character->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

          display_name_policy(d);
	  write_to_output(d, "@YDid I get that right@W,@C %s @W(@MY@W/@MN@W)?@n", tmp_name);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;

  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }

    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, ADMLVL_GOD, true, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
	write_to_output(d, "Your site (domain/IP address) has been denied access from making new characters.\r\nPlease contact"
			   "seraphime@d20mud.com if you feel this should not apply to you.\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	write_to_output(d, "The game is currently closed to making new characters.  Please check our website at "
	                   "http://forgottenrealms.d20mud.com/\r\nfor more information.\r\n");
	mudlog(NRM, ADMLVL_GOD, true, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      write_to_output(d, "@YGender (Male / Female) @W:@n ");
      STATE(d) = CON_QSEX;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "@ROkay, what IS it, then@W?@n ");
      free(d->character->name);
      d->character->name = NULL;
      STATE(d) = CON_GET_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

    ProtocolNoEcho( d, false );    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, d->account->password), d->account->password, MAX_PWD_LENGTH)) {
	mudlog(BRF, ADMLVL_GOD, true, "Bad PW: %s [%s]", d->account->name, d->host);
	if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {	/* 3 strikes and you're out. */
	  write_to_output(d, "Wrong password... disconnecting.\r\n");
	  STATE(d) = CON_CLOSE;
	} else {
	  write_to_output(d, "Wrong password.\r\nPassword: ");
	  ProtocolNoEcho( d, true );
	}
	return;
      }

      /* Password was correct. */
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	write_to_output(d, "Your site (domain/IP address) has been denied access from playing Age of Legends.\r\nPlease contact"
			   "seraphime@@d20mud.com if you feel this should not apply to you.\r\n");
	mudlog(NRM, ADMLVL_GOD, true, "Connection attempt for %s denied from %s.  Host is banned.", d->account->name, d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      show_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, d->account->name)) {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(d->account->password, CRYPT(arg, d->account->name), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
    *(d->account->password + MAX_PWD_LENGTH) = '\0';

    write_to_output(d, "\r\nPlease retype @gpassword@n: ");
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;
    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, d->account->password), d->account->password,
		MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    ProtocolNoEcho( d, false );

    if (STATE(d) == CON_CNFPASSWD) {
      show_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;

    } else {
      write_to_output(d, "\r\nDone.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->sex = SEX_FEMALE;
      break;
    default:
      write_to_output(d, "@RYou either have one or you don't@W.@n\r\n"
                "@YSo, what IS your sex@W?@n ");
      return;
    }

      STATE(d) = CON_AFTER_DESC;
      SEND_TO_Q("\r\n*** PRESS RETURN TO CONTINUE ***\r\n", d);
    break;

    case CON_QALIGNMENT:
    switch (*arg) {
      case 't':
      case 'T':
        display_alignment_help(d);
        STATE(d) = CON_ALIGNMENT_HELP;
        return;
    }
    parse_alignment(d->character, *arg);
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i < 1 || player_i > NUM_ALIGNMENTS) {
        write_to_output(d, "\r\nThat's not a valid alignment.\r\nAlignment: ");
        return;
      }
    }
    else {
      display_alignments(d);
      return;
    }

    display_races(d);
    STATE(d) = CON_QRACE;
    break;
      
  case CON_ALIGNMENT_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_alignments(d);
      STATE(d) = CON_QALIGNMENT;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_ALIGNMENTS || player_i < 1) {
	write_to_output(d, "\r\nThat's not an alignment.\r\nHelp on Alignment #: ");
	break;
      }
      player_i -= 1;
      show_help(d, alignments[player_i]);
    } else {
      display_alignment_help(d);
    }
    STATE(d) = CON_ALIGNMENT_HELP;
    break;

  case CON_RACE_CONFIRM:
    switch(*arg) {
      case 'y':
      case 'Y':
        display_classes(d);
        STATE(d) = CON_QCLASS;   
        return;

      case 'n':
      case 'N':
        GET_REAL_RACE(d->character) = RACE_UNDEFINED;
        display_races(d);
        STATE(d) = CON_QRACE;
        return;

      default:
        write_to_output(d, "\r\nPlease select Yes or No: ");
        return;
    }

    break;

  case CON_RACE_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_races(d);
      STATE(d) = CON_QRACE;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_RACES || player_i < 1) {
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
	break;
      }
      player_i -= 1;
      if (race_ok_gender[(int)GET_SEX(d->character)][player_i])
	show_help(d, race_list[player_i].name);
      else
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
    } else {
      display_races_help(d);
    }
    STATE(d) = CON_RACE_HELP;
    break;
  
  case CON_CLASS_HELP:
    display_classes(d);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:

    two_arguments(arg, arg1, arg2);

    if (!*arg1 || (strcmp(arg1, "fighter") && strcmp(arg1, "barbarian") && strcmp(arg1, "cleric") &&
        strcmp(arg1, "rogue") && strcmp(arg1, "monk") && strcmp(arg1, "mage") && strcmp(arg1, "paladin") && strcmp(arg1, "help"))) {
      send_to_char(d->character, "\r\nThat is not a valid class.\r\n\r\n");
      send_to_char(d->character, "To select a class type the class name.  To learn more about a class type help followed\r\n");
      send_to_char(d->character, "by the class name.\r\n");
      send_to_char(d->character, "\r\n");
      send_to_char(d->character, "Classes: paladin fighter rogue mage cleric monk barbarian\r\n");
      send_to_char(d->character, "\r\n");
      send_to_char(d->character, "What class would you like to start as? ");
      return;
    }

    if (!strcmp(arg1, "fighter"))
      GET_CLASS(d->character) = CLASS_FIGHTER;
    else if (!strcmp(arg1, "barbarian"))
      GET_CLASS(d->character) = CLASS_BARBARIAN;
    else if (!strcmp(arg1, "cleric"))
      GET_CLASS(d->character) = CLASS_CLERIC;
    else if (!strcmp(arg1, "rogue"))
      GET_CLASS(d->character) = CLASS_ROGUE;
    else if (!strcmp(arg1, "monk"))
      GET_CLASS(d->character) = CLASS_MONK;
    else if (!strcmp(arg1, "mage"))
      GET_CLASS(d->character) = CLASS_WIZARD;
    else if (!strcmp(arg1, "paladin"))
      GET_CLASS(d->character) = CLASS_PALADIN;
    else {

      if (*arg2) {
        if (strcmp(arg2, "fighter") && strcmp(arg2, "barbarian") && strcmp(arg2, "cleric") &&
          strcmp(arg2, "rogue") && strcmp(arg2, "monk") && strcmp(arg1, "paladin") && strcmp(arg2, "mage")) {
          send_to_char(d->character, "\r\nThat is not a valid class to seek help on.\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "To select a class type the class name.  To learn more about a class type help followed\r\n");
          send_to_char(d->character, "by the class name.\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "Classes: paladin fighter rogue mage cleric monk barbarian\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "What class would you like to start as? ");
          return;
        }
        if (!(this_help = find_help(arg2))) {
          send_to_char(d->character, "\r\nThat is not a valid class to seek help on.\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "To select a class type the class name.  To learn more about a class type help followed\r\n");
          send_to_char(d->character, "by the class name.\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "Classes: paladin fighter rogue mage cleric monk barbarian\r\n");
          send_to_char(d->character, "\r\n");
          send_to_char(d->character, "What class would you like to start as? ");
          return;
        }
        write_to_output(d, "\r\n@W%s@n\r\n%s\r\n", this_help->keywords, this_help->entry);
        write_to_output(d, "\r\n*** PRESS RETURN: ");
        STATE(d) = CON_CLASS_HELP;
        return;                    
      }
      else {
        send_to_char(d->character, "\r\nThat is not a valid class to seek help on.\r\n");
        send_to_char(d->character, "\r\n");
        send_to_char(d->character, "To select a class type the class name.  To learn more about a class type help followed\r\n");
        send_to_char(d->character, "by the class name.\r\n");
        send_to_char(d->character, "\r\n");
        send_to_char(d->character, "Classes: paladin fighter rogue mage cleric monk barbarian\r\n");
        send_to_char(d->character, "\r\n");
        send_to_char(d->character, "What class would you like to start as? ");
        return;
      }
    }
			
    write_to_output(d, "\r\n*** PRESS RETURN: ");
    STATE(d) = CON_QROLLSTATS;	
 	
    break;

  case CON_SDESC:
    if (parse_sdesc(arg, d->character) == FALSE) {
      disp_sdesc_menu(d);
      write_to_output(d, "Your short description must be less than 60 characters and more than 15.  Please correct this error.\r\n");
      STATE(d) = CON_SDESC;
      break;
    }

    GET_PC_SDESC(d->character) = strdup(arg);

    write_to_output(d, "Your short desc is: %s\r\n", GET_SDESC(d->character));
		
    SEND_TO_Q("\r\nPlease enter your 'alias list' -- words people can use\r\n", d);
    SEND_TO_Q("to identify you by.  This should include all attributes in\r\n", d);
    SEND_TO_Q("your short description.  For example, if your short description was:\r\n", d);
    SEND_TO_Q("   a fat, balding man with shifty eyes.\r\n", d);
    SEND_TO_Q("Your alias list would be: fat balding shifty\r\n--> ", d);
    STATE(d) = CON_ALIAS;
    
    break;

  case CON_ALIAS:
    if (!*arg) {
      SEND_TO_Q("You must enter an alias list:\r\n", d);
      break;
    }
    else
    {
      int aliasSize = 0;
      if (GET_NAME(d->character))
      {
        aliasSize = strlen(GET_NAME(d->character));
      }
      aliasSize += 3 + strlen(pc_race_types[GET_RACE(d->character)]);
      aliasSize += strlen(arg);
 
      if (aliasSize > (MAX_NAME_LENGTH * 4)) {
        SEND_TO_Q("Your alias list was too long, please re-enter:\r\n", d);
        break; 
      }
    }		
		
    d->character->player_specials->keywords = strdup(arg);
		
    write_to_output(d, "\r\nYou finish setting your custom descs, press return to re-enter the game. ");
	
    STATE(d) = CON_PLAYING;
    break;

  case CON_QROLLSTATS:
    if (CONFIG_REROLL_PLAYER_CREATION && 
       (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_1)) {
      switch (*arg) {
      case 'y':
      case 'Y':
        break;
      case 'n':
      case 'N':
      default:
        cedit_creation(d->character);
        write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                              "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                              "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
           GET_STR(d->character), GET_DEX(d->character), 
           GET_CON(d->character), GET_INT(d->character), 
           GET_WIS(d->character), GET_CHA(d->character));
        write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
        return;
      }
    } else if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2 ||
        CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3) {
        if (CONFIG_REROLL_PLAYER_CREATION && 
           (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2)) {
          switch (*arg) {
            case 'y':
            case 'Y':
              break;
            case 'n':
            case 'N':
            default:
              cedit_creation(d->character);
              write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                                    "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                                    "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
                 GET_STR(d->character), GET_DEX(d->character),
                 GET_CON(d->character), GET_INT(d->character),
                 GET_WIS(d->character), GET_CHA(d->character));
	      write_to_output(d, "Initial statistics, you may reassign individual numbers\r\n");
	      write_to_output(d, "between statistics after choosing yes.\r\n");
              write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
              return;
          }
        }
	else
      cedit_creation(d->character);
      if (!d->olc) {
        CREATE(d->olc, struct oasis_olc_data, 1);
      }
      if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3)
        OLC_VAL(d) = CONFIG_INITIAL_POINTS_POOL;
      else
        OLC_VAL(d) = 0;

      STATE(d) = CON_QSTATS;
      stats_disp_menu(d);
      break;
    } else {
      cedit_creation(d->character);
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character);
    save_player_index();
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;
    total = GET_STR(d->character) / 2 + GET_CON(d->character) / 2 + 
            GET_WIS(d->character) / 2 + GET_INT(d->character) / 2 + 
            GET_DEX(d->character) / 2 + GET_CHA(d->character) / 2;
    total -= 30;
    if (!PRF_FLAGGED(d->character, PRF_ANONYMOUS))
      mudlog(CMP, ADMLVL_NONE, true, "New player: %s [%s %s]", 
             GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
             (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[GET_CLASS(d->character)]);
    mudlog(CMP, ADMLVL_IMMORT, true, "New player: %s [%s %s]", 
             GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
             (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[GET_CLASS(d->character)]);
  
    break;

  /* --relistan 2/22/99 for configurable stats */
  case CON_QSTATS:

    if (parse_stats(d, arg)) {

      STATE(d) = CON_PLAYING;
      SEND_TO_Q("\r\n@YYou are finished setting stats and return to the normal game.\r\n\r\n@n", d);
      return;
	      SEND_TO_Q("Now you are required to enter some descriptions in order to make your character\r\n", d);
      SEND_TO_Q("playable in our description-based game.  The first descriptions you enter are\r\n", d);
      SEND_TO_Q("mandatory at this stage.  They are based on a series of features you choose for\r\n", d);
      SEND_TO_Q("your character from lists.  The second set of descriptions are optional at\r\n", d);
      SEND_TO_Q("this point, though they need to be entered at some stage.  The second set of\r\n", d);
      SEND_TO_Q("descriptions will be your permanent descriptions and are completely custom-written\r\n", d);
      SEND_TO_Q("by you, but they need to be approved by a staff member.  Until the second set are\r\n", d);
      SEND_TO_Q("written and approved, you will use the first set, the ones you are about to do now,\r\n", d);
      SEND_TO_Q("and until the second set are written and approved, you will only be able to level\r\n", d);
      SEND_TO_Q("up to level 10.  This is usually about 15-30+ hours of game play so it should\r\n", d);
      SEND_TO_Q("be plenty of time for you to come up with the descriptions and have an immortal\r\n", d);
      SEND_TO_Q("approve them.  Don't forget as well, that you are required to write at least a few\r\n", d);
      SEND_TO_Q("sentences of a character background or personality profile to be sent to the staff\r\n", d);
      SEND_TO_Q("for approval as well.\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("So without further ado, let's get ready to enter your descriptions.  You will first\r\n", d);
      SEND_TO_Q("choose an initial descrtiptor type from the list below.  Then you will choose a word\r\n", d);
      SEND_TO_Q("or phrase to describe it.  At this point you can accept your description or customize\r\n", d);
      SEND_TO_Q("it further with a second descriptor and describing word/phrase.  Try to keep your\r\n", d);
      SEND_TO_Q("description somewhat short as this will be used in place of your name in any situation\r\n", d);
      SEND_TO_Q("where someone has not been told your name using the introduce command.\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("@Y(Press Enter to Continue)@n\r\n", d);

      STATE(d) = CON_GEN_DESCS_INTRO;

      break;	
    } 
    break;

  case CON_GEN_DESCS_INTRO:

      SEND_TO_Q("Current short description: @W", d);
			tmpdesc = current_short_desc(d->character);
      SEND_TO_Q(tmpdesc, d);
			free(tmpdesc);
      SEND_TO_Q("@n\r\n\r\n", d);
      short_desc_descriptors_menu(d->character);
      STATE(d) = CON_GEN_DESCS_DESCRIPTORS_1;

      return;

  case CON_GEN_DESCS_DESCRIPTORS_1:

      count = NUM_FEATURE_TYPES;

      if (atoi(arg) < 1 || atoi(arg) > count) {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
        return;
      }

      GET_PC_DESCRIPTOR_1(d->character) = atoi(arg);
      short_desc_adjectives_menu(d->character, GET_PC_DESCRIPTOR_1(d->character));
      
      STATE(d) = CON_GEN_DESCS_ADJECTIVES_1;

      return;
	
    case CON_GEN_DESCS_ADJECTIVES_1:

      count = count_adjective_types(GET_PC_DESCRIPTOR_1(d->character));

      if (atoi(arg) < 1 || atoi(arg) > count) {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
        return;
      }

      GET_PC_ADJECTIVE_1(d->character) = atoi(arg);

      SEND_TO_Q("@Y(Press return to continue)@n", d);
      STATE(d) = CON_GEN_DESCS_MENU;
      return;

    case CON_GEN_DESCS_DESCRIPTORS_2:

      count = NUM_FEATURE_TYPES;

      if (atoi(arg) < 1 || atoi(arg) > count) {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
        return;
      }

      GET_PC_DESCRIPTOR_2(d->character) = atoi(arg);

      short_desc_adjectives_menu(d->character, GET_PC_DESCRIPTOR_2(d->character));
      
      STATE(d) = CON_GEN_DESCS_ADJECTIVES_2;

      return;

    case CON_GEN_DESCS_ADJECTIVES_2:

      count = count_adjective_types(GET_PC_DESCRIPTOR_2(d->character));

      if (atoi(arg) < 1 || atoi(arg) > count) {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
        return;
      }

      GET_PC_ADJECTIVE_2(d->character) = atoi(arg);

      SEND_TO_Q("@Y(Press return to continue)@n", d);
      STATE(d) = CON_GEN_DESCS_MENU;
      return;


    case CON_GEN_DESCS_MENU:

      SEND_TO_Q("Current short description: @W", d);
			tmpdesc = current_short_desc(d->character);
      SEND_TO_Q(tmpdesc, d);
			free(tmpdesc);
      SEND_TO_Q("@n\r\n\r\n", d);
      SEND_TO_Q("Are you happy with this short description?\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("1) I'm happy with it and am ready to play the game!\r\n", d);
      SEND_TO_Q("2) I'm happy with it and want to make my custom descs now as well.\r\n", d);
      SEND_TO_Q("3) I'm not happy with it and want to start over.\r\n", d);
      if (GET_PC_DESCRIPTOR_2(d->character) == 0)
        SEND_TO_Q("4) I'm not happy with it because I want to add a second descriptor.\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("What would you like to do? (1-4): ", d);      

      STATE(d) = CON_GEN_DESCS_MENU_PARSE;

      return;

    case CON_GEN_DESCS_MENU_PARSE:

      switch (atoi(arg)) {

      case 1:
        STATE(d) = CON_PLAYING;
        SEND_TO_Q("Your character descrptions are complete.  Press return to continue!\r\n", d);
        break;

      case 2:

      write_to_output(d, "This feature has been temporarily disabled.\r\n");
      return;

        break;


      case 3:

        GET_PC_DESCRIPTOR_1(d->character) = 0;
        GET_PC_ADJECTIVE_1(d->character) = 0;
        GET_PC_DESCRIPTOR_2(d->character) = 0;
        GET_PC_ADJECTIVE_2(d->character) = 0;
        STATE(d) = CON_GEN_DESCS_INTRO;
        SEND_TO_Q("@YPress enter to continue)@n\r\n", d);
        break;

      case 4:
        if (GET_PC_DESCRIPTOR_2(d->character) == 0) {
          STATE(d) = CON_GEN_DESCS_DESCRIPTORS_2;

          SEND_TO_Q("Current short description: @W", d);
					char *tmpdesc = current_short_desc(d->character);
          SEND_TO_Q(tmpdesc, d);
					free(tmpdesc);
          SEND_TO_Q("@n\r\n\r\n", d);
          short_desc_descriptors_menu(d->character);

          SEND_TO_Q("\r\n@Y(Press enter to choose your second descriptor)@n\r\n", d);
          return;
        }
        else {
          SEND_TO_Q("You have already set your second descriptor.  Please choose another option\r\n", d);
          return;
        }
        break;

      default:
        SEND_TO_Q("That is not a valid option.  Please choose again.\r\n", d);
        break;

      }

      return;		
	
    case CON_AFTER_DESC:
    if (d->olc) {
      free(d->olc);
    }
    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character);
    save_player_index();
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    if (!PRF_FLAGGED(d->character, PRF_ANONYMOUS))
      mudlog(CMP, ADMLVL_NONE, true, "New player: %s", GET_NAME(d->character));
    mudlog(CMP, ADMLVL_IMMORT, true, "New player: %s", GET_NAME(d->character));
    STATE(d) = CON_RMOTD;
    return;
	
  case CON_RMOTD:		/* read CR after printing motd   */
    add_llog_entry(d->character,LAST_CONNECT);
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    write_to_output(d, "%s", CONFIG_MENU);
    STATE(d) = CON_MENU;
    break;

  case CON_QUIT_GAME_EARLY:
    if (!strcmp(arg, "quit")) {
      write_to_output(d, "\r\n\r\nThank you for playing our game.  Please visit us again soon.  Bye now!\r\n\r\n");
      write_to_output(d, "%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    else {
      FILE *fp;
      fp = fopen("/home/aol/d20/log/quitmsg", "a+");
      fprintf(fp, "%s: %s\n", GET_NAME(d->character), arg);
      fclose(fp);
    }
    break;

  case CON_LEVELUP_START:
	  display_levelup_classes(d);
	  STATE(d) = CON_LEVELUP_CLASSES;
	  break;

  case CON_LEVELUP_CLASSES:
          
	  class = (ubyte) atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          } else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "list") || is_abbrev(arg1, "display")) {
            display_levelup_classes(d);
            return;
          }

	  if (class < 1 || class > TOP_PC_CLASS) {
		  write_to_output(d, "That is not a valid class, please select again: ");
		  return;
	  }

          if (prestige_classes_dl_aol[class-1] && GET_CLASS_RANKS(d->character, class-1) >= 20) {
            write_to_output(d, "You cannot advance any prestige class beyond rank 20.\r\n");
            return;
          }

          if (prestige_classes_dl_aol[class-1] && !has_unlocked_class(d->character, class-1)) {
            write_to_output(d, "You have not yet unlocked that class.  See @YHELP ACCOUNT EXPERIENCE@n.\r\n");
            return;
          }

	  if (do_class_ok_general(d->character, class - 1, FALSE) == -4) {
		  write_to_output(d, "That class is not available, please select again: ");
		  return;
	  }
	  if (do_class_ok_general(d->character, class - 1, FALSE) == -2) {
	  	  write_to_output(d, "Your alignment is not appropriate for that class, please select again: ");
	  	  return;
	  }
	  if (do_class_ok_general(d->character, class - 1, FALSE) == -3) {
	  	  write_to_output(d, "You have reached the maximum allowable level for that class, please select again: ");
	  	  return;
	  }
	  if (do_class_ok_general(d->character, class - 1, FALSE) == -5) {
	  	  write_to_output(d, "You have already taken your maximum number of classes, please select again: ");
	  	  return;
	  }
	  if (do_class_ok_general(d->character, class - 1, FALSE) == -6) {
	  	  write_to_output(d, "You must be sponsored by a staff member to take that class, please select again: ");
	  	  return;
	  }
	  if (do_class_ok_general(d->character, class - 1, TRUE) < 1) {
	  	  return;
	  }
	  if (d->character->levelup == NULL)
		CREATE(d->character->levelup, struct level_data, 1);
	  if (d->character->levelup != NULL)
		d->character->levelup->class = MAX(0, (class - 1));
	  write_to_output(d, "\r\nYou have chosen the %s class.\r\n\r\n", pc_class_types_core[class - 1]);
	  if (d->character->levelup != NULL)
		init_levelup(d->character);
          if (d->character->levelup->class == CLASS_FAVORED_SOUL || d->character->levelup->class == CLASS_SORCERER) {
            STATE(d) = CON_LEVELUP_SPELLS;
            write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
          }
          else 
          {
            STATE(d) = CON_LEVELUP_SKILLS;
            display_levelup_skills(d->character, TRUE);
          }
	  break;

  case CON_LEVELUP_SPELLS:

    if (!*arg) {
      write_to_output(d, "Please type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");
      return;
    }

    half_chop(arg, arg1, argument);
    if (is_abbrev(arg1, "help")) {
      do_help(ch, argument, 0, 0);
            return;
    }

    i = find_skill_num(arg, SKTYPE_SPELL);

    if (atoi(arg) == -1) {
      STATE(d) = CON_LEVELUP_SPELLS_CONFIRM;
      write_to_output(d, "Are you sure you wish to continue?  Your selections can't be changed if you proceed and any unspent spell slots will be lost.\r\n");
    }
    else if (i >= 0 && i <= TOP_SPELL) {

      if (i < 0 || i > TOP_SPELL) {
        write_to_output(d, "That is not a valid spell.\r\n");
        write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
      }
      else {
        for (j = 0; j < MAX_NUM_KNOWN_SPELLS; j++) {
          if (d->character->levelup->spells_known[j] == i) {
            found = TRUE;
            d->character->levelup->spells_known[j] = 0;
            d->character->levelup->spell_slots[spell_info[i].class_level[d->character->levelup->class]]++;
            write_to_output(d, "Spell '%s' removed from list of spells known.\r\n", spell_info[i].name);
            write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
          }
        }
        if (!found) {
          for (j = 0; j < MAX_NUM_KNOWN_SPELLS; j++) {
            if (d->character->levelup->spells_known[j] == 0) {
              found = TRUE;
              if (d->character->levelup->spell_slots[spell_info[i].class_level[d->character->levelup->class]] > 0) {
                write_to_output(d, "Spell '%s' added to list of spells known.\r\n", spell_info[i].name);
                write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
                d->character->levelup->spells_known[j] = i;
                d->character->levelup->spell_slots[spell_info[i].class_level[d->character->levelup->class]]--;
                return;
              }
              else {
                send_to_char(d->character, "You can't learn any new spells of that level.\r\n");
                write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
                break;
              }
            }    
          }
        }
      }
      if (!found) {
        int k = 0;
        write_to_output(d, "\r\n\r\n");
        for (k = 0; k < MAX_NUM_KNOWN_SPELLS; k++)
          write_to_output(d, "%d ", d->character->levelup->spells_known[k]);
        write_to_output(d, "\r\n\r\n");
        // character has a filled spells_known list >= MAX_NUM_KNOWN_SPELLS
        send_to_char(d->character, "You can't learn any more spells.  Please speak to an administrator.\r\n");
        write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
      }
    }
    else if (atoi(arg) >= 0 && atoi(arg) <= 9) {
      sprintf(buf, "%s", class_names_dl_aol[d->character->levelup->class]);
      one_argument(buf, arg2);
      sprintf(buf, "%s %d", arg2, atoi(arg));
      GET_CLASS_NONEPIC(d->character, d->character->levelup->class)++;
      do_spells(d->character, buf, 0, 0);
      GET_CLASS_NONEPIC(d->character, d->character->levelup->class)--;
      write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
    }
    else {
      write_to_output(d, "That is not a valid spell level.\r\n");
    }
    break;

  case CON_LEVELUP_FEATS:

	  i = atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) 
          {
            do_help(ch, argument, 0, 0);
            return;
          }
          else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "list") || is_abbrev(arg1, "display")) 
          {
            display_levelup_feats(d->character);
            return;
          }

	  if (i < 0) 
    {

		  if (d->character->levelup->feat_points > 0 || d->character->levelup->epic_feat_points > 0 || d->character->levelup->num_class_feats > 0 || d->character->levelup->num_epic_class_feats > 0) 
      {
 			  write_to_output(d, "You still have feats left to spend.  If you do not use them you will lose them.  Continue and lose them? ");
			  STATE(d) = CON_LEVELUP_FEATS_CONFIRM;
			  return;
		  }

		  display_levelup_trains(d->character);
		  STATE(d) = CON_LEVELUP_TRAINS;
		  return;

	  }

	  if (feat_is_available(d->character, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn) 
    {
  		write_to_output(d, "Feat Description: %s\r\n\r\nDo you wish to take this feat?\r\n", feat_list[i].description);
  		d->character->levelup->tempFeat = i;
  		STATE(d) = CON_LEVELUP_FEATS_PROCESS;
  		return;
  	}
	  else 
    {
		  display_levelup_feats(d->character);
		  write_to_output(d, "Either the feat does not exist, you do not qualify for that feat, or you already have it.\r\nPlease select again.\r\n");
		  return;		
	  }
	break;

  case CON_LEVELUP_FEATS_PROCESS:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

	i = d->character->levelup->tempFeat;

	if (is_abbrev("yes", arg) || is_abbrev("YES", arg) || is_abbrev("Yes", arg)) 
  {
		  switch (i) 
      {

		  case FEAT_IMPROVED_CRITICAL:
		  case FEAT_WEAPON_FINESSE:
		  case FEAT_WEAPON_FOCUS:
		  case FEAT_WEAPON_SPECIALIZATION:
		  case FEAT_GREATER_WEAPON_FOCUS:
		  case FEAT_GREATER_WEAPON_SPECIALIZATION:
		  case FEAT_IMPROVED_WEAPON_FINESSE:
		  case FEAT_WEAPON_PROFICIENCY_EXOTIC:
		  case FEAT_MONKEY_GRIP:
		  case FEAT_CRITICAL_FOCUS:
		  case FEAT_WEAPON_MASTERY:
		  case FEAT_WEAPON_FLURRY:
		  case FEAT_WEAPON_SUPREMACY:
		
			  display_levelup_weapons(d->character);
			  d->character->levelup->tempFeat = i;
			  STATE(d) = CON_LEVELUP_FEATS_WEAPONS;
  		  return;

		  case FEAT_SKILL_FOCUS:
		  case FEAT_EPIC_SKILL_FOCUS:
			  display_levelup_skills(d->character, FALSE);
			  d->character->levelup->tempFeat = i;
			  STATE(d) = CON_LEVELUP_FEATS_SKILLS;
			  return;

		  default:
			  if (handle_levelup_feat_points(d->character, i, 0)) 
        {
				  display_levelup_feats(d->character);
				  write_to_output(d, "\r\nYou have learned the %s feat!\r\nPress enter to continue.\r\n", feat_list[i].name);
				  handle_levelup_feat_points(d->character, i, 1);
				  return;

			  }
			  else 
        {
				  return;
			  }
			  break;
		  }
	  }
	  else 
    {
  		display_levelup_feats(d->character);
  		write_to_output(d, "Please select again: ");
  		STATE(d) = CON_LEVELUP_FEATS;
  		return;
  	}

	  break;

  case CON_LEVELUP_FEATS_WEAPONS:

	  i = atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) 
          {
            do_help(ch, argument, 0, 0);
            return;
          } 
          else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "list") || is_abbrev(arg1, "display")) 
          {
            display_levelup_weapons(d->character);
            return;
          }

	  if (i < 0) 
    {
  		display_levelup_feats(d->character);
  		STATE(d) = CON_LEVELUP_FEATS;
  		return;
	  }

	  if (i < MIN_WEAPON_DAMAGE_TYPES || i > MAX_WEAPON_DAMAGE_TYPES) 
    {
		  write_to_output(d, "That is not a valid weapon damage type.\r\nPlease select the number beside the weapon damage type you want.\r\n");
		  return;
	  }

	  if (has_combat_feat(d->character, feat_to_subfeat(d->character->levelup->tempFeat), i) || d->character->levelup->feat_weapons[d->character->levelup->tempFeat] == i) 
    {
		  write_to_output(d, "\r\nYou already have that feat.\r\n");
 		 return;
  	}

	  if (handle_levelup_feat_points(d->character, d->character->levelup->tempFeat, 0)) 
    {
		  display_levelup_feats(d->character);
		  write_to_output(d, "\r\nYou have learned the %s: %s feat!\r\nPress enter to continue.\r\n", feat_list[d->character->levelup->tempFeat].name, weapon_damage_types[i-MIN_WEAPON_DAMAGE_TYPES]);
		  handle_levelup_feat_points(d->character, d->character->levelup->tempFeat, 1);
		  d->character->levelup->feat_weapons[d->character->levelup->tempFeat] = i;
		  d->character->levelup->tempFeat = 0;
		  STATE(d) = CON_LEVELUP_FEATS;
		  return;
	  }
	  else 
    {
		  write_to_output(d, "Make another choice or type -1 to exit this menu and continue.\r\n");
		  return;
	  }

	  break;

  case CON_LEVELUP_FEATS_SKILLS:
	  i = atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          } else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "list") || is_abbrev(arg1, "display")) {
            display_levelup_skills(d->character, TRUE);
            return;
          }

	  if (i < 0) {
		display_levelup_feats(d->character);
		STATE(d) = CON_LEVELUP_FEATS;
		return;
	  }

	  if (i > SKILL_TABLE_SIZE) {
		  write_to_output(d, "That is not a valid skill, please select again.\r\n\r\n");
		  return;
	  }

	  if (d->character->levelup->feat_skills[d->character->levelup->tempFeat] == i) {
		write_to_output(d, "\r\nYou already have that feat.\r\n");
 		return;
  	  }

	  if ((spell_info[i].skilltype == SKTYPE_SKILL && i >= SKILL_LOW_SKILL && i <=
	            SKILL_HIGH_SKILL) && (spell_info[i].can_learn_skill[d->character->levelup->class] == SKLEARN_CLASS || spell_info[i].can_learn_skill[d->character->levelup->class] == SKLEARN_CROSSCLASS) &&
	            !IS_SET(spell_info[i].flags, SKFLAG_CRAFT)) {

		  if (handle_levelup_feat_points(d->character, d->character->levelup->tempFeat, 0)) {
	  		  display_levelup_feats(d->character);
	  		  write_to_output(d, "\r\nYou have learned the %s: %s feat!\r\nPress enter to continue.\r\n", feat_list[d->character->levelup->tempFeat].name, spell_info[i].name);
	  		  handle_levelup_feat_points(d->character, d->character->levelup->tempFeat, 1);
			  d->character->levelup->feat_skills[d->character->levelup->tempFeat] = i;
	  		  d->character->levelup->tempFeat = 0;
	  		  STATE(d) = CON_LEVELUP_FEATS;
	  		  return;
	  	  }
	  	  else {
	  		  return;
	  	  }
	  }
	  else {
		  write_to_output(d, "\r\nThat is not a valid skill.  Please select again.\r\n");
		  return;
	  }

	  break;

  case CON_LEVELUP_FEATS_CONFIRM:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

	if (is_abbrev("yes", arg) || is_abbrev("YES", arg) || is_abbrev("Yes", arg)) {
		display_levelup_trains(d->character);
		STATE(d) = CON_LEVELUP_TRAINS;
   		return;
	}
	else {
		display_levelup_feats(d->character);
		write_to_output(d, "Returning to feats menu.\r\n\r\n");
		STATE(d) = CON_LEVELUP_FEATS;
		return;
	}

	break;

  case CON_LEVELUP_SPELLS_CONFIRM:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

	if (is_abbrev("yes", arg) || is_abbrev("YES", arg) || is_abbrev("Yes", arg)) {
                display_levelup_skills(d->character, TRUE);
		STATE(d) = CON_LEVELUP_SKILLS;
   		return;
	}
	else {
		write_to_output(d, "Returning to spells menu.\r\n\r\n");
                write_to_output(d, "\r\nPlease type in either a spell level number to list spells of, a spell name to add to/remove from your repetoire, or -1 to continue\r\n");      
		STATE(d) = CON_LEVELUP_SPELLS;
		return;
	}

	break;


  case CON_LEVELUP_TRAINS:

	  i = atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          } else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "list") || is_abbrev(arg1, "display")) {
            display_levelup_trains(d->character);
            return;
          }

	  if (i < 1) {
		  if (d->character->levelup->num_trains > 0) {
			  write_to_output(d, "Do you wish to finish spending your ability score trains?  You have to use them now or lose them. Continue and lose them?\r\n");
			  STATE(d) = CON_LEVELUP_TRAINS_CONFIRM;
			  return;
		  }

		  display_levelup_summary(d->character);
		  STATE(d) = CON_LEVELUP_SUMMARY;
		  return;
	  }

	  if (i > 6) {
		  display_levelup_trains(d->character);

		  write_to_output(d, "\r\n That is not a valid ability score, please select again: ");
		  return;

	  }
	  display_levelup_trains(d->character);


	  if (d->character->levelup->num_trains < 1) {
	    write_to_output(d, "You do not have any ability trains left to increase anything.\r\nPlease press enter to continue.\r\n");
	    return;
	  }
          else {
	  switch (i) {
	  case 1:
		  write_to_output(d, "\r\nYou have increased your strength by one!\r\n");
		  break;
	  case 2:
  		  write_to_output(d, "\r\nYou have increased your dexterity by one!\r\n");
  		  break;
	  case 3:
  		  write_to_output(d, "\r\nYou have increased your constitution by one!\r\n");
  		  break;
	  case 4:
 		  write_to_output(d, "\r\nYou have increased your intelligence by one!\r\n");
  		  break;
	  case 5:
  		  write_to_output(d, "\r\nYou have increased your wisdom by one!\r\n");
  		  break;
	  case 6:
  		  write_to_output(d, "\r\nYou have increased your charisma by one!\r\n");
  		  break;
	  }

	  d->character->levelup->trains[i-1]++;
	  d->character->levelup->num_trains--;
          }
	  break;

  case CON_LEVELUP_TRAINS_CONFIRM:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

		if (is_abbrev("no", arg) || is_abbrev("NO", arg) || is_abbrev("No", arg)) {
			display_levelup_trains(d->character);
			write_to_output(d, "Returning to ability score trains menu.\r\n\r\n");
			STATE(d) = CON_LEVELUP_TRAINS;
	   		return;
		}
		else {
			display_levelup_summary(d->character);
			STATE(d) = CON_LEVELUP_SUMMARY;
			return;
		}

		break;

  case CON_LEVELUP_SKILLS_CONFIRM:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

	if (is_abbrev("yes", arg) || is_abbrev("YES", arg) || is_abbrev("Yes", arg)) {
		display_levelup_feats(d->character);
		STATE(d) = CON_LEVELUP_FEATS;
   		return;
	}
	else {
		display_levelup_skills(d->character, TRUE);
		write_to_output(d, "Returning to skills menu.\r\n\r\n");
		STATE(d) = CON_LEVELUP_SKILLS;
		return;
	}

	break;

  case CON_LEVELUP_SUMMARY:

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }

	if (is_abbrev("yes", arg) || is_abbrev("YES", arg) || is_abbrev("Yes", arg)) {
		do_advance_level(d->character, d->character->levelup->class, FALSE);
		STATE(d) = CON_PLAYING;
   		return;
	}
	else {
		write_to_output(d, "\r\n\r\nLevelling process cancelled.  Changes lost.  Returning to game.\r\n");
		STATE(d) = CON_PLAYING;
		return;
	}

	break;

  case CON_LEVELUP_SKILLS:
	  i = atoi(arg);

	  half_chop(arg, arg1, argument);
          if (is_abbrev(arg1, "help")) {
            do_help(ch, argument, 0, 0);
            return;
          }


	  if (i < 0) {
		  if (d->character->levelup->practices > 0) {
			  write_to_output(d, "You still have %d skill points left to spend.  If you do not use them you will lose them forever. Continue & lose them?", d->character->levelup->practices);
			  STATE(d) = CON_LEVELUP_SKILLS_CONFIRM;
			  return;
		  }
		  display_levelup_feats(d->character);
		  STATE(d) = CON_LEVELUP_FEATS;
		  return;
	  }

	  if (i > SKILL_TABLE_SIZE) {
		  write_to_output(d, "That is not a valid skill, please select again.\r\n\r\n");
		  return;
	  }

	  if ((spell_info[i].skilltype == SKTYPE_SKILL && i >= SKILL_LOW_SKILL && i <=
	            SKILL_HIGH_SKILL) && !IS_SET(spell_info[i].flags, SKFLAG_CRAFT)) {

		  if ((GET_SKILL_BASE(d->character, i) + d->character->levelup->skills[i]) >=  (d->character->levelup->level)) {
			  write_to_output(d, "You have reached your maximum rank in that skill.\r\n\r\n");
			  return;
		  }

		  if (d->character->levelup->practices <  1) {
	  		  write_to_output(d, "You do not have enough skill points left to raise that skill.\r\n\r\n");
	  		  return;
	  	  }

		  write_to_output(d, "You have raised your skill in %s by one.\r\n\r\n", spell_info[i].name);
	  }

	  else if ((spell_info[i].skilltype == (SKTYPE_SKILL + SKTYPE_LANG) && i >= SKILL_LANG_LOW && i <=
	            SKILL_LANG_HIGH)) {
	  	  if ((GET_SKILL(d->character, i) + d->character->levelup->skills[i]) > 0) {
	  		write_to_output(d, "You already know that language.\r\n");
	  		return;
	  	  }

		  if (d->character->levelup->practices <  1) {
	  		  write_to_output(d, "You do not have enough skill points left to learn that language.\r\n\r\n");
	  		  return;
	  	  }

		  write_to_output(d, "You have learned the %s language!  Press enter to continue.\r\n\r\n", spell_info[i].name);
	  }
	  else {
		  display_levelup_skills(d->character, TRUE);
		  write_to_output(d, "\r\nThat is not a valid skill.  Please select again.\r\n");
		  return;
	  }

	  d->character->levelup->skills[i]++;
	  d->character->levelup->practices--;;
 	  display_levelup_skills(d->character, TRUE);
	  
	  break;


  case CON_SELECT_COMBAT_ACTION:

    switch (*arg) {
      case 'Q':
      case 'q':
        write_to_output(d, "You decide to end your turn.\r\n");
	STATE(d) = CON_PLAYING;
	fight_action(d->character->next_fighting, d->character->top_of_initiative);
	break;
      case '1':
        if (GET_EQ(d->character, WEAR_WIELD) && 
            ((FIGHTING(d->character)->combat_pos - d->character->combat_pos) <= 
             (weapon_list[GET_OBJ_VAL(GET_EQ(d->character, WEAR_WIELD), 0)].range * 3))) {
          if (d->character->standard_action_spent == 0) {  
            d->character->standard_action_spent = 1;
	    hit(d->character, FIGHTING(d->character), TYPE_UNDEFINED);
          } else {
            display_combat_menu(d);
            write_to_output(d, "You have already spent your standard action and cannot attack.  Please select again.\r\n");
          }
        }
        else {
          write_to_output(d, "Your foe is not within range of your currently equipped primary weapon.\r\n");
        }
        break;

      case '2':
        if (IS_NPC(FIGHTING(d->character)) || TRUE) {
        if ((FIGHTING(d->character)->combat_pos - d->character->combat_pos) > 0) {
          if (AFF_FLAGGED(d->character, AFF_NEXTNOACTION)) {
            if (d->character->move_action_spent == 0) {
              write_to_output(d, "You take a 5-foot step away from your opponent.\r\n");
              d->character->combat_pos -= 5;  
              d->character->move_action_spent = 1;
            } else {
              write_to_output(d, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (d->character->move_action_spent == 0) {
              write_to_output(d, "You move %d feet away from your opponent.  You are now %d feet away from each other.\r\n",
                              MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos),
                              FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->combat_pos += MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->move_action_spent = 1;
            } else if (d->character->standard_action_spent == 0) {
              write_to_output(d, "You move %d feet away from your opponent.  You are now %d feet away from each other.\r\n",
                              MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos),
                              FIGHTING(d->character)->combat_pos - d->character->combat_pos);
//              if (FIGHTING(d->character)->combat_pos < d->character->combat_pos)
//                d->character->combat_pos += MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->combat_pos += MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->standard_action_spent = 1;
            } else {
              write_to_output(d, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
        } else {
          write_to_output(d, "You are already as close as you can get to your target.\r\n");
        }
        } else {
          write_to_output(d, "Movement is not yet implemented for player-vs-player combat.\r\n");
        }
        break;

      case '3':
        if (IS_NPC(FIGHTING(d->character)) || TRUE) {
          if (AFF_FLAGGED(d->character, AFF_NEXTNOACTION)) {
            if (d->character->move_action_spent == 0) {
              write_to_output(d, "You take a 5-foot step away from your opponent.\r\n");
              d->character->combat_pos -= 5;  
              d->character->move_action_spent = 1;
            } else {
              write_to_output(d, "You cannot take a 5-foot step as you already moved this turn.\r\n");
            }
          }
          else {
            if (d->character->move_action_spent == 0) {
              write_to_output(d, "You move %d feet away from your opponent.  You are now %d feet away from each other.\r\n",
                              MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos),
                              FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->combat_pos -= get_speed(d->character);
              d->character->move_action_spent = 1;
            } else if (d->character->standard_action_spent == 0) {
              write_to_output(d, "You move %d feet away from your opponent.  You are now %d feet away from each other.\r\n",
                              MIN(get_speed(d->character), FIGHTING(d->character)->combat_pos - d->character->combat_pos),
                              FIGHTING(d->character)->combat_pos - d->character->combat_pos);
              d->character->combat_pos -= get_speed(d->character);
              d->character->standard_action_spent = 1;
            } else {
              write_to_output(d, "You cannot move as you already used all your standard and move actions this turn.\r\n");
            }
          }
        } else {
          write_to_output(d, "Movement is not yet implemented for player-vs-player combat.\r\n");
        }
        break;

      case '4':
	write_to_output(d, "Type the command you wish to make.\r\n");
        STATE(d) = CON_CUSTOM_COMBAT_ACTION;
        break;

      default:
        write_to_output(d, "That is not a valid selection.  Please choose again.\r\n");
        break;	
	
    }
    break;

  case CON_CUSTOM_COMBAT_ACTION:
    STATE(d) = CON_PLAYING;
    command_interpreter(d->character, arg);
    STATE(d) = CON_SELECT_COMBAT_ACTION;
    display_combat_menu(d);
    break;

  case CON_EMAIL:
    if (!*arg) {
        send_to_char(d->character, "Please enter a valid email for your account.  You only have to do this once.\r\nEmail Address: ");
        STATE(d) = CON_EMAIL;
        return;      
    }
    if (!strstr(arg, "@") || !strstr(arg, ".")) {
        send_to_char(d->character, "Please enter a valid email for your account.  You only have to do this once.\r\nEmail Address: ");
        STATE(d) = CON_EMAIL;
        return;
    }
    d->account->email = strdup(arg);

    // Open mysql connection
    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
      log("Cannot connect to mysql database in enter player game.");
    }

    sprintf(query, "SELECT account_name FROM player_emails_d20 WHERE account_name='%s'", d->account->name);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL) {
      if ((row = mysql_fetch_row(res)) != NULL) {
        if (atoi(row[0]) > 0) {
          found = TRUE;
        }
      }
    }
    mysql_free_result(res);

    if (!found) {
      sprintf(query, "INSERT INTO player_emails_d20 (account_name, email) VALUES('%s', '%s')", d->account->name, arg);
    } else {
      sprintf(query, "UPDATE player_emails_d20 SET email='%s' WHERE account_name='%s'", arg, d->account->name);
    }
    if (mysql_query(conn, query)) {
       log("Cannot set email for account %s", d->account->name);
    }

    mysql_close(conn);

    send_to_char(d->character, "Thank you.  You may now continue as you were and enter the game if you wish.\r\n");

    write_to_output(d, CONFIG_MENU);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU: {		/* get selection from main menu  */

    switch (*arg) {
    case '0':
      add_llog_entry(d->character, LAST_QUIT);
      write_to_output(d, "Goodbye.\r\n");
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      if (GET_CLASS_LEVEL(d->character) > 5 && !strcmp(d->account->email, "Not Set Yet")) {
        send_to_char(d->character, "Please enter a valid email for your account.  You only have to do this once.\r\nEmail Address: ");
        STATE(d) = CON_EMAIL;
        return;
      }
      if (IS_SET_AR(PLR_FLAGS(d->character), PLR_DELETED)) {
        send_to_char(d->character, "You cannot enter the game as that character.  They have been deleted.\r\n");
        return;
      }
      load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
      act("$n has entered the game.", TRUE, d->character, 0, d->character, TO_NOTVICT);

      d->character->time.logon = time(0);
      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      STATE(d) = CON_PLAYING;      

      if (GET_LEVEL(d->character) == 0) {
//	do_start(d->character);
	send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      look_at_room(IN_ROOM(d->character), d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char(d->character, "\r\nYou have mail waiting.\r\n");
      if (load_result == 2) {	/* rented items lost */
	send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
		"Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);

      int pct = 0;
      for (pct = 1; pct < NUM_POLLS; pct++) {
        if (d->account && d->account->polls[pct] == 0 && poll_list[pct].active == TRUE) {
          send_to_char(d->character, "@l@WYou have new polls to vote on (type poll).@n\r\n");
          break;
        }
      }
        
      break;

    case '2':
      SEND_TO_Q("   Enter a description you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("NOTE:  You must have a long description to be approved, with a *minimum of\r\n", d);
      SEND_TO_Q("       three lines* about your character.  Suggestions for things to write\r\n", d);
      SEND_TO_Q("       about are color of eyes and hair, facial hair, facial distinctions\r\n", d);
      SEND_TO_Q("       such as prominent cheekbones, body type/health, attractiveness, age,\r\n", d);
      SEND_TO_Q("       the character's apparant attitude, scars, tattoos, etc.\r\n", d);
      SEND_TO_Q("       Here is an example:\r\n\r\n", d);
      SEND_TO_Q("   Before you stands a tall man with bright golden blond hair and fierce\r\n", d);
      SEND_TO_Q("emerald green eyes.  His freshly shaven face surveys his surroundings searching\r\n", d);
      SEND_TO_Q("for anything that might be a threat or a benefit to him.  While not tall, \r\n", d);
      SEND_TO_Q("overly muscular or particularly handsome there is something about this man that\r\n", d);
      SEND_TO_Q("makes him stand out in a crowd.\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("When done, type '/fi' on a blank line to format the description.\r\n", d);
      SEND_TO_Q("Then type '/s' to save the description.  ('/h' for help)\r\n", d);

      if (d->character->player_specials->description) {
        free(d->character->player_specials->description);
        d->character->player_specials->description = NULL;
      }

      d->str = &d->character->player_specials->description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_LDESC;	
      break;

      if (d->character->description) {
	write_to_output(d, "Current description:\r\n%s", d->character->description);
	/*
	 * Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->description);
	 * d->character->description = NULL;
	 */
	d->backstr = strdup(d->character->description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str = &d->character->description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      send_to_char(d->character, "Please enter a valid email for your account.  You only have to do this once.\r\nEmail Address: ");
      STATE(d) = CON_EMAIL;
      return;

    case '4':
      write_to_output(d, "\r\nEnter your old password: ");
      ProtocolNoEcho( d, true );
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      show_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s", CONFIG_MENU);
      break;
    }
    break;
  }

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      ProtocolNoEcho( d, false );
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    ProtocolNoEcho( d, false );
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
		"Character not deleted.\r\n\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_ADMLEVEL(d->character) < ADMLVL_GRGOD)
	SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all
         the player's immediately
      */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }

      delete_aliases(GET_NAME(d->character));
      delete_variables(GET_NAME(d->character));
      write_to_output(d, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      mudlog(NRM, ADMLVL_GOD, true, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  case CON_CLOSE:
    break;

  case CON_ASSEDIT:
    assedit_parse(d, arg);
    break;

  case CON_GEDIT:
    gedit_parse(d, arg);
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}

/*
 * Code to disable or enable buggy commands on the run, saving
 * a list of disabled commands to disk. Originally created by
 * Erwin S. Andreasen (erwin@andreasen.org) for Merc. Ported to
 * CircleMUD by Alexei Svitkine (Myrdred), isvitkin@sympatico.ca.
 *
 * Syntax is:
 *   disable - shows disabled commands
 *   disable <command> - toggles disable status of command
 * 
 */

ACMD(do_disable)
{
  int i = 0, length = 0;
  DISABLED_DATA *p, *temp;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument) { /* Nothing specified. Show disabled commands. */
    if (!disabled_first) /* Any disabled at all ? */
      send_to_char(ch, "There are no disabled commands.\r\n");
    else {
      send_to_char(ch,
        "Commands that are currently disabled:\r\n\r\n"
        " Command       Disabled by     Level\r\n"
        "-----------   --------------  -------\r\n");
      for (p = disabled_first; p; p = p->next)
        send_to_char(ch, " %-12s   %-12s    %3d\r\n", p->command->command, p->disabled_by, p->level);
    }
    return;
  }

  /* command given - first check if it is one of the disabled commands */
  for (length = strlen(argument), p = disabled_first; p ;  p = p->next)
    if (!strncmp(argument, p->command->command, length))
    break;
        
  if (p) { /* this command is disabled */

    /* Was it disabled by a higher level imm? */
    if (GET_LEVEL(ch) < p->level) {
      send_to_char(ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, disabled_first, next);
    send_to_char(ch, "Command '%s' enabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, true, "(GC) %s has enabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    free(p->disabled_by);
    free(p);
    save_disabled(); /* save to disk */

  } else { /* not a disabled command, check if the command exists */

    for (length = strlen(argument), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, argument, length))
        if (GET_LEVEL(ch) >= cmd_info[i].minimum_level &&
            GET_ADMLEVEL(ch) >= cmd_info[i].minimum_admlevel)
          break;

    /*  Found?     */            
    if (*cmd_info[i].command == '\n') {
      send_to_char(ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char (ch, "You cannot disable the disable command.\r\n");
      return;
    }

    /* Disable the command */
    CREATE(p, struct disabled_data, 1);
    p->command = &cmd_info[i];
    p->disabled_by = strdup(GET_NAME(ch)); /* save name of disabler  */
    p->level = GET_LEVEL(ch);           /* save level of disabler */    
    p->subcmd = cmd_info[i].subcmd;       /* the subcommand if any  */    
    p->next = disabled_first;
    disabled_first = p; /* add before the current first element */
    send_to_char(ch, "Command '%s' disabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, true, "(GC) %s has disabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    save_disabled(); /* save to disk */
  }
}

/* check if a command is disabled */   
int check_disabled(const struct command_info *command)
{
  DISABLED_DATA *p;

  for (p = disabled_first; p ; p = p->next)
    if (p->command->command_pointer == command->command_pointer)
      if (p->command->subcmd == command->subcmd)
        return true;

  return false;
}

/* Load disabled commands */
void load_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;
  int i = 0 ;
  char line[READ_SIZE]={'\0'}, name[MAX_INPUT_LENGTH]={'\0'}, temp[MAX_INPUT_LENGTH]={'\0'};

  if (disabled_first)
    free_disabled();

  if ((fp = fopen(DISABLED_FILE, "r")) == NULL)
    return; /* No disabled file.. no disabled commands. */

  while (get_line(fp, line)) { 
    if (!str_cmp(line, END_MARKER))
      break; /* break loop if we encounter the END_MARKER */
    CREATE(p, struct disabled_data, 1);
    sscanf(line, "%s %d %hd %s", name, &(p->subcmd), &(p->level), temp);
    /* Find the command in the table */
    for (i = 0; *cmd_info[i].command != '\n'; i++)
      if (!str_cmp(cmd_info[i].command, name))
        break;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      log("WARNING: load_disabled(): Skipping unknown disabled command - '%s'!", name);
      free(p);
    } else { /* add new disabled command */
      p->disabled_by = strdup(temp);
      p->command = &cmd_info[i];
      p->next = disabled_first;
      disabled_first = p;
    }
  }
  fclose(fp);
}

/* Save disabled commands */
void save_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;

  if (!disabled_first) {
    /* delete file if no commands are disabled */
    unlink(DISABLED_FILE);
    return;
   }

  if ((fp = fopen(DISABLED_FILE, "w")) == NULL) {
    log("SYSERR: Could not open " DISABLED_FILE " for writing");
    return;
  }

  for (p = disabled_first; p ; p = p->next)
    fprintf (fp, "%s %d %d %s\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  fprintf(fp, "%s\n", END_MARKER);
  fclose(fp);
}
  
/* free all disabled commands from memory */
void free_disabled()
{
  DISABLED_DATA *p;

  while (disabled_first) {
    p = disabled_first;
    disabled_first = disabled_first->next;
    free(p->disabled_by);
    free(p);
  }
}

void disp_sdesc_menu(struct descriptor_data *d) {

  write_to_output(d, "Now you must select a short description for your character.  Age of Legends uses a\r\n");
  write_to_output(d, "description system for its characters, along with an intro system.  This means that\r\n");
  write_to_output(d, "initially your character will not know anyone else\'s name, and will only see their\r\n");
  write_to_output(d, "descriptions in place of their name until they receive other characters\' intros.  The\r\n");
  write_to_output(d, "short description is used in place of a character's name whenever that character performs\r\n");
  write_to_output(d, "an action.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "For example:\r\n");
  write_to_output(d, "  If my character\'s name was Thoran and my short description was \'a dark haired, dark eyed man\'\r\n");
  write_to_output(d, "  Then if I were to attack something, a person with my intro would see:\r\n");
  write_to_output(d, "  Thoran swings his warhammer at your skull!\r\n");
  write_to_output(d, "  If the person did not have my intro they would see:\r\n");
  write_to_output(d, "  A dark haired, dark eyed man swings his warhammer at your skull!\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Short descriptions must be a minimum of 15 characters and a maximum of 40 characters in length\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Enter your short description here: ");
}

int parse_sdesc(char *arg, struct char_data *ch)
{

  if (strlen(arg) > 60 || strlen(arg) < 15)
    return 0;

  return 1;  

}

void skip_dot(char **string)
{
  for (; **string && (**string == '.'); (*string)++);
}

char *one_arg_dots(char *argument, char *first_arg)
{
  skip_dot(&argument);

  while (*argument && !(*argument == '.'))
  {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}

void display_levelup_classes(struct descriptor_data *d) 
{
	int i = 0;
	int count = 0;

	for (i = 0; i < TOP_PC_CLASS; i++) 
  {
		if (i == CLASS_NPC_EXPERT || i == CLASS_CLASSLESS || i == CLASS_ARTISAN || class_in_game_dl_aol[i] == FALSE)
			continue;
		write_to_output(d, "%s%2d) %-30s   @n", do_class_ok_general(d->character, i, FALSE) > 0 ? "@y" : "@r", i + 1, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
		if (count % 2 == 1)
			write_to_output(d, "\r\n");
		count++;
	}
	if (count % 2 == 0)
		write_to_output(d, "\r\n");
	write_to_output(d, "\r\n");

	write_to_output(d, "Please select a class from the list (@yyellow@n signifies you qualify for the class requirements, @rred@n signifies you do not)\r\nYour selection: ");
}

void init_levelup(struct char_data *ch) 
{
  struct level_data *l = ch->levelup;
  int i = 0;
  int isepic = FALSE;
  int epiclevel = 21;

  l->level = GET_CLASS_LEVEL(ch) + 1;
  if ((l->level + 1) >= epiclevel)
    isepic = TRUE;
  for (i = 0; i <= 9; i++) 
  {
    l->spell_slots[i] = MAX(0, sorcerer_spells_known[MIN(21, GET_CLASS_RANKS(ch, l->class) + 1)][i]); 
  }

  for (i = 0; i < MAX_NUM_KNOWN_SPELLS; i++) 
  {
    l->spells_known[i] = ch->player_specials->spells_known[i];
    if (l->spells_known[i] >= 0 && l->spells_known[i] <= TOP_SPELL && spell_info[l->spells_known[i]].class_level[l->class] <= 9)
      l->spell_slots[spell_info[l->spells_known[i]].class_level[l->class]]--;
  }


  l->feat_points = GET_FEAT_POINTS(ch) + (isepic ? 0 : (l->level % 2 == 1 ? 1 : 0));
  l->practices = 0;
  if (l->level == 1) 
  {
    l->feat_points++;
  }
  l->epic_feat_points = GET_EPIC_FEAT_POINTS(ch) + (isepic ? (l->level % 3 == 0 ? 1 : 0) : 0);
  for (i = 0; i < NUM_FEATS_DEFINED; i++) 
  {
    l->feats[i] = 0;
    l->feat_skills[i] = 0;
    l->feat_weapons[i] = 0;
  }
  l->num_trains = GET_TRAINS(ch) + (l->level % 4 == 0 ? 1 : 0);
  l->practices += GET_PRACTICES(ch, l->class) + num_levelup_practices(ch, l->class);
  for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) 
  {
    l->skills[i] = 0;
  }
  l->tempFeat = 0;
  for (i = 0; i < 6; i ++)
    l->trains[i] = 0;
  l->num_class_feats = GET_CLASS_FEATS(ch, l->class) + (isepic ? 0 : num_levelup_class_feats(ch, l->class, l->level));
  l->num_epic_class_feats = GET_EPIC_CLASS_FEATS(ch, l->class) + (isepic ? num_levelup_class_feats(ch, l->class, l->level) : 0);
}

ACMD(do_levelup) 
{

  if (!has_unlocked_race(ch, GET_RACE(ch))) 
  {
    send_to_char(ch, "You cannot gain more levels on this character until you've unlocked your current race.  See @YHELP ACCOUNT EXPERIENCE@n.\r\n");
    return;
  }

  if (STATE(ch->desc) == CON_PLAYING &&
      (GET_CLASS_LEVEL(ch) == 0 || (GET_LEVEL(ch) < (CONFIG_LEVEL_CAP - 1) && GET_EXP(ch) >= 
      level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch))))) 
  {
	send_to_char(ch, "Proceeding to level-up screen. Press enter to continue.\r\n\r\n");
	STATE(ch->desc) = CON_LEVELUP_START;
  } 
  else 
  {
    send_to_char(ch, "You are not yet ready for further advancement.\r\n");
  }

}

void display_levelup_trains(struct char_data *ch) 
{

	send_to_char(ch,
	"Ability Trains Remaining: %d\r\n"
	"\r\n"
	"1) Strength     : %d\r\n"
	"2) Dexterity    : %d\r\n"
	"3) Constitution : %d\r\n"
	"4) Intelligence : %d\r\n"
	"5) Wisdom       : %d\r\n"
	"6) Charisma     : %d\r\n"
	"\r\n"
	"Please select the ability that you wish to increase, or type -1 to continue: ",

	ch->levelup->num_trains,
	ch->real_abils.str + ch->levelup->trains[0],
	ch->real_abils.dex + ch->levelup->trains[1],
	ch->real_abils.con + ch->levelup->trains[2],
	ch->real_abils.intel + ch->levelup->trains[3],
	ch->real_abils.wis + ch->levelup->trains[4],
	ch->real_abils.cha + ch->levelup->trains[5]
	);

}

void display_levelup_summary(struct char_data *ch) {

	display_levelup_changes(ch, FALSE);

	send_to_char(ch,
			"\r\n"
			"Do you wish to accept these changes?\r\n"
			"Type yes to accept or no to cancel and start over.\r\n"
			);

}


void display_levelup_changes(struct char_data *ch, int apply_changes) {

	if (ch == NULL || ch->levelup == NULL) {
	  return;
	}

	int i = 0;

	send_to_char(ch,
			"\r\n"
			"Your Level Summary:\r\n"
			"\r\n"
			"Skills:\r\n"
			"\r\n"
			);

	for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
		if (ch->levelup && ch->levelup->skills[i] > 0) {
			send_to_char(ch, "%s raised from %d to %d.\r\n", spell_info[i].name, GET_SKILL(ch, i), GET_SKILL(ch, i) + ch->levelup->skills[i]);
			if (apply_changes)
				GET_SKILL_BASE(ch, i) += ch->levelup->skills[i];
		}
	}

	send_to_char(ch,
			"\r\n"
			"Feats:\r\n"
			"\r\n"
			);

	for (i = 0; i < NUM_FEATS_DEFINED; i++) {
		if (ch->levelup->feats[i] > 0) {
			send_to_char(ch, "%s", feat_list[i].name);
			if (apply_changes)
                          process_add_feat(ch, i);

			switch (i) {
			case FEAT_IMPROVED_CRITICAL:
			case FEAT_WEAPON_FINESSE:
			case FEAT_WEAPON_FOCUS:
			case FEAT_WEAPON_SPECIALIZATION:
			case FEAT_GREATER_WEAPON_FOCUS:
			case FEAT_GREATER_WEAPON_SPECIALIZATION:
			case FEAT_IMPROVED_WEAPON_FINESSE:
			case FEAT_WEAPON_PROFICIENCY_EXOTIC:
			case FEAT_MONKEY_GRIP:
			case FEAT_CRITICAL_FOCUS:
			case FEAT_WEAPON_MASTERY:
			case FEAT_WEAPON_FLURRY:
			case FEAT_WEAPON_SUPREMACY:

				if (apply_changes)
					SET_COMBAT_FEAT(ch, feat_to_subfeat(i), ch->levelup->feat_weapons[i]);
				send_to_char(ch, " (%s)", weapon_damage_types[ch->levelup->feat_weapons[i]-MIN_WEAPON_DAMAGE_TYPES]);
				break;
			case FEAT_SKILL_FOCUS:
			case FEAT_EPIC_SKILL_FOCUS:
				if (apply_changes)
					ch->player_specials->skill_focus[ch->levelup->feat_skills[i] - SKILL_LOW_SKILL] += 1;
				send_to_char(ch, " (%s)", spell_info[ch->levelup->feat_skills[i]].name);
				break;
			}
			send_to_char(ch, "\r\n");
		}
	}

	send_to_char(ch,
			"\r\n"
			"Ability Score Trains:\r\n"
			"\r\n"
			);

	for (i = 0; i < 6; i++) {
		if (ch->levelup->trains[i] > 0) {
			switch (i) {
			case 0:
				if (apply_changes)
					ch->real_abils.str += ch->levelup->trains[i];
				send_to_char(ch, "Strength raised from %d to %d.\r\n", ch->real_abils.str, ch->real_abils.str + ch->levelup->trains[i]);
				break;
			case 1:
				if (apply_changes)
					ch->real_abils.dex += ch->levelup->trains[i];
				send_to_char(ch, "Dexterity raised from %d to %d.\r\n", ch->real_abils.dex, ch->real_abils.dex + ch->levelup->trains[i]);
				break;
			case 2:
				if (apply_changes)
					ch->real_abils.con += ch->levelup->trains[i];
				send_to_char(ch, "Constitution raised from %d to %d.\r\n", ch->real_abils.con, ch->real_abils.con + ch->levelup->trains[i]);
				break;
			case 3:
				if (apply_changes)
					ch->real_abils.intel += ch->levelup->trains[i];
				send_to_char(ch, "Intelligence raised from %d to %d.\r\n", ch->real_abils.intel, ch->real_abils.intel + ch->levelup->trains[i]);
				break;
			case 4:
				if (apply_changes)
					ch->real_abils.wis += ch->levelup->trains[i];
				send_to_char(ch, "Wisdom raised from %d to %d.\r\n", ch->real_abils.wis, ch->real_abils.wis + ch->levelup->trains[i]);
				break;
			case 5:
				if (apply_changes)
					ch->real_abils.cha += ch->levelup->trains[i];
				send_to_char(ch, "Charisma raised from %d to %d.\r\n", ch->real_abils.cha, ch->real_abils.cha + ch->levelup->trains[i]);
				break;

			}
		}
	}

  if (apply_changes) {

    // MySQL Save

    char query[MAX_INPUT_LENGTH]={'\0'};

    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
      log("Cannot connect to mysql database in display_levelup_changes.");
    }

    sprintf(query, "INSERT INTO player_levels (idnum,char_name,char_level,char_class) VALUES('%ld','%s','%d','%s')", GET_IDNUM(ch), GET_NAME(ch), ch->levelup->level, class_names_dl_aol[ch->levelup->class]);
    if (mysql_query(conn, query)) {
      log("Mysql Query Error in display_levelup_changes: %s", query);
    }

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;

    sprintf(query, "SELECT id FROM player_levels WHERE idnum='%ld' AND char_name='%s' ORDER BY char_level DESC LIMIT 1", GET_IDNUM(ch), GET_NAME(ch));
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL) {
      if ((row = mysql_fetch_row(res)) != NULL) {
        if (atoi(row[0]) > 0) {
          send_to_char(ch, "@YPost this on your facebook wall!  http://www.ageofdragons.org/fblevelup.php?id=%s\r\n@n", row[0]);
        }
      }
    }
    mysql_free_result(res);


    mysql_close(conn);
  }
}

void display_combat_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  write_to_output(d, "\r\n"
                     "@CCOMBAT MENU:@n\r\n"
                     "\r\n"
                     "@C1)@n Attack Target       @C2)@n Approach Target\r\n"
                     "@C3)@n Retreat             @C4)@n Custom Command (Incl. Spells)\r\n"
                     "@CQ)@n End Your Turn\r\n"
                     "\r\n"
                     "Actions: Standard (%d) Move (%d) Minor(Not Yet Implemented)\r\n"
                     "\r\n"
                     "Your Choice: ",
                     1 - ch->standard_action_spent, 1 - ch->move_action_spent);
}

void process_add_feat(struct char_data *ch, int feat_num)
{

  int subval = 0;
  struct damreduct_type *dptr, *reduct, *temp;

  switch (feat_num) {
  case FEAT_GREAT_FORTITUDE:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_FORTITUDE) += 2;
    break;
  case FEAT_IRON_WILL:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_WILL) += 2;
    break;
  case FEAT_LIGHTNING_REFLEXES:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_REFLEX) += 2;
    break;
  case FEAT_TOUGHNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_MAX_HIT(ch) += GET_LEVEL(ch);
    break;
  case FEAT_ENDURANCE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_MAX_MOVE(ch) += GET_LEVEL(ch) * 10;
    break;
  case FEAT_EXTRA_RAGE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_SPELL_MASTERY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_SPELL_MASTERY_POINTS(ch) += MAX(1, ability_mod_value(GET_INT(ch)));
    break;
  case FEAT_ACROBATIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_ACROBATICS, GET_SKILL_BONUS(ch, SKILL_ACROBATICS) + 2);
    break;
  case FEAT_ALERTNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_PERCEPTION, GET_SKILL_BONUS(ch, SKILL_PERCEPTION) + 2);
    SET_SKILL_BONUS(ch, SKILL_SENSE_MOTIVE, GET_SKILL_BONUS(ch, SKILL_SENSE_MOTIVE) + 2);
    break;
  case FEAT_ANIMAL_AFFINITY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HANDLE_ANIMAL, GET_SKILL_BONUS(ch, SKILL_HANDLE_ANIMAL) + 2);
    SET_SKILL_BONUS(ch, SKILL_RIDE, GET_SKILL_BONUS(ch, SKILL_RIDE) + 2);
    break;
  case FEAT_DECEITFUL:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_DISGUISE, GET_SKILL_BONUS(ch, SKILL_DISGUISE) + 2);
    SET_SKILL_BONUS(ch, SKILL_LINGUISTICS, GET_SKILL_BONUS(ch, SKILL_LINGUISTICS) + 2);
    break;
  case FEAT_DEFT_HANDS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND, GET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND) + 2);
    SET_SKILL_BONUS(ch, SKILL_DISABLE_DEVICE, GET_SKILL_BONUS(ch, SKILL_DISABLE_DEVICE) + 2);
    break;
  case FEAT_MAGICAL_APTITUDE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_SPELLCRAFT, GET_SKILL_BONUS(ch, SKILL_SPELLCRAFT) + 2);
    SET_SKILL_BONUS(ch, SKILL_USE_MAGIC_DEVICE, GET_SKILL_BONUS(ch, SKILL_USE_MAGIC_DEVICE) + 2);
    break;
  case FEAT_PERSUASIVE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_BLUFF, GET_SKILL_BONUS(ch, SKILL_DIPLOMACY) + 2);
    SET_SKILL_BONUS(ch, SKILL_INTIMIDATE, GET_SKILL_BONUS(ch, SKILL_INTIMIDATE) + 2);
    break;
  case FEAT_SELF_SUFFICIENT:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HEAL, GET_SKILL_BONUS(ch, SKILL_HEAL) + 2);
    SET_SKILL_BONUS(ch, SKILL_SURVIVAL, GET_SKILL_BONUS(ch, SKILL_SURVIVAL) + 2);
    break;
  case FEAT_STEALTHY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_STEALTH, GET_SKILL_BONUS(ch, SKILL_STEALTH) + 2);
    SET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST, GET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST) +2);
    break;
  case FEAT_DAMAGE_REDUCTION:
    if (ch->damage_reduction_feats == 5) {
      send_to_char(ch, "You can only take the damage reduction feat 5 times.\r\n");
      return;
    }
    ch->damage_reduction_feats++;
    subval = HAS_FEAT(ch, feat_num) + 3;
    SET_FEAT(ch, feat_num, subval);
    for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
      if (reduct->feat == FEAT_DAMAGE_REDUCTION) {
        REMOVE_FROM_LIST(reduct, ch->damreduct, next);
      }
    }
    CREATE(dptr, struct damreduct_type, 1);
    dptr->next = ch->damreduct;
    ch->damreduct = dptr;
    dptr->spell = 0;
    dptr->feat = FEAT_DAMAGE_REDUCTION;
    dptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION);
    dptr->duration = -1;
    dptr->max_damage = -1;
    int q = 0;
    for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
      dptr->damstyle[q] = dptr->damstyleval[q] = 0;
    dptr->damstyle[0] = DR_NONE;
    break;
  case FEAT_FAST_HEALING:
    if (ch->fast_healing_feats == 5) {
      send_to_char(ch, "You can only take the fast healing feat 5 times.\r\n");
      return;
    }
    ch->fast_healing_feats++;
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_SNEAK_ATTACK:
    if (ch->sneak_attack_feats == 5) {
      send_to_char(ch, "You can only take the sneak attack feat 5 times.\r\n");
      return;
    }
    ch->sneak_attack_feats++;
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_IMPROVED_SNEAK_ATTACK:
    if (ch->imp_sneak_attack_feats == 5) {
      send_to_char(ch, "You can only take the improved sneak attack feat 5 times.\r\n");
      return;
    }
    ch->imp_sneak_attack_feats++;
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_ARMOR_SKIN:
    if (ch->armor_skin_feats == 5) {
      send_to_char(ch, "You can only take the armor skin feat 5 times.\r\n");
      return;
    }
    ch->armor_skin_feats++;
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  default:
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  }
}
