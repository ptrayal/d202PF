/* ************************************************************************
*   File: sqlobjsave.c                                  Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save into sql  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <mysql.h>
#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "sqlcommon.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5

/* external variables */
MYSQL *sql_connect (void);
void sql_real_query (char *query);
void sql_disconnect (void);
extern MYSQL *conn;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int min_rent_cost;
extern int max_obj_save;	/* change in config.c */
extern char *coin_name[];

/* Extern functions */
ACMD(do_action);
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int invalid_class(struct char_data *ch, struct obj_data *obj);

/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data *ch, struct char_data *recep, int display, int factor);
int Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);
void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct obj_data *obj, long *cost, long *nitems, int display, int factor);
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
int sql_obj_to_store(struct char_data *ch, struct obj_data *obj, MYSQL *sql, int location);
void update_obj_file(void);
int sql_write_rentcode(struct char_data *ch, long time, int net_cost_per_diem, int rentcode, int gold, int account, int nitems, int mode);
int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode);
int Crash_save(struct char_data *ch, struct obj_data *obj, int location);
void Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost);
void Crash_restore_weight(struct obj_data *obj);
void Crash_extract_objs(struct obj_data *obj);
int Crash_is_unrentable(struct obj_data *obj);
void Crash_extract_norents(struct obj_data *obj);
void Crash_extract_expensive(struct obj_data *obj);
void Crash_calculate_rent(struct obj_data *obj, int *cost);
void Crash_rentsave(struct char_data *ch, int cost);
void Crash_cryosave(struct char_data *ch, int cost);

struct obj_data *sql_obj_from_store(MYSQL_ROW row, int *location)
{
  	struct obj_data *obj;
  	obj_rnum itemnum;
//  	int j;

  	*location = 0;
  	if ((itemnum = real_object(atoi(row[0]))) == NOTHING)
    		return (NULL);

  	obj = read_object(itemnum, REAL);
#if USE_AUTOEQ
  	*location = atoi(row[1]);
#endif
  	GET_OBJ_VAL(obj, 0) 	= atoi(row[2]);
  	GET_OBJ_VAL(obj, 1) 	= atoi(row[3]);
  	GET_OBJ_VAL(obj, 2) 	= atoi(row[4]);
  	GET_OBJ_VAL(obj, 3) 	= atoi(row[5]);
  	GET_OBJ_EXTRA(obj) 	= atoi(row[6]);
  	GET_OBJ_WEIGHT(obj) 	= atoi(row[7]);
  	GET_OBJ_TIMER(obj) 	= atoi(row[8]);
  	GET_OBJ_AFFECT(obj) 	= atoi(row[9]);
/*
  	for (j = 0; j < MAX_OBJ_AFFECT; j++)
    		obj->affected[j] = object.affected[j];
*/
  	return (obj);
}
/*Cannot remove untill after house sql conversion */
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{
  	struct obj_data *obj;
  	obj_rnum itemnum;
  	int j;

  	*location = 0;
  	if ((itemnum = real_object(object.item_number)) == NOTHING)
    		return (NULL);

  	obj = read_object(itemnum, REAL);
#if USE_AUTOEQ
  	*location = object.location;
#endif
  	GET_OBJ_VAL(obj, 0) = object.value[0];
  	GET_OBJ_VAL(obj, 1) = object.value[1];
  	GET_OBJ_VAL(obj, 2) = object.value[2];
  	GET_OBJ_VAL(obj, 3) = object.value[3];
  	GET_OBJ_EXTRA(obj) = object.extra_flags;
  	GET_OBJ_WEIGHT(obj) = object.weight;
  	GET_OBJ_TIMER(obj) = object.timer;
  	GET_OBJ_AFFECT(obj) = object.bitvector;

  	for (j = 0; j < MAX_OBJ_AFFECT; j++)
    		obj->affected[j] = object.affected[j];

  	return (obj);
}

int sql_obj_to_store(struct char_data *ch, struct obj_data *obj, MYSQL *sql, int location)
{
	char query[MAX_STRING_LENGTH];
//  	int j;

	sprintf(query, "INSERT INTO %s SET", mySQL_rent_obj_table);
	sprintf(query, "%s idnum = %ld,", query, GET_IDNUM(ch));
	sprintf(query, "%s item_number = %d,", query, GET_OBJ_VNUM(obj));
#if USE_AUTOEQ
  	sprintf(query, "%s location = %d,", query, location);
#endif
	sprintf(query, "%s obj_val_0 = %d,", query, GET_OBJ_VAL(obj, 0));
	sprintf(query, "%s obj_val_1 = %d,", query, GET_OBJ_VAL(obj, 1));
	sprintf(query, "%s obj_val_2 = %d,", query, GET_OBJ_VAL(obj, 2));
	sprintf(query, "%s obj_val_3 = %d,", query, GET_OBJ_VAL(obj, 3));
	sprintf(query, "%s extra_flags = %d,", query, GET_OBJ_EXTRA(obj));
	sprintf(query, "%s weight = %d,", query, GET_OBJ_WEIGHT(obj));
	sprintf(query, "%s timer = %d", query, GET_OBJ_TIMER(obj));
/*
  	object.bitvector = GET_OBJ_AFFECT(obj);
  	for (j = 0; j < MAX_OBJ_AFFECT; j++)
    		object.affected[j] = obj->affected[j];
*/
	sql_real_query(query);
	/* Debugging statement */
//	send_to_char(ch,query);
  	return (1);
}

/*Cannot remove untill after house sql conversion */
int Obj_to_store(struct obj_data *obj, FILE *fl, int location)
{
  	int j;
  	struct obj_file_elem object;

  	object.item_number = GET_OBJ_VNUM(obj);
#if USE_AUTOEQ
  	object.location = location;
#endif
  	object.value[0] = GET_OBJ_VAL(obj, 0);
  	object.value[1] = GET_OBJ_VAL(obj, 1);
  	object.value[2] = GET_OBJ_VAL(obj, 2);
  	object.value[3] = GET_OBJ_VAL(obj, 3);
  	object.extra_flags = GET_OBJ_EXTRA(obj);
  	object.weight = GET_OBJ_WEIGHT(obj);
  	object.timer = GET_OBJ_TIMER(obj);
  	object.bitvector = GET_OBJ_AFFECT(obj);
  	for (j = 0; j < MAX_OBJ_AFFECT; j++)
    		object.affected[j] = obj->affected[j];

  	if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1) {
    		log("%s: SYSERR: error writing object in Obj_to_store", strerror(errno));
    		return (0);
  	}
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
    			case WEAR_LIGHT:
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
    			case WEAR_SHIELD:
      				if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        				location = LOC_INVENTORY;
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
    			case WEAR_WIELD:
      				if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        				location = LOC_INVENTORY;
      			break;
    			case WEAR_HOLD:
      				if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
			break;
      				if (IS_WARRIOR(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
			break;
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
        			mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        			location = LOC_INVENTORY;
      			}
    		}
  	}
  	if (location <= 0)	/* Inventory */
    		obj_to_char(obj, ch);
}

void Crash_listrent(struct char_data *ch, char *name)
{
  	FILE *fl;
  	char filename[MAX_INPUT_LENGTH];
  	struct obj_file_elem object;
  	struct obj_data *obj;
  	struct rent_info rent;
  	int numread;

  	if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
   		return;
  	if (!(fl = fopen(filename, "rb"))) {
    		send_to_char(ch, "%s has no rent file.\r\n", name);
    		return;
  	}
  	numread = fread(&rent, sizeof(struct rent_info), 1, fl);

  	/* Oops, can't get the data, punt. */
  	if (numread == 0) {
    		send_to_char(ch, "Error reading rent information.\r\n");
    		fclose(fl);
    		return;
  	}

  	send_to_char(ch, "%s\r\n", filename);
  	switch (rent.rentcode) {
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
  	while (!feof(fl)) {
    		fread(&object, sizeof(struct obj_file_elem), 1, fl);
    		if (ferror(fl)) {
      			fclose(fl);
      			return;
    		}
    		if (!feof(fl))
      			if (real_object(object.item_number) != NOTHING) {
				obj = read_object(object.item_number, VIRTUAL);
#if USE_AUTOEQ
				send_to_char(ch, " [%5d] (%5dau) <%2d> %-20s\r\n",
				object.item_number, GET_OBJ_RENT(obj),
				object.location, obj->short_description);
#else
				send_to_char(ch, " [%5d] (%5dau) %-20s\r\n",
				object.item_number, GET_OBJ_RENT(obj),
				obj->short_description);
#endif
				extract_obj(obj);
      			}
  	}
  	fclose(fl);
}

int sql_write_rentcode(struct char_data *ch, long time, int net_cost_per_diem, int rentcode, int gold, int account, int nitems, int mode)
{
	char query[MAX_INPUT_LENGTH];
	int sqlmode=0;
	MYSQL_RES *result;
	
	if (IS_NPC(ch))
		return(0);
		
	if (mode == MOD_INSERT) {
		sprintf(query, "SELECT idnum FROM %s WHERE idnum = %ld", mySQL_rent_table, GET_IDNUM(ch));
		sql_connect();
		sql_real_query(query);
		result = mysql_store_result(conn);
		
		if (mysql_num_rows(result)==0)
			sqlmode=SQL_INSERT;
		else
			sqlmode=SQL_REPLACE;

		mysql_free_result(result);
		sql_disconnect();
	}
	
	if (sqlmode == SQL_INSERT) {
		sprintf(query, "INSERT INTO %s SET", mySQL_rent_table);
	}
	
	if (sqlmode == SQL_REPLACE) {
		sprintf(query, "REPLACE INTO %s SET", mySQL_rent_table);
	}
	
	if (sqlmode == SQL_INSERT || sqlmode == SQL_REPLACE) {
		sprintf(query, "%s idnum = %ld,", 		query, GET_IDNUM(ch));
		sprintf(query, "%s time = %ld,", 		query, time);
		sprintf(query, "%s net_cost_per_diem = %d,", 	query, net_cost_per_diem);
		sprintf(query, "%s rentcode = %d,", 		query, rentcode);
		sprintf(query, "%s gold = %d,", 		query, gold);
		sprintf(query, "%s account = %d,", 		query, account);
		sprintf(query, "%s nitems = %d", 		query, nitems);
		
		sql_connect();
		sql_real_query(query);
		sql_disconnect();
		/* Debugging statement */
//		send_to_char(ch, query);
	}
	return (1);
}

/*
 * Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $)
 */
int sql_rent_load(struct char_data *ch)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
  	char query[MAX_STRING_LENGTH];
  	struct rent_info rent;
  	int cost, orig_rent_code, num_objs = 0, j, n, i;
  	int rentcode, nitems;
  	float num_of_days;
  	/* AutoEQ addition. */
  	struct obj_data *obj, *obj2, *cont_row[MAX_BAG_ROWS];
  	int location;
  	
  	/* Empty all of the container lists (you never know ...) */
  	for (j = 0; j < MAX_BAG_ROWS; j++)
    		cont_row[j] = NULL;
    		
	sprintf(query, "SELECT time, rentcode, net_cost_per_diem, gold, account, nitems FROM %s WHERE idnum = %ld", mySQL_rent_table, GET_IDNUM(ch));
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
		
	if (mysql_num_rows(result)==0) {
		mysql_free_result(result);
		sql_disconnect();
		log("SQLERROR: No rent file for %s(%ld) found.", GET_PC_NAME(ch), GET_IDNUM(ch));
      		send_to_char(ch,
		"\r\n&R********************* &WNOTICE &R*********************&n\r\n"
		"&WThere was a problem loading your rentfile.\r\n"
		"Contact a God for assistance.&n\r\n");
		mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game with no equipment.", GET_NAME(ch));
		return(1);
	}
	
	row = mysql_fetch_row(result);
	
	rentcode=atoi(row[1]);
	nitems	=atoi(row[5]);
	
	if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT) {
		num_of_days = (float) (time(0) - atoi(row[0])) / SECS_PER_REAL_DAY;
    		cost = (int) (rent.net_cost_per_diem * num_of_days);
    		GET_GOLD(ch) = (COINS(ch)[0] + (COINS(ch)[1] * 10) + (COINS(ch)[2] * 100) + (COINS(ch)[3] * 1000) + (COINS(ch)[4] * 10000));
		 if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
		 	mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game, rented equipment lost (no $).", GET_NAME(ch));
      			Crash_crashsave(ch);
      			mysql_free_result(result);
			sql_disconnect();
      			return (2);
      		} else {
      			GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      			GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      		}
      	}
      	switch (orig_rent_code = rentcode) {
		case RENT_RENTED:
    			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s un-renting and entering game.", GET_NAME(ch));
		break;
  		case RENT_CRASH:
    			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    		break;
  		case RENT_CRYO:
    			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    		break;
  		case RENT_FORCED:
  		case RENT_TIMEDOUT:
    			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    		break;
  		default:
    			mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), rent.rentcode);
    		break;
  	}
  	
  	if (nitems == 0) {
  		mysql_free_result(result);
		sql_disconnect();
  	} else {
  		mysql_free_result(result);
  		sprintf(query, "SELECT item_number, location, obj_val_0, obj_val_1, obj_val_2, obj_val_3, extra_flags, weight, timer, bitvector, affects FROM %s WHERE idnum = %ld",
  			mySQL_rent_obj_table, GET_IDNUM(ch));
  		sql_real_query(query);
  		result = mysql_store_result(conn);
		while ((row = mysql_fetch_row(result))) {
			++num_objs;
    			if ((obj = sql_obj_from_store(row, &location)) == NULL)
      				continue;
      				
      			auto_equip(ch, obj, location);

    			if (location > 0) {		/* Equipped */
      				for (j = MAX_BAG_ROWS - 1; j > 0; j--) {
        				if (cont_row[j]) {	/* No container, back to inventory. */
          					for (; cont_row[j]; cont_row[j] = obj2) {
            						obj2 = cont_row[j]->next_content;
            						obj_to_char(cont_row[j], ch);
          					}
          					cont_row[j] = NULL;
        				}
      				}
      				if (cont_row[0]) {	/* Content list existing. */
        				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) { /* Remove object, fill it, equip again. */
          					obj = unequip_char(ch, location - 1);
          					obj->contains = NULL;	/* Should be NULL anyway, but just in case. */
          					for (; cont_row[0]; cont_row[0] = obj2) {
            						obj2 = cont_row[0]->next_content;
            						obj_to_obj(cont_row[0], obj);
          					}
          					equip_char(ch, obj, location - 1);
        				} else {			/* Object isn't container, empty the list. */
          					for (; cont_row[0]; cont_row[0] = obj2) {
            						obj2 = cont_row[0]->next_content;
            						obj_to_char(cont_row[0], ch);
          					}
          					cont_row[0] = NULL;
        				}
      				}
    			} else {	/* location <= 0 */
      				for (j = MAX_BAG_ROWS - 1; j > -location; j--) {
        				if (cont_row[j]) {	/* No container, back to inventory. */
          					for (; cont_row[j]; cont_row[j] = obj2) {
            						obj2 = cont_row[j]->next_content;
            						obj_to_char(cont_row[j], ch);
          					}
          					cont_row[j] = NULL;
        				}
      				}
      				if (j == -location && cont_row[j]) {	/* Content list exists. */
        				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) { /* Take the item, fill it, and give it back. */
          					obj_from_char(obj);
          					obj->contains = NULL;
          					for (; cont_row[j]; cont_row[j] = obj2) {
            						obj2 = cont_row[j]->next_content;
            						obj_to_obj(cont_row[j], obj);
          					}
          					obj_to_char(obj, ch);	/* Add to inventory first. */
        				} else {	/* Object isn't container, empty content list. */
          					for (; cont_row[j]; cont_row[j] = obj2) {
            						obj2 = cont_row[j]->next_content;
            						obj_to_char(cont_row[j], ch);
          					}
          					cont_row[j] = NULL;
        				}
      				}
      				if (location < 0 && location >= -MAX_BAG_ROWS) {
        				obj_from_char(obj);
        				if ((obj2 = cont_row[-location - 1]) != NULL) {
          					while (obj2->next_content)
            						obj2 = obj2->next_content;
          					obj2->next_content = obj;
        				} else
          					cont_row[-location - 1] = obj;
      				}
    			}
	
		}
		sql_disconnect();
  		mudlog(NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE, "%s (level %d) has %d object%s (max %d).",
		GET_NAME(ch), GET_LEVEL(ch), num_objs, num_objs != 1 ? "s" : "", max_obj_save);
	}
  		for (i = 0, n = 0; i < NUM_WEARS; i++) {
    			if (GET_EQ(ch, i))
      				n++;
		}
  		nitems = n + IS_CARRYING_N(ch);
    		/* sql_write_rentcode(ch, time, net_cost_per_diem, rentcode, gold, account, nitems, mode) */
  		sql_write_rentcode(ch, time(0), -1, RENT_CRASH, -1, -1, nitems, MOD_INSERT);

 		if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    			return (0);
  		else
    			return (1);	
  		
}

/* Function to delete the items in the rent_obj table */
void crash_delete_objs(struct char_data *ch)
{
	char query[MAX_STRING_LENGTH];
	
	sql_connect();
    	sprintf(query, "DELETE FROM %s WHERE idnum = %ld", mySQL_rent_obj_table, GET_IDNUM(ch));
  	sql_real_query(query);

	/* Debugging statement */
//	send_to_char(ch,query);	
	sql_disconnect();
	
}

/* Function to delete a rentfile and rent objects */
void crash_delete_rent(struct char_data *ch)
{
	char buf[MAX_STRING_LENGTH], query[MAX_STRING_LENGTH];

	sql_connect();
    	sprintf(query, "DELETE FROM %s WHERE idnum = %ld", mySQL_rent_table, GET_IDNUM(ch));
  	sql_real_query(query);

	/* Debugging statement */
//	send_to_char(ch,query);	
	sql_disconnect();
	
	crash_delete_objs(ch);
	sprintf(buf,"Deleted RentFile For %s",GET_NAME(ch));
	mudlog(NRM, LVL_IMMORT, TRUE, buf);
}

int Crash_save(struct char_data *ch, struct obj_data *obj, int location)
{
  	struct obj_data *tmp;
  	int result;

  	if (obj) {
    		Crash_save(ch, obj->next_content, location);
    		Crash_save(ch, obj->contains, MIN(0, location) - 1);
    		sql_connect();
    		result = sql_obj_to_store(ch, obj, conn, location);
		sql_disconnect();

    		for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    		if (!result)
      			return (0);
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

  	if (OBJ_FLAGGED(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 || GET_OBJ_RNUM(obj) == NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY)
    		return (1);

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


void Crash_crashsave(struct char_data *ch)
{
  	struct rent_info rent;
  	int j,n,i;

  	if (IS_NPC(ch))
    		return;

  	rent.rentcode = RENT_CRASH;
  	rent.time = time(0);
  	for (i = 0, n = 0; i < NUM_WEARS; i++) {
    		if (GET_EQ(ch, i))
      			n++;
	}
  	rent.nitems = n + IS_CARRYING_N(ch);
  	
  	/* sql_write_rentcode(ch, time, net_cost_per_diem, rentcode, gold, account, nitems, mode) */
  	if (!sql_write_rentcode(ch, rent.time, -1,  rent.rentcode, -1, -1, rent.nitems, MOD_INSERT))
  		return;
  		
	crash_delete_objs(ch);

  	for (j = 0; j < NUM_WEARS; j++)
    		if (GET_EQ(ch, j)) {
      			if (!Crash_save(ch, GET_EQ(ch, j), j + 1))
				return;

      			Crash_restore_weight(GET_EQ(ch, j));
    		}

  	if (!Crash_save(ch, ch->carrying, 0)) {
    		return;
  	}
  	Crash_restore_weight(ch->carrying);

  	REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_idlesave(struct char_data *ch)
{
  	struct rent_info rent;
  	int j, n, i;
  	int cost, cost_eq;

  	if (IS_NPC(ch))
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

  	GET_GOLD(ch) = (COINS(ch)[0] + (COINS(ch)[1] * 10) + (COINS(ch)[2] * 100) + (COINS(ch)[3] * 1000) + (COINS(ch)[4] * 10000));

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
      				crash_delete_objs(ch);
      				return;
    			}
  	}
  	rent.net_cost_per_diem = cost;

  	GET_GOLD(ch) = (COINS(ch)[0] + (COINS(ch)[1] * 10) + (COINS(ch)[2] * 100) + (COINS(ch)[3] * 1000) + (COINS(ch)[4] * 10000));


  	rent.rentcode = RENT_TIMEDOUT;
  	rent.time = time(0);
  	rent.gold = GET_GOLD(ch);
  	rent.account = GET_BANK_GOLD(ch);
   	for (i = 0, n = 0; i < NUM_WEARS; i++) {
    		if (GET_EQ(ch, i))
     			n++;
	}
  	rent.nitems = n + IS_CARRYING_N(ch);
  	
  	/* sql_write_rentcode(ch, time, net_cost_per_diem, rentcode, gold, account, nitems, mode) */
    	if (!sql_write_rentcode(ch, rent.time, rent.net_cost_per_diem, rent.rentcode, rent.gold, rent.account, rent.nitems, MOD_INSERT))
  		return;

	crash_delete_objs(ch);

  	for (j = 0; j < NUM_WEARS; j++) {
    		if (GET_EQ(ch, j)) {
      			if (!Crash_save(ch, GET_EQ(ch, j), j + 1)) {
       				return;
      			}
      			Crash_restore_weight(GET_EQ(ch, j));
      			Crash_extract_objs(GET_EQ(ch, j));
    		}
  	}
  	if (!Crash_save(ch, ch->carrying, 0))
    		return;

  	Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data *ch, int cost)
{
  	struct rent_info rent;
  	int j, n, i;

  	if (IS_NPC(ch))
    	return;

  	Crash_extract_norent_eq(ch);
  	Crash_extract_norents(ch->carrying);

  	GET_GOLD(ch) = (COINS(ch)[0] + (COINS(ch)[1] * 10) + (COINS(ch)[2] * 100) + (COINS(ch)[3] * 1000) + (COINS(ch)[4] * 10000));

  	rent.net_cost_per_diem = cost;
  	rent.rentcode = RENT_RENTED;
  	rent.time = time(0);
  	rent.gold = GET_GOLD(ch);
  	rent.account = GET_BANK_GOLD(ch);
    	for (i = 0, n = 0; i < NUM_WEARS; i++) {
    		if (GET_EQ(ch, i))
      			n++;
	}
  	rent.nitems = n + IS_CARRYING_N(ch);

  	/* sql_write_rentcode(ch, time, net_cost_per_diem, rentcode, gold, account, nitems, mode) */
    	if (!sql_write_rentcode(ch, rent.time, rent.net_cost_per_diem, rent.rentcode, rent.gold, rent.account, rent.nitems, MOD_INSERT))
  		return;

	crash_delete_objs(ch);

  	for (j = 0; j < NUM_WEARS; j++)
    		if (GET_EQ(ch, j)) {
      			if (!Crash_save(ch, GET_EQ(ch,j), j + 1)) {
        			return;
      			}
      		Crash_restore_weight(GET_EQ(ch, j));
      		Crash_extract_objs(GET_EQ(ch, j));
    		}
  	if (!Crash_save(ch, ch->carrying, 0)) {
    		return;
  	}
  	
  	Crash_extract_objs(ch->carrying);
}


void Crash_cryosave(struct char_data *ch, int cost)
{
  	struct rent_info rent;
  	int j, i, n;

  	if (IS_NPC(ch))
    		return;

  	Crash_extract_norent_eq(ch);
  	Crash_extract_norents(ch->carrying);

  	GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  	rent.rentcode = RENT_CRYO;
  	rent.time = time(0);
  	rent.gold = GET_GOLD(ch);
  	rent.account = GET_BANK_GOLD(ch);
  	rent.net_cost_per_diem = 0;
    	for (i = 0, n = 0; i < NUM_WEARS; i++) {
    		if (GET_EQ(ch, i))
      			n++;
	}
  	rent.nitems = n + IS_CARRYING_N(ch);

  	/* sql_write_rentcode(ch, time, net_cost_per_diem, rentcode, gold, account, nitems, mode) */
    	if (!sql_write_rentcode(ch, rent.time, rent.net_cost_per_diem, rent.rentcode, rent.gold, rent.account, rent.nitems, MOD_INSERT))
  		return;
  	
  	crash_delete_objs(ch);
  	
  	for (j = 0; j < NUM_WEARS; j++)
    		if (GET_EQ(ch, j)) {
      			if (!Crash_save(ch, GET_EQ(ch, j), j + 1))
        			return;
      			
      			Crash_restore_weight(GET_EQ(ch, j));
      			Crash_extract_objs(GET_EQ(ch, j));
    		}
  	if (!Crash_save(ch, ch->carrying, 0))
    		return;

  	Crash_extract_objs(ch->carrying);
  	SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data *ch, struct char_data *recep,
			      long cost)
{
  	char buf[256];
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
      			char buf[128];

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
        			char buf[256];

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

  	totalcost = min_rent_cost * factor;

  	Crash_report_rent(ch, recep, ch->carrying, &totalcost, &numitems, display, factor);

  	for (i = 0; i < NUM_WEARS; i++)
    		Crash_report_rent(ch, recep, GET_EQ(ch, i), &totalcost, &numitems, display, factor);
/*  removed to force rent even if no items -jdr
  	if (!numitems) {
    		act("$n tells you, 'But you are not carrying anything!  Just quit!'",
			FALSE, recep, 0, ch, TO_VICT);
    		return (0);
  	}
*/
  	if (numitems > max_obj_save) {
    		char buf[256];

    		snprintf(buf, sizeof(buf), "$n tells you, 'Sorry, but I cannot store more than %d items.'", max_obj_save);
    		act(buf, FALSE, recep, 0, ch, TO_VICT);
    		return (0);
  	}
  	if (display) {
    		char buf[256];

    		snprintf(buf, sizeof(buf), "$n tells you, 'Plus, my %d %s fee..'", min_rent_cost * factor, coin_name[0]);
    		act(buf, FALSE, recep, 0, ch, TO_VICT);

    		snprintf(buf, sizeof(buf), "$n tells you, 'For a total of %ld %s %s.'", totalcost, coin_name[0],
			factor == RENT_FACTOR ? " per day" : "");
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
  	const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp", "cough", "fart", "twiddle", "yawn" };

  	if (!cmd && !rand_number(0, 5)) {
    		do_action(recep, NULL, find_command(action_table[rand_number(0, 8)]), 0);
    		return (FALSE);
  	}

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

  	if (free_rent) {
    		act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
			FALSE, recep, 0, ch, TO_VICT);
    		return (1);
  	}

  	if (CMD_IS("rent")) {
    		char buf[128];

    		if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      			return (TRUE);
    		if (mode == RENT_FACTOR)
      			snprintf(buf, sizeof(buf), "$n tells you, 'Rent will cost you %d %s per day.'", cost, coin_name[0]);
    		else if (mode == CRYO_FACTOR)
      			snprintf(buf, sizeof(buf), "$n tells you, 'It will cost you %d %s to be frozen.'", cost, coin_name[0]);
    		act(buf, FALSE, recep, 0, ch, TO_VICT);

    		GET_GOLD(ch) = (COINS(ch)[0] + (COINS(ch)[1] * 10) + (COINS(ch)[2] * 100) + (COINS(ch)[3] * 1000) + (COINS(ch)[4] * 10000));

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
      			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has rented (%d/day, %d tot.)",
				GET_NAME(ch), cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
    		} else {			/* cryo */
      			act("$n stores your belongings and helps you into your private chamber.\r\n"
	  			"A white mist appears in the room, chilling you to the bone...\r\n"
	  			"You begin to lose consciousness...",
	  		FALSE, recep, 0, ch, TO_VICT);
      			Crash_cryosave(ch, cost);
      			mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has cryo-rented.", GET_NAME(ch));
      			SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
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
  	struct descriptor_data *d;
  	for (d = descriptor_list; d; d = d->next) {
    		if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      			if (PLR_FLAGGED(d->character, PLR_CRASH)) {
				Crash_crashsave(d->character);
				save_char(d->character);
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      			}
    		}
  	}
}
