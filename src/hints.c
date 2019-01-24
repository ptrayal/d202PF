#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "feats.h"
#include "spells.h"
#include "utils.h"
#include "assemblies.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"

#define NUM_HINTS 15


char *hints[];

void show_hints(void)
{
    int roll = 0;
    struct char_data *ch, *next_char;

    roll = dice(1, NUM_HINTS) - 1;

    while (roll >= (NUM_HINTS - 2))
        roll = dice(1, NUM_HINTS) - 1;

    roll = MAX(0, roll);

    for (ch = character_list; ch; ch = next_char) 
    {
        next_char = ch->next;

        if (IS_NPC(ch) || !ch->desc)
            continue;

        if (PRF_FLAGGED(ch, PRF_NOHINTS))
            continue;

        send_to_char(ch, hints[roll]);
    }
}

char *hints[NUM_HINTS] = 
{

  "@R[HINT]: @yDifferent spellcasting classes use different commands to cast their spells.\r\n"
  "See HELP CAST for more information.  Also, some classes require that you memorize your\r\n"
  "spells ahead of time.  See HELP MEMORIZE for more information.@nr\n",

  "@R[HINT]: @yYou will gain more experience from grouping.  Monster experience is increased\r\n"
  "for groups, with larger groups receiving more total experience.  This total experience is\r\n"
  "divided amongst all members based on their level.@n\r\n",

  "@R[HINT]: @yTo view information about your character, type SCORE.  To view more detailed\n"
  "information on your character, and to see exact numbers, use STATS.@n\r\n",

  "@R[HINT]: @yWhen you have enough experience to level simply type @Ylevelup@y and follow\r\n"
  "the on-screen prompts.  At the end of the level you can view what you chose and decide\r\n"
  "whether to accept the changes or not.  Once accepted there is no going back short of a\r\n"
  "character respec.\r\n",

  "@R[HINT]: @yIf you are curious what a specific feat does type FEATS COMPLETE DESC and find your\r\n"
  "feat in the list.  To see what you need to get a specific feat type FEATS COMPLETE REQ and find\r\n"
  "your feat in the list.@n\r\n",

  "@R[HINT]: @yIf you are new to DnD 3.5e rules or need some help with the rules or in building\r\n"
  "your character, go to http://www.d20srd.org/ which is an online listing of all Open Gaming\r\n"
  "License DnD 3.5e rules.  Another good site to check is http://nwn2.wikia.com/.  Please note\r\n"
  "that we have made our own variations on certain rules, so always see if there is a help file\r\n"
  "before searching the web.@n\r\n",

  "@R[HINT]: @yIf you get stuck and need some help you can ask another person your question\r\n"
  "by typing NEWBIE or CHAT and then your question.  You can also refer to our online help system.\r\n"
  "For help of feats type HELP FEAT <FEAT NAME>, for skills HELP SKILL <SKILL NAME, spells are\r\n"
  "HELP SPELL <SPELL NAME>, classes are HELP CLASS <CLASS NAME> and races are HELP RACE <RACE NAME>.@n\r\n",

  "@R[HINT]: @yIf you have an idea for a hint, please email your idea to ptrayal@@gmail.com.@n\r\n",

  "@R[HINT]: @yIf you do not wish to see any more hints, type NOHINTS.@n\r\n",

  "@R[HINT]: @yMake sure to type 'backup' often.  This backs up your equipment file, so if\r\n"
  "anything happens, an imm can reimburse all of your gear with a single command.  Make sure\r\n"
  "never to type this when you don't have any equipment, such as right after you die, or your\r\n"
  "backup will be one without any equipment as well.\r\n",

  "@R[HINT]: @yClerics can spontaneously cast either heals or inflict spells.  To enable\r\n"
  "spontaneous casting type spontaneous, and if you are good, every spell you cast will be a cure\r\n"
  "spell of the appropriate level, or if you are evil, every spell you cast will be an inflict\r\n"
  "spell of the appropriate level.  If you are neutral, you have to declare your affinity to\r\n"
  "either positive or negative energy with the setaffinity command.  Once you choose your\r\n"
  "affinity you can never change it yourself, you must have an imm do it.\r\n",

  "@R[HINT]: @yIf you are the tank type, you can toggle on and off your tank status by typing\r\n"
  "@Ytank@y.  While on you will automatically intercept all attacks on any party member, so long\r\n"
  "as no one else has it toggled on as well.\r\n",

  "\r\n",

  "\r\n",

  NULL

};
