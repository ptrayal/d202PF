/* ************************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
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
#include "utils.h"
#include "house.h"
#include "constants.h"

/* external functions */
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);

/* local globals */
struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* local functions */
int House_get_filename(room_vnum vnum, char *filename, size_t maxlen);
int House_load(room_vnum vnum);
int House_save(struct obj_data *obj, FILE *fp, int location);
void House_restore_weight(struct obj_data *obj);
void House_delete_file(room_vnum vnum);
int find_house(room_vnum vnum);
void House_save_control(void);
void hcontrol_list_houses(struct char_data *ch);
void hcontrol_build_house(struct char_data *ch, char *arg);
void hcontrol_destroy_house(struct char_data *ch, char *arg);
void hcontrol_pay_house(struct char_data *ch, char *arg);
ACMD(do_hcontrol);
ACMD(do_house);

extern int xap_objs;

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return (0);

  snprintf(filename, maxlen, LIB_HOUSE"%d.house", vnum);
  return (1);
}


/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    House_save(obj->next_content, fp, location);
    House_save(obj->contains, fp, MIN(0, location) - 1);
    result = Obj_to_store(obj, fp, location);
    if (!result)
      return (0);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
  }
  return (1);
}


/* restore weight of containers after House_save has changed them for saving */
void House_restore_weight(struct obj_data *obj)
{
  if (obj) {
    House_restore_weight(obj->contains);
    House_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}


/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
{

  int rnum = 0;
  char buf[MAX_STRING_LENGTH]={'\0'};
  FILE *fp;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;
  if (!House_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    log("SYSERR: Error saving house file: %s", strerror(errno));
    return;
  }
  if (!House_save(world[rnum].contents, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);
  House_restore_weight(world[rnum].contents);
  REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}


/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  return;

  char filename[MAX_INPUT_LENGTH]={'\0'};
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)
      log("SYSERR: Error deleting house file #%d. (1): %s", vnum, strerror(errno));
    return;
  }
  fclose(fl);
  if (remove(filename) < 0)
    log("SYSERR: Error deleting house file #%d. (2): %s", vnum, strerror(errno));
}





/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

int find_house(room_vnum vnum)
{
  int i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return (i);

  return (NOWHERE);
}



/* Save the house control information */
void House_save_control(void)
{
  FILE *fl;

  if (!(fl = fopen(HCONTROL_FILE, "wb"))) 
  {
    log("SYSERR: Unable to open house control file.: %s", strerror(errno));
    fclose(fl);
    return;
  }
  
  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  if (fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl) != (size_t)num_of_houses)
  {
    perror("SYSERR: Unable to save house control file.");
    fclose(fl);
    return;   
  }

  fclose(fl);
}


/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void)
{
  struct house_control_rec temp_house;
  room_rnum real_house, real_atrium;
  FILE *fl;

  memset((char *)house_control,0,sizeof(struct house_control_rec)*MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
    if (errno == ENOENT)
      log("   House control file '%s' does not exist.", HCONTROL_FILE);
    else
      log("SYSERR: " HCONTROL_FILE ": %s", strerror(errno));
    return;
  }
  while (!feof(fl) && num_of_houses < MAX_HOUSES) {
    fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

    if (feof(fl))
      break;

    if (get_name_by_id(temp_house.owner) == NULL)
      continue;			/* owner no longer exists -- skip */

    if ((real_house = real_room(temp_house.vnum)) == NOWHERE)
      continue;			/* this vnum doesn't exist -- skip */

    if (find_house(temp_house.vnum) != NOWHERE)
      continue;			/* this vnum is already a house -- skip */

    if ((real_atrium = real_room(temp_house.atrium)) == NOWHERE)
      continue;			/* house doesn't have an atrium -- skip */

    if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
      continue;			/* invalid exit num -- skip */

    if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
      continue;			/* exit num mismatch -- skip */

    house_control[num_of_houses++] = temp_house;

    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE); 
	SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_load(temp_house.vnum);
  }

  fclose(fl);
  House_save_control();
}



/* "House Control" functions */

const char *HCONTROL_FORMAT =
"Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
"       hcontrol destroy <house vnum>\r\n"
"       hcontrol pay <house vnum>\r\n"
"       hcontrol show\r\n";

void hcontrol_list_houses(struct char_data *ch)
{
  int i = 0;
  char *timestr, *temp;
  char built_on[128]={'\0'}, last_pay[128]={'\0'}, own_name[MAX_NAME_LENGTH + 1]={'\0'};

  if (!num_of_houses) {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(ch,
	"Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
	"-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    if (house_control[i].built_on) {
      timestr = asctime(localtime(&(house_control[i].built_on)));
      *(timestr + 10) = '\0';
      strlcpy(built_on, timestr, sizeof(built_on));
    } else
      strcpy(built_on, "Unknown");	/* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (house_control[i].last_payment) {
      timestr = asctime(localtime(&(house_control[i].last_payment)));
      *(timestr + 10) = '\0';
      strlcpy(last_pay, timestr, sizeof(last_pay));
    } else
      strcpy(last_pay, "None");	/* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp);	/* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(ch, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
	    house_control[i].vnum, house_control[i].atrium, built_on,
	    house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(ch, i, TRUE);
  }
}



void hcontrol_build_house(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH]={'\0'};
  struct house_control_rec temp_house;
  room_vnum virt_house, virt_atrium;
  room_rnum real_house, real_atrium;
  sh_int exit_num;
  long owner;

  if (num_of_houses >= MAX_HOUSES) {
    send_to_char(ch, "Max houses already defined.\r\n");
    return;
  }

  /* first arg: house's vnum */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  virt_house = atoi(arg1);
  if ((real_house = real_room(virt_house)) == NOWHERE) {
    send_to_char(ch, "No such room exists.\r\n");
    return;
  }
  if ((find_house(virt_house)) != NOWHERE) {
    send_to_char(ch, "House already exists.\r\n");
    return;
  }

  /* second arg: direction of house's exit */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((exit_num = search_block(arg1, dirs, FALSE)) < 0 &&
      (exit_num = search_block(arg1, abbr_dirs, FALSE)) < 0) {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
    return;
  }
  if (TOROOM(real_house, exit_num) == NOWHERE) {
    send_to_char(ch, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
    return;
  }

  real_atrium = TOROOM(real_house, exit_num);
  virt_atrium = GET_ROOM_VNUM(real_atrium);

  if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
    send_to_char(ch, "A house's exit must be a two-way door.\r\n");
    return;
  }

  /* third arg: player's name */
  one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((owner = get_id_by_name(arg1)) < 0) {
    send_to_char(ch, "Unknown player '%s'.\r\n", arg1);
    return;
  }

  temp_house.mode = HOUSE_PRIVATE;
  temp_house.vnum = virt_house;
  temp_house.atrium = virt_atrium;
  temp_house.exit_num = exit_num;
  temp_house.built_on = time(0);
  temp_house.last_payment = 0;
  temp_house.owner = owner;
  temp_house.num_of_guests = 0;

  house_control[num_of_houses++] = temp_house;

  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
  SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  House_crashsave(virt_house);

  send_to_char(ch, "House built.  Mazel tov!\r\n");
  House_save_control();
}



void hcontrol_destroy_house(struct char_data *ch, char *arg)
{

  int i = 0, j = 0;
  room_rnum real_atrium, real_house;

  if (!*arg) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((i = find_house(atoi(arg))) == NOWHERE) {
    send_to_char(ch, "Unknown house.\r\n");
    return;
  }
  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", atoi(arg), house_control[i].atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[i].vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house_control[i].vnum);
  else {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE); 
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE); 
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH); 
  }
  
  House_delete_file(house_control[i].vnum);

  for (j = i; j < num_of_houses - 1; j++)
    house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char(ch, "House deleted.\r\n");
  House_save_control();

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < num_of_houses; i++)
    if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
}


void hcontrol_pay_house(struct char_data *ch, char *arg)
{
  int i = 0;

  if (!*arg)
    send_to_char(ch, "%s", HCONTROL_FORMAT);
  else if ((i = find_house(atoi(arg))) == NOWHERE)
    send_to_char(ch, "Unknown house.\r\n");
  else {
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Payment for house %s collected by %s.", arg, GET_NAME(ch));

    house_control[i].last_payment = time(0);
    House_save_control();
    send_to_char(ch, "Payment recorded.\r\n");
  }
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH]={'\0'}, arg2[MAX_INPUT_LENGTH]={'\0'};

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(ch);
  else
    send_to_char(ch, "%s", HCONTROL_FORMAT);
}


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  char arg[MAX_INPUT_LENGTH]={'\0'};
  int i = 0, j = 0, id = 0;

  one_argument(argument, arg);

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    send_to_char(ch, "You must be in your house to set guests.\r\n");
  else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
    send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
  else if (GET_IDNUM(ch) != house_control[i].owner)
    send_to_char(ch, "Only the primary owner can set guests.\r\n");
  else if (!*arg)
    House_list_guests(ch, i, FALSE);
  else if ((id = get_id_by_name(arg)) < 0)
    send_to_char(ch, "No such player.\r\n");
  else if (id == GET_IDNUM(ch))
    send_to_char(ch, "It's your house!\r\n");
  else {
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (house_control[i].guests[j] == id) {
	for (; j < house_control[i].num_of_guests; j++)
	  house_control[i].guests[j] = house_control[i].guests[j + 1];
	house_control[i].num_of_guests--;
	House_save_control();
	send_to_char(ch, "Guest deleted.\r\n");
	return;
      }
    if (house_control[i].num_of_guests == MAX_GUESTS) {
      send_to_char(ch, "You have too many guests.\r\n");
      return;
    }
    j = house_control[i].num_of_guests++;
    house_control[i].guests[j] = id;
    House_save_control();
    send_to_char(ch, "Guest added.\r\n");
  }
}



/* Misc. administrative functions */


/* crash-save all the houses */
void House_save_all(void)
{
  int i;
  room_rnum real_house;

  for (i = 0; i < num_of_houses; i++)
    if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
      if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
	House_crashsave(house_control[i].vnum);
}


/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data *ch, room_vnum house)
{
  int i, j;

  if (GET_ADMLEVEL(ch) >= ADMLVL_GRGOD || (i = find_house(house)) == NOWHERE)
    return (1);

  switch (house_control[i].mode) {
  case HOUSE_PRIVATE:
    if (GET_IDNUM(ch) == house_control[i].owner)
      return (1);
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (GET_IDNUM(ch) == house_control[i].guests[j])
	return (1);
  }

  return (0);
}

void House_list_guests(struct char_data *ch, int i, int quiet)
{
  int j = 0, num_printed = 0;
  char *temp;

  if (house_control[i].num_of_guests == 0) {
    if (!quiet)
      send_to_char(ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(ch, "  Guests: ");

  for (num_printed = j = 0; j < house_control[i].num_of_guests; j++) {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].guests[j])) == NULL)
      continue;

    num_printed++;
    send_to_char(ch, "%c%s ", UPPER(*temp), temp + 1);
  }

  if (num_printed == 0)
    send_to_char(ch, "all dead");

  send_to_char(ch, "\r\n");
}

int House_load(room_vnum rvnum) 
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH] = { '\0' };
  char buf1[MAX_STRING_LENGTH] = { '\0' };
  char buf2[MAX_STRING_LENGTH] = { '\0' };
  char line[256] = { '\0' };
  int t[21],danger,zwei=0;
  struct obj_data *temp;
  int locate=0, j, nr,k,num_objs=0;
  struct obj_data *obj1;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  struct extra_descr_data *new_descr;
  room_rnum rrnum;

  if ((rrnum = real_room(rvnum)) == NOWHERE)
    return 0;

  if (!House_get_filename(rvnum, fname, sizeof(fname)))
    return 0;

  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {  /* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING HOUSE FILE %s (5)", fname);
      log("%s: %s", buf1, strerror(errno));
    }
    return 0;
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
        if ((temp->name = fread_string(fl, buf2)) == NULL) {
          temp->name = "undefined";
        }

        if ((temp->short_description = fread_string(fl, buf2)) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fl, buf2)) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fl, buf2)) == NULL) {
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
        }

        free_extra_descriptions(temp->ex_description);
        temp->ex_description = NULL;

        get_line(fl,line);
        for (k=j=zwei=0;!zwei && !feof(fl);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              new_descr->keyword = fread_string(fl, buf2);
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
        obj_to_room(temp, rrnum);
      } else {
        continue;
      }

/*No need to check if its equipped since rooms can't equip things --firebird_223*/

          for (j = MAX_BAG_ROWS-1;j > -locate;j--)
            if (cont_row[j]) { /* no container -> back to ch's inventory */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_room(cont_row[j], rrnum);
              }
              cont_row[j] = NULL;
            }

          if (j == -locate && cont_row[j]) { /* content list existing */
            if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
              /* take item ; fill ; give to char again */
              obj_from_room(temp);
              temp->contains = NULL;
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_obj(cont_row[j], temp);
              }
              obj_to_room(temp, rrnum); /* add to inv first ... */
            } else { /* object isn't container -> empty content list */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_room(cont_row[j], rrnum);
              }
              cont_row[j] = NULL;
            }
          }

          if (locate < 0 && locate >= -MAX_BAG_ROWS) {
               /* let obj be part of content list
                  but put it at the list's end thus having the items
                  in the same order as before renting */
            obj_from_room(temp);
            if ((obj1 = cont_row[-locate-1])) {
              while (obj1->next_content)
                obj1 = obj1->next_content;
              obj1->next_content = temp;
            } else
              cont_row[-locate-1] = temp;
          }
       } else {
         get_line(fl, line);
      }
    }


  fclose(fl);

    return 1;
}


int is_house_yours(struct char_data *ch, int vnum) 
{
  int i = 0;
  char *temp;
  char own_name[MAX_NAME_LENGTH + 1]={'\0'};

  if (!num_of_houses) 
  {
    return FALSE;
  }

  for (i = 0; i < num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp);	/* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    if (strcmp(CAP(own_name), GET_NAME(ch)) || house_control[i].vnum != vnum)
      continue;
    else
      return TRUE;
  }
  return FALSE;
}

void list_your_houses(struct char_data *ch) 
{
  int i = 0;
  char *timestr, *temp;
  char built_on[128]={'\0'}, last_pay[128]={'\0'}, own_name[MAX_NAME_LENGTH + 1]={'\0'};

  if (!num_of_houses) 
  {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(ch,
	"Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
	"-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    if (house_control[i].built_on) {
      timestr = asctime(localtime(&(house_control[i].built_on)));
      *(timestr + 10) = '\0';
      strlcpy(built_on, timestr, sizeof(built_on));
    } else
      strcpy(built_on, "Unknown");	/* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (house_control[i].last_payment) {
      timestr = asctime(localtime(&(house_control[i].last_payment)));
      *(timestr + 10) = '\0';
      strlcpy(last_pay, timestr, sizeof(last_pay));
    } else
      strcpy(last_pay, "None");	/* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp);	/* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    if (strcmp(CAP(own_name), GET_NAME(ch)))
      continue;
    send_to_char(ch, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
	    house_control[i].vnum, house_control[i].atrium, built_on,
	    house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(ch, i, TRUE);
  }
}

ACMD(do_hpay)
{
  char arg[200]={'\0'};

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Your Houses:\r\n\r\n");
    list_your_houses(ch);
    return;
  }

  int vnum = atoi(arg);

  if (!is_house_yours(ch, vnum)) {
    send_to_char(ch, "That house does not belong to you.\r\n");
    return;
  }

  int hs;

  if ((hs = find_house(vnum)) == NOWHERE) {
    send_to_char(ch, "That is not a valid house vnum.\r\n");
    return;
  }

  if (GET_GOLD(ch) < 5000) {
    send_to_char(ch, "Monthly House and Shop rent is 5000 coins.\r\n");
    return;
  }

  GET_GOLD(ch) -= 5000;

  mudlog(NRM, ADMLVL_IMMORT, TRUE, "Payment for house %s paid by %s.", arg, GET_NAME(ch));

  house_control[hs].last_payment = time(0);
  House_save_control();
  send_to_char(ch, "Payment recorded. Cost was 5000 gold coins.  You will not have to pay rent for another 30 days.\r\n");

}
