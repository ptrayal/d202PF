/* ************************************************************************
*   File: Guild.h                                                         *
*  Usage: GuildMaster's: Definition for everything needed by shop.c       *
*                                                                         *
* Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
************************************************************************ */
 
struct guild_data {
  int vnum;                      /* number of the guild */
  int skills[SKILL_TABLE_SIZE];  /*array to keep track of which things we'll train */
  float charge;                  /* charge * skill level = how much we'll charge */
  char *no_such_skill;           /* message when we don't teach that skill */
  char *not_enough_gold;         /* message when the student doesn't have enough gold */
  int minlvl;                    /* Minumum level guildmaster will train */
  int gm;                        /* GM's vnum */
  int with_who[SW_ARRAY_MAX];    /* whom we dislike */
  int open, close;               /*when we will train */
  SPECIAL(*func);                /* secondary spec_proc for the GM */
};

#define GM_NUM(i)  (guild_index[i].vnum)
#define GM_CHARGE(i) (guild_index[i].charge)
#define GM_MINLVL(i) (guild_index[i].minlvl)
#define GM_TRAINER(i) (guild_index[i].gm)
#define GM_WITH_WHO(i) (guild_index[i].with_who)
#define GM_OPEN(i) (guild_index[i].open)
#define GM_CLOSE(i) (guild_index[i].close)
#define GM_FUNC(i) (guild_index[i].func)
#define GM_NO_SKILL(i)	 (guild_index[i].no_such_skill)
#define GM_NO_GOLD(i)	 (guild_index[i].not_enough_gold)
#define GM_COST(i,j,k) (int) (GM_CHARGE(i)*spell_info[j].min_level[(int)GET_CLASS(k)])

#define NOTRAIN_GOOD(i)		(IS_SET_AR((GM_WITH_WHO(i)),TRADE_NOGOOD))
#define NOTRAIN_EVIL(i)		(IS_SET_AR((GM_WITH_WHO(i)),TRADE_NOEVIL))
#define NOTRAIN_NEUTRAL(i)	(IS_SET_AR((GM_WITH_WHO(i)),TRADE_NONEUTRAL))

#define TRAIN_WIZARD(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYWIZARD))
#define TRAIN_CLERIC(i)		(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYCLERIC))
#define TRAIN_ROGUE(i)		(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYROGUE))
#define TRAIN_FIGHTER(i) 	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYFIGHTER))
#define TRAIN_MONK(i)		(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYMONK))
#define TRAIN_PALADIN(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYPALADIN))

#define NOTRAIN_WIZARD(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOWIZARD))
#define NOTRAIN_CLERIC(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOCLERIC))
#define NOTRAIN_ROGUE(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOROGUE))
#define NOTRAIN_FIGHTER(i) 	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOFIGHTER))

#define NOTRAIN_HUMAN(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOHUMAN))
#define NOTRAIN_ELF(i)		(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOELF))
#define NOTRAIN_GNOME(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOGNOME))
#define NOTRAIN_DWARF(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODWARF))
#define NOTRAIN_HALF_ELF(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOHALF_ELF))
#define NOTRAIN_HALFLING(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOHALFLING))  
#define NOTRAIN_DROW_ELF(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODROW_ELF))  
#define NOTRAIN_ANIMAL(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOANIMAL))  
#define NOTRAIN_CONSTRUCT(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOCONSTRUCT))
#define NOTRAIN_DEMON(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODEMON))    
#define NOTRAIN_DRAGON(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODRAGON))    
#define NOTRAIN_FISH(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOFISH))		
#define NOTRAIN_GIANT(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOGIANT))   
#define NOTRAIN_GOBLIN(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOGOBLIN))   
#define NOTRAIN_INSECT(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOINSECT))   
#define NOTRAIN_ORC(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOORC))       
#define NOTRAIN_SNAKE(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOSNAKE))   
#define NOTRAIN_TROLL(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOTROLL))   
#define NOTRAIN_HALF_ORC(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOHALF_ORC))   
#define NOTRAIN_MINOTAUR(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOMINOTAUR))   
#define NOTRAIN_KOBOLD(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOKOBOLD))   
#define NOTRAIN_LIZARDFOLK(i)	(IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOLIZARDFOLK))

/* Functions defined in guild.c */
int print_skills_by_type(struct char_data *ch, char *buf, int maxsz, int sktype);
void levelup_parse(struct descriptor_data *d, char *arg);
