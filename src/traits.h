/**
 * @file traits.h
 * @brief Pathfinder Trait System Header
 *
 * This file contains all trait-related constants, structures, and function
 * declarations for the Pathfinder trait system. Traits are character bonuses
 * selected at 1st level only during character creation.
 */

#ifndef _TRAITS_H_
#define _TRAITS_H_

/* Trait Categories */
#define TRAIT_TYPE_UNDEFINED    0
#define TRAIT_TYPE_COMBAT       1
#define TRAIT_TYPE_MAGIC        2
#define TRAIT_TYPE_FAITH        3
#define TRAIT_TYPE_SOCIAL       4
#define NUM_TRAIT_TYPES         5

/* Trait Effect Types */
#define TRAIT_EFFECT_NONE           0
#define TRAIT_EFFECT_SKILL_BONUS    1  /* Bonus to specific skill */
#define TRAIT_EFFECT_SAVE_BONUS     2  /* Bonus to Fort/Ref/Will */
#define TRAIT_EFFECT_INITIATIVE     3  /* Bonus to initiative */
#define TRAIT_EFFECT_HP             4  /* Bonus HP */
#define TRAIT_EFFECT_SPECIAL        5  /* Custom code required */

/* Individual Trait Definitions */
#define TRAIT_UNDEFINED         0

/* Combat Traits (1-15) */
#define TRAIT_REACTIONARY       1
#define TRAIT_ARMOR_EXPERT      2
#define TRAIT_KILLER            3
#define TRAIT_ANATOMIST         4
#define TRAIT_BRUISING_INTELLECT 5
#define TRAIT_COURAGEOUS        6
#define TRAIT_DEFENDER_OF_SOCIETY 7
#define TRAIT_FENCER            8
#define TRAIT_RESILIENT         9
#define TRAIT_RURAL             10
#define TRAIT_WEAPON_EXPERT     11
#define TRAIT_BULLY             12

/* Magic Traits (16-30) */
#define TRAIT_MAGICAL_KNACK     16
#define TRAIT_DANGEROUSLY_CURIOUS 17
// #define TRAIT_FOCUSED_MIND      18
#define TRAIT_GIFTED_ADEPT      19
#define TRAIT_HEDGE_MAGICIAN    20
#define TRAIT_MAGICAL_LINEAGE   21
#define TRAIT_MATHEMATICAL_PRODIGY 22
#define TRAIT_SKEPTIC           23
#define TRAIT_CLASSICALLY_SCHOOLED 24
#define TRAIT_FAST_LEARNER      25

/* Faith Traits (31-45) */
#define TRAIT_BLESSED           31
#define TRAIT_BIRTHMARK         32
#define TRAIT_SACRED_TOUCH      33
#define TRAIT_CHILD_OF_TEMPLE   34
#define TRAIT_DEVOTEE_OF_GREEN  35
#define TRAIT_INDOMITABLE_FAITH 36
#define TRAIT_SACRED_CONDUIT    37
#define TRAIT_BEACON            38
#define TRAIT_OATHBOUND         39
#define TRAIT_FATES_FAVORED     40

/* Social Traits (46-60) */
#define TRAIT_FAST_TALKER       46
#define TRAIT_CHARMING          47
#define TRAIT_PERSUASIVE        48
#define TRAIT_STUDENT_OF_PHILOSOPHY 49
#define TRAIT_SUSPICIOUS        50
#define TRAIT_RICH_PARENTS      51
#define TRAIT_INFLUENCE         52
#define TRAIT_NATURAL_BORN_LEADER 53
#define TRAIT_POVERTY_STRICKEN  54
#define TRAIT_MERCHANT          55

/* Total number of defined traits */
#define NUM_TRAITS_DEFINED      55

/* Save type constants for TRAIT_EFFECT_SAVE_BONUS */
#define TRAIT_SAVE_FORT         0
#define TRAIT_SAVE_REFLEX       1
#define TRAIT_SAVE_WILL         2

/* Maximum traits in system (for future expansion) */
#ifndef MAX_TRAITS
#define MAX_TRAITS 200
#endif

/* Trait Information Structure */
struct trait_info {
    char *name;              /* Display name */
    sbyte in_game;           /* Is implemented? */
    sbyte can_learn;         /* Can be selected? */
    char *category;          /* Category name for display */
    byte category_type;      /* TRAIT_TYPE_* constant */
    char *prerequisites;     /* Prerequisite text */
    char *description;       /* Full description */
    byte effect_type;        /* TRAIT_EFFECT_* constant */
    sh_int effect_value;     /* Numeric value (bonus amount) */
    sh_int effect_specific;  /* Which skill/save/stat affected */
};

/* External Declarations */
extern struct trait_info trait_list[NUM_TRAITS_DEFINED + 1];
extern int trait_sort_info[MAX_TRAITS + 1];
extern const char *trait_types[];

/* Function Prototypes */

/* Initialization */
void assign_traits(void);
void sort_traits(void);
void traito(int traitnum, char *name, sbyte in_game, sbyte can_learn,
            char *category, byte category_type, char *prerequisites,
            char *description, byte effect_type, sh_int effect_value,
            sh_int effect_specific);

/* Validation Functions */
int can_select_trait(struct char_data *ch, int traitnum, int selection_num);
int meets_trait_prerequisites(struct char_data *ch, int traitnum);
byte get_first_trait_category(struct char_data *ch);
int has_trait(struct char_data *ch, int traitnum);

/* Effect Calculation Functions */
int get_trait_skill_bonus(struct char_data *ch, int skill);
int get_trait_save_bonus(struct char_data *ch, int save_type);
int get_trait_initiative_bonus(struct char_data *ch);
int get_trait_hp_bonus(struct char_data *ch);

/* Utility Functions */
int find_trait_num(const char *name);
void list_traits_available(struct char_data *ch, int selection_num);
void list_traits_known(struct char_data *ch);
void list_traits_complete(struct char_data *ch);
const char *get_trait_category_name(byte category_type);

/* Display Functions */
void display_trait_intro(struct descriptor_data *d);
void display_traits_by_category(struct char_data *ch, byte exclude_cat, int selection_num);

#endif /* _TRAITS_H_ */
