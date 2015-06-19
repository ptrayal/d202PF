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

  add_deity(DEITY_TYMORA, "Tymora", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_LUCK, DOMAIN_PROTECTION,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN, "Good Fortune, Skill, Victory, Adventurers",
    "\r\n"
    );

  add_deity(DEITY_MASK, "Mask", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_EVIL, DOMAIN_LUCK, DOMAIN_TRICKERY,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN, "Thieves, Thievery, Shadows",
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

    add_deity(DEITY_ISTISHIA, "Istishia", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_OCEAN, DOMAIN_STORM,
          DOMAIN_TRAVEL, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Water, Purification, Wetness",
            "\r\n"
    );

    add_deity(DEITY_KELEMVOR, "Kelemvor", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Death, the Dead",
          "\r\n"
          );

    add_deity(DEITY_KOSSUTH, "Kossuth", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_RENEWAL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Fire, Purification through Fire",
          "\r\n"
          );

    add_deity(DEITY_LATHANDER, "Lathander", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_NOBILITY, DOMAIN_PROTECTION,
          DOMAIN_RENEWAL, DOMAIN_STRENGTH, DOMAIN_SUN, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Spring, Dawn, Youth, Birth, Vitality, Athletics",
          "\r\n"
          );

    add_deity(DEITY_LOVIATAR, "Loviatar", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_SUFFERING, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Pain, Hurt, Agony, Torment, Suffering Torture",
          "\r\n"
          );

    add_deity(DEITY_MALAR, "Malar", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_MOON, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Hunters, Stalking, Bloodlust, Evil Lycanthropes",
          "\r\n"
          );

    add_deity(DEITY_MIELIKKI, "Meilikki", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_FAERUNIAN,
          "Forests, Forest Creatures, Rangers, Dryads, Autumn",
          "\r\n"
          );

    add_deity(DEITY_OGHMA, "Oghma", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CHARM, DOMAIN_LUCK, DOMAIN_KNOWLEDGE,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Knowledge, Invention, Inspiration, Bards",
          "\r\n"
          );

    add_deity(DEITY_SELUNE, "Selune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_MOON,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Moon, Stars, Navigation, Prophecy, Questers, Good && Neutral Lycanthropes",
          "\r\n"
          );

    add_deity(DEITY_SILVANUS, "Silvanus", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_RENEWAL,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_FAERUNIAN,
          "Wild Nature, Druids",
          "\r\n"
          );

    add_deity(DEITY_SUNE, "Sune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_FAERUNIAN,
          "Beauty, Love, Passion",
          "\r\n"
          );

    add_deity(DEITY_TALONA, "Talona", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Disease, Poison",
          "\r\n"
          );

    add_deity(DEITY_TALOS, "Talos", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_CHAOS,
          DOMAIN_EVIL, DOMAIN_STORM, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FAERUNIAN,
          "Storms, Destruction, Rebellion, Conflagrations, Earthquakes, Vortices",
          "\r\n"
          );

    add_deity(DEITY_TIAMAT, "Tiamat", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_SCALYKIND,
          DOMAIN_TYRANNY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN,
          "Evil Dragons && Reptiles, Greed, Chessenta",
          "\r\n"
          );

    add_deity(DEITY_TORM, "Torm", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Duty, Loyalty, Obedience, Paladins",
          "\r\n"
          );

    add_deity(DEITY_TYR, "Tyr", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_RETRIBUTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Justice",
          "\r\n"
          );

    add_deity(DEITY_UMBERLEE, "Umberlee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_OCEAN, DOMAIN_STORM, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_FAERUNIAN,
          "Oceans, Currents, Waves, Sea Winds",
          "\r\n"
          );

    add_deity(DEITY_WAUKEEN, "Waukeen", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, DOMAIN_TRADE,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_NUNCHAKU, DEITY_PANTHEON_FAERUNIAN,
          "Trade, Money, Wealth",
          "\r\n"
          );

    // End FR Pantheon Deities

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

// Pathfinder deities
    add_deity(DEITY_ABADAR, "Abadar", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_EARTH, DOMAIN_LAW, DOMAIN_NOBILITY, DOMAIN_PROTECTION,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_LIGHT_CROSSBOW, DEITY_PANTHEON_GOLARION,
            "Cities, Wealth, Merchants, Law",
            "\r\n"
            );

    add_deity(DEITY_ASMODEUS, "Asmodeus", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_FIRE, DOMAIN_MAGIC,
        DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_GOLARION,
        "Contracts, Pride, Slavery, Tyranny",
        "\r\n"
        );

    add_deity(DEITY_CALISTRIA, "Calistria", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_KNOWLEDGE, DOMAIN_LUCK,
        DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_GOLARION,
        "Lust, Revenge, Trickery",
        "\r\n"
        );

    add_deity(DEITY_CAYDEN_CAILEAN, "Cayden Cailean", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD, DOMAIN_LIBERATION,
        DOMAIN_STRENGTH, DOMAIN_TRAVEL, WEAPON_TYPE_RAPIER, DEITY_PANTHEON_GOLARION,
        "Ale, Bravery, Freedom, Wine",
        "\r\n"
        );

    add_deity(DEITY_DESNA, "Desna", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_LIBERATION, DOMAIN_LUCK,
        DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_GOLARION,
        "Dreams, Luck, Stars, Travelers",
        "\r\n"
        );

    add_deity(DEITY_ERASTIL, "Erastil", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_COMMUNITY, DOMAIN_GOOD, DOMAIN_LAW,
        DOMAIN_PLANT, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_BOW, DEITY_PANTHEON_GOLARION,
        "Family, Farming, Hunting, Trade",
        "\r\n"
        );

    add_deity(DEITY_GORUM, "Gorum", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_DESTRUCTION, DOMAIN_GLORY, DOMAIN_STRENGTH, 
        DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_GOLARION,
        "Battle, Strength, War",
        "\r\n"
        );

    add_deity(DEITY_GOZREH, "Gozreh", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_WATER,
        DOMAIN_WEATHER, DOMAIN_UNDEFINED, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_GOLARION,
        "Nature, The Sea, Weather",
        "\r\n"
        );

    add_deity(DEITY_IOMEDAE, "Iomedae", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GLORY, DOMAIN_GOOD, DOMAIN_LAW, DOMAIN_SUN, DOMAIN_WAR,
        DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_GOLARION,
        "Honor, Justice, Rulership, Valor",
        "\r\n"
        );

    add_deity(DEITY_IOMEDAE, "Iomedae", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_LAW, DOMAIN_SUN, DOMAIN_WAR,
        DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_GOLARION,
        "Honor, Justice, Rulership, Valor",
        "\r\n"
        );

    add_deity(DEITY_IRORI, "Irori", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_HEALING, DOMAIN_KNOWLEDGE, DOMAIN_LAW, DOMAIN_RUNE,
        DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_GOLARION,
        "History, Knowledge, Self-Perfection",
        "\r\n"
        );

    add_deity(DEITY_LAMASHTU, "Lamashtu", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_STRENGTH, DOMAIN_TRICKERY,
        DOMAIN_MADNESS, DOMAIN_UNDEFINED, WEAPON_TYPE_KUKRI, DEITY_PANTHEON_GOLARION,
        "Madness, Monsters, Nightmares",
        "\r\n"
        );

    add_deity(DEITY_NETHYS, "Nethys", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_KNOWLEDGE, DOMAIN_MAGIC, DOMAIN_PROTECTION,
        DOMAIN_RUNE, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_GOLARION,
        "Magic",
        "\r\n"
        );

    add_deity(DEITY_NORGORBER, "Norgorber", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_CHARM, DOMAIN_DEATH, DOMAIN_EVIL, DOMAIN_KNOWLEDGE,
        DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_GOLARION,
        "Greed, Murder, Poison, Secrets",
        "\r\n"
        );

    add_deity(DEITY_PHARASMA, "Pharasma", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_HEALING, DOMAIN_KNOWLEDGE, DOMAIN_REPOSE,
        DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_GOLARION,
        "Death, Fate, Prophecy, Rebirth",
        "\r\n"
        );

    add_deity(DEITY_ROVAGUG, "Rovagug", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_DESTRUCTION, DOMAIN_WEATHER,
        DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_AXE, DEITY_PANTHEON_GOLARION,
        "Destruction, Disaster, Wrath",
        "\r\n"
        );

    add_deity(DEITY_SARENRAE, "Sarenrae", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_FIRE, DOMAIN_GLORY, DOMAIN_GOOD, DOMAIN_HEALING,
        DOMAIN_SUN, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_GOLARION,
        "Healing, Honesty, Redemption, Sun",
        "\r\n"
        );

    add_deity(DEITY_SHELYN, "Shelyn", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_AIR, DOMAIN_CHARM, DOMAIN_GOOD, DOMAIN_LUCK,
        DOMAIN_PROTECTION, DOMAIN_UNDEFINED, WEAPON_TYPE_GLAIVE, DEITY_PANTHEON_GOLARION,
        "Art, Beauty, Love, Music",
        "\r\n"
        );

    add_deity(DEITY_TORAG, "Torag", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_ARTIFICE, DOMAIN_EARTH, DOMAIN_GOOD, DOMAIN_LAW,
        DOMAIN_PROTECTION, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_GOLARION,
        "Forge, Protection, Strategy",
        "\r\n"
        );

    add_deity(DEITY_URGATHOA, "Urgathoa", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DEATH, DOMAIN_EVIL, DOMAIN_MAGIC, DOMAIN_STRENGTH,
        DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_GOLARION,
        "Disease, Gluttony, Undeath",
        "\r\n"
        );

    add_deity(DEITY_ZON_KUTHON, "Zon-Kuthon", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_DEATH, DOMAIN_DESTRUCTION, DOMAIN_EVIL,
        DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_GOLARION,
        "Darkness, Envy, Loss, Pain",
        "\r\n"
        );

};
