/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KNUL  ""

/* conditional color.  pass it a pointer to a char_data and a color level. */
#define C_OFF	0
#define C_ON	1
#define C_BRI	C_ON	/* Compatibility hack */
#define C_NRM	C_ON	/* Compatibility hack */
#define C_CMP	C_ON	/* Compatibility hack */

#define COLOR_CHOICES(ch)	(IS_NPC(ch) ? NULL : ch->player_specials ? ch->player_specials->color_choices : NULL)
#define _clrlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_COLOR) ? 1 : 0) : 0)
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_BRI)
#define QRED CCRED(ch,C_BRI)
#define QGRN CCGRN(ch,C_BRI)
#define QYEL CCYEL(ch,C_BRI)
#define QBLU CCBLU(ch,C_BRI)
#define QMAG CCMAG(ch,C_BRI)
#define QCYN CCCYN(ch,C_BRI)
#define QWHT CCWHT(ch,C_BRI)

#define ANSISTART "\x1B["
#define ANSISEP ';'
#define ANSISEPSTR ";"
#define ANSIEND 'm'
#define ANSIENDSTR "m"

/* Attributes */
#define AA_NORMAL       "0"
#define AA_BOLD         "1"
#define AA_UNDERLINE    "4"
#define AA_BLINK        "5"
#define AA_REVERSE      "7"
#define AA_INVIS        "8"
/* Foreground colors */
#define AF_BLACK        "30"
#define AF_RED          "31"
#define AF_GREEN        "32"
#define AF_YELLOW       "33"
#define AF_BLUE         "34"
#define AF_MAGENTA      "35"
#define AF_CYAN         "36"
#define AF_WHITE        "37"
/* Background colors */
#define AB_BLACK        "40"
#define AB_RED          "41"
#define AB_GREEN        "42"
#define AB_YELLOW       "43"
#define AB_BLUE         "44"
#define AB_MAGENTA      "45"
#define AB_CYAN         "46"
#define AB_WHITE        "47"
