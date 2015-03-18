#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "deities.h"


struct deity_info deity_list[NUM_DEITIES];

#define Y TRUE
#define N FALSE

void init_deities(void)
{

  int i = 0, j = 0;

  for (i = 0; i < NUM_DEITIES; i++) {

    deity_list[i].name = "None";
    deity_list[i].ethos = ETHOS_NEUTRAL;
    deity_list[i].alignment = ALIGNMENT_NEUTRAL;
    for (j = 0; j < 6; j++)
      deity_list[i].domains[j] = DOMAIN_UNDEFINED;
    deity_list[i].favored_weapon = WEAPON_TYPE_UNARMED;
    deity_list[i].pantheon = DEITY_PANTHEON_NONE;
    deity_list[i].portfolio = "Nothing";
    deity_list[i].description = "You do not worship a deity at all for reasons of your own.";
  }

}

void add_deity(int deity, char *name, int ethos, int alignment, int d1, int d2, int d3, int d4, int d5, int d6, int weapon, int pantheon,
               char *portfolio, char *description) {

  deity_list[deity].name = name;
  deity_list[deity].ethos = ethos;
  deity_list[deity].alignment = alignment;
  deity_list[deity].domains[0] = d1;
  deity_list[deity].domains[1] = d2;
  deity_list[deity].domains[2] = d3;
  deity_list[deity].domains[3] = d4;
  deity_list[deity].domains[4] = d5;
  deity_list[deity].domains[5] = d6;
  deity_list[deity].favored_weapon = weapon;
  deity_list[deity].pantheon = pantheon;
  deity_list[deity].portfolio = portfolio;
  deity_list[deity].description = description;

}

void assign_deities(void) {

  init_deities();

// This is the guide line of 80 characters for writing descriptions of proper
// length.  Cut and paste it where needed then return it here.
// -----------------------------------------------------------------------------


  add_deity(DEITY_NONE, "None", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL, "The Faithless",
	"Those who choose to worship no deity at all are known as the faithless.  It is their\r\n"
	"destiny to become part of the living wall in Kelemvor's domain when they die, to\r\n"
	"ultimately have their very soul devoured and destroyed forever.\r\n");

  add_deity(DEITY_ILMATER, "Ilmater", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW, DOMAIN_STRENGTH, DOMAIN_SUFFERING,
            DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN, "Endurance, Suffering, Martyrdom, Perseverance",
	"\r\n"
    );

  add_deity(DEITY_TEMPUS, "Tempus", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_WAR,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN, "War, Battle, Warriors",
    "\r\n"
    );

  add_deity(DEITY_UTHGAR, "Uthgar", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_RETRIBUTION, DOMAIN_STRENGTH,
            DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN, "Uthgardt Barabrian Tribes, Physical Strength",
    "\r\n"
    );

  add_deity(DEITY_UBTAO, "Ubtao", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_PLANNING, DOMAIN_PLANT, DOMAIN_PROTECTION, DOMAIN_SCALYKIND,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN, "Creation, Jungles, Cult, the Chultans, Dinosaurs",
    "\r\n"
    );

  add_deity(DEITY_TYMORA, "Tymora", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_LUCK, DOMAIN_PROTECTION,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN, "Good Fortune, Skill, Victory, Adventurers",
    "\r\n"
    );

  add_deity(DEITY_MASK, "Mask", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_EVIL, DOMAIN_LUCK, DOMAIN_TRICKERY,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN, "Thieves, Thievery, Shadows",
    "\r\n"
    );


  add_deity(DEITY_LLIIRA, "Lliira", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_FAMILY, DOMAIN_GOOD,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN,
            "Joy, Happiness, Dance, Festivals, Freedom, Liberty",
    "\r\n"
    );

  add_deity(DEITY_HELM, "Helm", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
            "Guardians, Protectors, Protection",

    "\r\n"
    );

  add_deity(DEITY_SHAR, "Shar", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_CAVERN, DOMAIN_DARKNESS, DOMAIN_EVIL,
          DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_KUKRI, DEITY_PANTHEON_FAERUNIAN,
            "Dark, Night, Loss, Secrets, Underdark",

    "\r\n"
    );


  add_deity(DEITY_MYSTRA, "Mystra", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_ILLUSION, DOMAIN_RUNE, 
          DOMAIN_KNOWLEDGE, DOMAIN_MAGIC, DOMAIN_SPELL, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN,
            "Magic, Spells, The Weave",

    "\r\n"
    );

  add_deity(DEITY_AKADI, "Akadi", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_ILLUSION, DOMAIN_TRAVEL,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Air, Movement, Speed, Flying Creatures",
    "\r\n"
    );

  add_deity(DEITY_AURIL, "Auril", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_AIR, DOMAIN_EVIL, DOMAIN_STORM,
          DOMAIN_WATER, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN,
          "Cold, Winter",
    "\r\n"
    );

  add_deity(DEITY_AZUTH, "Azuth", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_ILLUSION, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE,
          DOMAIN_LAW, DOMAIN_SPELL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_FAERUNIAN,
          "Wizards, Mages, Spellcasters in General",
    "\r\n"
    );

    add_deity(DEITY_BANE, "Bane", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_EVIL, DOMAIN_HATRED,
          DOMAIN_LAW, DOMAIN_TYRANNY, DOMAIN_UNDEFINED, WEAPON_TYPE_MORNINGSTAR, DEITY_PANTHEON_FAERUNIAN,
          "Hatred, Tyranny, Fear",
    "\r\n"
    );

    add_deity(DEITY_BESHABA, "Beshaba", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_FATE,
          DOMAIN_LUCK, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Random Mischief, Misfortune, Bad Luck, Accidents",
    "\r\n"
    );

    add_deity(DEITY_CHAUNTEA, "Chauntea", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_EARTH, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_PROTECTION, DOMAIN_RENEWAL, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_FAERUNIAN,
          "Agriculture, Gardeners, Farmers, Summer",
    "\r\n"
    );

    add_deity(DEITY_CYRIC, "Cyric", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_DESTRUCTION,
          DOMAIN_ILLUSION, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Murder, Lies, Intrigue, Strife, Deception, Illusion",
    "\r\n"
    );

    add_deity(DEITY_DENEIR, "Deneir", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_RUNE,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Glyphs, Images, Literature, Scribes, Cartography",
    "\r\n"
    );

    add_deity(DEITY_ELDATH, "Eldath", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_FAMILY, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_NET, DEITY_PANTHEON_FAERUNIAN,
          "Quiet Places, Springs, Pools, Peace, Waterfalls",
    "\r\n"
    );

    add_deity(DEITY_FINDER_WYVERNSPUR, "Finder Wyvernspur", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS,
          DOMAIN_CHARM, DOMAIN_RENEWAL,
          DOMAIN_SCALYKIND, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Cycle of Life, Transformation of Art, Saurials",
    "\r\n"
    );

    add_deity(DEITY_GARAGOS, "Garagos", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_DESTRUCTION,
          DOMAIN_STRENGTH,
          DOMAIN_WAR, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "War, Skill-at-Arms, Destruction, Plunder",
    "\r\n"
    );

    add_deity(DEITY_GARGAUTH, "Gargauth", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_CHARM, DOMAIN_EVIL, DOMAIN_LAW,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Betrayal, Cruelty, Politcal Corruption, Powerbrokers",
    "\r\n"
    );

    add_deity(DEITY_GOND, "Gond", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CRAFT, DOMAIN_EARTH, DOMAIN_FIRE,
          DOMAIN_KNOWLEDGE, DOMAIN_METAL, DOMAIN_PLANNING, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Artifice, Craft, COnstruction, Smithwork",
    "\r\n"
    );

    add_deity(DEITY_GRUMBAR, "Grumbar", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CAVERN, DOMAIN_EARTH, DOMAIN_METAL,
          DOMAIN_TIME, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Earth, Solidity, Changelessness, Oaths",
    "\r\n"
    );

    add_deity(DEITY_GWAERON_WINDSTROM, "Dwaeron Windstrom", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_KNOWLEDGE,
          DOMAIN_PLANT, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Tracking, Rangers of the North",
    "\r\n"
    );

    add_deity(DEITY_HOAR, "Hoar", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_FATE, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_JAVELIN, DEITY_PANTHEON_FAERUNIAN,
          "Revenge, Retribution, Poetic Justice",
    "\r\n"
    );

    add_deity(DEITY_ISTISHIA, "Istishia", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_OCEAN, DOMAIN_STORM,
          DOMAIN_TRAVEL, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Water, Purification, Wetness",
            "\r\n"
    );

    add_deity(DEITY_JERGAL, "Jergal", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_RUNE, DOMAIN_SUFFERING, DOMAIN_UNDEFINED, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_FAERUNIAN,
          "Fatalism, Proper Burial, Guardianship of Tombs",
            "\r\n"
    );

    add_deity(DEITY_KELEMVOR, "Kelemvor", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Death, the Dead",
          "Kelemvor (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_KOSSUTH, "Kossuth", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_RENEWAL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Fire, Purification through Fire",
          "Kossuth (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_LATHANDER, "Lathander", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_NOBILITY, DOMAIN_PROTECTION,
          DOMAIN_RENEWAL, DOMAIN_STRENGTH, DOMAIN_SUN, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Spring, Dawn, Youth, Birth, Vitality, Athletics",
          "Lathander (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_LOVIATAR, "Loviatar", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_SUFFERING, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Pain, Hurt, Agony, Torment, Suffering Torture",
          "Loviatar (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_LURUE, "Lurue", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORTSPEAR, DEITY_PANTHEON_FAERUNIAN,
          "Talking Beasts, Intelligent Non-Humanoid Creatures",
          "Lurue (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_MALAR, "Malar", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_MOON, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Hunters, Stalking, Bloodlust, Evil Lycanthropes",
          "Malar (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_MIELIKKI, "Meilikki", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_FAERUNIAN,
          "Forests, Forest Creatures, Rangers, Dryads, Autumn",
          "Meilikki (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_MILIL, "Milil", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_CHARM, DOMAIN_GOOD, DOMAIN_KNOWLEDGE,
          DOMAIN_MOBILITY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_RAPIER, DEITY_PANTHEON_FAERUNIAN,
          "Poetry, Song, Eloquence",
          "Milil (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_NOBANION, "Nobanion", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_LAW,
          DOMAIN_NOBILITY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN,
          "Royalty, Lions and Feline Beasts, Good Beasts",
          "Nobanion (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_OGHMA, "Oghma", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CHARM, DOMAIN_LUCK, DOMAIN_KNOWLEDGE,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Knowledge, Invention, Inspiration, Bards",
          "Oghma (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_RED_KNIGHT, "Red Knight", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_NOBILITY, DOMAIN_PLANNING,
          DOMAIN_WAR, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Strategy, Planning, Tactics",
          "Red Knight (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SAVRAS, "Savras", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_FATE, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_MAGIC, DOMAIN_SPELL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Divination, Fate, Truth",
          "Savras (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SELUNE, "Selune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_MOON,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Moon, Stars, Navigation, Prophecy, Questers, Good && Neutral Lycanthropes",
          "Selune (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SHARESS, "Sharess", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Hedonism, Sensual Fulfilment, Festhalls, Cats",
          "Sharess (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SHAUNDAKUL, "Shaundakul", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_PORTAL,
          DOMAIN_PROTECTION, DOMAIN_TRADE, DOMAIN_TRAVEL, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Travel, Exploration, Caravans, Portals",
          "Shaundakul (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SHIALLIA, "Shiallia", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_RENEWAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FAERUNIAN,
          "Woodland GLades && Fertility, The High Forest, Neverwinter Wood",
          "Shiallia (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SIAMORPHE, "Siamorphe", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_LAW, DOMAIN_NOBILITY,
          DOMAIN_PLANNING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LIGHT_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Nobles, Rightful Rule of Nobility, Human Royalty",
          "Siamorphe (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SILVANUS, "Silvanus", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_RENEWAL,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_FAERUNIAN,
          "Wild Nature, Druids",
          "Silvanus (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SUNE, "Sune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_FAERUNIAN,
          "Beauty, Love, Passion",
          "Sune (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TALONA, "Talona", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Disease, Poison",
          "Talona (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TALOS, "Talos", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_CHAOS,
          DOMAIN_EVIL, DOMAIN_STORM, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FAERUNIAN,
          "Storms, Destruction, Rebellion, Conflagrations, Earthquakes, Vortices",
          "Talos (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TIAMAT, "Tiamat", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_SCALYKIND,
          DOMAIN_TYRANNY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN,
          "Evil Dragons && Reptiles, Greed, Chessenta",
          "Tiamat (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TORM, "Torm", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Duty, Loyalty, Obedience, Paladins",
          "Torm (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TYR, "Tyr", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_RETRIBUTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Justice",
          "Tyr (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_UMBERLEE, "Umberlee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_OCEAN, DOMAIN_STORM, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_FAERUNIAN,
          "Oceans, Currents, Waves, Sea Winds",
          "Umberlee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VALKUR, "Valkur", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_GOOD,
          DOMAIN_OCEAN, DOMAIN_PROTECTION, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_FAERUNIAN,
          "Sailors, Ships, Favorable Winds, Naval Combat",
          "Valkur (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VELSHAROON, "Velsharoon", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DEATH, DOMAIN_EVIL, DOMAIN_MAGIC,
          DOMAIN_UNDEATH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FAERUNIAN,
          "Necromancy, Necormancers, Evil Liches, Undeath, Undead",
          "Velsharoon (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_WAUKEEN, "Waukeen", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, DOMAIN_TRADE,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_NUNCHAKU, DEITY_PANTHEON_FAERUNIAN,
          "Trade, Money, Wealth",
          "Waukeen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    // End FR Pantheon Deities

  // Dwarven Pantheon

    add_deity(DEITY_BERRONAR_TRUESILVER, "Berronar Truesilver", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_FAMILY, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DWARVEN,
          "Safety, Honesty, Home, Healing, The Dwarven Family, Records, Marriage, Faithfulness, Loyalty, Oaths",
          "Berronar Truesilver (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_ABBATHOR, "Abbathor", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_TRADE,
          DOMAIN_TRICKERY, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DWARVEN,
          "Greed",
          "Abbathor (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_CLANGEDDIN_SILVERBEARD, "Clangeddin Silverbeard", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_STRENGTH, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Battle, War, Bravery, Honor in Battle",
          "Clangeddin Silverbeard (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );
    
    add_deity(DEITY_DEEP_DUERRA, "Deep Duerra", ETHOS_LAWFUL, ALIGNMENT_EVIL,
          DOMAIN_DWARF, DOMAIN_EVIL, DOMAIN_MENTALISM,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Psionics, COnquest, Expansion",
          "Deep Duerra (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_DUGMAREN_BRIGHTMANTLE, "Dugmaren Brightmantle", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_KNOWLEDGE, DOMAIN_RUNE, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Scholarship, Invention, Discovery",
          "Dugmaren Brightmantle (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_DUMATHOIN, "Dumathoin", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_DWARF, DOMAIN_CAVERN, DOMAIN_CRAFT,
          DOMAIN_EARTH, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_FR_DWARVEN,
          "Buried Wealth, Ores, Gems, Mining, Exploration, Shield Dwarves, Guardian of the Dead",
          "Dumathoin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_GORM_GULTHYN, "Gorm Gulthyn", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Guardian of all Dwarves, Defense, Watchfulness",
          "Gorm Gulthyn (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_HAELA_BRIGHTAXE, "Haela Brightaxe", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Luck in Battle, Joy of Battle, Dwarven Fighters",
          "Haela Brightaxe (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_LADUGUER, "Laduguer", ETHOS_LAWFUL, ALIGNMENT_EVIL,
          DOMAIN_DWARF, DOMAIN_CRAFT, DOMAIN_EVIL,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DWARVEN,
          "Magic Weapon Creation, Artisans, Duergar, Magic",
          "Laduguer (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_MARTHAMMOR_DUIN, "Marthammor Duin", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DWARVEN,
          "Guides, Explorers, Expatriates, Travelers, Lightning",
          "Marthammor Duin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SHARINDLAR, "Sharindlar", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_CHAOS, DOMAIN_MOON, WEAPON_TYPE_WHIP, DEITY_PANTHEON_FR_DWARVEN,
          "Healing, Mercy, Romantic Love, Fertility, Dancing, Courtship, the Moon",
          "Sharindlar (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_THARD_HARR, "Thard Harr", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_DWARVEN,
          "Wild Dwarves, Jungle Survival, Hunting",
          "Thard Harr (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VERGADAIN, "Vergadain", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_TRADE,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Wealth, Luck, Chance, Non-Evil Thieves, Suspicion, Trickery, Negotiations, Sly Cleverness",
          "Vergadain (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_MORADIN, "Moradin", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_CRAFT, DOMAIN_DWARF, DOMAIN_EARTH, DOMAIN_GOOD,
            DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DWARVEN,
            "Dwarves, Creation, Smithing, Metalcraft, Stonework",
    "\r\n"
    );

  // Elven Pantheon

  add_deity(DEITY_AERDRIE_FAENYA, "Aerdrie Faenya", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_STORM, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Air, Weather, Avians, Rain, Fertility, Avariels",
          "Aerdrie Faenya (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_ANGHARRADH, "Angharradh", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_CHAOS, DOMAIN_RENEWAL, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FR_ELVEN,
          "Spring, Fertility, Planting, Birth, Defense, Wisdom",
          "Angharradh (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CORELLON_LARETHIAN, "Corellon Larethian", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_MAGIC, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_CHAOS, DOMAIN_WAR, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_ELVEN,
          "Magic, Music, Arts, Crafts, War, The Elven Race, Sun Elves, Poetry, Bards, Warriors",
          "Corellon Larethian (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_DEEP_SASHELAS, "Deep Sashelas", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_OCEAN, DOMAIN_CHAOS, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_FR_ELVEN,
          "Oceans, Sea Elves, Creation, Knowledge",
          "Deep Sashelas (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_EREVAN_ILESERE, "Erevan Ilesere", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_LUCK, DOMAIN_TRICKERY,
          DOMAIN_CHAOS, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_ELVEN,
          "Mischief, Change, Rogues",
          "Erevan Ilesere (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_FENMAREL_MESTARINE, "Fenmarel Mestarine", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_ANIMAL, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_ELVEN,
          "Feral Elves, Outcasts, Scapegoats, Isolation",
          "Fenmarel Mestarine (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_HANALI_CELANIL, "Hanali Celanil", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_MAGIC, DOMAIN_CHAOS, DOMAIN_PROTECTION, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_ELVEN,
          "Love, Romance, Beauty, Enchantment, Magic Item Artistry, Fine Art, Artists",
          "Hanali Celanil (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_LABELAS_ENORETH, "Labelas Enoreth", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_TIME, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Time, Longevity, The Moment of Choice, History",
          "Labelas Enoreth (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_RILLIFANE_RALLATHIL, "Rillifane Rallathil", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Woodlands, Nature, Wild Elves, Druids",
          "Rillifane Rallathil (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SEHANINE_MOONBOW, "Sehanine Moonbow", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_ILLUSION, DOMAIN_CHAOS, DOMAIN_MOON, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Mysticism, Dreams, Death, Journeys, Transcendence, the moon, stars && heavens, Moon Elves",
          "Sehanine Moonbow (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHEVARASH, "Shevarash", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_RETRIBUTION, DOMAIN_WAR,
          DOMAIN_CHAOS, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_COMPOSITE_LONGBOW, DEITY_PANTHEON_FR_ELVEN,
          "Hatred of the Drow, Vengeance, Crusades, Loss",
          "Shevarash (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SOLONOR_THELANDIRA, "Solonor Thelandira", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_COMPOSITE_LONGBOW, DEITY_PANTHEON_FR_ELVEN,
          "Archery, Hunting, Wilderness Survival",
          "Solonor Thelandira (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Gnome Pantheon

  add_deity(DEITY_BAERVAN_WILDWANDERER, "Baervan Wildwanderer", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_ANIMAL, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORTSPEAR, DEITY_PANTHEON_FR_GNOME,
          "Forests, Travel, Nature",
          "Baervan Wildwanderer (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_BARAVAR_CLOAKSHADOW, "Baravar Cloakshadow", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_TRICKERY, DOMAIN_ILLUSION, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_GNOME,
          "Illusions, Deception, Traps, Wards",
          "Daravar Cloakshadow (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CALLARDURAN_SMOOTHHANDS, "Callarduran Smoothhands", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_GNOME, DOMAIN_CAVERN, DOMAIN_CRAFT,
          DOMAIN_EARTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_GNOME,
          "Stone, The Underdark, Mining, Svirfneblin",
          "Callarduran Smoothhands (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_FLANDAL_STEELSKIN, "Flandal Steelskin", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_METAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_GNOME,
          "Mining, Physical Fitness, Smithing, Metalworking",
          "Flandal Steelskin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GAERDAL_IRONHAND, "Gaerdal Ironhand", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_GNOME,
          "Vigilance, Combat, Martial Defense",
          "Gaerdal Ironhand (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GARL_GLITTERGOLD, "Garl Glittergold", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_LAW, DOMAIN_TRICKERY, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_GNOME,
          "Protection, Humor, Gem-Cutting, Trickery, Gnomes",
          "Garl Glittergold (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SEGOJAN_EARTHCALLER, "Segojan Earthcaller", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CAVERN, DOMAIN_GOOD,
          DOMAIN_EARTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_GNOME,
          "Earth, Nature, the Dead",
          "Segojan Earthcaller (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_URDLEN, "Urdlen", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_GNOME, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_EARTH, DOMAIN_HATRED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_GNOME,
          "Greed, Bloodlust, Evil, Hatred, Spriggans, Uncontrolled Impulse",
          "Urdlen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );


  // Halfing Pantheon

  add_deity(DEITY_ARVOREEN, "Arvoreen", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_HALFLING,
          "Defense, War, Vigilence, Halfling Warriors, Duty",
          "Arvoreen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_BRANDOBARIS, "Brandobaris", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_LUCK, DOMAIN_TRAVEL,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_HALFLING,
          "Stealth, Thievery, Adventuring, Halfling Rogues",
          "Brandobaris (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CYRROLLALEE, "Cyrrollalee", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_FAMILY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_HALFLING,
          "Friendship, Trust, the Hearth, Hospitality, Crafts",
          "Cyrrollalee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHEELA_PERYROYL, "Sheela Peryroyl", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_AIR, DOMAIN_CHARM,
          DOMAIN_PLANT, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SICKLE, DEITY_PANTHEON_FR_HALFLING,
          "Nature, Agriculture, Weather, Song, Dance, Beauty, Romantic Love",
          "Sheela Peryroyl (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_UROGALAN, "Urogalan", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_DEATH,
          DOMAIN_PROTECTION, DOMAIN_EARTH, DOMAIN_UNDEFINED, WEAPON_TYPE_DIRE_FLAIL, DEITY_PANTHEON_FR_HALFLING,
          "Earth, Death, Protection of the Dead",
          "Urogalan (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_YONDALLA, "Yondalla", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_FAMILY, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_HALFLING,
          "Protection, Bounty, Halflings, Children, Security, Leadership, Wisdom, Creation, Family, Tradition",
          "Yondalla (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Orc Pantheon

  add_deity(DEITY_BAHGTRU, "Bahgtru", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_STRENGTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Loyalty, Stupidity, Brute Strength",
          "Bahgtru (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GRUUMSH, "Gruumsh", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_STRENGTH, DOMAIN_HATRED, DOMAIN_WAR, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FR_ORC,
          "Orcs, COnquest, Survival, Strength, Territory",
          "Gruumsh (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_ILNEVAL, "Ilneval", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_WAR, DOMAIN_EVIL,
          DOMAIN_DESTRUCTION, DOMAIN_PLANNING, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_ORC,
          "War, Combat, Overwhelming Numbers, Strategy",
          "Ilneval (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_LUTHIC, "Luthic", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CAVERN, DOMAIN_EVIL,
          DOMAIN_EARTH, DOMAIN_FAMILY, DOMAIN_HEALING, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Caves, Orc Females, Home, Wisdom, Fertility, Healing, Servitude",
          "Luthic (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHARGAAS, "Shargaas", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_DARKNESS, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_ORC,
          "Night, Thieves, Stealth, Darkness, The Underdark",
          "Shargaas (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_YURTRUS, "Yurtrus", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_DEATH, DOMAIN_EVIL,
          DOMAIN_DESTRUCTION, DOMAIN_SUFFERING, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Death, Disease",
          "Yurtrus (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Drow Pantheon

  add_deity(DEITY_LOLTH, "Lolth", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_DARKNESS, DOMAIN_DESTRUCTION,
          DOMAIN_DROW, DOMAIN_EVIL, DOMAIN_SPIDER, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DROW,
          "Spiders, Evil, Darkness, Chaos, Assassins, Drow",
          "Lolth (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GHAUNADAUR, "Ghaunadaur", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CAVERN, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_HATRED, DOMAIN_EVIL, DOMAIN_SLIME, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DROW,
          "Oozes, Slimes, Jellies, Outcasts, Ropers, Rebels",
          "Ghaunadaur (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_KIARANSALEE, "Kiaransalee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_UNDEATH, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_RETRIBUTION, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DROW,
          "Undead, Vengeance",
          "Kiaransalee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SELVETARM, "Selvetarm", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_SPIDER, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DROW,
          "Drow Warriors",
          "Selvetarm (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_VHAERAUN, "Vhaeraun", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_TRAVEL, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_TRICKERY, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_DROW,
          "Thievery, Drow Males, Evil Activity on the Surface",
          "Vhaeraun (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_EILISTRAEE, "Eilistraee", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_DROW, DOMAIN_ELF,
            DOMAIN_GOOD, DOMAIN_MOON, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FR_DROW,
            "Song, Beauty, Dance, Swordwork, Hunting, Moonlight",
    "\r\n"
    );

  add_deity(DEITY_BRANCHALA, "Branchala", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_LUCK, DOMAIN_TRICKERY, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_RAPIER, DEITY_PANTHEON_DL_PRE_CAT,
            "Music, Poetry, Bards",

            "\r\n"
            );

  add_deity(DEITY_HABBAKUK, "Habbakuk", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_WATER, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_DL_PRE_CAT,
            "Animals, Water, Passion",

            "\r\n"
            );

  add_deity(DEITY_KIRI_JOLITH, "Kiri-Jolith", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_STRENGTH, DOMAIN_WAR, DOMAIN_TRICKERY, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_DL_PRE_CAT,
            "War, Courage, Honor",

            "\r\n"
            );

  add_deity(DEITY_MAJERE, "Majere", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_LAW, DOMAIN_MEDITATION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_DL_PRE_CAT,
            "Discipline, Loyalty",

            "\r\n"
            );

  add_deity(DEITY_MISHAKAL, "Mishakal", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_COMMUNITY, DOMAIN_HEALING, DOMAIN_PROTECTION, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
            "Healers, Artists, Midwives, Scholars",

            "\r\n"
            );

  add_deity(DEITY_PALADINE, "Paladine", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_LAW, DOMAIN_PROTECTION, DOMAIN_GOOD, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_DL_PRE_CAT,
            "Wisdom, Redemption, Altruism, Good Dragons",

            "\r\n"
            );

  add_deity(DEITY_SOLINARI, "Solinari", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
            "Magic, Arcane Knowledge, Good Wizards",

            "\r\n"
            );

  add_deity(DEITY_CHISLEV, "Chislev", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_ANIMAL, DOMAIN_EARTH, DOMAIN_PLANT,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_DL_PRE_CAT,
            "Nature, Wilderness, Beasts",

            "\r\n"
            );

  add_deity(DEITY_GILEAN, "Gilean", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_LIBERATION, DOMAIN_PROTECTION, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
            "Balance, Knowledge, Learning, Freedom, Watchfulness",

            "\r\n"
            );

  add_deity(DEITY_LUNITARI, "Lunitari", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_ILLUSION, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
            "Magic, Arcane Knowledge, Neutral Wizards",

            "\r\n"
            );

  add_deity(DEITY_REORX, "Reorx", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_EARTH, DOMAIN_FIRE, DOMAIN_FORGE, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_DL_PRE_CAT,
            "Creation, Gambling, Artisans, Engineering",

            "\r\n"
            );

  add_deity(DEITY_SHINARE, "Shinare", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_LUCK, DOMAIN_TRAVEL, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LIGHT_MACE, DEITY_PANTHEON_DL_PRE_CAT,
            "Wealth, Commerce, Travel",

            "\r\n"
            );

  add_deity(DEITY_SIRRION, "Sirrion", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_FIRE, DOMAIN_PASSION, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_DL_PRE_CAT,
            "Fire, Strength, Sun",

            "\r\n"
            );

  add_deity(DEITY_ZIVILYN, "Zivilyn", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_INSIGHT, DOMAIN_KNOWLEDGE, DOMAIN_MEDIATION, DOMAIN_UNDEFINED,
                DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
                "Wisdom, Foresight, Prophecy",

                "\r\n"
                );

  add_deity(DEITY_TAKHISIS, "Takhisis", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_LAW, DOMAIN_EVIL, DOMAIN_WAR, DOMAIN_DESTRUCTION,
              DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_MORNINGSTAR, DEITY_PANTHEON_DL_PRE_CAT,
              "Conquest, Tyranny, Evil Dragons",

              "\r\n"
              );

  add_deity(DEITY_CHEMOSH, "Chemosh", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DEATH, DOMAIN_EVIL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED,
              DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SICKLE, DEITY_PANTHEON_DL_PRE_CAT,
              "Death, Undead, Murder",

              "\r\n"
              );

  add_deity(DEITY_HIDDUKEL, "Hiddukel", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_TREACHERY, DOMAIN_TRICKERY, DOMAIN_UNDEFINED,
                DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_DL_PRE_CAT,
                "Wealth, Thieves, Lies",

                "\r\n"
                );

  add_deity(DEITY_MORGION, "Morgion", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_EVIL, DOMAIN_PESTILENCE, DOMAIN_UNDEFINED,
                DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_DL_PRE_CAT,
                "Disease, Planning, Suffering",

                "\r\n"
                );

  add_deity(DEITY_SARGONNAS, "Sargonnas", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_LAW, DOMAIN_FIRE, DOMAIN_EVIL, DOMAIN_WAR,
                DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_AXE, DEITY_PANTHEON_DL_PRE_CAT,
                "Vengeance, Conquest, Strength, Rage",

                "\r\n"
                );

  add_deity(DEITY_NUITARI, "Nuitari", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_DL_PRE_CAT,
            "Magic, Arcane Knowledge, Evil Wizards",

            "\r\n"
            );

  add_deity(DEITY_ZEBOIM, "Zeboim", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_CHAOS, DOMAIN_STORM, DOMAIN_WATER,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_DL_PRE_CAT,
            "Sea, Storms, Envy",

            "\r\n"
            );


};
