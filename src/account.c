#include "conf.h"
#include "sysdep.h"

#include "mysql/mysql.h"
#include "structs.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "pfdefaults.h"
#include "feats.h"
#include "dg_scripts.h"
#include "comm.h"
#include "genmob.h"
#include "constants.h"
#include "spells.h"
#include "polls.h"
#include "screen.h"

//Last editted by Jeremy

extern MYSQL *conn;
extern int circle_shutdown, circle_reboot;
void tag_argument(char *argument, char *tag);

int load_account(char *name, struct account_data *account)
{
  int i = 0;
  FILE *fl;
  char fname[READ_SIZE];
  char buf[128], line[MAX_INPUT_LENGTH + 1], tag[6];
  int num = 0, num2 = 0, num3 = 0;

  if (!account)
    return -1;

  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    account->character_names[i] = NULL;
  account->experience = 0;
  account->gift_experience = 0;
  account->level = 0;
  account->read_rules = 0;
  account->websiteAccount = NULL;
  for (i = 0; i < NUM_POLLS; i++)
    account->polls[i] = 0;
  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
    account->races[i] = 0;
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
    account->classes[i] = 999;
  account->email = strdup("Not Set Yet");

  if (!get_filename(fname, sizeof(fname), ACT_FILE, name))
    return (-1);
  if (!(fl = fopen(fname, "r"))) {
//    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open account file %s", fname);
    return (-1);
  }
  while (get_line(fl, line)) {
    tag_argument(line, tag);

    switch (*tag) {
      case 'C':
        if (!strcmp(tag, "Char")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%s", buf);
            if (strcmp(buf, "-1"))
              account->character_names[i] = strdup(buf);
            i++;
          } while (strcmp(buf, "-1"));
        }
        else if (!strcmp(tag, "Clas")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d", &num2);
            if (num2 != -1)
              account->classes[i] = num2;
            i++;
          } while (num2 != -1);
        }
        break;
      case 'E':
        if (!strcmp(tag, "Exp "))  account->experience = atoi(line);
        break;
      case 'G':
        if (!strcmp(tag, "Gift"))  account->gift_experience = atoi(line);
        break;
      case 'L':
        if (!strcmp(tag, "Levl"))  account->level = atoi(line);
        break;
      case 'M':
        if (!strcmp(tag, "Mail"))  account->email = strdup(line);
        break;
      case 'N':
        if (!strcmp(tag, "Name"))  account->name = strdup(line);
        break;
      case 'P':
        if (!strcmp(tag, "Pswd"))  strcpy(account->password, line);
        else if (!strcmp(tag, "Poll"))  {
          num = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d %d", &num2, &num3);
            if(num2 != -1)
              account->polls[num2] = num3;
            num++;
          } while (num2 != -1);
        }
        break;      
      case 'R':
        if (!strcmp(tag, "Race")) {
          i = 0;
          do {
            get_line(fl, line);
            sscanf(line, "%d", &num2);
            if (num2 != -1)
              account->races[i] = num2;
            i++;
          } while (num2 != -1);
        }
        else if (!strcmp(tag, "Rule"))  account->read_rules = atoi(line);
        break;      
      case 'W':
        if (!strcmp(tag, "WPas"))  account->web_password = strdup(line);
        break;      
    }
  }

  if (fl)
    fclose(fl);

  return 0;

}
void save_account(struct account_data *account)
{
  char fname[100];
  FILE *fl;
  int i = 0;

  if (!account)
    return;

  if (account == NULL || !get_filename(fname, sizeof(fname), ACT_FILE, account->name))
    return;
  if (!(fl = fopen(fname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open account file %s for write", fname);
    char newdir[100];
    if (CONFIG_DFLT_PORT == 4000)
      sprintf(newdir, "/home/aod/d20PlayPort/lib");
    else
      sprintf(newdir, "/home/aod/d20CodePort/lib");

    if (!chdir(newdir)) {
      mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Could not change directory to %s to salvage shutdown necessity.", newdir);
      char curdir[500];
      getcwd(curdir, sizeof(curdir));
      mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Current working directory is %s.", curdir); 
    }

    system("touch crashfilehere");

    if (!(fl = fopen(fname, "w"))) {
      struct char_data *ch = NULL;
      for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || !ch->desc)
          continue;

        send_to_char(ch, 
                   "@l@RThe filesystem has become corrupted and the mud must shut down to avoid lost data.  Please remember\r\n"
                   "to save often to avoid lost data until this bug is fixed.  Thank you for your patience and understanding.@n\r\n");
      }     
      circle_shutdown = 1;
      return;
    }
  }

  fprintf(fl, "Name: %s\n", account->name);
  if (account->email)
  fprintf(fl, "Mail: %s\n", account->email);
  fprintf(fl, "Pswd: %s\n", account->password);
  fprintf(fl, "Rule: %d\n", account->read_rules);
  fprintf(fl, "Exp : %d\n", account->experience);
  fprintf(fl, "Gift: %d\n", account->gift_experience);
  fprintf(fl, "Levl: %d\n", account->level);
  fprintf(fl, "WPas: %s\n", account->web_password);

  i = 0;
  fprintf(fl, "Char:\n");
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    if (account->character_names[i] != NULL)
      fprintf(fl, "%s\n", account->character_names[i]);
  fprintf(fl, "-1\n");

  // Save Poll Data
  fprintf(fl, "Poll:\n");
  for (i = 0; i <= NUM_POLLS; i++) {
    if (account->polls[i] > 0)
      fprintf(fl, "%d %d\n", i, account->polls[i]);
  }
  fprintf(fl, "-1 -1\n");

  // Save Unlocked Races
  fprintf(fl, "Race:\n");
  for (i = 0; i < MAX_UNLOCKED_RACES; i++) {
    fprintf(fl, "%d\n", account->races[i]);
  }
  fprintf(fl, "-1\n");

  // Save Unlocked Classes
  fprintf(fl, "Clas:\n");
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++) {
    fprintf(fl, "%d\n", account->classes[i]);
  }
  fprintf(fl, "-1\n");


  fclose(fl);

  struct descriptor_data *d;

  /* characters */
  for (d = descriptor_list; d; d = d->next) {
    if (d && d->character && d->account && d->account->name && account->name && !strcmp(d->account->name, account->name))
      load_account(account->name, d->account);
  }


}

void show_account_menu(struct descriptor_data *d)
{
  int i = 0;

  write_to_output(d, "ACCOUNT MENU\r\n");
  write_to_output(d, "------------\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "To create a new character type @Ycreate@n.\r\n");
  write_to_output(d, "To delete a character in your account log in as the character and type @Ysuicide@n.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Characters:\r\n");
  write_to_output(d, "-----------\r\n");

  // Open mysql connection
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
    log("Cannot connect to mysql database in enter player game.");
  }

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  char query[MAX_INPUT_LENGTH];

  if (d->account) {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
      if (d->account->character_names[i] != NULL) {
        write_to_output(d, "%d) %-20s", i + 1, d->account->character_names[i]);
        sprintf(query, "SELECT alignment, race, classes, level FROM player_data WHERE name='%s'", d->account->character_names[i]);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL) {
          if ((row = mysql_fetch_row(res)) != NULL) {
            write_to_output(d, "     Level %s %s %s %s", row[3], row[0], row[1], row[2]);
          }
        }
        mysql_free_result(res);
        write_to_output(d, "\r\n");
      }
    }
  }
    mysql_close(conn);



  write_to_output(d, "\r\n");
  write_to_output(d, "Your choice: ");
}

void combine_accounts(void) {

  struct descriptor_data *d;
  struct descriptor_data *k;

  /* characters */
  for (d = descriptor_list; d; d = d->next) {
    for (k = descriptor_list; k; k = k->next) {
      if (d && k && d->account && k->account && d->account != k->account && 
        d->character && k->character && d->character->account_name && k->character->account_name &&
        !strcmp(d->character->account_name, k->character->account_name)) {
        d->account = k->account;
        return;
      }
    }
  }

}

ACMD(do_account)
{

  if (IS_NPC(ch) || !ch->desc || !ch->desc->account) {
    send_to_char(ch, "The account command can only be used by player characters with a valid account.\r\n");
    return;
  }

  struct account_data *acc = ch->desc->account;

  send_to_char(ch,
    "Account Information for %s:\r\n"
    "\r\n"
    "Email: %s\r\n"
    "Level: %d\r\n"
    "Experience: %d\r\n"
    "Gift Experience: %d\r\n"
    "Web Site Password: %s\r\n"
    "Characters:\r\n",
    acc->name, *acc->email ? acc->email : "Not Set", acc->level, acc->experience, acc->gift_experience, acc->web_password);

  int i = 0;
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
    if (acc->character_names[i] != NULL)
      send_to_char(ch, "  %s\r\n", acc->character_names[i]);
  }

  sbyte found = FALSE;

  send_to_char(ch, "Unlocked Advanced Races:\r\n");
  for (i = 0; i < MAX_UNLOCKED_RACES; i++) {
    if (acc->races[i] > 0 && race_list[acc->races[i]].is_pc) {
      send_to_char(ch, "  %s\r\n", race_list[acc->races[i]].name);
      found = TRUE;
    }
  }  
  if (!found)
    send_to_char(ch, "  None.\r\n");

  found = FALSE;

  send_to_char(ch, "Unlocked Prestige Classes:\r\n");
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++) {
    if (acc->classes[i] < 999) {
      send_to_char(ch, "  %s\r\n", class_names_dl_aol[acc->classes[i]]);
      found = TRUE;
    }
  }  
  if (!found)
    send_to_char(ch, "  None.\r\n");

}
