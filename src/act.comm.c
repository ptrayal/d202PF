/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "mysql/mysql.h"
#include "structs.h"
#include "feats.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "spells.h"
#include "boards.h"
#include "modules.h"

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_intro);
ACMD(do_say);
ACMD(do_say_languages);
ACMD(do_osay);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_respond);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);

extern char *which_desc(struct char_data *ch);
extern int has_intro(struct char_data *ch, struct char_data *target);

char const *languages_dl_aol[] =
{
  "common",
  "thieves cant",
  "druidic",
  "abyssal",
  "elven",
  "gnome",
  "dwarven",
  "celestial",
  "draconic",
  "gullytalk",
  "kenderspeak",
  "goblin",
  "kothian",
  "giant",
  "kobold",
  "solamnic",
  "ergot",
  "istarian",
  "baliforian",
  "kharolisian",
  "nordmaarian",
  "icespeak",
  "barbarian",
  "\n",
  NULL
};

char const *languages_fr[] =
{
  "common",
  "thieves cant",
  "druidic",
  "abyssal",
  "elven",
  "gnome",
  "dwarven",
  "celestial",
  "draconic",
  "aklo",
  "orcish",
  "aquan",
  "halfing",
  "goblin",
  "auran",
  "gnoll",
  "giant",
  "kobold",
  "ignan",
  "infernal",
  "sylvan",
  "terran",
  "aboleth",
  "drow sign language",
  "boggard",
  "sphinx",
  "strix",
  "cyclops",
  "dark folk",
  "grippli",
  "tengu",
  "protean",
  "treant",
  "undercommon",
  "\n",
  NULL
};

void list_languages(struct char_data *ch)
{
    int a = 0, i = 0;

    send_to_char(ch, "Languages Known (red means currently speaking):\r\n[");
    for (i = MIN_LANGUAGES ; i <= CampaignMaxLanguages() ; i++)
    {
        if (GET_SKILL(ch, i) || affected_by_spell(ch, SPELL_TONGUES) || HAS_FEAT(ch, FEAT_TONGUE_OF_THE_SUN_AND_MOON))
        {
            send_to_char(ch, "%s %s%s%s",
                a++ != 0 ? "," : "",
                SPEAKING(ch) == i ? "@r": "@n",
                (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr)[i-MIN_LANGUAGES], "@n");
        }
    }
    send_to_char(ch, "%s ]\r\n", a== 0 ? " None!" : "");
}


ACMD(do_languages)
{
    char arg[MAX_STRING_LENGTH] = {'\0'};
    int i = 0, found = false;

    if (CONFIG_ENABLE_LANGUAGES)
    {
        one_argument(argument, arg);

        if (!*arg)
            list_languages(ch);
        else
        {
            for (i = MIN_LANGUAGES; i <= CampaignMaxLanguages(); i++)
            {
                if (((search_block(arg, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr), false) == i - MIN_LANGUAGES) && (GET_SKILL(ch, i) ||
                        affected_by_spell(ch, SPELL_TONGUES) || HAS_FEAT(ch, FEAT_TONGUE_OF_THE_SUN_AND_MOON))))
                {
                    SPEAKING(ch) = i;
                    send_to_char(ch, "You now speak %s.\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr)[i - MIN_LANGUAGES]);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                send_to_char(ch, "You do not know of any such language.\r\n");
                return;
            }
        }
    }
    else
    {
        send_to_char(ch, "But everyone already understands everyone else!\r\n");
        return;
    }

}

void garble_text(char *string, int known, int lang)
{
  char letters[50] = "";
  int i = 0;

  switch (lang) {
  case SKILL_LANG_DWARVEN:
    strcpy (letters, "hprstwxyz");
    break;
  case SKILL_LANG_ELVEN:
    strcpy (letters, "aefhilnopstu");
    break;
  default:
    strcpy (letters, "aehiopstuwxyz");
    break;
  }

  for (i = 0; i < (int) strlen(string); ++i) {
    if (isalpha(string[i]) && (!known)) {
      string[i] = letters[rand_number(0, (int) strlen(letters) - 1)];
    }
  }

}

ACMD(do_say)
{
  if (AFF_FLAGGED(ch, AFF_SILENCE) || PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL ||
      race_list[GET_RACE(ch)].family == RACE_TYPE_PLANT)) {
      send_to_char(ch, "You cannot speak in this form.\r\n");
      return;
  }

  if (has_curse_word(ch, argument)) {
    return;
  }

  do_say_languages(ch, argument, 0, 0);

}

ACMD(do_say_languages)
{
  char nametext[100]={'\0'};
  char logmsg[MAX_STRING_LENGTH]={'\0'};

  skip_spaces(&argument);

  if(!*argument) 
  {
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  } 
  else 
  {
    char ibuf[MAX_INPUT_LENGTH]={'\0'};
    char obuf[MAX_INPUT_LENGTH]={'\0'};
    char nbuf[MAX_INPUT_LENGTH]={'\0'};
    char verb[10]={'\0'};
    struct char_data *tch;

    strcpy(nbuf, argument);     /* for intro purposes */

    if (argument[strlen(argument)-1] == '!') {
      strcpy(verb, "exclaim");
    } else if(argument[strlen(argument)-1] == '?') {
      strcpy(verb, "ask");
    } else {
      strcpy(verb, "say");
    }

    strcpy(ibuf, argument);     /* preserve original text */

    if (!IS_NPC(ch)) {
      sprintf(nametext, "%s", GET_NAME(ch));
      garble_text(ibuf, GET_SKILL(ch, SPEAKING(ch)) ? 1 : 100, SPEAKING(ch));
    } else  {
      garble_text(ibuf, 1, MIN_LANGUAGES);
    }

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if (tch != ch && AWAKE(tch) && tch->desc) {
        strcpy(obuf, ibuf);     /* preserve the first garble */
        if (!IS_NPC(ch) && !IS_NPC(tch) && !GET_SKILL(tch, SPEAKING(ch)) && !affected_by_spell(tch, SPELL_TONGUES)
            && !affected_by_spell(tch, SPELL_COMPREHEND_LANGUAGES) && !HAS_FEAT(tch, FEAT_TONGUE_OF_THE_SUN_AND_MOON)) {
          garble_text(obuf, GET_SKILL(tch, SPEAKING(ch)) ? 1 : 0, SPEAKING(ch));
        } else {
          garble_text(ibuf, 1, MIN_LANGUAGES);
        }
        if (!IS_NPC(ch) && !IS_NPC(tch) && !GET_SKILL(tch, SPEAKING(ch)) && !affected_by_spell(tch, SPELL_TONGUES)
            && !affected_by_spell(tch, SPELL_COMPREHEND_LANGUAGES) && !HAS_FEAT(tch, FEAT_TONGUE_OF_THE_SUN_AND_MOON)) {
					char *tmpdesc = NULL;
          send_to_char(tch, "%s %ss, in an unfamiliar tongue, '%s'\r\n", CAN_SEE(tch, ch) ? (has_intro(tch, ch) ? GET_NAME(ch) : (tmpdesc = which_desc(ch))) : "Someone", verb, obuf);
					free(tmpdesc);
        } else {
          send_to_char(tch, "@W%s %ss in %s '%s@W'@n\r\n", CAN_SEE(tch, ch) ? (has_intro(tch, ch) ? GET_NAME(ch) : which_desc(ch)) : "Someone", verb,
	(CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr)[SPEAKING(ch)-MIN_LANGUAGES], obuf);
          GET_RP_EXP(ch) += strlen(obuf);
          if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TAVERN)) {
            GET_RP_EXP(ch) += strlen(obuf);
          }
          sprintf(logmsg, "RPLOG: %s %ss in %s '%s'\r\n", GET_NAME(ch), verb,
                  (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr)[SPEAKING(ch)-MIN_LANGUAGES], obuf);
          log("%s", logmsg);
        }
      }
    }
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(ch, "%s", CONFIG_OK);
    } else {
      delete_doubledollar(argument);
      send_to_char(ch, "@WYou %s in %s, '%s@W'@n\r\n", verb, (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? languages_dl_aol : languages_fr)[SPEAKING(ch)-MIN_LANGUAGES], argument);
    }

    /* trigger check */
    speech_mtrigger(ch, argument);
    speech_wtrigger(ch, argument);
  }
}

ACMD(do_osay)
{

  if (has_curse_word(ch, argument)) {
    return;
  }

  skip_spaces(&argument);

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say out of character?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 12]={'\0'};
    char verb[10]={'\0'};

    if (argument[strlen(argument)-1] == '!')
      strcpy(verb, "exclaim");
    else if(argument[strlen(argument)-1] == '?')
      strcpy(verb, "ask");
    else
      strcpy(verb, "say");

    snprintf(buf, sizeof(buf), "$n %ss out of character, '%s@n'", verb, argument);
    act(buf, false, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else {
      delete_doubledollar(argument);
      send_to_char(ch, "You %s out of character, '%s@n'\r\n", verb, argument);
    }
  /* trigger check */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
  }
}

ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }



  if (has_curse_word(ch, argument)) {
    return;
  }

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
    char buf[MAX_STRING_LENGTH]={'\0'};

    if (ch->master)
      k = ch->master;
    else
      k = ch;

    snprintf(buf, sizeof(buf), "%s-- $n: '%s@n'", subcmd == SCMD_RP_GSAY ? "@W" : "@G", argument);


    int x = 0;
    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch)) {
      act(buf, false, ch, 0, k, TO_VICT | TO_SLEEP);
      x++;
    }
    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch)) {
	act(buf, false, ch, 0, f->follower, TO_VICT | TO_SLEEP);
        x++;
      }

  if (x > 0 && subcmd == SCMD_RP_GSAY)
    GET_RP_EXP(ch) += strlen(argument);


    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
	act(buf, false, ch, 0,0, TO_CHAR | TO_SLEEP);
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH]={'\0'};

  snprintf(buf, sizeof(buf), "@M$n tells you, '%s@M'@n", arg);
  act(buf, false, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
    snprintf(buf, sizeof(buf), "@MYou tell $N, '%s@M'@n", arg);
    act(buf, false, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }

  struct char_data *tch = NULL;

  if (!IS_NPC(vict) && !IS_NPC(ch)) {
    if (GET_LAST_TELL(vict) == NOBODY)
      GET_LAST_TELL(vict) = GET_IDNUM(ch);

    for (tch = character_list; tch; tch = tch->next) {
      if (GET_LAST_TELL(vict) == GET_IDNUM(tch)) {
        if (GET_ADMLEVEL(tch) == 0) {
          GET_LAST_TELL(vict) = GET_IDNUM(ch);
        }
        break;
      }
    }
  }
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && GET_ADMLEVEL(ch) == 0)
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && GET_ADMLEVEL(ch) == 0))
    act("$E can't hear you.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (true);

  return (false);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH]={'\0'}, buf2[MAX_INPUT_LENGTH]={'\0'};

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (!(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)) || (GET_ADMLEVEL(vict) > 0 && GET_ADMLEVEL(ch) == 0))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (has_curse_word(ch, buf2)) {
    return;
  }
  else if (is_tell_ok(ch, vict)) {
    perform_tell(ch, vict, buf2);
    GET_LAST_TELL(ch) = GET_IDNUM(vict);
  }
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }

  if (has_curse_word(ch, argument)) {
    return;
  }

  skip_spaces(&argument);


  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char(ch, "That player is no longer playing.\r\n");
    else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}


ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH]={'\0'}, buf2[MAX_INPUT_LENGTH]={'\0'};
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL ||
      race_list[GET_RACE(ch)].family == RACE_TYPE_PLANT)) {
      send_to_char(ch, "You cannot speak in this form.\r\n");
      return;
    }
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL ||
      race_list[GET_RACE(ch)].family == RACE_TYPE_PLANT)) {
      send_to_char(ch, "You cannot speak in this form.\r\n");
      return;
    }
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }


  if (has_curse_word(ch, argument)) {
    return;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    char buf1[MAX_STRING_LENGTH]={'\0'};

    snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
    act(buf1, false, ch, 0, vict, TO_VICT);

		char *tmpdesc = NULL;
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, has_intro(ch, vict) ? GET_NAME(vict) : (tmpdesc = which_desc(vict)), buf2);
		free(tmpdesc);
    act(action_others, false, ch, 0, vict, TO_NOTVICT);
  }
}


/*
 * buf1, buf2 = MAX_OBJECT_NAME_LENGTH
 *	(if it existed)
 */
ACMD(do_write)
{
  extern struct index_data *obj_index;
  struct obj_data *paper, *pen = NULL, *obj;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH]={'\0'}, buf2[MAX_STRING_LENGTH]={'\0'};

  /* before we do anything, lets see if there's a board involved. */
  for (obj = ch->carrying; obj;obj=obj->next_content) {
    if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
      break;
    }
  }
  
  if(!obj) {
    for (obj = world[IN_ROOM(ch)].contents; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	break;
      }
    }
  }
  
  if(obj) {                /* then there IS a board! */
    write_board_message(GET_OBJ_VNUM(obj),ch,argument);
    act ("$n begins to write a note on $p.", true, ch, obj, 0, TO_ROOM);
    return;
  }
  
  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_WIELD2))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_WIELD2);
    else
      pen = GET_EQ(ch, WEAR_WIELD2);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", false, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", false, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;
 
    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->action_description);
    }
 
    /* we can write - hooray! */
    act("$n begins to jot down a note.", true, ch, 0, 0, TO_ROOM);
    SET_BIT_AR(GET_OBJ_EXTRA(paper),ITEM_UNIQUE_SAVE);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH]={'\0'}, arg[MAX_INPUT_LENGTH]={'\0'};


  if (has_curse_word(ch, argument)) {
    return;
  }

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH]={'\0'};

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (ADM_FLAGGED(ch, ADM_TELLALL)) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, false, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL) {
      act(buf, false, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", CONFIG_OK);
      else
	act(buf, false, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24]={'\0'};
  char buf1[MAX_INPUT_LENGTH]={'\0'};

  if (subcmd == SCMD_SHOUT && AFF_FLAGGED(ch, AFF_WILD_SHAPE) &&
      (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL ||
      race_list[GET_RACE(ch)].family == RACE_TYPE_PLANT)) {
      send_to_char(ch, "You cannot speak in this form.\r\n");
      return;
  }

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = 
  {
    {"You cannot holler!!\r\n",
      "holler",
      "",
      "@G"},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
      "@G"},

    {"You cannot chat!!\r\n",
      "chat",
      "You aren't even on the channel!\r\n",
      "@C"},

    {"You cannot speak on the auction channel!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
      "@M"},

    {"You cannot speak on the newbie channel!\r\n",
      "newbie",
      "You aren't even on the channel!\r\n",
      "@Y"}
  };

  /* to keep pets, etc from being ordered to shout */
  if (!ch->desc)
    return;

  if (AFF_FLAGGED(ch, AFF_SILENCE) && subcmd == SCMD_SHOUT) {
	  send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
	  act("$n moves $s mouth but no sound comes out", TRUE, ch, 0, ch, TO_NOTVICT);
	  return;  	
  }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && GET_ADMLEVEL(ch) == 0) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }
/*
  if (subcmd == SCMD_GOSSIP && *argument == '*') { 
    subcmd = SCMD_GEMOTE; 
  } 

  if (subcmd == SCMD_GEMOTE) { 
    ACMD(do_gmote); 
    if (*argument == '*') 
      do_gmote(ch, argument + 1, 0, 1); 
    else 
      do_gmote(ch, argument, 0, 1); 

    return; 
  } 
*/

  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }

  /* make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }

  if (has_curse_word(ch, argument)) {
    return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* Make gossip * social and gemote social behave the same */
/*  if (subcmd == SCMD_GOSSIP && *argument == '*') {
    subcmd = SCMD_GEMOTE;
  }
*/

  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < CONFIG_HOLLER_MOVE_COST) {
      send_to_char(ch, "You're too exhausted to holler.\r\n");
      return;
    } else
      GET_MOVE(ch) -= CONFIG_HOLLER_MOVE_COST;
  }
  /* set up the color on code */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* first, set up strings to be given to the communicator */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else
    send_to_char(ch, "%sYou %s, '%s%s'@n\r\n", color_on, com_msgs[subcmd][1], argument, color_on);

/*  snprintf(buf1, sizeof(buf1), "%s%s %ss, '%s%s'@n", color_on, GET_NAME(ch), com_msgs[subcmd][1], argument, color_on);*/

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (IS_PLAYING(i) && i != ch->desc && i->character &&
       (IS_NPC(i->character) || !PRF_FLAGGED(i->character, channels[subcmd])) &&
/*       (IS_NPC(i->character) || !PLR_FLAGGED(i->character, PLR_WRITING)) &&*/
	!(ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && GET_ADMLEVEL(i->character) == 0) ) {

      snprintf(buf1, sizeof(buf1), "%s%s %ss, '%s%s'@n", color_on, CAN_SEE(i->character, ch) ? GET_NAME(ch) : "Someone", com_msgs[subcmd][1], 
               argument, color_on);
      act(buf1, false, ch, 0, i->character, TO_VICT | TO_SLEEP);
    }
  }
  char logmsg[MAX_STRING_LENGTH]={'\0'};

  for (i = descriptor_list; i; i = i->next) {
    if (IS_PLAYING(i) && i != ch->desc && i->character &&
       !PRF_FLAGGED(i->character, channels[subcmd]) &&
       !PLR_FLAGGED(i->character, PLR_WRITING) &&
       !IS_NPC(i->character) && GET_ADMLEVEL(i->character) == 0 && subcmd == SCMD_SHOUT &&
       (!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && GET_ADMLEVEL(i->character) == 0)) {

      GET_RP_EXP(ch) += strlen(argument) / 5;
      
      sprintf(logmsg, "RPLOG: %s %ss '%s'\r\n", GET_NAME(ch), "narrate", argument);
      log("%s", logmsg);
      break;
    }
  }
  
}


ACMD(do_qcomm)
{
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }

    if (has_curse_word(ch, argument)) {
      return;
    }

  skip_spaces(&argument);

  if (AFF_FLAGGED(ch, AFF_SILENCE) || PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH]={'\0'};
    struct descriptor_data *i;

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else if (subcmd == SCMD_QSAY) {
      snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
      act(buf, false, ch, 0, argument, TO_CHAR);
    } else
      act(argument, false, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
      snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
    else
      strlcpy(buf, argument, sizeof(buf));

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

ACMD(do_respond) 
{
    struct obj_data *obj;
    char number[MAX_STRING_LENGTH] = {'\0'};
    int found = 0;
    int mnum = 0;

    if(IS_NPC(ch))
    {
        send_to_char(ch, "As a mob, you never bothered to learn to read or write.\r\n");
        return;
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
        if(GET_OBJ_TYPE(obj) == ITEM_BOARD)
        {
            found = 1;
            break;
        }
    }
    if(!obj)
    {
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
        {
            if(GET_OBJ_TYPE(obj) == ITEM_BOARD)
            {
                found = 1;
                break;
            }
        }
    }
    if (obj)
    {
        argument = one_argument(argument, number);
        if (!*number)
        {
            send_to_char(ch, "Respond to what?\r\n");
            return;
        }
        if (!isdigit(*number) || (!(mnum = atoi(number))))
        {
            send_to_char(ch, "You must type the number of the message you wish to reply to.\r\n");
            return;
        }
        board_respond(GET_OBJ_VNUM(obj), ch, mnum);
    }

    /* No board in the room? Send generic message -spl */
    if (found == 0)
    {
        send_to_char(ch, "Sorry, you may only reply to messages posted on a board.\r\n");
    }
}

ACMD(do_petition)
{
    byte found = FALSE;
    struct char_data *tch = NULL;
    struct descriptor_data *d = NULL;

    skip_spaces(&argument);

    if (GET_PETITION(ch) > 0 && GET_PETITION(ch) < 50)
    {
        send_to_char(ch, "You may only petition for help once every five minutes.\r\n");
        return;
    }

    GET_PETITION(ch) = 1;

    for (d = descriptor_list; d; d = d->next)
    {
        tch = d->character;
        if (IS_NPC(ch))
            continue;
        if (GET_ADMLEVEL(tch) == 0)
            continue;
        if (GET_INVIS_LEV(tch) > 0)
            continue;
        found = TRUE;
        send_to_char(tch, "@l@Y[PETITION FOR HELP FROM %s]@n : %s\r\n", GET_NAME(ch), *argument ? argument : "");
    }

    if (found)
    {
        send_to_char(ch, "\r\nYour petition has been received.  Please be patient.  If you have not been contacted by\r\n"
                     "a staff member on the matter within five minutes, you may submit another petition.  The\r\n"
                     "fact that you receive this message means that an eligible staff member is online and\r\n"
                     "flagged as willing to help, however they may be afk, and thus take some time to\r\n"
                     "respond.  They will, however, definitely respond eventually.  Once a staff\r\n"
                     "member has contacted you, you will be transferred to a special imm room.  While in this\r\n"
                     "room we ask that you turn off triggers that cause spam to other people in the room.\r\n"
                     "Please do not partake in any actions that you are not willing to drop within a couple\r\n"
                     "of minutes of receiving a tell from a staff member.  Thank you for your patience.\r\n"
                     "\r\n");
    }
    else
    {
        GET_PETITION(ch) = 0;
        send_to_char(ch, "There are currently no eligible staff members available to assist you.  If the matter\r\n"
                     "is urgent, we ask that you try asking other players by using the newbie or chat\r\n"
                     "commands.  Otherwise you may send a mail to the staff member in charge of your\r\n"
                     "question or concern.\r\n"
                     "\r\n"
                     "Please refer to the following list to see who to send the mail to:\r\n"
                     "Dran: Questions or Problems with Mobs, Objects, Rooms, and Zones in General.\r\n"
                     "Gicker: Everything else.\r\n"
                     "\r\n"
                     "The command to send a mail is @Ymail <person's name>@n.  Thank you for your patience.\r\n"
                     "Please try again in 10 or 20 minutes if you still require assistance then.\r\n"
                     "\r\n");
    }
}
