/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "mysql/mysql.h"

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "deities.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "quest.h"
#include "char_descs.h"
#include "polls.h"
#include "pets.h"
#include "oasis.h"
#include "modules.h"

/* extern variables */
extern struct poll_data poll_list[NUM_POLLS];
extern int class_hit_die_size_fr[NUM_CLASSES];
extern struct deity_info deity_list[NUM_DEITIES];
extern struct race_data race_list[NUM_RACES];
extern int num_religion_members[NUM_DEITIES];
extern struct spell_info_type spell_info[];
extern const char *class_abbrevs_core[];
extern const char *class_abbrevs_dl_aol[];
extern const char *deity_names_fr[];
extern const char *deity_names_dl_aol[];
extern const char *domain_names[];
extern int deity_domains[][4];
extern int deity_alignments[][9];
extern char *mag_summon_msgs[];
extern void set_height_and_weight_by_race(struct char_data *ch);
extern struct char_data *ch_selling;
extern struct char_data *ch_buying;
extern const char *eye_descriptions[];
extern char *nose_descriptions[];
extern char *ear_descriptions[];
extern char *face_descriptions[];
extern char *scar_descriptions[];
extern char *hair_descriptions[];
extern char *build_descriptions[];
extern char *complexion_descriptions[];

/* extern procedures */
int compute_companion_base_hit(struct char_data *ch);
int compute_companion_armor_class(struct char_data *ch, struct char_data *att);
int compute_summon_armor_class(struct char_data *ch, struct char_data *att);
int compute_mount_armor_class(struct char_data *ch, struct char_data *att);
int is_player_grouped(struct char_data *target, struct char_data *group);
void stop_guard(struct char_data *ch);
void stop_auction(int type, struct char_data * ch);
void remove_player(int pfilepos);
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data *ch, struct char_data * killer);
void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int level, int race);
char *which_desc(struct char_data *ch);
int has_intro(struct char_data *ch, struct char_data *target);
int strlencolor( char *arg);
int skill_roll(struct char_data *ch, int skillnum);
int get_skill_value(struct char_data *ch, int skillnum);
void advance_mob_level(struct char_data *ch, int whichclass);
void set_auto_mob_stats(struct char_data *mob);
int mob_exp_by_level(int level);
void award_rp_exp_char(struct char_data *i);
void set_attributes(struct char_data *ch, int str, int con, int dex, int intel, int wis, int cha);
void init_respec_char(struct char_data *ch);
void init_char_respec_two(struct char_data *ch);


/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_value);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
int spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
int spell_in_domain(struct char_data *ch, int spellnum);
ACMD(do_scribe);
ACMD(do_pagelength);
ACMD(do_form);
ACMD(do_reform);
int find_form_pos(struct char_data *ch);
ACMD(do_random);
ACMD(do_skillcheck);
ACMD(do_devote);
ACMD(do_domain);
ACMD(do_sponsor);
ACMD(do_skin);
void set_familiar_stats(struct char_data *ch);

char *deity_strings[100];

ACMD(do_skin)
{
    char arg1[MAX_INPUT_LENGTH];
    struct obj_data *cont, *obj;
    int temp, i, found=0;
    int percent;
    struct obj_data *jj, *next_thing2;
    one_argument(argument, arg1);

   
 if (GET_POS(ch) == POS_FISHING || GET_POS(ch) == POS_DIGGING || GET_POS(ch) ==  POS_RIDING) {
     send_to_char(ch, "You are not in a proper position for that!\r\n");
    return;
}


    if (!*arg1)
    { send_to_char(ch, "Skin what?\r\n");
      return;
    }
 /* need to check to make sure they are trying to skin a corpse */
    if(strcmp(arg1, "corpse"))
    { 
	send_to_char(ch, "Just skin corpses.\r\n");
	return;
    }
 /* need to make sure they are wielding an object before checking its type */
    if(!GET_EQ(ch, WEAR_WIELD))
    { send_to_char(ch, "Not with your bare hands.\r\n");
      return;
    } 
 /* of course if you have races, maybe clawed races could skin corpses
     without a weapon */
 /* You may want to change this to some type of cutting weapon. */
    if(GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) != ITEM_WEAPON)
    { send_to_char(ch, "Can't skin with that!\r\n");
      return; 
    }

    if((cont = get_obj_in_list_vis(ch, &arg1[0], NULL, world[ch->in_room].contents)))
      found = 1;
         
    if (found)
    {
   
      if (GET_OBJ_LEVEL(cont) == (ubyte)-1) {
	    send_to_char(ch, "That corpse has been mutilated beyond any possible use.\r\n");
		return;
	  }
   
      if ((percent = skill_roll(ch, SKILL_SURVIVAL)) < (10 + GET_OBJ_LEVEL(cont)))
      {
 /* Don't bother imms with skill checks */
      if(GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
      {
	    if (percent == 1 && (percent < (GET_OBJ_LEVEL(cont) - 10))) {
          send_to_char(ch, "You hack at the corpse until it becomes a descrated pile of flesh.\r\n");
          act("$n hacks at the corpse until it becomes a desecrated pile of flesh.", TRUE, ch, 0,
            0, TO_ROOM);	
		  GET_OBJ_LEVEL(cont) = -1;
		}
		else {  
          send_to_char(ch, "You crudely hack at the corpse awhile.\r\n");
          act("$n spends some time crudely hacking at the corpse.", TRUE, ch, 0,
            0, TO_ROOM);
		}
        return;
	  }
     }
   found = 0;
   for(i = 0; i < 4; i++)
   {
   if(cont->obj_flags->skin_data[i] == 0)
     continue;
/* Note: you can't use object vnum 0 with this. */
   if((temp = real_object(cont->obj_flags->skin_data[i])) > 0)
    {   
    found = 1;
    obj = read_object( temp, REAL);

    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("You skin $p from the corpse and put it down.", TRUE, ch, obj, 0, TO_CHAR);
    act("$n skins $p from the corpse and puts it down.", TRUE, ch, obj, 0, TO_ROOM);
    obj_to_room(obj, ch->in_room);
    }

    else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    act("You skin $p from the corpse and put it down.", TRUE, ch, obj, 0, TO_CHAR);
    act("$n skins $p from the corpse and puts it down.", TRUE, ch, obj, 0, TO_ROOM);

    obj_to_room(obj, ch->in_room);
  }
    else {
    obj_to_char(obj, ch);
    act("You skin $p from the corpse!", TRUE, ch, obj, 0, TO_CHAR);
    act("$n skins $p from the corpse!", TRUE, ch, obj, 0, TO_ROOM);
     }
    }
   }
   if(!found)
   {
   act("You spend some time crudely hacking at the corpse.", TRUE, ch, 0,
     0, TO_CHAR);
   act("$n spends some time crudely hacking at the corpse.", TRUE, ch, 0,
      0, TO_ROOM);
   }

   for (jj = cont->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content;       /* Next in inventory */
          obj_from_obj(jj);

          if (cont->in_obj) {
            obj_to_obj(jj, cont->in_obj);
          } else if (cont->carried_by) {
            obj_to_room(jj, cont->carried_by->in_room);
          } else if (cont->in_room != NOWHERE) {
            obj_to_room(jj, cont->in_room);
          } else {
            assert(FALSE);
          }
        }
   extract_obj(cont);
   return; 
 }
send_to_char(ch, "Just skin corpses ok?\r\n");
}

ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
/*
    if ((ch->time.played % 3600) / 60 < 1) {
      send_to_char(ch, "@YWe are sorry to see you leave so soon.  Please assist us in making this game better\r\n");
      send_to_char(ch, "by leaving us with a short message about what we can do to improve our game and your\r\n");
      send_to_char(ch, "early gaming experience on our mud.  Type your message, with the word \"quit\" on a\r\n");
      send_to_char(ch, "line by itself in order to finish or quit the game right away without leaving a message.\r\n");
      send_to_char(ch, "Thanks so much for trying our game.  We wish you the best in your future endeavors.@n\r\n");
      STATE(ch->desc) = CON_QUIT_GAME_EARLY;
    }
*/
    award_rp_exp_char(ch);
    stop_guard(ch);
    act("$n has left the game.", true, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s has quit the game.", GET_NAME(ch));
    if ((ch->time.played % 3600) / 60 >= 1) 
      send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    if (CONFIG_FREE_RENT || GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) > 1)
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
      Crash_rentsave(ch, GET_ROOM_VNUM(IN_ROOM(ch)));
    }
    else
      ch->player_specials->extract = TRUE;

    if (GET_ROOM_VNUM(IN_ROOM(ch)) > 1)
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

   if (GET_QUEST_TIME(ch) != (ush_int)-1)
     quest_timeout(ch);

   if (ch == ch_selling)
     stop_auction(AUC_QUIT_CANCEL, NULL);

    save_char(ch);

    extract_char(ch);		/* Char is saved before extracting. */
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * this prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. this code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (CONFIG_AUTO_SAVE && !GET_ADMLEVEL(ch)) {
      send_to_char(ch, "Saving.\r\n");
      write_aliases(ch);
      save_char(ch);      
      save_char_pets(ch);
      Crash_crashsave(ch, FALSE); 
      if (GET_ROOM_VNUM(IN_ROOM(ch)) > 1) {
        GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
//        send_to_char(ch, "LOADROOM: %d, REAL ROOM: %d\r\n", GET_LOADROOM(ch), real_room(GET_LOADROOM(ch)));
      }
      if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
        House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));             
      return;
    }
    send_to_char(ch, "Saving.\r\n");
    if (GET_ROOM_VNUM(IN_ROOM(ch)) > 1) {
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
//      send_to_char(ch, "LOADROOM: %d, REAL ROOM: %d\r\n", GET_LOADROOM(ch), real_room(GET_LOADROOM(ch)));
    }
    write_aliases(ch);
    save_char(ch);      
    save_char_pets(ch);
    Crash_crashsave(ch, FALSE); 
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
      House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));  
    return;    
  }

  if (GET_ROOM_VNUM(IN_ROOM(ch)) > 1) {
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
//    send_to_char(ch, "LOADROOM: %d, REAL ROOM: %d\r\n", GET_LOADROOM(ch), real_room(GET_LOADROOM(ch)));
  }


  write_aliases(ch);
  save_char(ch);
  save_char_pets(ch);
  Crash_crashsave(ch, FALSE);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}


ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int roll, detect, diffc = 20, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  /* STEALING is not an untrained skill */
  if (!GET_SKILL_BASE(ch, SKILL_SLEIGHT_OF_HAND)) {
    send_to_char(ch, "You'd be sure to be caught!\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "That isn't such a good idea...\r\n");
    return;
  }

  roll = roll_skill(ch, SKILL_SLEIGHT_OF_HAND);

  /* Can also add +2 synergy bonus for bluff of 5 or more */
  if (GET_SKILL(ch, SKILL_BLUFF) > 4)
    roll = roll +2;

  if (GET_POS(vict) < POS_SLEEPING)
    detect = 0;
  else
    detect = (roll_skill(vict, SKILL_PERCEPTION) > roll);

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (ADM_FLAGGED(vict, ADM_NOSTEAL) || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    roll = -10;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {
    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {
      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", false, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
             send_to_char(ch, "Impossible!\r\n");
             return;
           }
	  act("You unequip $p and steal it.", false, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", false, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */
      diffc += GET_OBJ_WEIGHT(obj);
      if (roll >= diffc) {	/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
          if (!give_otrigger(obj, vict, ch) || 
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
      if (detect > roll) {	/* Are you noticed? */
      ohoh = true;
	send_to_char(ch, "Oops..you were noticed.\r\n");
        if (roll >= diffc) {
	  act("$n has stolen $p from you!", false, ch, obj, vict, TO_VICT);
	  act("$n steals $p from $N.", true, ch, obj, vict, TO_NOTVICT);
    } else {
	  act("$n tried to steal something from you!", false, ch, 0, vict, TO_VICT);
	  act("$n tries to steal something from $N.", true, ch, 0, vict, TO_NOTVICT);
        }
      }
    }
  } else {			/* Steal some coins */
    diffc += 5;	/* People take care of their money */
    if (roll >= diffc) {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
    if (detect > roll) {
      ohoh = true;
      send_to_char(ch, "Oops..\r\n");
      if (roll >= diffc) {
        act("$n has stolen your gold!", false, ch, 0, vict, TO_VICT);
        act("$n steals some gold from $N.", true, ch, 0, vict, TO_NOTVICT);
      } else {
        act("You discover that $n has $s hands in your wallet.", false, ch, 0, vict, TO_VICT);
        act("$n tries to steal gold from $N.", true, ch, 0, vict, TO_NOTVICT);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  /* if (IS_NPC(ch))
    return; */

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  int appeared = 0;

  if (GET_ADMLEVEL(ch)) {
    perform_immort_vis(ch);
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    appeared = 1;
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }

  if (AFF_FLAGGED(ch, AFF_ETHEREAL) && affectedv_by_spell(ch, ART_EMPTY_BODY)) {
    affectv_from_char(ch, ART_EMPTY_BODY);
    if (AFF_FLAGGED(ch, AFF_ETHEREAL)) {
      send_to_char(ch, "Returning to the material plane will not be so easy.\r\n");
    } else {
      send_to_char(ch, "You return to the material plane.\r\n");
      if (!appeared)
        act("$n flashes into existence.", false, ch, 0, 0, TO_ROOM);
    }
    appeared = 1;
  }

  if (!appeared)
    send_to_char(ch, "You are already visible.\r\n");
}


ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself, read HELP RULES TITLES for more info.\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlencolor(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else if (!strstr(argument, GET_NAME(ch)))
    send_to_char(ch, "Titles must contain your character's name.\r\n");
  else if (!*argument) {
    set_title(ch, GET_NAME(ch));
    send_to_char(ch, "Okay, you're now %s.\r\n", GET_TITLE(ch)); 
  }
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s.\r\n", GET_TITLE(ch));
  }
}

int valid_group(struct char_data *ch, struct char_data *vict)
{

  struct follow_type *f;
  struct char_data *k;
  int maxlevel = 0;
  int minlevel = 0;

  k = ch->master ? ch->master : ch;

  if (is_player_grouped(ch, k)) {
    maxlevel = GET_LEVEL(k);
    minlevel = GET_LEVEL(k);
  }

  for (f = k->followers; f; f = f->next)
    if (!IS_NPC(f->follower) && is_player_grouped(ch, f->follower)) {
      maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
      maxlevel = MIN(minlevel, GET_LEVEL(f->follower));
    }

  maxlevel = MAX(maxlevel, GET_LEVEL(vict));
  minlevel = MIN(minlevel, GET_LEVEL(vict));

  if ((maxlevel - minlevel) > 10)
    return FALSE;

  return TRUE;
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

/*
  if (!valid_group(ch, vict)) {
    send_to_char(ch, "%s is not within the required level limit of the group.\r\n", GET_NAME(vict));
    send_to_char(vict, "You are not within the required level limit of the group.\r\n");
    return (0);
  }
*/
  SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);

  if (ch != vict) {
    act("$N is now a member of your group.", false, ch, 0, vict, TO_CHAR);
    REMOVE_BIT_AR(PRF_FLAGS(vict), PRF_LFG);
    GET_LFG_STRING(vict) = NULL;
  }
  act("You are now a member of $n's group.", false, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", false, ch, 0, vict, TO_NOTVICT);
  return (1);
}


void print_group(struct char_data *ch)
{
//  send_to_char(ch, "This display is currently disabled for debugging purposes.\r\n");
//  return;

  struct char_data *k;
  struct follow_type *f;
  int found = FALSE;
  char colora[5]; char color2[5];

  sprintf(colora, "@C");
  sprintf(color2, "@C");

  float xp = 0.0;
  int percent = 0;
  int int_xp = 0;
  int int_percent = 0;


  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");
  
    k = (ch->master ? ch->master : ch);

    if (k != NULL && AFF_FLAGGED(k, AFF_GROUP)) {

      if (k == ch)
        found = TRUE;

      if (GET_HIT(k) >= GET_MAX_HIT(k))
        sprintf(colora, "@C");
      else if (GET_HIT(k) >= (GET_MAX_HIT(k) * 76 / 100))
        sprintf(colora, "@G");
      else if (GET_HIT(k) >= (GET_MAX_HIT(k) * 51 / 100))
        sprintf(colora, "@Y");
      else if (GET_HIT(k) >= (GET_MAX_HIT(k) * 26 / 100))
        sprintf(colora, "@M");
      else if (GET_HIT(k) >= (GET_MAX_HIT(k) * 1 / 100))
        sprintf(colora, "@R");
      else
        sprintf(colora, "@r");

      sprintf(buf, "@W    %-15s [ %s%3d@W/@C%3d@W Hp ",
               GET_NAME(k), colora, GET_HIT(k), GET_MAX_HIT(k));
      send_to_char(ch, "%s", buf);

      if (GET_MOVE(k) >= GET_MAX_MOVE(k))
        sprintf(color2, "@C");
      else if (GET_MOVE(k) >= (GET_MAX_MOVE(k) * 76 / 100))
        sprintf(color2, "@G");
      else if (GET_MOVE(k) >= (GET_MAX_MOVE(k) * 51 / 100))
        sprintf(color2, "@Y");
      else if (GET_MOVE(k) >= (GET_MAX_MOVE(k) * 26 / 100))
        sprintf(color2, "@M");
      else if (GET_MOVE(k) >= (GET_MAX_MOVE(k) * 1 / 100))
        sprintf(color2, "@R");
      else
        sprintf(color2, "@r");

      // Determine exp percent to next level

       if (GET_LEVEL(k) == 1 && race_list[GET_RACE(k)].level_adjustment) {
         xp = ((float) GET_EXP(k)) /
                ((float) level_exp((GET_CLASS_LEVEL(k) + 1), GET_REAL_RACE(k)));
       }
       else {
         xp = (((float) GET_EXP(k)) - ((float) level_exp(GET_CLASS_LEVEL(k), GET_REAL_RACE(k)))) /
                (((float) level_exp((GET_CLASS_LEVEL(k) + 1), GET_REAL_RACE(k)) -
                (float) level_exp(GET_CLASS_LEVEL(k), GET_REAL_RACE(k))));
      }

      xp *= (float) 1000.0;
      percent = (int) xp % 10;
      xp /= (float) 10;
      int_xp = MAX(0, (int) xp);
      int_percent = MAX(0, MIN((int) percent, 99));


      sprintf(buf, "| %s%4d@W/@C%4d@W Mv ] %d.%d%% TNL %-7s %s @M(Group Leader)@n\r\n",
               color2, GET_MOVE(k), GET_MAX_MOVE(k),
               int_xp, int_percent,
               race_list[GET_RACE(k)].abbrev, class_desc_str(k, 1, 0));
      send_to_char(ch, "%s", buf);

    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP) || (f->follower == ch && found))
	continue;

    if (f->follower == ch)
      found = TRUE;

    if (AFF_FLAGGED(f->follower, AFF_GROUP)) {

      if (GET_HIT(f->follower) >= GET_MAX_HIT(f->follower))
        sprintf(colora, "@C");
      else if (GET_HIT(f->follower) >= GET_MAX_HIT(f->follower) * 76 / 100)
        sprintf(colora, "@G");
      else if (GET_HIT(f->follower) >= GET_MAX_HIT(f->follower) * 51 / 100)
        sprintf(colora, "@Y");
      else if (GET_HIT(f->follower) >= GET_MAX_HIT(f->follower) * 26 / 100)
        sprintf(colora, "@M");
      else if (GET_HIT(f->follower) >= GET_MAX_HIT(f->follower) * 1 / 100)
        sprintf(colora, "@R");
      else
        sprintf(colora, "@r");


      sprintf(buf, "@W    %-15s [ %s%3d@W/@C%3d@W Hp ",
               GET_NAME(f->follower), colora, GET_HIT(f->follower), 
               GET_MAX_HIT(f->follower));
      send_to_char(ch, "%s", buf);

      if (GET_MOVE(f->follower) >= GET_MAX_MOVE(f->follower))
        sprintf(color2, "@C");
      else if (GET_MOVE(f->follower) >= GET_MAX_MOVE(f->follower) * 76 / 100)
        sprintf(color2, "@G");
      else if (GET_MOVE(f->follower) >= GET_MAX_MOVE(f->follower) * 51 / 100)
        sprintf(color2, "@Y");
      else if (GET_MOVE(f->follower) >= GET_MAX_MOVE(f->follower) * 26 / 100)
        sprintf(color2, "@M");
      else if (GET_MOVE(f->follower) >= GET_MAX_MOVE(f->follower) * 1 / 100)
        sprintf(color2, "@R");
      else
        sprintf(color2, "@r");


      // Determine exp percent to next level

       if (GET_LEVEL(f->follower) == 1 && race_list[GET_RACE(f->follower)].level_adjustment) {
         xp = ((float) GET_EXP(f->follower)) /
                ((float) level_exp((GET_CLASS_LEVEL(f->follower) + 1), GET_REAL_RACE(f->follower)));
       }
       else {
         xp = (((float) GET_EXP(f->follower)) - ((float) level_exp(GET_CLASS_LEVEL(f->follower), GET_REAL_RACE(f->follower)))) /
                (((float) level_exp((GET_CLASS_LEVEL(f->follower) + 1), GET_REAL_RACE(f->follower)) -
                (float) level_exp(GET_CLASS_LEVEL(f->follower), GET_REAL_RACE(f->follower))));
      }

      xp *= (float) 1000.0;
      percent = (int) xp % 10;
      xp /= (float) 10;
      int_xp = MAX(0, (int) xp);
      int_percent = MAX(0, MIN((int) percent, 99));


      sprintf(buf, "| %s%4d@W/@C%4d@W Mv ] %d.%d%% TNL %-7s %s@n\r\n",
               color2, GET_MOVE(f->follower), GET_MAX_MOVE(f->follower),
               int_xp, int_percent,
               race_list[GET_RACE(f->follower)].abbrev, class_desc_str(f->follower, 1, 0));
      send_to_char(ch, "%s", buf);
    }
  }
  }
}



ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	false, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", false, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", false, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", false, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", false, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", true, f->follower, NULL, ch, TO_CHAR);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", false, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", false, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", false, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %d/%d Hit Points & %d/%d Stamina\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, true, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, true, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}



ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    /* Abusing signed/unsigned to make sizeof work. */
		char *tmpdesc = NULL;
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		has_intro(k, ch)? GET_NAME(ch) : (tmpdesc = which_desc(ch)), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", has_intro(k, ch) ? GET_NAME(ch) : tmpdesc);
    }
		free(tmpdesc);
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	GET_GOLD(f->follower) += share;
	send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}



ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  mag_item = GET_EQ(ch, WEAR_WIELD2);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
    case SCMD_USE:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but in the
       *  function which handles 'quaff', 'recite', and 'use'.
       */
      return;
    }
  }

  if (FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);

  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_value)
{
  char arg[MAX_INPUT_LENGTH];
  int value_lev;
  int max = 0;

  one_argument(argument, arg);

  if (!*arg) {
    switch (subcmd) {
    case SCMD_WIMPY:
        if (GET_WIMP_LEV(ch)) {
          send_to_char(ch, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
          return;
        } else {
          send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
          return;
        }
      break;
    case SCMD_POWERATT:
        if (GET_POWERATTACK(ch)) {
          send_to_char(ch, "Your current power attack level -%d accuracy +%ddamage.\r\n",
                       GET_POWERATTACK(ch), GET_POWERATTACK(ch));
          return;
        } else {
          send_to_char(ch, "You are not currently using power attack.\r\n");
          return;
        }
      break;
    }
  }

  if (isdigit(*arg)) {
    switch (subcmd) {
    case SCMD_WIMPY:
      /* 'wimp_level' is a player_special. -gg 2/25/98 */
      if (IS_NPC(ch))
        return;
      if ((value_lev = atoi(arg)) != 0) {
        if (value_lev < 0)
	  send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (value_lev > GET_MAX_HIT(ch))
	  send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
        else if (value_lev > (GET_MAX_HIT(ch) / 2))
	  send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
        else {
	  send_to_char(ch, "Okay, you'll wimp out if you drop below %d hit points.\r\n", value_lev);
	  GET_WIMP_LEV(ch) = value_lev;
        }
      } else {
        send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
        GET_WIMP_LEV(ch) = 0;
      }
      break;
    case SCMD_POWERATT:
      if (HAS_FEAT(ch, FEAT_IMPROVED_POWER_ATTACK))
        max = 10;
      else
        max = 5;
      if ((value_lev = atoi(arg)) != 0) {
        if (value_lev < 0)
	  send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (value_lev > MIN(max, GET_ACCURACY_BASE(ch)))
	  send_to_char(ch, "You may only specify a value up to %d.\r\n", MIN(max, GET_ACCURACY_BASE(ch)));
        else {
	  send_to_char(ch, "Okay, you'll sacrifice %d points of accuracy for damage.\r\n", value_lev);
	  GET_POWERATTACK(ch) = value_lev;
        }
      } else {
        send_to_char(ch, "Okay, you will no longer use power attack.\r\n");
        GET_POWERATTACK(ch) = 0;
      }
      break;
    default:
      log("Unknown subcmd to do_value %d called by %s", subcmd, GET_NAME(ch));
      break;
    }
  } else
    send_to_char(ch, "Specify a value.  (0 to disable)\r\n");
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { H | K | V | X} | all | none }\r\n");
    return;
  }
  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    if (GET_CLASS_RANKS(ch, CLASS_MONK))
      SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPGOLD);
//    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
  }
  else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPGOLD);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
  }
  else {

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
      if (PRF_FLAGGED(ch, PRF_DISPHP))
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
      else
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'k':
      if (PRF_FLAGGED(ch, PRF_DISPKI))
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
      else
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
	break;
      case 'v':
      if (PRF_FLAGGED(ch, PRF_DISPMOVE))
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
      else
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      case 'x':
      if (PRF_FLAGGED(ch, PRF_DISPEXP))
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
      else
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
	break;
      default:
	send_to_char(ch, "Usage: prompt { H | K | V | X } | all | none }\r\n");
	return;
      }
    }
  }
  send_to_char(ch, "Done.\r\n");
}

ACMD(do_gen_write)
{
  FILE *fl;
  const char *filename;
  struct stat fbuf;
  time_t rawtime;
  struct tm *info;
  char buffer[80];

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  case SCMD_NEWS:
    filename = NEWS_FILE;
    break;
  default:
    send_to_char(ch, "That option does not exist.\r\n");
    return;
  }

  time(&rawtime);
  info = localtime( &rawtime);
  strftime(buffer, 80, "%b-%d-%Y", info);

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }

  if (stat(filename, &fbuf) < 0) {
    send_to_char(ch, "Cannot find file, please contact an immortal.\r\n");
    log("SYSERR: Can't stat() file: %s", strerror(errno));
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    log("SYSERR: do_gen_write: %s", strerror(errno));
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  if (subcmd == SCMD_NEWS)
    fprintf(fl, "@W%-8s@n @Y(%11s)@n %s\n", GET_NAME(ch), (buffer), argument);
  else
    fprintf(fl, "%-8s (%11s) [%5d] %s\n", GET_NAME(ch), (buffer),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  if (subcmd == SCMD_NEWS) {
    struct char_data *tch;
    for (tch = character_list; tch; tch = tch->next) {
      if (!IS_NPC(tch) && tch->desc)
        send_to_char(tch, "@C%s has added news: @W%s@n\r\n", GET_NAME(ch), argument);
    }
  }
  log("%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}



#define TOG_OFF 0
#define TOG_ON  1

ACMD(do_gen_tog)
{
  long result = 0;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"}, 
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
     "You will now Auto-Assist.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autosac disabled.\r\n",
    "Autosac enabled.\r\n"},
    {"You will no longer attempt to be sneaky.\r\n",
    "You will try to move as silently as you can.\r\n"},
    {"You will no longer attempt to stay hidden.\r\n",
    "You will try to stay hidden.\r\n"},
    {"You will no longer automatically memorize spells in your list.\r\n",
    "You will automatically memorize spells in your list.\r\n"},
    {"Viewing newest board messages first.\r\n",
      "Viewing eldest board messages first.  Wierdo.\r\n"},
    {"Compression will be used if your client supports it.\r\n",
     "Compression will not be used even if your client supports it.\r\n"},
    {"You are now deaf to the clan channel.\r\n",
    "You can now hear the clan channel.\r\n"},
    {"You will now hear all clan channels.\r\n",
    "You will no longer see all clan channels.\r\n"},
    {"Autoattack disabled.\r\n",
    "Autoattack enabled.\r\n"},
    {"Bleeding attack disabled.\r\n",
    "Bleeding attack enabled.\r\n"},
    {"Powerful sneak disabled.\r\n",
    "Powerful sneak enabled.\r\n"},
    {"Knockdown disabled.\r\n",
    "Knockdown enabled.\r\n"},
    {"Robilars gambit disabled.\r\n",
    "Robilars gambit enabled.\r\n"},
    {"You will no longer take 10 on eligible dice rolls.\r\n",
    "You will now take ten on eligible dice rolls.\r\n"},
    {"You will no longer have your summon tank for you.\r\n",
    "You will now have your summon tank for you.\r\n"},
    {"You will no longer have your mount tank for you.\r\n",
    "You will now have your mount tank for you.\r\n"},
    {"You will no longer use your divine bond with your weapon.\r\n",
    "You will now use your divine bond with your weapon.\r\n"},
    {"You will no longer tank for your party.\r\n",
    "You will now tank for your party.\r\n"},
    {"You will no longer have your animal companion tank for you.\r\n",
    "You will now have your animal companion tank for you.\r\n"},
    {"Brief mode with map disabled.\r\n",
    "Brief mode with map enabled.\r\n"},
    {"Parry mode disabled.\r\n",
    "Parry mode enabled.\r\n"},
    {"Player vs player mode disabled.\r\n",
    "Player vs player mode enabled.\r\n"},
    {"Text output during fights has been reset to normal.\r\n",
    "Text output during fights has been minimized.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_PVP:
    if (PRF_FLAGGED(ch, PRF_PVP) && ch->pvp_timer > 0) {
      send_to_char(ch, "You cannot turn off your pvp timer for another %d minutes.\r\n", ch->pvp_timer);
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_PVP);
    break;
  case SCMD_PARRY:
    result = PRF_TOG_CHK(ch, PRF_PARRY);
    break;
  case SCMD_BRIEFMAP:
    result = PRF_TOG_CHK(ch, PRF_BRIEFMAP);
    break;
  case SCMD_FIGHT_SPAM:
//    result = PRF_TOG_CHK(ch, PRF_FIGHT_SPAM);
    break;
  case SCMD_SUMMON_TANK:
    result = PRF_TOG_CHK(ch, PRF_SUMMON_TANK);
    break;
  case SCMD_COMPANION_TANK:
    result = PRF_TOG_CHK(ch, PRF_COMPANION_TANK);
    break;
  case SCMD_MOUNT_TANK:
    result = PRF_TOG_CHK(ch, PRF_MOUNT_TANK);
    break;
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_KNOCKDOWN:
    result = PRF_TOG_CHK(ch, PRF_KNOCKDOWN);
    break;
  case SCMD_ROBILARS_GAMBIT:
    result = PRF_TOG_CHK(ch, PRF_ROBILARS_GAMBIT);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_TANK:
    result = AFF_TOG_CHK(ch, AFF_TANKING);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
    result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", true, ch, 0, 0, TO_ROOM);
    else
      act("$n has come back from AFK.", true, ch, 0, 0, TO_ROOM);
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");  	
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK))
      mudlog(CMP, GET_LEVEL(ch), true, 
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    else
      mudlog(CMP, GET_LEVEL(ch), true,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC: 
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC); 
    break; 
  case SCMD_SNEAK:
    if (affected_by_spell(ch, SPELL_FAERIE_FIRE)) {
      send_to_char(ch, "You cannot sneak while covered in faerie fire.\r\n");
      return;
    }
    result = AFF_TOG_CHK(ch, AFF_SNEAK);
    break;
  case SCMD_HIDE:
    if (affected_by_spell(ch, SPELL_FAERIE_FIRE)) {
      send_to_char(ch, "You cannot hide while covered in faerie fire.\r\n");
      return;
    }
    if (FIGHTING(ch) && HAS_FEAT(ch, FEAT_HIDE_IN_PLAIN_SIGHT) && 
      skill_roll(FIGHTING(ch), SKILL_PERCEPTION) < skill_roll(ch, SKILL_STEALTH)) 
    {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
    result = AFF_TOG_CHK(ch, AFF_HIDE);
    break;
  case SCMD_AUTOMEM: 
    result = PRF_TOG_CHK(ch, PRF_AUTOMEM); 
    break; 
  case SCMD_VIEWORDER:
    result = PRF_TOG_CHK(ch, PRF_VIEWORDER);
    break;
  case SCMD_NOCOMPRESS:
    if (CONFIG_ENABLE_COMPRESSION) {
      result = PRF_TOG_CHK(ch, PRF_NOCOMPRESS);
      break;
    } else {
      send_to_char(ch, "Sorry, compression is globally disabled.\r\n");
    }
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;

  case SCMD_AUTOATTACK:
    result = PRF_TOG_CHK(ch, PRF_AUTOATTACK);
    break;

  case SCMD_BLEEDING_ATTACK:
    result = PRF_TOG_CHK(ch, PRF_BLEEDING_ATTACK);
    break;

  case SCMD_TAKE_TEN:
    result = PRF_TOG_CHK(ch, PRF_TAKE_TEN);
    break;

  case SCMD_DIVINE_BOND:
    result = PRF_TOG_CHK(ch, PRF_DIVINE_BOND);
    break;

  case SCMD_POWERFUL_SNEAK:
    result = PRF_TOG_CHK(ch, PRF_POWERFUL_SNEAK);
    break;

  case SCMD_CLANTALK:
    if (!GET_CLAN(ch) && (GET_ADMLEVEL(ch) < ADMLVL_GRGOD)) {
      send_to_char(ch, "You don't belong to any clan.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_CLANTALK);
    break;
  case SCMD_ALLCTELL:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL) {
      send_to_char(ch, "You are not high enough to listen to all clan channels.\r\n");      
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_ALLCTELL);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    /*  SYSERR_DESC:
     *  This is the same as the unhandled case in do_gen_ps(), but in the
     *  function which handles 'compact', 'brief', and so forth.
     */
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
  num_lines = 0,
  req_lines = 0,
  i,
  j;
  int l;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
 } fields[] = {
     { "none",           ADMLVL_GOD,    "Does Nothing" },
     { "bug",            ADMLVL_GOD,    "../lib/misc/bugs"},
     { "typo",           ADMLVL_GOD,   "../lib/misc/typos"},
     { "news",           ADMLVL_GOD,   "../lib/misc/news"},
     { "ideas",          ADMLVL_GOD,    "../lib/misc/ideas"},
     { "xnames",         ADMLVL_GOD,     "../lib/misc/xnames"},
     { "levels",         ADMLVL_GOD,    "../log/levels" },
     { "rip",            ADMLVL_GOD,    "../log/rip" },
     { "players",        ADMLVL_GOD,    "../log/newplayers" },
     { "rentgone",       ADMLVL_GOD,    "../log/rentgone" },
     { "errors",         ADMLVL_GOD,    "../log/errors" },
     { "godcmds",        ADMLVL_GOD,    "../log/godcmds" },
     { "syslog",         ADMLVL_GOD,    "../syslog" },
     { "crash",          ADMLVL_GOD,    "../syslog.CRASH" },
     { "\n", 0, "\n" }
};

   skip_spaces(&argument);

   if (!*argument && subcmd != SCMD_NEWS) {
     strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
     for (j = 0, i = 1; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         sprintf(buf+strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     send_to_char(ch, "%s", buf);
     return;
   }

  if (subcmd != SCMD_NEWS) {

     two_arguments(argument, field, value);

     for (l = 0; *(fields[l].cmd) != '\n'; l++)
       if (!strncmp(field, fields[l].cmd, strlen(field)))
     break;

     if(*(fields[l].cmd) == '\n') {
       send_to_char(ch, "That is not a valid option!\r\n");
       return;
     }

     if (GET_ADMLEVEL(ch) < fields[l].level) {
       send_to_char(ch, "You are not godly enough to view that file!\r\n");
       return;
     }

     if(!*value)
       req_lines = 30; /* default is the last 30 lines */
     else
       req_lines = atoi(value);
   
  }
  else {
    one_argument(argument, value);

     l = 3;

     send_to_char(ch, "\r\n@W%s News@n\r\n@Y-----------@n\r\n", MUD_NAME);

     if(!*value)
       req_lines = 30; /* default is the last 30 lines */
     else
       req_lines = atoi(value);
  }

   if (!(req_file=fopen(fields[l].file,"r")) && subcmd != SCMD_NEWS) {
     mudlog(BRF, ADMLVL_IMPL, true,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }
   else if (!(req_file=fopen("../lib/misc/news", "r"))) {
     mudlog(BRF, ADMLVL_IMPL, true,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }

   get_line(req_file,line);
   while (!feof(req_file)) {
     num_lines++;
     get_line(req_file,line);
   }
   rewind(req_file);

   req_lines = MIN(MIN(req_lines, num_lines),150);
   
   buf[0] = '\0';

   get_line(req_file,line);
   while (!feof(req_file)) {
     cur_line++;
     if(cur_line > (num_lines - req_lines)) 
       sprintf(buf+strlen(buf),"%s\r\n", line);
	   
     get_line(req_file,line);
   }
   fclose(req_file);

   page_string(ch->desc, buf, 1);
   if (subcmd == SCMD_NEWS)
     send_to_char(ch, "\r\n@WTo view more than 30 lines type news <# of lines to view>.@n\r\n\r\n");

}

ACMD(do_compare)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1, *obj2;
  struct char_data *tchar;
  int value1 = 0, value2 = 0, o1, o2;
  char *msg = NULL;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Compare what to what?\n\r");
    return;
  }

  o1 = generic_find(arg1, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  o2 = generic_find(arg2, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj2);

  if (!o1 || !o2) {
    send_to_char(ch, "You do not have that item.\r\n");
    return;
  }
  if ( obj1 == obj2 ) {
    msg = "You compare $p to itself.  It looks about the same.";
  } else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2)) {
    msg = "You can't compare $p and $P.";
  } else {
    switch ( GET_OBJ_TYPE(obj1) ) {
      default:
      msg = "You can't compare $p and $P.";
      break;
      case ITEM_ARMOR:
      value1 = GET_OBJ_VAL(obj1, VAL_ARMOR_APPLYAC);
      value2 = GET_OBJ_VAL(obj2, VAL_ARMOR_APPLYAC);
      break;
      case ITEM_WEAPON:
      value1 = (1 + GET_OBJ_VAL(obj1, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj1, VAL_WEAPON_DAMDICE);
      value2 = (1 + GET_OBJ_VAL(obj2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj2, VAL_WEAPON_DAMDICE);
      break;
    }
  }

  if ( msg == NULL ) {
    if ( value1 == value2 )
      msg = "$p and $P look about the same.";
    else if ( value1  > value2 )
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act( msg, false, ch, obj1, obj2, TO_CHAR );
  return;
}

ACMD(do_break)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int brk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you break SOMETHING.\r\n");
    return;
  }

  if (!(brk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to break!\r\n");
    return;
  }


  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, "Seems like it's already broken!\r\n");
    return;
  }

  /* Ok, break it! */
  send_to_char(ch, "You ruin %s.\r\n", obj->short_description);
  act("$n ruins $p.", false, ch, obj, 0, TO_ROOM);
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = -1;
  TOGGLE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);

  return;
}

ACMD(do_fix)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int brk;
  int roll = 0;
  int dc = 0;
  int valnum = 0;
  
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you fix SOMETHING.\r\n");
    return;
  }
  
  if (!(brk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to fix!\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
    valnum = VAL_WEAPON_HEALTH;
    GET_OBJ_VAL(obj, VAL_WEAPON_MAXHEALTH) = 100;
    if (GET_OBJ_VAL(obj, VAL_WEAPON_HEALTH) >= 100) {
      send_to_char(ch, "This item is not in need of repair.\r\n");
	return;
    }
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT) {
    valnum = VAL_ARMOR_HEALTH;
    GET_OBJ_VAL(obj, VAL_ARMOR_MAXHEALTH) = 100;
    if (GET_OBJ_VAL(obj, VAL_ARMOR_HEALTH) >= 100) {
      send_to_char(ch, "This item is not in need of repair.\r\n");
	return;
    }
  }
  else {
    valnum = VAL_ALL_HEALTH;
    GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) = 100;
    if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 100) {
      send_to_char(ch, "This item is not in need of repair.\r\n");
	return;
    }
  }
  
  dc = 0;
  
  if ((brk) && OBJ_FLAGGED(obj, ITEM_BROKEN)) {
  dc += 5;
  }
  
  dc += MAX(0, GET_OBJ_LEVEL(obj) - 10);

  int cost = GET_OBJ_LEVEL(obj) * MAX(1, GET_OBJ_LEVEL(obj) / 5) * 2;

  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "Repairing that has a material cost of %d %s.\r\n", cost, MONEY_STRING);
    return;
  }

  int skill_type = SKILL_BLACKSMITHING;

  int material = GET_OBJ_MATERIAL(obj);

  if (IS_CLOTH(material))
    skill_type = SKILL_TAILORING;
  else if (IS_LEATHER(material))
    skill_type = SKILL_TANNING;
  else if (IS_PRECIOUS_METAL(material))
    skill_type = SKILL_GOLDSMITHING;
  else if (IS_WOOD(material))
    skill_type = SKILL_WOODWORKING;
  
  
  if (get_skill_value(ch, skill_type) + 20 < dc) {
  send_to_char(ch, "This item is beyond your ability to repair.\r\n");
  return;
  }
  
//  GET_GOLD(ch) -= cost;
  gain_gold(ch, -cost, GOLD_ONHAND);

  send_to_char(ch, "You use your %s skill to try and repair %s.\r\n", spell_info[skill_type].name, obj->short_description);

  if ((roll = skill_roll(ch, skill_type)) >= dc) {
  
  send_to_char(ch, "You repair %s at a material cost of %d.\r\n", obj->short_description, cost);
  act("$n repairs $p.", false, ch, obj, 0, TO_ROOM);
  GET_OBJ_VAL(obj, valnum) = MAX(0, GET_OBJ_VAL(obj, valnum));

  // if they beat the dc by 10 ormore the object gets extradamage and hitpoints
  if (roll >= (dc))
    GET_OBJ_VAL(obj, valnum) += (roll - dc) * 5;

  if (IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN))
    REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
  }
  else {
    GET_OBJ_VAL(obj, valnum) -= (dc - roll) / 2;
	if (GET_OBJ_VAL(obj, valnum) > 0) {
	  send_to_char(ch, "Your attempt to repair %s damages it even further, costing you %d %s in materials.", obj->short_description, cost, MONEY_STRING);
	  act("$n's attempt to repair $p damages it further.", false, ch, obj, 0, TO_ROOM);
	}
	else {
	  send_to_char(ch, "Your attempt to repair %s breaks it completely, also costing you %d %s in materials.", obj->short_description, cost, MONEY_STRING );
	  act("$n's attempt to repair $p breaks it completely.", false, ch, obj, 0, TO_ROOM);	
	}
  }

  return;
}

/* new spell memorization code */
/* remove a spell from a character's innate linked list */
void innate_remove(struct char_data * ch, struct innate_node * inn)
{
  struct innate_node *temp;

  if (ch->innate == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(inn, ch->innate, next);
  free(inn);
}

void innate_add(struct char_data * ch, int innate, int timer)
{
  struct innate_node * inn;

  CREATE(inn, struct innate_node, 1);
  inn->timer = timer;
  inn->spellnum = innate;
  inn->next = ch->innate;
  ch->innate = inn;
}

/* Returns true if the spell/skillnum is innate to the character
   (as opposed to just practiced).
 */
int is_innate(struct char_data *ch, int spellnum) 
{
  if (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS) 
  {
    switch(spellnum) 
    {
    case SPELL_FAERIE_FIRE:
      if(GET_RACE(ch) == RACE_DROW_ELF)
        return true;
      break;
    }
  }
  return false;
}

/* returns FALSE if the spell is found in the innate linked list. 
  this just means the ability is ready if they can even get the
  ability. (see is_innate). 
*/
int is_innate_ready(struct char_data *ch, int spellnum)
{
  struct innate_node *inn, *next_inn;

  for(inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if(inn->spellnum == spellnum)
      return false;
  }
  return true;
}

/* Adds a node to the linked list with a timer which indicates
   that the innate skill/spell is NOT ready to be used.

   is_innate_ready(...) should be called first to determine if
   there is already a timer set for the spellnum.
*/
void add_innate_timer(struct char_data *ch, int spellnum) 
{
  int timer = 6; /* number of ticks */

  switch(spellnum) 
  {
    case ABILITY_CALL_MOUNT:
    case ABILITY_CALL_COMPANION:
      timer = 50;
      break;
    case SPELL_ACMD_PILFER:
      timer = 100;
      break;
    case SPELL_SKILL_HEAL_USED:
      timer = 50;
      break;
    case SPELL_FAERIE_FIRE:
      timer = 100;   
      break;
    case ABIL_LAY_HANDS:
      timer = 100;
      break;
    case ABIL_SMITE_EVIL:
      timer = 100;
      break;
    case SPELL_AFF_RAGE:
    	timer = 100;
      break;
    case SPELL_AFF_DEFENSIVE_STANCE:
    	timer = 100;
      break;
    case SPELL_AFF_STRENGTH_OF_HONOR:
      timer = 100;
      break;
    case SPELL_AFF_TURN_UNDEAD:
      timer = 100;
      break;
    case SPELL_BARD_SONGS:
      timer = 100;
      break;
    case SPELL_BARD_SPELLS:
      timer = 100;
      break;
    case SPELL_SORCERER_SPELLS:
      timer = 100;
      break;
    case SPELL_ASSASSIN_SPELLS:
      timer = 100;
      break;
    default:
      timer = 100;
      break;
  }

  if (is_innate_ready(ch, spellnum)) 
  {
    innate_add(ch, spellnum, timer);
  } else if (spellnum != ABIL_SMITE_EVIL && spellnum != SPELL_AFF_RAGE)
  {
    send_to_char(ch, "BUG!\r\n");
  }
}


/* called when a player enters the game to set permanent affects.
   Usually these will stay set, but certain circumstances will
   cause them to wear off (ie. removing eq with a perm affect).
*/
void add_innate_affects(struct char_data *ch) 
{
#if defined(CAMPAIGN_FORGOTTEN_REALMS)
  switch(GET_RACE(ch)) 
  {  
  case RACE_MOON_ELF:
  case RACE_SHIELD_DWARF:
  case RACE_ROCK_GNOME:
    affect_modify(ch, APPLY_NONE, 0, 0, AFF_INFRAVISION, true);
    break;
  }
#else
  switch(GET_RACE(ch)) 
  {  
  case RACE_DWARF:
  case RACE_ELF
  case RACE_GNOME:
    affect_modify(ch, APPLY_NONE, 0, 0, AFF_INFRAVISION, true);
    break;
  }
#endif
  affect_total(ch);
}

ACMD(do_timers) 
{

  struct innate_node *inn, *next_inn;

  send_to_char(ch, "Timers left:\r\n%-30s %s\r\n\r\n", "Affect Name", "Rounds Until Refreshed");

  for (inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    send_to_char(ch, "%-30s %d rounds until refreshed\r\n", spell_info[inn->spellnum].name, inn->timer);
  }

}

/* Called to update the innate timers */
void update_innate(struct char_data *ch)
{
  struct innate_node *inn, *next_inn;
  int i = 0;

  for (inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if (inn->timer > 0) { 
      inn->timer--;
    } else {
      switch(inn->spellnum) {
       case SPELL_TOUCH_OF_UNDEATH:
           send_to_char(ch, "Your touch of undeath uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER);
           break;
       case SPELL_ANIMATE_DEAD:
           send_to_char(ch, "Your animate dead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_ANIMATE_DEAD) * 3;
           break;
       case SPELL_SUMMON_UNDEAD:
           send_to_char(ch, "Your summon undead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_SUMMON_UNDEAD) * 3;
           break;
       case SPELL_SUMMON_GREATER_UNDEAD:
           send_to_char(ch, "Your summon greater undead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_SUMMON_GREATER_UNDEAD) * 3;
           break;
       case SPELL_BREATH_WEAPON:
        send_to_char(ch, "Your breath weapon usage has been restored to full.\r\n");
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_BREATH_WEAPON);
        break;
       case SPELL_DRAGON_MOUNT_BREATH:
        send_to_char(ch, "Your dragon mount breath weapon usage has been restored to full.\r\n");
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH);
        break;
      case SPELL_INSPIRE_COURAGE:
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
        send_to_char(ch, "Your inspire courage/greatness uses have been restored to full.\r\n");
        break;
      case SPELL_INSPIRE_GREATNESS:
        GET_INNATE(ch, SPELL_INSPIRE_COURAGE) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
        send_to_char(ch, "Your inspire greatness/courage uses have been restored to full.\r\n");
        break;
      case SPELL_INVISIBLE:
        if (GET_RACE(ch) == RACE_DUERGAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your invisibility uses have been restored to full.\r\n");
        break;
      case SPELL_ENLARGE_PERSON:
        if (GET_RACE(ch) == RACE_DUERGAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your enlarge person uses have been restored to full.\r\n");
        break;
      case SPELL_BLINDNESS:
        if (GET_RACE(ch) == RACE_DROW_ELF || GET_RACE(ch) == RACE_SVIRFNEBLIN || GET_RACE(ch) == RACE_TIEFLING)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your blindness uses have been refreshed to full.\r\n");
        break;
      case SPELL_FAERIE_FIRE:
        if (GET_RACE(ch) == RACE_DROW_ELF)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your faerie fire uses have been refreshed to full.\r\n");
        break;
      case SPELL_BLUR:
        if (GET_RACE(ch) == RACE_SVIRFNEBLIN)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your blur uses have been refreshed to full.\r\n");
        break;
      case SPELL_DAYLIGHT:
        if (GET_RACE(ch) == RACE_AASIMAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your ball of light uses have been refreshed to full.\r\n");
        break;
      case SPELL_FLY:
        if (GET_RACE(ch) == RACE_AIR_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your fly uses have been refreshed to full.\r\n");
        break;
      case SPELL_FIREBALL:
        if (GET_RACE(ch) == RACE_FIRE_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your fireball uses have been refreshed to full.\r\n");
        break;
      case SPELL_HASTE:
        if (HAS_FEAT(ch, FEAT_HASTE))
          GET_INNATE(ch, inn->spellnum) = 3;
        send_to_char(ch, "Your haste uses have been refreshed to full.\r\n");
        break;
      case SPELL_ICE_STORM:
        if (GET_RACE(ch) == RACE_WATER_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your ice storm uses have been refreshed to full.\r\n");
        break;
      case SPELL_STONESKIN:
        if (GET_RACE(ch) == RACE_EARTH_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your stoneskin uses have been refreshed to full.\r\n");
        break;
      case ABIL_LAY_HANDS:
        GET_LAY_HANDS(ch) = (GET_CLASS_RANKS(ch, CLASS_PALADIN) / 2) + ability_mod_value(GET_CHA(ch));
        send_to_char(ch, "Your lay on hands uses have been restore to full.\r\n");
        break;
      case ABIL_SMITE_EVIL:
        send_to_char(ch, "Your smite opportunities have been restored to full.\r\n");
        if (IS_GOOD(ch))
          GET_SMITE_EVIL(ch) = HAS_FEAT(ch, FEAT_SMITE_EVIL);
        break;
       case SPELL_AFF_RAGE:
        send_to_char(ch, "Your rage opportunities have been restored to full.\r\n");
        GET_RAGE(ch) = HAS_FEAT(ch, FEAT_RAGE);
        break;
       case SPELL_AFF_DEFENSIVE_STANCE:
        send_to_char(ch, "Your defensive stance opportunities have been restored to full.\r\n");
        GET_DEFENSIVE_STANCE(ch) = HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE);
        break;
       case SPELL_BARD_SONGS:
        send_to_char(ch, "Your bard songs usage has been restored to full.\r\n");
        GET_BARD_SONGS(ch) = GET_CLASS_RANKS(ch, CLASS_BARD) + (HAS_FEAT(ch, FEAT_EXTRA_MUSIC) ? 4 : 0);
        break;
       case SPELL_EPIC_SPELLS:
        send_to_char(ch, "Your epic spells usage has been restored to full.\r\n");
        GET_EPIC_SPELLS(ch) = get_skill_value(ch, SKILL_KNOWLEDGE) / 10;
        break;
       case SPELL_BARD_SPELLS:
        send_to_char(ch, "Your bard spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
        for (i = 0; i <= 6; i++)
          if (findslotnum(ch, i))
            GET_BARD_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_SORCERER_SPELLS:
        send_to_char(ch, "Your sorcerer spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
        for (i = 0; i <= 9; i++)
          if (findslotnum(ch, i))
            GET_SORCERER_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_FAVORED_SOUL_SPELLS:
        send_to_char(ch, "Your favored soul spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
        for (i = 0; i <= 9; i++)
          if (findslotnum(ch, i))
            GET_FAVORED_SOUL_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_ASSASSIN_SPELLS:
        send_to_char(ch, "Your assassin spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
        for (i = 0; i <= 4; i++)
          if (findslotnum(ch, i))
            GET_ASSASSIN_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_AFF_STRENGTH_OF_HONOR:
        send_to_char(ch, "Your strength of honor opportunities have been restored to full.\r\n");
        GET_STRENGTH_OF_HONOR(ch) = HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR);
        break; 
       case SPELL_AFF_TURN_UNDEAD:
         send_to_char(ch, "Your turn undead opportunities have been restored to full.\r\n");
	 GET_TURN_UNDEAD(ch) = 3 + ability_mod_value(GET_CHA(ch)) + (HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2);
	 break;
      default:
        send_to_char(ch, "You are now able to use your innate %s again.\r\n", spell_info[inn->spellnum].name);
        break;
      }  
      innate_remove(ch, inn);
    }
  } 
}

ACMD(do_abilities)
{

  send_to_char(ch, "Below are your abilities:\r\n\r\n");

  if (HAS_FEAT(ch, FEAT_TOUCH_OF_UNDEATH)) {
    send_to_char(ch, "@n");
  }


  struct innate_node *inn = NULL;
  int i = 0;

    switch (dice(1, 1000)) {

       case SPELL_TOUCH_OF_UNDEATH:
           send_to_char(ch, "Your touch of undeath uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = GET_CLASS_RANKS(ch, CLASS_DEATH_MASTER);
           break;
       case SPELL_ANIMATE_DEAD:
           send_to_char(ch, "Your animate dead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_ANIMATE_DEAD) * 3;
           break;
       case SPELL_SUMMON_UNDEAD:
           send_to_char(ch, "Your summon undead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_SUMMON_UNDEAD) * 3;
           break;
       case SPELL_SUMMON_GREATER_UNDEAD:
           send_to_char(ch, "Your summon greater undead uses have been restored to full.\r\n");
           GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_SUMMON_GREATER_UNDEAD) * 3;
           break;
       case SPELL_BREATH_WEAPON:
        send_to_char(ch, "Your breath weapon usage has been restored to full.\r\n");
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_BREATH_WEAPON);
        break;
       case SPELL_DRAGON_MOUNT_BREATH:
        send_to_char(ch, "Your dragon mount breath weapon usage has been restored to full.\r\n");
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH);
        break;
      case SPELL_INSPIRE_COURAGE:
        GET_INNATE(ch, inn->spellnum) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
        send_to_char(ch, "Your inspire courage/greatness uses have been restored to full.\r\n");
        break;
      case SPELL_INSPIRE_GREATNESS:
        GET_INNATE(ch, SPELL_INSPIRE_COURAGE) = HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
        send_to_char(ch, "Your inspire greatness/courage uses have been restored to full.\r\n");
        break;
      case SPELL_INVISIBLE:
        if (GET_RACE(ch) == RACE_DUERGAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your invisibility uses have been restored to full.\r\n");
        break;
      case SPELL_ENLARGE_PERSON:
        if (GET_RACE(ch) == RACE_DUERGAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your enlarge person uses have been restored to full.\r\n");
        break;
      case SPELL_BLINDNESS:
        if (GET_RACE(ch) == RACE_DROW_ELF || GET_RACE(ch) == RACE_SVIRFNEBLIN || GET_RACE(ch) == RACE_TIEFLING)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your blindness uses have been refreshed to full.\r\n");
        break;
      case SPELL_FAERIE_FIRE:
        if (GET_RACE(ch) == RACE_DROW_ELF)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your faerie fire uses have been refreshed to full.\r\n");
        break;
      case SPELL_BLUR:
        if (GET_RACE(ch) == RACE_SVIRFNEBLIN)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your blur uses have been refreshed to full.\r\n");
        break;
      case SPELL_DAYLIGHT:
        if (GET_RACE(ch) == RACE_AASIMAR)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your ball of light uses have been refreshed to full.\r\n");
        break;
      case SPELL_FLY:
        if (GET_RACE(ch) == RACE_AIR_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your fly uses have been refreshed to full.\r\n");
        break;
      case SPELL_FIREBALL:
        if (GET_RACE(ch) == RACE_FIRE_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your fireball uses have been refreshed to full.\r\n");
        break;
      case SPELL_HASTE:
        if (HAS_FEAT(ch, FEAT_HASTE))
          GET_INNATE(ch, inn->spellnum) = 3;
        send_to_char(ch, "Your haste uses have been refreshed to full.\r\n");
        break;
      case SPELL_ICE_STORM:
        if (GET_RACE(ch) == RACE_WATER_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your ice storm uses have been refreshed to full.\r\n");
        break;
      case SPELL_STONESKIN:
        if (GET_RACE(ch) == RACE_EARTH_GENESI)
          GET_INNATE(ch, inn->spellnum) = 1;
        send_to_char(ch, "Your stoneskin uses have been refreshed to full.\r\n");
        break;
      case ABIL_LAY_HANDS:
        GET_LAY_HANDS(ch) = (GET_CLASS_RANKS(ch, CLASS_PALADIN) / 2) + ability_mod_value(GET_CHA(ch));
        send_to_char(ch, "Your lay on hands uses have been restore to full.\r\n");
        break;
      case ABIL_SMITE_EVIL:
        send_to_char(ch, "Your smite opportunities have been restored to full.\r\n");
        if (IS_GOOD(ch))
          GET_SMITE_EVIL(ch) = HAS_FEAT(ch, FEAT_SMITE_EVIL);
        break;
       case SPELL_AFF_RAGE:
        send_to_char(ch, "Your rage opportunities have been restored to full.\r\n");
        GET_RAGE(ch) = HAS_FEAT(ch, FEAT_RAGE);
        break;
       case SPELL_AFF_DEFENSIVE_STANCE:
        send_to_char(ch, "Your defensive stance opportunities have been restored to full.\r\n");
        GET_DEFENSIVE_STANCE(ch) = HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE);
        break;
       case SPELL_BARD_SONGS:
        send_to_char(ch, "Your bard songs usage has been restored to full.\r\n");
        GET_BARD_SONGS(ch) = GET_CLASS_RANKS(ch, CLASS_BARD) + (HAS_FEAT(ch, FEAT_EXTRA_MUSIC) ? 4 : 0);
        break;
       case SPELL_EPIC_SPELLS:
        send_to_char(ch, "Your epic spells usage has been restored to full.\r\n");
        GET_EPIC_SPELLS(ch) = get_skill_value(ch, SKILL_KNOWLEDGE) / 10;
        break;
       case SPELL_BARD_SPELLS:
        send_to_char(ch, "Your bard spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
        for (i = 0; i <= 6; i++)
          if (findslotnum(ch, i))
            GET_BARD_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_SORCERER_SPELLS:
        send_to_char(ch, "Your sorcerer spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
        for (i = 0; i <= 9; i++)
          if (findslotnum(ch, i))
            GET_SORCERER_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_FAVORED_SOUL_SPELLS:
        send_to_char(ch, "Your favored soul spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
        for (i = 0; i <= 9; i++)
          if (findslotnum(ch, i))
            GET_FAVORED_SOUL_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_ASSASSIN_SPELLS:
        send_to_char(ch, "Your assassin spells usage has been restored to full.\r\n");
        GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
        for (i = 0; i <= 4; i++)
          if (findslotnum(ch, i))
            GET_ASSASSIN_SPELLS(ch, i) = findslotnum(ch, i);
        break;
       case SPELL_AFF_STRENGTH_OF_HONOR:
        send_to_char(ch, "Your strength of honor opportunities have been restored to full.\r\n");
        GET_STRENGTH_OF_HONOR(ch) = HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR);
        break; 
       case SPELL_AFF_TURN_UNDEAD:
         send_to_char(ch, "Your turn undead opportunities have been restored to full.\r\n");
	 GET_TURN_UNDEAD(ch) = 3 + ability_mod_value(GET_CHA(ch)) + (HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2);
	 break;

  }
}

int spell_in_book(struct obj_data *obj, int spellnum)
{
  int i;
  bool found = false;

  if (!obj->sbinfo)
    return false;

  for (i=0; i < SPELLBOOK_SIZE; i++)
    if (obj->sbinfo[i].spellname == spellnum) {
      found = true;
      break;
    }

  if (found)
    return 1;

  return 0;
}

int spell_in_scroll(struct obj_data *obj, int spellnum)
{
  if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) == spellnum)
    return true;

  return false;
}

int spell_in_domain(struct char_data *ch, int spellnum)
{
  int i = 0;
		
  if (DOMAIN_FLAGGED(spellnum, DOMAIN_UNDEFINED)) {
    return false;
  }
  

  for (i = 0; i < 6; i++) {
    if (deity_list[GET_DEITY(ch)].domains[i] == DOMAIN_UNDEFINED)
      continue;
    if (DOMAIN_FLAGGED(spellnum, deity_list[GET_DEITY(ch)].domains[i]))
      return TRUE;
  }

  for (i = 0; i < NUM_CLASSES; i++) {
    if (GET_CLASS_RANKS(ch, i) > 0) {
      if (spell_info[spellnum].class_level[CLASS_WIZARD] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_MAGE && i == CLASS_WIZARD)
        return true;
      if (spell_info[spellnum].class_level[CLASS_CLERIC] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC && i == CLASS_CLERIC)
        return true;
      if (spell_info[spellnum].class_level[CLASS_PALADIN] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN && i == CLASS_PALADIN)
        return true;
      if (spell_info[spellnum].class_level[CLASS_DRUID] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_DRUID && i == CLASS_DRUID)
        return true;
      if (spell_info[spellnum].class_level[CLASS_RANGER] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_RANGER && i == CLASS_RANGER)
        return true;
      if (spell_info[spellnum].class_level[CLASS_BARD] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_BARD && i == CLASS_BARD)
        return true;
      if (spell_info[spellnum].class_level[CLASS_SORCERER] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_SORCERER && i == CLASS_SORCERER)
        return true;
      if (spell_info[spellnum].class_level[CLASS_CLERIC] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_FAVORED_SOUL && i == CLASS_FAVORED_SOUL)
        return true;
      if (spell_info[spellnum].class_level[CLASS_ASSASSIN] != 99 && GET_MEM_TYPE(ch) == MEM_TYPE_ASSASSIN && i == CLASS_ASSASSIN)
        return true;
    }
  }

  return false;
}


//room_vnum freeres[] = {
// LAWFUL_GOOD 	(room_vnum) CONFIG_MORTAL_START,
// LAWFUL_NEUTRAL 	(room_vnum) CONFIG_MORTAL_START,
// LAWFUL_EVIL 		(room_vnum) CONFIG_MORTAL_START,
// NEUTRAL_GOOD 	(room_vnum) CONFIG_MORTAL_START,
// NEUTRAL_NEUTRAL 	(room_vnum) CONFIG_MORTAL_START,
// NEUTRAL_EVIL 	(room_vnum) CONFIG_MORTAL_START,
// CHAOTIC_GOOD 	(room_vnum) CONFIG_MORTAL_START,
// CHAOTIC_NEUTRAL 	(room_vnum) CONFIG_MORTAL_START,
// CHAOTIC_EVIL 	(room_vnum) CONFIG_MORTAL_START
//};


ACMD(do_resurrect)
{
  room_rnum rm;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!AFF_FLAGGED(ch, AFF_SPIRIT)) {
    send_to_char(ch, "But you're not even dead!\r\n");
    return;
  }


  if (ch->pvp_death == 0) {
    send_to_char(ch, "@RYou take an experience and gold penalty and pray for charity resurrection.@n\r\n");
    gain_exp(ch, -((level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch)) - level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch))) / 20));
    gain_gold(ch, -(GET_GOLD(ch)/2), GOLD_ONHAND);

    if (GET_DEITY(ch) == DEITY_NONE) {
      send_to_char(ch, "@RBecause you do not have a deity to sponsor your resurrection, you must pay double the penalty.@n\r\n");
      gain_exp(ch, -((level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch)) - level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch))) / 20));
      gain_gold(ch, -(GET_GOLD(ch)/2), GOLD_ONHAND);
    }
  } 
  else
    ch->pvp_death = 0;

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);

  if (GET_HIT(ch) < 1)
    GET_HIT(ch) = 1;

//  if ((rm = real_room(freeres[ALIGN_TYPE(ch)])) == NOWHERE)
  rm = real_room(CONFIG_MORTAL_START);

  if (rm != NOWHERE) {
    char_from_room(ch);
    char_to_room(ch, rm);
    look_at_room(IN_ROOM(ch), ch, 0);
  }

  act("$n's body forms in a pool of @Bblue light@n.", true, ch, 0, 0, TO_ROOM);
}

ACMD(do_pagelength)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You current page length is set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else if (is_number(arg)) {
    GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg), 5), 255);
    send_to_char(ch, "Okay, your page length is now set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else {
    send_to_char(ch, "Please specify a number of lines (5 - 255).\r\n");
  }
}

ACMD(do_form) /* Lars - 02/00 */
{
  struct char_data *k;
  struct follow_type *f;
  int i, counter = 0, found = false;
  char buf[MAX_STRING_LENGTH];
   
  /* Are we grouped? */
  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not a member of any group!\r\n");
  else {   /* yes we are */
    send_to_char(ch, "Positions:\r\n");
   
    k = (ch->master ? ch->master : ch);
              
    /*
     * error checking
     *
     * this has been commented out, but left in the event you have problems
     * getting this to work.  These checks will print out all formation positions,
     * and their values (taken or free).  It will also print the total number
     * of members in the group.
     */

/*    for (i = 0; i < MAX_FORM_POSITIONS; i++) {
      j = 0;
      if (GET_FORM_TOTAL(k, i) > 0)
        j++;
      sprintf(buf, "Pos: %d, Value: %d\r\n", i, j);
      send_to_char(buf, k);
    } */
    
    /* print the leaders position */
/*    if (AFF_FLAGGED(k, AFF_GROUP)) { 
      sprintf(buf, "Pos %d: $N", GET_FORM_POS(k));
      act(buf, false, ch, 0, k, TO_CHAR);
    } */      
    
    /* now print followers positions */
/*    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
        continue;
   
      sprintf(buf, "Pos %d: $N", GET_FORM_POS(f->follower));
      act(buf, false, ch, 0, f->follower, TO_CHAR);
    } */
    
    send_to_char(ch, "Formation:\r\n");
   
    /* format the output */
    for (i = 0; i < MAX_FORM_POSITIONS; i++) {  /* loop through total form positions */
      if (GET_FORM_POS(k) == i) {   /* lets check for the leader first */
        sprintf(buf, "[%-15s] ", GET_NAME(k));  /* found, spit out the output */
        send_to_char(ch, "%s", buf);
        if (counter >= FORM_POS_FRONT) {   /* checks if this is the 1/3rd entry on a line */
          send_to_char(ch, "\r\n");
          counter = 0;
        } else
          counter++;  /* it's not, increment */
      } else {
        for (f = k->followers; f; f = f->next) {  /* onto the followers */
          if (GET_FORM_POS(f->follower) == i) {  /* are you the one? */
            sprintf(buf, "[%-15s] ", GET_NAME(f->follower) );  /* yes, send your name */
            send_to_char(ch, "%s", buf);
            found = true;
          }
        }
        if (!found)
          send_to_char(ch, "[               ] "); /* empty brackets to format menu logically if no one is found */
        if (counter >= FORM_POS_FRONT) {  /* more checking for rows */
          send_to_char(ch, "\r\n");
          counter = 0;
        } else
          counter++; /* increment */
      }
      found = false;
    }
  }
}
           
ACMD(do_reform)
{
  struct char_data *vict;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int j, i;
   
  two_arguments(argument, buf, buf2);
        
  if (!*buf) {
    send_to_char(ch, "Whom do you wish to reform?\r\n");
    return;
  }  
   
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person doesn't seem to be here.\r\n");
    return;
  }     
  if ((vict->master != ch) && (vict != ch)) {
    send_to_char(ch, "That person is not in your group.\r\n");
    return;
  }  
  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "That person is not in your group.\r\n");
    return;
  }
  if (ch->master && ch != vict) {
    send_to_char(ch, "You have to lead the group to reform other members.\r\n");
    return;
  }
  
  if (!*buf2) {
    send_to_char(ch, "Reform to which position?\r\n");
    return;
  }
    
  j = atoi(buf2);

  j--;  

  if (j > MAX_FORM_POSITIONS || j < 0) {
    send_to_char(ch, "That is not a valid position.\r\n");
    return;
  }
    
  if (GET_FORM_TOTAL(ch, j) > 0) {              /* need to replace with current position holder */
    send_to_char(ch, "That position is already taken.\r\n");
    return;
  } else {
    for (i = 0; i < MAX_FORM_POSITIONS; i++) {  /* find the old position */
      if (GET_FORM_POS(vict) == i)              /* is this it? */
        GET_FORM_TOTAL(ch, i) = 0;              /* set the old position in the array to 0 */
    }
    GET_FORM_POS(vict) = j;                     /* give them the new position */
    GET_FORM_TOTAL(ch, j) = 1;                  /* update the array */
    sprintf(buf, "$N has been reformed to position %d.\r\n", j + 1);
    act(buf, false, ch, 0, vict, TO_CHAR);
    send_to_char(vict, "You have been reformed.\r\n");
    return;
  }
}

int find_form_pos(struct char_data *ch)
{
  int i;
  
  for (i = 0; i < MAX_FORM_POSITIONS; i++) {
    if (GET_FORM_TOTAL(ch, i) > 0)  /* occupied? */
      continue;
    else {
      GET_FORM_TOTAL(ch, i) = 1;  /* position is free, take it! */
      return (i);
    }   
  }
  return (0);   /* if all else fails, should never get here */
}

ACMD(do_intro)
{
  int i = 0;
  char arg[MAX_STRING_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (IS_NPC(ch)) {
    send_to_char(ch, "Only player character can introduce themselves.\r\n");
    return;
  }

  if (!ch->desc) {
    send_to_char(ch, "Only player character can introduce themselves.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "To whom do you wish to introduce yourself to?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Doesn't seem to be anyone around with that description.\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(ch, "Hi.  Have you met yourself yet?  Might want to get that noggin of yours checked out. :)\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "You can't introduce yourself to an NPC.\r\n");
	    return;
  }

  if (!vict->desc) {
    send_to_char(ch, "You can't introduce yourself to someone who's not connected.\r\n");
    return;
  }

  for (i = 0; i < MAX_INTROS; i++) {
    if (vict->player_specials->intro_list[i][0] == GET_IDNUM(ch)) {
      send_to_char(ch, "They already know you!\r\n");
      return;
    }
  }

  // For each record in the intro list, starting at the last, copy the value of the preceeding
  // record into the actual slot.  This basically moves all the intros forward one slot
  // leaving the first one available for the new intro

  for ( i = MAX_INTROS - 1; i > 0; i--)
    vict->player_specials->intro_list[i][0] = vict->player_specials->intro_list[i-1][0];

  vict->player_specials->intro_list[0][0] = GET_IDNUM(ch); 

  if (VALID_INTRO(vict)) {
   act("You introduce yourself to $N.", true, ch, 0, vict, TO_CHAR);
   act("You learn $n's name.", true, ch, 0, vict, TO_VICT);
   act("$n introduces $mself to $N.", true, ch, 0, vict, TO_NOTVICT);
   GET_INTROS_GIVEN(ch)++;
   GET_INTROS_RECEIVED(vict)++;
  }
  else
    act("@RYou cannot receive any more intros until you have given out more of your own.@n", false, ch, 0, vict, TO_VICT);


  return;
}

ACMD(do_greet)
{
  int i = 0;
  int temp = 0;
  int counter = 0;
  char arg[MAX_STRING_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mobs can't greet anyone.\r\n");
    return;
  }

  if (!ch->desc) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Who would you like to greet?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person doesn't seem to be here.\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "You can't greet mobs.\r\n");
    return;
  }

  if (!vict->desc) {
    send_to_char(ch, "That person is not available right now.  Try again later.\r\n");
    return;
  }

  // Search through all the victims's intros until we find the character's.  Store the
  // character's pfile id # and the position in the intro list
  for (i = 0; i < MAX_INTROS; i++) {
    if (GET_IDNUM(ch) == vict->player_specials->intro_list[i][0]) {
      temp = vict->player_specials->intro_list[i][0];
      counter = i;
    }
  }
  
  // Move all intros forward one slot, up until the position stored previously, to
  // effectively erase the old intro.
  for (i = counter; i > 0; i--) {
    vict->player_specials->intro_list[i][0] = vict->player_specials->intro_list[i-1][0];
  }

  // Place the character's intro at the front of the list.  Tada!
  vict->player_specials->intro_list[0][0] = temp;

  act("You give $m a hearty greeting.", false, vict, 0, ch, TO_VICT);
  act("$n gives you a hearty greeting.", true, ch, 0, vict, TO_VICT);
  act("$n gives $N a hearty greeting.", true , ch, 0, vict, TO_NOTVICT);

  return;

}

ACMD(do_setactive)
{

  int i;
  char arg[MAX_STRING_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
   
    send_to_char(ch, "You may switch your active class to one of the following classes:\r\n");
    send_to_char(ch, "(syntax is setact # (based on which number appears before the class you wish))\r\n");

    for (i = 0; i <= NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i))
        send_to_char(ch, "%d) %s\r\n", i, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
    }
    return;
  }

  for (i = 0; i < NUM_CLASSES; i++) {
 
    if ( atoi(arg) == i && GET_CLASS_RANKS(ch, i)) {
      GET_CLASS(ch) = i;
      return;
    }
  }

  send_to_char(ch, "That isn't a valid class.  You may switch your active class to one of the following:\r\n");
  send_to_char(ch, "(syntax is setact # (based on which number appears before the class you wish))\r\n");
  for (i = 0; i < NUM_CLASSES; i++) {
    if (GET_CLASS_RANKS(ch, i))
      send_to_char(ch, "%d) %s\r\n", i, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
  }
  return;
}

ACMD(do_flurry)
{
	if (!AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS)) {
		send_to_char(ch, "You will now attack using the flurry of blows method.\r\n");
		SET_BIT_AR(AFF_FLAGS(ch), AFF_FLURRY_OF_BLOWS);
	}
	else {
		send_to_char(ch, "You will no longer attack using the flurry of blows method.\r\n");
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLURRY_OF_BLOWS);
	}
}
ACMD(do_rapidshot)
{
	if (!AFF_FLAGGED(ch, AFF_RAPID_SHOT)) {
		send_to_char(ch, "You will now attack using the rapid shot method.\r\n");
		SET_BIT_AR(AFF_FLAGS(ch), AFF_RAPID_SHOT);
	}
	else {
		send_to_char(ch, "You will no longer attack using the rapid shot method.\r\n");
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_RAPID_SHOT);
	}
}
ACMD(do_autofeint)
{
	if (!PRF_FLAGGED(ch, PRF_AUTOFEINT)) {
		send_to_char(ch, "You will now automatically try to feint each round of combat..\r\n");
		SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOFEINT);
	}
	else {
		send_to_char(ch, "You will no longer automatically try to feint each round of combat.\r\n");
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOFEINT);
	}
}
ACMD(do_craftingbrief)
{
	if (!PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF)) {
		SET_BIT_AR(PRF_FLAGS(ch), PRF_CRAFTING_BRIEF);
		send_to_char(ch, "You will no longer see crafting process updates.\r\n");
	}
	else {
		send_to_char(ch, "You will now see crafting process updates.\r\n");
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_CRAFTING_BRIEF);
	}
}
ACMD(do_contained_areas)
{
	if (!PRF_FLAGGED(ch, PRF_CONTAINED_AREAS)) {
		SET_BIT_AR(PRF_FLAGS(ch), PRF_CONTAINED_AREAS);
		send_to_char(ch, "You will now limit your area spells to only those you are fighting.\r\n");
	}
	else {
		send_to_char(ch, "You will no longer limit your area spells to only those you are fighting.\r\n");
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_CONTAINED_AREAS);
	}
}
ACMD(do_nohints)
{
	if (!PRF_FLAGGED(ch, PRF_NOHINTS)) {
		SET_BIT_AR(PRF_FLAGS(ch), PRF_NOHINTS);
		send_to_char(ch, "You will no longer see game hints.\r\n");
	}
	else {
		send_to_char(ch, "You will now see game hints every so often.\r\n");
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHINTS);
	}
}
ACMD(do_metamagic)
{

  char arg[100];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please specify the metamagic feat you wish to enable.\r\nOptions: maximize, extend\r\n");
    return;
  }

  if (is_abbrev(arg, "maximize")) {
    if (!HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL)) {
      send_to_char(ch, "You do not have the maximize spell feat.\r\n");
      return;
    }
    if (!PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL)) {
      send_to_char(ch, "You will now cast spells using the maximize spell metamagic ability.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_MAXIMIZE_SPELL);
    }
    else {
      send_to_char(ch, "You will no longer cast spells using the maximize spell metamagic ability.\r\n");
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_MAXIMIZE_SPELL);
    }
  }
  else if (is_abbrev(arg, "extend")) {
    if (!HAS_FEAT(ch, FEAT_EXTEND_SPELL)) {
      send_to_char(ch, "You do not have the extend spell feat.\r\n");
      return;
    }
    if (!PRF_FLAGGED(ch, PRF_EXTEND_SPELL)) {
      send_to_char(ch, "You will now cast spells using the extend spell metamagic ability.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_EXTEND_SPELL);
    }
    else {
      send_to_char(ch, "You will no longer cast spells using the extend spell metamagic ability.\r\n");
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EXTEND_SPELL);
    }
  }  else if (is_abbrev(arg, "quicken")) {
    if (!HAS_FEAT(ch, FEAT_QUICKEN_SPELL)) {
      send_to_char(ch, "You do not have the quicken spell feat.\r\n");
      return;
    }
    if (!PRF_FLAGGED(ch, PRF_QUICKEN_SPELL)) {
      send_to_char(ch, "You will now cast spells using the quicken spell metamagic ability.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_QUICKEN_SPELL);
    }
    else {
      send_to_char(ch, "You will no longer cast spells using the quicken spell metamagic ability.\r\n");
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUICKEN_SPELL);
    }
  }
  else if (is_abbrev(arg, "empower")) {
    if (!HAS_FEAT(ch, FEAT_EMPOWER_SPELL)) {
      send_to_char(ch, "You do not have the empower spell feat.\r\n");
      return;
    }
    if (!PRF_FLAGGED(ch, PRF_EMPOWER_SPELL)) {
      send_to_char(ch, "You will now cast spells using the empower spell metamagic ability.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_EMPOWER_SPELL);
    }
    else {
      send_to_char(ch, "You will no longer cast spells using the empower spell metamagic ability.\r\n");
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EMPOWER_SPELL);
    }
  }
  else if (is_abbrev(arg, "intensify")) {
    if (!HAS_FEAT(ch, FEAT_INTENSIFY_SPELL)) {
      send_to_char(ch, "You do not have the intensify spell feat.\r\n");
      return;
    }
    if (!PRF_FLAGGED(ch, PRF_INTENSIFY_SPELL)) {
      send_to_char(ch, "You will now cast spells using the intensify spell metamagic ability.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_INTENSIFY_SPELL);
      if (PRF_FLAGGED(ch, PRF_MAXIMIZE_SPELL))
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_MAXIMIZE_SPELL);
      if (PRF_FLAGGED(ch, PRF_EMPOWER_SPELL))
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EMPOWER_SPELL);
    }
    else {
      send_to_char(ch, "You will no longer cast spells using the empower spell metamagic ability.\r\n");
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_EMPOWER_SPELL);
    }
  }
  else {
    send_to_char(ch, "Please specify the metamagic feat you wish to enable.\r\nOptions: maximize, extend\r\n");
    return;
  }
}

ACMD(do_random) {
	
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];		
  char buf[MAX_STRING_LENGTH];

	int num = 0, size, mod;
	
	one_argument(two_arguments(argument, arg1, arg2), arg3);
		
	if (!*arg1) {
		num = 1;
	}
	else
		num = atoi(arg1);
	
	if (!*arg2) {
		size = 20;
	}
	else
		size = atoi(arg2);
		
	if (!*arg3) {
		mod = 0;
	}
	else
		mod = atoi(arg3);		
		
	sprintf(buf, "(OOC) You roll %dd%d+%d resulting in a roll of... %d!", num, size, mod, dice(num, size) + mod);
	act(buf, FALSE, ch, 0, 0, TO_CHAR);
	sprintf(buf, "(OOC) $n rolls %dd%d+%d resulting in a roll of... %d!", num, size, mod, dice(num, size) + mod);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	
	return;
}

ACMD(do_skillcheck) {
	char arg[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	int skillnum = 0;
	char *s, *t;
	struct char_data *vict;
	int result = 0;
	char buf[MAX_STRING_LENGTH];
	
	two_arguments(argument, arg, arg2);
	
	if (!*arg) {
	  send_to_char(ch, "You must specify which skill you want to check.\r\n");
		return;
	}
	
	
	if (!*arg2 && GET_ADMLEVEL(ch) > 0) {
	  send_to_char(ch, "You must provide the name of the character you wish to check the skill of.\r\n");
		return;
	}
	
	if (GET_ADMLEVEL(ch) > 0) {
    if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
		  send_to_char(ch, "That character is not in this room.\r\n");
			return;
		}
	}
	else {
	  vict = ch;
	}
	
  s = strtok(argument, "'");	

  if (s == NULL) {
    send_to_char(ch, "You must specify a skill.\r\n");
    return;
  }
	
  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char(ch, "Please enclose the skill name in single quotes: ' \r\n");
    return;
  }
  t = strtok(NULL, "\0");
	
  skillnum = find_skill_num(s, SKTYPE_SKILL);

	if (skillnum <= MAX_SPELLS) {
	  send_to_char(ch, "That is not a valid skill.\r\n");
		return;
	}
	
	result = skill_roll(vict, skillnum);
	
	sprintf(buf, "(OOC) $n makes a skill check for %s with a result of %d.", spell_info[skillnum].name, result);
	
	if (vict != ch) {
	  act(buf, TRUE, vict, 0, ch, TO_VICT);
	}
	else {
	  act(buf, TRUE, ch, 0, vict, TO_CHAR);
	  act(buf, TRUE, ch, 0, vict, TO_ROOM);
	}
	return;
}

ACMD(do_devote)
{

  char arg[200], arg2[200], arg3[200], buf2[200];
	int i=0, j=0;

	one_argument(two_arguments(argument, arg, arg2), arg3);
	
	if (!*arg) {
		send_to_char(ch, "What deity do you wish to devote yourself to?\r\n"
                                 "  ('devote info <deity>' for details, 'devote list' for a list, \r\n"
                                 "   'devote search' to search or 'devote none' for none, case sensative)\r\n");
		return;
	}
	

        for (i = 0; i < NUM_DEITIES; i++) {
          skip_spaces(&argument);
          if (is_abbrev(argument, deity_list[i].name) && deity_list[i].pantheon != DEITY_PANTHEON_NONE) {
            if (GET_DEITY(ch) == i) {
              send_to_char(ch, "You are already devoted to that deity.\r\n");
              return;
            }
            if (deity_list[i].pantheon != DetermineCampaign())
            {
              send_to_char(ch, "That is not a valid deity.\r\n");
              return;
            }
            if (GET_DEITY(ch) != 0) {
              send_to_char(ch, "You have already selected the deity %s.  If you would like to change this you will need staff assistance.  See HELP PETITION.\r\n",
                           deity_list[GET_DEITY(ch)].name);
              return;
            }

            num_religion_members[GET_DEITY(ch)]--;
            num_religion_members[GET_DEITY(ch)] = MAX(0, num_religion_members[GET_DEITY(ch)]);            
            num_religion_members[i]++;
            GET_DEITY(ch) = i;
            send_to_char(ch, "You have devoted yourself to the church of %s.\r\n", deity_list[i].name);
            return;
          }
        }


        if (!strcmp(arg, "list")) {

          if (!*arg2) {
            send_to_char(ch, "Which pantheon would you like to list? (good|neutral|evil|lawful|chaotic|all)\r\n");
            return;
          }

          send_to_char(ch, "Deities of %s\r\n~~~~~~~~~~~~~~~~\r\n", CampaignWorld[CONFIG_CAMPAIGN]);

          for (i = 0; i < NUM_DEITIES; i++) {
            if ((!strcmp(arg2, "good") && deity_list[i].alignment < 500) || 
                (!strcmp(arg2, "neutral") && (deity_list[i].alignment != 0 && deity_list[i].ethos != 0)) ||
                (!strcmp(arg2, "evil") && deity_list[i].alignment  > -500) ||
                (!strcmp(arg2, "lawful") && deity_list[i].ethos < 500) ||
                (!strcmp(arg2, "chaotic") && deity_list[i].ethos > -500))
              continue;

            if (deity_list[i].pantheon == DetermineCampaign()) 
            {
              send_to_char(ch, "@Y%s@n (%s)\r\nFavored Weapon: %s\r\n",  deity_list[i].name, GET_ALIGN_ABBREV(deity_list[i].ethos, deity_list[i].alignment), 
                           weapon_list[deity_list[i].favored_weapon].name);
              sprintf(buf2, "@n");
              sprintf(buf2, "Domains: ");
              for (j = 0; j < 6; j++) {
                if (deity_list[i].domains[j] != DOMAIN_UNDEFINED) 
                {
                  if (j > 0)
                    sprintf(buf2, "%s, ", buf2);
                    sprintf(buf2, "%s%s", buf2, domain_names[deity_list[i].domains[j]]);
                }
              }
              send_to_char(ch, "%-50s", buf2);
              send_to_char(ch, "\r\nPortfolio: %s\r\n", deity_list[i].portfolio);
              send_to_char(ch, "@W----------------------------------------------------------------------@n\r\n");
            }
          }
//          send_to_char(ch, "%s", buf);
          return;
        }

        if (!strcmp(arg, "search")) {

          if (!*arg2) {
            send_to_char(ch, "What is the search type you wish to use?\r\n");
            send_to_char(ch, "Syntax is @Ydevote search <name|alignment|weapon|domain|portfolio> <keyword>@n.\r\n");
            return;
          }

          if (!*arg3) {
            send_to_char(ch, "What is the search keyword you wish to use?\r\n");
            send_to_char(ch, "Syntax is @Ydevote search <name|alignment|weapon|domain|portfolio> <keyword>@n.\r\n");
            return;
          }

          send_to_char(ch, "Deities of %s\r\n~~~~~~~~~~~~~~~~\r\n", CampaignWorld[CONFIG_CAMPAIGN]);

          for (i = 0; i < NUM_DEITIES; i++) {
              if ((!strcmp(arg2, "name") && !strstr(deity_list[i].name, CAP(arg3))) ||
                  (!strcmp(arg2, "alignment") && !strstr(GET_ALIGN_STRING(deity_list[i].ethos, deity_list[i].alignment), CAP(arg3))) ||
                  (!strcmp(arg2, "weapon") && !strstr(weapon_list[deity_list[i].favored_weapon].name, arg3)) ||
                  (!strcmp(arg2, "domain") && !strstr(domain_names[deity_list[i].domains[0]], CAP(arg3)) && 
                  !strstr(domain_names[deity_list[i].domains[1]], CAP(arg3)) && !strstr(domain_names[deity_list[i].domains[2]], CAP(arg3)) && 
                  !strstr(domain_names[deity_list[i].domains[3]], CAP(arg3)) && !strstr(domain_names[deity_list[i].domains[4]], CAP(arg3)) && 
                  !strstr(domain_names[deity_list[i].domains[5]], CAP(arg3))) ||
                  (!strcmp(arg2, "portfolio") && !strstr(deity_list[i].portfolio, CAP(arg3)) && !strstr(deity_list[i].portfolio, arg3)))
              continue;

            if (deity_list[i].pantheon == DetermineCampaign()) {
              send_to_char(ch, "@Y%s@n (%s)\r\nFavored Weapon: %s\r\n",  deity_list[i].name, GET_ALIGN_ABBREV(deity_list[i].ethos, deity_list[i].alignment), 
                           weapon_list[deity_list[i].favored_weapon].name);
              sprintf(buf2, "@n");
              sprintf(buf2, "Domains: ");
              for (j = 0; j < 6; j++) {
                if (deity_list[i].domains[j] != DOMAIN_UNDEFINED) {
                  if (j > 0)
                    sprintf(buf2, "%s, ", buf2);
                    sprintf(buf2, "%s%s", buf2, domain_names[deity_list[i].domains[j]]);
                }
              }
              send_to_char(ch, "%-50s", buf2);
              send_to_char(ch, "\r\nPortfolio: %s\r\n", deity_list[i].portfolio);
              send_to_char(ch, "@W----------------------------------------------------------------------@n\r\n");
            }
          }
          return;
        }

        if (!strcmp(arg, "info")) {
 
          if (!*arg2) {
            send_to_char(ch, "Which deity would you like to know more about?  Please capitalize the name for proper results.\r\n");
            return;
          }

          for (i = 0; i < NUM_DEITIES; i++) {
            if (!strcmp(CAP(arg2), deity_list[i].name)) {
              send_to_char(ch, "\r\n%s\r\n", deity_list[i].description);
              return;
            }
          }
        }

        send_to_char(ch, "That deity does not exist in this world. (or is not yet implemented)\r\n");
        return;

}

ACMD(do_domain)
{

  int j = 0;

  if (!(GET_DEITY(ch) + 1)) {
    send_to_char(ch, "You must worship a deity to choose your domains.\r\n");
    return;
  }
	
  if (!(IS_CLERIC(ch))) {
    send_to_char(ch, "Only clerics may choose domains.\r\n");
    return;
  }
		
  send_to_char(ch, "The domains of your deity are as follows:\r\n");

  for (j = 0; j < 6; j++)
    if (deity_list[GET_DEITY(ch)].domains[j] != DOMAIN_UNDEFINED) {
      if (j > 0)
        send_to_char(ch, ", ");
      send_to_char(ch, "%s", domain_names[deity_list[GET_DEITY(ch)].domains[j]]);
    }

  send_to_char(ch, "\r\nYou automatically have access to the spells and powers from ALL of these domains (you do not need to choose).\r\n");
  send_to_char(ch, "Type 'help domain <domain name>' for more information on a particular domain's spells and powers.\r\n");
};

ACMD(do_sponsor) {
	
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];	
	char *s;
	char className[MAX_STRING_LENGTH];
	struct char_data *victim;
	int i, j;
	
	half_chop(argument, arg1, arg2);

  if (!*arg1) {
  	send_to_char(ch, "Whom do you wish to sponsor?\r\n");
  	return;
  }
  
  if (!*arg2) {
  	send_to_char(ch, "What profession do you wish to sponsor them as?\r\n");
  	return;
  }  
  
  
  //skip_spaces(&argument);		
	
  if (!(victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person doesn't seem to be here.\r\n");
    return;
  }
	
  if (IS_NPC(victim)) {
  	send_to_char(ch, "You cannot sponsor a mob.\r\n");
  	return;
  }
  
  for (i = 0; i < strlen(arg2); i++)
    arg2[i] = tolower(arg2[i]);
  
  s = strtok(arg2, "\0");
	
	for (i = 0; i < NUM_CLASSES; i++) {
		sprintf(className, "%s", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
		for (j = 0; j < strlen((CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]); j++)
		  className[j] = tolower(className[j]);
	  if (is_abbrev(s, className)) {
	  	if (GET_CLASS_RANKS(ch, i) > 0 || GET_ADMLEVEL(ch) > ADMLVL_BUILDER) {
	  		send_to_char(ch, "@wYou have sponsored %s to advance as a %s.@n\r\n", PERS(victim, ch), (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
	  		send_to_char(victim, "@wYou have been sposored by %s to advance as a %s.@n\r\n", PERS(ch, victim), (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i]);
	  		GET_CLASS_SPONSOR(victim, i) = TRUE;
	  		return;
	  	}
	  	else {
	  		send_to_char(ch, "@wYou can't sponsor someone as that profession, as you are not one yourself.@n\r\n");
	  		return;
	  	}
	  }
	  else {
	  	continue;
	  }
	}
	send_to_char(ch, "@wThat class does not exist.@n\r\n");
	return;
};

#define CALL_TYPE_PALADIN_MOUNT    1
#define CALL_TYPE_PURCHASED_MOUNT  2
#define CALL_TYPE_FAMILIAR         3
#define CALL_TYPE_PET              4
#define CALL_TYPE_ANIMAL_COMPANION 5

ACMD(do_callset)
{

  struct char_data *mob = NULL;
  char arg1[100];
  char arg2[100];
  int ranks = 0;
 
  two_arguments(argument, arg1, arg2);
  
  if (!*arg1) {
    send_to_char(ch, "You can only set a 'mount', 'companion', 'familiar' or 'pet'.\r\n");
	return;
  }
  if (!*arg2) {
    send_to_char(ch, "You can only set a 'mount', 'companion', 'familiar' or 'pet'.\r\n");
	return;
  }
  
  if (strcmp(arg2, "setup") && !(mob = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That mob doesn't seem to be here.\r\n");
    return;
  }

  if (!IS_NPC(mob)) {
    send_to_char(ch, "You can only set mobs to be your pets.  Try charming them instead.\r\n");
    return;
  }
  if (GET_MAX_HIT(mob) > (3 * GET_HITDICE(mob) * (class_hit_die_size_fr[GET_CLASS(mob)] + ability_mod_value(GET_CON(mob)) ) ) ) {
    send_to_char(ch, "You cannot tame mobs who are more powerful than a normal mob their level.\r\n");
    return;
  }

  if (SCRIPT(mob)) {
    send_to_char(ch, "You cannot tame mobs with scripts on them.\r\n");
    return;
  }

  if (race_list[GET_RACE(mob)].family != RACE_TYPE_ANIMAL && race_list[GET_RACE(mob)].family != RACE_TYPE_MAGICAL_BEAST) {
    send_to_char(ch, "Only animals and magical beasts can be made as pets.\r\n");
    return;
  }

  if (is_abbrev(arg1, "mount")) {

    if (!strcmp(arg2, "setup")) {
      if (!GET_MOUNT_VNUM(ch)) {
        send_to_char(ch, "You must purchase a mount first.\r\n");
        return;
      }
/*
      GET_MOB_SETUP(ch) = MOB_SETUP_MOUNT;
      STATE(ch->desc) = CON_MOB_SETUP_SHORT_DESC;
*/
      return;
    }

    if (!MOB_FLAGGED(mob, MOB_MOUNTABLE)) {
      send_to_char(ch, "You can only set mounts as mobs which can actually be mounted.\r\n");
      return;
    }
        
    if (GET_GOLD(ch) >= GET_LEVEL(mob) *500) {
      if (get_skill_value(ch, SKILL_HANDLE_ANIMAL) >= GET_LEVEL(mob)) {
        send_to_char(ch, "You purchase the mount from it's owner for %d coins.\r\n", GET_LEVEL(mob) * 500);
        GET_MOUNT_VNUM(ch) = GET_MOB_VNUM(mob);
        GET_GOLD(ch) -= GET_LEVEL(mob) * 500;
      }
      else {
        send_to_char(ch, "You'll need to become better with animals if you want to befriend this beast.\r\n");
        return;
      }
    }
    else {
      send_to_char(ch, "You need %d coins in order to purchase this beast from its owner.\r\n", GET_LEVEL(mob) * 500);
      return;
    }
  }
  else if (is_abbrev(arg1, "familiar")) {
    if (!HAS_FEAT(ch, FEAT_SUMMON_FAMILIAR)) {
      send_to_char(ch, "You must have the summon familiar feat in order to have a familiar.\r\n");
      return;
    }
    if (get_size(mob) > SIZE_SMALL) {
      send_to_char(ch, "Familiars must be of size small of less.\r\n");
      return;
    }
    send_to_char(ch, "You befriend the beast and make it your familiar.\r\n");
    GET_FAMILIAR_VNUM(ch) = GET_MOB_VNUM(mob);
  }
  else if (is_abbrev(arg1, "companion")) {
    send_to_char(ch, "Please use the command 'petset' to create your animal companion.\r\n");
    return;
    if (!HAS_FEAT(ch, FEAT_ANIMAL_COMPANION)) {
      send_to_char(ch, "You must have the animal companion feat in order to have an animal companion.\r\n");
      return;
    }
    ranks = get_skill_value(ch, SKILL_HANDLE_ANIMAL);
    ranks = MAX(ranks, GET_CLASS_RANKS(ch, CLASS_DRUID));
    ranks = MAX(ranks, GET_CLASS_RANKS(ch, CLASS_RANGER));
    if (ranks >= GET_LEVEL(mob)) {
      send_to_char(ch, "You befriend the beast and make it your animal companion.\r\n");
      GET_COMPANION_VNUM(ch) = GET_MOB_VNUM(mob);
    }
    else {
      send_to_char(ch, "You'll need to become better with animals if you want to befriend this beast.\r\n");
      return;
    }
  }
  else if (is_abbrev(arg1, "pet")) {
    if (get_size(mob) > SIZE_SMALL) {
      send_to_char(ch, "Pets must be of size small of less.\r\n");
      return;
    }
    if (GET_LEVEL(mob) <= 3) {
      send_to_char(ch, "You befriend the beast and make it your pet.\r\n");
      GET_PET_VNUM(ch) = GET_MOB_VNUM(mob);
    }
    else {
      send_to_char(ch, "Pets must be level 3 or lower.\r\n");
      return;
    }
  }
  else {
    send_to_char(ch, "You can only set a 'mount', 'companion', 'familiar' or 'pet'.\r\n");
	return;
  }
  save_char(ch);

}

ACMD(do_petset)
{		
	if( ch->desc->connected != CON_PLAYING)
	{
		send_to_char(ch, "You must be in the game before you can use the Pet Editor!\r\n");
		return;
	}
        if( !HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) )
        {
            send_to_char(ch, "You need to be able to control a companion in order to use the Pet Editor!\r\n");
            return;
        }
	send_to_char(ch, "Entering Pet Edit Mode. Use commands to get a list of current commands.\r\n\r\n");
	ch->desc->connected = CON_PETSET;
	nanny(ch->desc, "show");	
}

ACMD(do_call)
{

  send_to_char(ch, "The call command is currently disabled as it and mob summoning is causing\r\n"
                   "an inordiante number of crashes.  We are going to reimplement the summon/pet\r\n"
                   "system, but until then, summoning mobs and pets will be disabled.\r\n");
  return;


  struct char_data *mob;
  char arg1[MAX_STRING_LENGTH];
  int mob_vnum = 0;
  int call_type = 0;
  struct follow_type *f;
  int j = 0; 
 
  one_argument(argument, arg1);
  
  if (!*arg1) {
    send_to_char(ch, "You can only call a 'mount', 'paladin-mount', 'companion', 'familiar' or 'pet'.\r\n");
	return;
  }

  if (num_charmies(ch) > 0) {
    send_to_char(ch, "You can only control one creature at a time.\r\n");
    return;
  }

  
  if (is_abbrev(arg1, "mount")) {
	if (GET_MOUNT_VNUM(ch)) {
	  mob_vnum = GET_MOUNT_VNUM(ch);
	  call_type = CALL_TYPE_PURCHASED_MOUNT;
	}
	else {
	  send_to_char(ch, "You have no mount to call.\r\n");
	  return;
	}
  }
  else if (is_abbrev(arg1, "paladin-mount")) {
    if (HAS_FEAT(ch, FEAT_CALL_MOUNT)) {
	  mob_vnum = 199;
	  call_type = CALL_TYPE_PALADIN_MOUNT;
	}
	else {
	  send_to_char(ch, "You have no paladin mount to call.\r\n");
	  return;
	}  
  }
  else if (is_abbrev(arg1, "familiar")) {
    if (!HAS_FEAT(ch, FEAT_SUMMON_FAMILIAR)) {
	  send_to_char(ch, "You have no familiar to call.\r\n");
	  return;
	}
	mob_vnum = GET_FAMILIAR_VNUM(ch);
	if (mob_vnum == 0) {
	  send_to_char(ch, "You have still not chosen a familiar.  Please see help class-ability-summon-familiar.\r\n");
	  return;
	}	
	call_type = CALL_TYPE_FAMILIAR;
  }
  else if (is_abbrev(arg1, "companion")) {
/*
    send_to_char(ch, "Please use the command 'pray for companion' to call your animal companion.\r\n");
    return;
*/
    if (!HAS_FEAT(ch, FEAT_ANIMAL_COMPANION)) {
	  send_to_char(ch, "You have no animal companion to call.\r\n");
	  return;
	}
	mob_vnum = GET_COMPANION_VNUM(ch);
	if (mob_vnum == 0) {
	  send_to_char(ch, "You have still not chosen an animal companion.  Please see help class-ability-animal-companion.\r\n");
	  return;
	}
	call_type = CALL_TYPE_ANIMAL_COMPANION;
  }
  else if (is_abbrev(arg1, "pet")) {
    if (!GET_PET_VNUM(ch)) {
	  send_to_char(ch, "You have no pet to call.");
	  return;
	}
	mob_vnum = GET_PET_VNUM(ch);
	call_type = CALL_TYPE_PET;
  }
  else {
    send_to_char(ch, "You can only call a 'mount', 'paladin-mount', 'companion', 'familiar' or 'pet'.\r\n");
	return;
  }
  
  if (mob_vnum) {
  
    for (f = ch->followers; f; f = f->next) {
      if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
        if (GET_MOB_VNUM(f->follower) == 199 && call_type == CALL_TYPE_PALADIN_MOUNT) {
	      send_to_char(ch, "You have already called your paladin mount.\r\n");
  		  return;
	    }
	    else if (GET_MOB_VNUM(f->follower) == GET_MOUNT_VNUM(ch) && call_type == CALL_TYPE_PURCHASED_MOUNT) {
	      send_to_char(ch, "You have already called your mount.\r\n");
		  return;
	    }
	    else if (GET_MOB_VNUM(f->follower) == GET_FAMILIAR_VNUM(ch) && call_type == CALL_TYPE_FAMILIAR) {
	      send_to_char(ch, "You have already called your familiar.\r\n");
		  return;
	    }
	    else if (GET_MOB_VNUM(f->follower) == GET_COMPANION_VNUM(ch) && call_type == CALL_TYPE_ANIMAL_COMPANION)  {
	      send_to_char(ch, "You have already called your animal companion.\r\n");
		  return;
	    }
	    else if (GET_MOB_VNUM(f->follower) == GET_PET_VNUM(ch) && call_type == CALL_TYPE_PET) {
	      send_to_char(ch, "You have already called your pet.\r\n");
		  return;
	    }
      }    
    }
  
    mob = read_mobile(mob_vnum, virtual);
	
	if (mob) {

          GET_CLASS(mob) = CLASS_FIGHTER;

          if (call_type == CALL_TYPE_ANIMAL_COMPANION || call_type == CALL_TYPE_FAMILIAR || call_type == CALL_TYPE_PALADIN_MOUNT)
            for (j = 0; j <= (GET_LEVEL(ch) - GET_HITDICE(mob)); j++) {
              advance_mob_level(mob, GET_CLASS(mob));
              GET_HITDICE(mob)++;
            }
            set_auto_mob_stats(mob);
	  

          if (GET_MAX_HIT(mob) >= (3 * GET_HITDICE(mob) * (class_hit_die_size_fr[GET_CLASS(mob)] + ability_mod_value(GET_CON(mob)) ) ) ) {
            GET_MAX_HIT(mob) = (2 * GET_HITDICE(mob) * (class_hit_die_size_fr[GET_CLASS(mob)] + ability_mod_value(GET_CON(mob)) ) );
            GET_HIT(mob) = GET_MAX_HIT(mob);
          }

          char_to_room(mob, IN_ROOM(ch));
          IS_CARRYING_W(mob) = 0;
          IS_CARRYING_N(mob) = 0;
          SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
          load_mtrigger(mob);
          add_follower(mob, ch);
          if (FIGHTING(ch)) {
            set_fighting(mob, FIGHTING(ch));
          }
          mob->master_id = GET_IDNUM(ch);	

     //      if (call_type == CALL_TYPE_FAMILIAR) {
    //	    set_familiar_bonus(ch);
    //	  }
	  
	  act("$N comes forth, answering $n's call.", TRUE, ch, 0, mob, TO_ROOM);
	  act("$N comes forth, answering your call.", TRUE, ch, 0, mob, TO_CHAR);
	 
	}
	else {
	  send_to_char(ch, "There was an error loading the mobile.  Error code 2.  Please contact an immortal.\r\n");
	  return;
	}
  
  }
  else {
    send_to_char(ch, "There was an error loading the mobile.  Error code 1.  Please contact an immortal.\r\n");
	return;
  }

}

ACMD(do_choose) {

  char arg[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  
  two_arguments(argument, arg, arg2);
  
  
  if (!*arg) {
    send_to_char(ch, "The only choice at this time is familiar bonus.\r\n");
	return;
  }
  
 
}

void set_familiar_stats(struct char_data *ch) {

return;

}

ACMD(do_haggle)
{
  if (PRF_FLAGGED(ch, PRF_HAGGLE)) {
    send_to_char(ch, "You will no longer try to haggle with prices at shops and inns.\r\n");
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_HAGGLE);
    return;
  }
  else {
    send_to_char(ch, "You will now try to haggle with prices at shops and inns.\r\n");
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_HAGGLE);
    return;
  }
}

ACMD(do_recall)
{
  char arg[100], buf[200];

  one_argument(argument, arg);

  if (IS_NPC(ch)) {
    OUTPUT_TO_CHAR("Monsters can't recall!!\r\n", ch);
    return;
  }

  if (GET_RECALL(ch) <= 1) // Simply sanity check to avoid a crash
    GET_RECALL(ch) = CONFIG_MORTAL_START;

  if (!*arg) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NORECALL)) {
      OUTPUT_TO_CHAR("A magical shroud prevents recall!\r\n", ch);
      return;
    }

    OUTPUT_TO_CHAR("Recalling.\r\n", ch);
    act("$n recalls.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(GET_RECALL(ch)));
    act("$n appears in a swirling mist.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(IN_ROOM(ch), ch, 0);

    if (GET_LEVEL(ch) > 10 && GET_ADMLEVEL(ch) < 1) {
      int gold = GET_LEVEL(ch) * 25; 
      if (GET_GOLD(ch) < gold && GET_BANK_GOLD(ch) < gold) {
        send_to_char(ch, "You do not have enough gold to recall, you need %d.\r\n", gold);
        return;
      }

      if (GET_GOLD(ch) < gold)
        GET_BANK_GOLD(ch) -= gold;
      else
        GET_GOLD(ch) -= gold;

      send_to_char(ch, "\r\nRecalling cost you %d gold coins.\r\n", gold);
    }

    return;
   
  }
  else if (!str_cmp(arg, "set"))
  {
    sprintf(buf, "Recall set.\r\n");
    OUTPUT_TO_CHAR(buf, ch);
    GET_RECALL(ch) = world[IN_ROOM(ch)].number;
    return;
  } else if (!str_cmp(arg, "reset"))
  {
    sprintf(buf, "Recall reset.\r\n");
    OUTPUT_TO_CHAR(buf, ch);
    GET_RECALL(ch) = CONFIG_MORTAL_START;
    return;
  } else {
    sprintf(buf, "Recall what?\r\n");
    OUTPUT_TO_CHAR(buf, ch);
	return;
  }
}

/*
ACMD(do_transfer_credits)
{

  char arg1[100], arg2[100];
  int num = 0;
  struct char_data *target;
  int loaded_fom_file = FALSE;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "To whom do you wish to transfer your credits?  transfercredits <name> <number of credits>\r\n");
    return;
  }

  if (!*arg2) {
    send_to_char(ch, "How many credits do you wish to transfer? transfercredits <name> <number of credits>\r\n");
    return;
  }

  num = atoi(arg2);

  if (num > (GET_CREDITS(ch) - GET_CREDITS_SPENT(ch))) {
    send_to_char(ch, "You only have %d credits total.  You tried to send %d.\r\n", GET_CREDITS(ch) - GET_CREDITS_SPENT(ch), num);
    return;
  }

  if (num <= 0) {
    send_to_char(ch, "You must transfer at least one credit.\r\n");
    return;
  }

  if (!(target = get_char_notvis(ch, arg1, NULL, FIND_CHAR_WORLD))) {
    if (load_char(arg1, target) == -1) {
      send_to_char(ch, "There is no player on record named %s.\r\n", arg1);
      return;
    }
    sprintf(GET_LOGIN_MSG(target), "%s\r\n%s has transfered %d credits to you!\r\n", GET_LOGIN_MSG(target) ? GET_LOGIN_MSG(target) : "", num);
  }

  GET_CREDITS(target) += num;
  GET_CREDITS_SPENT(ch) += num;


  send_to_char(ch, " 

  if (loaded_from_file)
    extract_char(target);

}
*/

ACMD(do_heal)
{

  struct char_data *vict;
  char arg1[100];
  int roll;
  char buf[200];
 
  one_argument(argument, arg1);

  if (!(is_innate_ready(ch, SPELL_SKILL_HEAL_USED))) {
    send_to_char(ch, "You have already used your healing attempt for now.  You must wait until you can heal again.\r\n");
    return;  
  }
  
  if (!*arg1) {
    send_to_char(ch, "Who do you wish to heal?\r\n");
	return;
  }
  
  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person doesn't seem to be here.\r\n");
    return;
  }

  if (GET_HEAL_AMOUNT(vict)) {
    send_to_char(ch, "That person has already been bandaged and cared for with the heal skill.\r\n");
    return;
  }

  if (vict != ch && GET_HIT(vict) <= 0) {
    act("Your healing attempt stabilizes $N, saving $M from the clutches of death.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's healing attempt stabilizes you, saving you from the clutches of death.", FALSE, ch, 0, vict, TO_VICT);
    act("$n's healing attempt stabilizes $N, saving $M from the clutches of death.", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_HIT(vict) = 1;
    GET_POS(vict) = POS_RESTING;
    if (is_innate_ready(ch, SPELL_SKILL_HEAL_USED) && GET_HEAL_USED(ch) == 0) {
      add_innate_timer(ch, SPELL_SKILL_HEAL_USED);
    }
    return;
  }

  roll = skill_roll(ch, SKILL_HEAL);
  GET_HEAL_ROLL(vict) = roll;
  GET_HEAL_AMOUNT(vict) = roll;
  GET_HIT(vict) = MIN(GET_HIT(vict) + roll, GET_MAX_HIT(vict));

  if (vict == ch) {
    sprintf(buf, "You healed yourself for %d hit points plus %d hit points over %d rounds.", 
            GET_HEAL_ROLL(vict), GET_HEAL_ROLL(vict), GET_HEAL_AMOUNT(vict) / (MAX(1, 
            GET_HEAL_ROLL(vict) / 10) + 
            GET_HEAL_ROLL(vict) % 10 == 0 ? 0 : 1));
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
  }
  else {
    sprintf(buf, "You healed $N for %d hit points and %d hit points over %d rounds.", 
            GET_HEAL_ROLL(vict), 
            GET_HEAL_ROLL(vict), 
            GET_HEAL_AMOUNT(vict) / (MAX(1, GET_HEAL_ROLL(vict) / 10) + GET_HEAL_ROLL(vict) % 10 == 0 ? 
            0 : 1));
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    sprintf(buf, "$n healed you for %d hit points and %d hit points over %d rounds.", 
            GET_HEAL_ROLL(vict), 
            GET_HEAL_ROLL(vict), 
            GET_HEAL_AMOUNT(vict) / (MAX(1, GET_HEAL_ROLL(vict) / 10) + GET_HEAL_ROLL(vict) % 10 == 0 ? 
            0 : 1));
    act(buf, FALSE, ch, 0, vict, TO_VICT);
  }
  sprintf(buf, "$n healed $N for %d hit points and %d hit points over %d rounds.", GET_HEAL_ROLL(vict), 
          GET_HEAL_ROLL(vict),
          GET_HEAL_AMOUNT(vict) / (MAX(1, GET_HEAL_ROLL(vict) / 10) + GET_HEAL_ROLL(vict) % 10 == 0 ? 0 
          : 1));
  act(buf, FALSE, ch, 0, vict, TO_NOTVICT);

  if (is_innate_ready(ch, SPELL_SKILL_HEAL_USED) && GET_HEAL_USED(ch) == 0) {
    add_innate_timer(ch, SPELL_SKILL_HEAL_USED);
  }
  if (GET_FIGHT_BLEEDING_DAMAGE(vict))
    send_to_char(vict, "Your bleeding stops.\r\n");
  GET_FIGHT_BLEEDING_DAMAGE(vict) = 0;

}

ACMD(do_suicide)
{
  int i;
  char name[100];
  struct account_data *account;
  int pfilepos = GET_PFILEPOS(ch);
  char arg[100];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You must supply your account password to delete your character.\r\n");
    return;
  }

  if (IS_NPC(ch))
    return;

  account = ch->desc->account;


  if (strncmp(CRYPT(arg, account->password), account->password, MAX_PWD_LENGTH)) {
    send_to_char(ch, "Incorrect password.  Suicide attempt cancelled.\r\n");
    return;
  }

  sprintf(name, "%s", GET_NAME(ch));

  if (!PLR_FLAGGED(ch, PLR_SUICIDE)) {
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SUICIDE);
    send_to_char(ch, "You have been set for deletion.  Either type nosuicide to cancel or suicide again to delete.\r\n");
    return;
  }

  SET_BIT_AR(PLR_FLAGS(ch), PLR_DELETED);
  save_char(ch);
  send_to_char(ch, "You have committed suicide and have been deleted.\r\n");
  mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s has self-deleted.", GET_NAME(ch));
  extract_char(ch);
  if (ch->desc && ch->desc->account)
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
      if (account->character_names[i] && !strcmp(name, account->character_names[i])) {
        account->character_names[i] = NULL;
        save_account(account);
      }
  remove_player(pfilepos);
}

ACMD(do_nosuicide)
{
  if (!PLR_FLAGGED(ch, PLR_SUICIDE)) {
    send_to_char(ch, "You are not trying to commit suicide... are you?\r\n");
    return;
  }
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_SUICIDE);
  send_to_char(ch, "You wisely decide not to take your own life.\r\n");
}

ACMD(do_bonuslevels)
{

  char arg[100];
  int i = 0;

  one_argument(argument, arg);

  if (!*arg) {

    send_to_char(ch, "You have %d arcane bonus levels to spend and %d divine bonus levels to spend.\r\n", 
                 ch->player_specials->bonus_levels_arcane, ch->player_specials->bonus_levels_divine);
    for (i = 0; i < NUM_CLASSES; i++)
      if (ch->player_specials->bonus_levels[i] > 0)
        send_to_char(ch, "%s: %d bonus levels\r\n", 
                    (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? pc_class_types_dl_aol : pc_class_types_core)[i],
                    ch->player_specials->bonus_levels[i]);
    return;
  }

  for (i = 0; i < NUM_CLASSES; i++) {
    if (is_abbrev(arg, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[i]))
      break;
  }

  if (!IS_CHAR_SPELLCASTER_CLASS(ch, i)) {
    send_to_char(ch, "You do not have spellcasting ability in that class.\r\n");
    return;
  }

  if (IS_ARCANE_CLASS(i)) {
    if (ch->player_specials->bonus_levels_arcane <= 0) {
      send_to_char(ch, "You don't have any arcane bonus levels to spend.\r\n");
      return;
    }
    ch->player_specials->bonus_levels_arcane--;
    ch->player_specials->bonus_levels[i] += 1;
    send_to_char(ch, "Your caster level in the %s class has increased by 1 to bring it to a new total of %d.\r\n",
                     (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[i],
                     GET_CASTER_LEVEL(ch, i));
   
  }
  else if (IS_DIVINE_CLASS(i)) {
    if (ch->player_specials->bonus_levels_divine <= 0) {
      send_to_char(ch, "You don't have any divine bonus levels to spend.\r\n");
      return;
    }
    ch->player_specials->bonus_levels_divine--;
    ch->player_specials->bonus_levels[i] += 1;
    send_to_char(ch, "Your caster level in the %s class has increased by 1 to bring it to a new total of %d.\r\n",
                     (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? class_names_dl_aol : class_names_core)[i],
                     GET_CASTER_LEVEL(ch, i));
   
  }
  else {
    send_to_char(ch, "That class is not a spellcaster.\r\n");
    return;
  }
}

ACMD(do_spontaneous)
{

  if (GET_CLASS_RANKS(ch, CLASS_CLERIC) <= 0 && GET_CLASS_RANKS(ch, CLASS_DRUID) < 0) {
    send_to_char(ch, "Only clerics and druid can use spontaneous spellcasting.\r\n");
    return;
  }

  if (IS_SET_AR(PRF_FLAGS(ch), PRF_SPONTANEOUS)) {
    send_to_char(ch, "You are no longer using spontaneous spellcasting.\r\n");
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SPONTANEOUS);
    return;
  }
  else {
    send_to_char(ch, "You are now using spontaneous spellcasting.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_SPONTANEOUS);
    return;
  }

}

ACMD(do_setaffinity)
{

  char arg[100];

  one_argument(argument, arg);

  if (GET_CLASS_RANKS(ch, CLASS_CLERIC) <= 0 || !IS_NEUTRAL(ch)) {
    send_to_char(ch, "Only neutral clerics can declare their affinity to positive or negative energy.\r\n");
    return;
  }

  if (PRF_FLAGGED(ch, PRF_POSITIVE) || PRF_FLAGGED(ch, PRF_NEGATIVE)) {
    send_to_char(ch, "You have already delcared your affinity to %s energy.\r\n", PRF_FLAGGED(ch, PRF_POSITIVE) ? "positive" : "negative");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Please choose to declare your affinity to either positive or negative energy\r\n");
    return;
  }

  if (is_abbrev(arg, "positive")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_POSITIVE);
    send_to_char(ch, "You have declared an affinity towards positive energy.\r\n");
    return;
  }
  else if (is_abbrev(arg, "negative")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_NEGATIVE);
    send_to_char(ch, "You have declared an affinity towards negative energy.\r\n");
    return;
  }
  else {
    send_to_char(ch, "Please choose to declare your affinity to either positive or negative energy\r\n");
    return;
  }

}

ACMD(do_backup)
{

  send_to_char(ch, "You backup your equipment and inventory.\r\n");
  Crash_crashsave(ch, TRUE);

}

ACMD(do_companionsettype)
{

  char arg[100];
  int i;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please choose one of the following as your animal companion type:\r\n");
    for (i = 1;i <= NUM_COMPANION_TYPES; i++)
      send_to_char(ch, "%s\r\n", companion_types[i]);
    return;
  }

  if (GET_COMPANION_TYPE(ch)) {
    send_to_char(ch, "You have already chosen your animal companion type.\r\n");
    return;
  }

  for (i = 1; i <= NUM_COMPANION_TYPES; i++)
    if (!strcmp(arg, companion_types[i]))
      break;

  if (i > NUM_COMPANION_TYPES) {
    send_to_char(ch, "Please choose one of the following as your animal companion type:\r\n");
    for (i = 1;i <= NUM_COMPANION_TYPES; i++)
      send_to_char(ch, "%s\r\n", companion_types[i]);
    return;
  }

  GET_COMPANION_TYPE(ch) = i;

  send_to_char(ch, "You have chosen your animal companion type to be '%s'.\r\n", companion_types[i]);

}

ACMD(do_companionsetname)
{

  char arg[100];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What would you like to name your animal companion?\r\n");
    return;
  }

  if (strlen(arg) > 100)
    arg[99] = '\0';

	/* Always do a free() on strdup()ed strings,
	 * this one in free_char() int db.c */
  GET_COMPANION_NAME(ch) = strdup(arg);

  send_to_char(ch, "You have named your animal companion, '%s'.\r\n", argument);

}

ACMD(do_gift)
{

  char arg1[100], arg2[100], arg3[100];
  struct char_data *vict = NULL;
  long exp = 0;

  one_argument(one_argument(one_argument(argument, arg1), arg2), arg3);

  if (!*arg1 || !*arg2 || !*arg3) {
    send_to_char(ch, "Syntax is 'gift <target name> <gift exp amount> <reason for gift: roleplay | helpedme>\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)) || !CAN_SEE(ch, vict)) {
    send_to_char(ch, "That person either doesn't exist or isn't online.\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(ch, "You can't gift experience to yourself.\r\n");
    return;
  } 

  if ((exp = atoi(arg2)) < 0) {
    send_to_char(ch, "You can't gift negative experience.\r\n");
    return;
  }

  if (exp > ch->desc->account->gift_experience) {
    send_to_char(ch, "You don't have that much gift experience to gift.  You only have %d.\r\n", ch->desc->account->gift_experience);
    return;
  }

  if (strcmp(arg3, "roleplay") && strcmp(arg3, "helpedme")) {
    send_to_char(ch, "You can only gift experience for role playing or people who have helped you. (roleplay | helpedme)\r\n"
                     "Abuse of this system will result in you losing all account exp and no longer being able to receive it.\r\n"
                     "You will also lose all account exp rewards such as advanced races.  See help gift account experience.\r\n");
    return;
  }

  if (vict->desc == NULL || vict->desc->account == NULL) {
    send_to_char(ch, "That person cannot receive your gift right now, try again later.\r\n");
    return;
  }

  if (vict->desc->account == ch->desc->account) {
    send_to_char(ch, "You cannot gift experience to another character on your own account and you may only have one account at any time.\r\n");
    return;
  }

  ch->desc->account->gift_experience -= exp;
//  vict->desc->account->experience += exp;

  exp = (level_exp(GET_CLASS_LEVEL(vict) + 1, GET_REAL_RACE(vict)) - level_exp(GET_CLASS_LEVEL(vict), GET_REAL_RACE(vict))) * exp / 1000;

  if (exp < 0)
    exp += -(exp * 2);

	char *tmpdesc = NULL;
  send_to_char(ch, "@YYou have gifted %ld experience to %s for %s.@n\r\n", exp, has_intro(ch, vict) ? GET_NAME(vict) : (tmpdesc = which_desc(vict)),
               !strcmp(arg3, "helpedme") ? "helping you" : "role playing");
  send_to_char(vict, "@Y%s has gifted %ld experience to you for %s.@n\r\n", CAP(has_intro(vict, ch) ? GET_NAME(ch) : tmpdesc), exp,
               !strcmp(arg3, "helpedme") ? "helping them" : "role playing");
	free(tmpdesc);
  mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s has gifted %ld experience to %s for %s.", GET_NAME(ch), exp, GET_NAME(vict),
               !strcmp(arg3, "helpedme") ? "helping them" : "role playing");

  gain_exp(vict, exp);

}

ACMD(do_dismiss_mob)
{

  struct char_data *vict;
  char arg1[200];

  one_argument(argument, arg1);

  if (!*arg1) {
    send_to_char(ch, "Who do you want to dismiss?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)) || !CAN_SEE(ch, vict)) {
    send_to_char(ch, "That mob either doesn't exist or isn't online.\r\n");
    return;
  }

  if (!IS_NPC(vict)) {
    send_to_char(ch, "That person is not a mob.\r\n");
    return;
  }

  if (!AFF_FLAGGED(vict, AFF_CHARM) || vict->master != ch) {
    send_to_char(ch, "That mob is not a charmie of yours.\r\n");
    return;
  }

  act("You dismiss $N who returns to $S home.", false, ch, 0, vict, TO_CHAR);
  act("$n dismisses $N who returns to $S home.", false, ch, 0, vict, TO_ROOM);

  extract_char(vict);
}

ACMD(do_summon)
{

  if (IS_NPC(ch)) {
    send_to_char(ch, "Only player characters can have summons.\r\n");
    return;
  }

  if (ch->player_specials->summon_num == 0) {
    send_to_char(ch, "You do not currently have anything summoned.\r\n");
    return;
  }


  send_to_char(ch, "You have summoned a %s.\r\n", ch->player_specials->summon_desc);
  send_to_char(ch, "He has %d of %d hit points.\r\n", ch->player_specials->summon_cur_hit , ch->player_specials->summon_max_hit);
  send_to_char(ch, "He has an armor class of %d and damage reduction of %d.\r\n", compute_summon_armor_class(ch, NULL), ch->player_specials->summon_dr);
  send_to_char(ch, "He has the following attacks:\r\n");

  int i = 0;
  for (i = 0; i < 5; i++) {
    if (ch->player_specials->summon_attack_to_hit[i] > 0) {
      send_to_char(ch, "Attack %d: +%d to hit for %dd%d+%d damage.\r\n", i + 1, ch->player_specials->summon_attack_to_hit[i], ch->player_specials->summon_attack_ndice[i],
                   ch->player_specials->summon_attack_sdice[i], ch->player_specials->summon_attack_dammod[i]);
    }
  }
  send_to_char(ch, "\r\n");

}

ACMD(do_companion)
{

  if (IS_NPC(ch)) {
    send_to_char(ch, "Only player characters can have animal companions.\r\n");
    return;
  }

  if (ch->player_specials->companion_num == 0) {
    send_to_char(ch, "You do not currently have an animal companion.\r\n");
    return;
  }


  send_to_char(ch, "Your animal companion is a %s.\r\n", ch->player_specials->companion_desc);
  send_to_char(ch, "He has %d of %d hit points.\r\n", ch->player_specials->companion_cur_hit , ch->player_specials->companion_max_hit);
  send_to_char(ch, "He has an armor class of %d and damage reduction of %d.\r\n", compute_companion_armor_class(ch, NULL), 
ch->player_specials->companion_dr);
  send_to_char(ch, "He has the following attacks:\r\n");

  int i = 0;
  for (i = 0; i < 5; i++) {
    if (ch->player_specials->companion_attack_to_hit[i] > 0) {
      send_to_char(ch, "Attack %d: +%d to hit for %dd%d+%d damage.\r\n", i + 1, ch->player_specials->companion_attack_to_hit[i] + 
                   compute_companion_base_hit(ch), ch->player_specials->companion_attack_ndice[i],
                   ch->player_specials->companion_attack_sdice[i], ch->player_specials->companion_attack_dammod[i]);
    }
  }
  send_to_char(ch, "\r\n");

}


ACMD(do_respec)
{

  int i = 0;

//  one_argument(argument, arg);

  skip_spaces(&argument);

  if (!ch->desc) {
    send_to_char(ch, "Only players can respec.\r\n");
    return;
  }

  if (!ch->desc->account) {
    send_to_char(ch, "Only online players can respec.\r\n");
    return;
  }

  if (!*argument) {
    send_to_char(ch, "You must supply your account password to respec your character.\r\n");
    return;
  }
/*
  if (ch->desc->account->experience < 1000) {
    send_to_char(ch, "You need 1000 account experience or more to respec your character.\r\n");
    return;
  }
*/

  if (!strncmp(CRYPT(argument, ch->desc->account->password), ch->desc->account->password, MAX_PWD_LENGTH)) {

    if(GET_CLASS_LEVEL(ch) <= 1) {
      send_to_char(ch, "You have to be higher than level 1 to respec.\r\n");
      return;
    }


//    ch->desc->account->experience = MAX(0, ch->desc->account->experience - 1000);

    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i))
        obj_to_char(unequip_char(ch, i), ch);
    }

    GET_WISHES(ch) += GET_WISH_STR(ch);
    GET_WISHES(ch) += GET_WISH_DEX(ch);
    GET_WISHES(ch) += GET_WISH_CON(ch);
    GET_WISHES(ch) += GET_WISH_INT(ch);
    GET_WISHES(ch) += GET_WISH_WIS(ch);
    GET_WISHES(ch) += GET_WISH_CHA(ch);

    ch->real_abils.str = 10;
    ch->real_abils.dex = 10;
    ch->real_abils.con = 10;
    ch->real_abils.intel = 10;
    ch->real_abils.wis = 10;
    ch->real_abils.cha = 10;
    ch->setstats_not_avail = FALSE;
    ch->stat_points_given = FALSE;
    int ax = get_artisan_exp(ch);
    init_respec_char(ch);
    init_char_respec_two(ch);
    GET_ARTISAN_EXP(ch) = ax;
    save_char(ch);

    send_to_char(ch, "You have respecced your character.  You may level up your clases again by typing @Ylevelup@n.\r\n"
                     "You will need to reassign your stat points first though, with the @Ysetstats@n command.\r\n");
   
    return;
  } else {
    send_to_char(ch, "There was an issue matching your password given with the one on file.  If you believe this to be in error please inform a staff member.\r\n");
    return;
  }
}


ACMD(do_setcamp)
{

  struct obj_data *wood = NULL;
  struct obj_data *camp = NULL;
  int roll = 0;

  if (!(is_innate_ready(ch, SPELL_SKILL_CAMP_USED))) {
    send_to_char(ch, "You have already used your camping attempt for now.  You must wait until you can set camp again.\r\n");
    return;  
  }

  for (wood = ch->carrying; wood; wood = wood->next_content)
    if (GET_OBJ_VNUM(wood) == 64015)
      break;

  if (!wood || GET_OBJ_VNUM(wood) != 64015) {
    for (wood = ch->carrying; wood; wood = wood->next_content)
      if (GET_OBJ_VNUM(wood) == 64031)
        break;
  }

  if (!wood || (GET_OBJ_VNUM(wood) != 64015 && GET_OBJ_VNUM(wood) != 64031)) {
    for (wood = ch->carrying; wood; wood = wood->next_content)
      if (GET_OBJ_VNUM(wood) == 64032)
        break;
  }

  
  
  if (!wood || (GET_OBJ_VNUM(wood) != 64015 && GET_OBJ_VNUM(wood) != 64031 &&GET_OBJ_VNUM(wood) != 64032)) {
    send_to_char(ch, "You need to have some regular wood in your inventory to make a campfire.\r\n");
    return;
  }

  camp = read_object(30094, VIRTUAL);

  if (!camp)
    return;

  roll = skill_roll(ch, SKILL_SURVIVAL);

  if (roll <= 10) {
    send_to_char(ch, "You fail to set up a proper camp.\r\n");
  }
  else {
    GET_OBJ_TIMER(camp) = roll;

    GET_OBJ_VAL(camp, 0) = roll;

    obj_from_char(wood);
    extract_obj(wood);

    obj_to_room(camp, IN_ROOM(ch));

    act("You set up camp.", false, ch, 0, 0, TO_CHAR);
    act("$n sets up camp.", false, ch, 0, 0, TO_ROOM);
  }

  if (is_innate_ready(ch, SPELL_SKILL_CAMP_USED) && GET_CAMP_USED(ch) == 0) {
    add_innate_timer(ch, SPELL_SKILL_CAMP_USED);
  }

}

ACMD(do_lfg)
{

  char arg[100];
  struct char_data *i = NULL, *next_char = NULL;

  one_argument(argument, arg);

  skip_spaces(&argument);

  if (!*arg) {

    send_to_char(ch, "\r\nPlayers Looking for a Group.\r\n"
                     "~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                     "\r\n"
                     "%-20s %-5s %-5s %-7s %s\r\n",
                     "Player Name", "Level", "Align", "Race", "Classes");

    for (i = character_list; i; i = next_char) {
      next_char = i->next;

      if (IS_NPC(ch) || !i->desc)
        continue;

      if (PRF_FLAGGED(i, PRF_LFG)) {
        send_to_char(ch, "%-20s %-5d %-5s %-7s %s\r\n", GET_NAME(i), GET_LEVEL(i),
        GET_ALIGN_ABBREV(GET_ETHOS(i), GET_ALIGN(i)),
        race_list[GET_RACE(i)].abbrev,
        class_desc_str(i, 1, 0));
        if (GET_LFG_STRING(i) != NULL)
          send_to_char(ch, "  %s\r\n", GET_LFG_STRING(i));
      }
    }
  }
  else if (strlen(argument) > 60) {
    send_to_char(ch, "Your lfg string must be 60 characters or less.\r\n");
    return;
  }
  else if (!strcmp(arg, "set")) {
	if (!PRF_FLAGGED(ch, PRF_LFG)) {
		send_to_char(ch, "You will now be listed as looking for group.\r\n");
		SET_BIT_AR(PRF_FLAGS(ch), PRF_LFG);
	}
	else {
		send_to_char(ch, "You will no longer be listed as looking for group.\r\n");
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LFG);
                GET_LFG_STRING(ch) = NULL;
	}
  }
  else {
    sprintf(argument, "%s@n", argument);

    GET_LFG_STRING(ch) = strdup(argument);
 
    send_to_char(ch, "Your looking for group string is now:\r\n"
                     "  %s\r\n", argument);
  }
}

ACMD(do_favoredenemy) {

  char arg[100];
  int i = 0;

  one_argument(argument, arg);

  if (HAS_FEAT(ch, FEAT_FAVORED_ENEMY_AVAILABLE) < 1) {
    send_to_char(ch, "You do not have any favored enemy choices at the moment.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Race Families Available for Choosing:\r\n");
    for (i = 1; i <= NUM_RACE_TYPES; i++)
      send_to_char(ch, "%s\r\n", race_types[i]);
    send_to_char(ch, "\r\n");
    return;
  }

  for (i = 1; i <= NUM_RACE_TYPES; i++)
    if (!strcmp(race_types[i], arg)) {
      SET_COMBAT_FEAT(ch, CFEAT_FAVORED_ENEMY, i);
      SET_FEAT(ch, FEAT_FAVORED_ENEMY, HAS_REAL_FEAT(ch, FEAT_FAVORED_ENEMY) + 1);
      SET_FEAT(ch, FEAT_FAVORED_ENEMY_AVAILABLE, HAS_REAL_FEAT(ch, FEAT_FAVORED_ENEMY_AVAILABLE) - 1);
      send_to_char(ch, "You choose the favored enemy: %s.\r\n", race_types[i]);
      return;
    }

  if (i > NUM_RACE_TYPES) {
    send_to_char(ch, "Race Families Available for Choosing:\r\n");
    for (i = 1; i <= NUM_RACE_TYPES; i++)
      send_to_char(ch, "%s\r\n", race_types[i]);
    send_to_char(ch, "\r\n");
  }
  

}

ACMD(do_disguise)
{
    char arg[20], arg2[20], arg3[20], buf[MAX_STRING_LENGTH];

    one_argument(two_arguments(argument, arg, arg2), arg3);
    int mod = 0, i = 0, choice = 0;

    if (!*arg) {
        send_to_char(ch, "You need to state whether this is desc1 or desc2, or if you are finishing or removing your disguise. (desc1|desc2|finish|remove)\r\n");
        return;
    }

    if (strlen(arg) > 20) {
        send_to_char(ch, "The first argument must be 20 characters or less.\r\n");
        return;
    }

    if (!strcmp(arg, "remove")) {
      GET_DISGUISE_SEX(ch) = 0;
      GET_DISGUISE_DESC_1(ch) = 0;
      GET_DISGUISE_ADJ_1(ch) = 0;
      GET_DISGUISE_DESC_2(ch) = 0;
      GET_DISGUISE_ADJ_2(ch) = 0;
      if (!AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
          GET_DISGUISE_RACE(ch) = 0;
          REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DISGUISED);
      }
      send_to_char(ch, "You remove your disguise.\r\n");
      act("$n removes $s disguise.", true, ch, 0, 0, TO_ROOM);
      return;
    }

    if (!strcmp(arg, "finish")) {
        if (GET_DISGUISE_SEX(ch) == 0 && GET_DISGUISE_DESC_1(ch) == 0 && GET_DISGUISE_ADJ_1(ch) == 0 &&
            GET_DISGUISE_DESC_2(ch) == 0 && GET_DISGUISE_ADJ_2(ch) == 0 && GET_DISGUISE_RACE(ch) == 0)
            mod += 5;
        if (GET_DISGUISE_SEX(ch) != 0)
            mod -= 2;
        else
            GET_DISGUISE_SEX(ch) = GET_SEX(ch);
        if (GET_DISGUISE_RACE(ch) != 0 && !AFF_FLAGGED(ch, AFF_WILD_SHAPE))
            mod -= 2;
        else
            GET_DISGUISE_RACE(ch) = GET_RACE(ch);
        GET_DISGUISE_ROLL(ch) = mod + roll_skill(ch, SKILL_DISGUISE);
        send_to_char(ch, "You put on a disguise.\r\n");
        act("$n puts on a disguise.", true, ch, 0, 0, TO_ROOM);
        SET_BIT_AR(AFF_FLAGS(ch), AFF_DISGUISED);
				char *tmpdesc;
        send_to_char(ch, "Your disguise looks like: %s", tmpdesc = which_desc(ch));
				free(tmpdesc);
        act("The disguise looks like: $n.", true, ch, 0, 0, TO_ROOM);
        return;
    }

    if (!*arg2) {
        send_to_char(ch, "You need to state what part of your disguise you want to set.  Enter list for options.\r\n");
        return;
    }

    if (strlen(arg2) > 20) {
        send_to_char(ch, "The second argument must be 20 characters or less.\r\n");
        return;
    }

    if (!*arg3) {
        send_to_char(ch, "You need to state what you want to set that part of the disguise to.  Enter list for options.\r\n");
        return;
    }

    if (strlen(arg3) > 20) {
        send_to_char(ch, "The third argument must be 20 characters or less.\r\n");
        return;
    }

    if (!strcmp(arg2, "list")) {
        send_to_char(ch, "Possible choices are: (race|sex|eyes|nose|ears|face|scar|hair|build|complexion)\r\n");
        return;
    }

    if (!strcmp(arg2, "race")) {
        if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
            send_to_char(ch, "You cannot disguise your race while wild shaped.\r\n");
            return;
        }
        if (!strcmp(arg3, "list")) {
            send_to_char(ch, "Possible choices are:\r\n");
            for (i=0; i < NUM_RACES; i++) {
                if (!race_list[i].is_pc || race_list[i].size != get_size(ch))
                    continue;
                sprintf(buf, "%d) %-30s ", i, race_list[i].name);
                if (i % 2 == 1)
                    sprintf(buf, "%s\r\n", buf);
                SEND_TO_Q(buf, ch->desc);
            }
            if (i % 2 == 0)
                SEND_TO_Q("\r\n", ch->desc);
            SEND_TO_Q("\r\n", ch->desc);
            return;
        }
        choice = atoi(arg3);

        for (i=0; i < NUM_RACES; i++) {
            if (!race_list[i].is_pc || race_list[i].size != get_size(ch))
                continue;
            if (choice == i)
                break;
        }

        if (i < 1 || i > NUM_RACES) {
            send_to_char(ch, "That is an invalid race choice.  Please use list to see what is available.\r\n");
            return;
        }

        GET_DISGUISE_RACE(ch) = i;
        send_to_char(ch, "You have set your disguise race to %s.  This will incur a -2 to your final check. Type disguise finish to enable.\r\n", race_list[i].name);
        return;
    }
    else if (!strcmp(arg2, "sex")) {


    }

    if (!strcmp(arg, "desc1")) {
        if (!strcmp(arg2, "eyes")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_EYE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, eye_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_EYE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_EYE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid eye choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_EYES;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise eyes to %s.  Type disguise finish to enable.\r\n", eye_descriptions[i]);
            return;
        }
        else if (!strcmp(arg2, "nose")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_NOSE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, nose_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_NOSE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_NOSE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid nose choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_NOSE;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise nose to %s.  Type disguise finish to enable.\r\n", nose_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "ears")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_EAR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, ear_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_EAR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_EAR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid ear choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_EARS;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise ears to %s.  Type disguise finish to enable.\r\n", ear_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "face")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_FACE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, face_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_FACE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_FACE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid face choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_FACE;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise face to %s.  Type disguise finish to enable.\r\n", face_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "scar")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_SCAR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, scar_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_SCAR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_SCAR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid scar choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_SCAR;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise scar to %s.  Type disguise finish to enable.\r\n", scar_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "hair")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_HAIR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, hair_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_HAIR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_HAIR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid hair choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_HAIR;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise hair to %s.  Type disguise finish to enable.\r\n", hair_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "build")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_BUILD_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, build_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_BUILD_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_BUILD_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid build choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_BUILD;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise build to %s.  Type disguise finish to enable.\r\n", build_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "complexion")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_COMPLEXION_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, complexion_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_COMPLEXION_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_COMPLEXION_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid complexion choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_1(ch) = FEATURE_TYPE_COMPLEXION;
            GET_DISGUISE_ADJ_1(ch) = i;
            send_to_char(ch, "You have set your disguise complexion to %s.  Type disguise finish to enable.\r\n", complexion_descriptions[i]);
            return;

        }
        else {
            send_to_char(ch, "Invalid choice. (race|sex|eyes|nose|ears|face|scar|hair|build|complexion)\r\n");
            return;
        }

    }
    else if (!strcmp(arg, "desc2")) {
        if (!strcmp(arg2, "eyes")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_EYE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, eye_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_EYE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_EYE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid eye choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_EYES;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise eyes to %s.  Type disguise finish to enable.\r\n", eye_descriptions[i]);
            return;
        }
        else if (!strcmp(arg2, "nose")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_NOSE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, nose_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_NOSE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_NOSE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid nose choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_NOSE;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise nose to %s.  Type disguise finish to enable.\r\n", nose_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "ears")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_EAR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, ear_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_EAR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_EAR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid ear choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_EARS;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise ears to %s.  Type disguise finish to enable.\r\n", ear_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "face")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_FACE_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, face_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_FACE_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_FACE_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid face choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_FACE;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise face to %s.  Type disguise finish to enable.\r\n", face_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "scar")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_SCAR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, scar_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_SCAR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_SCAR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid scar choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_SCAR;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise scar to %s.  Type disguise finish to enable.\r\n", scar_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "hair")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_HAIR_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, hair_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_HAIR_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_HAIR_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid hair choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_HAIR;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise hair to %s.  Type disguise finish to enable.\r\n", hair_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "build")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_BUILD_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, build_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_BUILD_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_BUILD_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid build choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_BUILD;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise build to %s.  Type disguise finish to enable.\r\n", build_descriptions[i]);
            return;

        }
        else if (!strcmp(arg2, "complexion")) {
            if (!strcmp(arg3, "list")) {
                send_to_char(ch, "Possible choices are:\r\n");
                for (i=0; i < NUM_COMPLEXION_DESCRIPTORS; i++) {
                    sprintf(buf, "%d) %-30s ", i, complexion_descriptions[i]);
                    if (i % 2 == 1)
                        sprintf(buf, "%s\r\n", buf);
                    SEND_TO_Q(buf, ch->desc);
                }
                if (i % 2 == 0)
                    SEND_TO_Q("\r\n", ch->desc);
                SEND_TO_Q("\r\n", ch->desc);
                return;
            }
            choice = atoi(arg3);

            for (i=0; i < NUM_COMPLEXION_DESCRIPTORS; i++) {
                if (choice == i)
                    break;
            }

            if (i < 1 || i > NUM_COMPLEXION_DESCRIPTORS) {
                send_to_char(ch, "That is an invalid complexion choice.  Please use list to see what is available.\r\n");
                return;
            }

            GET_DISGUISE_DESC_2(ch) = FEATURE_TYPE_COMPLEXION;
            GET_DISGUISE_ADJ_2(ch) = i;
            send_to_char(ch, "You have set your disguise complexion to %s.  Type disguise finish to enable.\r\n", complexion_descriptions[i]);
            return;

        }
        else {
            send_to_char(ch, "Invalid choice. (race|sex|eyes|nose|ears|face|scar|hair|build|complexion)\r\n");
            return;
        }

    }

    send_to_char(ch, "You need to state whether this is desc1 or desc2, or if you are finishing or removing your disguise. (desc1|desc2|finish|remove)\r\n");
    
    return;
}

struct wild_shape_mods {
  byte strength;
  byte constitution;
  byte dexterity;
  byte natural_armor;
};

struct wild_shape_mods *set_wild_shape_mods(int race)
{

  struct wild_shape_mods *abil_mods;

  CREATE(abil_mods, struct wild_shape_mods, 1);

  abil_mods->strength = 0;
  abil_mods->constitution = 0;
  abil_mods->dexterity = 0;
  abil_mods->natural_armor = 0;

  switch(race_list[race].family)
  {
    case RACE_TYPE_ANIMAL:
      switch (race_list[race].size)
      {
        case SIZE_DIMINUTIVE:
          abil_mods->dexterity = 6;
          abil_mods->strength = -4;
          abil_mods->natural_armor = 10;
          break;
        case SIZE_TINY:
          abil_mods->dexterity = 4;
          abil_mods->strength = -2;
          abil_mods->natural_armor = 10;
          break;
        case SIZE_SMALL:
          abil_mods->dexterity = 2;
          abil_mods->natural_armor = 10;
          break;
        case SIZE_MEDIUM:
          abil_mods->strength = 2;
          abil_mods->natural_armor = 20;
          break;
        case SIZE_LARGE:
          abil_mods->dexterity = -2;
          abil_mods->strength = 4;
          abil_mods->natural_armor = 40;
          break;
        case SIZE_HUGE:
          abil_mods->dexterity = -4;
          abil_mods->strength = 6;
          abil_mods->natural_armor = 60;
          break;
      }
      break;
    case RACE_TYPE_MAGICAL_BEAST:
      switch (race_list[race].size)
      {
        case SIZE_TINY:
          abil_mods->dexterity = 8;
          abil_mods->strength = -2;
          abil_mods->natural_armor = 30;
          break;
        case SIZE_SMALL:
          abil_mods->dexterity = 4;
          abil_mods->natural_armor = 20;
          break;
        case SIZE_MEDIUM:
          abil_mods->strength = 4;
          abil_mods->natural_armor = 40;
          break;
        case SIZE_LARGE:
          abil_mods->dexterity = -2;
          abil_mods->strength = 6;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 60;
          break;
      }
      break;
    case RACE_TYPE_PLANT:
      switch (race_list[race].size)
      {
        case SIZE_SMALL:
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 20;
          break;
        case SIZE_MEDIUM:
          abil_mods->strength = 2;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 20;
          break;
        case SIZE_LARGE:
          abil_mods->strength = 4;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 40;
          break;
        case SIZE_HUGE:
          abil_mods->strength = 8;
          abil_mods->dexterity = -2;
          abil_mods->constitution = 4;
          abil_mods->natural_armor = 60;
          break;
      }
      break;
    case RACE_TYPE_ELEMENTAL:
      switch (race) 
      {
        case RACE_SMALL_FIRE_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 2;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 20;
          break;
        case RACE_MEDIUM_FIRE_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 4;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 30;
          break;
        case RACE_LARGE_FIRE_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 4;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 40;
          break;
        case RACE_HUGE_FIRE_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 6;
          abil_mods->constitution = 4;
          abil_mods->natural_armor = 40;
          break;
        case RACE_SMALL_AIR_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 2;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 20;
          break;
        case RACE_MEDIUM_AIR_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 4;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 30;
          break;
        case RACE_LARGE_AIR_ELEMENTAL:
          abil_mods->strength = 2;
          abil_mods->dexterity = 4;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 40;
          break;
        case RACE_HUGE_AIR_ELEMENTAL:
          abil_mods->strength = 4;
          abil_mods->dexterity = 6;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 40;
          break;
        case RACE_SMALL_EARTH_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 2;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 40;
          break;
        case RACE_MEDIUM_EARTH_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 4;
          abil_mods->constitution = 0;
          abil_mods->natural_armor = 50;
          break;
        case RACE_LARGE_EARTH_ELEMENTAL:
          abil_mods->strength = 6;
          abil_mods->dexterity = -2;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 60;
          break;
        case RACE_HUGE_EARTH_ELEMENTAL:
          abil_mods->strength = 8;
          abil_mods->dexterity = -2;
          abil_mods->constitution = 4;
          abil_mods->natural_armor = 60;
          break;
        case RACE_SMALL_WATER_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 0;
          abil_mods->constitution = 2;
          abil_mods->natural_armor = 40;
          break;
        case RACE_MEDIUM_WATER_ELEMENTAL:
          abil_mods->strength = 0;
          abil_mods->dexterity = 0;
          abil_mods->constitution = 4;
          abil_mods->natural_armor = 50;
          break;
        case RACE_LARGE_WATER_ELEMENTAL:
          abil_mods->strength = 2;
          abil_mods->dexterity = -2;
          abil_mods->constitution = 6;
          abil_mods->natural_armor = 60;
          break;
        case RACE_HUGE_WATER_ELEMENTAL:
          abil_mods->strength = 4;
          abil_mods->dexterity = -2;
          abil_mods->constitution = 8;
          abil_mods->natural_armor = 60;
          break;
      }
      break;
    default:
      abil_mods->strength = race_list[race].ability_mods[0];
      abil_mods->constitution = race_list[race].ability_mods[1];
      abil_mods->dexterity = race_list[race].ability_mods[4];
      break;
  }
  return abil_mods;
}


ACMD(do_wildshape) {
    
    int i = 0;
    char buf[200];
    struct wild_shape_mods *abil_mods;

    skip_spaces(&argument);

  if (!HAS_FEAT(ch, FEAT_WILD_SHAPE)) {
      send_to_char(ch, "You do not have the ability to shapechange using wild shape.\r\n");
      return;
  }

  int druid = GET_CLASS_RANKS(ch, CLASS_DRUID);

    if (!*argument) {
        send_to_char(ch, "Please select a race to switch to or select 'return'.\r\n");
        for (i = 0; i < NUM_RACES; i++) {
            if (race_list[i].family != RACE_TYPE_ANIMAL && race_list[i].family != RACE_TYPE_ELEMENTAL &&
                race_list[i].family != RACE_TYPE_PLANT && race_list[i].family != RACE_TYPE_MAGICAL_BEAST)
                continue;
            if (race_list[i].family == RACE_TYPE_ELEMENTAL && ((druid < 6) ||
            (druid >= 6 && druid < 8 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_SMALL)) ||
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].family == RACE_TYPE_PLANT && ((druid < 8) || 
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].size > SIZE_HUGE || race_list[i].size < SIZE_DIMINUTIVE)
                continue;
            if (race_list[i].family == RACE_TYPE_ANIMAL) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 6)
                        continue;
                      break;
                    case SIZE_HUGE:
                    case SIZE_DIMINUTIVE:
                      if (druid < 8)
                        continue;
                      break;
                    default:
                        continue;
              }
            }
            if (race_list[i].family == RACE_TYPE_MAGICAL_BEAST) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 8)
                        continue;
                      break;
                    case SIZE_SMALL:
                    case SIZE_MEDIUM:
                      if (druid < 6)
                        continue;
                      break;
                    default:
                        continue;
              }
            }

            abil_mods = set_wild_shape_mods(i);

            send_to_char(ch, "%-40s Str [%s%-2d] Con [%s%-2d] Dex [%s%-2d] NatAC [%s%-2d]\r\n", race_list[i].name, 
                         abil_mods->strength >= 0 ? "+" : "", abil_mods->strength,
                         abil_mods->constitution >= 0 ? "+" : "", abil_mods->constitution,
                         abil_mods->dexterity >= 0 ? "+" : "", abil_mods->dexterity,
                         abil_mods->natural_armor >= 0 ? "+" : "", abil_mods->natural_armor
                        );
        }
        return;
    }

    if (strlen(argument) > 100) {
        send_to_char(ch, "The race name argument cannot be any longer than 100 characters.\r\n");
        return;
    }

    if (!strcmp(argument, "return")) {
        if (!AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
          send_to_char(ch, "You are not wild shaped.\r\n");
          return;
        }

        abil_mods = set_wild_shape_mods(GET_DISGUISE_RACE(ch));

        set_attributes(ch, ch->real_abils.str - abil_mods->strength,
                   ch->real_abils.con - abil_mods->constitution,
                   ch->real_abils.dex - abil_mods->dexterity,
                   ch->real_abils.intel,
                   ch->real_abils.wis,
                   ch->real_abils.cha);

        GET_ARMOR(ch) -= abil_mods->natural_armor;

        abil_mods = set_wild_shape_mods(GET_REAL_RACE(ch));


        set_attributes(ch, ch->real_abils.str + abil_mods->strength,
                   ch->real_abils.con + abil_mods->constitution,
                   ch->real_abils.dex + abil_mods->dexterity,
                   ch->real_abils.intel,
                   ch->real_abils.wis,
                   ch->real_abils.cha);

        sprintf(buf, "You change shape into a %s.", race_list[GET_REAL_RACE(ch)].name);
        act(buf, true, ch, 0, 0, TO_CHAR);
        sprintf(buf, "$n changes shape into a %s.", race_list[GET_REAL_RACE(ch)].name);
        act(buf, true, ch, 0, 0, TO_ROOM);
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);
        if (!GET_DISGUISE_DESC_1(ch))
          REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DISGUISED);
        GET_HIT(ch) += GET_LEVEL(ch);
        GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
        GET_DISGUISE_RACE(ch) = 0;
        affect_total(ch);
        return;
    }

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
    send_to_char(ch, "You must return to your normal shape before assuming a new form.\r\n");
    return;
  }

        for (i = 0; i < NUM_RACES; i++) {
            if (race_list[i].family != RACE_TYPE_ANIMAL && race_list[i].family != RACE_TYPE_ELEMENTAL &&
                race_list[i].family != RACE_TYPE_PLANT && race_list[i].family != RACE_TYPE_MAGICAL_BEAST)
                continue;
            if (race_list[i].family == RACE_TYPE_ELEMENTAL && ((druid < 6) ||
            (druid >= 6 && druid < 8 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_SMALL)) ||
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].family == RACE_TYPE_PLANT && ((druid < 8) || 
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].size > SIZE_HUGE || race_list[i].size < SIZE_DIMINUTIVE)
                continue;
            if (race_list[i].family == RACE_TYPE_ANIMAL) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 6)
                        continue;
                      break;
                    case SIZE_HUGE:
                    case SIZE_DIMINUTIVE:
                      if (druid < 8)
                        continue;
                      break;
                    default:
                        continue;
              }
            }
            if (race_list[i].family == RACE_TYPE_MAGICAL_BEAST) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 8)
                        continue;
                      break;
                    case SIZE_SMALL:
                    case SIZE_MEDIUM:
                      if (druid < 6)
                        continue;
                      break;
                    default:
                        continue;
              }
            }
          if (!strcmp(argument, race_list[i].name))
            break;
        }

    if (i >= NUM_RACES) {
        send_to_char(ch, "Please select a race to switch to or select 'return'.\r\n");
        for (i = 0; i < NUM_RACES; i++) {
            if (race_list[i].family != RACE_TYPE_ANIMAL && race_list[i].family != RACE_TYPE_ELEMENTAL &&
                race_list[i].family != RACE_TYPE_PLANT && race_list[i].family != RACE_TYPE_MAGICAL_BEAST)
                continue;
            if (race_list[i].family == RACE_TYPE_ELEMENTAL && ((druid < 6) ||
            (druid >= 6 && druid < 8 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_SMALL)) ||
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].family == RACE_TYPE_PLANT && ((druid < 8) || 
            (druid >= 8 && druid < 10 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_MEDIUM)) ||
            (druid >= 10 && druid < 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_LARGE)) ||
            (druid >= 12 && (race_list[i].size < SIZE_SMALL || race_list[i].size > SIZE_HUGE))))
                continue;
            if (race_list[i].size > SIZE_HUGE || race_list[i].size < SIZE_DIMINUTIVE)
                continue;
            if (race_list[i].family == RACE_TYPE_ANIMAL) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 6)
                        continue;
                      break;
                    case SIZE_HUGE:
                    case SIZE_DIMINUTIVE:
                      if (druid < 8)
                        continue;
                      break;
                    default:
                        continue;
              }
            }
            if (race_list[i].family == RACE_TYPE_MAGICAL_BEAST) {
              switch (race_list[i].size) {
                    case SIZE_LARGE:
                    case SIZE_TINY:
                      if (druid < 8)
                        continue;
                      break;
                    case SIZE_SMALL:
                    case SIZE_MEDIUM:
                      if (druid < 6)
                        continue;
                      break;
                    default:
                        continue;
              }
            }

            abil_mods = set_wild_shape_mods(i);

            send_to_char(ch, "%-40s Str [%s%-2d] Con [%s%-2d] Dex [%s%-2d] NatAC [%s%-2d]\r\n", race_list[i].name, 
                         abil_mods->strength >= 0 ? "+" : "", abil_mods->strength,
                         abil_mods->constitution >= 0 ? "+" : "", abil_mods->constitution,
                         abil_mods->dexterity >= 0 ? "+" : "", abil_mods->dexterity,
                         abil_mods->natural_armor >= 0 ? "+" : "", abil_mods->natural_armor
                        );
        }
        return;
    }

    if (druid < 20 && GET_INNATE(ch, SPELL_WILD_SHAPE) < 1) {
        if (is_innate_ready(ch, SPELL_WILD_SHAPE))
          GET_INNATE(ch, SPELL_WILD_SHAPE) = HAS_FEAT(ch, FEAT_WILD_SHAPE);
        else if (GET_ADMLEVEL(ch) == 0) {
    	  send_to_char(ch, "You have already used up all of your wild shape opportunities today.\r\n");
  	  return;
        }
    }

    GET_DISGUISE_RACE(ch) = i;

        abil_mods = set_wild_shape_mods(GET_DISGUISE_RACE(ch));

        set_attributes(ch, ch->real_abils.str + abil_mods->strength,
                   ch->real_abils.con + abil_mods->constitution,
                   ch->real_abils.dex + abil_mods->dexterity,
                   ch->real_abils.intel,
                   ch->real_abils.wis,
                   ch->real_abils.cha);

        GET_ARMOR(ch) += abil_mods->natural_armor;

        abil_mods = set_wild_shape_mods(GET_REAL_RACE(ch));


        set_attributes(ch, ch->real_abils.str - abil_mods->strength,
                   ch->real_abils.con - abil_mods->constitution,
                   ch->real_abils.dex - abil_mods->dexterity,
                   ch->real_abils.intel,
                   ch->real_abils.wis,
                   ch->real_abils.cha);

    sprintf(buf, "You change shape into a %s.", race_list[GET_DISGUISE_RACE(ch)].name);
    act(buf, true, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n changes shape into a %s.", race_list[GET_DISGUISE_RACE(ch)].name);
    act(buf, true, ch, 0, 0, TO_ROOM);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_DISGUISED);
    GET_HIT(ch) += GET_LEVEL(ch);
    GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));

    if (druid < 20) {
      GET_INNATE(ch, SPELL_WILD_SHAPE)--;

      if (is_innate_ready(ch, SPELL_WILD_SHAPE) && GET_INNATE(ch, SPELL_WILD_SHAPE) <= 0) {
        add_innate_timer(ch, SPELL_WILD_SHAPE);
      }
    }
    affect_total(ch);
}

ACMD(do_websiteaccount)
{

  if (IS_NPC(ch) || !ch->desc || !ch->desc->account) {
    send_to_char(ch, "Only connected player characters with valid accounts may assign a web site password to their game account.\r\n");
    return;
  }

  char arg[200];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You must supply the website password to assign to this game account.\r\n");
    return;
  }

  if (strlen(arg) > 200) {
    send_to_char(ch, "The name of the web site password must be 200 characters or less.\r\n");
    return;
  }

  ch->desc->account->web_password = strdup(arg);

  send_to_char(ch, "Your web site password is now %s.  You may view this at any time by typing 'account'.  You may change it at any time by using this command.\r\n", ch->desc->account->web_password);
  save_char(ch);
}

ACMD(do_wishlist) {

	unsigned short int i=0, type=0, specific=0, freeslot=FALSE;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "What would you like to add to your wish list? (wishlist <item>|list|show|erase)\r\n");
		return;
	}

	if (!strcmp(argument, "show")) {
		for (i = 0; i < 10; i++)
			if (ch->player_specials->wishlist[i][0] != 0)
				break;
		if (i == 10) {
			send_to_char(ch, "You have nothing on your wish list yet.\r\n");
			return;
		}

		send_to_char(ch, "Your Wish List:\r\n");
		for (i = 0; i < 10; i++) {
			if (ch->player_specials->wishlist[i][0] == 0)
				break;
			send_to_char(ch, "%d) %s\r\n", i + 1, ch->player_specials->wishlist[i][0] == 1 ? weapon_list[ch->player_specials->wishlist[i][1]].name : armor_list[ch->player_specials->wishlist[i][1]].name);
		}
		send_to_char(ch, "\r\n");
		return;
	}

	if (!strcmp(argument, "erase")) {
		for (i = 0; i < 10; i++) {
			ch->player_specials->wishlist[i][0] = 0;
			ch->player_specials->wishlist[i][1] = 0;
		}
		send_to_char(ch, "Your wish list has been erased.\r\n");
		return;
	}

	if (!strcmp(argument, "list")) {
		send_to_char(ch, "Weapons:\r\n");
		for (i = 0; i <= NUM_WEAPON_TYPES; i++) {
			send_to_char(ch, "%-30s  ", weapon_list[i].name);
			if (i % 3 == 2)
				send_to_char(ch, "\r\n");
		}
		if (i % 3 != 2)
			send_to_char(ch, "\r\n");
		send_to_char(ch, "\r\n");

		send_to_char(ch, "\r\nArmor and Shields:\r\n");
		for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++) {
			send_to_char(ch, "%-30s  ", armor_list[i].name);
			if (i % 3 == 2)
				send_to_char(ch, "\r\n");
		}
		if (i % 3 != 2)
			send_to_char(ch, "\r\n");
		send_to_char(ch, "\r\n");

		return;
	}

	for (i = 0; i <= NUM_WEAPON_TYPES; i++) {
		if (!strcmp(argument, weapon_list[i].name)) {
			type = 1;
			specific = i;
			break;
		}
	}

	if (type == 0) {
		for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++) {
			if (!strcmp(argument, armor_list[i].name)) {
				type = 2;
				specific = i;
				break;
			}
		}
    }

	if (type == 0) {
		send_to_char(ch, "There is no armor or weapon of that type.  Please select again, or type wishlist list to see what is available.\r\n");
		return;
	}

	for (i = 0; i < 10; i++) {
		if (ch->player_specials->wishlist[i][0] == 0) {
			freeslot = TRUE;
			ch->player_specials->wishlist[i][0] = type;
			ch->player_specials->wishlist[i][1] = specific;
			break;
		}
	}

	if (freeslot)
		send_to_char(ch, "You have added %s to your wish list.\r\n", type == 1 ? weapon_list[specific].name : armor_list[specific].name);
	else
		send_to_char(ch, "Your wish list is full, please erase it and re-create it.\r\n");

	return;
}

ACMD(do_combatcmd)
{

  if (*argument) {
    if (ch->combat_cmd) {
      free(ch->combat_cmd);
      send_to_char(ch, "Your combat command has been cleared.\r\n");
      return;
    }
    else {
      send_to_char(ch, "What would you like to queue as your next combat command.\r\n");
      return;
    }
  }

  skip_spaces(&argument);

  if (ch->combat_cmd)
    free(ch->combat_cmd);

  ch->combat_cmd = strdup(argument);

  send_to_char(ch, "You have queued the command '%s' as the next action you will take when your turn comes up in combat.\r\n", ch->combat_cmd);
}

ACMD(do_poll)
{

  if (!ch->desc || !ch->desc->account) {
    send_to_char(ch, "You cannot access polls at this time.\r\n");
    return;
  }

  char arg[200], arg2[200];
  int i = 0, j = 0;
  char *title = NULL;
  char *title2 = NULL;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "The following polls are available:\r\n");
    for (i = 0; i < NUM_POLLS; i++) {
      if (poll_list[i].active) {
        title = strdup(poll_list[i].title);
        title2 = strtok(title, "?");
        send_to_char(ch, "%s%d) %s?\r\n@n", ch->desc->account->polls[i] > 0 ? "@Y" : "", i, title2);
      }
    }
    send_to_char(ch, "\r\nTo view a poll's options and/or results, type @Ypoll <poll number>@n  (yellow means you've already voted).\r\n");
    return;
  }

  i = atoi(arg);

  if (i < 1 || i >= NUM_POLLS) {
    send_to_char(ch, "That is not an eligible poll.\r\n");
    return;
  }

  if (poll_list[i].active == FALSE) {
    send_to_char(ch, "That poll is not active.\r\n");
    return;
  }

  int tot_votes = 0;

  for (j = 1; j < 20; j++) {
    tot_votes += poll_list[i].votes[j];
  }


  if (!*arg2) {
    send_to_char(ch, "Below is the information for the poll requested:\r\n");
    send_to_char(ch, "\r\n%s\r\n\r\n", poll_list[i].title);
    for (j = 1; j < 20; j++) {
      if (poll_list[i].options[j]) {
        send_to_char(ch, "%d) %d %d%% %s\r\n", j, poll_list[i].votes[j], poll_list[i].votes[j] * 100 / MAX(1, tot_votes), poll_list[i].options[j]);
      }
    }
    send_to_char(ch, "\r\nTo vote please type @Ypoll %d <option number>@n.\r\n", i);
    return;
  }

  if (poll_list[i].revote == FALSE && ch->desc->account->polls[i] > 0) {
    send_to_char(ch, "You have already voted on that poll.\r\n");
    return;
  }
  
  j = atoi(arg2);

  if (j < 1 || j > 19 || poll_list[i].options[j] == NULL) {
    send_to_char(ch, "That is not a valid poll option.\r\n");
    return;
  }

  extern MYSQL *conn;
  // Open mysql connection
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
    log("Cannot connect to mysql database in polls.");
  }


  char query[300];

  if (ch->desc->account->polls[i] > 0) {
    ch->desc->account->polls[i] = j;
    save_account(ch->desc->account);

    if (CONFIG_DFLT_PORT == 9080) 
    {
      sprintf(query, "UPDATE poll_data SET option = '%d', date = NOW() WHERE name = '%s' AND poll_num = '%d'", j, ch->desc->account->name, i);
      if (mysql_query(conn, query)) 
      {
         log("Cannot update poll vote.");
      }
    }
  }
  else {
    ch->desc->account->polls[i] = j;
    poll_list[i].votes[j]++;
    save_account(ch->desc->account);

    if (CONFIG_DFLT_PORT == 9080) 
    {
      sprintf(query, "INSERT INTO 'poll_data' ('name', 'poll_num', 'option', 'date') VALUES ('%s', '%d', '%d', NOW())", ch->desc->account->name, i, j);
      if (mysql_query(conn, query)) 
      {
         log("Cannot insert poll vote.");
      }
    }
  }  

  send_to_char(ch, "@Y You cast your vote for option %d!@n\r\n", j);
  send_to_char(ch, "\r\n");
  do_poll(ch, arg, 0, 0);

  mysql_close(conn);

}

ACMD(do_callmount)
{

  int mob_num = ch->player_specials->mount;
  char buf2[400];

  if (HAS_REAL_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST)) {
    if (IS_EVIL(ch))
      mob_num = PET_ADULT_RED_DRAGON;
    else if (IS_GOOD(ch))
      mob_num = PET_ADULT_SILVER_DRAGON;
    else
      mob_num = PET_ADULT_COPPER_DRAGON;
  }

  if (mob_num <= 0 || mob_num >= NUM_PETS) {
    if (GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 5) {
      mob_num = PET_HEAVY_WAR_HORSE;
    } else {
      send_to_char(ch, "You do not have a mount.  Go see Caris just south of the Palanthas Gate to buy a mount.\r\n");
      return;
    }
  }

  if (!is_innate_ready(ch, ABILITY_CALL_MOUNT)) {
    send_to_char(ch, "You cannot call your mount again until 5 minutes have passed from when you last called it.\r\n");
    return;
  }

  int i = 0;

  int add_hit = 0;
  int add_ac = 0;

  add_hit += GET_CLASS_RANKS(ch, CLASS_PALADIN) > 4 ? ((GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2) / 3 * 2)  : 0;
  add_ac += GET_CLASS_RANKS(ch, CLASS_PALADIN) > 4 ? 2 + ((GET_CLASS_RANKS(ch, CLASS_PALADIN) - 2) / 3 * 2)  : 0;

  if (mob_num == PET_ADULT_RED_DRAGON || mob_num == PET_ADULT_SILVER_DRAGON) {
    add_hit += HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST) * 18;
    add_ac += HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST) * 30;
  }

  // assign the pet stats to the character's summon vars
  ch->player_specials->mount_num = mob_num;
  ch->player_specials->mount_desc = strdup(pet_list[mob_num].desc);
  ch->player_specials->mount_max_hit = pet_list[mob_num].max_hit + (add_hit * 8);
  ch->player_specials->mount_cur_hit = ch->player_specials->mount_max_hit;
  ch->player_specials->mount_ac = MAX(150 + (pet_list[mob_num].level *10 / 2), pet_list[mob_num].level * 15) + (add_ac * 10);
  ch->player_specials->mount_dr = pet_list[mob_num].dr;
  for (i = 0; i < 5; i++) {
      ch->player_specials->mount_attack_to_hit[i] = pet_list[mob_num].attacks_to_hit[i];
      ch->player_specials->mount_attack_ndice[i] = pet_list[mob_num].attacks_ndice[i];
      ch->player_specials->mount_attack_sdice[i] = pet_list[mob_num].attacks_sdice[i];
      ch->player_specials->mount_attack_dammod[i] = pet_list[mob_num].attacks_dammod[i];
  }

  sprintf(buf2, "You call forth %s!", ch->player_specials->mount_desc);
  act(buf2, true, ch, 0, 0, TO_CHAR);
  sprintf(buf2, "$n calls forth %s!", ch->player_specials->mount_desc);
  act(buf2, true, ch, 0, 0, TO_ROOM);

  add_innate_timer(ch, ABILITY_CALL_MOUNT);

}

ACMD(do_callcompanion)
{

  int mob_num = ch->player_specials->companion_num;
  char buf2[400];

  if (mob_num <= 0 || mob_num >= NUM_PETS) {
      send_to_char(ch, "You do not have an animal companion.  Go see Caris just south of the Palanthas Gate to get a companion.\r\n");
    return;
  }

  if (!is_innate_ready(ch, ABILITY_CALL_COMPANION)) {
    send_to_char(ch, "You cannot call your companion again until 5 minutes have passed from when you last called it.\r\n");
    return;
  }

  int i = 0;

  int add_hit = 0;
  int add_ac = 0;

  add_hit += MAX(0, ((MAX(0, GET_CLASS_RANKS(ch, CLASS_RANGER) - 4)) + GET_CLASS_RANKS(ch, CLASS_DRUID)) - pet_list[mob_num].level);
  add_ac += MAX(0, ((MAX(0, GET_CLASS_RANKS(ch, CLASS_RANGER) - 4)) + GET_CLASS_RANKS(ch, CLASS_DRUID)) - pet_list[mob_num].level);

  // assign the pet stats to the character's animal companion vars
  ch->player_specials->companion_num = mob_num;
  ch->player_specials->companion_desc = strdup(pet_list[mob_num].desc);
  ch->player_specials->companion_max_hit = pet_list[mob_num].max_hit + (add_hit * 10);
  ch->player_specials->companion_cur_hit = ch->player_specials->companion_max_hit;
  ch->player_specials->companion_ac = MAX(150 + (pet_list[mob_num].level * 10 / 2), pet_list[mob_num].level * 15) + (add_ac * 10);
  ch->player_specials->companion_dr = pet_list[mob_num].dr;
  for (i = 0; i < 5; i++) {
      ch->player_specials->companion_attack_to_hit[i] = pet_list[mob_num].attacks_to_hit[i];
      ch->player_specials->companion_attack_ndice[i] = pet_list[mob_num].attacks_ndice[i];
      ch->player_specials->companion_attack_sdice[i] = pet_list[mob_num].attacks_sdice[i];
      ch->player_specials->companion_attack_dammod[i] = pet_list[mob_num].attacks_dammod[i];
  }

  sprintf(buf2, "You call forth %s!", ch->player_specials->companion_desc);
  act(buf2, true, ch, 0, 0, TO_CHAR);
  sprintf(buf2, "$n calls forth %s!", ch->player_specials->companion_desc);
  act(buf2, true, ch, 0, 0, TO_ROOM);

  add_innate_timer(ch, ABILITY_CALL_COMPANION);

}

ACMD(do_accexp)
{

    char arg[200], arg2[200];
    int i = 0, j = 0;

    two_arguments(argument, arg, arg2);

    if (!*arg) 
    {
        send_to_char(ch, "Would you like to spend account exp on an advanced @Yrace@n or a prestige @Yclass@n?\r\n");
        return;
    }

    if (is_abbrev(arg, "race")) 
    {
        if (!*arg2) 
        {
            send_to_char(ch, "Please choose from the following races:\r\n");
            for (i = 0; i < NUM_RACES; i++) 
            {
                if (race_list[i].is_pc == FALSE || race_list[i].level_adjustment == 0 || has_unlocked_race(ch, i))
                    continue;
                send_to_char(ch, "%s (%d account experience)\r\n", race_list[i].name, race_list[i].level_adjustment * 5000);
            }
            return;
        }
        for (i = 0; i < NUM_RACES; i++) 
        {
            if (race_list[i].is_pc == FALSE || race_list[i].level_adjustment == 0 || has_unlocked_race(ch, i))
                continue;
            if (is_abbrev(arg2, race_list[i].name))
                break;
        }

        if (i >= NUM_RACES) 
        {
            send_to_char(ch, "Either that race does not exist, is not an advanced race, is not available for players, or you've already unlocked it.\r\n");
            return;
        }
        if (ch->desc && ch->desc->account) 
        {
            for (j = 0; j < MAX_UNLOCKED_RACES; j++) 
            {
                if (ch->desc->account->races[j] == 0)
                    break;
            }
            if (j >= MAX_UNLOCKED_RACES) 
            {
                send_to_char(ch, "All of your advanced race slots are filled.  Please submit a petition to ask for the limit to be increased.\r\n");
                return;
            }
            if (ch->desc->account->experience >= (race_list[i].level_adjustment * 5000)) 
            {
                ch->desc->account->experience -= race_list[i].level_adjustment * 5000;
                ch->desc->account->races[j] = i;
                save_account(ch->desc->account);
                send_to_char(ch, "You have unlocked the advanced race '%s' for all character and future characters on your account!.\r\n", race_list[i].name);
                return;
            } 
            else 
            {
                send_to_char(ch, "You need %d account experience to purchase that advanced race and you only have %d.\r\n", race_list[i].level_adjustment * 5000,
                    ch->desc->account->experience);
                return;
            }
        }
        else 
        {
            send_to_char(ch, "There is a problem with your account and the race could not be unlocked.  Please submit a petition to staff.\r\n");
            return;
        }
    }
    else if (is_abbrev(arg, "class")) 
    {
        if (!*arg2) 
        {
            send_to_char(ch, "Please choose from the following classes:\r\n");
            for (i = 0; i < NUM_CLASSES; i++) 
            {
                if (!class_in_game_core[i] || has_unlocked_class(ch, i) || !prestige_classes_core[i])
                    continue;
                send_to_char(ch, "%s (5000 account experience)\r\n", class_names_core[i]);
            }
            return;
        }
        for (i = 0; i < NUM_CLASSES; i++) 
        {
            if (!class_in_game_core[i] || has_unlocked_class(ch, i) || !prestige_classes_core[i])
                continue;
            if (is_abbrev(arg2, class_names_core[i]))
                break;
        }

        if (i >= NUM_CLASSES) 
        {
            send_to_char(ch, "Either that class does not exist, is not a prestige class, is not available for players, or you've already unlocked it.\r\n");
            return;
        }
        if (ch->desc && ch->desc->account) 
        {
            for (j = 0; j < MAX_UNLOCKED_CLASSES; j++) 
            {
                if (ch->desc->account->classes[j] == 999)
                    break;
            }
            if (j >= MAX_UNLOCKED_CLASSES) 
            {
                send_to_char(ch, "All of your prestige class slots are filled.  Please submit a petition to ask for the limit to be increased.\r\n");
                return;
            }
            if (ch->desc->account->experience >= (5000)) 
            {
                ch->desc->account->experience -= 5000;
                ch->desc->account->classes[j] = i;
                save_account(ch->desc->account);
                send_to_char(ch, "You have unlocked the prestige class '%s' for all character and future characters on your account!.\r\n", class_names_core[i]);
                return;
            } 
            else 
            {
                send_to_char(ch, "You need 5000 account experience to purchase that prestige class and you only have %d.\r\n",
                    ch->desc->account->experience);
                return;
            }
        }
        else 
        {
            send_to_char(ch, "There is a problem with your account and the class could not be unlocked.  Please submit a petition to staff.\r\n");
            return;
        }
    }
    else 
    {
        send_to_char(ch, "You must choose to unlock either a race or a class.\r\n");
        return;
    }

}

ACMD(do_rerollheightandweight)
{

  set_height_and_weight_by_race(ch);

  send_to_char(ch, "Your new height is %d'%d\" and your new weight is %dlbs.\r\n",
               GET_HEIGHT(ch) / 30 , (GET_HEIGHT(ch) % 30) / 5 * 2, GET_WEIGHT(ch) * 22 / 10);

}

ACMD(do_setheight)
{

  char arg[200];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please type setheight <new height in cm>. There are 2.5 cm in each inch.\r\n");
    send_to_char(ch, "Height for your race must be between %d and %d cm.\r\n",
                 (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 9 / 10), (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 11 / 10));
    return;
  }

  int height = atoi(arg);

  if (height < (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 9 / 10) || height > (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 11 / 10)) {
    send_to_char(ch, "Height for your race must be between %d and %d cm. You entered %d cm which is %d'%d\"\r\n", 
                 (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 9 / 10), (race_list[GET_RACE(ch)].height[GET_SEX(ch)] * 11 / 10),
                 height, height / 30, (height % 30) / 5 * 2);
    return;
  }

  GET_HEIGHT(ch) = height;

  send_to_char(ch, "Your new height is %d cm or %d'%d\".\r\n", GET_HEIGHT(ch), GET_HEIGHT(ch) / 30, (GET_HEIGHT(ch) % 30) / 5 * 2);

}

ACMD(do_setweight)
{

  char arg[200];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please type setweight <new weight in kg>. There are 2.2 lbs in each kg.\r\n");
    send_to_char(ch, "Weight for your race must be between %d and %d kg.\r\n",
                 (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 7 / 10), (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 13 / 10));
    return;
  }

  int weight = atoi(arg);

  if (weight < (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 7 / 10) || weight > (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 13 / 10)) {
    send_to_char(ch, "Weight for your race must be between %d and %d kg.  You entered %d kg which is %d lbs.\r\n", 
                 (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 7 / 10), (race_list[GET_RACE(ch)].weight[GET_SEX(ch)] * 13 / 10),
                 weight, weight * 22 / 10);
    return;
  }

  GET_WEIGHT(ch) = weight;

  send_to_char(ch, "Your new weight is %d kg or %dlbs.\r\n", GET_WEIGHT(ch), GET_WEIGHT(ch) * 22 / 10);

}

ACMD(do_mentor)
{
  char arg[100];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please select a level between 1 and %d for which to mentor.  Or type '@Ymentor off@n to turn off mentoring.\r\n", GET_CLASS_LEVEL(ch) - 1);
    return;
  }

  if (FIGHTING(ch)) {
    send_to_char(ch, "You cannot adjust your mentor level while fighting.\r\n");
    return;
  }

  if (zone_table[world[IN_ROOM(ch)].zone].zone_status == 3) {
    int min_j = 60;
    int max_j = 0;
    int j = 0;
    for (j = 1; j < NUM_LEVEL_RANGES; j++) {
      if (IS_SET(zone_table[world[IN_ROOM(ch)].zone].level_range, (1 << j))) {
        if (j < min_j)
          min_j = j;
        if (j > max_j)
          max_j = j;
      }
    }
    if (min_j == 60 || max_j == 0) {  
      send_to_char(ch, "That dungeon has not had its min or max level set, and thus cannot be entered.\r\n");
      return;
    }
    if (is_abbrev(arg, "off")) {
      if ((GET_CLASS_LEVEL(ch) < ((min_j * 2) - 1) || GET_CLASS_LEVEL(ch) > (max_j * 2))) {
        send_to_char(ch, "Your class level must be between %d and %d to turn off mentor within the dungeon.\r\n", (min_j * 2) - 1, max_j * 2);
        return;    
      }
    }
    else if (atoi(arg) < ((min_j * 2) - 1) || atoi(arg) > (max_j * 2)) {
      send_to_char(ch, "Your mentor level must be between %d and %d within this dungeon.\r\n", (min_j * 2) - 1, max_j * 2);
      return;
    }
  }

  if (is_abbrev(arg, "off")) {
    ch->mentor_level = 0;
    send_to_char(ch, "You are no longer mentoring.\r\n");
    return;
  }

  int char_lev = atoi(arg);

  if (char_lev <= 0 || char_lev >= GET_CLASS_LEVEL(ch)) {
    send_to_char(ch, "Please select a level between 1 and %d for which to mentor.  Or type '@Ymentor off@n to turn off mentoring.\r\n", GET_CLASS_LEVEL(ch) - 1);
    return;
  }

  ch->mentor_level = char_lev;

  send_to_char(ch, "You are now mentoring at level %d.  Your armor class, hit bonus and damage will be commensurately reduced.\r\n", ch->mentor_level);

}
int get_innate_timer(struct char_data *ch, int spellnum)
{
  struct innate_node *inn, *next_inn;

  for(inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if(inn->spellnum == spellnum)
      return inn->timer;
  }
  return 0;
}

ACMD(do_set_stats)
{

  if (ch->real_abils.str + ch->real_abils.intel + ch->real_abils.wis + ch->real_abils.con + ch->real_abils.dex + ch->real_abils.cha == 60 && ch->stat_points_given == FALSE) {
    GET_STAT_POINTS(ch) = 20;
    ch->stat_points_given = TRUE;
  }

  char arg1[100];
  char arg2[100];

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Which stat do you want to adjust?\r\n");
    return;
  }
  if (!*arg2 && !is_abbrev("show", arg1) && !is_abbrev("reset", arg1)) {
    send_to_char(ch, "What do you wish to change the stat to?\r\n");
    return;
  }

  if (is_abbrev(arg1, "reset")) {
    if (ch->setstats_not_avail == TRUE) {
      send_to_char(ch, "You cannot reset your stats unless you are in the process of respeccing your character.  (HELP RESPEC)\r\n");
      return;
    }
    send_to_char(ch, "You reset your stats back to the default values.\r\n");
    GET_STAT_POINTS(ch) = 20;
    ch->real_abils.str = 10;
    ch->real_abils.dex = 10;
    ch->real_abils.con = 10;
    ch->real_abils.intel = 10;
    ch->real_abils.wis = 10;
    ch->real_abils.cha = 10;
    return;
  }
  else if (is_abbrev(arg1, "show")) {
    send_to_char(ch, "Here are your current stats:\r\n");
    send_to_char(ch, "Strength     : %d\r\n", ch->real_abils.str);
    send_to_char(ch, "Dexterity    : %d\r\n", ch->real_abils.dex);
    send_to_char(ch, "Constitution : %d\r\n", ch->real_abils.con);
    send_to_char(ch, "Intelligence : %d\r\n", ch->real_abils.intel);
    send_to_char(ch, "Wisdom       : %d\r\n", ch->real_abils.wis);
    send_to_char(ch, "Charisma     : %d\r\n", ch->real_abils.cha);
    send_to_char(ch, "Stat Points  : %d\r\n", GET_STAT_POINTS(ch));
    return;
  }
  if (GET_STAT_POINTS(ch) == 0) {
    send_to_char(ch, "You need to type @Ysetstats reset@n if you want to change your stats after using all of your points.\r\n");
    return;
  }
  if (is_abbrev(arg1, "strength")) {
    ch->real_abils.str = stat_assign_stat(ch->real_abils.str, arg2, ch);
  }
  else if (is_abbrev(arg1, "dexterity")) {
    ch->real_abils.dex = stat_assign_stat(ch->real_abils.dex, arg2, ch);
  }
  else if (is_abbrev(arg1, "constitution")) {
    ch->real_abils.con = stat_assign_stat(ch->real_abils.con, arg2, ch);
  }
  else if (is_abbrev(arg1, "intelligence")) {
    ch->real_abils.intel = stat_assign_stat(ch->real_abils.intel, arg2, ch);
  }
  else if (is_abbrev(arg1, "wisdom")) {
    ch->real_abils.wis = stat_assign_stat(ch->real_abils.wis, arg2, ch);
  }
  else if (is_abbrev(arg1, "charisma")) {
    ch->real_abils.cha = stat_assign_stat(ch->real_abils.cha, arg2, ch);
  }
  else {
    send_to_char(ch, "That is not a valid ability score.\r\n");
  }

  send_to_char(ch, "Strength     : %d\r\n", ch->real_abils.str);
  send_to_char(ch, "Dexterity    : %d\r\n", ch->real_abils.dex);
  send_to_char(ch, "Constitution : %d\r\n", ch->real_abils.con);
  send_to_char(ch, "Intelligence : %d\r\n", ch->real_abils.intel);
  send_to_char(ch, "Wisdom       : %d\r\n", ch->real_abils.wis);
  send_to_char(ch, "Charisma     : %d\r\n", ch->real_abils.cha);
  send_to_char(ch, "Stat Points  : %d\r\n", GET_STAT_POINTS(ch));


  return;

}
