/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL 12
#define DEFAULT_WAND_LVL  12

#define CAST_UNDEFINED  (-1)
#define CAST_SPELL  0
#define CAST_POTION 1
#define CAST_WAND 2
#define CAST_STAFF  3
#define CAST_SCROLL 4
#define CAST_STRIKE 5

#define MAG_DAMAGE    (1 << 0)
#define MAG_AFFECTS   (1 << 1)
#define MAG_UNAFFECTS   (1 << 2)
#define MAG_POINTS    (1 << 3)
#define MAG_ALTER_OBJS    (1 << 4)
#define MAG_GROUPS    (1 << 5)
#define MAG_MASSES    (1 << 6)
#define MAG_AREAS   (1 << 7)
#define MAG_SUMMONS   (1 << 8)
#define MAG_CREATIONS   (1 << 9)
#define MAG_MANUAL    (1 << 10)
#define MAG_AFFECTSV    (1 << 11)
#define MAG_ACTION_FREE   (1 << 12)
#define MAG_ACTION_PARTIAL  (1 << 13)
#define MAG_ACTION_FULL   (1 << 14)
#define MAG_NEXTSTRIKE    (1 << 15)
#define MAG_TOUCH_MELEE   (1 << 16)
#define MAG_TOUCH_RANGED  (1 << 17)
#define MAG_LOOP					(1 << 18)

#define MAGSAVE_NONE    (1 << 0)
#define MAGSAVE_FORT    (1 << 1)
#define MAGSAVE_REFLEX    (1 << 2)
#define MAGSAVE_WILL    (1 << 3)
#define MAGSAVE_HALF    (1 << 4)
#define MAGSAVE_PARTIAL   (1 << 5)
#define MAGSAVE_DEATH   (1 << 6)

#define MAGCOMP_DIVINE_FOCUS  (1 << 0)
#define MAGCOMP_EXP_COST  (1 << 1)
#define MAGCOMP_FOCUS   (1 << 2)
#define MAGCOMP_MATERIAL  (1 << 3)
#define MAGCOMP_SOMATIC   (1 << 4)
#define MAGCOMP_VERBAL    (1 << 5)


#define TYPE_UNDEFINED      (-1)
#define SPELL_RESERVED_DBC    0  /* SKILL NUMBER ZERO -- RESERVED */

#define SPELL_TYPE_ARCANE   0
#define SPELL_TYPE_DIVINE   1

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define SPELL_MAGE_ARMOR    1
#define SPELL_TELEPORT      2
#define SPELL_BLESS     3
#define SPELL_BLINDNESS     4
#define SPELL_BURNING_HANDS   5
#define SPELL_CALL_LIGHTNING    6
#define SPELL_CHARM     7
#define SPELL_CHILL_TOUCH   8
#define SPELL_COLOR_SPRAY   10
#define SPELL_CONTROL_WEATHER   11
#define SPELL_CREATE_FOOD   12
#define SPELL_CREATE_WATER    13
#define SPELL_REMOVE_BLINDNESS    14
#define SPELL_CURE_CRITIC   15
#define SPELL_CURE_LIGHT    16
#define SPELL_BESTOW_CURSE    17
#define SPELL_DETECT_ALIGN    18
#define SPELL_SEE_INVIS     19
#define SPELL_DETECT_MAGIC    20
#define SPELL_DETECT_POISON   21
#define SPELL_DISPEL_EVIL   22
#define SPELL_EARTHQUAKE    23
#define SPELL_ENCHANT_WEAPON    24
#define SPELL_ENERGY_DRAIN    25
#define SPELL_FIREBALL      26
#define SPELL_HARM      27
#define SPELL_HEAL      28
#define SPELL_INVISIBLE     29
#define SPELL_LIGHTNING_BOLT    30
#define SPELL_LOCATE_OBJECT   31
#define SPELL_MAGIC_MISSILE   32
#define SPELL_POISON      33
#define SPELL_PROT_FROM_EVIL    34
#define SPELL_REMOVE_CURSE    35
#define SPELL_SANCTUARY     36
#define SPELL_SHOCKING_GRASP    37
#define SPELL_SLEEP     38
#define SPELL_BULL_STRENGTH   39
#define SPELL_SUMMON      40
#define SPELL_VENTRILOQUATE   41
#define SPELL_WORD_OF_RECALL    42
#define SPELL_NEUTRALIZE_POISON   43
#define SPELL_SENSE_LIFE    44
#define SPELL_ANIMATE_DEAD    45
#define SPELL_DISPEL_GOOD   46
#define SPELL_GROUP_ARMOR   47
#define SPELL_MASS_HEAL     48
#define SPELL_GROUP_RECALL    49
#define SPELL_DARKVISION    50
#define SPELL_WATERWALK     51
#define SPELL_PORTAL      52
#define SPELL_PARALYZE      53
#define SPELL_INFLICT_LIGHT   54
#define SPELL_INFLICT_CRITIC    55
#define SPELL_IDENTIFY      56
#define SPELL_RACIAL_NATURAL_ARMOR 57
#define ABIL_TURNING      58
#define ABIL_LAY_HANDS      59
#define SPELL_RESISTANCE    60
#define SPELL_ACID_SPLASH   61
#define SPELL_DAZE      62
#define SPELL_FLARE     63
#define SPELL_RAY_OF_FROST    64
#define SPELL_DISRUPT_UNDEAD    65
#define SPELL_LESSER_GLOBE_OF_INVUL 66
#define SPELL_COMPREHEND_LANGUAGES 67
#define SPELL_MINOR_CREATION    68
#define SPELL_SUMMON_MONSTER_I    69
#define SPELL_SUMMON_MONSTER_II   70
#define SPELL_SUMMON_MONSTER_III  71
#define SPELL_SUMMON_MONSTER_IV   72
#define SPELL_SUMMON_MONSTER_V    73
#define SPELL_SUMMON_MONSTER_VI   74
#define SPELL_SUMMON_MONSTER_VII  75
#define SPELL_SUMMON_MONSTER_VIII 76
#define SPELL_SUMMON_MONSTER_IX   77
#define SPELL_FIRE_SHIELD   78
#define SPELL_ICE_STORM     79
#define SPELL_SHOUT     80
#define SPELL_FEAR      81
#define SPELL_CLOUDKILL     82
#define SPELL_MAJOR_CREATION    83
#define SPELL_HOLD_MONSTER    84
#define SPELL_CONE_OF_COLD    85
#define SPELL_ANIMAL_GROWTH   86
#define SPELL_BALEFUL_POLYMORPH   87
#define SPELL_PASSWALL      88
#define SPELL_CURE_MINOR    89
#define SPELL_MINOR_REFRESH   90
#define SPELL_LIGHT_REFRESH   91
#define SPELL_COURAGE     92
#define SPELL_AURA_OF_COURAGE   93
#define SPELL_AURA_OF_GOOD    94
#define SPELL_SLEEP_SINGLE    95
#define SPELL_AFF_STUNNED   96
#define SPELL_DIVINE_FAVOR    97
#define SPELL_SHIELD_OF_FAITH   98
#define SPELL_MAGIC_WEAPON    99
#define SPELL_VIRTUE      100
#define SPELL_CURE_MODERATE   101
#define SPELL_CURE_SERIOUS    102
#define SPELL_BLESS_OBJECT    103
#define SPELL_BLESS_SINGLE    104
#define SPELL_MODERATE_REFRESH    105
#define SPELL_SERIOUS_REFRESH   106
#define SPELL_CRITICAL_REFRESH    107
#define SPELL_MASS_REJUVENATE   108
#define SPELL_LESSER_MASS_REJUVENATE  109
#define SPELL_MINOR_MASS_REJUVENATE 110
#define SPELL_REJUVENATE    111
#define SPELL_TRIP      112
#define SPELL_TRIPPING      113
#define ABIL_SMITE_EVIL     114
#define SPELL_AFF_TAUNTED   115
#define SPELL_AFF_NO_DEITY  116
#define SPELL_AFF_RAGE      117
#define SPELL_AFF_TAUNTING  118
#define SPELL_AFF_FATIGUED	119
#define SPELL_AFF_STRENGTH_OF_HONOR 120
#define SPELL_BEARS_ENDURANCE 121
#define SPELL_AFF_TURN_UNDEAD 122
#define SPELL_DEATH_KNELL 123
#define SPELL_DELAY_POISON 124
#define SPELL_EAGLES_SPLENDOR 125
#define SPELL_TONGUES 126
#define SPELL_INFLICT_MODERATE       127
#define SPELL_OWLS_WISDOM 128
#define SPELL_FLY                    129
#define SPELL_LEVITATE               130
#define SPELL_SOUND_BURST 131
#define SPELL_UNDETECTABLE_ALIGNMENT 132
#define SPELL_POISON_TIMER 133
#define SPELL_ACID_ARROW 134
#define SPELL_BLUR                    135
#define SPELL_MASS_CHARM             136
#define SPELL_THORNFLESH             137
#define SPELL_CRIPPLE                138
#define SPELL_LEGEND_LORE            139
#define SPELL_MINOR_SANCTUARY        140
#define SPELL_HEALING_LIGHT          141
#define SPELL_PRAYER                 142
#define SPELL_BARKSKIN               143
#define SPELL_STONESKIN              144
#define SPELL_HARMFUL_WRATH          145
#define SPELL_CURE_DEAFNESS          146
#define SPELL_STATUE                 147
#define SPELL_MIRROR_IMAGE           148
#define SPELL_FLIGHT                 149
#define SPELL_WEAKNESS               150
#define SPELL_DEAFENING_WIND         151
#define SPELL_CURE_DISEASE           152
#define SPELL_REMOVE_PARALYSIS       153
#define SPELL_RESTORATION            154
#define SPELL_REGENERATION           155
#define SPELL_HASTE                  156
#define SPELL_DISEASE                157
#define SPELL_FAMILIAR_BONUS         158
#define SPELL_SOUL_DRAIN             159
#define SPELL_WITHER                 160
#define SPELL_SLOW                   161
#define SPELL_GATE                   162
#define SPELL_ELEMENTAL_STORM        163
#define SPELL_CREATE_SPRING          164
#define SPELL_PROT_FROM_GOOD         165
#define SPELL_PROT_FROM_UNDEAD       166
#define SPELL_PROT_FROM_FIRE         167
#define SPELL_PROT_FROM_FROST        168
#define SPELL_PROT_FROM_ELEMENTS     169
#define SPELL_IMPERVIOUS_MIND        170
#define SPELL_CALM                   171
#define SPELL_CLOAK_OF_FEAR          172
#define SPELL_INSPIRE_FEAR           173
#define SPELL_HEROES_FEAST           174
#define SPELL_BRAVERY                175
#define SPELL_GREATER_ELEMENTAL      176
#define SPELL_BLADE_BARRIER          177
#define SPELL_FAERIE_FIRE            178
#define SPELL_SUNRAY                 179
#define SPELL_MOONBEAM               180
#define SPELL_DISPEL_MAGIC           181
#define SPELL_LIGHT                  182
#define SPELL_DARKNESS               183
#define SPELL_SILENCE                184
#define SPELL_SPHERE_SILENCE         185
#define SPELL_INSPIRE_AWE            186
#define SPELL_CURE_SEVERE            187
#define SPELL_CAUSE_LIGHT            188
#define SPELL_CAUSE_MODERATE         189
#define SPELL_CAUSE_SEVERE          190
#define SPELL_CAUSE_CRITIC          191
#define SPELL_GOODBERRY             192
#define SPELL_MAGIC_VESTMENT        193
#define SPELL_FREE_ACTION           194
#define SPELL_MAGICAL_STONE         195
#define SPELL_SHILLELAGH            196
#define SPELL_SPIRIT_HAMMER         197
#define SPELL_FLAMESTRIKE           198
#define SPELL_AID                   199
#define SPELL_CALL_ANIMAL_SPIRIT    200
#define SPELL_SUMMON_INSECTS        201
#define SPELL_ANIMAL_SUMMONING      202
#define SPELL_ANIMAL_SUMMONING_II   203
#define SPELL_ANIMAL_SUMMONING_III  204
#define SPELL_CREEPING_DOOM         205
#define SPELL_INSECT_PLAGUE         206
#define SPELL_RAINBOW               207
#define SPELL_ENTANGLE              208
#define SPELL_THORNSHIELD           209
#define SPELL_STICKS_TO_SNAKES      210
#define SPELL_AERIAL_SERVANT        211
#define SPELL_SUMMON_GUARD          212
#define SPELL_DUST_DEVIL            213
#define SPELL_FLAME_BLADE           214
#define SPELL_WATER_BREATHING       215
#define SPELL_CONJURE_ELEMENTAL     216
#define SPELL_WIND_WALK             217
#define SPELL_PHANTASMAL_SNAKE      218
#define SPELL_CONTROL_UNDEAD        219
#define SPELL_BREATH_OF_LIFE        220
#define SPELL_BLACK_PLAGUE          221
#define SPELL_REFLECTING_POOL       222
#define SPELL_RECOLLECTION          223
#define SPELL_REMOVE_FEAR           224
#define SPELL_DIVINE_WRATH          225
#define SPELL_HOLY_WORD             226
#define SPELL_MYSTIC_SPIRIT         227
#define SPELL_STORM_SUMMONING       228
#define SPELL_DEHYDRATION           229
#define SPELL_MIRE                  230
#define SPELL_ABJURE                231
#define SPELL_ADAMANT_MACE          232
#define SPELL_HOLD_PERSON           233
#define SPELL_WEB                   234
#define SPELL_GUST_OF_WIND          235
#define SPELL_MINOR_GLOBE           236
#define SPELL_MAJOR_GLOBE           237
#define SPELL_VAMPIRIC_TOUCH        238
#define SPELL_DANCING_SWORD         239
#define SPELL_SCRY                  240
#define SPELL_CHAIN_LIGHTNING       241
#define SPELL_TELEPORT_OBJECT       242
#define SPELL_ARROW_OF_BONE         243
#define SPELL_REVEAL_ILLUSION       244
#define SPELL_POLYMORPH             245
#define SPELL_BLACKMANTLE           246
#define SPELL_BLINK                 247
#define SPELL_PRISMATIC_SPRAY       248
#define SPELL_METEOR_SWARM          249
#define SPELL_MAGICAL_SUSCEPT       250
#define SPELL_ANTIMAGIC_AURA        251
#define SPELL_DECEPTIVE_IMAGERY     252
#define SPELL_SPELL_TURNING         253
#define SPELL_RECHARGE_ITEM         254
#define SPELL_VAMPIRIC_AURA         255
#define SPELL_REFRESH               256
#define SPELL_POWER_WORD_KILL       257
#define SPELL_CONCEAL_ALIGN         258
#define SPELL_FOX_CUNNING           259
#define SPELL_CAT_GRACE             260
#define SPELL_BLESS_WEAPON          261
#define SPELL_DAYLIGHT              262
#define SPELL_MAGIC_CIRCLE_AGAINST_EVIL 263
#define SPELL_GREATER_MAGIC_WEAPON  264
#define SPELL_HEAL_MOUNT            265
#define SPELL_FLAME_WEAPON          266
#define SPELL_MENDING               267
#define SPELL_CHARM_ANIMAL          268
#define SPELL_CALM_ANIMAL           269
#define SPELL_MAGIC_STONE           270
#define SPELL_MAGIC_FANG            271
#define SPELL_LONGSTRIDER           272
#define SPELL_ATHLETICS             273
#define SPELL_OBSCURING_MIST        274
#define SPELL_PASS_WITHOUT_TRACE    275
#define SPELL_SUMMON_NATURE_I       276
#define SPELL_SUMMON_NATURE_II      277
#define SPELL_SUMMON_NATURE_III     278
#define SPELL_SUMMON_NATURE_IV      279
#define SPELL_SUMMON_NATURE_V       280
#define SPELL_SUMMON_NATURE_VI      281
#define SPELL_SUMMON_NATURE_VII     282
#define SPELL_SUMMON_NATURE_VIII    283
#define SPELL_SUMMON_NATURE_IX      284
#define SPELL_FLAME_ARROW           285
#define SPELL_ENTANGLED             286
#define SPELL_CALL_LIGHTNING_BOLT   287
#define SPELL_MASS_HARM             288
#define SPELL_INFLICT_SERIOUS       289
#define SPELL_INFLICT_MINOR         290
#define SPELL_COUNTERSONG           291
#define SPELL_INSPIRE_COURAGE       292
#define SPELL_INSPIRE_GREATNESS     293
#define SPELL_INSPIRE_HEROICS       294
#define SPELL_INSPIRE_COMPETENCE    295
#define SPELL_DISINTIGRATE          296
#define SPELL_HORRID_WILTING        297
#define SPELL_DELAYED_BLAST_FIREBALL 298
#define SPELL_METEOR_SWARM_AREA     299
#define SPELL_SLAY_LIVING           300
#define SPELL_RAISE_DEAD            301
#define SPELL_RESURRECTION          302
#define SPELL_TRUE_RESURRECTION     303
#define SPELL_REINCARNATE           304
#define SPELL_MISSILE_SWARM         305
#define SPELL_GREATER_MISSILE_SWARM 306
#define SPELL_DEATH_WARD            307
#define SPELL_BESTOW_CURSE_PENALTIES 308
#define SPELL_BESTOW_CURSE_DAZE     309
#define SPELL_MASS_BULLS_STRENGTH   310
#define SPELL_MASS_BEARS_ENDURANCE  311
#define SPELL_MASS_CATS_GRACE       312
#define SPELL_MASS_FOXS_CUNNING     313
#define SPELL_MASS_OWLS_WISDOM      314
#define SPELL_MASS_EAGLES_SPLENDOR  315
#define SPELL_MASS_AID              316
#define SPELL_GREATER_STONESKIN     317
#define SPELL_PREMONITION           319
#define SPELL_FINGER_OF_DEATH       320
#define SPELL_WAIL_OF_THE_BANSHEE   321
#define SPELL_GREATER_INVISIBILITY  322
#define SPELL_DEEP_SLUMBER          323
#define SPELL_ENLARGE_PERSON        324
#define SPELL_MASS_ENLARGE_PERSON   325
#define SPELL_FREEDOM_OF_MOVEMENT   326
#define SPELL_CLAN_RECALL           327
#define SPELL_FIRE_STORM            328
#define SPELL_DRAGON_KNIGHT         329
#define SPELL_EPIC_MAGE_ARMOR       330
#define SPELL_FLOATING_DISC         331
#define SPELL_KEEN_EDGE             332
#define SPELL_WEAPON_OF_IMPACT      333
#define SPELL_WISH                  334
#define SPELL_MIRACLE               335
#define SPELL_SUMMON_UNDEAD         336
#define SPELL_SUMMON_GREATER_UNDEAD 337
#define SPELL_GREATER_MAGIC_FANG    338
#define SPELL_FLAMING_SPHERE        339
#define SPELL_SCORCHING_RAY         340
#define SPELL_WISH_RING             341
#define SPELL_BLEEDING_DAMAGE       342
#define ABILITY_CALL_MOUNT          343
#define SPELL_SICKENED              344
#define SPELL_RESIST_SICKENED       345
#define SPELL_RESIST_FEAR           346
#define SPELL_LEARNING              347
#define SPELL_IMPROVED_LEARNING     348
#define SPELL_GREATER_LEARNING      349
#define SPELL_EPIC_LEARNING         350
#define SPELL_ON_FIRE               351
#define SPELL_WHIRLWIND             352
#define SPELL_VORTEX                353
#define SPELL_ACMD_PILFER           354
#define ABILITY_CALL_COMPANION      355
#define SPELL_DRAGON_MOUNT_BREATH   356
#define SPELL_BATTLE_STRIKE         357
#define SPELL_DARK_RAGE             358
#define SPELL_FORCE_DISARM          359
#define SPELL_FORCE_GRIP            360
#define SPELL_FORCE_HEAL            361
#define SPELL_FORCE_LIGHTNING       362
#define SPELL_FORCE_STUN            363
#define SPELL_MOVE_OBJECT           364
#define SPELL_NEGATE_ENERGY         365
#define SPELL_FINANCIAL_EXPERT      366
#define SPELL_PVP_REWARD_TIMER      367
#define SPELL_THEORY_TO_PRACTICE    368
#define SPELL_SMITE                 369
#define SPELL_FEINT                 370
#define SPELL_CHALLENGE             371
#define SPELL_KICK                  372
#define SPELL_TAUNT                 373
#define SPELL_GOAD                  374
#define SPELL_INTIMIDATE            375
#define SPELL_DARK_HEALING          376
#define SPELL_BACTA_SPRAY           377
#define SPELL_BACTA_JET             378
#define SPELL_RECITE                379
#define SPELL_QUAFF                 380
#define SPELL_OVERCHARGE_SHOT       381
#define SPELL_EVASIVE_FIGHTING      382
#define SPELL_DOUBLE_BURST          383
#define SPELL_FORCE_SURGE           384
#define SPELL_FORCE_SLAM            385
#define SPELL_FORCE_PUSH            386
#define SPELL_HEAD_SHOT             387
#define SPELL_AIM                   388
#define SPELL_COVER_FIRE            389
#define SPELL_HUNTERS_TARGET        390
#define SPELL_FAMILIAR_FOE          391

#define TOP_SPELL           391
#define MAX_SPELLS          600
#define MAX_SPELL_LEVELS    9


#define SKILL_LANG_BASIC        601
#define SKILL_LANG_UNDERWORLD   602
#define SKILL_LANG_ANCIENT      603
#define SKILL_LANG_BINARY       604
#define SKILL_LANG_BOCCE        605
#define SKILL_LANG_GNOME        606
#define SKILL_LANG_BOTHESE      607
#define SKILL_LANG_CEREAN       608
#define SKILL_LANG_DOSH         609
#define SKILL_LANG_DURESE       610
#define SKILL_LANG_EWOKESE      611
#define SKILL_LANG_GAMORREAN    612
#define SKILL_LANG_GUNGANESE    613
#define SKILL_LANG_HIGH_GALACTIC 614
#define SKILL_LANG_HUTTESE      615
#define SKILL_LANG_ITHORESE     616
#define SKILL_LANG_JAWA         617
#define SKILL_LANG_KEL_DOR      618
#define SKILL_LANG_MON_CALAMARIAN 619
#define SKILL_LANG_QUARRENESE   620
#define SKILL_LANG_RODESE       621
#define SKILL_LANG_RYL          622
#define SKILL_LANG_SHYRIIWOOK   623
#define SKILL_LANG_SULLUSTESE   624
#define SKILL_LANG_ZABRAK       625
#define SKILL_LANG_CHEUNH       626

#define SKILL_LANG_COMMON       601
#define SKILL_LANG_THIEVES_CANT 602
#define SKILL_LANG_DRUIDIC      603
#define SKILL_LANG_ABYSSAL      604
#define SKILL_LANG_ELVEN        605
#define SKILL_LANG_GNOME        606
#define SKILL_LANG_DWARVEN      607
#define SKILL_LANG_CELESTIAL    608
#define SKILL_LANG_DRACONIC     609
#define SKILL_LANG_GULLYTALK    610
#define SKILL_LANG_ORCISH       610
#define SKILL_LANG_KENDERSPEAK  611
#define SKILL_LANG_HALFLING     611
#define SKILL_LANG_GOBLIN       612
#define SKILL_LANG_KOTHIAN      613
#define SKILL_LANG_CHONDATHAN   613
#define SKILL_LANG_GIANT        614
#define SKILL_LANG_KOBOLD       615
#define SKILL_LANG_SOLAMNIC     616
#define SKILL_LANG_ICE_BARBARIAN 616
#define SKILL_LANG_ERGOT        617
#define SKILL_LANG_MIDANI       617
#define SKILL_LANG_ISTARIAN     618
#define SKILL_LANG_CHULTAN      618
#define SKILL_LANG_BALIFORIAN   619
#define SKILL_LANG_TUIGAN       619
#define SKILL_LANG_KHAROLISIAN  620
#define SKILL_LANG_LANTANESE    620
#define SKILL_LANG_NORDMAARIAN  621
#define SKILL_LANG_MULHORANDI   621
#define SKILL_LANG_ICESPEAK     622
#define SKILL_LANG_RASHEMI      622
#define SKILL_LANG_BARBARIAN    623
#define SKILL_LANG_UNDERCOMMON  623

#define SKILL_LANG_LOW SKILL_LANG_COMMON
#define SKILL_LANG_HIGH SKILL_LANG_CHEUNH

#define MIN_LANGUAGES     SKILL_LANG_LOW
#define MAX_LANGUAGES     SKILL_LANG_HIGH

#define MAX_LANGUAGES_FR     SKILL_LANG_CHEUNH
#define MAX_LANGUAGES_DL_AOL MAX_LANGUAGES_FR

#define SKILL_WP_UNARMED    650 /* Barehanded weapon group        */


#define SPELL_FIRE_BREATH   651
#define SPELL_GAS_BREATH    652
#define SPELL_FROST_BREATH  653
#define SPELL_ACID_BREATH   654
#define SPELL_LIGHTNING_BREATH 655
#define SKILL_KICK 656
#define SPELL_SKILL_HEAL_USED 657
#define SPELL_BARD_SONGS    658
#define SPELL_NATURAL_ARMOR_INCREASE 659
#define SPELL_BARD_SPELLS 660
#define SPELL_SORCERER_SPELLS 661
#define SPELL_NO_DEATH_ATTACK 662
#define SPELL_ARMOR_SKIN 663
#define SPELL_ASSASSIN_SPELLS 664
#define SPELL_AFF_DEFENSIVE_STANCE 665
#define SPELL_AFF_CHALLENGED 666
#define SPELL_AFF_INTIMIDATED 667
#define SPELL_FEAT_DIVINE_MIGHT 668
#define SPELL_FEAT_DIVINE_SHIELD 669
#define SPELL_FEAT_DIVINE_VENGEANCE 670
#define SPELL_AFF_DISARMED 671
#define SPELL_DEATH_ATTACK 672
#define SPELL_SKILL_CAMP_USED 673
#define SPELL_BREATH_WEAPON 674
#define SPELL_FAVORED_SOUL_SPELLS 675
#define SPELL_EPIC_SPELLS 676
#define SPELL_TOUCH_OF_UNDEATH 677
#define SPELL_WILD_SHAPE 678
#define SPELL_WILD_SHAPE_ELEMENTAL 679
/*
 * to make an affect induced by dg_affect look correct on 'stat' we need
 * to define it with a 'spellname'.
 */
#define SPELL_DG_AFFECT     699

/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     700
#define TYPE_STUN                    701
#define TYPE_LASH                    702
#define TYPE_SLASH                   703
#define TYPE_BITE                    704
#define TYPE_BLUDGEON                705
#define TYPE_SHOOT                   706
#define TYPE_CLEAVE                  707
#define TYPE_CLAW                    708
#define TYPE_FREEZE                  709
#define TYPE_THRASH                  710
#define TYPE_PIERCE                  711
#define TYPE_BLAST                   712
#define TYPE_PUNCH                   713
#define TYPE_FLAMES                  714
#define TYPE_GORE                    715
#define TYPE_BATTER                  716
/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING               799

#define SKILL_ACROBATICS      	800
#define SKILL_ATHLETICS      	801
#define SKILL_DECEPTION         802
#define SKILL_ENDURANCE   	803
#define SKILL_GATHER_INFORMATION 804
#define SKILL_INITIATIVE     	805
#define SKILL_TACTICS           806
#define SKILL_KNOWLEDGE   	807
#define SKILL_MECHANICS         808
#define SKILL_PERCEPTION      	809
#define SKILL_PERSUASION        810
#define SKILL_PILOT      	811
#define SKILL_RIDE 	   	812
#define SKILL_SURVIVAL     	813
#define SKILL_TREAT_INJURY     	814
#define SKILL_USE_COMPUTER     	815
#define SKILL_USE_THE_FORCE    	816
#define SKILL_CRAFTING_THEORY   817
#define SKILL_ARMORTECH         818
#define SKILL_WEAPONTECH        819
#define SKILL_TINKERING         820
#define SKILL_STEALTH      	821
#define SKILL_HARVESTING        822
#define SKILL_ROBOTICS          823

#define SKILL_LOW_SKILL SKILL_ACROBATICS
#define SKILL_HIGH_SKILL SKILL_ROBOTICS

#define SKILL_FORESTING         818
#define SKILL_MINING            819
#define SKILL_FARMING           830
#define SKILL_BLACKSMITHING     829
#define SKILL_TAILORING         822
#define SKILL_TANNING           823
#define SKILL_GOLDSMITHING      824
#define SKILL_WOODWORKING       825
#define SKILL_COOKING           826
#define SKILL_HUNTING           827
#define SKILL_HERBALISM         828

#define SKILL_BALANCE           800
#define SKILL_USE_ROPE          800
#define SKILL_APPRAISE		810
#define SKILL_USE_MAGIC_DEVICE  816
#define SKILL_FORGERY           807
#define SKILL_SPELLCRAFT        816
#define SKILL_HIDE              813
#define SKILL_MOVE_SILENTLY     813
#define SKILL_SPOT              809
#define SKILL_LISTEN            809
#define SKILL_SEARCH            809
#define SKILL_LORE              807
#define SKILL_OPEN_LOCK         815
#define SKILL_HANDLE_ANIMAL     812
#define SKILL_SENSE_MOTIVE      802
#define SKILL_BLUFF             802
#define SKILL_SLEIGHT_OF_HAND   802
#define SKILL_INTIMIDATE        810
#define SKILL_DIPLOMACY         810
#define SKILL_HEAL              814
#define SKILL_DISGUISE          802
#define SKILL_TUMBLE		800
#define SKILL_DISABLE_DEVICE    808
#define SKILL_DECIPHER_SCRIPT   807
#define SKILL_ESCAPE_ARTIST     800
#define SKILL_PERFORM           810
#define SKILL_COMBAT_TACTICS    806
#define SKILL_CONCENTRATION     816

#define ART_STUNNING_FIST   	995
#define ART_WHOLENESS_OF_BODY 	996
#define ART_ABUNDANT_STEP   	997
#define ART_QUIVERING_PALM    	998
#define ART_EMPTY_BODY      	999

#define SAVING_FORTITUDE  0
#define SAVING_REFLEX   1
#define SAVING_WILL   2

#define SAVING_OBJ_IMPACT       0
#define SAVING_OBJ_HEAT         1
#define SAVING_OBJ_COLD         2
#define SAVING_OBJ_BREATH       3
#define SAVING_OBJ_SPELL        4

#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF    (1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP (1 << 10)
#define TAR_OBJ_TEMP    (1 << 11)

#define SKTYPE_NONE   0
#define SKTYPE_SPELL    (1 << 0)
#define SKTYPE_SKILL    (1 << 1)
#define SKTYPE_LANG   (1 << 2)
#define SKTYPE_WEAPON   (1 << 3)
#define SKTYPE_ART    (1 << 4)

#define SKFLAG_NEEDTRAIN  (1 << 0) /* Disallow use of 0 skill with only stat mod */
#define SKFLAG_STRMOD   (1 << 1)
#define SKFLAG_DEXMOD   (1 << 2)
#define SKFLAG_CONMOD   (1 << 3)
#define SKFLAG_INTMOD   (1 << 4)
#define SKFLAG_WISMOD   (1 << 5)
#define SKFLAG_CHAMOD   (1 << 6)
#define SKFLAG_ARMORBAD (1 << 7)
#define SKFLAG_ARMORALL (1 << 8)
#define SKFLAG_CRAFT    (1 << 9)

#define SKLEARN_CANT    0 /* This class can't learn this skill */
#define SKLEARN_CROSSCLASS  1 /* Cross-class skill for this class */
#define SKLEARN_CLASS   2 /* Class skill for this class */
#define SKLEARN_BOOL    3 /* Skill is known or not */

#define MEM_TYPE_MAGE       1
#define MEM_TYPE_CLERIC     2
#define MEM_TYPE_PALADIN    3
#define MEM_TYPE_DRUID      4
#define MEM_TYPE_RANGER     5
#define MEM_TYPE_BARD       6
#define MEM_TYPE_BLACKGUARD 7
#define MEM_TYPE_ASSASSIN   8
#define MEM_TYPE_JEDI       8
#define MEM_TYPE_SORCERER   9
#define MEM_TYPE_MUSIC      10
#define MEM_TYPE_ART        11
#define MEM_TYPE_INNATE     12
#define MEM_TYPE_FAVORED_SOUL 13

#define MEM_TYPE_MIN      MEM_TYPE_MAGE
#define MEM_TYPE_MAX      MEM_TYPE_FAVORED_SOUL

struct spell_info_type {
   byte min_position; /* Position for caster   */
   int mana_min;        /* Min amount of mana used by a spell (highest lev) */
   int mana_max;  /* Max amount of mana used by a spell (lowest lev) */
   int mana_change;     /* Change in mana used by spell from lev to lev */
   int ki_min;    /* Min amount of mana used by a spell (highest lev) */
   int ki_max;    /* Max amount of mana used by a spell (lowest lev) */
   int ki_change; /* Change in mana used by spell from lev to lev */

   int min_level[NUM_CLASSES];
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;  /* Input size not limited. Originates from string constants. */
   const char *wear_off_msg;  /* Input size not limited. Originates from string constants. */
   int race_can_learn[NUM_RACES];
   int skilltype;       /* Is it a spell, skill, art, feat, or what? used as bitvector */
   int flags;
   int save_flags;
   int comp_flags;
   byte can_learn_skill[NUM_CLASSES];
   int spell_level;
   int school;
   int domain;
   int hate;
   int class_level[NUM_CLASSES];
   int bard_song;
   int perform_rank;
   int bard_feat;
   int epic_dc;
   int elemental;
   int artisan_type;
   const char *alignment;
};

extern struct spell_info_type spell_info[];

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : if fighting, and no argument, select tar_char as self
   bit 8 : if fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : if no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   const char *singular;
   const char *plural;
};


#define ASPELL(spellname) \
void  spellname(int level, struct char_data *ch, \
      struct char_data *victim, struct obj_data *obj, const char *arg)

#define MANUAL_SPELL(spellname) spellname(level, caster, cvict, ovict, arg);

ASPELL(spell_battle_strike);
ASPELL(spell_dark_rage);
ASPELL(spell_force_grip);
ASPELL(spell_force_disarm);
ASPELL(spell_force_heal);
ASPELL(spell_force_lightning);
ASPELL(spell_force_stun);
ASPELL(spell_force_surge);
ASPELL(spell_force_slam);
ASPELL(spell_force_push);
ASPELL(spell_move_object);
ASPELL(spell_dark_healing);

ASPELL(spell_clan_recall);
ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_charm_animal);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_portal);
ASPELL(spell_raise_dead);
ASPELL(spell_resurrection);
ASPELL(spell_true_resurrection);
ASPELL(art_abundant_step);
ASPELL(spell_dispel_magic);
ASPELL(spell_wish);
ASPELL(spell_wish_ring);
ASPELL(spell_miracle);

/* basic magic calling functions */

int find_skill_num(char *name, int sktype);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum);

void mag_loop(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum);

void mag_groups(int level, struct char_data *ch, int spellnum);

void mag_masses(int level, struct char_data *ch, int spellnum);

void mag_areas(int level, struct char_data *ch, int spellnum);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, const char *arg);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 int spellnum);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum);

void mag_creations(int level, struct char_data *ch, int spellnum);

void mag_affectsv(int level, struct char_data *ch, struct char_data *victim,
  int spellnum);

int call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int spellnum, int level, int casttype, const char *arg);

void  mag_objectmagic(struct char_data *ch, struct obj_data *obj,
      char *argument);

int cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, int spellnum, const char *arg);

int find_savetype(int spellnum);
int calc_spell_dc(struct char_data *ch, int spellnum);
int mag_newsaves(int savetype, struct char_data *ch, struct char_data *victim, int spellnum, int dc);

/* other prototypes */
void skill_level(int spell, int chclass, int level);
void skill_race_class(int spell, int race, int learntype);
void init_skill_classes(void);
void init_skill_race_classes(void);
void skill_class(int skill, int chclass, int learntype);
const char *skill_name(int num);
int roll_skill(const struct char_data *ch, int snum);
int roll_resisted(const struct char_data *actor, int sact, const struct char_data *resistor, int sres);
int skill_type(int skill);
