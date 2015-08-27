/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/*
 * this file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  if you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "deities.h"
#include "pets.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "feats.h"
#include "oasis.h"
#include "quest.h"

extern void racial_ability_modifiers(struct char_data *ch);
extern void set_height_and_weight_by_race(struct char_data *ch);
extern struct spell_info_type spell_info[];
extern struct race_data race_list[NUM_RACES];
extern int bard_spells_known_table[21][10];

/* local functions */
int mob_exp_by_level(int level);
void snoop_check(struct char_data *ch);
int parse_class(struct char_data *ch, char arg);
int num_attacks(struct char_data *ch, int offhand);
void roll_real_abils(struct char_data *ch);
void do_start(struct char_data *ch);
int backstab_dice(struct char_data *ch);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int level, int race);
byte object_saving_throws(int material_type, int type);
int load_levels();
void assign_auto_stats(struct char_data *ch);
void cedit_creation(struct char_data *ch);
void display_alignments(struct descriptor_data *d);
void display_alignment_help(struct descriptor_data *d);
void parse_alignment(struct char_data *ch, char choice);
int get_saving_throw(int chclass, int savetype);
int num_levelup_class_feats(struct char_data *ch, int whichclass, int ranks);
int do_class_ok_general(struct char_data *ch, int whichclass, int show_text);
void do_advance_level(struct char_data *ch, int whichclass, int manual);
int num_levelup_practices(struct char_data *ch, int whichclass);

// external functions

void perform_cinfo( int clan_number, const char *messg, ... );
int findslotnum(struct char_data *ch, int spelllvl);
int calculate_max_hit(struct char_data *ch);
void display_levelup_summary(struct char_data *ch);
void display_levelup_changes(struct char_data *ch, int apply_changes);


int *free_start_feats[];


/* Names first */
const char *class_abbrevs_core[] = {
  "Wiz",
  "Cle",
  "Rog",
  "Fig",
  "Mon",
  "Pal",
  "Brb",
  "Brd",
  "Rng",
  "Dru",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Due",
  "Exp",
  "MTh",
  "Sor",
  "Cls",
  "Exp",
  "Exp",
  "DDs",
  "AAr",
  "Exp",  
  "Npc",
  "Asn",
  "Exp",
  "Art",
  "ATr",
  "Exp",
  "EKn",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "\n",
  NULL
};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *pc_class_types_core[] = 
{
  "Wizard",
  "Cleric",
  "Rogue",
  "Fighter",
  "Monk",
  "Paladin",
  "Barbarian",
  "Bard",
  "Ranger",
  "Druid",
  "Expansion 1",
  "Expansion 2",
  "Expansion 3",
  "Expansion 4",
  "Expansion 5",
  "Expansion 6",  
  "Expansion 7",
  "Duelist",
  "Expansion 8",
  "Mystic Theurge",
  "Sorcerer",
  "Classless",
  "Expansion 9",
  "Expansion 10",
  "Dragon Disciple",
  "Arcane Archer",
  "Expansion 11",  
  "Npc",
  "Assassin",
  "Expansion 12",
  "Artisan",
  "Arcane Trickster",
  "Expansion 13",
  "Eldritch Knight",
  "Expansion 14",
  "Expansion 15",
  "Expansion 16",
  "Expansion 17",
  "Expansion 18",
  "Expansion 19",
  "Expansion 20",
  "Expansion 21",
  "Expansion 22",
  "Expansion 23",
  "Expansion 24",
  "Expansion 25",
  "Expansion 26",
  "Expansion 27",
  "Expansion 28",
  "Expansion 29",
  "Expansion 30",
  "Expansion 31",
  "Expansion 32",
  "Expansion 33",
  "Expansion 34",
  "Expansion 35",
  "\n",
  NULL
};
/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *class_names_core[] = {
  "wizard",
  "cleric",
  "rogue",
  "fighter",
  "monk",
  "paladin",
  "barbarian",
  "bard",
  "ranger",
  "druid",
  "expansion 1",
  "expansion 2",
  "expansion 3",
  "expansion 4",
  "expansion 5",
  "expansion 6",
  "expansion 7",
  "duelist",
  "expansion 8",
  "mystic theurge",
  "sorcerer",
  "classless",
  "expansion 9",
  "expansion 10",
  "dragon disciple",
  "arcane archer",
  "expansion 11",  
  "npc",
  "assassin",
  "expansion 12",
  "artisan",
  "arcane trickster",
  "expansion 13",
  "eldritch knight",
  "expansion 14",
  "expansion 15",
  "expansion 16",
  "expansion 17",
  "expansion 18",
  "expansion 19",
  "expansion 20",
  "expansion 21",
  "expansion 22",
  "expansion 23",
  "expansion 24",
  "expansion 25",
  "expansion 26",
  "expansion 27",
  "expansion 28",
  "expansion 29",
  "expansion 30",
  "expansion 31",
  "expansion 32",
  "expansion 33",
  "expansion 34",
  "expansion 35",
  "\n",
  NULL
};
const char *class_abbrevs_dl_aol[] = {
  "Mag",
  "Cle",
  "Rog",
  "Fig",
  "Mon",
  "Pal",
  "Brb",
  "Brd",
  "Rng",
  "Dru",
  "KCr",
  "KSw",
  "KRo",
  "KLi",
  "KSk",
  "KTh",
  "Wiz",
  "Due",
  "Gla",
  "MTh",
  "Sor",
  "Cls",
  "Def",
  "WMs",
  "DDs",
  "AAr",  
  "Ibl",
  "Npc",
  "Asn",
  "BGd",
  "Art",
  "ATr",
  "FSo",
  "EKn",
  "PMa",
  "SFi",
  "DrR",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "Exp",
  "\n",
  NULL
};
const char *pc_class_types_dl_aol[] = {
  "Mage",
  "Cleric",
  "Rogue",
  "Fighter",
  "Monk",
  "Paladin",
  "Barbarian",
  "Bard",
  "Ranger",
  "Druid",
  "Knight of the Crown",
  "Knight of the Sword",
  "Knight of the Rose",
  "Knight of the Lily",
  "Knight of the Skull",
  "Knight of the Thorn",  
  "Wizard of High Sorcery",
  "Duelist",
  "Gladiator",
  "Mystic Theurge",
  "Sorcerer",
  "Classless",
  "Dwarven Defender",
  "Weapon Master",
  "Dragon Disciple",
  "Arcane Archer",
  "Invisible Blade",  
  "Npc",
  "Assassin",
  "Blackguard",
  "Artisan",
  "Arcane Trickster",
  "Favored Soul",
  "Eldritch Knight",
  "Pale Master",
  "Sacred Fist",
  "Dragon Rider",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "Expansion",
  "\n",
  NULL
};
const char *class_names_dl_aol[] = {
  "mage",
  "cleric",
  "rogue",
  "fighter",
  "monk",
  "paladin",
  "barbarian",
  "bard",
  "ranger",
  "druid",
  "knight of the crown",
  "knight of the sword",
  "knight of the rose",
  "knight of the lily",
  "knight of the skull",
  "knight of the thorn",
  "wizard of high sorcery",
  "duelist",
  "gladiator",
  "mystic theurge",
  "sorcerer",
  "classless",
  "dwarven defender",
  "weapon master",
  "dragon disciple",
  "arcane archer",
  "invisible blade",  
  "npc",
  "assassin",
  "blackguard",
  "artisan",
  "arcane trickster",
  "favored soul",
  "eldritch knight",
  "pale master",
  "sacred fist",
  "dragon rider",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "expansion",
  "\n",
  NULL
};

#define Y   true
#define N   false

int class_ok_available_core[NUM_CLASSES] = {
/* W, C, R, F, M, P, B, B, R, D, X, S, R, L, 
     S, T, W, D, G, M, S, C, D, W, D, A, I, N */ 

   Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y,
     Y, Y, N, Y, N, Y, Y, N, Y, Y, Y, Y, N, N,

/* A, B, A, A, F, E, D, X, X, X, X, X, X, X,
     X, X, X, X, X, X, X, X, X, X, X, X, X, X */ 

   Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
     N, N, N, N, N, N, N, N, N, N, N, N, N, N
};

int class_ok_available_dl_aol[NUM_CLASSES] = {
/* W, C, R, F, M, P, B, B, R, D, C, S, R, L, 
     S, T, W, D, G, M, S, C, D, W, D, A, I, N */ 

   Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y,
     Y, Y, N, Y, N, Y, Y, N, Y, Y, Y, Y, N, N,

/* A, B, A, A, F, E, D, S, D, X, X, X, X, X,
     X, X, X, X, X, X, X, X, X, X, X, X, X, X */ 

   Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N,
     N, N, N, N, N, N, N, N, N, N, N, N, N, N
};

int class_ok_available_dl_aol_old[NUM_CLASSES] = {
/* W, C, R, F, M, P, B, B, R, D, C, S, R, L, 
     S, T, W, D, G, M, S, N, D, W, D, A, I, N */ 

   Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N,
     N, N, N, Y, N, Y, Y, N, Y, Y, N, Y, Y, N,

/* A, B, A, A, F, E, X, X, X, X, X, X, X, X, 
     X, X, X, X, X, X, X, X, X, X, X, X, X, X */ 

   Y, N, Y, Y, Y, Y, N, N, N, N, N, N, N, N,
     N, N, N, N, N, N, N, N, N, N, N, N, N, N
};


/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int class_ok_align_core[9][NUM_CLASSES] = 
{
/*         M, C, T, F, M, P, B, B, R, D  C  S  R  L
           S  T  W  D  G  M  S  C  D  W  D  A  I  N 
           A  B  A  A  F  E  D  S  D  X  X  X  X  X
           X  X  X  X  X  X  X  X  X  X  X  X  X  X */
/* LG*/  { Y, Y, Y, Y, Y, Y, N, Y, Y, N, Y, Y, Y, N,
           N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* NG*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, N,
           N, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CG*/  { Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, N, N,
           N, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* LN*/  { Y, Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* TN*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CN*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* LE*/  { Y, Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* NE*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CE*/  { Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
};

int class_ok_align_dl_aol[9][NUM_CLASSES] = {
/*         M, C, T, F, M, P, B, B, R, D  C  S  R  L
           S  T  W  D  G  M  S  C  D  W  D  A  I  N 
           A  B  A  A  F  E  D  S  D  X  X  X  X  X
           X  X  X  X  X  X  X  X  X  X  X  X  X  X */
/* LG*/  { Y, Y, Y, Y, Y, Y, N, Y, Y, N, Y, Y, Y, N,
           N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, Y, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* NG*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, N,
           N, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, N, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CG*/  { Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, N, N,
           N, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, N, N, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* LN*/  { Y, Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* TN*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CN*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, N, N, N, N, N, N,
           N, N, N, N, N, N, N, N, Y, N, N, N, N, N },
/* LE*/  { Y, Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* NE*/  { Y, Y, Y, Y, N, N, Y, Y, Y, Y, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* CE*/  { Y, Y, Y, Y, N, N, Y, Y, Y, N, N, N, N, Y,
           Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, N, N,
           Y, N, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N,
           N, N, N, N, N, N, N, N, N, N, N, N, N, N },
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int prestige_classes_core[NUM_CLASSES] = 
{
/* WIZARD	*/ N,
/* CLERIC	*/ N,
/* ROGUE	*/ N,
/* FIGHTER	*/ N,
/* MONK		*/ N,
/* PALADIN	*/ N,
/* BARBARIAN*/ N,
/* BARD     */ N,
/* RANGER   */ N,
/* DRUID    */ N,
/* EXPANSION 1 */ Y,
/* EXPANSION 2 */ Y,
/* EXPANSION 3 */ Y,
/* EXPANSION 4 */ Y,
/* EXPANSION 5 */ Y,
/* EXPANSION 6 */ Y,
/* EXPANSION 7 */ Y,
/* DUELIST  */ Y,
/* EXPANSION 8 */ Y,
/* MYSTIC TH*/ Y,
/* SORCERER */ N,
/* CLASSLESS*/ N,
/* EXPANSION 9 */ Y,
/* EXPANSION 10*/ Y,
/* DISCIPLE */ Y,
/* ARC ARCH */ Y,
/* EXPANSION 11 */ Y,
/* NPC      */ N,
/* ASSASSIN */ Y,
/* EXPANSION 12*/ Y,
/* ARTISAN  */ N,
/* ARCANE TR*/ Y,
/* EXPANSION 13 */ Y,
/* EL KNIGHT*/ Y,
/* EXPANSION 14*/ Y,
/* EXPANSION 15*/ Y,
/* EXPANSION 16*/ Y,
/* EXPANSION 17 */ Y,
/* EXPANSION 18 */ Y,
/* EXPANSION 19 */ Y,
/* EXPANSION 20 */ Y,
/* EXPANSION 21 */ Y,
/* EXPANSION 22 */ Y,
/* EXPANSION 23 */ Y,
/* EXPANSION 24 */ Y,
/* EXPANSION 25 */ Y,
/* EXPANSION 26 */ Y,
/* EXPANSION 27 */ Y,
/* EXPANSION 28 */ Y,
/* EXPANSION 29 */ Y,
/* EXPANSION 30 */ Y,
/* EXPANSION 31 */ Y,
/* EXPANSION 32 */ Y,
/* EXPANSION 33 */ Y,
/* EXPANSION 34 */ Y,
/* EXPANSION 35 */ Y,
  0
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int prestige_classes_dl_aol[NUM_CLASSES] = {
/* MAGE  	*/ N,
/* CLERIC	*/ N,
/* ROGUE	*/ N,
/* FIGHTER	*/ N,
/* MONK		*/ N,
/* PALADIN	*/ N,
/* BARBARIAN*/ N,
/* BARD     */ N,
/* RANGER   */ N,
/* DRUID    */ N,
/* CROWN    */ Y,
/* SWORD    */ Y,
/* ROSE     */ Y,
/* LILY     */ Y,
/* SKULL    */ Y,
/* THORN    */ Y,
/* WIZARD   */ Y,
/* DUELIST  */ Y,
/* GLADIATOR*/ Y,
/* MYSTIC TH*/ Y,
/* SORCERER */ N,
/* CLASSLESS*/ N,
/* DEFENDER */ Y,
/* WEAP MAST*/ Y,
/* DISCIPLE */ Y,
/* ARC ARCH */ Y,
/* INVIS BLD*/ N,
/* NPC      */ N,
/* ASSASSIN */ Y,
/* BLCKGUARD*/ N,
/* ARTISAN  */ N,
/* ARCANE TR*/ Y,
/* FAV SOUL */ N,
/* EL KNIGHT*/ Y,
/* PALE MAS*/ Y,
/* SACRED_FS*/ Y,
/* DRAGON_RD*/ Y,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
/* EXPANSION*/ N,
  0
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
/* -1 indicates no limit to the number of levels in this class under
 * epic rules */
int class_max_ranks_core[NUM_CLASSES] = {
/* WIZARD  	*/ -1,
/* CLERIC	*/ -1,
/* ROGUE	*/ -1,
/* FIGHTER	*/ -1,
/* MONK		*/ -1,
/* PALADIN	*/ -1,
/* BARBARIAN*/ -1,
/* BARD     */ -1,
/* RANGER   */ -1,
/* DRUID    */ -1,
/* EXPANSION 1 */ -1,
/* EXPANSION 2    */ 10,
/* EXPANSION 3     */ 10,
/* EXPANSION 4     */ 10,
/* EXPANSION 5     */ 10,
/* EXPANSION 6    */ 10,
/* EXPANSION 7   */ 10,
/* DUELIST  */ 10,
/* EXPANSION 8*/ 10,
/* MYSTIC   */ 10,
/* SORCERER */ -1,
/* CLASSLESS*/ -1,
/* EXPANSION 9 */ 10,
/* EXPANSION 10 */ 10,
/* DISCIPLE */ 10,
/* ARC ARCH */ 10,
/* EXPANSION 11 */ -1,
/* NPC      */ -1,
/* ASSASSIN */ 10,
/* EXPANSION 12 */ -1,
/* ARTISAN  */ -1,
/* ARCANE TR*/ 10,
/* EXPANSION 13 */ -1,
/* EL KNIGHT*/ 10,
/* EXPANSION 14*/ 10,
/* EXPANSION 15*/ -1,
/* EXPANSION 16*/ -1,
/* EXPANSION 17*/ -1,
/* EXPANSION 18*/ -1,
/* EXPANSION 19*/ -1,
/* EXPANSION 20*/ -1,
/* EXPANSION 21*/ -1,
/* EXPANSION 22*/ -1,
/* EXPANSION 23*/ -1,
/* EXPANSION 24*/ -1,
/* EXPANSION 25*/ -1,
/* EXPANSION 26*/ -1,
/* EXPANSION 27*/ -1,
/* EXPANSION 28*/ -1,
/* EXPANSION 29*/ -1,
/* EXPANSION 30*/ -1,
/* EXPANSION 31*/ -1,
/* EXPANSION 32*/ -1,
/* EXPANSION 33*/ -1,
/* EXPANSION 34*/ -1,
/* EXPANSION 35*/ -1,
  0
};

int class_max_ranks_dl_aol[NUM_CLASSES] = {
/* MAGE  	*/ -1,
/* CLERIC	*/ -1,
/* ROGUE	*/ -1,
/* FIGHTER	*/ -1,
/* MONK		*/ -1,
/* PALADIN	*/ -1,
/* BARBARIAN*/ -1,
/* BARD     */ -1,
/* RANGER   */ -1,
/* DRUID    */ -1,
/* CROWN    */ 10,
/* SWORD    */ 10,
/* ROSE     */ 10,
/* LILY     */ 10,
/* SKULL    */ 10,
/* THORN    */ 10,
/* WIZARD   */ 10,
/* DUELIST  */ 10,
/* GLADIATOR*/ 10,
/* MYSTIC   */ 10,
/* SORCERER */ -1,
/* CLASSLESS*/ -1,
/* DEFENDER */ 10,
/* WEAP MAST*/ 10,
/* DISCIPLE */ 10,
/* ARC ARCH */ 10,
/* EXPANSION*/ -1,
/* NPC      */ -1,
/* ASSASSIN */ 10,
/* BLCKGUARD*/ -1,
/* ARTISAN  */ -1,
/* ARCANE TR*/ 10,
/* FAV SOUL */ -1,
/* EL KNIGHT*/ 10,
/* DEATHMAST*/ 10,
/* SACRED_FS*/ 10,
/* DRAGON_RD*/ 10,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
/* EXPANSION*/ -1,
  0
};


int class_in_game_core[NUM_CLASSES] = {
/* WIZARD	*/  TRUE,
/* CLERIC	*/  TRUE,
/* ROGUE	*/  TRUE,
/* FIGHTER */  TRUE,
/* MONK	*/  TRUE,
/* PALADIN	*/  TRUE,
/* BARBARIAN*/  TRUE,
/* BARD     */    TRUE,
/* RANGER   */   TRUE,
/* DRUID    */   TRUE,
/* EXPANSION 1 */ FALSE,
/* EXPANSION 2 */   FALSE,
/* EXPANSION 3 */    FALSE,
/* EXPANSION 4 */     FALSE,
/* EXPANSION 5 */    FALSE,
/* EXPANSION 6 */   FALSE,
/* EXPANSION 7 */   FALSE,
/* DUELIST  */   FALSE,
/* EXPANSION 8 */  FALSE,
/* MYSTIC TH*/ TRUE,
/* SORCERER */ TRUE,
/* CLASSLESS*/ FALSE,
/* EXPANSION 9 */ FALSE,
/* EXPANSION 10 */ FALSE,
/* DISCIPLE */ TRUE,
/* ARC ARCH */ TRUE,
/* EXPANSION 11 */ FALSE,
/* NPC      */ FALSE,
/* ASSASSIN */ TRUE,
/* EXPANSION 12 */ FALSE,
/* ARTISAN  */ TRUE,
/* ARCANE TR*/ TRUE,
/* EXPANSION 13 */ FALSE,
/* EL KNIGHT*/ TRUE,
/* EXPANSION 14 */ FALSE,
/* EXPANSION 15 */ FALSE,
/* EXPANSION 16 */ FALSE,
/* EXPANSION 17 */ FALSE,
/* EXPANSION 18 */ FALSE,
/* EXPANSION 19 */ FALSE,
/* EXPANSION 20 */ FALSE,
/* EXPANSION 21 */ FALSE,
/* EXPANSION 22 */ FALSE,
/* EXPANSION 23 */ FALSE,
/* EXPANSION 24 */ FALSE,
/* EXPANSION 25 */ FALSE,
/* EXPANSION 26 */ FALSE,
/* EXPANSION 27 */ FALSE,
/* EXPANSION 28 */ FALSE,
/* EXPANSION 29 */ FALSE,
/* EXPANSION 30 */ FALSE,
/* EXPANSION 31 */ FALSE,
/* EXPANSION 32 */ FALSE,
/* EXPANSION 33 */ FALSE,
/* EXPANSION 34 */ FALSE,
/* EXPANSION 35 */ FALSE,
  0
};

int class_in_game_dl_aol[NUM_CLASSES] = {
/* MAGE  	*/  TRUE,
/* CLERIC	*/  TRUE,
/* ROGUE	*/  TRUE,
/* FIGHTER	*/  TRUE,
/* MONK	*/  TRUE,
/* PALADIN	*/  TRUE,
/* BARBARIAN*/  TRUE,
/* BARD     */    TRUE,
/* RANGER   */   TRUE,
/* DRUID    */   TRUE,
/* CROWN    */   TRUE,
/* SWORD    */   TRUE,
/* ROSE     */    TRUE,
/* LILY     */   FALSE,
/* SKULL    */   FALSE,
/* THORN    */   FALSE,
/* WIZARD   */   FALSE,
/* DUELIST  */   TRUE,
/* GlADIATOR*/  FALSE,
/* MYSTIC TH*/ TRUE,
/* SORCERER */ TRUE,
/* CLASSLESS*/ FALSE,
/* DEFENDER */ TRUE,
/* WEAP_MAST*/ TRUE,
/* DISCIPLE */ TRUE,
/* ARC ARCH */ TRUE,
/* INVIS BLD*/ FALSE,
/* NPC      */ FALSE,
/* ASSASSIN */ TRUE,
/* BLCKGRD  */ FALSE,
/* ARTISAN  */ FALSE,
/* ARCANE TR*/ TRUE,
/* FAV_SOUL */ TRUE,
/* EL KNIGHT*/ TRUE,
/* DEATHMAST*/ TRUE,
/* SACRED_FS*/ TRUE,
/* DRAGON_RD*/ TRUE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
/* EXPANSION*/ FALSE,
  0
};

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */
int parse_class(struct char_data *ch, char arg)
{
  int chclass = CLASS_UNDEFINED;
  switch (arg) 
  {
  case 'A': case 'a': chclass = CLASS_WIZARD; break;
  case 'B': case 'b': chclass = CLASS_CLERIC; break;
  case 'C': case 'c': chclass = CLASS_ROGUE; break;
  case 'D': case 'd': chclass = CLASS_FIGHTER; break;
  case 'E': case 'e': chclass = CLASS_MONK; break;
  case 'F': case 'f': chclass = CLASS_PALADIN; break;
  case 'G': case 'g': chclass = CLASS_BARBARIAN; break;
  case 'H': case 'h': chclass = CLASS_BARD; break;
  case 'I': case 'i': chclass = CLASS_RANGER; break;
  case 'J': case 'j': chclass = CLASS_DRUID; break;
  case 'L': case 'l': chclass = CLASS_SORCERER; break;
  default:  chclass = CLASS_UNDEFINED; break;

  }
  if (chclass >= 0 && chclass < NUM_CLASSES)
    if (!class_ok_general(ch, chclass))
      chclass = CLASS_UNDEFINED;
  return (chclass);
}

int class_ok_num_classes(struct char_data *ch, int whichclass) 
{

  return TRUE;

  if (GET_ADMLEVEL(ch) > 0)
    return TRUE;

  if (whichclass == CLASS_ARTISAN)
    return TRUE;

  int i = 0;
  int num_classes = 0;
  int found = FALSE;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (GET_CLASS_RANKS(ch, i) > 0 && i != CLASS_ARTISAN) {
      num_classes++;
      if (i == whichclass)
        found = TRUE;
    }
  }

  if (num_classes > 3 && !found)
    return FALSE;

  return TRUE;

}

int class_ok_general(struct char_data *ch, int whichclass) 
{
	return do_class_ok_general(ch, whichclass, TRUE);
}

/*
 * Is anything preventing this character from advancing in this class?
 */
int do_class_ok_general(struct char_data *ch, int whichclass, int show_text)
{
  int c1 = FALSE, c2 = FALSE, c3 = FALSE, c4 = FALSE, c5 = FALSE, c6 = FALSE,
      c7 = FALSE, c8 = FALSE, c9 = FALSE, c10 = FALSE, c11 = FALSE;
  if (whichclass < 0 || whichclass >= NUM_CLASSES) {
    log("Invalid class %d in class_ok_general", whichclass);
    return 0;
  }
  if (!class_ok_available_dl_aol[whichclass])
    return -4;
//  if (!class_ok_race[(int)GET_RACE(ch)][whichclass])
//    return -1;
  if (!(CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_ok_align_dl_aol : class_ok_align_core)[ALIGN_TYPE(ch)][whichclass])
    return -2;
  if (!IS_EPIC(ch) && (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_max_ranks_dl_aol : class_max_ranks_core)[whichclass] > -1 &&
      GET_CLASS_RANKS(ch, whichclass) >= (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_max_ranks_dl_aol : class_max_ranks_core)[whichclass])
    return -3;
  if (!class_ok_num_classes(ch, whichclass))
    return -5;
/*
  if (prestige_classes_dl_aol[whichclass] && GET_CLASS_SPONSOR(ch, whichclass) == FALSE)
	  return -6;
*/
  switch (whichclass) {
    case CLASS_GLADIATOR:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
	return (1);

  case CLASS_FAVORED_SOUL:
    if (GET_DEITY(ch) == 0) {
      if (show_text)
      send_to_char(ch, "You must choose a deity before you can become a favored soul.\r\n");
      return 0;
    }
    return 1;

case CLASS_DRAGON_RIDER:
    if (GET_LEVEL(ch) < 1)
      return (0);

    if (GET_SKILL(ch, SKILL_HANDLE_ANIMAL) < 30) {
    	if (show_text) send_to_char(ch, "You must have at least 30 ranks in the handle animal skill to become a dragon rider.\r\n");
    } else {
      c1 = TRUE;
    }

    if (GET_SKILL(ch, SKILL_RIDE) < 30) {
    	if (show_text) send_to_char(ch, "You must have at least 30 ranks in the ride skill to become a dragon rider.\r\n");
    } else {
      c2 = TRUE;
    }

    if (c1 && c2)
      return TRUE;
    return FALSE;

case CLASS_SACRED_FIST:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_UNARMED_STRIKE)) {
    	if (show_text) send_to_char(ch, "You must have the improved unarmed strike feat to become a sacred fist.\r\n");
    } else {
      c1 = TRUE;
    }

    if (GET_SKILL(ch, SKILL_ACROBATICS) < 5) {
    	if (show_text) send_to_char(ch, "You must have at least 5 ranks in the acrobatics skill to become a sacred fist.\r\n");
    } else {
      c2 = TRUE;
    }

    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    if (findslotnum(ch, 3) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    if (findslotnum(ch, 3) > 0)
      c7 = TRUE;
    if (!c4 && !c7)
    	if (show_text) send_to_char(ch, "You must be able to cast 3rd level divine spells to become a sacred.\r\n");
    if (c1 && c2 && (c4 || c7))
      return TRUE;
    return FALSE;

case CLASS_DEATH_MASTER:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_SKILL(ch, SKILL_KNOWLEDGE) < 8) {
    	if (show_text) send_to_char(ch, "You must have a knowledge skill of at least 8 to become a pale master.\r\n");
    } else {
      c1 = TRUE;
    }
    
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 3) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (findslotnum(ch, 3) > 0)
      c7 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 3) > 0)
      c8 = TRUE;
    if (!c4 && !c7 && !c8)
    	if (show_text) send_to_char(ch, "You must be able to cast 3rd level arcane spells to become a pale master.\r\n");
    if (c1 &&  (c4 || c7 || c8))
      return TRUE;
    return FALSE;

  case CLASS_ARCANE_ARCHER:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (!IS_ELF(ch) && !IS_HALF_ELF(ch)) {
    	if (show_text) send_to_char(ch, "You must be an elf or a half elf to become an arcane archer.\r\n");
    } else {
      c1 = TRUE;
    }

    if (GET_BAB(ch) < 6) {
    	if (show_text) send_to_char(ch, "You must have a base attack bonus of 6 or higher to become an arcane archer.\r\n");
    } else {
      c2 = TRUE;
    }

    if (!HAS_REAL_FEAT(ch, FEAT_POINT_BLANK_SHOT)) {
    	if (show_text) send_to_char(ch, "You must have the point blank shot feat to become an arcane archer.\r\n");
    } else {
      c3 = TRUE;
    }
    
    if (!HAS_REAL_FEAT(ch, FEAT_PRECISE_SHOT)) {
    	if (show_text) send_to_char(ch, "You must have the precise shot feat to become an arcane archer.\r\n");
    } else {
      c5 = TRUE;
    }

    if (!HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_LONG_BOW) && !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_COMPOSITE_LONGBOW) &&
        !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_SHORT_BOW) && !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_COMPOSITE_SHORTBOW) &&
        !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_DAMAGE_TYPE_PIERCING)) {     
    	if (show_text) send_to_char(ch, "You must have the weapon focus feat in any bow, or piercing weapons to become an arcane archer.\r\n");
    } else {
      c6 = TRUE;
    }


    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 1) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (findslotnum(ch, 1) > 0)
      c7 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 1) > 0)
      c8 = TRUE;
    if (!c4 && !c7 && !c8)
    	if (show_text) send_to_char(ch, "You must be able to cast 1st level arcane spells to become an arcane archer.\r\n");
    if (c1 && c2 && c3 && c5 && c6 && (c4 || c7 || c8))
      return TRUE;
    return FALSE;

  case CLASS_ARCANE_TRICKSTER:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_SKILL(ch, SKILL_DISABLE_DEVICE) < 7) {
    	if (show_text) send_to_char(ch, "You must have a disable device skill of at least 7 to become an arcane trickster.\r\n");
    } else {
      c1 = TRUE;
    }
    if (GET_SKILL(ch, SKILL_KNOWLEDGE) < 4) {
    	if (show_text) send_to_char(ch, "You must have a knowledge skill of at least 4 to become an arcane trickster.\r\n");
    } else {
      c2 = TRUE;
    }


    if (GET_SKILL(ch, SKILL_LINGUISTICS) < 7) {
    	if (show_text) send_to_char(ch, "You must have a linguistics script skill of at least 7 to become an arcane trickster.\r\n");
    } else {
      c3 = TRUE;
    }
    if (GET_SKILL(ch, SKILL_ESCAPE_ARTIST) < 7) {
    	if (show_text) send_to_char(ch, "You must have an escape artist skill of at least 7 to become an arcane trickster.\r\n");
    } else {
      c5 = TRUE;
    }

    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 3) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (findslotnum(ch, 3) > 0)
      c7 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 3) > 0)
      c8 = TRUE;
    if (!c4 && !c7 && !c8)
      if (show_text) send_to_char(ch, "You must be able to cast 3rd level arcane spells to become an arcane trickster.\r\n");
    if (c1 && c2 && (c4 || c7 || c8) && c3 && c5)
      return TRUE;
    return FALSE;

  case CLASS_ELDRITCH_KNIGHT:
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 3) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (findslotnum(ch, 3) > 0)
      c7 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 3) > 0)
      c8 = TRUE;
    if (!c4 && !c7 && !c8)
      if (show_text) send_to_char(ch, "You must be able to cast 3rd level arcane spells to become an eldritch knight.\r\n");
    if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_MARTIAL))
      c1 = TRUE;
    if (!c1)
      if (show_text) send_to_char(ch, "You must be proficient in the use of martial weapons to become an eldritch knight.\r\n");
    if (c1 && (c4 || c7 || c8))
      return TRUE;
    return FALSE;


  case CLASS_ASSASSIN:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_SKILL(ch, SKILL_DISGUISE) < 4) {
      if (show_text) send_to_char(ch, "You need a disguise skill of at least 4 to become an assassin.\r\n");
    } else {
      c1 = TRUE;
    }
    if (GET_SKILL(ch, SKILL_STEALTH) < 8) {
      if (show_text) send_to_char(ch, "You need a stealth skill of at least 8 to become an assassin.\r\n");
    } else {
      c2 = TRUE;
    }
    if (GET_SKILL(ch, SKILL_STEALTH) < 8) {
      if (show_text) send_to_char(ch, "You need a stealth skill of at least 8 to become an assassin.\r\n");
    } else {
      c3 = TRUE;
    }
    if (c1 && c2 && c3)
      return TRUE;
    return FALSE;  

  case CLASS_DRAGON_DISCIPLE:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_SKILL(ch, SKILL_KNOWLEDGE) >= 8)
      c1 = TRUE;
    else
      if (show_text) send_to_char(ch, "You need a knowledge skill of at least 8 to qualify for a dragon disciple.\r\n");
    if (IS_BARD(ch) || IS_SORCERER(ch))
      c2 = TRUE;
    else
      if (show_text) send_to_char(ch, "You need to be able to cast arcane spells without preparation to qualify for dragon disciple.\r\n");
    if (GET_SKILL(ch, SKILL_LANG_DRACONIC))
      c3 = TRUE;
    else
      if (show_text) send_to_char(ch, "You need to be able to speak draconic in order to become a dragon disciple.\r\n");
    if (c1 && c2 && c3)
      return TRUE;
    return FALSE;  
  
  case CLASS_WEAPON_MASTER:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_BAB(ch) >= 5)
      c1 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have a base attack bonus of +5 of higher to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_WEAPON_FOCUS))
      c2 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have the weapon focus feat in any weapon to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_DODGE))
      c3 = TRUE;  
    else
      if (show_text) send_to_char(ch, "You must have the dodge feat in order to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_MOBILITY))
      c4 = TRUE;  
    else
      if (show_text) send_to_char(ch, "You must have the mobility feat in order to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_SPRING_ATTACK))
      c5 = TRUE;  
    else
      if (show_text) send_to_char(ch, "You must have the spring attack feat in order to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_COMBAT_EXPERTISE))
      c6 = TRUE;  
    else
      if (show_text) send_to_char(ch, "You must have the combat expertise feat in order to become a weapon master.\r\n");
    if (HAS_FEAT(ch, FEAT_WHIRLWIND_ATTACK))
      c7 = TRUE;  
    else
      if (show_text) send_to_char(ch, "You must have the whirlwind attack feat in order to become a weapon master.\r\n");
    if (c1 && c2 && c3 && c4 && c5 && c6 && c7)
      return TRUE;
    return FALSE;

  case CLASS_MYSTIC_THEURGE:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_SKILL(ch, SKILL_KNOWLEDGE) >= 6)
      c1 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have a minimum knowledge skill of 6 to become a mystic theurge.\r\n");
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    if (findslotnum(ch, 2) > 0)
      c2 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
    if (findslotnum(ch, 2) > 0)
      c3 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    if (findslotnum(ch, 2) > 0)
      c5 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
    if (findslotnum(ch, 2) > 0)
      c6 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    if (findslotnum(ch, 2) > 0)
      c9 = TRUE;
    if (!c2 && !c3 && !c5 && !c6 && !c9)
      if (show_text) send_to_char(ch, "You must be able to cast 2nd level divine spells to become a mystic theurge.\r\n");
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 2) > 0)
      c4 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
    if (findslotnum(ch, 2) > 0)
      c10 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (findslotnum(ch, 2) > 0)
      c7 = TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 2) > 0)
      c8 = TRUE;
    if (!c4 && !c7 && !c8 && !c10)
      if (show_text) send_to_char(ch, "You must be able to cast 2nd level arcane spells to become a mystic theurge.\r\n");
    if (GET_CLASS_SPONSOR(ch, CLASS_MYSTIC_THEURGE))
      c11 = TRUE;
    else 
      if (show_text) send_to_char(ch, "You must be sponsored by an immortal to become a mystic theurge.  Please see help sponsor.\r\n");
    if (c1 && (c2 || c3 || c5 || c6 || c9) && (c4 || c7 || c10) && c11)
     return TRUE;
    return FALSE;
  case CLASS_DWARVEN_DEFENDER:
    if (GET_LEVEL(ch) < 1)
      return (0);
    if (GET_BAB(ch) >= 7)
      c1 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have a minimum base attack bonus of 7 to be a dwarven defender.\r\n");
    if (race_list[GET_RACE(ch)].family == RACE_TYPE_DWARF)
      c2 = TRUE;
    else
      if (show_text) send_to_char(ch, "Only dwarves can become dwarven defenders.\r\n");
    if (HAS_FEAT(ch, FEAT_DODGE))
      c3 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have the dodge feat to become a dwarven defender.\r\n");
    if (HAS_FEAT(ch, FEAT_ENDURANCE))
      c4 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have the endurance feat to become a dwarven defender.\r\n");
    if (HAS_FEAT(ch, FEAT_TOUGHNESS))
      c5 = TRUE;
    else
      if (show_text) send_to_char(ch, "You must have the toughness feat to become a dwarven defender.\r\n");

    if (c1 && c2 && c3 && c4 && c5)
      return TRUE;

    return FALSE;
  case CLASS_KNIGHT_OF_THE_CROWN:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_CROWN))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order to become a %s.  Please see help sponsor@n\r\n",
(CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 3)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +3 before you may enter the order of the crown.@n\r\n");
    if (GET_SAVE(ch, SAVING_FORTITUDE) >= 4)
    	c3 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum fortitude save of +4 before you may enter the order of the crown.@n\r\n");
    if (GET_SKILL(ch, SKILL_DIPLOMACY) >= 2)
    	c4 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum diplomacy skill of +2 before you may enter the order of the crown.@n\r\n");
    if (GET_SKILL(ch, SKILL_RIDE) >= 2)
    	c5 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum ride skill of +2 before you may enter the order of the crown.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
    	c6 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the heavy armor proficiency feat before you may enter the order of the crown.@n\r\n");
    if (HAS_FEAT(ch, FEAT_HONORBOUND))
    	c7 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the honorbound feat before you may enter the order of the crown.@n\r\n");
    if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_MARTIAL))
    	c8 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the martial weapon proficiency feat before you may enter the order of the crown.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
    	c9 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the shield proficiency feat before you may enter the order of the crown.@n\r\n");
    if (c1 && c2 && c3 && c4 && c5 && c6 && c7 && c8 && c9)
    	return (1);
    return (0);
  case CLASS_KNIGHT_OF_THE_SWORD:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_SWORD))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order to become a %s.  Please see help sponsor.@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 6)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +6 before you may enter the order of the sword.@n\r\n");
    if (GET_SAVE(ch, SAVING_WILL) >= 4)
    	c3 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum willpower save of +4 before you may enter the order of the sword.@n\r\n");
    if (GET_SKILL(ch, SKILL_DIPLOMACY) >= 4)
    	c4 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum diplomacy skill of +4 before you may enter the order of the sword.@n\r\n");
    if (GET_SKILL(ch, SKILL_RIDE) >= 4)
    	c5 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum ride skill of +4 before you may enter the order of the sword.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ENDURANCE))
    	c6 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the endurance feat before you may enter the order of the sword.@n\r\n");
    if (HAS_FEAT(ch, FEAT_HONORBOUND))
    	c7 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the honorbound feat before you may enter the order of the sword.@n\r\n");
    if (GET_CLASS_RANKS(ch, CLASS_CLERIC) > 0 || GET_CLASS_RANKS(ch, CLASS_PALADIN) > 3 ||
    	  GET_CLASS_RANKS(ch, CLASS_RANGER) > 3 || GET_CLASS_RANKS(ch, CLASS_DRUID) > 0)
    	c8 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the ability to case divine spells before you may enter the order of the sword.@n\r\n");
    if (c1 && c2 && c3 && c4 && c5 && c6 && c7 && c8)
    	return (1);
    return (0);    
  case CLASS_KNIGHT_OF_THE_ROSE:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_ROSE))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order to become a %s.  Please see help sponsor@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 8)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +8 before you may enter the order of the rose.@n\r\n");
//    if (GET_SAVE(ch, SAVING_WILL) >= 7)
//    	c3 = TRUE;
//    else
//    	if (show_text) send_to_char(ch, "@wYou must have a minimum willpower save of +7 before you may enter the order of the rose.@n\r\n");
    if (GET_SKILL(ch, SKILL_DIPLOMACY) >= 8)
    	c4 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum diplomacy skill of +8 before you may enter the order of the rose.@n\r\n");
    if (GET_SKILL(ch, SKILL_RIDE) >= 8)
    	c5 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum ride skill of +8 before you may enter the order of the rose.@n\r\n");
    if (HAS_FEAT(ch, FEAT_LEADERSHIP))
    	c6 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the leadership feat before you may enter the order of the rose.@n\r\n");
    if (HAS_FEAT(ch, FEAT_HONORBOUND))
    	c7 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the honorbound feat before you may enter the order of the rose.@n\r\n");
    if (HAS_FEAT(ch, FEAT_MOUNTED_COMBAT))
    	c8 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the mounted combat feat before you may enter the order of the rose.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ENDURANCE))
    	c9 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the endurance feat before you may enter the order of the rose.@n\r\n");
    if (HAS_FEAT(ch, FEAT_AURA_OF_COURAGE))
    	c10 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the aura of courage feat before you may enter the order of the rose.@n\r\n");
    if (GET_CLASS_RANKS(ch, CLASS_CLERIC) > 0 || GET_CLASS_RANKS(ch, CLASS_PALADIN) > 3 ||
    	  GET_CLASS_RANKS(ch, CLASS_RANGER) > 3 || GET_CLASS_RANKS(ch, CLASS_DRUID) > 0)
    	c11 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the ability to case divine spells before you may enter the order of the rose.@n\r\n");
    if (c1 && c2 && c4 && c5 && c6 && c7 && c8 && c9 && c10 && c11)
    	return (1);
    return (0);
  case CLASS_KNIGHT_OF_THE_LILY:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_LILY))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order become a %s.  Please see help sponsor.@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 5)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +5 before you may enter the order of the lily.@n\r\n");
    if (GET_SKILL(ch, SKILL_INTIMIDATE) >= 4)
    	c3 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum intimidate skill of +4 before you may enter the order of the lily.@n\r\n");
   if (HAS_FEAT(ch, FEAT_HONORBOUND))
    	c4 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the honorbound feat before you may enter the order of the lily.@n\r\n");
    if (c1 && c2 && c3 && c4)
    	return (1);
    return (0);    
  case CLASS_KNIGHT_OF_THE_SKULL:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_SKULL))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order to become a %s.  Please see help sponsor@n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 3)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +3 before you may enter the order of the skull.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ALERTNESS))
    	c3 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the alertness feat before you may enter the order of the crown.@n\r\n");
    if (GET_CLASS_RANKS(ch, CLASS_CLERIC) > 4 || GET_CLASS_RANKS(ch, CLASS_PALADIN) > 8 ||
    	  GET_CLASS_RANKS(ch, CLASS_RANGER) > 8 || GET_CLASS_RANKS(ch, CLASS_DRUID) > 4)
    	c4 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the ability to case 3rd-level divine spells before you may enter the order of the skull.@n\r\n");
  if (c1 && c2 && c3 && c4)
    	return (1);
    return (0);
  case CLASS_KNIGHT_OF_THE_THORN:
  	if (GET_LEVEL(ch) < 1)
  		return (0);
  	if (GET_CLASS_SPONSOR(ch, CLASS_KNIGHT_OF_THE_THORN))
  		c1 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must be sponsored by an immortal in order to become a %s.  Please see help sponsor.@n\r\n",  (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[whichclass]);
    if (GET_BAB(ch) >= 3)
    	c2 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +3 before you may enter the order of the thorn.@n\r\n");
    if (GET_SAVE(ch, SAVING_FORTITUDE) >= 4)
    	c3 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum fortitude save of +4 before you may enter the order of the thorn.@n\r\n");
//    if (GET_SAVE(ch, SAVING_WILL) >= 3)
//    	c4 = TRUE;
//    else
//    	if (show_text) send_to_char(ch, "@wYou must have a minimum willpower save of +7 before you may enter the order of the thorn.@n\r\n");
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
    	c5 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the heavy armor proficiency feat before you may enter the order of the thorn.@n\r\n");
    if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_MARTIAL))
    	c6 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the martial weapon proficiency feat before you may enter the order of the thorn.@n\r\n");
    if (GET_SKILL(ch, SKILL_SPELLCRAFT) >= 8)
    	c7 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have a minimum spellcraft skill of +8 before you may enter the order of the thorn.@n\r\n");
  if ((GET_CLASS_RANKS(ch, CLASS_WIZARD) + GET_CLASS_RANKS(ch, CLASS_WIZARD_OF_HIGH_SORCERY)) > 3)
    	c8 = TRUE;
    else
    	if (show_text) send_to_char(ch, "@wYou must have the ability to case 2nd-level arcane spells before you may enter the order of the thorn.@n\r\n");
  // Also needs one metamagic feat when they are implemented
  if (c1 && c2 && c3 && c5 && c6 && c7 && c8)
    	return (1);
    return (0);
  case CLASS_WIZARD_OF_HIGH_SORCERY:
    return (0);
  case CLASS_DUELIST:
  	if (GET_LEVEL(ch) < 1)
  		return (0);  	
  	if (GET_BAB(ch) >= 6)
  		c1 = TRUE;
  	else
  		if (show_text) send_to_char(ch, "@wYou must have a minimum base attack bonus of +6 to become a duelist.@n\r\n");
  	if (GET_SKILL(ch, SKILL_ACROBATICS) >= 5)
  		c2 = TRUE;
  	else 
  		if (show_text) send_to_char(ch, "@wYou must have a minimum tumble skill of +5 to become a duelist.@n\r\n");
  	if (GET_SKILL(ch, SKILL_PERFORM) >= 3)
  		c3 = TRUE;
  	else
  		if (show_text) send_to_char(ch, "@wYou must have a minimum perform skill of +3 to become a duelist.@n\r\n");
  	if (HAS_FEAT(ch, FEAT_DODGE))
  		c4 = TRUE;
  	else
  		if (show_text) send_to_char(ch, "@wYou must have the dodge feat to become a duelist.@n\r\n");
  	if (HAS_FEAT(ch, FEAT_MOBILITY))
  		c5 = TRUE;
  	else
  		if (show_text) send_to_char(ch, "@wYou must have the mobility feat to become a duelist.@n\r\n");
  	if (HAS_FEAT(ch, FEAT_WEAPON_FINESSE))
  		c6 = TRUE;
  	else
  		if (show_text) send_to_char(ch, "@wYou must have the weapon finesse feat to becme a duelist.@n\r\n");
  	if (c1 && c2 && c3 && c4 && c5 && c6)
  		return (1);
  	return (0);
  default:
    return 1; /* All other classes can be taken */
  }
}
/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only WIZARDS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. if you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
struct guild_info_type guild_info[] = {
/* Kortaal */
  { CLASS_WIZARD,	3017,	SCMD_EAST	},
  { CLASS_CLERIC,	3004,	SCMD_NORTH	},
  { CLASS_ROGUE,	3027,	SCMD_EAST	},
  { CLASS_FIGHTER,	3021,	SCMD_EAST	},
/* Brass Dragon */
  { 254 /*-999 all */ ,	5065,	SCMD_WEST	},
/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};
/* 
 * These tables hold the various level configuration setting;
 * experience points, base hit values, character saving throws.
 * They are read from a configuration file (normally etc/levels)
 * as part of the boot process.  The function load_levels() at
 * the end of this file reads in the actual values.
 */
const char *config_sect[] = 
{
  "version",
  "experience",
  "vernum",
  "fortitude",
  "reflex",
  "will",
  "basehit",
  "\n"
};
#define CONFIG_LEVEL_VERSION	0
#define CONFIG_LEVEL_EXPERIENCE	1
#define CONFIG_LEVEL_VERNUM	2
#define CONFIG_LEVEL_FORTITUDE	3
#define CONFIG_LEVEL_REFLEX	4
#define CONFIG_LEVEL_WILL	5
#define CONFIG_LEVEL_BASEHIT	6
char level_version[READ_SIZE];
int level_vernum = 0;
int save_classes[SAVING_WILL][NUM_CLASSES];
int basehit_classes[NUM_CLASSES];
int exp_multiplier;
byte object_saving_throws(int material_type, int type)
{
  switch (type) {
  case SAVING_OBJ_IMPACT:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 20;
    case MATERIAL_CERAMIC:
      return 35;
    case MATERIAL_ORGANIC:
      return 40;
    case MATERIAL_WOOD:
      return 50;
    case MATERIAL_IRON:
    case MATERIAL_LEATHER:
      return 70;
    case MATERIAL_STEEL:
    case MATERIAL_DARKWOOD:
    case MATERIAL_COLD_IRON:
      return 85;
    case MATERIAL_MITHRIL:
    case MATERIAL_STONE:
    case MATERIAL_ALCHEMICAL_SILVER:
    case MATERIAL_DRAGONHIDE:
      return 90;
    case MATERIAL_DIAMOND:
    case MATERIAL_ADAMANTINE:
      return 95;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_HEAT:
    switch (material_type) {
    case MATERIAL_WOOL:
      return 15;
    case MATERIAL_PAPER:
      return 20;
    case MATERIAL_COTTON:
    case MATERIAL_SATIN:
    case MATERIAL_SILK:
    case MATERIAL_BURLAP:
    case MATERIAL_VELVET:
    case MATERIAL_WOOD:
      return 25;
    case MATERIAL_ONYX:
    case MATERIAL_CURRENCY:
      return 45;
    case MATERIAL_GLASS:
      return 55;
    case MATERIAL_PLATINUM:
      return 75;
    case MATERIAL_ADAMANTINE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DRAGONHIDE:
      return 85;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_COLD:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 35;
    case MATERIAL_ORGANIC:
    case MATERIAL_CURRENCY:
      return 45;
    case MATERIAL_DRAGONHIDE:
      return 80;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_BREATH:
    switch (material_type) {
    case MATERIAL_DRAGONHIDE:
      return 85;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_SPELL:
    switch (material_type) {
    default:
      return 50;
      break;
    }
  default:
    log("SYSERR: Invalid object saving throw type.");
    break;
  }
  /* Should not get here unless something is wrong. */
  return 100;
}
#define SAVE_MANUAL 0
#define SAVE_LOW 1
#define SAVE_HIGH 2
char *save_type_names[] = 
{
  "manual",
  "low",
  "high"
};
#define BASEHIT_MANUAL 0
#define BASEHIT_LOW     1
#define BASEHIT_MEDIUM  2
#define BASEHIT_HIGH    3
char *basehit_type_names[] = 
{
  "manual",
  "low",
  "medium",
  "high"
};
/*
 * this is for the new saving throws, that slowly go up as you level
 * if save_lev == 0, use the class tables
 * otherwise ignore the class and just determine the save for this level
 * of that type of save.
 */
int saving_throw_lookup(int save_lev, int chclass, int savetype, int level)
{
  if (level < 0) {
    log("SYSERR: Requesting saving throw for invalid level %d!", level);
    level = 0;
  }
  if (chclass >= NUM_CLASSES || chclass < 0) {
    log("SYSERR: Requesting saving throw for invalid class %d!", chclass);
    chclass = 0;
  }
  if (!save_lev)
    save_lev = get_saving_throw(chclass, savetype);
//    save_lev = save_classes[savetype][chclass];
  switch (save_lev) {
  case SAVE_MANUAL:
    log("Save undefined for class %s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[chclass]);
    return 0;
  case SAVE_LOW:
    return level / 3;
    break;
  case SAVE_HIGH:
    return (level / 2 ) + 2;
    break;
  default:
    log("Unknown save type %d in load_levels", save_lev);
    return 0;
    break;
  }
}
/* Base hit for classes and levels.
 * if hit_type == 0, use the class tables
 * otherwise ignore the class and just determine the base hit for this level
 * of that hit_type.
 */
int base_hit(int hit_type, int chclass, int level)
{
  if (level < 0 ) 
  {
    log("SYSERR: Requesting base hit for invalid level %d!", level);
    level = 0;
  }
  if (chclass >= NUM_CLASSES || chclass < 0) {
    log("SYSERR: Requesting base hit for invalid class %d!", chclass);
    chclass = 0;
  }
  if (!hit_type)
    hit_type = basehit_classes[chclass];
  if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS) 
  {
    switch (chclass) 
    {
      case CLASS_SORCERER:
      case CLASS_WIZARD:
      case CLASS_MYSTIC_THEURGE:
      case CLASS_ARCANE_TRICKSTER:
      case CLASS_DEATH_MASTER:
      hit_type = BASEHIT_LOW;
      break;
      case CLASS_CLERIC:
      case CLASS_DRAGON_DISCIPLE:
      case CLASS_FAVORED_SOUL:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_ROGUE:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_FIGHTER:
      case CLASS_ARCANE_ARCHER:
      case CLASS_ELDRITCH_KNIGHT:
      case CLASS_DRAGON_RIDER:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_MONK:
      case CLASS_SACRED_FIST:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_DRUID:
      hit_type = BASEHIT_MEDIUM;
      break;    
      case CLASS_ASSASSIN:
      hit_type = BASEHIT_MEDIUM;
      break;    
      case CLASS_RANGER:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_BARBARIAN:
      hit_type = BASEHIT_HIGH;
      break;  	  	
      case CLASS_PALADIN:
      hit_type = BASEHIT_HIGH;
      break;	
      case CLASS_BARD:
      hit_type = BASEHIT_MEDIUM;
      break;

      case CLASS_DWARVEN_DEFENDER:
      case CLASS_WEAPON_MASTER:
      case CLASS_KNIGHT_OF_THE_CROWN:
      case CLASS_KNIGHT_OF_THE_SWORD:
      case CLASS_KNIGHT_OF_THE_ROSE:
      case CLASS_KNIGHT_OF_THE_LILY:
      case CLASS_GLADIATOR:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_DUELIST:
      hit_type = BASEHIT_HIGH;
      break;

      case CLASS_KNIGHT_OF_THE_THORN:
      case CLASS_WIZARD_OF_HIGH_SORCERY:
      hit_type = BASEHIT_LOW;
      break;

      case CLASS_KNIGHT_OF_THE_SKULL:
      hit_type = BASEHIT_MEDIUM;
      break;
      default:
      hit_type = BASEHIT_MEDIUM;
      break;
    }
  }
  else 
  {
    switch (chclass) 
    {
      case CLASS_SORCERER:
      case CLASS_WIZARD:
      case CLASS_MYSTIC_THEURGE:
      case CLASS_ARTISAN:
      case CLASS_ARCANE_TRICKSTER:
      case CLASS_DEATH_MASTER:
      hit_type = BASEHIT_LOW;
      break;
      case CLASS_CLERIC:
      case CLASS_DRAGON_DISCIPLE:
      case CLASS_FAVORED_SOUL:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_ROGUE:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_ELDRITCH_KNIGHT:
      case CLASS_FIGHTER:
      case CLASS_DRAGON_RIDER:
      case CLASS_ARCANE_ARCHER:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_MONK:
      case CLASS_SACRED_FIST:
      hit_type = BASEHIT_MEDIUM;
      break;
      case CLASS_DRUID:
      hit_type = BASEHIT_MEDIUM;
      break;    
      case CLASS_ASSASSIN:
      hit_type = BASEHIT_MEDIUM;
      break;    
      case CLASS_RANGER:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_BARBARIAN:
      hit_type = BASEHIT_HIGH;
      break;  	  	
      case CLASS_PALADIN:
      hit_type = BASEHIT_HIGH;
      break;	
      case CLASS_BARD:
      hit_type = BASEHIT_MEDIUM;
      break;

      case CLASS_DWARVEN_DEFENDER:
      case CLASS_KNIGHT_OF_THE_CROWN:
      case CLASS_WEAPON_MASTER:
      case CLASS_KNIGHT_OF_THE_SWORD:
      case CLASS_KNIGHT_OF_THE_ROSE:
      case CLASS_KNIGHT_OF_THE_LILY:
      case CLASS_GLADIATOR:
      hit_type = BASEHIT_HIGH;
      break;
      case CLASS_DUELIST:
      hit_type = BASEHIT_HIGH;
      break;

      case CLASS_KNIGHT_OF_THE_THORN:
      case CLASS_WIZARD_OF_HIGH_SORCERY:
      hit_type = BASEHIT_LOW;
      break;

      case CLASS_KNIGHT_OF_THE_SKULL:
      hit_type = BASEHIT_MEDIUM;
      break;
      default:
      hit_type = BASEHIT_MEDIUM;
      break;
    }
  }  	
  switch (hit_type) 
  {
    case BASEHIT_MANUAL:
    log("Base hit undefined for class %s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[chclass]);
    return 0;
    case BASEHIT_LOW:
    return level / 2;
    break;
    case BASEHIT_MEDIUM:
    return level * 3 / 4;
    break;
    case BASEHIT_HIGH:
    return level;
    break;
    default:
    log("Unknown basehit type %d in load_levels", hit_type);
    return 0;
    break;
  }
}

#define IS_MONK_WEAPON(obj) (weapon_list[GET_OBJ_VAL(obj, 0)].weaponFamily == WEAPON_FAMILY_MONK)

#define IS_RANGED_WEAPON(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON && \
        IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_RANGED))

#define IS_THROWN_WEAPON(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON && \
        IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_THROWN))


/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int num_attacks(struct char_data *ch, int offhand)
{
  int attk;

  struct obj_data *wielded, *armor, *shield, *held;

  wielded = GET_EQ(ch, WEAR_WIELD1);
  armor   = GET_EQ(ch, WEAR_BODY);
  shield  = GET_EQ(ch, WEAR_SHIELD);
  held = GET_EQ(ch, WEAR_WIELD2);

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      wielded = armor = shield = held = NULL;
  }

  if (offhand) {
    if (!held)
      return 0;
    if (held && GET_OBJ_VAL(held, 0) == WEAPON_TYPE_UNARMED)
      return 0;
    if (wielded && held &&
        (IS_RANGED_WEAPON(wielded) || IS_RANGED_WEAPON(held)))
      return 1;
    if (wielded && held && GET_FORM_POS(ch) > FORM_POS_FRONT &&
        (IS_THROWN_WEAPON(wielded) || IS_THROWN_WEAPON(held)))
      return 1;    
    if (HAS_FEAT(ch, FEAT_PERFECT_TWO_WEAPON_FIGHTING))
      return 4;
    if (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      return 3;
    if (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING))
      return 2;
    else
      return 1;
  }


  	
  attk = GET_ACCURACY_BASE(ch) + 4;

  if ((!wielded || (wielded && IS_MONK_WEAPON(wielded))) && (!armor || (armor && GET_OBJ_TYPE(armor) == ITEM_WORN)) && !shield) {
    attk += base_hit(BASEHIT_HIGH, CLASS_FIGHTER, GET_CLASS_RANKS(ch, CLASS_MONK)) -
                  base_hit(BASEHIT_MEDIUM, CLASS_MONK, GET_CLASS_RANKS(ch, CLASS_MONK));
  }

  attk /= 5;

  attk = MIN(4, attk);

  if (affected_by_spell(ch, SPELL_SLOW))
    attk = 1;

  if (wielded && IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) {
    held = wielded;

    if (HAS_FEAT(ch, FEAT_PERFECT_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING))
      attk++;
  }

  attk -= ch->parries;

  attk = MAX(1, attk);
  return attk;
}

int num_parry_attacks(struct char_data *ch, int offhand)
{
  int attk;

  struct obj_data *wielded, *armor, *shield, *held;

  wielded = GET_EQ(ch, WEAR_WIELD1);
  armor   = GET_EQ(ch, WEAR_BODY);
  shield  = GET_EQ(ch, WEAR_SHIELD);
  held = GET_EQ(ch, WEAR_WIELD2);

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      wielded = armor = shield = held = NULL;
  }

  if (offhand) {
    if (!held)
      return 0;
    if (held && GET_OBJ_VAL(held, 0) == WEAPON_TYPE_UNARMED)
      return 0;
    if (wielded && held &&
        (IS_RANGED_WEAPON(wielded) || IS_RANGED_WEAPON(held)))
      return 1;
    if (wielded && held && GET_FORM_POS(ch) > FORM_POS_FRONT &&
        (IS_THROWN_WEAPON(wielded) || IS_THROWN_WEAPON(held)))
      return 1;    
    if (HAS_FEAT(ch, FEAT_PERFECT_TWO_WEAPON_FIGHTING))
      return 4;
    if (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      return 3;
    if (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING))
      return 2;
    else
      return 1;
  }


  	
  attk = GET_ACCURACY_BASE(ch) + 4;

  if ((!wielded || (wielded && IS_MONK_WEAPON(wielded))) && (!armor || (armor && GET_OBJ_TYPE(armor) == ITEM_WORN)) && !shield) {
    attk += base_hit(BASEHIT_HIGH, CLASS_FIGHTER, GET_CLASS_RANKS(ch, CLASS_MONK)) -
                  base_hit(BASEHIT_MEDIUM, CLASS_MONK, GET_CLASS_RANKS(ch, CLASS_MONK));
  }

  attk /= 5;

  attk = MIN(4, attk);

  if (affected_by_spell(ch, SPELL_SLOW))
    attk = 1;

  if (wielded && IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) {
    held = wielded;

    if (HAS_FEAT(ch, FEAT_PERFECT_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING))
      attk++;
    if (HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING))
      attk++;
  }



  attk = MAX(1, attk);
  return attk;
}


/* Class Template Attribute values were created so all default PC classes would
   total 80 before racial modifiers. Non defaults add up to 60. */
int class_template[NUM_CLASSES][6] = {
/* 		      S,  D,  C,  I,  W,  C */
/* Mage   	*/ { 10, 13, 13, 18, 16, 10 },
/* Cleric 	*/ { 13, 10, 13, 10, 18, 16 },
/* Rogue 	*/ { 13, 18, 13, 16, 10, 10 },
/* Fighter 	*/ { 18, 13, 16, 10, 13, 10 },
/* Monk 	*/ { 13, 16, 13, 10, 18, 10 },
/* Paladin 	*/ { 18, 10, 13, 10, 16, 13 },
/* Expert 	*/ { 10, 10, 10, 10, 10, 10 },
/* Adept 	*/ { 10, 10, 10, 10, 10, 10 },
/* Commoner 	*/ { 10, 10, 10, 10, 10, 10 },
/* Aristrocrat 	*/ { 10, 10, 10, 10, 10, 10 },
/* Warrior 	*/ { 10, 10, 10, 10, 10, 10 }
};
/* Race Template Attribute values were created so all default PC races would
   total 80 before racial modifiers. Non defaults add up to 60. */
void cedit_creation(struct char_data *ch)
{
  switch (CONFIG_CREATION_METHOD) {
    case CEDIT_CREATION_METHOD_3: /* Points Pool */
      ch->real_abils.str = 10;
      ch->real_abils.dex = 10;
      ch->real_abils.con = 10;
      ch->real_abils.intel = 10;
      ch->real_abils.wis = 10;
      ch->real_abils.cha = 10;
      break;
    case CEDIT_CREATION_METHOD_4: /* Racial based template */
        ch->real_abils.str = class_template[GET_CLASS(ch)][0];
        ch->real_abils.dex = class_template[GET_CLASS(ch)][1];
        ch->real_abils.con = class_template[GET_CLASS(ch)][2];
        ch->real_abils.intel = class_template[GET_CLASS(ch)][3];
        ch->real_abils.wis = class_template[GET_CLASS(ch)][4];
        ch->real_abils.cha = class_template[GET_CLASS(ch)][5];
      break;
    case CEDIT_CREATION_METHOD_5: /* Class based template */
        ch->real_abils.str = class_template[GET_CLASS(ch)][0];
        ch->real_abils.dex = class_template[GET_CLASS(ch)][1];
        ch->real_abils.con = class_template[GET_CLASS(ch)][2];
        ch->real_abils.intel = class_template[GET_CLASS(ch)][3];
        ch->real_abils.wis = class_template[GET_CLASS(ch)][4];
        ch->real_abils.cha = class_template[GET_CLASS(ch)][5];
      break;
    case CEDIT_CREATION_METHOD_2: /* Random rolls, player can adjust */
    case CEDIT_CREATION_METHOD_1: /* Standard random roll, system assigned */
    default:
      roll_real_abils(ch);
      break;
  }
}
/*
 * Roll the 6 stats for a character... each stat is made of some combination
 * of 6-sided dice with various rolls discarded.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
/* This character creation method is original */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp, temp2, total, count = 20;
  ubyte table[6];
  ubyte rolls[6];
  do {
    for (i = 0; i < 6; i++)
      table[i] = 0;
    for (i = 0; i < 6; i++) {
      for (j = 0; j < 6; j++)
        rolls[j] = rand_number(1, 6);
      switch (i) {
      case 0: /* We want one bad roll */
        /* Worst 3 out of 4 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
               MAX(rolls[0], MAX(rolls[1], MAX(rolls[2], rolls[3])));
        break;
      case 5: /* We want one very good roll */
        /* Best 3 out of 4 + best of 2 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] + rolls[4] + rolls[5] -
               (MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3]))) +
                MIN(rolls[4], rolls[5]));
        break;
      default: /* We want the rest to be 'better than average' */
        /* Best 3 out of 4 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
               MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));
        break;
      }
      for (k = 0; k < 6; k++)
        if (table[k] < temp) {
	  temp2 = table[k];
	  table[k] = temp;
	  temp = temp2;
        }
    }
    switch (GET_CLASS(ch)) {
    case CLASS_WIZARD:
      ch->real_abils.intel = table[0];
      ch->real_abils.wis = table[1];
      ch->real_abils.dex = table[2];
      ch->real_abils.con = table[3];
      switch (GET_RACE(ch)) {
        default:
          ch->real_abils.str = table[4];
          ch->real_abils.cha = table[5];
      }
      break;
    case CLASS_CLERIC:
      ch->real_abils.wis = table[0];
      switch (GET_RACE(ch)) {
        default:
          ch->real_abils.cha = table[1];
          ch->real_abils.str = table[2];
          ch->real_abils.intel = table[4];
          ch->real_abils.dex = table[5];
        break;
      }
      ch->real_abils.con = table[3];
      break;
    case CLASS_ROGUE:
      ch->real_abils.dex = table[0];
      ch->real_abils.intel = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.str = table[3];
      ch->real_abils.cha = table[4];
      ch->real_abils.wis = table[5];
      break;
    case CLASS_FIGHTER:
      switch (GET_RACE(ch)) {
      default:
        ch->real_abils.str = table[0];
        ch->real_abils.con = table[1];
        ch->real_abils.dex = table[2];
        break;
      }
      ch->real_abils.cha = table[5];
      ch->real_abils.intel = table[4];
      ch->real_abils.wis = table[3];
      break;
      case CLASS_MONK:
        switch (GET_RACE(ch)) {
        default:
          ch->real_abils.wis = table[0];
          ch->real_abils.dex = table[1];
          ch->real_abils.str = table[2];
        break;
        }
        ch->real_abils.con = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
        break;
      case CLASS_PALADIN:
        switch (GET_RACE(ch)) {
        default:
          ch->real_abils.str = table[0];
          ch->real_abils.con = table[1];
          ch->real_abils.wis = table[2];
          ch->real_abils.cha = table[3];
          ch->real_abils.dex = table[4];
          break;
        }
        ch->real_abils.intel = table[5];
      break;
    }
    total = ch->real_abils.str / 2 + ch->real_abils.con / 2 +
            ch->real_abils.wis / 2 + ch->real_abils.cha / 2 +
            ch->real_abils.dex / 2 + ch->real_abils.intel / 2 ;
    total -= 30;
    /*
     * if the character is not worth playing, reroll.
     * total == total ability_mod_value for all stats. if the totals for
     * ability mods would be 0 or less, the character sucks.
     * if the highest ability is 13, the character also sucks.
     * if the totals for ability mods would be higher than 12, the char
     * is just too powerful.
     * They get to keep the first acceptable character rolled; if no good
     * characters appear in {count} rerolls, they are stuck with the last
     * roll.
     */
  } while (--count && (total < 0 || table[0] < 14 || total > 12));
  if (!count)
    mudlog(NRM, ADMLVL_GOD, true,
           "Unlucky new player %s has stat mods totalling %d and highest stat %d",
           GET_NAME(ch), total, table[0]);
}
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
int class_hit_die_size_fr[NUM_CLASSES] = {
/* Wi */ 4,
/* Cl */ 8,
/* Ro */ 6,
/* Fi */ 10,
/* Mo */ 8,
/* Pa */ 10,
/* Ba */ 12,
/* Bd */ 6,
/* Ra */ 8,
/* Dr */ 8,
/* Cr */ 10,
/* Sw */ 8,
/* Rs */ 10,
/* Li */ 10,
/* Sk */ 8,
/* Th */ 6,
/* Wi */ 4,
/* Du */ 10,
/* Gl */ 12,
/* My */ 4,
/* So */ 4,
/* No */ 8,
/* DD */ 12,
/* WM */ 10,
/* DD */ 12,
/* AA */ 8,
/* EQ */ 6,
/* NP */ 6,
/* AS */ 6,
/* BG */ 6,
/* AR */ 6,
/* AT */ 4,
/* FS */ 8,
/* EK */ 6,
/* DM */ 6,
/* SF */ 8,
/* DR */ 10,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6
};

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
int class_hit_die_size_dl_aol[NUM_CLASSES] = {
/* Wi */ 4,
/* Cl */ 8,
/* Ro */ 6,
/* Fi */ 10,
/* Mo */ 8,
/* Pa */ 10,
/* Ba */ 12,
/* Bd */ 6,
/* Ra */ 10,
/* Dr */ 8,
/* Cr */ 10,
/* Sw */ 8,
/* Rs */ 10,
/* Li */ 10,
/* Sk */ 8,
/* Th */ 6,
/* Du */ 10,
/* Gl */ 12,
/* My */ 4,
/* So */ 4,
/* No */ 8,
/* DD */ 12,
/* WM */ 10,
/* DD */ 12,
/* EP */ 6,
/* EQ */ 6,
/* NP */ 6,
/* AS */ 6,
/* BG */ 6,
/* AR */ 6,
/* AT */ 4,
/* FS */ 8,
/* EK */ 6,
/* DM */ 6,
/* SF */ 8,
/* DR */ 10,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6,
/* EX */ 6
};

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  int i, j;
  struct obj_data *obj;
//  struct obj_data * objNew, * objCont;	
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOATTACK);
  if (GET_CLASS(ch) < 0 || GET_CLASS(ch) > NUM_CLASSES) {
    log("Unknown character class %d in do_start, resetting.", GET_CLASS(ch));
    GET_CLASS(ch) = 0;
  }
  set_title(ch, GET_NAME(ch));
  GET_FEAT_POINTS(ch) = 1;
  GET_MAX_HIT(ch) = 30;
  GET_MAX_MANA(ch) = 0;
  GET_MAX_MOVE(ch) = 1000;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */

  extern int race_ages[NUM_RACE_TYPES+1];

  age(ch)->year = MAX(1, race_ages[race_list[GET_RACE(ch)].family] * (111 - dice(1, 21)) / 100);
  age(ch)->month = 0;

  if (IS_HUMAN(ch))
    GET_FEAT_POINTS(ch) += 1;

  if (GET_EXP(ch) <= 1) {

  GET_EXP(ch) = 1;

  SET_SKILL(ch, SKILL_LANG_COMMON, 1);
  SET_SKILL(ch, SKILL_BLACKSMITHING, 1);
  SET_SKILL(ch, SKILL_GOLDSMITHING, 1);
  SET_SKILL(ch, SKILL_TAILORING, 1);
  SET_SKILL(ch, SKILL_HUNTING, 1);
  SET_SKILL(ch, SKILL_FARMING, 1);
  SET_SKILL(ch, SKILL_FORESTING, 1);
  SET_SKILL(ch, SKILL_WOODWORKING, 1);
  SET_SKILL(ch, SKILL_COOKING, 1);
  SET_SKILL(ch, SKILL_MINING, 1);
  SET_SKILL(ch, SKILL_TANNING, 1);
  SET_SKILL(ch, SKILL_HERBALISM, 1);
  SET_SKILL(ch, SKILL_CRAFTING_THEORY, 1);


  if (race_list[GET_RACE(ch)].language != SKILL_LANG_COMMON)
    SET_SKILL(ch, race_list[GET_RACE(ch)].language, 1);

  

  SPEAKING(ch) = SKILL_LANG_COMMON;
  /* assign starting items etc...*/
  switch GET_CLASS(ch) {
    case CLASS_WIZARD:
      obj = read_object(30096, virtual);
      obj_to_char(obj, ch);
      GET_GOLD(ch) = 75;
      break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
      GET_GOLD(ch) = 125;
      break;
    case CLASS_ROGUE:
      GET_GOLD(ch) = 125;
      break;
    case CLASS_MONK:
      GET_ETHIC_ALIGNMENT(ch) = 500;
      GET_GOLD(ch) = 25;
      break;    
    case CLASS_PALADIN:
      GET_ETHIC_ALIGNMENT(ch) = 500;
      GET_ALIGNMENT(ch) = 500;
      GET_GOLD(ch) = 150;
      break;
    case CLASS_FIGHTER:
      GET_GOLD(ch) = 150;   
      break;
    case CLASS_RANGER:
      GET_GOLD(ch) = 150;   
      break;      
    case CLASS_ARTISAN:
      GET_GOLD(ch) = 200;   
      break;      
    case CLASS_BARBARIAN:
    	GET_ETHIC(ch) = -500;
      SET_BIT_AR(AFF_FLAGS(ch), AFF_ILLITERATE);
      GET_GOLD(ch) = 150;
      break;
    case CLASS_BARD:
      GET_GOLD(ch) = 125;
      obj = read_object(30096, virtual);
      obj_to_char(obj, ch);
      break;

    default:
      GET_GOLD(ch) = 150;
      break;
  }
    obj = read_object(30098, virtual);
    obj_to_char(obj, ch);
    obj = read_object(2317, virtual);
    obj_to_char(obj, ch);
    obj = read_object(2317, virtual);
    obj_to_char(obj, ch);
    obj = read_object(2317, virtual);
    obj_to_char(obj, ch);

  racial_ability_modifiers(ch);
  ch->aff_abils = ch->real_abils;
  set_height_and_weight_by_race(ch);

  switch (GET_RACE(ch)) 
  {
  case RACE_CENTAUR:
    GET_MAX_HIT(ch) += 32;
    GET_ACCURACY_BASE(ch) = 4;
    GET_SAVE_BASE(ch, SAVING_FORTITUDE) += 1;
    GET_SAVE_BASE(ch, SAVING_REFLEX)  += 4;
    GET_SAVE_BASE(ch, SAVING_WILL)  += 4;
    GET_PRACTICES(ch, GET_CLASS(ch)) = 7;
    GET_PRACTICES(ch, GET_CLASS(ch)) *= MAX(1, 2 + ability_mod_value(ch->real_abils.intel));
    SET_FEAT(ch, FEAT_WEAPON_PROFICIENCY_SIMPLE, 1);
    GET_FEAT_POINTS(ch) = 2;
    break;

  case RACE_MINOTAUR:
    if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS) 
    {    
    GET_MAX_HIT(ch) += 32;
    GET_ACCURACY_BASE(ch) = 4;
    GET_SAVE_BASE(ch, SAVING_FORTITUDE) += 4;
    GET_SAVE_BASE(ch, SAVING_REFLEX)  += 2;
    GET_SAVE_BASE(ch, SAVING_WILL)  += 2;
    GET_PRACTICES(ch, GET_CLASS(ch)) = 7;
    GET_PRACTICES(ch, GET_CLASS(ch)) *= MAX(1, 2 + ability_mod_value(ch->real_abils.intel));
    SET_FEAT(ch, FEAT_WEAPON_PROFICIENCY_SIMPLE, 1);
    SET_FEAT(ch, FEAT_TRACK, 1);
      GET_FEAT_POINTS(ch) = 2;
    }
    break;

  case RACE_OGRE:
    GET_MAX_HIT(ch) += 32;
    GET_ACCURACY_BASE(ch) = 3;
    GET_SAVE_BASE(ch, SAVING_FORTITUDE) += 4;
    GET_SAVE_BASE(ch, SAVING_REFLEX)  += 1;
    GET_SAVE_BASE(ch, SAVING_WILL)  += 1;
    GET_PRACTICES(ch, GET_CLASS(ch)) = 7;
    GET_PRACTICES(ch, GET_CLASS(ch)) *= MAX(1, 2 + ability_mod_value(ch->real_abils.intel));
    SET_FEAT(ch, FEAT_WEAPON_PROFICIENCY_SIMPLE, 1);
    SET_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);
    GET_FEAT_POINTS(ch) = 2;
    break;
  default:

    for (i = 0; (j = free_start_feats[GET_CLASS(ch)][i]); i++) 
    {
      SET_FEAT(ch, j, 1);
    }

    GET_SAVE_BASE(ch, SAVING_FORTITUDE) += saving_throw_lookup(0, GET_CLASS(ch), SAVING_FORTITUDE, 1);
    GET_SAVE_BASE(ch, SAVING_REFLEX)  += saving_throw_lookup(0, GET_CLASS(ch), SAVING_REFLEX, 1);
    GET_SAVE_BASE(ch, SAVING_WILL)  += saving_throw_lookup(0, GET_CLASS(ch), SAVING_WILL, 1);

//    GET_ACCURACY_BASE(ch) += base_hit(0, GET_CLASS(ch), 1);

    break;
  }
  GET_LOADROOM(ch) = CONFIG_MORTAL_START;
  GET_CLAN(ch) = 11;
  SET_BIT_AR(PRF_FLAGS(ch), PRF_CLANTALK);
  perform_cinfo(GET_CLAN(ch), "%s is now a member of the clan", GET_NAME(ch));
  clear_quest(ch);

  } // if get_exp <= 1

  if (GET_CLASS(ch) == CLASS_WIZARD)
    GET_RESEARCH_TOKENS(ch) = 2;

  GET_MAX_HIT(ch) = calculate_max_hit(ch);
  GET_HIT(ch) = GET_MAX_HIT(ch);

  mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_KI(ch) = GET_MAX_KI(ch);
  GET_COND(ch, THIRST) = -1;
  GET_COND(ch, FULL) = -1;
  GET_COND(ch, DRUNK) = -1;
  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
  ch->player_specials->olc_zone = NOWHERE;
  save_char(ch);

}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_wizard[] = 
{
  FEAT_SCRIBE_SCROLL,
  FEAT_WEAPON_PROFICIENCY_WIZARD,
  0
};
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_cleric[] = 
{
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_TURN_UNDEAD,
  FEAT_WEAPON_PROFICIENCY_DEITY,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  0
};
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_rogue[] = 
{
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_WEAPON_PROFICIENCY_ROGUE,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  0
};
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_fighter[] = 
{
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  0
};
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_monk[] = 
{
  FEAT_FLURRY_OF_BLOWS,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_WEAPON_PROFICIENCY_MONK,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  0
};
int free_start_feats_paladin[] = 
{
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_AURA_OF_GOOD,
  FEAT_DETECT_EVIL,
  FEAT_SMITE_EVIL,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  0
};

int free_start_feats_barbarian[] = 
{
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_FAST_MOVEMENT,
  FEAT_RAGE,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
	0
};

int free_start_feats_ranger[] = 
{
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_FAVORED_ENEMY_AVAILABLE,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
	0
};

int free_start_feats_arcane_archer[] = 
{
	FEAT_WEAPON_PROFICIENCY_SIMPLE,
	FEAT_ARMOR_PROFICIENCY_LIGHT,
	FEAT_ARMOR_PROFICIENCY_MEDIUM,
	FEAT_ARMOR_PROFICIENCY_SHIELD,
	FEAT_ARMOR_PROFICIENCY_LIGHT,
	FEAT_WEAPON_PROFICIENCY_MARTIAL,
	0
};

int free_start_feats_druid[] = 
{
  FEAT_ANIMAL_COMPANION,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_NATURE_SENSE,
  FEAT_WEAPON_PROFICIENCY_DRUID,
  FEAT_WILD_EMPATHY,
	0
};

int free_start_feats_bard[] = 
{
	FEAT_WEAPON_PROFICIENCY_SIMPLE,
	FEAT_ARMOR_PROFICIENCY_LIGHT,
	FEAT_ARMOR_PROFICIENCY_SHIELD,
	0
};

int free_start_feats_gladiator[] = 
{
	0
};

int free_start_feats_artisan[] = 
{
	0
};

int free_start_feats_mystic[] = 
{
        0
};

int free_start_feats_sorcerer[] = 
{
	FEAT_WEAPON_PROFICIENCY_SIMPLE,
        0
};

int free_start_feats_noble[] = 
{
        0
};

int free_start_feats_knight_of_crown[] = 
{
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_STRENGTH_OF_HONOR,
  FEAT_KNIGHTLY_COURAGE,
  0
};

int free_start_feats_knight_of_sword[] = 
{
	0
};

int free_start_feats_knight_of_rose[] = 
{
	0
};

int free_start_feats_favored_soul[] = 
{
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_WEAPON_PROFICIENCY_DEITY,
	0
};

int free_start_feats_wizard_of_high_sorcery[] = 
{
	0
};

int free_start_feats_duelist[] = 
{
  FEAT_WEAPON_PROFICIENCY_SIMPLE,
  FEAT_WEAPON_PROFICIENCY_MARTIAL,
  FEAT_CANNY_DEFENSE,
  FEAT_PRECISE_STRIKE,
  0
};

int free_start_feats_none[] =
{
	0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int *free_start_feats[] = {
 /* CLASS_WIZARD	*/ free_start_feats_wizard,
 /* CLASS_CLERIC	*/ free_start_feats_cleric,
 /* CLASS_ROGUE		*/ free_start_feats_rogue,
 /* CLASS_FIGHTER	*/ free_start_feats_fighter,
 /* CLASS_MONK		*/ free_start_feats_monk,
 /* CLASS_PALADIN	*/ free_start_feats_paladin,
 /* CLASS_BARBARIAN*/ free_start_feats_barbarian,
 /* CLASS_BARD  */   free_start_feats_bard,
 /* CLASS_RANGER*/   free_start_feats_ranger,
 /* CLASS_DRUID */   free_start_feats_druid,
 /* EXPANSION 1  */   free_start_feats_knight_of_crown,
 /* EXPANSION 2  */   free_start_feats_knight_of_sword,    
 /* EXPANSION 3  */   free_start_feats_knight_of_rose,
 /* EXPANSION 4  */   free_start_feats_none,
 /* EXPANSION 5  */   free_start_feats_none,
 /* EXPANSION 6  */   free_start_feats_none,    
 /* EXPANSION 7  */   free_start_feats_wizard_of_high_sorcery,
 /*CLASS_DUELIST*/   free_start_feats_duelist,
 /* EXPANSION 8  */   free_start_feats_gladiator,
 /* CLASS_MYST  */   free_start_feats_mystic,    
 /* CLASS_SORC  */   free_start_feats_sorcerer,
 /* CLASS_NOBL  */   free_start_feats_noble,
 /* EXPANSION 9  */   free_start_feats_none,
 /* EXPANSION 10 */   free_start_feats_none,       
 /* CLASS_DDIS  */   free_start_feats_none,       
 /* CLASS_AARC  */   free_start_feats_arcane_archer,       
 /* EXPANSION 11  */   free_start_feats_none,       
 /* CLASS_NPC   */   free_start_feats_none,       
 /* CLASS_ASSN  */   free_start_feats_none,       
 /* EXPANSION 12  */   free_start_feats_none,       
 /* CLASS_ARTSN */   free_start_feats_artisan,       
 /* CLASS_ATRK  */   free_start_feats_none,       
 /* EXPANSION 13  */   free_start_feats_favored_soul,   
 /* CLASS_ELDKN */   free_start_feats_none,       
 /* EXPANSION 14 */   free_start_feats_none,
 /* EXPANSION 15  */   free_start_feats_none,       
 /* EXPANSION 16  */   free_start_feats_none,       
 /* EXPANSION 17  */   free_start_feats_none,       
 /* EXPANSION 18  */   free_start_feats_none,       
 /* EXPANSION 19  */   free_start_feats_none,       
 /* EXPANSION 20  */   free_start_feats_none,       
 /* EXPANSION 21  */   free_start_feats_none,       
 /* EXPANSION 22  */   free_start_feats_none,       
 /* EXPANSION 23  */   free_start_feats_none,       
 /* EXPANSION 24  */   free_start_feats_none,       
 /* EXPANSION 25  */   free_start_feats_none,       
 /* EXPANSION 26  */   free_start_feats_none,       
 /* EXPANSION 27  */   free_start_feats_none,       
 /* EXPANSION 28  */   free_start_feats_none,       
 /* EXPANSION 29  */   free_start_feats_none,       
 /* EXPANSION 30  */   free_start_feats_none,       
 /* EXPANSION 31  */   free_start_feats_none,       
 /* EXPANSION 32  */   free_start_feats_none,       
 /* EXPANSION 33  */   free_start_feats_none,       
 /* EXPANSION 34  */   free_start_feats_none,       
 /* EXPANSION 35  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none,       
 /* CLASS_OPEN  */   free_start_feats_none
};

int level_feats[][6] = {
	
	// This array contains the information required for when a character levels.
	// The first field is the class the character must be or undefined if doesn't apply
	// The second field is the race the character must be or undefined if doesn't apply
	// The third field is the condition that must be met to receive the feat
	// The fourth field is whether the feat stacks or not.
	// The fifth field is the minimum level the character must be to receive the feat
	// The sixed field is the feat itself to be received
	
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  2, FEAT_UNCANNY_DODGE, TRUE},  
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  3, FEAT_TRAP_SENSE, TRUE},             
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  4, FEAT_RAGE, TRUE},   
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  5, FEAT_IMPROVED_UNCANNY_DODGE, TRUE},       
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  6, FEAT_TRAP_SENSE, TRUE},   
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  7, FEAT_DAMAGE_REDUCTION, TRUE},   
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  8, FEAT_RAGE, TRUE},
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  9, FEAT_TRAP_SENSE, TRUE},   
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  10, FEAT_DAMAGE_REDUCTION, TRUE},    
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  11, FEAT_GREATER_RAGE, TRUE},        
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  11, FEAT_RAGE, TRUE},
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  12, FEAT_TRAP_SENSE, TRUE},    
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  13, FEAT_DAMAGE_REDUCTION, TRUE},  
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  14, FEAT_INDOMITABLE_WILL, TRUE},      
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  15, FEAT_TRAP_SENSE, TRUE},    
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  15, FEAT_RAGE, TRUE},
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  16, FEAT_DAMAGE_REDUCTION, TRUE},  
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  17, FEAT_TIRELESS_RAGE, TRUE},     
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  18, FEAT_TRAP_SENSE, TRUE},    
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  19, FEAT_DAMAGE_REDUCTION, TRUE},      
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  20, FEAT_RAGE, TRUE},
  {CLASS_BARBARIAN, RACE_UNDEFINED,      FALSE,  20, FEAT_MIGHTY_RAGE, TRUE},

  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  1, FEAT_BARDIC_MUSIC, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  1, FEAT_BARDIC_KNOWLEDGE, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  1, FEAT_COUNTERSONG, TRUE},  
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  1, FEAT_FASCINATE, TRUE},  
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  1, FEAT_INSPIRE_COURAGE, TRUE},  
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  3, FEAT_INSPIRE_COMPETENCE, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  6, FEAT_SUGGESTION, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  8, FEAT_INSPIRE_COURAGE, TRUE},  
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  9, FEAT_INSPIRE_GREATNESS, TRUE},  
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  12, FEAT_SONG_OF_FREEDOM, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  14, FEAT_INSPIRE_COURAGE, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  15, FEAT_INSPIRE_HEROICS, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  18, FEAT_MASS_SUGGESTION, TRUE}, 
  {CLASS_BARD,      RACE_UNDEFINED,      FALSE,  20, FEAT_INSPIRE_COURAGE, TRUE}, 

  // Left as a placeholder for cleric.
  // {CLASS_CLERIC,    RACE_UNDEFINED,      FALSE,  1, FEAT_TURN_UNDEAD, TRUE},

  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  2, FEAT_WOODLAND_STRIDE, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  3, FEAT_TRACKLESS_STEP, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  4, FEAT_RESIST_NATURES_LURE, TRUE},
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  4, FEAT_WILD_SHAPE, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  6, FEAT_WILD_SHAPE, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  8, FEAT_WILD_SHAPE, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  9, FEAT_VENOM_IMMUNITY, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  10, FEAT_WILD_SHAPE, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  12, FEAT_WILD_SHAPE, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  13, FEAT_THOUSAND_FACES, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  14, FEAT_WILD_SHAPE, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  15, FEAT_TIMELESS_BODY, TRUE}, 
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  16, FEAT_WILD_SHAPE, TRUE},  
  {CLASS_DRUID,     RACE_UNDEFINED,      FALSE,  18, FEAT_WILD_SHAPE, TRUE},  
  
	// Left as a placeholder for fighter.
  // {CLASS_FIGHTER,   RACE_UNDEFINED,      FALSE,  1, FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, TRUE},	

  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  2, FEAT_EVASION, TRUE},    
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  3, FEAT_STILL_MIND, TRUE},   
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  4, FEAT_KI_STRIKE, TRUE},  
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  4, FEAT_SLOW_FALL, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  5, FEAT_SLOW_FALL, TRUE}, // Yes, twice =)
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 5, FEAT_PURITY_OF_BODY, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 6, FEAT_SLOW_FALL, TRUE},     
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 7, FEAT_WHOLENESS_OF_BODY, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 8, FEAT_SLOW_FALL, TRUE}, 
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 9, FEAT_IMPROVED_EVASION, TRUE},    
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 10, FEAT_KI_STRIKE, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 10, FEAT_SLOW_FALL, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 11, FEAT_DIAMOND_BODY, TRUE},     
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 11, FEAT_GREATER_FLURRY, TRUE}, 
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 12, FEAT_ABUNDANT_STEP, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE, 12, FEAT_SLOW_FALL, TRUE},    
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  13, FEAT_DIAMOND_SOUL, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  14, FEAT_SLOW_FALL, TRUE},   
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  15, FEAT_QUIVERING_PALM, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  15, FEAT_KI_STRIKE, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  32, FEAT_SLOW_FALL, TRUE},     
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  16, FEAT_TIMELESS_BODY, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  17, FEAT_TONGUE_OF_THE_SUN_AND_MOON, TRUE},    
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  18, FEAT_SLOW_FALL, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  19, FEAT_EMPTY_BODY, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  20, FEAT_PERFECT_SELF, TRUE},
  {CLASS_MONK,      RACE_UNDEFINED,      FALSE,  20, FEAT_SLOW_FALL, TRUE},       

  {CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  2, FEAT_DIVINE_GRACE, TRUE},	
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  2, FEAT_LAYHANDS, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  3, FEAT_AURA_OF_COURAGE, TRUE},		
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  3, FEAT_DIVINE_HEALTH, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  3, FEAT_TURN_UNDEAD, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  5, FEAT_CALL_MOUNT, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  5, FEAT_DIVINE_BOND, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  5, FEAT_SMITE_EVIL, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  6, FEAT_REMOVE_DISEASE, TRUE},			
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE,  9, FEAT_REMOVE_DISEASE, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 10, FEAT_SMITE_EVIL, TRUE},			
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 12, FEAT_REMOVE_DISEASE, TRUE},			
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 14, FEAT_REMOVE_DISEASE, TRUE},		
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 15, FEAT_SMITE_EVIL, TRUE},
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 18, FEAT_REMOVE_DISEASE, TRUE},			
	{CLASS_PALADIN,   RACE_UNDEFINED,      FALSE, 20, FEAT_SMITE_EVIL, TRUE},			

  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  1, FEAT_TRACK, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  2, FEAT_WILD_EMPATHY, TRUE},
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  2, FEAT_TWO_WEAPON_FIGHTING, TRUE},
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  2, FEAT_RAPID_SHOT, TRUE},
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  3, FEAT_ENDURANCE, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  4, FEAT_ANIMAL_COMPANION, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  5, FEAT_FAVORED_ENEMY_AVAILABLE, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  6, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  6, FEAT_MANYSHOT, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  7, FEAT_WOODLAND_STRIDE, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  8, FEAT_SWIFT_TRACKER, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  9, FEAT_EVASION, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  10, FEAT_FAVORED_ENEMY_AVAILABLE, TRUE},   
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  11, FEAT_GREATER_TWO_WEAPON_FIGHTING, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  11, FEAT_IMPROVED_PRECISE_SHOT, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  13, FEAT_CAMOUFLAGE, TRUE},  
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  15, FEAT_FAVORED_ENEMY_AVAILABLE, TRUE},   
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  17, FEAT_HIDE_IN_PLAIN_SIGHT, TRUE}, 
  {CLASS_RANGER,    RACE_UNDEFINED,      FALSE,  20, FEAT_FAVORED_ENEMY_AVAILABLE, TRUE},
  
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   1, FEAT_SNEAK_ATTACK,TRUE},
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   1, FEAT_TRAPFINDING, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   2, FEAT_EVASION, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   3, FEAT_SNEAK_ATTACK, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   3, FEAT_TRAP_SENSE, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   4, FEAT_UNCANNY_DODGE, TRUE },		
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   5, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   6, FEAT_TRAP_SENSE, TRUE },		
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   7, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   8, FEAT_IMPROVED_UNCANNY_DODGE, TRUE },		
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   9, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   9, FEAT_TRAP_SENSE, TRUE },		
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,   11, FEAT_SNEAK_ATTACK, TRUE },
  {CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  12, FEAT_TRAP_SENSE, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  13, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  15, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  15, FEAT_TRAP_SENSE, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  17, FEAT_SNEAK_ATTACK, TRUE },
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  18, FEAT_TRAP_SENSE, TRUE },	
	{CLASS_ROGUE,     RACE_UNDEFINED,      TRUE,  19, FEAT_SNEAK_ATTACK, TRUE },								

  // {CLASS_WIZARD,    RACE_UNDEFINED,      TRUE,   6, FEAT_SCRIBE_SCROLL, TRUE },   

  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 1, FEAT_NATURAL_ARMOR_INCREASE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 2, FEAT_STRENGTH_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 2, FEAT_CLAWS_AND_BITE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 3, FEAT_BREATH_WEAPON, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 4, FEAT_NATURAL_ARMOR_INCREASE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 4, FEAT_STRENGTH_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 5, FEAT_BLINDSENSE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 6, FEAT_CONSTITUTION_BOOST, TRUE},
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 7, FEAT_BREATH_WEAPON, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 7, FEAT_NATURAL_ARMOR_INCREASE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 8, FEAT_INTELLIGENCE_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 9, FEAT_WINGS, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_DRAGON_APOTHEOSIS, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_BLINDSENSE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_NATURAL_ARMOR_INCREASE, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_STRENGTH_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_STRENGTH_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_CHARISMA_BOOST, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_BREATH_WEAPON, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_LOW_LIGHT_VISION, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_DARKVISION, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_SLEEP_PARALYSIS_IMMUNITY, TRUE},	
  {CLASS_DRAGON_DISCIPLE, RACE_UNDEFINED, FALSE, 10, FEAT_ELEMENTAL_IMMUNITY, TRUE},	

  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 1,  FEAT_LEARNED_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 2,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 4,  FEAT_PROFICIENT_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 6,  FEAT_BRANDING, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 8,  FEAT_PROFICIENT_HARVESTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 10,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 12,  FEAT_SCAVENGE, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 14,  FEAT_PROFICIENT_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 16,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 18,  FEAT_PROFICIENT_HARVESTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 20,  FEAT_MASTERWORK_CRAFTING, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 22,  FEAT_PROFICIENT_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 24,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 26,  FEAT_PROFICIENT_HARVESTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 28,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 30,  FEAT_ELVEN_CRAFTING, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 32,  FEAT_PROFICIENT_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 34,  FEAT_PROFICIENT_HARVESTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 36,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 38,  FEAT_PROFICIENT_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 40,  FEAT_DWARVEN_CRAFTING, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 43,  FEAT_FAST_CRAFTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 46,  FEAT_PROFICIENT_HARVESTER, TRUE},	
  {CLASS_ARTISAN,         RACE_UNDEFINED, FALSE, 50,  FEAT_DRACONIC_CRAFTING, TRUE},	

  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  1,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  1,  FEAT_DEATH_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  1,  FEAT_POISON_USE, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  2,  FEAT_POISON_SAVE_BONUS, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  2,  FEAT_UNCANNY_DODGE, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  3,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  4,  FEAT_POISON_SAVE_BONUS, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  5,  FEAT_IMPROVED_UNCANNY_DODGE, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  5,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  6,  FEAT_POISON_SAVE_BONUS, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  7,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  8,  FEAT_POISON_SAVE_BONUS, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  8,  FEAT_HIDE_IN_PLAIN_SIGHT, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  9,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ASSASSIN,        RACE_UNDEFINED, FALSE,  10,  FEAT_POISON_SAVE_BONUS, TRUE},	

  {CLASS_ARCANE_TRICKSTER,RACE_UNDEFINED, FALSE,  2,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ARCANE_TRICKSTER,RACE_UNDEFINED, FALSE,  4,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ARCANE_TRICKSTER,RACE_UNDEFINED, FALSE,  6,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ARCANE_TRICKSTER,RACE_UNDEFINED, FALSE,  8,  FEAT_SNEAK_ATTACK, TRUE},	
  {CLASS_ARCANE_TRICKSTER,RACE_UNDEFINED, FALSE, 10,  FEAT_SNEAK_ATTACK, TRUE},	

  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  1,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  3,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  4,  FEAT_ENHANCE_ARROW_ELEMENTAL, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  5,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  6,  FEAT_ENHANCE_ARROW_DISTANCE, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  7,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  8,  FEAT_ENHANCE_ARROW_ELEMENTAL_BURST, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  9,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  10,  FEAT_ENHANCE_ARROW_ALIGNED, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  11,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  13,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  15,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  17,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	
  {CLASS_ARCANE_ARCHER,   RACE_UNDEFINED, FALSE,  19,  FEAT_ENHANCE_ARROW_MAGIC, TRUE},	

  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE,  3,  FEAT_WEAPON_FOCUS, TRUE},	
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE,  3,  FEAT_ENERGY_RESISTANCE, TRUE},
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE,  5,  FEAT_ENERGY_RESISTANCE, TRUE},
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE, 10,  FEAT_ENERGY_RESISTANCE, TRUE},
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE, 12,  FEAT_WEAPON_SPECIALIZATION, TRUE},
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE, 15,  FEAT_ENERGY_RESISTANCE, TRUE},
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE, 17,  FEAT_WINGS, TRUE},	
  {CLASS_FAVORED_SOUL,    RACE_UNDEFINED, FALSE, 20,  FEAT_ENERGY_RESISTANCE, TRUE},

  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 1,  FEAT_DRAGON_MOUNT_BOOST, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 1,  FEAT_DRAGON_MOUNT_BREATH, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 3,  FEAT_DRAGON_MOUNT_BOOST, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 5,  FEAT_DRAGON_MOUNT_BOOST, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 6,  FEAT_DRAGON_MOUNT_BREATH, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 7,  FEAT_DRAGON_MOUNT_BOOST, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 9,  FEAT_DRAGON_MOUNT_BOOST, TRUE},
  {CLASS_DRAGON_RIDER,    RACE_UNDEFINED, FALSE, 9,  FEAT_DRAGON_MOUNT_BREATH, TRUE},


#if defined(CAMPAIGN_FORGOTTEN_REALMS)
	{CLASS_UNDEFINED, RACE_MOON_ELF,       FALSE,  1, FEAT_LOW_LIGHT_VISION, TRUE},
	{CLASS_UNDEFINED, RACE_MOON_ELF,       FALSE,  1, FEAT_WEAPON_PROFICIENCY_ELF, TRUE},
#endif
	{CLASS_WIZARD,    RACE_UNDEFINED,      FALSE,  1, FEAT_SUMMON_FAMILIAR, TRUE},
  {CLASS_WIZARD,    RACE_UNDEFINED,      FALSE,  1, FEAT_SCRIBE_SCROLL, TRUE},
  
  {CLASS_SORCERER,  RACE_UNDEFINED,      FALSE,  1, FEAT_ESCHEW_MATERIALS, TRUE},
  {CLASS_SORCERER,  RACE_UNDEFINED,      FALSE,  1, FEAT_SUMMON_FAMILIAR, TRUE},

	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 1,  FEAT_STRENGTH_OF_HONOR, TRUE},	
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 1,  FEAT_KNIGHTLY_COURAGE, TRUE},				
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 2,  FEAT_HEROIC_INITIATIVE, TRUE},
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 3,  FEAT_DIEHARD, TRUE},
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 4,  FEAT_HONORABLE_WILL, TRUE},		
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 4,  FEAT_STRENGTH_OF_HONOR, TRUE},			
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 5,  FEAT_HEROIC_INITIATIVE, TRUE},
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 6,  FEAT_MIGHT_OF_HONOR, TRUE},	
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 6,  FEAT_ARMORED_MOBILITY, TRUE},	
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 7,  FEAT_STRENGTH_OF_HONOR, TRUE},			
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 8,  FEAT_HEROIC_INITIATIVE, TRUE},		
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 9,  FEAT_AURA_OF_COURAGE, TRUE},
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 10,  FEAT_CROWN_OF_KNIGHTHOOD, TRUE},		
	{CLASS_KNIGHT_OF_THE_CROWN, RACE_UNDEFINED, FALSE, 10,  FEAT_STRENGTH_OF_HONOR, TRUE},		

	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 1,  FEAT_SMITE_EVIL, TRUE},	
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 2,  FEAT_AURA_OF_COURAGE, TRUE},		
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 2,  FEAT_TURN_UNDEAD, TRUE},
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 3,  FEAT_AURA_OF_COURAGE, TRUE},
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 5,  FEAT_SMITE_EVIL, TRUE},	
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 10, FEAT_SMITE_EVIL, TRUE},	
	{CLASS_KNIGHT_OF_THE_SWORD, RACE_UNDEFINED, FALSE, 10, FEAT_SOUL_OF_KNIGHTHOOD, TRUE},		

	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 1,  FEAT_DETECT_EVIL, TRUE},
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 1,  FEAT_RALLYING_CRY, TRUE},		
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 1,  FEAT_AURA_OF_GOOD, TRUE},
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 2,  FEAT_INSPIRE_COURAGE, TRUE},
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 3,  FEAT_DIVINE_GRACE, TRUE},		
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 3,  FEAT_LEADERSHIP, TRUE},
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 4,  FEAT_INSPIRE_GREATNESS, TRUE},	
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 4,  FEAT_TURN_UNDEAD, TRUE},				
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 5,  FEAT_INSPIRE_COURAGE, TRUE},				
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 6,  FEAT_WISDOM_OF_THE_MEASURE, TRUE},	
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 7,  FEAT_LEADERSHIP, TRUE},	
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 8,  FEAT_INSPIRE_COURAGE, TRUE},	
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 9,  FEAT_FINAL_STAND, TRUE},	
	{CLASS_KNIGHT_OF_THE_ROSE,  RACE_UNDEFINED, FALSE, 10,  FEAT_KNIGHTHOODS_FLOWER, TRUE},	

	{CLASS_DUELIST,   	    RACE_UNDEFINED, FALSE,  1,  FEAT_CANNY_DEFENSE, TRUE},	
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  2,  FEAT_IMPROVED_REACTION, TRUE},
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  2,  FEAT_PARRY, TRUE},
	{CLASS_DUELIST,   	    RACE_UNDEFINED, FALSE,  3,  FEAT_ENHANCED_MOBILITY, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  4,  FEAT_COMBAT_REFLEXES, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  4,  FEAT_GRACE, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  5,  FEAT_RIPOSTE, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  6,  FEAT_ACROBATIC_CHARGE, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  7,  FEAT_ELABORATE_PARRY, TRUE},		
	{CLASS_DUELIST,  	    RACE_UNDEFINED, FALSE,  8,  FEAT_IMPROVED_REACTION, TRUE},
	{CLASS_DUELIST, 	    RACE_UNDEFINED, FALSE,  9,  FEAT_DEFLECT_ARROWS, TRUE},	
	{CLASS_DUELIST, 	    RACE_UNDEFINED, FALSE,  9,  FEAT_NO_RETREAT, TRUE},	
	{CLASS_DUELIST,   	    RACE_UNDEFINED, FALSE,  10, FEAT_CRIPPLING_CRITICAL, TRUE},

	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 1,  FEAT_SNEAK_ATTACK, TRUE},		
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 2,  FEAT_DEMORALIZE, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 3,  FEAT_DIEHARD, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 3,  FEAT_SNEAK_ATTACK, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 4,  FEAT_UNBREAKABLE_WILL, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 6,  FEAT_ARMORED_MOBILITY, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 7,  FEAT_SNEAK_ATTACK, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 8,  FEAT_UNBREAKABLE_WILL, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 10,  FEAT_ONE_THOUGHT, TRUE},	
	{CLASS_KNIGHT_OF_THE_LILY,  RACE_UNDEFINED, FALSE, 10,  FEAT_SNEAK_ATTACK, TRUE},	

	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 1,  FEAT_DETECT_GOOD, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 1,  FEAT_SMITE_GOOD, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 2,  FEAT_AURA_OF_EVIL, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 2,  FEAT_DARK_BLESSING, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 3,  FEAT_DISCERN_LIES, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 3,  FEAT_TURN_UNDEAD, TRUE},
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 5,  FEAT_SMITE_GOOD, TRUE},	
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 10,  FEAT_FAVOR_OF_DARKNESS, TRUE},						
	{CLASS_KNIGHT_OF_THE_SKULL,  RACE_UNDEFINED, FALSE, 10,  FEAT_SMITE_GOOD, TRUE},

	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 1,  FEAT_DIVINER, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 1,  FEAT_READ_OMENS, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 2,  FEAT_ARMORED_SPELLCASTING, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 3,  FEAT_AURA_OF_TERROR, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 4,  FEAT_WEAPON_TOUCH, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 5,  FEAT_ARMORED_SPELLCASTING, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 6,  FEAT_READ_PORTENTS, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 6,  FEAT_ARMORED_SPELLCASTING, TRUE},									
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 8,  FEAT_ARMORED_SPELLCASTING, TRUE},		
	{CLASS_KNIGHT_OF_THE_THORN,  RACE_UNDEFINED, FALSE, 10,  FEAT_COSMIC_UNDERSTANDING, TRUE},				

	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 1,   FEAT_DAMAGE_REDUCTION},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 1,   FEAT_DEFENSIVE_STANCE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 2,   FEAT_DAMAGE_REDUCTION},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 2,   FEAT_UNCANNY_DODGE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 3,   FEAT_DEFENSIVE_STANCE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 4,   FEAT_TRAP_SENSE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 4,   FEAT_DAMAGE_REDUCTION},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 5,   FEAT_DEFENSIVE_STANCE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 6,   FEAT_IMPROVED_UNCANNY_DODGE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 6,   FEAT_DAMAGE_REDUCTION},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 7,   FEAT_DEFENSIVE_STANCE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 8,   FEAT_DAMAGE_REDUCTION},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 8,   FEAT_MOBILE_DEFENSE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 8,   FEAT_TRAP_SENSE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 9,   FEAT_DEFENSIVE_STANCE},
	{CLASS_DWARVEN_DEFENDER,    RACE_UNDEFINED, FALSE, 10,  FEAT_DAMAGE_REDUCTION},

	{CLASS_WEAPON_MASTER,       RACE_UNDEFINED, FALSE, 1,  FEAT_WEAPON_OF_CHOICE},
	{CLASS_WEAPON_MASTER,       RACE_UNDEFINED, FALSE, 1,  FEAT_KI_DAMAGE},
	{CLASS_WEAPON_MASTER,       RACE_UNDEFINED, FALSE, 4,  FEAT_INCREASED_MULTIPLIER},
	{CLASS_WEAPON_MASTER,       RACE_UNDEFINED, FALSE, 7,  FEAT_SUPERIOR_WEAPON_FOCUS},
	{CLASS_WEAPON_MASTER,       RACE_UNDEFINED, FALSE, 10,  FEAT_KI_CRITICAL},


  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 1, FEAT_ARMOR_PROFICIENCY_LIGHT},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 1, FEAT_BONE_ARMOR},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 2, FEAT_ANIMATE_DEAD},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 3, FEAT_UNDEAD_FAMILIAR},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 4, FEAT_SUMMON_UNDEAD},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 5, FEAT_WEAPON_FOCUS},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 6, FEAT_ARMOR_PROFICIENCY_MEDIUM},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 6, FEAT_BONE_ARMOR},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 7, FEAT_WEAPON_SPECIALIZATION},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 8, FEAT_SUMMON_GREATER_UNDEAD},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 9, FEAT_TOUCH_OF_UNDEATH},
  {CLASS_DEATH_MASTER,       RACE_UNDEFINED, FALSE, 10, FEAT_ESSENCE_OF_UNDEATH},

	// This is always the last array element
	{CLASS_UNDEFINED, RACE_UNDEFINED,      FALSE,   1, FEAT_UNDEFINED, FALSE}
};

int epic_level_feats[][7] =
{
  { CLASS_ROGUE, 0, 2, 1, TRUE, FEAT_SNEAK_ATTACK, 1},
  { CLASS_ROGUE, 0, 4, 0, TRUE, FEAT_TRAP_SENSE, 1},
  { CLASS_BARBARIAN, 0, 3, 0, TRUE, FEAT_TRAP_SENSE, 1},
  { CLASS_BARBARIAN, 0, 3, 1, TRUE, FEAT_DAMAGE_REDUCTION, 1},
  { CLASS_BARBARIAN, 0, 4, 0, FALSE, FEAT_RAGE, 1},
  { CLASS_DRUID, -2, 4, 0, TRUE, FEAT_WILD_SHAPE, 1},
  { CLASS_PALADIN, 0, 5, 0, TRUE, FEAT_SMITE_EVIL, 1},
  { CLASS_PALADIN, 0, 3, 0, TRUE, FEAT_REMOVE_DISEASE, 1},
  { CLASS_RANGER, 0, 5, 0, TRUE, FEAT_FAVORED_ENEMY_AVAILABLE, 1},
  { CLASS_ASSASSIN, 0, 2, 1, TRUE, FEAT_SNEAK_ATTACK, 1},
  { CLASS_ASSASSIN, 0, 2, 0, TRUE, FEAT_POISON_SAVE_BONUS, 1},
  { CLASS_DRAGON_DISCIPLE, 1, 4, 0, TRUE, FEAT_BREATH_WEAPON, 1},
  { CLASS_TEMPLAR, 0, 5, 0, TRUE, FEAT_SMITE_EVIL, 1},
  { CLASS_KNIGHT, 0, 3, 1, TRUE, FEAT_STRENGTH_OF_HONOR, 1},
  { CLASS_KNIGHT, 0, 3, 2, TRUE, FEAT_HEROIC_INITIATIVE, 1},
  { CLASS_CHAMPION, 0, 4, 3, TRUE, FEAT_LEADERSHIP, 1},
  { CLASS_CHAMPION, 0, 4, 0, TRUE, FEAT_INSPIRE_COURAGE, 1},
  { CLASS_FAVORED_SOUL, 0, 5, 0, TRUE, FEAT_ENERGY_RESISTANCE, 1},
  { CLASS_DWARVEN_DEFENDER, 1, 2, 0, TRUE, FEAT_DEFENSIVE_STANCE, 1},
  { CLASS_DRAGON_RIDER, 0, 2, 1, TRUE, FEAT_DRAGON_MOUNT_BOOST, 1},
  { CLASS_DRAGON_RIDER, 0, 3, 0, TRUE, FEAT_DRAGON_MOUNT_BREATH, 1},
  { CLASS_WEAPON_MASTER, 0, 4, 0, TRUE, FEAT_KI_DAMAGE, 1},

  // This is always the last one
  { CLASS_UNDEFINED, 0, 0, 0, TRUE, FEAT_UNDEFINED, 0}

};

int deity_domains[][4] = {

// Good Deities	
/* 0 Paladine */    { DOMAIN_LAW,         DOMAIN_GOOD,       DOMAIN_PROTECTION,  DOMAIN_UNDEFINED },
/* 1 Branchala */   { DOMAIN_CHAOS,       DOMAIN_GOOD,       DOMAIN_LUCK,        DOMAIN_TRICKERY  },
/* 2 Habbakuk */    { DOMAIN_ANIMAL,      DOMAIN_GOOD,       DOMAIN_WATER,       DOMAIN_UNDEFINED },
/* 3 Kiri-Jolith */ { DOMAIN_GOOD,        DOMAIN_STRENGTH,   DOMAIN_WAR,         DOMAIN_UNDEFINED },
/* 4 Majere */      { DOMAIN_GOOD,        DOMAIN_LAW,        DOMAIN_MEDITATION,  DOMAIN_UNDEFINED },
/* 5 Mishakal */    { DOMAIN_GOOD,        DOMAIN_HEALING,    DOMAIN_PROTECTION,   DOMAIN_COMMUNITY },
/* 6 Solinari */    { DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED,  DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED },

// Neutral Deities
/* 7 Chislev */     { DOMAIN_AIR,         DOMAIN_ANIMAL,     DOMAIN_PLANT,       DOMAIN_EARTH     },
/* 8 Gilean */      { DOMAIN_KNOWLEDGE,   DOMAIN_LIBERATION, DOMAIN_PROTECTION,  DOMAIN_UNDEFINED },
/* 9 Lunitari */    { DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED,  DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED },
/* 10 Reorx */      { DOMAIN_EARTH,       DOMAIN_FIRE,       DOMAIN_FORGE,       DOMAIN_UNDEFINED },
/* 11 Shinare */    { DOMAIN_LAW,         DOMAIN_LUCK,       DOMAIN_TRAVEL,      DOMAIN_UNDEFINED },
/* 12 Sirrion */    { DOMAIN_CHAOS,       DOMAIN_FIRE,       DOMAIN_PASSION,     DOMAIN_UNDEFINED },
/* 13 Zivilyn */    { DOMAIN_INSIGHT,     DOMAIN_KNOWLEDGE,  DOMAIN_MEDITATION,  DOMAIN_UNDEFINED },

// Evil Deities
/* 14 Takhisis */   { DOMAIN_DESTRUCTION, DOMAIN_EVIL,       DOMAIN_TREACHERY,   DOMAIN_UNDEFINED },
/* 15 Chemosh */    { DOMAIN_DEATH,       DOMAIN_EVIL,       DOMAIN_TRICKERY,    DOMAIN_UNDEFINED },
/* 16 Hiddukel */   { DOMAIN_EVIL,        DOMAIN_TREACHERY,  DOMAIN_TRICKERY,    DOMAIN_UNDEFINED },
/* 17 Morgion */    { DOMAIN_DESTRUCTION, DOMAIN_EVIL,       DOMAIN_PESTILENCE,  DOMAIN_UNDEFINED },
/* 18 Nuitari */    { DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED,  DOMAIN_UNDEFINED,   DOMAIN_UNDEFINED },
/* 19 Sargonnas */  { DOMAIN_EVIL,        DOMAIN_FIRE,       DOMAIN_LAW,         DOMAIN_WAR       },
/* 20 Zeboim */     { DOMAIN_CHAOS,       DOMAIN_EVIL,       DOMAIN_STORM,       DOMAIN_WATER     }
};

int deity_alignments[][9] =
{

//                   LG, NG, CG, LN, TN, CN, LE, NE, CE
// Good Deities	
/* 0 Paladine */    { Y,  Y,  N,  Y,  N,  N,  N,  N,  N },
/* 1 Branchala */   { N,  Y,  Y,  N,  N,  Y,  N,  N,  N },
/* 2 Habbakuk */    { Y,  Y,  Y,  N,  N,  N,  N,  N,  N },
/* 3 Kiri-Jolith */ { Y,  Y,  N,  Y,  N,  N,  N,  N,  N },
/* 4 Majere */      { Y,  Y,  N,  Y,  N,  N,  N,  N,  N },
/* 5 Mishakal */    { Y,  Y,  Y,  N,  N,  N,  N,  N,  N },
/* 6 Solinari */    { Y,  Y,  N,  Y,  N,  N,  N,  N,  N },

// Neutral Deities
/* 7 Chislev */     { N,  Y,  N,  Y,  Y,  Y,  N,  Y,  N },
/* 8 Gilean */      { N,  Y,  N,  Y,  Y,  Y,  N,  Y,  N },
/* 9 Lunitari */    { N,  Y,  N,  Y,  Y,  Y,  N,  Y,  N },
/* 10 Reorx */      { N,  Y,  N,  Y,  Y,  Y,  N,  Y,  N },
/* 11 Shinare */    { N,  N,  N,  Y,  Y,  N,  N,  N,  N },
/* 12 Sirrion */    { N,  N,  Y,  N,  N,  Y,  N,  N,  Y },
/* 13 Zivilyn */    { N,  Y,  N,  Y,  Y,  Y,  N,  Y,  N },

// Evil Deities
/* 14 Takhisis */   { N,  N,  N,  N,  N,  N,  Y,  Y,  Y },
/* 15 Chemosh */    { N,  N,  N,  N,  N,  N,  Y,  Y,  Y }, 
/* 16 Hiddukel */   { N,  N,  N,  N,  N,  Y,  N,  Y,  Y },
/* 17 Morgion */    { N,  N,  N,  N,  N,  N,  Y,  Y,  Y },
/* 18 Nuitari */    { N,  N,  N,  Y,  N,  N,  Y,  Y,  N },
/* 19 Sargonnas */  { N,  N,  N,  Y,  N,  N,  Y,  Y,  N },
/* 20 Zeboim */     { N,  N,  N,  N,  N,  Y,  N,  Y,  Y },
};

/*
 * this function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */

void advance_level(struct char_data *ch, int whichclass) {
	do_advance_level(ch, whichclass, TRUE);
}

void do_advance_level(struct char_data *ch, int whichclass, int manual)
{
  struct levelup_data *llog;
  int add_hp =0, add_move = 0, add_mana = 0, add_prac = 1, add_ki = 0, add_train, i, q=0, j = 0, n = 0, m = 0, ranks;
  int add_acc = 0, add_fort = 0, add_reflex = 0, add_will = 0;
  int add_gen_feats = 0, add_class_feats = 0;
  int evasion = false, impEvasion = false, impDisarm = false, combatReflexes = false, sneakAttack = false;
  int layhands = false;
  int bardLevel = 0;
  int addSpells = 0;
  int level = 0;
  char buf[MAX_STRING_LENGTH]={'\0'};
  char featbuf[MAX_STRING_LENGTH]={'\0'};
  int research_sessions = 0;
  struct damreduct_type *ptr, *reduct, *temp;
  int arcane_bonus = 0, divine_bonus = 0;
  int epiclevel = 21;

  if (whichclass < 0 || whichclass >= NUM_CLASSES) {
    log("Invalid class %d passed to advance_level, resetting.", whichclass);
    whichclass = 0;
  }

  if (!CONFIG_ALLOW_MULTICLASS && whichclass != GET_CLASS(ch)) {
    log("Attempt to gain a second class without multiclass enabled for %s", GET_NAME(ch));
    whichclass = GET_CLASS(ch);
  }

  GET_CLASS(ch) = whichclass;

  if (whichclass == CLASS_ROGUE)
      SET_SKILL(ch, SKILL_LANG_THIEVES_CANT, 1);
  if (whichclass == CLASS_DRUID)
      SET_SKILL(ch, SKILL_LANG_DRUIDIC, 1);

  if (GET_CLASS_LEVEL(ch) == 0) {
    do_start(ch);
  } 

  if ((CAMPAIGN_FORGOTTEN_REALMS == CONFIG_CAMPAIGN ? prestige_classes_core[whichclass] : prestige_classes_dl_aol[whichclass]) ? 
       GET_CLASS_LEVEL(ch) >= LVL_EPICSTART - 10 : GET_CLASS_LEVEL(ch) >= LVL_EPICSTART-1) { /* Epic character */
    GET_CLASS_EPIC(ch, whichclass)++;
  } else {
    GET_CLASS_NONEPIC(ch, whichclass)++;
  }

  if (GET_CLASS_RANKS(ch, whichclass) > 0) {
    for (m = 0; (n = free_start_feats[GET_CLASS(ch)][m]); m++) {
      if (!HAS_REAL_FEAT(ch, n)) {
        send_to_char(ch, "@YYou have gained the %s feat!\r\n", feat_list[n].name);
        SET_FEAT(ch, n, 1);
      }
    }
  }


  i = 0;
  sprintf(featbuf, "@n");
  while (level_feats[i][4] != FEAT_UNDEFINED) {
    if (GET_CLASS(ch) == level_feats[i][0] && level_feats[i][1] == RACE_UNDEFINED && GET_CLASS_RANKS(ch, level_feats[i][0]) >= level_feats[i][3]) {

      if (!((!HAS_REAL_FEAT(ch, level_feats[i][4]) && GET_CLASS_RANKS(ch, level_feats[i][0]) > level_feats[i][3] &&
              GET_CLASS_RANKS(ch, level_feats[i][0]) > 0) ||
          GET_CLASS_RANKS(ch, level_feats[i][0]) == level_feats[i][3])) {
        i++;
        continue;
      }

      if (level_feats[i][4] == FEAT_SNEAK_ATTACK)
        sprintf(featbuf, "%s@YYour sneak attack has increased to +%dd6!@n\r\n", featbuf, HAS_FEAT(ch, FEAT_SNEAK_ATTACK) + 1);    	
      else if (level_feats[i][4] == FEAT_WEAPON_FOCUS) {
        if (whichclass == CLASS_FAVORED_SOUL) {
          SET_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, deity_list[GET_DEITY(ch)].favored_weapon);
          sprintf(featbuf, "%s@YYou have gained weapon focus in your deity's favored weapon: %s.@n\r\n", featbuf, 
                  weapon_list[deity_list[GET_DEITY(ch)].favored_weapon].name);
        }
        else if (whichclass == CLASS_DEATH_MASTER) {
          SET_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_SCYTHE);
          sprintf(featbuf, "%s@YYou have gained weapon focus in the scythe.@n\r\n", featbuf);
        }
      }
      else if (level_feats[i][4] == FEAT_WEAPON_SPECIALIZATION) {
        if (whichclass == CLASS_FAVORED_SOUL) {
          SET_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION, deity_list[GET_DEITY(ch)].favored_weapon);
          sprintf(featbuf, "%s@YYou have gained weapon specialization in your deity's favored weapon: %s.@n\r\n", featbuf, 
                  weapon_list[deity_list[GET_DEITY(ch)].favored_weapon].name);
        }
        else if (whichclass == CLASS_DEATH_MASTER) {
          SET_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION, WEAPON_TYPE_SCYTHE);
          sprintf(featbuf, "%s@YYou have gained weapon specialization in the scythe.@n\r\n", featbuf);
        }
      }
      else if (level_feats[i][4] == FEAT_DAMAGE_REDUCTION) {
        for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
          if (reduct->feat == FEAT_DAMAGE_REDUCTION) {
            REMOVE_FROM_LIST(reduct, ch->damreduct, next);
          }
        }
        CREATE(ptr, struct damreduct_type, 1);
        ptr->next = ch->damreduct;
        ch->damreduct = ptr;
        ptr->spell = 0;
        ptr->feat = FEAT_DAMAGE_REDUCTION;
        ptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) + 1;
        ptr->duration = -1;
        ptr->max_damage = -1;
        for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
          ptr->damstyle[q] = ptr->damstyleval[q] = 0;
        ptr->damstyle[0] = DR_NONE;
      }
      else if (level_feats[i][4] == FEAT_STRENGTH_BOOST) {
        ch->real_abils.str += 2;
        sprintf(featbuf, "%s@YYour natural strength has increased by +2!\r\n", featbuf);
      }
      else if (level_feats[i][4] == FEAT_CHARISMA_BOOST) {
        ch->real_abils.cha += 2;
        sprintf(featbuf, "%s@YYour natural charisma has increased by +2!\r\n", featbuf);
      }
      else if (level_feats[i][4] == FEAT_CONSTITUTION_BOOST) {
        ch->real_abils.con += 2;
        sprintf(featbuf, "%s@YYour natural constitution has increased by +2!\r\n", featbuf);
      }
      else if (level_feats[i][4] == FEAT_INTELLIGENCE_BOOST) {
        ch->real_abils.intel += 2;
        sprintf(featbuf, "%s@YYour natural intelligence has increased by +2!\r\n", featbuf);
      }
      else if (level_feats[i][4] == FEAT_WINGS) {
        SET_FEAT(ch, FEAT_WINGS, HAS_REAL_FEAT(ch, FEAT_WINGS) + 1);
        sprintf(featbuf, "%s@YYou have grown a set of %s wings!\r\n", featbuf,
                GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE) ? "draconic" :
                  (IS_GOOD(ch) ? "feathered" : "batlike"));
      }
      else if (level_feats[i][4] == FEAT_DRAGON_APOTHEOSIS) {
        sprintf(featbuf, "%s@YYour old race has been converted into a half dragon!\r\n", featbuf);
      }
      else {
        if (HAS_FEAT(ch, level_feats[i][4]))
          sprintf(featbuf, "%s@YYou have improved your %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
        else 
          sprintf(featbuf, "%s@YYou have gained the %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
      }
      SET_FEAT(ch, level_feats[i][4], HAS_REAL_FEAT(ch, level_feats[i][4]) + 1);
    }
    else if (level_feats[i][0] == CLASS_UNDEFINED && level_feats[i][1] == GET_RACE(ch) && !HAS_FEAT(ch, level_feats[i][4])) {
      if (level_feats[i][2] == TRUE) {
        if (i == FEAT_TWO_WEAPON_FIGHTING && GET_CLASS(ch) == CLASS_RANGER)
 	  //if (!HAS_FEAT(ch, FEAT_RANGER_TWO_WEAPON_STYLE))
          continue;
      }
      if (HAS_FEAT(ch, level_feats[i][4]))
        sprintf(featbuf, "%s@YYou have improved your %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
      else 
        sprintf(featbuf, "%s@YYou have gained the %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
      SET_FEAT(ch, level_feats[i][4], HAS_REAL_FEAT(ch, level_feats[i][4]) + 1);    	
    }
    else if (GET_CLASS(ch) == level_feats[i][0] && level_feats[i][1] == GET_RACE(ch) && GET_CLASS_RANKS(ch, level_feats[i][0]) == level_feats[i][3]) {
      if (HAS_FEAT(ch, level_feats[i][4]))
        sprintf(featbuf, "%s@YYou have improved your %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
      else 
        sprintf(featbuf, "%s@YYou have gained the %s class ability!@n\r\n", featbuf, feat_list[level_feats[i][4]].name);    	
      SET_FEAT(ch, level_feats[i][4], HAS_REAL_FEAT(ch, level_feats[i][4]) + 1);    	
    }

    i++;
  }


  if (whichclass == CLASS_ARTISAN) {
    send_to_char(ch, "%s", featbuf);
//    GET_CLASS_NONEPIC(ch, CLASS_ARTISAN)++;
    return;
  }

 
  if (whichclass == CLASS_BARD) {

    bardLevel = GET_CLASS_RANKS(ch, CLASS_BARD);

    for (i = 0; i < 7; i++) {
      addSpells = MAX(0, bard_spells_known_table[bardLevel][i]) - MAX(0, bard_spells_known_table[bardLevel-1][i]);
      ch->player_specials->bard_spells_to_learn[i] += addSpells;
      if (addSpells > 0)
        send_to_char(ch, "You have gained the ability to learn %d new level %d bard spells.  See help learnbardspells.\r\n", addSpells, i);
    }

  }
  
  level = GET_CLASS_LEVEL(ch) + 1;
  ranks = GET_CLASS_RANKS(ch, whichclass) + 1;
  CREATE(llog, struct levelup_data, 1);
  llog->next = ch->level_info;
  llog->prev = NULL;
  if (llog->next)
    llog->next->prev = llog;
  ch->level_info = llog;
  llog->skills = llog->feats = NULL;
  llog->type = LEVELTYPE_CLASS;
  llog->spec = whichclass;
  llog->level = GET_CLASS_LEVEL(ch);

  if (CAMPAIGN_FORGOTTEN_REALMS ? prestige_classes_core[whichclass] : prestige_classes_dl_aol[whichclass])
    epiclevel = 11;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (ranks) {
  case 1:
    for (i = 0; (j = free_start_feats[whichclass][i]); i++) {
      SET_FEAT(ch, j, 1);
    }
    break;
  default:
  	break;
  }
  
  if (whichclass == CLASS_WIZARD || whichclass == CLASS_KNIGHT_OF_THE_THORN || whichclass == CLASS_WIZARD_OF_HIGH_SORCERY)
  	research_sessions = GET_RESEARCH_TOKENS(ch) += 2;
  
  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_CLASS_LEVEL(ch) >= (LVL_EPICSTART - 1)) { /* Epic character */
    if ((level + 1) % 2) {
      add_acc = 1;
    } else {
      add_fort = 1;
      add_reflex = 1;
      add_will = 1;
    }
  } else if (ranks == 1) { /* First level of a given class */
    add_fort = saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, 1);
    add_reflex = saving_throw_lookup(0, whichclass, SAVING_REFLEX, 1);
    add_will = saving_throw_lookup(0, whichclass, SAVING_WILL, 1);
    add_acc = base_hit(0, whichclass, 1);
  } else { /* Normal level of a non-epic class */
    add_fort = saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, ranks) -
               saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, ranks - 1);
    add_reflex = saving_throw_lookup(0, whichclass, SAVING_REFLEX, ranks) -
                 saving_throw_lookup(0, whichclass, SAVING_REFLEX, ranks - 1);
    add_will = saving_throw_lookup(0, whichclass, SAVING_WILL, ranks) -
               saving_throw_lookup(0, whichclass, SAVING_WILL, ranks - 1);
    add_acc = base_hit(0, whichclass, ranks) - base_hit(0, whichclass, ranks-1);
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (ranks >= (epiclevel)) { /* Epic class */
    if (CAMPAIGN_FORGOTTEN_REALMS ? prestige_classes_core[whichclass] : prestige_classes_dl_aol[whichclass])
      j = ranks - 10;
    else
      j = ranks - 20;

    n = 0;
    while (epic_level_feats[n][0] != CLASS_UNDEFINED) {
      if (whichclass == epic_level_feats[n][0]) {
        if ((((j + epic_level_feats[n][1]) % epic_level_feats[n][2]) == epic_level_feats[n][3]) == epic_level_feats[n][4]) {
          if (epic_level_feats[n][5] == FEAT_DAMAGE_REDUCTION) {
            SET_FEAT(ch, FEAT_DAMAGE_REDUCTION, HAS_REAL_FEAT(ch, FEAT_DAMAGE_REDUCTION) + epic_level_feats[n][6]);
            for (reduct = ch->damreduct; reduct; reduct = reduct->next) {
              if (reduct->feat == FEAT_DAMAGE_REDUCTION) {
                REMOVE_FROM_LIST(reduct, ch->damreduct, next);
              }
            }
            CREATE(ptr, struct damreduct_type, 1);
            ptr->next = ch->damreduct;
            ch->damreduct = ptr;
            ptr->spell = 0;
            ptr->feat = FEAT_DAMAGE_REDUCTION;
            ptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION);
            ptr->duration = -1;
            ptr->max_damage = -1;
            for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
              ptr->damstyle[q] = ptr->damstyleval[q] = 0;
            ptr->damstyle[0] = DR_NONE;
          } else {
            SET_FEAT(ch, epic_level_feats[n][5], HAS_REAL_FEAT(ch, epic_level_feats[n][5]) + epic_level_feats[n][6]);
          }
          sprintf(featbuf, "%s@YYou have improved your %s class ability!@n\r\n", featbuf, feat_list[epic_level_feats[n][5]].name);
        }
      }
      n++;
    }
  }

  if (manual)
	  add_class_feats = num_levelup_class_feats(ch, whichclass, ranks);
  else
	  add_class_feats = ch->levelup->num_class_feats;

  switch (whichclass) {
    case CLASS_ARCANE_ARCHER:
      if (((ranks - 1) % 4) != 0)
        arcane_bonus++;
      break;
    case CLASS_ELDRITCH_KNIGHT:
      if (ranks > 1)
        arcane_bonus++;
      break;
    case CLASS_DEATH_MASTER:
      if (ranks > 1)
        arcane_bonus++;
      break;
    case CLASS_MYSTIC_THEURGE:
        arcane_bonus++;
        divine_bonus++;
      break;
    case CLASS_SACRED_FIST:
      divine_bonus++;
      break;
    case CLASS_ARCANE_TRICKSTER:
      arcane_bonus++;
      break;
    case CLASS_TEMPLAR:
    case CLASS_ZHENTARIM_PRIEST:
      divine_bonus++;
      break;
    case CLASS_ZHENTARIM_WIZARD:
      arcane_bonus++;
      break;
  }

  ch->player_specials->bonus_levels_arcane += arcane_bonus;
  ch->player_specials->bonus_levels_divine += divine_bonus;


  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (whichclass) {
  case CLASS_WIZARD:
  case CLASS_KNIGHT_OF_THE_THORN:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_SORCERER:
  case CLASS_FAVORED_SOUL:
  case CLASS_ELDRITCH_KNIGHT:
  case CLASS_DEATH_MASTER:
    add_mana = 10;
    add_move = rand_number(1, 2);
    break;
  case CLASS_CLERIC:
  case CLASS_KNIGHT_OF_THE_SKULL:
    add_mana = 10;
    add_move = rand_number(1, 2);
    break;
  case CLASS_DRAGON_DISCIPLE:
    add_mana = 5;
    add_move = rand_number(1, 2);
    break;
  case CLASS_ARCANE_TRICKSTER:
    add_mana = 10;
    add_move = rand_number(1, 2);
    break;
  case CLASS_ROGUE:
    add_move = rand_number(1, 3);
    break;
  case CLASS_ARTISAN:
    add_move = rand_number(1, 2);
    break;
  case CLASS_FIGHTER:
  case CLASS_KNIGHT_OF_THE_LILY:
    add_move = rand_number(1, 3);
    break;
  case CLASS_MONK:
    add_move = rand_number(1, 3);
    add_ki = 10 + ability_mod_value(ch->real_abils.wis);
    break;
  case CLASS_ASSASSIN:
    add_move = rand_number(1, 3);
    break;
  case CLASS_PALADIN:
    add_mana = 5;
    add_move = rand_number(1, 3);
    break;
  case CLASS_BARBARIAN:
  	add_move = rand_number(1, 3);
  	break;
  case CLASS_DRUID:
        add_mana = 10;
  	add_move = rand_number(1, 3);
 	break;
  case CLASS_RANGER:
        add_mana = 5;
  	add_move = rand_number(1, 3);
  	break;
  case CLASS_BARD:
        add_mana = 10;
  	add_move = rand_number(1, 3);
  	break;
  case CLASS_DUELIST:
  	add_move = rand_number(1, 3);
  	break;
  case CLASS_KNIGHT_OF_THE_CROWN:
  case CLASS_KNIGHT_OF_THE_ROSE:
  case CLASS_DWARVEN_DEFENDER:
  case CLASS_WEAPON_MASTER:
  case CLASS_KNIGHT_OF_THE_SWORD:  	 
  	add_move = rand_number(1, 3);
  	break;  	  	
  }
  if (HAS_FEAT(ch, FEAT_ENDURANCE))
    add_move++;
  add_move *= 10;
  add_move = MAX(0, add_move);


  if (manual)
	  add_prac = num_levelup_practices(ch, whichclass);
  else
	  add_prac = ch->levelup->practices;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */

  if (level % 2 == 1) {
    if (manual)
      add_gen_feats++;
    else
      add_gen_feats = ch->levelup->feat_points;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  i = ability_mod_value(ch->real_abils.con);
  if (level > 1) {
    j = (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_hit_die_size_dl_aol : class_hit_die_size_fr)[whichclass];
    add_hp = MAX(1, i + j);
  }
  if (HAS_FEAT(ch, FEAT_TOUGHNESS) && (level >3))
    add_hp += 1;
  llog->hp_roll = j;
  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  add_train = (level % 4) ? 0 : 1;
  if (add_train) {
    if (manual)
      GET_TRAINS(ch) += add_train;
    else
      GET_TRAINS(ch) = ch->levelup->num_trains;
  }

  llog->mana_roll = add_mana;
  llog->move_roll = add_move;
  llog->ki_roll = add_ki;
  llog->add_skill = add_prac;
  add_prac = add_prac;

  if (whichclass != CLASS_ARTISAN) {
  GET_PRACTICES(ch, whichclass) += add_prac;
  GET_MAX_HIT(ch) += add_hp;
  GET_MAX_MOVE(ch) += add_move;
  GET_MAX_MANA(ch) += add_mana;
  GET_MAX_KI(ch) += add_ki;
  if (GET_CLASS_LEVEL(ch) > 20) { /* Epic character */
    GET_EPIC_FEAT_POINTS(ch) += add_gen_feats;
    GET_EPIC_CLASS_FEATS(ch, whichclass) += add_class_feats;
  } else {
    GET_FEAT_POINTS(ch) += add_gen_feats;
    GET_CLASS_FEATS(ch, whichclass) += add_class_feats;
  }
}
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }
  add_gen_feats = add_gen_feats;
  add_class_feats = add_class_feats;
  send_to_char(ch, "@YYou have gained a level in %s!\r\n@n", pc_class_types_dl_aol[whichclass]);
  send_to_char(ch, "@Y");
  display_levelup_changes(ch, TRUE);
  send_to_char(ch, "@n");
  send_to_char(ch, "@YYou have gained the following:\r\n@n");
  send_to_char(ch, "@rAny values below zero signify you spent what you gained plus what you had from before.\r\n");
  if (add_hp) send_to_char(ch, "@Y  Hit Points:     @W%d\r\n@n", add_hp);
  if (add_mana) send_to_char(ch, "@Y  Metamagic Points: @W%d\r\n@n", add_mana);
  if (add_move) send_to_char(ch, "@Y  Stamina Points: @W%d\r\n@n", add_move);
  if (add_ki) send_to_char(ch, "@Y  Ki Points: @W%d\r\n@n", add_ki);
  if (add_fort) send_to_char(ch, "@Y  Fortitude Save: @W%d\r\n@n", add_fort);
  if (add_will) send_to_char(ch, "@Y  Willpower Save: @W%d\r\n@n", add_will);
  if (add_reflex) send_to_char(ch, "@Y  Reflex Save:    @W%d\r\n@n", add_reflex);
  if (add_train) send_to_char(ch, "@Y  Ability Trains: @W%d\r\n@n", add_train);
  if (GET_CLASS_LEVEL(ch) > 20) {
    if (add_gen_feats) send_to_char(ch, "@Y  Epic Feats:  @W%d\r\n@n", add_gen_feats);
    if (add_class_feats) send_to_char(ch, "@Y  Epic Class Feats:    @W%d\r\n@n", add_class_feats);
  }
  else {
    if (add_gen_feats) send_to_char(ch, "@Y  General Feats:  @W%d\r\n@n", add_gen_feats);
    if (add_class_feats) send_to_char(ch, "@Y  Class Feats:    @W%d\r\n@n", add_class_feats);
  }
  if (add_acc) send_to_char(ch, "@Y  To-Hit Bonus:   @W%d\r\n@n", add_acc);
  if (add_prac) send_to_char(ch, "@Y  Skill Points:   @W%d\r\n@n", add_prac);
  if (research_sessions) send_to_char(ch, "@Y  Spell Research Sessions: @W%d\r\n", research_sessions);
  if (arcane_bonus) send_to_char(ch, "@Y  You have gained an arcane bonus level to spend.\r\n");
  if (divine_bonus) send_to_char(ch, "@Y  You have gained a divine bonus level to spend.\r\n");
  if (evasion) send_to_char(ch, "@YYou have gained the evasion feat!@n\r\n");
  if (impEvasion) send_to_char(ch, "@YYou have gained the improved evasion feat!@n\r\n");
  if (impDisarm) send_to_char(ch, "@YYou have gained the improved disarm feat!@n\r\n");
  if (combatReflexes) send_to_char(ch, "@YYou have gained the combat reflexes feat!@n\r\n");
  if (layhands) send_to_char(ch, "@YYou have gained the lay hands feat!@n\r\n");
  if (sneakAttack) send_to_char(ch, "@YYour sneak attack has increased to +%dd6!!@n\r\n", HAS_FEAT(ch, FEAT_SNEAK_ATTACK));
  send_to_char(ch, "%s", featbuf);
  if (whichclass == CLASS_MONK) {
    buf[0] = 0;
    j = 0;
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
      if (IS_SET(spell_info[i].skilltype, SKTYPE_ART))
        if (ranks >= spell_info[i].min_level[CLASS_MONK] && !GET_SKILL(ch, i)) {
          if (j)
            strncat(buf, ", ", sizeof(buf)-1);
          strncat(buf, spell_info[i].name, sizeof(buf)-1);
          SET_SKILL(ch, i, 1);
          j += 1;
        }
    if (j) {
      send_to_char(ch, "You gain the following abilit%s, usable via the \"art\" command:%s%s\r\n",
                   j > 1 ? "ies" : "y", j > 1 ? "\r\n" : " ", buf);
    }
  }
  llog->accuracy = add_acc;
  llog->fort = add_fort;
  llog->reflex = add_reflex;
  llog->will = add_will;
  GET_ACCURACY_BASE(ch) += MAX(0, add_acc);
  GET_SAVE_BASE(ch, SAVING_FORTITUDE) += add_fort;
  GET_SAVE_BASE(ch, SAVING_REFLEX) += add_reflex;
  GET_SAVE_BASE(ch, SAVING_WILL) += add_will;
  GET_MAX_HIT(ch) = calculate_max_hit(ch);
  GET_FEAT_POINTS(ch) = 0;
  GET_EPIC_FEAT_POINTS(ch) = 0;
  GET_CLASS_FEATS(ch, whichclass) = 0;
  GET_EPIC_CLASS_FEATS(ch, whichclass) = 0;
  GET_PRACTICES(ch, whichclass) = 0;
  GET_TRAINS(ch) = 0;

  if (!manual)
    GET_CLASS_LEVEL(ch) += 1;

  for (i = 0; i < MAX_NUM_KNOWN_SPELLS; i++) {
    if (ch->levelup->spells_known[i] != ch->player_specials->spells_known[i]) {
      send_to_char(ch, "@YYou have added the %s spell to your repetoire.\r\n@n", spell_info[ch->levelup->spells_known[i]].name);
    }
    ch->player_specials->spells_known[i] = ch->levelup->spells_known[i];
  }
    

  ch->levelup = NULL;

  snoop_check(ch);
  save_char(ch);
}

/*
 * How many +1d6 dice does the character get for sneak attacks?
 */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int backstab_dice(struct char_data *ch)
{
  int dice;
  if (!IS_NPC(ch)) {
    dice = HAS_FEAT(ch, FEAT_SNEAK_ATTACK);
    if (dice < 0)
      return 0;
    return dice;
  } else {
    if (GET_CLASS(ch) == CLASS_ROGUE)
      return (GET_LEVEL(ch) + 1) / 2;
    else
      return 0;
  }
}
/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_WIZARD(ch))
    return true;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return true;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_FIGHTER) && IS_FIGHTER(ch))
    return true;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return true;
  return false;
}
/*
 * SPELLS AND SKILLS.  this area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_skill_race_classes(void)
{
  return;
}
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
void init_skill_classes(void)
{
  int i = 0, j = 0;

  // This initializes all skills
  for (i = 0; i < NUM_CLASSES; i++) {
    for (j = 0; j < SKILL_TABLE_SIZE; j++)
      skill_class(j, i, SKLEARN_CROSSCLASS);	
  }  
  
  // These skills belong to all classes unless changed elsewhere below
  for (i = 0; i < NUM_CLASSES; i++) 
  {
    skill_class(SKILL_BLACKSMITHING, i, SKLEARN_CLASS);
    skill_class(SKILL_COMBAT_TACTICS, i, SKLEARN_CLASS);
    skill_class(SKILL_COOKING, i, SKLEARN_CLASS);
    skill_class(SKILL_CRAFTING_THEORY, i, SKLEARN_CLASS);
    skill_class(SKILL_FARMING, i, SKLEARN_CLASS);
    skill_class(SKILL_FORESTING, i, SKLEARN_CLASS);
    skill_class(SKILL_GOLDSMITHING, i, SKLEARN_CLASS);
    skill_class(SKILL_HERBALISM, i, SKLEARN_CLASS);
    skill_class(SKILL_HUNTING, i, SKLEARN_CLASS);
    skill_class(SKILL_KNOWLEDGE, i, SKLEARN_CLASS);
    skill_class(SKILL_MINING, i, SKLEARN_CLASS);
    skill_class(SKILL_TAILORING, i, SKLEARN_CLASS);
    skill_class(SKILL_TANNING, i, SKLEARN_CLASS);
    skill_class(SKILL_WOODWORKING, i, SKLEARN_CLASS);

  }

  skill_class(SKILL_PERCEPTION, CLASS_ARCANE_ARCHER, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_ARCANE_ARCHER, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_ARCANE_ARCHER, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_ARCANE_ARCHER, SKLEARN_CLASS);


  /* DEATH MASTER */
  skill_class(SKILL_DIPLOMACY, CLASS_DEATH_MASTER, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_DEATH_MASTER, SKLEARN_CLASS);
  skill_class(SKILL_KNOWLEDGE, CLASS_DEATH_MASTER, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_DEATH_MASTER, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_DEATH_MASTER, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_DEATH_MASTER, SKLEARN_CLASS);

  /* ELDRITCH KNIGHT */
  skill_class(SKILL_LINGUISTICS, CLASS_ELDRITCH_KNIGHT, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_ELDRITCH_KNIGHT, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_ELDRITCH_KNIGHT, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_ELDRITCH_KNIGHT, SKLEARN_CLASS);

  /* FAVORED SOULS */
  skill_class(SKILL_DIPLOMACY, CLASS_FAVORED_SOUL, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_FAVORED_SOUL, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_FAVORED_SOUL, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_FAVORED_SOUL, SKLEARN_CLASS);

  /* ARTISANS */
  skill_class(SKILL_CRAFTING_THEORY, CLASS_ARTISAN, SKLEARN_CLASS);

  /* MAGES/WIZARDS */
  skill_class(SKILL_APPRAISE, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_WIZARD, SKLEARN_CANT);
  skill_class(SKILL_SPELLCRAFT, CLASS_WIZARD, SKLEARN_CLASS);
  // SORCERER
  skill_class(SKILL_APPRAISE, CLASS_SORCERER, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_SORCERER, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_SORCERER, SKLEARN_CLASS);
  /* CLERICS */
  skill_class(SKILL_APPRAISE, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_CLERIC, SKLEARN_CANT);
  skill_class(SKILL_SPELLCRAFT, CLASS_CLERIC, SKLEARN_CLASS);
  /* THIEVES */
  skill_class(SKILL_ACROBATICS, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_APPRAISE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DISABLE_DEVICE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ROGUE, SKLEARN_CLASS);
  /* FIGHTERS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_FIGHTER, SKLEARN_CLASS);
  // dragon riders
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_DRAGON_RIDER, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_DRAGON_RIDER, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_DRAGON_RIDER, SKLEARN_CANT);
  skill_class(SKILL_RIDE, CLASS_DRAGON_RIDER, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_DRAGON_RIDER, SKLEARN_CANT);
  /* MONKS */
  skill_class(SKILL_ACROBATICS, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_MONK, SKLEARN_CLASS);
  // Sacred Fists
  skill_class(SKILL_ACROBATICS, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_SACRED_FIST, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_SACRED_FIST, SKLEARN_CLASS);
  /* PALADINS */
  skill_class(SKILL_DIPLOMACY, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_PALADIN, SKLEARN_CLASS);
  /* BARBARIANS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_BARBARIAN, SKLEARN_CLASS);    
  skill_class(SKILL_INTIMIDATE, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_BARBARIAN, SKLEARN_CLASS);            
  /* RANGERS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_RANGER, SKLEARN_CLASS);            
  skill_class(SKILL_HEAL, CLASS_RANGER, SKLEARN_CLASS);            
  skill_class(SKILL_PERCEPTION, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_RANGER, SKLEARN_CLASS);            
  skill_class(SKILL_SURVIVAL, CLASS_RANGER, SKLEARN_CLASS);            
  // Druids
  skill_class(SKILL_DIPLOMACY, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_HEAL, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_PERCEPTION, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_RIDE, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_SPELLCRAFT, CLASS_DRUID, SKLEARN_CLASS);            
  skill_class(SKILL_SURVIVAL, CLASS_DRUID, SKLEARN_CLASS);            
  // Bards
  skill_class(SKILL_ACROBATICS, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_APPRAISE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_BARD, SKLEARN_CLASS);

  // Dragon Disciples
  skill_class(SKILL_DIPLOMACY, CLASS_DRAGON_DISCIPLE, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_DRAGON_DISCIPLE, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_DRAGON_DISCIPLE, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_DRAGON_DISCIPLE, SKLEARN_CLASS);

 // Assassins
  skill_class(SKILL_ACROBATICS, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_DISABLE_DEVICE, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_ASSASSIN, SKLEARN_CLASS);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ASSASSIN, SKLEARN_CLASS);

  // Arcane Trickster
  skill_class(SKILL_ACROBATICS, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_DISABLE_DEVICE, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);
  skill_class(SKILL_STEALTH, CLASS_ARCANE_TRICKSTER, SKLEARN_CLASS);


#if defined(CAMPAIGN_FORGOTTEN_REALMS)  

  /* CROWN KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_PURPLE_DRAGON_KNIGHT, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_PURPLE_DRAGON_KNIGHT, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_PURPLE_DRAGON_KNIGHT, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_PURPLE_DRAGON_KNIGHT, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_PURPLE_DRAGON_KNIGHT, SKLEARN_CLASS);

  /* SWORD KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_PURPLE_DRAGON_TEMPLAR, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_PURPLE_DRAGON_TEMPLAR, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_PURPLE_DRAGON_TEMPLAR, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_PURPLE_DRAGON_TEMPLAR, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_PURPLE_DRAGON_TEMPLAR, SKLEARN_CLASS);

  /* ROSE KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_PURPLE_DRAGON_LORD, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_PURPLE_DRAGON_LORD, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_PURPLE_DRAGON_LORD, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_PURPLE_DRAGON_LORD, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_PURPLE_DRAGON_LORD, SKLEARN_CLASS);

  /* DUELISTS */
  skill_class(SKILL_LINGUISTICS, CLASS_DUELIST, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_DUELIST, SKLEARN_CANT);  
  skill_class(SKILL_ACROBATICS, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_DUELIST, SKLEARN_CLASS);
  // LILY_KNIGHTS
  skill_class(SKILL_LINGUISTICS, CLASS_ZHENTARIM_KNIGHT, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ZHENTARIM_KNIGHT, SKLEARN_CANT);    
  skill_class(SKILL_DIPLOMACY, CLASS_ZHENTARIM_KNIGHT, SKLEARN_CLASS);   
  skill_class(SKILL_INTIMIDATE, CLASS_ZHENTARIM_KNIGHT, SKLEARN_CLASS);   
  skill_class(SKILL_RIDE, CLASS_ZHENTARIM_KNIGHT, SKLEARN_CLASS);     
  //SKULL_KNIGHTS
  skill_class(SKILL_LINGUISTICS, CLASS_ZHENTARIM_PRIEST, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ZHENTARIM_PRIEST, SKLEARN_CANT);    
  skill_class(SKILL_SPELLCRAFT, CLASS_ZHENTARIM_PRIEST, SKLEARN_CLASS);     
  skill_class(SKILL_DIPLOMACY, CLASS_ZHENTARIM_PRIEST, SKLEARN_CLASS); 
  skill_class(SKILL_INTIMIDATE, CLASS_ZHENTARIM_PRIEST, SKLEARN_CLASS); 
  skill_class(SKILL_RIDE, CLASS_ZHENTARIM_PRIEST, SKLEARN_CLASS);       
  //THORN_KNIGHTS               
  skill_class(SKILL_LINGUISTICS, CLASS_ZHENTARIM_WIZARD, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ZHENTARIM_WIZARD, SKLEARN_CANT);    
  skill_class(SKILL_SPELLCRAFT, CLASS_ZHENTARIM_WIZARD, SKLEARN_CLASS); 
  skill_class(SKILL_DIPLOMACY, CLASS_ZHENTARIM_WIZARD, SKLEARN_CLASS); 
  skill_class(SKILL_INTIMIDATE, CLASS_ZHENTARIM_WIZARD, SKLEARN_CLASS); 
  skill_class(SKILL_RIDE, CLASS_ZHENTARIM_WIZARD, SKLEARN_CLASS);         
  skill_class(SKILL_SPELLCRAFT, CLASS_ZHENTARIM_WIZARD, SKLEARN_CLASS);   
#else
  /* CROWN KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_CROWN, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_CROWN, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_CROWN, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_CROWN, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_CROWN, SKLEARN_CLASS);
  /* SWORD KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_SWORD, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_SWORD, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_SWORD, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_SWORD, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_SWORD, SKLEARN_CLASS);
  /* ROSE KNIGHTS */
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_ROSE, SKLEARN_CLASS);
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_ROSE, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_ROSE, SKLEARN_CANT);
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_ROSE, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_ROSE, SKLEARN_CLASS);
  /* DUELISTS */
  skill_class(SKILL_LINGUISTICS, CLASS_DUELIST, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_DUELIST, SKLEARN_CANT);  
  skill_class(SKILL_ACROBATICS, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_PERCEPTION, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_DUELIST, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_DUELIST, SKLEARN_CLASS);
  // LILY_KNIGHTS
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_LILY, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_LILY, SKLEARN_CANT);    
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_LILY, SKLEARN_CLASS);   
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_LILY, SKLEARN_CLASS);   
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_LILY, SKLEARN_CLASS);     
  //SKULL_KNIGHTS
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_SKULL, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_SKULL, SKLEARN_CANT);    
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_SKULL, SKLEARN_CLASS); 
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_SKULL, SKLEARN_CLASS); 
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_SKULL, SKLEARN_CLASS);       
  //THORN_KNIGHTS               
  skill_class(SKILL_LINGUISTICS, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CANT);    
  skill_class(SKILL_DIPLOMACY, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CLASS); 
  skill_class(SKILL_INTIMIDATE, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CLASS); 
  skill_class(SKILL_RIDE, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CLASS);         
  skill_class(SKILL_SPELLCRAFT, CLASS_KNIGHT_OF_THE_THORN, SKLEARN_CLASS);   
#endif  

  skill_class(SKILL_SENSE_MOTIVE, CLASS_MYSTIC_THEURGE, SKLEARN_CLASS);   
  skill_class(SKILL_SPELLCRAFT, CLASS_MYSTIC_THEURGE, SKLEARN_CLASS);   

  skill_class(SKILL_PERCEPTION, CLASS_DWARVEN_DEFENDER, SKLEARN_CLASS);   
  skill_class(SKILL_SENSE_MOTIVE, CLASS_DWARVEN_DEFENDER, SKLEARN_CLASS);   

  skill_class(SKILL_LINGUISTICS, CLASS_WEAPON_MASTER, SKLEARN_CANT);
  skill_class(SKILL_SURVIVAL, CLASS_WEAPON_MASTER, SKLEARN_CANT);
  skill_class(SKILL_RIDE, CLASS_WEAPON_MASTER, SKLEARN_CLASS);

}
/* Function to return the exp required for each class/level */
int level_exp(int level, int race)
{
  int amount = 0;
  level += race_list[race].level_adjustment;

  if (level == 0)
    return 0;

  amount = level_exp(level - 1 - race_list[race].level_adjustment, race) + mob_exp_by_level(level - 1) * (100 + (MAX(0, (level - 2)) * 25));
 
  return amount;
}
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
sbyte ability_mod_value(int abil)
{
  return ((int)(abil / 2)) - 5;
}
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
sbyte dex_mod_capped(const struct char_data *ch)
{
  sbyte mod;
  struct obj_data *armor;
  mod = ability_mod_value(GET_DEX(ch));
  armor = GET_EQ(ch, WEAR_BODY);
  if (armor && GET_OBJ_TYPE(armor) == ITEM_ARMOR) {
    mod = MIN(mod, GET_OBJ_VAL(armor, VAL_ARMOR_MAXDEXMOD));
  }
  if (IS_MEDIUM_LOAD((struct char_data *)ch))
    mod = MIN(mod, 3);
  if (IS_HEAVY_LOAD((struct char_data *)ch))
    mod = MIN(mod, 1);
  if (IS_OVER_LOAD((struct char_data *)ch))
    mod = MIN(mod, 0);
  return mod;
}
int cabbr_ranktable[NUM_CLASSES];
int comp_rank(const void *a, const void *b)
{
  int first, second;
  first = *(int *)a;
  second = *(int *)b;
  return cabbr_ranktable[second] - cabbr_ranktable[first];
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
char *class_desc_str(struct char_data *ch, int howlong, int wantthe)
{
    static char str[MAX_STRING_LENGTH]={'\0'};
    char *ptr = str;
    int i = 0, rank = 0, j = 0;
    int rankorder[NUM_CLASSES];
    char *buf, *buf2, *buf3;

    if (wantthe)
        ptr += sprintf(str, "the ");

    if (howlong) 
    {
        buf2 = buf = buf3 = "";
        if (howlong == 2) 
        {
            buf3 = " ";
            if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART)
                ptr += sprintf(ptr, "Epic ");
        }
        for (i = 0; i < NUM_CLASSES; i++) 
        {
            cabbr_ranktable[i] = GET_CLASS_RANKS(ch, i);
            rankorder[i] = i;
        }
rankorder[0] = GET_CLASS(ch); /* we always want primary class first */
        rankorder[GET_CLASS(ch)] = 0;
        qsort((void *)rankorder, NUM_CLASSES, sizeof(int), comp_rank);
        for (i = 0; i < NUM_CLASSES; i++) 
        {
            rank = rankorder[i];
            if (cabbr_ranktable[rank] == 0)
                continue;
            ptr += snprintf(ptr, sizeof(str) - (ptr - str), "%s%s%s%s%s%d", buf, buf2, buf,
                CONFIG_CAMPAIGN == CAMPAIGN_GOLARION ? 
                (howlong == 2 ? (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core) : class_abbrevs_core)[rank] : 
                (howlong == 2 ? (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core) : class_abbrevs_dl_aol)[rank], 
                buf3, cabbr_ranktable[rank]);
            buf2 = "/";
            if (howlong == 2)
                buf = " ";
        }
        return str;
    } 
    else 
    {
        rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
        j = GET_CLASS(ch);
        for (i = 0; i < NUM_CLASSES; i++)
            if (GET_CLASS_RANKS(ch, i) > rank) 
            {
                j = i;
                rank = GET_CLASS_RANKS(ch, j);
            }
            rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
            snprintf(ptr, sizeof(str) - (ptr - str), "%s%d%s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[GET_CLASS(ch)],
                rank, GET_LEVEL(ch) == rank ? "" : "+");
            return str;
    }
}

int total_skill_levels(struct char_data *ch, int skill)
{
  int i = 0, j, total = 0;
  for (i = 0; i < NUM_CLASSES; i++) {
    j = 1 + GET_CLASS_RANKS(ch, i) - spell_info[skill].min_level[i];
    if (j > 0)
     total += j;
  }
  return total;
}
int load_levels()
{
  FILE *fp;
  char line[READ_SIZE]={'\0'}, sect_name[READ_SIZE] = { '\0' }, *ptr;
  int  linenum = 0, tp, cls, sect_type = -1;
  if (!(fp = fopen(LEVEL_CONFIG, "r"))) 
  {
    log("SYSERR: Could not open level configuration file, error: %s!", 
         strerror(errno));
    return -1;
  }
  for (tp = 0; tp < NUM_CLASSES; tp++) {
    for (cls = 0; cls <= SAVING_WILL; cls++) {
      save_classes[cls][tp] = 0;
    }
    basehit_classes[tp] = 0;
  }
  for (;;) {
    linenum++;
    if (!fgets(line, READ_SIZE, fp)) {  /* eof check */
      log("SYSERR: Unexpected EOF in file %s.", LEVEL_CONFIG);
      return -1;
    } else if (*line == '$') { /* end of file */
      break;
    } else if (*line == '*') { /* comment line */
      continue;
    } else if (*line == '#') { /* start of a section */
      if ((tp = sscanf(line, "#%s", sect_name)) != 1) {
        log("SYSERR: Format error in file %s, line number %d - text: %s.", 
             LEVEL_CONFIG, linenum, line);
        return -1;
      } else if ((sect_type = search_block(sect_name, config_sect, false)) == -1) {
          log("SYSERR: Invalid section in file %s, line number %d: %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
      }
    } else {
      if (sect_type == CONFIG_LEVEL_VERSION) {
        if (!strncmp(line, "Suntzu", 6)) {
          log("SYSERR: Suntzu %s config files are not compatible with rasputin", LEVEL_CONFIG);
          return -1;
        } else
          strcpy(level_version, line); /* OK - both are READ_SIZE] */
      } else if (sect_type == CONFIG_LEVEL_VERNUM) {
	level_vernum = atoi(line);
      } else if (sect_type == CONFIG_LEVEL_EXPERIENCE) {
        tp = atoi(line);
        exp_multiplier = tp;
      } else if ((sect_type >= CONFIG_LEVEL_FORTITUDE && sect_type <= CONFIG_LEVEL_WILL) ||
                 sect_type == CONFIG_LEVEL_BASEHIT) {
        for (ptr = line; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (!ptr || !*ptr || !isdigit(*ptr)) {
          log("SYSERR: Cannot find class number in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        cls = atoi(ptr);
        for (; ptr && *ptr && isdigit(*ptr); ptr++);
        for (; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (ptr && *ptr && !isdigit(*ptr)) {
          log("SYSERR: Non-numeric entry in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (ptr && *ptr) /* There's a value */
          tp = atoi(ptr);
        else {
          log("SYSERR: Need 1 value in %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (cls < 0 || cls >= NUM_CLASSES) {
          log("SYSERR: Invalid class number %d in file %s, line number %d.", 
              cls, LEVEL_CONFIG, linenum);
          return -1;
        } else {
          if (sect_type == CONFIG_LEVEL_BASEHIT)
            basehit_classes[cls] = tp;
          else
            save_classes[SAVING_FORTITUDE + sect_type - CONFIG_LEVEL_FORTITUDE][cls] = tp;
        }
      } else {
        log("Unsupported level config option");
      }
    }
  }
  fclose(fp);
  for (cls = 0; cls < NUM_CLASSES; cls++)
    log("Base hit for class %s: %s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[cls], basehit_type_names[basehit_classes[cls]]);
  for (cls = 0; cls < NUM_CLASSES; cls++)
    log("Saves for class %s: fort=%s, reflex=%s, will=%s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[cls],
        save_type_names[save_classes[SAVING_FORTITUDE][cls]],
        save_type_names[save_classes[SAVING_REFLEX][cls]],
        save_type_names[save_classes[SAVING_WILL][cls]]);
  return 0;
}
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int highest_skill_value(int level, int type)
{
  // Pathfinder rules
  return level;

  switch (type) {
    case SKLEARN_CROSSCLASS:
      return (level + 3) / 2;
    case SKLEARN_CLASS:
      return level + 3;
    case SKLEARN_BOOL:
      return 1;
    case SKLEARN_CANT:
    default:
      return 0;
  }
}
/* This returns the number of classes that should be penalized. */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int exp_penalty(struct char_data *ch)
{
  int high, highclass, i, fcl, pen;
  fcl = race_list[GET_RACE(ch)].favored_class[GET_SEX(ch)]; /* This class is skipped for penalties */
  if (fcl == -1) { /* -1 means find highest and use it as 'favored' class */
    for (high = fcl = i = 0; i < NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i) > high) {
        fcl = i;
        high = GET_CLASS_RANKS(ch, i);
      }
    }
  }
  for (high = highclass = i = 0; i < NUM_CLASSES; i++) {
    /* Favored class and prestige classes don't count */
    if (i == fcl || (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? prestige_classes_dl_aol : prestige_classes_core)[i])
      continue;
    if (GET_CLASS_RANKS(ch, i) > high) {
      highclass = i;
      high = GET_CLASS_RANKS(ch, i);
    }
  }
  /*
   * OK, we know their favored class and highest class, now we can figure
   * out the penalty
   */
  pen = 0;
  for (i = 0; i < NUM_CLASSES; i++) {
    if (i == fcl || (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? prestige_classes_dl_aol : prestige_classes_core)[i] || !GET_CLASS_RANKS(ch, i))
      continue;
    if (GET_CLASS_RANKS(ch, i) < (high - 1))
      pen++;
  }
  if (GET_RACE(ch) == RACE_HUMAN) // || GET_RACE(ch) == RACE_HALF_ELF)
  	pen--;
  
  return pen;
}
/*
 * -20% for each class 2 or more less than highest class, ignoring favored
 * class and any prestige classes
 */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int calc_penalty_exp(struct char_data *ch, int gain)
{
  return gain;

  int pen;
  int a = 1, b = 1;
  for (pen = exp_penalty(ch); pen > 0; pen--) {
    a *= 4;
    b *= 5;
  }
  gain *= a;
  gain /= b;
  return gain;
}
int size_scaling_table[NUM_SIZES][4] = {
/*                   str       dex     con  nat arm */
/* Fine		*/ { -10,	-2,	-2,	0 },
/* Diminutive	*/ { -10,	-2,	-2,	0 },
/* Tiny		*/ { -8,	-2,	-2,	0 },
/* Small	*/ { -4,	-2,	-2,	0 },
/* Medium	*/ { 0,		0,	0,	0 },
/* Large	*/ { 8,		-2,	4,	2 },
/* Huge		*/ { 16,	-4,	8,	5 },
/* Gargantuan	*/ { 24,	-4,	12,	9 },
/* Colossal	*/ { 32,	-4,	16,	14 }
};
/*
 * Assign stats to a mob based on race/class/size/level
 */
/* Original algorithm */
void assign_auto_stats(struct char_data *ch)
{
  struct abil_data *ab;
  int sz, extra, tmp;
  if (!ch)
    return;
  ab = &ch->real_abils;
 
  ab->intel = ab->wis = ab->str = ab->con = ab->dex = ab->cha = 10;
  racial_ability_modifiers(ch);
  sz = get_size(ch);
  
  ab->str += size_scaling_table[sz][0];
  ab->dex += size_scaling_table[sz][1];
  ab->con += size_scaling_table[sz][2];
  extra = GET_LEVEL(ch) / 3; /* For PC's it's every 4, we want to make it compensate */
  switch (GET_CLASS(ch)) {
  case CLASS_WIZARD:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->intel += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->dex > ab->str)
      ab->dex += extra;
    else
      ab->str += extra;
    break;
  case CLASS_CLERIC:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->wis += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    ab->cha += extra;
    break;
  case CLASS_ROGUE:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->dex += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->intel += tmp;
    if (ab->con > ab->str)
      ab->con += extra;
    else
      ab->str += extra;
    break;
  case CLASS_MONK:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->wis += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->str > ab->dex)
      ab->str += extra;
    else
      ab->dex += extra;
    break;
  case CLASS_PALADIN:
    tmp = extra / 2;
    extra -= tmp;
    if (ab->str > ab->dex)
      ab->str += tmp;
    else
      ab->dex += tmp;
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->wis > ab->cha)
      ab->wis += extra;
    else
      ab->cha += extra;
    break;
  case CLASS_FIGHTER:
  default:
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->dex > ab->str)
      ab->dex += extra;
    else
      ab->str += extra;
    break;
  }
  ch->aff_abils = ch->real_abils;
  return;
}
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
time_t birth_age(struct char_data *ch)
{
  int cltype;
  int race;
  int tmp;
  switch (GET_CLASS(ch)) {
  case CLASS_WIZARD:
  case CLASS_CLERIC:
  case CLASS_MONK:
    cltype = 2;
    break;
  case CLASS_FIGHTER:
  case CLASS_PALADIN:
    cltype = 1;
    break;
  case CLASS_ROGUE:
  default:
    cltype = 0;
    break;
  }

  race = GET_RACE(ch) < NUM_RACES ? GET_RACE(ch) : RACE_HUMAN;
  tmp = racial_aging_data[race].adult + dice(racial_aging_data[race].classdice[cltype][0], racial_aging_data[race].classdice[cltype][1]);
  tmp *= SECS_PER_MUD_YEAR;
  tmp += rand_number(0, SECS_PER_MUD_YEAR);
  return tmp;
}
time_t max_age(struct char_data *ch)
{
  int race;
  size_t tmp;
  if (ch->time.maxage)
    return ch->time.maxage - ch->time.birth;

  race = GET_RACE(ch) < NUM_RACES ? GET_RACE(ch) : RACE_HUMAN;
  tmp = racial_aging_data[race].venerable + dice(racial_aging_data[race].maxdice[0], racial_aging_data[race].maxdice[1]);
  tmp *= SECS_PER_MUD_YEAR;
  tmp += rand_number(0, SECS_PER_MUD_YEAR);
  return tmp;
}
/*
 * Returns:
 * 0 on failure
 * 1 on raising normal class level
 * 2 on raising epic class level
 * 3 on raising both
 */
int raise_class_only(struct char_data *ch, int cl, int v)
{
  if (cl < 0 || cl >= NUM_CLASSES)
    return 0;
  if (GET_CLASS_LEVEL(ch) + v < LVL_EPICSTART) {
    GET_CLASS_NONEPIC(ch, cl) += v;
    return 1;
  }
  if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART - 1) {
    GET_CLASS_EPIC(ch, cl) += v;
    return 2;
  }
  GET_CLASS_NONEPIC(ch, cl) += (v -= (LVL_EPICSTART - (1 + GET_CLASS_LEVEL(ch))));
  GET_CLASS_EPIC(ch, cl) += v;
  return 3;
}

const int class_feats_wizard[] = 
{
  FEAT_SPELL_MASTERY,
  FEAT_BREW_POTION,
  FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR,
  FEAT_CRAFT_ROD,
  FEAT_CRAFT_STAFF,
  FEAT_CRAFT_WAND,
  FEAT_CRAFT_WONDEROUS_ITEM,
  FEAT_FORGE_RING,
  FEAT_SCRIBE_SCROLL,
  FEAT_EMPOWER_SPELL,
  FEAT_ENLARGE_SPELL,
  FEAT_EXTEND_SPELL,
  FEAT_HEIGHTEN_SPELL,
  FEAT_MAXIMIZE_SPELL,
  FEAT_INTENSIFY_SPELL,
  FEAT_QUICKEN_SPELL,
  FEAT_SILENT_SPELL,
  FEAT_STILL_SPELL,
  FEAT_WIDEN_SPELL,
  FEAT_EMPOWERED_MAGIC,
  FEAT_FASTER_MEMORIZATION,
  FEAT_ENHANCED_SPELL_DAMAGE,
  FEAT_ENHANCE_SPELL,
  FEAT_EPIC_SPELLCASTING,
  FEAT_UNDEFINED
};
/*
 * Rogues follow opposite logic - they can take any feat in place of these,
 * all of these are abilities that are not normally able to be taken as
 * feats. Most classes can ONLY take from these lists for their class
 * feats.
 */
const int class_feats_rogue[] = {
  FEAT_SELF_CONCEALMENT,
  FEAT_SNEAK_ATTACK_OF_OPPORTUNITY,
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_DODGE,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_WEAPON_PROFICIENCY_EXOTIC,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_GREATER_WEAPON_SPECIALIZATION,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_BASH,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SNATCH_ARROWS,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_SPECIALIZATION,
  FEAT_WHIRLWIND_ATTACK,
  FEAT_IMPROVED_EVASION,
  FEAT_CRIPPLING_STRIKE,
  FEAT_DEFENSIVE_ROLL,
  FEAT_IMPROVED_EVASION,
  FEAT_OPPORTUNIST,
  FEAT_SKILL_MASTERY,
  FEAT_SLIPPERY_MIND,
  FEAT_BLEEDING_ATTACK,
  FEAT_POWERFUL_SNEAK,
  FEAT_SNEAK_ATTACK,
  FEAT_IMPROVED_SNEAK_ATTACK,
  FEAT_SNEAK_ATTACK_OF_OPPORTUNITY,
  FEAT_UNDEFINED
};
const int class_feats_fighter[] = {
  FEAT_EPIC_PROWESS,
  FEAT_SWARM_OF_ARROWS,
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_DODGE,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_WEAPON_PROFICIENCY_EXOTIC,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_GREATER_WEAPON_SPECIALIZATION,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_BASH,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SNATCH_ARROWS,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_SPECIALIZATION,
  FEAT_WHIRLWIND_ATTACK,
  FEAT_DAMAGE_REDUCTION,
  FEAT_FAST_HEALING,
  FEAT_ARMOR_SKIN,
  FEAT_ARMOR_SPECIALIZATION_LIGHT,
  FEAT_ARMOR_SPECIALIZATION_MEDIUM,
  FEAT_ARMOR_SPECIALIZATION_HEAVY,
  FEAT_WEAPON_MASTERY,
  FEAT_WEAPON_FLURRY,
  FEAT_WEAPON_SUPREMACY,
  FEAT_ROBILARS_GAMBIT,
  FEAT_KNOCKDOWN,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};

const int class_feats_arcane_archer[] = {
  FEAT_EPIC_PROWESS,
  FEAT_EPIC_TOUGHNESS,
  FEAT_SWARM_OF_ARROWS,  
  FEAT_UNDEFINED,
};

const int class_feats_arcane_trickster[] = {
  FEAT_SNEAK_ATTACK,
  FEAT_SNEAK_ATTACK_OF_OPPORTUNITY,
  FEAT_SELF_CONCEALMENT,
  FEAT_UNDEFINED
};

const int class_feats_eldritch_knight[] = {
  FEAT_EPIC_PROWESS,
  FEAT_SWARM_OF_ARROWS,
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_DODGE,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_WEAPON_PROFICIENCY_EXOTIC,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_GREATER_WEAPON_SPECIALIZATION,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_BASH,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SNATCH_ARROWS,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_SPECIALIZATION,
  FEAT_WHIRLWIND_ATTACK,
  FEAT_DAMAGE_REDUCTION,
  FEAT_ARMOR_SKIN,
  FEAT_EPIC_SPELLCASTING,
  FEAT_PERFECT_TWO_WEAPON_FIGHTING,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};

const int class_feats_favored_soul[] = {
  FEAT_ARMOR_SKIN,
  FEAT_EPIC_SPELLCASTING,
  FEAT_UNDEFINED
};
const int class_feats_assassin[] = {
  FEAT_SNEAK_ATTACK,
  FEAT_SNEAK_ATTACK_OF_OPPORTUNITY,
  FEAT_UNDEFINED
};
const int class_feats_paladin[] = {
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_PROWESS,
  FEAT_ARMOR_SKIN,
  FEAT_GREAT_SMITING,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_monk[] = {
  FEAT_STUNNING_FIST,
  FEAT_EPIC_PROWESS,
  FEAT_SELF_CONCEALMENT,
  FEAT_IMPROVED_TRIP,
  FEAT_DEFLECT_ARROWS,
  FEAT_COMBAT_REFLEXES,
  FEAT_FAST_HEALING,
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_druid[] = {
  FEAT_FAST_HEALING,
  FEAT_UNDEFINED
};
const int class_feats_barbarian[] = {
  FEAT_FAST_HEALING,
  FEAT_EPIC_PROWESS,
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_ranger[] = {
  FEAT_FAST_HEALING,
  FEAT_EPIC_PROWESS,
  FEAT_SWARM_OF_ARROWS,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_defender[] = {
  FEAT_FAST_HEALING,
  FEAT_EPIC_PROWESS,
  FEAT_DAMAGE_REDUCTION,
  FEAT_ARMOR_SKIN,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_champion[] = {
  FEAT_DAMAGE_REDUCTION,
  FEAT_ARMOR_SKIN,
  FEAT_EPIC_PROWESS,
  FEAT_GREAT_SMITING,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_dragon_disciple[] = {
  FEAT_ARMOR_SKIN,
  FEAT_EPIC_PROWESS,
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_TOUGHNESS,
  FEAT_UNDEFINED
};
const int class_feats_sorcerer[] = 
{
  FEAT_BLOODLINE_ARCANE,
  FEAT_BLOODLINE_ABYSSAL,
  FEAT_BLOODLINE_FEY,
  FEAT_UNDEFINED
};


const int no_class_feats[] = {
  FEAT_UNDEFINED
};
const int *class_bonus_feats[NUM_CLASSES] = {
/* WIZARD		*/ class_feats_wizard,
/* CLERIC		*/ no_class_feats,
/* ROGUE		*/ class_feats_rogue,
/* FIGHTER		*/ class_feats_fighter,
/* MONK			*/ class_feats_monk,
/* PALADIN		*/ class_feats_paladin,
/* BARBARIAN		*/ class_feats_barbarian,
/* BARD   		*/ no_class_feats,
/* RANGER		*/ class_feats_ranger,
/* DRUID         	*/ class_feats_druid,
/* EXPANSION 1 */ no_class_feats,
/* EXPANSION 2 */ no_class_feats,
/* EXPANSION 3 */ no_class_feats,
/* EXPANSION 4 */ no_class_feats,
/* EXPANSION 5 */ no_class_feats,
/* EXPANSION 6 */ no_class_feats,
/* EXPANSION 7 */ no_class_feats,
/* DUELIST		*/ no_class_feats,
/* EXPANSION 8 */ no_class_feats,
/* MYSTIC THEURGE	*/ no_class_feats,
/* SORCERER		*/ class_feats_sorcerer,
/* CLASSLESS		*/ no_class_feats,
/* EXPANSION 9 */ no_class_feats,
/* EXPANSION 10 */ no_class_feats,
/* DRAGON DISCIPLE	*/ class_feats_dragon_disciple,
/* ARCANE ARCHER	*/ class_feats_arcane_archer,
/* EXPANSION 11 */ no_class_feats,
/* NPC		        */ no_class_feats,
/* ASSASSIN             */ class_feats_assassin,
/* EXPANSION 12         */ no_class_feats,
/* ARTISAN              */ no_class_feats,
/* ARCANE TRICKSTER     */ class_feats_arcane_trickster,
/* EXPANSION 13         */ no_class_feats,
/* EXPANSION 14         */ no_class_feats,
/* EXPANSION 15         */ no_class_feats,
/* EXPANSION 16         */ no_class_feats,
/* EXPANSION 17         */ no_class_feats,
/* EXPANSION 18         */ no_class_feats,
/* EXPANSION 19         */ no_class_feats,
/* EXPANSION 20         */ no_class_feats,
/* EXPANSION 21         */ no_class_feats,
/* EXPANSION 22         */ no_class_feats,
/* EXPANSION 23         */ no_class_feats,
/* EXPANSION 24         */ no_class_feats,
/* EXPANSION 25         */ no_class_feats,
/* EXPANSION 26         */ no_class_feats,
/* EXPANSION 27         */ no_class_feats,
/* EXPANSION 28         */ no_class_feats,
/* EXPANSION 29         */ no_class_feats,
/* EXPANSION 30         */ no_class_feats,
/* EXPANSION 31         */ no_class_feats,
/* EXPANSION 32         */ no_class_feats,
/* EXPANSION 33         */ no_class_feats,
/* EXPANSION 34         */ no_class_feats,
/* EXPANSION 35         */ no_class_feats
};

void display_alignments(struct descriptor_data *d)
{

  write_to_output(d, "\r\n");
  write_to_output(d, " 1) Lawful Good                         Please select an option from the left by\r\n");
  write_to_output(d, " 2) Lawful Neutral                      selecting the number which preceeds it.\r\n");
  write_to_output(d, " 3) Lawful Evil                         For help on any of the alignments press\r\n");
  write_to_output(d, " 4) Neutral Good                        the letter 'T' on your keyboard and\r\n");
  write_to_output(d, " 5) True Neutral                        then press enter.\r\n");
  write_to_output(d, " 6) Neutral Evil\r\n");
  write_to_output(d, " 7) Chaotic Good\r\n");
  write_to_output(d, " 8) Chaotic Neutral\r\n");
  write_to_output(d, " 9) Chaotic Evil\r\n");
  write_to_output(d, "\r\n");

}

void display_alignment_help(struct descriptor_data *d)
{

  write_to_output(d, "\r\n");
  write_to_output(d, " 1) Help On Lawful Good                 Please select an option from the left by\r\n");
  write_to_output(d, " 2) Help On Lawful Neutral              selecting the number which preceeds it.\r\n");
  write_to_output(d, " 3) Help On Lawful Evil                 To return to the alignment selection\r\n");
  write_to_output(d, " 4) Help On Neutral Good                menu press the letter 'T' and then press\r\n");
  write_to_output(d, " 5) Help On True Neutral                enter.\r\n");
  write_to_output(d, " 6) Help On Neutral Evil\r\n");
  write_to_output(d, " 7) Help On Chaotic Good\r\n");
  write_to_output(d, " 8) Help On Chaotic Neutral\r\n");
  write_to_output(d, " 9) Help On Chaotic Evil\r\n");
  write_to_output(d, "\r\n");

}

void parse_alignment(struct char_data *ch, char choice) 
{
  
  if (choice == '1') {
    GET_ETHOS(ch) = ETHOS_LAWFUL;
    GET_ALIGNMENT(ch) = ALIGNMENT_GOOD;
    return;
  }
  if (choice == '2') {
    GET_ETHOS(ch) = ETHOS_LAWFUL;
    GET_ALIGNMENT(ch) = ALIGNMENT_NEUTRAL;
    return;
  }
  if (choice == '3') {
    GET_ETHOS(ch) = ETHOS_LAWFUL;
    GET_ALIGNMENT(ch) = ALIGNMENT_EVIL;
    return;
  }
  if (choice == '4') {
    GET_ETHOS(ch) = ETHOS_NEUTRAL;
    GET_ALIGNMENT(ch) = ALIGNMENT_GOOD;
    return;
  }
  if (choice == '5') {
    GET_ETHOS(ch) = ETHOS_NEUTRAL;
    GET_ALIGNMENT(ch) = ALIGNMENT_NEUTRAL;
    return;
  }
  if (choice == '6') {
    GET_ETHOS(ch) = ETHOS_NEUTRAL;
    GET_ALIGNMENT(ch) = ALIGNMENT_EVIL;
    return;
  }
  if (choice == '7') {
    GET_ETHOS(ch) = ETHOS_CHAOTIC;
    GET_ALIGNMENT(ch) = ALIGNMENT_GOOD;
    return;
  }
  if (choice == '8') {
    GET_ETHOS(ch) = ETHOS_CHAOTIC;
    GET_ALIGNMENT(ch) = ALIGNMENT_NEUTRAL;
    return;
  }
  if (choice == '9') {
    GET_ETHOS(ch) = ETHOS_CHAOTIC;
    GET_ALIGNMENT(ch) = ALIGNMENT_EVIL;
    return;
  }

    return;
}
int get_saving_throw(int chclass, int savetype) 
{

  switch (savetype) {
    case SAVING_FORTITUDE:
    switch (chclass) {
      case CLASS_ELDRITCH_KNIGHT:
      case CLASS_CLERIC:
      case CLASS_SACRED_FIST:
      case CLASS_DRAGON_RIDER:
      case CLASS_FIGHTER:
      case CLASS_MONK:
      case CLASS_FAVORED_SOUL:
      case CLASS_PALADIN:
      case CLASS_BARBARIAN:
      case CLASS_ARCANE_ARCHER:    
      case CLASS_KNIGHT_OF_THE_CROWN:
      case CLASS_KNIGHT_OF_THE_SWORD:
      case CLASS_KNIGHT_OF_THE_ROSE:
      case CLASS_KNIGHT_OF_THE_SKULL:
      case CLASS_KNIGHT_OF_THE_LILY:
      case CLASS_GLADIATOR:
      case CLASS_DWARVEN_DEFENDER:
      case CLASS_DRUID:
      case CLASS_DRAGON_DISCIPLE:
        return SAVE_HIGH;
    }
    break;
    case SAVING_REFLEX:
    switch (chclass) {
      case CLASS_MONK:
      case CLASS_SACRED_FIST:
      case CLASS_DRAGON_RIDER:
      case CLASS_FAVORED_SOUL:
      case CLASS_ROGUE:
      case CLASS_ARCANE_ARCHER:    
      case CLASS_ASSASSIN:
      case CLASS_BARD:
      case CLASS_DUELIST:
      case CLASS_ARTISAN:
      case CLASS_ARCANE_TRICKSTER:
        return SAVE_HIGH;

    }
    break;
    case SAVING_WILL:
    switch (chclass) {
      case CLASS_DEATH_MASTER:
      case CLASS_ARCANE_TRICKSTER:
      case CLASS_FAVORED_SOUL:
      case CLASS_ARTISAN:
      case CLASS_WIZARD:
      case CLASS_SORCERER:
      case CLASS_MONK:
      case CLASS_SACRED_FIST:
      case CLASS_CLERIC:
      case CLASS_BARD:
      case CLASS_DRUID:
      case CLASS_KNIGHT_OF_THE_SWORD:
      case CLASS_KNIGHT_OF_THE_ROSE:
      case CLASS_KNIGHT_OF_THE_SKULL:
      case CLASS_KNIGHT_OF_THE_THORN:
      case CLASS_WIZARD_OF_HIGH_SORCERY:
      case CLASS_DWARVEN_DEFENDER:
      case CLASS_MYSTIC_THEURGE:
      case CLASS_DRAGON_DISCIPLE:
        return SAVE_HIGH;

    }
    break;
  }
  return SAVE_LOW;
}


int mob_exp_by_level(int level)
{
 int i = 0, amount = 0;
  int mult = 100;

  switch(level) {

  case 0:
    return 0;
    break;
  case 1:
    return 25 * mult / 100;
    break;
  case 2:
    return 33 * mult / 100;
    break;
  case 3:
    return 50 * mult / 100;
    break;
  case 4:
    return 65 * mult / 100;
    break;
  case 5:
    return 80 * mult / 100;
    break;
  case 6:
    return 100 * mult / 100;
    break;
  case 7:
    return 125 * mult / 100;
    break;
  case 8:
    return 150 * mult / 100;
    break;
  case 9:
    return 200 * mult / 100;
    break;
  case 10:
    return 250 * mult / 100;
    break;
  case 11:
    return 300 * mult / 100;
    break;
  case 12:
    return 375 * mult / 100;
    break;
  case 13:
    return 450 * mult / 100;
    break;
  case 14:
    return 550 * mult / 100;
    break;
  case 15:
    return 650 * mult / 100;
    break;
  case 16:
    return 800 * mult / 100;
    break;
  case 17:
    return 950 * mult / 100;
    break;
  case 18:
    return 1200 * mult / 100;
    break;
  case 19:
    return 1450 * mult / 100;
    break;
  case 20:
    return 1700 * mult / 100;
    break;
  default:
    amount = mob_exp_by_level(20);
    for (i = 0; i < level - 20; i++)
      amount += 300 + (((level - 20) % 3 == 0) ? (level - 20) / 3 * 50 : 0) * mult / 100;
    return amount;
    break;
  }
}

int mob_gold_by_level(int level)
{

 int i = 0, amount = 0;

  switch(level) {

  case 0:
    return 0;
    break;
  case 1:
    return 1;
    break;
  case 2:
    return 2;
    break;
  case 3:
    return 3;
    break;
  case 4:
    return 4;
    break;
  case 5:
    return 5;
    break;
  case 6:
    return 6;
    break;
  case 7:
    return 7;
    break;
  case 8:
    return 8;
    break;
  case 9:
    return 9;
    break;
  case 10:
    return 10;
    break;
  case 11:
    return 12;
    break;
  case 12:
    return 14;
    break;
  case 13:
    return 16;
    break;
  case 14:
    return 18;
    break;
  case 15:
    return 20;
    break;
  case 16:
    return 23;
    break;
  case 17:
    return 26;
    break;
  case 18:
    return 29;
    break;
  case 19:
    return 32;
    break;
  case 20:
    return 35;
    break;
  default:
    amount = mob_gold_by_level(20);
    for (i = 0; i < (level - 20); i++)
      amount +=  ((level / 8) + 1);
    return amount;
    break;
  }


}

int num_levelup_class_feats(struct char_data *ch, int whichclass, int ranks) {

	int add_class_feats = 0;
	int epiclevel = 21;
	int j = 0;

	/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
	  if (ranks >= (epiclevel - 1)) { /* Epic class */
	      j = ranks - 20;
	    switch (whichclass) 
      {
	// Epic Rogues
	    case CLASS_ROGUE:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Barbarians
	    case CLASS_BARBARIAN:
	      if (!(j % 4)) 
        {
	        add_class_feats++;
	      }
	      break;
	// Epic Druids
	    case CLASS_DRUID:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Paladins
	    case CLASS_PALADIN:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Rangers
	    case CLASS_RANGER:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Arcane Tricksters
	    case CLASS_ARCANE_TRICKSTER:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
        // Epic dragon riders
	    case CLASS_DRAGON_RIDER:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
        // Epic Sacred Fists
	    case CLASS_SACRED_FIST:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Eldritch Knight
	    case CLASS_ELDRITCH_KNIGHT:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Assassins
	    case CLASS_ASSASSIN:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Dragon Disciples
	    case CLASS_DRAGON_DISCIPLE:
	      if (!(j % 4))
	        add_class_feats++;
	      break;
	// Epic Fighters
	    case CLASS_FIGHTER:
	      if (!(j % 2))
	        add_class_feats++;
	      break;
	// Epic Duelists
	    case CLASS_DUELIST:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Templars
	    case CLASS_TEMPLAR:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Knights
	    case CLASS_KNIGHT:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Champions
	    case CLASS_CHAMPION:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Favored SOuls
	    case CLASS_FAVORED_SOUL:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	// Epic Monks
	    case CLASS_MONK:
	      if (!(j % 5))
	        add_class_feats++;
	      break;
	// Epic Dwarven Defenders
	    case CLASS_DWARVEN_DEFENDER:
	      if (!(j % 5))
	        add_class_feats++;
	      break;
	// Epic Mystic Theurges
	    case CLASS_MYSTIC_THEURGE:
	      if (!(j % 6))
	        add_class_feats++;
	      break;
	// Epic NPCs
	    case CLASS_NPC_EXPERT:
 	      break;
	// All other Epics
	    default:
	      if (!(j % 3))
	        add_class_feats++;
	      break;
	    }
	  } 
    else 
    {
	    switch (whichclass) 
      {
	    case CLASS_ELDRITCH_KNIGHT:
	      if (ranks == 1)
	        add_class_feats++;
	      break;
	    case CLASS_FIGHTER:
	      if (ranks == 1 || !(ranks % 2))
	        add_class_feats++;
	      break;
	    case CLASS_ROGUE:
	      if (!((ranks + 1)% 2))
	        add_class_feats++;
	      break;
	    case CLASS_KNIGHT_OF_THE_CROWN:
	    case CLASS_KNIGHT_OF_THE_SWORD:
	      if (!((ranks) % 3))
	        add_class_feats++;
	      break;
	    case CLASS_MONK:
	      if (ranks == 1 || ranks == 2 || ranks == 6)
	        add_class_feats++;
	      break;
	    case CLASS_WIZARD:
	      if (!(ranks % 5))
	        add_class_feats++;
	      break;
	    case CLASS_SORCERER:
        if (ranks == 1 || ranks ==7 || ranks == 19)
          add_class_feats++;
        break;
      default:
	      break;
	    }
	  }

	  return add_class_feats;
}

int num_levelup_practices(struct char_data *ch, int whichclass) 
{

	int add_prac = 0;

	  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
	  switch (whichclass) {
	  case CLASS_WIZARD:
	  case CLASS_KNIGHT_OF_THE_THORN:
	  case CLASS_MYSTIC_THEURGE:
	  case CLASS_SORCERER:
	  case CLASS_FAVORED_SOUL:
	  case CLASS_ELDRITCH_KNIGHT:
	  case CLASS_DEATH_MASTER:
	  case CLASS_DRAGON_RIDER:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_CLERIC:
	  case CLASS_KNIGHT_OF_THE_SKULL:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_DRAGON_DISCIPLE:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_ARCANE_TRICKSTER:
	    add_prac = 6 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_ROGUE:
	    add_prac = 8 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_ARTISAN:
	    add_prac = 8 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_FIGHTER:
	  case CLASS_KNIGHT_OF_THE_LILY:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_MONK:
	  case CLASS_SACRED_FIST:
          case CLASS_ARCANE_ARCHER:
	    add_prac = 4 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_ASSASSIN:
	    add_prac = 4 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_PALADIN:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	    break;
	  case CLASS_BARBARIAN:
	    add_prac = 4 + ability_mod_value(ch->real_abils.intel);
	  	break;
	  case CLASS_DRUID:
	    add_prac = 4 + ability_mod_value(ch->real_abils.intel);
	 	break;
	  case CLASS_RANGER:
	    add_prac = 6 + ability_mod_value(ch->real_abils.intel);
	  	break;
	  case CLASS_BARD:
	    add_prac = 6 + ability_mod_value(ch->real_abils.intel);
	  	break;
	  case CLASS_DUELIST:
	    add_prac = 4 + ability_mod_value(ch->real_abils.intel);
	  	break;
	  case CLASS_KNIGHT_OF_THE_CROWN:
	  case CLASS_KNIGHT_OF_THE_ROSE:
	  case CLASS_DWARVEN_DEFENDER:
	  case CLASS_WEAPON_MASTER:
	  case CLASS_KNIGHT_OF_THE_SWORD:
	    add_prac = 2 + ability_mod_value(ch->real_abils.intel);
	  	break;
	  }
	  if (IS_HUMAN(ch))
	    add_prac++;
	  add_prac = MAX(1, add_prac);

	  return add_prac;
}

ACMD(do_classabilities)
{

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Please specify a class name to view ability progression on.\r\n");
    return;
  }  


  int i = 0;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (is_abbrev(argument, class_names_dl_aol[i])) {
      break;    
    }
  }

  if (i > NUM_CLASSES) {
    send_to_char(ch, "Please specify a class name to view ability progression on.\r\n");
    return;    
  }

  int max_ranks = CONFIG_LEVEL_CAP;

  if (prestige_classes_dl_aol[i]) {
    max_ranks = 21;
  }

  int j = 0;
  int n = 0;
  int found = FALSE;

  for (j = 1; j < max_ranks; j++) {
    found = FALSE;    
    if (j >= ((max_ranks == 21) ? 10 : 20)) {
      n = 0;
      while (epic_level_feats[n][0] != CLASS_UNDEFINED) {        
        if (i == epic_level_feats[n][0]) {
          if ((((j - ((max_ranks == 21) ? 10 : 20) + epic_level_feats[n][1]) % epic_level_feats[n][2]) == epic_level_feats[n][3]) == epic_level_feats[n][4]) {
            send_to_char(ch, "%2d) %s increases by %d\r\n", j, feat_list[epic_level_feats[n][5]].name, epic_level_feats[n][6]);
            found = TRUE;
          }
        }
        n++;
      }
    } else {
      n = 0;
      while (level_feats[n][4] != FEAT_UNDEFINED) {
        if (i == level_feats[n][0] && level_feats[n][1] == RACE_UNDEFINED && j == level_feats[n][3]) {
          send_to_char(ch, "%2d) %s increases by %d\r\n", j, feat_list[level_feats[n][4]].name, 
                       (i == CLASS_DWARVEN_DEFENDER && level_feats[n][4] == FEAT_DAMAGE_REDUCTION) ? 3 : 1);
            found = TRUE;
        }
        n++;
      }
    }
    if (!found) {
      send_to_char(ch, "%2d) no ability gain this level\r\n", j);
    }
  }

}
