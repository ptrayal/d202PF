#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: spec_procs.c 62 2009-03-25 23:06:34Z gicker $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "spec_procs.h"
#include "feats.h"
#include "oasis.h"
#include "pets.h"
#include "house.h"
#include "dg_scripts.h"
#include "clan.h"

#include "mysql/mysql.h"

void perform_mail_delete(struct char_data *ch, int mnum);
void perform_mail_list(struct char_data *ch, int type);
void perform_mail_read(struct char_data *ch, int mnum);

extern MYSQL *conn;
extern struct clan_type *clan_info;

void send_editor_help(struct descriptor_data *d);

ACMD(do_new_mail)
{

  char arg3[200], arg4[200];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Commands are:\r\n"
                     "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                     "-- mail list sent             : (shows the mail in your outbox)\r\n"
                     "-- mail read <num>            : (read the specified mail)\r\n"
                     "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                     "-- mail send <name> <subject> : (sends a mail to specified person)\r\n"
                );
    return;
  } else {
    half_chop(argument, arg3, arg4);
    
    if (!*arg3) {
      send_to_char(ch, "Commands are:\r\n"
                       "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                       "-- mail list sent             : (shows the mail in your outbox)\r\n"
                       "-- mail read <num>            : (read the specified mail)\r\n"
                       "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                       "-- mail send <name> <subject> : (sends a mail to specified person)\r\n"
                  );
      return;
    }
    if (is_abbrev(arg3, "list")) {
//      send_to_char(ch, "MYSQL_SERVER: %s\nMYSQL_USER: %s\nMYSQL_DB: %s\nMYSQL_PASSWD: %s\n", MYSQL_SERVER, MYSQL_USER, MYSQL_DB, MYSQL_PASSWD);
      if (!*arg4 || is_abbrev(arg4, "inbox")) {
        perform_mail_list(ch, 1);
        return;
      } else if (is_abbrev(arg4, "sent")) {
        perform_mail_list(ch, 2);
        return;
      } else {
        send_to_char(ch, "Options are: mail list (inbox|sent)\r\n");
        return;
      }
    } else if (is_abbrev(arg3, "read")) {
      if (!*arg4) {
        send_to_char(ch, "You must specify which mail idnum you wish to read.\r\n");
        return;
      }
      int mnum = atoi(arg4);
      if (mnum <= 0) {
        send_to_char(ch, "The mail idnum must be greater than zero.\r\n");
        return;
      }
      perform_mail_read(ch, mnum);
      return;
    } else if (is_abbrev(arg3, "delete")) {
      if (!*arg4) {
        send_to_char(ch, "You must specify which mail idnum you wish to delete.\r\n");
        return;
      }
      int mnum = atoi(arg4);
      if (mnum <= 0) {
        send_to_char(ch, "The mail idnum must be greater than zero.\r\n");
        return;
      }
      perform_mail_delete(ch, mnum);
      return;  
    } else if (is_abbrev(arg3, "send")) {
      extern int circle_copyover;
      if (circle_copyover) {
        send_to_char(ch, "A copyover is scheduled.  Please wait until after to write your mail.\r\n");
        return;
      }
      char arg5[200], arg6[200];
      half_chop(arg4, arg5, arg6);
      if (!*arg5) {
        send_to_char(ch, "You must specify a mail recipient.\r\n");
        return;
      }
      if (!*arg6) {
        send_to_char(ch, "You must specify a mail subject.\r\n");
        return;
      }

      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;
      int found = FALSE;

      // Open mysql connection
      conn = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      sprintf(arg5, "%s", CAP(arg5));

      char query[MAX_INPUT_LENGTH];
      char *end;
      end = stpcpy(query, "SELECT name FROM player_data WHERE name=");
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, arg5, strlen(arg5));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        if ((row = mysql_fetch_row(res)) != NULL) {
          found = TRUE;
        }
      }
      mysql_free_result(res);

      mysql_close(conn);

//      send_to_char(ch, "%s\r\n", query);

      struct clan_type *cptr = NULL;

      for (cptr = clan_info; cptr; cptr = cptr->next) {

          if (cptr == NULL) {
            continue;
          }

          if (!strcmp(cptr->member_look_str, arg5)) {
            found = TRUE;
//            send_to_char(ch, "%s\r\n", cptr->member_look_str);
            break;
          }
      }


      if (!found && (strcmp(arg5, "All") || GET_ADMLEVEL(ch) < 4)) {
        send_to_char(ch, "That character doesn't exist in our mail database.\r\n");
        return;
      } 

      ch->new_mail_receiver = strdup(CAP(arg5));
      ch->new_mail_subject = strdup(arg6);
      if (ch->new_mail_content) {
        ch->new_mail_content = NULL;
      }

      send_editor_help(ch->desc);
      act("$n starts to write a mail.", TRUE, ch, 0, 0, TO_ROOM);
      SET_BIT_AR(PLR_FLAGS(ch), PLR_MAILING);     /* string_write() sets writing. */
      ch->new_mail_content = strdup("@n");
      string_write(ch->desc, &ch->new_mail_content, MAX_STRING_LENGTH, -999, NULL);
      STATE(ch->desc) = CON_NEWMAIL;
      send_to_char(ch, "Please write your mail in the space below.  Type /s when you are done.\r\n\r\n");
      return;
    } else {
      send_to_char(ch, "Commands are:\r\n"
                       "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                       "-- mail list sent             : (shows the mail in your outbox)\r\n"
                       "-- mail read <num>            : (read the specified mail)\r\n"
                       "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                       "-- mail send <name> <subject> : (sends a mail to specified person)\r\n"
                  );
      return;
    }
  }
}

void perform_mail_list(struct char_data *ch, int type) {
      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;
      MYSQL_RES *res2 = NULL;
      MYSQL_ROW row2 = NULL;
      MYSQL *conn2 = NULL;
      MYSQL_RES *res3 = NULL;
      MYSQL_ROW row3 = NULL;
      MYSQL *conn3 = NULL;
      // Open mysql connection
      conn = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      conn2 = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn2, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      conn3 = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn3, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      sbyte unread = TRUE, deleted = FALSE;

      send_to_char(ch, "    %-7s %-20s %s\r\n", "MAIL ID", type != 1 ? "RECIPIENT" : "SENDER", "SUBJECT");
      send_to_char(ch, "    %-7s %-20s %s\r\n", "-------", "--------------------", "-----------------------------------");
      char query[MAX_INPUT_LENGTH];
      sprintf(query, "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE %s='%s' OR %s='All' ORDER BY mail_id DESC", type == 1 ? "receiver" : "sender", GET_NAME(ch), type == 1 ? "receiver" : "sender");
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        while ((row = mysql_fetch_row(res)) != NULL) {

          unread = TRUE; deleted = FALSE;

          sprintf(query, "SELECT mail_id,player_name FROM player_mail_deleted WHERE player_name='%s' AND mail_id='%s'", GET_NAME(ch), row[0]);
          mysql_query(conn2, query);
          res2 = mysql_use_result(conn2);
          if (res2 != NULL) {
            if ((row2 = mysql_fetch_row(res2)) != NULL) {
              deleted = TRUE;
            }
          }
          mysql_free_result(res2);

          sprintf(query, "SELECT mail_id,player_name FROM player_mail_read WHERE player_name='%s' AND mail_id='%s'", GET_NAME(ch), row[0]);
//          send_to_char(ch, "%s\r\n", query);
          mysql_query(conn3, query);
          res3 = mysql_use_result(conn3);
          if (res3 != NULL) {
            if ((row3 = mysql_fetch_row(res3)) != NULL) {
              unread = FALSE;
            }
          }
          mysql_free_result(res3);

          if (!deleted) {
            send_to_char(ch, "%-3s %-7s %-20s %s\r\n", unread ? "NEW" : "",
                         row[0], type == 1 ? row[1] : row[2], row[3]
                        );
          }
        }
      }       
      mysql_free_result(res);

      mysql_close(conn);
      mysql_close(conn2);
      mysql_close(conn3);
}

void perform_mail_read(struct char_data *ch, int mnum) {

      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;
      MYSQL *conn2 = NULL;

      // Open mysql connection
      conn = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      // Open mysql connection
      conn2 = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn2, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      char mnums[20];

      sprintf(mnums, "%d", mnum);

      sbyte found = FALSE;

      char query[MAX_INPUT_LENGTH];
      sprintf(query, "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE sender='%s' OR receiver='%s' OR receiver='All'", GET_NAME(ch), GET_NAME(ch));
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        if ((row = mysql_fetch_row(res)) != NULL) {
          found = TRUE;
        }
      }
      mysql_free_result(res);

      if (!found) {
        send_to_char(ch, "That mail is not accessible to you.\r\n");
        return;
      }
      found = FALSE;

      char *end;
      char buf[200];

      end = stpcpy(query, "SELECT mail_id,sender,receiver,subject,message FROM player_mail WHERE mail_id=");
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn, query);

      res = mysql_use_result(conn);
      if (res != NULL) {
        if ((row = mysql_fetch_row(res)) != NULL) {
          found = TRUE;
          send_to_char(ch, "Mail Id: %s Sender: %s Recipient: %s\r\n"
                           "Subject: %s\r\n"
                           "Message:\r\n"
                           "%s\r\n\r\n",
                           row[0], row[1], row[2], row[3], row[4]
                      );
          sprintf(buf, "DELETE FROM player_mail_read WHERE player_name='%s' AND mail_id=", GET_NAME(ch));
          end = stpcpy(query, buf);
          *end++ = '\'';
          end += mysql_real_escape_string(conn2, end, mnums, strlen(mnums));
          *end++ = '\'';
          *end++ = '\0';
          mysql_query(conn2, query);

          sprintf(buf, "INSERT INTO player_mail_read (player_name, mail_id) VALUES('%s',", GET_NAME(ch));
          end = stpcpy(query, buf);
          *end++ = '\'';
          end += mysql_real_escape_string(conn2, end, mnums, strlen(mnums));
          *end++ = '\'';
          *end++ = ')';
          *end++ = '\0';
          mysql_query(conn2, query);

        }
      }
      mysql_free_result(res);

      mysql_close(conn);
      mysql_close(conn2);

      if (!found) {
        send_to_char(ch, "There is no mail by that id number.\r\n");
      }

}

void perform_mail_delete(struct char_data *ch, int mnum) {

      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;

      // Open mysql connection
      conn = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      char mnums[20];

      sprintf(mnums, "%d", mnum);

      sbyte found = FALSE;

      char query[MAX_INPUT_LENGTH];
      sprintf(query, "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE sender='%s' OR receiver='%s' OR (receiver='All')", GET_NAME(ch), GET_NAME(ch));
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        if ((row = mysql_fetch_row(res)) != NULL) {
          found = TRUE;
        }
      }
      mysql_free_result(res);

      if (!found) {
        send_to_char(ch, "That mail is not accessible to you.\r\n");
        return;
      }
      found = FALSE;

      char *end;
      char buf[200];
      sprintf(buf, "DELETE FROM player_mail_read WHERE player_name='%s' AND mail_id=", GET_NAME(ch));
      end = stpcpy(query, buf);
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn, query);

      sprintf(buf, "DELETE FROM player_mail_deleted WHERE player_name='%s' AND mail_id=", GET_NAME(ch));
      end = stpcpy(query, buf);
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn, query);

      sprintf(buf, "INSERT INTO player_mail_deleted (player_name, mail_id) VALUES('%s',", GET_NAME(ch));
      end = stpcpy(query, buf);
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = ')';
      *end++ = '\0';
      mysql_query(conn, query);

      mysql_close(conn);



      send_to_char(ch, "You have successfully deleted that mail.\r\n");

}

void new_mail_alert(struct char_data *ch)
{
      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;
      MYSQL_RES *res2 = NULL;
      MYSQL_ROW row2 = NULL;
      MYSQL *conn2 = NULL;
      MYSQL_RES *res3 = NULL;
      MYSQL_ROW row3 = NULL;
      MYSQL *conn3 = NULL;
      // Open mysql connection
      conn = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      conn2 = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn2, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      conn3 = mysql_init(NULL);

      /* Connect to database */
      if (!mysql_real_connect(conn3, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("Cannot connect to mysql database in mail send.");
      }

      sbyte unread = TRUE, deleted = FALSE;
      int num_unread = 0, num_mails = 0, num_read = 0, num_deleted = 0;;
      
      char query[MAX_INPUT_LENGTH];
      sprintf(query, "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE receiver='%s' OR %s='All' ORDER BY mail_id DESC", GET_NAME(ch), "receiver");
      mysql_query(conn, query);
      res = mysql_use_result(conn);
      if (res != NULL) {
        while ((row = mysql_fetch_row(res)) != NULL) {

          unread = TRUE; deleted = FALSE; num_mails++;

          sprintf(query, "SELECT mail_id,player_name FROM player_mail_deleted WHERE player_name='%s' AND mail_id='%s'", GET_NAME(ch), row[0]);
          mysql_query(conn2, query);
          res2 = mysql_use_result(conn2);
          if (res2 != NULL) {
            if ((row2 = mysql_fetch_row(res2)) != NULL) {
              deleted = TRUE;
              num_deleted++;
            }
          }
          mysql_free_result(res2);

          sprintf(query, "SELECT mail_id,player_name FROM player_mail_read WHERE player_name='%s' AND mail_id='%s'", GET_NAME(ch), row[0]);
          mysql_query(conn3, query);
          res3 = mysql_use_result(conn3);
          if (res3 != NULL) {
            if ((row3 = mysql_fetch_row(res3)) != NULL) {
              unread = FALSE;
              num_read++;                            
            }
          }
          mysql_free_result(res3);
        }
      }       
      mysql_free_result(res);

      mysql_close(conn);
      mysql_close(conn2);
      mysql_close(conn3);

      num_unread = num_mails - num_read - num_deleted;

      if (num_unread > 0) {
        send_to_char(ch, "@Y@lYou have %d NEW mail messages!\r\n@n", num_unread);
      }
}
