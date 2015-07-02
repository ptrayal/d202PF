/***********************************************************************
** FEATS.H                                                            **
** Header file for the Gates of Krynn Feat System.                    **
** Initial code by Paladine (Stephen Squires)                         **
** Created Thursday, September 5, 2002                                **
**                                                                    **
***********************************************************************/

#ifndef FEATS_H_
#define FEATS_H_


/* Functions defined in feats.c */
int is_proficient_with_armor(const struct char_data *ch, int armor_type);
int is_proficient_with_weapon(const struct char_data *ch, int weapon_type);
int find_feat_num(char *name);
int feat_to_subfeat(int feat);
extern struct feat_info feat_list[];

/* Feats defined below up to MAX_FEATS */

#define FEAT_UNDEFINED        0
#define FEAT_ALERTNESS        1
#define FEAT_ARMOR_PROFICIENCY_HEAVY    3
#define FEAT_ARMOR_PROFICIENCY_LIGHT    4
#define FEAT_ARMOR_PROFICIENCY_MEDIUM   5
#define FEAT_BLIND_FIGHT      6
#define FEAT_BREW_POTION      7
#define FEAT_CLEAVE       8
#define FEAT_COMBAT_CASTING     9
#define FEAT_COMBAT_REFLEXES      10
#define FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR 11
#define FEAT_CRAFT_ROD        12
#define FEAT_CRAFT_STAFF      13
#define FEAT_CRAFT_WAND       14
#define FEAT_CRAFT_WONDEROUS_ITEM   15
#define FEAT_DEFLECT_ARROWS     16
#define FEAT_DODGE        17
#define FEAT_EMPOWER_SPELL      18
#define FEAT_ENDURANCE        19
#define FEAT_ENLARGE_SPELL      20
#define FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD 21
#define FEAT_EXPERTISE        22
#define FEAT_EXTEND_SPELL     23
#define FEAT_EXTRA_TURNING      24
#define FEAT_FAR_SHOT       25
#define FEAT_FORGE_RING       26
#define FEAT_GREAT_CLEAVE     27
#define FEAT_GREAT_FORTITUDE      28
#define FEAT_HEIGHTEN_SPELL     29
#define FEAT_IMPROVED_BULL_RUSH     30
#define FEAT_IMPROVED_CRITICAL      31
#define FEAT_RAGE                       32
#define FEAT_FAST_MOVEMENT              33
#define FEAT_LAYHANDS       34
#define FEAT_AURA_OF_GOOD     35
#define FEAT_AURA_OF_COURAGE      36
#define FEAT_DIVINE_GRACE     37
#define FEAT_SMITE_EVIL       38
#define FEAT_REMOVE_DISEASE     39
#define FEAT_DIVINE_HEALTH      40
#define FEAT_TURN_UNDEAD      41
#define FEAT_DETECT_EVIL      42
#define FEAT_HEROIC_INITIATIVE 43
#define FEAT_IMPROVED_REACTION 44
#define FEAT_ENHANCED_MOBILITY 45
#define FEAT_GRACE							46
#define FEAT_PRECISE_STRIKE			47
#define FEAT_ACROBATIC_CHARGE   48
#define FEAT_ELABORATE_PARRY    49
#define FEAT_DAMAGE_REDUCTION   50
#define FEAT_GREATER_RAGE				51
#define FEAT_MIGHTY_RAGE				52
#define FEAT_TIRELESS_RAGE			53
#define FEAT_ARMORED_MOBILITY   54
#define FEAT_CROWN_OF_KNIGHTHOOD 55
#define FEAT_MIGHT_OF_HONOR     56
#define FEAT_SOUL_OF_KNIGHTHOOD 57
#define FEAT_RALLYING_CRY      58
#define FEAT_LEADERSHIP_BONUS 60
#define FEAT_IMPROVED_DISARM      61
#define FEAT_IMPROVED_INITIATIVE    62
#define FEAT_IMPROVED_TRIP      63
#define FEAT_IMPROVED_TWO_WEAPON_FIGHTING 64
#define FEAT_IMPROVED_UNARMED_STRIKE    65
#define FEAT_IRON_WILL        66
#define FEAT_LEADERSHIP       67
#define FEAT_LIGHTNING_REFLEXES     68
#define FEAT_MARTIAL_WEAPON_PROFICIENCY   69
#define FEAT_MAXIMIZE_SPELL     70
#define FEAT_MOBILITY       71
#define FEAT_MOUNTED_ARCHERY      72
#define FEAT_MOUNTED_COMBAT     73
#define FEAT_POINT_BLANK_SHOT     74
#define FEAT_POWER_ATTACK     75
#define FEAT_PRECISE_SHOT     76
#define FEAT_QUICK_DRAW       77
#define FEAT_QUICKEN_SPELL      78
#define FEAT_RAPID_SHOT       79
#define FEAT_RIDE_BY_ATTACK     80
#define FEAT_RUN        81
#define FEAT_SCRIBE_SCROLL      82
#define FEAT_SHIELD_PROFICIENCY     83
#define FEAT_SHOT_ON_THE_RUN      84
#define FEAT_SILENT_SPELL     85
#define FEAT_SIMPLE_WEAPON_PROFICIENCY    86
#define FEAT_SKILL_FOCUS      87
#define FEAT_SPELL_FOCUS      88
#define FEAT_SPELL_MASTERY      96
#define FEAT_SPELL_PENETRATION      97
#define FEAT_SPIRITED_CHARGE      98
#define FEAT_SPRING_ATTACK      99
#define FEAT_STILL_SPELL      100
#define FEAT_STUNNING_FIST      101
#define FEAT_SUNDER       102
#define FEAT_TOUGHNESS        103
#define FEAT_TRACK        104
#define FEAT_TRAMPLE        105
#define FEAT_TWO_WEAPON_FIGHTING    106
#define FEAT_WEAPON_FINESSE     107
#define FEAT_STRENGTH_OF_HONOR  108
#define FEAT_KNIGHTLY_COURAGE   109
#define FEAT_CANNY_DEFENSE      110
#define FEAT_HONORBOUND         111
#define FEAT_WISDOM_OF_THE_MEASURE 113
#define FEAT_FINAL_STAND				114
#define FEAT_KNIGHTHOODS_FLOWER 115
#define FEAT_INDOMITABLE_WILL		116
#define FEAT_UNCANNY_DODGE		117
#define FEAT_IMPROVED_UNCANNY_DODGE 118
#define FEAT_TRAP_SENSE 119
#define FEAT_UNARMED_STRIKE 120
#define FEAT_STILL_MIND 121
#define FEAT_KI_STRIKE 122
#define FEAT_SLOW_FALL 123
#define FEAT_PURITY_OF_BODY 124
#define FEAT_WHOLENESS_OF_BODY 125
#define FEAT_DIAMOND_BODY 126
#define FEAT_GREATER_FLURRY 127
#define FEAT_ABUNDANT_STEP 128
#define FEAT_DIAMOND_SOUL 129
#define FEAT_QUIVERING_PALM 130
#define FEAT_TIMELESS_BODY 131
#define FEAT_TONGUE_OF_THE_SUN_AND_MOON 132
#define FEAT_EMPTY_BODY 133
#define FEAT_PERFECT_SELF 134
#define FEAT_SUMMON_FAMILIAR 135
#define FEAT_TRAPFINDING 136
#define FEAT_WEAPON_FOCUS     137
#define FEAT_HONORABLE_WILL 138
#define FEAT_LOW_LIGHT_VISION 139
#define FEAT_FLURRY_OF_BLOWS 140
#define FEAT_IMPROVED_WEAPON_FINESSE 141
#define FEAT_DEMORALIZE 142
#define FEAT_UNBREAKABLE_WILL 143
#define FEAT_ONE_THOUGHT 144
#define FEAT_DETECT_GOOD 145
#define FEAT_SMITE_GOOD 146
#define FEAT_AURA_OF_EVIL 147
#define FEAT_DARK_BLESSING 148
#define FEAT_DISCERN_LIES 149
#define FEAT_FAVOR_OF_DARKNESS 150
#define FEAT_DIVINER 151
#define FEAT_READ_OMENS 152
#define FEAT_ARMORED_SPELLCASTING 153
#define FEAT_AURA_OF_TERROR 154
#define FEAT_WEAPON_TOUCH 155
#define FEAT_READ_PORTENTS 156
#define FEAT_COSMIC_UNDERSTANDING 157
#define FEAT_IMPROVED_TAUNTING 158
#define FEAT_IMPROVED_INSTIGATION 159
#define FEAT_IMPROVED_INTIMIDATION 160
#define FEAT_FAVORED_ENEMY 161
#define FEAT_WILD_EMPATHY 162
#define FEAT_COMBAT_STYLE 163
#define FEAT_ANIMAL_COMPANION 164
#define FEAT_IMPROVED_COMBAT_STYLE 165
#define FEAT_WOODLAND_STRIDE 166
#define FEAT_WEAPON_SPECIALIZATION    167
#define FEAT_SWIFT_TRACKER 168
#define FEAT_COMBAT_STYLE_MASTERY 169
#define FEAT_CAMOUFLAGE 170
#define FEAT_HIDE_IN_PLAIN_SIGHT 171
#define FEAT_NATURE_SENSE 172
#define FEAT_TRACKLESS_STEP 173
#define FEAT_RESIST_NATURES_LURE 174
#define FEAT_WILD_SHAPE 175
#define FEAT_WILD_SHAPE_LARGE 176
#define FEAT_VENOM_IMMUNITY 177
#define FEAT_WILD_SHAPE_TINY 178
#define FEAT_WILD_SHAPE_PLANT 179
#define FEAT_THOUSAND_FACES 180
#define FEAT_WILD_SHAPE_HUGE 181
#define FEAT_WILD_SHAPE_ELEMENTAL 182
#define FEAT_WILD_SHAPE_HUGE_ELEMENTAL 183
#define FEAT_FAVORED_ENEMY_AVAILABLE 184
#define FEAT_CALL_MOUNT 185
#define FEAT_ABLE_LEARNER 186
#define FEAT_EXTEND_RAGE 187
#define FEAT_EXTRA_RAGE 188
#define FEAT_FAST_HEALER 189
#define FEAT_DEFENSIVE_STANCE 190
#define FEAT_MOBILE_DEFENSE 191
#define FEAT_WEAPON_OF_CHOICE 192
#define FEAT_KI_DAMAGE 193
#define FEAT_INCREASED_MULTIPLIER 194
#define FEAT_KI_CRITICAL 195
#define FEAT_SUPERIOR_WEAPON_FOCUS 196
#define FEAT_WHIRLWIND_ATTACK     197
#define FEAT_WEAPON_PROFICIENCY_DRUID   198
#define FEAT_WEAPON_PROFICIENCY_ROGUE   199
#define FEAT_WEAPON_PROFICIENCY_MONK    200
#define FEAT_WEAPON_PROFICIENCY_WIZARD    201
#define FEAT_WEAPON_PROFICIENCY_ELF   202
#define FEAT_ARMOR_PROFICIENCY_SHIELD   203
#define FEAT_SNEAK_ATTACK     204
/* Evasion and improved evasion are not actually feats, but we treat them like feats
 * just to make it easier */
#define FEAT_EVASION        205
#define FEAT_IMPROVED_EVASION     206
#define FEAT_ACROBATIC        207
// #define FEAT_AGILE                208
#define FEAT_ANIMAL_AFFINITY      209
// #define FEAT_ATHLETIC       210
#define FEAT_AUGMENT_SUMMONING      211
#define FEAT_COMBAT_EXPERTISE     212
#define FEAT_DECEITFUL        213
#define FEAT_DEFT_HANDS       214
#define FEAT_DIEHARD        215
// #define FEAT_DILIGENT       216
#define FEAT_ESCHEW_MATERIALS     217
#define FEAT_EXOTIC_WEAPON_PROFICIENCY    218
#define FEAT_GREATER_SPELL_FOCUS    219
#define FEAT_GREATER_SPELL_PENETRATION    220
#define FEAT_GREATER_TWO_WEAPON_FIGHTING  221
#define FEAT_GREATER_WEAPON_FOCUS   222
#define FEAT_GREATER_WEAPON_SPECIALIZATION  223
#define FEAT_IMPROVED_COUNTERSPELL    224
#define FEAT_IMPROVED_FAMILIAR      225
#define FEAT_IMPROVED_FEINT     226
#define FEAT_IMPROVED_GRAPPLE     227
#define FEAT_IMPROVED_OVERRUN     228
#define FEAT_IMPROVED_PRECISE_SHOT    229
#define FEAT_IMPROVED_SHIELD_BASH   230
#define FEAT_IMPROVED_SUNDER      231
#define FEAT_IMPROVED_TURNING     232
// #define FEAT_INVESTIGATOR     233
#define FEAT_MAGICAL_APTITUDE     234
#define FEAT_MANYSHOT       235
#define FEAT_NATURAL_SPELL      236
// #define FEAT_NEGOTIATOR       237
// #define FEAT_NIMBLE_FINGERS     238
#define FEAT_PERSUASIVE       239
#define FEAT_RAPID_RELOAD     240
#define FEAT_SELF_SUFFICIENT      241
#define FEAT_STEALTHY       242
#define FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD 243
#define FEAT_TWO_WEAPON_DEFENSE     244
#define FEAT_WIDEN_SPELL      245
#define FEAT_CRIPPLING_STRIKE     246
#define FEAT_DEFENSIVE_ROLL     247
#define FEAT_OPPORTUNIST      248
#define FEAT_SKILL_MASTERY      249
#define FEAT_SLIPPERY_MIND      250
#define FEAT_NATURAL_ARMOR_INCREASE 251
#define FEAT_SNATCH_ARROWS      252
#define FEAT_STRENGTH_BOOST 253
#define FEAT_CLAWS_AND_BITE 254
#define FEAT_BREATH_WEAPON 255
#define FEAT_BLINDSENSE 256
#define FEAT_CONSTITUTION_BOOST 257
#define FEAT_INTELLIGENCE_BOOST 258
#define FEAT_WINGS 259
#define FEAT_DRAGON_APOTHEOSIS 260
#define FEAT_CHARISMA_BOOST 261
#define FEAT_SLEEP_PARALYSIS_IMMUNITY 262
#define FEAT_ELEMENTAL_IMMUNITY 263
#define FEAT_BARDIC_MUSIC 264
#define FEAT_BARDIC_KNOWLEDGE 265
#define FEAT_COUNTERSONG 266
#define FEAT_FASCINATE 267
#define FEAT_INSPIRE_COURAGE 268
#define FEAT_INSPIRE_COMPETENCE 269
#define FEAT_SUGGESTION 270
#define FEAT_INSPIRE_GREATNESS 271
#define FEAT_SONG_OF_FREEDOM 272
#define FEAT_INSPIRE_HEROICS 273
#define FEAT_MASS_SUGGESTION 274
#define FEAT_DARKVISION 275
#define FEAT_LINGERING_SONG 276
#define FEAT_EXTRA_MUSIC 277
#define FEAT_EXCEPTIONAL_TURNING 278
#define FEAT_IMPROVED_POWER_ATTACK 279
#define FEAT_MONKEY_GRIP 280
#define FEAT_FAST_CRAFTER 281
#define FEAT_PROFICIENT_CRAFTER 282
#define FEAT_PROFICIENT_HARVESTER 283
#define FEAT_SCAVENGE 284
#define FEAT_MASTERWORK_CRAFTING 285
#define FEAT_ELVEN_CRAFTING 286
#define FEAT_DWARVEN_CRAFTING 287
#define FEAT_BRANDING 288
#define FEAT_DRACONIC_CRAFTING 289
#define FEAT_LEARNED_CRAFTER 290
#define FEAT_POISON_USE 291
#define FEAT_DEATH_ATTACK 292
#define FEAT_POISON_SAVE_BONUS 293
#define FEAT_GREAT_STRENGTH 294
#define FEAT_GREAT_DEXTERITY 295
#define FEAT_GREAT_CONSTITUTION 296
#define FEAT_GREAT_WISDOM 297
#define FEAT_GREAT_INTELLIGENCE 298
#define FEAT_GREAT_CHARISMA 299
#define FEAT_ARMOR_SKIN 300
#define FEAT_FAST_HEALING 301
#define FEAT_FASTER_MEMORIZATION 302
#define FEAT_EMPOWERED_MAGIC 303
#define FEAT_ENHANCED_SPELL_DAMAGE 304
#define FEAT_ENHANCE_SPELL 305
#define FEAT_GREAT_SMITING 306
#define FEAT_DIVINE_MIGHT 307
#define FEAT_DIVINE_SHIELD 308
#define FEAT_DIVINE_VENGEANCE 309
#define FEAT_PERFECT_TWO_WEAPON_FIGHTING 310
#define FEAT_EPIC_DODGE 311
#define FEAT_IMPROVED_SNEAK_ATTACK 312
#define FEAT_DAMAGE_REDUCTION_FS 313
#define FEAT_HASTE 314
#define FEAT_DEITY_WEAPON_PROFICIENCY 315
#define FEAT_ENERGY_RESISTANCE 316
#define FEAT_EPIC_SKILL_FOCUS 317
#define FEAT_EPIC_SPELLCASTING 318
#define FEAT_POWER_CRITICAL 319
#define FEAT_IMPROVED_NATURAL_WEAPON 320
#define FEAT_BONE_ARMOR 321
#define FEAT_ANIMATE_DEAD 322
#define FEAT_UNDEAD_FAMILIAR 323
#define FEAT_SUMMON_UNDEAD 324
#define FEAT_SUMMON_GREATER_UNDEAD 325
#define FEAT_TOUCH_OF_UNDEATH 326
#define FEAT_ESSENCE_OF_UNDEATH 327
#define FEAT_DIVINE_BOND 328
#define FEAT_COMBAT_CHALLENGE 329
#define FEAT_IMPROVED_COMBAT_CHALLENGE 330
#define FEAT_GREATER_COMBAT_CHALLENGE 331
#define FEAT_EPIC_COMBAT_CHALLENGE 332
#define FEAT_BLEEDING_ATTACK 333
#define FEAT_POWERFUL_SNEAK 334
#define FEAT_ARMOR_SPECIALIZATION_LIGHT 335
#define FEAT_ARMOR_SPECIALIZATION_MEDIUM 336
#define FEAT_ARMOR_SPECIALIZATION_HEAVY 337
#define FEAT_WEAPON_MASTERY 338
#define FEAT_WEAPON_FLURRY 339
#define FEAT_WEAPON_SUPREMACY 340
#define FEAT_ROBILARS_GAMBIT 341
#define FEAT_KNOCKDOWN 342
#define FEAT_EPIC_TOUGHNESS 343
#define FEAT_AUTOMATIC_QUICKEN_SPELL 344
#define FEAT_ENHANCE_ARROW_MAGIC 345
#define FEAT_ENHANCE_ARROW_ELEMENTAL 346
#define FEAT_ENHANCE_ARROW_ELEMENTAL_BURST 347
#define FEAT_ENHANCE_ARROW_ALIGNED 348
#define FEAT_ENHANCE_ARROW_DISTANCE 349
#define FEAT_INTENSIFY_SPELL 350
#define FEAT_SNEAK_ATTACK_OF_OPPORTUNITY 351
#define FEAT_STEADFAST_DETERMINATION 352
#define FEAT_SELF_CONCEALMENT 354
#define FEAT_SWARM_OF_ARROWS 355
#define FEAT_EPIC_PROWESS 356
#define FEAT_PARRY 357
#define FEAT_RIPOSTE 358
#define FEAT_NO_RETREAT 359
#define FEAT_CRIPPLING_CRITICAL 360
#define FEAT_DRAGON_MOUNT_BOOST 361
#define FEAT_DRAGON_MOUNT_BREATH 362
#define FEAT_SACRED_FLAMES 363
#define FEAT_FINANCIAL_EXPERT 364
#define FEAT_THEORY_TO_PRACTICE 365
#define FEAT_RUTHLESS_NEGOTIATOR 366

#endif
