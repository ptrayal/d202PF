/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "dg_scripts.h"
#include "feats.h"
#include "oasis.h"
#include "quest.h"
#include "pets.h"

/* local vars */

int extractions_pending = 0;
int obj_bit[AF_ARRAY_MAX] = {0, 0, 0, 0};


/* external vars */
extern struct char_data *combat_list;
extern struct armor_table armor_list[];
extern int no_affect_eq[NUM_NO_AFFECT_EQ];

/* local functions */
void stop_guard(struct char_data *ch);
int highest_armor_type(struct char_data *ch);
int apply_ac(struct char_data *ch, int eq_pos);
void update_object(struct obj_data *obj, int use);
void update_char_objects(struct char_data *ch);
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);
void set_max_affect(struct char_data *ch, int loc, int mod, int spec, bool add);
int should_remove_stat(struct char_data *ch, int loc, int mod, int spec);
int stacked_effect(int type);

/* external functions */
int calculate_max_hit(struct char_data *ch);
void die(struct char_data *ch, struct char_data * killer);
void remove_follower(struct char_data *ch);
void clearMemory(struct char_data *ch);
ACMD(do_return);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where);
void perform_remove(struct char_data * ch, int pos);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
SPECIAL(shop_keeper);
void Crash_rentsave(struct char_data *ch, int cost);
void update_encumberance(struct char_data *ch);

char *fname(const char *namelist)
{
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* Stock isname().  Leave this here even if you put in a newer  *
 * isname().  Used for OasisOLC.                                */
int is_name(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  if (!*str || !*namelist || !str || !namelist)
    return (0);

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    /* skip to next name */
   for (; isalpha(*curname); curname++);
     if (!*curname)
       return (0);
    curname++;                  /* first char of new name */
  }
}

/* allow abbreviations */
#define WHITESPACE " ,\t"
int isname(const char *str, const char *namelist)
{
  char *newlist;
  char *strlist;
  char *curtok;
  char temp[200]={'\0'};
  char *toks[10];
  int i = 0, j = 0, k = 0;
  int found[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int success = FALSE;

   if (!str || !*str || !namelist || !*namelist)
     return 0;

  if (!strcmp(str, namelist)) /* the easy way */
     return 1;

  newlist = strip_color((char *)namelist); // make a copy since strtok 'modifies' strings

  strlist = strip_color((char *)str);

//    newlist = strdup(namelist);
//    strlist = strdup(str);

  for(curtok = strtok(strlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE)) {
    if (curtok && i < 10) {
      toks[i] = strdup(curtok);
      i++;
    }
  }

  for(curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE)) {
    if (k >= 10)
      continue;

    for (j = 0; j < i; j++) {
      if (curtok && toks[j]) {
        snprintf(temp, sizeof(temp), "%s", toks[j]);
        if (is_abbrev(temp, curtok)) {
          found[k] = TRUE;
          break;
        }
      }
    }
    k++;
  }

  for (j = 0; j < k; j++) {
    if (found[j] == TRUE)
      success++;
  }

  free(newlist);
  free(strlist);
  for (j = 0; j < i; j++) {
    free(toks[j]);
  }
  
  if (success >= i) {
    return 1;
  }  
  return 0;
}

void aff_apply_modify(struct char_data *ch, int loc, int mod, int spec, char *msg, bool add)
{
  struct damreduct_type *ptr, *reduct, *temp;
  int q = 0;
  
  if (mod == 0)
    return;

  switch (loc) {
  case APPLY_NONE:
    break;

  case APPLY_CARRY_WEIGHT:
    GET_CARRY_STR_MOD(ch) += mod;
    break;

  case APPLY_STR:
    GET_STR(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON(ch) += mod;
    if (!ch->free)
      GET_MAX_HIT(ch) = calculate_max_hit(ch);
    break;
  case APPLY_CHA:
    GET_CHA(ch) += mod;
    break;

  case APPLY_CLASS:
    /* ??? GET_CLASS(ch) += mod; */
    break;

  /*
   * My personal thoughts on these two would be to set the person to the
   * value of the apply.  That way you won't have to worry about people
   * making +1 level things to be imp (you restrict anything that gives
   * immortal level of course).  It also makes more sense to set someone
   * to a class rather than adding to the class number. -gg
   */

  case APPLY_LEVEL:
    /* ??? GET_LEVEL(ch) += mod; */
    break;

  case APPLY_AGE:
    ch->time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MANA:
    GET_MAX_MANA(ch) += mod;
    break;

  case APPLY_HIT:
    break;

  case APPLY_MOVE:
    GET_MAX_MOVE(ch) += mod;
    break;

  case APPLY_KI:
    GET_MAX_KI(ch) += mod;
    break;

  case APPLY_GOLD:
    break;

  case APPLY_EXP:
    break;

  case APPLY_AC_DEFLECTION:
  case APPLY_AC_DODGE:
  case APPLY_AC_NATURAL:
  case APPLY_AC_ARMOR:
  case APPLY_AC_SHIELD:
    GET_ARMOR(ch) += mod;
    break;

  case APPLY_HITROLL:
    GET_ACCURACY_MOD(ch) += mod;
    break;

  case APPLY_ACCURACY:
  case APPLY_DAMAGE:
    break;

  case APPLY_DAMROLL:
    GET_DAMAGE_MOD(ch) += mod;
    break;

  case APPLY_RACE:
    /* ??? GET_RACE(ch) += mod; */
    break;

  case APPLY_TURN_LEVEL:
    GET_TLEVEL(ch) += mod;
    break;

  case APPLY_SPELL_LVL_0:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_0) += mod;
    break;

  case APPLY_SPELL_LVL_1:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_1) += mod;
    break;

  case APPLY_SPELL_LVL_2:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_2) += mod;
    break;

  case APPLY_SPELL_LVL_3:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_3) += mod;
    break;

  case APPLY_SPELL_LVL_4:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_4) += mod;
    break;

  case APPLY_SPELL_LVL_5:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_5) += mod;
    break;

  case APPLY_SPELL_LVL_6:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_6) += mod;
    break;

  case APPLY_SPELL_LVL_7:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_7) += mod;
    break;

  case APPLY_SPELL_LVL_8:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_8) += mod;
    break;

  case APPLY_SPELL_LVL_9:
    if(!IS_NPC(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_9) += mod;
    break;

  case APPLY_FORTITUDE:
    GET_SAVE_MOD(ch, SAVING_FORTITUDE) += mod;
    break;

  case APPLY_REFLEX:
    GET_SAVE_MOD(ch, SAVING_REFLEX) += mod;
    break;

  case APPLY_WILL:
    GET_SAVE_MOD(ch, SAVING_WILL) += mod;
    break;

  case APPLY_SKILL:
//    SET_SKILL_BONUS(ch, spec, GET_SKILL_BONUS(ch, spec) + mod);
    break;

  case APPLY_FEAT:

      if (spec == FEAT_DAMAGE_REDUCTION) 
      {
        if (add) 
        {
          for (reduct = ch->damreduct; reduct; reduct = reduct->next) 
          {
            if (reduct->feat == FEAT_DAMAGE_REDUCTION) 
            {
              REMOVE_FROM_LIST(reduct, ch->damreduct, next);
            }
          }
          CREATE(ptr, struct damreduct_type, 1);
          ptr->next = ch->damreduct;
          ch->damreduct = ptr;
          ptr->spell = 0;
          ptr->feat = FEAT_DAMAGE_REDUCTION;
          ptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) + MIN(6, mod);
          ptr->duration = -1;
          ptr->max_damage = -1;
          for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
            ptr->damstyle[q] = ptr->damstyleval[q] = 0;
          ptr->damstyle[0] = DR_NONE;
        } 
        else 
        {
          for (reduct = ch->damreduct; reduct; reduct = reduct->next) 
          {
            if (reduct->feat == FEAT_DAMAGE_REDUCTION) 
            {
              REMOVE_FROM_LIST(reduct, ch->damreduct, next);
            }
          }
          if (HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) + mod > 0)
          {
            CREATE(ptr, struct damreduct_type, 1);
            ptr->next = ch->damreduct;
            ch->damreduct = ptr;
            ptr->spell = 0;
            ptr->feat = FEAT_DAMAGE_REDUCTION;
            ptr->mod = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) + mod;
            ptr->duration = -1;
            ptr->max_damage = -1;
            for (q = 0; q < MAX_DAMREDUCT_MULTI; q++)
              ptr->damstyle[q] = ptr->damstyleval[q] = 0;
            ptr->damstyle[0] = DR_NONE;
          }
        }
      }


    break;

  case APPLY_ALLSAVES:
  case APPLY_RESISTANCE:
    GET_SAVE_MOD(ch, SAVING_FORTITUDE) += mod;
    GET_SAVE_MOD(ch, SAVING_REFLEX) += mod;
    GET_SAVE_MOD(ch, SAVING_WILL) += mod;
    break;

  case APPLY_ABILITY:
    break;

  default:
    log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
    break;

  } /* switch */

}


void affect_modify(struct char_data * ch, int loc, int mod, int spec, long bitv, bool add)
{
  if (add) {
    SET_BIT_AR(AFF_FLAGS(ch), bitv);
  } else {
    REMOVE_BIT_AR(AFF_FLAGS(ch), bitv);
    mod = -mod;
  } 

  aff_apply_modify(ch, loc, mod, spec, "affect_modify", add);
}


void affect_modify_ar(struct char_data * ch, int loc, int mod, int spec, int bitv[], bool add)
{
  int i , j;

  if (add) {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j)) {
          SET_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
        }
  } else {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j))
          REMOVE_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
    mod = -mod;
  }

//  set_max_affect(ch, loc, mod, spec, add);

  aff_apply_modify(ch, loc, mod, spec, "affect_modify_ar", add);
}

int calculate_best_mod(struct char_data *ch, int location, int specific, int except_eq, int except_spell)
{
  struct affected_type *af = NULL;
  int noeffect = FALSE;
  int i = 0, k = 0, j = 0;
  int best = 0;
  int modifier = 0;

  if (location == APPLY_AC_DODGE || location == APPLY_NONE)
    return 0;
  
  for (af = ch->affected; af; af = af->next) {
    if (stacked_effect(af->type))
      continue;

    modifier = af->modifier;

    if (af->type == except_spell)
      continue;
    if (af->location == location && modifier > best)
      if (af->specific == specific)
        best = modifier;
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      noeffect = FALSE;
      for (k = 0; k < NUM_NO_AFFECT_EQ; k++) 
        if (OBJWEAR_FLAGGED(GET_EQ(ch, i), no_affect_eq[k]))
          noeffect = TRUE;
      if (noeffect)
        continue;
      if (i == except_eq)
        continue; 
      
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        modifier = GET_EQ(ch, i)->affected[j].modifier;
        if (GET_EQ(ch, i)->affected[j].location == location && modifier > best)
          if (GET_EQ(ch, i)->affected[j].specific == specific)
            best = modifier;
      }  
    }
  }


  return best;
}

int calculate_second_best_mod(struct char_data *ch, int location, int specific, int except_eq, int except_spell)
{
  struct affected_type *af;
  int noeffect = FALSE;
  int i, k, j;
  int best = 0;
  int secondbest = 0;
  int modifier = 0;

  if (location == APPLY_AC_DODGE || location == APPLY_NONE)
    return 0;
  
  for (af = ch->affected; af; af = af->next) {
    if (stacked_effect(af->type))
      continue;

    modifier = af->modifier;

    if (af->type == except_spell)
      continue;
    if (af->location == location && modifier > best && af->specific == specific) {
      secondbest = best;
      best = modifier;
    }
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      noeffect = FALSE;
      for (k = 0; k < NUM_NO_AFFECT_EQ; k++) 
        if (OBJWEAR_FLAGGED(GET_EQ(ch, i), no_affect_eq[k]))
          noeffect = TRUE;
      if (noeffect)
        continue;
      if (i == except_eq)
        continue; 
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        modifier = GET_EQ(ch, i)->affected[j].modifier;
        if (GET_EQ(ch, i)->affected[j].location == location && modifier > best &&
            GET_EQ(ch, i)->affected[j].specific == specific)
          secondbest = best;
          best = modifier;
      }  
    }
  }

  return secondbest;
}


/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data *ch)
{
  struct affected_type *af;
  int i, j=0, k, noeffect = FALSE;
  int a, b;
  int modifier = 0;

  GET_SPELLFAIL(ch) = GET_ARMORCHECK(ch) = GET_ARMORCHECKALL(ch) = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      noeffect = FALSE;
      for (k = 0; k < NUM_NO_AFFECT_EQ; k++) 
        if (OBJWEAR_FLAGGED(GET_EQ(ch, i), no_affect_eq[k]))
          noeffect = TRUE;
      if (noeffect) {
        continue;
      }
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON)
        continue;
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {

	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      0,
		      0,
		      GET_OBJ_PERM(GET_EQ(ch, i)), FALSE);
      }
    }
  }

  for (af = ch->affected; af; af = af->next) {
    if (stacked_effect(af->type))
      affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);
    else
      affect_modify(ch, af->location, 0, af->specific, af->bitvector, FALSE);
  }
  for (i = 0; i < NUM_APPLIES; i++) {
    modifier = calculate_best_mod(ch, i, 0, -1, -1);
    affect_modify(ch, i, modifier, 0, AFF_NONE, FALSE);
  }

  ch->aff_abils = ch->real_abils;

  GET_SAVE_MOD(ch, SAVING_FORTITUDE) = HAS_FEAT(ch, FEAT_GREAT_FORTITUDE) * 3;
  GET_SAVE_MOD(ch, SAVING_REFLEX) = HAS_FEAT(ch, FEAT_LIGHTNING_REFLEXES) * 3;
  GET_SAVE_MOD(ch, SAVING_WILL) = HAS_FEAT(ch, FEAT_IRON_WILL) * 3;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i) == NULL)
      continue;
    if (GET_EQ(ch, i)) {
      noeffect = FALSE;
      for (k = 0; k < NUM_NO_AFFECT_EQ; k++) 
        if (OBJWEAR_FLAGGED(GET_EQ(ch, i), no_affect_eq[k]))
          noeffect = TRUE;
      if (noeffect) {
        continue;
      }
      if (noeffect)
        continue;
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON)
        continue;
      for(a = 0; a < AF_ARRAY_MAX; a++)
        for(b = 0; b < 32; b++)
          if(IS_SET_AR(GET_OBJ_PERM(GET_EQ(ch, i)), (a*32)+b)) {
            SET_BIT_AR(AFF_FLAGS(ch), (a*32)+b);
          }

      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR && i == WEAR_BODY) {
        GET_SPELLFAIL(ch) = MAX(GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_SPELLFAIL), GET_SPELLFAIL(ch));
        GET_ARMORCHECKALL(ch) = MIN(GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_CHECK), GET_ARMORCHECKALL(ch));
        if (!is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), 9)))
          GET_ARMORCHECK(ch) = MIN(GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_CHECK), GET_ARMORCHECK(ch));
      }
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      0,
		      GET_EQ(ch, i)->affected[j].specific,
		      GET_OBJ_PERM(GET_EQ(ch, i)), TRUE);
      }
    }
  }

  

  for (af = ch->affected; af; af = af->next) {  
    if (stacked_effect(af->type))
      affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);
    else
      affect_modify(ch, af->location, 0, af->specific, af->bitvector, TRUE);
  }

  for (i = 0; i < NUM_APPLIES; i++) {
    modifier = calculate_best_mod(ch, i, 0, -1, -1);
    affect_modify(ch, i, modifier, 0, AFF_NONE, TRUE);
  }

  /* Make certain values are between 0..100, not < 0 and not > 100! */

  GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), 100));
  GET_INT(ch) = MAX(0, MIN(GET_INT(ch), 100));
  GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), 100));
  GET_CON(ch) = MAX(0, MIN(GET_CON(ch), 100));
  GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), 100));
  GET_STR(ch) = MAX(0, MIN(GET_STR(ch), 100));
}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  if (!ch->affected) {
    ch->next_affect = affect_list;
    affect_list = ch;
  }
  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, 0, af->specific, af->bitvector, TRUE);

  if (stacked_effect(af->type)) {
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);
  }
  else if (af->modifier > calculate_best_mod(ch, af->location, af->specific, -1, af->type)) {
    affect_modify(ch, af->location, calculate_best_mod(ch, af->location, af->specific, -1, af->type),
                  af->specific, AFF_NONE, FALSE);
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);
  }

  affect_total(ch);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *temp;

  if (ch->affected == NULL) {
    core_dump();
    return;
  }

  affect_modify(ch, af->location, 0, af->specific, af->bitvector, FALSE);

  if (stacked_effect(af->type)) {
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);
  }
  else if (af->modifier > calculate_best_mod(ch, af->location, af->specific, -1, af->type)) {
      affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);
      affect_modify(ch, af->location, calculate_best_mod(ch, af->location, af->specific, -1, af->type),
                  af->specific, AFF_NONE, TRUE);
  }

  REMOVE_FROM_LIST(af, ch->affected, next);

  free(af);
  affect_total(ch);
  if (!ch->affected) {
    struct char_data *temp;
    REMOVE_FROM_LIST(ch, affect_list, next_affect);
    ch->next_affect = NULL;
  }
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp != NULL && ((struct affected_type *) hjp)->type == type)
      affect_remove(ch, (struct affected_type *) hjp);
  }
}



/* Call affect_remove with every spell of spelltype "skill" */
void affectv_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affectedv; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affectv_remove(ch, hjp);
  }
}



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(struct char_data *ch, int type)
{
  if (!ch)
    return FALSE;

  if (IS_NPC(ch) && GET_POS(ch) < POS_SLEEPING)
    return FALSE;

  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next) {
    if (hjp->type == type)
      return (TRUE);
  }
  return (FALSE);
}



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affectedv_by_spell(struct char_data *ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affectedv; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (TRUE);

  return (FALSE);
}



void affect_join(struct char_data *ch, struct affected_type *af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
	af->duration += hjp->duration;
      if (avg_dur)
	af->duration /= 2;

      if (add_mod)
	af->modifier += hjp->modifier;
      if (avg_mod)
	af->modifier /= 2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}


/* move a player out of a room */
void char_from_room(struct char_data *ch)
{
  struct char_data *temp;
  int i;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) != NULL)
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
        if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS))
	  world[IN_ROOM(ch)].light--;

  REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room);
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}


/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room)
{
  int i;

  if (ch == NULL || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		room, top_of_world, ch);
  else {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    IN_ROOM(ch) = room;

    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
	  if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS))
	    world[room].light++;

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }

  }
    autoquest_trigger_check(ch, ch, 0, AQ_ROOM_FIND);
    autoquest_trigger_check(ch, ch, 0, AQ_MOB_FIND);
}


/* give an object to a char   */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    IN_ROOM(object) = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    autoquest_trigger_check(ch, NULL, object, AQ_OBJ_FIND);

    update_encumberance(ch);		
		
    /* set flag for crash-save system, but not on mobs! */
    if (!IS_NPC(ch))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
  } else
    log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}


/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data *ch, int eq_pos)
{
  int factor;

  if (GET_EQ(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if ((GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR_SUIT))
    factor = 100;
  else if ((GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR)) {
    factor = 100;
  }
  else
    return (0);

  return (GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_APPLYAC));
}

int invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (obj == NULL)
    return FALSE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
    return TRUE;
  return FALSE;
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos)
{
  int a, b, j;

  if (pos < 0 || pos >= NUM_WEARS) {
    core_dump();
    return;
  }

  if (GET_EQ(ch, pos)) {
    log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    return;
  }
  if (obj == NULL)
    return;

  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (IN_ROOM(obj) != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
/*
  if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
    send_to_char(ch, "You suddenly feel anxious and unsettled.  You are unable to use this item.\r\n");
//    SET_BIT_AR(AFF_FLAGS(ch), AFF_ANTI_EQUIPPED);  
    obj_to_char(obj, ch);
    return;
  }
*/
  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if ((GET_OBJ_TYPE(obj) == ITEM_ARMOR || (GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)) && !IS_NPC(ch))
    GET_ARMOR(ch) += apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS))	/* if light is ON */
	world[IN_ROOM(ch)].light++;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", GET_NAME(ch));

  
  for(a = 0; a < AF_ARRAY_MAX; a++)
    for(b = 0; b < 32; b++)
      if(IS_SET_AR(GET_OBJ_PERM(obj), (a*32)+b)) {
        SET_BIT_AR(AFF_FLAGS(ch), (a*32)+b);
      }

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {

    if ((obj->affected[j].modifier) < 0) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, obj->affected[j].specific, GET_OBJ_PERM(obj), TRUE);
    }
    else if ((obj->affected[j].modifier) > calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].specific, pos, -1)) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, obj->affected[j].specific, GET_OBJ_PERM(obj), TRUE);
      affect_modify_ar(ch, obj->affected[j].location, calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].specific, 
                    pos, -1), obj->affected[j].specific, obj_bit, FALSE);
    }
  }

  affect_total(ch);
}



struct obj_data *unequip_char(struct char_data *ch, int pos)
{
  int  j, a, b;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL) {
    core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;
/*
  if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
    for (i = 0; i < NUM_WEARS; i++) {
      objList = GET_EQ(ch, i);
      if (invalid_align(ch, objList) || invalid_class(ch, objList) || invalid_race(ch, objList))
        found = true;
    }
    if (!found) {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ANTI_EQUIPPED);
      send_to_char(ch, "You suddenly feel less anxious and unsettled.  Your combat penalties have been removed.\r\n");
    }

  }
*/

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_ARMOR_SUIT)
    GET_ARMOR(ch) -= apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS))	/* if light is ON */
	world[IN_ROOM(ch)].light--;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  for(a = 0; a < AF_ARRAY_MAX; a++)
    for(b = 0; b < 32; b++)
      if(IS_SET_AR(GET_OBJ_PERM(obj), (a*32)+b)) {
        REMOVE_BIT_AR(AFF_FLAGS(ch), (a*32)+b);
      }

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (obj->affected[j].modifier < 0) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, obj->affected[j].specific, GET_OBJ_PERM(obj), TRUE);
    }
    else if (obj->affected[j].modifier > calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].specific, pos, -1)) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, obj->affected[j].specific, GET_OBJ_PERM(obj), FALSE);
      affect_modify_ar(ch, obj->affected[j].location, calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].specific, 
                    pos, -1), obj->affected[j].specific, obj_bit, TRUE);
    }
  }

  affect_total(ch);

  return (obj);
}


int get_number(char **name)
{
  int i = 0;
  char *ppos;
  char number[MAX_INPUT_LENGTH]={'\0'};

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL) {
    *ppos++ = '\0';
    strlcpy(number, *name, sizeof(number));
    strcpy(*name, ppos);	/* strcpy: OK (always smaller) */

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return (0);

    return (atoi(number));
  }
  return (1);
}



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int *number, room_rnum room)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = world[room].people; i && *number; i = i->next_in_room)
    if (isname(name, i->name) || isname(name, GET_KEYWORDS(i)))
      if (--(*number) == 0)
	      return (i);

  return (NULL);
}



/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}



/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room)
{
  if (!object || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	room, top_of_world, object);
  else {
    object->next_content = world[room].contents;
    world[room].contents = object;
    IN_ROOM(object) = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
  }
}


/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;

  if (!object || IN_ROOM(object) == NOWHERE) {
    log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	object, IN_ROOM(object));
    return;
  }

  REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content);

  if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
    SET_BIT_AR(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
  IN_ROOM(object) = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
	obj, obj, obj_to);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  tmp_obj = obj->in_obj;

  /* Only add weight to container, or back to carrier for non-eternal
     containers.  Eternal means max capacity < 0 */
  if (GET_OBJ_VAL(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0)
  {
  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  }
}


/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container */
  /* Only worry about weight for non-eternal containers
     Eternal means max capacity < 0 */
  if (GET_OBJ_VAL(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0)
  {
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  }

  obj->in_obj = NULL;
  obj->next_content = NULL;
}


/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}


/* Extract an object from the world */
void extract_obj(struct obj_data *obj)
{
  struct obj_data *temp;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) != NOTHING)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
     extract_script(obj, OBJ_TRIGGER);

   if (GET_OBJ_RNUM(obj) == NOTHING || obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
     free_proto_script(obj, OBJ_TRIGGER);

  free_obj(obj);
}



void update_object(struct obj_data *obj, int use)
{
	return;
  if (!obj)
    return;
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}


void update_char_objects(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS) > 0) {
        i = --GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS);
	if (i == 1) {
	  send_to_char(ch, "Your light begins to flicker and fade.\r\n");
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (i == 0) {
	  send_to_char(ch, "Your light sputters out and dies.\r\n");
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[IN_ROOM(ch)].light--;
	}
      }
      update_object(GET_EQ(ch, i), 2);
    }

  if (ch->carrying)
    update_object(ch->carrying, 1);
}



/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch)
{
  struct char_data *k, *temp;
  struct descriptor_data *d;

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
        GET_NAME(ch), __FILE__);
    exit(1);
  }

  /*
   * We're booting the character of someone who has switched so first we
   * need to stuff them back into their own body.  This will set ch->desc
   * we're checking below this loop to the proper value.
   */
  if (!IS_NPC(ch) && !ch->desc) {
    for (d = descriptor_list; d; d = d->next)
      if (d->original == ch) {
	do_return(d->character, NULL, 0, 0);
        break;
      }
  }

  if (ch->desc) {
    /*
     * This time we're extracting the body someone has switched into
     * (not the body of someone switching as above) so we need to put
     * the switcher back to their own body.
     *
     * If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu.
     */
    if (ch->desc->original)
      do_return(ch, NULL, 0, 0);
    else {
      /*
       * Now we boot anybody trying to log in with the same character, to
       * help guard against duping.  CON_DISCONNECT is used to close a
       * descriptor without extracting the d->character associated with it,
       * for being link-dead, so we want CON_CLOSE to clean everything up.
       * If we're here, we know it's a player so no IS_NPC check required.
       */
      for (d = descriptor_list; d; d = d->next) {
        if (d == ch->desc)
          continue;
        if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
          STATE(d) = CON_CLOSE;
      }
      STATE(ch->desc) = CON_MENU;
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }

  /* On with the character's assets... */

  if (ch->followers || ch->master)
    die_follower(ch);

  /* Stop riding */
  if (RIDING(ch) || RIDDEN_BY(ch))
    dismount_char(ch);

  if (GUARDED_BY(ch) || GUARDING(ch)) {
    stop_guard(ch);
  }

/*
  // transfer objects to room, if any 
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, IN_ROOM(ch));
  }

  // transfer equipment to room, if any 
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i), IN_ROOM(ch));
*/
  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }
  /* we can't forget the hunters either... */
  for (temp = character_list; temp; temp = temp->next)
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;

  char_from_room(ch);

  if (IS_NPC(ch)) {
    if (GET_MOB_RNUM(ch) != NOTHING)	/* prototyped */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);
    if (SCRIPT(ch))
      extract_script(ch, MOB_TRIGGER);
    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));
  } else {
      save_char(ch);
      Crash_delete_crashfile(ch);
  }

  /* If there's a descriptor, they're in the menu now. */
  if (IS_NPC(ch) || !ch->desc)
    free_char(ch);
}


/*
 * Q: Why do we do this?
 * A: Because trying to iterate over the character
 *    list with 'ch = ch->next' does bad things if
 *    the current character happens to die. The
 *    trivial workaround of 'vict = next_vict'
 *    doesn't work if the _next_ person in the list
 *    gets killed, for example, by an area spell.
 *
 * Q: Why do we leave them on the character_list?
 * A: Because code doing 'vict = vict->next' would
 *    get really confused otherwise.
 */
void extract_char(struct char_data *ch)
{
  if (!ch)
    return;

  struct follow_type *foll;
  int i;
  struct obj_data *obj;

  if (IS_NPC(ch)) {
    if (!IS_SET_AR(MOB_FLAGS(ch), MOB_NOTDEADYET))
      SET_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
    else
      return;
  } else {
    if (!IS_SET_AR(PLR_FLAGS(ch), PLR_NOTDEADYET))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);
    else
      return;
  }

  for (foll = ch->followers; foll; foll = foll->next) {
    if (IS_NPC(foll->follower) && AFF_FLAGGED(foll->follower, AFF_CHARM) &&
        (IN_ROOM(foll->follower) == IN_ROOM(ch) || IN_ROOM(ch) == 1)) {
      /* transfer objects to char, if any */
      while (foll->follower->carrying) {
        obj = foll->follower->carrying;
        obj_from_char(obj);
        obj_to_char(obj, ch);
      }

      /* transfer equipment to char, if any */
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(foll->follower, i)) {
          obj = unequip_char(foll->follower, i);
          obj_to_char(obj, ch);
        }
      
      extract_char(foll->follower);
    }
  }

  extractions_pending++;
}


/*
 * I'm not particularly pleased with the MOB/PLR
 * hoops that have to be jumped through but it
 * hardly calls for a completely new variable.
 * Ideally it would be its own list, but that
 * would change the '->next' pointer, potentially
 * confusing some code. Ugh. -gg 3/15/2001
 *
 * NOTE: This doesn't handle recursive extractions.
 */
void extract_pending_chars(void)
{
  struct char_data *vict, *next_vict, *prev_vict, *temp;

  if (extractions_pending < 0)
    log("SYSERR: Negative (%d) extractions pending.", extractions_pending);

  for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict) {
    next_vict = vict->next;

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    REMOVE_FROM_LIST(vict, affect_list, next_affect);
    REMOVE_FROM_LIST(vict, affectv_list, next_affectv);
    if (vict)
      extract_char_final(vict);
    extractions_pending--;

    if (prev_vict)
      prev_vict->next = next_vict;
    else
      character_list = next_vict;
  }

  if (extractions_pending > 0)
    log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);

  extractions_pending = 0;
}



/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */


struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (str_cmp(i->name, name)) /* If not same, continue */
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}

struct char_data *get_player_notvis(struct char_data *ch, char *name, int *number, int inroom)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (str_cmp(i->name, name)) /* If not same, continue */
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}


struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
			char *tmpdesc = NULL;
      if (isname(name, i->name) || isname(name, GET_KEYWORDS(i)) || isname(name, (const char *) (tmpdesc = which_desc(i)))) {
        if (CAN_SEE(ch, i)) {
	  if (--(*number) == 0) {
	    return (i);
          }
        }
      }
	    free(tmpdesc);
//    }
  }

  return (NULL);
}

struct char_data *get_char_room_notvis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
/*    if (!IS_NPC(i) && GET_RACE(i) == RACE_IRDA && GET_IRDA_SHAPE_STATUS(i) == 1) {
      if (isname(name, i->name) || isname(name, GET_IRDA_KEYWORDS_1(i))) {
	  if (--(*number) == 0) {
	    return (i);
          }
      }
    }
 
    else if (!IS_NPC(i) && GET_RACE(i) == RACE_IRDA && GET_IRDA_SHAPE_STATUS(i) == 2) {
      if (isname(name, i->name) || isname(name, GET_IRDA_KEYWORDS_2(i))) {
	  if (--(*number) == 0) {
	    return (i);
          }
      }
    }
    else {
*/
			char *tmpdesc = NULL;
      if (isname(name, i->name) || isname(name, GET_KEYWORDS(i)) || isname(name, (const char *) (tmpdesc = which_desc(i)))) {
	  if (--(*number) == 0) {
	    return (i);
          }
      }
			free(tmpdesc);
//    }
  }

  return (NULL);
}


struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return get_player_vis(ch, name, NULL, 0);

  for (i = character_list; i && *number; i = i->next) {
      if (!IS_NPC(i) && isname(name, i->name)) {
        if (CAN_SEE(ch, i)) {
	  if (--(*number) == 0) {
	    return (i);
          }
        }
      }
  }

  for (i = character_list; i && *number; i = i->next) {
			char *tmpdesc = NULL;
      if (isname(name, i->name) || isname(name, GET_KEYWORDS(i)) || isname(name, (const char *) (tmpdesc = which_desc(i)))) {
        if (CAN_SEE(ch, i)) {
	  if (--(*number) == 0) {
	    return (i);
          }
        }
      }
	    free(tmpdesc);
  }

  return (NULL);
}

struct char_data *get_char_world_notvis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_notvis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return get_player_notvis(ch, name, NULL, 0);


  for (i = character_list; i && *number; i = i->next) {
      if (!IS_NPC(i) && isname(name, i->name)) {
        if (--(*number) == 0) {
          return (i);
        }
      }
  }


  for (i = character_list; i && *number; i = i->next) {
			char *tmpdesc = NULL;
      if (isname(name, i->name) || isname(name, GET_KEYWORDS(i)) || isname(name, (const char *) (tmpdesc = which_desc(i)))) {
        if (true) {
	  if (--(*number) == 0) {
	    return (i);
          }
        }
      }
	    free(tmpdesc);
  }
  return (NULL);
}


struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(ch, name, number);
  else
    return (NULL);
}

struct char_data *get_char_notvis(struct char_data *ch, char *name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_notvis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_notvis(ch, name, number);
  else
    return (NULL);
}


struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list)
{
  struct obj_data *i;
  int num = 0;

  if (!number) 
  {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = list; i && *number; i = i->next_content)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i) || (GET_OBJ_TYPE(i) == ITEM_LIGHT))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}


/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *number)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, number, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, number, world[IN_ROOM(ch)].contents)) != NULL)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && *number; i = i->next)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}


struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (NULL);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (equipment[j]);

  return (NULL);
}


int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (j);

  return (-1);
}


const char *money_desc(int amount)
{
  int cnt;
  struct {
    int limit;
    const char *description;
  } money_table[] = {
    {          1, "a gold coin"				},
    {         10, "a tiny pile of gold coins"		},
    {         20, "a handful of gold coins"		},
    {         75, "a little pile of gold coins"		},
    {        200, "a small pile of gold coins"		},
    {       1000, "a pile of gold coins"		},
    {       5000, "a big pile of gold coins"		},
    {      10000, "a large heap of gold coins"		},
    {      20000, "a huge mound of gold coins"		},
    {      75000, "an enormous mound of gold coins"	},
    {     150000, "a small mountain of gold coins"	},
    {     250000, "a mountain of gold coins"		},
    {     500000, "a huge mountain of gold coins"	},
    {    1000000, "an enormous mountain of gold coins"	},
    {          0, NULL					},
  };

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of gold coins");
}


struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200]={'\0'};
  int y;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("coin gold");
    obj->short_description = strdup("a gold coin");
    obj->description = strdup("One miserable gold coin is lying here.");
    new_descr->keyword = strdup("coin gold");
    new_descr->description = strdup("It's just one miserable little gold coin.");
  } else {
    obj->name = strdup("coins gold");
    obj->short_description = strdup(money_desc(amount));
    snprintf(buf, sizeof(buf), "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("coins gold");
    if (amount < 10)
      snprintf(buf, sizeof(buf), "There are %d coins.", amount);
    else if (amount < 100)
      snprintf(buf, sizeof(buf), "There are about %d coins.", 10 * (amount / 10));
    else if (amount < 1000)
      snprintf(buf, sizeof(buf), "It looks to be about %d coins.", 100 * (amount / 100));
    else if (amount < 100000)
      snprintf(buf, sizeof(buf), "You guess there are, maybe, %d coins.",
	      1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
    else
      strcpy(buf, "There are a LOT of coins.");	/* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  GET_OBJ_MATERIAL(obj) = MATERIAL_GOLD;
  GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) = 100;
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 100; 
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, VAL_MONEY_SIZE) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
}


/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, struct char_data *ch, struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int i, found, number;
  char name_val[MAX_INPUT_LENGTH]={'\0'};
  char *name = name_val;

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);
  if (!(number = get_number(&name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0) {
	*tar_obj = GET_EQ(ch, i);
	found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{

  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (is_abbrev("all.", arg)) {
    //arg = strdup(replace_string(arg, "all.", ""));
    strcpy(arg, arg + 4);	/* strcpy: OK (always less) */
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}

void affectv_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  if (!ch->affected) {
    ch->next_affect = affectv_list;
    affect_list = ch;
  }
  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;
  affect_total(ch);
}

void affectv_remove(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *temp;

  if (ch->affectedv == NULL) {
    core_dump();
    return;
  }
  REMOVE_FROM_LIST(af, ch->affectedv, next);
  free(af);
  affect_total(ch);
  if (!ch->affectedv) {
    struct char_data *temp;
    REMOVE_FROM_LIST(ch, affectv_list, next_affectv);
    ch->next_affectv = NULL;
  }
}

void affectv_join(struct char_data *ch, struct affected_type *af,
                      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affectedv; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
        af->duration += hjp->duration;
      if (avg_dur)
        af->duration /= 2;

      if (add_mod)
        af->modifier += hjp->modifier;
      if (avg_mod)
        af->modifier /= 2;

      affectv_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

int is_better(struct obj_data *object, struct obj_data *object2)
{
  int value1=0, value2=0;

  switch (GET_OBJ_TYPE(object)) {
    case ITEM_ARMOR:
      value1 = GET_OBJ_VAL(object, VAL_ARMOR_APPLYAC);
      value2 = GET_OBJ_VAL(object2, VAL_ARMOR_APPLYAC);
    break;
    case ITEM_WEAPON:
      value1 = (1 + GET_OBJ_VAL(object, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object, VAL_WEAPON_DAMDICE);
      value2 = (1 + GET_OBJ_VAL(object2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object2, VAL_WEAPON_DAMDICE);
    break;
    default:
    break;
  }

  if (value1 > value2)
    return 1;
  else
    return 0;
}

/* check and see if this item is better */
void item_check(struct obj_data *object, struct char_data *ch)
{
  int where=0;

  if (IS_HUMANOID(ch) && !(mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)) {
    if (invalid_align(ch, object) || invalid_class(ch, object))
      return;

    switch (GET_OBJ_TYPE(object)) {
      case ITEM_WEAPON:
        if (!GET_EQ(ch, WEAR_WIELD1)) {
          perform_wear(ch, object, WEAR_WIELD1);
        } else {
          if (is_better(object, GET_EQ(ch, WEAR_WIELD1))) {
            perform_remove(ch, WEAR_WIELD1);
            perform_wear(ch, object, WEAR_WIELD1);
          }
        }
      break;
      case ITEM_ARMOR:
      case ITEM_WORN:
        where = find_eq_pos(ch, object, 0);
        if (!GET_EQ(ch, where)) {
          perform_wear(ch, object, where);
        } else {
          if (is_better(object, GET_EQ(ch, where))) {
            perform_remove(ch, where);
            perform_wear(ch, object, where);
          } 
        }

      break;
      default:
      break;
    }
  }
}

/* dismount_char() / fr: Daniel Koepke (dkoepke@california.com)
 *   If a character is mounted on something, we dismount them.  If
 *   someone is mounting our character, then we dismount that someone.
 *   This is used for cleaning up after a mount is cancelled by
 *   something (either intentionally or by death, etc.)
 */
void stop_guard(struct char_data *ch)
{
  if (GUARDING(ch)) {
    GUARDED_BY(GUARDING(ch)) = NULL;
    GUARDING(ch) = NULL;
  }
  else if (GUARDED_BY(ch)) {
    GUARDING(GUARDED_BY(ch)) = NULL;
    GUARDED_BY(ch) = NULL;
  }
}

void dismount_char(struct char_data *ch) 
{

  
  RIDDEN_BY(RIDING(ch)) = NULL;
  RIDING(ch) = NULL;
  
  GET_POS(ch) = POS_STANDING;

/*
  if (IS_NPC(ch)) {
    if (RIDING(ch)) {
      SET_RIDDEN_BY_PC(RIDING(ch), NULL);
      SET_RIDING_PC(ch, NULL);
      GET_POS(ch) = POS_STANDING;
    }
    if (RIDDEN_BY(ch)) {
      SET_RIDING_PC(RIDDEN_BY(ch), NULL);
      SET_RIDDEN_BY_PC(ch, NULL);
      GET_POS(ch) = POS_STANDING;
    }
  }
  else {
    if (RIDING(ch)) {
      SET_RIDDEN_BY_MOB(RIDING(ch), NULL);
      SET_RIDING_MOB(ch, NULL);
      GET_POS(ch) = POS_STANDING;
    }
    if (RIDDEN_BY(ch)) {
      SET_RIDING_MOB(RIDDEN_BY(ch), NULL);
      SET_RIDDEN_BY_MOB(ch, NULL);
      GET_POS(ch) = POS_STANDING;
    }
  }
*/
}


/* mount_char() / fr: Daniel Koepke (dkoepke@california.com)
 *   Sets _ch_ to mounting _mount_.  This does not make any checks
 *   what-so-ever to see if the _mount_ is mountable, etc.  That is
 *   left up to the calling function.  This does not present any
 *   messages, either.
 */
void mount_char(struct char_data *ch, struct char_data *mount) 
{
  if (IS_NPC(ch))
    SET_RIDING_MOB(ch, mount);
  else 
    SET_RIDING_PC(ch, mount);

  if (IS_NPC(mount))
    SET_RIDDEN_BY_MOB(mount, ch);
  else
    SET_RIDDEN_BY_PC(mount, ch);
  GET_POS(ch) = POS_RIDING;
}

int get_speed(struct char_data *ch) {

  if (!ch || ch == NULL)
    return 25;

  int speed = 20;
  int i = 0;
  int ft20 = 20, ft30 = 30;
  int mult = 3;

  if (IS_DWARF(ch) ||
      IS_KENDER(ch) ||
      IS_HALFLING(ch) ||
      IS_GNOME(ch) || get_size(ch) == SIZE_SMALL)
    speed = 20;
  else if (IS_OGRE(ch))
    speed = 40;
  else if (IS_CENTAUR(ch))
    speed = 50;
  else if (IS_ANIMAL(ch))
    speed = 10 + (10 * MAX(0, get_size(ch)));
  else if (IS_CHEETAH(ch) || IS_EAGLE(ch))
    speed = 80;
  else
    speed = 30;

  if (ch->player_specials->mounted == MOUNT_SUMMON)
    speed = pet_list[ch->player_specials->summon_num].speed;

  if (ch->player_specials->mounted == MOUNT_MOUNT)
    speed = pet_list[ch->player_specials->mount_num].speed;

  if (affected_by_spell(ch, SPELL_HASTE))
    speed *= 2;

  if (affected_by_spell(ch, SPELL_SLOW))
    speed /= 2;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR || GET_OBJ_TYPE(GET_EQ(ch, i)) == 
        ITEM_ARMOR_SUIT)) {
      ft20 = MAX(15, MIN(ft20, armor_list[GET_OBJ_VAL(GET_EQ(ch, i), 1)].twentyFoot));
      ft30 = MAX(15, MIN(ft30, armor_list[GET_OBJ_VAL(GET_EQ(ch, i), 1)].thirtyFoot));
    }
  }
 
  if (!ch->player_specials->mounted) {
  if (speed == 20) {
    speed = ft20;
  } else if (speed == 30) {
    speed = ft30;
  } else {
    if (highest_armor_type(ch) == ARMOR_TYPE_MEDIUM) {
      speed *= 80;
      speed /= 100;
    } else if (highest_armor_type(ch) == ARMOR_TYPE_HEAVY) {
      speed *= 67;
      speed /= 100;
    }
  }
  }

  if (!ch->player_specials->mounted && HAS_FEAT(ch, FEAT_FAST_MOVEMENT) && highest_armor_type(ch) <= ARMOR_TYPE_MEDIUM)
    speed += 10;
  if (!ch->player_specials->mounted && (GET_CLASS_RANKS(ch, CLASS_MONK) + GET_CLASS_RANKS(ch, CLASS_SACRED_FIST)) >= 3 && highest_armor_type(ch) <= ARMOR_TYPE_NONE)
    speed += (GET_CLASS_RANKS(ch, CLASS_MONK) + GET_CLASS_RANKS(ch, CLASS_SACRED_FIST)) / 3 * 10;

  if (IS_MEDIUM_LOAD(ch) || IS_HEAVY_LOAD(ch) || IS_OVER_LOAD(ch))
    speed = speed * 7 / 10;

  if (IS_SET_AR(PRF_FLAGS(ch), PRF_RUN) || IS_SET_AR(PRF_FLAGS(ch), PRF_JOG)) {
    if (IS_LIGHT_LOAD(ch))
      mult = 4;
    if (IS_MEDIUM_LOAD(ch))
      mult = 4;
    if (IS_HEAVY_LOAD(ch))
      mult = 3;
    if (IS_SET_AR(PRF_FLAGS(ch), PRF_JOG))
      mult-= 2;
    if (HAS_FEAT(ch, FEAT_RUN))
      mult++;
    speed *= mult;
  }

  if (affected_by_spell(ch, SPELL_ENTANGLED) || affected_by_spell(ch, SPELL_ENTANGLE))
    speed /= 2;


  if (!ch->player_specials->mounted && affected_by_spell(ch, SPELL_FLY))
    speed = MAX(speed, 60);


  if (!ch->player_specials->mounted && affected_by_spell(ch, SPELL_AFF_DEFENSIVE_STANCE))
    speed = MIN(5, speed);

  return MAX(1, speed);
}

int highest_armor_type(struct char_data *ch) {

  int type = ARMOR_TYPE_NONE;
  int i = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR ||
          GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR_SUIT)
        type = MAX(type, GET_OBJ_VAL(GET_EQ(ch, i), 1));
  }

  return (type);

}

void update_crafting_progress(void) {
  return;
}

void set_max_affect(struct char_data *ch, int loc, int mod, int spec, bool add) {

  struct affected_type *af, *next;
  struct char_data *i;
  int x, y=0,j=0,k=0;
  int max_skill_mod[SKILL_TABLE_SIZE];
  int max_feat_mod[NUM_FEATS_DEFINED];
  int max_bonus[NUM_APPLIES];
  int num[NUM_APPLIES];
  int num_skill[SKILL_TABLE_SIZE];
  int num_feat[NUM_FEATS_DEFINED];

  for (x = 0; x < NUM_APPLIES; x++) {  
    if (x != loc)
      continue;
    num[x] = 0;
    if (x == APPLY_AC_DODGE || x == APPLY_NONE)
      continue;
    for (j = 0; j < NUM_WEARS; j++) {
      for (k = 0; k < MAX_OBJ_AFFECT; k++) {
        if (GET_EQ(ch, j) && GET_EQ(ch, j)->affected[k].location == x) {
          if (x == APPLY_SKILL) {
            for (y = 0; y <= SKILL_TABLE_SIZE; y++) {
              num_skill[y] = 0;
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_skill_mod[y]) 
                  max_skill_mod[y] = GET_EQ(ch, j)->affected[k].modifier;
            }
          }
          else if (x == APPLY_FEAT) {
            for (y = 0; y <= NUM_FEATS_DEFINED; y++) {
              num_feat[y] = 0;
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_feat_mod[y]) 
                  max_feat_mod[y] = GET_EQ(ch, j)->affected[k].modifier;
            }
          }
          else if (GET_EQ(ch, j)->affected[k].modifier >= max_bonus[x]) 
            max_bonus[x] = GET_EQ(ch, j)->affected[k].modifier;
        }
      }
    }
    for (i = affect_list; i; i = i->next_affect) {
      if (i == ch) {
        for (af = i->affected; af; af = next) {
          next = af->next;
          if (af->location == x) {
            if (x == APPLY_SKILL) {
              for (y = 0; y <= SKILL_TABLE_SIZE; y++)
                if (af->specific == y)
                  if (af->modifier > max_skill_mod[y])
                    max_skill_mod[y] = af->modifier;
            }
            else if (x == APPLY_FEAT) {
              for (y = 0; y <= NUM_FEATS_DEFINED; y++)
                if (af->specific == y)
                  if (af->modifier > max_feat_mod[y])
                    max_feat_mod[y] = af->modifier;
            } 
            else if (af->modifier > max_bonus[x])
              max_bonus[x] = af->modifier;
          }
        }
      }
    }	
  }

  if (loc == APPLY_SKILL) {
    for (y = 0; y <= SKILL_TABLE_SIZE; y++)
      if (spec == y) {
        if (mod > max_skill_mod[spec] && mod > 0) {
          if (add)
            aff_apply_modify(ch, x, GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y], y, 
                             "set_max_affect", FALSE);
          else
            aff_apply_modify(ch, x, -(GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y]), y, 
                             "set_max_affect", FALSE);                     
        }
      }
  }
  else if (loc == APPLY_FEAT) {
    for (y = 0; y <= NUM_FEATS_DEFINED; y++)
      if (spec == y) {
        if (mod > max_feat_mod[spec] && mod > 0) {
          if (add)
            aff_apply_modify(ch, x, GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y], y, 
                             "set_max_affect", FALSE);
          else
            aff_apply_modify(ch, x, -(GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y]), y, 
                             "set_max_affect", FALSE);                     
        }
      }
  }
  else if (mod > max_bonus[loc] && mod > 0) {
    if (add)
      aff_apply_modify(ch, x, GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y], y, 
                       "set_max_affect", FALSE);
    else
      aff_apply_modify(ch, x, -(GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y]), y, 
                       "set_max_affect", FALSE);                     
  }

/*
  for (x = 0; x < NUM_APPLIES; x++) {  
    if (x == APPLY_AC_DODGE || x == APPLY_NONE)
      continue;
    for (j = 0; j < NUM_WEARS; j++) {
      for (k = 0; k < MAX_OBJ_AFFECT; k++) {
        if (GET_EQ(ch, j) && GET_EQ(ch, j)->affected[k].location == x) {
          if (x == APPLY_SKILL) {
            for (y = 0; y <= SKILL_TABLE_SIZE; y++)
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_skill_mod[y] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
                  if (add)
                    aff_apply_modify(ch, x, GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y], y, 
                                     "set_max_affect", FALSE);
                  else
                    aff_apply_modify(ch, x, -(GET_EQ(ch, j)->affected[k].modifier - max_skill_mod[y]), y, 
                                     "set_max_affect", FALSE);                     
                }
          }
          else if (x == APPLY_FEAT) {
            for (y = 0; y <= NUM_FEATS_DEFINED; y++)
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_feat_mod[y] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
                  if (GET_EQ(ch, j)->affected[k].modifier == max_feat_mod[y])
                    num_feat[y]++;
                  if (num_feat[y] != 1)
                    aff_apply_modify(ch, x, -GET_EQ(ch, j)->affected[k].modifier, y, "set_max_affect", FALSE);
                }
          }
          else if (GET_EQ(ch, j)->affected[k].modifier >= max_bonus[x] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
            if (GET_EQ(ch, j)->affected[k].modifier == max_bonus[x])
              num[x]++;
            if (num[x] != 1)
              aff_apply_modify(ch, x, -GET_EQ(ch, j)->affected[k].modifier, 0, "set_max_affect", FALSE);
          }
        }
      }
    }
    for (i = affect_list; i; i = i->next_affect) {
      if (i == ch) {
        for (af = i->affected; af; af = next) {
          next = af->next;
          if (af->location == x) {
            if (x == APPLY_SKILL) {
              for (y = 0; y <= SKILL_TABLE_SIZE; y++)
                if (af->specific == y)
                  if (af->modifier < max_skill_mod[y] && af->modifier > 0) {
                    if (af->modifier == max_skill_mod[y])
                      num_skill[y]++;
                    if (num_skill[y] != 1)
                      aff_apply_modify(ch, x, -af->modifier, y, "set_max_affect", FALSE);
                  }
            }
            else if (x == APPLY_FEAT) {
              for (y = 0; y <= NUM_FEATS_DEFINED; y++)
                if (af->specific == y)
                  if (af->modifier < max_feat_mod[y] && af->modifier > 0) {
                    if (af->modifier == max_feat_mod[y])
                      num_feat[y]++;
                    if (num_feat[y] != 1)
                      aff_apply_modify(ch, x, -af->modifier, y, "set_max_affect", FALSE);
                  }
            } 
            else if (af->modifier < max_bonus[x] && af->modifier > 0) {
              if (af->modifier == max_bonus[x])
                num[x]++;
              if (num[x] != 1)
                aff_apply_modify(ch, x, -af->modifier, y, "set_max_affect", FALSE);
            }
          }
        }
      }
    }	
  }  
*/
}

int should_remove_stat(struct char_data *ch, int loc, int mod, int spec) {

  struct affected_type *af, *next;
  struct char_data *i;
  int x, y=0, j, k;
  int max_skill_mod[SKILL_TABLE_SIZE];
  int max_feat_mod[NUM_FEATS_DEFINED];
  int max_bonus[NUM_APPLIES];
  int num[NUM_APPLIES];
  int num_skill[SKILL_TABLE_SIZE];
  int num_feat[NUM_FEATS_DEFINED];


  if (mod < 0)
    return TRUE;

  for (x = 0; x < NUM_APPLIES; x++) {  
    num[x] = 0;
    if (x != loc)
      continue;
    for (j = 0; j < NUM_WEARS; j++) {
      for (k = 0; k < MAX_OBJ_AFFECT; k++) {
        if (GET_EQ(ch, j) && GET_EQ(ch, j)->affected[k].location == x) {
          if (x == APPLY_SKILL) {
            for (y = 0; y <= SKILL_TABLE_SIZE; y++) {
              num_skill[y] = 0;
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_skill_mod[y]) 
                  max_skill_mod[y] = GET_EQ(ch, j)->affected[k].modifier;
            }
          }
          else if (x == APPLY_FEAT) {
            for (y = 0; y <= NUM_FEATS_DEFINED; y++) {
              num_feat[y] = 0;
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_feat_mod[y]) 
                  max_feat_mod[y] = GET_EQ(ch, j)->affected[k].modifier;
            }
          }
          else if (GET_EQ(ch, j)->affected[k].modifier >= max_bonus[x]) 
            max_bonus[x] = GET_EQ(ch, j)->affected[k].modifier;
        }
      }
    }
    for (i = affect_list; i; i = i->next_affect) {
      if (i == ch) {
        for (af = i->affected; af; af = next) {
          next = af->next;
          if (af->location == x) {
            if (x == APPLY_SKILL) {
              for (y = 0; y <= SKILL_TABLE_SIZE; y++)
                if (af->specific == y)
                  if (af->modifier > max_skill_mod[y])
                    max_skill_mod[y] = af->modifier;
            }
            else if (x == APPLY_FEAT) {
              for (y = 0; y <= NUM_FEATS_DEFINED; y++)
                if (af->specific == y)
                  if (af->modifier > max_feat_mod[y])
                    max_feat_mod[y] = af->modifier;
            } 
            else if (af->modifier > max_bonus[x])
              max_bonus[x] = af->modifier;
          }
        }
      }
    }	
  }

  for (x = 0; x < NUM_APPLIES; x++) {  
    if (x == APPLY_AC_DODGE)
      continue;
    for (j = 0; j < NUM_WEARS; j++) {
      for (k = 0; k < MAX_OBJ_AFFECT; k++) {
        if (GET_EQ(ch, j) && GET_EQ(ch, j)->affected[k].location == x) {
          if (x == APPLY_SKILL) {
            for (y = 0; y <= SKILL_TABLE_SIZE; y++)
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier <= max_skill_mod[y] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
                  if (GET_EQ(ch, j)->affected[k].modifier == max_skill_mod[y])
                    num_skill[y]++;
                }
          }
          else if (x == APPLY_FEAT) {
            for (y = 0; y <= NUM_FEATS_DEFINED; y++)
              if (GET_EQ(ch, j)->affected[k].specific == y)
                if (GET_EQ(ch, j)->affected[k].modifier >= max_feat_mod[y] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
                  if (GET_EQ(ch, j)->affected[k].modifier == max_feat_mod[y])
                    num_feat[y]++;
                }
          }
          else if (GET_EQ(ch, j)->affected[k].modifier >= max_bonus[x] &&
                    GET_EQ(ch, j)->affected[k].modifier > 0) {
            if (GET_EQ(ch, j)->affected[k].modifier == max_bonus[x])
              num[x]++;
          }
        }
      }
    }
    for (i = affect_list; i; i = i->next_affect) {
      if (i == ch) {
        for (af = i->affected; af; af = next) {
          next = af->next;
          if (af->location == x) {
            if (x == APPLY_SKILL) {
              for (y = 0; y <= SKILL_TABLE_SIZE; y++)
                if (af->specific == y)
                  if (af->modifier >= max_skill_mod[y] && af->modifier > 0) {
                    if (af->modifier == max_skill_mod[y])
                      num_skill[y]++;
                  }
            }
            else if (x == APPLY_FEAT) {
              for (y = 0; y <= NUM_FEATS_DEFINED; y++)
                if (af->specific == y)
                  if (af->modifier >= max_feat_mod[y] && af->modifier > 0) {
                    if (af->modifier == max_feat_mod[y])
                      num_feat[y]++;
                  }
            } 
            else if (af->modifier == max_bonus[x] && af->modifier > 0) {
              if (af->modifier == max_bonus[x])
                num[x]++;
            }
          }
        }
      }
    }	
  }  

  if (loc == APPLY_SKILL) {
    for (y = 0; y <= SKILL_TABLE_SIZE; y++)
      if (spec == y) {
        if (mod > max_skill_mod[spec] && mod > 0) {
            return TRUE;
        }
        else if (mod == max_skill_mod[spec] && mod > 0 && num_skill[spec] < 1) {
            return TRUE;
        }
      }

  }
  else if (loc == APPLY_FEAT) {
    for (y = 0; y <= NUM_FEATS_DEFINED; y++)
      if (spec == y) {
        if (mod > max_feat_mod[spec] && mod > 0) {
            return TRUE;
        }
        if (mod == max_feat_mod[spec] && mod > 0 && num_feat[spec] < 1) {
            return TRUE;
        }
      }
  }
  else if (mod > max_bonus[loc] && mod > 0) {
    return TRUE;
  }
  else if (mod == max_bonus[loc] && mod > 0 && num[loc] < 1) {
    return TRUE;
  }

  return FALSE;
}

int stacked_effect(int type) {

  switch (type) {
    case SPELL_AFF_STRENGTH_OF_HONOR:
    case SPELL_AFF_DEFENSIVE_STANCE:
    case SPELL_BLESS_SINGLE:
    case SPELL_DIVINE_FAVOR:
    case SPELL_AID:
    case SPELL_HASTE:
    case SPELL_INSPIRE_COURAGE:
    case SPELL_INSPIRE_GREATNESS:
    case SPELL_INSPIRE_HEROICS:
    case SPELL_INSPIRE_COMPETENCE:
    case SPELL_PRAYER:
    case SPELL_DEATH_KNELL:
    case SPELL_RACIAL_NATURAL_ARMOR:
    case SPELL_AURA_OF_GOOD:
    case SPELL_ARMOR_SKIN:
    case SPELL_AFF_RAGE:
    case SPELL_AFF_TAUNTED:
    case SPELL_AFF_INTIMIDATED:
    case SPELL_AFF_CHALLENGED:
    case SPELL_BESTOW_CURSE:
    case SPELL_AFF_FATIGUED:
    case SPELL_POISON:
    case SPELL_FEAT_DIVINE_MIGHT:
    case SPELL_FEAT_DIVINE_SHIELD:
    case SPELL_GREATER_INVISIBILITY:
    case SPELL_ENLARGE_PERSON:
      return TRUE;
  }

return FALSE;

}

const char *last_array[11] = 
{
  "Connect",
  "Enter Game",
  "Reconnect",
  "Takeover",
  "Quit",
  "Idleout",
  "Disconnect",
  "Shutdown",
  "Copyover",
  "Crash",
  "Playing"
  "Copyover"
};

struct last_entry *find_llog_entry(int punique, int idnum,int close) {
  FILE *fp;
  struct last_entry mlast;
  struct last_entry *llast;
  int size,recs,tmp;

  if(!(fp=fopen(LAST_FILE,"r"))) {
    log("error opening last_file for reading");
    return NULL;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */

  recs = size/sizeof(struct last_entry);
  /* we'll search last to first, since it's faster than any thing else
        we can do (like searching for the last shutdown/etc..) */
  for(tmp=recs-1; tmp > 0; tmp--) {
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);
    fread(&mlast,sizeof(struct last_entry),1,fp);
        /*another one to keep that stepback */
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.punique == punique &&
        mlast.idnum == idnum) {
        /* then we've found a match */
      CREATE(llast,struct last_entry,1);
      memcpy(llast,&mlast,sizeof(struct last_entry));
      fclose(fp);
      return llast;
    }
        /* check for crash/reboot/etc code */
    if(mlast.punique < 0 && close != 0) {
      fclose(fp);
      return NULL;
    }
        /*not the one we seek. next */
  }
        /*not found, no problem, quit */
  fclose(fp);
  return NULL;
}

  /* mod_llog_entry assumes that llast is accurate */
void mod_llog_entry(struct last_entry *llast,int type) 
{
  FILE *fp;
  struct last_entry mlast;
  int size = 0,recs = 0,tmp = 0;

  if(!(fp=fopen(LAST_FILE,"r+"))) {
    log("error opening last_file for reading and writing");
    return;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */

  recs = size/sizeof(struct last_entry);

  /* we'll search last to first, since it's faster than any thing else
        we can do (like searching for the last shutdown/etc..) */
  for(tmp=recs; tmp > 0; tmp--) {
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);
    fread(&mlast,sizeof(struct last_entry),1,fp);
        /*another one to keep that stepback */
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.punique == llast->punique &&
        mlast.idnum == llast->idnum) {
        /* then we've found a match */
        /* lets assume quit is inviolate, mainly because
                disconnect is called after each of these */
      if(mlast.close_type != LAST_QUIT &&
          mlast.close_type != LAST_IDLEOUT &&
          mlast.close_type != LAST_REBOOT &&
          mlast.close_type != LAST_COPYOVER &&
          mlast.close_type != LAST_SHUTDOWN) {
        mlast.close_type=type;
      }
      mlast.close_time=time(0);

        /*write it, and we're done!*/
      fwrite(&mlast,sizeof(struct last_entry),1,fp);
      fclose(fp);
      return;
    }
        /*not the one we seek. next */
  }
  fclose(fp);

        /*not found, no problem, quit */
  return;
}

void add_llog_entry(struct char_data *ch, int type) 
{
  FILE *fp;
  struct last_entry *llast;

  /* so if a char enteres a name, but bad password, otherwise
        loses link before he gets a pref assinged, we
        won't record it */
  if(GET_PREF(ch) <= 0) {
    return;
  }

  /* we use the close at 0 because if we're modifying a current one,
     we'll take the most recent one, but, if the mud has rebooted/etc
     then they won't be _after_ that (after coming from the direction
     that we're counting, which is last to first) */
  llast = find_llog_entry(GET_PREF(ch), GET_IDNUM(ch),0);

  if(llast == NULL) {  /* no entry found, add ..error if close! */
    CREATE(llast,struct last_entry,1);
    strncpy(llast->username,GET_NAME(ch),16);
    strncpy(llast->hostname,GET_HOST(ch),128);
    llast->idnum=GET_IDNUM(ch);
    llast->punique=GET_PREF(ch);
    llast->time=time(0);
    llast->close_time=0;
      llast->close_type=LAST_CRASH;


    if(!(fp=fopen(LAST_FILE,"a"))) {
      log("error opening last_file for appending");
      free(llast);
      return;
    }
    fwrite(llast,sizeof(struct last_entry),1,fp);
    fclose(fp);
    return;
  } else {
    /* we're modifying a found entry */
    mod_llog_entry(llast,type);
    free(llast);
  }
}

/* debugging stuff, if you wanna see the whole file */
char *list_llog_entry() 
{
  FILE *fp;
  struct last_entry llast;
  char buffer[12800]={'\0'};
  char storage_buffer[12800]={'\0'};
  extern const char *last_array[];

  if(!(fp=fopen(LAST_FILE,"r"))) 
  {
    log("bad things.");
    return strdup("Error.");
  }

  snprintf(buffer, sizeof(buffer),"Last log\r\n");

  fread(&llast,sizeof(struct last_entry),1,fp);

  while(!feof(fp)) 
  {
    snprintf(storage_buffer, sizeof(storage_buffer),"%s%10s\t%s\t%d\t%s",
        buffer,llast.username,last_array[llast.close_type],
        llast.punique,ctime(&llast.time));
    fread(&llast,sizeof(struct last_entry),1,fp);
  }
  fclose(fp);
  return strdup(storage_buffer);
}

void extract_linkless_characters(void)
{

  struct char_data *ch, *next;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;
    if (ch->desc->original)
      continue;
    if (!IS_NPC(ch) && !ch->desc && !ch->desc->original) {
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
      save_char(ch);
      extract_char(ch);
    }
  }

}
