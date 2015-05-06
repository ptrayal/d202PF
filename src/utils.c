/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include <math.h>

SVNHEADER("$Id: utils.c 62 2009-03-25 23:06:34Z gicker $");

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "feats.h"
#include "constants.h"
#include "genolc.h"
#include "oasis.h"
#include "pets.h"


#define isspace_ignoretabs(c) ((c)!='\t' && isspace(c))

// local globals

long times_harvested[1000000];

/* external globals */
extern struct time_data time_info;
extern int class_hit_die_size_dl_aol[NUM_CLASSES];
extern mob_vnum *monsum_list[9][9];
extern const char *AssemyTypes[MAX_ASSM+1];
extern int assembly_skills[MAX_ASSM];
extern int spell_sort_info[SKILL_TABLE_SIZE + 1];
extern int level_feats[][6];

/* external functions */
byte can_use_available_actions(struct char_data *ch, byte action);
int get_touch_ac(struct char_data *ch);
char *current_short_desc(struct char_data *ch);
int find_armor_type(int specType);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
int count_metamagic_feats(struct char_data *ch);
int has_intro(struct char_data *ch, struct char_data *target);
char *which_desc(struct char_data *ch);
char * change_coins(int coins);
void modify_coins(struct char_data *ch, int type, int operand, int amount) ;
void convert_coins(struct char_data *ch);
int get_skill_value(struct char_data *ch, int skillnum);
int skill_roll(struct char_data *ch, int skillnum);
int get_skill_mod(struct char_data *ch, int skillnum);
int strlencolor( char *arg);
int has_favored_enemy(struct char_data *ch, struct char_data *victim);
int highest_armor_type(struct char_data *ch);
int level_exp(int level, int race);
int art_level_exp(int level);
int guild_level_exp(int level);

/* local functions */
void get_name(struct char_data* ch, char** chname);
int is_player_grouped(struct char_data *target, struct char_data *group);
int get_combat_bonus(struct char_data *ch);
int get_combat_defense(struct char_data *ch);

int art_level_exp(int level) {
  return (level * 200) + ((level > 0) ? art_level_exp(level - 1) : 0);
}

int guild_level_exp(int level) {
  return ((level - 1) * 1000) + ((level > 1) ? art_level_exp(level - 2) : 0);
}

void get_name(struct char_data* ch, char** chname)
{
  *chname = (char*) NULL;

  if (ch != (struct char_data*) NULL)
  {
    if (IS_NPC(ch))
    {
      *chname = strdup((ch)->player_specials->short_descr);
    }
    else
    {
      *chname = strdup((ch)->name);
      *chname = strtok(*chname, " ");
    }
  }
}

void get_name_II(struct char_data* ch, const struct char_data* vi, char** chname
)
{
  *chname = (char*) NULL;

  if (ch != (struct char_data*) NULL)
  {
    if (IS_NPC(ch))
    {
      *chname = strdup((ch)->player_specials->short_descr);
    }
    else if (has_intro((struct char_data *) vi, ch))
    {
      *chname = strdup((ch)->name);
      *chname = strtok(*chname, " ");
    }
    else
    {
      *chname = strdup((ch)->player_specials->short_descr);
    }
  }
}

void get_name_IV(struct char_data* ch, char** chname)
{
  *chname = (char*) NULL;

  if (ch != (struct char_data*) NULL)
  {
    if (IS_NPC(ch))
    {
      *chname = strdup((ch)->player_specials->short_descr);
    }
    else if (PRF_FLAGGED(ch, PRF_NOTSELF)) {
      *chname = strdup((ch)->player_specials->name_dis);
    }
    else
    {
      *chname = strdup((ch)->name);
      *chname = strtok(*chname, " ");
    }
  }
}

void get_name_III(struct char_data* ch, const struct char_data* vi, char** chname)
{
  *chname = (char*) NULL;

  if (ch != (struct char_data*) NULL)
  {
    if (IS_NPC(ch))
    {
      *chname = strdup((ch)->player_specials->short_descr);
    }
    else if (has_intro((struct char_data *) vi, ch) && !PRF_FLAGGED(ch, PRF_NOTSELF))    {
      *chname = strdup((ch)->name);
      *chname = strtok(*chname, " ");
    }

 else if (has_intro((struct char_data *) vi, ch) && PRF_FLAGGED(ch, PRF_NOTSELF) && PRF_FLAGGED(vi, PRF_DETECT)) {
      *chname = strdup((ch)->name);
      *chname = strtok(*chname, " ");
    }


   else if (PRF_FLAGGED(ch, PRF_NOTSELF) && !PRF_FLAGGED(vi, PRF_DETECT)) {
      *chname = strdup((ch)->player_specials->name_dis);
    }

   else {
      *chname = strdup((ch)->player_specials->short_descr);
    }
  }
}


void get_pers(struct char_data* ch, const struct char_data* vi, char** chname)
{
    if (PRF_FLAGGED(ch, PRF_NOTSELF)) {
      *chname = strdup((ch)->player_specials->name_dis);
    }

   else if (CAN_SEE((struct char_data *)vi, ch))
   {
      get_name_II(ch, vi, chname);
   }
   else
   {
      *chname = strdup("someone");
   }
}

void get_pers_II(struct char_data* ch, const struct char_data* vi, char** chname)
{
   if (CAN_SEE((struct char_data *)vi, ch))
   {
      get_name_II(ch, vi, chname);
   }
   else
   {
      *chname = strdup("someone");
   }
}

void choose_name(struct char_data* ch,  char** chname)
{
    if (PRF_FLAGGED(ch, PRF_NOTSELF)) {
     get_name_IV(ch, chname);

    }
   else
   {
     get_name(ch, chname);
   }
}

void choose_name_II(struct char_data* ch, const struct char_data* vi, char** chname)
{
    if (PRF_FLAGGED(ch, PRF_NOTSELF)) {
      get_name_III(ch, vi, chname);
    }
    else 
    {
     get_name_II(ch, vi, chname);
    }
}

/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /*
   * this should always be of the form:
   *
   *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.
   */
  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}

int min_dice(int num, int size, int min)
{
  int sum = 0;
  int roll = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0) {
    roll = rand_number(1, size);
    if (roll < min)
      roll = min;
    sum += roll;
  }

  return (sum);
}


/* Be wary of sign issues with this. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/* Be wary of sign issues with this. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}

char *CAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@'); i += 2);

  txt[i] = UPPER(txt[i]);
  return (txt);
}

char *UNCAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@'); i += 2);

  txt[i] = LOWER(txt[i]);
  return (txt);
}


#if !defined(HAVE_STRLCPY)
/*
 * A 'strlcpy' function in the same fashion as 'strdup' below.
 *
 * this copies up to totalsize - 1 bytes from the source string, placing
 * them and a trailing NUL into the destination string.
 *
 * Returns the total length of the string it tried to copy, not including
 * the trailing NUL.  So a '>= totalsize' test says it was truncated.
 * (Note that you may have _expected_ truncation because you only wanted
 * a few characters from the source string.)
 */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif


#if !defined(HAVE_STRDUP)
/* Create a duplicate of a string */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}


#ifndef str_cmp
/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


#ifndef strn_cmp
/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, ADMLVL_IMMORT, true, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/*
 * new variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }
/*
  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;
*/
  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (!i || !i->character)
      continue;
    if (level == ADMLVL_NONE && (GET_ADMLEVEL(i->character) != 0 || (i->account && i->account->level != 0)))
      continue;
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level && i->account && i->account->level < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}



/*
 * if you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "NOBITS ", reslen);

  return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
  int nr, teller, found = false;

  *result = '\0';

  for(teller = 0; teller < maxar && !found; teller++)
    for (nr = 0; nr < 32 && !found; nr++) {
	  if (IS_SET_AR(bitvector, (teller*32)+nr)) {
        if (*names[(teller*32)+nr] != '\n') {
          if (*names[(teller*32)+nr] != '\0') {

    strcat(result, names[(teller*32)+nr]);

    strcat(result, " ");
          }
		} else {

  strcat(result, "UNDEFINED ");
        }
	  }
      if (*names[(teller*32)+nr] == '\n')
        found = true;
    }

  if (!*result)
    strcpy(result, "NOBITS ");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 30;	/* 0..29 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 12;	/* 0..11 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(NULL) - when);
}

struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->time.birth);

  return (&player_age);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (true);
  }

  return (false);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master == NULL) {
    core_dump();
    return;
  }

    act("You stop following $N.", false, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", true, ch, 0, ch->master, TO_NOTVICT);
  if (!(DEAD(ch->master) || (ch->master->desc && STATE(ch->master->desc) == CON_MENU)))
    act("$n stops following you.", true, ch, 0, ch->master, TO_VICT);

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
}


int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  /* Summoned creatures don't count against total */
  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) &&
        !AFF_FLAGGED(lackey->follower, AFF_SUMMONED) &&
        lackey->follower->master == ch)
      total++;

  return (total);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", false, ch, 0, leader, TO_CHAR);
  if (IN_ROOM(ch) != NOWHERE && IN_ROOM(leader) != NOWHERE && CAN_SEE(leader, ch)) {
    act("$n starts following you.", true, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", true, ch, 0, leader, TO_NOTVICT);
  }
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.
 */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case NEW_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case BACKUP_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = SUF_BAK;
    break;
  case HOUSE_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = SUF_HOUSE;
    break;
  case OLD_OBJ_FILES:
    prefix = LIB_PLROBJS;
     suffix = SUF_OLD_OBJS;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  case PET_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PET;
    break;
  case PET_FILE_NEW:
	  prefix = LIB_PLRFILES;
	  suffix = SUF_PET_NEW;
	  break;
  case ACT_FILE:
    prefix = LIB_ACTFILES;
    suffix = SUF_ACT;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/*
 * this function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
extern FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 0	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  fflush(player_fl);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
#endif
}


/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (false);
  }

  if (world[room].light)
    return (false);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (true);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (false);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (true);

  return (false);
}

int count_metamagic_feats(struct char_data *ch)
{
  int count = 0;                // Number of Metamagic Feats Known

  if (HAS_FEAT(ch, FEAT_STILL_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_SILENT_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_QUICKEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_HEIGHTEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EXTEND_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EMPOWER_SPELL))
    count++;

  return count;
}

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */

#include <sys/types.h>
#include <sys/stat.h>

#ifdef CIRCLE_WINDOWS
#include <direct.h>
/* I shouldn't need to include the following line, right? */
#include <windows.h>

int xdir_scan(char *dir_name, struct xap_dir *xapdirptest) {
  HANDLE dirhandle;
  WIN32_FIND_DATA wtfd;
  int i, total = 0;
  struct xap_dir *xapdirp;

  xapdirp = xapdirptest;

  xapdirp->current = -1;
  xapdirp->total = -1;

  dirhandle = FindFirstFile(dir_name, &wtfd);

  if(dirhandle == INVALID_HANDLE_VALUE) {
    return -1;
  }

  (xapdirp->total)++;
  while(FindNextFile(dirhandle, &wtfd)) {
    (xapdirp->total)++;
  }

  if(GetLastError() != ERROR_NO_MORE_FILES) {
    xapdirp->total = -1;
    return -1;
  }

  FindClose(dirhandle);
  dirhandle = FindFirstFile(dir_name, &wtfd);

  xapdirp->namelist = (char **) malloc(sizeof(char *) * total);

  i = 0;
  while(FindNextFile(dirhandle, &wtfd) != 0) {
    xapdirp->namelist[i] = strdup(wtfd.cFileName);
    i++;
  }
  FindClose(dirhandle);

  xapdirp->current=0;
  return xapdirp->total;
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i];
  }

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1];
}

#else
#include <dirent.h>
#include <unistd.h>

int xdir_scan(char *dir_name, struct xap_dir *xapdirp) {
  xapdirp->total = scandir(dir_name,&(xapdirp->namelist),0,alphasort);
  xapdirp->current = 0;

  return(xapdirp->total);
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i]->d_name;
}

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1]->d_name;
}

#endif

void xdir_close(struct xap_dir *xd) {
  int i;
  for(i=0;i < xd->total;i++) {
    free(xd->namelist[i]);
  }
  free(xd->namelist);
  xd->namelist = NULL;
  xd->current = xd->total = -1;
}

int xdir_get_total(struct xap_dir *xd) {
  return xd->total;
}

int insure_directory(char *path, int isfile) {
  char *chopsuey = strdup(path);
  char *p;
  char *temp;
#ifdef CIRCLE_WINDOWS
  struct _stat st;
#else
  struct stat st;
#endif

  extern int errno;

  // if it's a file, remove that, we're only checking dirs;
  if(isfile) {
    if(!(p=strrchr(path,'/'))) {
      free(chopsuey);
      return 1;
    }
    *p = '\0';
  }

  // remove any trailing /'s

  while(chopsuey[strlen(chopsuey)-1] == '/') {
    chopsuey[strlen(chopsuey) -1 ] = '\0';
  }

  // check and see if it's already a dir

  #ifndef S_ISDIR
  #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
  #endif

  #ifdef CIRCLE_WINDOWS
  if(!_stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
  #else
  if(!stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
  #endif
    free(chopsuey);
    return 1;
  }

  temp = strdup(chopsuey);
  if((p = strrchr(temp,'/')) != NULL) {
    *p = '\0';
  }
  if(insure_directory(temp,0) &&

#ifdef CIRCLE_WINDOWS
     !_mkdir(chopsuey)) {
#else
     !mkdir(chopsuey, S_IRUSR | S_IWRITE | S_IEXEC | S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH)) {
#endif
    free(temp);
    free(chopsuey);
    return 1;
  }

  if(errno == EEXIST &&
#ifdef CIRCLE_WINDOWS
     !_stat(temp,&st)
#else
     !stat(temp,&st)
#endif
     && S_ISDIR(st.st_mode)) {
    free(temp);
    free(chopsuey);
    return 1;
  } else {
    free(temp);
    free(chopsuey);
    return 1;
  }
}


int default_admin_flags_mortal[] =
  { -1 };

int default_admin_flags_immortal[] =
  { ADM_SEEINV, ADM_SEESECRET, ADM_FULLWHERE, ADM_NOPOISON, ADM_WALKANYWHERE,
    ADM_NODAMAGE, ADM_NOSTEAL, -1 };

int default_admin_flags_builder[] =
  { -1 };

int default_admin_flags_god[] =
  { ADM_ALLSHOPS, ADM_TELLALL, ADM_KNOWWEATHER, ADM_MONEY, ADM_EATANYTHING,
    ADM_NOKEYS, -1 };

int default_admin_flags_grgod[] =
  { ADM_TRANSALL, ADM_FORCEMASS, ADM_ALLHOUSES, -1 };

int default_admin_flags_impl[] =
  { ADM_SWITCHMORTAL, ADM_INSTANTKILL, ADM_CEDIT, -1 };

int *default_admin_flags[ADMLVL_OWNER + 1] = {
  default_admin_flags_mortal,
  default_admin_flags_immortal,
  default_admin_flags_builder,
  default_admin_flags_god,
  default_admin_flags_grgod,
  default_admin_flags_impl
};

void admin_set(struct char_data *ch, int value)
{
  void run_autowiz(void);
  int i;
  int orig = GET_ADMLEVEL(ch);

  if (GET_ADMLEVEL(ch) == value)
    return;
  if (GET_ADMLEVEL(ch) < value) { /* Promotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true,
           "%s promoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) < value) {
      GET_ADMLEVEL(ch)++;
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        SET_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
    }
    run_autowiz();
    if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    }
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
      for (i = 0; i < 3; i++)
        GET_COND(ch, i) = (char) -1;
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }
    return;
  }
  if (GET_ADMLEVEL(ch) > value) { /* Demotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true,
           "%s demoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) > value) {
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        REMOVE_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
      GET_ADMLEVEL(ch)--;
    }
    run_autowiz();
    if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
    }
    return;
  }
}

int has_intro(struct char_data *ch, struct char_data *target) 
{
    
    int i;
    
    if (
        ((AFF_FLAGGED(target, AFF_WILD_SHAPE) && !is_player_grouped(ch, target)) ||
        !AFF_FLAGGED(target, AFF_WILD_SHAPE)) &&
        AFF_FLAGGED(target, AFF_DISGUISED) && skill_roll(ch, SKILL_PERCEPTION) < GET_DISGUISE_ROLL(target)) {
        DISGUISE_SEEN(target) = FALSE;
        return false;
    }
    else if (AFF_FLAGGED(target, AFF_DISGUISED))
        DISGUISE_SEEN(target) = TRUE;


    if (GET_ADMLEVEL(ch) > 0)
      return true;

    if (GET_ADMLEVEL(target) > 0)
      return true;
    
    if (GET_LEVEL(target) == 0)
      return true;

    if (ch == target)
      return true;
    
    if (IS_NPC(target) || IS_NPC(ch))
      return true;
    
    //if (!(&GET_SDESC(target)))
      //return true;

    if (AFF_FLAGGED(target, AFF_DISGUISED) && skill_roll(ch, SKILL_PERCEPTION) < skill_roll(target, SKILL_DISGUISE) &&
        !AFF_FLAGGED(ch, AFF_TRUE_SIGHT) && !is_player_grouped(ch, target))
        return false;

    if ((skill_roll(ch, SKILL_GATHER_INFORMATION) / 2) > MAX(1, 35 - GET_CLASS_LEVEL(target)))
      return true;
    
    for (i = 0; i < MAX_INTROS; i++) {
      if (ch->player_specials->intro_list[i][0] == GET_IDNUM(target))
        return true;
      else
        continue;
    }

  return false;   
}

int has_intro_idnum(struct char_data *ch, int idnum) 
{
    
    int i;
    

    if (GET_ADMLEVEL(ch) > 0)
      return true;

    if (GET_IDNUM(ch) == idnum)
      return true;
    
    if (IS_NPC(ch))
      return true;
    
    for (i = 0; i < MAX_INTROS; i++) {
      if (ch->player_specials->intro_list[i][0] == idnum)
        return true;
      else
        continue;
    }

  return false;   
}

char *which_desc(struct char_data *ch)
{
  char buf[100];
	
  if (!ch)
    return strdup("error");

  if (IS_NPC(ch))
    return strdup(ch->short_descr);

  if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
		char *tmpdesc = current_short_desc(ch);
		sprintf(buf, "the spirit of %s", tmpdesc);
		free(tmpdesc);
    return strdup(buf);
	}

  if (!IS_APPROVED(ch) || !GET_PC_SDESC(ch))
    return current_short_desc(ch);

  return strdup(GET_PC_SDESC(ch));
}

void convert_coins(struct char_data *ch)
{

  int adamantine = 0,
      mithril = 0,
      steel = 0,
      bronze = 0,
      copper = 0,
      coins = GET_GOLD(ch);

  adamantine = coins / 250000;
  coins %= 250000;

  mithril = coins / 10000;
  coins %= 10000;

  steel = coins / 100;
  coins %= 100;

  bronze = coins / 10;
  coins %= 10;

  copper = coins;
  
  return;
}

void modify_coins(struct char_data *ch, int type, int operand, int amount) 
{

  if (!operand)
    return;

  if (operand == OPERAND_ADD) {
    amount = amount;
  }

  if (operand == OPERAND_SUBTRACT) {
    amount = 0 - amount;
  }

  return;
}

int skill_roll(struct char_data *ch, int skillnum) {
	
	int roll;
	
	roll = dice(1, 20);

        if (PRF_FLAGGED(ch, PRF_TAKE_TEN))
          roll = 10;

	roll += get_skill_value(ch, skillnum);
	
	return roll;
}

int is_class_skill(struct char_data *ch, int skillnum) {
  int i = 0;
  if (GET_SKILL_BASE(ch, skillnum) > 0) {
    for (i = 0; i < NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i)) {
        if (spell_info[skillnum].can_learn_skill[i] == SKLEARN_CLASS) {
          return TRUE;
        }
      }
    }
  }
  return FALSE;

}

int get_skill_value(struct char_data *ch, int skillnum) 
{
	
  int value = 0;
  int armor_check_penalty = 0;
  int i = 0, j = 0;
	
  value = GET_SKILL(ch, skillnum);
  value += GET_SKILL_BONUS(ch, skillnum);
  value += get_skill_mod(ch, skillnum);

  if (affected_by_spell(ch, SPELL_SICKENED))
    value -= 2;

  for (i = 0; i < NUM_WEARS; i++) {
    for (j = 0; j < 6; j++) {
      if (GET_EQ(ch, i) && GET_EQ(ch, i)->affected[j].location == APPLY_SKILL &&
          GET_EQ(ch, i)->affected[j].specific == skillnum)
        value += GET_EQ(ch, i)->affected[j].modifier;
        break;
    }
    if (GET_EQ(ch, i) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR ||
        GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR_SUIT))
      if (!is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), 9)) &&
          GET_OBJ_VAL(GET_EQ(ch, i), 3) > armor_check_penalty)
      armor_check_penalty = GET_OBJ_VAL(GET_EQ(ch, i), 3);
    if (GET_EQ(ch, i)) {
      for (j = 0; j < 6; j++) {
        if (GET_EQ(ch, i)->affected[j].location == APPLY_SKILL &&
            GET_EQ(ch, i)->affected[j].specific == skillnum)
          value += GET_EQ(ch, i)->affected[j].modifier;
      }
    }
  }

  if (affected_by_spell(ch, SPELL_INSPIRE_COMPETENCE))
    value += 2;

  if (ch->player_specials->skill_focus[skillnum - SKILL_LOW_SKILL] > 0)
    value += 3;
  if (ch->player_specials->skill_focus[skillnum - SKILL_LOW_SKILL] > 1)
    value += 10;

  switch (skillnum) 
  {
    case SKILL_COMBAT_TACTICS:
      break;
    case SKILL_TANNING:
    case SKILL_TAILORING:
    case SKILL_WOODWORKING:
      value += HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER);
      break;
    case SKILL_MINING:
    case SKILL_HUNTING:
    case SKILL_FORESTING:
    case SKILL_FARMING:
      value += HAS_FEAT(ch, FEAT_PROFICIENT_HARVESTER);
      break;
    case SKILL_PERCEPTION:
      if (IS_GNOME(ch) || IS_HALFLING(ch))
        value += 2;
      break;
    case SKILL_ACROBATICS:
      if (IS_HALFLING(ch))
        value += 2;
    case SKILL_STEALTH:
      value += (SIZE_MEDIUM - get_size(ch)) * 4;
      break;
    case SKILL_LORE:
      if (HAS_FEAT(ch, FEAT_NATURE_SENSE))
        value += 2;
      value += GET_CLASS_RANKS(ch, CLASS_BARD);
      break;
    case SKILL_SENSE_MOTIVE:
      if (HAS_FEAT(ch, FEAT_HONORBOUND))
        value += 2;
      break;
    case SKILL_GATHER_INFORMATION:
      value += GET_CLASS_RANKS(ch, CLASS_BARD);
      break;
    case SKILL_BLACKSMITHING:
      if (IS_DWARF(ch))
        value += 2;
      value += HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER);
      break;
    case SKILL_GOLDSMITHING:
      if (IS_DWARF(ch))
        value += 2;
      value += HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER);
      break;
  }

  if (IS_OVER_LOAD(ch))
    armor_check_penalty += 10;
  else if (IS_HEAVY_LOAD(ch))
    armor_check_penalty += 6;
  if (IS_MEDIUM_LOAD(ch))
    armor_check_penalty += 3;

  if (IS_SET(spell_info[skillnum].flags, SKFLAG_DEXMOD) ||
      IS_SET(spell_info[skillnum].flags, SKFLAG_STRMOD))
    value -= armor_check_penalty;
	
  if (HAS_FEAT(ch, FEAT_ABLE_LEARNER))
    value += 1;

  if (affected_by_spell(ch, SPELL_PRAYER))
    value += 1;

  if (affected_by_spell(ch, SPELL_BESTOW_CURSE_PENALTIES))
    value -= 4;

  if (ch->mentor_level > 0)
    value = value * ch->mentor_level / GET_CLASS_LEVEL(ch);

  return value;
}

int strlencolor(char *arg) 
{
	
	int length = 0;
	int num_cc = 0;
	int i = 0;
	
	length = strlen(arg);
	
	for (i = 0; i < length; i++) {
		if (arg[i] == '@') {
			if (arg[i+1] == '@') {
				i++;
				num_cc++;
			}
			else
				num_cc++;
		}
		continue;
	}
	
	return (length - (num_cc * 2));
}

int has_favored_enemy(struct char_data *ch, struct char_data *victim)
{

  if (!ch || !victim)
    return FALSE;

  if (IS_NPC(ch))
    return FALSE;

  if ( victim->race >= NUM_RACES )
  {
    log("SYSERR:  Illegal race for mob %d in room %d (has_favored_enemy)", GET_MOB_VNUM(victim), world[ch->in_room].number);
    return FALSE;
  }

  if (HAS_COMBAT_FEAT(ch, CFEAT_FAVORED_ENEMY, race_list[GET_RACE(victim)].family))
    return TRUE;

  return FALSE;

}

char * rewind_string(int num, char *string)
{

  int i = 0;

  for (i = 0; i < strlen(string); i++) {
    string[i] = string[i + num];
  }

  return string;

}

char * a_or_an(char *string) {
  switch(tolower(*string)) {
		case 'a':
		case 'e':
		case 'i':
		case 'u':
		case 'o':
    return "an";
	}

  return "a";
}

int find_armor_type(int specType)
{

  switch (specType) {
  
  case SPEC_ARMOR_TYPE_PADDED:
  case SPEC_ARMOR_TYPE_LEATHER:
  case SPEC_ARMOR_TYPE_STUDDED_LEATHER:
  case SPEC_ARMOR_TYPE_LIGHT_CHAIN:
    return ARMOR_TYPE_LIGHT;

  case SPEC_ARMOR_TYPE_HIDE:
  case SPEC_ARMOR_TYPE_SCALE:
  case SPEC_ARMOR_TYPE_CHAINMAIL:
  case SPEC_ARMOR_TYPE_PIECEMEAL:
    return ARMOR_TYPE_MEDIUM;

  case SPEC_ARMOR_TYPE_SPLINT:
  case SPEC_ARMOR_TYPE_BANDED:
  case SPEC_ARMOR_TYPE_HALF_PLATE:
  case SPEC_ARMOR_TYPE_FULL_PLATE:
    return ARMOR_TYPE_HEAVY;

  case SPEC_ARMOR_TYPE_BUCKLER:
  case SPEC_ARMOR_TYPE_SMALL_SHIELD:
  case SPEC_ARMOR_TYPE_LARGE_SHIELD:
  case SPEC_ARMOR_TYPE_TOWER_SHIELD:
    return ARMOR_TYPE_SHIELD;
  }

  return ARMOR_TYPE_LIGHT;
}


/*
   strfrmt (String Format) function
   Used by automap/map system
   Re-formats a string to fit within a particular size box.
   Recognises @ color codes, and if a line ends in one color, the
   next line will start with the same color.
   Ends every line with @n to prevent color bleeds.
*/
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad)
{
  static char ret[MAX_STRING_LENGTH];
  char line[MAX_INPUT_LENGTH];
  char *sp = str;
  char *lp = line;
  char *rp = ret;
  char *wp;
  int wlen = 0, llen = 0, lcount = 0;
  char last_color='n';
  bool new_line_started = FALSE;

  memset(line, '\0', MAX_INPUT_LENGTH);
  /* Nomalize spaces and newlines */
  /* Split into lines, including convert \\ into \r\n */
  while(*sp) {
    /* eat leading space */
    while(*sp && isspace_ignoretabs(*sp)) sp++;
    /* word begins */
    wp = sp;
    wlen = 0;
    while(*sp) { /* Find the end of the word */
      if(isspace_ignoretabs(*sp)) break;
      if(*sp=='\\' && sp[1] && sp[1]=='\\') {
        if(sp!=wp)
          break; /* Finish dealing with the current word */
        sp += 2; /* Eat the marker and any trailing space */
        while(*sp && isspace_ignoretabs(*sp)) sp++;
        wp = sp;
        /* Start a new line */
        if(hpad)
          for(; llen < w; llen++)
            *lp++ = ' ';
        *lp++ = '\r';
        *lp++ = '\n';
        *lp++ = '\0';
        rp += sprintf(rp, "%s", line);
        llen = 0;
        lcount++;
        lp = line;
      } else if (*sp=='`'||*sp=='$'||*sp=='#'||*sp=='@') {
        if (sp[1] && (sp[1]==*sp))
          wlen++; /* One printable char here */
        if (*sp=='@' && (sp[1]!=*sp)) /* Color code, not @@ */
          last_color = sp[1];
        sp += 2; /* Eat the whole code regardless */
       } else if (*sp=='\t'&&sp[1]) {
          char MXPcode = sp[1]=='[' ? ']' : sp[1]=='<' ? '>' : '\0';
          sp += 2; /* Eat the code */
          if (MXPcode)
          {
          while (*sp!='\0'&&*sp!=MXPcode)
          ++sp; /* Eat the rest of the code */
          }
      } else {
        wlen++;
        sp++;
      }
    }
    if(llen + wlen + (lp==line ? 0 : 1) > w) {
      /* Start a new line */
      if(hpad)
        for(; llen < w; llen++)
          *lp++ = ' ';
      *lp++ = '@';  /* 'normal' color */
      *lp++ = 'n';
      *lp++ = '\r'; /* New line */
      *lp++ = '\n';
      *lp++ = '\0';
      sprintf(rp, "%s", line);
      rp += strlen(line);
      llen = 0;
      lcount++;
      lp = line;
      if (last_color != 'n') {
        *lp++ = '@';  /* restore previous color */
        *lp++ = last_color;
        new_line_started = TRUE;
      }
    }
    /* add word to line */
    if (lp!=line && new_line_started!=TRUE) {
      *lp++ = ' ';
      llen++;
    }
    new_line_started = FALSE;
    llen += wlen ;
    for( ; wp!=sp ; *lp++ = *wp++);
  }
  /* Copy over the last line */
  if(lp!=line) {
    if(hpad)
      for(; llen < w; llen++)
        *lp++ = ' ';
    *lp++ = '\r';
    *lp++ = '\n';
    *lp++ = '\0';
    sprintf(rp, "%s", line);
    rp += strlen(line);
    lcount++;
  }
  if(vpad) {
    while(lcount < h) {
      if(hpad) {
        memset(rp, ' ', w);
        rp += w;
      }
      *rp++ = '\r';
      *rp++ = '\n';
      lcount++;
    }
    *rp = '\0';
  }
  return ret;
}

/*
   strpaste (String Paste) function
   Takes two long strings (multiple lines) and joins them side-by-side.
   Used by the automap/map system
*/
char *strpaste(char *str1, char *str2, char *joiner)
{
  static char ret[MAX_STRING_LENGTH+1];
  char *sp1 = str1;
  char *sp2 = str2;
  char *rp = ret;
  int jlen = strlen(joiner);

  while((rp - ret) < MAX_STRING_LENGTH && (*sp1 || *sp2)) {
     /* Copy line from str1 */
    while((rp - ret) < MAX_STRING_LENGTH && *sp1 && !ISNEWL(*sp1))
      *rp++ = *sp1++;
    /* Eat the newline */
    if(*sp1) {
      if(sp1[1] && sp1[1]!=sp1[0] && ISNEWL(sp1[1]))
        sp1++;
      sp1++;
    }

    /* Add the joiner */
    if((rp - ret) + jlen >= MAX_STRING_LENGTH)
      break;
    strcpy(rp, joiner);
    rp += jlen;

     /* Copy line from str2 */
    while((rp - ret) < MAX_STRING_LENGTH && *sp2 && !ISNEWL(*sp2))
      *rp++ = *sp2++;
    /* Eat the newline */
    if(*sp2) {
      if(sp2[1] && sp2[1]!=sp2[0] && ISNEWL(sp2[1]))
        sp2++;
      sp2++;
    }

    /* Add the newline */
    if((rp - ret) + 2 >= MAX_STRING_LENGTH)
      break;
    *rp++ = '\r';
    *rp++ = '\n';
  }
  /* Close off the string */
  *rp = '\0';
  return ret;
}

int has_mob_follower(struct char_data *ch, int vnum)
{

  struct follow_type *k;

 
  for (k = ch->followers; k->next->follower != ch; k = k->next)
    if (IS_NPC(k->follower) && GET_MOB_VNUM(k->follower) == vnum)
      return TRUE;

  return FALSE;

}

int valid_craft_material(int mat_used, int craft_type, int mat_craft)
{

  switch (craft_type) {
    case SCMD_FORGE:
      if (mat_craft == MATERIAL_STEEL && matching_craft_materials(mat_used, mat_craft))
        return TRUE;
    case SCMD_TAN:
      if (mat_craft == MATERIAL_LEATHER && matching_craft_materials(mat_used, mat_craft))
        return TRUE;

    case SCMD_CRAFT:
      if ((mat_craft == MATERIAL_GOLD || mat_craft == MATERIAL_DIAMOND) && matching_craft_materials(mat_used, mat_craft))
        return TRUE;

    case SCMD_ASSEMBLE:
    case SCMD_FLETCH:
      if (mat_craft == MATERIAL_WOOD && matching_craft_materials(mat_used, mat_craft))
        return TRUE;

    case SCMD_THATCH:
    case SCMD_WEAVE:
    case SCMD_KNIT:
      if (mat_craft == MATERIAL_COTTON && matching_craft_materials(mat_used, mat_craft))
        return TRUE;

  }
  return FALSE;
}

int matching_craft_materials(int mat_used, int mat_craft) 
{

  switch (mat_craft) {
    case MATERIAL_STEEL:
      switch (mat_used) {
        case MATERIAL_STEEL:
        case MATERIAL_ADAMANTINE:
        case MATERIAL_MITHRIL:
        case MATERIAL_IRON:
        case MATERIAL_COPPER:
        case MATERIAL_BRONZE:
        case MATERIAL_BRASS:
        case MATERIAL_ALCHEMAL_SILVER:
        case MATERIAL_COLD_IRON:
        case MATERIAL_DRAGONHIDE:
        case MATERIAL_BONE:
          return TRUE;
      }
      break;
    case MATERIAL_GOLD:
      switch (mat_used) {
        case MATERIAL_GOLD:
        case MATERIAL_GLASS:
        case MATERIAL_STEEL:
        case MATERIAL_BONE:
        case MATERIAL_CRYSTAL:
        case MATERIAL_ADAMANTINE:
        case MATERIAL_MITHRIL:
        case MATERIAL_IRON:
        case MATERIAL_COPPER:
        case MATERIAL_PLATINUM:
        case MATERIAL_IVORY:
        case MATERIAL_BRASS:
        case MATERIAL_BRONZE:
        case MATERIAL_SILVER:
        case MATERIAL_ALCHEMAL_SILVER:
        case MATERIAL_COLD_IRON:
          return TRUE;
      }
      break;
    case MATERIAL_WOOD:
      switch (mat_used) {
        case MATERIAL_WOOD:
        case MATERIAL_DARKWOOD:
          return TRUE;
      }
      break;
    case MATERIAL_LEATHER:
      switch (mat_used) {
        case MATERIAL_LEATHER:
        case MATERIAL_DARKWOOD:
          return TRUE;
      }
      break;
    case MATERIAL_COTTON:
      switch (mat_used) {
        case MATERIAL_WOOD:
        case MATERIAL_DARKWOOD:
          return TRUE;
      }
      break;
    case MATERIAL_DIAMOND:
      switch (mat_used) {
        case MATERIAL_WOOD:
        case MATERIAL_DARKWOOD:
          return TRUE;
      }
      break;
  }
  return FALSE;
}

// Harvesting Node Placement Functions & global vars

int mining_nodes = 0;
int farming_nodes = 0;
int hunting_nodes = 0;
int foresting_nodes = 0;
int orphans = 0;
int lockboxes = 0;

char * node_keywords(int material)
{

  switch (material) {
    case MATERIAL_STEEL:
      return strdup("vein iron ore");
    case MATERIAL_COLD_IRON:
      return strdup("vein cold iron ore");
    case MATERIAL_MITHRIL:
      return strdup("vein mithril ore");
    case MATERIAL_ADAMANTINE:
      return strdup("vein adamantine ore");
    case MATERIAL_SILVER:
      return strdup("vein copper silver ore");
    case MATERIAL_GOLD:
      return strdup("vein gold platinum ore");
    case MATERIAL_WOOD:
      return strdup("tree fallen");
    case MATERIAL_DARKWOOD:
      return strdup("tree darkwood fallen");
    case MATERIAL_LEATHER:
      return strdup("game freshly killed");
    case MATERIAL_DRAGONHIDE:
      return strdup("wyvern freshly killed");
    case MATERIAL_HEMP:
      return strdup("hemp plants");
    case MATERIAL_COTTON:
      return strdup("cotton plants");
    case MATERIAL_VELVET:
      return strdup("cache of cloth");
    case MATERIAL_SILK: 
      return strdup("silkworms");
  }
  return strdup("node harvesting");
}

char * node_sdesc(int material)
{
  switch (material) {
    case MATERIAL_STEEL:
      return strdup("a vein of iron ore");
    case MATERIAL_COLD_IRON:
      return strdup("a vein of cold iron ore");
    case MATERIAL_MITHRIL:
      return strdup("a vein of mithril ore");
    case MATERIAL_ADAMANTINE:
      return strdup("a vein of adamantine ore");
    case MATERIAL_SILVER:
      return strdup("a vein of copper and silver ore");
    case MATERIAL_GOLD:
      return strdup("a vein of gold and platinum ore");
    case MATERIAL_WOOD:
      return strdup("a fallen tree");
    case MATERIAL_DARKWOOD:
      return strdup("a fallen darkwood tree");
    case MATERIAL_LEATHER:
      return strdup("the corpse of some freshly killed game");
    case MATERIAL_DRAGONHIDE:
      return strdup("the corpse of a freshly killed baby wyvern");
    case MATERIAL_HEMP:
      return strdup("a patch of hemp plants");
    case MATERIAL_COTTON:
      return strdup("a patch of cotton plants");
    case MATERIAL_VELVET:
      return strdup("an abandoned cache of cloths");
    case MATERIAL_SILK:
      return strdup("a large family of silkworms");
  }
  return strdup("a harvesting node");
}

char * node_desc(int material)
{
  switch (material) {
    case MATERIAL_STEEL:
      return strdup("A vein of iron ore is here.");
    case MATERIAL_COLD_IRON:
      return strdup("A vein of cold iron ore is here.");
    case MATERIAL_MITHRIL:
      return strdup("A vein of mithril ore is here.");
    case MATERIAL_ADAMANTINE:
      return strdup("A vein of adamantine ore is here.");
    case MATERIAL_SILVER:
      return strdup("A vein of copper and silver ore is here.");
    case MATERIAL_GOLD:
      return strdup("A vein of gold and platinum ore is here.");
    case MATERIAL_WOOD:
      return strdup("A fallen tree is here.");
    case MATERIAL_DARKWOOD:
      return strdup("A fallen darkwood tree is here.");
    case MATERIAL_LEATHER:
      return strdup("The corpse of some freshly killed game is here.");
    case MATERIAL_DRAGONHIDE:
      return strdup("The corpse of a freshly killed baby wyvern is here.");
    case MATERIAL_HEMP:
      return strdup("A patch of hemp plants is here.");
    case MATERIAL_COTTON:
      return strdup("A patch of cotton plants is here.");
    case MATERIAL_VELVET:
      return strdup("An abandoned cache of cloths is here.");
    case MATERIAL_SILK:
      return strdup("A large family of silkworms is here.");
  }
  return strdup("A harvesting node is here.  Please inform an imm, this is an error.");
}

int random_node_material(int allowed) {

  if (mining_nodes >= (allowed * 2) && foresting_nodes >= allowed &&
      farming_nodes >= allowed && hunting_nodes >= allowed)
    return MATERIAL_STEEL;

  int rand = 0;

  rand = dice(1, 100);

  if (rand <= 34) {

    // mining

    if (mining_nodes >= (allowed * 2))
      return random_node_material(allowed);

    rand = dice(1, 100);

    if (rand <= 80) {

      // blacksmithing

      rand = dice(1, 1000);

      if (rand <= 900)
        return MATERIAL_STEEL;
      else if (rand <= 980)
        return MATERIAL_COLD_IRON;
      else if (rand <= 999)
        return MATERIAL_MITHRIL;
      else
        return MATERIAL_ADAMANTINE;

    }
    else {

      // goldsmithing

      rand = dice(1, 100);

      if (rand <= 90)
        return MATERIAL_SILVER;
      else
        return MATERIAL_GOLD;

    }

  }
  else if (rand <= 67) {
    rand = dice(1, 100);

    // farming

    if (farming_nodes >= allowed)
      return random_node_material(allowed);

    rand = dice(1, 100);

    if (rand <= 30)
      return MATERIAL_HEMP;
    else if (rand <= 90)
      return MATERIAL_COTTON;
    else if (rand <= 99)
      return MATERIAL_VELVET;
    else
      return MATERIAL_SILK;

  }
  else {
    // foresting

    if (foresting_nodes >= allowed)
      return random_node_material(allowed);

    rand = dice(1, 100);
    
    if (rand <= 50) {

    rand = dice(1, 100);
    if (rand <= 99)
      return MATERIAL_LEATHER;
    else
      return MATERIAL_DRAGONHIDE;

    }
    else {

    rand = dice(1, 100);

    if (rand <= 99)
      return MATERIAL_WOOD;
    else
      return MATERIAL_DARKWOOD;

    }
  }

  return MATERIAL_STEEL;
}

void reset_harvesting_rooms(void)
{

  int i = 0;

  for (i = 0; i < 1000000; i++)
    times_harvested[i] = 0;

  int cnt = 0;
  int num_rooms = 0;
  int nodes_allowed = 0;
  struct obj_data *obj = NULL;
  int orphans_allowed = 0;
  int lockboxes_allowed = 0;


  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (zone_table[world[cnt].zone].zone_status < 2 || world[cnt].sector_type == SECT_CITY)
      continue;
    num_rooms++;
  }

  nodes_allowed = num_rooms / 33;
  orphans_allowed = lockboxes_allowed = num_rooms / 80;

  if (mining_nodes >= (nodes_allowed * 2) && foresting_nodes >= nodes_allowed &&
      farming_nodes >= nodes_allowed && hunting_nodes >= nodes_allowed)
    return;

  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (ROOM_FLAGGED(cnt, ROOM_HOUSE))
      continue;

    if (zone_table[world[cnt].zone].zone_status < 2)
      continue;

    if (dice(1, 80) == 1) {
      obj = read_object(64098, VIRTUAL);
      if (!obj)
        continue;
      if (orphans >= orphans_allowed)
        continue;
      orphans++;
      obj_to_room(obj, cnt);
    }
  }

  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (ROOM_FLAGGED(cnt, ROOM_HOUSE))
      continue;

    if (zone_table[world[cnt].zone].zone_status < 2)
      continue;

    if (dice(1, 80) == 1) {
      obj = read_object(64097, VIRTUAL);
      if (!obj)
        continue;
      if (lockboxes >= lockboxes_allowed)
        continue;
      lockboxes++;
      obj_to_room(obj, cnt);
    }
  }

  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (ROOM_FLAGGED(cnt, ROOM_HOUSE))
      continue;
    if (zone_table[world[cnt].zone].zone_status < 2 || world[cnt].sector_type == SECT_CITY)
      continue;
    if (dice(1, 33) == 1) {
      obj = read_object(64099, VIRTUAL);
      if (!obj)
        continue;
      GET_OBJ_MATERIAL(obj) = random_node_material(nodes_allowed);
      switch (GET_OBJ_MATERIAL(obj)) {
        case MATERIAL_STEEL:
        case MATERIAL_COLD_IRON:
        case MATERIAL_MITHRIL:
        case MATERIAL_ADAMANTINE:
        case MATERIAL_SILVER:
        case MATERIAL_GOLD:
          if (mining_nodes >= nodes_allowed) 
            continue;
          else
            mining_nodes++;
          break;
        case MATERIAL_WOOD:
        case MATERIAL_DARKWOOD:
        case MATERIAL_LEATHER:
        case MATERIAL_DRAGONHIDE:
          if (foresting_nodes >= nodes_allowed) 
            continue;
          else
            foresting_nodes++;
        case MATERIAL_HEMP:
        case MATERIAL_COTTON:
        case MATERIAL_WOOL:
        case MATERIAL_VELVET:
        case MATERIAL_SATIN:
        case MATERIAL_SILK:
          if (farming_nodes >= nodes_allowed) 
            continue;
          else
            farming_nodes++;
          break;
        default:
          continue;
          break;
      }
      GET_OBJ_VAL(obj, 0) = dice(2, 3);

      /* strdup()ed in node_foo() functions */
      obj->name = node_keywords(GET_OBJ_MATERIAL(obj));
      obj->short_description = node_sdesc(GET_OBJ_MATERIAL(obj));
      obj->description = node_desc(GET_OBJ_MATERIAL(obj));
      obj_to_room(obj, cnt);
    }
  }
}

int has_daylight(struct char_data *ch)
{

  struct char_data *vict;

  if (ch == NULL)
    return FALSE;

  if (affected_by_spell(ch, SPELL_DAYLIGHT))
    return TRUE;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (affected_by_spell(vict, SPELL_DAYLIGHT))
      return TRUE;

  if (weather_info.sky == SKY_CLOUDY || weather_info.sky == SKY_RAINING || weather_info.sky == SKY_LIGHTNING)
    return FALSE;

  if (OUTSIDE(ch) && (weather_info.sunlight ==  SUN_RISE || weather_info.sunlight == SUN_LIGHT))
    return TRUE;

  return FALSE;
}

int has_light(struct char_data *ch)
{
  int i = 0;
  struct char_data *vict;

  if (ch == NULL)
    return FALSE;

  if (OUTSIDE(ch) && (weather_info.sunlight ==  SUN_RISE || weather_info.sunlight == SUN_LIGHT))
    return TRUE;

  if (affected_by_spell(ch, SPELL_DAYLIGHT))
    return TRUE;

  if (affected_by_spell(ch, SPELL_LIGHT))
    return TRUE;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
    if (affected_by_spell(vict, SPELL_DAYLIGHT))
      return TRUE;
    if (affected_by_spell(vict, SPELL_LIGHT))
      return TRUE;
  }

  if ((GET_EQ(ch, WEAR_WIELD) || (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON)) &&
      GET_CLASS_RANKS(ch, CLASS_PALADIN) >= 20)
    return TRUE;

    /* Is the character using a working light source? */
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i))
        if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
          if (GET_OBJ_VAL(GET_EQ(ch, i),VAL_LIGHT_HOURS))
            return TRUE;
    }


  return FALSE;
}

int get_saving_throw_value(struct char_data *victim, int savetype)
{

    int total = 0;

    if (savetype == SAVING_FORTITUDE) 
    {
        total += ability_mod_value(GET_CON(victim));
        if (HAS_FEAT(victim, FEAT_GREAT_FORTITUDE))
            total += 2;
    }
    else if (savetype == SAVING_REFLEX) 
    {
        total += ability_mod_value(GET_DEX(victim));
        if (HAS_FEAT(victim, FEAT_LIGHTNING_REFLEXES))
            total += 2;
    }
    else if (savetype == SAVING_WILL) 
    {
        if (HAS_FEAT(victim, FEAT_STEADFAST_DETERMINATION))
            total += MAX(ability_mod_value(GET_WIS(victim)), ability_mod_value(GET_CON(victim)));
        else
            total += ability_mod_value(GET_WIS(victim));
        if (HAS_FEAT(victim, FEAT_IRON_WILL))
            total += 2;
    }

    if (IS_HALFLING(victim))
        total += 1;

    if (has_daylight(victim) && GET_RACE(victim) == RACE_GRAY_DWARF)
        total -= 2;

    if (has_daylight(victim) && GET_RACE(victim) == RACE_DROW_ELF)
        total -= 1;

    if (HAS_FEAT(victim, FEAT_DIVINE_GRACE) || HAS_FEAT(victim, FEAT_DARK_BLESSING))
        total += ability_mod_value(GET_CHA(victim));

    total += GET_SAVE(victim, savetype);

    if (affected_by_spell(victim, SPELL_PRAYER))
        total += 1;

    if (affected_by_spell(victim, SPELL_BESTOW_CURSE_PENALTIES))
        total -= 4;

    return total;

}

int stat_assign_stat(int abil, char *arg, struct char_data *ch)
{
  int temp;
        int cost = 0;
        int i;
        int orig = abil;

        if (abil < 15)
         cost = abil;
        else {
          cost = 15;
                for (i = 0; i < abil - 15; i++)
                  cost += i + 2;
        }

  if (abil > 0) {
      GET_STAT_POINTS(ch) = GET_STAT_POINTS(ch) + cost;
      abil = 0;
  }
  temp = atoi(arg);

  temp = LIMIT(temp, 8, 18);

        if (temp < 15)
         cost = temp;
        else {
          cost = 15;
                for (i = 0; i < temp - 15; i++)
                  cost += i + 2;
        }

  if (cost > GET_STAT_POINTS(ch)) {
          write_to_output(ch->desc, "You don't have enough points to purchase that ability score rank.\r\n") ;
                if (orig < 15)
                 cost = orig;
                else {
                  cost = 15;
                        for (i = 0; i < orig - 15; i++)
                          cost += i + 2;
                }
                GET_STAT_POINTS(ch) -= cost;
          return orig;
  }

  /* This should throw an error! */
  if (GET_STAT_POINTS(ch) <= 0) {
    temp = 0;
    GET_STAT_POINTS(ch) = 0;
    mudlog(NRM, ADMLVL_IMMORT, TRUE, "Stat total below 0: possible code error");
  }
  abil = temp;

  GET_STAT_POINTS(ch) -= cost;

  return abil;
}

int is_metal_item(struct obj_data *obj)
{

  switch (GET_OBJ_MATERIAL(obj)) {
    case MATERIAL_GOLD:
    case MATERIAL_STEEL:
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_IRON:
    case MATERIAL_COPPER:
    case MATERIAL_PLATINUM:
    case MATERIAL_BRASS:
    case MATERIAL_BRONZE:
    case MATERIAL_SILVER:
    case MATERIAL_ALCHEMAL_SILVER:
    case MATERIAL_COLD_IRON:
      return TRUE;
  }

  return FALSE;

}

char *replace_string(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

int num_charmies(struct char_data *ch)
{
  struct follow_type *f;
  int num = 0, i = 0, count = 0;
  int sum_found = FALSE;

  for (f = ch->followers; f; f = f->next) {
    if (!IS_NPC(f->follower))
      continue;

    if (GET_MOB_VNUM(f->follower) == GET_COMPANION_VNUM(ch) ||
        GET_MOB_VNUM(f->follower) == GET_FAMILIAR_VNUM(ch) ||
        GET_MOB_VNUM(f->follower) == GET_PET_VNUM(ch) ||
        GET_MOB_VNUM(f->follower) == GET_MOUNT_VNUM(ch) ||
        GET_MOB_VNUM(f->follower) == 199) // Paladin Mount
      continue;

    for (i = 8; i >= 0; i--) { 
      for (count = 0; monsum_list[i][ALIGN_TYPE(ch)][count] != NOBODY; count++) {
        if (GET_MOB_VNUM(f->follower) == monsum_list[i][ALIGN_TYPE(ch)][count]) {
          if (sum_found)
            num++;
          else
            sum_found = TRUE;
        }
      }
    }        

    if (AFF_FLAGGED(f->follower, AFF_CHARM))
      num++;
  }

  return num;

}

int power_essence_bonus(struct obj_data *essence)
{

  if (!essence)
    return 0;

  if (GET_OBJ_VNUM(essence) == 64100 || GET_OBJ_VNUM(essence) == 64105 || GET_OBJ_VNUM(essence) == 64110 ||
      GET_OBJ_VNUM(essence) == 64115 || GET_OBJ_VNUM(essence) == 64120 || GET_OBJ_VNUM(essence) == 64125)
    return 1;
  if (GET_OBJ_VNUM(essence) == 64101 || GET_OBJ_VNUM(essence) == 64106 || GET_OBJ_VNUM(essence) == 64111 ||
      GET_OBJ_VNUM(essence) == 64116 || GET_OBJ_VNUM(essence) == 64121 || GET_OBJ_VNUM(essence) == 64126)
    return 2;
  if (GET_OBJ_VNUM(essence) == 64102 || GET_OBJ_VNUM(essence) == 64107 || GET_OBJ_VNUM(essence) == 64112 ||
      GET_OBJ_VNUM(essence) == 64117 || GET_OBJ_VNUM(essence) == 64122 || GET_OBJ_VNUM(essence) == 64127)
    return 3;
  if (GET_OBJ_VNUM(essence) == 64103 || GET_OBJ_VNUM(essence) == 64108 || GET_OBJ_VNUM(essence) == 64113 ||
      GET_OBJ_VNUM(essence) == 64118 || GET_OBJ_VNUM(essence) == 64123 || GET_OBJ_VNUM(essence) == 64128)
    return 4;
  if (GET_OBJ_VNUM(essence) == 64104 || GET_OBJ_VNUM(essence) == 64109 || GET_OBJ_VNUM(essence) == 64114 ||
      GET_OBJ_VNUM(essence) == 64119 || GET_OBJ_VNUM(essence) == 64124 || GET_OBJ_VNUM(essence) == 64129)
    return 5;
  else 
    return 0;

  return 0;

}

int determine_misc_item_bonus_type(struct obj_data *obj, struct obj_data *e_one, struct obj_data *e_two, int bonus)
{

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FINGER)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two))) {
      switch (bonus) {
        case 1:
          return APPLY_SPELL_LVL_0;
        case 2:
          return APPLY_SPELL_LVL_1;
        case 3:
          return APPLY_SPELL_LVL_2;
        case 4:
          return APPLY_SPELL_LVL_3;
        case 5:
          return APPLY_SPELL_LVL_4;
        case 6:
          return APPLY_SPELL_LVL_5;
        case 7:
          return APPLY_SPELL_LVL_6;
        case 8:
          return APPLY_SPELL_LVL_7;
        case 9:
          return APPLY_SPELL_LVL_8;
        default:
          return APPLY_SPELL_LVL_9;
      }
    }
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two))) {
      switch (bonus) {
        case 1:
          return APPLY_SPELL_LVL_0;
        case 2:
          return APPLY_SPELL_LVL_1;
        case 3:
          return APPLY_SPELL_LVL_2;
        case 4:
          return APPLY_SPELL_LVL_3;
        case 5:
          return APPLY_SPELL_LVL_4;
        case 6:
          return APPLY_SPELL_LVL_5;
        case 7:
          return APPLY_SPELL_LVL_6;
        case 8:
          return APPLY_SPELL_LVL_7;
        case 9:
          return APPLY_SPELL_LVL_8;
        default:
          return APPLY_SPELL_LVL_9;
      }
    }
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_WILL;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_REFLEX;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_FORTITUDE;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_AC_DEFLECTION;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WRIST)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_TURN_LEVEL;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_AC_DEFLECTION;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_AC_DODGE;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_HIT;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_AC_NATURAL;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_MOVE;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HEAD)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_WIS;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_INT;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_CHA;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_CON;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_DEX;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_STR;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WAIST)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_FORTITUDE;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_STR;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_CHA;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_CON;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_DEX;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_MOVE;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_NECK)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_ALLSAVES;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_WIS;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_AC_DODGE;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_AC_DEFLECTION;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_AC_NATURAL;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_HIT;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FEET)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_REFLEX;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_CON;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_AC_DODGE;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_CHA;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_DEX;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_STR;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HANDS)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_AC_ARMOR;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_DEX;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_STR;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_HIT;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_AC_NATURAL;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_MOVE;
  }

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ABOUT)) {
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_FIRE_ESSENCE(e_two)))
          return APPLY_STR;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_AIR_ESSENCE(e_two)))
          return APPLY_DEX;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_EARTH_ESSENCE(e_two)))
          return APPLY_CON;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_SPIRIT_ESSENCE(e_two)))
          return APPLY_INT;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_WIS;
    if ((e_one && IS_POWER_ESSENCE(e_one)) && (e_two && IS_WATER_ESSENCE(e_two)))
          return APPLY_CHA;
    if (e_one && e_two)
      return APPLY_NONE;
    if ((e_one && IS_POWER_ESSENCE(e_one)) || (e_two && IS_POWER_ESSENCE(e_two)))
      return APPLY_AC_DEFLECTION;
    if ((e_one && IS_SPIRIT_ESSENCE(e_one)) || (e_two && IS_SPIRIT_ESSENCE(e_two)))
      return APPLY_CHA;
    if ((e_one && IS_WATER_ESSENCE(e_one)) || (e_two && IS_WATER_ESSENCE(e_two)))
      return APPLY_AC_DODGE;
    if ((e_one && IS_AIR_ESSENCE(e_one)) || (e_two && IS_AIR_ESSENCE(e_two)))
      return APPLY_WIS;
    if ((e_one && IS_EARTH_ESSENCE(e_one)) || (e_two && IS_EARTH_ESSENCE(e_two)))
      return APPLY_INT;
    if ((e_one && IS_FIRE_ESSENCE(e_one)) || (e_two && IS_FIRE_ESSENCE(e_two)))
      return APPLY_ALLSAVES;
  }

  return APPLY_NONE;
}

int valid_misc_item_material_type(struct obj_data *obj, int mat)
{

  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ABOUT) && (IS_CLOTH(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FINGER) && (IS_PRECIOUS_METAL(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WRIST) && (IS_PRECIOUS_METAL(mat) || IS_LEATHER(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HEAD) && (IS_CLOTH(mat) || IS_LEATHER(mat) || IS_HARD_METAL(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WAIST) && (IS_LEATHER(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FEET) && (IS_LEATHER(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HANDS) && (IS_CLOTH(mat) || IS_LEATHER(mat)))
    return TRUE;
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_NECK) && (IS_PRECIOUS_METAL(mat)))
    return TRUE;

  return FALSE;
}

int random_essence_vnum(int bonus) {

  int roll = 0;

  switch (bonus) {
    case 1:
      roll = dice(1, 6);
      switch (roll) {
        case 1:
          return 64100;
        case 2:
          return 64105;
        case 3:
          return 64110;
        case 4:
          return 64115;
        case 5:
          return 64120;
        case 6:
          return 64125;
      }
      break;
    case 2:
      roll = dice(1, 6);
      switch (roll) {
        case 1:
          return 64101;
        case 2:
          return 64106;
        case 3:
          return 64111;
        case 4:
          return 64116;
        case 5:
          return 64121;
        case 6:
          return 64126;
      }
      break;
    case 3:
      roll = dice(1, 6);
      switch (roll) {
        case 1:
          return 64102;
        case 2:
          return 64107;
        case 3:
          return 64112;
        case 4:
          return 64117;
        case 5:
          return 64122;
        case 6:
          return 64127;
      }
      break;
    case 4:
      roll = dice(1, 6);
      switch (roll) {
        case 1:
          return 64103;
        case 2:
          return 64108;
        case 3:
          return 64113;
        case 4:
          return 64118;
        case 5:
          return 64123;
        case 6:
          return 64128;
      }
      break;
    case 5:
      roll = dice(1, 6);
      switch (roll) {
        case 1:
          return 64104;
        case 2:
          return 64109;
        case 3:
          return 64114;
        case 4:
          return 64119;
        case 5:
          return 64124;
        case 6:
          return 64129;
      }
      break;
  }
  return 64100;
}

int is_player_grouped(struct char_data *target, struct char_data *group) {

  struct char_data *k;
  struct follow_type *f;

  if (!target || !group)
    return FALSE;

  if (group == target)
    return TRUE;

  if (!AFF_FLAGGED(target, AFF_GROUP) || !AFF_FLAGGED(group, AFF_GROUP))
    return FALSE;

  if (group == target->master || target == group->master)
    return TRUE;

  if (group->master)
    k = group->master;
  else
    k = group;

  for (f = k->followers; f; f = f->next) {
    if (f->follower == target)
      return TRUE;
  }
  
  return FALSE;
}

int calculate_max_hit(struct char_data *ch) {

  if (IS_NPC(ch) || GET_ADMLEVEL(ch) > 0 || !ch->desc)
    return GET_MAX_HIT(ch);

  int hp = 20;
  int i = 0;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (GET_CLASS_RANKS(ch, i) > 0 && i != CLASS_ARTISAN) {
      hp += MAX(0, MIN(40, GET_CLASS_RANKS(ch, i))) * (MAX(4, MIN(12, class_hit_die_size_dl_aol[i])) + 
            ability_mod_value(GET_CON(ch)));
      if (!IS_NPC(ch)) {
        hp += GET_CLASS_RANKS(ch, i) * HAS_FEAT(ch, FEAT_TOUGHNESS);
      }
    }
  }

  if (affected_by_spell(ch, SPELL_INSPIRE_GREATNESS))
    hp += 20 + (ability_mod_value(GET_CON(ch)) * GET_CLASS_LEVEL(ch));

  switch (GET_RACE(ch)) {
    case RACE_MINOTAUR:
      hp += 32;
      break;
  }

  int hitmod = 0;

  int j = 0;

  struct obj_data *obj;

  for (i = 0; i < NUM_WEARS; i++) {
    if ((obj = GET_EQ(ch, i))) {
      for (j = 0; j < 6; j++) {
        if (obj->affected[j].location == APPLY_HIT)
          hitmod = MAX(hitmod, obj->affected[j].modifier);
      }
    }
  }

  hp += hitmod;

  hp += HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS) * 30;

  if (ch->mentor_level > 0)
    hp = hp * ch->mentor_level / GET_CLASS_LEVEL(ch);

  return MAX(1, MIN(5000, hp));
}

int has_curse_word(struct char_data *ch, char *arg_orig)
{

  if (!*arg_orig)
    return FALSE;

  char *arg = strdup(arg_orig);

  int j = 0, k = 0;
  char word[100];

  for (j = 0; j < NUM_CURSE_WORDS; j++) {
    sprintf(word, "%s", curse_words[j]);
    for (k = 0; k < strlen(word); k++)
      word[k] = tolower(word[k]);
    for (k = 0; k < strlen(arg); k++)
      arg[k] = tolower(arg[k]);
    if (strstr(arg, word) || !strcmp(arg, word)) {
      send_to_char(ch, "Swearing is not allowed on this game.  Please phrase what you want to say with cleaner language.\r\n"
                       "Attempting to bypass this safeguard will result in you losing the ability to communicate.\r\n");
      send_to_char(ch, "The curse word in question was '@R%s@n'.\r\n", word);
      if (arg)
        free(arg);
      return TRUE;      
    }
  }
  
  if (arg)
    free(arg);
  return FALSE;

}


int valid_crafting_descs(int vnum, char *string)
{

  switch (vnum) {
    case 203:
      if (strstr(string, "chain shirt"))
        return TRUE;
      break;
    case 207:
      if (strstr(string, "breastplate") || strstr(string, "breast plate"))
        return TRUE;
      break;
    case 211:
      if (strstr(string, "field plate armor") || strstr(string, "full plate armor"))
        return TRUE;
      break;
    case 216:
      if (strstr(string, "large shield"))
        return TRUE;
      break;
    case 218:
      if (strstr(string, "dirk") || strstr(string, "knife"))
        return TRUE;
      break;
    case 225:
      if (strstr(string, "staff"))
        return TRUE;
      break;
    case 234:
      if (strstr(string, "gladius") || strstr(string, "wakizashi"))
        return TRUE;
      break;
    case 238:
      if (strstr(string, "broad sword"))
        return TRUE;
      break;
    case 240:
      if (strstr(string, "epee"))
        return TRUE;
      break;
    case 242:
      if (strstr(string, "battle hammer"))
        return TRUE;
      break;
    case 255:
      if (strstr(string, "katana"))
        return TRUE;
      break;
    case 295:
      if (strstr(string, "shirt"))
        return TRUE;
      if (strstr(string, "tunic"))
        return TRUE;
      if (strstr(string, "robe"))
        return TRUE;
      if (strstr(string, "jacket"))
        return TRUE;
      if (strstr(string, "gi"))
        return TRUE;
      if (strstr(string, "suit"))
        return TRUE;
      if (strstr(string, "dress"))
        return TRUE;
      if (strstr(string, "blouse"))
        return TRUE;
      break;
    case 285:
      if (strstr(string, "band"))
        return TRUE;
      break;
    case 286:
      if (strstr(string, "necklace"))
        return TRUE;
      if (strstr(string, "choker"))
        return TRUE;
      if (strstr(string, "chain"))
        return TRUE;
      if (strstr(string, "collar"))
        return TRUE;
      if (strstr(string, "pendant"))
        return TRUE;
      if (strstr(string, "gorget"))
        return TRUE;
      if (strstr(string, "badge"))
        return TRUE;
      break;
    case 287:
      if (strstr(string, "bracelet"))
        return TRUE;
      if (strstr(string, "wristband"))
        return TRUE;
      if (strstr(string, "charm"))
        return TRUE;
      break;
    case 288:
      if (strstr(string, "cape"))
        return TRUE;
      if (strstr(string, "mantle"))
        return TRUE;
      if (strstr(string, "shroud"))
        return TRUE;
      break;
    case 289:
      if (strstr(string, "girdle"))
        return TRUE;
      if (strstr(string, "waistband"))
        return TRUE;
      break;
    case 290:
      if (strstr(string, "gauntlets"))
        return TRUE;
      if (strstr(string, "hand wraps"))
        return TRUE;
      break;
    case 291:
      if (strstr(string, "shoes"))
        return TRUE;
      if (strstr(string, "sandals"))
        return TRUE;
      if (strstr(string, "sollerets"))
        return TRUE;
      break;
    case 292:
      if (strstr(string, "hat"))
        return TRUE;
      if (strstr(string, "hood"))
        return TRUE;
      if (strstr(string, "helm"))
        return TRUE;
      if (strstr(string, "crown"))
        return TRUE;
      if (strstr(string, "tiara"))
        return TRUE;
      if (strstr(string, "headband"))
        return TRUE;
      if (strstr(string, "visor"))
        return TRUE;
      if (strstr(string, "cap"))
        return TRUE;
      break;
    case 293:
      if (strstr(string, "cestus"))
        return TRUE;
      if (strstr(string, "cesti"))
        return TRUE;
      if (strstr(string, "hand razors"))
        return TRUE;
      if (strstr(string, "claws"))
        return TRUE;
      break;
  }

  return FALSE;

}

char * list_crafting_descs(int vnum)
{

  switch (vnum) {
    case 203:
      return ("chain shirt\r\n");
    case 207:
      return ("breast plate\r\nbreastplate\r\n");      
    case 211:
      return ("field plate armor\r\nfull plate armor\r\n");
    case 216:
      return ("large shield");
    case 218:
      return ("dirk\r\nknife\r\n");
    case 225:
      return ("staff\r\n");
    case 234:
      return ("gladius\r\nwakizashi\r\n");
    case 238:
      return ("broad sword\r\n");
    case 240:
      return ("epee\r\n");
    case 242:
      return ("battle hammer\r\n");
    case 255:
      return ("katana\r\n");
    case 295:
      return ("shirt\r\ntunic\r\nrobe\r\njacket\r\ngi\r\nsuit\r\ndress\r\nblouse\r\n");
    case 2605:
      return ("band\r\n");
    case 2606:
      return ("necklace\r\nchoker\r\nchain\r\ncollar\r\npendant\r\ngorget\r\nbadge\r\n");
    case 2607:
      return ("bracelet\r\nwristband\r\ncharm\r\n");
    case 2608:
      return ("cape\r\nmantle\r\nshroud\r\n");
    case 2609:
      return ("girdle\r\nwaistband\r\n");
    case 2610:
      return ("gauntlets\r\nhand wraps\r\n");
    case 2611:
      return ("shoes\r\nsandals\r\nsollerets\r\n");
    case 2612:
      return ("hat\r\nhood\r\nhelm\r\ncrown\r\ntiara\r\nheadband\r\nvisor\r\ncap\r\n");
    case 2614:
      return ("cestus\r\ncesti\r\nhand razors\r\nclaws\r\n");
  }
  return ("");
}

int calc_spellfail(struct char_data *ch) {

  int spellfail = 0;
  struct obj_data *armor = GET_EQ(ch, WEAR_BODY);
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);
  int bone_armor = HAS_FEAT(ch, FEAT_BONE_ARMOR) * 10;
  int armored_spellcasting = HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING) * 5;

  if (armor && (GET_OBJ_TYPE(armor) == ITEM_ARMOR || GET_OBJ_TYPE(armor) == ITEM_ARMOR_SUIT)) {
    if (GET_CLASS_RANKS(ch, CLASS_BARD) > 0 && GET_MEM_TYPE(ch) == MEM_TYPE_BARD && 
        highest_armor_type(ch) == ARMOR_TYPE_LIGHT && !shield)
      spellfail += 0;
    else
      spellfail += GET_OBJ_VAL(armor, 6);
  }

  if (shield && (GET_OBJ_TYPE(shield) == ITEM_ARMOR || GET_OBJ_TYPE(shield) == ITEM_ARMOR_SUIT)) {
    spellfail += GET_OBJ_VAL(shield, 6);
  }

  if (bone_armor && armor && GET_OBJ_MATERIAL(armor) == MATERIAL_BONE) {
    bone_armor -= spellfail;
    spellfail -= HAS_FEAT(ch, FEAT_BONE_ARMOR) * 10;
  }
  if (bone_armor > 0 && shield && GET_OBJ_MATERIAL(shield) == MATERIAL_BONE)
    spellfail -= bone_armor;

  if (armored_spellcasting) {
    armored_spellcasting -= spellfail;
    spellfail -= HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING) * 5;
  }
  if (armored_spellcasting > 0)
    spellfail -= armored_spellcasting;

  spellfail = MAX(0, spellfail);

  return spellfail;
}

/* Ugly hack to remove memory leak /Malar */
char * change_coins(int coins) {
  static char buf[100];
  sprintf(buf, "%d gold coins", coins);
  return buf;
}

double get_artisan_exp(struct char_data *ch)
{
  double exp = 0;
  int i = 0, j = 0;


  for (i = 0; i < SKILL_TABLE_SIZE + 1; i++) {
    if ((spell_info[spell_sort_info[i]].skilltype == SKTYPE_SKILL && spell_sort_info[i] >= SKILL_LOW_SKILL && spell_sort_info[i] <=
        SKILL_HIGH_SKILL) && IS_SET(spell_info[spell_sort_info[i]].flags, SKFLAG_CRAFT) && GET_SKILL(ch, spell_sort_info[i]) > 0) {
      for (j = 1; j <= MAX(0, GET_SKILL(ch, spell_sort_info[i])); j++) 
        exp += art_level_exp(j);
    }
  }

  for (i = 2; i <= GET_CLASS_RANKS(ch, CLASS_ARTISAN); i++)
    exp += art_level_exp(j);

  exp += GET_ARTISAN_EXP(ch);

  return exp;
}

void award_rp_points(struct char_data *ch, int points, int add)
{

  int i = 1, done = FALSE;
  
  while (!done) {

    if (points < (i * 10000)) {
      if (done) break;
      if ((points + add) > (i * 10000)) {
        GET_RP_EXP_BONUS(ch) += 1;
        send_to_char(ch, "@YYou have gained an extra 1%% (%d%% total) to all experience gains for your role playing!@n\r\n", GET_RP_EXP_BONUS(ch));
      }
      else 
        done = TRUE;
    }
    else if (points < (i * 20000)) {
      if (done) break;
      if ((points + add) > (i * 20000)) {
        GET_RP_GOLD_BONUS(ch) += 5;
        send_to_char(ch, "@YYou have gained an extra 5%% (%d%% total) to all monetary gains for your role playing!@n\r\n", GET_RP_GOLD_BONUS(ch));
      }
      else
        done = TRUE;
    }
    else if (points < (i * 30000)) {
      if (done) break;
      if ((points + add) > (i * 30000)) {
        GET_RP_ACCOUNT_EXP(ch) += 500;
        send_to_char(ch, "@YYou have gained an additional 500 account experience (%d total) for your role playing!@n\r\n", GET_RP_ACCOUNT_EXP(ch));
        if (ch->desc && ch->desc->account) {
          ch->desc->account->experience += 500;
          save_account(ch->desc->account);
        }
      }
      else
        done = TRUE;
    }
    else if (points < (i * 40000)) {
      if (done) break;
      if ((points + add) > (i * 40000)) {
        GET_RP_QP_BONUS(ch) += 10;
        send_to_char(ch, "@YYou have gained an extra 10%% (%d%% total) to all quest point bonuses for your role playing!@n\r\n", 
                     GET_RP_QP_BONUS(ch));
      }
      else
        done = TRUE;
    }
    else if (points < (i * 50000)) {
      if (done) break;
      if ((points + add) > (i * 50000)) {
        GET_RP_ART_EXP_BONUS(ch) += 1;
        send_to_char(ch, "@YYou have gained an extra 1%% (%d%% total) to all artisan experience gains for your role playing!@n\r\n", 
                     GET_RP_ART_EXP_BONUS(ch));
      }
      else
        done = TRUE;
    }
    else if (points < (i * 60000)) {
      if (done) break;
      if ((points + add) > (i * 60000)) {
        GET_RP_CRAFT_BONUS(ch) += 2;
        send_to_char(ch, "@YYou have gained an extra 0.2%% (%d%% total) to your chance to craft exceptional items for your role playing!@n\r\n", 
                     GET_RP_CRAFT_BONUS(ch));
      }
      else
        done = TRUE;
    }

    i++;
  }

  GET_RP_POINTS(ch) += add;
}

int calc_summoner_level(struct char_data *ch, int ch_class)
{
	int rval = 0;
	
	if( ch_class == CLASS_DRUID)
	{
		if( GET_CLASS_RANKS(ch, CLASS_RANGER) >= 4 && GET_CLASS_RANKS(ch, CLASS_RANGER) > GET_CLASS_RANKS(ch, CLASS_DRUID))
		{
			rval = GET_CLASS_RANKS(ch, CLASS_RANGER) - 3;
			return rval;
		}
		else
		{
			rval = GET_CLASS_RANKS(ch, CLASS_DRUID);
			return rval;
		}
	}

	return 0;
}

char *do_lower(char *buf)
{
    char rVal[MSL];
    int i, rLength;
    if(buf[0] == '\0')
    {
        return NULL;
    }
    rLength = strlen(buf);
    for(i = 0; i <= rLength; i++)
        rVal[i] = tolower(buf[i]);

    return (buf = rVal);
}

char *do_upper(char *buf, bool do_all)
{
    char rVal[MSL];
    int i, rLength;
    if(buf[0] == '\0')
    {
        return NULL;
    }
    if(do_all == TRUE)
    {
        rLength = strlen(buf);
        for(i = 0; i <= rLength; i++)
            rVal[i] = toupper(buf[i]);
    }
    else
    {
        rLength = strlen(buf);
        for(i = 0; i <= rLength; i++)
        {
            if( i == 0) // First word is always Upper Case
                rVal[i] = toupper(buf[i]);
            else if( isspace(buf[i - 1])) // Checks to see if the last character was a whitespace character. If so it will make the current one caps.
                rVal[i] = toupper(buf[i]);
            else
                rVal[i] = buf[i];
        }
    }
    return (buf = rVal);

}

void send_to_world(char *message)
{
  struct char_data *tch = NULL;

  for (tch = character_list; tch; tch = tch->next) {
    if (!IS_NPC(tch))
      send_to_char(tch, message);
  }
}

int get_feat_value(struct char_data *ch, int featnum)
{

  int i = 0, j = 0, k = 0;
  int featval = ch->feats[featnum];
  int found = FALSE;

  for (i = 0; i < NUM_WEARS; i++) 
  {
    if (GET_EQ(ch, i)) 
    {
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
      {
        if (!GET_EQ(ch, i) || !GET_EQ(ch, i)->affected)
          continue;
        if (GET_EQ(ch, i)->affected[j].location == APPLY_FEAT && GET_EQ(ch, i)->affected[j].specific == featnum)
        {
          while (level_feats[k][4] != FEAT_UNDEFINED) 
          {
            if (level_feats[k][4] == featnum) 
            {
              found = TRUE;
              if (GET_CLASS(ch) == level_feats[k][0] && level_feats[k][1] == RACE_UNDEFINED && 
                  GET_CLASS_RANKS(ch, level_feats[k][0]) <= level_feats[k][3]) 
              {
                featval += GET_EQ(ch, i)->affected[j].modifier;
                break;
              }
            }
            k++;
          }
          if (!found)
            featval += GET_EQ(ch, i)->affected[j].modifier;
        }
      }
    }  
  }

  return featval;

}

char *alignment_string(struct char_data *ch)
{
  char align[100];

  if (GET_ALIGNMENT(ch) == 0 && GET_ETHIC_ALIGNMENT(ch) == 0)
    sprintf(align, "True Neutral");
  else
    sprintf(align, "%s %s", GET_ETHIC_ALIGNMENT(ch) > 0 ? "Lawful" : (GET_ETHIC_ALIGNMENT(ch) < 0 ? "Chaotic" : "Neutral"),
            GET_ALIGNMENT(ch) > 0 ? "Good" : (GET_ALIGNMENT(ch) < 0 ? "Evil" : "Neutral"));

  return strdup(align);
}

int get_combat_bonus(struct char_data *ch) {

  int bonus = 0;

  if ((get_size(ch) - SIZE_MEDIUM) > 0)
    bonus += pow(2, get_size(ch) - SIZE_MEDIUM);
  else if ((SIZE_MEDIUM - get_size(ch)) > 0)
    bonus -= pow(2, SIZE_MEDIUM - get_size(ch));

  bonus += GET_BAB(ch);

  bonus += ability_mod_value(GET_STR(ch));

  bonus += get_skill_value(ch, SKILL_COMBAT_TACTICS) / 10;

  return bonus;

}

int get_combat_defense(struct char_data *ch) {

  int defense = 0;

  defense = get_touch_ac(ch);

  defense += GET_BAB(ch) * 10;

  defense += ability_mod_value(GET_STR(ch)) * 10;

  if ((get_size(ch) - SIZE_MEDIUM) > 0)
    defense += pow(2, get_size(ch) - SIZE_MEDIUM) * 10;
  else if ((SIZE_MEDIUM - get_size(ch)) > 0)
    defense -= pow(2, SIZE_MEDIUM - get_size(ch)) * 10;

  // already calculated in get_touch_ac and we need to use the special size modifier above instead
  defense -= (SIZE_MEDIUM - get_size(ch)) * 10;

  defense += get_skill_value(ch, SKILL_COMBAT_TACTICS);

  return defense;

}

int is_flying(struct char_data *ch)
{

  if (affected_by_spell(ch, SPELL_FLY))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_WINGS))
    return TRUE;

  if (IS_EAGLE(ch))
    return TRUE;

  if (ch->player_specials->mounted == MOUNT_SUMMON && pet_list[ch->player_specials->summon_num].flying)
    return TRUE;

  if (ch->player_specials->mounted == MOUNT_MOUNT && pet_list[ch->player_specials->mount_num].flying)
    return TRUE;

  return FALSE;

}

char *strip_color(char *string)
{
/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @
*/

  int i = 0, j = 0;
  char newstring[MAX_STRING_LENGTH];

  for (i = 0; i < strlen(string); i++) {
    if (string[i] == '@') {
      if (string[i+1] == 'n' || string[i+1] == 'd' || string[i+1] == 'D' || string[i+1] == 'b' || string[i+1] == 'B' ||
          string[i+1] == 'g' || string[i+1] == 'G' || string[i+1] == 'c' || string[i+1] == 'C' || string[i+1] == 'n' ||
          string[i+1] == 'R' || string[i+1] == 'm' || string[i+1] == 'M' || string[i+1] == 'y' || string[i+1] == 'Y' ||
          string[i+1] == 'w' || string[i+1] == 'W' || string[i+1] == '0' || string[i+1] == '1' || string[i+1] == '2' ||
          string[i+1] == '3' || string[i+1] == '4' || string[i+1] == '5' || string[i+1] == '6' || string[i+1] == '7' ||
          string[i+1] == 'x' || string[i+1] == 'l' || string[i+1] == 'o' || string[i+1] == 'u' || string[i+1] == 'e') {
          i++;
          continue;
      }
      else {
        continue;
      }
    }
    else {
          newstring[j] = string[i];
          j++;
          continue;
    }
  }

  newstring[j] = '\0';

  return strdup(newstring);

}

int get_synth_bonus(struct char_data *ch)
{

  return ch->synth_value;


}

int has_unlocked_race(struct char_data *ch, int race)
{

  if (!ch || !ch->desc || !ch->desc->account)
    return FALSE;

  if (race_list[race].level_adjustment == 0)
    return TRUE;

  if (GET_ADMLEVEL(ch) > 0)
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
    if (ch->desc->account->races[i] == race)
      return TRUE;

  return FALSE;

}

int has_unlocked_class(struct char_data *ch, int class)
{

  if (!ch || !ch->desc || !ch->desc->account)
    return FALSE;

  if (!prestige_classes_dl_aol[class])
    return TRUE;

  if (GET_ADMLEVEL(ch) > 0)
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
    if (ch->desc->account->classes[i] == class)
      return TRUE;

  return FALSE;

}

int convert_material_vnum(int vnum)
{

  switch (vnum) {
    case 64000:
      return 64002;

    case 64002:
      return 64007;

    case 64007:
      return 64010;

    case 64014:
      return 64029;

    case 64029:
      return 64030;

    case 64030:
      return 64025;

    case 64015:
      return 64031;

    case 64031:
      return 64032;

    case 64032:
      return 64016;

    case 64020:
      return 64017;

    case 64017:
      return 64021;

    case 64021:
      return 64023;

    case 64010:
    case 64016:
    case 64023:
    case 64025:
      return 0;
    
  }

  return 0;
}
int get_crafting_material(int obj_vnum, int mat_type) {

  return 0;
/*

  struct obj_data *obj = read_obj(obj_vnum, VIRTUAL);

  if (!obj)
    return 64000;

  int mat = GET_OBJ_MATERIAL(obj);

  if (IS_HARD_METAL(mat)) {
    if (mat_type == MAT_RARE)
      return 64007;
    if (mat_type == MAT_UNCOMMON)
      return 64002;
    else
      return 64000;
  }
  else if (IS_PRECIOUS_METAL(mat)) {
    if (mat_type == MAT_RARE)
      return 64005;
    if (mat_type == MAT_UNCOMMON)
      return 64004;
    else
      return 64027;
  }
  else if (IS_WOOD(mat)) {
    if (mat_type == MAT_RARE)
      return 64005;
    if (mat_type == MAT_UNCOMMON)
      return 64004;
    else
      return 64027;
  }
  else if (IS_LEATHER(mat)) {
    if (mat_type == MAT_RARE)
      return 64005;
    if (mat_type == MAT_UNCOMMON)
      return 64004;
    else
      return 64027;
  }
  else if (IS_CLOTH(mat)) {
    if (mat_type == MAT_RARE)
      return 64005;
    if (mat_type == MAT_UNCOMMON)
      return 64004;
    else
      return 64027;
  }
  else {
    RETURN 64000;
  }

*/
}
/*
char *randomString(int length) {

  int i = 0;

  char letters[62] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
                       'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
                       '0','1','2','3','4','5','6','7','8','9','0'};

  char string[length+1];
  int roll = 0;

  for (i = 0; i < length; i++) {
    roll = dice(1, 63) - 1;
    string[i] = letters[roll];
  }

  string[length+1] = '\0';

  return strdup(string);

}
*/


int get_bonus_type_int(char *arg)
{

  if (!*arg)
    return 0;

  if (is_abbrev(arg, "strength")) {
    return APPLY_STR;
  } else if (is_abbrev(arg, "dexterity")) {
    return APPLY_DEX;
  } else if (is_abbrev(arg, "constitution")) {
    return APPLY_CON;
  } else if (is_abbrev(arg, "intelligence")) {
    return APPLY_INT;
  } else if (is_abbrev(arg, "wisdom")) {
    return APPLY_WIS;
  } else if (is_abbrev(arg, "charisma")) {
    return APPLY_CHA;
  } else if (is_abbrev(arg, "hit-points")) {
    return APPLY_HIT;
  } else if (is_abbrev(arg, "stamina")) {
    return APPLY_MOVE;
  } else if (is_abbrev(arg, "deflection")) {
    return APPLY_AC_DEFLECTION;
  } else if (is_abbrev(arg, "natural-armor")) {
    return APPLY_AC_NATURAL;
  } else if (is_abbrev(arg, "armor-bonus")) {
    return APPLY_AC_ARMOR;
  } else if (is_abbrev(arg, "shield-bonus")) {
    return APPLY_AC_SHIELD;
  } else if (is_abbrev(arg, "weapon-bonus")) {
    return APPLY_ACCURACY;
  } else if (is_abbrev(arg, "spell-level-0")) {
    return APPLY_SPELL_LVL_0;
  } else if (is_abbrev(arg, "spell-level-1")) {
    return APPLY_SPELL_LVL_1;
  } else if (is_abbrev(arg, "spell-level-2")) {
    return APPLY_SPELL_LVL_2;
  } else if (is_abbrev(arg, "spell-level-3")) {
    return APPLY_SPELL_LVL_3;
  } else if (is_abbrev(arg, "spell-level-4")) {
    return APPLY_SPELL_LVL_4;
  } else if (is_abbrev(arg, "spell-level-5")) {
    return APPLY_SPELL_LVL_5;
  } else if (is_abbrev(arg, "spell-level-6")) {
    return APPLY_SPELL_LVL_6;
  } else if (is_abbrev(arg, "spell-level-7")) {
    return APPLY_SPELL_LVL_7;
  } else if (is_abbrev(arg, "spell-level-8")) {
    return APPLY_SPELL_LVL_8;
  } else if (is_abbrev(arg, "spell-level-9")) {
    return APPLY_SPELL_LVL_9;
  } else if (is_abbrev(arg, "ki-points")) {
    return APPLY_KI;
  } else if (is_abbrev(arg, "fortitude")) {
    return APPLY_FORTITUDE;
  } else if (is_abbrev(arg, "reflex")) {
    return APPLY_REFLEX;
  } else if (is_abbrev(arg, "willpower")) {
    return APPLY_WILL;
  } else if (is_abbrev(arg, "all-saves")) {
    return APPLY_ALLSAVES;
  } else {
    return 0;
  }
  
  return 0;

}

char * get_bonus_type(char *arg)
{

  if (!*arg)
    return strdup("@n");

  if (is_abbrev(arg, "strength")) {
    return strdup("strength");
  } else if (is_abbrev(arg, "dexterity")) {
    return strdup("dexterity");
  } else if (is_abbrev(arg, "constitution")) {
    return strdup("constitution");
  } else if (is_abbrev(arg, "intelligence")) {
    return strdup("intelligence");
  } else if (is_abbrev(arg, "wisdom")) {
    return strdup("wisdom");
  } else if (is_abbrev(arg, "charisma")) {
    return strdup("charisma");
  } else if (is_abbrev(arg, "hit-points")) {
    return strdup("hit-points");
  } else if (is_abbrev(arg, "stamina")) {
    return strdup("stamina");
  } else if (is_abbrev(arg, "deflection")) {
    return strdup("deflection");
  } else if (is_abbrev(arg, "natural-armor")) {
    return strdup("natural-armor");
  } else if (is_abbrev(arg, "armor-bonus")) {
    return strdup("armor-bonus");
  } else if (is_abbrev(arg, "shield-bonus")) {
    return strdup("shield-bonus");
  } else if (is_abbrev(arg, "weapon-bonus")) {
    return strdup("weapon-bonus");
  } else if (is_abbrev(arg, "spell-level-0")) {
    return strdup("spell-level-0");
  } else if (is_abbrev(arg, "spell-level-1")) {
    return strdup("spell-level-1");
  } else if (is_abbrev(arg, "spell-level-2")) {
    return strdup("spell-level-2");
  } else if (is_abbrev(arg, "spell-level-3")) {
    return strdup("spell-level-3");
  } else if (is_abbrev(arg, "spell-level-4")) {
    return strdup("spell-level-4");
  } else if (is_abbrev(arg, "spell-level-5")) {
    return strdup("spell-level-5");
  } else if (is_abbrev(arg, "spell-level-6")) {
    return strdup("spell-level-6");
  } else if (is_abbrev(arg, "spell-level-7")) {
    return strdup("spell-level-7");
  } else if (is_abbrev(arg, "spell-level-8")) {
    return strdup("spell-level-8");
  } else if (is_abbrev(arg, "spell-level-9")) {
    return strdup("spell-level-9");
  } else if (is_abbrev(arg, "ki-points")) {
    return strdup("ki-points");
  } else if (is_abbrev(arg, "fortitude")) {
    return strdup("fortitude");
  } else if (is_abbrev(arg, "reflex")) {
    return strdup("reflex");
  } else if (is_abbrev(arg, "willpower")) {
    return strdup("willpower");
  } else if (is_abbrev(arg, "all-saves")) {
    return strdup("all-saves");
  } else {
    return strdup("@n");
  }
  
  return strdup("@n");

}

char * list_bonus_types(void)
{

  char buf[1000];

  sprintf(buf, 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n" 
    "%-19s %-19s %-19s %-19s\r\n",
    "strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma", "hit-points", "stamina",
    "deflection", "natural-armor", "armor-bonus", "shield-bonus", "weapon-bonus", 
    "spell-level-0", "spell-level-1", "spell-level-2", "spell-level-3", "spell-level-4", "spell-level-5",
    "spell-level-6", "spell-level-7", "spell-level-8", "spell-level-9", "ki-points", "fortitude",
    "reflex", "willpower", "all-saves"
  );

  return strdup(buf);
}

int get_highest_group_level(struct char_data *ch)
{
  struct char_data *tch = NULL;

  int highest = MIN(GET_CLASS_LEVEL(ch), ch->mentor_level > 0 ? ch->mentor_level : GET_CLASS_LEVEL(ch));

  for (tch = character_list; tch; tch = tch->next) {

    if (IS_NPC(tch))
      continue;

    if (is_player_grouped(ch, tch)) {
      if (MIN(GET_CLASS_LEVEL(ch), ch->mentor_level > 0 ? ch->mentor_level : GET_CLASS_LEVEL(ch)) > highest)
        highest = MIN(GET_CLASS_LEVEL(ch), ch->mentor_level > 0 ? ch->mentor_level : GET_CLASS_LEVEL(ch));
    }
  }
  return highest;
}

int can_enter_dungeon(struct char_data *ch, int room) {

  if (room == NOWHERE)
    return FALSE;

  if (zone_table[world[room].zone].zone_status == 3) {
    int min_j = 60;
    int max_j = 0;
    int j = 0;
    for (j = 1; j < NUM_LEVEL_RANGES; j++) {
      if (IS_SET(zone_table[world[room].zone].level_range, (1 << j))) {
        if (j < min_j)
          min_j = j;
        if (j > max_j)
          max_j = j;
      }
    }
    if (min_j == 60 || max_j == 0) {
      send_to_char(ch, "That dungeon has not had its min or max level set, and thus cannot be entered.\r\n");
      return FALSE;
    }
    if (ch->mentor_level > 0) {
      if (ch->mentor_level  < ((min_j * 2) - 1) || ch->mentor_level > (max_j * 2)) {
        send_to_char(ch, "Your mentor level must be between %d and %d within this dungeon.\r\n", (min_j * 2) - 1, max_j * 2);
        return FALSE;
      } 
    }
    else if (GET_CLASS_LEVEL(ch)  < ((min_j * 2) - 1) || GET_CLASS_LEVEL(ch) > (max_j * 2)) {
      send_to_char(ch, "Your class level must be between %d and %d within this dungeon.\r\n", (min_j * 2) - 1, max_j * 2);
      return FALSE;
    } 
  }

  return TRUE;

}

void fight_output(const char *str, struct char_data *ch, struct char_data *vict, int type)
{

  struct char_data *tch;

  switch (type) {

    case TO_CHAR:
      if (!PRF_FLAGGED(ch, PRF_FIGHT_SPAM) && ch->combat_output == OUTPUT_NORMAL)
        act(str, false, ch, 0, vict, TO_CHAR | TO_SLEEP);     
    break;
    
    case TO_VICT:
      if (!PRF_FLAGGED(vict, PRF_FIGHT_SPAM) && vict->combat_output == OUTPUT_NORMAL)
        act(str, false, ch, 0, vict, TO_VICT | TO_SLEEP);
    break;

    case TO_NOTVICT:
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (!PRF_FLAGGED(tch, PRF_FIGHT_SPAM) && tch != vict && tch != ch)
          perform_act(str, ch, 0, vict, tch);
      }
    break;

    case TO_ROOM:
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if ((!PRF_FLAGGED(tch, PRF_FIGHT_SPAM) && tch != ch)  || tch->combat_output != OUTPUT_SPARSE)
          perform_act(str, ch, 0, vict, tch);
      }
    break;
  }
}

void sendMSDP(struct descriptor_data *d) {
  if (!d)
    return;
  if (!d->character)
    return;
  struct char_data *ch = d->character;
  char val[200];
  struct char_data *tch = NULL;
  int gcnt = 0;
  char var[200], var2[200], var3[200], var4[200], var5[200], var6[200];
  float xp = 0;
  float percent = 0;
  int int_xp = 0; // Groups
  sprintf(val, "%d", GET_HIT(ch));
  MSDPSendPair( d, "HEALTH", val);
  sprintf(val, "%d", GET_MAX_HIT(ch));
  MSDPSendPair( d, "MAX_HEALTH", val);
  sprintf(val, "%d", GET_MANA(ch));
  MSDPSendPair( d, "MANA", val);
  sprintf(val, "%d", GET_MAX_MANA(ch));
  MSDPSendPair( d, "MAX_MANA", val);
  sprintf(val, "%d", GET_MOVE(ch));
  MSDPSendPair( d, "MOVEMENT", val);
  sprintf(val, "%d", GET_MAX_MOVE(ch));
  MSDPSendPair( d, "MAX_MOVEMENT", val);
  if (FIGHTING(ch)) {
    sprintf(val, "%d", GET_HIT(FIGHTING(ch)) * 100 / GET_MAX_HIT(FIGHTING(ch)));
    MSDPSendPair( d, "OPPONENT_HEALTH", val);
    if (FIGHTING(FIGHTING(ch))) {
      sprintf(val, "%d", GET_HIT(FIGHTING(FIGHTING(ch))));
      MSDPSendPair( d, "TANK_HEALTH", val);
      sprintf(val, "%d", GET_MAX_HIT(FIGHTING(FIGHTING(ch))));
      MSDPSendPair( d, "TANK_MAX_HEALTH", val);
    }
    else {
      sprintf(val, "0");
      MSDPSendPair( d, "TANK_HEALTH", val);
      MSDPSendPair( d, "TANK_MAX_HEALTH", val);
    }
  }
  else {
    sprintf(val, "0");
    MSDPSendPair( d, "OPPONENT_HEALTH", val);
    MSDPSendPair( d, "TANK_HEALTH", val);
    MSDPSendPair( d, "TANK_MAX_HEALTH", val);
  }
  tch = ch;
  xp = (((float) GET_EXP(tch)) - ((float) level_exp(GET_CLASS_LEVEL(tch), GET_REAL_RACE(tch)))) /
        (((float) level_exp((GET_CLASS_LEVEL(tch) + 1), GET_REAL_RACE(tch)) -
        (float) level_exp(GET_CLASS_LEVEL(tch), GET_REAL_RACE(tch))));
  xp *= (float) 1000.0;
  percent = (int) xp % 10;
  xp /= (float) 10;
  int_xp = MAX(0, (int) xp);
  sprintf(val, "%d", int_xp);
  MSDPSendPair(d, "XP_TNL", val);
  for (tch = character_list; tch; tch = tch->next) {
    if (is_player_grouped(ch, tch)) {
      gcnt++;
      xp = (((float) GET_EXP(tch)) - ((float) level_exp(GET_CLASS_LEVEL(tch), GET_REAL_RACE(tch)))) /
            (((float) level_exp((GET_CLASS_LEVEL(tch) + 1), GET_REAL_RACE(tch)) -
            (float) level_exp(GET_CLASS_LEVEL(tch), GET_REAL_RACE(tch))));
      xp *= (float) 1000.0;
      percent = (int) xp % 10;
      xp /= (float) 10;
      int_xp = MAX(0, (int) xp);
         
     sprintf(var, "%s", GET_NAME(tch));
     sprintf(var2, "%d", GET_HIT(tch));
     sprintf(var3, "%d", GET_MAX_HIT(tch));
     sprintf(var4, "%d", GET_MOVE(tch));
     sprintf(var5, "%d", GET_MAX_MOVE(tch));
     sprintf(var6, "%d", int_xp);
 
     if (gcnt == 1) {
        MSDPSendPair( d, "GROUP1_NAME", var);
        MSDPSendPair( d, "GROUP1_CUR_HP", var2);
        MSDPSendPair( d, "GROUP1_MAX_HP", var3);
        MSDPSendPair( d, "GROUP1_CUR_MV", var4);
        MSDPSendPair( d, "GROUP1_MAX_MV", var5);
        MSDPSendPair( d, "GROUP1_TNL", var6);
      }
      if (gcnt == 2) {
        MSDPSendPair( d, "GROUP2_NAME", var);
        MSDPSendPair( d, "GROUP2_CUR_HP", var2);
        MSDPSendPair( d, "GROUP2_MAX_HP", var3);
        MSDPSendPair( d, "GROUP2_CUR_MV", var4);
        MSDPSendPair( d, "GROUP2_MAX_MV", var5);
        MSDPSendPair( d, "GROUP2_TNL", var6);
      }
      if (gcnt == 3) {
        MSDPSendPair( d, "GROUP3_NAME", var);
        MSDPSendPair( d, "GROUP3_CUR_HP", var2);
        MSDPSendPair( d, "GROUP3_MAX_HP", var3);
        MSDPSendPair( d, "GROUP3_CUR_MV", var4);
        MSDPSendPair( d, "GROUP3_MAX_MV", var5);
        MSDPSendPair( d, "GROUP3_TNL", var6);
      }
      if (gcnt == 4) {
        MSDPSendPair( d, "GROUP4_NAME", var);
        MSDPSendPair( d, "GROUP4_CUR_HP", var2);
        MSDPSendPair( d, "GROUP4_MAX_HP", var3);
        MSDPSendPair( d, "GROUP4_CUR_MV", var4);
        MSDPSendPair( d, "GROUP4_MAX_MV", var5);
        MSDPSendPair( d, "GROUP4_TNL", var6);
      }
      if (gcnt == 5) {
        MSDPSendPair( d, "GROUP5_NAME", var);
        MSDPSendPair( d, "GROUP5_CUR_HP", var2);
        MSDPSendPair( d, "GROUP5_MAX_HP", var3);
        MSDPSendPair( d, "GROUP5_CUR_MV", var4);
        MSDPSendPair( d, "GROUP5_MAX_MV", var5);
        MSDPSendPair( d, "GROUP5_TNL", var6);
      }
    }
  }
}

int combat_skill_roll(struct char_data *ch, int skillnum)
{

  int roll = skill_roll(ch, skillnum);

  if (roll >= 75) {
     return 8;
  } else if (roll >= 65) {
    return 7;
  } else if (roll >= 55) {
    return 6;
  } else if (roll >= 45) {
    return 5;
  } else if (roll >= 35) {
    return 4;
  } else if (roll >= 25) {
    return 3;
  } else if (roll >= 20) {
    return 2;
  } else if (roll >= 15) {
    return 1;
  } else {
    return 0;
  }
}


