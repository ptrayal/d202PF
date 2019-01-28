/************************************************************************
* hedit.c 	Hedit version 2.0 for Oasis OLC				*
* by Steve Wolfe - siv@cyberenet.net					*
* Updated by Scott Meisenholder for Oasis 2.0.6
* ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "handler.h"

/* List each help entry saved, was used for debugging. */

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct help_index_element *help_table;
extern int top_of_helpt;
extern struct descriptor_data *descriptor_list;
extern void strip_string(char *buffer);
void hedit_disp_menu(struct descriptor_data *d);


ACMD(do_oasis_hedit)
{
  int counter = 0;

  char arg[MAX_INPUT_LENGTH]={'\0'};
  struct descriptor_data *d;
  
  if (GET_OLC_ZONE(ch) != HEDIT_PERMISSION && GET_LEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You don't have access to editing Help files.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_HEDIT) {
      send_to_char(ch, "Sorry, only one can person can edit help files at a time.\r\n");
      return;
    }

  one_argument(argument, arg);
    
  if (!*arg) {
    send_to_char(ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = ch->desc;

  if (!str_cmp("save", arg)) {
    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), true, "OLC: %s saves help files.", GET_NAME(ch));
    send_to_char(ch, "Writing help file..\r\n");
    hedit_save_to_disk(d);
    send_to_char(ch, "Done.\r\n");
    return;
  }
  
  /*
   * Give descriptor an OLC structure.
   */
  if (d && d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  OLC_NUM(d) = 0;
  
   for (OLC_ZNUM(d) = 0; OLC_ZNUM(d) < top_of_helpt; OLC_ZNUM(d)++)
    if (isname(arg, help_table[OLC_ZNUM(d)].keywords))
      break;

  counter = OLC_ZNUM(d);

  if (counter <= 0) 
     hedit_setup_new(d, arg);
  else 
     hedit_setup_existing(d, OLC_ZNUM(d));
  
 

  STATE(d) = CON_HEDIT;
  act("$n starts using OLC.", true, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, ADMLVL_IMMORT, true, "OLC: %s starts editing help files.", GET_NAME(ch));
}


/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void hedit_setup_new(struct descriptor_data *d, char *new_key)
{
  CREATE(OLC_HELP(d), struct help_index_element, 1);

  OLC_HELP(d)->keywords = strdup(new_key);
  OLC_HELP(d)->entry = strdup("This is an unfinished help entry.\r\n");
  hedit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void hedit_setup_existing(struct descriptor_data *d, int rnum)
{
  struct help_index_element *help;

  /*
   * Build a copy of the help entry for editing.
   */
  CREATE(help, struct help_index_element, 1);

  *help = help_table[rnum];
  /*
   * Allocate space for all strings.
   */
  help->keywords = strdup(help_table[rnum].keywords ?
	help_table[rnum].keywords : "UNDEFINED");
  help->entry = strdup(help_table[rnum].entry ?
	help_table[rnum].entry : "undefined\r\n");

  /*
   * Attach copy of help entry to player's descriptor.
   */
  OLC_HELP(d) = help;
  OLC_VAL(d) = 0;
  hedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void hedit_save_internally(struct descriptor_data *d)
{
  int i, rnum;
  struct help_index_element *new_help_table;

  rnum = OLC_ZNUM(d);
  /*
   * Help entry exists exists: free and replace it.
   */
  if (rnum > 0) {
    free_help(help_table + rnum);
    help_table[rnum] = *OLC_HELP(d);
  } else {			/* Entry doesn't exist, hafta add it. */
    CREATE(new_help_table, struct help_index_element, top_of_helpt + 2);

    /*
     * Insert new entry at the top - why not?
     */
    new_help_table[0] = *(OLC_HELP(d));

    /*
     * Count through help table.
     */
    for (i = 0; i <= top_of_helpt; i++)
      new_help_table[i + 1] = help_table[i];

    /*
     * Copy help table over to new one.
     */
    free(help_table);
    help_table = new_help_table;
    top_of_helpt++;
  }
     hedit_save_to_disk(d);
}

/*------------------------------------------------------------------------*/

void hedit_save_to_disk(struct descriptor_data *d)
{
    char buf[MAX_STRING_LENGTH]={'\0'}, buf1[MAX_STRING_LENGTH]={'\0'}, buf2[MAX_STRING_LENGTH]={'\0'};
    FILE *fp;
    struct help_index_element *help;
    int i = 0;

    sprintf(buf, "%s/%s.new", HLP_PREFIX, HELP_FILE);
    if (!(fp = fopen(buf, "w+"))) 
    {
        mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: Cannot open help file!");
        return;
    }

    for (i = 0; i <= top_of_helpt; i++) 
    {
        help = (help_table + i);

        if (HEDIT_LIST)
        {
            sprintf(buf1, "OLC: Saving help entry %d.", i);
            log("%s", buf1);
        }

        /*
        * Remove the '\r\n' sequences from description.
        */
        strcpy(buf1, help->entry ? help->entry : "Empty");
        strip_cr(buf1);

        /*
        * Forget making a buffer, lets just write the thing now.
        */
        fprintf(fp, "%s\n%s%s#%d\n",
            help->keywords ? help->keywords : "UNDEFINED", buf1, help->entry ? "" : "\n",
            help->min_level);
    }

    /*
    * Write final line and close.
    */
    fprintf(fp, "$~\n");
    fclose(fp);
    sprintf(buf2, "%s/%s", HLP_PREFIX, HELP_FILE);
    /*
    * We're fubar'd if we crash between the two lines below.
    */
    rename(buf, buf2);
//  remove(buf);
}


/*------------------------------------------------------------------------*/

void free_help(struct help_index_element *help)
{

  if (help->keywords)
    free(help->keywords);
  if (help->entry)
    free(help->entry);

/*  free(help); */
/* Had to remove due to crash each edit and atempt to save internally    *
 * There may be a memory leak, but if this function not used extensively *
 * then it would probably not harm much.                                 */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * The main menu.
 */
void hedit_disp_menu(struct descriptor_data *d)
{
  struct help_index_element *help;
  
  help = OLC_HELP(d);

  write_to_output(d,
#if defined(CLEAR_SCREEN)
	  ""
#endif
	  "@c1@n) Keywords    : @y%s\r\n"
	  "@c2@n) Entry       :\r\n@y%s"
	  "@c3@n) Min Level   : @y%d\r\n"
	  "@cQ@n) Quit\r\n"
	  "Enter choice : ",

	  help->keywords,
	  help->entry,
	  help->min_level
	  );
   OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void hedit_parse(struct descriptor_data *d, char *arg)
{
  char buf[MAX_STRING_LENGTH]={'\0'};
  int number = 0;

  switch (OLC_MODE(d)) {
  case HEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      hedit_save_internally(d);
      snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character), OLC_HELP(d)->keywords);
      mudlog(true, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, buf);
      write_to_output(d, "Help files saved to disk.\r\n");
     
      /*
       * do NOT free strings! Just the help structure. 
       */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /*
       * Free everything up, including strings, etc.
       */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save this help entry internally? : \r\n");
      break;
    }
    return;

  case HEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Something has been modified. */
	write_to_output(d, "Do you wish to save this help entry? : ");
	OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      write_to_output(d, "Enter keywords:-\r\n] ");
      OLC_MODE(d) = HEDIT_KEYWORDS;
      break;
    case '2':
      OLC_MODE(d) = HEDIT_ENTRY;
#if defined(CLEAR_SCREEN)
      write_to_output(d, "\x1B[H\x1B[J");
#endif
      write_to_output(d, "Enter help entry: (/s saves /h for help)\r\n");
      d->backstr = NULL;
      if (OLC_HELP(d)->entry) {
	write_to_output(d, "%s", OLC_HELP(d)->entry);
	d->backstr = strdup(OLC_HELP(d)->entry);
      }
      d->str = &OLC_HELP(d)->entry;
      d->max_str = MAX_HELP_ENTRY;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      write_to_output(d, "********************************************************************************\r\n");
      write_to_output(d, "********************************************************************************\r\n");
      break;
    case '3':
      write_to_output(d, "Enter min level:-\r\n] ");
      OLC_MODE(d) = HEDIT_MIN_LEVEL;
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      hedit_disp_menu(d);
      break;
    }
    return;

  case HEDIT_KEYWORDS:
    if (OLC_HELP(d)->keywords)
      free(OLC_HELP(d)->keywords);
    if (strlen(arg) > MAX_HELP_KEYWORDS)
      arg[MAX_HELP_KEYWORDS - 1] = '\0';
    strip_cr(arg);
    OLC_HELP(d)->keywords = strdup((arg && *arg) ? arg : "UNDEFINED");
    break;

  case HEDIT_ENTRY:
    /*
     * We will NEVER get here, we hope.
     */
    mudlog(true, ADMLVL_BUILDER, BRF, "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
    break;

  case HEDIT_MIN_LEVEL:
    number = atoi(arg);
    if ((number < 0) || (number > ADMLVL_IMPL))
      write_to_output(d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
    else {
      OLC_HELP(d)->min_level = number;
      break;
    }
    return;

  default:
    /*
     * We should never get here.
     */
    mudlog(true, ADMLVL_BUILDER, BRF, "SYSERR: Reached default case in parse_hedit");
    break;
  }
  /*
   * if we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  hedit_disp_menu(d);
}

void hedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case HEDIT_ENTRY:
    default:
      hedit_disp_menu(d);
      break;
 }
}



