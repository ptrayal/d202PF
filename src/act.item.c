/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "feats.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "assemblies.h"
#include "boards.h"
#include "quest.h"
#include "house.h"

/* extern functions */
char *find_exdesc(char *word, struct extra_descr_data *list);
void item_check(struct obj_data *object, struct char_data *ch);
int has_intro(struct char_data *ch, struct char_data *victim);
int skill_roll(struct char_data *ch, int skillnum);
int random_essence_vnum(int bonus);

/* local functions */
int calculate_container_weight(struct obj_data *obj);
int can_take_obj(struct char_data *ch, struct obj_data *obj);
void get_check_money(struct char_data *ch, struct obj_data *obj);
int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void get_from_room(struct char_data *ch, char *arg, int amount);
void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount);
void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data *ch, char *arg);
void weight_change_object(struct obj_data *obj, int weight);
void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
void name_from_drinkcon(struct obj_data *obj);
void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount);
void name_to_drinkcon(struct obj_data *obj, int type);
void wear_message(struct char_data *ch, struct obj_data *obj, int where);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
void perform_remove(struct char_data *ch, int pos);
int hands(struct char_data * ch);
long max_carry_weight(struct char_data *ch);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
char * change_coins(int coins);
void modify_coins(struct char_data *ch, int type, int operand, int amount);
void convert_coins(struct char_data *ch);
void update_encumberance(struct char_data *ch);
void advance_crafting_progress(struct char_data *ch, int cmd);


extern long           g_lNumAssemblies;
extern ASSEMBLY       *g_pAssemblyTable;
extern int mining_nodes;
extern int farming_nodes;
extern int hunting_nodes;
extern int foresting_nodes;


// local vars

int curbid = 0;				/* current bid on item being auctioned */
int aucstat = AUC_NULL_STATE;		/* state of auction.. first_bid etc.. */
struct obj_data *obj_selling = NULL;	/* current object for sale */
struct char_data *ch_selling = NULL;	/* current character selling obj */
struct char_data *ch_buying  = NULL;	/* current character buying the object */

char *auctioneer[AUC_BID + 1] = {
	
	"The auctioneer auctions, '$n puts $p up for sale at %d coins.'",
	"The auctioneer auctions, '$p at %d coins going once!.'",
	"The auctioneer auctions, '$p at %d coins going twice!.'",
	"The auctioneer auctions, 'Last call: $p going for %d coins.'",
	"The auctioneer auctions, 'Unfortunately $p is unsold, returning it to $n.'",
	"The auctioneer auctions, 'SOLD! $p to $n for %d coins!.'",
	"The auctioneer auctions, 'Sorry, $n has cancelled the auction.'",
	"The auctioneer auctions, 'Sorry, $n has left us, the auction can't go on.'",
	"The auctioneer auctions, 'Sorry, $p has been confiscated, shame on you $n.'",
	"The auctioneer tells you, '$n is selling $p for %d gold.'",
	"The auctioneer auctions, '$n bids %d coins on $p.'"
};

// external vars

extern struct descriptor_data *descriptor_list;


ACMD(do_synthesize)
{

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You must drop something before you can synthesize anything.\r\n");
    return;
  }

  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until your current task ends.\r\n");
    return;
  }

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any crafting actions.\r\n");
    return;
  }

  struct obj_data *obj = NULL;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What would you like to synthesize?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
    send_to_char(ch, "There doesn't seem to be %s in your inventory.\r\n", argument);
    return;
  }

  if (!IS_GEMSTONE(obj) && !IS_PETRIFIED_WOOD(obj) && !IS_FOSSIL(obj)) {
    send_to_char(ch, "That isn't a valid item to synthesize into a magical essence.\r\n");
    return;
  }

  GET_CRAFTING_OBJ(ch) = obj;

  if (GET_CRAFTING_OBJ(ch) && GET_OBJ_LEVEL(GET_CRAFTING_OBJ(ch)) > get_skill_value(ch, SKILL_CRAFTING_THEORY)) {
    send_to_char(ch, "Your crafting theory skill isn't high enough to synthesize that gemstone.\r\n");
    return;
  }

  ch->synth_value = MAX(0, GET_OBJ_LEVEL(obj) / 4);

  GET_CRAFTING_TYPE(ch) = SCMD_SYNTHESIZE;
  GET_CRAFTING_TICKS(ch) = GET_ADMLEVEL(ch) ? 1 : 3;

  send_to_char(ch, "You begin to synthesize a crystal from %s.\r\n", obj->short_description);
  act("$n begins to synthesize a crystal from $p.", FALSE, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
  obj_from_char(obj);
  extract_obj(obj);
 
}

ACMD(do_divide)
{
  struct obj_data *obj = NULL;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What would you like to divide?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
    send_to_char(ch, "There doesn't seem to be %s in your inventory.\r\n", argument);
    return;
  }

  if (!IS_ESSENCE(obj) || GET_OBJ_LEVEL(obj) <= 1) {
    send_to_char(ch, "That isn't a valid magical essence to divide into smaller ones.\r\n");
    return;
  }


  GET_CRAFTING_OBJ(ch) = read_object(64100, VIRTUAL);
  if (GET_OBJ_VNUM(obj) == 64101) {
    GET_CRAFTING_REPEAT(ch) = 2;
  }
  else if (GET_OBJ_VNUM(obj) == 64102) {
    GET_CRAFTING_REPEAT(ch) = 3;
  }
  else if (GET_OBJ_VNUM(obj) == 64103) {
    GET_CRAFTING_REPEAT(ch) = 4;
  }
  else if (GET_OBJ_VNUM(obj) == 64104) {
    GET_CRAFTING_REPEAT(ch) = 5;
  }
  else {
    send_to_char(ch, "That is not a valid magical essence or it cannot be divided further.\r\n");
  }

  if (GET_CRAFTING_OBJ(ch) == NULL) {
    send_to_char(ch, "Error, please report to an imm.\r\n");
    return;
  }

  GET_CRAFTING_TYPE(ch) = SCMD_DIVIDE;
  GET_CRAFTING_TICKS(ch) = 1;

  char buf[200]={'\0'};

  sprintf(buf, "You begin to create %s (x%d) from %s.\r\n", GET_CRAFTING_OBJ(ch)->short_description, GET_CRAFTING_REPEAT(ch), obj->short_description);
  send_to_char(ch, "%s", buf);
  sprintf(buf, "$n begins to synthesize %s (x%d) from %s.", GET_CRAFTING_OBJ(ch)->short_description, GET_CRAFTING_REPEAT(ch), obj->short_description);
  act(buf, FALSE, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
  obj_from_char(obj);
  extract_obj(obj);

}

ACMD(do_disenchant)
{

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You must drop something before you can disenchant anything.\r\n");
    return;
  }

  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until your current task ends.\r\n");
    return;
  }

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any crafting actions.\r\n");
    return;
  }

  struct obj_data *obj = NULL;
  struct obj_data *essence = NULL;
  int chance = 100;
  int level = 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What would you like to disenchant?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying))) {
    send_to_char(ch, "There doesn't seem to be %s in your inventory.\r\n", argument);
    return;
  }

  if (!IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {
    send_to_char(ch, "Only magical items can be disenchanted.\r\n");
    return;
  }

  if (obj && GET_OBJ_LEVEL(obj) > get_skill_value(ch, SKILL_CRAFTING_THEORY)) {
    send_to_char(ch, "Your crafting_theory skill isn't high enough to disenchant that item.\r\n");
    return;
  }


  if (GET_OBJ_TYPE(obj) == ITEM_POTION)
    chance = 20;
  else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
    chance = 30;
  else if (GET_OBJ_TYPE(obj) == ITEM_WAND)
    chance = 10 + GET_OBJ_VAL(obj, VAL_WAND_CHARGES);
  else if (GET_OBJ_TYPE(obj) == ITEM_STAFF)
    chance = 25 + GET_OBJ_VAL(obj, VAL_STAFF_CHARGES);
  else if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
           GET_OBJ_TYPE(obj) != ITEM_ARMOR_SUIT && GET_OBJ_TYPE(obj) != ITEM_WORN) {
    send_to_char(ch, "You cannot disenchant that item.\r\n");
    return;
  }

  if (dice(1, 100) <= chance) {
    level = GET_OBJ_LEVEL(obj);
    if (level <= 4)
      essence = read_object(64100, VIRTUAL); // minor
    else if (level <= 8)
      essence = read_object(64101, VIRTUAL); // lesser
    else if (level <= 12)
      essence = read_object(64102, VIRTUAL); // medium
    else if (level <= 16)
      essence = read_object(64103, VIRTUAL); // greater
    else 
      essence = read_object(64104, VIRTUAL); // major

  }
  else {
    essence = read_object(64012, VIRTUAL); // failed attempt
  }
  GET_CRAFTING_TYPE(ch) = SCMD_DISENCHANT;
  GET_CRAFTING_TICKS(ch) = GET_ADMLEVEL(ch) ? 1 : 3;
  GET_CRAFTING_OBJ(ch) = essence;

  send_to_char(ch, "You begin to disenchant %s.\r\n", obj->short_description);
  act("$n begins to disenchant $p.", FALSE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);
  extract_obj(obj);
}

ACMD(do_harvest_new)
{

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You must drop something before you can harvest anything else.\r\n");
    return;
  }

  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until your current task ends.\r\n");
    return;
  }

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any crafting actions.\r\n");
    return;
  }

  struct obj_data * obj = NULL;
  int roll = 0;
  int skillnum = SKILL_MINING; 

  struct obj_data *node;

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You need to specify what you want to harvest.\r\n");
    return;
  }

  if (!(node = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
    send_to_char(ch, "That doesn't seem to be present in this room.\r\n");
    return;
  }

  if (GET_OBJ_VNUM(node) != 64099) {
    send_to_char(ch, "That is not a harvesting node.\r\n");
    return;
  }

  int material = GET_OBJ_MATERIAL(node);

  if (IS_WOOD(material) && IS_LEATHER(material)) {
    skillnum = SKILL_FORESTING;
  } else if (IS_CLOTH(material)) {
    skillnum = SKILL_FARMING;
  }
  // else skill is mining.

  int minskill = 0;

  switch (material) {

    case MATERIAL_STEEL:
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(64000, VIRTUAL); // steel
      else if (roll <= 96)
        obj = read_object(64000, VIRTUAL); // steel
      else if (roll <= 98)
        obj = read_object(64003, VIRTUAL); // onyx
      else
        obj = read_object(64001, VIRTUAL); // obsidian
      minskill = 1;
    break;

    case MATERIAL_COLD_IRON:
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(64002, VIRTUAL); // cold iron
      else if (roll <= 52)
        obj = read_object(64003, VIRTUAL); // onyx
      else
        obj = read_object(64002, VIRTUAL); // cold iron
      minskill = 15;
    break;

   case MATERIAL_MITHRIL:
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(64007, VIRTUAL); // mithril
      else if (roll <= 96)
        obj = read_object(64007, VIRTUAL); // mithril
      else if (roll <= 98)
        obj = read_object(64006, VIRTUAL); // ruby
      else
        obj = read_object(64008, VIRTUAL); // sapphire
      minskill  = 28;
    break;    

    case MATERIAL_ADAMANTINE:
      roll = dice(1, 100);
      if (roll <= 4)
        obj = read_object(64010, VIRTUAL); // adamantine
      else if (roll <= 96)
        obj = read_object(64028, VIRTUAL); // platinum
      else {
        if (dice(1, 2) % 2 == 0)
          obj = read_object(64011, VIRTUAL); // diamond
        else
          obj = read_object(64009, VIRTUAL); // emerald
      }
      minskill = 42;
    break;    

    case MATERIAL_SILVER:
      roll = dice(1, 10);
      if (roll <= (8)) {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64027, VIRTUAL); // copper
        else if (roll <= 96)
          obj = read_object(64027, VIRTUAL); // copper
        else if (roll <= 98)
          obj = read_object(64003, VIRTUAL); // onyx
        else
          obj = read_object(64001, VIRTUAL); // obsidian
      }
      else {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64004, VIRTUAL); // silver
        else if (roll <= 52)
          obj = read_object(64003, VIRTUAL); // onyx
        else
          obj = read_object(64004, VIRTUAL); // silver
      }
      minskill = 1;
    break;

    case MATERIAL_GOLD:
      roll = dice(1, 10);
      if (roll <= (8)) {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64005, VIRTUAL); // gold
        else if (roll <= 96)
          obj = read_object(64005, VIRTUAL); // gold
        else if (roll <= 98)
          obj = read_object(64006, VIRTUAL); // ruby
        else
          obj = read_object(64008, VIRTUAL); // sapphire
      }
      else {
        roll = dice(1, 100);
        if (roll <= 4)
          obj = read_object(64028, VIRTUAL); // platinum
        else if (roll <= 96)
          obj = read_object(64028, VIRTUAL); // platinum
        else {
          if (dice(1, 2) % 2 == 0)
            obj = read_object(64011, VIRTUAL); // diamond
          else
            obj = read_object(64009, VIRTUAL); // emerald
        }
      }
      minskill = 30;      
    break;

    case MATERIAL_WOOD:
      roll = dice(1, 100);
      if (roll <= (80)) {
        if (dice(1, 100) <= 96)
          obj = read_object(64015, VIRTUAL); // alderwood
        else
          obj = read_object(64037, VIRTUAL); // fossilized bird egg
      }
      else if (roll <= (94)) {
        if (dice(1, 100) <= 96)
          obj = read_object(64031, VIRTUAL); // yew
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
      }
      else {
        if (dice(1, 100) <= 96)
          obj = read_object(64032, VIRTUAL); // oak
        else
          obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      }
    minskill = 1;
    break;

    case MATERIAL_DARKWOOD:
      if (dice(1, 100) <= 96)
        obj = read_object(64016, VIRTUAL); // darkwood
      else
        obj = read_object(64040, VIRTUAL); // fossilized dragon egg
      minskill = 38;
    break;

    case MATERIAL_LEATHER:
      roll = dice(1, 100);
      if (roll <= (82)) {
        if (dice(1, 100) <= 96) {
          obj = read_object(64014, VIRTUAL); // low quality hide
        }
        else
          obj = read_object(64037, VIRTUAL); // fossilized bird egg
      }
      else if (roll <= (94)) {
        if (dice(1, 10) <= 96) {
          obj = read_object(64029, VIRTUAL); // medium quality hide
        }
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
      }
      else {
        if (dice(1, 100) <= 96) {
          obj = read_object(64030, VIRTUAL); // high quality hide
        }
        else
          obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      }
      minskill = 1;
    break;

    case MATERIAL_HEMP:
      if (dice(1, 100) <= 96)
        obj = read_object(64020, VIRTUAL); // hemp
      else
        obj = read_object(64037, VIRTUAL); // fossilized bird egg
      minskill = 1;
    break;

    case MATERIAL_COTTON:
        if (dice(1, 100) <= 96) {
          obj = read_object(64017, VIRTUAL); // cotton
        }
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
    minskill = 12;
    break;

    case MATERIAL_VELVET:
      if (dice(1, 100) <= 96) {
        obj = read_object(64021, VIRTUAL); // velvet
      }
      else
        obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      minskill = 25;
    break;

    case MATERIAL_SILK:
        if (dice(1, 100) <= 96) {
          if ((dice(1, 20) % 2) == 0)
            obj = read_object(64022, VIRTUAL); // satin
          else
            obj = read_object(64023, VIRTUAL); // silk
        }
        else
          obj = read_object(64040, VIRTUAL); // fossilized dragon egg
    minskill = 38;
    break;

    default:
      send_to_char(ch, "That is not a valid node type, please report this to a staff member.\r\n");
      return;
  }

  if (!obj) {
    send_to_char(ch, "That is not a valid node type, please report this to a staff member.\r\n");
    return;
  }

  if (get_skill_value(ch, skillnum) < minskill) {
    send_to_char(ch, "You need a minimum %s skill of %d, while yours is only %d.\r\n", spell_info[skillnum].name, minskill, get_skill_value(ch, skillnum));
    return;
  }

  if (obj == NULL)
    obj = read_object(64012, VIRTUAL);
 

  GET_CRAFTING_TYPE(ch) = subcmd;
  GET_CRAFTING_TICKS(ch) = 1;
  GET_CRAFTING_OBJ(ch) = obj;
  ch->player_specials->crafting_exp_mult = get_skill_value(ch, skillnum) / 2;

  // Tell the character they made something. 
  char buf[200]={'\0'};
  sprintf(buf, "You begin to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_CHAR);

  // Tell the room the character made something. 
  sprintf(buf, "$n begins to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_ROOM);

  if (node)
    GET_OBJ_VAL(node, 0)--;

  if (node && GET_OBJ_VAL(node, 0) <= 0) {
    switch (skillnum) {
      case SKILL_MINING:
        mining_nodes--;
        break;
      case SKILL_FORESTING:
        foresting_nodes--;
        break;
      case SKILL_FARMING:
        farming_nodes--;
        break;
    }
    obj_from_room(node);
    extract_obj(node);
  }

    return;
}

ACMD(do_harvest)
{

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You must drop something before you can harvest anything else.\r\n");
    return;
  }

  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until your current task ends.\r\n");
    return;
  }

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any crafting actions.\r\n");
    return;
  }

  int mine = FALSE, forest = FALSE, farm = FALSE;
  struct obj_data * obj = NULL;
  int roll = 0, rollmod = 0, modamt[4], modsum = 0;
  char buf[100]={'\0'};
  int skillnum = SKILL_MINING; 

  modamt[0] = 0;
  modamt[1] = 0;
  modamt[2] = 0;
  modamt[3] = 0;

  if (subcmd == SCMD_MINE)
    mine = TRUE;
  if (subcmd == SCMD_FOREST)
    forest = TRUE;
  if (subcmd == SCMD_FARM)
    farm = TRUE;

  if (times_harvested[world[IN_ROOM(ch)].number] >= 5) {
    send_to_char(ch, "There is nothing left here to %s.\r\n", CMD_NAME);
    return;
  }

  if (mine) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINE) || SECT(IN_ROOM(ch)) == SECT_MOUNTAIN || SECT(IN_ROOM(ch)) == SECT_HILLS ||
        SECT(IN_ROOM(ch)) == SECT_CAVE ) {
      times_harvested[world[IN_ROOM(ch)].number]++;
      roll = dice(1, 1000);
      rollmod = get_skill_value(ch, SKILL_MINING);
      if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINE_RICH))
        rollmod += 25;
      else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINE_ABUNDANT))
        rollmod += 10;
      
      rollmod *= 10;

      roll += rollmod;

      modamt[0] = rollmod * 60 / 100;
      modamt[1] = rollmod * 25 / 100;
      modamt[2] = rollmod * 10 / 100;
      modamt[3] = rollmod * 5 / 100;

      modsum = modamt[0] + modamt[1] + modamt[2];

      modamt[0] += rollmod - modsum;

      if (roll <= (500))
        obj = read_object(64012, VIRTUAL); // stone, filler object, nothing will be loaded
      else if (roll <= (820 + modamt[0])) {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64000, VIRTUAL); // steel
        else if (roll <= 96)
          obj = read_object(64027, VIRTUAL); // copper
        else if (roll <= 98)
          obj = read_object(64003, VIRTUAL); // onyx
        else
          obj = read_object(64001, VIRTUAL); // obsidian
      }
      else if (roll <= (920 + modamt[0] + modamt[1])) {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64002, VIRTUAL); // cold iron
        else if (roll <= 52)
          obj = read_object(64003, VIRTUAL); // onyx
        else
          obj = read_object(64004, VIRTUAL); // silver
      }
      else if (roll <= (970 + modamt[0] + modamt[1] + modamt[2])) {
        roll = dice(1, 100);
        if (roll <= 48)
          obj = read_object(64005, VIRTUAL); // gold
        else if (roll <= 96)
          obj = read_object(64007, VIRTUAL); // mithril
        else if (roll <= 98)
          obj = read_object(64006, VIRTUAL); // ruby
        else
          obj = read_object(64008, VIRTUAL); // sapphire
      }
      else {
        roll = dice(1, 100);
        if (roll <= 4)
          obj = read_object(64010, VIRTUAL); // adamantine
        else if (roll <= 96)
          obj = read_object(64028, VIRTUAL); // platinum
        else {
          if (dice(1, 2) % 2 == 0)
            obj = read_object(64011, VIRTUAL); // diamond
          else
            obj = read_object(64009, VIRTUAL); // emerald
        }
      }
    }
    else {
      send_to_char(ch, "You can't mine here.\r\n");
      return;
    }
  }
  if (forest) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOREST) || SECT(IN_ROOM(ch)) == SECT_FOREST) {
      times_harvested[world[IN_ROOM(ch)].number]++;
      roll = dice(1, 1000);
      skillnum = SKILL_FORESTING;
      rollmod = get_skill_value(ch, SKILL_FORESTING);
      if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOREST_RICH))
        rollmod += 25;
      else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOREST_ABUNDANT))
        rollmod += 10;

      rollmod *= 10;

      roll += rollmod;

      modamt[0] = rollmod * 60 / 100;
      modamt[1] = rollmod * 25 / 100;
      modamt[2] = rollmod * 10 / 100;
      modamt[3] = rollmod * 5 / 100;

      modsum = modamt[0] + modamt[1] + modamt[2];

      modamt[0] += rollmod - modsum;

      if (dice(1,10) < 6) {

      if (roll <= (500))
        obj = read_object(64012, VIRTUAL); // stone, filler object, nothing will be loaded
      else if (roll <= (820 + modamt[0])) {
        if (dice(1, 100) <= 96)
          obj = read_object(64015, VIRTUAL); // alderwood
        else
          obj = read_object(64037, VIRTUAL); // fossilized bird egg
      }
      else if (roll <= (920 + modamt[0] + modamt[1])) {
        if (dice(1, 100) <= 96)
          obj = read_object(64031, VIRTUAL); // yew
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
      }
      else if (roll <= (970 + modamt[0] + modamt[1] + modamt[2])) {
        if (dice(1, 100) <= 96)
          obj = read_object(64032, VIRTUAL); // oak
        else
          obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      }
      else {
        if (dice(1, 100) <= 96)
          obj = read_object(64016, VIRTUAL); // darkwood
        else
          obj = read_object(64040, VIRTUAL); // fossilized dragon egg
      }
      } else {
      if (roll <= (500))
        obj = read_object(64012, VIRTUAL); // stone, filler object, nothing will be loaded
      else if (roll <= (820 + modamt[0])) {
        if (dice(1, 100) <= 96) {
          obj = read_object(64014, VIRTUAL); // low quality hide
        }
        else
          obj = read_object(64037, VIRTUAL); // fossilized bird egg
      }
      else if (roll <= (920 + modamt[0] + modamt[1])) {
        if (dice(1, 10) <= 96) {
          obj = read_object(64029, VIRTUAL); // medium quality hide
        }
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
      }
      else if (roll <= (970 + modamt[0] + modamt[1] + modamt[2])) {
        if (dice(1, 100) <= 96) {
          obj = read_object(64030, VIRTUAL); // high quality hide
        }
        else
          obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      }
      else {
        if (dice(1, 100) <= 96) {
          obj = read_object(64025, VIRTUAL); // dragon hide
        }
        else
          obj = read_object(64040, VIRTUAL); // fossilized dragon egg
      }
      } // end dice(1, 10) < 6
    }
    else {
      send_to_char(ch, "You can't forest here.\r\n");
      return;
    }
  }
  if (farm) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FARM) || SECT(IN_ROOM(ch)) == SECT_FIELD) {
      times_harvested[world[IN_ROOM(ch)].number]++;
      roll = dice(1, 1000);
      skillnum = SKILL_FARMING;
      rollmod = get_skill_value(ch, SKILL_FARMING);
      if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FARM_RICH))
        rollmod += 25;
      else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FARM_ABUNDANT))
        rollmod += 10;

      rollmod *= 10;

      roll += rollmod;

      modamt[0] = rollmod * 60 / 100;
      modamt[1] = rollmod * 25 / 100;
      modamt[2] = rollmod * 10 / 100;
      modamt[3] = rollmod * 5 / 100;

      modsum = modamt[0] + modamt[1] + modamt[2];

      modamt[0] += rollmod - modsum;

      if (roll <= (500))
        obj = read_object(64012, VIRTUAL); // stone, filler object, nothing will be loaded
      else if (roll <= (820 + modamt[0])) {
        if (dice(1, 100) <= 96)
          obj = read_object(64020, VIRTUAL); // hemp
        else
          obj = read_object(64037, VIRTUAL); // fossilized bird egg
      }
      else if (roll <= (920 + modamt[0] + modamt[1])) {
        if (dice(1, 100) <= 96) {
          if ((dice(1, 20) % 2) == 0)
            obj = read_object(64024, VIRTUAL); // wool
          else
            obj = read_object(64017, VIRTUAL); // cotton
        }
        else
          obj = read_object(64038, VIRTUAL); // fossilized giant lizard egg
      }
      else if (roll <= (970 + modamt[0] + modamt[1] + modamt[2])) {
        if (dice(1, 100) <= 96) {
          obj = read_object(64021, VIRTUAL); // velvet
        }
        else
          obj = read_object(64039, VIRTUAL); // fossilized wyvern egg
      }
      else {
        if (dice(1, 100) <= 96) {
          if ((dice(1, 20) % 2) == 0)
            obj = read_object(64022, VIRTUAL); // satin
          else
            obj = read_object(64023, VIRTUAL); // silk
        }
        else
          obj = read_object(64040, VIRTUAL); // fossilized dragon egg
      }
    }
    else {
      send_to_char(ch, "You can't farm here.\r\n");
      return;
    }
  }

  if (obj == NULL)
    obj = read_object(64012, VIRTUAL);
 

  GET_CRAFTING_TYPE(ch) = subcmd;
  GET_CRAFTING_TICKS(ch) = 1;
  GET_CRAFTING_OBJ(ch) = obj;
  ch->player_specials->crafting_exp_mult = get_skill_value(ch, skillnum) / 2;

  // Tell the character they made something. 
  sprintf(buf, "You begin to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_CHAR);

  // Tell the room the character made something. 
  sprintf(buf, "$n begins to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_ROOM);
    return;
  

}
/*

ACMD(do_assemble)
{

  int i;
  struct obj_data *obj;
  char buf[100]={'\0'};
  char arg2[100]={'\0'};
  int is_weapon = FALSE;
  int is_armor = FALSE;
  int is_misc = FALSE;


  skip_spaces(&argument);

    if (!*argument) {
      send_to_char(ch, "Type your craft command followed by list to see what you can make.\r\n");
      return;
    }

    if (GET_CRAFT_MATERIAL(ch) == NULL) {
      send_to_char(ch, "You first have to select a resource type from your inventory using the setcraftmaterial command.\r\n");
      return;
    }

    for (i = 0; i < NUM_WEAPON_TYPES; i++) {
      if (is_abbrev(argument, weapon_list[i].name)) {
        is_weapon = TRUE;
        break;
      }
    }

    if (!is_weapon) {
      for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++) {
        if (is_abbrev(argument, armor_list[i].name)) {
          is_armor = TRUE;
          break;
        }
      }
    }

    if (!is_weapon && !is_armor && !is_misc) {
      for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++) {
        if (is_abbrev(argument, craft_names[i])) {
          is_misc = TRUE;
          break;
        }
      }
    }

    CREATE(obj, struct obj_data, 1);

    if (is_weapon) {

      if (!valid_craft_material(GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch)), subcmd, weapon_list[i].material) || 
          GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch) == MATERIAL_DRAGONHIDE) {
        send_to_char(ch, "You cannot create a %s out of %s.\r\n", weapon_list[i].name, material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]);
        return;
      }

      obj = read_object(218, VIRTUAL);
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);

      sprintf(buf, "%s %s", AN(material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]), material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))], weapon_list[i].name);
      obj->short_description = strdup(buf);
      sprintf(buf, "%s %s lies here.", AN(material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]), material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))], weapon_list[i].name);
      obj->description = strdup(buf);
      sprintf(buf, "%s %s", weapon_list[i].name, material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]);
      obj->name = strdup(buf);
      GET_OBJ_TYPE(obj) = ITEM_WEAPON;
      set_weapon_values(obj, i);
    }
    else if (is_armor) {

      if (!valid_craft_material(GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch)), subcmd, armor_list[i].material)) {
        send_to_char(ch, "You cannot create a %s out of %s.\r\n", armor_list[i].name, material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]);
        return;
      }

      if (armor_list[i].armorType == ARMOR_TYPE_SHIELD) {

        obj = read_object(213, VIRTUAL);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);


        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_SHIELD);

        sprintf(buf, "%s %s", a_or_an(armor_list[i].name), armor_list[i].name);
        obj->short_description = strdup(buf);
        sprintf(buf, "%s %s lies here.", a_or_an(armor_list[i].name), armor_list[i].name);
        obj->description = strdup(buf);
        sprintf(buf, "%s %s", armor_list[i].name, material_names[armor_list[i].material]);
        obj->name = strdup(buf);
      }
      else {

        obj = read_object(200, VIRTUAL);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE); 

        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY);

        sprintf(buf, "a suit of %s",  armor_list[i].name);
        obj->short_description = strdup(buf);
        sprintf(buf, "A suit of %s lies here.", armor_list[i].name);
        obj->description = strdup(buf);
        sprintf(buf, "%s %s", armor_list[i].name, material_names[armor_list[i].material]);
        obj->name = strdup(buf);
      }
      GET_OBJ_TYPE(obj) = ITEM_ARMOR;
      set_armor_values(obj, i);
    }
    else {

      if (!valid_craft_material(GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch)), subcmd, craft_materials[i])) {
        send_to_char(ch, "You cannot create a %s out of %s.\r\n", weapon_list[i].name, material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]);
        return;
      }
        obj = read_object(297, VIRTUAL);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE); 

        SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
        SET_BIT_AR(GET_OBJ_WEAR(obj), craft_wears[i]);

        sprintf(buf, "a %s %s",  material_names[(GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))], craft_names[i]);
        obj->short_description = strdup(buf);
        sprintf(buf, "A %s %s lies here.", material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))], craft_names[i]);
        obj->description = strdup(buf);
        sprintf(buf, "%s %s", craft_names[i], material_names[GET_OBJ_MATERIAL(GET_CRAFT_MATERIAL(ch))]);
        obj->name = strdup(buf);
    }
    else if (!is_armor && !is_weapon && !is_misc) {
      send_to_char(ch, "That is not a proper item name.  Please type list to see what is available.\r\n");
      return;
    }

  GET_CRAFTING_TYPE(ch) = subcmd;
  GET_CRAFTING_TICKS(ch) = GET_ADMLEVEL(ch) ? 1 : 3;
  GET_CRAFTING_OBJ(ch) = obj;

  // Tell the character they made something. 
  sprintf(buf, "You begin to %s $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_CHAR);

  // Tell the room the character made something. 
  sprintf(buf, "$n begins to %s $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
    return;

}
*/
ACMD(do_assemble)
{
    long lVnum = NOTHING;
    struct obj_data *pObject = NULL;
    char buf[MAX_STRING_LENGTH]={'\0'};
    long i = 0;
    long j = 0;
    long lRnum = NOTHING;

    skip_spaces(&argument);

    if (*argument == '\0') 
    {
        send_to_char(ch, "What would you like to %s?  Type %s list for a list of what you can make.\r\n", CMD_NAME, CMD_NAME);
        return;
    }
    else if (isdigit(argument[0])) 
    {
        send_to_char(ch, "When using vnums as your keyword precede the vnum with the word obj.  ie. obj60000.\r\n");
        return;
    } 
    else if (!strcmp(argument, "list")) 
    {

        if( g_pAssemblyTable == NULL )
        {
            return;
        }

        send_to_char(ch, "%-40s\r\n----------------------------------------\r\n", "Object Name (copy and paste for best results)");

        for( i = 0; i < g_lNumAssemblies; i++ )
        {
            if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) < 0 )
            {
                log( "SYSERR: assemblyFindAssembly(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum );
            }
            else if(g_pAssemblyTable[i].uchAssemblyType == (unsigned char) subcmd && 
                obj_proto[lRnum].level <= get_skill_value(ch, assembly_skills[subcmd]))
            {
                send_to_char(ch, "%-40s\r\n", obj_proto[lRnum].short_description);
            }
        }
        return;
    } 
    else if ((lVnum = assemblyFindAssembly(argument)) < 0) 
    {
        if (subcmd == SCMD_LIST_COMPONENTS)
        {
            send_to_char(ch, "There is no such craft for that item.\r\n");
        }
        else
        {
            send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
        }
        return;
    }
    else if (subcmd == SCMD_LIST_COMPONENTS) 
    {

        for( i = 0; i < g_lNumAssemblies; i++ )
        {
            if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) < 0 )
            {
                send_to_char(ch, "[-----] ***RESERVED***\r\n");
                log( "SYSERR: assemblyListToChar(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum);
            }
            else if (g_pAssemblyTable[ i ].lVnum == lVnum)
            {
                send_to_char(ch, "The components needed to craft %s are:\r\n", obj_proto[ lRnum ].short_description);
                for( j = 0; j < g_pAssemblyTable[ i ].lNumComponents; j++ )
                {
                    if( (lRnum = real_object( g_pAssemblyTable[ i ].pComponents[ j ].lVnum )) < 0 )
                    {
                        send_to_char(ch, " -----: ***RESERVED***\r\n");
                        log( "SYSERR: assemblyListToChar(): Invalid component vnum #%ld in assembly for vnum #%ld.",
                         g_pAssemblyTable[ i ].pComponents[ j ].lVnum, g_pAssemblyTable[ i ].lVnum );
                    }
                    else
                    {
                        sprintf( buf, "   %-40.40s Extract=%-3.3s InRoom=%-3.3s\r\n",
                         obj_proto[ lRnum ].short_description,
                         (g_pAssemblyTable[ i ].pComponents[ j ].bExtract ? "Yes" : "No"),
                         (g_pAssemblyTable[ i ].pComponents[ j ].bInRoom  ? "Yes" : "No") );
                        send_to_char(ch, "%s", buf);
                    }
                }
                send_to_char(ch, "\r\n");
                if ((pObject = read_object(lVnum, VIRTUAL)) != NULL ) {
                    spell_identify(GET_LEVEL(ch), ch, ch, pObject, NULL);
                }
                break;
            }
        }


        return;
    }


/* Create the assembled object. */
    if ((pObject = read_object(lVnum, VIRTUAL)) == NULL ) {
        send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
        return;
    }
    add_unique_id(pObject);

    if (assemblyGetType(lVnum) != subcmd) {
        send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
        return;
    } else if (!assemblyCheckComponents(lVnum, ch)) {
        send_to_char(ch, "You haven't got all the things you need.\r\n");
        return;
    }

    if (!GET_SKILL(ch, assembly_skills[subcmd])) {
        send_to_char(ch, "You don't know how to %s.\r\n", CMD_NAME);
        return;
    }

    GET_CRAFTING_TYPE(ch) = subcmd;
    GET_CRAFTING_TICKS(ch) = 6 + GET_OBJ_LEVEL(pObject) - HAS_FEAT(ch, FEAT_FAST_CRAFTER);
    GET_CRAFTING_OBJ(ch) = pObject;

/* Tell the character they made something. */
    sprintf(buf, "You begin to %s $p.", CMD_NAME);
    act(buf, FALSE, ch, pObject, NULL, TO_CHAR);

/* Tell the room the character made something. */
    sprintf(buf, "$n begins to %s $p.", CMD_NAME);
    act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
}

void advance_crafting_progress(struct char_data *ch, int cmd) {

  int roll = 0;
  int skill = 0;
  int over = 0;
  int dc = 0;
  int mult = 1;
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];

  if (ch->player_specials->craftingType == SCMD_FORGE)
    skill = SKILL_BLACKSMITHING;
  
  roll = skill_roll(ch, skill);

  if (roll > (dc = (10 + GET_OBJ_LEVEL(ch->player_specials->craftingObject)))) {
    over = ((roll - dc) / 5);
    if (over > 0)
      mult += over;
    ch->player_specials->craftingProgress += roll * dc * mult * 10;
    sprintf(to_char, "You continue to %s $p.", CMD_NAME);
    sprintf(to_room, "$n continues to %s $p.", CMD_NAME);
  }
  else {
    if ((dc - roll) >= 5) {
      ch->player_specials->craftingProgress -= (dc - roll) * 10 * 10;
      sprintf(to_char, "You undermine your progress to %s $p.", CMD_NAME);
      sprintf(to_room, "$n undermines $s progress to %s $p.", CMD_NAME);
    }
    else {
      sprintf(to_char, "Your attempt to %s $p makes no progress.", CMD_NAME);
      sprintf(to_room, "$n's attempt to %s $p makes no progress.", CMD_NAME);
    }
  }

  act(to_char, FALSE, ch, ch->player_specials->craftingObject, NULL, TO_CHAR);
  act(to_room, FALSE, ch, ch->player_specials->craftingObject, NULL, TO_ROOM);

}

void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
{

  if (!drop_otrigger(obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;

  extern int circle_copyover;

  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot put things into containers (to prevent loss of crafting materials).\r\n");
    return;
  }


  if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) &&
      (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) > 0) &&
      (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && IN_ROOM(cont) != NOWHERE)
    act("You can't get $p out of your hand.", FALSE, ch, obj, NULL, TO_CHAR);
  else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP)) {
      SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
      act("You get a strange feeling as you put $p in $P.", FALSE,
                ch, obj, cont, TO_CHAR);
    } else
      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    /* If object placed in portal or vehicle, move it to the portal destination */
    if ((GET_OBJ_TYPE(cont) == ITEM_PORTAL) ||
        (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)) {
      obj_from_obj(obj);
      obj_to_room(obj, real_room(GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)));
      if (GET_OBJ_TYPE(cont) == ITEM_PORTAL) {
        act("What? $U$p disappears from $P in a puff of smoke!", 
	     TRUE, ch, obj, cont, TO_ROOM);
        act("What? $U$p disappears from $P in a puff of smoke!", 
	     FALSE, ch, obj, cont, TO_CHAR);
      }
    }
  }
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  } else {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(ch, "You can only put things into one container at a time.\r\n");
  else if (!*thecont) {
    send_to_char(ch, "What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
  } else {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
    else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && 
             (GET_OBJ_TYPE(cont) != ITEM_PORTAL) &&
           (GET_OBJ_TYPE(cont) != ITEM_VEHICLE) )
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
      send_to_char(ch, "You'd better open it first!\r\n");
    else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying)))
	  send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
	else if (obj == cont && howmany == 1)
	  send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
	else {
	  while (obj && howmany) {
	    next_obj = obj->next_content;
            if (obj != cont) {
              howmany--;
	      perform_put(ch, obj, cont);
            }
	    obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
	  }
	}
      } else {
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char(ch, "You don't seem to have anything to put in it.\r\n");
	  else
	    send_to_char(ch, "You don't seem to have any %ss.\r\n", theobj);
	}
      }
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}



int can_take_obj(struct char_data *ch, struct obj_data *obj)
{
  if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    act("You can't take anything, you're a ghost!", FALSE, ch, obj, 0, TO_CHAR);
    return(0);
  } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return(0);
  } else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  return (1);
}


void get_check_money(struct char_data *ch, struct obj_data *obj)
{
  int value = GET_OBJ_VAL(obj, VAL_MONEY_SIZE);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  extract_obj(obj);

  value *= (100 + GET_RP_GOLD_BONUS(ch));
  value /= 100;

  GET_GOLD(ch) += value;

  if (value == 1)
    send_to_char(ch, "There was 1 coin.\r\n");
  else
    send_to_char(ch, "There were %d coins.\r\n", value);
}


void perform_get_from_container(struct char_data *ch, struct obj_data *obj,
				     struct obj_data *cont, int mode)
{
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch)) { 
      obj_from_obj(obj);
      obj_to_char(obj, ch);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      if (IS_NPC(ch)) {
        item_check(obj, ch);
      }
      get_check_money(ch, obj);
    }
  }
}


void get_from_container(struct char_data *ch, struct obj_data *cont,
			     char *arg, int mode, int howmany)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains))) {
      char buf[MAX_STRING_LENGTH]={'\0'};

      snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      struct obj_data *obj_next;
      while (obj && howmany--) {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
	act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else {
        char buf[MAX_STRING_LENGTH]={'\0'};

	snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.", arg);
	act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}


int perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{

  if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    if (IS_NPC(ch))
      item_check(obj, ch);
    get_check_money(ch, obj);
  }
  return (0);
}

char *find_exdesc_keywords(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->keyword);

  return (NULL);
}

void get_from_room(struct char_data *ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;
  char *descword;

  /* Are they trying to take something in a room extra description? */
  if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
    send_to_char(ch, "You can't take %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PLAYER_SHOP) && !House_can_enter(ch, world[IN_ROOM(ch)].number)) {
    send_to_char(ch, "You cannot get items from a shop that you do not own or have guest priviledges to.\r\n");
    return;
  }
  
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
   if ((descword = find_exdesc_keywords(arg, world[IN_ROOM(ch)].ex_description)) != NULL)
     send_to_char(ch, "%s: you can't take that!\r\n", fname(descword));
   else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    else {
      struct obj_data *obj_next;
      while(obj && howmany--) {
	obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
	send_to_char(ch, "There doesn't seem to be anything here.\r\n");
      else
	send_to_char(ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}



ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (!*arg1)
    send_to_char(ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2);	/* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3);	/* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont)
	send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      else if (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)
        send_to_char(ch, "You will need to enter it first.\r\n");
      else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && 
           !((GET_OBJ_TYPE(cont) == ITEM_PORTAL) && (OBJVAL_FLAGGED(cont, CONT_CLOSEABLE))))
	act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
	get_from_container(ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
	send_to_char(ch, "Get from all of what?\r\n");
	return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	  }
	}
      for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
	}
      if (!found) {
	if (cont_dotmode == FIND_ALL)
	  send_to_char(ch, "You can't seem to find any containers.\r\n");
	else
	  send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}


void perform_drop_gold(struct char_data *ch, int amount,
		            byte mode, room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (GET_GOLD(ch) < amount)
    send_to_char(ch, "You don't have that many coins!\r\n");
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
	send_to_char(ch, "You throw some gold into the air where it disappears in a puff of smoke!\r\n");
	act("$n throws some gold into the air where it disappears in a puff of smoke!",
	    FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        char buf[MAX_STRING_LENGTH]={'\0'};

        if (!drop_wtrigger(obj, ch)) {
          extract_obj(obj);
          return;
        }

        if (!drop_wtrigger(obj, ch) && (obj)) { /* obj may be purged */
          extract_obj(obj);
          return;
        }

	snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);

	send_to_char(ch, "You drop some gold.\r\n");
	obj_to_room(obj, IN_ROOM(ch));
      }
    } else {
      char buf[MAX_STRING_LENGTH]={'\0'};

      snprintf(buf, sizeof(buf), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      send_to_char(ch, "You drop some gold which disappears in a puff of smoke!\r\n");
    }
    GET_GOLD(ch) -= amount;
    convert_coins(ch);
  }
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")

int perform_drop(struct char_data *ch, struct obj_data *obj,
		     byte mode, const char *sname, room_rnum RDR)
{
  char buf[MAX_STRING_LENGTH]={'\0'};
  int value = 0;

  if (!drop_otrigger(obj, ch))
    return 0;
  
  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  snprintf(buf, sizeof(buf), "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);

  snprintf(buf, sizeof(buf), "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);

  if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode) {
  case SCMD_DROP:
    obj_to_room(obj, IN_ROOM(ch));
    return (0);
  case SCMD_DONATE:
    obj_to_room(obj, RDR);
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    /*  SYSERR_DESC:
     *  This error comes from perform_drop() and is output when perform_drop()
     *  is called with an illegal 'mode' argument.
     */
    break;
  }

  return (0);
}



ACMD(do_drop)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi, num_don_rooms;
  const char *sname;

  switch (subcmd) {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    /* fail + double chance for room 1   */
    num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +       
                    (CONFIG_DON_ROOM_2 != NOWHERE)     +
                    (CONFIG_DON_ROOM_3 != NOWHERE)     + 1 ; 
    switch (rand_number(0, num_don_rooms)) {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = real_room(CONFIG_DON_ROOM_1);
      break;
    case 3: RDR = real_room(CONFIG_DON_ROOM_2); break;
    case 4: RDR = real_room(CONFIG_DON_ROOM_3); break;

    }
    if (RDR == NOWHERE) {
      send_to_char(ch, "Sorry, you can't donate anything right now.\r\n");
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PLAYER_SHOP) && !House_can_enter(ch, world[IN_ROOM(ch)].number)) {
    send_to_char(ch, "You cannot drop items in a shop that you do not own or have guest priviledges to.\r\n");
    return;
  }


  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", sname);
    return;
  } else if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
      perform_drop_gold(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      do {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
	send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
	send_to_char(ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be carrying anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  amount += perform_drop(ch, obj, mode, sname, RDR);
	}
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
	send_to_char(ch, "What do you want to %s all of?\r\n", sname);
	return;
      }
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);

      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK)) {
    send_to_char(ch, "You have been rewarded by the gods!\r\n");
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_GOLD(ch) += amount;
  }
}


void perform_give(struct char_data *ch, struct char_data *vict,
		       struct obj_data *obj)
{
  if (!give_otrigger(obj, ch, vict)) 
    return;
  if (!receive_mtrigger(vict, ch, obj))
    return;

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

  autoquest_trigger_check( ch, vict, obj, AQ_OBJ_RETURN);
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(ch, "To who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (NULL);
}
/* utility function for give */

void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount)
{
  char buf[MAX_STRING_LENGTH]={'\0'};

  if (amount <= 0) {
    send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))) {
    send_to_char(ch, "You don't have that many coins!\r\n");
    return;
  }
  send_to_char(ch, "%s", CONFIG_OK);

  snprintf(buf, sizeof(buf), "$n gives you %s coin%s.", change_coins(amount), amount == 1 ? "" : "s");
  act(buf, FALSE, ch, 0, vict, TO_VICT);

  snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))
    GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;

  bribe_mtrigger(vict, ch, amount);
}

ACMD(do_give)
{
  char arg[MAX_STRING_LENGTH];
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Give what to who?\r\n");
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL)
	perform_give_gold(ch, vict, amount);
      return;
    } else if (!*arg)	/* Give multiple code. */
      send_to_char(ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(ch, argument)))
      return;
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) 
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      while (obj && amount--) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	perform_give(ch, vict, obj);
	obj = next_obj;
      }
    }
  } else {
    char buf1[MAX_INPUT_LENGTH]={'\0'};

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
	send_to_char(ch, "All of what?\r\n");
	return;
      }
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be holding anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (CAN_SEE_OBJ(ch, obj) &&
	      ((dotmode == FIND_ALL || isname(arg, obj->name))))
	    perform_give(ch, vict, obj);
	}
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}


void weight_change_object(struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (IN_ROOM(obj) != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
    /*  SYSERR_DESC:
     *  weight_change_object() outputs this error when weight is attempted to
     *  be removed from an object that is not carried or in another object.
     */
  }
}



void name_from_drinkcon(struct obj_data *obj)
{
  char *new_name, *cur_name, *next;
  char *liqname;
  int liqlen, cpylen;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  liqname = (char *) drinknames[GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)];
  if (!isname(liqname, obj->name)) {
    log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
    /*  SYSERR_DESC:
     *  From name_from_drinkcon(), this error comes about if the object
     *  noted (by keywords and item vnum) does not contain the liquid string
     *  being searched for.
     */
    return;
  }

  liqlen = strlen(liqname);
  CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

  for (cur_name = obj->name; cur_name; cur_name = next) {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strn_cmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " ");	/* strcat: OK (size precalculated) */
    strncat(new_name, cur_name, cpylen);	/* strncat: OK (size precalculated) */
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}



void name_to_drinkcon(struct obj_data *obj, int type)
{
  char *new_name;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", obj->name, drinknames[type]);	/* sprintf: OK */

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);

  obj->name = new_name;
}



ACMD(do_drink)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight;
  int on_ground = 0;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    send_to_char(ch, "Drink from what?\r\n");
    return;
  }
  if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    } else
      on_ground = 1;
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char(ch, "You can't drink from that!\r\n");
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char(ch, "You have to be holding that to drink from it.\r\n");
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char(ch, "You can't seem to get close enough to your mouth.\r\n");
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
    send_to_char(ch, "Your stomach can't contain anymore!\r\n");
    return;
  }
  if (!GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL)) {
    send_to_char(ch, "It's empty.\r\n");
    return;
  }
  
  if (!consume_otrigger(temp, ch, OCMD_DRINK))  /* check trigger */
    return;
    
  if (subcmd == SCMD_DRINK) {
    char buf[MAX_STRING_LENGTH]={'\0'};

    snprintf(buf, sizeof(buf), "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    if (temp->action_description)
      act(temp->action_description, TRUE, ch, temp, 0, TO_CHAR);

    if (drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK];
    else
      amount = rand_number(3, 10);

  } else {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    amount = 1;
  }

  amount = MIN(amount, GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL));

  /* You can't subtract more than the object weighs
     Objects that are eternal (max capacity -1) don't get a
     weight subtracted */
  if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
  {
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));
  weight_change_object(temp, -weight);	/* Subtract amount */
  }

  gain_condition(ch, DRUNK,  drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK]  * amount / 4);
  gain_condition(ch, FULL,   drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][FULL]   * amount / 4);
  gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][THIRST] * amount / 4);

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You feel drunk.\r\n");

  if (GET_COND(ch, THIRST) > 20)
    send_to_char(ch, "You don't feel thirsty any more.\r\n");

  if (GET_COND(ch, FULL) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(temp, VAL_DRINKCON_POISON)) {	/* The crap was poisoned ! */
    send_to_char(ch, "Oops, it tasted rather strange!\r\n");
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 3;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  /* empty the container, and no longer poison.
     Only remove if it's max capacity > 0, not eternal */
  if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
  {
  GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL) -= amount;
  if (!GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL)) {	/* The last bit */
    name_from_drinkcon(temp);
    GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID) = 0;
    GET_OBJ_VAL(temp, VAL_DRINKCON_POISON) = 0;
  }
  }
  return;
}



ACMD(do_eat)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    send_to_char(ch, "Eat what?\r\n");
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && GET_ADMLEVEL(ch) == 0) {
    send_to_char(ch, "You can't eat THAT!\r\n");
    return;
  }
  if (GET_COND(ch, FULL) > 20) {/* Stomach full */
    send_to_char(ch, "You are too full to eat more!\r\n");
    return;
  }

  if (!consume_otrigger(food, ch, OCMD_EAT))  /* check trigger */
    return;

  if (subcmd == SCMD_EAT) {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    if (food->action_description)
      act(food->action_description, FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, VAL_FOOD_FOODVAL) : 1);

  gain_condition(ch, FULL, amount);

  if (GET_COND(ch, FULL) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(food, VAL_FOOD_POISON) && GET_ADMLEVEL(ch) == 0) {
    /* The crap was poisoned ! */
    send_to_char(ch, "Oops, that tasted rather strange!\r\n");
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else {
    if (!(--GET_OBJ_VAL(food, VAL_FOOD_FOODVAL))) {
      send_to_char(ch, "There's nothing left now.\r\n");
      extract_obj(food);
    }
  }
}


ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) {		/* No arguments */
      send_to_char(ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char(ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) {		/* no arguments */
      send_to_char(ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) {		/* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) {	/* pour */
    if (!*arg2) {
      send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!str_cmp(arg2, "out")) {
      if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
      {
      act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
      act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */

      name_from_drinkcon(from_obj);
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
      }
      else
      {
        send_to_char(ch, "You can't possibly pour that container out!\r\n");
      }

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char(ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) != 0) &&
      (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) != GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID))) {
    send_to_char(ch, "There is already another liquid in it!\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) < 0) ||
      (!(GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) < GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY)))) {
    send_to_char(ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(ch, "You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)], arg2);

  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) = GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID);

  /* Then how much to pour */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
  {
  GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) -= (amount =
			 (GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL)));

  GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) = GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);

  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) < 0) {	/* There was too little */
    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
    amount += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
    name_from_drinkcon(from_obj);
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
  }
  }
  else
  {
    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) = GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);
  }

  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) =
    (GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) || GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON));

  /* And the weight boogie for non-eternal from_objects */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
  { 
  weight_change_object(from_obj, -amount);
  }
  weight_change_object(to_obj, amount);	/* Add weight */
}



void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
  const char *wear_messages[][2] = {
    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n slides $p on to $s right ring finger.",
    "You slide $p on to your right ring finger."},

    {"$n slides $p on to $s left ring finger.",
    "You slide $p on to your left ring finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p on $s body.",
    "You wear $p on your body."},

    {"$n wears $p on $s head.",
    "You wear $p on your head."},

    {"$n puts $p on $s legs.",
    "You put $p on your legs."},

    {"$n wears $p on $s feet.",
    "You wear $p on your feet."},

    {"$n puts $p on $s hands.",
    "You put $p on your hands."},

    {"$n wears $p on $s arms.",
    "You wear $p on your arms."},

    {"$n straps $p around $s arm as a shield.",
    "You start to use $p as a shield."},

    {"$n wears $p about $s body.",
    "You wear $p around your body."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n puts $p on around $s right wrist.",
    "You put $p on around your right wrist."},

    {"$n puts $p on around $s left wrist.",
    "You put $p on around your left wrist."},

    {"$n wields $p.",
    "You wield $p."},

    {"$n grabs $p.",
    "You grab $p."},

    {"$n covers $s face with $p.",
    "You wear $p over your face."},

    {"$n puts $p in $s right ear.",
    "You put $p in your right ear."},

    {"$n puts $p in $s left ear.",
    "You put $p in your left ear."},

	{"$n wears $p around $s right ankle.",
	"You wear $p around your right ankle."},
	
	{"$n wears $p around $s left ankle.",
	"You wear $p around your left ankle."},

	{"$n activates $p and it begins to hover above $s head.",
	"You activate $p and it begins to hover above your head."},

	{"$n straps $p to $s back.",
	"You strap $p to your back."},

	{"$n wears $p on $s shoulders.",
	"You wear $p on your shoulders."},	
	
	{"$n wears $p in $s nose.",
	"You wear $p in your nose."},

	{"$n hangs $p on $s belt.",
	"You hang $p on your belt."},

	{"$n slings $p across $s back.",
	"You sling $p across your back"},

	{"$n slings $p across $s back.",
	"You sling $p across your back"},

	{"$n slings $p across $s back.",
	"You sling $p across your back"},

	{"$n wears $p around $s waist.",
	"You wear $p around your waist."},

	{"$n straps $p to $s hand as a sheath.",
	"You strap $p to your hand as a sheath."},

	{"$n straps $p to $s back as a sheath.",
	"You strap $p to your back as a sheath."},
	
	{"$n straps $p to $s back as a sheath.",
	"You strap $p to your back as a sheath."},	

	{"$n hangs $p on $s right hip as a sheath.",
	"You hang $p on your right hip as a sheath."},

	{"$n hangs $p on $s left hip as a sheath.",
	"You hang $p on your left hip as a sheath."},	
	
	{"$n straps $p aside $s right bicep as a sheath.",
	"You strap $p aside your right bicep as a sheath."},
	
	{"$n straps $p aside $s left bicep as a sheath.",
	"You strap $p aside your left bicep as a sheath."},	

	{"$n straps $p inside $s right forearm as a sheath.",
	"You strap $p inside your right forearm as a sheath."},
	
	{"$n straps $p inside $s left forearm as a sheath.",
	"You strap $p inside your left forearm as a sheath."},	
	
	{"$n fits $s mount with $p as barding.",
	"You fit your mount with $p as barding."},

    {"$n wields $p in $s main hand.",
    "You wield $p in your main hand."},

    {"$n wields $p in $s offhand.",
    "You wield $p in your offhand."},

	{"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES1",
	"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES1"},
	
	{"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES2",
	"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES2"},

	{"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES3",
	"IF YOU SEE THIS MESSAGE PLEASE REPORT IT TO A STAFF MEMBER AS CODE WEARMES3"},

	{"$n fits $s mount with $p as barding.",
	"You fit your mount with $p as barding."}

  };

  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

int hands(struct char_data * ch)
{
  int x;

  if (GET_EQ(ch, WEAR_WIELD1)) {
    if ((GET_EQ(ch, WEAR_WIELD) && IS_SET(weapon_list[GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 0)].weaponFlags, WEAPON_FLAG_DOUBLE)) ||
        (IS_BOW(GET_EQ(ch, WEAR_WIELD))) || 
        ((OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD1), ITEM_2H)|| wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD1)) == WIELD_TWOHAND)
        && !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), 0)))) {
      x = 2;
    } else
    x = 1;
  } else x = 0;

  if (GET_EQ(ch, WEAR_WIELD2)) {
    if ((OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD2), ITEM_2H) || wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD2)) == WIELD_TWOHAND) 
        && !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), 0))) {
      x += 2;
    } else
    x += 1;
  }

  if (GET_EQ(ch, WEAR_SHIELD)) {
    x += 1;
  }



  return x;
}

void perform_wear(struct char_data *ch, struct obj_data *obj, int where)
{
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] = {
    ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
    ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
    ITEM_WEAR_TAKE, ITEM_WEAR_TAKE, ITEM_WEAR_MASK, ITEM_WEAR_EAR,
    ITEM_WEAR_EAR, ITEM_WEAR_ANKLE, ITEM_WEAR_ANKLE, ITEM_WEAR_ABOVE,
    ITEM_WEAR_BACK, ITEM_WEAR_SHOULDER, ITEM_WEAR_NOSE, ITEM_WEAR_ONBELT,
    ITEM_WEAR_ONBACK, ITEM_WEAR_ONBACK, ITEM_WEAR_WAIST, ITEM_WEAR_ONBACK,
    ITEM_WEAR_ONBACK, ITEM_WEAR_ONBACK, ITEM_WEAR_WAIST, ITEM_WEAR_ONBACK,
    ITEM_WEAR_ONBACK, ITEM_WEAR_ONBACK, ITEM_WEAR_WAIST, ITEM_WEAR_ONBACK,
    ITEM_WEAR_ONBACK, ITEM_WEAR_BARDING  
  };

  const char *already_wearing[] = {
    "You're already using a light.\r\n",
    " YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR1\r\n",
    "You're already wearing something on both of your ring fingers.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR2.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "You're already wearing something on your body.\r\n",
    "You're already wearing something on your head.\r\n",
    "You're already wearing something on your legs.\r\n",
    "You're already wearing something on your feet.\r\n",
    "You're already wearing something on your hands.\r\n",
    "You're already wearing something on your arms.\r\n",
    "You're already using a shield.\r\n",
    "You're already wearing something about your body.\r\n",
    "You already have something around your waist.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR3.\r\n",
    "You're already wearing something around both of your wrists.\r\n",
    "You're already wielding a weapon.\r\n",
    "You're already holding something.\r\n",
    "You're already wearing something over your face.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR4.\r\n",
    "You're already wearing something in both ears.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR5.\r\n",
	"You're already wearing something on both ankles\r\n",
	"There's already something floating above your head.\r\n",
	"You're already wearing something on your back.\r\n",
	"You're already wearing something on your shoulders.\r\n",
	"You're already wearing something in your nose.\r\n",
	"You're already wearing something on your belt.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR6.\r\n",
    "Your mount is already wearing barding.\r\n"
    "Your mount is already wearing barding.\r\n"
    "You're already carrying too much on your back.\r\n",
	"You're already wearing too much around your waist\r\n",
	"You already have something sheathed in your off-hand\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR8.\r\n",
	"You already have two sheaths strapped across your back.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR9.\r\n",
	"You already have sheaths hanging from your both sides of your hips.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR10.\r\n",
	"You already have sheaths strapped outside your two biceps.\r\n",
    "Your mount is already wearing barding.\r\n"
    "Your mount is already wearing barding.\r\n"
	"You're already wielding a weapon in your main hand.\r\n",
	"You're already using something in your offhand.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR12.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR13.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT AS CODE WEAR14.\r\n",
    "Your mount is already wearing barding.\r\n"
  };
  /* first, make sure that the wear position is valid. */
  
  if (GET_OBJ_VAL(obj, 13) != GET_IDNUM(ch) && GET_OBJ_VAL(obj, 13) != 0) {
    send_to_char(ch, "@n%s is bound to another individual and cannot be equipped.  (See @YHELP BINDING ITEMS@n)\r\n",
                 (obj->short_description));
    return;
  }

  if ((where == WEAR_BARDING) && ch->player_specials->mounted != MOUNT_MOUNT && ch->player_specials->mounted != MOUNT_SUMMON) {
    act("You are not mounted therefore you cannot fit a mount with $p for barding.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if ((where == WEAR_BODY || where == WEAR_SHIELD) && obj->size != get_size(ch)) {
    act("@c$p@y is not the right size for you to wear, you'll have to resize it.@n", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (where == WEAR_SHIELD && hands(ch) > 1) {
    act("You cannot wear $p as a shield while dual wielding.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (where == WEAR_WIELD2 && hands(ch) > 1) {
    act("You cannot dual wield $p when using a shield.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (!CAN_WEAR(obj, wear_bitvectors[where]) && where != WEAR_WIELD1 && where != WEAR_WIELD2) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (IS_NPC(ch) && !OBJ_FLAGGED(obj, ITEM_MOBITEM)) {
    send_to_char(ch, "You can only wear items specifically designed for mob use.\r\n");
	return;
  }  
  
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) || (where == WEAR_EAR_R) || (where == WEAR_WIELD1))
    if (GET_EQ(ch, where))
      where++;

  /* checks for 2H sanity */
  if ((OBJ_FLAGGED(obj, ITEM_2H)||(wield_type(get_size(ch), obj) == WIELD_TWOHAND)) && hands(ch) > 0 && GET_OBJ_TYPE(obj) == ITEM_WEAPON
      && !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(obj, 0))) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (where == WEAR_SHIELD && hands(ch) > 1) {
    send_to_char(ch, "You do not have enough hands.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && (GET_OBJ_SIZE(obj) - get_size(ch)) > 1 && 
      !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(obj, 0))) {
    send_to_char(ch, "That weapon is too large for you to wield.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_SIZE(obj) > get_size(ch) && hands(ch) > 0 && 
      !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(obj, 0))) {
    send_to_char(ch,  "You require two free hands to wield that weapon.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT && (GET_EQ(ch, WEAR_BODY) || GET_EQ(ch, WEAR_ARMS) || GET_EQ(ch, WEAR_LEGS))) {

    send_to_char(ch, "You cannot wear a suit of armor over armor already worn on your arms, body or legs.\r\n");
    return;

  }

  if (((where == WEAR_WIELD1)  ||
       (where == WEAR_WIELD2)) && (hands(ch) > 1 && !HAS_COMBAT_FEAT(ch, CFEAT_MONKEY_GRIP, GET_OBJ_VAL(obj, 0)))) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (GET_EQ(ch, where)) {
    send_to_char(ch, "%s", already_wearing[where]);
    return;
  }

  /* See if a trigger disallows it */
  if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
    return;

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);
}



int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
  int where = -1;

  const char *keywords[] = {
    "!RESERVED!",
    "finger",
    "!RESERVED!",
    "neck",
    "!RESERVED!",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "shield",
    "about",
    "waist",
    "wrist",
    "!RESERVED!",
    "!RESERVED!",
    "!RESERVED!",
    "back",
    "ear",
    "!RESERVED!",
    "ankle",
    "!RESERVED!",
    "above",
    "barding",
    "\n"
  };

  if (!arg || !*arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_HOLD))        where = WEAR_WIELD2;
    if (CAN_WEAR(obj, ITEM_WEAR_PACK))        where = WEAR_BACKPACK;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))         where = WEAR_EAR_R;
    if (CAN_WEAR(obj, ITEM_WEAR_WINGS))       where = WEAR_WINGS;
    if (CAN_WEAR(obj, ITEM_WEAR_MASK))        where = WEAR_MASK;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOVE))       where = WEAR_ABOVE;
    if (CAN_WEAR(obj, ITEM_WEAR_BARDING))     where = WEAR_BARDING;
  } else if (((where = search_block(arg, keywords, FALSE)) < 0) || (*arg=='!')) {
    send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);
    return -1;
  }
  return (where);
}



ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      send_to_char(ch, "You cannot do this while wild shaped.\r\n");
      return;
  }

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Wear what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char(ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
        if (MAX(1, GET_CLASS_LEVEL(ch)) < GET_OBJ_LEVEL(obj)) {
          act("$p: you are not experienced enough to use that.",
              FALSE, ch, obj, 0, TO_CHAR);
        } else if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
          act("$p: it seems to be broken.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
          items_worn++;
        if (GET_EQ(ch, WEAR_BODY) && where != WEAR_SHIELD && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR ||
            GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR_SUIT) && !CAN_WEAR(obj, ITEM_WEAR_BARDING) &&
            (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You are already wearing a full suit of armor.\r\n");
            return;
          }
          if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, 9)) 
            && (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          }
          if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)
            SET_BIT_AR(AFF_FLAGS(ch), AFF_WEARING_SUIT);
          perform_wear(ch, obj, where);
        }
      }
    }
    if (!items_worn)
      send_to_char(ch, "You don't seem to have anything wearable.\r\n");
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char(ch, "Wear all of what?\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (MAX(1, GET_CLASS_LEVEL(ch)) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
          if (GET_EQ(ch, WEAR_BODY) && where != WEAR_SHIELD && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR ||
            GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR_SUIT) && !CAN_WEAR(obj, ITEM_WEAR_BARDING) &&
            (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You are already wearing a full suit of armor.\r\n");
            return;
          }
          if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, 9)) 
            && (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          }
          if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)
            SET_BIT_AR(AFF_FLAGS(ch), AFF_WEARING_SUIT);
          perform_wear(ch, obj, where);
	} else
	  act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else if (MAX(1, GET_CLASS_LEVEL(ch)) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
        if (GET_EQ(ch, WEAR_BODY) && where != WEAR_SHIELD && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR ||
            GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR_SUIT) && !CAN_WEAR(obj, ITEM_WEAR_BARDING) &&
            (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You are already wearing a full suit of armor.\r\n");
            return;
          }
          if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, 9))
            && (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) {
            send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          }
          if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)
            SET_BIT_AR(AFF_FLAGS(ch), AFF_WEARING_SUIT);
          perform_wear(ch, obj, where);
      } else if (!*arg2)
	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}



ACMD(do_wield)
{
  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      send_to_char(ch, "You cannot do this while wild shaped.\r\n");
      return;
  }

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char(ch, "You can't wield that.\r\n");
    else if (GET_OBJ_WEIGHT(obj) > (gen_carry_weight(ch) / 3))
      send_to_char(ch, "It's too heavy for you to use.\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else {
      if (!IS_NPC(ch) && !is_proficient_with_weapon(ch, GET_OBJ_VAL(obj, VAL_WEAPON_SKILL))
        && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
        send_to_char(ch, "You have no proficiency with this type of weapon.\r\nYour attack accuracy will be greatly reduced.\r\n");
      perform_wear(ch, obj, WEAR_WIELD1);
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}



ACMD(do_grab)
{
  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      send_to_char(ch, "You cannot do this while wild shaped.\r\n");
      return;
  }

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      perform_wear(ch, obj, WEAR_WIELD2);
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
      GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
	  GET_OBJ_TYPE(obj) != ITEM_POTION)
	send_to_char(ch, "You can't hold that.\r\n");
      else
	perform_wear(ch, obj, WEAR_WIELD2);
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}



void perform_remove(struct char_data *ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
    /*  SYSERR_DESC:
     *  This error occurs when perform_remove() is passed a bad 'pos'
     *  (location) to remove an object from.
     */
  else if (OBJ_FLAGGED(obj, ITEM_NODROP))
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
  else {
    if (!remove_otrigger(obj, ch))
       return;
    if (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WEARING_SUIT);
    obj_to_char(unequip_char(ch, pos), ch);
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  }
}



ACMD(do_remove)
{

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
      send_to_char(ch, "You cannot do this while wild shaped.\r\n");
      return;
  }

  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  int i, dotmode, found = 0, msg;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Remove what?\r\n");
    return;
  }
  /* lemme check for a board FIRST */
  for (obj = ch->carrying; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE (obj) == ITEM_BOARD) {
      found = 1;
      break;
    }
  }
  if (!obj) {
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
      if (GET_OBJ_TYPE (obj) == ITEM_BOARD) {
	found = 1;
	break;
      }
    }
  }

  if (found) {
    if (!isdigit (*arg) || (!(msg = atoi (arg)))) {
      found = 0;
    } else {
      remove_board_msg (GET_OBJ_VNUM (obj), ch, msg);
    }
  }
  if (!found) {
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL) {
    found = 0;
      for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i)) {
	perform_remove(ch, i);
	found = 1;
      }
      }
      if (!found) {
      send_to_char(ch, "You're not using anything.\r\n");
      }
  } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
      send_to_char(ch, "Remove all of what?\r\n");
      } else {
      found = 0;
	for (i = 0; i < NUM_WEARS; i++) {
	if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	    isname(arg, GET_EQ(ch, i)->name)) {
	  perform_remove(ch, i);
	  found = 1;
	}
	}
	if (!found) {
	send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
    }
      }
  } else {
      if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0) {
      send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      } else {
      perform_remove(ch, i);
  }
    }
  }
  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
}

ACMD(do_sac) 
{ 
   char arg[MAX_INPUT_LENGTH]; 
   struct obj_data *j, *jj, *next_thing2; 

  one_argument(argument, arg); 

  if (!*arg) { 
     send_to_char(ch, "Destroy what?\n\r"); 
     return; 
   } 

  if (!(j = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(j = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) { 
     send_to_char(ch, "It doesn't seem to be here.\n\r"); 
     return; 
   } 

  if (!CAN_WEAR(j, ITEM_WEAR_TAKE)) { 
     send_to_char(ch, "You can't sacrifice that!\n\r"); 
     return; 
   } 

   act("$n destroys $p and tosses the remains aside.", FALSE, ch, j, 0, TO_ROOM); 

   send_to_char(ch, "You destroy %s and toss the remains aside.\r\n", GET_OBJ_SHORT(j));
   for (jj = j->contains; jj; jj = next_thing2) { 
     next_thing2 = jj->next_content;       /* Next in inventory */ 
     obj_from_obj(jj); 
      
     if (j->carried_by) 
       obj_to_room(jj, IN_ROOM(j)); 
     else if (IN_ROOM(j) != NOWHERE) 
       obj_to_room(jj, IN_ROOM(j)); 
     else 
       assert(FALSE); 
   } 
   extract_obj(j);
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const int max_carry_load[] = {
  0,
  10,
  20,
  30,
  40,
  50,
  60,
  70,
  80,
  90,
  100,
  115,
  130,
  150,
  175,
  200,
  230,
  260,
  300,
  350,
  400,
  460,
  520,
  600,
  700,
  800,
  920,
  1040,
  1200,
  1400,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640,
  1640
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
long max_carry_weight(struct char_data *ch)
{
  return (gen_carry_weight(ch) * 33334 / 10000);
}

long gen_carry_weight(struct char_data *ch)
{
  int abil, total = 0, i;
  int j = 0, k = 7;
  struct affected_type *af;  

	abil = MAX(1, MIN(100, GET_STR(ch)));

        abil += MAX(0, GET_CARRY_STR_MOD(ch));

	// This algorithm is a formula that closely resembles the d20 rules for max carrying weight.
	// It's advantage is that it allows for infinite strength values without much code.
	//  It is highly reccommend over the use of a presice value table for this reason.
	
  for (i = 1; i <= abil; i++) {
	  if (i <= 12) {
		  if (i % 3 == 0)
		    total += 4 + ((i - 1) / 10);
		  else
		    total += 3 + (((i - 1) / 10) * 2);
		}
		else {
		  if ((i + 1) % 3 == 0) {
			  total += k + (1 << j);
				j++;
				k = k + (j * 3);
			}
			else
			  total += k;
		}
	}

  if (get_size(ch) > SIZE_MEDIUM)
    total *= (get_size(ch) - SIZE_MEDIUM + 1);

  for (af = ch->affected; af; af = af->next)
    if (af->type == SPELL_FLOATING_DISC) {
      total += af->level * 100;
      break;
    }
  
  // We're multiplying weights by ten to account for tenths of a pound
  return total * 10;
}

void update_encumberance(struct char_data *ch)
{

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_MEDIUM);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_HEAVY);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_LIGHT);
  return;  

  if (IS_CARRYING_W(ch) <= gen_carry_weight(ch)) {
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_MEDIUM);
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_HEAVY);
	  SET_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_LIGHT);
	}
  else if (IS_CARRYING_W(ch) <= gen_carry_weight(ch) * 2) {
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_LIGHT);
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_HEAVY);
	  SET_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_MEDIUM);
	}
  else {
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_MEDIUM);
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_LIGHT);
	  SET_BIT_AR(AFF_FLAGS(ch), AFF_ENCUMBERANCE_HEAVY);
	}	
}

void calculate_current_weight(struct char_data *ch) {

  struct obj_data *obj;
  int i = 0;

  int weight = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if ((obj = GET_EQ(ch, i))) {
      weight += MAX(0, MIN(1000, GET_WEIGHT(obj)));

      if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
        weight += calculate_container_weight(obj);
      }
    }
  }

  for (obj = ch->carrying; obj; obj = obj->next_content) {
    weight += MAX(0, MIN(1000, GET_WEIGHT(obj)));

    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      weight += calculate_container_weight(obj);
    }
  }

  IS_CARRYING_W(ch) = weight;  

}

int calculate_container_weight(struct obj_data *obj2) {
 
  struct obj_data *obj = obj2;
  
  int weight = 0;

  for (obj = obj->contains; obj; obj = obj->next_content) {
    weight += MAX(0, MIN(1000, GET_WEIGHT(obj)));

    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      weight += calculate_container_weight(obj);
    }
  }

  return weight;
}

ACMD(do_lore)
{

  struct obj_data *obj;
  int roll = 0, dc = 0;
  char arg[MAX_STRING_LENGTH];
  int skill = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What item would you like to do a lore check on?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have a %s in your inventory.\r\n", arg);
        return;
  }

  skill = SKILL_KNOWLEDGE;

  roll = skill_roll(ch, skill);
  dc = 10 + GET_OBJ_LEVEL(obj);

  if (OBJ_FLAGGED(obj, ITEM_IDENTIFIED)) {
    REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NOLORE);
  }

  if (GET_ADMLEVEL(ch) == 0  && (OBJ_FLAGGED(obj, ITEM_NOLORE) || (roll < (dc + 5) && !OBJ_FLAGGED(obj, ITEM_IDENTIFIED)))) {
    send_to_char(ch, "You cannot recall any information about this item.\r\n");
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NOLORE);
    return;
  }

  send_to_char(ch, "@YYou are able to recall lore about this item.@n\r\n");

  spell_identify(GET_LEVEL(ch), ch, ch, obj, arg);

  return;

}
void start_auction(struct char_data * ch, struct obj_data * obj, int bid);
void auc_stat(struct char_data * ch, struct obj_data *obj);
void stop_auction(int type, struct char_data * ch);
void check_auction(void);
void auc_send_to_all(char *messg, bool buyer);
char buf[MAX_STRING_LENGTH]={'\0'};
ACMD(do_auction);
ACMD(do_bid);

void start_auction(struct char_data * ch, struct obj_data * obj, int bid)
{
	/* Take object from character and set variables */
	
	obj_from_char(obj);
	obj_selling = obj;
	ch_selling	= ch;
	ch_buying = NULL;
	curbid = bid;

	/* Tell th character where his item went */
	sprintf(buf, "%s magic flies away from your hands to be auctioned!\r\n", obj_selling->short_description);
	CAP(buf);
	send_to_char(ch_selling, "%s", buf);
	
	/* Anounce the item is being sold */
	sprintf(buf, auctioneer[AUC_NULL_STATE], curbid);
	auc_send_to_all(buf, FALSE);

	aucstat = AUC_OFFERING;
}

void check_auction(void)
{
	
	switch (aucstat)
	{
	case AUC_NULL_STATE:
		return;
	case AUC_OFFERING:
	{
		sprintf(buf, auctioneer[AUC_OFFERING], curbid);
		CAP(buf);
		auc_send_to_all(buf, FALSE);
		aucstat = AUC_GOING_ONCE;
		return;
	}
	case AUC_GOING_ONCE:
	{
		
		sprintf(buf, auctioneer[AUC_GOING_ONCE], curbid);
		CAP(buf);
		auc_send_to_all(buf, FALSE);
		aucstat = AUC_GOING_TWICE;
		return;
	}
	case AUC_GOING_TWICE:
	{
		
		sprintf(buf, auctioneer[AUC_GOING_TWICE], curbid);
		CAP(buf);
		auc_send_to_all(buf, FALSE);
		aucstat = AUC_LAST_CALL;
		return;
	}
	case AUC_LAST_CALL:
	{
		
		if (ch_buying == NULL) {
			
			sprintf(buf, "%s", auctioneer[AUC_LAST_CALL]);
			
			CAP(buf);
			auc_send_to_all(buf, FALSE);
			
			sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->short_description);
			CAP(buf);
			send_to_char(ch_selling, "%s", buf);
			obj_to_char(obj_selling, ch_selling);
			
			/* Reset auctioning values */
			obj_selling = NULL;
			ch_selling = NULL;
			ch_buying = NULL;
			curbid = 0;
			aucstat = AUC_NULL_STATE;
			return;
		}
		else
		{
			
			sprintf(buf, auctioneer[AUC_SOLD], curbid);
			auc_send_to_all(buf, TRUE);
			
			/* Give the object to the buyer */
			obj_to_char(obj_selling, ch_buying);
			sprintf(buf, "%s flies out the sky and into your hands, what a steel!\r\n", obj_selling->short_description);
			CAP(buf);
			send_to_char(ch_buying, "%s", buf);

			sprintf(buf, "Congrats! You have sold %s for %d coins!\r\n", obj_selling->short_description, curbid);
			send_to_char(ch_buying, "%s", buf);
	
			/* Give selling char the money for his stuff */
			GET_GOLD(ch_selling) += curbid;
			
			/* Reset auctioning values */
			obj_selling = NULL;
			ch_selling = NULL;
			ch_buying = NULL;
			curbid = 0;
			aucstat = AUC_NULL_STATE;
			return;						
		}

	}
	}
}

ACMD(do_auction)
{
	char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
	struct obj_data *obj;
	int bid = 0;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1) {
		send_to_char(ch, "Auction what?\r\n");
		return;
	}
	else if (is_abbrev(arg1, "cancel") || is_abbrev(arg1, "stop"))
	{
		if ((ch != ch_selling && GET_ADMLEVEL(ch) < ADMLVL_GRGOD) || aucstat == AUC_NULL_STATE)
		{
			send_to_char(ch, "You're not even selling anything!\r\n");
			return;
		}
		else if (ch == ch_selling)
		{
			stop_auction(AUC_NORMAL_CANCEL, NULL);
			return;
		}
		else
		{
			stop_auction(AUC_WIZ_CANCEL, ch);
		}
	}
	else if (is_abbrev(arg1, "stats") || is_abbrev(arg1, "identify"))
	{
		auc_stat(ch, obj_selling);
		return;
	}
	else if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
		send_to_char(ch, "%s", buf);
		return;
	}
	else if (!OBJ_FLAGGED(obj, ITEM_IDENTIFIED)) {
           send_to_char(ch, "You can only auction off an item that has been identified.\r\n");
           return;
        }
	else if (!*arg2 && (bid = GET_OBJ_COST(obj)) <= 0) {
		sprintf(buf, "What should be the minimum bid?\r\n");
		send_to_char(ch, "%s", buf);
		return;
	}
	else if (*arg2 && (bid = atoi(arg2)) <= 0)
	{
		send_to_char(ch, "Come on? One coin at least?\r\n");
		return;
	}
	else if (aucstat != AUC_NULL_STATE) {
		sprintf(buf, "Sorry, but %s is already auctioning %s at %d coins!\r\n", GET_NAME(ch_selling),
				obj_selling->short_description, bid);
		send_to_char(ch, "%s", buf);
		return;
	}
	else if (OBJ_FLAGGED(obj, ITEM_NOSELL))
	{
		send_to_char(ch, "Sorry but you can't sell that!\r\n");
		return;
	}
	else 
	{
		send_to_char(ch, "%s", CONFIG_OK);
		start_auction(ch, obj, bid);
		return;
	}
}

ACMD(do_bid)
{
	char arg[MAX_INPUT_LENGTH];
	int bid;

	if(IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Bid yes, good idea, but HOW MUCH??\r\n");
		return;
	}
	else if(aucstat == AUC_NULL_STATE)
	{
		send_to_char(ch, "Thats very enthusiastic of you, but nothing is being SOLD!\r\n");
		return;
	}
	else if(ch == ch_selling)
	{
		send_to_char(ch, "Why bid on something your selling?  You can 'cancel' the auction!\r\n");
		return;
	}
	else if((bid = atoi(arg)) < ((int) curbid * 1.1 - 1) && ch_buying != NULL)
	{
		sprintf(buf, "You must bid at least 10 percent more than the current bid. (%d)\r\n", (int) (curbid * 1.1));
		send_to_char(ch, "%s", buf);
		return;
	}
	else if(ch_buying == NULL && bid < curbid)
	{
		sprintf(buf, "You must at least bid the minimum!\r\n");
		send_to_char(ch, "%s", buf);
		return;
	}
	else if(bid > GET_GOLD(ch))
	{
		send_to_char(ch, "You don't have that much gold!\r\n");
		return;
	}
	else
	{
		if (ch == ch_buying)
			GET_GOLD(ch) -= (bid - curbid);
		else
		{
			GET_GOLD(ch) -= bid;
			
				if(!(ch_buying == NULL))
					GET_GOLD(ch_buying) += curbid;
		}

		curbid = bid;
		ch_buying = ch;
		
		
		sprintf(buf, auctioneer[AUC_BID], bid);
		auc_send_to_all(buf, TRUE);
		
		aucstat = AUC_OFFERING;
		return;
	}
}

void stop_auction(int type, struct char_data * ch)
{
	
	switch (type)
	{

	case AUC_NORMAL_CANCEL:
		{
		
		sprintf(buf, "%s", auctioneer[AUC_NORMAL_CANCEL]);
		auc_send_to_all(buf, FALSE);
		break;
		}
	case AUC_QUIT_CANCEL:
		{
		
		sprintf(buf, "%s", auctioneer[AUC_QUIT_CANCEL]);
		auc_send_to_all(buf, FALSE);
		break;
		}
	case AUC_WIZ_CANCEL:
		{
		
		sprintf(buf, "%s", auctioneer[AUC_WIZ_CANCEL]);
		auc_send_to_all(buf, FALSE);
		break;
		}
	default:
		{
			send_to_char(ch, "Sorry, that is an unrecognised cancel command, please report.");
			return;
		}
	}

	
	if (type != AUC_WIZ_CANCEL)
	{
		sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->short_description);
		CAP(buf);
		send_to_char(ch_selling, "%s", buf);
		obj_to_char(obj_selling, ch_selling);
	}
	else
	{
		sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->short_description);
		CAP(buf);
		send_to_char(ch, "%s", buf);
		obj_to_char(obj_selling, ch);
	}
	
	
	if (!(ch_buying == NULL))
		GET_GOLD(ch_buying) += curbid;
		
	obj_selling = NULL;
	ch_selling	= NULL;
	ch_buying = NULL;
	curbid = 0;

	aucstat = AUC_NULL_STATE;

}

void auc_stat(struct char_data *ch, struct obj_data *obj)
{
			
	if (aucstat == AUC_NULL_STATE)
	{
		send_to_char(ch, "Nothing is being auctioned!\r\n");
		return;
	}
	else if (ch == ch_selling)
	{
		send_to_char(ch, "You should have found that out BEFORE auctioning it!\r\n");
		return;
	}
	else if (GET_GOLD(ch) < 0)
	{
		send_to_char(ch, "You can't afford to find the stats on that, it costs 50 coins!\r\n");
		return;
	}
	else
	{
		/* auctioneer tells the character the auction details */
		sprintf(buf, auctioneer[AUC_STAT], curbid);
		act(buf, TRUE, ch_selling, obj, ch, TO_VICT | TO_SLEEP);
//		GET_GOLD(ch) -= 50;	

                call_magic(ch, NULL, obj_selling, SPELL_IDENTIFY, 60, CAST_SPELL, NULL);
	}
}

void auc_send_to_all(char *messg, bool buyer)
{
  struct descriptor_data *i;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next)
  {
	if (PRF_FLAGGED(i->character, PRF_NOAUCT))
		continue;
	
	
	if (buyer)
		act(messg, TRUE, ch_buying, obj_selling, i->character, TO_VICT | TO_SLEEP);
	else
		act(messg, TRUE, ch_selling, obj_selling, i->character, TO_VICT | TO_SLEEP);
	
  }
}

ACMD(do_binditem) 
{
  char arg[200];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What item from your inventory would you like to bind?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have an item by that description in your inventory.  Have you removed it yet?\r\n");
    return;
  }

  int cost = GET_OBJ_LEVEL(obj) * 100;

  if (GET_OBJ_VAL(obj, 13) == GET_IDNUM(ch)) {
    send_to_char(ch, "You have already bound this item to yourself.\r\n");
    return;
  } else if (GET_OBJ_VAL(obj, 13) != 0) {
    send_to_char(ch, "This item has been bound to someone else and will forever be unusable to others.\r\n");
    return;
  }

  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d %s to bind this item, but you only have %d on hand.\r\n", cost, MONEY_STRING, GET_GOLD(ch));
    return;
  }

  gain_gold(ch, -cost, GOLD_ONHAND);

  if (GET_OBJ_VAL(obj, 13) == 0) {

    int val = 0;
    int mult = 1;
    int i = 0;

    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0)) {

        mult = 1;
        val = obj->affected[i].location;

        if (val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_DEFLECTION || val == APPLY_AC_DODGE || val == APPLY_AC_ARMOR)
          mult = 10;
        if (val == APPLY_HIT)
          mult = 5;
        if (val == APPLY_KI)
          mult = 5;
        if (val == APPLY_MOVE)
          mult = 100;

        obj->affected[i].modifier += mult;
        
      }
    }
  }

  GET_OBJ_VAL(obj, 13) = GET_IDNUM(ch);

  send_to_char(ch, "%s has been successfully bound to you for %d %s.\r\n", (obj->short_description), cost, MONEY_STRING);
  }
