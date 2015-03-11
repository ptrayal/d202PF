
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "feats.h"
#include "spells.h"
#include "player_guilds.h"
#include "utils.h"
#include "assemblies.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"

extern const char *alignments[];

void check_guild_level(struct char_data *ch);

char *guild_bonuses[NUM_PLAYER_GUILDS][4];

char *guild_names[] =
{
  "Fighters Guild",
  "Thieves Guild",
  "Archers Guild",
  "Arcanists Guild",
  "Devotionists Guild",
  "Artisans Guild",
  "\n"
};

char *guild_abbrevs[] =
{
  "Fighters",
  "Thieves",
  "Archers",
  "Arcanists",
  "Devotionists",
  "Artisans",
  "\r\n"
};

char *guild_descs[] =
{

"The fighter's guild offers bonuses to melee attack and damage, armor class and hit points.\r\n",

"The thieves guild offers bonuses to damage, dodge ac, certain skills and gold gains.\r\n",

"The archer's guild offers bonuses to ranged attack and damage, movement speed, and certain skills.\r\n",

"The arcanist's guild offers bonuses to spell damage, spell dc, spell resistance and certain skills.\r\n",

"The devotionist's guild offers bonuses to instant healing procs, damage reduction, healing over time, attack rolls, and saving throws.\r\n",

"The artisan's guild offers bonuses to artisan skills, material costs, supply order rewards, and critical crafting chance bonuses.\r\n",

"\r\n"

};


ACMD(do_chooseguild)
{
  int i = 0;
  struct char_data *tch, *next_char;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
    if (!strcasecmp(argument, guild_names[i]))
      break;
  }

  if (i >= NUM_PLAYER_GUILDS+1) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  if (GET_GUILD(ch) != GUILD_UNDEFINED) {
    send_to_char(ch, "You already chose the guild: %s\r\n", guild_names[GET_GUILD(ch)]);
    return;
  }

  if (GET_SUBGUILD(ch) != GUILD_UNDEFINED && i == GUILD_OPERATIVES) {
    send_to_char(ch, "You already have the subguild: %s\r\n", guild_names[GET_SUBGUILD(ch)]);
    return;
  }

  GET_GUILD(ch) = i;

  send_to_char(ch, "You have now become a member of the %s guild! Congratulations!\r\n", guild_names[i]);

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    if (GET_GUILD(tch) != i && GET_ADMLEVEL(tch) == 0)
      continue;

    send_to_char(tch, "@C[%s]: @W%s has just joined the %s!  Welcome them!@n\r\n", guild_abbrevs[i], GET_NAME(ch), guild_names[i]);
  }
  
}

ACMD(do_listguilds)
{

  int i = 0;

  send_to_char(ch, "\r\n");

  send_to_char(ch, "@WGuilds of the Realms@n\r\n"
                   "@W~~~~~~~~~~~~~~~~~~~~@n\r\n");

  for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
    send_to_char(ch, "@W%s@n\r\n  %s", guild_names[i], guild_descs[i]);
  }

  send_to_char(ch, "\r\n");

}

ACMD(do_showguild)
{

  int i = 0;
  int j = 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
    if (!strcmp(argument, guild_names[i]))
      break;
  }

  if (i >= NUM_PLAYER_GUILDS+1) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  send_to_char(ch, "\r\n");

  send_to_char(ch, "@W%s\r\n", guild_names[i]);
  for (j = 0; j < strlen(guild_names[i]); j++)
    send_to_char(ch, "~");
  send_to_char(ch, "@n\r\n");

  send_to_char(ch, "Guild Abbreviation: %s\r\n\r\n", guild_abbrevs[i]);

  send_to_char(ch, "Guild Description:\r\n\r\n%s\r\n", guild_descs[i]);

  send_to_char(ch, "\r\n");

}

ACMD(do_setfalseethos)
{

  char arg[100];
  int ethos = 0;
  int prev = 0;

  if (GET_SUBGUILD(ch) != GUILD_OPERATIVES) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please select either lawful, neutral or chaotic.\r\n");
    return;
  }

  if (!strcmp(arg, "lawful")) {
    send_to_char(ch, "You have set your false ethos to lawful.\r\n");
    ethos = 500;
  }
  if (!strcmp(arg, "neutral")) {
    send_to_char(ch, "You have set your false ethos to neutral.\r\n");
    ethos = 0;
  }
  if (!strcmp(arg, "chaotic")) {
    send_to_char(ch, "You have set your false ethos to chaotic.\r\n");
    ethos = -500;
  }

  if (GET_GUILD(ch) < 0) {
    GET_FALSE_ETHOS(ch) = ethos;
    return;
  }

  prev = GET_ETHOS(ch);
  GET_FALSE_ETHOS(ch) = ethos;

}

ACMD(do_setfalsealign)
{

  char arg[100];
  int align = 0;
  int prev = 0;

  if (GET_SUBGUILD(ch) != GUILD_OPERATIVES) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please select either good, neutral or evil.\r\n");
    return;
  }

  if (!strcmp(arg, "good")) {
    send_to_char(ch, "You have set your false alignment to good.\r\n");
    align = 500;
  }
  if (!strcmp(arg, "neutral")) {
    send_to_char(ch, "You have set your false alignment to neutral.\r\n");
    align = 0;
  }
  if (!strcmp(arg, "evil")) {
    send_to_char(ch, "You have set your false alignment to evil.\r\n");
    align = -500;
  }

  if (GET_GUILD(ch) < 0) {
    GET_FALSE_ALIGNMENT(ch) = align;
    return;
  }

  prev = GET_ALIGNMENT(ch);
  GET_FALSE_ALIGNMENT(ch) = align;

}

ACMD(do_guildchat)
{
  struct char_data *tch, *next_char;

  if (AFF_FLAGGED(ch, AFF_SILENCE) || PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "You move your mouth but no sound comes out.\r\n");
    return;
  }


  if (GET_GUILD(ch) < 0) {
    send_to_char(ch, "You must join a guild before you can guildchat.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (has_curse_word(ch, argument)) {
    return;
  }

  if (!*argument) {
    send_to_char(ch, "Available guild commands are:\r\n"
                     "chooseguild <full guild name>\r\n"
                     "guildchat <message>\r\n"
                     "guildscore\r\n"
                     "guildexp <source type> <amount>\r\n"
                     "showguild <full guild name>\r\n"
                     "\r\n");
    return;
  }

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    if (GET_GUILD(tch) != GET_GUILD(ch) && GET_ADMLEVEL(tch) == 0)
      continue;

    send_to_char(tch, "@C[%s]: @W%s guildchats, '%s'.@n\r\n", guild_abbrevs[GET_GUILD(ch)], GET_NAME(ch), argument);
  }

}

ACMD(do_guildwho)
{
  struct char_data *tch, *next_char;

  int i = 0;

  if (GET_GUILD(ch) < 0) {
    send_to_char(ch, "You must join a guild before you can guildwho.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (*argument) {

    for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
      if (!strcasecmp(argument, guild_names[i]))
        break;
    }
  }
  else i = GET_GUILD(ch);

  if (i >= NUM_PLAYER_GUILDS+1 || GET_ADMLEVEL(ch) == 0) {
    i = GET_GUILD(ch);
  }

  send_to_char(ch, "@WMembers of the %s Online@n\r\n\r\n", guild_names[i]);

  skip_spaces(&argument);

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    if (GET_GUILD(tch) != i)
      continue;

    send_to_char(ch, "%s\r\n", GET_TITLE(tch));
  }

  send_to_char(ch, "\r\n");

}

ACMD(do_subguildchat)
{
  struct char_data *tch, *next_char;

  skip_spaces(&argument);

  if (GET_SUBGUILD(ch) < 0) {
    send_to_char(ch, "You must have a subguild in order to subguildchat.\r\n");
    return;
  }

  if (!*argument) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    if (GET_SUBGUILD(tch) != GET_SUBGUILD(ch) && GET_ADMLEVEL(tch) == 0)
      continue;

    send_to_char(tch, "@C[%s]: @W%s guildchats, '%s'.@n\r\n", guild_abbrevs[GET_SUBGUILD(ch)], GET_NAME(ch), argument);
  }

}

ACMD(do_subguildwho)
{
  struct char_data *tch, *next_char;

  int i = 0;

  if (GET_SUBGUILD(ch) < 0 && GET_ADMLEVEL(ch) == 0) {
    send_to_char(ch, "You must join a subguild before you can subguildwho.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (*argument) {

    for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
      if (!strcasecmp(argument, guild_names[i]))
        break;
    }
  }
  else i = GET_SUBGUILD(ch);

  if (i >= NUM_PLAYER_GUILDS+1 || GET_ADMLEVEL(ch) == 0) {
    i = GET_SUBGUILD(ch);
  }

  if (i < 0)
    return;

  send_to_char(ch, "@WMembers of the %s Online@n\r\n\r\n", guild_names[i]);

  skip_spaces(&argument);

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    if (GET_SUBGUILD(tch) != i)
      continue;

    send_to_char(ch, "%s\r\n", GET_TITLE(tch));
  }

  send_to_char(ch, "\r\n");

}

ACMD(do_playerguilds)
{

  struct char_data *tch, *next_char;

  send_to_char(ch, "@WPlayers Online and their Respective Guilds/Subguilds@n\r\n"
                   "@W~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@nr\n");

  for (tch = character_list; tch; tch = next_char) {
    next_char = tch->next;

    if (IS_NPC(tch) || !tch->desc)
      continue;

    send_to_char(ch, "@W%s: @C%s @D(%s)@n\r\n", GET_NAME(tch), GET_GUILD(tch) >= 0 ? guild_abbrevs[GET_GUILD(tch)] : "None",
                 GET_SUBGUILD(ch) >= 0 ? guild_abbrevs[GET_SUBGUILD(ch)] : "None");

  }

  send_to_char(ch, "\r\n");

}

ACMD(do_guildscore)
{

  if (GET_GUILD(ch) == -1) {
    send_to_char(ch, "You are not a member of any guild.  Please choose a guild using the @Ychooseguild@n command.\r\n");
    return;
  }

  send_to_char(ch, "%s\r\n", GET_TITLE(ch));
  send_to_char(ch, "Guild: %s\r\n", guild_names[GET_GUILD(ch)]);
  send_to_char(ch, "Guild Rank: %d\r\n", GET_GUILD_RANK(ch));
  send_to_char(ch, "Guild Exp: %d\r\n", GET_GUILD_EXP(ch));
  send_to_char(ch, "\r\nYou need %d more guild exp for rank %d.\r\n", 
               MAX(0, guild_level_exp(GET_GUILD_RANK(ch) + 1) - GET_GUILD_EXP(ch)), GET_GUILD_RANK(ch) + 1);
  send_to_char(ch, "\r\nGuild Bonuses:\r\n");
  int j = 1;
  for (j = 1; j <= 4; j++) {
    send_to_char(ch, "%dx %s \r\n", ((GET_GUILD_RANK(ch) +4 - j)/ 4), guild_bonuses[GET_GUILD(ch)][j%4]); 
  }
  
}

ACMD(do_guildexp)
{

  char arg[200], arg2[200];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "This command will allow you to convert certain resources into guild exp.\r\n");
    send_to_char(ch, "The syntax is 'guildexp <conversion type> <amount>'\r\n");
    send_to_char(ch, "available conversion types are 'experience', 'questpoints', 'gold'.\r\n");
    return;
  }
  if (!*arg2) {
    send_to_char(ch, "You need to specify an amount to convert.\r\n");
    return;
  }

  int amount = atoi(arg2);
  int guild_exp = 0;

  if (is_abbrev(arg, "experience")) {
    if (amount < 1 || amount > 100) {
      send_to_char(ch, "You need to specify a percent of your experience to next level, from 1 to 100.\r\n");
      return;
    }
    int percent = (GET_EXP(ch) - level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)) * 100) /
                  (level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch)) -
                   level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)));

    if (percent < amount) {
      send_to_char(ch, "You do not have enough experience to convert that amount.\r\n");
      return;
    }

    GET_EXP(ch) -= (level_exp(GET_CLASS_LEVEL(ch) + 1, GET_REAL_RACE(ch)) -
                   level_exp(GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch))) * amount / 100;


    guild_exp = amount * 10;

    GET_GUILD_EXP(ch) += guild_exp;

    send_to_char(ch, "You have successfully converted %d%% of your experience required for next level into %d guild experience.\r\n"
                     "Your guild experience is now at %d which means you need %d more for guild rank %d.\r\n",
                     amount, guild_exp, GET_GUILD_EXP(ch), guild_level_exp(GET_GUILD_RANK(ch) + 1) -
                     GET_GUILD_EXP(ch), GET_GUILD_RANK(ch) + 1);
    check_guild_level(ch);
    return;
  } else if (is_abbrev(arg, "questpoints")) {

    if (amount < 1) {
      send_to_char(ch, "You must convert at least 1 quest point.\r\n");
      return;
    }

    if (amount > GET_QUESTPOINTS(ch)) {
      send_to_char(ch, "You only have %d quest points to convert to begin with.\r\n", GET_QUESTPOINTS(ch));
      return;
    }

    int level = MAX(GET_CLASS_LEVEL(ch), GET_ARTISAN_LEVEL(ch));

    guild_exp = amount * 10 / MAX(1, (level / 5));

    GET_GUILD_EXP(ch) += guild_exp;

    GET_QUESTPOINTS(ch) -= amount;

    send_to_char(ch, "You have successfully converted %d of your quest points into %d guild experience.\r\n"
                     "Your guild experience is now at %d which means you need %d more for guild rank %d.\r\n",
                     amount, guild_exp, GET_GUILD_EXP(ch), guild_level_exp(GET_GUILD_RANK(ch) + 1) -
                     GET_GUILD_EXP(ch), GET_GUILD_RANK(ch) + 1);
    check_guild_level(ch);
    return;
  } else if (is_abbrev(arg, "gold")) {

    int factor = GET_CLASS_LEVEL(ch) * GET_CLASS_LEVEL(ch) * 2;

    if (amount < factor) {
      send_to_char(ch, "You must convert a minimum of %d gold coins.\r\n", factor);
      return;
    }

    if (GET_GOLD(ch) < amount) {
      send_to_char(ch, "You do not have that much gold on hand.\r\n");
      return;
    }

    guild_exp = amount / factor * 50;

    GET_GUILD_EXP(ch) += guild_exp;

    send_to_char(ch, "You have successfully converted %d of your gold coins into %d guild experience.\r\n"
                     "Your guild experience is now at %d which means you need %d more for guild rank %d.\r\n",
                     amount, guild_exp, GET_GUILD_EXP(ch), guild_level_exp(GET_GUILD_RANK(ch) + 1) -
                     GET_GUILD_EXP(ch), GET_GUILD_RANK(ch) + 1);
    check_guild_level(ch);
    return;    
  } else {
    send_to_char(ch, "This command will allow you to convert certain resources into guild exp.\r\n");
    send_to_char(ch, "The syntax is 'guildexp <conversion type> <amount>'\r\n");
    send_to_char(ch, "available conversion types are 'experience', 'questpoints', 'gold'.\r\n");
    return;
  }
}

char *guild_bonuses[NUM_PLAYER_GUILDS][4] = {
  {
    // Fighter's Guild
    "20% base +8% per rank chance for 0.5 bonus per rank to melee weapon attack rolls",
    "20% base + 8% per rank chance for +0.5 armor bonus to armor class when wearing medium or heavy armor",
    "+1 bonus to athletics and intimidate skills",
    "10% base + 4% per rank chance for +1 bonus per rank to melee weapon damage rolls"
  },
  {
    // Theives Guild
    "3% base plus 3% per rank for +2 bonus per rank to damage rolls when flanking",
    "50% base +5% per rank for +0.5 dodge bonus per rank to armor class",
    "+1% bonus per rank to gold gains",
    "+1 bonus to open lock, stealth, and disable device skills"
  },
  {
    // Archers guild
    "20% base + 8% per rank chance for +0.5 bonus to ranged weapon attack rolls",
    "+1 bonus to perception, stealth and athletics skills",
    "+3\' per rank per round bonus to movement speed when wearing no armor or light or medium armor",
    "10% base + 4% per rank for +1 bonus per rank to ranged weapon damage rolls"
  },
  {
    // Arcanists Guild
    "15% base +1% per rank chance for a +1 bonus per rank per die to all spell damage",
    "15% base +1% per rank chance for a +1 bonus per rank to spell dc",
    "15% base +1% per rank chance for a +1 bonus per rank to spell penetration",
    "+1 bonus to lore, spellcraft and concentration skills"
  },
  {
    // Devotionist Guild
    "5% base +1% per rank chance per round for healing 5 hit points per rank instantaneously",
    "5% base +1% per rank chance per hit to reduce damage done by 1 per rank",
    "+1 out of combat hit point regen rate & +10 out of combat stamina point regen",
    "10% base +2% per rank chance to gain +1 per rank to saving throws"
  },
  {
    // Artisan Guild
    "+1 bonus per rank to all artisan skills of your chosen profession",
    "5% per rank chance to have the material cost of the item being made reduced by 1",
    "5% per rank bonus to all supply order rewards",
    "1% per rank bonus to critical crafting success chance"
  }
};

void check_guild_level(struct char_data *ch)
{

  if (GET_GUILD(ch) == -1) {
    return;
  }

  if (GET_GUILD_EXP(ch) > guild_level_exp(GET_GUILD_RANK(ch)+1)) {
    GET_GUILD_RANK(ch)++;
    send_to_char(ch, "You have gained a new rank in your guild!\r\n"
                     "You have gained the following bonus for your new rank:\r\n"
                     "(Please note bonuses are cumulative, though percent chances are not unless otherwise stated)\r\n"
                     "@Y%s.@n\r\n"
                     "Type 'guildbonuses' to view all of your current guild bonuses for your rank and guild.\r\n", 
                     guild_bonuses[GET_GUILD(ch)][GET_GUILD_RANK(ch)%4]);
    return;
  }
}

ACMD(do_guildbonuses)
{

  int i = 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  for (i = 0; i < NUM_PLAYER_GUILDS+1; i++) {
    if (!strcasecmp(argument, guild_names[i]))
      break;
  }

  if (i >= NUM_PLAYER_GUILDS+1) {
    send_to_char(ch, "You have to type out the full guild name exactly as shown in 'listguilds'.\r\n");
    return;
  }

  int j = 0;

  send_to_char(ch, "Below are the bonuses by rank for the %s guild.\r\n"
                   "Please note that this list repeats itself after level 4.\r\n", guild_names[i]);

  for (j = 1; j <= 4; j++) {
    send_to_char(ch, "%d) %s\r\n", j, guild_bonuses[i][j%4]);
  }

  send_to_char(ch, "\r\n");

}
