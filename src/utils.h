/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* external declarations and prototypes **********************************/

extern char *race_types[];
extern char *curse_words[];
extern int essence_vnums[NUM_ESSENCE_TYPES];
extern int craft_pattern_vnums_real[NUM_CRAFT_TYPES];
extern const char *craft_pattern_descs[];
extern const char *companion_types[];
extern const char *apply_text[];
extern long times_harvested[1000000];
extern int class_in_game_fr[NUM_CLASSES];
extern int class_in_game_dl_aol[NUM_CLASSES];
extern const char *craft_names[NUM_CRAFTS];
extern int craft_skills[NUM_CRAFTS];
extern int craft_wears[NUM_CRAFTS];
extern char *deity_strings[100];
extern const char * deity_names[];
extern const char *deity_names_fr[];
extern const char *deity_names_dl_aol[];
extern struct weather_data weather_info;
extern FILE *logfile;
extern int class_ok_available_fr[NUM_CLASSES];
extern int class_ok_available_dl_aol[NUM_CLASSES];
extern int prestige_classes_fr[NUM_CLASSES];
extern int prestige_classes_dl_aol[NUM_CLASSES];
extern char * pc_race_types[NUM_RACES];
extern char *race_names[NUM_RACES];
extern const char *pc_class_types_fr[];
extern const char *pc_class_types_dl_aol[];
extern const char *class_names_fr[];
extern const char *class_names_dl_aol[];
extern char *campaign_names[];
extern char *campaign_abbrevs[];
extern const char *weapon_type[];
extern const char *armor_type[];
extern const char *wield_names[];
extern int class_hit_die_size_fr[NUM_CLASSES];
extern int class_hit_die_size_dl_aol[NUM_CLASSES];
extern struct race_data race_list[NUM_RACES];
extern struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES + 1];
extern struct weapon_table weapon_list[MAX_WEAPON_TYPES + 1];
extern const char *material_names[];
extern const char *AssemyTypes[MAX_ASSM+1];
extern int assembly_skills[MAX_ASSM];
extern char *speeder_locales_dl[][5];
extern char *shuttle_locales_dl[][7];


#define log			basic_mud_log

#define READ_SIZE	256

/* public functions in utils.c */
char * list_bonus_types(void);
char * get_bonus_type(char *arg);
int get_bonus_type_int(char *arg);
char *randomString(int length);
int level_exp(int level, int race);
int guild_level_exp(int level);
int get_synth_bonus(struct char_data *ch);
int is_class_skill(struct char_data *ch, int skillnum);
byte can_use_available_actions(struct char_data *ch, byte action);
void do_attack_of_opportunity(struct char_data *ch, struct char_data *victim, char *type);
int convert_material_vnum(int vnum);
int is_flying(struct char_data *ch);
int get_combat_bonus(struct char_data *ch);
int get_combat_defense(struct char_data *ch);
int has_intro_idnum(struct char_data *ch, int idnum);
int get_feat_value(struct char_data *ch, int featnum);
void fight_output(const char *str, struct char_data *ch, struct char_data *vict, int type);
int can_enter_dungeon(struct char_data *ch, int room);
void send_to_world(char *message);
int is_class_feat(int featnum, int class); // feats.c
void award_rp_points(struct char_data *ch, int points, int add);
double get_artisan_exp(struct char_data *ch);
char *get_attack_text(struct char_data *ch);
int mob_exp_by_level(int level);
void add_innate_timer(struct char_data *ch, int spellnum);
char *get_weapon_dam(struct char_data *ch);
int calculate_max_hit(struct char_data *ch);
int is_player_grouped(struct char_data *target, struct char_data *group);
int calc_spellfail(struct char_data *ch);
long gen_carry_weight(struct char_data *ch);
int has_curse_word(struct char_data *ch, char *arg);
int has_unlocked_race(struct char_data *ch, int race);
int has_unlocked_class(struct char_data *ch, int class);
char *strip_color(char *string);
void gain_artisan_exp(struct char_data *ch, int gain);
int get_highest_group_level(struct char_data *ch);
void gain_gold(struct char_data *ch, int gain, int type);
int get_rp_bonus(struct char_data *ch, int type);
int valid_misc_item_material_type(struct obj_data *obj, int mat);
int determine_misc_item_bonus_type(struct obj_data *obj, struct obj_data *e_one, struct obj_data *e_two, int bonus);
int power_essence_bonus(struct obj_data *essence);
int has_weapon_feat(struct char_data *ch, int i, int j);
int num_charmies(struct char_data *ch);
char *replace_string(char *str, char *orig, char *rep);
int findslotnum(struct char_data *ch, int spelllvl);
int knows_spell(struct char_data *ch, int spellnum);
int is_metal_item(struct obj_data *obj);
int stat_assign_stat(int abil, char *arg, struct char_data *ch);
int get_saving_throw_value(struct char_data *victim, int savetype);
int has_daylight(struct char_data *ch);
int has_light(struct char_data *ch);
char *which_desc(struct char_data *ch);
int matching_craft_materials(int mat_used, int mat_craft);
int valid_craft_material(int mat_used, int craft_type, int mat_craft);
int get_skill_value(struct char_data *ch, int skillnum);
int skill_roll(struct char_data *ch, int skillnum);
int has_mob_follower(struct char_data *ch, int vnum);
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad);
char *strpaste(char *str1, char *str2, char *joiner);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	basic_mud_vlog(const char *format, va_list args);
int	touch(const char *path);
void	mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));
void	log_death_trap(struct char_data *ch);
int	rand_number(int from, int to);
int	dice(int number, int size);
int	min_dice(int number, int size, int min);
size_t	sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t	sprinttype(int type, const char *names[], char *result, size_t reslen);
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
time_t	mud_time_to_secs(struct time_info_data *now);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void    core_dump_real(const char *who, int line);
int	room_is_dark(room_rnum room);
char *rewind_string(int num, char *string);
char *a_or_an(char *string);
int calc_summoner_level(struct char_data *ch, int ch_class);
char *do_lower(char *buf);
char *do_upper(char *buf, bool do_all);

#define core_dump()		core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */
#ifndef str_cmp
int	str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int	strn_cmp(const char *arg1, const char *arg2, int n);
#endif

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* integer utils */

#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);
char *UNCAP(char *txt);


/* Followers */
int	num_followers_charmed(struct char_data *ch);
void	die_follower(struct char_data *ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
void	stop_follower(struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
void	look_at_room(room_rnum target_room, struct char_data *ch, int mode);

/* in act.movement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in act.item.c */
long	max_carry_weight(struct char_data *ch);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
int	ki_gain(struct char_data *ch);
void	advance_level(struct char_data *ch, int whichclass);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);

/* in class.c */
char *  class_desc_str(struct char_data *ch, int howlong, int wantthe);
int     total_skill_levels(struct char_data *ch, int skill);
int     class_ok_general(struct char_data *ch, int whichclass);
sbyte  ability_mod_value(int abil);
sbyte  dex_mod_capped(const struct char_data *ch);
int	saving_throw_lookup(int save_lev, int chclass, int savetype, int level);
int	base_hit(int hit_type, int chclass, int level);
int	highest_skill_value(int level, int type);
int     calc_penalty_exp(struct char_data *ch, int gain);
int	raise_class_only(struct char_data *ch, int cl, int v);

/* in races.c */
int	get_size(struct char_data *ch);
int get_size_bonus(int sz);
int wield_type(int chsize, const struct obj_data *weap);

/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define SCRIPT_VARS_FILE 3
#define NEW_OBJ_FILES   4
#define PLR_FILE        5
#define PET_FILE        6
#define PET_FILE_NEW	7
#define OLD_OBJ_FILES   8
#define MAX_FILES       8
#define ACT_FILE        9
#define BACKUP_OBJ_FILES 10
#define HOUSE_OBJ_FILES 11

/* breadth-first searching */
#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_NO_PATH		(-3)

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR	120
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(30*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(12*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define SEND_TO_Q(string, d) (write_to_output(d, "%s", string))
#define OUTPUT_TO_CHAR(string, ch) (send_to_char(ch, "%s", string))

/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ log("%s: SYSERR: malloc failure", strerror(errno)); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ log("%s: SYSERR: realloc failure", strerror(errno)); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev)\
      if((item) == (head))			\
      {						\
            head = (item)->next;  		\
            if(head) head->prev = NULL;		\
      }						\
      else					\
      {						\
        temp = head;				\
          while(temp && (temp->next != (item)))	\
            temp = temp->next;			\
             if(temp)				\
            {					\
               temp->next = item->next;		\
               if(item->next)			\
                item->next->prev = temp;	\
            }					\
      }						\

/* basic bitvector utils *************************************************/


#define Q_FIELD(x)  ((int) (x) / 32)
#define Q_BIT(x)    (1 << ((x) % 32))

#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = \
                                   (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  if we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  if you really couldn't care less, change this to a '#if 0'.
 */
#if 0
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->act)
#define PLR_FLAGS(ch)	((ch)->act)
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
#define AFF_FLAGS(ch)	((ch)->affected_by)
#define ADM_FLAGS(ch)	((ch)->admflags)
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define SPELL_ROUTINES(spl)	(spell_info[spl].routines)
#define MOB_FEATS(ch)	((ch)->mob_feats)
#define SPELL_DOMAINS(spl) (spell_info[spl].domain)
#define DAMAGE_TYPES(weapon) (weapon_list[GET_OBJ_VAL(weapon, 0)].damageTypes)
#define WEAPON_FLAGS(weapon) (weapon_list[GET_OBJ_VAL(weapon, 0)].weaponFlags)
#define LEVEL_RANGES(zone_num)  (zone_table[zone_num].level_range)

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */
#define IS_NPC(ch)	(IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))
#define ADM_FLAGGED(ch, flag) (IS_SET_AR(ADM_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJAFF_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_PERM(obj), (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_WEAR(obj), (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))
#define IS_OBJ_STAT(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))
#define DOMAIN_FLAGGED(spl, flag) (IS_SET(SPELL_DOMAINS(spl), (flag)))
#define HAS_DAMAGE_TYPE(weapon, type) (IS_SET(DAMAGE_TYPES(weapon), (type)))
#define WEAPON_FLAGGED(weapon, flag) (IS_SET(WEAPON_FLAGS(weapon), (flag)))
#define HAS_LEVEL_RANGE(zone, range) (IS_SET(LEVEL_RANGES(zone), (range)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))
#define ADM_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(ADM_FLAGS(ch), (flag))) & Q_BIT(flag))
#define AFF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(AFF_FLAGS(ch), (flag))) & Q_BIT(flag))

/* new define for quick check */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

/* room utils ************************************************************/


#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)

#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)


/* char utils ************************************************************/

/*  GMB 10/29/99 - rewrite of GET_NAME and GET_NAME_II */
void get_name(struct char_data* ch, char** chname);
void get_name_II(struct char_data* ch, const struct char_data* vi, char** chname);
void get_name_IV(struct char_data* ch, char** chname);
void get_name_III(struct char_data* ch, const struct char_data* vi, char** chname);
void choose_name(struct char_data* ch, char** chname);
void choose_name_II(struct char_data* ch, const struct char_data* vi, char** chname);
void get_pers(struct char_data* ch, const struct char_data* vi, char** chname);
void get_pers_II(struct char_data* ch, const struct char_data* vi, char** chname);

#define GET_NAME_I(xch, xk) { char* xk = (char*) NULL; get_name(xch, &xk);
#define GET_NAME_II(xch, xvi, xk) { char* xk = (char*) NULL; get_name_II(xch, xvi, &xk);
#define GET_NAME_IV(xch, xk) { char* xk = (char*) NULL; get_name(xch, &xk);
#define GET_NAME_III(xch, xvi, xk) { char* xk = (char*) NULL; get_name_II(xch, xvi, &xk);
#define CHOOSE_NAME(xch, xk) { char* xk = (char*) NULL; choose_name(xch, &xk);
#define CHOOSE_NAME_II(xch, xvi, xk) { char* xk = (char*) NULL; choose_name_II(xch, xvi, &xk);
#define GET_PERS_II(xch, xvi, xk) { char* xk = (char*) NULL; get_pers(xch, xvi, &xk);
#define FREE_NAME(xk) if (xk != (char*) NULL) free(xk); xk = (char*) NULL; }

#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_ACCOUNT_NUM(ch) (ch->player_specials->accountNumber)
#define GET_PC_SDESC(ch) (ch->player_specials->short_descr)
#define GET_SDESC(ch)	(ch->short_descr)
#define GET_IRDA_SDESC_1(ch) (ch->player_specials->irda_short_descr_one)
#define GET_IRDA_SDESC_2(ch) (ch->player_specials->irda_short_descr_two)
#define GET_IRDA_DESCRIPTION_1(ch) (ch->player_specials->irda_description_one)
#define GET_IRDA_DESCRIPTION_2(ch) (ch->player_specials->irda_description_two)
#define GET_PLAYER_KEYWORDS(ch) (ch->player_specials->keywords)
#define GET_KEYWORDS(ch) (IS_NPC(ch) ? ch->name : GET_PLAYER_KEYWORDS(ch))
#define GET_IRDA_KEYWORDS_1(ch) (ch->player_specials->irda_keywords_one)
#define GET_IRDA_KEYWORDS_2(ch) (ch->player_specials->irda_keywords_two)
#define GET_PC_NAME(ch)	((ch)->name)
#define GET_NAME(ch)    (IS_NPC(ch) ?  (ch)->short_descr : GET_PC_NAME(ch))
/*(GET_RACE(ch)  == 17 && \
//			 GET_IRDA_SHAPE_STATUS(ch) == 1 ? GET_IRDA_NAME_1(ch) : \
//			 (GET_RACE(ch) == 17 && GET_IRDA_SHAPE_STATUS(ch) == 2 ? \
			 GET_IRDA_NAME_2(ch) : GET_PC_NAME(ch))))
*/
#define GET_IRDA_NAME_1(ch) (ch->player_specials->irda_name_one)
#define GET_IRDA_NAME_2(ch) (ch->player_specials->irda_name_two)
#define GET_TITLE(ch)   ((ch)->title)
#define GET_IRDA_TITLE_1(ch) (ch->player_specials->irda_title_one)
#define GET_IRDA_TITLE_2(ch) (ch->player_specials->irda_title_two)
#define GET_ADMLEVEL(ch)	((ch)->admlevel)

#define GET_LOGIN_MSG(ch)    (ch->player_specials->login_messages)

#define GET_IRDA_SHAPE_STATUS(ch) (ch->player_specials->irda_shape_status)

#define GET_CLASS_LEVEL(ch)	((ch)->level)
#define GET_LEVEL_ADJ(ch)	(race_list[GET_RACE(ch)].level_adjustment)
#define GET_DIVINE_LEVEL(ch) (GET_CLASS_RANKS(ch, 1) + \
			  GET_CLASS_RANKS(ch, 11) + GET_CLASS_RANKS(ch, 12) + \
			  GET_CLASS_RANKS(ch, 14))
#define GET_ARCANE_LEVEL(ch) (GET_CLASS_RANKS(ch, 0) + GET_CLASS_RANKS(ch, 15))

#define GET_POISON_DAMAGE_TYPE(ch) (ch->player_specials->poisonDamageType)
#define GET_POISON_DAMAGE_AMOUNT(ch) (ch->player_specials->poisonDamageAmount)

#define IS_APPROVED(ch)   ((ch)->player_specials->approved)

#define GET_PC_DESCRIPTOR_1(ch) (ch->player_specials->sdesc_descriptor_1)
#define GET_PC_DESCRIPTOR_2(ch) (ch->player_specials->sdesc_descriptor_2)
#define GET_PC_ADJECTIVE_1(ch) (ch->player_specials->sdesc_adjective_1)
#define GET_PC_ADJECTIVE_2(ch) (ch->player_specials->sdesc_adjective_2)

#define GET_LEVEL_STAGE(ch)	((ch)->player_specials->level_stage)
#define GET_HITDICE(ch)		((ch)->race_level)
#define GET_LEVEL(ch)	(MAX(1, GET_CLASS_LEVEL(ch) + GET_LEVEL_ADJ(ch) + GET_HITDICE(ch)))
#define GET_PASSWD(ch)	((ch)->passwd)
#define GET_PFILEPOS(ch)((ch)->pfilepos)

#define GET_TICKS_PASSED(ch) ((ch)->player_specials->ticks_passed)

#define RIDING(ch)	      ((ch)->riding)		/* (DAK) */
#define RIDDEN_BY(ch)	  ((ch)->ridden_by)	/* (DAK) */
#define SET_RIDING_MOB(ch, obj)    ((ch)->riding = obj)
#define SET_RIDDEN_BY_MOB(ch, obj) ((ch)->ridden_by = obj)
#define SET_RIDING_PC(ch, obj)     ((ch)->riding = obj)
#define SET_RIDDEN_BY_PC(ch, obj)  ((ch)->ridden_by = obj)

#define GET_CLASS(ch)   ((ch)->chclass)
#define GET_CLASS_NONEPIC(ch, whichclass) ((ch)->chclasses[whichclass])
#define GET_CLASS_EPIC(ch, whichclass) ((ch)->epicclasses[whichclass])
#define GET_CLASS_RANKS(ch, whichclass) (IS_NPC(ch) ? ((GET_CLASS(ch) == whichclass) ? GET_LEVEL(ch) : 0 ) : \
                                         GET_CLASS_NONEPIC(ch, whichclass) + \
                                         GET_CLASS_EPIC(ch, whichclass))
#define GET_CHOSEN_CLASS(ch)	((ch)->player_specials->chosen_class)
#define GET_LEVEL_STAGE(ch)	((ch)->player_specials->level_stage)

#define GET_CLASS_SPONSOR(ch, cls) (ch->player_specials->class_sponsor[cls])

#define GET_DEITY(ch)   (ch->player_specials->deity)
#define GET_DOMAIN_ONE(ch) (ch->player_specials->domain_one)
#define GET_DOMAIN_TWO(ch) (ch->player_specials->domain_two)
#define HAS_DOMAIN(ch, i) (deity_list[GET_DEITY(ch)].domains[0] == i || \
	                       deity_list[GET_DEITY(ch)].domains[1] == i || \
	                       deity_list[GET_DEITY(ch)].domains[2] == i || \
	                       deity_list[GET_DEITY(ch)].domains[3] == i || \
	                       deity_list[GET_DEITY(ch)].domains[4] == i || \
	                       deity_list[GET_DEITY(ch)].domains[5] == i)
#define GET_REAL_RACE(ch)    ((ch)->race)
#define GET_RACE(ch)    (AFF_FLAGGED(ch, AFF_WILD_SHAPE) ? GET_DISGUISE_RACE(ch) : GET_REAL_RACE(ch))
#define GET_HOME(ch)	((ch)->hometown)
#define GET_HEIGHT(ch)	((ch)->height)
#define GET_WEIGHT(ch)	((ch)->weight)
#define GET_SEX(ch)	((ch)->sex)
#define GET_TLEVEL(ch)	((ch)->player_specials->tlevel)

#define GET_CARRY_STR_MOD(ch) (ch->carry_strength_mod)

#define GET_STR(ch)     ((ch)->aff_abils.str)
/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_MOUNT_VNUM(ch)     (ch->player_specials->mount_vnum)
#define GET_PET_VNUM(ch)       (ch->player_specials->pet_vnum)
#define GET_FAMILIAR_VNUM(ch)  (ch->player_specials->familiar_vnum)
#define GET_COMPANION_VNUM(ch) (ch->player_specials->companion_vnum)

#define GET_EXP(ch)	  ((ch)->exp)
#define GET_RAGE(ch)         (ch->rage)
#define GET_DEFENSIVE_STANCE(ch)  (ch->player_specials->defensive_stance)
#define GET_STRENGTH_OF_HONOR(ch) (ch->strength_of_honor)
#define GET_SMITE_EVIL(ch)	((ch)->player_specials->smite_evil)
#define GET_TURN_UNDEAD(ch)   ((ch)->player_specials->turn_undead)
#define GET_DAMAGE_TAKEN(ch)	((ch)->damage_taken)
#define GET_MOUNTED_ATTACKS_AVOIDED(ch) (ch->player_specials->mounted_attacks_avoided)
#define GET_ROUNDS_RUNNING(ch) (ch->player_specials->rounds_running)
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
#define GET_ARMOR(ch)     ((ch)->armor)
#define GET_HIT(ch)	  ((ch)->hit)
#define GET_MAX_HIT(ch)	  ((ch)->max_hit)
#define GET_HP_BONUS(ch)  ((ch)->hp_bonus)
#define GET_MOVE(ch)	  ((ch)->move)
#define GET_MAX_MOVE(ch)  ((ch)->max_move)
#define GET_MANA(ch)	  ((ch)->mana)
#define GET_MAX_MANA(ch)  ((ch)->max_mana)
#define GET_KI(ch)	  ((ch)->ki)
#define GET_MAX_KI(ch)    ((ch)->max_ki)
#define GET_GOLD(ch)	  ((ch)->gold)
#define GET_ADAMANTINE(ch) ((ch)->player_specials->adamantine)
#define GET_MITHRIL(ch)	   ((ch)->player_specials->mithril)
#define GET_STEEL(ch)	   ((ch)->player_specials->steel)
#define GET_BRONZE(ch)	   ((ch)->player_specials->bronze)
#define GET_COPPER(ch)	   ((ch)->player_specials->copper)
#define GET_BANK_GOLD(ch) ((ch)->bank_gold)
#define GET_ACCURACY_BASE(ch) ((ch)->accuracy)
#define GET_ACCURACY_MOD(ch) ((ch)->accuracy_mod)
#define GET_DAMAGE_MOD(ch) ((ch)->damage_mod)
#define GET_SPELLFAIL(ch) ((ch)->spellfail)
#define GET_MODIFIED_SPELLFAIL(ch) (GET_CLASS_RANKS(ch, CLASS_BARD) > 1 ? (GET_EQ(ch, WEAR_BODY) && GET_OBJ_VAL(GET_EQ(ch, WEAR_BODY), 0) <= 15 \
                                   && !GET_EQ(ch, WEAR_SHIELD) ? 0 : GET_SPELLFAIL(ch)) : GET_SPELLFAIL(ch))
#define GET_ARMORCHECK(ch) ((ch)->armorcheck)
#define GET_ARMORCHECKALL(ch) ((ch)->armorcheckall)
#define GET_RESEARCH_TOKENS(ch) ((ch)->player_specials->research_tokens)


#define SET_SPELL_SLOT(ch, lvl, num) \
              (ch->mob_specials.spell_slots[lvl - 1] = num)

#define GET_SPELL_SLOT(ch, spl) \
              (!IS_NPC(ch) ? 0 : \
               ((spl < 2) ? ch->mob_specials.spell_slots[0] : \
                ((spl < 3) ? ch->mob_specials.spell_slots[1] : \
                 ((spl < 4) ? ch->mob_specials.spell_slots[2] : \
                  ((spl < 5) ? ch->mob_specials.spell_slots[3] : \
                   ((spl < 6) ? ch->mob_specials.spell_slots[4] : \
                    ((spl < 7) ? ch->mob_specials.spell_slots[5] : \
                     ((spl < 8) ? ch->mob_specials.spell_slots[6] : \
                      ((spl < 9) ? ch->mob_specials.spell_slots[7] : \
                       ((spl < 10) ? ch->mob_specials.spell_slots[8] : 0))))))))))

#define GET_POS(ch)		((ch)->position)
#define GET_IDNUM(ch)		((ch)->idnum)
#define GET_ID(x)		((x)->id)
#define IS_CARRYING_W(ch)	((ch)->carry_weight)
#define IS_CARRYING_N(ch)	((ch)->carry_items)
#define FIGHTING(ch)		((ch)->fighting)
#define GET_FIGHTING_MAX_LVL(ch) (ch->max_fighting_level)
#define HUNTING(ch)		((ch)->hunting)
#define GET_POWERATTACK(ch)	((ch)->powerattack)
#define GET_EXPERTISE_BONUS(ch) (ch->player_specials->expertise_mod)
#define GET_SAVE_BASE(ch, i)	((ch)->saving_throw[i])
#define GET_SAVE_MOD(ch, i)	((ch)->apply_saving_throw[i])
#define GET_SAVE(ch, i)		(GET_SAVE_BASE(ch, i) + GET_SAVE_MOD(ch, i))
#define GET_ALIGNMENT(ch)	((ch)->alignment)
#define GET_ALIGN(ch)     (GET_ALIGNMENT(ch))
#define GET_ETHIC_ALIGNMENT(ch)	((ch)->player_specials->alignment_ethic)
#define GET_ETHIC(ch)   (GET_ETHIC_ALIGNMENT(ch))
#define GET_ETHOS(ch)		((ch)->player_specials->alignment_ethic)
#define GET_FALSE_ALIGNMENT(ch)	((ch)->player_specials->false_alignment)
#define GET_FALSE_ETHOS(ch)	((ch)->player_specials->false_ethos)

#define GET_ALIGN_ABBREV(e, a)  (e > 250 ? (a > 250 ? "LG" : (a < -250 ? "LE" : "LN")) : ( \
                                 e < -250 ? (a > 250 ? "CG" : (a < -250 ? "CE" : "CN")) : ( \
                                 (a > 250 ? "NG" : (a < -250 ? "NE" : "TN")) ) ) )

#define GET_ALIGN_STRING(e, a)  (e > 250 ? (a > 250 ? "Lawful Good" : (a < -250 ? "Lawful Evil" : "Lawful Neutral")) : ( \
                                 e < -250 ? (a > 250 ? "Chaotic Good" : (a < -250 ? "Chaotic Evil" : "Chaotic Neutral")) : ( \
                                 (a > 250 ? "Neutral Good" : (a < -250 ? "Neutral Evil" : "True Neutral")) ) ) )

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_TEMP_LOADROOM(ch)   (ch->player_specials->temp_load_room)
#define GET_RECALL(ch)          ((ch)->player_specials->recall_room)
#define GET_PRACTICES(ch,cl)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->class_skill_points[cl]))
#define GET_RACE_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->skill_points))
#define GET_TRAINS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ability_trains))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_OLC_ZONE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->olc_zone))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_HOST(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))

#define GET_SKILL_BONUS(ch, i)		(ch->skillmods[i])
#define SET_SKILL_BONUS(ch, i, value)	do { (ch)->skillmods[i] = value; } while (0)
#define GET_SKILL_BASE(ch, i)		(ch->skills[i])
#define GET_SKILL_RANKS(ch, i)		(GET_SKILL_BASE(ch, i) + (is_class_skill(ch, i) ? 3 : 0))
#define GET_SKILL(ch, i)		(IS_NPC(ch) ? (spell_info[i].can_learn_skill[GET_CLASS(ch)] == SKLEARN_CLASS ? \
					GET_LEVEL(ch) : 0) : ((ch)->skills[i] + GET_SKILL_BONUS(ch, i) + (is_class_skill(ch, i) ? 3 : 0)))
#define SET_SKILL(ch, i, val)		do { (ch)->skills[i] = val; } while(0)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_DESCRIPTION(ch)      (IS_NPC(ch) ? (ch)->description : GET_PLAYER_DESC(ch))
#define GET_SHORT_DESC(ch)       ((ch)->short_descr)
#define GET_LONG_DESC(ch)        ((ch)->long_descr)

#define GET_PC_BG_ONE               ((ch)->player_specials->background_one)
#define GET_PC_BG_TWO               ((ch)->player_specials->background_two)
#define GET_PC_BG_THREE               ((ch)->player_specials->background_three)
#define GET_PC_BG_FOUR               ((ch)->player_specials->background_four)
#define GET_PC_BG_FIVE               ((ch)->player_specials->background_five)
#define GET_PC_BG_SIX               ((ch)->player_specials->background_six)

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) (max_carry_weight(ch))
#define CAN_CARRY_N(ch) (GET_DEX(ch) * 10 + GET_LEVEL(ch) * 10)
//#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   ((!affected_by_spell((struct char_data *) ch, SPELL_BLINDNESS) || HAS_FEAT(ch, FEAT_BLINDSENSE)) \
     && (AFF_FLAGGED(ch, AFF_INFRAVISION) || HAS_FEAT(ch, 139) || \
   (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) || has_daylight((struct char_data *) ch) || \
   has_light((struct char_data *) ch) || (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_DARK) && SECT(IN_ROOM(ch)) != SECT_FOREST \
    && SECT(IN_ROOM(ch)) != SECT_UNDERWATER)))
   // feat 139 is low light vision

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 250)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -250)
#define IS_LAWFUL(ch)   (GET_ETHIC_ALIGNMENT(ch) >= 250)
#define IS_CHAOTIC(ch)  (GET_ETHIC_ALIGNMENT(ch) <= -250)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_FALSE_GOOD(ch)    (GET_FALSE_ALIGNMENT(ch) >= 250)
#define IS_FALSE_EVIL(ch)    (GET_FALSE_ALIGNMENT(ch) <= -250)
#define IS_FALSE_LAWFUL(ch)   (GET_FALSE_ETHOS(ch) >= 250)
#define IS_FALSE_CHAOTIC(ch)  (GET_FALSE_ETHOS(ch) <= -250)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define IS_ETHIC_NEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)	((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) + \
                         (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))
#define FALSE_ALIGN_TYPE(ch)	((IS_FALSE_GOOD(ch) ? 0 : (IS_FALSE_EVIL(ch) ? 6 : 3)) + \
                         (IS_FALSE_LAWFUL(ch) ? 0 : (IS_FALSE_CHAOTIC(ch) ? 2 : 1)))
#define ALIGN_LAWFUL_GOOD		0
#define ALIGN_LAWFUL_NEUTRAL	1
#define ALIGN_LAWFUL_EVIL		2
#define ALIGN_NEUTRAL_GOOD		3
#define ALIGN_NEUTRAL_NEUTRAL	4
#define ALIGN_NEUTRAL_EVIL		4
#define ALIGN_CHAOTIC_GOOD		6
#define ALIGN_CHAOTIC_NEUTRAL	7
#define ALIGN_CHAOTIC_EVIL		8


/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_CEDIT || STATE(d) == CON_PLAYING ||     \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_AEDIT ||    \
			STATE(d) == CON_GEDIT || STATE(d) == CON_QSTATS ||      \
                        STATE(d) == CON_GEN_DESCS_INTRO || STATE(d) == CON_GEN_DESCS_DESCRIPTORS_1 ||      \
                        STATE(d) == CON_GEN_DESCS_DESCRIPTORS_2 || STATE(d) == CON_GEN_DESCS_ADJECTIVES_1 ||      \
                        STATE(d) == CON_GEN_DESCS_ADJECTIVES_2 || STATE(d) == CON_GEN_DESCS_MENU ||      \
                        STATE(d) == CON_GEN_DESCS_MENU_PARSE || STATE(d) == CON_PETSET || \
			(STATE(d) >= CON_LEVELUP_START && STATE(d) <= CON_LEVELUP_END) || \
			(STATE(d) >= CON_GEN_DESCS_INTRO && STATE(d) <= CON_GEN_DESCS_MENU_PARSE) \
			)

#define SENDOK(ch)    ((((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch))))
/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)

/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * if using unsigned types, the top array index will catch everything.
 * if using signed types, NOTHING will catch the majority of bad accesses.
 */
#define VALID_OBJ_RNUM(obj)	(GET_OBJ_RNUM(obj) <= top_of_objt && \
				 GET_OBJ_RNUM(obj) != NOTHING)

#define GET_ITEM_LORE_TYPE(obj) (obj->item_lore_type)
#define GET_OBJ_LEVEL(obj)      ((obj)->level)
#define GET_OBJ_PERM(obj)       ((obj)->bitvector)
#define GET_OBJ_TYPE(obj)	((obj)->type_flag)
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_RENT(obj)	((obj)->cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->extra_flags)
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->extra_flags[(i)])
#define GET_OBJ_WEAR(obj)	((obj)->wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->timer)
#define GET_OBJ_SIZE(obj)	((obj)->size)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_SPEC(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].func : NULL)

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == 1)

#define CAN_WEAR(obj, part)	OBJWEAR_FLAGGED((obj), (part))
#define GET_OBJ_MATERIAL(obj)   ((obj)->value[7])
#define GET_OBJ_SHORT(obj)	((obj)->short_description)

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")
#define MAFE(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "male":"female") : "it")

#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (!affected_by_spell((struct char_data *) sub, SPELL_BLINDNESS) || HAS_FEAT(sub, FEAT_BLINDSENSE)) && \
   ((IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION)) || \
    CAN_SEE_IN_DARK(sub)))

#define INVIS_OK(sub, obj) \
 ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj) && !AFF_FLAGGED(sub, AFF_BLIND) && \
       (!affected_by_spell((struct char_data *) sub, SPELL_BLINDNESS) || HAS_FEAT(sub, FEAT_BLINDSENSE)))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_ADMLEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#define CAN_GO(ch, door) (ch && EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define RACE(ch)      (pc_race_types[(int)GET_RACE(ch)])
#define CLASS_ABBR(ch) (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS ? \
                        class_abbrevs_fr[(int)GET_CLASS(ch)] : \
                        class_abbrevs_dl_aol[(int)GET_CLASS(ch)])
#define RACE_ABBR(ch) (race_abbrevs[(int)GET_RACE(ch)])

#define IS_WIZARD(ch)           (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_WIZARD : GET_CLASS_RANKS(ch, CLASS_WIZARD) > 0)
#define IS_CLERIC(ch)           (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_CLERIC : GET_CLASS_RANKS(ch, CLASS_CLERIC) > 0)
#define IS_ROGUE(ch)            (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_ROGUE : GET_CLASS_RANKS(ch, CLASS_ROGUE) > 0)
#define IS_FIGHTER(ch)          (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_FIGHTER : GET_CLASS_RANKS(ch, CLASS_FIGHTER) > 0)
#define IS_MONK(ch)             (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_MONK : GET_CLASS_RANKS(ch, CLASS_MONK) > 0)
#define IS_PALADIN(ch)          (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_PALADIN : GET_CLASS_RANKS(ch, CLASS_PALADIN) > 0)
#define IS_PALADIN_SPELLCASTER(ch) (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_PALADIN && GET_LEVEL(ch) > 3 : GET_CLASS_RANKS(ch, CLASS_PALADIN) > 3)
#define IS_RANGER(ch)           (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_RANGER : GET_CLASS_RANKS(ch, CLASS_RANGER) > 0)
#define IS_RANGER_SPELLCASTER(ch) (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_RANGER && GET_LEVEL(ch) > 3 : GET_CLASS_RANKS(ch, CLASS_RANGER) > 3)
#define IS_DRUID(ch)            (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_DRUID : GET_CLASS_RANKS(ch, CLASS_DRUID) > 0)
#define IS_BARD(ch)             (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_BARD : GET_CLASS_RANKS(ch, CLASS_BARD) > 0)
#define IS_SORCERER(ch)         (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_SORCERER : GET_CLASS_RANKS(ch, CLASS_SORCERER) > 0)
#define IS_FAVORED_SOUL(ch)         (IS_NPC(ch) ? GET_CLASS(ch) == CLASS_FAVORED_SOUL : GET_CLASS_RANKS(ch, CLASS_FAVORED_SOUL) > 0)

#define IS_MEM_BASED_CASTER(ch) (IS_WIZARD(ch) || IS_CLERIC(ch) || IS_PALADIN_SPELLCASTER(ch) || \
                                 IS_RANGER_SPELLCASTER(ch) || IS_DRUID(ch))

#define IS_SPELLCASTER(ch)      (IS_WIZARD(ch) || IS_CLERIC(ch) || IS_PALADIN_SPELLCASTER(ch) || \
				 IS_RANGER_SPELLCASTER(ch) || IS_DRUID(ch) || IS_BARD(ch) || IS_SORCERER(ch) || IS_FAVORED_SOUL(ch))

#define IS_CHAR_SPELLCASTER_CLASS(ch, i) ((i == CLASS_PALADIN && IS_PALADIN_SPELLCASTER(ch)) || \
                                         (i == CLASS_RANGER && IS_RANGER_SPELLCASTER(ch)) || \
                                         (i == CLASS_WIZARD && IS_WIZARD(ch)) || (i == CLASS_CLERIC && IS_CLERIC(ch)) || \
                                         (i == CLASS_DRUID && IS_DRUID(ch)) || (i == CLASS_BARD && IS_BARD(ch)) || \
					 (i == CLASS_SORCERER && IS_SORCERER(ch)) || (i == CLASS_FAVORED_SOUL && IS_FAVORED_SOUL(ch)))


#define GET_CASTER_LEVEL(ch, i) (IS_CHAR_SPELLCASTER_CLASS(ch, i) ? (((i == CLASS_PALADIN || i == CLASS_RANGER) ? -4 : 0) + \
                                 GET_CLASS_RANKS(ch, i) + ch->player_specials->bonus_levels[i]) : 0)

#define IS_HUMAN(ch)            (race_list[GET_RACE(ch)].family == RACE_TYPE_HUMAN)

#define IS_ELF(ch)              (race_list[GET_RACE(ch)].family == RACE_TYPE_ELF)

#define IS_GNOME(ch)		(race_list[GET_RACE(ch)].family == RACE_TYPE_GNOME)

#define IS_DEEP_GNOME(ch)	(GET_RACE(ch) == RACE_DEEP_GNOME)

#define IS_IRDA(ch) 		(GET_RACE(ch) == RACE_IRDA)

#define IS_DWARF(ch)		(race_list[GET_RACE(ch)].family == RACE_TYPE_DWARF)

#define IS_HALF_ELF(ch)		(GET_RACE(ch) == RACE_HALF_ELF)

#define IS_HALFLING(ch)		(race_list[GET_RACE(ch)].family == RACE_TYPE_HALFLING)

#define IS_KENDER(ch)		(race_list[GET_RACE(ch)].family == RACE_TYPE_KENDER)

#define IS_ORC(ch)              (race_list[GET_RACE(ch)].family == RACE_TYPE_ORC)

#define IS_HALF_ORC(ch)		(GET_RACE(ch) == RACE_HALF_ORC)

#define IS_DROW_ELF(ch)         (GET_RACE(ch) == RACE_DROW_ELF)

#define IS_NATIVE_OUTSIDER(ch)  (FALSE)

#define IS_GIANT(ch)            (race_list[GET_RACE(ch)].family == RACE_TYPE_GIANT)

#define IS_OGRE(ch)             (GET_RACE(ch) == RACE_OGRE || GET_RACE(ch) == RACE_HALF_OGRE)

#define IS_CENTAUR(ch)          (GET_RACE(ch) == RACE_CENTAUR)

#define IS_ANIMAL(ch)           (race_list[GET_RACE(ch)].family == RACE_TYPE_ANIMAL)

#define IS_CHEETAH(ch)		(GET_RACE(ch) == RACE_CHEETAH)

#define IS_EAGLE(ch)		(GET_RACE(ch) == RACE_EAGLE)

#define IS_CONSTRUCT(ch)        (race_list[GET_RACE(ch)].family == RACE_TYPE_CONSTRUCT)

#define IS_DEMON(ch)            (FALSE)

#define IS_DRAGON(ch)           (FALSE)

#define IS_FISH(ch)             (FALSE)

#define IS_GOBLIN(ch)           (FALSE)

#define IS_INSECT(ch)           (FALSE)

#define IS_SNAKE(ch)            (FALSE)

#define IS_TROLL(ch)            (FALSE)

#define IS_MINOTAUR(ch)         (race_list[GET_RACE(ch)].family == RACE_TYPE_MINOTAUR)

#define IS_DRACONIAN(ch)        (race_list[GET_RACE(ch)].family == RACE_TYPE_DRACONIAN)

#define IS_SEA_ELF(ch)          (GET_RACE(ch) == RACE_DARGONESTI_ELF || GET_RACE(ch) == RACE_TYPE_DIMERNESTI_ELF)

#define IS_KOBOLD(ch)           (FALSE)

#define IS_LIZARDFOLK(ch)       (FALSE)

#define IS_UNDEAD(ch)           (race_list[GET_RACE(ch)].family == RACE_TYPE_UNDEAD)

#define IS_MONSTROUS_HUMANOID(ch) (race_list[GET_RACE(ch)].family == RACE_TYPE_MONSTROUS_HUMANOID)

#define IS_HUMANOID(ch)		((IS_HUMAN(ch) || IS_DWARF(ch) || IS_ELF(ch) || \
				IS_HALFLING(ch) || IS_KENDER(ch) || IS_MINOTAUR(ch) || \
				IS_GNOME(ch) || \
				IS_MONSTROUS_HUMANOID(ch) || IS_ORC(ch) || IS_GIANT(ch)) && \
				GET_CLASS_RANKS(ch, CLASS_MONK) < 20)



#define IS_HUMANOID_RACE(race)    (race_list[race].family == RACE_TYPE_HUMAN || \
                                   race_list[race].family == RACE_TYPE_DWARF || \
                                   race_list[race].family == RACE_TYPE_ELF || \
                                   race_list[race].family == RACE_TYPE_HALFLING || \
                                   race_list[race].family == RACE_TYPE_KENDER || \
                                   race_list[race].family == RACE_TYPE_GNOME || \
                                   race_list[race].family == RACE_TYPE_MONSTROUS_HUMANOID || \
                                   race_list[race].family == RACE_TYPE_FEY || \
                                   race_list[race].family == RACE_TYPE_ORC || \
                                   race_list[race].family == RACE_TYPE_GIANT \
                                  )



#define OUTSIDE(ch) (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))

#define SPEAKING(ch)     ((ch)->player_specials->speaking)

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#define false 0

#define true (!false)

#define virtual 1

#define FALSE 0

#define TRUE (!FALSE)

#if !defined(YES)
#define YES 1
#endif

#if !defined(NO)
#define NO 0
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/*******************  Config macros *********************/

#define CONFIG_CONFFILE         config_info.CONFFILE

#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
#define CONFIG_IDLE_VOID        config_info.play.idle_void
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
#define CONFIG_LEVEL_CAP	config_info.play.level_cap
#define CONFIG_STACK_MOBS	config_info.play.stack_mobs
#define CONFIG_STACK_OBJS	config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING	config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION	config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL	config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION	config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES	config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE	config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER	config_info.play.exp_multiplier
/* Map/Automap options */
#define CONFIG_MAP             config_info.play.map_option
#define CONFIG_MAP_SIZE        config_info.play.map_size
#define CONFIG_MINIMAP_SIZE    config_info.play.minimap_size
#define CONFIG_CAMPAIGN         config_info.play.campaign


  /** Crash Saves **/
#define CONFIG_FREE_RENT        config_info.csd.free_rent
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

  /** Room Numbers **/
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3

  /** Game Operation **/
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
#define CONFIG_MENU             config_info.operation.MENU
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
#define CONFIG_START_MESSG      config_info.operation.START_MESSG

  /** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

  /** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS	config_info.advance.allow_multiclass

  /** For tick system **/
#define CONFIG_PULSE_VIOLENCE	config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE	config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE	config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE	config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD	config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY	config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE	config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE	config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT	config_info.ticks.pulse_current

  /** Character Creation Method **/
#define CONFIG_CREATION_METHOD	config_info.creation.method

#define GET_EPIC_SPELLS(ch) (ch->epic_spells)
#define GET_BARD_SONGS(ch) (ch->player_specials->bard_songs)
#define GET_BARD_SPELLS(ch, i) (ch->player_specials->bard_spells[i])
#define GET_SORCERER_SPELLS(ch, i) (ch->player_specials->sorcerer_spells[i])
#define GET_FAVORED_SOUL_SPELLS(ch, i) (ch->player_specials->favored_soul_spells[i])
#define GET_ASSASSIN_SPELLS(ch, i) (ch->player_specials->assassin_spells[i])

// Wizard Memorization
#define GET_SPELLMEM(ch, i)	((ch->player_specials->spellmem[i]))
#define GET_MEMCURSOR(ch)	((ch->player_specials->memcursor))

// Cleric Memorization
#define GET_SPELLMEM_C(ch, i)	((ch->player_specials->spellmem_c[i]))
#define GET_MEMCURSOR_C(ch)	((ch->player_specials->memcursor_c))

// Paladin Memorization
#define GET_SPELLMEM_P(ch, i)	((ch->player_specials->spellmem_p[i]))
#define GET_MEMCURSOR_P(ch)	((ch->player_specials->memcursor_p))

// Druid Memorization
#define GET_SPELLMEM_D(ch, i)	((ch->player_specials->spellmem_d[i]))
#define GET_MEMCURSOR_D(ch)	((ch->player_specials->memcursor_d))

// Ranger Memorization
#define GET_SPELLMEM_R(ch, i)	((ch->player_specials->spellmem_r[i]))
#define GET_MEMCURSOR_R(ch)	((ch->player_specials->memcursor_r))

// Bard Memorization
#define GET_SPELLMEM_B(ch, i)	((ch->player_specials->spellmem_b[i]))
#define GET_MEMCURSOR_B(ch)	((ch->player_specials->memcursor_b))

#define GET_MEM_TYPE(ch)        (ch->player_specials->mem_type)

/* returns the number of spells per slot */
#define GET_SPELL_LEVEL(ch, i)	((ch)->player_specials->spell_level[i])
#define SINFO			(spell_info[spellnum])
#define IS_ARCANE_SPELL(spellnum) (SINFO.school != SCHOOL_UNDEFINED)
#define IS_DIVINE_SPELL(spellnum) (SINFO.domain != DOMAIN_UNDEFINED)
#define IS_ARCANE_CLASS(class)  (class == CLASS_WIZARD || class == CLASS_BARD || class == CLASS_SORCERER)
#define IS_DIVINE_CLASS(class)	(class == CLASS_CLERIC || class == CLASS_PALADIN || class == CLASS_RANGER || \
                                 class == CLASS_DRUID)
#define IS_ARCANE(ch)		(IS_WIZARD(ch))
#define IS_DIVINE(ch)		(IS_CLERIC(ch) || IS_PALADIN(ch))
#define HAS_REAL_FEAT(ch, i)	(ch->feats[i])
#define HAS_FEAT(ch, i)		(get_feat_value(ch, i))
#define SET_FEAT(ch, i, j)      (ch->feats[i] = j)
#define MOB_HAS_FEAT(ch, i)	(IS_SET_AR(MOB_FEATS(ch), i))
#define MOB_SET_FEAT(ch, i)	(SET_BIT_AR(MOB_FEATS(ch), i))
#define HAS_COMBAT_FEAT(ch,i,j) (IS_SET_AR(ch->combat_feats[i], j))
#define SET_COMBAT_FEAT(ch,i,j) (SET_BIT_AR((ch)->combat_feats[(i)], (j)))
#define HAS_SCHOOL_FEAT(ch,i,j) (IS_SET((ch)->school_feats[(i)], (j)))
#define SET_SCHOOL_FEAT(ch,i,j) (SET_BIT((ch)->school_feats[(i)], (j)))
#define GET_BAB(ch)		(GET_LVL_0_BAB(ch) + GET_ACCURACY_BASE(ch))
#define GET_LVL_0_BAB(ch)	((ch->levelup && ch->levelup->class && ( \
				ch->levelup->class == CLASS_FIGHTER || \
				ch->levelup->class == CLASS_PALADIN || \
				ch->levelup->class == CLASS_RANGER || \
				ch->levelup->class == CLASS_BARBARIAN)) ? \
				1 : 0)
				
#define GET_TOTAL_AOO(ch) (ch->attacks_of_opportunity)
#define GET_TOTAL_ARROWS_DEFLECTED(ch) (ch->arrows_deflected)
#define GET_ENMITY(ch) (ch->player_specials->enmity)
#define GET_HATE(ch)	(GET_ENMITY(ch))


#define GET_SPELL_MASTERY_POINTS(ch) \
				(ch->player_specials->spell_mastery_points)
#define GET_FEAT_POINTS(ch)	(ch->player_specials->feat_points)
#define GET_EPIC_FEAT_POINTS(ch) \
				(ch->player_specials->epic_feat_points)
#define GET_CLASS_FEATS(ch,cl)	(ch->player_specials->class_feat_points[cl])
#define GET_EPIC_CLASS_FEATS(ch,cl) \
				(ch->player_specials->epic_class_feat_points[cl])
#define IS_EPIC_LEVEL(ch)	(GET_CLASS_LEVEL(ch) >= 20 || (ch->levelup && ch->levelup->level >= 20))
#define IS_EPIC(ch)             (IS_EPIC_LEVEL(ch))

#define GET_FORM_POS(ch)	(ch->player_specials->form_pos)
#define GET_FORM_TOTAL(ch, i)   (ch->player_specials->form_total[i])

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */

#ifdef CIRCLE_UNIX
#include <dirent.h>

struct xap_dir {
  int total, current;
  struct dirent **namelist;
};

#elif defined(CIRCLE_WINDOWS)

struct xap_dir {
  int total,current;
  char **namelist;
};

#endif

int xdir_scan(char *dir_name, struct xap_dir *xapdirp);
int xdir_get_total(struct xap_dir *xd);
char *xdir_get_name(struct xap_dir *xd, int num);
char *xdir_next(struct xap_dir *xd);
void xdir_close(struct xap_dir *xd);
int insure_directory(char *path, int isfile);
void admin_set(struct char_data *ch, int value);
#define GET_PAGE_LENGTH(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))
#define GET_SCREEN_WIDTH(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->screen_width))

/* Autoquests data */
#define GET_QUESTPOINTS(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->questpoints))
#define GET_QUEST(ch)           CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->current_quest))
#define GET_QUEST_COUNTER(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->quest_counter))
#define GET_QUEST_TIME(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->quest_time))
#define GET_NUM_QUESTS(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->num_completed_quests))
#define GET_QUEST_TYPE(ch)      (real_quest(GET_QUEST((ch))) != NOTHING ? aquest_table[real_quest(GET_QUEST((ch)))].type : AQ_UNDEFINED )


#define IN_OLC(ch) (STATE(ch->desc) == CON_OEDIT || STATE(ch->desc) == CON_IEDIT || \
                    STATE(ch->desc) == CON_ZEDIT || STATE(ch->desc) == CON_SEDIT || \
                    STATE(ch->desc) == CON_MEDIT || STATE(ch->desc) == CON_REDIT || \
                    STATE(ch->desc) == CON_CEDIT || STATE(ch->desc) == CON_AEDIT || \
                    STATE(ch->desc) == CON_TRIGEDIT || STATE(ch->desc) == CON_ASSEDIT || \
                    STATE(ch->desc) == CON_GEDIT || STATE(ch->desc) == CON_HEDIT || \
                    STATE(ch->desc) == CON_LEVELUP)

#define SUGGESTED_OBJ_COST(obj) GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1)

#define GET_HEAL_ROLL(ch) (ch->player_specials->heal_roll)
#define GET_HEAL_AMOUNT(ch) (ch->player_specials->heal_amount)
#define GET_HEAL_USED(ch) (ch->player_specials->heal_used)
#define GET_CAMP_USED(ch) (ch->player_specials->camp_used)
#define GET_STAT_POINTS(ch) (ch->player_specials->stat_points)

#define GET_CRAFTING_TYPE(ch)   (ch->player_specials->crafting_type)
#define GET_CRAFTING_TICKS(ch)  (ch->player_specials->crafting_ticks)
#define GET_CRAFTING_OBJ(ch) (ch->player_specials->crafting_object)
#define GET_CRAFTING_REPEAT(ch) (ch->crafting_repeat)

#define GET_ACT_LEVEL(ch) (ch->desc ? (ch->desc->account ? ch->desc->account->level : 0) : 0)

#define GET_TAVERN_EXP_BONUS(ch) (ch->player_specials->tavern_exp_bonus_rounds)

#define GET_COMPANION_NAME(ch) (ch->player_specials->companion_name)
#define GET_COMPANION_TYPE(ch) (ch->player_specials->companion_type)

#define IS_POWER_ESSENCE(obj) (GET_OBJ_TYPE(obj) == ITEM_MATERIAL && \
                               (GET_OBJ_VNUM(obj) == 64100  || GET_OBJ_VNUM(obj) == 64101 || GET_OBJ_VNUM(obj) == 64102 \
                               || GET_OBJ_VNUM(obj) == 64103 || GET_OBJ_VNUM(obj) == 64104))
#define IS_SPIRIT_ESSENCE(obj) (GET_OBJ_VNUM(obj) >= 64110 && GET_OBJ_VNUM(obj) <= 64114)
#define IS_FIRE_ESSENCE(obj) (GET_OBJ_VNUM(obj) >= 64105 && GET_OBJ_VNUM(obj) <= 64109)
#define IS_WATER_ESSENCE(obj) (GET_OBJ_VNUM(obj) >= 64115 && GET_OBJ_VNUM(obj) <= 64119)
#define IS_AIR_ESSENCE(obj) (GET_OBJ_VNUM(obj) >= 64120 && GET_OBJ_VNUM(obj) <= 64124)
#define IS_EARTH_ESSENCE(obj) (GET_OBJ_VNUM(obj) >= 64125 && GET_OBJ_VNUM(obj) <= 64129)

#define IS_ESSENCE(obj) (IS_POWER_ESSENCE(obj) || IS_SPIRIT_ESSENCE(obj) || IS_FIRE_ESSENCE(obj) || \
                        IS_WATER_ESSENCE(obj) || IS_AIR_ESSENCE(obj) || IS_EARTH_ESSENCE(obj))

#define IS_CLOTH(mat)      (mat == MATERIAL_COTTON || mat == MATERIAL_SILK || mat == MATERIAL_SATIN || \
                            mat == MATERIAL_VELVET || mat == MATERIAL_WOOL || mat == MATERIAL_HEMP)
#define IS_LEATHER(mat)    (mat == MATERIAL_LEATHER || mat == MATERIAL_BURLAP || mat == MATERIAL_DRAGONHIDE)
#define IS_WOOD(mat)       (mat == MATERIAL_WOOD || mat == MATERIAL_DARKWOOD)
#define IS_HARD_METAL(mat) (mat == MATERIAL_STEEL || mat == MATERIAL_ALCHEMAL_SILVER || \
                            mat == MATERIAL_COLD_IRON || mat == MATERIAL_MITHRIL || mat == MATERIAL_ADAMANTINE)
#define IS_PRECIOUS_METAL(mat) (mat == MATERIAL_SILVER || mat == MATERIAL_GOLD || mat == MATERIAL_COPPER || \
                                mat == MATERIAL_PLATINUM)

#define IS_GEMSTONE(mobj)  (GET_OBJ_VNUM(obj) == 64001 || GET_OBJ_VNUM(obj) == 64003 || GET_OBJ_VNUM(obj) == 64006 || \
                            GET_OBJ_VNUM(obj) == 64008 || GET_OBJ_VNUM(obj) == 64009 || GET_OBJ_VNUM(obj) == 64011)
#define IS_PETRIFIED_WOOD(mobj)  (GET_OBJ_VNUM(obj) == 64033 || GET_OBJ_VNUM(obj) == 64034 || GET_OBJ_VNUM(obj) == 64035 || \
                            GET_OBJ_VNUM(obj) == 64036)
#define IS_FOSSIL(mobj)  (GET_OBJ_VNUM(obj) == 64037 || GET_OBJ_VNUM(obj) == 64038 || GET_OBJ_VNUM(obj) == 64039 || \
                            GET_OBJ_VNUM(obj) == 64040)
#define GET_DEATH_ATTACK(ch) (ch->player_specials->death_attack)
#define GET_MARK_ROUNDS(ch)  (ch->player_specials->mark_rounds)
#define GET_MARK(ch)         (ch->player_specials->mark_target)
#define GET_UNDEATH_TOUCH(ch) (ch->touch_of_undeath)

#define IS_EPIC_FEAT(featnum) (feat_list[featnum].epic == TRUE)

#define GET_GUILD(ch)      (ch->player_specials->guild)
#define GET_SUBGUILD(ch)   (ch->player_specials->subguild)
#define GET_GUILD_RANK(ch) (ch->player_specials->guild_rank)
#define GET_GUILD_EXP(ch)  (ch->player_specials->guild_exp)

#define GET_ARTISAN_EXP(ch) (ch->player_specials->artisan_experience)
#define GET_BONUS_LEVELS(ch, i) (ch->player_specials->bonus_levels[i])
#define GET_SPELLCASTER_LEVEL(ch) (ch->player_specials->caster_level)

#define GUARDING(ch) (ch->guarding)
#define GUARDED_BY(ch) (ch->guarded_by)

#define GET_INNATE(ch, i) (ch->player_specials->innate_abilities[i])

#define GET_LFG_STRING(ch) (ch->player_specials->lfg_string)

#define IS_LIGHT_LOAD(ch)  (IS_CARRYING_W(ch) <= gen_carry_weight(ch))
#define IS_MEDIUM_LOAD(ch) (IS_CARRYING_W(ch) > gen_carry_weight(ch) && IS_CARRYING_W(ch) <= (gen_carry_weight(ch) * 2))
#define IS_HEAVY_LOAD(ch)  (IS_CARRYING_W(ch) > (gen_carry_weight(ch) * 2) && IS_CARRYING_W(ch) <= (gen_carry_weight(ch) * 3.33))
#define IS_OVER_LOAD(ch)   (IS_CARRYING_W(ch) > (gen_carry_weight(ch) * 3.33))

#define GET_FIGHT_PRECISE_ATTACK(ch)     (ch->fight_precise_attack)
#define GET_FIGHT_SNEAK_ATTACK(ch)       (ch->fight_sneak_attack)
#define GET_FIGHT_SPRING_ATTACK(ch)       (ch->fight_spring_attack)
#define GET_FIGHT_CRITICAL_HIT(ch)       (ch->fight_critical_hit)
#define GET_FIGHT_DAMAGE_REDUCTION(ch)   (ch->fight_damage_reduction)
#define GET_FIGHT_DAMAGE_REDUCTION_ACTUAL(ch)   (ch->fight_damage_reduction_actual)
#define GET_FIGHT_DEATH_ATTACK(ch)       (ch->fight_death_attack)
#define GET_FIGHT_UNDEATH_TOUCH(ch)       (ch->fight_touch_of_undeath)
#define GET_FIGHT_DAMAGE_DONE(ch)        (ch->fight_damage_done)
#define GET_FIGHT_NUMBER_OF_ATTACKS(ch)  (ch->fight_number_of_attacks)
#define GET_FIGHT_NUMBER_OF_HITS(ch)     (ch->fight_number_of_hits)
#define GET_FIGHT_MESSAGE_PRINTED(ch)     (ch->fight_message_printed)
#define GET_FIGHT_BLEEDING(ch)		(ch->bleeding_attack)
#define GET_FIGHT_BLEEDING_DAMAGE(ch)	(ch->bleeding_damage)
#define GET_DAMAGE_TAKEN_THIS_ROUND(ch) (ch->dam_round_taken)
#define GET_DAMAGE_DEALT_THIS_ROUND(ch) (ch->dam_round_dealt)

#define GET_CLAN(ch)          ((ch)->clan)
#define GET_CLAN_RANK(ch)     ((ch)->rank)

#define GET_BREATH_WEAPON(ch) (ch->player_specials->breath_weapon)

#define GET_INTROS_GIVEN(ch)     (ch->player_specials->intros_given)
#define GET_INTROS_RECEIVED(ch)  (ch->player_specials->intros_received)
#define VALID_INTRO(ch)          (GET_INTROS_RECEIVED(ch) <= 5 || \
                                  (GET_INTROS_GIVEN(ch) * 2 >= GET_INTROS_RECEIVED(ch)))

#define GET_WISH_STR(ch) (ch->wish_str)
#define GET_WISH_CON(ch) (ch->wish_con)
#define GET_WISH_DEX(ch) (ch->wish_dex)
#define GET_WISH_INT(ch) (ch->wish_int)
#define GET_WISH_WIS(ch) (ch->wish_wis)
#define GET_WISH_CHA(ch) (ch->wish_cha)

#define GET_AUTOQUEST_VNUM(ch)		(ch->autoquest_vnum)
#define GET_AUTOQUEST_KILLNUM(ch)	(ch->autoquest_killnum)
#define GET_AUTOQUEST_QP(ch)		(ch->autoquest_qp)
#define GET_AUTOQUEST_EXP(ch)		(ch->autoquest_exp)
#define GET_AUTOQUEST_GOLD(ch)		(ch->autoquest_gold)
#define GET_AUTOQUEST_DESC(ch)		(ch->autoquest_desc)

#define GET_AUTOCQUEST_VNUM(ch)		(ch->autocquest_vnum)
#define GET_AUTOCQUEST_MAKENUM(ch)	(ch->autocquest_makenum)
#define GET_AUTOCQUEST_QP(ch)		(ch->autocquest_qp)
#define GET_AUTOCQUEST_EXP(ch)		(ch->autocquest_exp)
#define GET_AUTOCQUEST_GOLD(ch)		(ch->autocquest_gold)
#define GET_AUTOCQUEST_DESC(ch)		(ch->autocquest_desc)
#define GET_AUTOCQUEST_MATERIAL(ch)	(ch->autocquest_material)

#define GET_RP_EXP(ch) (ch->rp_exp)

#define GET_ACCOUNT_NAME(ch) (ch->account_name)

#define GET_DISGUISE_DESC_1(ch) (ch->disguise_dsc1)
#define GET_DISGUISE_DESC_2(ch) (ch->disguise_dsc2)
#define GET_DISGUISE_ADJ_1(ch) (ch->disguise_adj1)
#define GET_DISGUISE_ADJ_2(ch) (ch->disguise_adj2)
#define GET_DISGUISE_RACE(ch) (ch->disguise_race)
#define GET_DISGUISE_SEX(ch) (ch->disguise_sex)
#define GET_DISGUISE_ROLL(ch) (ch->disguise_roll)
#define DISGUISE_SEEN(ch) (ch->disguise_seen)

#define GET_STAT_MOB_KILLS(ch) (ch->stat_mob_kills)

#define GET_RP_POINTS(ch)		(ch->rp_points)
#define GET_RP_EXP_BONUS(ch)		(ch->rp_exp_bonus)
#define GET_RP_GOLD_BONUS(ch)		(ch->rp_gold_bonus)
#define GET_RP_ACCOUNT_EXP(ch)		(ch->rp_account_exp)
#define GET_RP_QP_BONUS(ch)		(ch->rp_qp_bonus)
#define GET_RP_CRAFT_BONUS(ch)		(ch->rp_craft_bonus)
#define GET_RP_ART_EXP_BONUS(ch)	(ch->rp_art_exp_bonus)

#define craft_pattern_vnums(i)		(craft_pattern_vnums_real[i] - 200 + 30000)

#define HAS_LOW_LIGHT_VIS(ch)		(HAS_FEAT(ch, FEAT_LOW_LIGHT_VISION) || IS_ELF(ch) || IS_HALF_ELF(ch))
#define HAS_DARKVISION(ch)		(HAS_FEAT(ch, FEAT_DARKVISION) || IS_DWARF(ch))

#define GET_LAY_HANDS(ch)		(ch->player_specials->lay_hands)

#define GET_PETITION(ch)			(ch->petition)

#define GET_GATHER_INFO(ch)		(ch->gather_info)
#define GET_EXTRA_ACC_EXP(ch)		(ch->extra_account_exp)

#define IS_BOSS_MOB(ch)			(IS_NPC(ch) ? ((MOB_FLAGGED(ch, MOB_LIEUTENANT) || MOB_FLAGGED(ch, MOB_CAPTAIN) || \
					 MOB_FLAGGED(ch, MOB_BOSS) || MOB_FLAGGED(ch, MOB_FINAL_BOSS)) ? TRUE : FALSE) : FALSE)
#define IS_LONG_WEAPON(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON && \
        IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_REACH))
#define IS_THROWN_WEAPON(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON && \
        IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_THROWN))
#define IS_RANGED_WEAPON(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON && \
        IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_RANGED))

#define IS_MONK_WEAPON(obj) (weapon_list[GET_OBJ_VAL(obj, 0)].weaponFamily == WEAPON_FAMILY_MONK)

#define HAS_WEAPON_MASTERY(ch, obj) ((HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, (obj ? GET_OBJ_VAL(obj, VAL_WEAPON_SKILL) : \
                                        WEAPON_TYPE_UNARMED)) && HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE)) || \
                                      (has_weapon_feat(ch, FEAT_WEAPON_FOCUS, (obj ? GET_OBJ_VAL(obj, VAL_WEAPON_SKILL) : \
                                        WEAPON_TYPE_UNARMED)) && HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE)))

#define IS_COMPOSITE_BOW(obj) (GET_OBJ_VAL(obj, 0) == WEAPON_TYPE_COMPOSITE_LONGBOW || \
                               GET_OBJ_VAL(obj, 0) == WEAPON_TYPE_COMPOSITE_SHORTBOW)

#define IS_BOW(obj) (GET_OBJ_VAL(obj, 0) == WEAPON_TYPE_LONG_BOW || \
                     GET_OBJ_VAL(obj, 0) == WEAPON_TYPE_SHORT_BOW || \
                     IS_COMPOSITE_BOW(obj))

#define d20 (dice(1, 20))

#define GET_ARTISAN_TYPE(ch)	(ch->artisan_type)
#define GET_ARTISAN_LEVEL(ch)	(GET_CLASS_RANKS(ch, CLASS_ARTISAN))

#define GET_PREF(ch) 		(ch->pref)

#define IS_MOUNT_DRAGON(ch)	(ch->player_specials->mount_num == PET_ADULT_RED_DRAGON || \
                                 ch->player_specials->mount_num == PET_ADULT_SILVER_DRAGON)

#define GET_WISHES(ch)		(ch->wishes)
#define CAMPAIGN_DESOLATION	(CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
#define MONEY_STRING            ((CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE ? "steel coins" : "gold coins"))

#define GET_REPUTATION(ch)      (ch->reputation)

