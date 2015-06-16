
#define DEITY_NONE			0

// FORGOTTEN REALMS DEITIES

// Faerunian Pantheon

#define DEITY_AKADI 			1
#define DEITY_AURIL			2  //Unused
#define DEITY_AZUTH			3
#define DEITY_BANE			4
#define DEITY_BESHABA			5  //Unused
#define DEITY_CHAUNTEA			6
#define DEITY_CYRIC			7
#define DEITY_DENEIR			8  //Unused
#define DEITY_ELDATH			9  //Unused
#define DEITY_FINDER_WYVERNSPUR 	10  //Unused
#define DEITY_GARAGOS			11  //Unused
#define DEITY_GARGAUTH			12  //Unused
#define DEITY_GOND			13
#define DEITY_GRUMBAR			14
#define DEITY_GWAERON_WINDSTROM		15  //Unused
#define DEITY_HELM			16
#define DEITY_HOAR			17  //Unused
#define DEITY_ILMATER			18
#define DEITY_ISTISHIA			19
#define DEITY_JERGAL			20  //Unused
#define DEITY_KELEMVOR			21
#define DEITY_KOSSUTH			22
#define DEITY_LATHANDER			23
#define DEITY_LLIIRA			24  //Unused
#define DEITY_LOVIATAR			25
#define DEITY_LURUE			26  //Unused
#define DEITY_MALAR			27
#define DEITY_MASK			28
#define DEITY_MIELIKKI			29
#define DEITY_MILIL			30  //Unused
#define DEITY_MYSTRA			31
#define DEITY_NOBANION			32  //Unused
#define DEITY_OGHMA			33
#define DEITY_RED_KNIGHT		34  //Unused
#define DEITY_SAVRAS			35  //Unused
#define DEITY_SELUNE			36
#define DEITY_SHAR			37
#define DEITY_SHARESS			38  //Unused
#define DEITY_SHAUNDAKUL		39  //Unused
#define DEITY_SHIALLIA			40  //Unused
#define DEITY_SIAMORPHE			41  //Unused
#define DEITY_SILVANUS			42
#define DEITY_SUNE			43
#define DEITY_TALONA			44
#define DEITY_TALOS			45
#define DEITY_TEMPUS			46
#define DEITY_TIAMAT			47
#define DEITY_TORM			48
#define DEITY_TYMORA			49
#define DEITY_TYR			50
#define DEITY_UBTAO			51  //Unused
#define DEITY_ULUTIU			52  //Unused
#define DEITY_UMBERLEE			53
#define DEITY_UTHGAR			54  //Unused
#define DEITY_VALKUR			55  //Unused
#define DEITY_VELSHAROON		56  //Unused
#define DEITY_WAUKEEN			57

// Pre Cataclysmic Dragonlance Deities

// Good
#define DEITY_PALADINE			120
#define DEITY_KIRI_JOLITH		121
#define DEITY_MAJERE			122
#define DEITY_HABBAKUK			123
#define DEITY_MISHAKAL			124
#define DEITY_BRANCHALA			125
#define DEITY_SOLINARI			126
// Neutral
#define DEITY_GILEAN			127
#define DEITY_REORX			128
#define DEITY_SHINARE			129
#define DEITY_SIRRION			130
#define DEITY_ZIVILYN			131
#define DEITY_CHISLEV			132
#define DEITY_LUNITARI			133
// Evil
#define DEITY_TAKHISIS			134
#define DEITY_SARGONNAS			135
#define DEITY_CHEMOSH			136
#define DEITY_HIDDUKEL			137
#define DEITY_MORGION			138
#define DEITY_ZEBOIM			139
#define DEITY_NUITARI			140

// Pathfinder Deities
#define DEITY_ABADAR            141
#define DEITY_ASMODEUS          142
#define DEITY_CALISTRIA         143



#define NUM_DEITIES			144 // One more than the last entry please :)

#define DEITY_PANTHEON_NONE		0
#define DEITY_PANTHEON_ALL		1
#define DEITY_PANTHEON_FAERUNIAN	2
#define DEITY_PANTHEON_DL_PRE_CAT       9
#define DEITY_PANTHEON_DL_4TH_AGE       9
#define DEITY_PANTHEON_GOLARION			10

extern struct deity_info deity_list[NUM_DEITIES];

