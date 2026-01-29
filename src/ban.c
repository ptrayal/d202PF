/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

/* local globals */
struct ban_list_element *ban_list = NULL;

/* local functions */
void load_banned(void);
int isbanned(char *hostname);
void _write_one_node(FILE *fp, struct ban_list_element *node);
void write_ban_list(void);
ACMD(do_ban);
ACMD(do_unban);
int Valid_Name(char *newname);
void Read_Invalid_List(void);
void Free_Invalid_List(void);


const char *ban_types[] = 
{
    "no",
    "new",
    "select",
    "all",
    "ERROR",
    NULL
};


void load_banned(void)
{
    FILE *fl;
    int i, date;
    char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
    char name[MAX_NAME_LENGTH + 1];
    struct ban_list_element *next_node;

    ban_list = 0;

    if (!(fl = fopen(BAN_FILE, "r")))
    {
        if (errno != ENOENT)
        {
            log("SYSERR: Unable to open banfile '%s': %s", BAN_FILE, strerror(errno));
        }
        else
            log("   Ban file '%s' doesn't exist.", BAN_FILE);
        return;
    }

    /* Added length limits to prevent buffer overflow:
     * ban_type: 99 chars max (array is 100)
     * site_name: 50 chars max (BANNED_SITE_LENGTH)
     * name: 20 chars max (MAX_NAME_LENGTH)
     */
    while (fscanf(fl, " %99s %50s %d %20s ", ban_type, site_name, &date, name) == 4)
    {
        CREATE(next_node, struct ban_list_element, 1);
        strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);  /* strncpy: OK (n_n->site:BANNED_SITE_LENGTH+1) */
        next_node->site[BANNED_SITE_LENGTH] = '\0';
        strncpy(next_node->name, name, MAX_NAME_LENGTH);  /* strncpy: OK (n_n->name:MAX_NAME_LENGTH+1) */
        next_node->name[MAX_NAME_LENGTH] = '\0';
        next_node->date = date;

        /* Initialize to default value in case no match is found */
        next_node->type = BAN_NOT;

        for (i = BAN_NOT; i <= BAN_ALL; i++)
            if (!strcmp(ban_type, ban_types[i]))
                next_node->type = i;

        /* Warn if an invalid ban type was found in the file */
        if (next_node->type == BAN_NOT && strcmp(ban_type, ban_types[BAN_NOT]))
        {
            log("WARNING: Invalid ban type '%s' for site '%s' in ban file, defaulting to BAN_NOT",
                ban_type, site_name);
        }

        next_node->next = ban_list;
        ban_list = next_node;
    }

    fclose(fl);
}


int isbanned(char *hostname)
{
    int i;
    struct ban_list_element *banned_node;
    char lowered_hostname[MAX_HOST_LENGTH + 1];
    char *src, *dst;

    if (!hostname || !*hostname)
        return (0);

    /* Copy hostname to local buffer and convert to lowercase without modifying original */
    src = hostname;
    dst = lowered_hostname;
    while (*src && (dst - lowered_hostname) < MAX_HOST_LENGTH)
    {
        *dst++ = LOWER(*src++);
    }
    *dst = '\0';

    /* Check against all banned sites, return the highest ban level found */
    i = 0;
    for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
        if (strstr(lowered_hostname, banned_node->site))  /* if banned site is a substring of hostname */
            i = MAX(i, banned_node->type);

    return (i);
}


void _write_one_node(FILE *fp, struct ban_list_element *node)
{
    if (!node)
        return;

    _write_one_node(fp, node->next);
    fprintf(fp, "%s %s %ld %s\n", ban_types[node->type],
            node->site, (long) node->date, node->name);
}


void write_ban_list(void)
{
    FILE *fl;

    if (!(fl = fopen(BAN_FILE, "w")))
    {
        log("SYSERR: Unable to open '" BAN_FILE "' for writing: %s", strerror(errno));
        return;
    }

    _write_one_node(fl, ban_list); /* recursively write from end to start */
    fclose(fl);
}


#define BAN_LIST_FORMAT "%-25.25s  %-8.8s  %-15.15s  %-16.16s\r\n"
ACMD(do_ban)
{
    char flag[MAX_INPUT_LENGTH];
    char site[MAX_INPUT_LENGTH];
    char *nextchar;
    char timestr[16];
    int i;
    struct ban_list_element *ban_node;

    /* No arguments - display the current ban list */
    if (!*argument)
    {
        if (!ban_list)
        {
            send_to_char(ch, "No sites are banned.\r\n");
            return;
        }
        send_to_char(ch, BAN_LIST_FORMAT,
                     "Banned Site Name",
                     "Ban Type",
                     "Banned On",
                     "Banned By");
        send_to_char(ch, BAN_LIST_FORMAT,
                     "---------------------------------",
                     "---------------------------------",
                     "---------------------------------",
                     "---------------------------------");

        for (ban_node = ban_list; ban_node; ban_node = ban_node->next)
        {
            if (ban_node->date)
            {
                strlcpy(timestr, asctime(localtime(&(ban_node->date))), sizeof(timestr));
                timestr[10] = '\0';
            }
            else
            {
                strcpy(timestr, "Unknown"); /* strcpy: OK (strlen("Unknown") < 16) */
            }

            send_to_char(ch, BAN_LIST_FORMAT, ban_node->site, ban_types[ban_node->type], timestr, ban_node->name);
        }
        return;
    }

    /* Parse arguments */
    two_arguments(argument, flag, site);
    if (!*site || !*flag)
    {
        send_to_char(ch, "Usage: ban {all | select | new} site_name\r\n");
        return;
    }

    /* Validate flag */
    if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all") || !str_cmp(flag, "new")))
    {
        send_to_char(ch, "Flag must be ALL, SELECT, or NEW.\r\n");
        return;
    }

    /* Check if site is already banned */
    for (ban_node = ban_list; ban_node; ban_node = ban_node->next)
    {
        if (!str_cmp(ban_node->site, site))
        {
            send_to_char(ch, "That site has already been banned -- unban it to change the ban type.\r\n");
            return;
        }
    }

    /* Create new ban entry */
    CREATE(ban_node, struct ban_list_element, 1);
    snprintf(ban_node->site,
             sizeof(ban_node->site),
             "%.*s",
             (int)sizeof(ban_node->site) - 1,
             site);


    /* Convert site to lowercase */
    for (nextchar = ban_node->site; *nextchar; nextchar++)
        *nextchar = LOWER(*nextchar);

    ban_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH); /* strncpy: OK (b_n->name:MAX_NAME_LENGTH+1) */
    ban_node->name[MAX_NAME_LENGTH] = '\0';
    ban_node->date = time(0);

    /* Set ban type - initialize to BAN_NEW as default */
    ban_node->type = BAN_NEW;
    for (i = BAN_NEW; i <= BAN_ALL; i++)
    {
        if (!str_cmp(flag, ban_types[i]))
        {
            ban_node->type = i;
            break; /* Found match, exit loop */
        }
    }

    /* Add to ban list */
    ban_node->next = ban_list;
    ban_list = ban_node;

    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has banned %s for %s players.",
           GET_NAME(ch), site, ban_types[ban_node->type]);
    send_to_char(ch, "Site banned.\r\n");
    write_ban_list();
}

#undef BAN_LIST_FORMAT


ACMD(do_unban)
{
    char site[MAX_INPUT_LENGTH];
    struct ban_list_element * ban_node, *temp;
    int found = 0;

    one_argument(argument, site);
    if (!*site)
    {
        send_to_char(ch, "A site to unban might help.\r\n");
        return;
    }

    /* Search for the banned site */
    ban_node = ban_list;
    while (ban_node && !found)
    {
        if (!str_cmp(ban_node->site, site))
            found = 1;
        else
            ban_node = ban_node->next;
    }

    if (!found)
    {
        send_to_char(ch, "That site is not currently banned.\r\n");
        return;
    }

    /* Remove from list and log */
    REMOVE_FROM_LIST(ban_node, ban_list, next);
    send_to_char(ch, "Site unbanned.\r\n");
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s removed the %s-player ban on %s.",
           GET_NAME(ch), ban_types[ban_node->type], ban_node->site);

    free(ban_node);
    write_ban_list();
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	200

char *invalid_list[MAX_INVALID_NAMES];
int num_invalid = 0;

int Valid_Name(char *newname)
{
    int i;
    struct descriptor_data *dt;
    char tempname[MAX_INPUT_LENGTH];

    /*
     * Make sure someone isn't trying to create this same name.  We want to
     * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  The creating login
     * will not have a character name yet and other people sitting at the
     * prompt won't have characters yet.
     *
     * New, unindexed characters (i.e., characters who are in the process of creating)
     * will have an idnum of -1, set by clear_char() in db.c.  If someone is creating a
     * character by the same name as the one we are checking, then the name is invalid,
     * to prevent character duping.
     * THIS SHOULD FIX THE 'invalid name' if disconnected from OLC-bug - WELCOR 9/00
     */
    for (dt = descriptor_list; dt; dt = dt->next)
        if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname))
            if (GET_IDNUM(dt->character) == -1)
                return (IS_PLAYING(dt));

    /* Check for at least one vowel - early exit optimization */
    for (i = 0; newname[i]; i++)
    {
        if (strchr("aeiouyAEIOUY", newname[i]))
            break;
    }

    /* Return invalid if no vowel found */
    if (!newname[i])
        return (0);

    /* Return valid if invalid name list doesn't exist */
    if (num_invalid < 1)
        return (1);

    /* Convert to lowercase for case-insensitive comparison */
    strlcpy(tempname, newname, sizeof(tempname));
    for (i = 0; tempname[i]; i++)
        tempname[i] = LOWER(tempname[i]);

    /* Does the desired name contain a string in the invalid list? */
    for (i = 0; i < num_invalid; i++)
        if (strstr(tempname, invalid_list[i]))
            return (0);

    return (1);
}


/* What's with the wacky capitalization in here? */
void Free_Invalid_List(void)
{
    int i;

    for (i = 0; i < num_invalid; i++)
        free(invalid_list[i]);

    num_invalid = 0;
}

void Read_Invalid_List(void)
{
    FILE *fp;
    char temp[256];

    if (!(fp = fopen(XNAME_FILE, "r")))
    {
        log("SYSERR: Unable to open '" XNAME_FILE "' for reading: %s", strerror(errno));
        return;
    }

    num_invalid = 0;
    while (get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES)
        invalid_list[num_invalid++] = strdup(temp);

    if (num_invalid >= MAX_INVALID_NAMES)
    {
        log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
        fclose(fp);
        return;
    }

    fclose(fp);
}
