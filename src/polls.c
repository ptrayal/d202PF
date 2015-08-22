#include "mysql/mysql.h"

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "polls.h"

struct poll_data poll_list[NUM_POLLS];
void free_poll(int pnum);

void add_poll(int pnum, char *question, sbyte active, sbyte revote)
{
  poll_list[pnum].title = strdup(question);
  poll_list[pnum].active = active;
  poll_list[pnum].revote = revote;
  poll_list[pnum].options[0] = strdup("\r\n");
}

void add_poll_option(int pnum, int onum, char *option)
{
  extern MYSQL *conn;

  // Open mysql connection
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) 
  {
    log("Cannot connect to mysql database in add poll option.");
  }

  MYSQL_RES *result = NULL;
  MYSQL_ROW row = NULL;
  char query[300]={'\0'};

  if (onum < 0 || onum > 9)
    return;

  poll_list[pnum].options[onum] = strdup(option);

  if (CONFIG_DFLT_PORT == 9080) 
  {
    sprintf(query, "SELECT name FROM  `poll_data` WHERE  `poll_num` = '%d' AND  `option` = '%d'", pnum, onum);

    mysql_query(conn, query);
    result = mysql_use_result(conn);

    if (result == NULL) 
    {
      poll_list[pnum].votes[onum] = 0;
      log("NO RESULT FOUND FOR POLL NUMBER %d OPTION %d", pnum, onum);
      log(query);
    }
    else 
    {
      while ((row = mysql_fetch_row(result))) 
      {
          poll_list[pnum].votes[onum]++;
          log("RESULT FOR FOR POLL NUMBER %d OPTION %d is %d", pnum, onum, atoi(row[0]));          
          log(query);
      }
    }
  }

  mysql_close(conn);
}

void build_poll_list(void)
{
  add_poll(0, "Test", FALSE, FALSE);
  add_poll_option(0, 1, "Test Option");

  add_poll(1, "How did you hear about d20MUD?", TRUE, FALSE);
  add_poll_option(1, 1, "Through a search engine search.");
  add_poll_option(1, 2, "From a MUD Connector forum post.");
  add_poll_option(1, 3, "From a MUD Connector MUD search.");
  add_poll_option(1, 4, "From a facebook advertisement.");
  add_poll_option(1, 5, "Through a friend / word of mouth.");
  add_poll_option(1, 6, "Another unmentioned method.");
  add_poll_option(1, 7, "From a MUD Connector banner ad.");

  add_poll(5, "What time zone do you normally play from?\r\n", TRUE, TRUE);
  add_poll_option(5, 1, "-12 to -9 GMT");
  add_poll_option(5, 2, "Pacific / -8 GMT");
  add_poll_option(5, 3, "Mountain / -7 GMT");
  add_poll_option(5, 4, "Central / -6 GMT");
  add_poll_option(5, 5, "Eastern / -5 GMT");
  add_poll_option(5, 6, "Atlantic / NFLD / -4 GMT");
  add_poll_option(5, 7, "Europe / Africa / Middle East");
  add_poll_option(5, 8, "Asia / Oceania / Australia");
  add_poll_option(5, 9, "Hawaii");

  add_poll(6, "What kind of death penalty do you prefer?\r\n", TRUE, FALSE);
  add_poll_option(6, 1, "Current System: Exp Loss");
  add_poll_option(6, 2, "Weakness system with reduced damage and hit points for 10 minutes");
  add_poll_option(6, 3, "Weakness system with small penalty to future exp gains for 10 minutes");
  add_poll_option(6, 4, "Other System (please post it on the forums at d20mud.com)");

}

void free_poll(int pnum)
{
  free(poll_list[pnum].title);
  free(poll_list[pnum].options[0]);
}
