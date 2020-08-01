#ifndef _CLAN_H_
#define _CLAN_H_

/* functions used in clan.c */
void free_clan(struct clan_type *c);
struct clan_type *enqueue_clan(void);
void dequeue_clan(int clannum);
void order_clans(void);
void create_clanfile(void);
void load_clans(void);
void save_clans(void);

/* functions related to do_clan */
void do_apply( struct char_data *ch, struct clan_type *cptr );
void do_accept( struct char_data *ch, struct clan_type *cptr, struct char_data *vict );
void do_reject( struct char_data *ch, struct clan_type *cptr, struct char_data *vict );
void do_dismiss( struct char_data *ch, struct clan_type *cptr, struct char_data *vict);
struct char_data *find_clan_char(struct char_data *ch, char *arg);
int app_check( struct clan_type *cptr, struct char_data *applicant );
void show_clan_who(struct char_data *ch, struct clan_type *clan);
void perform_ctell(struct char_data *ch, const char *messg, ...);
void perform_cinfo( int clan_number, const char *messg, ... );

/* helper functions for information */
char *get_clan_name(int clan);
char *get_blank_clan_name(int clan);
char *get_rank_name(int clan, int rank);

/* ACMD's */
ACMD(do_clan);
ACMD(do_show_clan);

/* SPECIALs */
SPECIAL(clan_guard);

#endif
