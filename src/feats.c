/*****************************************************************************
** FEATS.C                                                                  **
** Source code for the Gates of Krynn Feats System.                         **
** Initial code by Paladine (Stephen Squires)                               **
** Created Thursday, September 5, 2002                                      **
**                                                                          **
*****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "feats.h"
#include "grid.h"

/* Local Functions */
void list_class_feats(struct char_data *ch);
void assign_feats(void);
void feato(int featnum, char *name, int in_game, int can_learn, int can_stack, char *prerequisites, char *description);
void list_feats_known(struct char_data *ch, char *arg); 
void list_feats_available(struct char_data *ch, char *arg); 
void list_feats_complete(struct char_data *ch, char *arg); 
int compare_feats(const void *x, const void *y);
void sort_feats(void);	
int find_feat_num(char *name);
void load_weapons(void);
void load_armor(void);
void display_levelup_feats(struct char_data *ch);
int has_combat_feat(struct char_data *ch, int i, int j);
int has_feat(struct char_data *ch, int featnum);
void set_feat(struct char_data *ch, int i, int j);
void display_levelup_weapons(struct char_data *ch);
int has_weapon_feat(struct char_data *ch, int i, int j);
int has_weapon_feat_full(struct char_data *ch, int i, int j, int display);

/* Global Variables and Structures */
struct feat_info feat_list[NUM_FEATS_DEFINED+1];
int feat_sort_info[MAX_FEATS + 1];
char buf3[MAX_STRING_LENGTH];
char buf4[MAX_STRING_LENGTH];
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES + 1];
struct weapon_table weapon_list[MAX_WEAPON_TYPES + 1];
const char *weapon_type[MAX_WEAPON_TYPES + 1];

/* External variables and structures */
extern int level_feats[][6];
extern int spell_sort_info[SKILL_TABLE_SIZE+1];
extern struct spell_info_type spell_info[];
extern const int *class_bonus_feats[];
extern char *weapon_damage_types[];

/* External functions*/
int count_metamagic_feats(struct char_data *ch);
int find_armor_type(int specType);

/* START */

/* Helper function for t sort_feats function - not very robust and should not be reused.
 * SCARY pointer stuff! */
int compare_feats(const void *x, const void *y)
{
  int   a = *(const int *)x,
        b = *(const int *)y;
  
  return strcmp(feat_list[a].name, feat_list[b].name);
}

/* sort feats called at boot up */
void sort_feats(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= NUM_FEATS_DEFINED; a++)
    feat_sort_info[a] = a;

  qsort(&feat_sort_info[1], NUM_FEATS_DEFINED, sizeof(int), compare_feats);
}

/* checks if the char has the feat either saved to file or in the process
 of acquiring it in study */
int has_feat(struct char_data *ch, int featnum) 
{

  if (ch->desc && ch->levelup && STATE(ch->desc) >= CON_LEVELUP_START && STATE(ch->desc) <= CON_LEVELUP_END) 
  {
    return (HAS_FEAT(ch, featnum) + ch->levelup->feats[featnum]);
  }
/*
  // this is for the option of allowing feats on items
  struct obj_data *obj;
  int i = 0, j = 0;

  for (j = 0; j < NUM_WEARS; j++) {
    if ((obj = GET_EQ(ch, j)) == NULL)
      continue;
    for (i = 0; i < 6; i++) {
      if (obj->affected[i].location == APPLY_FEAT && obj->affected[i].specific == featnum)
        return (HAS_FEAT(ch, featnum) + obj->affected[i].modifier);
    }
  }
*/
  return HAS_FEAT(ch, featnum);
}


void feato(int featnum, char *name, int in_game, int can_learn, int can_stack, char *prerequisites, char *description)
{
  feat_list[featnum].name = name;
  feat_list[featnum].in_game = in_game;
  feat_list[featnum].can_learn = can_learn;
  feat_list[featnum].can_stack = can_stack;
  feat_list[featnum].prerequisites = prerequisites;
  feat_list[featnum].description = description;
}

void epicfeat(int featnum)
{
  feat_list[featnum].epic = TRUE;
}

void combatfeat(int featnum)
{
  feat_list[featnum].combat_feat = TRUE;
}

void free_feats(void)
{
  /* Nothing to do right now */
}

void assign_feats(void)
{

  int i = 0;

  // Initialize the list of feats.

  for (i = 0; i <= NUM_FEATS_DEFINED; i++) 
  {
    feat_list[i].name = "Unused Feat";
    feat_list[i].in_game = FALSE;
    feat_list[i].can_learn = FALSE;
    feat_list[i].can_stack = FALSE;
    feat_list[i].prerequisites = "ask staff";
    feat_list[i].description = "ask staff";
    feat_list[i].epic = FALSE;
    feat_list[i].combat_feat = FALSE;
  }

/* feat-number | in-game name | in-game? | learnable? | stackable? | prerequisite | description |*/

feato(FEAT_ABLE_LEARNER, "Able Learner", TRUE, TRUE, FALSE, "-", "+1 bonus to all skills");
feato(FEAT_ACROBATIC, "Acrobatic", TRUE, TRUE, FALSE, "-", "+2 bonus on Acrobatics and Fly checks. Fly is not implemented yet.");
feato(FEAT_ALERTNESS, "Alertness", TRUE, TRUE, FALSE, "-", "+2 bonus on Perception and Sense Motive checks."); 
feato(FEAT_ANIMAL_AFFINITY, "Animal Affinity", TRUE, TRUE, FALSE, "-", "+2 bonus on Handle Animal and Ride checks.");
feato(FEAT_ARMOR_PROFICIENCY_HEAVY, "Armor Proficiency (Heavy)", TRUE, TRUE, FALSE, "Armor Proficiency  (Medium)", "No penalties on attack rolls while wearing heavy armor"); 
feato(FEAT_ARMOR_PROFICIENCY_LIGHT, "Armor Proficiency (Light)", TRUE, TRUE, FALSE, "-", "No penalties on attack rolls while wearing light armor"); 
feato(FEAT_ARMOR_PROFICIENCY_MEDIUM, "Armor Proficiency (Medium)", TRUE, TRUE, FALSE, "Armor Proficiency (Light)", "No penalties on attack rolls while wearing medium armor"); 
feato(FEAT_ARMOR_PROFICIENCY_SHIELD, "Armor Proficiency (Shield)", TRUE, FALSE, FALSE, "-", "No penalties on attack rolls when using a shield"); 
feato(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, "Armor Proficiency (Tower Shield)", TRUE, TRUE, FALSE, "Armor Proficiency (Shield)", "No penalties on attack rolls when using a tower shield");
feato(FEAT_AUGMENT_SUMMONING, "Augment Summoning", TRUE, TRUE, FALSE, "Spell Focus (conjuration)", "Summoned creatures gain +4 Str and Con.");
feato(FEAT_BLIND_FIGHT, "Blind-Fight", TRUE, TRUE, FALSE, "-", "reduced penalties when fighting blind oragainst invisible opponents"); 
feato(FEAT_BREW_POTION, "Brew Potion", FALSE, FALSE, FALSE, "Caster level 3rd", "Create magic potions."); 
feato(FEAT_CLEAVE, "Cleave", TRUE, TRUE, FALSE, "Power Attack", "Make an additional attack if the first one hits.");
feato(FEAT_COMBAT_CASTING, "Combat Casting", TRUE, TRUE, FALSE, "-", "+4 bonus on concentration checks for defensive casting"); 
feato(FEAT_COMBAT_EXPERTISE, "Combat Expertise", TRUE, TRUE, FALSE, "Int 13", "Trade attack bonus for AC bonus");
feato(FEAT_COMBAT_REFLEXES, "Combat Reflexes", TRUE, TRUE, FALSE, "-", "Make additional attacks of opportunity");
feato(FEAT_COMBAT_STYLE, "Combat Style", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_COMBAT_STYLE_MASTERY, "Combat Style Master", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR, "Craft Magical Arms and Armor", FALSE, FALSE, FALSE, "Caster level 5th", "Create magic armors, shields, and weapons."); 
feato(FEAT_CRAFT_ROD, "Craft Rod", FALSE, FALSE, FALSE, "Caster level 9th", "Create magic rods");
feato(FEAT_CRAFT_STAFF, "Craft Staff", FALSE, FALSE, FALSE, "Caster level 11th", "Create magic staves"); 
feato(FEAT_CRAFT_WAND, "Craft Wand", FALSE, FALSE, FALSE, "Caster level 5th", "Create magic wands"); 
feato(FEAT_CRAFT_WONDEROUS_ITEM, "Craft Wonderous Item", FALSE, FALSE, FALSE, "Caster level 3rd", "Create magic wondrous items"); 
feato(FEAT_CRITICAL_FOCUS, "Critical Focus", TRUE, TRUE, TRUE, "Base attack bonus +9", "+4 bonus on attack rolls made to confirm critical hits");
feato(FEAT_DARKVISION, "Darkvision", TRUE, FALSE, FALSE, "-", "Darkvision is the extraordinary ability to see with no light source at all");
feato(FEAT_DECEITFUL, "Deceitful", TRUE, TRUE, FALSE, "-", "+2 bonus on Bluff and Disguise checks");
feato(FEAT_DEFLECT_ARROWS, "Deflect Arrows", TRUE, TRUE, FALSE, "Dex 13, Improved Unarmed Strike", "Avoid one ranged attack per round"); 
feato(FEAT_DEFT_HANDS, "Deft Hands", TRUE, TRUE, FALSE, "-", "+2 bonus on Disable Device and Sleight of Hand checks");
feato(FEAT_DIEHARD, "Diehard", TRUE, TRUE, FALSE, "Endurance", "will stay alive and conscious until -10 hp or lower");
feato(FEAT_DIVINE_MIGHT, "Divine Might", TRUE, TRUE, FALSE, "turn undead, power attack, cha 13, str 13", "Add cha bonus to damage for number of rounds equal to cha bonus");
feato(FEAT_DIVINE_SHIELD, "Divine Shield", TRUE, TRUE, FALSE, "turn undead, power attack, cha 13, str 13", "Add cha bonus to armor class for number of rounds equal to cha bonus");
feato(FEAT_DIVINE_VENGEANCE, "Divine Vengeance", TRUE, TRUE, FALSE, "turn undead, extra turning", "Add 2d6 damage against undead for number of rounds equal to cha bonus");
feato(FEAT_DODGE, "Dodge", TRUE, TRUE, FALSE, "Dex 13", "+1 dodge bonus to AC."); 
feato(FEAT_EMPOWER_SPELL, "Empower Spell", TRUE, TRUE, FALSE, "Caster level 1st", "all variable numerical effects of a spell are increased by 50%%"); 
feato(FEAT_EMPOWERED_MAGIC, "Empowered Magic", TRUE, TRUE, FALSE, "Caster level 1st", "+1 to all spell dcs");
feato(FEAT_ENDURANCE, "Endurance", TRUE, TRUE, FALSE, "-", "+4 to con and skill checks made to resist fatigue and 1 extra move point per level"); 
feato(FEAT_ENERGY_RESISTANCE, "Energy Resistance", TRUE, TRUE, TRUE, "-", "reduces all energy related damage by 3 per rank");
feato(FEAT_ENHANCED_SPELL_DAMAGE, "Enhanced Spell Damage", TRUE, TRUE, FALSE, "Caster level 1st", "+1 spell damage per die rolled");
feato(FEAT_ENLARGE_SPELL, "Enlarge Spell", FALSE, FALSE, FALSE, "ask staff", "ask staff"); 
feato(FEAT_ESCHEW_MATERIALS, "Eschew Materials", FALSE, FALSE, FALSE, "ask staff", "Cast spells without material components");
feato(FEAT_EXCEPTIONAL_TURNING, "Exceptional Turning", TRUE, FALSE, FALSE, "sun cleric domain", "+1d10 hit dice of undead turned");
feato(FEAT_EXTEND_RAGE, "Extend Rage", TRUE, TRUE, FALSE, "ask staff", "ask staff");
feato(FEAT_EXTEND_SPELL, "Extend Spell", TRUE, TRUE, FALSE, "Caster level 1st", "durations of spells are 50%% longer when enabled"); 
feato(FEAT_EXTRA_RAGE, "Extra Rage", TRUE, TRUE, FALSE, "Rage class feature", "ask staff");
feato(FEAT_EXTRA_TURNING, "Extra Turning", TRUE, TRUE, FALSE, "1st-level Cleric or Paladin", "2 extra turn attempts per day");
feato(FEAT_FAR_SHOT, "Far Shot", FALSE, FALSE, FALSE, "Point-Blank Shot", "Decrease ranged penalties by half");
feato(FEAT_FAST_HEALER, "Fast Healer", TRUE, TRUE, FALSE, "Con 13, Diehard", "+2 hp healed per round");
feato(FEAT_FASTER_MEMORIZATION, "Faster Memorization", TRUE, TRUE, FALSE, "memorization based Caster level 1", "decreases spell memorization time");
feato(FEAT_FORGE_RING, "Forge Ring", FALSE, FALSE, FALSE, "Caster level 7th", "Create magic rings"); 
feato(FEAT_GREAT_CLEAVE, "Great Cleave", FALSE, FALSE, FALSE, "Cleave, base attack bonus +4", "ask staff");
feato(FEAT_GREAT_FORTITUDE, "Great Fortitude", TRUE, TRUE, FALSE, "-", "+2 on Fortitude saves");
feato(FEAT_GREATER_SPELL_FOCUS, "Greater Spell Focus", FALSE, FALSE, TRUE, "Spell Focus", "ask staff");
feato(FEAT_GREATER_SPELL_PENETRATION, "Greater Spell Penetration", FALSE, FALSE, FALSE, "Spell Penetration", "ask staff");
feato(FEAT_GREATER_TWO_WEAPON_FIGHTING, "Greater Two Weapon Fighting", TRUE, TRUE, FALSE, "DEX 19, Base attack bonus +11, Improved Two-Weapon Fighting", "gives an additional offhand weapon attack");
feato(FEAT_GREATER_WEAPON_FOCUS, "Greater Weapon Focus", TRUE, TRUE, TRUE, "Weapon Focus, 8th-level fighter", "additional +1 to hit rolls with weapon (stacks)");
feato(FEAT_GREATER_WEAPON_SPECIALIZATION, "Greater Weapon Specialization", TRUE, TRUE, TRUE, "Weapon Specialization, 12th-level fighter", "+2 bonus on damage rolls with one weapon");
feato(FEAT_HEIGHTEN_SPELL, "Heighten Spell", FALSE, FALSE, FALSE, "ask staff", "ask staff"); 
feato(FEAT_IMPROVED_BULL_RUSH, "Improved Bull Rush", FALSE, FALSE, FALSE, "Power Attack", "ask staff");
feato(FEAT_IMPROVED_COMBAT_CHALLENGE, "Improved Combat Challenge", TRUE, TRUE, FALSE, "10 ranks in diplomacy, intimidate or bluff, combat challenge", "allows you to make all mobs focus their attention on you");
feato(FEAT_IMPROVED_COMBAT_STYLE, "Improved Combat Style", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_IMPROVED_COUNTERSPELL, "Improved Counterspell", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_IMPROVED_CRITICAL, "Improved Critical", TRUE, TRUE, TRUE, "Proficient with weapon, Base attack bonus +8", "Double the threat range of one weapon");
feato(FEAT_IMPROVED_DISARM, "Improved Disarm", FALSE, FALSE, FALSE, "Combat Expertise", "ask staff"); 
feato(FEAT_IMPROVED_FAMILIAR, "Improved Familiar", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_IMPROVED_FEINT, "Improved Feint", TRUE, TRUE, FALSE, "Combat Expertise", "can feint and make one attack per round (or sneak attack if they have it)");
feato(FEAT_IMPROVED_GRAPPLE, "Improved Grapple", FALSE, FALSE, FALSE, "Dex 13, Improved Unarmed Strike", "ask staff");
feato(FEAT_IMPROVED_INITIATIVE, "Improved Initiative", TRUE, TRUE, FALSE, "-", "+4 bonus on initiative checks");
feato(FEAT_IMPROVED_NATURAL_WEAPON, "Improved Natural Weapons", TRUE, TRUE, FALSE, "Natural Weapon, Base attack bonus +4", "increase damage dice by one category for natural weapons");
feato(FEAT_IMPROVED_OVERRUN, "Improved Overrun", FALSE, FALSE, FALSE, "Power Attack", "ask staff");
feato(FEAT_IMPROVED_SHIELD_BASH, "Improved Shield Bash", FALSE, FALSE, FALSE, "Shield Proficiency", "ask staff");
feato(FEAT_IMPROVED_SNEAK_ATTACK, "Improved Sneak Attack", TRUE, TRUE, TRUE, "sneak attack 1d6 or more", "each rank gives +5%% chance per attack, per rank to be a sneak attack.");
feato(FEAT_IMPROVED_SUNDER, "Improved Sunder", FALSE, FALSE, FALSE, "Power Attack", "ask staff");
feato(FEAT_IMPROVED_TRIP, "Improved Trip", TRUE, TRUE, FALSE, "Combat Expertise", "no attack of opportunity when tripping, +4 to trip check");
feato(FEAT_IMPROVED_TURNING, "Improved Turning", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, "Improved Two Weapon Fighting", TRUE, TRUE, FALSE, "dex 17, two weapon fighting, base attack bonus of +6 or more", "extra attack with offhand weapon at -5 penalty");
feato(FEAT_IMPROVED_UNARMED_STRIKE, "Improved Unarmed Strike", TRUE, TRUE, FALSE, "-", "Always considered armed");
feato(FEAT_IMPROVED_UNCANNY_DODGE, "Improved Uncanny Dodge", TRUE, FALSE, FALSE, "-", "cannot be flanked (or sneak attacked");
feato(FEAT_IMPROVED_WEAPON_FINESSE, "Improved Weapon Finesse", TRUE, TRUE, TRUE, "weapon finesse, weapon focus, base attack bonus of 4+", "add dex bonus to damage instead of str for light weapons");
feato(FEAT_IRON_WILL, "Iron Will", TRUE, TRUE, FALSE, "-", "+2 bonus on Will saves");
feato(FEAT_KNOCKDOWN, "Knockdown", TRUE, TRUE, FALSE, "improved trip", "when active, any melee attack that deals 10 damage or more invokes a free automatic trip attempt against your target");
feato(FEAT_LEADERSHIP, "Leadership", TRUE, TRUE, FALSE, "Character level 7th", "can have more and higher level followers, group members get extra exp on kills and hit/ac bonuses");
feato(FEAT_LEADERSHIP_BONUS, "Improved Leadership", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_LIGHTNING_REFLEXES, "Lightning Reflexes", TRUE, TRUE, FALSE, "-", "+2 bonus on Reflex saves");
feato(FEAT_LOW_LIGHT_VISION, "Low Light Vision", TRUE, FALSE, FALSE, "-", "can see in the dark outside only");
feato(FEAT_MAGICAL_APTITUDE, "Magical Aptitude", TRUE, TRUE, FALSE, "-", "+2 bonus on Spellcraft and Use Magic Device checks");
feato(FEAT_MAXIMIZE_SPELL, "Maximize Spell", TRUE, TRUE, FALSE, "Caster level 1st", "all spells cast while maximised enabled do maximum effect.");
feato(FEAT_MOBILITY, "Mobility", TRUE, TRUE, FALSE, "Dodge", "+4 AC against attacks of opportunity from movement");
feato(FEAT_MONKEY_GRIP, "Monkey Grip", FALSE, TRUE, TRUE, "-", "can wield weapons one size larger than wielder in one hand with -2 to attacks.");
feato(FEAT_MOUNTED_ARCHERY, "Mounted Archery", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_MOUNTED_COMBAT, "Mounted Combat", TRUE, TRUE, FALSE, "Ride 1 rank", "once per round rider may negate a hit against him with a successful ride vs attack roll check");
feato(FEAT_NATURAL_ARMOR_INCREASE, "Natural Armor Increase", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_NATURAL_SPELL, "Natural Spell", TRUE, TRUE, FALSE, "Wis 13, wild shape class feature.", "Cast spells while using wild shape");
feato(FEAT_PERSUASIVE, "Persuasive", TRUE, TRUE, FALSE, "-", "+2 bonus on Diplomacy and Intimidate checks");
feato(FEAT_POINT_BLANK_SHOT, "Point Blank Shot", TRUE, TRUE, FALSE, "-", "+1 to hit and dam rolls with ranged weapons in the same room");
feato(FEAT_POWER_ATTACK, "Power Attack", TRUE, TRUE, FALSE, "Str 13, base attack bonus +1", "subtract a number from hit and add to dam.  If 2H weapon add 2x dam instead");
feato(FEAT_PRECISE_SHOT, "Precise Shot", TRUE, TRUE, FALSE, "Point-Blank Shot", "You may shoot in melee without the standard -4 to hit penalty");
feato(FEAT_PRECISE_STRIKE, "Precise Strike", TRUE, FALSE, FALSE, "-", "+1d6 damage when using only one weapon and no shield");
feato(FEAT_QUICK_DRAW, "Quick Draw", FALSE, FALSE, FALSE, "Base attack bonus +1", "ask staff");
feato(FEAT_QUICKEN_SPELL, "Quicken Spell", TRUE, TRUE, FALSE, "Caster level 1st", "allows you to cast spell as a move action instead of standard action");
feato(FEAT_RAPID_RELOAD, "Rapid Reload", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_RAPID_SHOT, "Rapid Shot", TRUE, TRUE, FALSE, "Dex 13, Point-Blank Shot", "can make extra attack per round with ranged weapon at -2 to all attacks");
feato(FEAT_RIDE_BY_ATTACK, "Ride-By Attack", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_RUN, "Run", TRUE, TRUE, FALSE, "ask staff", "ask staff");
feato(FEAT_SCRIBE_SCROLL, "Scribe Scroll", FALSE, FALSE, FALSE, "Caster level 1st", "Create magic scrolls");
feato(FEAT_SELF_SUFFICIENT, "Self Sufficient", TRUE, TRUE, FALSE, "-", "You get a +2 bonus on all Heal checks and Survival checks.");
feato(FEAT_SHOT_ON_THE_RUN, "Shot on the Run", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_SILENT_SPELL, "Silent Spell", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_SKILL_FOCUS, "Skill Focus", TRUE, TRUE, TRUE, "-", "+3 bonus on one skill");
feato(FEAT_SNEAK_ATTACK, "Sneak Attack", TRUE, TRUE, TRUE, "-", "+1d6 to damage when flanking");
feato(FEAT_SNEAK_ATTACK_OF_OPPORTUNITY, "Sneak Attack of Opportunity", TRUE, TRUE, FALSE, "sneak attack +8d6, opportunist feat", "makes all opportunity attacks sneak attacks");
feato(FEAT_SPELL_FOCUS, "Spell Focus", FALSE, FALSE, FALSE, "Caster level 1st", "Add +1 to the Difficulty Class for all saving throws against spells from the school of magic you select.");
feato(FEAT_SPELL_MASTERY, "Spell Mastery", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_SPELL_PENETRATION, "Spell Penetration", FALSE, FALSE, FALSE, "-", "+2 bonus on level checks to beat spell resistance");
feato(FEAT_SPIRITED_CHARGE, "Spirited Charge", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_SPRING_ATTACK, "Spring Attack", TRUE, TRUE, FALSE, "Mobility, base attack bonus +4", "free attack of opportunity against combat abilities (ie. kick, trip)");
feato(FEAT_STEADFAST_DETERMINATION, "Steadfast Determination", TRUE, TRUE, FALSE, "Endurance", "allows you to use your con bonus instead of your wis bonus for will saves");
feato(FEAT_STEALTHY, "Stealthy", TRUE, TRUE, FALSE, "-", "+2 bonus on Escape Artist and Stealth checks");
feato(FEAT_STILL_SPELL, "Still Spell", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_STRENGTH_BOOST, "Strength Boost", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_STUNNING_FIST, "Stunning Fist", TRUE, TRUE, FALSE, "Dex 13, Wis 13, Improved Unarmed Strike, base attack bonus +8", "Stun opponent with an unarmed strike");
feato(FEAT_SUMMON_FAMILIAR, "Summon Familiar", TRUE, FALSE, FALSE, "-", "summon a magical pet");
feato(FEAT_SUNDER, "Sunder", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_TOUGHNESS, "Toughness", TRUE, TRUE, FALSE, "-", "+3 hit points, +1 per Hit Die beyond 3");
feato(FEAT_TRACK, "Track", FALSE, FALSE, FALSE, "-", "use survival skill to track others");
feato(FEAT_TRAMPLE, "Trample", FALSE, FALSE, FALSE, "Mounted Combat", "ask staff");
feato(FEAT_TWO_WEAPON_DEFENSE, "Two Weapon Defense", TRUE, TRUE, FALSE, "Two-Weapon Fighting", "Gain +1 shield bonus when fighting with two weapons");
feato(FEAT_TWO_WEAPON_FIGHTING, "Two-Weapon Fighting", TRUE, TRUE, FALSE, "Dex 15", "Reduce two-weapon fighting penalties");
feato(FEAT_UNARMED_STRIKE, "Unarmed Strike", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WEAPON_FINESSE, "Weapon Finesse", TRUE, TRUE, FALSE, "-", "Use Dex instead of Str on attack rolls with light weapons");
feato(FEAT_WEAPON_FLURRY, "Weapon Flurry", TRUE, TRUE, TRUE, "Weapon Mastery, base attack bonus +14", "2nd attack at -5 to hit with standard action or extra attack at full bonus with full round action");
feato(FEAT_WEAPON_FOCUS, "Weapon Focus", TRUE, TRUE, TRUE, "Proficiency with weapon, base attack bonus +1", "+1 to hit rolls for selected weapon");
feato(FEAT_WEAPON_MASTERY, "Weapon Mastery", TRUE, TRUE, TRUE, "Weapon Specialization, Base Attack Bonus +8", "+2 to hit and damage with that weapon");
feato(FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD, "Weapon Proficiency (Bastard Sword)", FALSE, TRUE, FALSE, "ask staff", "ask staff");
feato(FEAT_WEAPON_PROFICIENCY_ELF, "Weapon Proficiency (Elf)", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WEAPON_PROFICIENCY_EXOTIC, "Weapon Proficiency (Exotic)", TRUE, TRUE, TRUE, "Base attack bonus +1", "You understand how to use that type of exotic weapon in combat.");
feato(FEAT_WEAPON_PROFICIENCY_MARTIAL, "Weapon Proficiency (Martial)", TRUE, TRUE, FALSE, "-", "You understand how to use martial weapons in combat.");
feato(FEAT_WEAPON_PROFICIENCY_SIMPLE, "Weapon Proficiency (Simple)", TRUE, TRUE, FALSE, "-", "You are trained in the use of basic weapons.");
feato(FEAT_WEAPON_SUPREMACY, "Weapon Supremacy", TRUE, TRUE, TRUE, "Weapon Mastery, greater weapon focus, greater weapon specialization, fighter level 18", "+4 to resist disarm, ignore grapples, add +5 to hit roll when miss by 5 or less, can take 10 on attack rolls, +1 bonus to AC when wielding weapon");
feato(FEAT_WHIRLWIND_ATTACK, "Whirlwind Attack", TRUE, TRUE, FALSE, "Dex 13, Combat Expertise, Spring Attack, base attack bonus +4", "allows you to attack everyone in the room or everyone you are fighting (with contain) as a standard action");
feato(FEAT_WIDEN_SPELL, "Widen Spell", FALSE, FALSE, FALSE, "Caster level 1st", "ask staff");

feato(FEAT_UNCANNY_DODGE, "Uncanny Dodge", TRUE, FALSE, FALSE, "-", "retains dex bonus when flat footed or against invis opponents");


/* Artisan Class feats */
feato(FEAT_BRANDING, "Branding", TRUE, FALSE, FALSE, "3rd-level Artisan", "All items made carry the artisan's brand");
feato(FEAT_DRACONIC_CRAFTING, "Draconic Crafting", TRUE, FALSE, FALSE, "20th-level Artisan", "All magical items created gain higher bonuses w/o increasing level");
feato(FEAT_DWARVEN_CRAFTING, "Dwarven Crafting", TRUE, FALSE, FALSE, "15th-level Artisan", "All weapons and armor made have higher bonuses");
feato(FEAT_ELVEN_CRAFTING, "Elven Crafting", TRUE, FALSE, FALSE, "11th-level Artisan", "All equipment made is 50%% weight and uses 50%% materials");
feato(FEAT_FAST_CRAFTER, "Fast Crafter", TRUE, FALSE, FALSE, "1st-level Artisan", "Reduces crafting time");
feato(FEAT_LEARNED_CRAFTER, "Learned Crafter", TRUE, FALSE, FALSE, "1st-level Artisan", "Artisan gains exp for crafting items and harvesting");
feato(FEAT_MASTERWORK_CRAFTING, "Masterwork Crafting", TRUE, FALSE, FALSE, "6th-level Artisan", "All equipment made is masterwork");
feato(FEAT_PROFICIENT_CRAFTER, "Proficient Crafter", TRUE, FALSE, FALSE, "2nd-level Artisan", "Increases all crafting skills");
feato(FEAT_PROFICIENT_HARVESTER, "Proficient Harvester", TRUE, FALSE, FALSE, "4th-level Artisan", "Increases all harvesting skills");
feato(FEAT_SCAVENGE, "Scavenge", TRUE, FALSE, FALSE, "5th-level Artisan", "Can find materials on corpses");

/* Barbarian Class feats */
feato(FEAT_DAMAGE_REDUCTION, "Damage Reduction", TRUE, TRUE, TRUE, "7th-level Barbarian", "1/- damage reduction per rank of feat, 3/- for epic");
feato(FEAT_FAST_MOVEMENT, "Fast Movement", TRUE, FALSE, TRUE, "1st-level Barbarian", "10ft bonus to speed in light or medium armor");
feato(FEAT_GREATER_RAGE, "Greater Rage", TRUE, FALSE, FALSE, "11th-level Barbarian", "+6 to str and con when raging");
feato(FEAT_INDOMITABLE_WILL, "Indomitable Will", TRUE, FALSE, FALSE, "14th-level Barbarian", "ask staff");
feato(FEAT_RAGE, "Rage", TRUE, FALSE, TRUE, "1st-level Barbarian", "+4 bonus to con and str for several rounds");
feato(FEAT_TIRELESS_RAGE, "Tireless Rage", TRUE, FALSE, FALSE, "17th-level Barbarian", "no fatigue after raging");

/* Bard Class feats */
feato(FEAT_COUNTERSONG, "Countersong", TRUE, FALSE, FALSE, "1st-level Bard", "ask staff");
feato(FEAT_FASCINATE, "Fascinate", TRUE, FALSE, FALSE, "1st-level Bard", "ask staff");
feato(FEAT_BARDIC_KNOWLEDGE, "Bardic Knowledge", TRUE, FALSE, FALSE, "1st-level Bard", "ask staff");
feato(FEAT_BARDIC_MUSIC, "Bardic Music", TRUE, FALSE, FALSE, "1st-level Bard", "ask staff");
feato(FEAT_INSPIRE_COMPETENCE, "Inspire Competence", TRUE, FALSE, FALSE, "3rd-level Bard", "ask staff");
feato(FEAT_INSPIRE_COURAGE, "Inspire Courage", TRUE, FALSE, FALSE, "1st-level Bard", "ask staff");
feato(FEAT_INSPIRE_GREATNESS, "Inspire Greatness", TRUE, FALSE, FALSE, "9th-level Bard", "ask staff");
feato(FEAT_INSPIRE_HEROICS, "Inspire Heroics", TRUE, FALSE, FALSE, "15th-level Bard", "ask staff");
feato(FEAT_SONG_OF_FREEDOM, "Song of Freedom", TRUE, FALSE, FALSE, "12t-level Bard", "ask staff");
feato(FEAT_SUGGESTION, "Suggestion", TRUE, FALSE, FALSE, "6th-level Bard", "ask staff");
feato(FEAT_MASS_SUGGESTION, "Mass Suggestion", TRUE, FALSE, FALSE, "18th-level Bard", "ask staff");
feato(FEAT_LINGERING_SONG, "Lingering Song", TRUE, TRUE, FALSE, "1st-level Bard", "5 extra rounds for bard songs");
feato(FEAT_EXTRA_MUSIC, "Extra Music", TRUE, TRUE, FALSE, "1st-level Bard", "4 extra bard music uses per day");

/* Cleric Class feats */
feato(FEAT_WEAPON_PROFICIENCY_DEITY, "Weapon Proficiency (Deity)", TRUE, FALSE, FALSE, "1st-level Cleric", "Clerics are proficient with the favored weapon of their deity");
feato(FEAT_TURN_UNDEAD, "Turn Undead", TRUE, TRUE, FALSE, "-", "Channel energy can be used to make undead flee");

/* Druid Class feats */
feato(FEAT_NATURE_SENSE, "Nature Sense", TRUE, FALSE, FALSE, "1st-level Druid", "A druid gains a +2 bonus on Knowledge and Survival checks");
feato(FEAT_TRACKLESS_STEP, "Trackless Step", TRUE, FALSE, FALSE, "3rd-level Druid", "cannot be tracked");
feato(FEAT_WHOLENESS_OF_BODY, "Wholeness of Body", TRUE, FALSE, FALSE, "7th-level Monk", "can heal class level *2 hp to self");
feato(FEAT_WEAPON_PROFICIENCY_DRUID, "Weapon Proficiency (Druid)", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE, "Wild Shape", TRUE, FALSE, FALSE, "4th-level Druid", "ask staff");
feato(FEAT_WILD_SHAPE_ELEMENTAL, "Wild Shape (Elemental)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE_HUGE, "Wild Shape (Huge)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE_HUGE_ELEMENTAL, "Wild Shape (Huge Elemental)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE_LARGE, "Wild Shape (Large)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE_PLANT, "Wild Shape (Plant)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_WILD_SHAPE_TINY, "Wild Shape (Tiny)", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_THOUSAND_FACES, "A Thousand Faces", TRUE, FALSE, FALSE, "13th-level Druid", "ask staff");
feato(FEAT_TIMELESS_BODY, "Timeless Body", TRUE, FALSE, FALSE, "15th-level Druid, 16th-level Monk", "immune to negative agining effects");
feato(FEAT_RESIST_NATURES_LURE, "Resist Nature's Lure", TRUE, FALSE, FALSE, "4th-level Druid", "+4 to spells and spell like abilities from fey creatures");

/* Fighter Class feats */
feato(FEAT_BRAVERY, "Bravery", TRUE, FALSE, FALSE, "2nd-level Fighter", "A fighter gains a +1 bonus on Will saves against fear. This bonus increases by +1 for every four levels beyond 2nd");
feato(FEAT_WEAPON_SPECIALIZATION, "Weapon Specialization", TRUE, TRUE, TRUE, "Weapon Focus, 4th-level fighter", "+2 bonus on damage rolls with one weapon");

/* Monk Class feats */
feato(FEAT_ABUNDANT_STEP, "Abundant Step", TRUE, FALSE, FALSE, "12th-level Monk", "ask staff");
feato(FEAT_FLURRY_OF_BLOWS, "Flurry of Blows", TRUE, FALSE, FALSE, "1st-level Monk", "extra attack when fighting unarmed at -2 to all attacks");
feato(FEAT_WEAPON_PROFICIENCY_MONK, "Weapon Proficiency (Monk)", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_STILL_MIND, "Still Mind", TRUE, FALSE, FALSE, "3rd-level Monk", "A Monk gains a +2 bonus vs. enchantment spells and effects");
feato(FEAT_VENOM_IMMUNITY, "Venom Immunity", TRUE, FALSE, FALSE, "9th-level Druid", "A druid gains immunity to all poisons.");
feato(FEAT_TONGUE_OF_THE_SUN_AND_MOON, "Tongue of the Sun and Moon", TRUE, FALSE, FALSE, "17th-level Monk", "A monk of 17th level or higher can speak with any living creature");
feato(FEAT_PURITY_OF_BODY, "Purity of Body", TRUE, FALSE, FALSE, "5th-level Monk", "A monk gains immunity to all diseases, including supernatural and magical diseases.");
feato(FEAT_QUIVERING_PALM, "Quivering Palm", TRUE, FALSE, FALSE, "15th-level Monk", "chance to kill on strike with unarmed attack");
feato(FEAT_PERFECT_SELF, "Perfect Self", TRUE, FALSE, FALSE, "20th-level Monk", "10/magic damage reduction");
feato(FEAT_DIAMOND_BODY, "Diamond Body", TRUE, FALSE, FALSE, "11th-level Monk", "A monk gains immunity to poisons of all kinds");
feato(FEAT_DIAMOND_SOUL, "Diamond Soul", TRUE, FALSE, FALSE, "13th-level Monk", "spell resistance equal to class level + 10");
feato(FEAT_KI_STRIKE, "Ki Strike", TRUE, FALSE, FALSE, "4th-level Monk", "unarmed attack considered a magical weapon");
feato(FEAT_EMPTY_BODY, "Empty Body", TRUE, FALSE, FALSE, "19th-level Monk", "50%% concealment for several rounds");
feato(FEAT_GREATER_FLURRY, "Greater Flurry", TRUE, FALSE, FALSE, "11th-level Monk", "extra unarmed attack when using flurry of blows at -5 penalty");
feato(FEAT_SLOW_FALL, "Slow Fall", TRUE, FALSE, FALSE, "-", "no damage for falling 10 ft/feat rank");

/* Paladin Class feats */
feato(FEAT_DETECT_EVIL, "Detect Evil", TRUE, TRUE, FALSE, "1st-level Paladin", "able to detect evil alignments");
feato(FEAT_SMITE_EVIL, "Smite Evil", TRUE, FALSE, FALSE, "5th-level Paladin", "add level to hit roll and charisma bonus to damage");
feato(FEAT_REMOVE_DISEASE, "Remove Disease", TRUE, TRUE, FALSE, "6th-level Paladin", "can cure diseases");
feato(FEAT_AURA_OF_COURAGE, "Aura of Courage", TRUE, TRUE, FALSE, "3rd-level Paladin", "+2 bonus to fear saves for group members");
feato(FEAT_CALL_MOUNT, "Call Mount", TRUE, FALSE, FALSE, "5th-level Paladin", "Allows you to call a paladin mount");
feato(FEAT_DIVINE_BOND, "Divine Bond", TRUE, FALSE, FALSE, "5th-level Paladin", "bonuses to attack and damage rolls when active");
feato(FEAT_DIVINE_GRACE, "Divine Grace", TRUE, TRUE, FALSE, "2nd-level Paladin", "charisma bonus added to all saving throw checks");
feato(FEAT_DIVINE_HEALTH, "Divine Health", TRUE, TRUE, FALSE, "3rd-level Paladin", "immune to disease");
feato(FEAT_LAYHANDS, "Lay on Hands", TRUE, TRUE, FALSE, "2nd-level Paladin", "heals (class level) * (charisma bonus) hit points once per day");

/* Ranger Class feats */
feato(FEAT_WILD_EMPATHY, "Wild Empathy", TRUE, FALSE, FALSE, "2nd-level Ranger", "ask staff");
feato(FEAT_SWIFT_TRACKER, "Swift Tracker", TRUE, FALSE, FALSE, "8th-level Ranger", "ask staff");
feato(FEAT_FAVORED_ENEMY, "Favored Enemy", TRUE, TRUE, TRUE, "1st-level Ranger", "ask staff");
feato(FEAT_FAVORED_ENEMY_AVAILABLE, "Available Favored Enemy Choice(s)", TRUE, FALSE, FALSE, "1st-level Ranger", "ask staff");
feato(FEAT_MANYSHOT, "Manyshot", TRUE, FALSE, FALSE, "6th-level Ranger", "extra ranged attack when rapid shot turned on");
feato(FEAT_IMPROVED_PRECISE_SHOT, "Improved Precise Shot", TRUE, FALSE, FALSE, "11th-level Ranger", "+1 to hit on all ranged attacks");
feato(FEAT_CAMOUFLAGE, "Camouflage", TRUE, FALSE, FALSE, "13th-level Ranger", "ask staff");
feato(FEAT_ANIMAL_COMPANION, "Animal Companion", TRUE, FALSE, FALSE, "4th-level Ranger", "ask staff");

/* Rogue Class feats */
feato(FEAT_SLIPPERY_MIND, "Slippery Mind", TRUE, TRUE, FALSE, "11th-level Rogue", "extra chance for will saves");
feato(FEAT_TRAPFINDING, "Trapfinding", TRUE, FALSE, FALSE, "1st-level Rogue", "A rogue adds 1/2 her level to Disable Device skill checks (minimum +1).");
feato(FEAT_WEAPON_PROFICIENCY_ROGUE, "Weapon Proficiency (Rogue)", FALSE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_OPPORTUNIST, "Opportunist", TRUE, TRUE, FALSE, "10th-level Rogue", "once per round the rogue may make an attack of opportunity against a foe an ally just struck");
feato(FEAT_CRIPPLING_STRIKE, "Crippling Strike", TRUE, TRUE, FALSE, "10th-level Rogue", "Chance to do 2 strength damage with a sneak attack.");
feato(FEAT_IMPROVED_EVASION, "Improved Evasion", TRUE, TRUE, FALSE, "11th-level Rogue", "as evasion but half damage of failed save");
feato(FEAT_DEFENSIVE_ROLL, "Defensive Roll", TRUE, TRUE, FALSE, "10th-level Rogue", "can roll reflex save vs damage dealt when hp is to be reduced below 0 to take half damage instead");

/* Rogue Talents*/
feato(FEAT_BLEEDING_ATTACK, "Bleeding Attack", TRUE, TRUE, FALSE, "Rogue Talent", "causes bleed damage on living targets who are hit by sneak attack.");
feato(FEAT_POWERFUL_SNEAK, "Powerful Sneak", TRUE, TRUE, FALSE, "Rogue Talent", "opt to take -2 to attacks and treat all sneak attack dice rolls of 1 as a 2");

/* Sorcerer Class feats */
feato(FEAT_BLOODLINE_ARCANE, "Bloodline (Arcane)", TRUE, TRUE, FALSE, "1st-level Sorcerer", "ask staff");
feato(FEAT_BLOODLINE_ABYSSAL, "Bloodline (Abyssal)", TRUE, TRUE, FALSE, "1st-level Sorcerer", "You gain claws which become more powerful as you grow in power.");
feato(FEAT_BLOODLINE_CELESTIAL, "Bloodline (Celestial)", TRUE, TRUE, FALSE, "1st-level Sorcerer", "You can unleash heavenly fire.");

/* Wizard Class feats */
feato(FEAT_WEAPON_PROFICIENCY_WIZARD, "Weapon Proficiency (Wizard)", FALSE, FALSE, FALSE, "1st-level Wizard", "ask staff");

/* Druid/Ranger Class feats */
feato(FEAT_WOODLAND_STRIDE, "Woodland Stride", TRUE, FALSE, FALSE, "2nd-level Druid, 7th-level Ranger", "A druid or ranger may move through any sort of undergrowth at her normal speed and without taking damage or suffering any other impairment.");

/* Barbarian/Rogue Class feats */
feato(FEAT_TRAP_SENSE, "Trap Sense", TRUE, FALSE, FALSE, "ask staff", "ask staff");

/* Monk/Rogue Class feats */
feato(FEAT_EVASION, "Evasion", TRUE, FALSE, FALSE, "2nd-level Monk, 2nd-level Rogue", "on successful reflex save no damage from spells and effects");

/* Assassin/Ranger Class feats */
feato(FEAT_HIDE_IN_PLAIN_SIGHT, "Hide in Plain Sight", TRUE, FALSE, FALSE, "17th-level Ranger, 8th-level Assassin", "ask staff");


/* PRESTIGE CLASSES */
/* Arcane Archer Class feats */
feato(FEAT_ENHANCE_ARROW_ALIGNED, "Enhance Arrow (Aligned)", TRUE, FALSE, FALSE, "10th-level Arcane Archer", "+1d6 holy/unholy damage with bows against different aligned creatures.");
feato(FEAT_ENHANCE_ARROW_DISTANCE, "Enhance Arrow (Distance)", TRUE, FALSE, FALSE, "6th-level Arcane Archer", "doubles range increment on weapon.");
feato(FEAT_ENHANCE_ARROW_ELEMENTAL, "Enhance Arrow (Elemental)", TRUE, FALSE, FALSE, "4th-level Arcane Archer", "+1d6 elemental damage with bows");
feato(FEAT_ENHANCE_ARROW_ELEMENTAL_BURST, "Enhance Arrow (Elemental Burst)", TRUE, FALSE, FALSE, "8th-level Arcane Archer", "+2d10 on critical hits with bows");
feato(FEAT_ENHANCE_ARROW_MAGIC, "Enhance Arrow (Magic)", TRUE, FALSE, FALSE, "1st-level Arcane Archer", "+1 to hit and damage with bows per rank");

/* Assassin Class feats */
feato(FEAT_DEATH_ATTACK, "Death Attack", TRUE, FALSE, FALSE, "1st-level Assassin", "Chance to kill a target with sneak attack or Paralysis after 3 rounds of hidden study.");
feato(FEAT_POISON_SAVE_BONUS,  "Poison Save Bonus", TRUE, FALSE, FALSE, "2nd-level Assassin", "Bonus to all saves against poison.");
feato(FEAT_POISON_USE, "Poison Use", TRUE, FALSE, FALSE, "1st-level Assassin", "Trained use in poisons without risk of poisoning self.");

/* Dragon Disciple Class feats */
feato(FEAT_SLEEP_PARALYSIS_IMMUNITY, "Sleep & Paralysis Immunity", TRUE, FALSE, FALSE, "10th-level Dragon Disciple", "ask staff");
feato(FEAT_WINGS, "Wings", TRUE, FALSE, FALSE, "9th-level Dragon Disciple", "ask staff");
feato(FEAT_BLINDSENSE, "Blindsense", TRUE, FALSE, FALSE, "5th-level Dragon Disciple", "ask staff");
feato(FEAT_BREATH_WEAPON, "Breath Weapon", TRUE, FALSE, FALSE, "3rd-level Dragon Disciple", "ask staff");
feato(FEAT_CHARISMA_BOOST, "Charisma Boost", TRUE, FALSE, FALSE, "10th-level Dragon Disciple", "ask staff");
feato(FEAT_CLAWS_AND_BITE, "Claws and Bite", TRUE, FALSE, FALSE, "2nd-level Dragon Disciple", "ask staff");
feato(FEAT_CONSTITUTION_BOOST, "Constitution Boost", TRUE, FALSE, FALSE, "6th-level Dragon Disciple", "ask staff");
feato(FEAT_DRAGON_APOTHEOSIS, "Dragon Apotheosis", TRUE, FALSE, FALSE, "10th-level Dragon Disciple", "ask staff");
feato(FEAT_ELEMENTAL_IMMUNITY, "Elemental Immunity", TRUE, FALSE, FALSE, "10th-level Dragon Disciple", "ask staff");
feato(FEAT_INTELLIGENCE_BOOST, "Intelligence Boost", TRUE, FALSE, FALSE, "8th-level Dragon Disciple", "ask staff");

/* Death Master Class feats */
feato(FEAT_SUMMON_GREATER_UNDEAD, "Summon Greater Undead", TRUE, FALSE, FALSE, "8th-level Death Master", "allows innate use of summon greater undead spell 3x per day");
feato(FEAT_SUMMON_UNDEAD, "Summon Undead", TRUE, FALSE, FALSE, "4th-level Death Master", "allows innate use of summon undead spell 3x per day");
feato(FEAT_TOUCH_OF_UNDEATH, "Touch of Undeath", TRUE, FALSE, FALSE, "9th-level Death Master", "allows for paralytic or instant death touch");
feato(FEAT_UNDEAD_FAMILIAR, "Undead Familiar", TRUE, FALSE, FALSE, "3rd-level Death Master", "allows for undead familiars");
feato(FEAT_ESSENCE_OF_UNDEATH, "Essence of Undeath", TRUE, FALSE, FALSE, "10th-level Death Master", "gives immunity to poison, disease, sneak attack and critical hits");
feato(FEAT_ANIMATE_DEAD, "Animate Dead", TRUE, FALSE, FALSE, "2nd-level Death Master", "allows innate use of animate dead spell 3x per day.");
feato(FEAT_BONE_ARMOR, "Bone Armor", TRUE, FALSE, FALSE, "1st-level Death Master", "allows creation of bone armor and 10%% arcane spell failure reduction in bone armor per rank.");

/* Duelist Class feats */
feato(FEAT_RIPOSTE, "Riposte", TRUE, FALSE, FALSE, "5th-level Duelist", "allows you to gain an attack of opportunity after a successful parry");
feato(FEAT_PARRY, "Parry", TRUE, FALSE, FALSE, "2nd-level Duelist", "allows you to parry incoming attacks");
feato(FEAT_NO_RETREAT, "No Retreat", TRUE, FALSE, FALSE, "9th-level Duelist", "allows you to gain an attack of opportunity against retreating opponents");
feato(FEAT_ACROBATIC_CHARGE, "Acrobatic Charge", TRUE, FALSE, FALSE, "6th-level Duelists", "can charge in situations when others cannot");
feato(FEAT_CANNY_DEFENSE, "Canny Defense", TRUE, FALSE, FALSE, "1st-level Duelist", "add int bonus (max class level) to ac when useing one light weapon and no shield");
feato(FEAT_CRIPPLING_CRITICAL, "Crippling Critical", TRUE, FALSE, FALSE, "10th-level Duelist", "allows your criticals to have random additional effects");
feato(FEAT_ELABORATE_PARRY, "Elaborate Parry", TRUE, FALSE, FALSE, "7th-level Duelist", "when fighting defensively or total defense, gains +1 dodge ac per class level");
feato(FEAT_ENHANCED_MOBILITY, "Enhanced Mobility", TRUE, FALSE, FALSE, "3rd-level Duelist", "ask staff");
feato(FEAT_GRACE, "Grace", TRUE, FALSE, FALSE, "4th-level Duelist", "ask staff");
feato(FEAT_IMPROVED_REACTION, "Improved Reaction", TRUE, FALSE, FALSE, "2nd-level Duelist", "+2 bonus to initiative checks (+4 at 8th class level)");

/* Dwarven Defender Class feats */
feato(FEAT_DEFENSIVE_STANCE, "Defensive Stance", TRUE, FALSE, FALSE, "1st-level Dwarven Defender", "Allows you to fight defensively with bonuses to ac and stats.");
feato(FEAT_MOBILE_DEFENSE, "Mobile Defense", TRUE, FALSE, FALSE, "8th-level Dwarven Defender", "Allows one to move while in defensive stance");


/* Weapon Master Class feats */
feato(FEAT_KI_CRITICAL, "Ki Critical", TRUE, FALSE, FALSE, "7th-level Weapon Master", "Weapons of choice have +1 to threat range per rank");
feato(FEAT_KI_DAMAGE, "Ki Damage", TRUE, FALSE, FALSE, "1st-level Weapon Master", "Weapons of Choice have 5 percent chance to deal max damage");
feato(FEAT_SUPERIOR_WEAPON_FOCUS, "Superior Weapon Focus", TRUE, FALSE, FALSE, "5th-level Weapon Master", "Weapons of choice have +1 to hit");
feato(FEAT_WEAPON_OF_CHOICE, "Weapons of Choice", TRUE, FALSE, FALSE, "1st-level Weapon Master", "All weapons with weapon focus gain special abilities");
feato(FEAT_INCREASED_MULTIPLIER, "Increased Multiplier", TRUE, FALSE, FALSE, "3rd-level Weapon Master", "Weapons of choice have +1 to their critical multiplier");

/* Sacred Fist Class feats */
feato(FEAT_SACRED_FLAMES, "Sacred Flames", TRUE, FALSE, FALSE, "sacred fist level 5", "allows you to use \"innate 'flame weapon'\" 3 times per 10 minutes");

/* UNUSED FEATS */
/* These are EPIC feats which are not used.*/
feato(FEAT_ARMOR_SKIN, "Armor Skin", TRUE, TRUE, TRUE, "Epic level", "Increases natural armor by 1");
feato(FEAT_INTENSIFY_SPELL, "Intensify Spell", TRUE, TRUE, FALSE, "Epic level, empower spell, maximize spell, spellcraft 30 ranks, ability ro cast lvl 9 arcane or divine spells", "maximizes damage/healing and then doubles it.");
feato(FEAT_AUTOMATIC_QUICKEN_SPELL, "Automatic Quicken Spell", TRUE, TRUE, TRUE, "Epic level, spellcraft 30 ranks, ability to cast level 9 arcane or divine spells", "You can cast level 0, 1, 2 & 3 spells automatically as if quickened.  Every addition rank increases the max spell level by 3.");
feato(FEAT_FAST_HEALING, "Fast Healing", TRUE, TRUE, TRUE, "Epic Level", "Heals 3 hp per rank each combat round if fighting otherwise every 6 seconds");
feato(FEAT_GREAT_SMITING, "Great Smiting", TRUE, TRUE, TRUE, "Epic Level", "For each rank in this feat you add your level in damage to all smite attacks");
feato(FEAT_ENHANCE_SPELL, "Increase Spell Damage (Enhance Spell)", TRUE, TRUE, FALSE, "Epic Level", "increase max number of damage dice for certain damage based spell by 5");
feato(FEAT_EPIC_COMBAT_CHALLENGE, "Epic Combat Challenge", TRUE, TRUE, FALSE, "Epic level, 20 ranks in diplomacy, intimidate or bluff, greater combat challenge", "as improved combat challenge, but both regular challenges and challenge all are minor actions");
feato(FEAT_EPIC_DODGE, "Epic Dodge", TRUE, TRUE, FALSE, "Epic level, dex 25, dodge, tumble 30, improved evasion, defensive roll", "automatically dodge first attack against you each round");
feato(FEAT_EPIC_PROWESS, "Epic Prowess", TRUE, TRUE, TRUE, "Epic level", "+1 to all attacks per rank");
feato(FEAT_EPIC_SKILL_FOCUS, "Epic Skill focus", TRUE, TRUE, TRUE, "Epic level, 20 ranks in the skill", "+10 in chosen skill");
feato(FEAT_EPIC_SPELLCASTING, "Epic Spellcasting", TRUE, TRUE, FALSE, "Epic level, lore 24, spellcraft 24", "allows you to cast epic spells");
feato(FEAT_EPIC_TOUGHNESS, "Epic Toughness", TRUE, TRUE, TRUE, "Epic level", "You gain +30 max hp.");
feato(FEAT_MIGHTY_RAGE, "Mighty Rage", TRUE, FALSE, FALSE, "Epic level, str 21, con 21, greater rage, rage 5/day", "+8 str and con and +4 to will saves when raging");
feato(FEAT_PERFECT_TWO_WEAPON_FIGHTING, "Perfect Two Weapon Fighting", TRUE, TRUE, FALSE, "Epic level, dex 25, greater two weapon fighting", "Extra attack with offhand weapon");
feato(FEAT_SELF_CONCEALMENT, "Self Concealment", TRUE, TRUE, TRUE, "Epic level, stealth 30 ranks, dex 30, tumble 30 ranks", "10%% miss chance for attacks against you per rank");
feato(FEAT_SWARM_OF_ARROWS, "Swarm of Arrows", TRUE, TRUE, FALSE, "Epic level, dex 23, point blank shot, rapid shot, weapon focus", "allows you to make a single ranged attack against everyone in range.");

/* Unused non-Epic Feats*/
feato(FEAT_ARMOR_SPECIALIZATION_HEAVY, "Armor Specialization (heavy)", TRUE, TRUE, FALSE, "Armor Proficiency (Heavy), Base attack bonus +12", "DR 2/- when wearing heavy armor");
feato(FEAT_ARMOR_SPECIALIZATION_LIGHT, "Armor Specialization (light)", TRUE, TRUE, FALSE, "Armor Proficiency (Light), Base attack bonus +12", "DR 2/- when wearing light armor");
feato(FEAT_ARMOR_SPECIALIZATION_MEDIUM, "Armor Specialization (medium)", TRUE, TRUE, FALSE, "Armor Proficiency (Medium), Base attack bonus +12", "DR 2/- when wearing medium armor");
feato(FEAT_ROBILARS_GAMBIT, "Robilars Gambit", FALSE, FALSE, FALSE, "combat reflexes, base attack bonus +12", "when active enemies gain +4 to hit and damage against you, but all melee attacks invoke an attack of opportunity from you.");
feato(FEAT_COMBAT_CHALLENGE, "Combat Challenge", TRUE, TRUE, FALSE, "Diplomacy, Intimidate or Bluff 5 ranks", "allows you to make a mob focus their attention on you");
feato(FEAT_DETECT_GOOD, "Detect Good", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_SMITE_GOOD, "Smite Good", TRUE, FALSE, FALSE, "ask staff", "ask staff");
feato(FEAT_GREATER_COMBAT_CHALLENGE, "Greater Combat Challenge", TRUE, TRUE, FALSE, "15 ranks in diplomacy, intimidate or bluff, improved combat challenge", "as improved combat challenge, but regular challenge is a minor action & challenge all is a move action ");
feato(FEAT_HASTE, "Haste", TRUE, FALSE, FALSE, "favored soul level 17", "can cast haste 3x per day");

/* Dragonlance specific feats or class abilities. */
if (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE)
{
  feato(FEAT_ARMORED_MOBILITY, "Armored Mobility", TRUE, FALSE, FALSE, "Knight of the Crown level 6, Knight of the Lily level 6", "heavy armor is treated as medium armor");
  feato(FEAT_ARMORED_SPELLCASTING, "Armored Spellcasting", TRUE, FALSE, FALSE, "Knight of the Thorn", "ask staff");
  feato(FEAT_AURA_OF_EVIL, "Aura of Evil", TRUE, FALSE, FALSE, "Knight of the Skull", "ask staff");
  feato(FEAT_AURA_OF_GOOD, "Aura of Good", TRUE, TRUE, FALSE, "Knight of the Rose", "+10 ac to all group members");
  feato(FEAT_AURA_OF_TERROR, "Aura of Terror", TRUE, FALSE, FALSE, "Knight of the Thorn", "ask staff");
  feato(FEAT_COSMIC_UNDERSTANDING, "Cosmic Understanding", TRUE, FALSE, FALSE, "Knight of the Thorn level 10", "ask staff");
  feato(FEAT_CROWN_OF_KNIGHTHOOD, "Crown of Knighthood", TRUE, FALSE, FALSE, "Knight of the Crown 10th", "ask staff");
  feato(FEAT_DARK_BLESSING, "Dark Blessing", TRUE, FALSE, FALSE, "Knight of the Skull level 2", "ask staff");
  feato(FEAT_DEMORALIZE, "Demoralize", TRUE, FALSE, FALSE, "Knight of the Lily level 2", "ask staff");
  feato(FEAT_DISCERN_LIES, "Discern Lies", TRUE, FALSE, FALSE, "Knight of the Skull level 3", "ask staff");
  feato(FEAT_DIVINER, "Diviner", TRUE, FALSE, FALSE, "Knight of the Thorn", "ask staff");
  feato(FEAT_DRAGON_MOUNT_BOOST, "Dragon Mount Boost", TRUE, FALSE, FALSE, "dragon rider prestige class", "gives +18 hp, +10 ac, +1 hit and +1 damage per rank in the feat");
  feato(FEAT_DRAGON_MOUNT_BREATH, "Dragon Mount Breath", TRUE, FALSE, FALSE, "dragon rider prestige class", "allows you to use your dragon mount's breath weapon once per rank, per 10 minutes.");
  feato(FEAT_FAVOR_OF_DARKNESS, "Favor of Darkness", TRUE, FALSE, FALSE, "Knight of the Skull level 10", "ask staff");
  feato(FEAT_FINAL_STAND, "Final Stand", TRUE, FALSE, FALSE, "Knight of the Rose level 9", "ask staff");
  feato(FEAT_HEROIC_INITIATIVE, "Heroic Initiative", TRUE, FALSE, FALSE, "Knight of the Crown 1st", "bonus to initiative checks");
  feato(FEAT_HONORABLE_WILL, "Honorable Will", TRUE, FALSE, FALSE, "Knight of the Crown 4th", "ask staff");
  feato(FEAT_HONORBOUND, "Honorbound", TRUE, TRUE, FALSE, "-", "+2 to saving throws against fear or compulsion effects, +2 to sense motive checks");
  feato(FEAT_KNIGHTHOODS_FLOWER, "Knighthood's Flower", TRUE, FALSE, FALSE, "Knight of the Rose 10th", "ask staff");
  feato(FEAT_KNIGHTLY_COURAGE, "Knightly Courage", TRUE, FALSE, FALSE, "Knight of the Crown 1st", "bonus to fear checks");
  feato(FEAT_MIGHT_OF_HONOR, "Might of Honor", TRUE, FALSE, FALSE, "Knight of the Crown level 6", "ask staff");
  feato(FEAT_ONE_THOUGHT, "One Thought", TRUE, FALSE, FALSE, "Knight of the Lily level 10", "ask staff");
  feato(FEAT_RALLYING_CRY, "Rallying Cry", TRUE, FALSE, FALSE, "Knight of the Rose level 1", "ask staff");
  feato(FEAT_READ_OMENS, "Read Omens", TRUE, FALSE, FALSE, "Knight of the Thorn level 1", "ask staff");
  feato(FEAT_READ_PORTENTS, "Read Portents", TRUE, FALSE, FALSE, "Knight of the Thorn level 6", "ask staff");
  feato(FEAT_SOUL_OF_KNIGHTHOOD, "Soul of Knighthood", TRUE, FALSE, FALSE, "Knight of the Sword level 10", "ask staff");
  feato(FEAT_STRENGTH_OF_HONOR, "Strength of Honor", TRUE, FALSE, TRUE, "Knight of the Crown level 1", "+4 to strength for several rounds");
  feato(FEAT_UNBREAKABLE_WILL, "Unbreakable Will", TRUE, FALSE, FALSE, "Knight of the Lily", "ask staff");
  feato(FEAT_WEAPON_TOUCH, "Weapon Touch", TRUE, FALSE, FALSE, "Knight of the Thorn level 4", "ask staff");
  feato(FEAT_WISDOM_OF_THE_MEASURE, "Wisdom of the Measure", TRUE, FALSE, FALSE, "Knight of the Rose level 6", "ask staff");
}

feato(FEAT_LAST_FEAT, "do not take me", FALSE, FALSE, FALSE, "placeholder feat", "placeholder feat");

combatfeat(FEAT_IMPROVED_CRITICAL);
combatfeat(FEAT_WEAPON_FINESSE);
combatfeat(FEAT_WEAPON_FOCUS);
combatfeat(FEAT_WEAPON_SPECIALIZATION);
combatfeat(FEAT_GREATER_WEAPON_FOCUS);
combatfeat(FEAT_GREATER_WEAPON_SPECIALIZATION);
combatfeat(FEAT_IMPROVED_WEAPON_FINESSE);
combatfeat(FEAT_MONKEY_GRIP);
combatfeat(FEAT_CRITICAL_FOCUS);
combatfeat(FEAT_WEAPON_MASTERY);
combatfeat(FEAT_WEAPON_FLURRY);
combatfeat(FEAT_WEAPON_SUPREMACY);

epicfeat(FEAT_LAST_FEAT);
}


// The follwing function is used to check if the character satisfies the various prerequisite(s) (if any)
// of a feat in order to learn it.

int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg)
{
  if (featnum > NUM_FEATS_DEFINED)
    return FALSE;

  if (feat_list[featnum].epic == TRUE && !IS_EPIC(ch))
    return FALSE;

  if (has_feat(ch, featnum) && !feat_list[featnum].can_stack)
    return FALSE;

  switch (featnum) {

  case FEAT_AUTOMATIC_QUICKEN_SPELL:
    if (GET_SKILL_RANKS(ch, SKILL_SPELLCRAFT) < 30)
      return FALSE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    return FALSE;

  case FEAT_EPIC_SPELLCASTING:
    if (GET_SKILL_RANKS(ch, SKILL_SPELLCRAFT) < 24)
      return FALSE;
    if (GET_SKILL_RANKS(ch, SKILL_KNOWLEDGE) < 24)
      return FALSE;
    if (GET_LEVEL(ch) < 21)
      return FALSE;
    return TRUE;

  case FEAT_DIEHARD:
    if (!HAS_REAL_FEAT(ch, FEAT_ENDURANCE))
      return FALSE;
    return TRUE;

  case FEAT_FAST_HEALER:
    if (!HAS_REAL_FEAT(ch, FEAT_DIEHARD))
      return FALSE;
    if (ch->real_abils.con < 13)
      return FALSE;
    return TRUE;

  case FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD:
  if (!HAS_REAL_FEAT(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
    return FALSE;
  return TRUE;

  case FEAT_INTENSIFY_SPELL:
    if (GET_SKILL_RANKS(ch, SKILL_SPELLCRAFT) < 30)
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_MAXIMIZE_SPELL))
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_EMPOWER_SPELL))
      return FALSE;
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    if (findslotnum(ch, 9) > 0)
      return TRUE;
    return FALSE;

  case FEAT_SWARM_OF_ARROWS:
    if (ch->real_abils.dex < 23)
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_POINT_BLANK_SHOT))
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_RAPID_SHOT))
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_WEAPON_FOCUS))
      return FALSE;
    return TRUE;

  case FEAT_RAPID_SHOT:
    if (ch->real_abils.dex <13)
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_POINT_BLANK_SHOT))
      return FALSE;
    return TRUE;
 
  case FEAT_DEFLECT_ARROWS:
    if (ch->real_abils.dex < 13)
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_UNARMED_STRIKE))
      return FALSE;
    return TRUE;

  case FEAT_FAST_HEALING:
    if (ch->fast_healing_feats >= 5)
      return FALSE;
    if (ch->real_abils.con < 25)
      return FALSE;
    return TRUE;    

  case FEAT_SELF_CONCEALMENT:
    if (HAS_REAL_FEAT(ch, FEAT_SELF_CONCEALMENT) >= 5)
      return FALSE;
    if (GET_SKILL_RANKS(ch, SKILL_STEALTH) < 30)
      return FALSE;
    if (GET_SKILL_RANKS(ch, SKILL_ACROBATICS) < 30)
      return FALSE;
    if (ch->real_abils.dex < 30)
      return FALSE;
    return TRUE;

  case FEAT_TRAMPLE:
    if (!HAS_REAL_FEAT(ch, FEAT_MOUNTED_COMBAT))
      return FALSE;
    return TRUE;

  case FEAT_ARMOR_SKIN:
    if (ch->armor_skin_feats >= 5)
      return FALSE;
    if (GET_LEVEL(ch) < 21)
      return FALSE;
    return TRUE;

  case FEAT_MOUNTED_COMBAT:
    if (GET_SKILL_RANKS(ch, SKILL_RIDE) < 1)
      return FALSE;
    return TRUE;

  case FEAT_COMBAT_CHALLENGE:
    if (GET_SKILL_RANKS(ch, SKILL_DIPLOMACY) < 5 &&
        GET_SKILL_RANKS(ch, SKILL_INTIMIDATE) < 5 &&
        GET_SKILL_RANKS(ch, SKILL_BLUFF) < 5)
      return false;
    return true;

  case FEAT_BLEEDING_ATTACK:
  case FEAT_POWERFUL_SNEAK:
    if (GET_CLASS_RANKS(ch, CLASS_ROGUE) > 2)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_COMBAT_CHALLENGE:
    if (GET_SKILL_RANKS(ch, SKILL_DIPLOMACY) < 10 &&
        GET_SKILL_RANKS(ch, SKILL_INTIMIDATE) < 10 &&
        GET_SKILL_RANKS(ch, SKILL_BLUFF) < 10)
      return false;
    if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_CHALLENGE))
      return false;
    return true;

  case FEAT_GREATER_COMBAT_CHALLENGE:
    if (GET_SKILL_RANKS(ch, SKILL_DIPLOMACY) < 15 &&
        GET_SKILL_RANKS(ch, SKILL_INTIMIDATE) < 15 &&
        GET_SKILL_RANKS(ch, SKILL_BLUFF) < 15)
      return false;
    if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_COMBAT_CHALLENGE))
      return false;
    return true;

  case FEAT_EPIC_COMBAT_CHALLENGE:
    if (GET_SKILL_RANKS(ch, SKILL_DIPLOMACY) < 20 &&
        GET_SKILL_RANKS(ch, SKILL_INTIMIDATE) < 20 &&
        GET_SKILL_RANKS(ch, SKILL_BLUFF) < 20)
      return false;
    if (!HAS_REAL_FEAT(ch, FEAT_GREATER_COMBAT_CHALLENGE))
      return false;
    return true;

  case FEAT_NATURAL_SPELL:
      if (ch->real_abils.wis < 13)
          return false;
      if (!has_feat(ch, FEAT_WILD_SHAPE))
          return false;
      return true;

  case FEAT_EPIC_DODGE:
    if (ch->real_abils.dex >= 25 && has_feat(ch, FEAT_DODGE) && has_feat(ch, FEAT_DEFENSIVE_ROLL) && GET_SKILL(ch, SKILL_ACROBATICS) >= 30)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_SNEAK_ATTACK:
    if (has_feat(ch, FEAT_SNEAK_ATTACK) >= 8)
      return TRUE;
    return FALSE;

  case FEAT_SNEAK_ATTACK:
    if (HAS_REAL_FEAT(ch, FEAT_SNEAK_ATTACK) < 8)
      return FALSE;
    return TRUE;

  case FEAT_SNEAK_ATTACK_OF_OPPORTUNITY:
    if (HAS_REAL_FEAT(ch, FEAT_SNEAK_ATTACK) < 8)
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_OPPORTUNIST))
      return FALSE;
    return TRUE;

  case FEAT_STEADFAST_DETERMINATION:
    if (!HAS_REAL_FEAT(ch, FEAT_ENDURANCE))
      return FALSE;
    return TRUE;

  case FEAT_GREAT_SMITING:
    if (ch->real_abils.cha >= 25 && has_feat(ch, FEAT_SMITE_EVIL))
      return TRUE;
    return FALSE;

  case FEAT_DIVINE_MIGHT:
  case FEAT_DIVINE_SHIELD:
    if (has_feat(ch, FEAT_TURN_UNDEAD) && has_feat(ch, FEAT_POWER_ATTACK) &&
        ch->real_abils.cha >= 13 && ch->real_abils.str >= 13)
      return TRUE;
    return FALSE;

  case FEAT_PRECISE_SHOT:
    if (!HAS_REAL_FEAT(ch, FEAT_POINT_BLANK_SHOT))
      return FALSE;
    return TRUE;

  case FEAT_DIVINE_VENGEANCE:
    if (has_feat(ch, FEAT_TURN_UNDEAD) && has_feat(ch, FEAT_EXTRA_TURNING))
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_EVASION:
  case FEAT_CRIPPLING_STRIKE:
  case FEAT_DEFENSIVE_ROLL:
  case FEAT_OPPORTUNIST:
    if (GET_CLASS_RANKS(ch, CLASS_ROGUE) < 10)
      return FALSE;
    return TRUE;

  case FEAT_FASTER_MEMORIZATION:
    if (IS_MEM_BASED_CASTER(ch))
      return TRUE;
    return FALSE;

  case FEAT_DAMAGE_REDUCTION:
    if (ch->damage_reduction_feats >= 5)
      return FALSE;
    if (ch->real_abils.con < 21)
      return FALSE;
    return TRUE;

  case FEAT_MONKEY_GRIP:
    if (!iarg)
      return TRUE;
    if (!is_proficient_with_weapon(ch, iarg))
      return FALSE;
    return TRUE;

  case FEAT_LAST_FEAT:
    return FALSE;

  case FEAT_BLOODLINE_ARCANE:
    if (has_feat(ch, FEAT_BLOODLINE_CELESTIAL))
      return FALSE;
    if (has_feat(ch, FEAT_BLOODLINE_ABYSSAL))
      return FALSE;
    if (ch->levelup && GET_CLASS_LEVEL(ch) <= 1)
      return TRUE;
    return FALSE;

  case FEAT_BLOODLINE_ABYSSAL:
    if (has_feat(ch, FEAT_BLOODLINE_CELESTIAL))
      return FALSE;
    if (has_feat(ch, FEAT_BLOODLINE_ARCANE))
      return FALSE;
    if (ch->levelup && GET_CLASS_LEVEL(ch) <= 1)
      return TRUE;
    return FALSE;

   case FEAT_BLOODLINE_CELESTIAL:
    if (has_feat(ch, FEAT_BLOODLINE_ABYSSAL))
      return FALSE;
    if (has_feat(ch, FEAT_BLOODLINE_ARCANE))
      return FALSE;
    if (ch->levelup && GET_CLASS_LEVEL(ch) <= 1)
      return TRUE;
    return FALSE;

 case FEAT_SLIPPERY_MIND:
    if (GET_CLASS_RANKS(ch, CLASS_ROGUE) >= 11)
      return TRUE;
    return FALSE;

  case FEAT_LINGERING_SONG:
  case FEAT_EXTRA_MUSIC:
    if (GET_CLASS_RANKS(ch, CLASS_BARD) > 0)
      return TRUE;
    return FALSE;

  case FEAT_EXTEND_RAGE:
  case FEAT_EXTRA_RAGE:
    if (has_feat(ch, FEAT_RAGE))
      return TRUE;
    return FALSE;

  case FEAT_FAVORED_ENEMY:
    if (has_feat(ch, FEAT_FAVORED_ENEMY_AVAILABLE))
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_NATURAL_WEAPON:
    if (GET_BAB(ch) < 4)
      return FALSE;
    if (GET_RACE(ch) == RACE_MINOTAUR)
      return TRUE;
    if (has_feat(ch, FEAT_CLAWS_AND_BITE))
      return TRUE;
    if (has_feat(ch, FEAT_IMPROVED_UNARMED_STRIKE))
      return TRUE;
    return FALSE;

  case FEAT_TWO_WEAPON_DEFENSE:
  	if (!has_feat(ch, FEAT_TWO_WEAPON_FIGHTING))
  		return FALSE;
  	if (ch->real_abils.dex < 15)
  		return FALSE;
  	return TRUE;

  case FEAT_COMBAT_EXPERTISE:
  	if (ch->real_abils.intel < 13)
  		return false;
    return true;
    
  case FEAT_IMPROVED_FEINT:
  	if (!has_feat(ch, FEAT_COMBAT_EXPERTISE))
  		return false;
  	return true;

  case FEAT_BRAVERY:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER))
      return true;
    return false;

  case FEAT_AURA_OF_GOOD:
  case FEAT_DETECT_EVIL:
  case FEAT_SMITE_EVIL:
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN))
      return true;
    return false;

  case FEAT_DIVINE_GRACE:
  case FEAT_LAYHANDS:
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > 1)
      return true;
    return false;

  case FEAT_AURA_OF_COURAGE:
  case FEAT_DIVINE_HEALTH:
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > 2)
      return true;
    return false;

  case FEAT_TURN_UNDEAD:
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > 3 || GET_CLASS_RANKS(ch, CLASS_CLERIC))
      return true;
    return false;

  case FEAT_REMOVE_DISEASE:
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > 5)
      return true;
    return false;

  case FEAT_ARMOR_PROFICIENCY_HEAVY:
    if (has_feat(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    return FALSE;

  case FEAT_ARMOR_PROFICIENCY_MEDIUM:
    if (has_feat(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    return FALSE;

  case FEAT_DODGE:
    if (ch->real_abils.dex >= 13)
      return TRUE;
    return FALSE;

  case FEAT_MOBILITY:
    if (has_feat(ch, FEAT_DODGE))
      return TRUE;
    return FALSE;

  case FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD:
    if (GET_BAB(ch) >= 1)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_DISARM:
    if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_EXPERTISE))
      return FALSE;
    return TRUE;

  case FEAT_IMPROVED_TRIP:
    if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_EXPERTISE))
      return FALSE;
    return TRUE;

  case FEAT_WHIRLWIND_ATTACK:
    if (!HAS_REAL_FEAT(ch, FEAT_DODGE))
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_MOBILITY))
      return FALSE;
    if (!HAS_REAL_FEAT(ch, FEAT_SPRING_ATTACK))
      return FALSE;
    if (ch->real_abils.intel < 13)
      return FALSE;
    if (ch->real_abils.dex < 13)
      return FALSE;
    if (GET_BAB(ch) < 4)
      return FALSE;
    return TRUE;

  case FEAT_SPRING_ATTACK:
  if (GET_BAB(ch) <4 )
    return FALSE;
  if (!HAS_REAL_FEAT(ch, FEAT_MOBILITY))
    return FALSE;
  return TRUE;

  case FEAT_STUNNING_FIST:
    if (has_feat(ch, FEAT_IMPROVED_UNARMED_STRIKE) && ch->real_abils.str >= 13 && ch->real_abils.dex >= 13 && GET_BAB(ch) >= 8)
      return TRUE;
    if (GET_CLASS_RANKS(ch, CLASS_MONK) > 0)
      return TRUE;
    return FALSE;
  
  case FEAT_POWER_ATTACK:
    if (ch->real_abils.str >= 13 && GET_BAB(ch) >= 1)
      return TRUE;
    return FALSE;

  case FEAT_CLEAVE:
  case FEAT_SUNDER:
    if (has_feat(ch, FEAT_POWER_ATTACK))
      return TRUE;
    return FALSE;

  case FEAT_FAR_SHOT:
    if (has_feat(ch, FEAT_POINT_BLANK_SHOT))
      return TRUE;
    return FALSE;

  case FEAT_TWO_WEAPON_FIGHTING:
    if (ch->real_abils.dex >= 15)
      return TRUE;
    return FALSE;

  case FEAT_LEADERSHIP:
    if (GET_LEVEL(ch) < 7)
      return FALSE;
    return TRUE;

  case FEAT_ENHANCE_SPELL:
  case FEAT_EPIC_TOUGHNESS:
    if (GET_LEVEL(ch) < 21)
      return FALSE;
    return TRUE;

  case FEAT_EPIC_PROWESS:
    if (HAS_REAL_FEAT(ch, FEAT_EPIC_PROWESS) >= 5)
      return FALSE;
    if (GET_LEVEL(ch) < 21)
      return FALSE;
    return TRUE;

  case FEAT_EPIC_SKILL_FOCUS:
    if (GET_LEVEL(ch) < 21)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (GET_SKILL(ch, iarg) >= 20)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_TWO_WEAPON_FIGHTING:
    if (ch->real_abils.dex >= 17 && has_feat(ch, FEAT_TWO_WEAPON_FIGHTING) && GET_BAB(ch) >= 6)
      return TRUE;
    return FALSE;
   
  case FEAT_GREATER_TWO_WEAPON_FIGHTING:
    if (ch->real_abils.dex >= 19 && has_feat(ch, FEAT_TWO_WEAPON_FIGHTING) && has_feat(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING) && GET_BAB(ch) >= 11)
      return TRUE;
    return FALSE;  	

  case FEAT_PERFECT_TWO_WEAPON_FIGHTING:
    if (ch->real_abils.dex >= 25 && has_feat(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_CRITICAL:
    if (GET_BAB(ch) < 8)
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_CRITICAL_FOCUS:
    if (GET_BAB(ch) < 9)
      return FALSE;
    return TRUE;

  case FEAT_WEAPON_MASTERY:
    if (GET_BAB(ch) < 8)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (!is_proficient_with_weapon(ch, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
      return FALSE;
    return TRUE;

  case FEAT_WEAPON_FLURRY:
    if (GET_BAB(ch) < 14)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (!is_proficient_with_weapon(ch, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
      return FALSE;
    return TRUE;

  case FEAT_WEAPON_SUPREMACY:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 17)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (!is_proficient_with_weapon(ch, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION, iarg))
      return FALSE;
    if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
      return FALSE;
    return TRUE;

  case FEAT_ROBILARS_GAMBIT:
    if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_REFLEXES))
      return FALSE;
    if (GET_BAB(ch) < 12)
      return FALSE;
    return TRUE;

  case FEAT_KNOCKDOWN:
    if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_TRIP))
      return FALSE;
    if (GET_BAB(ch) < 4)
      return FALSE;
    return TRUE;

  case FEAT_ARMOR_SPECIALIZATION_LIGHT:
    if (!HAS_REAL_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return FALSE;
    if (GET_BAB(ch) < 12)
      return FALSE;
    return TRUE;

  case FEAT_ARMOR_SPECIALIZATION_MEDIUM:
    if (!HAS_REAL_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return FALSE;
    if (GET_BAB(ch) < 12)
      return FALSE;
    return TRUE;

  case FEAT_ARMOR_SPECIALIZATION_HEAVY:
    if (!HAS_REAL_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
      return FALSE;
    if (GET_BAB(ch) < 12)
      return FALSE;
    return TRUE;

  case FEAT_WEAPON_FOCUS:
    if (GET_BAB(ch) < 1)
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_WEAPON_SPECIALIZATION:
    if (GET_BAB(ch) < 4 || !GET_CLASS_RANKS(ch, CLASS_FIGHTER))
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_GREATER_WEAPON_FOCUS:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 8)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (is_proficient_with_weapon(ch, iarg) && has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
      return TRUE;
    return FALSE;
    
  case  FEAT_IMPROVED_WEAPON_FINESSE:
  	if (!has_feat(ch, FEAT_WEAPON_FINESSE))
  	  return FALSE;
  	if (GET_BAB(ch) < 4)
  		return FALSE;
        if (!iarg)
          return TRUE;
  	if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
  		return FALSE;
        if (weapon_list[iarg].size >= get_size(ch))
          return FALSE;

  	return TRUE;

  case FEAT_GREATER_WEAPON_SPECIALIZATION:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 12)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (is_proficient_with_weapon(ch, iarg) &&
        has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg) &&
        has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg) &&
        has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
      return TRUE;
    return FALSE;

  case FEAT_BREW_POTION:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 3)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_ROD:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 9)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_STAFF:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 12)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_WAND:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_WONDEROUS_ITEM:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 3)
      return TRUE;
    return FALSE;

  case FEAT_FORGE_RING:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 7)
      return TRUE;
    return FALSE;

  case FEAT_SCRIBE_SCROLL:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    if (GET_LEVEL(ch) >= 1)
      return TRUE;
    return FALSE;

  case FEAT_EXTEND_SPELL:
  case FEAT_HEIGHTEN_SPELL:
  case FEAT_MAXIMIZE_SPELL:
  case FEAT_EMPOWERED_MAGIC:
  case FEAT_ENHANCED_SPELL_DAMAGE:
  case FEAT_SPELL_FOCUS:
  case FEAT_SPELL_PENETRATION:
  case FEAT_AUGMENT_SUMMONING:
  case FEAT_QUICKEN_SPELL:
  case FEAT_SILENT_SPELL:
  case FEAT_STILL_SPELL:
  case FEAT_EMPOWER_SPELL:
    if (IS_SPELLCASTER(ch))
      return TRUE;
    return FALSE;

  case FEAT_EXTRA_TURNING:
    if (GET_CLASS_RANKS(ch, CLASS_CLERIC))
      return TRUE;
    return FALSE;

  case FEAT_SPELL_MASTERY:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  default:
    return TRUE;

  }
}

int is_proficient_with_armor(const struct char_data *ch, int armor_type)
{

  int general_type = find_armor_type(armor_type);

  if (armor_type == SPEC_ARMOR_TYPE_CLOTHING)
    return TRUE;

  if (armor_type == SPEC_ARMOR_TYPE_TOWER_SHIELD &&
       !has_feat((char_data *) ch, FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD))
    return FALSE;

  switch (general_type) {
    case ARMOR_TYPE_LIGHT:
      if (has_feat((char_data *) ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
        return TRUE;
    break;
    case ARMOR_TYPE_MEDIUM:
      if (has_feat((char_data *) ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
        return TRUE;
    break;
    case ARMOR_TYPE_HEAVY:
      if (has_feat((char_data *) ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
        return TRUE;
    break;
    case ARMOR_TYPE_SHIELD:
      if (has_feat((char_data *) ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
        return TRUE;
    break;
    default:
      return TRUE;
  }
  return FALSE;
}

int is_proficient_with_weapon(const struct char_data *ch, int weapon_type)
{

  if (has_feat((char_data *) ch, FEAT_WEAPON_PROFICIENCY_DEITY) && weapon_type == deity_list[GET_DEITY(ch)].favored_weapon)
    return TRUE;
  
  if (has_feat((char_data *) ch, FEAT_WEAPON_PROFICIENCY_SIMPLE) &&
      IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_SIMPLE))
    return TRUE;

  if (has_feat((char_data *) ch, FEAT_WEAPON_PROFICIENCY_MARTIAL) &&
      IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_MARTIAL))
    return TRUE;

  if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_PROFICIENCY_EXOTIC, WEAPON_DAMAGE_TYPE_SLASHING) &&
      IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_EXOTIC) &&
      IS_SET(weapon_list[weapon_type].damageTypes, DAMAGE_TYPE_SLASHING)) 
  {
    return TRUE;
  }

  if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_PROFICIENCY_EXOTIC, WEAPON_DAMAGE_TYPE_PIERCING) &&
      IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_EXOTIC) &&
      IS_SET(weapon_list[weapon_type].damageTypes, DAMAGE_TYPE_PIERCING)) 
  {
    return TRUE;
  }

  if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_PROFICIENCY_EXOTIC, WEAPON_DAMAGE_TYPE_BLUDGEONING) &&
      IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_EXOTIC) &&
      IS_SET(weapon_list[weapon_type].damageTypes, DAMAGE_TYPE_BLUDGEONING)) 
  {
    return TRUE;
  }


  if (GET_CLASS_RANKS(ch, CLASS_MONK) && 
      weapon_list[weapon_type].weaponFamily == WEAPON_FAMILY_MONK)
    return TRUE;

  if (has_feat((char_data *) ch, FEAT_WEAPON_PROFICIENCY_DRUID) || GET_CLASS_RANKS(ch, CLASS_DRUID) > 0) {
    switch (weapon_type) {
      case WEAPON_TYPE_CLUB:
      case WEAPON_TYPE_DAGGER:
      case WEAPON_TYPE_QUARTERSTAFF:
      case WEAPON_TYPE_DART:
      case WEAPON_TYPE_SICKLE:
      case WEAPON_TYPE_SCIMITAR:
      case WEAPON_TYPE_SHORTSPEAR:
      case WEAPON_TYPE_SPEAR:
      case WEAPON_TYPE_SLING:
        return TRUE;
    }
  }

  if (GET_CLASS_RANKS(ch, CLASS_BARD) > 0) {
    switch (weapon_type) {
      case WEAPON_TYPE_LONG_SWORD:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_SAP:
      case WEAPON_TYPE_SHORT_SWORD:
      case WEAPON_TYPE_SHORT_BOW:
      case WEAPON_TYPE_WHIP:
        return TRUE;
    }
  }

  if (HAS_FEAT((struct char_data *) ch, FEAT_WEAPON_PROFICIENCY_ROGUE) || GET_CLASS_RANKS(ch, CLASS_ROGUE) > 0) {
    switch (weapon_type) {
      case WEAPON_TYPE_HAND_CROSSBOW:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_SAP:
      case WEAPON_TYPE_SHORT_SWORD:
      case WEAPON_TYPE_SHORT_BOW:
        return TRUE;
    }
  }

  if (HAS_FEAT((struct char_data *)ch, FEAT_WEAPON_PROFICIENCY_WIZARD) || GET_CLASS_RANKS(ch, CLASS_WIZARD) > 0) {
    switch (weapon_type) {
      case WEAPON_TYPE_DAGGER:
      case WEAPON_TYPE_QUARTERSTAFF:
      case WEAPON_TYPE_CLUB:
      case WEAPON_TYPE_HEAVY_CROSSBOW:
      case WEAPON_TYPE_LIGHT_CROSSBOW:
        return TRUE;
    }
  }

  if (HAS_FEAT((struct char_data *)ch, FEAT_WEAPON_PROFICIENCY_ELF) || IS_ELF(ch)) 
  {
    switch (weapon_type) {
      case WEAPON_TYPE_LONG_SWORD:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_LONG_BOW:
      case WEAPON_TYPE_COMPOSITE_LONGBOW:
      case WEAPON_TYPE_SHORT_BOW:
      case WEAPON_TYPE_CURVE_BLADE:
      case WEAPON_TYPE_COMPOSITE_SHORTBOW:
        return TRUE;
    }
  }

  if (IS_HALFLING(ch) && weapon_type == WEAPON_TYPE_SLING)
    return TRUE;

  if (GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER) > 0 && weapon_type == WEAPON_TYPE_SCYTHE)
    return TRUE;

  if (IS_DWARF(ch) && HAS_FEAT((struct char_data *)ch, FEAT_WEAPON_PROFICIENCY_MARTIAL)) 
  {
    switch (weapon_type) 
    {
      case WEAPON_TYPE_DWARVEN_WAR_AXE:
      case WEAPON_TYPE_DWARVEN_URGOSH:
      case WEAPON_TYPE_BATTLE_AXE:
      case WEAPON_TYPE_WARHAMMER:
      case WEAPON_TYPE_HEAVY_PICK:
        return TRUE;
    }
  }

  if (GET_RACE(ch) == RACE_CENTAUR) {
    switch (weapon_type) {
      case WEAPON_TYPE_LONG_SWORD:
      case WEAPON_TYPE_LONG_BOW:
      case WEAPON_TYPE_COMPOSITE_LONGBOW:
      case WEAPON_TYPE_SHORT_BOW:
      case WEAPON_TYPE_COMPOSITE_SHORTBOW:
        return TRUE;
    }
  }

  if (GET_RACE(ch) == RACE_OGRE && weapon_type == WEAPON_TYPE_GREAT_CLUB)
    return TRUE;

  return FALSE;

}

void list_feats_known(struct char_data *ch, char *arg) 
{
  int sortpos = 0, j = 0, mode = 0;
  char buf2[MAX_STRING_LENGTH]={'\0'};

  GRID_DATA *grid;
  GRID_ROW *row;

  if (*arg && is_abbrev(arg, "descriptions")) 
  {
    mode = 1;
  }
  else if (*arg && is_abbrev(arg, "requisites")) 
  {
    mode = 2;
  }

  // NEW GRID LAYOUT FOR FEATS.
    grid = create_grid(75);
    row = create_row(grid);
    row_append_cell(row, 75, "@WFeats Known@n");
    row = create_row(grid);

    if (!GET_FEAT_POINTS(ch))
    {
      row_append_cell(row, 75, "@RYou cannot learn any feats right now.@n");
    }
    else
    {
      row_append_cell(row, 75, "You can learn %d feat%s and %d class feat%s right now.", GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"), GET_CLASS_FEATS(ch, GET_CLASS(ch)), 
    (GET_CLASS_FEATS(ch, GET_CLASS(ch)) == 1 ? "" : "s"));
    }

    row = create_row(grid);
    row_append_cell(row, 35, "Feats");
    if (mode == 2)
    {
      row_append_cell(row, 40, "Prerequisites");
    }
    else
    {
      row_append_cell(row, 40, "Benefits");
    }

    for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) 
  {

    if (strlen(buf2) > MAX_STRING_LENGTH -32)
      break;

    int i = feat_sort_info[sortpos];
    if (HAS_FEAT(ch, i)  && feat_list[i].in_game) 
    {
      if (i == FEAT_FAST_CRAFTER) 
      {
          row = create_row(grid);
          row_append_cell(row, 35, "%s", feat_list[i].name);
          
          if (mode == 2) 
          {
            row_append_cell(row, 40, "@W%s \n(@G-%d seconds@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_FAST_CRAFTER) * 10);
          } 
          else 
          {
            row_append_cell(row, 40, "@W%s \n(@G-%d seconds@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_FAST_CRAFTER) * 10);
          } 
    }
    else if (i == FEAT_PROFICIENT_HARVESTER)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode ==2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d to checks@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_PROFICIENT_HARVESTER));
      }
      else
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d to checks@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_PROFICIENT_HARVESTER));
      }
    }
    else if (i == FEAT_PROFICIENT_CRAFTER)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode ==2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d to checks@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER));
      }
      else
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d to checks@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER));
      }
    }
    else if (i == FEAT_FASTER_MEMORIZATION)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d ranks@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d ranks@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION)); 
      }
    }
    else if (i == FEAT_FAST_HEALING)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d hp/round@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_FAST_HEALING));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d hp/round@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_FAST_HEALING)); 
      }
    }
    else if (i == FEAT_LEADERSHIP)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d%% group experience@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_LEADERSHIP) *5);
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d%% group experience@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_LEADERSHIP) *5); 
      }
    }
    else if (i == FEAT_SKILL_FOCUS || i == FEAT_EPIC_SKILL_FOCUS) 
    {
      row = create_row(grid);
      for (j = SKILL_LOW_SKILL; j < SKILL_HIGH_SKILL; j++) 
      {
        if (ch->player_specials->skill_focus[j-SKILL_LOW_SKILL] > 0) 
        {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, spell_info[j].name);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
        }
      }
    }
    else if (i == FEAT_EMPOWERED_MAGIC)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d to DC@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d to DC@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC)); 
      }
    }
    else if (i == FEAT_ENHANCED_SPELL_DAMAGE)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d dam/die@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d dam/die@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE)); 
      }
    }
    else if (i == FEAT_ENHANCE_SPELL)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G+%d damage dice@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_ENHANCE_SPELL));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G+%d damage dice@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_ENHANCE_SPELL)); 
      }
    }
    else if (i == FEAT_BREATH_WEAPON)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G%dd8 dmg|x%d/day@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_BREATH_WEAPON)*2, HAS_FEAT(ch, FEAT_BREATH_WEAPON));
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G%dd8 dmg|x%d/day@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_BREATH_WEAPON)*2, HAS_FEAT(ch, FEAT_BREATH_WEAPON)); 
      }
    }
    else if (i == FEAT_SELF_CONCEALMENT)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2)
      {
        row_append_cell(row, 40, "@W%s \n(@G%d%% miss@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) * 10);
      }
      else
      {
       row_append_cell(row, 40, "@W%s \n(@G%d%% miss@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) * 10); 
      }
    }
    else if (i == FEAT_NATURAL_ARMOR_INCREASE)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n\n (@G+%d natural AC@W)@n", feat_list[i].prerequisites, HAS_FEAT(ch, FEAT_NATURAL_ARMOR_INCREASE));
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n\n (@G+%d natural AC@W)@n", feat_list[i].description, HAS_FEAT(ch, FEAT_NATURAL_ARMOR_INCREASE));
      } 
    }
    else if (i == FEAT_DEFENSIVE_STANCE)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (@G%d/day@n)", feat_list[i].name, HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE));

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_RAGE)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (@G%d/day@n)", feat_list[i].name, HAS_FEAT(ch, FEAT_RAGE));

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_POISON_SAVE_BONUS)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_POISON_SAVE_BONUS));

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_ENERGY_RESISTANCE)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENERGY_RESISTANCE)*3);

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_DAMAGE_REDUCTION)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION));

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_ARMOR_SKIN)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s\n (+%d Natural AC)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMOR_SKIN));

      if (mode == 2) 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }

    else if (i == FEAT_FAVORED_ENEMY) 
      {
        row = create_row(grid);
        for (j = 1; j < NUM_RACE_TYPES; j++)
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_FAVORED_ENEMY, j))
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, race_types[j]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      }
    else if (i == FEAT_WEAPON_SUPREMACY) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SUPREMACY, j) || has_weapon_feat_full(ch, FEAT_WEAPON_SUPREMACY, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      }
    else if (i == FEAT_WEAPON_MASTERY) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_MASTERY, j) || has_weapon_feat_full(ch, FEAT_WEAPON_MASTERY, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      }
    else if (i == FEAT_WEAPON_FOCUS) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, j) || has_weapon_feat_full(ch, FEAT_WEAPON_FOCUS, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
   else if (i == FEAT_GREATER_WEAPON_FOCUS) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, j) || has_weapon_feat_full(ch, FEAT_GREATER_WEAPON_FOCUS, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
     else if (i == FEAT_WEAPON_SPECIALIZATION) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION, j) || has_weapon_feat_full(ch, FEAT_WEAPON_SPECIALIZATION, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
     else if (i == FEAT_GREATER_WEAPON_SPECIALIZATION) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION, j) || has_weapon_feat_full(ch, FEAT_GREATER_WEAPON_SPECIALIZATION, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_IMPROVED_WEAPON_FINESSE) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_WEAPON_FINESSE, j) || has_weapon_feat_full(ch, FEAT_IMPROVED_WEAPON_FINESSE, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_WEAPON_PROFICIENCY_EXOTIC) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_PROFICIENCY_EXOTIC, j) || has_weapon_feat_full(ch, FEAT_WEAPON_PROFICIENCY_EXOTIC, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_WEAPON_FLURRY) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FLURRY, j) || has_weapon_feat_full(ch, FEAT_WEAPON_FLURRY, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_WEAPON_PROFICIENCY_DEITY)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s\n(%s)", feat_list[i].name, weapon_list[deity_list[GET_DEITY(ch)].favored_weapon].name);
      if (mode == 2) 
      {
        
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_CRITICAL_FOCUS) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_CRITICAL_FOCUS, j) || has_weapon_feat_full(ch, FEAT_CRITICAL_FOCUS, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_IMPROVED_CRITICAL) 
      {
        row = create_row(grid);
        for (j = MIN_WEAPON_DAMAGE_TYPES; j <= MAX_WEAPON_DAMAGE_TYPES; j++) 
        {
          if (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_CRITICAL, j) || has_weapon_feat_full(ch, FEAT_IMPROVED_CRITICAL, j, FALSE)) 
          {
            row_append_cell(row, 35, "%s (%s)", feat_list[i].name, weapon_damage_types[j-MIN_WEAPON_DAMAGE_TYPES]);
            if (mode == 2) 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
            } 
            else 
            {
              row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
            } 
          }   
        }
      } 
    else if (i == FEAT_SNEAK_ATTACK)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (+%dd6)", feat_list[i].name, HAS_FEAT(ch, FEAT_SNEAK_ATTACK));
      if (mode == 2) 
      {
        
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }
    else if (i == FEAT_ENHANCE_ARROW_MAGIC)
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC));
      if (mode == 2) 
      {
        
        row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
      } 
    }// All non-specific feats go here.
    else
    {
      row = create_row(grid);
      row_append_cell(row, 35, "%s", feat_list[i].name);
        if (mode == 2) 
        {
          row_append_cell(row, 40, "@W%s@n", feat_list[i].prerequisites);
        } 
        else 
        {
          row_append_cell(row, 40, "@W%s@n", feat_list[i].description);
        }
      }
  }
}

    row = create_row(grid);
    row_append_cell(row, 75, "@WSyntax:\nfeats <known|available> <description|requisites>\n(both arguments optional)@n");

    grid_to_char(grid, ch, TRUE);

  // END NEW GRID LAYOUT
}

void list_feats_available(struct char_data *ch, char *arg) 
{
  char buf[MAX_STRING_LENGTH]={'\0'}, buf2[MAX_STRING_LENGTH]={'\0'};
  int sortpos = 0, mode = 0;
  int none_shown = TRUE;

  GRID_DATA *grid;
  GRID_ROW *row;
  // GRID_CELL *cell;
  
  if (*arg && is_abbrev(arg, "descriptions")) 
  {
    mode = 1;
  }
  else if (*arg && is_abbrev(arg, "requisites")) 
  {
    mode = 2;
  }
  else if (*arg && (is_abbrev(arg, "classfeats") || is_abbrev(arg, "class-feats"))) 
  {
    list_class_feats(ch);
    return;
  }

    // NEW GRID LAYOUT FOR FEATS.
    grid = create_grid(75);
    row = create_row(grid);
    row_append_cell(row, 75, "@WFeats Available to Learn@n");
    row = create_row(grid);

    if (!GET_FEAT_POINTS(ch))
    {
      row_append_cell(row, 75, "@RYou cannot learn any feats right now.@n");
    }
    else
    {
      row_append_cell(row, 75, "You can learn %d feat%s and %d class feat%s right now.", GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"), GET_CLASS_FEATS(ch, GET_CLASS(ch)), 
    (GET_CLASS_FEATS(ch, GET_CLASS(ch)) == 1 ? "" : "s"));
    }

    row = create_row(grid);
    row_append_cell(row, 35, "Feats");
    if (mode == 2)
    {
      row_append_cell(row, 40, "Prerequisites");
    }
    else
    {
      row_append_cell(row, 40, "Benefits");
    }

// LIST OF AVAILABLE FEATS
  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) 
  {
    int i = feat_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) 
    {
      strcat(buf2, "**OVERFLOW**\r\n"); 
      break;   
    }
    if (feat_is_available(ch, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn) 
    {
        row = create_row(grid);
        row_append_cell(row, 35, "@W%s@n", feat_list[i].name);
        if (mode == 2) 
        {
          row_append_cell(row, 40, "%s", feat_list[i].prerequisites);
        } 
        else 
        {
          row_append_cell(row, 40, "%s", feat_list[i].description);
        } 
    }
  }

  if (none_shown)
  {
    row = create_row(grid);
    row_append_cell(row, 75, "There are no feats available for you to learn at this point.");
  }
  
  row = create_row(grid);
  row_append_cell(row, 75, "@WSyntax:\nfeats <known|available> <description|requisites>\n(both arguments optional)@n");

  grid_to_char(grid, ch, TRUE);

  // END NEW GRID LAYOUT
 
  strcpy(buf2, buf);
}

void list_class_feats(struct char_data *ch)
{

  int featMarker = 1, featCounter = 0, sortpos = 0;
  char buf3[100]={'\0'};

    send_to_char(ch, "\r\n");
    send_to_char(ch, "@WClass Feats Available to Learn@n\r\n");
    send_to_char(ch, "@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@n\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "@W%-30s@n %s\r\n", "Feat Name", "Feat Description");
    send_to_char(ch, "@W%-30s@n %s\r\n", " ", "Feat Prerequisites");
    send_to_char(ch, "\r\n");


  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) 
  {
    int i = feat_sort_info[sortpos];
    if (feat_is_available(ch, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn) 
    {
      featMarker = 1;
      featCounter = 0;
      while (featMarker != 0) 
      {
        featMarker = class_bonus_feats[GET_CLASS(ch)][featCounter];
        if (i == featMarker) 
        {
          sprintf(buf3, "%s:", feat_list[i].name);
          send_to_char(ch, "@W%-30s@n %s\r\n", buf3, feat_list[featMarker].description);
          send_to_char(ch, "@W%-30s@n %s\r\n", " ", feat_list[featMarker].prerequisites);
        }
        featCounter++;
      }
    }  
  }
}

int is_class_feat(int featnum, int class) 
{

  int i = 0;
  int marker = class_bonus_feats[class][i];

  while (marker != FEAT_UNDEFINED) 
  {
    if (marker == featnum)
      return TRUE;
    marker = class_bonus_feats[class][++i];
  }

  return FALSE;

}

void list_feats_complete(struct char_data *ch, char *arg) 
{

  char buf[MAX_STRING_LENGTH]={'\0'}, buf2[MAX_STRING_LENGTH]={'\0'};
  int none_shown = TRUE;
  int mode = 0, sortpos = 0;

  if (*arg && is_abbrev(arg, "descriptions")) 
  {
    mode = 1;
  }
  else if (*arg && is_abbrev(arg, "requisites")) 
  {
    mode = 2;
  }

  GRID_DATA *grid;
  GRID_ROW *row;

// NEW GRID LAYOUT FOR FEATS.
  grid = create_grid(75);
  row = create_row(grid);
  row_append_cell(row, 75, "@WComplete Feat List@n");
  row = create_row(grid);

  if (!GET_FEAT_POINTS(ch))
  {
    row_append_cell(row, 75, "@RYou cannot learn any feats right now.@n");
  }
  else
  {
    row_append_cell(row, 75, "You can learn %d feat%s and %d class feat%s right now.", GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"), GET_CLASS_FEATS(ch, GET_CLASS(ch)), 
      (GET_CLASS_FEATS(ch, GET_CLASS(ch)) == 1 ? "" : "s"));
  }

  row = create_row(grid);
  row_append_cell(row, 35, "Feats");
  if (mode == 2)
  {
    row_append_cell(row, 40, "Prerequisites");
  }
  else
  {
    row_append_cell(row, 40, "Benefits");
  }

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) 
  {
    int i = feat_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) 
    {
      strcat(buf2, "**OVERFLOW**\r\n"); 
      break;   
    }
    if (feat_list[i].in_game) 
    {
      row = create_row(grid);

      row_append_cell(row, 35, "%s", feat_list[i].name);
      if (mode == 2) 
      {
        row_append_cell(row, 40, "%s", feat_list[i].prerequisites);
      } 
      else 
      {
        row_append_cell(row, 40, "%s", feat_list[i].description);
      } 

    }
  }
  if (none_shown)
  {
    row = create_row(grid);
    row_append_cell(row, 75, "There are no feats available for you to learn at this point.");
  }

  row = create_row(grid);
  row_append_cell(row, 75, "@WSyntax:\nfeats <known|available> <description|requisites|classfeats>\n(both arguments optional)@n");

  grid_to_char(grid, ch, TRUE);

  strcpy(buf2, buf);

  page_string(ch->desc, buf2, 1);
}

int find_feat_num(char *name)
{  
  int index = 0;
  char first[256]={'\0'}, first2[256]={'\0'};
   
  for (index = 1; index <= NUM_FEATS_DEFINED; index++) 
  {
    char *temp, *temp2;
  
    if (is_abbrev(name, feat_list[index].name))
      return (index);
    
    int ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *)feat_list[index].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) 
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }
  
    if (ok && !*first2)
      return (index);
  }
    
  return (-1);
}

ACMD(do_feats)
{
  char arg[80]={'\0'};
  char arg2[80]={'\0'};

  two_arguments(argument, arg, arg2);

  if (is_abbrev(arg, "known") || !*arg) 
  {
    list_feats_known(ch, arg2);
  } 
  else if (is_abbrev(arg, "available")) 
  {
    list_feats_available(ch, arg2);
  } 
  else if (is_abbrev(arg, "complete")) 
  {
    list_feats_complete(ch, arg2);
  }
}

int feat_to_subfeat(int feat)
{
  switch (feat) {
  case FEAT_SKILL_FOCUS:
    return CFEAT_SKILL_FOCUS;
  case FEAT_EPIC_SKILL_FOCUS:
    return CFEAT_EPIC_SKILL_FOCUS;
  case FEAT_IMPROVED_CRITICAL:
    return CFEAT_IMPROVED_CRITICAL;
  case FEAT_CRITICAL_FOCUS:
    return CFEAT_CRITICAL_FOCUS;
  case FEAT_WEAPON_FINESSE:
    return CFEAT_WEAPON_FINESSE;
  case FEAT_WEAPON_FOCUS:
    return CFEAT_WEAPON_FOCUS;
  case FEAT_WEAPON_SPECIALIZATION:
    return CFEAT_WEAPON_SPECIALIZATION;
  case FEAT_GREATER_WEAPON_FOCUS:
    return CFEAT_GREATER_WEAPON_FOCUS;
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
    return CFEAT_GREATER_WEAPON_SPECIALIZATION;
  case FEAT_SPELL_FOCUS:
    return SFEAT_SPELL_FOCUS;
  case FEAT_GREATER_SPELL_FOCUS:
    return SFEAT_GREATER_SPELL_FOCUS;
  case FEAT_IMPROVED_WEAPON_FINESSE:
    return CFEAT_IMPROVED_WEAPON_FINESSE;
  case FEAT_WEAPON_PROFICIENCY_EXOTIC:
    return CFEAT_WEAPON_PROFICIENCY_EXOTIC;
  case FEAT_MONKEY_GRIP:
    return CFEAT_MONKEY_GRIP;
  case FEAT_WEAPON_MASTERY:
    return CFEAT_WEAPON_MASTERY;
  case FEAT_WEAPON_FLURRY:
    return CFEAT_WEAPON_FLURRY;
  case FEAT_WEAPON_SUPREMACY:
    return CFEAT_WEAPON_SUPREMACY;
  default:
    return -1;
  }
}

void setweapon( int type, char *name, int numDice, int diceSize, int critRange, int critMult, 
int weaponFlags, int cost, int damageTypes, int weight, int range, int weaponFamily, int size, 
int material, int handle_type, int head_type) 
{

  weapon_type[type] = name;
  weapon_list[type].name = name;
  weapon_list[type].numDice = numDice;
  weapon_list[type].diceSize = diceSize;
  weapon_list[type].critRange = critRange;
  if (critMult == 2)
    weapon_list[type].critMult = CRIT_X2;
  else if (critMult == 3)
    weapon_list[type].critMult = CRIT_X3;
  else if (critMult == 4)
    weapon_list[type].critMult = CRIT_X4;
  weapon_list[type].weaponFlags = weaponFlags;
  weapon_list[type].cost = cost / 100;
  weapon_list[type].damageTypes = damageTypes;
  weapon_list[type].weight = weight;
  weapon_list[type].range = range;
  weapon_list[type].weaponFamily = weaponFamily;
  weapon_list[type].size = size;
  weapon_list[type].material = material;
  weapon_list[type].handle_type = handle_type;
  weapon_list[type].head_type = head_type;

}

void initialize_weapons(int type) 
{

  weapon_list[type].name = "unused weapon";
  weapon_list[type].numDice = 1;
  weapon_list[type].diceSize = 1;
  weapon_list[type].critRange = 0;
  weapon_list[type].critMult = 1;
  weapon_list[type].weaponFlags = 0;
  weapon_list[type].cost = 0;
  weapon_list[type].damageTypes = 0;
  weapon_list[type].weight = 0;
  weapon_list[type].range = 0;
  weapon_list[type].weaponFamily = 0;
  weapon_list[type].size = 0;
  weapon_list[type].material = 0;
  weapon_list[type].handle_type = 0;
  weapon_list[type].head_type = 0;

}

void load_weapons(void)
{
  int i = 0;

    for (i = 0; i <= MAX_WEAPON_TYPES; i++)
        initialize_weapons(i);

/*	setweapon(weapon number, num dam dice, size dam dice, crit range, crit mult, weapon flags, cost, damage type, weight, reach/range, weapon family, weapon size)
*/
	setweapon(WEAPON_TYPE_UNARMED, "unarmed", 1, 3, 0, 2, WEAPON_FLAG_SIMPLE, 200, 
DAMAGE_TYPE_BLUDGEONING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_ORGANIC, 
HANDLE_TYPE_GLOVE, HEAD_TYPE_FIST);
	setweapon(WEAPON_TYPE_DAGGER, "dagger", 1, 4, 1, 2, WEAPON_FLAG_THROWN | 
WEAPON_FLAG_SIMPLE, 200, DAMAGE_TYPE_PIERCING, 10, 10, WEAPON_FAMILY_SMALL_BLADE, SIZE_TINY, 
MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_LIGHT_MACE, "light mace", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 500, 
DAMAGE_TYPE_BLUDGEONING, 40, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SICKLE, "sickle", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 600, 
DAMAGE_TYPE_SLASHING, 20, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_CLUB, "club", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 10, 
DAMAGE_TYPE_BLUDGEONING, 30, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_WOOD, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_HEAVY_MACE, "heavy mace", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 1200, 
DAMAGE_TYPE_BLUDGEONING, 80, 0, WEAPON_FAMILY_CLUB, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_MORNINGSTAR, "morningstar", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 800, 
DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING, 60, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM, 
MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SHORTSPEAR, "shortspear", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE | 
WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_PIERCING, 30, 20, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, 
MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_LONGSPEAR, "longspear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE | 
WEAPON_FLAG_REACH, 500, DAMAGE_TYPE_PIERCING, 90, 0, WEAPON_FAMILY_SPEAR, SIZE_LARGE, 
MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_QUARTERSTAFF, "quarterstaff", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE,
10, DAMAGE_TYPE_BLUDGEONING, 40, 0, WEAPON_FAMILY_MONK, SIZE_LARGE, 
MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SPEAR, "spear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE | 
WEAPON_FLAG_THROWN | WEAPON_FLAG_REACH, 200, DAMAGE_TYPE_PIERCING, 60, 20, WEAPON_FAMILY_SPEAR, SIZE_LARGE, 
MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_HEAVY_CROSSBOW, "heavy crossbow", 1, 10, 1, 2, WEAPON_FLAG_SIMPLE 
| WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 5000, DAMAGE_TYPE_PIERCING, 80, 120, 
WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_LIGHT_CROSSBOW, "light crossbow", 1, 8, 1, 2, WEAPON_FLAG_SIMPLE 
| WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 3500, DAMAGE_TYPE_PIERCING, 40, 80, 
WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_DART, "dart", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN 
| WEAPON_FLAG_RANGED, 50, DAMAGE_TYPE_PIERCING, 5, 20, WEAPON_FAMILY_THROWN, SIZE_TINY, 
MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_JAVELIN, "javelin", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE | 
WEAPON_FLAG_THROWN | WEAPON_FLAG_RANGED, 100, DAMAGE_TYPE_PIERCING, 20, 30, 
WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_SLING, "sling", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | 
WEAPON_FLAG_RANGED, 10, DAMAGE_TYPE_BLUDGEONING, 1, 50, WEAPON_FAMILY_THROWN, SIZE_SMALL, 
MATERIAL_LEATHER, HANDLE_TYPE_STRAP, HEAD_TYPE_POUCH);
	setweapon(WEAPON_TYPE_THROWING_AXE, "throwing axe", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_THROWN, 800, DAMAGE_TYPE_SLASHING, 20, 10, WEAPON_FAMILY_AXE, SIZE_SMALL, 
MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_LIGHT_HAMMER, "light hammer", 1, 4, 0, 2, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_BLUDGEONING, 20, 20, WEAPON_FAMILY_HAMMER, SIZE_SMALL, 
MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_HAND_AXE, "hand axe", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL, 600, 
DAMAGE_TYPE_SLASHING, 30, 0, WEAPON_FAMILY_AXE, SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HANDLE, 
HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_KUKRI, "kukri", 1, 4, 2, 2, WEAPON_FLAG_MARTIAL, 800, 
DAMAGE_TYPE_SLASHING, 20, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_LIGHT_PICK, "light pick", 1, 4, 0, 4, WEAPON_FLAG_MARTIAL, 400, 
DAMAGE_TYPE_PIERCING, 30, 0, WEAPON_FAMILY_PICK, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SAP, "sap", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_SUBDUAL, 100, DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_SUBDUAL, 20, 0, 
WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SHORT_SWORD, "short sword", 1, 6, 1, 2, WEAPON_FLAG_MARTIAL, 
1000, DAMAGE_TYPE_PIERCING, 20, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_BATTLE_AXE, "battle axe", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1000, 
DAMAGE_TYPE_SLASHING, 60, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_FLAIL, "flail", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL, 800, 
DAMAGE_TYPE_BLUDGEONING, 50, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_LONG_SWORD, "long sword", 1, 8, 1, 2, WEAPON_FLAG_MARTIAL, 1500, 
DAMAGE_TYPE_SLASHING, 40, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_HEAVY_PICK, "heavy pick", 1, 6, 0, 4, WEAPON_FLAG_MARTIAL, 800, 
DAMAGE_TYPE_PIERCING, 60, 0, WEAPON_FAMILY_PICK, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_RAPIER, "rapier", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_BALANCED, 2000, DAMAGE_TYPE_PIERCING, 20, 0, WEAPON_FAMILY_SMALL_BLADE, 
SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_SCIMITAR, "scimitar", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL, 1500, 
DAMAGE_TYPE_SLASHING, 40, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_KHOPESH, "khopesh", 1, 8, 2, 2, WEAPON_FLAG_EXOTIC, 2500, 
DAMAGE_TYPE_SLASHING, 40, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_CURVE_BLADE, "elven curve blade", 1, 10, 2, 2, WEAPON_FLAG_EXOTIC, 6000, 
DAMAGE_TYPE_SLASHING, 70, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_TRIDENT, "trident", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_THROWN, 1500, DAMAGE_TYPE_PIERCING, 40, 0, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_WARHAMMER, "warhammer", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1200, 
DAMAGE_TYPE_BLUDGEONING, 50, 0, WEAPON_FAMILY_HAMMER, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_FALCHION, "falchion", 2, 4, 2, 2, WEAPON_FLAG_MARTIAL, 7500, 
DAMAGE_TYPE_SLASHING, 80, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_GLAIVE, "glaive", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_REACH, 800, DAMAGE_TYPE_SLASHING, 100, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_GREAT_AXE, "great axe", 1, 12, 0, 3, WEAPON_FLAG_MARTIAL, 2000, 
DAMAGE_TYPE_SLASHING, 120, 0, WEAPON_FAMILY_AXE, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_GREAT_CLUB, "great club", 1, 10, 0, 2, WEAPON_FLAG_MARTIAL, 500, 
DAMAGE_TYPE_BLUDGEONING, 80, 0, WEAPON_FAMILY_CLUB, SIZE_LARGE, MATERIAL_WOOD, 
HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_HEAVY_FLAIL, "heavy flail", 1, 10, 1, 2, WEAPON_FLAG_MARTIAL, 
1500, DAMAGE_TYPE_BLUDGEONING, 100, 0, WEAPON_FAMILY_FLAIL, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_GREAT_SWORD, "great sword", 2, 6, 1, 2, WEAPON_FLAG_MARTIAL, 
5000, DAMAGE_TYPE_SLASHING, 80, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_FULLBLADE, "fullblade", 2, 8, 1, 2, WEAPON_FLAG_EXOTIC, 
6000, DAMAGE_TYPE_SLASHING, 100, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_GUISARME, "guisarme", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_REACH, 900, DAMAGE_TYPE_SLASHING, 120, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_HALBERD, "halberd", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 120, 0, 
WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_LANCE, "lance", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_REACH | WEAPON_FLAG_CHARGE, 1000, DAMAGE_TYPE_PIERCING, 100, 0, 
WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_RANSEUR, "ranseur", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_PIERCING, 100, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_SCYTHE, "scythe", 2, 4, 0, 4, WEAPON_FLAG_MARTIAL, 1800, 
DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 100, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_LONG_BOW, "long bow", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 30, 100, WEAPON_FAMILY_BOW, SIZE_MEDIUM, 
MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW, "composite long bow", 1, 8, 0, 3, 
WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 30, 110, 
WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_GREATBOW, "great bow", 1, 12, 0, 3, 
WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 30, 200, 
WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_SHORT_BOW, "short bow", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL | 
WEAPON_FLAG_RANGED, 3000, DAMAGE_TYPE_PIERCING, 20, 60, WEAPON_FAMILY_BOW, SIZE_SMALL, 
MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW, "composite short bow", 1, 6, 0, 3, 
WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 20, 70, 
WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_KAMA, "kama", 1, 6, 0, 2, WEAPON_FLAG_EXOTIC, 200, 
DAMAGE_TYPE_SLASHING, 20, 0,  WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_NUNCHAKU, "nunchaku", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 200, 
DAMAGE_TYPE_BLUDGEONING, 20, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_WOOD, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_SAI, "sai", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN, 
100, DAMAGE_TYPE_BLUDGEONING, 10, 10, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_SIANGHAM, "siangham", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 300, 
DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL, 
HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
	setweapon(WEAPON_TYPE_BASTARD_SWORD, "bastard sword", 1, 10, 1, 2, WEAPON_FLAG_EXOTIC, 
3500, DAMAGE_TYPE_SLASHING, 60, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL, 
HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_DWARVEN_WAR_AXE, "dwarven war axe", 1, 10, 0, 3, 
WEAPON_FLAG_EXOTIC, 3000, DAMAGE_TYPE_SLASHING, 80, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM, 
MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_WHIP, "whip", 1, 3, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_REACH 
| WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP, 100, DAMAGE_TYPE_SLASHING, 20, 0, WEAPON_FAMILY_WHIP, 
SIZE_MEDIUM, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_CORD);
	setweapon(WEAPON_TYPE_SPIKED_CHAIN, "spiked chain", 2, 4, 0, 2, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_REACH | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP, 2500, DAMAGE_TYPE_PIERCING, 100, 0, 
WEAPON_FAMILY_WHIP, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_CHAIN);
	setweapon(WEAPON_TYPE_DOUBLE_AXE, "double-headed axe", 1, 8, 0, 3, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_DOUBLE, 6500, DAMAGE_TYPE_SLASHING, 150, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_DIRE_FLAIL, "dire flail", 1, 8, 0, 2, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_DOUBLE, 9000, DAMAGE_TYPE_BLUDGEONING, 100, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE, 
MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_HOOKED_HAMMER, "hooked hammer", 1, 6, 0, 4, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_DOUBLE, 2000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_BLUDGEONING, 60, 0, 
WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
	setweapon(WEAPON_TYPE_2_BLADED_SWORD, "two-bladed sword", 1, 8, 1, 2, 
WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 10000, DAMAGE_TYPE_SLASHING, 100, 0, 
WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_DWARVEN_URGOSH, "dwarven urgosh", 1, 7, 0, 3, WEAPON_FLAG_EXOTIC 
| WEAPON_FLAG_DOUBLE, 5000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING, 120, 0, 
WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
	setweapon(WEAPON_TYPE_HAND_CROSSBOW, "hand crossbow", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 20, 30, WEAPON_FAMILY_CROSSBOW, SIZE_SMALL, 
MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_HEAVY_REP_XBOW, "heavy repeating crossbow", 1, 10, 1, 2, 
WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED | WEAPON_FLAG_REPEATING, 40000, DAMAGE_TYPE_PIERCING, 120, 120, 
WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_LIGHT_REP_XBOW, "light repeating crossbow", 1, 8, 1, 2, 
WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 25000, DAMAGE_TYPE_PIERCING, 60, 80, 
WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
	setweapon(WEAPON_TYPE_BOLA, "bola", 1, 4, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN 
| WEAPON_FLAG_TRIP, 500, DAMAGE_TYPE_BLUDGEONING, 20, 10, WEAPON_FAMILY_THROWN, SIZE_MEDIUM, 
MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_CORD);
	setweapon(WEAPON_TYPE_NET, "net", 1, 1, 0, 1, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN | 
WEAPON_FLAG_ENTANGLE, 2000, DAMAGE_TYPE_BLUDGEONING, 60, 10, WEAPON_FAMILY_THROWN, SIZE_LARGE, 
MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_MESH);
	setweapon(WEAPON_TYPE_SHURIKEN, "shuriken", 1, 2, 0, 2, WEAPON_FLAG_EXOTIC | 
WEAPON_FLAG_THROWN, 20, DAMAGE_TYPE_PIERCING, 5, 10, WEAPON_FAMILY_MONK, SIZE_SMALL, 
MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_BLADE); 

}

void setarmor(int type, char *name, int armorType, int cost, int armorBonus, int dexBonus, int armorCheck, int spellFail, int thirtyFoot, int twentyFoot, int weight, int material)
{

  armor_list[type].name = name;
  armor_list[type].armorType = armorType;
  armor_list[type].cost = cost / 100;
  armor_list[type].armorBonus = armorBonus;
  armor_list[type].dexBonus = dexBonus;
  armor_list[type].armorCheck = armorCheck;
  armor_list[type].spellFail = spellFail;
  armor_list[type].thirtyFoot = thirtyFoot;
  armor_list[type].twentyFoot = twentyFoot;
  armor_list[type].weight = weight;
  armor_list[type].material = material;

}

void initialize_armor(int type)
{

  armor_list[type].name = "unused armor";
  armor_list[type].armorType = 0;
  armor_list[type].cost = 0;
  armor_list[type].armorBonus = 0;
  armor_list[type].dexBonus = 0;
  armor_list[type].armorCheck = 0;
  armor_list[type].spellFail = 0;
  armor_list[type].thirtyFoot = 0;
  armor_list[type].twentyFoot = 0;
  armor_list[type].weight = 0;
  armor_list[type].material = 0;
}

void load_armor(void) 
{

    int i = 0;

    for (i = 0; i <= NUM_SPEC_ARMOR_TYPES; i++)
	    initialize_armor(i);
	
	setarmor(SPEC_ARMOR_TYPE_CLOTHING, "clothing", ARMOR_TYPE_NONE, 100, 0, 999, 0, 0, 30, 20, 100, MATERIAL_COTTON);
	setarmor(SPEC_ARMOR_TYPE_PADDED, "padded armor", ARMOR_TYPE_LIGHT, 500, 10, 8, 0, 5, 30, 20, 100, MATERIAL_COTTON);
	setarmor(SPEC_ARMOR_TYPE_LEATHER, "leather armor", ARMOR_TYPE_LIGHT, 1000, 20, 6, 0, 10, 30, 20, 150, MATERIAL_LEATHER);
	setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER, "studded leather armor", ARMOR_TYPE_LIGHT, 2500, 30, 5, -1, 15, 30, 20, 200, MATERIAL_LEATHER);
	setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN, "light chainmail armor", ARMOR_TYPE_LIGHT, 10000, 40, 4, -2, 20, 30, 20, 250, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_HIDE, "hide armor", ARMOR_TYPE_MEDIUM, 1500, 30, 4, -3, 20, 20, 15, 250, MATERIAL_LEATHER);
	setarmor(SPEC_ARMOR_TYPE_SCALE, "scale armor", ARMOR_TYPE_MEDIUM, 5000, 40, 3, -4, 25, 20, 15, 300, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_CHAINMAIL, "chainmail armor", ARMOR_TYPE_MEDIUM, 15000, 50, 2, -5, 30, 20, 15, 400, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_PIECEMEAL, "piecemeal armor", ARMOR_TYPE_MEDIUM, 20000, 50, 3, -4, 25, 20, 15, 300, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_SPLINT, "splint mail armor", ARMOR_TYPE_HEAVY, 20000, 60, 0, -7, 40, 20, 15, 450, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_BANDED, "banded mail armor", ARMOR_TYPE_HEAVY, 25000, 60, 1, -6, 35, 20, 15, 350, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_HALF_PLATE, "half plate armor", ARMOR_TYPE_HEAVY, 60000, 70, 1, -6, 40, 20, 15, 500, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_FULL_PLATE, "full plate armor", ARMOR_TYPE_HEAVY, 150000, 80, 1, -6, 35, 20, 15, 500, MATERIAL_STEEL);
	setarmor(SPEC_ARMOR_TYPE_BUCKLER, "buckler shield", ARMOR_TYPE_SHIELD, 1500, 10, 99, -1, 5, 999, 999, 50, MATERIAL_WOOD);
	setarmor(SPEC_ARMOR_TYPE_SMALL_SHIELD, "small shield", ARMOR_TYPE_SHIELD, 900, 10, 99, -1, 5, 999, 999, 60, MATERIAL_WOOD);
	setarmor(SPEC_ARMOR_TYPE_LARGE_SHIELD, "heavy shield", ARMOR_TYPE_SHIELD, 2000, 20, 99, -2, 15, 999,999, 150, MATERIAL_WOOD);
	setarmor(SPEC_ARMOR_TYPE_TOWER_SHIELD, "tower shield", ARMOR_TYPE_SHIELD, 3000, 40, 2, -10, 50, 999, 999, 450, MATERIAL_WOOD);
}

void display_levelup_feats(struct char_data *ch) 
{

  int sortpos=0, count=0;
  int featMarker = 1, featCounter = 0;

  if (ch->levelup->feat_points > 5)
    ch->levelup->feat_points = 0;
  if (ch->levelup->num_class_feats > 5)
    ch->levelup->num_class_feats = 0;
  if (ch->levelup->epic_feat_points > 5)
    ch->levelup->epic_feat_points = 0;
  if (ch->levelup->num_epic_class_feats > 5)
    ch->levelup->num_epic_class_feats = 0;

  send_to_char(ch, "Available Feats to Learn:\r\n"
    "Number Available: Normal (%d) Class (%d) Epic (%d) Epic CLass (%d)\r\n\r\n",
    ch->levelup->feat_points, ch->levelup->num_class_feats, ch->levelup->epic_feat_points, ch->levelup->num_epic_class_feats);

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) 
  {
    int i = feat_sort_info[sortpos];
    int classfeat = FALSE;

    while (featMarker != 0) 
    {
      featMarker = class_bonus_feats[ch->levelup->class][featCounter];
      if (i == featMarker) {
        classfeat = TRUE;
      }
      featCounter++;
    }

    if (feat_is_available(ch, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn &&
      (!HAS_FEAT(ch, i) || feat_list[i].can_stack)) 
    {

      send_to_char(ch, "%s%3d) %-40s @n", classfeat ? "@C(C) " : "@y(N) ", i, feat_list[i].name);
      count++;
      if (count % 2 == 1)
        send_to_char(ch, "\r\n");
    }

  }

  if (count % 2 != 1)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  send_to_char(ch, "To select a feat, type the number beside it.  Class feats are in @Ccyan@n and marked with a (C).  When done type -1: ");

}

int has_combat_feat(struct char_data *ch, int i, int j) 
{

	if (ch->desc && ch->levelup && STATE(ch->desc) >= CON_LEVELUP_START && STATE(ch->desc) <= CON_LEVELUP_END) 
  {
		if ((IS_SET_AR((ch)->levelup->combat_feats[(i)], (j))))
			return TRUE;
	}

	if ((IS_SET_AR((ch)->combat_feats[(i)], (j))))
		return TRUE;

	return FALSE;
}

int has_weapon_feat(struct char_data *ch, int i, int j) 
{
  return has_weapon_feat_full(ch, i, j, TRUE);
}

int has_weapon_feat_full(struct char_data *ch, int i, int j, int display) 
{

  struct obj_data *obj = GET_EQ(ch, WEAR_WIELD);
  int k = 0;

  if (obj) {
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_SLASHING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_SLASHING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_SLASHING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_SLASHING))
      return TRUE;
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_BLUDGEONING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_BLUDGEONING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_BLUDGEONING))
      return TRUE;
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_PIERCING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_PIERCING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_PIERCING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_PIERCING))
      return TRUE;

    for (k = 0; k < MAX_WEAPON_TYPES; k++) {
      if (HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), k) && display &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, weapon_list[k].damageTypes) ||
          IS_SET(weapon_list[k].damageTypes, weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == weapon_list[k].damageTypes))
      {
        return TRUE;
      }
    }
    for (k = 0; k < 6; k++) {
      if (obj->affected[k].location == APPLY_FEAT && obj->affected[k].specific == i &&
          GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_VAL(obj, 0) == j)
      {
        return TRUE;
      }
      if (obj->affected[k].location == APPLY_FEAT && obj->affected[k].specific == i &&
          GET_OBJ_TYPE(obj) == ITEM_WEAPON && display &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, weapon_list[j].damageTypes) ||
          IS_SET(weapon_list[j].damageTypes, weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == weapon_list[j].damageTypes))
      {
        return TRUE;
      }
    }
  }

  obj = GET_EQ(ch, WEAR_HOLD);

  if (obj) {
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_SLASHING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_SLASHING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_SLASHING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_SLASHING))
      return TRUE;
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_BLUDGEONING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_BLUDGEONING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_BLUDGEONING))
      return TRUE;
    if (display && HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), WEAPON_DAMAGE_TYPE_PIERCING) &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_PIERCING) ||
          IS_SET(weapon_list[k].damageTypes, DAMAGE_TYPE_PIERCING) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == DAMAGE_TYPE_PIERCING))
      return TRUE;
    for (k = 0; k < MAX_WEAPON_TYPES; k++) {
      if (HAS_COMBAT_FEAT(ch, feat_to_subfeat(i), k) && display &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, weapon_list[k].damageTypes) ||
          IS_SET(weapon_list[k].damageTypes, weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == weapon_list[k].damageTypes))
      {
        return TRUE;
      }
    }
    for (k = 0; k < 6; k++) {
      if (obj->affected[k].location == APPLY_FEAT && obj->affected[k].specific == i &&
          GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_VAL(obj, 0) == j)
      {
        return TRUE;
      }
      if (obj->affected[k].location == APPLY_FEAT && obj->affected[k].specific == i &&
          GET_OBJ_TYPE(obj) == ITEM_WEAPON && display &&
         (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, weapon_list[j].damageTypes) ||
          IS_SET(weapon_list[j].damageTypes, weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes) ||
          weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes == weapon_list[j].damageTypes))
      {
        return TRUE;
      }
    }
  }

	return FALSE;
}

void display_levelup_weapons(struct char_data *ch) 
{
	int i=0;

	extern char *weapon_damage_types[];

  if (weapon_damage_types[i-MIN_WEAPON_DAMAGE_TYPES] != NULL)

	{
    send_to_char(ch, "Please select a weapon:\r\n\r\n");
  
    for (i = MIN_WEAPON_DAMAGE_TYPES; i <= MAX_WEAPON_DAMAGE_TYPES; i++) 
    {
      send_to_char(ch, "%2d) %-25s   ", i, weapon_damage_types[i-MIN_WEAPON_DAMAGE_TYPES]);
      if (i % 2 == 0)
        send_to_char(ch, "\r\n");
    }
  
    if (i % 2 != 0)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
  
    send_to_char(ch, "Please select a weapon by typing a number beside it: (-1 to cancel) ");
  }
}

void set_feat(struct char_data *ch, int i, int j) 
{

	if (ch->desc && ch->levelup && STATE(ch->desc) >= CON_LEVELUP_START && STATE(ch->desc) <= CON_LEVELUP_END) {
		ch->levelup->feats[i] = j;
		return;
	}

	SET_FEAT(ch, i, j);
}

#define FEAT_TYPE_NORMAL                1
#define FEAT_TYPE_NORMAL_CLASS          2
#define FEAT_TYPE_EPIC                  3
#define FEAT_TYPE_EPIC_CLASS            4


int handle_levelup_feat_points(struct char_data *ch, int feat_num, int return_val) 
{

  if (HAS_FEAT(ch, feat_num) && !feat_list[feat_num].can_stack) 
  {
    send_to_char(ch, "You already have this feat.\r\nPress enter to continue.\r\n");
    return FALSE;
  }


  int feat_points = ch->levelup->feat_points;
  int epic_feat_points = ch->levelup->epic_feat_points;
  int class_feat_points = ch->levelup->num_class_feats;
  int epic_class_feat_points = ch->levelup->num_epic_class_feats;
  int feat_type = 0;

  if (feat_list[feat_num].epic == TRUE) 
  {
    if (is_class_feat(feat_num, ch->levelup->class))
      feat_type = FEAT_TYPE_EPIC_CLASS;
    else
      feat_type = FEAT_TYPE_EPIC;
  }
  else 
  {
    if (is_class_feat(feat_num, ch->levelup->class))
      feat_type = FEAT_TYPE_NORMAL_CLASS;
    else
      feat_type = FEAT_TYPE_NORMAL;
  }

  if (return_val == 0)
  {
// if it's an epic feat, make sure they have an epic feat point

    if (feat_type == FEAT_TYPE_EPIC && epic_feat_points < 1) 
    {
      send_to_char(ch, "This is an epic feat and you do not have any epic feat points remaining.\r\n");
      send_to_char(ch, "Please press enter to continue.\r\n");
      return 0;
    }

// if it's an epic class feat, make sure they have an epic feat point or an epic class feat point

    if (feat_type == FEAT_TYPE_EPIC_CLASS && epic_feat_points < 1 && epic_class_feat_points < 1) 
    {
      send_to_char(ch, "This is an epic class feat and you do not have any epic feat points or epic class feat points remaining.\r\n");
      send_to_char(ch, "Please press enter to continue.\r\n");
      return 0;
    }

// if it's a normal feat, make sure they have a normal feat point

    if (feat_type == FEAT_TYPE_NORMAL && feat_points < 1) 
    {
      send_to_char(ch, "This is a normal feat and you do not have any normal feat points remaining.\r\n");
      send_to_char(ch, "Please press enter to continue.\r\n");
      return 0;
    }

// if it's a normal class feat, make sure they have a normal feat point or a normal class feat point

    if (feat_type == FEAT_TYPE_NORMAL_CLASS && feat_points < 1 && class_feat_points < 1 && epic_class_feat_points < 1) 
    {
      send_to_char(ch, "This is a normal class feat and you do not have any normal feat points or normal class feat points remaining.\r\n");
      send_to_char(ch, "Please press enter to continue.\r\n");
      return 0;
    }
    return 1;
  }
  else {
// reduce the appropriate feat point type based on what the feat type was set as above.  This simulatyes spendinng the feat

    if (feat_type == FEAT_TYPE_EPIC) 
    {
      epic_feat_points--;
    }
    else if (feat_type == FEAT_TYPE_EPIC_CLASS) 
    {
      if (epic_class_feat_points > 0)
        epic_class_feat_points--;
      else
        epic_feat_points--;
    }
    else if (feat_type == FEAT_TYPE_NORMAL) 
    {
      feat_points--;
    }
    else if (feat_type == FEAT_TYPE_NORMAL_CLASS) 
    {
      if (class_feat_points > 0)
        class_feat_points--;
      else if (feat_points > 0)
        feat_points--;
      if (epic_class_feat_points > 0)
        epic_class_feat_points--;
      else
        epic_feat_points--;
    }

    ch->levelup->feat_points = feat_points;
    ch->levelup->epic_feat_points = epic_feat_points;
    ch->levelup->num_class_feats = class_feat_points;
    ch->levelup->num_epic_class_feats = epic_class_feat_points;

    ch->levelup->feats[feat_num]++;

    return 1;
  }

  return 1;
}
