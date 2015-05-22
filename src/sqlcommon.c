/* Database Interface File */
/* Created for gates of krynn mud */
/* by. Jason Ragsdale (2002) */

/* Common Includes */
#include <mysql.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "oasis.h"
#include "sqlcommon.h"
#include "boards.h"
#include "dg_scripts.h"

/* Local File Function Includes */
extern int no_mail;
MYSQL *conn;
void Crash_crashsave(struct char_data *ch);
void init_intro(struct char_data *ch);
void crash_delete_rent(struct char_data *ch);
extern ush_int DFLT_PORT;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern long top_idnum;
extern const char *pc_race_types[];
extern const char *genders[];
extern const char *deity_names[];
extern int top_of_p_table;


/* Error Handler */
void sql_print_error (MYSQL *conn,char *message)
{
	char buf[MAX_STRING_LENGTH];
	
	sprintf (buf,"%s\n", message);
	mudlog(NRM, LVL_IMMORT, TRUE, buf);
	
	if (conn != NULL)
	{
		sprintf (buf,"Error %u (%s)\n",
			mysql_errno (conn),mysql_error (conn));
		mudlog(NRM, LVL_IMMORT, TRUE, buf);
	}
}

/* Connection Handler */
MYSQL *sql_connect (void)
{
	char temp[300];
	
	conn = mysql_init (NULL);
	
	if (conn == NULL)
	{
		sql_print_error (NULL, "mysql_init() failed (probably out of memory)\n");
		return (NULL);
	}
	
	if (DFLT_PORT == 4000)
		sprintf(temp,"gok_4000");
	else 
		sprintf(temp,"gok_4400");
	
	if (mysql_real_connect (conn,mySQL_host,mySQL_user,mySQL_pass,temp,mySQL_port,mySQL_socket,0) == NULL)
	{
		sql_print_error (conn, "mysql_real_connect() failed:\nError %u (%s)\n");
		exit(1);
	}
	
	return (conn);
}

/* Disconnection Handler */
void sql_disconnect (void)
{
#ifdef CIRCLE_UNIX
	char buf[MAX_STRING_LENGTH];
#endif	

	if (conn != NULL) {
		mysql_close(conn);
		conn = NULL;
	} else {
#ifdef CIRCLE_UNIX
		sprintf(buf, "SQL: Error : mysql_close on NULL connection at (%s)%s:%d.", __FILE__, __FUNCTION__, __LINE__);
		mudlog(NRM, LVL_IMMORT, TRUE, buf);
#endif
	}
}

/*Query Handler */
void sql_real_query (char *query)
{	
#ifdef CIRCLE_UNIX
	char buf[MAX_STRING_LENGTH];
#endif
	int sqlerror=0;
	
	if (mysql_real_query(conn, query, strlen(query))) {
    		sqlerror = mysql_errno(conn);
#ifdef CIRCLE_UNIX
			sprintf(buf, "SQL: Error With Query at (%s)%s:%d: %s (Query: %s)", __FILE__, __FUNCTION__, __LINE__, mysql_error(conn), query);
    		mudlog(NRM, LVL_IMMORT, TRUE, buf);
#endif
	}
}

/* escape wrapper */
char *sql_escape(MYSQL *sql, const char *str)
{
	static char sql_buffer[MAX_STRING_LENGTH*4+1];
	const char *top = sql_buffer + (MAX_STRING_LENGTH * 4);
	static char *ptr = sql_buffer;
	static char *rvl;
	int string_length = 0;	

	if (!str || !*str)
	  string_length = 0;
	else
	  string_length = strlen(str);

	if (ptr + string_length > top)
		ptr = sql_buffer;

	rvl = ptr;
	ptr += mysql_real_escape_string(sql, rvl, str, string_length) + 1;
	return (rvl);
}


/* Unused at this time
long sql_get_ptable_by_name(char *name)
{
 	int recs,i;
	MYSQL_RES *result;
  	MYSQL_ROW row;

  	one_argument(name, arg);
  	
  	for (i = 0; i <= top_of_p_table; i++)
    		if (!stcmp(player_table[i].name, arg))
      			return (i);

  	return (-1);
}

*/

long sql_get_id_by_name(char *name)
{
  	int recs;
  	long id;
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	char buf[MAX_STRING_LENGTH];
  	
  	sprintf(buf,"SELECT idnum FROM player_info WHERE name = '%s' AND deleted = 0",name);

	sql_connect();

	sql_real_query(buf);

	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		return (-1);
	}

	if (recs > 1) {
		sprintf(buf,"SQLERROR: More than one entry found in the index for %s.", name);
		log(buf);
		mysql_free_result(result);
		sql_disconnect();
		return (-1);
	} 
	
	row = mysql_fetch_row(result);
	id = atoi(row[0]);
	mysql_free_result(result);
	sql_disconnect();
	return (id);	
}

char *sql_get_name_by_id(long id)
{
	
  	int recs, mode=0;
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	char buf[MAX_STRING_LENGTH];
  	
  	if (conn == NULL) {
  		sql_connect();
  		mode = 1;
  	}
  	
  	sprintf(buf,"SELECT name FROM player_info WHERE idnum = '%ld' AND deleted = 0", id);

	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		return (NULL);
	}

	if (recs > 1) {
		sprintf(buf,"SQLERROR: More than one entry found in the index for %ld.", id);
		log(buf);
	return (NULL);
		} 
	
	row = mysql_fetch_row(result);
	mysql_free_result(result);
	
	if (mode == 1) {
  		sql_disconnect();
  	}
  	
	return (row[0]);



}

/*
int sql_find_name(char *name)
{
	int recs,id;
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	
  	sprintf(buf,"SELECT idnum FROM player_info WHERE name = '%s'",name);

	sql_connect();

	sql_real_query(buf);

	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		return (-1);
	}

	if (recs > 1) {
		sprintf(buf,"SQLERROR: More than one entry found in the index for %s.", name);
		log(buf);
		return (-1);
	} 
	
	row = mysql_fetch_row(result);
	id = atoi(row[0]);
	mysql_free_result(result);
	return (id);		
}
*/

/* Function to check last update time of a file */
void sql_login_check(struct char_data *ch)
{
	char buf[MAX_STRING_LENGTH];
	int recs,no,i;
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	
  	sprintf(buf, "SELECT name, last_update FROM %s WHERE last_update < date_sub(current_date,interval 1 month)",mySQL_player_table);
	sql_connect();
	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	sprintf(buf, "The following Players have not logged in for 1 month:\r\n");
	sprintf(buf, "%s=====================================================\r\n",buf);
	sprintf(buf, "%s Name         Last File Update\r\n",buf);
	sprintf(buf, "%s=====================================================\r\n",buf);
	
	for (i = 1,no = 1; no <= recs; i++) {
		row = mysql_fetch_row(result);
		sprintf(buf,"%s%-13s %-25s%s",buf,row[0],row[1],no++ % 1 == 0 ? "\r\n" : " ");
    	}
    	
    	if (no % 1 != 1)
  		sprintf(buf,"%s\r\n",buf);
  		
    	sprintf(buf,"%s    %d total.\r\n",buf,recs);
    	
	mysql_free_result(result);
	sql_disconnect();
    	
  	send_to_char(ch,buf);
}

/* Function to list players in the database */
void sql_list_players (struct char_data *ch)
{
	char buf[MAX_STRING_LENGTH], query[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int count=0;
  	
  	sprintf(query,"SELECT idnum,name,level,last_update FROM %s ORDER BY idnum ASC",mySQL_player_table);
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	sprintf(buf,"&W------------  SQL Player List ------------------&n\r\n");
	sprintf(buf,"%s&WIdnum - Name     -  Level -  Last File Update&n\r\n",buf);
	sprintf(buf,"%s&W------------------------------------------------&n\r\n",buf); 
	while ((row = mysql_fetch_row(result))) {
		sprintf(buf,"%s&M  %-3s   %-10s   %-2s      %-10s&n\r\n",
			buf,row[0],row[1],row[2],row[3]);
		count++;
		}
	sprintf(buf,"%s            &WTotal Player Count : &M%d&n\r\n",buf,count);
	mysql_free_result(result);
	sql_disconnect();
	
	/* Removed by jragsdale
	sprintf(buf,"%s&W--------Player Table----------------&n\r\n",buf);
	for (i = 0; i <= top_of_p_table; i++) {
	sprintf(buf,"%s&M  %-3ld   %-10s&n\r\n",
		buf,player_table[i].id,player_table[i].name);
	}
	*/
	
	send_to_char(ch,buf);
//	page_string(ch->desc, buf, 0);
	send_to_char(ch, "\r\n");
}	

/* Function to modify the Affects tables */
void sql_modify_db_aff (struct char_data *ch, int mode)
{
	char buf[MAX_STRING_LENGTH];
	struct affected_type *aff, af;
  	MYSQL_RES *result;
  	MYSQL_ROW row;

	if (mode == MOD_INSERT) {
		/* DELETE current entrys just so we dont have dups */
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_aff_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
		/* Only insert if we have affects */
		if(ch->affected) {
			/*Build the affected array*/
			aff=ch->affected;
			sprintf(buf, "INSERT INTO %s (idnum,type,duration,modifier,location,bitvector) VALUES",mySQL_aff_table);
			while (aff) {
				sprintf(buf+strlen(buf), "%s%ld,%d,%d,%d,%d,%ld)", aff==ch->affected ? "(" : ",(", GET_IDNUM(ch), aff->type, aff->duration, aff->modifier, aff->location, aff->bitvector);
      				aff=aff->next;
      			}
      			if (buf[strlen(buf)-1]==')')

      			/* Debugging statement */
//			send_to_char(ch,buf);

			sql_connect();
			sql_real_query(buf);
			sql_disconnect();
			/*Remove the affects*/
//			while (ch->affected) {
//				affect_remove(ch, ch->affected);
//			}
		}
	}
	
	if (mode == MOD_RETRIEVE) {
		sprintf(buf, "SELECT type FROM %s WHERE idnum = %ld",mySQL_aff_table,GET_IDNUM(ch));
		sql_connect();

      		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_real_query(buf);
		result = mysql_store_result(conn);
		/*Check if any rows were returned*/
		if (mysql_num_rows(result)==0) {
			mysql_free_result(result);
			sql_disconnect();
			return;
		} else {
			mysql_free_result(result);
			sprintf(buf, "SELECT type,duration,modifier,location,bitvector FROM %s WHERE idnum = %ld",mySQL_aff_table,GET_IDNUM(ch));

      			/* Debugging statement */
//			send_to_char(ch,buf);

			sql_real_query(buf);
			result = mysql_store_result(conn);
			/*build a struct af then apply to ch*/
			while ((row = mysql_fetch_row(result))) {
				af.type=atoi(row[0]);
				af.duration=atoi(row[1]);
				af.modifier=atoi(row[2]);
				af.location=atoi(row[3]);
				af.bitvector=atoi(row[4]);
				affect_to_char (ch, &af);
			}
			mysql_free_result(result);
			sql_disconnect();
		}
	}
	
	if (mode == MOD_DELETE) {
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_aff_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
}

/* Function to modify the spells database */
void sql_modify_db_spells (struct char_data *ch, int mode)
{
	char buf[MAX_STRING_LENGTH];

  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_INSERT))
  		return;
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_RETRIEVE))
    	  	return;
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_DELETE))
  		return;
	
	if (mode == MOD_INSERT) {
		/* DELETE current entrys just so we dont have dups */
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_spell_table,GET_IDNUM(ch));
	
		/* Debugging statement */
//		send_to_char(ch,buf);
	
		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
	
	if (mode == MOD_RETRIEVE) {
		
	}
	
	if (mode == MOD_DELETE) {
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_spell_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);
		
		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
}

/* Function to modify the intro database */
void sql_modify_db_intro (struct char_data *ch, int mode)
{
	char buf[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int i;

  	/* we dont save or delete immortal skills just set to all on when loading */
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_INSERT))
  		return;
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_RETRIEVE)) {
  		INTROS(ch)[0] = -1;
    	  	return;
  	}
  	
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_DELETE))
  		return;

    	  	

	if (mode == MOD_INSERT) {
		/* DELETE current entrys just so we dont have dups */
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_intro_table,GET_IDNUM(ch));
		
		/* Debugging statement */
//		send_to_char(ch,buf);
		
		sql_connect();
		sql_real_query(buf);
		
		for (i = 0; i < MAX_INTROS; i++) {
			if (INTROS(ch)[i] == -1)
				break;
			
			sprintf(buf,"INSERT INTO %s SET",mySQL_intro_table);
			sprintf(buf,"%s idnum = %ld,",buf,GET_IDNUM(ch));
			sprintf(buf,"%s known_idnum = %ld",buf,INTROS(ch)[i]);

			/*debugging statement*/
///			send_to_char(ch,buf);

			sql_real_query(buf);
		}
		sql_disconnect();
	}
	
	if (mode == MOD_RETRIEVE) {
		sprintf(buf, "SELECT idnum FROM %s WHERE idnum = %ld",mySQL_intro_table,GET_IDNUM(ch));
		sql_connect();
		
      		/* Debugging statement */
///		send_to_char(ch,buf);

		sql_real_query(buf);
		result = mysql_store_result(conn);

		if (mysql_num_rows(result)==0) {
			mysql_free_result(result);
			sql_disconnect();
			return;
		} else {
			mysql_free_result(result);
			sprintf(buf, "SELECT known_idnum FROM %s WHERE idnum = %ld",mySQL_intro_table,GET_IDNUM(ch));
			
      			/* Debugging statement */
///			send_to_char(ch,buf);

			sql_real_query(buf);
			result = mysql_store_result(conn);
			/*apply to ch*/
			i = 0;
			while ((row = mysql_fetch_row(result))) {
				INTROS(ch)[i] = atoi(row[0]);
				i++;
			}
			INTROS(ch)[i] = -1;
			
			mysql_free_result(result);
			sql_disconnect();
		}
	}
		
	if (mode == MOD_DELETE) {
		init_intro(ch);
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_intro_table,GET_IDNUM(ch));

		/* Debugging statement */
///		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
}

/* Function to modify the feats database */
void sql_modify_db_feats (struct char_data *ch, int mode)
{
	char buf[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int i;

  	/* we dont save or delete immortal skills just set to all on when loading */
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_INSERT))
  		return;
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_RETRIEVE)) {
  		for (i = 0; i <= NUM_FEATS_DEFINED; i++)
      	    		HAS_FEAT(ch, i) = TRUE;
    	  	return;
    	 }
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_DELETE))
  		return;

    	  	

	if (mode == MOD_INSERT) {
		/* DELETE current entrys just so we dont have dups */
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_feat_table,GET_IDNUM(ch));
		
		/* Debugging statement */
//		send_to_char(ch,buf);
		
		sql_connect();
		sql_real_query(buf);
		/*insert the feats the player knows*/
		for (i = 1; i <= NUM_FEATS_DEFINED; i++) {
			if (HAS_FEAT(ch,i)) {
				/*Build sql string*/
				sprintf(buf,"INSERT INTO %s SET",mySQL_feat_table);
				sprintf(buf,"%s idnum = %ld,",buf,GET_IDNUM(ch));
				sprintf(buf,"%s id = %d,",buf,i);
				sprintf(buf,"%s learned = %d",buf,HAS_FEAT(ch,i));

				/*debugging statement*/
//				send_to_char(ch,buf);

				sql_real_query(buf);
			}
		}
		sql_disconnect();
	}
	
	if (mode == MOD_RETRIEVE) {
		sprintf(buf, "SELECT id FROM %s WHERE idnum = %ld",mySQL_feat_table,GET_IDNUM(ch));
		sql_connect();
		
      		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_real_query(buf);
		result = mysql_store_result(conn);
		/*Check if any rows were returned*/
		if (mysql_num_rows(result)==0) {
			mysql_free_result(result);
			sql_disconnect();
			return;
		} else {
			mysql_free_result(result);
			sprintf(buf, "SELECT id,learned FROM %s WHERE idnum = %ld",mySQL_feat_table,GET_IDNUM(ch));
			
      			/* Debugging statement */
//			send_to_char(ch,buf);

			sql_real_query(buf);
			result = mysql_store_result(conn);
			/*apply to ch*/
			while ((row = mysql_fetch_row(result))) {
				SET_FEAT(ch, atoi(row[0]), atoi(row[1]));
			}
			
			mysql_free_result(result);
			sql_disconnect();
		}
	}
		
	if (mode == MOD_DELETE) {
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_feat_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
}

/* Function to modify the skills database */
void sql_modify_db_skills (struct char_data *ch, int mode)
{
	char buf[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int i;
  	
  	/* we dont save or delete immortal skills just set to all on when loading */
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_INSERT))
  		return;
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_RETRIEVE)) {
  		for (i = 0; i <= MAX_SKILLS; i++) {
      	    		GET_SKILL(ch, i) = 100;
      	    		GET_SKILL_BONUS(ch, i) = 2;
    	  	}
    	  	return;
    	 }
  		
  	if ((GET_LEVEL(ch) >= LVL_IMMORT) && (mode == MOD_DELETE))
  		return;
  	
	if (mode == MOD_INSERT) {
		/* DELETE current entrys just so we dont have dups */
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_skill_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		/*insert the feats the player knows*/
		for (i = 1; i <= MAX_SKILLS; i++) {
			if (GET_SKILL(ch,i)) {
				sprintf(buf,"INSERT INTO %s SET",mySQL_skill_table);
				sprintf(buf,"%s idnum = %ld,",buf,GET_IDNUM(ch));
				sprintf(buf,"%s id = %d,",buf,i);
				sprintf(buf,"%s rank = %d,",buf,GET_SKILL(ch,i));
				sprintf(buf,"%s bonus = %d",buf,GET_SKILL_BONUS(ch,i));

				/*debugging statement*/
//				send_to_char(ch,buf);

				sql_real_query(buf);
			}
		}
		sql_disconnect();
	}
	
	if (mode == MOD_RETRIEVE) {
		sprintf(buf, "SELECT id FROM %s WHERE idnum = %ld",mySQL_skill_table,GET_IDNUM(ch));
		sql_connect();

    		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_real_query(buf);
		result = mysql_store_result(conn);
		if (mysql_num_rows(result)==0) {
			mysql_free_result(result);
			sql_disconnect();
			return;
		} else {
			mysql_free_result(result);
			sprintf(buf, "SELECT id,rank,bonus FROM %s WHERE idnum = %ld",mySQL_skill_table,GET_IDNUM(ch));
			
      			/* Debugging statement */
///			send_to_char(ch,buf);
			
			sql_real_query(buf);
			result = mysql_store_result(conn);
			/*apply to ch*/
			while ((row = mysql_fetch_row(result))) {
				SET_SKILL(ch, atoi(row[0]), atoi(row[1]));
				SET_SKILL_BONUS(ch, atoi(row[0]), atoi(row[2]));
			}
			mysql_free_result(result);
			sql_disconnect();
		}
	}
		
	if (mode == MOD_DELETE) {
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_skill_table,GET_IDNUM(ch));

		/* Debugging statement */
//		send_to_char(ch,buf);

		sql_connect();
		sql_real_query(buf);
		sql_disconnect();
	}
}

/* Function to build the player index (player_i) */
void sql_build_player_index (void)
{
	char buf[MAX_STRING_LENGTH];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int nr = -1, i;
	long recs;
	
	sprintf(buf,"SELECT idnum,name FROM %s",mySQL_player_table);
	sql_connect();
	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	if (recs<0) {
		sprintf(buf,"ERROR: SELECT for all players returned %ld rows",recs);
		log(buf);
	}
	if (recs) {
		sprintf(buf,"  %ld players in SQL database.",recs);
		log(buf);
		CREATE (player_table, struct player_index_element, recs);
	} else {
		player_table = NULL;
		top_of_p_table = -1;
		mysql_free_result(result);
		sql_disconnect();
		return;
	}
	while ((row = mysql_fetch_row(result))) {
		nr++;
		CREATE(player_table[nr].name, char, strlen(row[1]) + 1);
		for (i = 0; (*(player_table[nr].name + i) = LOWER(*(row[1] + i))); i++);
		player_table[nr].id = atoi(row[0]);
		top_idnum = MAX(top_idnum, player_table[nr].id);
	}
	mysql_free_result(result);
	sql_disconnect();

	top_of_p_table = nr;
}

/* Stat command to look at player stats from the DB */
void sql_stat_file_char(struct char_data *ch, char *name) {
	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	my_ulonglong rows;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	/* When adding items to score please add at the bottom of this string */
	sprintf(buf,"SELECT");
    	sprintf(buf,"%s idnum,",buf);		//0
	sprintf(buf,"%s name,",buf);
	sprintf(buf,"%s short_descr,",buf);
	sprintf(buf,"%s long_descr,",buf);
	sprintf(buf,"%s description,",buf);
	sprintf(buf,"%s keywords,",buf);	//5
	sprintf(buf,"%s poofin,",buf);
	sprintf(buf,"%s poofout,",buf);
	sprintf(buf,"%s title,",buf);
	sprintf(buf,"%s clan,",buf);
	sprintf(buf,"%s clan_rank,",buf);	//10
	sprintf(buf,"%s religion,",buf);
	sprintf(buf,"%s email,",buf);
	sprintf(buf,"%s rppoints,",buf);
	sprintf(buf,"%s darts,",buf);
	sprintf(buf,"%s abil_points,",buf);	//15
	sprintf(buf,"%s level,",buf);
	sprintf(buf,"%s hometown,",buf);
	sprintf(buf,"%s weight,",buf);
	sprintf(buf,"%s height,",buf);
	sprintf(buf,"%s sex,",buf);		//20
	sprintf(buf,"%s class,",buf);
	sprintf(buf,"%s race,",buf);
	sprintf(buf,"%s birth,",buf);
	sprintf(buf,"%s played,",buf);
	sprintf(buf,"%s passwd,",buf);		//25
	sprintf(buf,"%s last_logon,",buf);
	sprintf(buf,"%s mana,",buf);
	sprintf(buf,"%s max_mana,",buf);
	sprintf(buf,"%s hit,",buf);
	sprintf(buf,"%s max_hit,",buf);		//30
	sprintf(buf,"%s move,",buf);
	sprintf(buf,"%s max_move,",buf);
	sprintf(buf,"%s armor,",buf);
	sprintf(buf,"%s gold,",buf);
	sprintf(buf,"%s bank_gold,",buf);	//35
	sprintf(buf,"%s exp,",buf);
	sprintf(buf,"%s hitroll,",buf);
	sprintf(buf,"%s damroll,",buf);
	sprintf(buf,"%s str,",buf);
	sprintf(buf,"%s str_add,",buf);		//40
	sprintf(buf,"%s intel,",buf);
	sprintf(buf,"%s wis,",buf);
	sprintf(buf,"%s dex,",buf);
	sprintf(buf,"%s con,",buf);
	sprintf(buf,"%s cha,",buf);		//45
	sprintf(buf,"%s wimp_level,",buf);
	sprintf(buf,"%s freeze_level,",buf);
	sprintf(buf,"%s invis_level,",buf);
	sprintf(buf,"%s load_room,",buf);
	sprintf(buf,"%s bad_pws,",buf);		//50
	sprintf(buf,"%s drunk,",buf);
	sprintf(buf,"%s full,",buf);
	sprintf(buf,"%s thirst,",buf);
	sprintf(buf,"%s spells_to_learn,",buf);
	sprintf(buf,"%s save0,",buf);		//55
	sprintf(buf,"%s save1,",buf);
	sprintf(buf,"%s save2,",buf);
	sprintf(buf,"%s save3,",buf);
	sprintf(buf,"%s save4,",buf);
	sprintf(buf,"%s plr_flags,",buf);	//60
	sprintf(buf,"%s prf_flags,",buf);
	sprintf(buf,"%s aff_flags,",buf);
	sprintf(buf,"%s olc_zone,",buf);
	sprintf(buf,"%s alignment,",buf);
	sprintf(buf,"%s last_update,",buf);	//65
	sprintf(buf,"%s feat_points,",buf);
        sprintf(buf,"%s skill_focus_points,",buf);
        sprintf(buf,"%s spell_mastery_points,",buf);
        sprintf(buf,"%s olc_zone2,", buf);
        sprintf(buf,"%s olc_zone3,", buf);	//70
        sprintf(buf,"%s olc_zone4,", buf);
        sprintf(buf,"%s olc_zone5,", buf);
        sprintf(buf,"%s barbarian_level,", buf);
        sprintf(buf,"%s bard_level,", buf);
        sprintf(buf,"%s cleric_level,", buf);	//75
        sprintf(buf,"%s druid_level,", buf);
        sprintf(buf,"%s fighter_level,", buf);
        sprintf(buf,"%s monk_level,", buf);
        sprintf(buf,"%s paladin_level,", buf);
        sprintf(buf,"%s ranger_level,", buf);	//80
        sprintf(buf,"%s rogue_level,", buf);
        sprintf(buf,"%s wizard_level,", buf);
        sprintf(buf,"%s arcane_archer_level,", buf);
        sprintf(buf,"%s dwarven_defender_level,", buf);
        sprintf(buf,"%s blackguard_level,", buf);	//85
        sprintf(buf,"%s loremaster_level,", buf);
        sprintf(buf,"%s shadow_dancer_level,", buf);
        sprintf(buf,"%s assassin_level,", buf);
        sprintf(buf,"%s knight_of_crown_level,", buf);
        sprintf(buf,"%s knight_of_sword_level,", buf);	//90
        sprintf(buf,"%s knight_of_rose_level,", buf);
        sprintf(buf,"%s knight_of_lily_level,", buf);
        sprintf(buf,"%s knight_of_skull_level,", buf);
        sprintf(buf,"%s knight_of_thorn_level,", buf);
        sprintf(buf,"%s deleted", buf);			//95
	sprintf(buf,"%s FROM %s WHERE name = '%s'",buf,mySQL_player_table,name);

	sql_connect();
	
	/* Debugging statement */
//	send_to_char(ch,buf);

	sql_real_query(buf);
	result = mysql_store_result(conn);
	row = mysql_fetch_row(result);
	if ((rows=mysql_num_rows(result)) > 1 || rows < 0) {
		sprintf(buf,"ERROR: Stat of %s returned %d files.\r\n",name, (int) rows);
		send_to_char(ch,buf);
	}
	
	if ((rows == 0)) {
		send_to_char(ch,"No such player file found.\r\n");
		mysql_free_result(result);
	} else {

	sprintf(buf,"&n***************************************************************************&n\r\n");
	if (atoi(row[95]) == 1) {
		sprintf(buf,"%s&n                        &RDELETED FLAG IS SET!!!!&n\r\n",buf);
	}
	sprintf(buf,"%s                        &WFile Stat of &M%-10s &WIDnum(&M%s&W)&n\r\n",
		buf,row[1],row[0]),
	sprintf(buf,"%s&n***************************************************************************&n\r\n",
		buf);
	sprintf(buf,"%s&WTitle            : &n%s\r\n",
		buf,row[8]);
	sprintf(buf,"%s&WShort Description: &n%s\r\n",
		buf,row[2]);
	sprintf(buf,"%s&WLong Description : &n%s\r\n",
		buf,row[3]);
	sprintf(buf,"%s&WKeywords         : &M%s\r\n",
		buf,row[5]);
	sprintf(buf,"%s&WPoofin           : &n%s\r\n",
		buf,row[6]);
	sprintf(buf,"%s&WPoofout          : &n%s\r\n",
		buf,row[7]);
	sprintf(buf,"%s&WEmail Address    : &M%s\r\n",
		buf,row[12]);
	sprintf(buf,"%s&WLevel : &M%-2s &WHometown : &M%-5s &WLoadroom : &M%-5s &WWeight/Height : &M%3s&W/&M%-3s&n\r\n",
		buf,row[16],row[17],row[49],row[18],row[19]);
	sprinttype(atoi(row[21]), (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? pc_class_types_core : pc_class_types_dl_aol), buf1, sizeof(buf1));
	sprinttype(atoi(row[22]), pc_race_types, buf2, sizeof(buf2));
	sprintf(buf,"%s&WClass : (&M%-2s&W) &M%-7s  &WRace : (&M%-2s&W) &M%-7s  &WAlignment : &M%-4s&n\r\n",
		buf,row[21],buf1,row[22],buf2,row[64]);
	sprinttype(atoi(row[20]), genders, buf1, sizeof(buf1));
	sprintf(buf,"%s&WSex : (&M%s&W) &M%s &WReligion : (&M%-2s&W) &M%-12s &WClan : &M%-2s &WClan Rank : &M%-2s&n\r\n",
		buf,row[20],buf1,row[11],deity_names[atoi(row[11])],row[9],row[10]);
	sprintf(buf,"%s&WRP Points : &M%-3s &WDarts : &M%-5s &WAbility Points : &M%-3s &WPractices : &M%-3s&n\r\n",
		buf,row[13],row[14],row[15],row[54]);
	sprintf(buf,"%s&WFeat Points : &M%-2s  &WSkill Focus Points : &M%-2s  &WSpell Mastery Points : &M%-2s&n\r\n",
		buf,row[66], row[67], row[68]);
	sprintf(buf,"%s&WHit/Hit : &M%5s&W/&M%-5s    &WMana/Max : &M%5s&W/&M%-5s     &WMove/Max : &M%5s&W/&M%-5s&n\r\n",
		buf,row[29],row[30],row[27],row[28],row[31],row[32]);
	sprintf(buf,"%s&WAC : &M%-3s    &WGold : &M%-6s    &WBank Gold : &M%-6s   &WExp : &M%-6s&n\r\n",
		buf,row[33],row[34],row[35],row[36]);
	sprintf(buf,"%s&WStr/Add : &M%2s&W/&M%-3s &WInt : &M%-2s &WWis : &M%-2s &WDex : &M%-2s &WCon : &M%-2s &WCha : &M%-2s&n\r\n",
		buf,row[39],row[40],row[41],row[42],row[43],row[44],row[45]);
	sprintf(buf,"%s&WHitroll : &M%-3s &WDamroll : &M%-3s    &WDrunk : &M%-2s &WFull : &M%-2s &WThirst : &M%-2s&n\r\n",
		buf,row[37],row[38],row[51],row[52],row[53]);
	sprintf(buf,"%s&WWimpLev : &M%-2s &WInvisLev : &M%-2s &WFreezeLev : &M%-2s   &WBad Passwords : &M%-2s&n\r\n",
		buf,row[46],row[48],row[47],row[50]);
	sprintf(buf,"%s&WSave0 : &M%-2s &WSave1 : &M%-2s &WSave2 : &M%-2s &WSave3 : &M%-2s &WSave4 : &M%-2s&n\r\n",
		buf,row[55],row[56],row[57],row[58],row[59]);
	sprintf(buf,"%s&BOLC Perms: &COLC1: &W%-2s &COLC2: &W%-2s &COLC3: &W%-2s &COLC4: &W%-2s &COLC5: &W%-2s&n\r\n",
		buf, row[63], row[69], row[70], row[71], row[72]);
	sprintf(buf,"%s&RRegular Class Levels:&n\r\n", buf);
	sprintf(buf,"%s&CBrb: &W%-2s  &CBrd: &W%-2s  &CCle: &W%-2s  &CDru: &W%-2s  &CFig: &W%-2s&n\r\n",
		buf, row[73], row[74], row[75], row[76], row[77]);
	sprintf(buf,"%s&CMnk: &W%-2s  &CPal: &W%-2s  &CRan: &W%-2s  &CRog: &W%-2s  &CWiz: &W%-2s&n\r\n",
		buf, row[78], row[79], row[80], row[81], row[82]);
	sprintf(buf,"%s&RPrestige Class Levels:&n\r\n", buf);
	sprintf(buf,"%s&CAAr: &W%-2s &CDwD: &W%-2s &CBGd: &W%-2s &CLMr: &W%-2s &CSDr: &W%-2s &CAsn: &W%-2s&n\r\n",
		buf, row[83], row[84], row[85], row[86], row[87], row[88]);
	sprintf(buf,"%s&CKCr: &W%-2s &CKSw: &W%-2s &CKRs: &W%-2s &CKLy: &W%-2s &CKSk: &W%-2s &CKTh: &W%-2s&n\r\n",
		buf, row[89], row[90], row[91], row[92], row[93], row[94]);
	sprintf(buf,"%s&WLast Database Update : &M%s&n\r\n",
		buf,row[65]);
	
	send_to_char(ch,buf);
	mysql_free_result(result);
	sql_disconnect();
}
}

/* change from sql_load_player, we return id of playerloaded insted of char struct */
int sql_player_load(char *name, struct char_data * ch) {
	
	struct affected_type tmp_aff[MAX_AFFECT];
	char buf[MAX_STRING_LENGTH];
	int i = 0;
	my_ulonglong rows;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	/* to save memory, only PC's -- not MOB's -- have player_specials */
	if (ch->player_specials == NULL)
    		CREATE (ch->player_specials, struct player_special_data, 1);
    	
    	if (!name || !*name) return -1;

	for (i = 0;i < MAX_AFFECT; i++) {
		tmp_aff[i].type = 0;
        	tmp_aff[i].duration = 0;
        	tmp_aff[i].bitvector = 0;
        	tmp_aff[i].modifier = 0;
        	tmp_aff[i].location = APPLY_NONE;
	}

    	/*Build the retirieve sql string */
    	sprintf(buf,"SELECT ");
    	sprintf(buf,"%s idnum,",buf);		//0
	sprintf(buf,"%s name,",buf);
	sprintf(buf,"%s short_descr,",buf);
	sprintf(buf,"%s long_descr,",buf);
	sprintf(buf,"%s description,",buf);
	sprintf(buf,"%s keywords,",buf);	//5
	sprintf(buf,"%s poofin,",buf);
	sprintf(buf,"%s poofout,",buf);
	sprintf(buf,"%s title,",buf);
	sprintf(buf,"%s clan,",buf);
	sprintf(buf,"%s clan_rank,",buf);	//10
	sprintf(buf,"%s religion,",buf);
	sprintf(buf,"%s email,",buf);
	sprintf(buf,"%s rppoints,",buf);
	sprintf(buf,"%s darts,",buf);
	sprintf(buf,"%s abil_points,",buf);	//15
	sprintf(buf,"%s level,",buf);
	sprintf(buf,"%s hometown,",buf);
	sprintf(buf,"%s weight,",buf);
	sprintf(buf,"%s height,",buf);
	sprintf(buf,"%s sex,",buf);		//20
	sprintf(buf,"%s class,",buf);
	sprintf(buf,"%s race,",buf);
	sprintf(buf,"%s birth,",buf);
	sprintf(buf,"%s played,",buf);
	sprintf(buf,"%s passwd,",buf);		//25
	sprintf(buf,"%s last_logon,",buf);
	sprintf(buf,"%s mana,",buf);
	sprintf(buf,"%s max_mana,",buf);
	sprintf(buf,"%s hit,",buf);
	sprintf(buf,"%s max_hit,",buf);		//30
	sprintf(buf,"%s move,",buf);
	sprintf(buf,"%s max_move,",buf);
	sprintf(buf,"%s armor,",buf);
	sprintf(buf,"%s gold,",buf);
	sprintf(buf,"%s bank_gold,",buf);	//35
	sprintf(buf,"%s exp,",buf);
	sprintf(buf,"%s hitroll,",buf);
	sprintf(buf,"%s damroll,",buf);
	sprintf(buf,"%s str,",buf);
	sprintf(buf,"%s str_add,",buf);		//40
	sprintf(buf,"%s intel,",buf);
	sprintf(buf,"%s wis,",buf);
	sprintf(buf,"%s dex,",buf);
	sprintf(buf,"%s con,",buf);
	sprintf(buf,"%s cha,",buf);		//45
	sprintf(buf,"%s wimp_level,",buf);
	sprintf(buf,"%s freeze_level,",buf);
	sprintf(buf,"%s invis_level,",buf);
	sprintf(buf,"%s load_room,",buf);
	sprintf(buf,"%s bad_pws,",buf);		//50
	sprintf(buf,"%s drunk,",buf);
	sprintf(buf,"%s full,",buf);
	sprintf(buf,"%s thirst,",buf);
	sprintf(buf,"%s spells_to_learn,",buf);
	sprintf(buf,"%s save0,",buf);		//55
	sprintf(buf,"%s save1,",buf);
	sprintf(buf,"%s save2,",buf);
	sprintf(buf,"%s save3,",buf);
	sprintf(buf,"%s save4,",buf);
	sprintf(buf,"%s plr_flags,",buf);	//60
	sprintf(buf,"%s prf_flags,",buf);
	sprintf(buf,"%s aff_flags,",buf);
	sprintf(buf,"%s olc_zone,",buf);
	sprintf(buf,"%s alignment,",buf);
	sprintf(buf,"%s feat_points,",buf);	//65
        sprintf(buf,"%s skill_focus_points,",buf);
        sprintf(buf,"%s spell_mastery_points,",buf);
	sprintf(buf,"%s olc_zone2,", buf);
	sprintf(buf,"%s olc_zone3,", buf);
	sprintf(buf,"%s olc_zone4,", buf);	//70
	sprintf(buf,"%s olc_zone5,", buf);
	sprintf(buf,"%s barbarian_level,", buf);
	sprintf(buf,"%s bard_level,", buf);
	sprintf(buf,"%s cleric_level,", buf);
	sprintf(buf,"%s druid_level,", buf);	//75
	sprintf(buf,"%s fighter_level,", buf);
	sprintf(buf,"%s monk_level,", buf);
	sprintf(buf,"%s paladin_level,", buf);
	sprintf(buf,"%s ranger_level,", buf);
	sprintf(buf,"%s rogue_level,", buf);	//80
	sprintf(buf,"%s wizard_level,", buf);
	sprintf(buf,"%s arcane_archer_level,", buf);
	sprintf(buf,"%s dwarven_defender_level,", buf);
	sprintf(buf,"%s blackguard_level,", buf);
	sprintf(buf,"%s loremaster_level,", buf);	//85
	sprintf(buf,"%s shadow_dancer_level,", buf);
	sprintf(buf,"%s assassin_level,", buf);
	sprintf(buf,"%s knight_of_crown_level,", buf);
	sprintf(buf,"%s knight_of_sword_level,", buf);
	sprintf(buf,"%s knight_of_rose_level,", buf);	//90
	sprintf(buf,"%s knight_of_lily_level,", buf);
	sprintf(buf,"%s knight_of_skull_level,", buf);
	sprintf(buf,"%s knight_of_thorn_level,", buf);
	sprintf(buf,"%s deleted,", buf);
	sprintf(buf,"%s last_name,", buf);		//95
	sprintf(buf,"%s short_title", buf);
	/*MAKE SURE THE LAST ENTRY OF THIS STRING DOES NOT HAVE A COMMA!!! */
	sprintf(buf,"%s FROM %s WHERE name = '%s'",buf,mySQL_player_table,name);
	sql_connect();
	/* Debugging statement */
//	send_to_char(ch,buf);
	sql_real_query(buf);
	result = mysql_store_result(conn);
	row = mysql_fetch_row(result);
	if ((rows=mysql_num_rows(result)) > 1 || rows < 0) {
		sprintf(buf,"ERROR: SELECT for %s returned %d rows!",name, (int) rows);
		mudlog(NRM, LVL_IMMORT, TRUE, buf);
	} 
	if (!rows) {
		mysql_free_result(result);
		return -1;
	}

	/* These entrys must be in the same order that they are retrieved above*/
	GET_IDNUM(ch) 		= atoi(row[0]);
	GET_PC_NAME(ch)		= strdup(row[1]);
	GET_PLR_SDESC(ch)	= strdup(row[2]);
	GET_PLR_LDESC(ch)	= strdup(row[3]);
	GET_PLR_DESC(ch)	= strdup(row[4]);
	GET_PLR_KEYW(ch)	= strdup(row[5]);
	POOFIN(ch)		= strdup(row[6]);
	POOFOUT(ch)		= strdup(row[7]);
	GET_TITLE(ch)		= strdup(row[8]);
	GET_CLAN(ch) 		= atoi(row[9]);
	GET_CLAN_RANK(ch) 	= atoi(row[10]);
	GET_RELIGION(ch) 	= atoi(row[11]);
	GET_EMAIL(ch)		= strdup(row[12]);
	GET_RPPOINTS(ch) 	= atoi(row[13]);
	GET_DARTS(ch) 		= atoi(row[14]);
	GET_ABIL_PTS(ch) 	= atoi(row[15]);
	GET_LEVEL(ch) 		= atoi(row[16]);
	GET_HOME(ch) 		= atoi(row[17]);
	GET_WEIGHT(ch) 		= atoi(row[18]);
	GET_HEIGHT(ch) 		= atoi(row[19]);
	GET_GENDER(ch) 		= atoi(row[20]);
	GET_CLASS(ch) 		= atoi(row[21]);
	GET_RACE(ch) 		= atoi(row[22]);
	GET_PLR_BIRTH(ch) 	= atoi(row[23]);
	GET_PLR_PLAYED(ch) 	= atoi(row[24]);
	strncpy(GET_PASSWD(ch),row[25],10);
	GET_PLR_LOGON(ch) 	= atoi(row[26]);
	GET_MANA(ch) 		= atoi(row[27]);
	GET_MAX_MANA(ch) 	= atoi(row[28]);
	GET_HIT(ch) 		= atoi(row[29]);
	GET_MAX_HIT(ch) 	= atoi(row[30]);
	GET_MOVE(ch) 		= atoi(row[31]);
	GET_MAX_MOVE(ch) 	= atoi(row[32]);
	GET_AC(ch) 		= atoi(row[33]);
	GET_GOLD(ch) 		= atoi(row[34]);
	GET_BANK_GOLD(ch) 	= atoi(row[35]);
	GET_EXP(ch) 		= atoi(row[36]);
	GET_HITROLL(ch) 	= atoi(row[37]);
	GET_DAMROLL(ch) 	= atoi(row[38]);
	GET_STR(ch) 		= atoi(row[39]);
//	GET_ADD(ch) 		= atoi(row[40]);
	GET_INT(ch) 		= atoi(row[41]);
	GET_WIS(ch) 		= atoi(row[42]);
	GET_DEX(ch) 		= atoi(row[43]);
	GET_CON(ch) 		= atoi(row[44]);
	GET_CHA(ch) 		= atoi(row[45]);
	GET_WIMP_LEV(ch) 	= atoi(row[46]);
	GET_FREEZE_LEV(ch) 	= atoi(row[47]);
	GET_INVIS_LEV(ch) 	= atoi(row[48]);
	GET_LOADROOM(ch) 	= atoi(row[49]);
	GET_BAD_PWS(ch) 	= atoi(row[50]);
	GET_COND(ch,DRUNK) 	= atoi(row[51]);
	GET_COND(ch,FULL) 	= atoi(row[52]);
	GET_COND(ch,THIRST) 	= atoi(row[53]);
	GET_PRACTICES(ch) 	= atoi(row[54]);
	GET_SAVE(ch,0) 		= atoi(row[55]);
	GET_SAVE(ch,1) 		= atoi(row[56]);
	GET_SAVE(ch,2) 		= atoi(row[57]);
	GET_SAVE(ch,3) 		= atoi(row[58]);
	GET_SAVE(ch,4) 		= atoi(row[59]);
	PLR_FLAGS(ch)		= atoi(row[60]);
	PRF_FLAGS(ch)		= atoi(row[61]);
	AFF_FLAGS(ch)		= atoi(row[62]);
//	GET_OLC_ZONE(ch) 	= atoi(row[63]);
	GET_ALIGNMENT(ch)	= atoi(row[64]);
	GET_FEAT_POINTS(ch)	= atoi(row[65]);
        GET_SKILL_FOCUS_POINTS(ch) 	= atoi(row[66]);
        GET_SPELL_MASTERY_POINTS(ch) 	= atoi(row[67]);
//	GET_OLC_ZONE2(ch)	= atoi(row[68]);
//	GET_OLC_ZONE3(ch)	= atoi(row[69]);
//	GET_OLC_ZONE4(ch)	= atoi(row[70]);
//	GET_OLC_ZONE5(ch)	= atoi(row[71]);
	ch->class_level[CLASS_BARBARIAN] = atoi(row[72]);
	ch->class_level[CLASS_BARD]	= atoi(row[73]);
	ch->class_level[CLASS_CLERIC]   = atoi(row[74]);
	ch->class_level[CLASS_DRUID]	= atoi(row[75]);
	ch->class_level[CLASS_FIGHTER]	= atoi(row[76]);
	ch->class_level[CLASS_MONK]	= atoi(row[77]);
	ch->class_level[CLASS_MYSTIC]	= atoi(row[78]);
	ch->class_level[CLASS_RANGER]	= atoi(row[79]);
	ch->class_level[CLASS_ROGUE]	= atoi(row[80]);
	ch->class_level[CLASS_WIZARD]	= atoi(row[81]);
	ch->class_level[CLASS_ARCANE_ARCHER] = atoi(row[82]);
	ch->class_level[CLASS_DWARVEN_DEFENDER]	= atoi(row[83]);
	ch->class_level[CLASS_BLACK_GUARD] = atoi(row[84]);
	ch->class_level[CLASS_LOREMASTER] = atoi(row[85]);
	ch->class_level[CLASS_SHADOWDANCER] = atoi(row[86]);
	ch->class_level[CLASS_ASSASSIN]	= atoi(row[87]);
	ch->class_level[CLASS_CROWN_KNIGHT] = atoi(row[88]);
	ch->class_level[CLASS_SWORD_KNIGHT] = atoi(row[89]);
	ch->class_level[CLASS_ROSE_KNIGHT] = atoi(row[90]);
	ch->class_level[CLASS_LILY_KNIGHT] = atoi(row[91]);
	ch->class_level[CLASS_SKULL_KNIGHT] = atoi(row[92]);
	ch->class_level[CLASS_THORN_KNIGHT] = atoi(row[93]);
	GET_DELETED(ch)		= atoi(row[94]);
	GET_LAST_NAME(ch)	= strdup(row[95]);
	GET_SHORT_TITLE(ch) 	= strdup(row[96]);

        COINS(ch)[0] = GET_GOLD(ch) % 10;
        COINS(ch)[1] = (GET_GOLD(ch) / 10) % 10;
        COINS(ch)[2] = (GET_GOLD(ch) / 100) % 10;
        COINS(ch)[3] = (GET_GOLD(ch) / 1000) % 10;
        COINS(ch)[4] = (GET_GOLD(ch) / 10000);

	if (GET_LEVEL(ch) >= LVL_IMMORT) {


		GET_COND(ch, FULL) = -1;
		GET_COND(ch, THIRST) = -1;
		GET_COND(ch, DRUNK) = -1;
	}

	mysql_free_result(result);
	sql_disconnect();
	/*Some cleanup stuff*/
	GET_LAST_TELL(ch) = NOBODY;
	ch->char_specials.carry_weight = 0;
  	ch->char_specials.carry_items = 0;
  	/*Add extras */
  	sql_modify_db_aff(ch,MOD_RETRIEVE);
 	sql_modify_db_feats(ch,MOD_RETRIEVE);
 	sql_modify_db_skills(ch,MOD_RETRIEVE);
// 	sql_modify_db_spells(ch,MOD_RETRIEVE);
 	sql_modify_db_intro(ch,MOD_RETRIEVE);
  	return GET_IDNUM(ch);
}

/* Main function to modify the player database */
void sql_modify_db (struct char_data *ch, int mode,room_rnum load_room)
{
	char buf[MAX_STRING_LENGTH];
	int sqlmode=0,i;
	struct obj_data *char_eq[NUM_WEARS];
	MYSQL_RES *result;
	
	/*We dont save mobs*/
	if (IS_NPC(ch))
		return;
	
	/*If MOD_INSERT check to see if player is in SQL, if yes SQL_REPLACE, if no SQL_INSERT*/
	if (mode == MOD_INSERT) {
		sprintf(buf,"SELECT idnum FROM %s WHERE idnum = %ld", mySQL_player_table, GET_IDNUM(ch));
		sql_connect();
		sql_real_query(buf);
		result = mysql_store_result(conn);

		if (mysql_num_rows(result)==0)
			sqlmode=SQL_INSERT;
		else
			sqlmode=SQL_REPLACE;
	
		mysql_free_result(result);
		sql_disconnect();
	}
	/*If MOD_INSERT unaffect and unequip char for "True" Stats, and save spells*/
	if (mode == MOD_INSERT) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(ch, i)) {
				char_eq[i] = unequip_char(ch, i);
#ifndef NO_EXTRANEOUS_TRIGGERS
				remove_otrigger(char_eq[i], ch);
#endif
			} else
				char_eq[i] = NULL;
		}
		
		sql_modify_db_aff(ch,MOD_INSERT);
		sql_modify_db_feats(ch,MOD_INSERT);
//		sql_modify_db_spells(ch,MOD_INSERT);
		sql_modify_db_skills(ch,MOD_INSERT);
		sql_modify_db_intro(ch,MOD_INSERT);
	}
	
	/*If we are MOD_DELETE, just remove players aff's from the affects table and fry spells also*/
	if (mode == MOD_DELETE) {
		sql_modify_db_aff(ch,MOD_DELETE);
		sql_modify_db_feats(ch,MOD_DELETE);
//		sql_modify_db_spells(ch,MOD_DELETE);
 		sql_modify_db_skills(ch,MOD_DELETE);
 		sql_modify_db_intro(ch,MOD_DELETE);
	}
	
	/*If MOD_DELETE set sqlmode SQL_DELETE*/
	if (mode == MOD_DELETE)
		sqlmode=SQL_DELETE;

	sql_connect();
	
	/*If SQL_INSERT set INSERT SQL string*/
	if (sqlmode == SQL_INSERT) {
		sprintf(buf,"INSERT INTO %s SET",
			mySQL_player_table);
	}
	
	/*If SQL_REPLACE set REPLACE SQL string*/
	if (sqlmode == SQL_REPLACE) {
		sprintf(buf,"REPLACE INTO %s SET",
			mySQL_player_table);
	}
	
	/*If SQL_DELETE set DELETE SQL string*/
	if (sqlmode == SQL_DELETE) {
		
		sprintf(buf,"DELETE FROM %s WHERE idnum = %ld",mySQL_player_table,GET_IDNUM(ch));
	}

	GET_GOLD(ch) = COINS(ch)[0] +
			(COINS(ch)[1] * 10) +
			(COINS(ch)[2] * 100) +
			(COINS(ch)[3] * 1000) +
			(COINS(ch)[4] * 10000);
	
	/*	
		If SQL_INSERT or SQL_REPLACE build string to save player struct
		Note this dosent have to be in any order.... just make sure the 
		last entry does not have a tailing comma 
	*/

	if (sqlmode == SQL_INSERT || sqlmode == SQL_REPLACE) {	
		sprintf(buf,"%s idnum = %ld,",		buf,GET_IDNUM(ch));
		sprintf(buf,"%s name = '%s',",		buf,GET_PC_NAME(ch));
		sprintf(buf,"%s short_descr = '%s',",	buf,sql_escape(conn, GET_PLR_SDESC(ch)));
		sprintf(buf,"%s long_descr = '%s',",	buf,sql_escape(conn, GET_PLR_LDESC(ch)));
		sprintf(buf,"%s description = '%s',",	buf,sql_escape(conn, GET_PLR_DESC(ch)));
		sprintf(buf,"%s keywords = '%s',",	buf,sql_escape(conn, GET_PLR_KEYW(ch)));	
		sprintf(buf,"%s poofin = '%s',",	buf,sql_escape(conn, POOFIN(ch)));
		sprintf(buf,"%s poofout = '%s',",	buf,sql_escape(conn, POOFOUT(ch)));
		sprintf(buf,"%s title = '%s',",		buf,sql_escape(conn, GET_TITLE(ch)));
		sprintf(buf,"%s clan = %d,",		buf,GET_CLAN(ch));
		sprintf(buf,"%s clan_rank = %d,",	buf,GET_CLAN_RANK(ch));
		sprintf(buf,"%s religion = %d,",	buf,GET_RELIGION(ch));
		sprintf(buf,"%s email = '%s',",		buf,GET_EMAIL(ch));
		sprintf(buf,"%s rppoints = %d,",	buf,GET_RPPOINTS(ch));
		sprintf(buf,"%s darts = %d,",		buf,GET_DARTS(ch));
		sprintf(buf,"%s abil_points = %d,",	buf,GET_ABIL_PTS(ch));
		sprintf(buf,"%s level = %d,",		buf,GET_LEVEL(ch));
		sprintf(buf,"%s hometown = %d,",	buf,GET_HOME(ch));
		sprintf(buf,"%s weight = %d,",		buf,GET_WEIGHT(ch));
		sprintf(buf,"%s height = %d,",		buf,GET_HEIGHT(ch));
		sprintf(buf,"%s sex = %d,",		buf,GET_GENDER(ch));
		sprintf(buf,"%s class = %d,",		buf,GET_CLASS(ch));
		sprintf(buf,"%s race = %d,",		buf,GET_RACE(ch));
		sprintf(buf,"%s birth = %ld,",		buf,GET_PLR_BIRTH(ch));
		sprintf(buf,"%s played = %d,",		buf,GET_PLR_PLAYED(ch));
		sprintf(buf,"%s passwd = '%s',",	buf,GET_PASSWD(ch));
		sprintf(buf,"%s last_logon = %ld,",	buf,GET_PLR_LOGON(ch));
		sprintf(buf,"%s mana = %d,",		buf,GET_MANA(ch));
		sprintf(buf,"%s max_mana = %d,",	buf,GET_MAX_MANA(ch));
		sprintf(buf,"%s hit = %d,",		buf,GET_HIT(ch));
		sprintf(buf,"%s max_hit = %d,",		buf,GET_MAX_HIT(ch));
		sprintf(buf,"%s move = %d,",		buf,GET_MOVE(ch));
		sprintf(buf,"%s max_move = %d,",	buf,GET_MAX_MOVE(ch));
		sprintf(buf,"%s armor = %d,",		buf,GET_AC(ch));
		sprintf(buf,"%s gold = %d,",		buf,GET_GOLD(ch));
		sprintf(buf,"%s bank_gold = %d,",	buf,GET_BANK_GOLD(ch));
		sprintf(buf,"%s exp = %d,",		buf,GET_EXP(ch));
		sprintf(buf,"%s hitroll = %d,",		buf,GET_HITROLL(ch));
		sprintf(buf,"%s damroll = %d,",		buf,GET_DAMROLL(ch));
		sprintf(buf,"%s str = %d,",		buf,GET_STR(ch));
//		sprintf(buf,"%s str_add = %d,",		buf,GET_ADD(ch));
		sprintf(buf,"%s intel = %d,",		buf,GET_INT(ch));
		sprintf(buf,"%s wis = %d,",		buf,GET_WIS(ch));
		sprintf(buf,"%s dex = %d,",		buf,GET_DEX(ch));
		sprintf(buf,"%s con = %d,",		buf,GET_CON(ch));
		sprintf(buf,"%s cha = %d,",		buf,GET_CHA(ch));
		sprintf(buf,"%s wimp_level = %d,",	buf,GET_WIMP_LEV(ch));
		sprintf(buf,"%s freeze_level = %d,",	buf,GET_FREEZE_LEV(ch));
		sprintf(buf,"%s invis_level = %d,",	buf,GET_INVIS_LEV(ch));
		sprintf(buf,"%s load_room = %d,",	buf,GET_LOADROOM(ch));
		sprintf(buf,"%s bad_pws = %d,",		buf,GET_BAD_PWS(ch));
		sprintf(buf,"%s drunk = %d,",		buf,GET_COND(ch, DRUNK));
		sprintf(buf,"%s full = %d,",		buf,GET_COND(ch, FULL));
		sprintf(buf,"%s thirst = %d,",		buf,GET_COND(ch, THIRST));
		sprintf(buf,"%s spells_to_learn = %d,",	buf,GET_PRACTICES(ch));
		sprintf(buf,"%s save0 = %d,",		buf,GET_SAVE(ch,0));
		sprintf(buf,"%s save1 = %d,",		buf,GET_SAVE(ch,1));
		sprintf(buf,"%s save2 = %d,",		buf,GET_SAVE(ch,2));
		sprintf(buf,"%s save3 = %d,",		buf,GET_SAVE(ch,3));
		sprintf(buf,"%s save4 = %d,",		buf,GET_SAVE(ch,4));
		sprintf(buf,"%s plr_flags = %ld,",	buf,PLR_FLAGS(ch));
		sprintf(buf,"%s prf_flags = %ld,",	buf,PRF_FLAGS(ch));
		sprintf(buf,"%s aff_flags = %ld,",	buf,AFF_FLAGS(ch));
		sprintf(buf,"%s alignment = %d,",	buf,GET_ALIGNMENT(ch));
		sprintf(buf,"%s olc_zone = %d,",	buf,0);
		sprintf(buf,"%s feat_points = %d,",	buf,GET_FEAT_POINTS(ch));
                sprintf(buf,"%s skill_focus_points = %d,",	buf,GET_SKILL_FOCUS_POINTS(ch));
                sprintf(buf,"%s spell_mastery_points = %d,",	buf,GET_SPELL_MASTERY_POINTS(ch));
		sprintf(buf,"%s olc_zone2 = %d,", 	buf, 0);
		sprintf(buf,"%s olc_zone3 = %d,", 	buf, 0);
		sprintf(buf,"%s olc_zone4 = %d,", 	buf, 0);
		sprintf(buf,"%s olc_zone5 = %d,", 	buf, 0);
                sprintf(buf,"%s barbarian_level = %d,", buf, ch->class_level[CLASS_BARBARIAN]);
                sprintf(buf,"%s bard_level = %d,", 	buf, ch->class_level[CLASS_BARD]);
                sprintf(buf,"%s cleric_level = %d,", 	buf, ch->class_level[CLASS_CLERIC]);
                sprintf(buf,"%s druid_level = %d,", 	buf, ch->class_level[CLASS_DRUID]);
                sprintf(buf,"%s fighter_level = %d,", 	buf, ch->class_level[CLASS_FIGHTER]);
                sprintf(buf,"%s monk_level = %d,", 	buf, ch->class_level[CLASS_MONK]);
                sprintf(buf,"%s paladin_level = %d,", 	buf, ch->class_level[CLASS_MYSTIC]);
                sprintf(buf,"%s ranger_level = %d,", 	buf, ch->class_level[CLASS_RANGER]);
                sprintf(buf,"%s rogue_level = %d,", 	buf, ch->class_level[CLASS_ROGUE]);
                sprintf(buf,"%s wizard_level = %d,", 	buf, ch->class_level[CLASS_WIZARD]);
                sprintf(buf,"%s arcane_archer_level = %d,", 	buf, ch->class_level[CLASS_ARCANE_ARCHER]);
                sprintf(buf,"%s dwarven_defender_level = %d,", 	buf, ch->class_level[CLASS_DWARVEN_DEFENDER]);
                sprintf(buf,"%s blackguard_level = %d,", 	buf, ch->class_level[CLASS_BLACK_GUARD]);
                sprintf(buf,"%s loremaster_level = %d,", 	buf, ch->class_level[CLASS_LOREMASTER]);
                sprintf(buf,"%s shadow_dancer_level = %d,", 	buf, ch->class_level[CLASS_SHADOWDANCER]);
                sprintf(buf,"%s assassin_level = %d,", 	buf, ch->class_level[CLASS_ASSASSIN]);
                sprintf(buf,"%s knight_of_crown_level = %d,", 	buf, ch->class_level[CLASS_CROWN_KNIGHT]);
                sprintf(buf,"%s knight_of_sword_level = %d,", 	buf, ch->class_level[CLASS_SWORD_KNIGHT]);
                sprintf(buf,"%s knight_of_rose_level = %d,", 	buf, ch->class_level[CLASS_ROSE_KNIGHT]);
                sprintf(buf,"%s knight_of_lily_level = %d,", 	buf, ch->class_level[CLASS_LILY_KNIGHT]);
                sprintf(buf,"%s knight_of_skull_level = %d,", 	buf, ch->class_level[CLASS_SKULL_KNIGHT]);
                sprintf(buf,"%s knight_of_thorn_level = %d,", 	buf, ch->class_level[CLASS_THORN_KNIGHT]);
                sprintf(buf,"%s deleted = %d,", 	buf, GET_DELETED(ch));
		sprintf(buf,"%s last_name = '%s',",	buf, GET_LAST_NAME(ch));
		sprintf(buf,"%s short_title = '%s',", 	buf, GET_SHORT_TITLE(ch));
		sprintf(buf,"%s last_update = NOW()",		buf);
	}
	
	/*Just a sanity check*/
	if (sqlmode == SQL_INSERT || sqlmode == SQL_REPLACE || sqlmode == SQL_DELETE) {
		sql_real_query(buf);
	}
	
	sql_disconnect();

	/*If SQL_INSERT || SQL_REPLACE reaffect and reequip char for Stats*/
	if (sqlmode == SQL_INSERT || sqlmode == SQL_REPLACE) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (char_eq[i]) {
#ifndef NO_EXTRANEOUS_TRIGGERS
				if (wear_otrigger(char_eq[i], ch, i))
#endif
					equip_char(ch, char_eq[i], i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      				else
        				obj_to_char(char_eq[i], ch);
#endif
    			}
  		}
	}

	/*Log a new player*/
	if (sqlmode == SQL_INSERT) {
		sprintf(buf,"Created Pfile For %s",GET_NAME(ch));
		mudlog(NRM, LVL_IMMORT, TRUE, buf);
		Crash_crashsave(ch);
	}
	
	/*Log a delete*/
	if (sqlmode == SQL_DELETE) {
		sprintf(buf,"Deleted Pfile For %s",GET_NAME(ch));
		mudlog(NRM, LVL_IMMORT, TRUE, buf);
		crash_delete_rent(ch);		
	}
	
	/* Debugging statement */
//	send_to_char(ch,buf);
}

/**************************
 *  Social DB Functions   *
 **************************/

/* Social list function */
void sql_social_list (struct char_data *ch)
{
	char buf[MAX_STRING_LENGTH];
	int recs,no,i;
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	
  	sprintf(buf, "SELECT social FROM %s ORDER BY social ASC",mySQL_social_table);
	sql_connect();
	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	sprintf(buf, "The following Socials are available:\r\n");
	
	for (i = 1,no = 1; no <= recs; i++) {
		row = mysql_fetch_row(result);
		sprintf(buf,"%s%-9s%s",buf,row[0],no++ % 6 == 0 ? "\r\n" : " ");
    	}
    	
    	if (no % 6 != 1)
  		sprintf(buf,"%s\r\n",buf);
    	
    	sprintf(buf,"%s    %d total socials.\r\n",buf,recs);
    	
  	send_to_char(ch,buf);	
}

/* Social Command Lookup and Usage */
void sql_social_cmd (struct char_data *ch, char *arg)
{
	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	int recs, mode = 0;
	struct char_data *vict;
	MYSQL_RES *result;
  	MYSQL_ROW row;

	/* Cant call a social without an argument */
	if (!*arg) {
		send_to_char(ch,"Huh?!?\r\n");
		return;
	}

	/*check for reserved chars */
  	if (strstr(arg, "'") || strstr(arg, "\"") || strstr(arg, "(") || strstr(arg, ")") || strstr(arg, "[") || strstr(arg, "]") || strstr(arg, "{") || strstr(arg, "}") || strstr(arg, "\\")) {
  		send_to_char(ch,"Huh?!?\r\n");
  		return;
  	}

	two_arguments(arg, buf1, buf2);
	
	sprintf(buf,"SELECT hide,toself_nothere,toself_notarg,toroom_notarg,toself_me,toroom_me,toself_targ,toroom_targ,totarg_targ,min_pos FROM %s WHERE social = '%s'",mySQL_social_table,buf1);

	/* Debugging statement */
//	send_to_char(ch,buf);

	sql_connect();
	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	/* Debugging statement */
//	send_to_char(ch,buf);

	if (recs == 0) {
		sprintf(buf,"Huh?!?\r\n");
		send_to_char(ch,buf);
	}
	
	if (recs > 1) {
		sprintf(buf,"More than one social of that name exisits, please tell an immortal.\r\n");
		send_to_char(ch,buf);
	} 
	
	if (recs == 1) {
		row = mysql_fetch_row(result);
		/* check if there is a buf2, if so check and see if that char is in the room */
		if (strlen(buf2) > 0) {
			/* char not found */
			if (strstr(row[1], "#"))
				return;
			if (!(vict = get_char_vis(ch, buf2, NULL, FIND_CHAR_ROOM))) {
				sprintf(buf,"%s\r\n",row[1]);
				send_to_char(ch,buf);
		  		return;
			} else {
				mode = 1;
			}
			/*social if vict = ch */
			if (vict == ch) {
				sprintf(buf,"%s\r\n",row[4]);
				send_to_char(ch,buf);
				act(row[5], atoi(row[0]), ch, 0, 0, TO_ROOM);
				return;
			}
			if (GET_POS(vict) < atoi(row[9])) {
      				act("$N is not in a proper position for that.",FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      				return;
			}
			if (mode == 1) {
				act(row[6], 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
				act(row[7], atoi(row[0]), ch, 0, vict, TO_NOTVICT);
      				act(row[8], atoi(row[0]), ch, 0, vict, TO_VICT);
      				return;
      			}
		}
		
			/* social if no arg */
			sprintf(buf,"%s\r\n",row[2]);
			send_to_char(ch,buf);
			act(row[3], atoi(row[0]), ch, 0, 0, TO_ROOM);
	}
	
	mysql_free_result(result);
	sql_disconnect();
}

/**************************
 * Help File DB Functions *
 **************************/

/* Help command lookup and display */
void sql_help_cmd (struct char_data *ch, char *arg)
{
	char buf[MAX_STRING_LENGTH];
	int recs;
	MYSQL_RES *result;
	MYSQL_RES *levres;
  	MYSQL_ROW row;
  	
  	if (strstr(arg, "'") || strstr(arg, "\"") || strstr(arg, "(") || strstr(arg, ")") || strstr(arg, "[") || strstr(arg, "]") || strstr(arg, "{") || strstr(arg, "}")) {
  		send_to_char(ch,"Sorry, '\"(){}[] are not needed when looking for help, try again without those characters.\r\n");
  		return;
  	}
  	
  	sprintf(buf,"SELECT helptext FROM %s WHERE keywords LIKE '%%%s%%'",mySQL_help_table,arg);
	sql_connect();
	sql_real_query(buf);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	/* Debugging statement */
//	sprintf(buf,"%d\r\n",recs);
//	send_to_char(ch,buf);
	
	if (recs == 0) {
		mysql_free_result(result);
		sprintf(buf,"Sorry, we did not find any help for \"%s\", please tell an immortal.\r\n",arg);
		send_to_char(ch,buf);
	}
	
	if (recs > 1) {
		mysql_free_result(result);
		sprintf(buf,"SELECT keywords FROM %s WHERE keywords LIKE '%%%s%%'",mySQL_help_table,arg);
		sql_real_query(buf);
		result = mysql_store_result(conn);
		sprintf(buf,"We found more than one entry for \"%s\", please use one from below:\r\n",arg);
		while ((row = mysql_fetch_row(result))) {
			sprintf(buf,"%s%s\r\n",buf,row[0]);
		}
		mysql_free_result(result);
		send_to_char(ch,buf);
	} 
	
	if (recs == 1) {
		sprintf(buf,"SELECT level FROM %s WHERE keywords LIKE '%%%s%%'",mySQL_help_table,arg);
		sql_real_query(buf);
		levres = mysql_store_result(conn);
		row = mysql_fetch_row(levres);
		if (GET_LEVEL(ch) < atoi(row[0])) {
			send_to_char(ch,"You cannot view this help file (Level Restriction)\r\n");
			mysql_free_result(levres);
			return;
		}
		row = mysql_fetch_row(result);
		sprintf(buf,"%s\r\n",row[0]);
		send_to_char(ch,buf);
		mysql_free_result(result);
	}
	
	sql_disconnect();
}

/**************************
 *    Mail DB Functions   *
 **************************/
void sql_renumber_maillist(long id)
{
	char query[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int recs, i;
	
	sprintf(query, "SELECT id FROM %s WHERE to_idnum = %ld",
		mySQL_mail_table, id);
	
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		return;
	}
	
	if (recs >= 1) {
		i = 1;
		while ((row = mysql_fetch_row(result))) {
			sprintf(query, "UPDATE %s SET mail_id = %d WHERE id = %d", 
				mySQL_mail_table, i, atoi(row[0]));
			sql_real_query(query);
			i++;
		}
	}
	mysql_free_result(result);
	sql_disconnect();
}

void sql_send_mail(struct char_data *ch, char *arg)
{
  	long recipient;
  	char buf[MAX_INPUT_LENGTH], **sqlmail;

  	if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
    		send_to_char(ch, "Sorry, you have to be level %d to send mail!\r\n", MIN_MAIL_LEVEL);
    		return;
  	}
  	
  	one_argument(arg, buf);

  	if (!*buf) {			/* you'll get no argument from me! */
  		send_to_char(ch, "You need to specify an addressee!\r\n");
   	 	return;
  	}
  	
  	if ((recipient = sql_get_id_by_name(buf)) < 0) {
  		send_to_char(ch, "No one by that name is registered here!\r\n");
      		return;
  	}
  	
  	act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
  	
  	send_to_char(ch, "Write your message, (/s saves /h for help)\r\n");
  		
  	SET_BIT(PLR_FLAGS(ch), PLR_MAILING);	/* string_write() sets writing. */

  	/* Start writing! */
  	CREATE(sqlmail, char *, 1);
  	string_write(ch->desc, sqlmail, MAX_MAIL_SIZE, recipient, NULL);	
}

void sql_list_mail(struct char_data *ch)
{
	char buf[MAX_STRING_LENGTH], query[MAX_STRING_LENGTH], status[3];
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int recs;
	
	sprintf(query, "SELECT mail_id, from_idnum, sent_time, mesg_status FROM %s WHERE to_idnum = %ld",
		mySQL_mail_table, GET_IDNUM(ch));
	
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		send_to_char(ch, "&GYou have no mudmail in your inbox.&n\r\n\r\n");
	}
	
	if (recs > 0) {
		sprintf(buf, "&G==============================================&n\r\n");
		sprintf(buf, "%s&WMsg   Status  From          Sent Time&n\r\n",buf);
		sprintf(buf, "%s&G==============================================&n\r\n",buf);
		
		while ((row = mysql_fetch_row(result))) {
			*status = '\0';
			
			if (atoi(row[3]) == 1)
				sprintf(status, "U");
			else if (atoi(row[3]) == 2)
				sprintf(status, "R");
			else if (atoi(row[3]) == 3)
				sprintf(status, "S");
			else
				sprintf(status, "X");
				
			sprintf(buf, "%s &M%-3s   (%-1s)    %-13s %-20s&n\r\n",buf, row[0], status, sql_get_name_by_id(atoi(row[1])), row[2]);
		}
		
		page_string(ch->desc, buf, 0);
		send_to_char(ch, "\r\n");
	}
	mysql_free_result(result);
	sql_disconnect(); 
}

void sql_read_mail(struct char_data *ch, char *arg)
{
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int recs, value;
  	char query[MAX_STRING_LENGTH], mail[MAX_STRING_LENGTH], status[3];


	if (!*arg) {
		send_to_char(ch, "You must enter a message number to read.\r\n");
		return;
	}

  	value = atoi(arg);
  	
	sprintf(query, "SELECT mail_id, from_idnum, sent_time, mesg_status, message FROM %s WHERE mail_id = %d AND to_idnum = %ld",
		mySQL_mail_table, value, GET_IDNUM(ch));
		
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		send_to_char(ch, "That message does not exisit.\r\n");
		return;
	}
	
	if (recs == 1) {
		sprintf(mail, "&G==============================================&n\r\n");
		row = mysql_fetch_row(result);
		
		if (atoi(row[3]) == 1)
			sprintf(status, "U");
		if (atoi(row[3]) == 2)
			sprintf(status, "R");
		if (atoi(row[3]) == 3)
			sprintf(status, "S");
		
		sprintf(mail, "%s&MID     : &W%s\r\n&MFrom   : &W%s\r\n&MTo     : &W%s\r\n&MSent At: &W%s\r\n&G==============================================&n\r\n&W%s&n\r\n",
			mail, row[0], sql_get_name_by_id(atoi(row[1])), GET_PC_NAME(ch), row[2], row[4]);
		
		page_string(ch->desc, mail, 0);
		send_to_char(ch, "\r\n");
		
		if (atoi(row[3]) == 1) {
			sprintf(query, "UPDATE %s SET mesg_status = 2, read_time = NOW() WHERE mail_id = %d AND to_idnum  = %ld",
				mySQL_mail_table, atoi(row[0]), GET_IDNUM(ch));
			sql_connect();
			sql_real_query(query);
		}
		
		mysql_free_result(result);
		sql_disconnect();
	}

}

void sql_delete_mail(struct char_data *ch, char *arg)
{
	MYSQL_RES *result;
  	int recs, value;
  	char query[MAX_STRING_LENGTH];

	if (!*arg) {
		send_to_char(ch, "You must enter a message number to delete.\r\n");
		return;
	}

  	value = atoi(arg);
  	
	sprintf(query, "SELECT id FROM %s WHERE mail_id = %d AND to_idnum = %ld",
		mySQL_mail_table, value, GET_IDNUM(ch));
		
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		send_to_char(ch, "That message does not exisit.\r\n");
		return;
	}
	
	if (recs == 1) {
		sprintf(query, "DELETE FROM %s WHERE mail_id = %d AND to_idnum = %ld", mySQL_mail_table, value, GET_IDNUM(ch));
		sql_real_query(query);
		send_to_char(ch, "Message %d was deleted.\r\n", value);
	}
	mysql_free_result(result);
	sql_disconnect();
	sql_renumber_maillist(GET_IDNUM(ch));
}

void sql_scribe_mail(struct char_data *ch, char *arg)
{
	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int recs, value;
  	struct obj_data *obj;
  	char query[MAX_STRING_LENGTH], mail[MAX_STRING_LENGTH];


	if (!*arg) {
		send_to_char(ch, "You must enter a message number to scribe.\r\n");
		return;
	}

  	value = atoi(arg);
  	
	sprintf(query, "SELECT mail_id, from_idnum, sent_time, mesg_status, message FROM %s WHERE mail_id = %d AND to_idnum = %ld",
		mySQL_mail_table, value, GET_IDNUM(ch));
		
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		send_to_char(ch, "That message does not exisit.\r\n");
		return;
	}
	
	if (recs == 1) {
		row = mysql_fetch_row(result);
		
		sprintf(mail, "&MID     : &W%s\r\n&MFrom   : &W%s\r\n&MTo     : &W%s\r\n&MSent At: &W%s\r\n&G==============================================&n\r\n&W%s&n\r\n",
			row[0], sql_get_name_by_id(atoi(row[1])), GET_PC_NAME(ch), row[2], row[4]);

    		obj = create_obj();
    		obj->item_number = NOTHING;
    		obj->name = strdup("mail paper letter");
    		obj->short_description = strdup("a piece of mail");
    		obj->description = strdup("Someone has left a piece of mail here.");

    		GET_OBJ_TYPE(obj) = ITEM_NOTE;
    		GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE | ITEM_WEAR_HOLD;
    		GET_OBJ_WEIGHT(obj) = 1;
    		GET_OBJ_COST(obj) = 30;
    		GET_OBJ_RENT(obj) = 10;
    		obj->action_description = strdup(mail);

    		if (obj->action_description == NULL)
      			obj->action_description = strdup("Mail system error - please report.  Error #11.\r\n");

   		obj_to_char(obj, ch);
   		 
   		send_to_char(ch, "You have scribed message %s.\r\n", row[0]);
		
		sprintf(query, "DELETE FROM %s WHERE mail_id = %d AND to_idnum = %ld", mySQL_mail_table, atoi(row[0]), GET_IDNUM(ch));
		sql_connect();
		sql_real_query(query);		
		
		mysql_free_result(result);
		sql_disconnect();
	}
}

void sql_reply_mail(struct char_data *ch, char *arg)
{
  	char query[MAX_INPUT_LENGTH],**mailwrite;
  	MYSQL_RES *result;
  	MYSQL_ROW row;
  	int recs, value;

  	if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
    		send_to_char(ch, "Sorry, you have to be level %d to send mail!\r\n", MIN_MAIL_LEVEL);
    		return;
  	}

  	if (!*arg) {			/* you'll get no argument from me! */
  		send_to_char(ch, "You need to specify a message to reply to!\r\n");
   	 	return;
  	}

  	value = atoi(arg);

  	sprintf(query, "SELECT from_idnum FROM %s WHERE mail_id = %d AND to_idnum = %ld",
  		mySQL_mail_table, value, GET_IDNUM(ch));
  	sql_connect();
  	sql_real_query(query);
  	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);
  	
	if (recs == 0) {
		mysql_free_result(result);
		sql_disconnect();
		send_to_char(ch, "That message does not exisit.\r\n");
		return;
	}
	
	if (recs == 1) {
		row = mysql_fetch_row(result);
		act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char(ch, "Write your message, (/s saves /h for help)\r\n");
		SET_BIT(PLR_FLAGS(ch), PLR_MAILING);	/* string_write() sets writing. */
		/* Start writing! */
  		CREATE(mailwrite, char *, 1);
  		string_write(ch->desc, mailwrite, MAX_MAIL_SIZE, atoi(row[0]), NULL);
	}
	mysql_free_result(result);
	sql_disconnect();
}

void sql_forward_mail(struct char_data *ch, char *arg)
{
	send_to_char(ch, "Disabled for now.\r\n");
	return;
}

void sql_check_mail(struct char_data *ch)
{
	char query[MAX_STRING_LENGTH];
	MYSQL_RES *result;
  	int recs;
	
	sprintf(query, "SELECT * FROM %s WHERE to_idnum = %ld AND mesg_status = 1",
		mySQL_mail_table, GET_IDNUM(ch));
	
	sql_connect();
	sql_real_query(query);
	result = mysql_store_result(conn);
	recs = mysql_num_rows(result);

	if (recs > 0) {
		send_to_char(ch, "&GYou have unread mudmail in your inbox.&n\r\n");
	}
	
	mysql_free_result(result);
	sql_disconnect(); 
}

void sql_store_mail(struct descriptor_data *d, long to, long from, char *message_pointer)
{
  	char query[MAX_STRING_LENGTH];
  	char *msg_txt = message_pointer;

  	if (from < 0 || to < 0 || !*message_pointer) {
    		log("SYSERR: Mail system -- non-fatal error #5. (from == %ld, to == %ld)", from, to);
    		return;
  	}
 
	sql_connect();
  	sprintf(query, "INSERT INTO %s (id, from_idnum, to_idnum, message, sent_time, mesg_status) VALUES(NULL, %ld, %ld, '%s', NOW(), %d)",
  		mySQL_mail_table, from, to, sql_escape(conn, msg_txt), MSG_UNREAD);
	sql_real_query(query);
	sql_disconnect();
	
	sql_renumber_maillist(to);
}				/* store mail */

/************************
 *  Local ACMD Commands *
 ************************/

#define MAIL_FORMAT \
"&GMail Command Usage :&n\r\n  &Gmail &Wsend    <&MName&W>&n\r\n  &Gmail &Wread    <&M#&W>&n\r\n  &Gmail &Wdelete  <&M#&W>&n\r\n  &Gmail &Wforward <&M#&W> <&MName&W>\r\n  &Gmail &Wreply   <&M#&W>&n\r\n  &Gmail &Wscribe  <&M#&W>&n\r\n"

ACMD(do_mail)
{
	char command[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
	two_arguments(argument, command, arg1);
	
	
	if (!*command) {
		sql_list_mail(ch);
		send_to_char(ch, "%s", MAIL_FORMAT);
		return;
	}
	
	if (is_abbrev(command, "send"))
		sql_send_mail(ch, arg1);
	else if (is_abbrev(command, "delete"))
		sql_delete_mail(ch, arg1);
	else if (is_abbrev(command, "read"))
		sql_read_mail(ch, arg1);
	else if (is_abbrev(command, "scribe"))
		sql_scribe_mail(ch, arg1);
	else if (is_abbrev(command, "reply"))
		sql_reply_mail(ch, arg1);
	else if (is_abbrev(command, "forward"))
		sql_forward_mail(ch, arg1);
	else
		send_to_char(ch, "%s", MAIL_FORMAT);
		return;
}

ACMD(do_last)
{
	send_to_char(ch, "Command Disabled.\r\n");
	return;
}

ACMD(do_social_list)
{
	sql_social_list(ch);
}

ACMD(do_mysql_test)
{
	send_to_char(ch, "Command Disabled.\r\n");
	return;
}

ACMD(do_mysql_login_chk)
{
	sql_login_check(ch);
}
