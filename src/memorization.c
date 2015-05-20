/*********************************
*     Memorization of spells     *
*********************************/

/*****************************************************************************+

A linked list takes care of the spells being memorized, then when it is
finished, it is saved using an array ch->player_specials->saved.spellmem[i].

Storage in playerfile:
ch->player_specials->saved.spellmem[i]

macros in utils.h:

GET_SPELLMEM(ch, i) returns the spell which is memorized in that slot.

defined in structs.h:

MAX_MEM is the total number of spell slots which can be held by a player.

void update_mem(void) is called in comm.c every tick to update the time
until a spell is memorized.

In spell_parser.c at the end of ACMD(do_cast), spell slots are checked when
a spell is cast, and the appropriate spell is removed from memory.

The number of available spell slots is determined by level and wisdom,

Expansion options:
Bard - compose
Priest - pray 
    
******************************************************************************/

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: memorization.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "feats.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "feats.h"

/* external functions */
int has_spellbook(struct char_data *ch, int spellnum);
int spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
int spell_in_domain(struct char_data *ch, int spellnum);

void update_mem(struct char_data *ch, bool mem_all);
int findslotnum(struct char_data *ch, int spelllvl);
int find_freeslot(struct char_data *ch, int spelllvl);
int find_memspeed(struct char_data *ch, bool display);

extern struct descriptor_data *descriptor_list;
extern struct char_data *is_playing(char *vict_name);
extern struct char_data *character_list;

int assassin_spells_per_day[21][10];

void memorize_remove(struct char_data * ch, struct memorize_node * mem);
void memorize_remove_c(struct char_data * ch, struct memorize_node * mem);
void memorize_remove_p(struct char_data * ch, struct memorize_node * mem);
void memorize_remove_d(struct char_data * ch, struct memorize_node * mem);
void memorize_remove_r(struct char_data * ch, struct memorize_node * mem);
void memorize_remove_b(struct char_data * ch, struct memorize_node * mem);
void memorize_add(struct char_data * ch, int spellnum, int timer);
void displayslotnum(struct char_data *ch, int class);

void do_mem_display(struct char_data *ch)
{
  int memcursor = 0;
  int class = 0;
  int spellmem = 0;
  int speedfx, count, i, sortpos, len;
  struct memorize_node *mem = NULL;
  char buf[MAX_STRING_LENGTH];

  switch (GET_MEM_TYPE(ch)) {
  case MEM_TYPE_MAGE:
    class = CLASS_WIZARD;
    memcursor = GET_MEMCURSOR(ch);
    break;
  case MEM_TYPE_CLERIC:
    class = CLASS_CLERIC;
    memcursor = GET_MEMCURSOR_C(ch);
    break;
  case MEM_TYPE_PALADIN:
    memcursor = GET_MEMCURSOR_P(ch);
    class = CLASS_PALADIN;
    break;
  case MEM_TYPE_DRUID:
    memcursor = GET_MEMCURSOR_D(ch);
    class = CLASS_DRUID;
    break;
  case MEM_TYPE_RANGER:
    memcursor = GET_MEMCURSOR_R(ch);
    class = CLASS_RANGER;
    break;
  case MEM_TYPE_BARD:
    memcursor = GET_MEMCURSOR_B(ch);
    class = CLASS_BARD;
    break;
  case MEM_TYPE_SORCERER:
    class = CLASS_SORCERER;
    break;
  case MEM_TYPE_FAVORED_SOUL:
    class = CLASS_FAVORED_SOUL;
    break;
  case MEM_TYPE_ASSASSIN:
    class = CLASS_ASSASSIN;
  }

  if (!findslotnum(ch, 1)) {
    snprintf(buf, MAX_STRING_LENGTH, "You do not have any spellcasting ability in the %s class.\r\n", 
             (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? class_names_fr : class_names_dl_aol)[class]);
    send_to_char(ch, buf);
    return;
  }

  len = sprintf(buf, "@cSpells in memory:\r\n@n");
  /* List the memorized spells
   * Using sprintf since it will be a cold day in hell before
   * we overflow the buffer here */
  for(i = 0; i < 10; i++)
  {    
    if((i >= 1) && (findslotnum(ch, i) < 1))
      break;
    len += sprintf(buf+len,
             "@c---@wLevel @R%d @wSpells@c---=============================@c---@w[@R%d @yslots@w]@c---@n\r\n",
             i, findslotnum(ch, i));
    count = 0;
    for(sortpos = 0; sortpos < memcursor; sortpos++)
    {
      if (len >= MAX_STRING_LENGTH - 128) 
      {
	    strcat(buf, "**OVERFLOW**\r\n");
	    break;
      }

      spellmem = 0;

      switch (GET_MEM_TYPE(ch)) {
        case MEM_TYPE_MAGE:
          spellmem = GET_SPELLMEM(ch, sortpos);
          break;   
        case MEM_TYPE_CLERIC:
          spellmem = GET_SPELLMEM_C(ch, sortpos);
          break;   
        case MEM_TYPE_PALADIN:
          spellmem = GET_SPELLMEM_P(ch, sortpos);
          break;   
        case MEM_TYPE_DRUID:
          spellmem = GET_SPELLMEM_D(ch, sortpos);
          break;   
        case MEM_TYPE_RANGER:
          spellmem = GET_SPELLMEM_R(ch, sortpos);
          break;   
        case MEM_TYPE_BARD:
          spellmem = GET_SPELLMEM_B(ch, sortpos);
          break;   
      }

      if ((spellmem != 0) && (spell_info[spellmem].class_level[class] == i))
      {
        count++;
	     len += sprintf(buf+len, "@y%-22.22s@n", 
                spell_info[spellmem].name);         
	     if(count%3 == 0) {
	       strcat(buf, "\r\n");
          len += 2;
        }
      }
    }
    if(count%3 != 0) {
      strcat(buf, "\r\n");
      len += 2;
    }
    len += sprintf(buf+len, "@w(@r%d@y/@r%d@w)@n\r\n", class == CLASS_BARD ? GET_BARD_SPELLS(ch, i) : 
      (class == CLASS_SORCERER ? GET_SORCERER_SPELLS(ch, i) : (class == CLASS_ASSASSIN ? GET_ASSASSIN_SPELLS(ch, i) : 
      (class == CLASS_FAVORED_SOUL ? GET_FAVORED_SOUL_SPELLS(ch, i) : count))), 
      findslotnum(ch, i));
  }		

  /* here, list of spells being memorized and time till */
  /* memorization complete                              */ 
  speedfx = find_memspeed(ch, TRUE);

  len += sprintf(buf+len, "@c----------------------------------------------------------------@n\r\n");
  len += sprintf(buf+len, "@cSpells being memorized:@n\r\n");
  count = 0;

  switch (GET_MEM_TYPE(ch)) {
    case MEM_TYPE_MAGE:
      mem = ch->memorized;
      break;
    case MEM_TYPE_CLERIC:
      mem = ch->memorized_c;
      break;
    case MEM_TYPE_PALADIN:
      mem = ch->memorized_p;
      break;
    case MEM_TYPE_DRUID:
      mem = ch->player_specials->memorized_d;
      break;
    case MEM_TYPE_RANGER:
      mem = ch->player_specials->memorized_r;
      break;
    case MEM_TYPE_BARD:
      mem = ch->player_specials->memorized_b;
      break;
    default:
      mem = ch->memorized;
      break;
  }

  
  for (mem = mem; mem; mem = mem->next) {
    len += sprintf(buf+len, "@y%-20.20s@w(@r%2d rounds@w)@n ",
                   spell_info[mem->spell].name, 
                   (mem->timer/speedfx) + ((mem->timer%speedfx) != 0));
    count++;
	 if (count%2 == 0) {
      strcat(buf, "\r\n");
      len += 2;
    }
  }
  if(count%2 != 0) {
    strcat(buf, "\r\n");
    len += 2;
  }
  if(count == 0)
    len += sprintf(buf + len, "@w(@rNone@w)@n\r\n");

  if (HAS_FEAT(ch, FEAT_EPIC_SPELLCASTING))
      sprintf(buf+len, "Epic Spells: [%d/%d]\r\n", GET_EPIC_SPELLS(ch), get_skill_value(ch, SKILL_KNOWLEDGE) / 10);

  page_string(ch->desc, buf, 1);
}


/*****************************************************************************+

   Memorization time for each spell is equal to the level of the spell + 2
   multiplied by 5 at POS_STANDING.  A level one spell would take 
   (1+2)*5=15 hours(ticks) to memorize when standing.

   POS_SITTING and POS_RESTING simulate the reduction in time by multiplying
   the number subtracted from the MEMTIME each tick by 5.  
   
   A character who has 15 hours(ticks) to memorize a spell standing will see
   this on his display.  When he is sitting, he will have 15 hours in MEMTIME,
   but the display will divide by the value returned in find_memspeed to show
   a value of 15/5 --> 3 hours sitting time.  

   If a tick occurs while sitting, update_mem will subtract 5 hours of 
   "standing time" which is one hour of "sitting time" from the timer.
   
******************************************************************************/
int find_memspeed(struct char_data *ch, bool display)
{
  int speedfx = 0; 

  if (GET_POS(ch) < POS_RESTING || GET_POS(ch) == POS_FIGHTING) {
    if(display)
      return 1;
    return speedfx;
  } else {
    if ((GET_POS(ch) == POS_RESTING) || (GET_POS(ch) == POS_SITTING))
      speedfx = 15;
      speedfx += HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION) * 3;
    if (GET_POS(ch) == POS_STANDING)
      speedfx = 2;
      speedfx += HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION);
    if (GET_COND(ch, DRUNK) > 10)
      speedfx = speedfx - 1;
    if (GET_COND(ch, FULL) == 0)
      speedfx = speedfx - 1;
    if (GET_COND(ch, THIRST) == 0)
      speedfx = speedfx - 1;
    speedfx = MAX(speedfx, 0);
    if(display)
      speedfx = MAX(speedfx, 1);
    return speedfx;
  }
}

/********************************************/
/* called during a tick to count down till  */ 
/* memorized in comm.c.                     */
/********************************************/
void update_mem(struct char_data *ch, bool mem_all)
{
  struct memorize_node *mem, *next_mem;
  struct memorize_node *mem_c, *next_mem_c;
  struct descriptor_data *d;
  struct char_data *i;
  int speedfx = 0;

  for (d = descriptor_list; d; d = d->next) {
    if(ch)
      i = ch;
    else if(d->original)
      i = d->original;
    else if(!(i = d->character))
      continue;
    speedfx = find_memspeed(i, FALSE);
    for (mem = i->memorized; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the mage spell %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM(i, GET_MEMCURSOR(i)) = mem->spell;
        GET_MEMCURSOR(i)++;

        memorize_remove(i, mem);
      }
    }
    for (mem_c = i->memorized_c; mem_c; mem_c = next_mem_c) {
      next_mem_c = mem_c->next;
      if (speedfx < mem_c->timer && !mem_all) {
        mem_c->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the cleric spell %s.\r\n", 
          spell_info[mem_c->spell].name);

        GET_SPELLMEM_C(i, GET_MEMCURSOR_C(i)) = mem_c->spell;
        GET_MEMCURSOR_C(i)++;

        memorize_remove_c(i, mem_c);
      }
    }
    for (mem = i->memorized_p; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the paladin spell %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM_P(i, GET_MEMCURSOR_P(i)) = mem->spell;
        GET_MEMCURSOR_P(i)++;

        memorize_remove_p(i, mem);
      }
    }
    for (mem = i->player_specials->memorized_d; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the druid spell %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM_D(i, GET_MEMCURSOR_D(i)) = mem->spell;
        GET_MEMCURSOR_D(i)++;

        memorize_remove_d(i, mem);
      }
    }
    for (mem = i->player_specials->memorized_r; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the ranger spell %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM_R(i, GET_MEMCURSOR_R(i)) = mem->spell;
        GET_MEMCURSOR_R(i)++;

        memorize_remove_r(i, mem);
      }
    }
    for (mem = i->player_specials->memorized_b; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing the bard spell %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM_B(i, GET_MEMCURSOR_B(i)) = mem->spell;
        GET_MEMCURSOR_B(i)++;

        memorize_remove_b(i, mem);
      }
    }
    if(ch)
      return;
  }
}

/* remove a spell from a character's memorize(in progress) linked list */
void memorize_remove(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->memorized == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->memorized, next);
  free(mem);
}

void memorize_remove_p(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->memorized_p == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->memorized_p, next);
  free(mem);
}

void memorize_remove_d(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->player_specials->memorized_d == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->player_specials->memorized_d, next);
  free(mem);
}

void memorize_remove_r(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->player_specials->memorized_r == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->player_specials->memorized_r, next);
  free(mem);
}

void memorize_remove_b(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->player_specials->memorized_b == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->player_specials->memorized_b, next);
  free(mem);
}

void memorize_remove_c(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->memorized_c == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->memorized_c, next);
  free(mem);
}

/* add a spell to a character's memorize(in progress) linked list */
void memorize_add(struct char_data * ch, int spellnum, int timer)
{
  struct memorize_node * mem;

  CREATE(mem, struct memorize_node, 1);
  mem->timer = timer;
  mem->spell = spellnum;
  switch (GET_MEM_TYPE(ch)) {
    case MEM_TYPE_MAGE:
      mem->next = ch->memorized;
      ch->memorized = mem;
      break;
    case MEM_TYPE_CLERIC:
      mem->next = ch->memorized_c;
      ch->memorized_c = mem;
      break;
    case MEM_TYPE_PALADIN:
      mem->next = ch->memorized_p;
      ch->memorized_p = mem;
      break;
    case MEM_TYPE_DRUID:
      mem->next = ch->player_specials->memorized_d;
      ch->player_specials->memorized_d = mem;
      break;
    case MEM_TYPE_RANGER:
      mem->next = ch->player_specials->memorized_r;
      ch->player_specials->memorized_r = mem;
      break;
    case MEM_TYPE_BARD:
      mem->next = ch->player_specials->memorized_b;
      ch->player_specials->memorized_b = mem;
      break;
  }
}

/********************************************/
/*  type is forget, memorize, or stop       */
/*  message 0 for no message, 1 for message */
/********************************************/

void do_mem_spell(struct char_data *ch, char *arg, int type, int message)
{
  char *s;
  int class = 0;
  int spellnum, i;
  struct memorize_node *mem, *next;
  struct obj_data *obj;
  bool found = FALSE, domain = FALSE;

  if (GET_MEM_TYPE(ch) == MEM_TYPE_MAGE) {
    class = CLASS_WIZARD;
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC) {
    class = CLASS_CLERIC;
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN) {
    class = CLASS_PALADIN;
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID) {
    class = CLASS_DRUID;
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
    class = CLASS_RANGER;
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD) {
    class = CLASS_BARD;
  }

  for (i = 0; i < strlen(arg); i++)
    if (arg[i] == '-')
      arg[i] = ' ';

  if (message == 1) {
    s = strtok(arg, "\0");
    if (s == NULL) {
      if (type == SCMD_MEMORIZE)
        send_to_char(ch, "Memorize what spell?!?\r\n");
      if (type == SCMD_STOP)
        send_to_char(ch, "Stop memorizing what spell?!?\r\n");
      if (type == SCMD_FORGET)
        send_to_char(ch, "Forget what spell?!?\r\n");
      return;
    }
    spellnum = find_skill_num(s, SKTYPE_SPELL);
  } else
    spellnum = atoi(arg);

  switch(type) {

/************* SCMD_MEMORIZE *************/

  case SCMD_MEMORIZE:
  if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
    send_to_char(ch, "Memorize what?!?\r\n");
    return;
  }

  if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC || GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN ||
      GET_MEM_TYPE(ch) == MEM_TYPE_DRUID || GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
      if (spell_in_domain(ch, spellnum)) {
        domain = TRUE;
      }
    
    if (!domain) {
      send_to_char(ch, "You are not granted that spell by your Diety!\r\n");
      return;
    }
  }
  else {

  /* check for spellbook stuff */

    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
      if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
        if (spell_in_book(obj, spellnum)) {
          found = TRUE;
          break;
        }
        continue;
      }
      if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
        if (spell_in_scroll(obj, spellnum) && spell_info[spellnum].class_level[CLASS_WIZARD] != 99) {
          found = TRUE;
          send_to_char(ch, "The @gmagical energy@n of the scroll leaves the paper and enters your @rmind@n!\r\n");
          send_to_char(ch, "With the @gmagical energy@n transfered from the scroll, the scroll withers to dust!\r\n");
          obj_from_char(obj);
          break;
        }
        continue;
      }

    }

    if (!found) {
      send_to_char(ch, "You don't seem to have %s in your spellbook.\r\n", spell_info[spellnum].name);
      return;
    }
  }


  if ((GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC || GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN ||
       GET_MEM_TYPE(ch) == MEM_TYPE_DRUID || GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) && (AFF_FLAGGED(ch, AFF_NO_DEITY) || !(GET_DEITY(ch) + 1)))  {
    send_to_char(ch, "You cannot memorize any new spells until you have received the blessing of a deity\r\n");
    return;
  }

  //if (!IS_ARCANE(ch) && GET_SKILL(ch, spellnum) <= 0) {
  //  send_to_char(ch, "You are unfamiliar with that spell.\r\n");
  //  return;
  //}


  /*if spell is practiced and there is an open spell slot*/
  if (find_freeslot(ch, spell_info[spellnum].class_level[class]) >= 1) {	  
    memorize_add(ch, spellnum, ((spell_info[spellnum].class_level[class] + 2) * 50));	
    if (message == 1) {
      switch(GET_MEM_TYPE(ch)) {
        case MEM_TYPE_MAGE:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "mage",
                   spell_info[spellnum].name);
          break;
        case MEM_TYPE_CLERIC:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "cleric",
                   spell_info[spellnum].name);
          break;
        case MEM_TYPE_PALADIN:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "paladin", 
                   spell_info[spellnum].name);
          break;
        case MEM_TYPE_DRUID:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "druid",
                   spell_info[spellnum].name);
          break;
        case MEM_TYPE_RANGER:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "ranger", 
                   spell_info[spellnum].name);
          break;
        case MEM_TYPE_BARD:
          send_to_char(ch, "You start memorizing the %s spell %s.\r\n", "bard", 
                   spell_info[spellnum].name);
          break;

      }
      return;
    }
  } else {
    if (message)  {
      switch(GET_MEM_TYPE(ch)) {
        case MEM_TYPE_MAGE:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "mage");
          break;
        case MEM_TYPE_CLERIC:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "cleric");
          break;
        case MEM_TYPE_PALADIN:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "paladin");
          break;
        case MEM_TYPE_DRUID:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "druid");
          break;
        case MEM_TYPE_RANGER:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "ranger");
          break;
        case MEM_TYPE_BARD:
          send_to_char(ch, "All of your level %d %s spell slots are currently filled\r\n", 
                   spell_info[spellnum].class_level[class], "bard");
          break;

      }
    }
    else {
      /* if automem toggle is on */
      switch(GET_MEM_TYPE(ch)) {
        case MEM_TYPE_MAGE:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "mage");
          break;
        case MEM_TYPE_CLERIC:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "cleric");
          break;
        case MEM_TYPE_PALADIN:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "paladin");
          break;
        case MEM_TYPE_DRUID:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "druid");
          break;
        case MEM_TYPE_RANGER:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "ranger");
          break;
        case MEM_TYPE_BARD:
          send_to_char(ch, "You cannot auto-rememorize because all of your level %d %s spell slots are currently filled.\r\n", 
                   spell_info[spellnum].class_level[class], "bard");
          break;

      }
    }
  }
  break;

/************* SCMD_STOP *************/

  case SCMD_STOP:
    if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
      send_to_char(ch, "Stop memorizing what?!?\r\n");
      return;
    }

    if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC) {
      for (mem = ch->memorized_c; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the cleric spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove_c(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID) {
      for (mem = ch->player_specials->memorized_d; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the druid spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove_d(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
      for (mem = ch->player_specials->memorized_r; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the ranger spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove_r(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD) {
      for (mem = ch->player_specials->memorized_b; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the bard spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove_b(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN) {
      for (mem = ch->memorized_p; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the paladin spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove_p(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    else {
      for (mem = ch->memorized; mem; mem = next) {
        if (mem->spell == spellnum) {
          send_to_char(ch, "You stop memorizing the mage spell %s.\r\n", spell_info[spellnum].name);
          memorize_remove(ch, mem);

          return;
        }
        next = mem->next;
      }
    }

    switch (GET_MEM_TYPE(ch)) {
      case MEM_TYPE_MAGE:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "mage",
                 spell_info[spellnum].name);
        break;
      case MEM_TYPE_CLERIC:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "cleric",
                 spell_info[spellnum].name);
        break;
      case MEM_TYPE_PALADIN:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "paladin",
                 spell_info[spellnum].name);
        break;
      case MEM_TYPE_RANGER:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "ranger",
                 spell_info[spellnum].name);
        break;
      case MEM_TYPE_DRUID:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "druid",
                 spell_info[spellnum].name);
        break;
      case MEM_TYPE_BARD:
        send_to_char(ch, "The %s spell %s is not being memorized.\r\n", "bard",
                 spell_info[spellnum].name);
        break;
    }
    break;

/************* SCMD_FORGET *************/

  case SCMD_FORGET:
    if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
      send_to_char(ch, "Forget what?!?\r\n");
      return;
    }
    if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC) {
      for (i = 0; i < GET_MEMCURSOR_C(ch); i++) {
        if(GET_SPELLMEM_C(ch, i) == spellnum) {
          GET_MEMCURSOR_C(ch)--;
          GET_SPELLMEM_C(ch, i) = GET_SPELLMEM_C(ch, GET_MEMCURSOR_C(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the cleric spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID) {
      for (i = 0; i < GET_MEMCURSOR_D(ch); i++) {
        if(GET_SPELLMEM_D(ch, i) == spellnum) {
          GET_MEMCURSOR_D(ch)--;
          GET_SPELLMEM_D(ch, i) = GET_SPELLMEM_D(ch, GET_MEMCURSOR_D(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the druid spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
      for (i = 0; i < GET_MEMCURSOR_R(ch); i++) {
        if(GET_SPELLMEM_R(ch, i) == spellnum) {
          GET_MEMCURSOR_R(ch)--;
          GET_SPELLMEM_R(ch, i) = GET_SPELLMEM_R(ch, GET_MEMCURSOR_R(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the ranger spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD) {
      for (i = 0; i < GET_MEMCURSOR_B(ch); i++) {
        if(GET_SPELLMEM_B(ch, i) == spellnum) {
          GET_MEMCURSOR_B(ch)--;
          GET_SPELLMEM_B(ch, i) = GET_SPELLMEM_B(ch, GET_MEMCURSOR_B(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the bard spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    else if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN) {
      for (i = 0; i < GET_MEMCURSOR_P(ch); i++) {
        if(GET_SPELLMEM_P(ch, i) == spellnum) {
          GET_MEMCURSOR_P(ch)--;
          GET_SPELLMEM_P(ch, i) = GET_SPELLMEM_P(ch, GET_MEMCURSOR_P(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the paladin spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    else {
      for (i = 0; i < GET_MEMCURSOR(ch); i++) {
        if(GET_SPELLMEM(ch, i) == spellnum) {
          GET_MEMCURSOR(ch)--;
          GET_SPELLMEM(ch, i) = GET_SPELLMEM(ch, GET_MEMCURSOR(ch));
  	  if (message) 
  	    send_to_char(ch, "You forget the mage spell, %s.\r\n", spell_info[spellnum].name);
          return;
        }
      }
    }
    send_to_char(ch, "%s is not memorized.\r\n", spell_info[spellnum].name);
    break;

/***********************************/

  }
}

/* returns the number of slots memorized if single is 0 
   returns 1 at the first occurance of the spell if memorized
   and single is 1 (true). 
*/
int is_memorized(struct char_data *ch, int spellnum, int single)
{
  int memcheck = 0, i;

  if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC) {
    for (i = 0; i < GET_MEMCURSOR_C(ch); i++){
      if (GET_SPELLMEM_C(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID) {
    for (i = 0; i < GET_MEMCURSOR_D(ch); i++){
      if (GET_SPELLMEM_D(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
    for (i = 0; i < GET_MEMCURSOR_R(ch); i++){
      if (GET_SPELLMEM_R(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }
  if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD) {
    for (i = 0; i < GET_MEMCURSOR_B(ch); i++){
      if (GET_SPELLMEM_B(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }
  else if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN) {
    for (i = 0; i < GET_MEMCURSOR_P(ch); i++){
      if (GET_SPELLMEM_P(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }
  else {
    for (i = 0; i < GET_MEMCURSOR(ch); i++){
      if (GET_SPELLMEM(ch, i) == spellnum){
        memcheck++;
        if(single)
          return 1;
      }
    }
  }

  return memcheck;
}

/**************************************************/
/* returns 1 if slot of spelllvl is open 0 if not */
/**************************************************/

int find_freeslot(struct char_data *ch, int spelllvl)
{
  struct memorize_node *mem, *next;
  int i, memcheck = 0;

  if (GET_MEM_TYPE(ch) == MEM_TYPE_CLERIC) {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR_C(ch); i++) {
      if (spell_info[GET_SPELLMEM_C(ch, i)].class_level[CLASS_CLERIC] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->memorized_c; mem; mem = next) {
      if (spell_info[mem->spell].class_level[CLASS_CLERIC] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }

  else if (GET_MEM_TYPE(ch) == MEM_TYPE_PALADIN) {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR_P(ch); i++) {
      if (spell_info[GET_SPELLMEM_P(ch, i)].class_level[CLASS_PALADIN] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->memorized_p; mem; mem = next) {

      if (spell_info[mem->spell].class_level[CLASS_PALADIN] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }

  else if (GET_MEM_TYPE(ch) == MEM_TYPE_DRUID) {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR_D(ch); i++) {
      if (spell_info[GET_SPELLMEM_D(ch, i)].class_level[CLASS_DRUID] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->player_specials->memorized_d; mem; mem = next) {

      if (spell_info[mem->spell].class_level[CLASS_DRUID] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }

  else if (GET_MEM_TYPE(ch) == MEM_TYPE_RANGER) {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR_R(ch); i++) {
      if (spell_info[GET_SPELLMEM_R(ch, i)].class_level[CLASS_RANGER] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->player_specials->memorized_r; mem; mem = next) {

      if (spell_info[mem->spell].class_level[CLASS_RANGER] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }

  else if (GET_MEM_TYPE(ch) == MEM_TYPE_BARD) {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR_B(ch); i++) {
      if (spell_info[GET_SPELLMEM_B(ch, i)].class_level[CLASS_BARD] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->player_specials->memorized_b; mem; mem = next) {

      if (spell_info[mem->spell].class_level[CLASS_BARD] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }

  else {
    /* checked the memorized array */
    for (i = 0;i < GET_MEMCURSOR(ch); i++) {
      if (spell_info[GET_SPELLMEM(ch, i)].class_level[CLASS_WIZARD] == spelllvl) {
        memcheck++;
      }
    }

    /* check the memorize linked list */
    for (mem = ch->memorized; mem; mem = next) {
      if (spell_info[mem->spell].class_level[CLASS_WIZARD] == spelllvl) {  
        memcheck++;
      }
      next = mem->next;
    }
  }


  if (memcheck < findslotnum(ch, spelllvl)) {
    return (1);
  } else {
    return 0;
  }
}

ACMD(do_memorize)
{
  char arg[MAX_INPUT_LENGTH];
  const char *classname;

  if(IS_NPC(ch))
    return;

  argument = one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "Please type <mem|stop|forget> <classname> <spell>\r\n");
    return;
  }

  if (is_abbrev(arg, "assassin")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;
    classname =  "assassin";
  } else if (is_abbrev(arg, "bard")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    classname = "bard";
  } else if (is_abbrev(arg, "cleric")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    classname = "cleric";
  } else if (is_abbrev(arg, "druid")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    classname = "druid";
  } else if (is_abbrev(arg, "favored-soul")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    classname = "favored soul";
  } else if (is_abbrev(arg, "mage") || is_abbrev(arg, "wizard")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    classname = "mage";
  } else if (is_abbrev(arg, "paladin")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
    classname = "paladin";
  } else if (is_abbrev(arg, "ranger")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
    classname = "ranger";
  } else if (is_abbrev(arg, "sorcerer")) {
    GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    classname = "sorcerer";
  } else {
    send_to_char(ch, "That is not a proper spellcasting class.\r\n");
    return;
  }

  one_argument(argument, arg);
  switch (subcmd) {
  case SCMD_MEMORIZE:
    if (!*arg)
      do_mem_display(ch);
    else
      do_mem_spell(ch, arg, SCMD_MEMORIZE, 1);
    break;
  case SCMD_STOP:
    if(!*arg) 
      send_to_char(ch, "Stop memorizing what %s spell?!?\r\n", classname);
    else
      do_mem_spell(ch, arg, SCMD_STOP, 1);
    break;
  case SCMD_FORGET:
    if(!*arg) {
      send_to_char(ch, "Forget what %s spell?\r\n", classname);
    } else
      do_mem_spell(ch, arg, SCMD_FORGET, 1);
    break;
  case SCMD_WHEN_SLOT:
    displayslotnum(ch, GET_CLASS(ch));
    break;
  default:
    log("SYSERR: Unknown subcmd %d passed to do_memorize (%s)", subcmd, __FILE__);
    break;
  }
}

/********************************************/
/*          Spell Levels                    */
/*  0   1   2   3   4   5   6   7   8   9   */
/********************************************/
  
          /* Wizard */
int spell_lvlmax_table[NUM_CLASSES][21][10] = {
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  2, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 4,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2,  1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 4,  3,  3,  2, -1, -1, -1, -1, -1, -1 }, 
  { 4,  4,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 4,  4,  3,  3,  2, -1, -1, -1, -1, -1 },
  { 4,  4,  4,  3,  2,  1, -1, -1, -1, -1 },
  { 4,  4,  4,  3,  3,  2, -1, -1, -1, -1 }, /* lvl 10 */
  { 4,  4,  4,  4,  3,  2,  1, -1, -1, -1 }, 
  { 4,  4,  4,  4,  3,  3,  2, -1, -1, -1 },
  { 4,  4,  4,  4,  4,  3,  2,  1, -1, -1 },
  { 4,  4,  4,  4,  4,  3,  3,  2, -1, -1 },
  { 4,  4,  4,  4,  4,  4,  3,  2,  1, -1 }, /* lvl 15 */
  { 4,  4,  4,  4,  4,  4,  3,  3,  2, -1 },
  { 4,  4,  4,  4,  4,  4,  4,  3,  2,  1 },
  { 4,  4,  4,  4,  4,  4,  4,  3,  3,  2 },
  { 4,  4,  4,  4,  4,  4,  4,  4,  3,  3 },
  { 4,  4,  4,  4,  4,  4,  4,  4,  4,  4 }}, /* lvl 20 */

          /* Cleric */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  2, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 4,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  4,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  4,  3,  2, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 5,  4,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  5,  4,  3,  2, -1, -1, -1, -1, -1 },
  { 6,  5,  4,  4,  3, -1, -1, -1, -1, -1 },
  { 6,  5,  5,  4,  3,  2, -1, -1, -1, -1 },
  { 6,  5,  5,  4,  4,  3, -1, -1, -1, -1 }, /* lvl 10 */
  { 6,  6,  5,  5,  4,  3,  2, -1, -1, -1 },
  { 6,  6,  5,  5,  4,  4,  3, -1, -1, -1 },
  { 6,  6,  6,  5,  5,  4,  3,  2, -1, -1 },
  { 6,  6,  6,  5,  5,  4,  4,  3, -1, -1 },
  { 6,  6,  6,  6,  5,  5,  4,  3,  2, -1 }, /* lvl 15 */
  { 6,  6,  6,  6,  5,  5,  4,  4,  3, -1 },
  { 6,  6,  6,  6,  6,  5,  5,  4,  3,  2 },
  { 6,  6,  6,  6,  6,  5,  5,  4,  4,  3 },
  { 6,  6,  6,  6,  6,  6,  5,  5,  4,  4 },
  { 6,  6,  6,  6,  6,  6,  5,  5,  5,  5 }}, /* lvl 20 */

          /* Rogue */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Fighter */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Monk */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Paladin */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  0, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1,  1,  1,  0, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1,  0, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  0, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1,  2,  2,  1,  1, -1, -1, -1, -1, -1 },
  {-1,  2,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  2, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Barbarian */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Bard  */
 {{ 0,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2,  0, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2,  1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 3,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  0, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  { 3,  3,  3,  2, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  0, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2, -1, -1, -1, -1, -1 }, /* lvl 15 */
  { 3,  3,  3,  3,  2,  0, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2,  1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2,  1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  3,  2, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  3,  2, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Ranger */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1,  1,  1,  0, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1, -1, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  0, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1,  2,  2,  1,  1, -1, -1, -1, -1, -1 },
  {-1,  2,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  2, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Druid */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 4,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  3,  2,  1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 5,  3,  3,  2, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  4,  2, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  3,  2,  1, -1, -1, -1, -1 },
  { 6,  4,  4,  3,  3,  2, -1, -1, -1, -1 }, /* lvl 10 */
  { 6,  5,  4,  4,  3,  2,  1, -1, -1, -1 },
  { 6,  5,  4,  4,  3,  3,  2, -1, -1, -1 },
  { 6,  5,  5,  4,  4,  3,  2,  1, -1, -1 },
  { 6,  5,  5,  4,  4,  3,  3,  2, -1, -1 },
  { 6,  5,  5,  5,  4,  4,  3,  2,  1, -1 }, /* lvl 15 */
  { 6,  5,  5,  5,  4,  4,  3,  3,  2, -1 },
  { 6,  5,  5,  5,  5,  4,  4,  3,  2,  1 },
  { 6,  5,  5,  5,  5,  4,  4,  3,  3,  2 },
  { 6,  5,  5,  5,  5,  5,  4,  4,  3,  3 },
  { 6,  5,  5,  5,  5,  5,  4,  4,  4,  4 }}, /* lvl 20 */

          /* Crown */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Sword */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Rose */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Lily */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Skull */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Thorn */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* High Sorcery */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Duelist */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Gladiator */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Mystic */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 5,  3, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 6,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  4, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 6,  6,  5,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  4, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  5,  3, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  6,  4, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  6,  5,  3, -1, -1, -1, -1 }, /* lvl 10 */
  { 6,  6,  6,  6,  6,  4, -11, -1, -1, -1 },
  { 6,  6,  6,  6,  5,  5,  3, -1, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  4, -1, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  5,  3, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  4, -1, -1 }, /* lvl 15 */
  { 6,  6,  6,  6,  6,  6,  6,  5,  3, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  4, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  5,  3 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  6,  4 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  6,  6 }}, /* lvl 20 */

          /* Sorcerer */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 5,  3, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 6,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  4, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 6,  6,  5,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  4, -1, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  5,  3, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  6,  4, -1, -1, -1, -1, -1 },
  { 6,  6,  6,  6,  5,  3, -1, -1, -1, -1 }, /* lvl 10 */
  { 6,  6,  6,  6,  6,  4, -11, -1, -1, -1 },
  { 6,  6,  6,  6,  5,  5,  3, -1, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  4, -1, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  5,  3, -1, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  4, -1, -1 }, /* lvl 15 */
  { 6,  6,  6,  6,  6,  6,  6,  5,  3, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  4, -1 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  5,  3 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  6,  4 },
  { 6,  6,  6,  6,  6,  6,  6,  6,  6,  6 }}, /* lvl 20 */

          /* Noble */ 
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Sword */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */



};

int spell_lvlmax(int whichclass, int chlevel, int slevel)
{
  if (chlevel < 1)
    return -1;
  chlevel = MIN(20, chlevel);
  slevel = MAX(0, MIN(9, slevel));

  if (whichclass == CLASS_ASSASSIN)
    return assassin_spells_per_day[chlevel][slevel];

  if (whichclass == CLASS_FAVORED_SOUL)
    whichclass = CLASS_SORCERER;

  return spell_lvlmax_table[whichclass][chlevel][slevel];
}

/***********************************************************************/

/********************************************/
/* bonus spells based on class attribute    */
/********************************************/

int spell_bonus(int attr, int lvl) {
  if (lvl < 1 || attr < 12)
    return 0;
  lvl = ((ability_mod_value(attr) - lvl) / 4) + 1;
  return MAX(lvl, 0);
}

/*****************************************************************/
/* Returns the number of spell slots for the character and spell */
/* level requested                                               */
/*****************************************************************/

int findslotnum(struct char_data *ch, int spelllvl)
{
  int spell_mod = -1, stat_mod = 0, slevel, class;

  if (IS_NPC(ch)) {
    /* This should be set elsewhere. Is it?
    if (GET_CLASS(ch) == CLASS_WIZARD)
      GET_MEM_TYPE(ch) = MEM_TYPE_MAGE;
    if (GET_CLASS(ch) == CLASS_CLERIC)
      GET_MEM_TYPE(ch) = MEM_TYPE_CLERIC;
    if (GET_CLASS(ch) == CLASS_PALADIN)
      GET_MEM_TYPE(ch) = MEM_TYPE_PALADIN;
    if (GET_CLASS(ch) == CLASS_DRUID)
      GET_MEM_TYPE(ch) = MEM_TYPE_DRUID;
    if (GET_CLASS(ch) == CLASS_RANGER)
      GET_MEM_TYPE(ch) = MEM_TYPE_RANGER;
    if (GET_CLASS(ch) == CLASS_BARD)
      GET_MEM_TYPE(ch) = MEM_TYPE_BARD;
    if (GET_CLASS(ch) == CLASS_SORCERER)
      GET_MEM_TYPE(ch) = MEM_TYPE_SORCERER;
    if (GET_CLASS(ch) == CLASS_FAVORED_SOUL)
      GET_MEM_TYPE(ch) = MEM_TYPE_FAVORED_SOUL;
    if (GET_CLASS(ch) == CLASS_ASSASSIN)
      GET_MEM_TYPE(ch) = MEM_TYPE_ASSASSIN;*/
    slevel = MAX(20, GET_LEVEL(ch));
    return spell_lvlmax(GET_CLASS(ch),slevel,spelllvl);
  }

  switch (GET_MEM_TYPE(ch)) {
  case MEM_TYPE_CLERIC: class = CLASS_CLERIC; break;
  case MEM_TYPE_PALADIN: class = CLASS_PALADIN; break;
  case MEM_TYPE_DRUID: class = CLASS_DRUID; break;
  case MEM_TYPE_RANGER: class = CLASS_RANGER; break;
  case MEM_TYPE_BARD: class = CLASS_BARD; break;
  case MEM_TYPE_MAGE: class = CLASS_WIZARD; break;
  case MEM_TYPE_ASSASSIN: class = CLASS_ASSASSIN; break;
  case MEM_TYPE_FAVORED_SOUL: class = CLASS_FAVORED_SOUL; break;
  default: class = CLASS_SORCERER; break;
  }

  slevel = GET_CLASS_RANKS(ch, class);    	
  slevel += ch->player_specials->bonus_levels[class];
  slevel = MIN(slevel, 30);

  if (GET_LEVEL(ch) > 0)
    slevel = MAX(slevel, 1);

  if (GET_CLASS_RANKS(ch, class))
    spell_mod = spell_lvlmax(class, slevel, spelllvl);

  /* check to see if they have any slots for that level and type */
  if (spell_mod == -1)
    return 0;

  /* check ability for bonus spell slots (depends on class) */
  if (GET_CLASS_RANKS(ch, class))
    switch (GET_MEM_TYPE(ch)) {
    case MEM_TYPE_ASSASSIN:
    case MEM_TYPE_MAGE:
      stat_mod = spell_bonus(GET_INT(ch), spelllvl);
      break;
    case MEM_TYPE_CLERIC:
    case MEM_TYPE_DRUID:
    case MEM_TYPE_RANGER:
    case MEM_TYPE_PALADIN:
      stat_mod = spell_bonus(GET_WIS(ch), spelllvl);
      break;
    case MEM_TYPE_BARD:
    case MEM_TYPE_SORCERER:
    case MEM_TYPE_FAVORED_SOUL:
      stat_mod = spell_bonus(GET_CHA(ch) + (GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE) * 2 / 3), spelllvl);
      break;
    }

  /* spell level modifying EQ */
  spell_mod += GET_SPELL_LEVEL(ch, spelllvl) + stat_mod;

  /* max of 10 slots per spell lvl */  
  return MAX(0, MIN(10, spell_mod)); 
}

void displayslotnum(struct char_data *ch, int class)
{
  char buf[MAX_INPUT_LENGTH];
  char slot_buf[MAX_STRING_LENGTH * 10];
  int i, j, tmp;

  snprintf(slot_buf, sizeof(slot_buf), "test\r\n");

  for(class = 0; class < NUM_CLASSES; class++) {
    snprintf(buf, sizeof(buf), "\r\n/* %s */\r\n\r\n", (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? pc_class_types_fr : pc_class_types_dl_aol)[class]);
    strcat(slot_buf, buf);
    for(i = 0; i < LVL_EPICSTART; i++) {
      strcat(slot_buf, "  { ");
      for(j = 0; j < MAX_SPELL_LEVEL; j++) {
        tmp = spell_lvlmax(class, i, j);
        if (tmp > -1)
          snprintf(buf, sizeof(buf), " %d%s ", tmp, (j==(MAX_SPELL_LEVEL - 1)) ? "" : ",");
        else
          snprintf(buf, sizeof(buf), " -%s ", (j==(MAX_SPELL_LEVEL - 1)) ? "" : ",");
        strcat(slot_buf, buf);
      }
      strcat(slot_buf, " },");
      if (!(i % 5)) {
        snprintf(buf, sizeof(buf), " /* lvl %d */", i);
        strcat(slot_buf, buf);
      }
      strcat(slot_buf, "\r\n");
    }
  }
  page_string(ch->desc, slot_buf, 1);
}

ACMD(do_scribe)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char *s, buf[READ_SIZE];
  int i, spellnum;
  struct obj_data *obj;
  int class = CLASS_WIZARD;

  half_chop(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Usually you scribe SOMETHING.\r\n");
    return;
  }

  if (!IS_ARCANE(ch) && GET_ADMLEVEL(ch) <= ADMLVL_IMMORT) {
    send_to_char(ch, "You really aren't qualified to do that...\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_SCRIBE_SCROLL)) {
    send_to_char(ch, "You haven't had the proper instruction yet!\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    return;
  }

  s = strtok(arg2, "\0");
  spellnum = find_skill_num(s, SKTYPE_SPELL);

  if ((spellnum < 1) || (spellnum >= SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
    send_to_char(ch, "Strange, there is no such spell.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
    /* check if spell is already in book */
    if (spell_in_book(obj, spellnum)) {
      send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n", spell_info[spellnum].name);
      return;
    }
    if (!obj->sbinfo) {
      CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
      memset((char *) obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }
    for (i=0; i < SPELLBOOK_SIZE; i++)
      if (obj->sbinfo[i].spellname == 0)
        break;

	if (i == SPELLBOOK_SIZE) {
      send_to_char(ch, "Your spellbook is full!\r\n");
      return;
    }

    if (!is_memorized(ch, spellnum, TRUE)) {
      send_to_char(ch, "You must have the spell committed to memory before you can scribe it!\r\n");
      return;
    }

    obj->sbinfo[i].spellname = spellnum;
    obj->sbinfo[i].pages = MAX(1, spell_info[spellnum].class_level[class] * 2);
    send_to_char(ch, "You scribe the spell '%s' into your spellbook, which takes up %d pages.\r\n", spell_info[spellnum].name, obj->sbinfo[i].pages);
  } else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
    if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) > 0) {
      send_to_char(ch, "The scroll has a spell enscribed on it!\r\n");
      return;
    }
    GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL) = GET_LEVEL(ch);
    GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) = spellnum;
    GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2) = -1;
    GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3) = -1;
    sprintf(buf, "a scroll of '%s'", spell_info[spellnum].name);
    obj->short_description = strdup(buf);
    send_to_char(ch, "You scribe the spell '%s' onto %s.\r\n", spell_info[spellnum].name, obj->short_description);
  } else {
    send_to_char(ch, "But you don't have anything suitable for scribing!\r\n");
    return;
  }

  send_to_char(ch, "The magical energy committed for the spell '%s' has been expended.\r\n", spell_info[spellnum].name);
  sprintf(buf, "%d", spellnum);
  do_mem_spell(ch, buf, SCMD_FORGET, 0);
  if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
  }
}

int bard_spells_known_table[21][10] = {
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  3, -1, -1, -1, -1, -1, -1, -1 }, // lvl 5
  { 6,  4,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  2, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  4,  2, -1, -1, -1, -1, -1 }, // lvl 10
  { 6,  4,  4,  4,  3, -1, -1, -1, -1, -1 }, 
  { 6,  4,  4,  4,  3, -1, -1, -1, -1, -1 },
  { 6,  4,  4,  4,  4,  2, -1, -1, -1, -1 },
  { 6,  4,  4,  4,  4,  3, -1, -1, -1, -1 },
  { 6,  4,  4,  4,  4,  3, -1, -1, -1, -1 }, // lvl 15
  { 6,  5,  4,  4,  4,  4,  2, -1, -1, -1 },
  { 6,  5,  5,  4,  4,  4,  3, -1, -1, -1 },
  { 6,  5,  5,  5,  4,  4,  3, -1, -1, -1 },
  { 6,  5,  5,  5,  5,  4,  3, -1, -1, -1 },
  { 6,  5,  5,  5,  5,  5,  4, -1, -1, -1 } // lvl 20
};

int assassin_spells_known_table[21][10] = {
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  4,  3, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  4,  3,  2, -1, -1, -1, -1, -1, -1 }, // lvl 5
  {-1,  4,  4,  3, -1, -1, -1, -1, -1, -1 },
  {-1,  4,  4,  3,  2, -1, -1, -1, -1, -1 },
  {-1,  4,  4,  4,  3, -1, -1, -1, -1, -1 },
  {-1,  4,  4,  4,  3, -1, -1, -1, -1, -1 },
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, // lvl 10
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, // lvl 15
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 }, 
  {-1,  4,  4,  4,  4, -1, -1, -1, -1, -1 } // lvl 20
};

int assassin_spells_per_day[21][10] = {
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  2,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  3,  1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  3,  2,  0, -1, -1, -1, -1, -1, -1 }, // lvl 5
  {-1,  3,  3,  1, -1, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  2,  0, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  2, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, // lvl 10
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, // lvl 15
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }, 
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 } // lvl 20
};


int sorcerer_spells_known[22][10] = {
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 4,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  4,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 6,  5,  3, -1, -1, -1, -1, -1, -1, -1 }, // lvl 5
  { 7,  5,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 7,  6,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 8,  6,  5,  4,  3, -1, -1, -1, -1, -1 },
  { 8,  6,  5,  4,  3, -1, -1, -1, -1, -1 },
  { 9,  6,  6,  5,  4,  3, -1, -1, -1, -1 }, // lvl 10
  { 9,  6,  6,  5,  4,  3, -1, -1, -1, -1 }, 
  { 9,  6,  6,  6,  5,  4,  3, -1, -1, -1 }, 
  { 9,  6,  6,  6,  5,  4,  3, -1, -1, -1 }, 
  { 9,  6,  6,  6,  6,  5,  4,  3, -1, -1 }, 
  { 9,  6,  6,  6,  6,  5,  4,  3, -1, -1 }, // lvl 15
  { 9,  6,  6,  6,  6,  6,  5,  4,  3, -1 }, 
  { 9,  6,  6,  6,  6,  6,  5,  4,  3, -1 }, 
  { 9,  6,  6,  6,  6,  6,  6,  5,  4,  3 }, 
  { 9,  6,  6,  6,  6,  6,  6,  5,  4,  3 }, 
  { 9,  6,  6,  6,  6,  6,  6,  6,  5,  4 } // lv 20
};

