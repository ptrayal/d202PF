/* ************************************************************
 * File: clanedit.c     From the guild patch by Sean Mountcastle *
 * Usage: For editing clan information               May 2001 *
 * Made into seperate file by Catherine Gore                  *
 *                                                            *
 * For use with CircleMUD 3.0/OasisOLC 2.0                    *
 ************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "constants.h"
#include "clan.h"

/* external globals */
extern const room_vnum mortal_start_room[NUM_STARTROOMS +1];
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern int cnum;
extern int newclan;
extern struct clan_type *clan_info;

/* external functions */
extern struct char_data *find_clan_char(struct char_data *ch, char *arg);
/* local functions */
void clanedit_disp_dirs_menu(struct descriptor_data *d);
void clanedit_disp_hometowns(struct descriptor_data *d);
void clanedit_disp_menu(struct descriptor_data * d);
void clanedit_free_clan(struct clan_type *cptr);
void clanedit_parse(struct descriptor_data *d, char *arg);
void clanedit_setup_new(struct descriptor_data *d);
void clanedit_setup_existing(struct descriptor_data *d, int number);
void clanedit_save_to_disk();

/******************************************************************************/
/** Routines                                                                 **/
/******************************************************************************/
ACMD(do_oasis_clanedit)
{
    struct descriptor_data *d;
    char buf1[MAX_STRING_LENGTH]={'\0'};
    struct clan_type *cptr = NULL;
    int number;

/****************************************************************************/
/** Parse any arguments.                                                   **/
/****************************************************************************/
    one_argument(argument, buf1);

    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL) 
    {
        send_to_char(ch, "You can't modify clans.\r\n");
        return;
    }

    d = ch->desc;

    if (!*buf1) 
    {
        send_to_char(ch, "Specify a clan to edit.\r\n");
        return;
    } 
    number = atoi(buf1);
    if(number == 0) 
    {
        send_to_char(ch, "Specify a valid clan to edit (-1 for new clan).\r\n");
        return;
    }
    if(number == -1) 
    {
        CREATE(d->olc, struct oasis_olc_data, 1);
        OLC_ZONE(d) = 0;
        clanedit_disp_menu(d);
    } 
    else 
    {
        for (cptr = clan_info; cptr && cptr->number != number; cptr=cptr->next);
        {
            if (cptr && (cptr->number == number)) 
            {
                CREATE(d->olc, struct oasis_olc_data, 1);
                OLC_ZONE(d) = 0;
                OLC_CLAN(d) = cptr;
                clanedit_disp_menu(d);
            } 
            else 
            {
                send_to_char(d->character, "Invalid clan number!\r\n");
                return;
            }
        }
    }
    STATE(d) = CON_CLANEDIT;
    act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

    mudlog(BRF, ADMLVL_IMMORT, TRUE,
        "OLC: %s starts editing clans.", GET_NAME(ch));

    send_to_char(ch, "Saving clans.\r\n");
    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
        "OLC: %s saves the clans.", GET_NAME(ch));

    clanedit_save_to_disk();
}

void clanedit_disp_menu(struct descriptor_data * d)
{
  room_rnum start_room;

  if (!OLC_CLAN(d))
    clanedit_setup_new(d);
  
  start_room=real_room(mortal_start_room[OLC_CLAN(d)->clan_entr_room]);

  write_to_output(d,
    "-- Clan number : [@y%d@n]\r\n"
    "@c1@n) Name        : @y%s\r\n"
    "@c2@n) Leader      : @y%s\r\n"
    "@c3@n) Rank6 Name  : @y%s\r\n"
    "@c4@n) Rank5 Name  : @y%s\r\n"
    "@c5@n) Rank4 Name  : @y%s\r\n"  
    "@c6@n) Rank3 Name  : @y%s\r\n"  
    "@c7@n) Rank2 Name  : @y%s\r\n"
    "@c8@n) Rank1 Name  : @y%s\r\n"  ,
    OLC_CLAN(d)->number,
    OLC_CLAN(d)->name,
    OLC_CLAN(d)->leadersname,
    OLC_CLAN(d)->rank_name[5],
    OLC_CLAN(d)->rank_name[4],
    OLC_CLAN(d)->rank_name[3],
    OLC_CLAN(d)->rank_name[2],
    OLC_CLAN(d)->rank_name[1],
    OLC_CLAN(d)->rank_name[0]);

  write_to_output(d,
    "@c9@n) Who String  : @y%s\r\n"
    "@cA@n) Guard 1     : @y%d %s\r\n"
    "@cF@n) Guard 2     : @y%d %s\r\n"
    "@cB@n) Direction   : @y%s\r\n"
    "@cC@n) P-Kill      : @y%s\r\n",

    OLC_CLAN(d)->member_look_str ?
    OLC_CLAN(d)->member_look_str : OLC_CLAN(d)->name,
    OLC_CLAN(d)->guard[0], ((OLC_CLAN(d)->guard[0] > 0) ? 
      mob_proto[real_mobile(OLC_CLAN(d)->guard[0])].short_descr : "Nobody"), 
    OLC_CLAN(d)->guard[1], ((OLC_CLAN(d)->guard[0] > 0) ?
      mob_proto[real_mobile(OLC_CLAN(d)->guard[1])].short_descr : "Nobody"),
    dirs[OLC_CLAN(d)->direction],
    OLC_CLAN(d)->pkill ? "Pkill Clan" : "No-pkill Clan");

  write_to_output(d,
    "@cD@n) Clan Gold   : @y%ld\r\n"
    "@cG@n) Clan recall : @y%d\r\n"
    "@cP@n) Purge this Clan\r\n"
    "@cQ@n) Quit\r\n"
    "Enter choice : ",
    
    OLC_CLAN(d)->clan_gold,
    OLC_CLAN(d)->clan_recall
    );
  
  OLC_MODE(d) = CLANEDIT_MAIN_MENU;
}

void clanedit_disp_dirs_menu(struct descriptor_data *d)
{
  int counter, columns=0;

  clear_screen(d);

  for (counter = 0; counter < NUM_OF_DIRS; counter++) {
    write_to_output(d, "%2d) %-10s %s", counter,  dirs[counter], 
      (++columns %2) ? "" : "\r\n");
  }

  return;
}

void clanedit_disp_hometowns(struct descriptor_data *d)
{
  int counter;
  room_rnum home_room;

  clear_screen(d);

  for (counter = 1; counter < NUM_STARTROOMS+1; counter++) {
    home_room = real_room(mortal_start_room[counter]);
    write_to_output(d, "%2d) %4d %s\r\n",  counter, 
      mortal_start_room[counter], world[home_room].name );
  }

  return;
}

void clanedit_free_clan(struct clan_type *cptr)
{
  dequeue_clan(cptr->number);
}

void clanedit_parse(struct descriptor_data *d, char *arg)
{
  int number;

  switch (OLC_MODE(d)) {
  case CLANEDIT_CONFIRM_SAVE:
    switch (*arg) {
    case 'y':
    case 'Y':
      save_clans();
      mudlog(CMP, ADMLVL_GOD, TRUE, "OLC: %s edits clan %d", GET_NAME(d->character), OLC_CLAN(d)->number);
      send_to_char(d->character, "Clan saved to disk and memory.\r\n");
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /* free everything up, including strings etc */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      send_to_char(d->character, "Do you wish to save this clan? : ");
      break;
    }
    return;
    
    case CLANEDIT_MAIN_MENU:
      switch (*arg) {
      case 'q':
      case 'Q':
        if (OLC_VAL(d)) {
          /*. Something has been modified .*/
          send_to_char(d->character, "Do you wish to save this clan? : ");
          OLC_MODE(d) = CLANEDIT_CONFIRM_SAVE;
        } else
          cleanup_olc(d, CLEANUP_STRUCTS);
        return;
      case '1':
        write_to_output(d, "Enter clan name:-\r\n| ");
        OLC_MODE(d) = CLANEDIT_NAME;
        break;
      case '2':
        write_to_output(d, "Enter clan leader's name: ");
        OLC_MODE(d) = CLANEDIT_LEADERSNAME;
        break;
      case '3':
        write_to_output(d, "Enter the title of Leader(Highest): ");
        OLC_MODE(d) = CLANEDIT_RANK6;       
        break;
      case '4':
        write_to_output(d, "Enter the title of Rank5: ");
        OLC_MODE(d) = CLANEDIT_RANK5;
        break;
      case '5':
        write_to_output(d, "Enter the title of Rank4: ");
        OLC_MODE(d) = CLANEDIT_RANK4;
        break;
      case '6':
        write_to_output(d, "Enter the title of Rank3: ");
        OLC_MODE(d) = CLANEDIT_RANK3;
        break;
      case '7':
        write_to_output(d, "Enter the title of Rank2: ");
        OLC_MODE(d) = CLANEDIT_RANK2;
        break;
      case '8':
        write_to_output(d, "Enter the title of Rank1(Lowest): ");
        OLC_MODE(d) = CLANEDIT_RANK1;
        break;
      case '9':
        write_to_output(d, "Enter clan member who string:-\r\n| ");
        OLC_MODE(d) = CLANEDIT_MBR_LOOK_STR;
        break;
      case 'a':
      case 'A':
        write_to_output(d, "Enter clan guard vnum: ");
        OLC_MODE(d) = CLANEDIT_GUARD1;
        break;
      case 'f':
      case 'F':
        write_to_output(d, "Enter clan guard vnum: ");
        OLC_MODE(d) = CLANEDIT_GUARD2;
        break;
      case 'b':
      case 'B':
        clanedit_disp_dirs_menu(d);
        write_to_output(d, "Enter direction of hometown: ");
        OLC_MODE(d) = CLANEDIT_DIRECTION;
        break;
      case 'c':
      case 'C':
        write_to_output(d, "Enter 1 for pkill, 0 for nopkill: ");
        OLC_MODE(d) = CLANEDIT_PKILL;
        break;
      case 'd':
      case 'D':
        write_to_output(d, "Enter the amount of clan gold: ");
        OLC_MODE(d) = CLANEDIT_CLAN_GOLD;
        break;
      case 'e':
      case 'E': 
        clanedit_disp_hometowns(d);
        write_to_output(d, "Enter clan hometown: ");
        OLC_MODE(d) = CLANEDIT_ENTR_ROOM;
        break; 
      case 'g':
      case 'G':
        write_to_output(d, "Enter the clan recall room number: ");
        OLC_MODE(d) = CLANEDIT_RECALL;
        break;
      case 'p':
      case 'P':
        if (GET_ADMLEVEL(d->character) >= ADMLVL_IMPL) {
          newclan = OLC_CLAN(d)->number;  /* next new clan will get this one's number  */
          /* free everything up, including strings etc */
          cleanup_olc(d, CLEANUP_ALL);
          cnum--;
          send_to_char(d->character, "Clan purged.\r\n");
        } else {
          write_to_output(d, "Sorry you are not allowed to do that at this time.\r\n");
          clanedit_disp_menu(d);
        }
        return;
      default:
        write_to_output(d, "Invalid choice!");
        clanedit_disp_menu(d);
        break;
      }
      return;
      
      case CLANEDIT_NAME:
        if (OLC_CLAN(d)->name)
          free(OLC_CLAN(d)->name);
        OLC_CLAN(d)->name = strdup(arg);
        break;
        
      /* could probably use a loop for these rank names ... */
      case CLANEDIT_LEADERSNAME:
        if (OLC_CLAN(d)->leadersname)
          free(OLC_CLAN(d)->leadersname);
        OLC_CLAN(d)->leadersname = strdup(arg);
        break;

      case CLANEDIT_RANK6:
        if (OLC_CLAN(d)->rank_name[5])
          free(OLC_CLAN(d)->rank_name[5]);
        OLC_CLAN(d)->rank_name[5] = strdup(arg);
        break;
              
      case CLANEDIT_RANK5:
        if (OLC_CLAN(d)->rank_name[4])
          free(OLC_CLAN(d)->rank_name[4]);
        OLC_CLAN(d)->rank_name[4] = strdup(arg);
        break;

      case CLANEDIT_RANK4:
        if (OLC_CLAN(d)->rank_name[3])
          free(OLC_CLAN(d)->rank_name[3]);
        OLC_CLAN(d)->rank_name[3] = strdup(arg);
        break;

      case CLANEDIT_RANK3:
        if (OLC_CLAN(d)->rank_name[2])
          free(OLC_CLAN(d)->rank_name[2]);
        OLC_CLAN(d)->rank_name[2] = strdup(arg);
        break;

      case CLANEDIT_RANK2:
        if (OLC_CLAN(d)->rank_name[1])
          free(OLC_CLAN(d)->rank_name[1]);
        OLC_CLAN(d)->rank_name[1] = strdup(arg);
        break;

     case CLANEDIT_RANK1:
        if (OLC_CLAN(d)->rank_name[0])
          free(OLC_CLAN(d)->rank_name[0]);
        OLC_CLAN(d)->rank_name[0] = strdup(arg);
        break;

      case CLANEDIT_MBR_LOOK_STR:
        if (OLC_CLAN(d)->member_look_str)
          free(OLC_CLAN(d)->member_look_str);
        OLC_CLAN(d)->member_look_str = strdup(arg);
        break;
        
      case CLANEDIT_ENTR_ROOM:
        number = atoi(arg);
        if (number <= 0 || number > NUM_STARTROOMS+1) 
          write_to_output(d, "That is not a start room, try again : ");
        else {
          OLC_CLAN(d)->clan_entr_room = number;
          mudlog(CMP, ADMLVL_GOD, TRUE, "OLC: %s edits HOMETOWN for clan %d", GET_NAME(d->character), OLC_CLAN(d)->number);
        }
        break;
      case CLANEDIT_GUARD1:
        if ((number = (atoi(arg))) == 0)
          OLC_CLAN(d)->guard[0] = number;
        else if ((real_mobile(number)) > 0)
          OLC_CLAN(d)->guard[0] = number;
        else {
          write_to_output(d, "That mobile does not exist, try again : ");
          OLC_MODE(d) = CLANEDIT_GUARD1;
          return;
        }
        break;
      case CLANEDIT_GUARD2:
        if ((number = (atoi(arg))) == 0)
          OLC_CLAN(d)->guard[1] = number;
        else if ((real_mobile(number)) > 0)
          OLC_CLAN(d)->guard[1] = number;
        else {
          write_to_output(d, "That mobile does not exist, try again : ");
          OLC_MODE(d) = CLANEDIT_GUARD2;
          return;
        }
        break;
      case CLANEDIT_DIRECTION:
        if ((number = atoi(arg)) < 0 || number > NUM_OF_DIRS) {
          write_to_output(d, "That is not a direction, try again : ");
          OLC_MODE(d) = CLANEDIT_DIRECTION;
          return;
        } else
          OLC_CLAN(d)->direction = number;
        break;
      case CLANEDIT_PKILL:
        if ((number = atoi(arg)) < 0 || number > 1) {
          write_to_output(d, "Pkill must be on or off (1 for on, 0 for off) : ");
          OLC_MODE(d) = CLANEDIT_PKILL;
          return;
        } else
          OLC_CLAN(d)->pkill = number;
        break;
      case CLANEDIT_CLAN_GOLD:
        if ((number = atoi(arg)) < 0 || number > 2000000000) {
          write_to_output(d, "Clan Gold must be between 0 and 2 billion : ");
          OLC_MODE(d) = CLANEDIT_CLAN_GOLD;
          return;
        } else                 
          OLC_CLAN(d)->clan_gold = number;
        break;
     case CLANEDIT_RECALL:
        if ((number = (atoi(arg))) == 0)
          OLC_CLAN(d)->clan_recall = number;
        else if ((real_room(number)) > 0)
          OLC_CLAN(d)->clan_recall = number;
        else {
          write_to_output(d, "That room does not exist, try again : ");
          OLC_MODE(d) = CLANEDIT_RECALL;
          return;
        }
        break;
      default:
        /* we should never get here */
        mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: Reached default case in clanedit_parse");
        break;
   }
   /* If we get this far, something has been changed */
   OLC_VAL(d) = 1;
   clanedit_disp_menu(d);
}

void clanedit_save_to_disk()
{
  save_clans();
}

/*
* Create a new clan with some default strings.
*/
void clanedit_setup_new(struct descriptor_data *d)
{
  int i;

  if ((OLC_CLAN(d) = enqueue_clan()) != NULL) {
    OLC_CLAN(d)->name = strdup("Unfinished Clan");
    OLC_CLAN(d)->number = newclan;
    OLC_CLAN(d)->leadersname = strdup("NoOne");
    OLC_CLAN(d)->rank_name[5] = strdup("Leader");
    OLC_CLAN(d)->rank_name[4] = strdup("Lieutenant");
    OLC_CLAN(d)->rank_name[3] = strdup("Sergeant"); 
    OLC_CLAN(d)->rank_name[2] = strdup("Member"); 
    OLC_CLAN(d)->rank_name[1] = strdup("Novice"); 
    OLC_CLAN(d)->rank_name[0] = strdup("Trainee");  
    OLC_CLAN(d)->pkill = 0;
    OLC_CLAN(d)->clan_gold = 0;
    OLC_CLAN(d)->member_look_str = NULL;
    OLC_CLAN(d)->clan_entr_room = 1;
    OLC_CLAN(d)->guard[0] = 0;
    OLC_CLAN(d)->guard[1] = 0;
    OLC_CLAN(d)->direction = 0;
    OLC_CLAN(d)->clan_recall = 0;
    for (i=0; i<20; i++)
      OLC_CLAN(d)->applicants[i] = NULL;
  } else
    fprintf(stderr, "SYSERR: Unable to create new clan!\r\n");
  cnum++;
  if (newclan == cnum)
    newclan++;
  else 
    newclan = cnum;
  clanedit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*
* Setup a new clan with existing data.
*/
void clanedit_setup_existing(struct descriptor_data *d, int number)
{
    struct clan_type *cptr = NULL;

    for (cptr = clan_info; cptr && cptr->number != number; cptr=cptr->next);
    {
        if (cptr && (cptr->number == number)) 
        {
            OLC_CLAN(d) = cptr;
            clanedit_disp_menu(d);
        } 
        else 
        {
            send_to_char(d->character, "Invalid clan number!\r\n");
            return;
        }
    }
    STATE(d) = CON_CLANEDIT;
}
