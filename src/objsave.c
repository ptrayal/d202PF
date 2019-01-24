/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "feats.h"
#include "utils.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	2
#define CRYO_FACTOR 	1

#define LOC_INVENTORY	0

/* external variables */
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int xap_objs;

/* Extern functions */
ACMD(do_action);
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int set_object_level(struct obj_data *obj);

/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data *ch, struct char_data *recep, int display, int factor);
int Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);
void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct obj_data *obj, long *cost, long *nitems, int display, int factor);
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
void update_obj_file(void);
int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode);
int Crash_save(struct obj_data *obj, FILE *fp, int location);
void Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost);
void Crash_restore_weight(struct obj_data *obj);
void Crash_extract_objs(struct obj_data *obj);
int Crash_is_unrentable(struct obj_data *obj);
void Crash_extract_norents(struct obj_data *obj);
void Crash_extract_expensive(struct obj_data *obj);
void Crash_calculate_rent(struct obj_data *obj, int *cost);
void Crash_rentsave(struct char_data *ch, int cost);
void Crash_cryosave(struct char_data *ch, int cost);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);



int Obj_to_store(struct obj_data *obj, FILE *fl, int location)
{
    my_obj_save_to_disk(fl, obj, location);
    return (1);
}

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */
void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) {	/* Was wearing it. */
    switch (j = (location - 1)) {
    case WEAR_UNUSED0:
      j = WEAR_WIELD2;
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
        location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
        location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
        location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
        location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
        location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
        location = LOC_INVENTORY;
      break;
    case WEAR_UNUSED1:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      j = WEAR_WIELD2;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD1:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD2:
      break;
    case WEAR_MASK:
      if (!CAN_WEAR(obj, ITEM_WEAR_MASK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BACKPACK:
      if (!CAN_WEAR(obj, ITEM_WEAR_PACK))
        location = LOC_INVENTORY;
      break;
    case WEAR_WINGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_WINGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_EAR_R:
    case WEAR_EAR_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_EAR))
        location = LOC_INVENTORY;
      break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0) {	    /* Wearable. */
      if (!GET_EQ(ch,j)) {
	/*
	 * Check the characters's alignment to prevent them from being
	 * zapped through the auto-equipping.
         */
         if (invalid_align(ch, obj) || invalid_class(ch, obj))
          location = LOC_INVENTORY;
        else
          equip_char(ch, obj, j);
      } else {	/* Oops, saved a player with double equipment? */
        mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0)	/* Inventory */
    obj_to_char(obj, ch);
}


int Crash_delete_file(char *name)
{
  char filename[50]={'\0'};
  FILE *fl;

   //if (!xap_objs) {
     //if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
       //return (0);
   //} else {
     if (!get_filename(filename, sizeof(filename), OLD_OBJ_FILES, name))
       return (0);
   //}

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)	/* if it fails but NOT because of no file */
      log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return (0);
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return (1);
}


int Crash_delete_crashfile(struct char_data *ch)
{
  char filename[MAX_INPUT_LENGTH]={'\0'};
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[MAX_INPUT_LENGTH]={'\0'};

  if(!get_filename(filename, sizeof(filename), OLD_OBJ_FILES, GET_NAME(ch)))
    return (0);

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)	/* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return (0);
  }

  if (!feof(fl))
    get_line(fl,line);
  sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,&gold,
         &account,&nitems);
  fclose(fl);

  if (rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return (1);
}

int Crash_clean_file(char *name)
{
  char filename[MAX_STRING_LENGTH]={'\0'};
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[MAX_STRING_LENGTH]={'\0'};

  if (!get_filename(filename, sizeof(filename), OLD_OBJ_FILES, name))
    return (0);

  if (!(fl = fopen(filename, "r+b"))) {
    if (errno != ENOENT)	/* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return (0);
  }

  if (!feof(fl)) {
    get_line(fl,line);
    sscanf(line, "%d %d %d %d %d %d",&rentcode,&timed,&netcost,
           &gold,&account,&nitems);
    fclose(fl);

    if ((rentcode == RENT_CRASH) ||
        (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
      if (timed < time(0) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
        const char *filetype;

        Crash_delete_file(name);
        switch (rentcode) {
        case RENT_CRASH:
          filetype = "crash";
          break;
        case RENT_FORCED:
          filetype = "forced rent";
          break;
        case RENT_TIMEDOUT:
          filetype = "idlesave";
          break;
        default:
          filetype = "UNKNOWN!";
          break;
        }
        log("    Deleting %s's %s file.", name, filetype);
        return (1);
      }
      /* Must retrieve rented items w/in 30 days */
    } else if (rentcode == RENT_RENTED)
      if (timed < time(0) - (CONFIG_RENT_TIMEOUT * SECS_PER_REAL_DAY)) {
        Crash_delete_file(name);
        log("    Deleting %s's rent file.", name);
        return (1);
      }
  }
  return (0);
}


void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
      Crash_clean_file(player_table[i].name);
}


void Crash_listrent(struct char_data *ch, char *name)
{
  FILE *fl=NULL;
  char filename[MAX_STRING_LENGTH]={'\0'};
  char buf[MAX_STRING_LENGTH]={'\0'};
  struct obj_data *obj;
  int rentcode, timed, netcost, gold, account, nitems, len;
  int t[10],nr;
  char line[MAX_STRING_LENGTH]={'\0'};
  char *sdesc;

  if (get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    fl = fopen(filename, "rb");

  if (!fl) {
    send_to_char(ch, "%s has no rent file.\r\n", name);
    return;
  }

  send_to_char(ch, "%s\r\n", filename);

  if (!feof(fl)) {
    get_line(fl,line);
    sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost, 
           &gold,&account,&nitems); 
  }

  switch (rentcode) {
  case RENT_RENTED:
    send_to_char(ch, "Rent\r\n");
    break;
  case RENT_CRASH:
    send_to_char(ch, "Crash\r\n");
    break;
  case RENT_CRYO:
    send_to_char(ch, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    send_to_char(ch, "TimedOut\r\n");
    break;
  default:
    send_to_char(ch, "Undef\r\n");
    break;
  }
  buf[0] = 0;
  len = 0;

  while(!feof(fl)) {
    get_line(fl,line);
    if(*line == '#') { /* swell - its an item */
      sscanf(line,"#%d",&nr);
      if(nr != NOTHING) {  /* then we can dispense with it easily */
        if (real_object(nr) != NOTHING) {
          obj=read_object(nr,VIRTUAL);
          if (len + 255 < sizeof(buf)) {
            len += snprintf(buf + len, sizeof(buf) - len, "[%5d] (%5dau) %-20s\r\n",
                            nr, GET_OBJ_RENT(obj), obj->short_description);
          } else {
            snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
            break;
          }
          extract_obj(obj);
        } else {
          if (len + 255 < sizeof(buf)) {
            len += snprintf(buf + len, sizeof(buf) - len,
                            "%s[-----] NONEXISTANT OBJECT #%d\r\n",buf, nr);
          } else {
            snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
            break;
          }
        }
      } else { /* its nothing, and a unique item. bleh. partial parse.*/
        get_line(fl,line);    /* this is obj+val */
        get_line(fl,line);    /* this is XAP */
        fread_string(fl,", listrent reading name");  /* screw the name */
        sdesc=fread_string(fl,", listrent reading sdesc");
        fread_string(fl,", listrent reading desc"); /* screw the long desc */
        fread_string(fl,", listrent reading adesc"); /* screw the action desc. */
        get_line(fl,line);    /* this is an important line.rent..*/
        sscanf(line,"%d %d %d %d %d",t,t+1,t+2,t+3,t+4);
        /* great we got it all, make the buf */
        if (len + 255 < sizeof(buf)) {
          len += snprintf(buf + len, sizeof(buf) - len,
                          "%s[%5d] (%5dau) %-20s\r\n",buf, nr, t[4],sdesc);
        } else {
          snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
          break;
        }
        /* best of all, we don't care if there's descs, or stuff..*/
        /* since we're only doing operations on lines beginning in # */
        /* i suppose you don't want to make exdescs start with # .:) */
      }
    }
  }

  page_string(ch->desc,buf,0);
  fclose(fl);
}


int Crash_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);
    result = Obj_to_store(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return (FALSE);
  }
  return (TRUE);
}


void Crash_restore_weight(struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j) == NULL)
      continue;

    if (Crash_is_unrentable(GET_EQ(ch, j)))
      obj_to_char(unequip_char(ch, j), ch);
    else
      Crash_extract_norents(GET_EQ(ch, j));
  }
}

void Crash_extract_objs(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data *obj)
{
  if (!obj)
    return (0);
#if CIRCLE_UNSIGNED_INDEX
  if (OBJ_FLAGGED(obj, ITEM_NORENT) ||
       GET_OBJ_RNUM(obj) == NOTHING || 
          (GET_OBJ_RNUM(obj) == NOTHING &&
          !IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))) {
    return (1);
#else
  if (OBJ_FLAGGED(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 ||
       GET_OBJ_RNUM(obj) <= NOTHING || 
          (GET_OBJ_RNUM(obj) <= NOTHING &&
          !IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))) {
    return (1);
#endif
   }
  return (0);
}


void Crash_extract_norents(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}


void Crash_extract_expensive(struct obj_data *obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data *obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj));
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}


void Crash_crashsave(struct char_data *ch, int backup)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (backup) {
    if (!get_filename(buf, sizeof(buf), BACKUP_OBJ_FILES, GET_NAME(ch)))
      return;
  }
  else {
    if (!get_filename(buf, sizeof(buf), NEW_OBJ_FILES, GET_NAME(ch)))
      return;
  }

  if (!(fp = fopen(buf, "wb")))
    return;

  fprintf(fp, "%d %d %d %d %d %d\r\n", RENT_CRASH, (int)time(0), 0, GET_GOLD(ch),
          GET_BANK_GOLD(ch), 0);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
	fclose(fp);
	return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }

  Crash_restore_weight(ch->carrying);

  fclose(fp);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_idlesave(struct char_data *ch)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  int j;
  int cost, cost_eq;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), NEW_OBJ_FILES, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);

  cost_eq = 0;
  for (j = 0; j < NUM_WEARS; j++)
    Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);

  cost += cost_eq;
  cost *= 2;			/* forcerent cost is 2x normal rent */

  if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
    for (j = 0; j < NUM_WEARS; j++)	/* Unequip players with low gold. */
      if (GET_EQ(ch, j))
        obj_to_char(unequip_char(ch, j), ch);

    while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
      Crash_extract_expensive(ch->carrying);
      cost = 0;
      Crash_calculate_rent(ch->carrying, &cost);
      cost *= 2;
    }
  }

  if (ch->carrying == NULL) {
    for (j = 0; j < NUM_WEARS && GET_EQ(ch, j) == NULL; j++) /* Nothing */ ;
    if (j == NUM_WEARS) {	/* No equipment or inventory. */
      fclose(fp);
      Crash_delete_file(GET_NAME(ch));
      return;
    }
  }

  fprintf(fp,"%d %d %d %d %d %d\r\n", RENT_TIMEDOUT, (int)time(0), cost,
          GET_GOLD(ch),GET_BANK_GOLD(ch), 0);

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data *ch, int cost)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), NEW_OBJ_FILES, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  fprintf(fp,"%d %d %d %d %d %d\r\n", RENT_RENTED, (int)time(0), cost,
          GET_GOLD(ch), GET_BANK_GOLD(ch), 0);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch,j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_cryosave(struct char_data *ch, int cost)
{
  char buf[MAX_INPUT_LENGTH]={'\0'};
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  fprintf(fp,"%d %d %d %d %d %d\r\n", RENT_CRYO, (int)time(0), 0, GET_GOLD(ch),
          GET_BANK_GOLD(ch), 0);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data *ch, struct char_data *recep,
			      long cost)
{
  char buf[256]={'\0'};
  long rent_deadline;

  if (!cost)
    return;

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  snprintf(buf, sizeof(buf), "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
	  "on hand and in the bank.'\r\n", rent_deadline, rent_deadline != 1 ? "s" : "");
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data *ch, struct char_data *recep,
			         struct obj_data *obj)
{
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      char buf[128]={'\0'};

      has_norents = 1;
      snprintf(buf, sizeof(buf), "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}



void Crash_report_rent(struct char_data *ch, struct char_data *recep,
		            struct obj_data *obj, long *cost, long *nitems, int display, int factor)
{
  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
      *cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
      if (display) {
        char buf[256]={'\0'};

	snprintf(buf, sizeof(buf), "$n tells you, '%5d coins for %s..'", GET_OBJ_RENT(obj) * factor, OBJS(obj, ch));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}



int Crash_offer_rent(struct char_data *ch, struct char_data *recep,
		         int display, int factor)
{
  int i;
  long totalcost = 0, numitems = 0, norent;

  norent = Crash_report_unrentables(ch, recep, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, recep, GET_EQ(ch, i));

  if (norent)
    return (0);

  totalcost = CONFIG_MIN_RENT_COST * factor;

  Crash_report_rent(ch, recep, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, recep, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
	FALSE, recep, 0, ch, TO_VICT);
    return (0);
  }
  if (numitems > CONFIG_MAX_OBJ_SAVE) {
    char buf[256]={'\0'};

    snprintf(buf, sizeof(buf), "$n tells you, 'Sorry, but I cannot store more than %d items.'", CONFIG_MAX_OBJ_SAVE);
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
    char buf[256]={'\0'};

    snprintf(buf, sizeof(buf), "$n tells you, 'Plus, my %d coin fee..'", CONFIG_MIN_RENT_COST * factor);
    act(buf, FALSE, recep, 0, ch, TO_VICT);

    snprintf(buf, sizeof(buf), "$n tells you, 'For a total of %ld coins%s.'", totalcost, factor == RENT_FACTOR ? " per day" : "");
    act(buf, FALSE, recep, 0, ch, TO_VICT);

    if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'", FALSE, recep, 0, ch, TO_VICT);
      return (0);
    } else if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, recep, totalcost);
  }
  return (totalcost);
}



int gen_receptionist(struct char_data *ch, struct char_data *recep,
		         int cmd, char *arg, int mode)
{
  int cost;
//  const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
//	  "cough", "fart", "twiddle", "yawn" };

//  if (!cmd && !rand_number(0, 5)) {
//    do_action(recep, NULL, find_command(action_table[rand_number(0, 8)]), 0);
//    return (FALSE);
//  }

  if (!ch->desc || IS_NPC(ch))
    return (FALSE);

  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return (FALSE);

  if (!AWAKE(recep)) {
    send_to_char(ch, "%s is unable to talk to you...\r\n", HSSH(recep));
    return (TRUE);
  }

  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return (TRUE);
  }

  if (CONFIG_FREE_RENT) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return (1);
  }

  if (CMD_IS("rent")) {
    char buf[128]={'\0'};

    if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      return (TRUE);
    if (mode == RENT_FACTOR)
      snprintf(buf, sizeof(buf), "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
    else if (mode == CRYO_FACTOR)
      snprintf(buf, sizeof(buf), "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
    act(buf, FALSE, recep, 0, ch, TO_VICT);

    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return (TRUE);
    }
    if (cost && (mode == RENT_FACTOR))
      Crash_rent_deadline(ch, recep, cost);

    if (mode == RENT_FACTOR) {
      act("$n stores your belongings and helps you into your private chamber.", FALSE, recep, 0, ch, TO_VICT);
      Crash_rentsave(ch, cost);
      mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has rented (%d/day, %d tot.)",
		GET_NAME(ch), cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
    } else {			/* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n",
//	  "A white mist appears in the room, chilling you to the bone...\r\n"
//	  "You begin to lose consciousness...",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_cryosave(ch, cost);
      mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has rented.", GET_NAME(ch));
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRYO);
    }

    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
    extract_char(ch);	/* It saves. */
  } else {
    Crash_offer_rent(ch, recep, TRUE, mode);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return (TRUE);
}


SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}


void Crash_save_all(void)
{
  extern int circle_shutdown;
  extern int circle_reboot;
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character, FALSE);
	save_char(d->character);
	if(circle_shutdown) {
	  if(circle_reboot) {
	     add_llog_entry(d->character,LAST_REBOOT);
          } else {
             add_llog_entry(d->character,LAST_SHUTDOWN);
	  }
        }
	REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}

int Crash_load(struct char_data *ch, int backup) 
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH]={'\0'};
  char buf1[MAX_STRING_LENGTH]={'\0'}, buf2[MAX_STRING_LENGTH]={'\0'};
  char line[256]={'\0'};
  int t[21],danger,zwei=0,num_of_days;
  int orig_rent_code;
  struct obj_data *temp;
  int locate=0, j, nr,k,cost,num_objs=0;
  struct obj_data *obj1;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  struct extra_descr_data *new_descr;
  int rentcode, timed, netcost, gold, account, nitems;


  if (backup) {
    if (!get_filename(fname, sizeof(fname), BACKUP_OBJ_FILES, GET_NAME(ch)))
      return 1;
  }
  else {
    if (!get_filename(fname, sizeof(fname), NEW_OBJ_FILES, GET_NAME(ch)))
      return 1;
  }

  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING OBJECT FILE %s (5)", fname);
      log("%s: %s", buf1, strerror(errno));
      send_to_char(ch, 
		   "\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n");
    }
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game with no equipment.", GET_NAME(ch));
    return 1;
  }

  if (!feof(fl))
    get_line(fl,line);

  sscanf(line,"%d %d %d %d %d %d",&rentcode, &timed,
        &netcost,&gold,&account,&nitems);

  if ((rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT)  && CONFIG_FREE_RENT != YES) {
    num_of_days = (float) (time(0) - timed) / SECS_PER_REAL_DAY;
    cost = (int) (netcost * num_of_days);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      fclose(fl);
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game, rented equipment lost (no $).", GET_NAME(ch));
      Crash_crashsave(ch, FALSE);
      return 2;
    } else {
      GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      save_char(ch);
    }
  }
  switch (orig_rent_code = rentcode) {
  case RENT_RENTED:
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s un-renting and entering game.", GET_NAME(ch));
    break;
  case RENT_CRASH:
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    break;
  case RENT_CRYO:
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s un-renting and entering game.", GET_NAME(ch));
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    break;
  default:
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    break;
  }

  for (j = 0;j < MAX_BAG_ROWS;j++)
    cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

  if(!feof(fl))
    get_line(fl, line);

  while (!feof(fl)) {
        temp=NULL;
        /* first, we get the number. Not too hard. */
    if(*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) {   /* then it is unique */
        temp = create_obj();
        temp->item_number=NOTHING;
        GET_OBJ_SIZE(temp) = SIZE_MEDIUM;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 999999) 
          continue;
        temp=read_object(nr,VIRTUAL);
        if (!temp) {
	  get_line(fl, line);
          continue;
        }
      }

      get_line(fl,line);

      sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, 
                   t + 10, t + 11, t + 12, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18, t + 19, t + 20);
      locate=t[0];
      GET_OBJ_VAL(temp,0) = t[1];
      GET_OBJ_VAL(temp,1) = t[2];
      GET_OBJ_VAL(temp,2) = t[3];
      GET_OBJ_VAL(temp,3) = t[4];
      GET_OBJ_VAL(temp,4) = t[5];
      GET_OBJ_VAL(temp,5) = t[6];
      GET_OBJ_VAL(temp,6) = t[7];
      GET_OBJ_VAL(temp,7) = t[8];
      GET_OBJ_EXTRA(temp)[0] = t[9];
      GET_OBJ_EXTRA(temp)[1] = t[10];
      GET_OBJ_EXTRA(temp)[2] = t[11];
      GET_OBJ_EXTRA(temp)[3] = t[12];
      GET_OBJ_VAL(temp,8) = t[13];
      GET_OBJ_VAL(temp,9) = t[14];
      GET_OBJ_VAL(temp,10) = t[15];
      GET_OBJ_VAL(temp,11) = t[16];
      GET_OBJ_VAL(temp,12) = t[17];
      GET_OBJ_VAL(temp,13) = t[18];
      GET_OBJ_VAL(temp,14) = t[19];
      GET_OBJ_VAL(temp,15) = t[20];

      get_line(fl,line);
       /* read line check for xap. */
      if(!strcmp("XAP",line)) {  /* then this is a Xap Obj, requires
                                       special care */
        if ((temp->name = fread_string(fl, "rented object name")) == NULL) {
          temp->name = "undefined";
        }

        if ((temp->short_description = fread_string(fl, "rented object short desc")) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fl, "rented object desc")) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fl, "rented object adesc")) == NULL) {
          temp->action_description=0;
        }

        if (!get_line(fl, line) ||
           (sscanf(line, "%d %d %d %d %d %d %d %d", t,t+1,t+2,t+3,t+4,t+5,t+6,t+7) != 8)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return 0;
        }
        temp->type_flag = t[0];
        temp->wear_flags[0] = t[1];
        temp->wear_flags[1] = t[2];
        temp->wear_flags[2] = t[3];
        temp->wear_flags[3] = t[4];
        temp->weight = t[5];
        temp->cost = t[6];
        GET_OBJ_LEVEL(temp) = t[7];

        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          temp->affected[j].location = APPLY_NONE;
          temp->affected[j].modifier = 0;
          temp->affected[j].specific = 0;
        }

        free_extra_descriptions(temp->ex_description);
        temp->ex_description = NULL;

        get_line(fl,line);
        for (k=j=zwei=0;!zwei && !feof(fl);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              sprintf(buf2, "rented object edesc keyword for object #%d", nr);
              new_descr->keyword = fread_string(fl, buf2);
              sprintf(buf2, "rented object edesc text for object #%d keyword %s", nr, new_descr->keyword);
              new_descr->description = fread_string(fl, buf2);
              new_descr->next = temp->ex_description;
              temp->ex_description = new_descr;
              get_line(fl,line);
              break;

            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                log("SYSERR: Too many object affectations in loading rent file");
                danger=1;
              }
              get_line(fl, line);
              sscanf(line, "%d %d %d", t, t + 1, t + 2);

              temp->affected[j].location = t[0];
              temp->affected[j].modifier = t[1];  
              temp->affected[j].specific = t[2];  
              j++;
              //GET_OBJ_LEVEL(temp) = set_object_level(temp);
              get_line(fl,line);
              break;
            case 'V':

              get_line(fl,line);
              sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9,
                           t + 10, t + 11, t + 12, t + 13, t + 14, t + 15);
              GET_OBJ_VAL(temp,16) = t[0];
              GET_OBJ_VAL(temp,17) = t[1];
              GET_OBJ_VAL(temp,18) = t[2];
              GET_OBJ_VAL(temp,19) = t[3];
              GET_OBJ_VAL(temp,20) = t[4];
              GET_OBJ_VAL(temp,21) = t[5];
              GET_OBJ_VAL(temp,22) = t[6];
              GET_OBJ_VAL(temp,23) = t[7];
              GET_OBJ_VAL(temp,24) = t[8];
              GET_OBJ_VAL(temp,25) = t[9];
              GET_OBJ_VAL(temp,26) = t[10];
              GET_OBJ_VAL(temp,27) = t[11];
              GET_OBJ_VAL(temp,28) = t[12];
              GET_OBJ_VAL(temp,29) = t[13];
              GET_OBJ_VAL(temp,30) = t[14];
              GET_OBJ_VAL(temp,31) = t[15];
              get_line(fl, line);
              break;
            case 'G':
              get_line(fl, line);
              sscanf(line, "%ld", (long int*)&temp->generation);
              get_line(fl, line);
              break;
            case 'U':
              get_line(fl, line);
              sscanf(line, "%lld", &temp->unique_id);
              get_line(fl, line);
              break;
            case 'S':
              if (j >= SPELLBOOK_SIZE) {
                log("SYSERR: Too many spells in spellbook loading rent file");
                danger=1;
              }
              get_line(fl, line);
              sscanf(line, "%d %d", t, t + 1);

              if (!temp->sbinfo) {
                CREATE(temp->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
                memset((char *) temp->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
              }
              temp->sbinfo[j].spellname = t[0];
              temp->sbinfo[j].pages = t[1];  
              j++;
              get_line(fl,line);
              break;
            case 'Z':
              get_line(fl, line);
              sscanf(line, "%d", (int *)&GET_OBJ_SIZE(temp));
              get_line(fl, line);
              break;
            case '$':
            case '#':
              zwei=1;
              break;
            default:
              zwei=1;
              break;
          }
        }      /* exit our for loop */
      }   /* exit our xap loop */
      if(temp != NULL) {
        num_objs++;
        check_unique_id(temp);
        add_unique_id(temp);
        if (GET_OBJ_TYPE(temp) == ITEM_DRINKCON) {
          name_from_drinkcon(temp);
          if (GET_OBJ_VAL(temp, 1) != 0 )
            name_to_drinkcon(temp, GET_OBJ_VAL(temp, 2));
        }

        auto_equip(ch, temp, locate);
      } else {
        continue;
      }
 /* 
   what to do with a new loaded item:

   if there's a list with <locate> less than 1 below this:
     (equipped items are assumed to have <locate>==0 here) then its
     container has disappeared from the file   *gasp*
      -> put all the list back to ch's inventory
   if there's a list of contents with <locate> 1 below this:
     check if it's a container
     - if so: get it from ch, fill it, and give it back to ch (this way the
         container has its correct weight before modifying ch)
     - if not: the container is missing -> put all the list to ch's inventory

   for items with negative <locate>:
     if there's already a list of contents with the same <locate> put obj to it
     if not, start a new list

 Confused? Well maybe you can think of some better text to be put here ...

 since <locate> for contents is < 0 the list indices are switched to
 non-negative
 */

        if (locate > 0) { /* item equipped */
          for (j = MAX_BAG_ROWS-1;j > 0;j--)
            if (cont_row[j]) { /* no container -> back to ch's inventory */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
              }
              cont_row[j] = NULL;
            }
          if (cont_row[0]) { /* content list existing */
            if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
              /* rem item ; fill ; equip again */
              temp = unequip_char(ch, locate-1);
              if (temp != NULL)
                temp->contains = NULL; /* should be empty - but who knows */
              for (;cont_row[0];cont_row[0] = obj1) {
                obj1 = cont_row[0]->next_content;
                obj_to_obj(cont_row[0], temp);
              }
              equip_char(ch, temp, locate-1);
            } else { /* object isn't container -> empty content list */
              for (;cont_row[0];cont_row[0] = obj1) {
                obj1 = cont_row[0]->next_content;
                obj_to_char(cont_row[0], ch);
              }
              cont_row[0] = NULL;
            }
          }
        } else { /* locate <= 0 */
          for (j = MAX_BAG_ROWS-1;j > -locate;j--)
            if (cont_row[j]) { /* no container -> back to ch's inventory */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
              } 
              cont_row[j] = NULL;
            }

          if (j == -locate && cont_row[j]) { /* content list existing */
            if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
              /* take item ; fill ; give to char again */
              obj_from_char(temp);
              temp->contains = NULL;
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_obj(cont_row[j], temp);
              }
              obj_to_char(temp, ch); /* add to inv first ... */
            } else { /* object isn't container -> empty content list */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
              }
              cont_row[j] = NULL;
            }
          }

          if (locate < 0 && locate >= -MAX_BAG_ROWS) {
               /* let obj be part of content list
                  but put it at the list's end thus having the items
                  in the same order as before renting */
            obj_from_char(temp);
            if ((obj1 = cont_row[-locate-1])) {
              while (obj1->next_content)
                obj1 = obj1->next_content;
              obj1->next_content = temp;
            } else
              cont_row[-locate-1] = temp;
          }
        } /* locate less than zero */
       } else {
         get_line(fl, line);
      }
    }

  /* Little hoarding check. -gg 3/1/98 */
  mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s (level %d) has %d object%s (max %d).", 
  GET_NAME(ch), GET_LEVEL(ch), num_objs, num_objs != 1 ? "s" : "", CONFIG_MAX_OBJ_SAVE);

  fclose(fl);

  Crash_crashsave(ch, FALSE);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}
