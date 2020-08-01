/************************************************************************
 * Generic OLC Library - Mobiles / genmob.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

int delete_mobile(mob_rnum);
int copy_mobile(struct char_data *to, struct char_data *from);
int add_mobile(struct char_data *, mob_vnum);
int copy_mob_strings(struct char_data *to, struct char_data *from);
int free_mob_strings(struct char_data *);
int free_mobile(struct char_data *mob);
int save_mobiles(zone_rnum rznum);
void extract_mobile_all(mob_vnum vnum);
int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd);

/* Handy macros. */
#define GET_NDD(mob)	((mob)->mob_specials.damnodice)
#define GET_SDD(mob)	((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob)	((mob)->name)
#define GET_LDESC(mob)	((mob)->long_descr)
#define GET_DDESC(mob)	((mob)->description)
#define GET_ATTACK(mob)	((mob)->mob_specials.attack_type)
