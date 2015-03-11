/* ***********************************************************************
*  File: alias.c				A utility to CircleMUD	 *
* Usage: writing/reading player's aliases.				 *
*									 *
* Code done by Jeremy Hess and Chad Thompson				 *
* Modifed by George Greer for inclusion into CircleMUD bpl15.		 *
*									 *
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		 *
*********************************************************************** */

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: alias.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"

void write_aliases(struct char_data *ch);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);

void write_aliases(struct char_data *ch)
{
  FILE *file;
  char fn[MAX_STRING_LENGTH];
  struct alias_data *temp;

  get_filename(fn, sizeof(fn), ALIAS_FILE, GET_NAME(ch));
  remove(fn);

  if (GET_ALIASES(ch) == NULL)
    return;

  if ((file = fopen(fn, "w")) == NULL) {
    log("SYSERR: Couldn't save aliases for %s in '%s': %s", GET_NAME(ch), fn, strerror(errno));
    /*  SYSERR_DESC:
     *  This error occurs when the server fails to open the relevant alias
     *  file for writing.  The text at the end of the error should give a
     *  valid reason why.
     */
    return;
  }

  for (temp = GET_ALIASES(ch); temp; temp = temp->next) {
    int aliaslen = strlen(temp->alias);
    int repllen = strlen(temp->replacement) - 1;

    fprintf(file, "%d\n%s\n"	/* Alias */
		  "%d\n%s\n"	/* Replacement */
		  "%d\n",	/* Type */
		aliaslen, temp->alias,
		repllen, temp->replacement + 1,
		temp->type);
  }
  
  fclose(file);
}

void read_aliases(struct char_data *ch)
{   
  FILE *file;
  char xbuf[MAX_STRING_LENGTH];
  struct alias_data *t2, *prev = NULL;
  int length;

  get_filename(xbuf, sizeof(xbuf), ALIAS_FILE, GET_NAME(ch));

  if ((file = fopen(xbuf, "r")) == NULL) {
    if (errno != ENOENT) {
      log("SYSERR: Couldn't open alias file '%s' for %s: %s", xbuf, GET_NAME(ch), strerror(errno));
      /*  SYSERR_DESC:
       *  This error occurs when the server fails to open the relevant alias
       *  file for reading.  The text at the end version should give a valid
       *  reason why.
       */
    }
    return;
  }
 
  CREATE(GET_ALIASES(ch), struct alias_data, 1);
  t2 = GET_ALIASES(ch); 

  char *trash = NULL;

  for (;;) {
    /* Read the aliased command. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    trash = fgets(xbuf, length + 1, file);
    t2->alias = strdup(xbuf);

    /* Build the replacement. */
    if (fscanf(file, "%d\n", &length) != 1)
       goto read_alias_error;

    *xbuf = ' ';		/* Doesn't need terminated, fgets() will. */
    trash = fgets(xbuf + 1, length + 1, file);

    t2->replacement = strdup(xbuf); 

    /* Figure out the alias type. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    t2->type = length; 

    if (feof(file))
      break;

    CREATE(t2->next, struct alias_data, 1);
    prev = t2;
    t2 = t2->next;
  }; 
  
  fclose(file);
  return;

read_alias_error:
  if (t2->alias)
    free(t2->alias);
  free(t2);
  if (prev)
    prev->next = NULL;
  fclose(file);
} 

void delete_aliases(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), ALIAS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting alias file %s: %s", filename, strerror(errno));
    /*  SYSERR_DESC:
     *  When an alias file cannot be removed, this error will occur,
     *  and the reason why will be the tail end of the error.
     */
}

