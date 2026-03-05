/**
 * @file traits.c
 * @brief Pathfinder Trait System Implementation
 *
 * This file contains the implementation of the Pathfinder trait system.
 * Traits are character bonuses selected at 1st level only during creation.
 *
 * Created: 2026-03-03
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "traits.h"

/* Global Variables */
struct trait_info trait_list[NUM_TRAITS_DEFINED + 1];
int trait_sort_info[MAX_TRAITS + 1];

/* Trait category names */
const char *trait_types[] = {
    "Undefined",
    "Combat",
    "Magic",
    "Faith",
    "Social",
    "\n"
};

/* Local Function Prototypes */
static int compare_traits(const void *x, const void *y);

/**
 * Helper function for sort_traits - compares trait names alphabetically
 */
static int compare_traits(const void *x, const void *y)
{
    int a = *(const int *)x;
    int b = *(const int *)y;

    return strcmp(trait_list[a].name, trait_list[b].name);
}

/**
 * Sort traits alphabetically by name for display purposes
 * Called at boot up
 */
void sort_traits(void)
{
    int a;

    /* Initialize array, avoiding reserved */
    for (a = 1; a <= NUM_TRAITS_DEFINED; a++) {
        trait_sort_info[a] = a;
    }

    qsort(&trait_sort_info[1], NUM_TRAITS_DEFINED, sizeof(int), compare_traits);
}

/**
 * Helper function to define a trait
 */
void traito(int traitnum, char *name, sbyte in_game, sbyte can_learn,
            char *category, byte category_type, char *prerequisites,
            char *description, byte effect_type, sh_int effect_value,
            sh_int effect_specific)
{
    trait_list[traitnum].name = name;
    trait_list[traitnum].in_game = in_game;
    trait_list[traitnum].can_learn = can_learn;
    trait_list[traitnum].category = category;
    trait_list[traitnum].category_type = category_type;
    trait_list[traitnum].prerequisites = prerequisites;
    trait_list[traitnum].description = description;
    trait_list[traitnum].effect_type = effect_type;
    trait_list[traitnum].effect_value = effect_value;
    trait_list[traitnum].effect_specific = effect_specific;
}

/**
 * Initialize all trait definitions
 * Called at boot
 */
void assign_traits(void)
{
    int i;

    /* Initialize all traits to undefined */
    for (i = 0; i <= NUM_TRAITS_DEFINED; i++) {
        trait_list[i].name = "Unused Trait";
        trait_list[i].in_game = FALSE;
        trait_list[i].can_learn = FALSE;
        trait_list[i].category = "none";
        trait_list[i].category_type = TRAIT_TYPE_UNDEFINED;
        trait_list[i].prerequisites = "-";
        trait_list[i].description = "undefined";
        trait_list[i].effect_type = TRAIT_EFFECT_NONE;
        trait_list[i].effect_value = 0;
        trait_list[i].effect_specific = 0;
    }

    /* Combat Traits */
    traito(TRAIT_REACTIONARY, "Reactionary", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You were bullied often as a child, but never quite developed an offensive "
           "response. Instead, you became adept at anticipating sudden attacks and reacting "
           "to danger quickly. You gain a +2 trait bonus on initiative checks.",
           TRAIT_EFFECT_INITIATIVE, 2, 0);

    traito(TRAIT_ARMOR_EXPERT, "Armor Expert", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You have worn armor as long as you can remember, either as part of your training "
           "to become a knight's squire or simply because you were seeking to emulate a hero. "
           "Your childhood armor wasn't the real thing as far as protection, but it did encumber "
           "you as much as real armor would have, and you've grown used to moving in such suits "
           "with relative grace. When you wear armor of any sort, reduce that suit's armor check "
           "penalty by 1, to a minimum check penalty of 0.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_KILLER, "Killer", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You made your first kill at a very young age and found the task of war or murder to "
           "your liking. You either take particular pride in a well-placed blow, or find vicious "
           "pleasure in twisting the blade to maximize your target's pain. You deal additional "
           "damage equal to your weapon's critical hit multiplier when you score a successful "
           "critical hit with a weapon; this additional damage is added to the final total, and "
           "is not multiplied by the critical hit multiple itself. This extra damage is a trait bonus.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_ANATOMIST, "Anatomist", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You have studied the workings of anatomy, either as a student at university or as an "
           "apprentice mortician or necromancer. You know where to aim your blows to strike vital "
           "organs. You gain a +1 trait bonus on all rolls made to confirm critical hits.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_BRUISING_INTELLECT, "Bruising Intellect", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "Your sharp intellect and rapier-like wit bruise egos. Intimidate is always a class "
           "skill for you, and you may use your Intelligence modifier when making Intimidate checks "
           "instead of your Charisma modifier.",
           TRAIT_EFFECT_SPECIAL, 1, SKILL_INTIMIDATE);

    traito(TRAIT_COURAGEOUS, "Courageous", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "Your childhood was brutal, but you persevered primarily through force of will and "
           "gut courage. Whenever you are affected by a fear effect, reduce the strength of that "
           "effect by one step. You gain a +2 trait bonus on saves against fear effects.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_DEFENDER_OF_SOCIETY, "Defender of the Society", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You take pride in your ability to defend yourself and others from danger. You gain a "
           "+1 trait bonus to Armor Class when fighting defensively or using the total defense action.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_FENCER, "Fencer", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You trained with blades and are skilled at defending yourself. You gain a +1 trait bonus "
           "on attack of opportunity attack rolls made with daggers, swords, and similar bladed weapons.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_RESILIENT, "Resilient", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "Growing up in a rough neighborhood or in the unforgiving wilds often forced you to subsist "
           "on food and water from doubtful sources. You've built up your constitution as a result. "
           "You gain a +1 trait bonus on Fortitude saves.",
           TRAIT_EFFECT_SAVE_BONUS, 1, TRAIT_SAVE_FORT);

    traito(TRAIT_RURAL, "Rural", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You spent your youth in the wild or in a rural area. You learned to react quickly to sudden "
           "danger. You gain a +1 trait bonus on Reflex saves.",
           TRAIT_EFFECT_SAVE_BONUS, 1, TRAIT_SAVE_REFLEX);

    traito(TRAIT_WEAPON_EXPERT, "Weapon Expert", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You've had excellent training with a specific weapon type. You gain a +1 trait bonus on "
           "attack rolls with weapons in your chosen weapon group.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_BULLY, "Bully", TRUE, TRUE,
           "Combat", TRAIT_TYPE_COMBAT, "-",
           "You grew up in an environment where the meek were ignored and you often had to resort to "
           "threats or violence to be heard. You gain a +1 trait bonus on Intimidate checks, and "
           "Intimidate is always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_INTIMIDATE);

    /* Magic Traits */
    traito(TRAIT_MAGICAL_KNACK, "Magical Knack", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "You were raised, either wholly or in part, by a magical creature, either after it found "
           "you abandoned in the woods or because your parents often left you in the care of a magical "
           "minion. This constant exposure to magic has made its mysteries easy for you to understand, "
           "even when you turn your mind to other devotions and tasks. Pick a class when you gain this "
           "trait—your caster level in that class gains a +2 trait bonus as long as this bonus doesn't "
           "raise your caster level above your current Hit Dice.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_DANGEROUSLY_CURIOUS, "Dangerously Curious", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "You have always been intrigued by magic, possibly because you were the child of a magician "
           "or priest. You often snuck into your parent's laboratory or shrine to tinker with spell "
           "components and magic devices, and frequently caused quite a bit of damage and headaches for "
           "your parent as a result. You gain a +1 bonus on Use Magic Device checks, and Use Magic Device "
           "is always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_USE_MAGIC_DEVICE);

    // traito(TRAIT_FOCUSED_MIND, "Focused Mind", TRUE, TRUE,
    //        "Magic", TRAIT_TYPE_MAGIC, "-",
    //        "Your childhood was dominated either by lessons of some sort (whether musical or academic) "
    //        "or by a horrible home life that encouraged your ability to block out distractions and focus "
    //        "on the immediate task at hand. You gain a +2 trait bonus on concentration checks.",
    //        TRAIT_EFFECT_SPECIAL, 2, SKILL_CONCENTRATION);

    traito(TRAIT_GIFTED_ADEPT, "Gifted Adept", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "Your interest in magic was inspired by witnessing a spell being cast in a particularly "
           "dramatic fashion, an event so powerful that you took your first steps on the path to "
           "magical mastery seeking to recreate that spell. Pick one spell when you choose this trait—from "
           "this point on, whenever you cast that spell, its effects manifest at +1 caster level.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_HEDGE_MAGICIAN, "Hedge Magician", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "You apprenticed for a time to an artisan who often built magic items, and he taught you "
           "many handy shortcuts and cost-saving techniques. Whenever you craft a magic item, you reduce "
           "the cost by 5%.",
           TRAIT_EFFECT_SPECIAL, 5, 0);

    traito(TRAIT_MAGICAL_LINEAGE, "Magical Lineage", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "One of your parents was a gifted spellcaster who not only used metamagic often, but also "
           "developed many magical items and perhaps even a new spell or two—and you have inherited a "
           "fragment of this greatness. Pick one spell when you choose this trait. When you apply "
           "metamagic feats to this spell that add at least 1 level to the spell, treat its actual "
           "level as 1 lower for determining the spell's final adjusted level.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_MATHEMATICAL_PRODIGY, "Mathematical Prodigy", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "Mathematics has always come easily for you, and you have always been able to \"see the math\" "
           "in the physical and magical world. You gain a +1 bonus on Knowledge (arcana) and Knowledge "
           "(engineering) checks, and one of these skills (your choice) is always a class skill for you.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_SKEPTIC, "Skeptic", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "Growing up, you were always on the lookout for tricks and any sort of magical deceit. "
           "This wariness has honed your senses, and makes you difficult to fool with mind-affecting "
           "magic. You gain a +2 trait bonus on all saving throws against illusion spells.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_CLASSICALLY_SCHOOLED, "Classically Schooled", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "-",
           "Your apprenticeship or early education was particularly focused on the direct application "
           "of magic. You gain a +1 trait bonus on Spellcraft checks, and Spellcraft is always a class "
           "skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_SPELLCRAFT);

    traito(TRAIT_FAST_LEARNER, "Fast Learner", TRUE, TRUE,
           "Magic", TRAIT_TYPE_MAGIC, "Sorcerer or Bard",
           "You have always been quick to pick up on new things and ideas. As a spontaneous caster, "
           "you are particularly skilled at adapting your magic. You gain 1 additional spell known at "
           "your highest spell level.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    /* Faith Traits */
    traito(TRAIT_BLESSED, "Blessed", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You have been blessed by a deity and have felt their grace and presence more than once. "
           "You gain a +1 trait bonus on all saving throws made against divine spells.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_BIRTHMARK, "Birthmark", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You were born with a strange birthmark that looks very similar to the holy symbol of the "
           "god you chose to worship later in life. This birthmark can serve you as a divine focus for "
           "casting spells, and as a physical manifestation of your faith, and it increases your "
           "devotion to your god. You gain a +2 trait bonus on all saving throws against charm and "
           "compulsion effects.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_SACRED_TOUCH, "Sacred Touch", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "Ability to channel energy",
           "You were exposed to a potent source of positive energy as a child, perhaps by being born "
           "under the right cosmic sign, or maybe because one of your parents was a gifted healer. "
           "As a standard action, you may automatically stabilize a dying creature merely by touching it. "
           "In addition, you gain a +1 trait bonus to the save DC of your channel energy ability.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_CHILD_OF_TEMPLE, "Child of the Temple", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You were raised in a temple and the priests there taught you many things. You gain a +1 "
           "trait bonus on Knowledge (religion) checks, and Knowledge (religion) is always a class skill "
           "for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_KNOWLEDGE);

    traito(TRAIT_DEVOTEE_OF_GREEN, "Devotee of the Green", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "Your faith in the natural world or one of the gods of nature makes it easy for you to "
           "pick up on related concepts. You gain a +1 trait bonus on Knowledge (nature) checks, and "
           "Knowledge (nature) is always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_KNOWLEDGE);

    traito(TRAIT_INDOMITABLE_FAITH, "Indomitable Faith", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You were born in a region where your faith was not popular, but you still have never "
           "abandoned it. Your constant struggle to maintain your own faith has bolstered your drive. "
           "You gain a +1 trait bonus on Will saves.",
           TRAIT_EFFECT_SAVE_BONUS, 1, TRAIT_SAVE_WILL);

    traito(TRAIT_SACRED_CONDUIT, "Sacred Conduit", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "Domain class feature",
           "Your birth was particularly painful and difficult for your mother, who needed potent divine "
           "magic to ensure that you survived (your mother may or may not have survived). In any event, "
           "that magic infused you from an early age, and you now channel divine energy with greater "
           "ease than most. Whenever you channel energy or cast a domain spell, you gain a +1 trait bonus "
           "to the save DC.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_BEACON, "Beacon of Faith", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You are a shining light of your faith, and your steadfast conviction allows others to draw "
           "strength from your example. Allies within 30 feet of you who can see you gain a +2 trait "
           "bonus on saving throws against fear effects.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_OATHBOUND, "Oathbound", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "You have sworn an oath to your deity to uphold the faith and defeat its enemies. Your fervent "
           "determination to see your faith prevail grants you great resolve. You gain a +2 trait bonus "
           "on caster level checks made to overcome spell resistance.",
           TRAIT_EFFECT_SPECIAL, 2, 0);

    traito(TRAIT_FATES_FAVORED, "Fate's Favored", TRUE, TRUE,
           "Faith", TRAIT_TYPE_FAITH, "-",
           "The fates watch over you. Whenever you are under the effect of a luck bonus of any kind, "
           "that bonus increases by 1.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    /* Social Traits */
    traito(TRAIT_FAST_TALKER, "Fast Talker", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You had a knack for getting yourself into trouble as a child, and as a result developed a "
           "silver tongue at an early age. You gain a +1 trait bonus on Bluff checks, and Bluff is "
           "always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_BLUFF);

    traito(TRAIT_CHARMING, "Charming", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "Blessed with good looks, you've come to depend on the fact that others find you attractive. "
           "You gain a +1 trait bonus when you use Bluff or Diplomacy on a character that is (or could be) "
           "sexually attracted to you, and a +1 trait bonus to all saving throws against charm effects.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_PERSUASIVE, "Persuasive", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You have a natural way with words and body language that makes you better at convincing others "
           "to see things your way. You gain a +1 trait bonus on Diplomacy checks.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_DIPLOMACY);

    traito(TRAIT_STUDENT_OF_PHILOSOPHY, "Student of Philosophy", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "In your youth, you studied philosophy and honed your skills at logic and oration. You can use "
           "your Intelligence modifier in place of your Charisma modifier on Diplomacy checks to persuade "
           "others and on Bluff checks to convince others that a lie is true. (This trait does not affect "
           "Diplomacy checks to gather information or Bluff checks to feint in combat.)",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_SUSPICIOUS, "Suspicious", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You discovered at an early age that someone you trusted, perhaps an older sibling or a parent, "
           "had lied to you, and lied often, about something you had taken for granted, leaving you quick "
           "to question the claims of others. You gain a +1 trait bonus on Sense Motive checks, and Sense "
           "Motive is always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_SENSE_MOTIVE);

    traito(TRAIT_RICH_PARENTS, "Rich Parents", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You were born into a rich family, perhaps even the nobility, and even though you've struck out "
           "on your own, you enjoy a one-time benefit to your initial finances—your starting wealth increases "
           "to 900 gp.",
           TRAIT_EFFECT_SPECIAL, 900, 0);

    traito(TRAIT_INFLUENCE, "Influence", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You have connections in high places, and these connections help to augment your powers of persuasion. "
           "Choose a specific government or faction. You gain a +1 trait bonus on Diplomacy checks while "
           "interacting with members or potential new members of that government or faction.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_NATURAL_BORN_LEADER, "Natural-Born Leader", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You were born to lead, and your presence inspires others to follow. You've always been good at "
           "bossing others around, and they seem to like you despite (or perhaps because of) it. You gain "
           "a +1 trait bonus to your Leadership score. Leadership is a special feat that you may gain later.",
           TRAIT_EFFECT_SPECIAL, 1, 0);

    traito(TRAIT_POVERTY_STRICKEN, "Poverty-Stricken", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "Your childhood was tough, and you had to scrounge for food and shelter. Though your starting "
           "wealth is halved, you've learned the value of a copper piece and have since developed a keen "
           "sense for finding ways to survive. You gain a +1 bonus on Survival checks.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_SURVIVAL);

    traito(TRAIT_MERCHANT, "Merchant", TRUE, TRUE,
           "Social", TRAIT_TYPE_SOCIAL, "-",
           "You come from a family of traders and merchants, and you've picked up a keen sense for value. "
           "You gain a +1 trait bonus on Appraise checks, and Appraise is always a class skill for you.",
           TRAIT_EFFECT_SKILL_BONUS, 1, SKILL_APPRAISE);
}

/**
 * Find a trait number by name
 * Returns trait number or -1 if not found
 */
int find_trait_num(const char *name)
{
    int i;

    for (i = 0; i <= NUM_TRAITS_DEFINED; i++) {
        if (is_abbrev(name, trait_list[i].name)) {
            return i;
        }
    }

    return -1;
}

/**
 * Check if character has a specific trait
 */
int has_trait(struct char_data *ch, int traitnum)
{
    if (!ch || traitnum < 0 || traitnum > NUM_TRAITS_DEFINED)
        return FALSE;

    return HAS_TRAIT(ch, traitnum);
}

/**
 * Get the category of the first trait selected
 * Returns category type or TRAIT_TYPE_UNDEFINED if no traits
 */
byte get_first_trait_category(struct char_data *ch)
{
    int i;

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i)) {
            return trait_list[i].category_type;
        }
    }

    return TRAIT_TYPE_UNDEFINED;
}

/**
 * Check if character meets trait prerequisites
 * Currently basic check - can be expanded for deity, race, class requirements
 */
int meets_trait_prerequisites(struct char_data *ch, int traitnum)
{
    char *prereq;

    if (!ch || traitnum < 0 || traitnum > NUM_TRAITS_DEFINED)
        return FALSE;

    prereq = trait_list[traitnum].prerequisites;

    /* No prerequisites */
    if (!strcmp(prereq, "-"))
        return TRUE;

    /* TODO: Add specific prerequisite checking here */
    /* For now, traits with prerequisites return FALSE */
    /* This can be expanded to check for deity, race, class, etc. */

    return TRUE;  /* Allow all traits for now */
}

/**
 * Check if character can select a trait
 * Validates: trait exists, is in game, meets prerequisites, different category
 */
int can_select_trait(struct char_data *ch, int traitnum, int selection_num)
{
    byte first_category;

    if (!ch || traitnum < 0 || traitnum > NUM_TRAITS_DEFINED)
        return FALSE;

    /* Check if trait is in game and can be learned */
    if (!trait_list[traitnum].in_game || !trait_list[traitnum].can_learn)
        return FALSE;

    /* Check if already selected */
    if (HAS_TRAIT(ch, traitnum))
        return FALSE;

    /* Check category restriction for second trait */
    if (selection_num == 2) {
        first_category = get_first_trait_category(ch);
        if (first_category == trait_list[traitnum].category_type)
            return FALSE;  /* Same category as first trait */
    }

    /* Check prerequisites */
    if (!meets_trait_prerequisites(ch, traitnum))
        return FALSE;

    return TRUE;
}

/**
 * Get trait skill bonus for a specific skill
 */
int get_trait_skill_bonus(struct char_data *ch, int skill)
{
    int bonus = 0, i;

    if (!ch)
        return 0;

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i) &&
            trait_list[i].effect_type == TRAIT_EFFECT_SKILL_BONUS &&
            trait_list[i].effect_specific == skill) {
            bonus += trait_list[i].effect_value;
        }
    }

    return bonus;
}

/**
 * Get trait save bonus for a specific save type
 */
int get_trait_save_bonus(struct char_data *ch, int save_type)
{
    int bonus = 0, i;

    if (!ch)
        return 0;

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i) &&
            trait_list[i].effect_type == TRAIT_EFFECT_SAVE_BONUS &&
            trait_list[i].effect_specific == save_type) {
            bonus += trait_list[i].effect_value;
        }
    }

    return bonus;
}

/**
 * Get trait initiative bonus
 */
int get_trait_initiative_bonus(struct char_data *ch)
{
    int bonus = 0, i;

    if (!ch)
        return 0;

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i) &&
            trait_list[i].effect_type == TRAIT_EFFECT_INITIATIVE) {
            bonus += trait_list[i].effect_value;
        }
    }

    return bonus;
}

/**
 * Get trait HP bonus
 */
int get_trait_hp_bonus(struct char_data *ch)
{
    int bonus = 0, i;

    if (!ch)
        return 0;

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i) &&
            trait_list[i].effect_type == TRAIT_EFFECT_HP) {
            bonus += trait_list[i].effect_value;
        }
    }

    return bonus;
}

/**
 * Get trait category name
 */
const char *get_trait_category_name(byte category_type)
{
    if (category_type >= NUM_TRAIT_TYPES)
        return "Unknown";

    return trait_types[category_type];
}

/**
 * Display traits known by character
 */
void list_traits_known(struct char_data *ch)
{
    int i, count = 0;
    char buf[MAX_STRING_LENGTH];

    if (!ch)
        return;

    send_to_char(ch, "Your Traits:\r\n");
    send_to_char(ch, "------------\r\n");

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (HAS_TRAIT(ch, i)) {
            snprintf(buf, sizeof(buf), "%s (%s)\r\n  %s\r\n\r\n",
                    trait_list[i].name,
                    trait_list[i].category,
                    trait_list[i].description);
            send_to_char(ch, "%s", buf);
            count++;
        }
    }

    if (count == 0) {
        send_to_char(ch, "You have no traits.\r\n");
    }
}

/**
 * Display all available traits
 */
void list_traits_complete(struct char_data *ch)
{
    int i;
    char buf[MAX_STRING_LENGTH];

    if (!ch)
        return;

    send_to_char(ch, "All Available Traits:\r\n");
    send_to_char(ch, "--------------------\r\n\r\n");

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (trait_list[i].in_game && trait_list[i].can_learn) {
            snprintf(buf, sizeof(buf), "%s (%s)\r\n  %s\r\n\r\n",
                    trait_list[i].name,
                    trait_list[i].category,
                    trait_list[i].description);
            send_to_char(ch, "%s", buf);
        }
    }
}

/**
 * Display available traits for selection
 */
void list_traits_available(struct char_data *ch, int selection_num)
{
    int i;
    byte exclude_cat = TRAIT_TYPE_UNDEFINED;
    char buf[MAX_STRING_LENGTH];

    if (!ch)
        return;

    /* For second selection, exclude first trait's category */
    if (selection_num == 2) {
        exclude_cat = get_first_trait_category(ch);
    }

    send_to_char(ch, "Available Traits:\r\n");
    send_to_char(ch, "----------------\r\n\r\n");

    for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
        if (can_select_trait(ch, i, selection_num)) {
            if (selection_num == 1 || trait_list[i].category_type != exclude_cat) {
                snprintf(buf, sizeof(buf), "%-3d. %s (%s)\r\n",
                        i, trait_list[i].name, trait_list[i].category);
                send_to_char(ch, "%s", buf);
            }
        }
    }

    send_to_char(ch, "\r\nType the number of the trait you wish to select.\r\n");
}

/**
 * Display trait introduction during character creation
 */
void display_trait_intro(struct descriptor_data *d)
{
    if (!d || !d->character)
        return;

    send_to_char(d->character,
        "\r\n"
        "================================ TRAITS ================================\r\n"
        "\r\n"
        "Traits represent your character's background, upbringing, or natural\r\n"
        "talents. You may select TWO traits during character creation. Traits\r\n"
        "provide small bonuses and cannot be changed later.\r\n"
        "\r\n"
        "Rules:\r\n"
        "  * You may select exactly 2 traits\r\n"
        "  * Both traits must be from DIFFERENT categories\r\n"
        "  * Traits are permanent once selected\r\n"
        "\r\n"
        "Categories:\r\n"
        "  * Combat - Bonuses to combat abilities\r\n"
        "  * Magic  - Bonuses to magical abilities\r\n"
        "  * Faith  - Bonuses related to divine power\r\n"
        "  * Social - Bonuses to social skills\r\n"
        "\r\n"
        "========================================================================\r\n"
        "\r\n");
}

/**
 * Display traits by category for selection
 */
void display_traits_by_category(struct char_data *ch, byte exclude_cat, int selection_num)
{
    int i, cat;
    char buf[MAX_STRING_LENGTH];

    if (!ch)
        return;

    if (selection_num == 1) {
        send_to_char(ch, "\r\nSelect your FIRST trait:\r\n\r\n");
    } else {
        send_to_char(ch, "\r\nSelect your SECOND trait (from a different category):\r\n\r\n");
    }

    /* Display traits organized by category */
    for (cat = 1; cat < NUM_TRAIT_TYPES; cat++) {
        if (cat == exclude_cat)
            continue;  /* Skip excluded category */

        snprintf(buf, sizeof(buf), "\r\n--- %s Traits ---\r\n", trait_types[cat]);
        send_to_char(ch, "%s", buf);

        for (i = 1; i <= NUM_TRAITS_DEFINED; i++) {
            if (trait_list[i].category_type == cat &&
                can_select_trait(ch, i, selection_num)) {
                snprintf(buf, sizeof(buf), "  %-3d. %-30s %s\r\n",
                        i, trait_list[i].name, trait_list[i].description);
                send_to_char(ch, "%s", buf);
            }
        }
    }

    send_to_char(ch, "\r\nEnter trait number or 'help <number>' for details: ");
}

/**
 * ACMD: traits command
 * Display trait information
 */
ACMD(do_traits)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg || is_abbrev(arg, "known")) {
        list_traits_known(ch);
    } else if (is_abbrev(arg, "all")) {
        list_traits_complete(ch);
    } else if (is_abbrev(arg, "help")) {
        send_to_char(ch,
            "Traits Command Help:\r\n"
            "  traits         - Show your selected traits\r\n"
            "  traits known   - Show your selected traits\r\n"
            "  traits all     - Show all available traits\r\n"
            "\r\n"
            "Traits are selected during character creation and cannot be changed.\r\n");
    } else {
        send_to_char(ch, "Usage: traits [known|all|help]\r\n");
    }
}
