/************************************************************************
 * Generic OLC Library - Guilds / gengld.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "guild.h"
#include "genolc.h"
#include "gengld.h"
#include "genzon.h"

SVNHEADER("$Id: gengld.c 55 2009-03-20 17:58:56Z pladow $");


extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct guild_data *guild_index;
extern struct zone_data *zone_table;
extern int top_guild;
extern zone_rnum top_of_zone_table;

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_guild(struct guild_data *tgm, struct guild_data *fgm)
{
	int i;

	/*. Copy basic info over . */
	G_NUM(tgm) = G_NUM(fgm);
	G_CHARGE(tgm) = G_CHARGE(fgm);
	G_TRAINER(tgm) = G_TRAINER(fgm);
	for (i = 0; i < SW_ARRAY_MAX; i++)
		G_WITH_WHO(tgm)[i] = G_WITH_WHO(fgm)[i];
	G_OPEN(tgm) = G_OPEN(fgm);
	G_CLOSE(tgm) = G_CLOSE(fgm);
	G_MINLVL(tgm) = G_MINLVL(fgm);
	G_FUNC(tgm) = G_FUNC(fgm);


	/*. Copy the strings over . */
	free_guild_strings(tgm);
	G_NO_SKILL(tgm) = str_udup(G_NO_SKILL(fgm));
	G_NO_GOLD(tgm) = str_udup(G_NO_GOLD(fgm));

	for (i = 0; i < SKILL_TABLE_SIZE; i++)
		G_SK_AND_SP(tgm, i) = G_SK_AND_SP(fgm, i);

}

/*-------------------------------------------------------------------*/
/*. Free all the character strings in a guild structure . */

void free_guild_strings(struct guild_data *guild)
{
	if (G_NO_SKILL(guild)) {
		free(G_NO_SKILL(guild));
		G_NO_SKILL(guild) = NULL;
	}
	if (G_NO_GOLD(guild)) {
		free(G_NO_GOLD(guild));
		G_NO_GOLD(guild) = NULL;
	}
}

/*-------------------------------------------------------------------*/

/*. Free up the whole guild structure and its contents . */

void free_guild(struct guild_data *guild)
{
	free_guild_strings(guild);
	free(guild);
}

/*-------------------------------------------------------------------*/

/* returns the real number of the guild with given virtual number 
 *
 * We take so good care to keep it sorted - let's use it :) - Welcor
 */
guild_rnum real_guild(guild_vnum vnum)
{
  guild_rnum bot, top, mid, last_top;

  if (top_guild < 0)
    return NOWHERE;

  bot = 0;
  top = top_guild - top_guild_offset;

  /* perform binary search on guild_table */
  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if (GM_NUM(mid) == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if (GM_NUM(mid) > vnum)
      top = mid - 1;
    else
      bot = mid + 1;

    if (top > last_top)
      return NOWHERE;
  }
}

/*-------------------------------------------------------------------*/

/*. Generic string modifyer for guild master messages . */

void gedit_modify_string(char **str, char *new_g)
{
	char *pointer;
	char buf[MAX_STRING_LENGTH];

	/*. Check the '%s' is present, if not, add it . */
	if (*new_g != '%') {
		snprintf(buf, sizeof(buf), "%%s %s", new_g);
		pointer = buf;
	} else
		pointer = new_g;

	if (*str)
		free(*str);
	*str = strdup(pointer);
}

/*-------------------------------------------------------------------*/

int add_guild(struct guild_data *ngld)
{
  guild_rnum rguild;
  int found = 0;
  zone_rnum rznum = real_zone_by_thing(G_NUM(ngld));

  /*
   * The guild already exists, just update it.
   */
  if ((rguild = real_guild(G_NUM(ngld))) != NOWHERE) {
    copy_guild(&guild_index[rguild], ngld);
    if (rznum != NOWHERE) {
      add_to_save_list(zone_table[rznum].number, SL_GLD);
    } else
      mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine guild zone.");
    return rguild;
  }

  mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Creating new guild.");

  top_guild++;
  RECREATE(guild_index, struct guild_data, top_guild - top_guild_offset + 1);

  for (rguild = top_guild - top_guild_offset; rguild > 0; rguild--) {
    if (ngld->vnum > GM_NUM(rguild - 1)) {
      found = rguild;

      copy_guild(&guild_index[rguild], ngld);
      break;
    }
    guild_index[rguild] = guild_index[rguild - 1];
  }

  if (!found) {
    copy_guild(&guild_index[0], ngld);
  }

  if (rznum != NOWHERE) {
    add_to_save_list(zone_table[rznum].number, SL_GLD);
  }
  else
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine guild zone.");

  return rguild;
}

/*-------------------------------------------------------------------*/

int save_guilds(zone_rnum zone_num)
{
  int i, j, rguild;
  FILE *guild_file;
  char fname[64];
  struct guild_data *guild;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table)
#else
    if (zone_num < 0 || zone_num > top_of_zone_table)
#endif
      {
	log("SYSERR: GenOLC: save_guilds: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
	return FALSE;
      }

  snprintf(fname, sizeof(fname), "%s%d.gld", GLD_PREFIX, zone_table[zone_num].number);
  if (!(guild_file = fopen(fname, "w"))) {
    mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: OLC: Cannot open Guild file!");
    return FALSE;
  }

  /*. Search database for guilds in this zone . */
  for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
    if ((rguild = real_guild(i)) != NOWHERE) {
      fprintf(guild_file, "#%d~\n", i);
      guild = guild_index + rguild;

      for (j = 0; j < SKILL_TABLE_SIZE; j++)
	if (G_SK_AND_SP(guild, j))
	  fprintf(guild_file, "%d\n", j);

      fprintf(guild_file, "-1\n");

      /*. Save charge . */
      fprintf(guild_file, "%1.2f\n", G_CHARGE(guild));

      /*. Save messages . */
      fprintf(guild_file,
	      "%s~\n%s~\n",
	      /*. Added some small'n'silly defaults as sanity checks . */
	      (G_NO_SKILL(guild) ? G_NO_SKILL(guild) : "%s ERROR"),
	      (G_NO_GOLD(guild) ? G_NO_GOLD(guild) : "%s ERROR")
	      );

      /* Write what the GM teaches */
      fprintf(guild_file, "%d\n", G_MINLVL(guild));


      /*. Save the rest . */
      fprintf(guild_file, "%d\n%d\n%d\n%d\n",
	      G_TRAINER(guild) == NOBODY ? -1 : mob_index[G_TRAINER(guild)].vnum,
	      G_WITH_WHO(guild)[0],
	      G_OPEN(guild),
	      G_CLOSE(guild)
	      );
      for (j = 1; j < SW_ARRAY_MAX; j++)
	fprintf(guild_file, "%s%d", j == 1 ? "" : " ", G_WITH_WHO(guild)[j]);
      fprintf(guild_file, "\n");
    }
  }
  fprintf(guild_file, "$~\n");
  fclose(guild_file);

  if (in_save_list(zone_table[zone_num].number, SL_GLD)) {
    remove_from_save_list(zone_table[zone_num].number, SL_GLD);
    create_world_index(zone_table[zone_num].number, "gld");
    log("GenOLC: save_guilds: Saving guilds '%s'", fname);
  }
  return TRUE;
}

