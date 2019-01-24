#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"

// Local Functions

void award_special_magic_item(struct char_data *ch);
void assign_qp_value(struct obj_data *obj);
void get_random_essence(struct char_data *ch, int level);
void get_random_crystal(struct char_data *ch, int level);
void determine_treasure(struct char_data *ch, struct char_data *mob);
void award_masterwork_item(struct char_data *ch, int vnum);
void award_magic_item(int number, struct char_data *ch, struct char_data *mob, int grade);
void award_expendable_item(struct char_data *ch, int grade, int type);
void award_magic_weapon(struct char_data *ch, int grade, int moblevel);
int determine_random_weapon_type(void);
void award_magic_armor(struct char_data *ch, int grade, int moblevel);
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel);
int choose_metal_material(void);
int choose_cloth_material(void);


// Local constants
char *gemstones[];
char *ring_descs[];
char *wrist_descs[];
char *neck_descs[];
char *head_descs[];
char *hands_descs[];
char *cloak_descs[];
char *waist_descs[];
char *boot_descs[];
char *potion_descs[];
char *crystal_descs[];
char *colors[];
char *blade_descs[];
char *piercing_descs[];
char *blunt_descs[];
char *armor_special_descs[];
char *armor_crests[];

// External Constants
extern struct spell_info_type spell_info[SKILL_TABLE_SIZE];
extern const char * unused_spellname;
extern struct weapon_table weapon_list[];
extern char *handle_types[];
extern char *head_types[];
extern struct armor_table armor_list[];

// External Functions
SPECIAL(shop_keeper);
void set_weapon_values(struct obj_data *obj, int type);
int set_object_level(struct obj_data *obj);
char * change_coins(int coins);
void convert_coins(struct char_data *ch);
ACMD(do_split);
void set_armor_values(struct obj_data *obj, int type);

#define GRADE_MUNDANE 1
#define GRADE_MINOR   2
#define GRADE_MEDIUM  3
#define GRADE_MAJOR   4

#define TYPE_POTION 1
#define TYPE_SCROLL 2
#define TYPE_WAND   3
#define TYPE_STAFF  4

void determine_treasure(struct char_data *ch, struct char_data *mob)
{

  if (IS_NPC(ch) && (GET_MOB_VNUM(ch) > 64999 || GET_MOB_VNUM(ch) < 0))
    return;

  if (!IS_NPC(ch) && (GET_IDNUM(ch) > 30000 || GET_IDNUM(ch) < 0))
    return;


  int roll = 0;
  int factor = 30;
  int gold = 0;
  int level = GET_LEVEL(mob);
  char gold_buf[MAX_STRING_LENGTH]={'\0'};
  char buf[MAX_STRING_LENGTH]={'\0'};
  int grade = GRADE_MUNDANE;

  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
    ch = ch->master;

  gold = dice(GET_LEVEL(mob) * 2, GET_LEVEL(mob) * 5);

  if (IS_NPC(mob)) {
    if (MOB_FLAGGED(mob, MOB_BOSS)) {
      factor = 100;
      gold *= 5;
      gold /= 2;
    }
    else if (MOB_FLAGGED(mob, MOB_FINAL_BOSS)) {
      factor = 150;
      gold *= 5;
    }
    else if (MOB_FLAGGED(mob, MOB_CAPTAIN)) {
      factor = 80;
      gold *= 3;
      gold /= 2;
    }
    else if (MOB_FLAGGED(mob, MOB_LIEUTENANT))
      factor = 50;
  }
  else
    return;

  if (IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper) {
    factor = 1000;
    gold = 0;
  }

  gold *= (100 + GET_RP_GOLD_BONUS(ch));
  gold /= 100;
  
  factor += get_skill_value(ch, SKILL_PERCEPTION);

  if (GET_MOB_SPEC(mob) == shop_keeper)
    level = dice(1, GET_LEVEL(mob));

  roll = dice(1, 100);

  if (level >= 20) {
    grade = GRADE_MAJOR;
  }
  else if (level >= 16) {
    if (roll >= 61)
      grade = GRADE_MAJOR;
    else
      grade = GRADE_MEDIUM;
  }
  else if (level >= 12) {
    if (roll >= 81)
      grade = GRADE_MAJOR;
    else if (roll >= 11)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 8) {
    if (roll >= 96)
      grade = GRADE_MAJOR;
    else if (roll >= 31)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  }
  else if (level >= 4) {
    if (roll >= 76)
      grade = GRADE_MEDIUM;
    else if (roll >= 16)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }
  else {
    if (roll >= 96)
      grade = GRADE_MEDIUM;
    else if (roll >= 41)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }

  if (dice(1, 1000) <= (factor)) {
    award_magic_item(1, ch, mob, grade);


  roll = dice(1, 1000);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
      sprintf(buf, "@YYou have found %s coins on $N's corpse!@n", change_coins(gold));
      act(buf, FALSE, ch, 0, mob, TO_CHAR);
      sprintf(buf, "@Y$n has has found %s coins on $N's corpse!@n", change_coins(gold));
      act(buf, FALSE, ch, 0, mob, TO_NOTVICT);
//        GET_GOLD(ch) += gold;
        gain_gold(ch, gold, GOLD_ONHAND);
        convert_coins(ch);
      if (IS_AFFECTED(ch, AFF_GROUP) && (gold > 0) &&
        PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {  
        sprintf(gold_buf, "%d", gold);
        do_split(ch,gold_buf,0,0);
      }
    }  
  }
}

void award_magic_item(int number, struct char_data *ch, struct char_data *mob, int grade)
{

  if (!mob)
    mob = ch;

  if (dice(1, 1000) <= 1)
    award_special_magic_item(ch);
  if (dice(1, 100) <= 60)
    award_expendable_item(ch, grade, TYPE_POTION);
  if (dice(1, 100) <= 30)
    award_expendable_item(ch, grade, TYPE_SCROLL);
  if (dice(1, 100) <= 20)
    award_expendable_item(ch, grade, TYPE_WAND); 
  if (dice(1, 100) <= 10)
    award_expendable_item(ch, grade, TYPE_STAFF);
  if (dice(1, 100) <= 10)
    award_magic_weapon(ch, grade, GET_LEVEL(mob));
  if (dice(1, 100) <= 20)
    award_misc_magic_item(ch, grade, GET_LEVEL(mob));
  if (dice(1, 100) <= 10)
    award_magic_armor(ch, grade, GET_LEVEL(mob));
}

void award_masterwork_item(struct char_data *ch, int vnum)
{

  struct obj_data *obj = NULL;
  char buf[MAX_STRING_LENGTH]={'\0'};
  int sizeIncrease = 0;

  obj = read_object(vnum, VIRTUAL);

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  sprintf(buf, "%s masterwork", obj->name);
  obj->name =  strdup(buf);

  sprintf(buf, "a masterwork %s", obj->short_description + 2);
  obj->short_description = strdup(buf);

  sprintf(buf, "A masterwork %s", obj->description + 2);
  obj->description = strdup(buf);

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1;
    GET_OBJ_COST(obj) += 300;
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT) {
    GET_OBJ_VAL(obj, 3) = MIN(0, GET_OBJ_VAL(obj, 3) + 1);
    GET_OBJ_COST(obj) += 150;
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
    GET_OBJ_VAL(obj, 0) += (sizeIncrease = (dice(1, 20) * 5));
    GET_OBJ_COST(obj) += sizeIncrease * 2;
  }

  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  obj_to_char(obj, ch);

  send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
  
  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    sprintf(buf, "@Y$n has found %s!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }  

}

ACMD(do_loadmagic)
{

  char arg1[MAX_STRING_LENGTH]={'\0'};
  char arg2[MAX_STRING_LENGTH]={'\0'};

  int number = 1;
  int grade = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | medium | major] [# of items]\r\n");
    return;
  }
  
  if (*arg2 && !isdigit(arg2[0])) {
    send_to_char(ch, "The second number must be an integer.\r\n");
    return;
  }

  if (is_abbrev(arg1, "mundane"))
    grade = GRADE_MUNDANE;
  else if (is_abbrev(arg1, "minor"))
    grade = GRADE_MINOR;
  else if (is_abbrev(arg1, "medium"))
    grade = GRADE_MEDIUM;
  else if (is_abbrev(arg1, "major"))
    grade = GRADE_MAJOR;
  else {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | medium | major] [# of items]\r\n");
    return;
  }

  if (*arg2)
    number = atoi(arg2);

  award_magic_item(number, ch, NULL, grade);
 

}

void award_expendable_item(struct char_data *ch, int grade, int type)
{
//  return;

  int roll;
  int spelllevel = 0;
  int spelltype = 0;
  int numspells = 0;
  int i = 0, j = 0;
  int potion_desc = -1;
  int potion_color_1 = -1;
  int potion_color_2 = -1;
  struct obj_data *obj;
  int spellnum = 0;
  int size = 0;
  int scroll = FALSE;
  int wand = FALSE;
  int staff = FALSE;
  int potion = FALSE;
  int count = 0;
  char keywords[100]={'\0'};

  if (type == TYPE_STAFF)
    staff = TRUE;
  else if (type == TYPE_WAND)
    wand = TRUE;
  else if (type == TYPE_SCROLL)  
    scroll = TRUE;
  else
    potion = TRUE;


  char buf[MAX_STRING_LENGTH]={'\0'};

  roll = dice(1, 100);

  if (grade == GRADE_MINOR || grade == GRADE_MUNDANE) {

    if (roll >= 91)
      spelllevel = 3;
    else if (roll >= 61)
      spelllevel = 2;
    else
      spelllevel = 1;

  }
  else if (grade == GRADE_MEDIUM) {

    if (roll >= 91)
      spelllevel = 6;
    else if (roll >= 81)
      spelllevel = 5;
    else if (roll >= 61)
      spelllevel = 4;
    else if (roll >= 41)
      spelllevel = 3;
    else if (roll >= 21)
      spelllevel = 2;
    else
      spelllevel = 1;

  }
  else {

    if (roll >= 91)
      spelllevel = 9;
    else if (roll >= 81)
      spelllevel = 8;
    else if (roll >= 61)
      spelllevel = 7;
    else if (roll >= 41)
      spelllevel = 6;
    else if (roll >= 21)
      spelllevel = 5;
    else
      spelllevel = 4;

  }

  roll = dice(1, 100);


  if (potion) {
  if (roll >= 91)
    spelltype = MAG_CREATIONS;
  else if (roll >= 81)
    spelltype = MAG_SUMMONS;
  else if (roll >= 61)
    spelltype = MAG_UNAFFECTS;
  else if (roll >= 31)
    spelltype = MAG_AFFECTS;
  else
    spelltype = MAG_POINTS;
  }
  else {
  if (roll >= 96)
    spelltype = MAG_CREATIONS;
  else if (roll >= 91)
    spelltype = MAG_SUMMONS;
  else if (roll >= 81)
    spelltype = MAG_UNAFFECTS;
  else if (roll >= 46)
    spelltype = MAG_AFFECTS;
  else if (roll >= 36)
    spelltype = MAG_DAMAGE;
  else if (roll >= 31)
    spelltype = MAG_LOOP;
  else if (roll >= 21)
    spelltype = MAG_AREAS;
  else
    spelltype = MAG_POINTS;
  }

  for (i = 0; i <= TOP_SPELL; i++) {
    if (IS_SET(spell_info[i].routines, spelltype) && spell_info[i].spell_level == (int) spelllevel) {
      if (potion && spell_info[i].save_flags != 0)
        continue;
      if (!strcmp(spell_info[i].name, (char *) unused_spellname))
        continue;
      numspells++;
    }
  }

  if (numspells == 0) {
    award_expendable_item(ch, grade, type);
    return;
  }

  roll = dice(1, numspells);

  for (i = 0; i <= MAX_SPELLS; i++) {
    for (j = 0; j < NUM_CLASSES; j++) {
      if (spell_info[i].class_level[j] < 99) {
        if (IS_SET(spell_info[i].routines, spelltype) && spell_info[i].spell_level == spelllevel &&
          strcmp(spell_info[i].name, unused_spellname)) {
          if (potion && spell_info[i].save_flags != 0)
            continue;
          count++;
        }
      }
      if (roll == count)
        spellnum = i;
    } 
  }

  if (spellnum <= 0 || spellnum > SKILL_TABLE_SIZE) {
    award_expendable_item(ch, grade, type);
    return;
  }

  for (j = 0; j < NUM_CLASSES; j++)
    if (spell_info[spellnum].class_level[j] < 99)
      break;

  if (j == NUM_CLASSES) {
    award_expendable_item(ch, grade, type);
    return;
  }

  i = 0;
  while (*(colors + i++)) { /* counting array */ }
  size = i;
  potion_color_1 = MAX(1, dice(1, (int) size) - 1);
  potion_color_2 = MAX(1, dice(1, (int) size) - 1);
  while (potion_color_2 == potion_color_1)
    potion_color_2 = MAX(1, dice(1, (int) size) - 1);
  i = 0;
  while (*(potion_descs + i++)) { /* counting array */ }
  size = i;
  potion_desc = MAX(1, dice(1, (int) size) - 1);

  obj = read_object(30099, VIRTUAL);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  roll = dice(1, 100);

	if (potion) {

  if (roll >= 91) { // two colors and descriptor
    sprintf(keywords, "potion-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';
    sprintf(buf, "vial potion %s %s %s %s %s", colors[potion_color_1 - 1], colors[potion_color_2 - 1], potion_descs[potion_desc - 1], spell_info[spellnum].name, keywords);    
    obj->name = strdup(buf);
    sprintf(buf, "a glass vial filled with a %s, %s and %s liquid", potion_descs[potion_desc - 1], colors[potion_color_1 - 1], 
                                                                    colors[potion_color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A glass vial filled with a %s, %s and %s liquid lies here.",
						potion_descs[potion_desc - 1], colors[potion_color_1 - 1], 
            colors[potion_color_2 - 1]);
    obj->description = strdup(buf);
        
  }
  else if (roll >= 66) { // one color and descriptor
    sprintf(keywords, "potion-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';
    sprintf(buf, "vial potion %s %s %s %s", colors[potion_color_1 - 1], potion_descs[potion_desc - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a glass vial filled with a %s %s liquid", potion_descs[potion_desc - 1], colors[potion_color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A glass vial filled with a %s and %s liquid lies here.", potion_descs[potion_desc - 1], colors[potion_color_1 - 1]);
    obj->description = strdup(buf);
  }
  else if (roll >= 41) { // two colors no descriptor
    sprintf(keywords, "potion-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';
    sprintf(buf, "vial potion %s %s %s %s", colors[potion_color_1 - 1], colors[potion_color_2 - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a glass vial filled with a %s and %s liquid", colors[potion_color_1 - 1], colors[potion_color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A glass vial filled with a %s and %s liquid lies here.", colors[potion_color_1 - 1], colors[potion_color_2 - 1]);
    obj->description = strdup(buf);
  }
  else {// one color no descriptor
    sprintf(keywords, "potion-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';
    sprintf(buf, "vial potion %s %s %s", colors[potion_color_1 - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a glass vial filled with a %s liquid", colors[potion_color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A glass vial filled with a %s liquid lies here.", colors[potion_color_1 - 1]);
    obj->description = strdup(buf);
  }


  GET_OBJ_MATERIAL(obj) = MATERIAL_GLASS;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  GET_OBJ_COST(obj) = spelllevel * 25 * (MAX(1, spelllevel * MAX(2, spelllevel / 2)));

  GET_OBJ_VAL(obj, 1) = spellnum;

  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  GET_OBJ_LEVEL(obj) = GET_OBJ_VAL(obj, VAL_POTION_LEVEL);

  GET_OBJ_VAL(obj, VAL_POTION_LEVEL) = 20;

  } else if (scroll) {

    sprintf(keywords, "scroll-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    sprintf(buf, "scroll ink %s %s %s", colors[potion_color_1 - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a scroll written in %s ink", colors[potion_color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A scroll written in %s ink lies here.", colors[potion_color_1 - 1]);
    obj->description = strdup(buf);

    GET_OBJ_MATERIAL(obj) = MATERIAL_PAPER;
        
    GET_OBJ_TYPE(obj) = ITEM_SCROLL;

    GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL) = MAX(1, ((spell_info[spellnum].spell_level - 1) * 2) + 1);

    GET_OBJ_COST(obj) = 10;

    for (count = 1; count < spelllevel; count++)
      GET_OBJ_COST(obj) *= 3;
    

    GET_OBJ_VAL(obj, 1) = spellnum;

    GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

    GET_OBJ_LEVEL(obj) = GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL);

    GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL) = 20;


  }
  else if (wand) {
    sprintf(keywords, "wand-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    sprintf(buf, "wand wooden runes %s %s %s", colors[potion_color_1 - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a wooden wand covered in %s runes", colors[potion_color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A wooden wand covered in %s runes lies here.", colors[potion_color_1 - 1]);
    obj->description = strdup(buf);

    GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
        
    GET_OBJ_TYPE(obj) = ITEM_WAND;

    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);

    GET_OBJ_VAL(obj, VAL_WAND_LEVEL) = MAX(1, ((spell_info[spellnum].spell_level - 1) * 2) + 1);

    GET_OBJ_COST(obj) = 100;

    for (count = 1; count < spelllevel; count++)
      GET_OBJ_COST(obj) *= 3;

    GET_OBJ_VAL(obj, VAL_WAND_SPELL) = spellnum;

    GET_OBJ_VAL(obj, VAL_WAND_CHARGES) = dice(1, 50);
    GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES) = 50;

    GET_OBJ_COST(obj) *= GET_OBJ_VAL(obj, VAL_WAND_CHARGES);

    GET_OBJ_COST(obj) /= 25;

    GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

    GET_OBJ_LEVEL(obj) = GET_OBJ_VAL(obj, VAL_WAND_LEVEL);

    GET_OBJ_VAL(obj, VAL_WAND_LEVEL) = 20;


  }
  else if (staff) {
    sprintf(keywords, "staff-%s", spell_info[spellnum].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    sprintf(buf, "staff wooden runes %s %s %s", colors[potion_color_1 - 1], spell_info[spellnum].name, keywords);
    obj->name = strdup(buf);
    sprintf(buf, "a wooden staff covered in %s runes", colors[potion_color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A wooden staff covered in %s runes lies here.", colors[potion_color_1 - 1]);
    obj->description = strdup(buf);

    GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
        
    GET_OBJ_TYPE(obj) = ITEM_STAFF;

    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);

    GET_OBJ_VAL(obj, VAL_STAFF_LEVEL) = MAX(1, ((spell_info[spellnum].spell_level - 1) * 2) + 1);

    GET_OBJ_COST(obj) = 250;

    for (count = 1; count < spelllevel; count++)
      GET_OBJ_COST(obj) *= 3;
    

    GET_OBJ_VAL(obj, VAL_STAFF_SPELL) = spellnum;

    GET_OBJ_VAL(obj, VAL_STAFF_CHARGES) = dice(1, 20);
    GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES) = 50;

    GET_OBJ_COST(obj) *= GET_OBJ_VAL(obj, VAL_STAFF_CHARGES);

    GET_OBJ_COST(obj) /= 10;

    GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

    GET_OBJ_LEVEL(obj) = GET_OBJ_VAL(obj, VAL_STAFF_LEVEL);

    GET_OBJ_VAL(obj, VAL_STAFF_LEVEL) = 20;

  }

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}

void award_magic_armor(struct char_data *ch, int grade, int moblevel) {

  int i = 0;
  int roll = 0;
  int type = 100;
  int rand = 0;
  int shield = FALSE;
  int robes = FALSE;
  int material = MATERIAL_STEEL;
  int desc1 = 0, desc2 = 0, desc3 = 0;
  char desc[MAX_STRING_LENGTH]={'\0'};
  char newDesc[MAX_STRING_LENGTH]={'\0'};
  char keywords[MAX_STRING_LENGTH]={'\0'};
  int crest = 0, color1 = 0, color2 = 0, armor_special_desc_roll = 0;
  int size = 0;
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH]={'\0'};

  int ac_bonus = 0;

  // Check power of magic item and assign an ac bonus value to the variable.

  if (grade == GRADE_MUNDANE) {
    ac_bonus = 0;
  }
  else if (grade == GRADE_MINOR) {
    ac_bonus = dice(1, 5) * 5;
  }
  else if (grade == GRADE_MEDIUM) {
    ac_bonus = (4 + dice(1, 4)) * 5;
  }
  else if (grade == GRADE_MAJOR) {
    ac_bonus = (6 + dice(1, 6)) * 5;
    moblevel -= 20;
    if (moblevel > 0)
      ac_bonus += 3 * moblevel;
  }

  // Find out if it's a shield or armor

  roll = dice(1, 100);

  if (roll <= 50) {
	  for (i = 0; i < 10; i++)
		  if (ch->player_specials->wishlist[i][0] == 2)
			  roll = 0;
	  if (roll == 0) {
                roll = dice(1, 10) - 1;
		while (ch->player_specials->wishlist[roll][0] != 2) {
			roll = dice(1, 10) - 1;
   		}
		type = ch->player_specials->wishlist[roll][1];
		if (type >= 13)
			  shield = TRUE;
	  }
  }
  else {
	  roll = dice(1, 80);

	  if (roll <= 33)
		  shield = TRUE;
  }

  // If it's armor find out what type it is

  if (!shield) {
    roll = dice(1, 100);

    if ((type == 100 && roll <= 10) || type == SPEC_ARMOR_TYPE_CLOTHING) {
      type = SPEC_ARMOR_TYPE_CLOTHING;
      roll = dice(1, 100);
      if (roll <= 25)
        material = MATERIAL_VELVET;
      else if (roll <= 25)
        material = MATERIAL_WOOL;
      else
        material = MATERIAL_COTTON;

    }
    else {
      roll = dice(1, 100);

      if ((type == 100 && roll <= 8) || type == SPEC_ARMOR_TYPE_PADDED) {
        type = SPEC_ARMOR_TYPE_PADDED;
        material = MATERIAL_COTTON;
      }
      if ((type == 100 && roll <= 16) || type == SPEC_ARMOR_TYPE_LEATHER) {
        type = SPEC_ARMOR_TYPE_LEATHER;
        material = MATERIAL_LEATHER;
      }
      if ((type == 100 && roll <= 24) || type == SPEC_ARMOR_TYPE_HIDE) {
        type = SPEC_ARMOR_TYPE_HIDE;
        material = MATERIAL_LEATHER;
      }
      if ((type == 100 && roll <= 32) || type == SPEC_ARMOR_TYPE_STUDDED_LEATHER) {
        type = SPEC_ARMOR_TYPE_STUDDED_LEATHER;
        material = MATERIAL_LEATHER;
      }
      if ((type == 100 && roll <= 40) || type == SPEC_ARMOR_TYPE_LIGHT_CHAIN)
    	  type = SPEC_ARMOR_TYPE_LIGHT_CHAIN;
      else if ((type == 100 && roll <= 48) || type == SPEC_ARMOR_TYPE_SCALE)
        	type = SPEC_ARMOR_TYPE_SCALE;
      else if ((type == 100 && roll <= 56) || type == SPEC_ARMOR_TYPE_CHAINMAIL)
        	type = SPEC_ARMOR_TYPE_CHAINMAIL;
      else if ((type == 100 && roll <= 64) || type == SPEC_ARMOR_TYPE_PIECEMEAL)
        	type = SPEC_ARMOR_TYPE_PIECEMEAL;
      else if ((type == 100 && roll <= 72) || type == SPEC_ARMOR_TYPE_SPLINT)
        	type = SPEC_ARMOR_TYPE_SPLINT;
      else if ((type == 100 && roll <= 80) || type == SPEC_ARMOR_TYPE_BANDED)
        	type = SPEC_ARMOR_TYPE_BANDED;
      else if ((type == 100 && roll <= 88) || type == SPEC_ARMOR_TYPE_HALF_PLATE)
        	type = SPEC_ARMOR_TYPE_HALF_PLATE;
      else if ((type == 100 && roll > 88) || type == SPEC_ARMOR_TYPE_FULL_PLATE)
        	type = SPEC_ARMOR_TYPE_FULL_PLATE;
    }
  }
  // Otherwise it's a shield so find out what type of shield

  else {
    roll = dice(1, 100);

    if ((type == 100 && roll <= 10) || type == SPEC_ARMOR_TYPE_BUCKLER)
      type = SPEC_ARMOR_TYPE_BUCKLER;
    else if ((type == 100 && roll <= 20) || type == SPEC_ARMOR_TYPE_SMALL_SHIELD) {
      type = SPEC_ARMOR_TYPE_SMALL_SHIELD;
      material = MATERIAL_WOOD;
    }
    else if ((type == 100 && roll <= 30) || type == SPEC_ARMOR_TYPE_SMALL_SHIELD)
      type = SPEC_ARMOR_TYPE_SMALL_SHIELD;
    else if ((type == 100 && roll <= 50) || type == SPEC_ARMOR_TYPE_LARGE_SHIELD) {
      type = SPEC_ARMOR_TYPE_LARGE_SHIELD;
      material = MATERIAL_WOOD;
    }
    else if ((type == 100 && roll <= 80) || type == SPEC_ARMOR_TYPE_LARGE_SHIELD)
      type = SPEC_ARMOR_TYPE_LARGE_SHIELD;
    else if ((type == 100 && roll > 80) || type == SPEC_ARMOR_TYPE_TOWER_SHIELD)
      type = SPEC_ARMOR_TYPE_TOWER_SHIELD;
  }

  // If it's made of metal find out what kind of metal

  if (material == MATERIAL_STEEL) {
    roll = dice(1, 100);

    if (roll >= 97) {
     material = MATERIAL_ADAMANTINE;
    }
    else if (roll >= 90) {
      material = MATERIAL_MITHRIL;
    }
    if (roll >= 87) {
     material = MATERIAL_DRAGONHIDE;
    }
  }
  else if (material == MATERIAL_LEATHER) {
    roll = dice(1, 100);

    if (roll >= 97) {
     material = MATERIAL_DRAGONHIDE;
    }
    else if (roll >= 90) {
      material = MATERIAL_LEATHER;
    }
  }

  if (shield) {
    if (material == MATERIAL_STEEL && dice(1, 10) <= 4)
      material = MATERIAL_WOOD;
    else if (material == MATERIAL_STEEL && dice (1, 5) == 1)
      material = MATERIAL_DARKWOOD;
    else if (material == MATERIAL_STEEL && dice(1, 5) == 1)
      material = MATERIAL_DRAGONHIDE;
  }
  // Set the base description

  // If it's a shield we need to check the first letter of the desc later to see whether
  // we need to start with "a" or "an".  Otherwise it's armor which has a static desc start

  int rare = dice(1, 100);
  int raregrade = 0;

  if (rare == 1) {
    raregrade = 3;
  } else if (rare <= 6) {
    raregrade = 2;
  } else if (rare <= 16) {
    raregrade = 1;
  }

  if (shield)  {
    if (raregrade == 0)
      sprintf(desc, "a");
    else if (raregrade == 1)
      sprintf(desc, "@G[Rare]@n a");
    else if (raregrade == 2)
      sprintf(desc, "@Y[Legendary]@n a");
    else if (raregrade == 3)
      sprintf(desc, "@M[Mythical]@n a");
  }
  else {
    if (raregrade == 0)
      sprintf(desc, "a suit of");
    else if (raregrade == 1)
      sprintf(desc, "@G[Rare]@n a suit of");
    else if (raregrade == 2)
      sprintf(desc, "@Y[Legendary]@n a suit of");
    else if (raregrade == 3)
      sprintf(desc, "@M[Mythical]@n a suit of");
  }


  // Find out if there's an armor special adjective in the desc

  desc1 = dice(1, 3);

  // There's an armor special adjective in the desc so find out which one

  if (desc1 == 3) {
    i = 0;
    while (*(armor_special_descs + i++)) { /* counting array */ }
    size = i;
    armor_special_desc_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s %s", desc, armor_special_descs[armor_special_desc_roll]);
  }

  // Find out if there's a color describer in the desc

  desc2 = dice(1, 5);

  // There's one color describer in the desc so find out which one

  if (desc2 == 3 || desc2 == 4) {
    i = 0;
    while (*(colors + i++)) { /* counting array */ }
    size = i;
    color1 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s", desc, desc1 == 3 ? "," : "", colors[color1]);
  }

  // There's two colors describer in the desc so find out which one

  if (desc2 == 5) {
    i = 0;
    while (*(colors + i++)) { /* counting array */ }
    size = i;
    color1 = MAX(0, dice(1, (int) size) - 2);
    color2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s and %s", desc, desc1 == 3 ? "," : "", colors[color1], colors[color2]);
  }

  // Insert the material type

  sprintf(desc, "%s %s", desc, material_names[material]);

  // Insert the armor type

  if (type == SPEC_ARMOR_TYPE_CLOTHING && dice(1,2) == 1) {
    sprintf(desc, "%s robes", desc);
    robes = TRUE;
  }
  else 
    sprintf(desc, "%s %s", desc, armor_list[type].name);


  // Find out if the armor has any crests or symbols

  desc3 = dice(1, 8);

  // It has a crest so find out which and set the desc

  if (desc3 >= 7) {
    i = 0;
    while (*(armor_crests + i++)) { /* counting array */ }
    size = i;
    crest = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s with %s %s crest", desc, AN(armor_crests[crest]), armor_crests[crest]);
  }

  // It has a symbol so find out which and set the desc

  else if (desc3 >= 5) {
    i = 0;
    while (*(armor_crests + i++)) { /* counting array */ }
    size = i;
    crest = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s covered in symbols of %s %s", desc, AN(armor_crests[crest]), armor_crests[crest]);
  }


  // If it's a shield set the first word as a or an depending on a starting vowel of the following
  // Then read a default shield item from the mud to use for base stats
  // Otherwise read a default armor suit item from the mud to use for base stats

  if (shield) {
    sprintf(newDesc, "Null");
    for (i = 2; desc[i]; i++) {
      newDesc[i-2] = desc[i];      
    }
    newDesc[i - 2] = '\0';
    sprintf(desc, "%s %s", AN(newDesc), newDesc);

  }

  if (shield) {
    obj = read_object(30012, VIRTUAL);
  }
  else
    obj = read_object(30000, VIRTUAL);

  // if the object is null end the armor award

  if (!obj) 
    return;

  // Set the default armor values by the type

  set_armor_values(obj, type);

  if (type == SPEC_ARMOR_TYPE_CLOTHING)
    GET_OBJ_TYPE(obj) = ITEM_WORN;

  // Set the armor material

  GET_OBJ_MATERIAL(obj) = material;  


  // Set the item as unique so it will save all unique stats and not as the base vnum obj

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  // If there's an ac bonus set the applies for it

  if (ac_bonus) {
    if (!shield)
      obj->affected[0].location = APPLY_AC_ARMOR;
    else
      obj->affected[0].location = APPLY_AC_SHIELD;
    obj->affected[0].modifier = ac_bonus;
  }

  // Set descriptions

  sprintf(keywords, "%s %s %s %s %s %s", material_names[material], color1 ? colors[color1] : "", color2 ? 
                     colors[color2] : "", robes ? "robes" : armor_list[type].name, shield ? "shield" : "suit", crest ?
                     armor_crests[crest] : "");

  obj->name = strdup(keywords);

  obj->short_description = strdup(desc);

  desc[0] = toupper(desc[0]);

  sprintf(desc, "%s is lying here.", desc);

  obj->description = strdup(desc);


  GET_OBJ_VAL(obj, 9) = type;

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  if (GET_OBJ_LEVEL(obj) >= CONFIG_LEVEL_CAP) {
    award_magic_armor(ch, grade, moblevel);
    return;
  }

  obj->affected[0].modifier += (raregrade * 10);

  GET_OBJ_COST(obj) = 100 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1) + armor_list[type].cost;

  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (raregrade*2)) / 3;

  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  rand = dice(1, 100);
  while (rand == 100) {
    rand = dice(1, 100);
    obj->affected[0].modifier += 10;
  }
  
  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
  
    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }

}

void award_magic_weapon(struct char_data *ch, int grade, int moblevel) {

  int i = 0;
  int roll = 0;
  int size = 0;
  int hit_bonus = 0;
  int dmg_bonus = 0;
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH]={'\0'};
  int special_roll = 0;
  int head_color_roll = 0;
  int hilt_color_roll = 0;
  char special[100]={'\0'};
  char head_color[100]={'\0'};
  char hilt_color[100]={'\0'};

  if (grade == GRADE_MUNDANE) {
    hit_bonus = 1;
    dmg_bonus = 0;
  }
  else if (grade == GRADE_MINOR) {
    roll = dice(1, 100);

    if (roll >= 86) {
      award_magic_weapon(ch, grade, moblevel);      
      return;
    }
    else if (roll >= 71) {
      hit_bonus = 2;
      dmg_bonus = 2;
    }
    else if (roll >= 51) {
      hit_bonus = dice(1, 2);
      dmg_bonus = dice(1, 2);
    }
    else {
      hit_bonus = 1;
      dmg_bonus = 1;
    }
  }
  else if (grade == GRADE_MEDIUM) {
    roll = dice(1, 100);

    if (roll >= 69) {
      award_magic_weapon(ch, grade, moblevel);      
      return;
    }
    else if (roll >= 63) {
      award_magic_weapon(ch, grade, moblevel);      
      return;
    }
    else if (roll >= 59) {
      hit_bonus = 4;
      dmg_bonus = 4;
    }
    else if (roll >= 51) {
      hit_bonus = dice(1, 2) + 2;
      dmg_bonus = dice(1, 2) + 2;
    }
    else if (roll >= 21) {
      hit_bonus = 3;
      dmg_bonus = 3;
    }
    else if (roll >= 16) {
      hit_bonus = dice(1, 2) + 1;
      dmg_bonus = dice(1, 2) + 1;
    }
    else if (roll >= 11) {
      hit_bonus = 2;
      dmg_bonus = 2;
    }
    else if (roll >= 6) {
      hit_bonus = dice(1, 2);
      dmg_bonus = dice(1, 2);
    }
    else {
      hit_bonus = 1;
      dmg_bonus = 1;
    }    
  }
  else {
    roll = dice(1, 100);

    if (roll >= 64) {
      award_magic_weapon(ch, grade, moblevel);      
      return;
    }
    else if (roll >= 50) {
      award_magic_weapon(ch, grade, moblevel);      
      return;
    }
    else if (roll >= 39) {
      hit_bonus = 5;
      dmg_bonus = 5;
    }
    else if (roll >= 31) {
      hit_bonus = dice(1, 2) + 3;
      dmg_bonus = dice(1, 2) + 3;
    }
    else if (roll >= 21) {
      hit_bonus = 4;
      dmg_bonus = 4;
    }
    else if (roll >= 11) {
      hit_bonus = dice(1, 2) + 2;
      dmg_bonus = dice(1, 2) + 2;
    }
    else {
      hit_bonus = 3;
      dmg_bonus = 3;
    }
    moblevel -= 20;
    if (moblevel > 0) {
      hit_bonus += moblevel / 4;
      dmg_bonus += moblevel / 4;
    }
  }
  
  obj = read_object(30020, VIRTUAL);

  roll = dice(1, 100);

  if (roll <= 50) {
	  for (i = 0; i < 10; i++)
		  if (ch->player_specials->wishlist[i][0] == 1)
			  roll = 0;
	  if (roll == 0) {
                roll = dice(1, 10) -1 ;
		while (ch->player_specials->wishlist[roll][0] != 1) {
			roll = dice(1, 10) - 1;
   		}
		roll = ch->player_specials->wishlist[roll][1];
	  }
  }
  else
	  roll = determine_random_weapon_type();



  set_weapon_values(obj, roll);

  if (roll == WEAPON_TYPE_UNARMED)
    GET_OBJ_MATERIAL(obj) = MATERIAL_LEATHER;

  if (GET_OBJ_MATERIAL(obj) == MATERIAL_STEEL) {
    roll = dice(1, 100);

    if (roll >= 97) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_ADAMANTINE;
    }
    else if (roll >= 90) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_MITHRIL;
    }
    else if (roll >= 81) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_ALCHEMICAL_SILVER;
    }
    else if (roll >= 71) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_COLD_IRON;
    }
    else {
      GET_OBJ_MATERIAL(obj) = MATERIAL_STEEL;
    }
  }
  else if (GET_OBJ_MATERIAL(obj) == MATERIAL_WOOD) {
    roll = dice(1, 100);

    if (roll >= 90) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_DARKWOOD;
    }
    else {
      GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
    }
  }

  int rare = dice(1, 100);
  int raregrade = 0;

  if (rare == 1) {
    raregrade = 3;
  } else if (rare <= 6) {
    raregrade = 2;
  } else if (rare <= 16) {
    raregrade = 1;
  }

  char desc[50]={'\0'};
  if (raregrade == 0)
    sprintf(desc, "@n");
  else if (raregrade == 1)
    sprintf(desc, "@G[Rare]@n ");
  else if (raregrade == 2)
    sprintf(desc, "@Y[Legendary]@n ");
  else if (raregrade == 3)
    sprintf(desc, "@M[Mythical]@n ");


  i = 0;
  while (*(colors + i++)) { /* counting array */ }
  size = i;
  head_color_roll = MAX(0, dice(1, (int) size) - 2);
  hilt_color_roll = MAX(0, dice(1, (int) size) - 2);
  sprintf(head_color, "%s", colors[head_color_roll]);
  sprintf(hilt_color, "%s", colors[hilt_color_roll]);

  if (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_SLASHING)) {
    i = 0;
    while (*(blade_descs + i++)) { /* counting array */ }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, blade_descs[special_roll]);
  }
  else if (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_PIERCING)) {
    i = 0;
    while (*(piercing_descs + i++)) { /* counting array */ }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, piercing_descs[special_roll]);
  }
  else {
    i = 0;
    while (*(blunt_descs + i++)) { /* counting array */ }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, blunt_descs[special_roll]);
  }

  roll = dice(1, 100);

  // special, head color, hilt color
  if (roll >= 91) {

    sprintf(buf, "%s %s-%s %s %s %s %s", special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s", a_or_an(special), special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s lies here.", a_or_an(special), 
            special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }
  // special, head color
  else if (roll >= 81) {

    sprintf(buf, "%s %s-%s %s %s", special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);    
    obj->name = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s", a_or_an(special), special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s lies here.", a_or_an(special), 
            special, 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }
  // special, hilt color
  else if (roll >= 71) {

    sprintf(buf, "%s %s %s %s %s", special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s %s with %s %s %s", a_or_an(special), special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s %s with %s %s %s lies here.", a_or_an(special), 
            special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);


  }
  // head color, hilt color
  else if (roll >= 41) {

    sprintf(buf, "%s-%s %s %s %s %s",
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s with %s %s %s", a_or_an(head_color), 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s with %s %s %s lies here.", a_or_an(head_color), 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }
  // head color
  else if (roll >= 31) {

    sprintf(buf, "%s-%s %s %s", 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);    
    obj->name = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s", a_or_an(head_color),
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s lies here.", a_or_an(head_color), 
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type], 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);


  }
  // hilt color
  else if (roll >= 21) {

    sprintf(buf, "%s %s %s %s",
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s with %s %s %s", a_or_an((char *)material_names[GET_OBJ_MATERIAL(obj)]), 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s with %s %s %s lies here.", 
            a_or_an((char *)material_names[GET_OBJ_MATERIAL(obj)]), 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }
  // special
  else if (roll >= 11) {

    sprintf(buf, "%s %s %s", special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s %s", a_or_an(special), special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s %s lies here.", a_or_an(special), special, 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }
  // none
  else {

    sprintf(buf, "%s %s",
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s", a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]), 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s lies here.", 
            a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]), 
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
		*buf = UPPER(*buf);
    obj->description = strdup(buf);

  }

  obj->name = strdup(replace_string(obj->name, "unarmed", "gauntlet"));
  obj->short_description = strdup(replace_string(obj->short_description, "unarmed", "gauntlet"));
  obj->description = strdup(replace_string(obj->description, "unarmed", "gauntlet"));

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  if (dmg_bonus) {
    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = dmg_bonus;
  }
  
  if (hit_bonus) {
    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = hit_bonus;
  }

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  if (GET_OBJ_LEVEL(obj) >= CONFIG_LEVEL_CAP) {
    award_magic_weapon(ch, grade, moblevel);
    return;
  }


  obj->affected[0].modifier += raregrade;
  obj->affected[1].modifier += raregrade;

  GET_OBJ_COST(obj) = 250 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);

  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (raregrade * 2)) / 3;

  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);


  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
  
    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}

void award_misc_magic_item(struct char_data *ch, int grade, int moblevel) {

  byte bonus = 0;
  byte roll = 0;
  byte roll2 = 0;
  int type = 0;
  sbyte jewelry = FALSE;
  int affect = 0;
  int subval = 0;
  byte misc_desc_roll_1 = 0;
  byte misc_desc_roll_2 = 0;
  byte misc_desc_roll_3 = 0;
  byte misc_desc_roll_4 = 0;
  int material = 0;
  struct obj_data *obj;
  int i = 0;
  int size = 0;
  char desc[200]={'\0'};
  
  // Figure out which item grade power it is and then find out the bonus of the effect on the item
  
  roll = dice(1, 100);
  
  switch (grade) {
  case GRADE_MUNDANE:
    bonus = 1;
    break;
  case GRADE_MINOR:
    if (roll <= 70)
	  bonus = 1;
	else
	  bonus = 2;
    break;  
  case GRADE_MEDIUM:
    if (roll <= 60)
	  bonus = 2;
    else if (roll <= 90)
	  bonus = 3;	  
	else
	  bonus = 4;
    break;
  case GRADE_MAJOR:
    if (roll <= 40)
	  bonus = 3;
    else if (roll <= 70)
	  bonus = 4;	  
    else if (roll <= 90)
	  bonus = 5;	  
    else
      bonus = 6;
    if ((moblevel - 20) > 0)
      bonus += MAX(0, moblevel - 20) / 3;
    break;
  }  

  // Find out what type the item is, where it will be worn
  
  roll = dice(1, 100);
  
  if (roll <= 15) {
    type = ITEM_WEAR_FINGER;
    obj = read_object(30085, VIRTUAL);
  }
  else if (roll <= 30)	 {
	type = ITEM_WEAR_WRIST;
    obj = read_object(30087, VIRTUAL);
  }
  else if (roll <= 45)	{	
    type = ITEM_WEAR_NECK;
    obj = read_object(30086, VIRTUAL);
  }
  else if (roll <= 55)	{	
	type = ITEM_WEAR_FEET;
    obj = read_object(30091, VIRTUAL);
  }
  else if (roll <= 65)	{	
	type = ITEM_WEAR_HEAD;
    obj = read_object(30092, VIRTUAL);
  }
  else if (roll <= 75)	{	
	type = ITEM_WEAR_HANDS;
    obj = read_object(30090, VIRTUAL);
  }
  else if (roll <= 85)	{	
	type = ITEM_WEAR_ABOUT;
    obj = read_object(30088, VIRTUAL);
  }
  else if (roll <= 95)	{	
	type = ITEM_WEAR_WAIST;
    obj = read_object(30103, VIRTUAL);
  }
  else {
	type = ITEM_WEAR_ABOVE;
    obj = read_object(30104, VIRTUAL);
  }

	
  // Decide whether the item is of type jewelry or not
	
  switch (type) {
  case ITEM_WEAR_FINGER:
  case ITEM_WEAR_WRIST:
  case ITEM_WEAR_NECK:
  case ITEM_WEAR_ABOVE:
    jewelry = TRUE;
	break;
  }

  roll = dice(1, 100);
  roll2 = dice(1, 100);

  if (roll <= 5)
    affect = APPLY_STR;
  else if (roll <= 10)	
    affect = APPLY_DEX;
  else if (roll <= 15)	
    affect = APPLY_INT;
  else if (roll <= 20)	
    affect = APPLY_WIS;
  else if (roll <= 25)		
    affect = APPLY_CON;
  else if (roll <= 30)		
    affect = APPLY_CHA;
  else if (roll <= 40)		
    affect = APPLY_AC_NATURAL;
  else if (roll <= 50)		
    affect = APPLY_AC_DEFLECTION;	
  else if (roll <= 55)		
    affect = APPLY_FORTITUDE;
  else if (roll <= 60)		
	affect = APPLY_REFLEX;
  else if (roll <= 65)		
	affect = APPLY_WILL;
  else if (roll <= 75) {	
        affect = APPLY_SKILL;        
	subval = SKILL_LOW_SKILL + dice(1, SKILL_HIGH_SKILL - SKILL_LOW_SKILL + 1) - 1;
        if (!strcmp(spell_info[subval].name, "!UNUSED!")) {
          award_misc_magic_item(ch, grade, moblevel);
          return;
        }
  }
  else if (roll <= 80) {	
        affect = APPLY_FEAT;        
	subval = dice(1, NUM_FEATS_DEFINED - 1) - 1;
        if (!feat_list[subval].in_game) {
          award_misc_magic_item(ch, grade, moblevel);
          return;
        }
  }
  else if (roll <= 90) {
    affect = APPLY_MOVE;
	bonus *= 100;
  }
  else if (roll <= 94)		
    affect = APPLY_AC_DODGE;
  else if (roll <= 97) {		
    affect = APPLY_ALLSAVES;
	bonus *= 3;
	bonus /= 4;
  }
  else {	
    if (grade == GRADE_MUNDANE || (grade == GRADE_MINOR && roll2 <= 25))
      affect = APPLY_SPELL_LVL_0;
    else if (grade == GRADE_MINOR && roll2 <= 60)
      affect = APPLY_SPELL_LVL_1;
    else if (grade == GRADE_MINOR && roll2 <= 85)  
      affect = APPLY_SPELL_LVL_2;
    else if (grade == GRADE_MINOR && roll2 <= 100)  
      affect = APPLY_SPELL_LVL_3;
    else if (grade == GRADE_MEDIUM && roll2 <= 50)  
      affect = APPLY_SPELL_LVL_4;
    else if (grade == GRADE_MEDIUM && roll2 <= 80)  
      affect = APPLY_SPELL_LVL_5;
    else if (grade == GRADE_MEDIUM && roll2 <= 100)  
      affect = APPLY_SPELL_LVL_6;
    else if (grade == GRADE_MAJOR && roll2 <= 50)  
      affect = APPLY_SPELL_LVL_7;
    else if (grade == GRADE_MAJOR && roll2 <= 80)  
      affect = APPLY_SPELL_LVL_8;
    else if (grade == GRADE_MAJOR && roll2 <= 100)  
      affect = APPLY_SPELL_LVL_9;
    bonus = (dice(1, 5) - 1) ? 1 : 2;
  }

  int rare = dice(1, 100);
  int raregrade = 0;

  if (rare == 1) {
    raregrade = 3;
  } else if (rare <= 6) {
    raregrade = 2;
  } else if (rare <= 16) {
    raregrade = 1;
  }

  char rdesc[50]={'\0'};
  if (raregrade == 0)
    sprintf(rdesc, "@n");
  else if (raregrade == 1)
    sprintf(rdesc, "@G[Rare]@n ");
  else if (raregrade == 2)
    sprintf(rdesc, "@Y[Legendary]@n ");
  else if (raregrade == 3)
    sprintf(rdesc, "@M[Mythical]@n ");


  if (type == ITEM_WEAR_FINGER) {
    material = choose_metal_material();
    i = 0;
    while (*(ring_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material], 
            ring_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material], 
            ring_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->description = strdup(CAP(desc));
  }
  else if (type == ITEM_WEAR_WRIST) {
    material = choose_metal_material();
    i = 0;
    while (*(wrist_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material], 
            wrist_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material], 
            wrist_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->description = strdup(CAP(desc));
  }
  else if (type == ITEM_WEAR_NECK) {
    material = choose_metal_material();
    i = 0;
    while (*(neck_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material], 
            neck_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material], 
            neck_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2] );
    obj->description = strdup(CAP(desc));
  }
  else if (type == ITEM_WEAR_FEET) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(boot_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) { }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa pair of %s %s leather %s", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            boot_descs[misc_desc_roll_1] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA pair of %s %s leather %s lie here.", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            boot_descs[misc_desc_roll_1] );
    obj->description = strdup(desc);
  }
  else if (type == ITEM_WEAR_HANDS) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(hands_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) { }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa pair of %s %s leather %s", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            hands_descs[misc_desc_roll_1] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA pair of %s %s leather %s lie here.", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            hands_descs[misc_desc_roll_1] );
    obj->description = strdup(desc);
  }
  else if (type == ITEM_WEAR_WAIST) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(waist_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) { }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s leather %s", rdesc, AN(armor_special_descs[misc_desc_roll_3]), armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            waist_descs[misc_desc_roll_1] );
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s leather %s lie here.", rdesc, AN(armor_special_descs[misc_desc_roll_3]), armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            waist_descs[misc_desc_roll_1] );
    obj->description = strdup(desc);
  }
  else if (type == ITEM_WEAR_ABOUT) {
    material = choose_cloth_material();
    i = 0;
    while (*(cloak_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) { }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_crests + i++)) { }
    size = i;
    misc_desc_roll_4 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s %s bearing the crest of %s %s", rdesc, AN(colors[misc_desc_roll_2]), colors[misc_desc_roll_2], 
            material_names[material], cloak_descs[misc_desc_roll_1], AN(armor_crests[misc_desc_roll_4]),
            armor_crests[misc_desc_roll_4]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s %s bearing the crest of %s %s", rdesc, AN(colors[misc_desc_roll_2]), colors[misc_desc_roll_2], 
            material_names[material], cloak_descs[misc_desc_roll_1], AN(armor_crests[misc_desc_roll_4]),
            armor_crests[misc_desc_roll_4]);
    obj->description = strdup(CAP(desc));
  }
  else if (type == ITEM_WEAR_ABOVE) {
    material = MATERIAL_GEMSTONE;
    i = 0;
    while (*(crystal_descs + i++)) { }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) { }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa %s %s ioun stone", rdesc, crystal_descs[misc_desc_roll_1], colors[misc_desc_roll_2]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA %s %s ioun stone hovers just above the ground here.", rdesc, crystal_descs[misc_desc_roll_1], colors[misc_desc_roll_2]);
    obj->description = strdup(desc);
  }

  GET_OBJ_MATERIAL(obj) = material;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  int val = affect;

  if (val >= APPLY_SPELL_LVL_0 && val <= APPLY_SPELL_LVL_9)
    bonus = MAX(1, bonus / 3);
  if (val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_ARMOR || val == APPLY_AC_DODGE)
    bonus *= 10;
  if (val == APPLY_HIT)
    bonus *= 5;
  if (val == APPLY_KI)
    bonus *= 5;
  if (val == APPLY_MOVE)
    bonus *= 100;


  obj->affected[0].location = affect;
  obj->affected[0].modifier = bonus;
  obj->affected[0].specific = subval; 

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  if (GET_OBJ_LEVEL(obj) >= CONFIG_LEVEL_CAP) {
    award_misc_magic_item(ch, grade, moblevel);
    return;
  }


  bonus = raregrade;
  if (val >= APPLY_SPELL_LVL_0 && val <= APPLY_SPELL_LVL_9)
    bonus = MAX(1, bonus / 3);
  if (val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_ARMOR || val == APPLY_AC_DODGE)
    bonus *= 10;
  if (val == APPLY_HIT)
    bonus *= 5;
  if (val == APPLY_KI)
    bonus *= 5;
  if (val == APPLY_MOVE)
    bonus *= 100;
  obj->affected[0].modifier += bonus;


  GET_OBJ_COST(obj) = 250 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);

  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (raregrade * 2)) / 3;

  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
  
    sprintf(desc, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(desc, FALSE, ch, 0, ch, TO_NOTVICT);
  }
  
}

int choose_metal_material(void) {

  int roll = dice(1, 9);

  if (roll == 1)
    return MATERIAL_GOLD;
  if (roll == 2)
    return MATERIAL_ADAMANTINE;
  if (roll == 3)
    return MATERIAL_MITHRIL;
  if (roll == 4)
    return MATERIAL_IRON;
  if (roll == 5)
    return MATERIAL_COPPER;
  if (roll == 6)
    return MATERIAL_PLATINUM;
  if (roll == 7)
    return MATERIAL_BRASS;
  if (roll == 8)
    return MATERIAL_BRONZE;
  else
    return MATERIAL_STEEL;
}

int choose_cloth_material(void) {

  int roll = dice(1, 7);

  if (roll == 1)
    return MATERIAL_COTTON;
  if (roll == 2)
    return MATERIAL_SATIN;
  if (roll == 3)
    return MATERIAL_BURLAP;
  if (roll == 4)
    return MATERIAL_VELVET;
  if (roll == 5)
    return MATERIAL_WOOL;
  if (roll == 6)
    return MATERIAL_SILK;
  else
    return MATERIAL_HEMP;
}

char *gemstones[] = 
{
  "onyx",
  "obsidian",
  "amber",
  "amethyst",
  "opal",
  "fire opal",
  "ruby",
  "emerald",
  "sapphire",
  "diamond",
  "agate",
  "citrine",
  "coral",
  "quartz",
  "peridot",
  "pearl",
  "malachite",
  "lapis lazuli",
  "jade",
  "jasper",
  "fire agate",
  "sphene",
  "spinel",
  "sunstone",
  "tigers eye",
  "topaz",
  "turquoise"
};

char *ring_descs[] = 
{
  "ring",
  "ring",
  "band"
};

char *wrist_descs[] = 
{
  "bracer",
  "bracer",
  "bracer",
  "bracelet",
  "bracelet",
  "bracelet",
  "armband",
  "bangle",
  "armlet",
  "charm"
};

char *neck_descs[] = 
{
  "necklace",
  "necklace",
  "necklace",
  "necklace",
  "pendant",
  "pendant",
  "amulet",
  "amulet",
  "chain",
  "chain",
  "choker",
  "gorget",
  "collar",
  "locket"
};

char *head_descs[] = 
{
  "helmet",
  "helmet",
  "helm",
  "helm",
  "greathelm",
  "greathelm",
  "tiara",
  "crown",
  "cap",
  "cap",
  "hat",
  "hat",
  "hood",
  "hood",
  "hood",
  "cowl",
  "headband"
};

char *hands_descs[] = 
{
  "gauntlets",
  "gloves",
  "gauntlets",
  "gloves"
};

char *cloak_descs[] = 
{
  "cloak",
  "cloak",
  "cloak",
  "shroud",
  "cape"
};

char *waist_descs[] = 
{
  "belt",
  "belt",
  "belt",
  "girdle",
  "girdle",
  "sash"
};

char *boot_descs[] = 
{
  "boots",
  "boots",
  "boots",
  "boots",
  "boots",
  "sandals",
  "moccasins",
  "shoes",
  "knee-high boots",
  "riding boots",
  "slippers"
};

char *blade_descs[] = 
{
  "serrated",
  "barbed",
  "sharp",
  "razor-sharp",
  "gem-encrusted",
  "jewel-encrusted",
  "fine-edged",
  "finely-forged",
  "grooved",
  "ancient",
  "elven-crafted",
  "dwarven-crafted",
  "magnificent",
  "fancy",
  "ceremonial",
  "shining",
  "glowing",
  "gleaming",
  "scorched",
  "exquisite",
  "anointed",
  "thick-bladed",
  "crescent-bladed",
  "rune-etched",
  "blackened",
  "slim",
  "curved",
  "glittering",
  "wavy-bladed",
  "dual-edged",
  "ornate",
  "brutal"
};

char *piercing_descs[] = 
{
  "barbed",
  "sharp",
  "needle-sharp",
  "gem-encrusted",
  "jewel-encrusted",
  "fine-pointed",
  "finely-forged",
  "grooved",
  "ancient",
  "elven-crafted",
  "dwarven-crafted",
  "magnificent",
  "fancy",
  "ceremonial",
  "shining",
  "glowing",
  "gleaming",
  "scorched",
  "exquisite",
  "thick-pointed",
  "tri-pointed",
  "rune-etched",
  "blackened",
  "slim",
  "glittering",
  "anointed",
  "dual-pointed",
  "ornate",
  "brutal"
};

char *blunt_descs[] = 
{
  "gem-encrusted",
  "jewel-encrusted",
  "finely-forged",
  "grooved",
  "ancient",
  "elven-crafted",
  "dwarven-crafted",
  "magnificent",
  "fancy",
  "ceremonial",
  "shining",
  "glowing",
  "gleaming",
  "scorched",
  "exquisite",
  "thick-headed",
  "crescent-headed",
  "rune-etched",
  "blackened",
  "massive",
  "glittering",
  "dual-headed",
  "brutal",
  "sturdy",
  "ornate",
  "notched",
  "spiked",
  "wickedly-spiked",
  "cruel-looking",
  "anointed"
};

char *colors[] = 
{
  "amber",
  "amethyst",
  "azure",
  "black",
  "blue",
  "brown",
  "cerulean",
  "cobalt",
  "copper",
  "crimson",
  "cyan",
  "emerald",
  "forest-green",
  "gold",
  "grey",
  "green",
  "indigo",
  "ivory",
  "jade",
  "lavender",
  "magenta",
  "malachite",
  "maroon",
  "midnight-blue",
  "navy-blue",
  "ochre",
  "olive",
  "orange",
  "pink",
  "powder-blue",
  "purple",
  "red",
  "royal-blue",
  "sapphire",
  "scarlet",
  "sepia",
  "silver",
  "slate-grey",
  "steel-blue",
  "tan",
  "turquiose",
  "ultramarine",
  "violet",
  "white",
  "yellow"
};

char *crystal_descs[] = 
{
  "sparkling",
  "shimmering",
  "iridescent",
  "mottled",
  "cloudy",
  "grainy",
  "thick",
  "thin",
  "glowing",
  "radiant",
  "glittering",
  "incandescant",
  "effulgant",
  "scintillating",
  "murky",
  "opaque",
  "shadowy",
  "tenebrous"
};

char *potion_descs[] = 
{

  "sparkling",
  "shimmering",
  "iridescent",
  "mottled",
  "cloudy",
  "milky",
  "grainy",
  "clumpy",
  "thick",
  "viscous",
  "bubbly",
  "thin",
  "watery",
  "oily",
  "coagulated",
  "gelatinous",
  "diluted",
  "syrupy",
  "gooey",
  "fizzy",
  "glowing",
  "radiant",
  "glittering",
  "incandescant",
  "effulgant",
  "scintillating",
  "murky",
  "opaque",
  "shadowy",
  "tenebrous"
};

char *armor_special_descs[] = 
{
  "spiked",
  "engraved",
  "ridged",
  "charred",
  "jeweled",
  "elaborate",
  "ceremonial",
  "expensive",
  "battered",
  "shadowy",
  "gleaming",
  "iridescent",
  "shining",
  "ancient",
  "glowing",
  "glittering",
  "exquisite",
  "magnificent",
  "dwarven-made",
  "elven-made",
  "gnomish-made",
  "finely-made",
  "gem-encrusted",
  "gold-laced",
  "silver-laced",
  "platinum-laced"
};

char *armor_crests[] = 
{
  "falcon",
  "dragon",
  "rose",
  "sword",
  "crown",
  "lily",
  "thorn",
  "skull",
  "shield",
  "mantis",
  "infinity-loop",
  "broken merchant scale",
  "bison",
  "phoenix",
  "white circle",
  "feather",
  "open book",
  "forging hammer",
  "griffon wing",
  "multicolored flame",
  "green and gold tree",
  "red-eyed hood",
  "red circle",
  "black circle",
  "five-headed dragon",
  "condor",
  "turtle shell",
  "cross",
  "open eye",
  "lightning bolt",
  "mounted horse",
  "dripping dagger",
  "jester's mask",
  "heart",
  "diamond",
  "pegasus",
  "unicorn",
  "battle axe",
  "bow and arrow"
};

int determine_random_weapon_type(void) {

  int roll1;
  int roll2;
  int roll3;

  roll1 = dice(1, 100);
  roll2 = dice(1, 100);
  roll3 = dice(1, 100);


  if (roll3 <= 67) {
    roll1 = dice(1, NUM_WEAPON_TYPES);
    return roll1;
  }
  if (roll1 <= 70) {
    if (roll2 <= 4)
      return WEAPON_TYPE_DAGGER;
    else if (roll2 <= 14)
      return WEAPON_TYPE_GREAT_AXE;
    else if (roll2 <= 24)
      return WEAPON_TYPE_GREAT_SWORD;
    else if (roll2 <= 28)
      return WEAPON_TYPE_KAMA;
    else if (roll2 <= 41)
      return WEAPON_TYPE_LONG_SWORD;
    else if (roll2 <= 45)
      return WEAPON_TYPE_LIGHT_MACE;
    else if (roll2 <= 50)
      return WEAPON_TYPE_HEAVY_MACE;
    else if (roll2 <= 54)
      return WEAPON_TYPE_NUNCHAKU;
    else if (roll2 <= 57)
      return WEAPON_TYPE_QUARTERSTAFF;
    else if (roll2 <= 61)
      return WEAPON_TYPE_RAPIER;
    else if (roll2 <= 66)
      return WEAPON_TYPE_SCIMITAR;
    else if (roll2 <= 70)
      return WEAPON_TYPE_SHORTSPEAR;
    else if (roll2 <= 74)
      return WEAPON_TYPE_UNARMED;
    else if (roll2 <= 84)
      return WEAPON_TYPE_BASTARD_SWORD;
    else if (roll2 <= 89)
      return WEAPON_TYPE_SHORT_SWORD;
    else
      return WEAPON_TYPE_DWARVEN_WAR_AXE;
  }
  else if (roll1 <= 80) {
    if (roll2 <= 3)
      return WEAPON_TYPE_DOUBLE_AXE;
    else if (roll2 <= 7)
      return WEAPON_TYPE_BATTLE_AXE;
    else if (roll2 <= 10)
      return WEAPON_TYPE_SPIKED_CHAIN;
    else if (roll2 <= 12)
      return WEAPON_TYPE_CLUB;
    else if (roll2 <= 16)
      return WEAPON_TYPE_HAND_CROSSBOW;
    else if (roll2 <= 21) {
      if (dice(1, 2) - 1)
        return WEAPON_TYPE_HEAVY_REP_XBOW;
      else
        return WEAPON_TYPE_LIGHT_REP_XBOW;
    }
    else if (roll2 <= 23)
      return WEAPON_TYPE_FALCHION;
    else if (roll2 <= 26)
      return WEAPON_TYPE_DIRE_FLAIL;
    else if (roll2 <= 35)
      return WEAPON_TYPE_FLAIL;
    else if (roll2 <= 39)
      return WEAPON_TYPE_SIANGHAM;
    else if (roll2 <= 41)
      return WEAPON_TYPE_GLAIVE;
    else if (roll2 <= 43)
      return WEAPON_TYPE_GREAT_CLUB;
    else if (roll2 <= 45)
      return WEAPON_TYPE_GUISARME;
    else if (roll2 <= 48)
      return WEAPON_TYPE_HALBERD;
    else if (roll2 <= 51)
      return WEAPON_TYPE_UNARMED;
    else if (roll2 <= 54)
      return WEAPON_TYPE_HOOKED_HAMMER;
    else if (roll2 <= 56)
      return WEAPON_TYPE_LIGHT_HAMMER;
    else if (roll2 <= 58)
      return WEAPON_TYPE_HAND_AXE;
    else if (roll2 <= 61)
      return WEAPON_TYPE_KUKRI;
    else if (roll2 <= 65)
      return WEAPON_TYPE_LANCE;
    else if (roll2 <= 67)
      return WEAPON_TYPE_LONGSPEAR;
    else if (roll2 <= 70)
      return WEAPON_TYPE_MORNINGSTAR;
    else if (roll2 <= 72)
      return WEAPON_TYPE_NET;
    else if (roll2 <= 74)
      return WEAPON_TYPE_HEAVY_PICK;
    else if (roll2 <= 76)
      return WEAPON_TYPE_LIGHT_PICK;
    else if (roll2 <= 78)
      return WEAPON_TYPE_RANSEUR;
    else if (roll2 <= 80)
      return WEAPON_TYPE_SAP;
    else if (roll2 <= 82)
      return WEAPON_TYPE_SCYTHE;
    else if (roll2 <= 84)
      return WEAPON_TYPE_SHURIKEN;
    else if (roll2 <= 86)
      return WEAPON_TYPE_SICKLE;
    else if (roll2 <= 89)
      return WEAPON_TYPE_2_BLADED_SWORD;
    else if (roll2 <= 91)
      return WEAPON_TYPE_TRIDENT;
    else if (roll2 <= 94)
      return WEAPON_TYPE_DWARVEN_URGOSH;
    else if (roll2 <= 97)
      return WEAPON_TYPE_WARHAMMER;
    else
      return WEAPON_TYPE_WHIP;
  }
  else {
    if (roll2 <= 10)
      return WEAPON_TYPE_THROWING_AXE;
    else if (roll2 <= 25)
      return WEAPON_TYPE_HEAVY_CROSSBOW;
    else if (roll2 <= 35)
      return WEAPON_TYPE_LIGHT_CROSSBOW;
    else if (roll2 <= 39)
      return WEAPON_TYPE_DART;
    else if (roll2 <= 41)
      return WEAPON_TYPE_JAVELIN;
    else if (roll2 <= 46)
      return WEAPON_TYPE_SHORT_BOW;
    else if (roll2 <= 61)
      return WEAPON_TYPE_COMPOSITE_SHORTBOW;
    else if (roll2 <= 65)
      return WEAPON_TYPE_SLING;
    else if (roll2 <= 75)
      return WEAPON_TYPE_LONG_BOW;
    else
      return WEAPON_TYPE_COMPOSITE_LONGBOW;
  }

  return WEAPON_TYPE_UNARMED;

}

void shop_random_treasure(void)
{

  int level = 0;
  int roll = 0;
  struct obj_data *obj = NULL;

  struct char_data *i, *next_char;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    if (!IS_NPC(i) || !IS_MOB(i) || GET_MOB_SPEC(i) != shop_keeper)
      continue;

    if (dice(1, 10) == 1) {
      if (!MOB_FLAGGED(i, MOB_CRYSTAL_VENDOR))
        determine_treasure(i, i);
      else {
        roll = dice(1, 10);
        level += roll;
        while (roll == 10 && level < 40) {
          roll = dice(1, 10);
          level += roll;
        }    
        if (dice(1, 2) == 1)
          get_random_crystal(i, level);  
        else
          get_random_essence(i, level);
        for (obj = i->carrying; obj; obj = obj->next_content) {
          assign_qp_value(obj);
        }
      }
    }
  }

}

void determine_crafting_component_treasure(struct char_data *ch, struct char_data *mob)
{

  int level = 0;
  int roll = dice(1, 100);
  if (!mob)
    return;

  if (!IS_NPC(mob))
    return;

  level = GET_LEVEL(mob);

  if (MOB_FLAGGED(mob, MOB_LIEUTENANT)) {
    level++;
    roll *= 3;
    roll /= 4;
  }
  if (MOB_FLAGGED(mob, MOB_CAPTAIN)) {
    level += 2;
    roll /= 2;
  }
  if (MOB_FLAGGED(mob, MOB_BOSS)) {
    level += 3;
    roll /= 3;
  }
  if (MOB_FLAGGED(mob, MOB_FINAL_BOSS)) {
    level += 5;
    roll /= 3;
  }

  roll = MAX(1, roll);

  if (roll > 2)
    return;

  if (roll <= 1) {
    get_random_crystal(ch, level);
    return;
  }
 
  get_random_essence(ch, level);

}

void get_random_essence(struct char_data *ch, int level) {


  struct obj_data *obj = NULL;
  int roll = dice(1, 1000);
  int max = 1000 + (level * 2);

  roll += level * 20;


    if (roll <= (max / 2))
          obj = read_object(64100, VIRTUAL); // minor power essence
    else if (roll <= (70 * max / 100))
          obj = read_object(64101, VIRTUAL); // lesser power essence
    else if (roll <= (85 * max / 100))
          obj = read_object(64102, VIRTUAL); // medium power essence
    else if (roll <= (94 * max / 100))
          obj = read_object(64103, VIRTUAL); // greater power essence
    else if (roll <= (98 * max / 100))
          obj = read_object(64104, VIRTUAL); // major power essence
    else
          get_random_essence(ch, level);

  if (obj != NULL) {
    obj_to_char(obj, ch);
    if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
      act("@YYou have found $p.@n", FALSE, ch, obj, 0, TO_CHAR);
      act("@Y$n has found $p.@n", FALSE, ch, obj, 0, TO_ROOM);
    }
  }

}

struct char_data * find_treasure_recipient(struct char_data *killer)
{

  struct char_data *k = killer;
  struct follow_type *f;
  int num_people = 1;
  int roll = 0;
  int i = 0;

  if (!AFF_FLAGGED(killer, AFF_GROUP))
    return killer;

  if (killer->master)
    k = killer->master;

  for (f = k->followers; f; f = f->next) {
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(killer) == IN_ROOM(f->follower))
      num_people++;
  }

  roll = dice(1, num_people);

  if (roll == 1)
    return k;

  i = 1;

  for (f = k->followers; f; f = f->next) {
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(killer) == IN_ROOM(f->follower))
      i++;
    if (i == roll)
      return f->follower;
  }

  return killer;  
  
}

void get_random_crystal(struct char_data *ch, int level) {

  struct obj_data *obj = NULL;
  int val = 0;
  int val2 = 0;
  int desc = -1;
  int color_1 = -1;
  int color_2 = -1;
  int i = 0;

  obj = read_object(64100, VIRTUAL);

  if (!obj)
    return;

  GET_OBJ_TYPE(obj) = ITEM_CRYSTAL;
  GET_OBJ_COST(obj) = 0;
  GET_OBJ_LEVEL(obj) = level;
  GET_OBJ_MATERIAL(obj) = MATERIAL_CRYSTAL;
  
  int roll = dice(1, 1000);

  if (roll <= 50) {
    val = APPLY_FEAT;
    while (TRUE) {
      val2 = dice(1, NUM_FEATS_DEFINED) - 1;
      if (feat_list[val2].in_game && feat_list[val2].can_learn)
        break;
    }    
  } else if (roll <= 100) {
    val = APPLY_SKILL;
    while (TRUE) {
      val2 = dice(1, SKILL_HIGH_SKILL - SKILL_LOW_SKILL + 1) + SKILL_LOW_SKILL - 1;
      if (spell_info[val2].skilltype == SKTYPE_SKILL)
        break;
    }    
  } else if (roll <= 450) {
    switch(dice(1, 4)) {
      case 1:
        val = APPLY_AC_DEFLECTION;
        break;
      case 2:
        val = APPLY_AC_NATURAL;
        break;
      case 3:
        val = APPLY_AC_ARMOR;
        break;
      case 4:
        val = APPLY_AC_SHIELD;
        break;
    }
  }
  else if (roll <= 566) {
    switch(dice(1, 2)) {
      case 1:
        val = APPLY_ACCURACY;
        break;
      case 2:
        val = APPLY_DAMAGE;
        break;
    }
  }
  else if ( roll <= 800) {
    switch(dice(1, 6)) {
      case 1:
        val = APPLY_STR;
        break;
      case 2:
        val = APPLY_CON;
        break;
      case 3:
        val = APPLY_DEX;
        break;
      case 4:
        val = APPLY_INT;
        break;
      case 5:
        val = APPLY_WIS;
        break;
      case 6:
        val = APPLY_CHA;
        break;
    }
  }
  else if (roll <= 938) {
    switch(dice(1, 6)) {
      case 1:
        val = APPLY_CARRY_WEIGHT;
        break;
      case 2:
        val = APPLY_MOVE;
        break;
      case 3:
        val = APPLY_KI;
        break;
      case 4:
        val = APPLY_FORTITUDE;
        break;
      case 5:
        val = APPLY_REFLEX;
        break;
      case 6:
        val = APPLY_WILL;
        break;
    }  
  }  
  else if (roll <= 988) {
    int roll2 = dice(1, 76);
    if (roll2 <= 16)
      val = APPLY_SPELL_LVL_0;
    else if (roll2 <= 30)
      val = APPLY_SPELL_LVL_1;
    else if (roll2 <= 40)
      val = APPLY_SPELL_LVL_2;
    else if (roll2 <= 58)
      val = APPLY_SPELL_LVL_3;
    else if (roll2 <= 60)
      val = APPLY_SPELL_LVL_4;
    else if (roll2 <= 66)
      val = APPLY_SPELL_LVL_6;
    else if (roll2 <= 70)
      val = APPLY_SPELL_LVL_7;
    else if (roll2 <= 74)
      val = APPLY_SPELL_LVL_8;
    else
      val = APPLY_SPELL_LVL_9;
  }
  else if (roll <= 994) {
    val = APPLY_AC_DODGE;
  }
  else
    val = APPLY_HITROLL;

  GET_OBJ_VAL(obj, 0) = val;
  GET_OBJ_VAL(obj, 1) = val2;

  i = 0;
  int size = 0;
  char buf[200]={'\0'};
  while (*(colors + i++)) { /* counting array */ }
  size = i;
  color_1 = MAX(1, dice(1, (int) size) - 1);
  color_2 = MAX(1, dice(1, (int) size) - 1);
  while (color_2 == color_1)
    color_2 = MAX(1, dice(1, (int) size) - 1);
  i = 0;
  while (*(crystal_descs + i++)) { /* counting array */ }
  size = i;
  desc = MAX(1, dice(1, (int) size) - 1);

  roll = dice(1, 100);

  if (roll >= 91) { // two colors and descriptor
    sprintf(buf, "crystal %s %s %s", colors[color_1 - 1], colors[color_2 - 1], crystal_descs[desc - 1]);    
    obj->name = strdup(buf);
    sprintf(buf, "a  %s, %s and %s crystal", crystal_descs[desc - 1], colors[color_1 - 1], colors[color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s, %s and %s crystal lies here.", crystal_descs[desc - 1], colors[color_1 - 1], colors[color_2 - 1]);
    obj->description = strdup(buf);
        
  }
  else if (roll >= 66) { // one color and descriptor
    sprintf(buf, "crystal %s %s", colors[color_1 - 1], crystal_descs[desc - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s %s crystal", crystal_descs[desc - 1], colors[color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s %s crystal lies here.", crystal_descs[desc - 1], colors[color_1 - 1]);
    obj->description = strdup(buf);
  }
  else if (roll >= 41) { // two colors no descriptor
    sprintf(buf, "crystal %s %s", colors[color_1 - 1], colors[color_2 - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s and %s crystal", colors[color_1 - 1], colors[color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s and %s crystal lies here.", colors[color_1 - 1], colors[color_2 - 1]);
    obj->description = strdup(buf);
  }
  else if (roll >= 21) {// one color no descriptor
    sprintf(buf, "crystal %s", colors[color_1 - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", colors[color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", colors[color_1 - 1]);
    obj->description = strdup(buf);
  }
  else  {// descriptor only
    sprintf(buf, "crystal %s", crystal_descs[desc - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", crystal_descs[desc - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", crystal_descs[desc - 1]);
    obj->description = strdup(buf);
  }

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s.@n\r\n", obj->short_description);
    act("@Y$n has found $p.@n", true, ch, obj, 0, TO_ROOM);
  }
}

void assign_qp_value(struct obj_data *obj) 
{

  int val0 = GET_OBJ_VAL(obj, 0);
  int factor = 0;
  int objLevel = GET_OBJ_LEVEL(obj);

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_QUEST);

  if (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL) {

    switch (val0) {

      // 33% chance to drop combined
      case APPLY_AC_DEFLECTION:
        factor = 10;
        break;
      case APPLY_AC_NATURAL:
        factor = 10;
        break;
      case APPLY_AC_ARMOR:
        factor = 10;
        break;
      case APPLY_AC_SHIELD:
        factor = 10;
        break;

      // 33% chance to drop combined
      case APPLY_ACCURACY:
        factor = 10;
        break;
      case APPLY_DAMAGE:
        factor = 10;
        break;

      // 14% chance to drop combined
      case APPLY_STR:
        factor = 20;
        break;
      case APPLY_CON:
        factor = 20;
        break;
      case APPLY_DEX:
        factor = 20;
        break;
      case APPLY_INT:
        factor = 20;
        break;
      case APPLY_WIS:
        factor = 20;
        break;
      case APPLY_CHA:
        factor = 20;
        break;
     
      // 13.8% chance to drop combined
      case APPLY_HIT:
        factor = 20;
        break;
      case APPLY_MOVE:
        factor = 20;
        break;
      case APPLY_KI:
        factor = 20;
        break;
      case APPLY_FORTITUDE:
        factor = 20;
        break;
      case APPLY_REFLEX:
        factor = 20;
        break;
      case APPLY_WILL:
        factor = 20;
        break;

      // 5% chance to drop combined
      case APPLY_SPELL_LVL_0:
        factor = 5;
        break;
      case APPLY_SPELL_LVL_1:
        factor = 6;
        break;
      case APPLY_SPELL_LVL_2:
        factor = 8;
        break;
      case APPLY_SPELL_LVL_3:
        factor = 10;
        break;
      case APPLY_SPELL_LVL_4:
        factor = 12;
        break;
      case APPLY_SPELL_LVL_5:
        factor = 15;
        break;
      case APPLY_SPELL_LVL_6:
        factor = 20;
        break;
      case APPLY_SPELL_LVL_7:
        factor = 30;
        break;
      case APPLY_SPELL_LVL_8:
        factor = 40;
        break;
      case APPLY_SPELL_LVL_9:
        factor = 50;
        break;

      // 1% chance to drop
      case APPLY_AC_DODGE:
        factor = 50;
        break;

      // 0.1% chance to drop
      case APPLY_HITROLL:
        factor = 100;
        break;

      // 0.1% chance to drop
      case APPLY_DAMROLL:
        factor = 100;
        break;

      case APPLY_SKILL:
        factor = 5;
        break;

      case APPLY_FEAT:
        factor = 25;
        break;

      default:
        factor = 1000000;
        break;
    }
    GET_OBJ_COST(obj) = objLevel * factor;
  }
  else if (IS_ESSENCE(obj)) 
  {
    if (objLevel < 4)
      GET_OBJ_COST(obj) = 20;
    else if (objLevel < 8)
      GET_OBJ_COST(obj) = 100;
    else if (objLevel < 12)
      GET_OBJ_COST(obj) = 200;
    else if (objLevel < 16)
      GET_OBJ_COST(obj) = 300;
    else
      GET_OBJ_COST(obj) = 500;
  }
}

void award_special_magic_item(struct char_data *ch)
{
    struct obj_data *obj = NULL;
    char buf[200]={'\0'};

    // Ring of Three Wishes  
    obj = read_object(30188, VIRTUAL);

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    obj_to_char(obj, ch);

    if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) 
    {
        send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
        sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
        act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
    }
}
