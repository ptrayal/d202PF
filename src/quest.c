/* ***********************************************************************
*    File:   quest.c                                  Part of CircleMUD  *
* Version:   2.1 (December 2005) Written for CircleMud CWG / Suntzu      *
* Purpose:   To provide special quest-related code.                      *
* Copyright: Kenneth Ray                                                 *
* Original Version Details:                                              *
* Morgaelin - quest.c                                                    *
* Copyright (C) 1997 MS                                                  *
*********************************************************************** */
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "quest.h"

/* External Functions */
int is_player_grouped(struct char_data *target, struct char_data *group);
ACMD(do_tell);
int level_exp(int level, int race);
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern long asciiflag_conv(char *flag);
/* Local Variables */
int cmd_tell;
const char *quest_types[] = {
  "Object",
  "Room",
  "Find mob",
  "Kill mob",
  "Save mob",
  "Return object",
  "Clear room",
  "\n"
};
const char *aq_flags[] = {
  "REPEATABLE",
  "\n"
};
const char *quest_cmd[] = {
  "list", "history", "join", "leave", "progress", "status", "\n"};
const char *quest_mort_usage =
  "Usage: quest list | history | progress | join <nn> | leave";
const char *quest_imm_usage =
  "Usage: quest list | history | progress | join <nn> | leave | status <vnum>";

/*--------------------------------------------------------------------------*/
/* Utility Functions                                                        */
/*--------------------------------------------------------------------------*/

qst_rnum real_quest(qst_vnum vnum)
{
  int rnum;

  for (rnum = 0; rnum < total_quests; rnum++)
    if (QST_NUM(rnum) == vnum)
      return(rnum);
  return(NOTHING);
}

int is_complete(struct char_data *ch, qst_vnum vnum)
{
  int i;

  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->completed_quests[i] == vnum)
      return TRUE;
  return FALSE;
}

qst_vnum find_quest_by_qmnum(struct char_data *ch, mob_rnum qm, int num)
{
  qst_rnum rnum;
  int found=0;
  for (rnum = 0; rnum < total_quests; rnum++) {
    if (qm == QST_MASTER(rnum))
      if (++found == num)
        return (QST_NUM(rnum));
  }
  return NOTHING;
}

/*--------------------------------------------------------------------------*/
/* Quest Loading and Unloading Functions                                    */
/*--------------------------------------------------------------------------*/

void destroy_quests(void)
{
  qst_rnum rnum = 0;

  if (!aquest_table)
    return;

  for (rnum = 0; rnum < total_quests; rnum++){
    free_quest_strings(&aquest_table[rnum]);
  }
  free(aquest_table);
  aquest_table = NULL;
  total_quests = 0;

  return;
}

int count_quests(qst_vnum low, qst_vnum high)
{
  int i, j;

  for (i = j = 0; QST_NUM(i) <= high; i++)
    if (QST_NUM(i) >= low)
      j++;

  return j;
}

void parse_quest(FILE *quest_f, int nr)
{
  static char line[256];
  static int i = 0, j;
  int retval = 0, t[7];
  char f1[128], buf2[MAX_STRING_LENGTH];
  aquest_table[i].vnum = nr;
  aquest_table[i].qm = NOBODY;
  aquest_table[i].name = NULL;
  aquest_table[i].desc = NULL;
  aquest_table[i].info = NULL;
  aquest_table[i].done = NULL;
  aquest_table[i].quit = NULL;
  aquest_table[i].flags = 0;
  aquest_table[i].type = -1;
  aquest_table[i].target = -1;
  aquest_table[i].prereq = NOTHING;
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = 0;
  aquest_table[i].prev_quest = NOTHING;
  aquest_table[i].next_quest = NOTHING;
  aquest_table[i].func = NULL;

  aquest_table[i].gold_reward = 0;
  aquest_table[i].exp_reward  = 0;
  aquest_table[i].obj_reward  = NOTHING;

  /* begin to parse the data */
  aquest_table[i].name = fread_string(quest_f, buf2);
  aquest_table[i].desc = fread_string(quest_f, buf2);
  aquest_table[i].info = fread_string(quest_f, buf2);
  aquest_table[i].done = fread_string(quest_f, buf2);
  aquest_table[i].quit = fread_string(quest_f, buf2);
  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %s %d %d %d %d",
             t, t+1, f1, t+2, t+3, t + 4, t + 5)) != 7) {
    log("Format error in numeric line (expected 7, got %d), %s\n",
        retval, line);
    exit(1);
  }
  aquest_table[i].type       = t[0];
  aquest_table[i].qm         = real_mobile(t[1]);
  aquest_table[i].flags      = asciiflag_conv(f1);
  aquest_table[i].target     = (t[2] == -1) ? NOTHING : t[2];
  aquest_table[i].prev_quest = (t[3] == -1) ? NOTHING : t[3];
  aquest_table[i].next_quest = (t[4] == -1) ? NOTHING : t[4];
  aquest_table[i].prereq     = (t[5] == -1) ? NOTHING : t[5];
  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %d %d %d %d %d",
		       t, t+1, t+2, t+3, t+4, t + 5, t + 6)) != 7) {
    log("Format error in numeric line (expected 7, got %d), %s\n",
        retval, line);
    exit(1);
  }
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = t[j];

  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %d",
             t, t+1, t+2)) != 3) {
    log("Format error in numeric (rewards) line (expected 3, got %d), %s\n",
        retval, line);
    exit(1);
  }

  aquest_table[i].gold_reward = t[0];
  aquest_table[i].exp_reward  = t[1];
  aquest_table[i].obj_reward  = (t[2] == -1) ? NOTHING : t[2];

  for (;;) {
    if (!get_line(quest_f, line)) {
      log("Format error in %s\n", line);
      exit(1);
    }
    switch(*line) {
    case 'S':
      total_quests = ++i;
      return;
      break;
    }
  }
} /* parse_quest */

void assign_the_quests(void)
{
  qst_rnum rnum;

  cmd_tell = find_command("tell");

  for (rnum = 0; rnum < total_quests; rnum ++) {
    if (QST_MASTER(rnum) == NOBODY) {
      log("SYSERR: Quest #%d has no questmaster specified.", QST_NUM(rnum));
      continue;
    }
    if (mob_index[QST_MASTER(rnum)].func &&
	mob_index[QST_MASTER(rnum)].func != questmaster)
      QST_FUNC(rnum) = mob_index[QST_MASTER(rnum)].func;
    mob_index[QST_MASTER(rnum)].func = questmaster;
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Completion Functions                                               */
/*--------------------------------------------------------------------------*/
void set_quest(struct char_data *ch, qst_rnum rnum)
{
  GET_QUEST(ch) = QST_NUM(rnum);
  GET_QUEST_TIME(ch) = QST_TIME(rnum);
  GET_QUEST_COUNTER(ch) = QST_QUANTITY(rnum);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

void clear_quest(struct char_data *ch)
{
  GET_QUEST(ch) = NOTHING;
  GET_QUEST_TIME(ch) = -1;
  GET_QUEST_COUNTER(ch) = 0;
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

void add_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch) +1);
  for (i=0; i < GET_NUM_QUESTS(ch); i++)
    temp[i] = ch->player_specials->completed_quests[i];

  temp[GET_NUM_QUESTS(ch)] = vnum;
  GET_NUM_QUESTS(ch)++;

 if (ch->player_specials->completed_quests)
    free(ch->player_specials->completed_quests);
  ch->player_specials->completed_quests = temp;
}

void remove_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i, j = 0;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch));
  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->completed_quests[i] != vnum)
      temp[j++] = ch->player_specials->completed_quests[i];

  GET_NUM_QUESTS(ch)--;

  if (ch->player_specials->completed_quests)
    free(ch->player_specials->completed_quests);
  ch->player_specials->completed_quests = temp;
}

void generic_complete_quest(struct char_data *ch)
{
  qst_rnum rnum;
  qst_vnum vnum = GET_QUEST(ch);
  struct obj_data *new_obj;
  int exp = 0;
  int level = 0;

  if (--GET_QUEST_COUNTER(ch) <= 0) {
    rnum = real_quest(vnum);
    if (QST_OBJ(rnum) > 0 && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {

      send_to_char(ch, "@l@CYou cannot complete this quest until you have made some room in your inventory.@n\r\n");
      return;

    }

    send_to_char(ch, "@l@WY O U   H A V E   C O M P L E T E D   A   Q U E S T !@n\r\n");

    GET_QUESTPOINTS(ch) += (QST_POINTS(rnum) * (100 + GET_RP_QP_BONUS(ch)) / 100);
    send_to_char(ch,
          "%s\r\nYou have been awarded %d quest points for your service.\r\n",
          QST_DONE(rnum), QST_POINTS(rnum));
   if (QST_GOLD(rnum)) {
      GET_GOLD(ch) += QST_GOLD(rnum);
      send_to_char(ch,
           "You have been awarded %d gold coins for your service.\r\n",
            QST_GOLD(rnum));
    }
    if (QST_EXP(rnum)) {

      level = QST_MINLEVEL(rnum);
      exp = (level_exp(level + 1, GET_REAL_RACE(ch)) - level_exp(level, 
             GET_REAL_RACE(ch))) * QST_EXP(rnum) / 100;

      gain_exp(ch, exp);
    }
    if (QST_OBJ(rnum) > 0) {
      

      if (real_object(QST_OBJ(rnum))) {
		if ((new_obj = create_obj()) != NULL) {
          new_obj = read_object((QST_OBJ(rnum)),VIRTUAL);
          obj_to_char(new_obj, ch);
          if (new_obj != NULL && GET_OBJ_SHORT(new_obj) != NULL)
          send_to_char(ch,
                "@nYou have been presented with %s@n for your service.\r\n",
                GET_OBJ_SHORT(new_obj));
        }
      }
   }
    add_completed_quest(ch, vnum);
    clear_quest(ch);
    if (QST_NEXT(rnum) > 0 && (real_quest(QST_NEXT(rnum)) != NOTHING) &&
         (QST_NEXT(rnum) != vnum) &&
         !is_complete(ch, QST_NEXT(rnum))) {
       rnum = real_quest(QST_NEXT(rnum));
       set_quest(ch, rnum);
       send_to_char(ch,
           "The next stage of your quest awaits:\r\n%s",
           QST_INFO(rnum));
    } 
   } 
  
} 

void autoquest_trigger_check(struct char_data *ch, struct char_data *vict,
		             struct obj_data *object, int type)
{
  struct char_data *i, *master = NULL;
  struct follow_type *f = NULL;
  qst_rnum rnum;
  int found = TRUE;
  struct char_data *tch = ch;

  if (!ch || ch == NULL)
    return;
  if ((!vict || vict == NULL) && type != AQ_ROOM_CLEAR && type != AQ_MOB_SAVE)
    return;
  if (IS_NPC(ch))
    return;


  for (ch = world[IN_ROOM(tch)].people; ch; ch = ch->next_in_room) {  

    if (!is_player_grouped(ch, tch) && ch != tch)
      continue;

  if (!ch || ch == NULL)
      continue;
  if ((!vict || vict == NULL) && type != AQ_ROOM_CLEAR && type != AQ_MOB_SAVE)
      continue;
  if (IS_NPC(ch))
      continue;
  if (GET_QUEST(ch) == NOTHING && type != AQ_MOB_KILL)  /* No current quest, skip this */
      continue;
  if (GET_QUEST_TYPE(ch) != type && type != AQ_MOB_KILL)
      continue;
  if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING)
      continue;

  switch (type) {
    case AQ_OBJ_FIND:
      if (QST_TARGET(rnum) == GET_OBJ_VNUM(object))
        generic_complete_quest(ch);
      break;
    case AQ_ROOM_FIND:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number)
        generic_complete_quest(ch);
      break;
    case AQ_MOB_FIND:
      for (i=world[IN_ROOM(ch)].people; i; i = i->next_in_room)
        if (IS_NPC(i))
          if (QST_TARGET(rnum) == GET_MOB_VNUM(i))
            generic_complete_quest(ch);
      break;
    case AQ_MOB_KILL:

      if (ch->master)
        master = ch->master;
      else
        master = ch;

      
      if (!IS_NPC(master) && IS_NPC(vict) && (master != vict)) 
        if ((AFF_FLAGGED(master, AFF_GROUP) && AFF_FLAGGED(ch, AFF_GROUP)) || master == ch) 
          if (GET_QUEST(master) != NOTHING && GET_QUEST_TYPE(master) == type)
            if ((rnum = real_quest(GET_QUEST(master))) != NOTHING)
              if (QST_TARGET(rnum) == GET_MOB_VNUM(vict)) 
                generic_complete_quest(master);

      for (f = master->followers; f; f = f->next) {
        if (!IS_NPC(f->follower) && IS_NPC(vict) && (f->follower != vict)) 
          if ((AFF_FLAGGED(f->follower, AFF_GROUP) && AFF_FLAGGED(ch, AFF_GROUP)) || f->follower == ch) 
            if (GET_QUEST(f->follower) != NOTHING && GET_QUEST_TYPE(f->follower) == type)
              if ((rnum = real_quest(GET_QUEST(f->follower))) != NOTHING)

                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict)) 
                  generic_complete_quest(f->follower);
      }

      break;
   case AQ_MOB_SAVE:
       if (ch == vict)
        found = FALSE;
      for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
         if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET))
            if ((GET_MOB_VNUM(i) != QST_TARGET(rnum)) &&
                !AFF_FLAGGED(i, AFF_CHARM))
              found = FALSE;
      if (found)
        generic_complete_quest(ch);
      break;
    case AQ_OBJ_RETURN:
      if (IS_NPC(vict) && (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)))
        if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum)))
          generic_complete_quest(ch);
      break;
    case AQ_ROOM_CLEAR:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number) {
        for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
          if (i && IS_NPC(i)) {
            found = FALSE;
            break;
          }
        }
        if (found) {
	  generic_complete_quest(ch);
        } else {
        }
      }
      break;
    default:
      log("SYSERR: Invalid quest type passed to autoquest_trigger_check");
      break;
  }
  }
}

void quest_timeout(struct char_data *ch)
{
  if ((GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != (ush_int)-1)) {
   clear_quest(ch);
    send_to_char(ch, "You have run out of time to complete the quest.\r\n");
  }
}

void check_timed_quests(void)
{
  struct char_data *ch;

  for (ch = character_list; ch; ch = ch->next)
    if (ch && !IS_NPC(ch) && (GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != (ush_int)-1))
      if (--GET_QUEST_TIME(ch) == 0)
       quest_timeout(ch);
}

/*--------------------------------------------------------------------------*/
/* Quest Command Helper Functions                                           */
/*--------------------------------------------------------------------------*/

void list_quests(struct char_data *ch, zone_rnum zone, qst_vnum vmin, qst_vnum vmax)
{
  qst_rnum rnum;
  qst_vnum bottom, top;
  int counter = 0;

  if (zone != NOWHERE) {
    bottom = zone_table[zone].bot;
    top    = zone_table[zone].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  /* Print the header for the quest listing. */
  send_to_char (ch,
  "Index VNum    Description                                  Questmaster\r\n"
  "----- ------- -------------------------------------------- -----------\r\n");
  for (rnum = 0; rnum < total_quests ; rnum++)
    if (QST_NUM(rnum) >= bottom && QST_NUM(rnum) <= top)
      send_to_char(ch, "@g%4d@n) [@g%-5d@n] @c%-44.44s@n @y[%5d]@n\r\n",
                   ++counter,
                   QST_NUM(rnum), QST_NAME(rnum),
                   mob_index[QST_MASTER(rnum)].vnum);
  if (!counter)
    send_to_char(ch, "None found.\r\n");
}

void quest_hist(struct char_data *ch)
{
  int i = 0, counter = 0;
  qst_rnum rnum = NOTHING;

  send_to_char(ch, "Quests that you have completed:\r\n"
    "Index Description                                          Questmaster\r\n"
    "----- ---------------------------------------------------- -----------\r\n");
  for (i = 0; i < GET_NUM_QUESTS(ch); i++) {
    if ((rnum = real_quest(ch->player_specials->completed_quests[i])) != NOTHING)
      send_to_char(ch, "@g%4d@n) @c%-52.52s@n @y%s@n\r\n",
	++counter, QST_DESC(rnum), GET_NAME(&mob_proto[QST_MASTER(rnum)]));
    else
      send_to_char(ch,
        "@g%4d@n) @cUnknown Quest (it no longer exists)@n\r\n", ++counter);
  }
  if (!counter)
    send_to_char(ch, "You haven't completed any quests yet.\r\n");
}

void quest_join(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum;
  qst_rnum rnum;
  char buf[MAX_INPUT_LENGTH];

  if (!*argument)
    snprintf(buf, sizeof(buf),
             "%s What quest did you wish to join?", GET_NAME(ch));
  else if (GET_QUEST(ch) != NOTHING)
    snprintf(buf, sizeof(buf),
             "%s But you are already part of a quest!", GET_NAME(ch));
  else if((vnum = find_quest_by_qmnum(ch, qm->nr, atoi(argument))) == NOTHING)
    snprintf(buf, sizeof(buf),
             "%s I don't know of such a quest!", GET_NAME(ch));
  else if ((rnum = real_quest(vnum)) == NOTHING)
    snprintf(buf, sizeof(buf),
             "%s I don't know of such a quest!", GET_NAME(ch));
 else if (is_complete(ch, vnum) && !(IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)))
    snprintf(buf, sizeof(buf),
             "%s You have already completed that quest!", GET_NAME(ch));
  else if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum))) {
    snprintf(buf, sizeof(buf),
            "%s That quest is not available to you yet!\r\nYou first need to complete the quest '%s' from %s.", GET_NAME(ch), 
            QST_NAME(real_quest(QST_PREV(rnum))), 
            QST_MASTER(real_quest(QST_PREV(rnum))) == NOBODY ? "" : GET_NAME(&mob_proto[QST_MASTER(real_quest(QST_PREV(rnum)))]));

  }
  else if ((QST_PREREQ(rnum) != NOTHING) &&
           (real_object(QST_PREREQ(rnum)) != NOTHING) &&
           (get_obj_in_list_num(real_object(QST_PREREQ(rnum)),
				ch->carrying) == NULL))
    snprintf(buf, sizeof(buf),
             "%s You need to have %s first!", GET_NAME(ch),
	     obj_proto[real_object(QST_PREREQ(rnum))].short_description);
  else {
    act("You join the quest.",    TRUE, ch, NULL, NULL, TO_CHAR);
    act("$n has joined a quest.", TRUE, ch, NULL, NULL, TO_ROOM);
    snprintf(buf, sizeof(buf),
             "%s Listen carefully to the instructions.", GET_NAME(ch));
    do_tell(qm, buf, cmd_tell, 0);
    set_quest(ch, rnum);
    send_to_char(ch, QST_INFO(rnum));
    if (QST_TIME(rnum) != -1)
      snprintf(buf, sizeof(buf),
        "%s You have a time limit of %d turn%s to complete the quest.",
        GET_NAME(ch), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
    else
      snprintf(buf, sizeof(buf),
        "%s You can take however long you want to complete the quest.",
	GET_NAME(ch));
  }
  do_tell(qm, buf, cmd_tell, 0);
  save_char(ch);
}

void quest_list(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum;
  qst_rnum rnum;

  if ((vnum = find_quest_by_qmnum(ch, qm->nr, atoi(argument))) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if ((rnum = real_quest(vnum)) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if (QST_INFO(rnum)) {
    send_to_char(ch,"Complete Details on Quest %d @c%s@n:\r\n%s",
                      vnum,
		      QST_DESC(rnum),
		      QST_INFO(rnum));
    if (QST_PREV(rnum) != NOTHING)
      send_to_char(ch, "You have to have completed quest %s first.\r\n",
          QST_NAME(real_quest(QST_PREV(rnum))));
    if (QST_TIME(rnum) != -1)
      send_to_char(ch,
         "There is a time limit of %d turn%s to complete the quest.\r\n",
          QST_TIME(rnum),
          QST_TIME(rnum) == 1 ? "" : "s");
  } else
    send_to_char(ch, "There is no further information on that quest.\r\n");
}

void quest_quit(struct char_data *ch)
{
  qst_rnum rnum;

  if (GET_QUEST(ch) == NOTHING)
    send_to_char(ch, "But you currently aren't on a quest!\r\n");
  else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
    clear_quest(ch);
    send_to_char(ch, "You are now no longer part of the quest.\r\n");
    save_char(ch);
  } else {
    clear_quest(ch);
    if (QST_QUIT(rnum) && (str_cmp(QST_QUIT(rnum), "undefined") != 0))
      send_to_char(ch, "%s", QST_QUIT(rnum));
    else
      send_to_char(ch, "You are now no longer part of the quest.\r\n");
   if (QST_PENALTY(rnum)) {
      GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
      send_to_char(ch,
        "You have lost %d quest points for your cowardice.\r\n",
        QST_PENALTY(rnum));
    }
    save_char(ch);
  }
}

void quest_progress(struct char_data *ch)
{
  qst_rnum rnum;

  if (GET_QUEST(ch) == NOTHING)
    send_to_char(ch, "But you currently aren't on a quest!\r\n");
  else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
    clear_quest(ch);
    send_to_char(ch, "Your quest seems to no longer exist.\r\n");
 } else {
    send_to_char(ch, "You are on the following quest:\r\n%s\r\n%s",
		     QST_DESC(rnum), QST_INFO(rnum));
    if (QST_QUANTITY(rnum) > 1)
      send_to_char(ch,
          "You still have to achieve %d out of %d goals for the quest.\r\n",
	  GET_QUEST_COUNTER(ch), QST_QUANTITY(rnum));
    if (GET_QUEST_TIME(ch) != (ush_int)-1)
      send_to_char(ch,
          "You have %d turn%s remaining to complete the quest.\r\n",
	  GET_QUEST_TIME(ch),
	  GET_QUEST_TIME(ch) == 1 ? "" : "s");
  }
}

void quest_show(struct char_data *ch, mob_rnum qm)
{
  qst_rnum rnum;
  int counter = 0;

  send_to_char(ch,
  "The following quests are available:\r\n"
  "Index Description                                          ( Vnum) Done? Repeatable?\r\n"
  "----- ---------------------------------------------------- ------- ----- -----------\r\n");
  for (rnum = 0; rnum < total_quests; rnum++)
    if (qm == QST_MASTER(rnum))
      send_to_char(ch, "@g%4d@n) @c%-52.52s@n @y(%5d)@n @y(%3s) (%s)@n\r\n",
        ++counter, QST_DESC(rnum), QST_NUM(rnum),
        (is_complete(ch, QST_NUM(rnum)) ? "Yes" : "No "),
        (IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) ? "Yes" : "No");
  if (!counter)
    send_to_char(ch, "There are no quests available here at the moment.\r\n");
}

void quest_stat(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  qst_rnum rnum;
  char buf[MAX_STRING_LENGTH];
  char targetname[MAX_STRING_LENGTH];

  if (GET_LEVEL(ch) < ADMLVL_IMMORT)
    send_to_char(ch, "Huh!?!\r\n");
  else if (!*argument)
    send_to_char(ch, "%s\r\n", quest_imm_usage);
  else if ((rnum = real_quest(atoi(argument))) == NOTHING )
    send_to_char(ch, "That quest does not exist.\r\n");
  else {
    sprintbit(QST_FLAGS(rnum), aq_flags, buf, sizeof(buf));
    switch (QST_TYPE(rnum)) {
      case AQ_OBJ_FIND:
      case AQ_OBJ_RETURN:
        snprintf(targetname, sizeof(targetname), "%s",
                 real_object(QST_TARGET(rnum)) == NOTHING ?
                 "An unknown object" :
		 obj_proto[real_object(QST_TARGET(rnum))].short_description);
	break;
      case AQ_ROOM_FIND:
      case AQ_ROOM_CLEAR:
        snprintf(targetname, sizeof(targetname), "%s",
	         real_room(QST_TARGET(rnum)) == NOWHERE ?
                 "An unknown room" :
		 world[real_room(QST_TARGET(rnum))].name);
        break;
      case AQ_MOB_FIND:
      case AQ_MOB_KILL:
      case AQ_MOB_SAVE:
	snprintf(targetname, sizeof(targetname), "%s",
                 real_mobile(QST_TARGET(rnum)) == NOBODY ?
		 "An unknown mobile" :
		 GET_NAME(&mob_proto[real_mobile(QST_TARGET(rnum))]));
	break;
      default:
	snprintf(targetname, sizeof(targetname), "Unknown");
	break;
    }
    send_to_char(ch,
        "VNum  : [@y%5d@n], RNum: [@y%5d@n] -- Questmaster: [@y%5d@n] @y%s@n\r\n"
        "Name  : @y%s@n\r\n"
	"Desc  : @y%s@n\r\n"
	"Accept Message:\r\n@c%s@n"
	"Completion Message:\r\n@c%s@n"
	"Quit Message:\r\n@c%s@n"
	"Type  : @y%s@n\r\n"
        "Target: @y%d@n @y%s@n, Quantity: @y%d@n\r\n"
	"Value : @y%d@n, Penalty: @y%d@n, Min Level: @y%2d@n, Max Level: @y%2d@n\r\n"
	"Flags : @c%s@n\r\n",
    	QST_NUM(rnum), rnum,
	QST_MASTER(rnum) == NOBODY ? -1 : mob_index[QST_MASTER(rnum)].vnum,
	QST_MASTER(rnum) == NOBODY ? "" : GET_NAME(&mob_proto[QST_MASTER(rnum)]),
        QST_NAME(rnum), QST_DESC(rnum),
        QST_INFO(rnum), QST_DONE(rnum),
	(QST_QUIT(rnum) &&
	 (str_cmp(QST_QUIT(rnum), "undefined") != 0)
       	 ? QST_QUIT(rnum) : "Nothing\r\n"),
    	quest_types[QST_TYPE(rnum)],
	QST_TARGET(rnum) == NOBODY ? -1 : QST_TARGET(rnum),
	targetname,
	QST_QUANTITY(rnum),
    	QST_POINTS(rnum), QST_PENALTY(rnum), QST_MINLEVEL(rnum),
	QST_MAXLEVEL(rnum), buf);
    if (QST_PREREQ(rnum) != NOTHING)
      send_to_char(ch, "Preq  : [@y%5d@n] @y%s@n\r\n",
        QST_PREREQ(rnum) == NOTHING ? -1 : QST_PREREQ(rnum),
        QST_PREREQ(rnum) == NOTHING ? "" :
	  real_object(QST_PREREQ(rnum)) == NOTHING ? "an unknown object" :
	      obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    if (QST_TYPE(rnum) == AQ_OBJ_RETURN)
      send_to_char(ch, "Mob   : [@y%5d@n] @y%s@n\r\n",
        QST_RETURNMOB(rnum),
	real_mobile(QST_RETURNMOB(rnum)) == NOBODY ? "an unknown mob" :
           mob_proto[real_mobile(QST_RETURNMOB(rnum))].short_descr);
    if (QST_TIME(rnum) != -1)
      send_to_char(ch, "Limit : There is a time limit of %d turn%s to complete.\r\n",
	  QST_TIME(rnum),
	  QST_TIME(rnum) == 1 ? "" : "s");
    else
      send_to_char(ch, "Limit : There is no time limit on this quest.\r\n");
    send_to_char(ch, "Prior :");
    if (QST_PREV(rnum) == NOTHING)
      send_to_char(ch, " @yNone.@n\r\n");
    else
      send_to_char(ch, " [@y%5d@n] @c%s@n\r\n",
        QST_PREV(rnum), QST_DESC(real_quest(QST_PREV(rnum))));
    send_to_char(ch, "Next  :");
    if (QST_NEXT(rnum) == NOTHING)
      send_to_char(ch, " @yNone.@n\r\n");
    else
      send_to_char(ch, " [@y%5d@n] @c%s@n\r\n",
        QST_NEXT(rnum), QST_DESC(real_quest(QST_NEXT(rnum))));
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Command Processing Function and Questmaster Special                */
/*--------------------------------------------------------------------------*/

ACMD(do_quest)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int  tp;

  two_arguments(argument, arg1, arg2);
  if (!*arg1)
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < ADMLVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
  else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < ADMLVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
  else {
    switch (tp) {
      case SCMD_QUEST_LIST:
      case SCMD_QUEST_JOIN:
        /* list, join should hve been handled by questmaster spec proc */
        send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
        break;
      case SCMD_QUEST_HISTORY:
        quest_hist(ch);
        break;
      case SCMD_QUEST_LEAVE:
        quest_quit(ch);
        break;
      case SCMD_QUEST_PROGRESS:
	quest_progress(ch);
	break;
      case SCMD_QUEST_STATUS:
        if (GET_LEVEL(ch) < ADMLVL_IMMORT)
          send_to_char(ch, "%s\r\n", quest_mort_usage);
        else
          quest_stat(ch, arg2);
        break;
      default: /* Whe should never get here, but... */
        send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < ADMLVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
	break;
    } /* switch on subcmd number */
  }
}

SPECIAL(questmaster)
{
  if (!CMD_IS("quest"))
    return 0;

  qst_rnum rnum;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int  tp;
  struct char_data *qm = (struct char_data *)me;

  /* check that qm mob has quests assigned */
  for (rnum = 0; (rnum < total_quests &&
                  QST_MASTER(rnum) != GET_MOB_RNUM(qm)) ; rnum ++);
  if (rnum >= total_quests)
    return FALSE; /* No quests for this mob */
  else if (QST_FUNC(rnum) && (QST_FUNC(rnum) (ch, me, cmd, argument)))
    return TRUE;  /* The secondary spec proc handled this command */
  else if (CMD_IS("quest")) {
    two_arguments(argument, arg1, arg2);
    if (!*arg1)
      return FALSE;
    else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
      return FALSE;
    else {
      switch (tp) {
      case SCMD_QUEST_LIST:
        if (!*arg2)
          quest_show(ch, GET_MOB_RNUM(qm));
        else
	  quest_list(ch, qm, arg2);
        break;
      case SCMD_QUEST_JOIN:
        quest_join(ch, qm, arg2);
        break;
      default:
	return FALSE; /* fall through to the do_quest command processor */
      } /* switch on subcmd number */
      return TRUE;
    }
  } else {
    return FALSE; /* not a questmaster command */
  }
}


