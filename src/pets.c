#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "deities.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "pets.h"

struct pet_data pet_list[NUM_PETS];

void add_pet(int pet_num, char *desc, int level, int max_hit, int ac, int dr);
void add_pet_attack(int pet_num, int attack_num, int to_hit, int ndice, int sdice, int dammod);
void init_pets(void);
void load_pets(void);
void free_summon(struct char_data *ch);

ACMD(do_dismount);

void init_pets(void)
{

  int i = 0, j = 0;

  for (i = 0; i < NUM_PETS; i++) {
    pet_list[i].desc = NULL;
    pet_list[i].level = 0;
    pet_list[i].max_hit = 0;
    pet_list[i].ac = 0;
    pet_list[i].dr = 0;
    for (j = 0; j < 5; j++) {
      pet_list[i].attacks_to_hit[j] = 0;
      pet_list[i].attacks_ndice[j] = 0;
      pet_list[i].attacks_sdice[j] = 0;
      pet_list[i].attacks_dammod[j] = 0;
    }
    pet_list[i].mount = FALSE;
    pet_list[i].flying = FALSE;
    pet_list[i].skill = 0;
    pet_list[i].cost = 0;
    pet_list[i].speed = 50;
    pet_list[i].size = SIZE_MEDIUM;  
  }

}

void add_pet(int pet_num, char *desc, int level, int max_hit, int ac, int dr)
{
  if (pet_num < 0 || pet_num >= NUM_PETS)
    return;

  pet_list[pet_num].desc = strdup(desc);
  pet_list[pet_num].level = level;
  pet_list[pet_num].max_hit = max_hit;
  pet_list[pet_num].ac = ac;
  pet_list[pet_num].dr = dr;
}

void add_pet_attack(int pet_num, int attack_num, int to_hit, int ndice, int sdice, int dammod)
{

  if (pet_num < 0 || pet_num >= NUM_PETS)
    return;

  if (attack_num < 0 || attack_num >= 5)
    return;

  pet_list[pet_num].attacks_to_hit[attack_num] = to_hit;
  pet_list[pet_num].attacks_ndice[attack_num] = ndice;
  pet_list[pet_num].attacks_sdice[attack_num] = sdice;
  pet_list[pet_num].attacks_dammod[attack_num] = dammod;

}

void pet_mount(int pet_num, int size, int speed)
{
  pet_list[pet_num].mount = TRUE;
  pet_list[pet_num].size = size;
  pet_list[pet_num].speed = speed;
}

void pet_flying(int pet_num)
{
  pet_list[pet_num].flying = TRUE;
}
void pet_cost(int pet_num, int cost, int skill) {
  pet_list[pet_num].cost = cost;
  pet_list[pet_num].skill = skill;
}


// Now the pets

void load_pets(void)
{

add_pet(PET_UNDEFINED, "an unrecognizable mass of flesh", 1, 1, 0, 0);

add_pet(PET_DOG, "a riding mastiff", 1, 10, 150, 0);
add_pet_attack(PET_DOG, 0, 5, 1, 4, 1);
pet_mount(PET_DOG, SIZE_MEDIUM, 50);
pet_cost(PET_DOG, 50, 1);

add_pet(PET_WOLF, "a timber wolf", 5, 20, 160, 0);
add_pet_attack(PET_WOLF, 0, 10, 1, 6, 3);
pet_mount(PET_WOLF, SIZE_MEDIUM, 50);
pet_cost(PET_WOLF, 1000, 5);

add_pet(PET_BLACK_BEAR, "a black bear", 10, 30, 130, 0);
add_pet_attack(PET_BLACK_BEAR, 0, 18, 1, 4, 4);
add_pet_attack(PET_BLACK_BEAR, 1, 18, 1, 4, 4);
add_pet_attack(PET_BLACK_BEAR, 2, 18, 1, 6, 2);
pet_mount(PET_BLACK_BEAR, SIZE_LARGE, 50);
pet_cost(PET_BLACK_BEAR, 5000, 10);

add_pet(PET_LION, "a lion", 15, 50, 150, 0);
add_pet_attack(PET_LION, 0, 25, 1, 4, 5);
add_pet_attack(PET_LION, 1, 25, 1, 4, 5);
add_pet_attack(PET_LION, 2, 25, 1, 8, 2);
pet_mount(PET_LION, SIZE_LARGE, 50);
pet_cost(PET_LION, 8000, 15);

add_pet(PET_GRIFFON, "a griffon", 20, 77, 170, 0);
add_pet_attack(PET_GRIFFON, 0, 30, 2, 6, 4);
add_pet_attack(PET_GRIFFON, 1, 30, 1, 4, 2);
add_pet_attack(PET_GRIFFON, 2, 30, 1, 4, 2);
pet_mount(PET_GRIFFON, SIZE_LARGE, 80);
pet_flying(PET_GRIFFON);
pet_cost(PET_GRIFFON, 15000, 20);

add_pet(PET_DIRE_LION, "a dire lion", 25, 88, 150, 0);
add_pet_attack(PET_DIRE_LION, 0, 35, 1, 6, 7);
add_pet_attack(PET_DIRE_LION, 1, 35, 1, 6, 7);
add_pet_attack(PET_DIRE_LION, 2, 35, 1, 8, 3);
pet_mount(PET_DIRE_LION, SIZE_LARGE, 50);
pet_cost(PET_DIRE_LION, 25000, 25);


add_pet(PET_MEGARAPTOR, "a megaraptor", 30, 107, 170, 0);
add_pet_attack(PET_MEGARAPTOR, 0, 40, 2, 6, 6);
add_pet_attack(PET_MEGARAPTOR, 1, 40, 1, 4, 6);
add_pet_attack(PET_MEGARAPTOR, 2, 40, 1, 4, 6);
add_pet_attack(PET_MEGARAPTOR, 3, 40, 1, 8, 6);
pet_mount(PET_MEGARAPTOR, SIZE_LARGE, 50);
pet_cost(PET_MEGARAPTOR, 50000, 30);


add_pet(PET_TRICERATOPS, "a triceratops", 40, 252, 180, 0);
add_pet_attack(PET_TRICERATOPS, 0, 50, 2, 12, 10);
add_pet_attack(PET_TRICERATOPS, 1, 50, 2, 8, 10);
pet_mount(PET_TRICERATOPS, SIZE_HUGE, 60);
pet_cost(PET_TRICERATOPS, 250000, 40);

add_pet(PET_ROC, "a roc", 50, 270, 170, 0);
add_pet_attack(PET_ROC, 0, 60, 2, 6, 12);
add_pet_attack(PET_ROC, 1, 60, 2, 6, 12);
add_pet_attack(PET_ROC, 2, 60, 2, 8, 6);
pet_mount(PET_ROC, SIZE_HUGE, 80);
pet_flying(PET_ROC);
pet_cost(PET_ROC, 1000000, 50);

add_pet(PET_RIDING_HORSE, "a riding horse", 3, 30, 130, 0);
add_pet_attack(PET_RIDING_HORSE, 0, 6, 1, 4, 1);
add_pet_attack(PET_RIDING_HORSE, 1, 6, 1, 4, 1);
pet_mount(PET_RIDING_HORSE, SIZE_LARGE, 60);
pet_cost(PET_RIDING_HORSE, 75, 1);

add_pet(PET_HEAVY_RIDING_HORSE, "a heavy riding horse", 3, 30, 130, 0);
add_pet_attack(PET_HEAVY_RIDING_HORSE, 0, 10, 1, 6, 1);
add_pet_attack(PET_HEAVY_RIDING_HORSE, 1, 10, 1, 6, 1);
pet_mount(PET_HEAVY_RIDING_HORSE, SIZE_LARGE, 60);
pet_cost(PET_HEAVY_RIDING_HORSE, 1500, 3);

add_pet(PET_WAR_HORSE, "a war horse", 5, 32, 140, 0);
add_pet_attack(PET_WAR_HORSE, 0, 14, 1, 4, 3);
add_pet_attack(PET_WAR_HORSE, 1, 14, 1, 4, 3);
pet_mount(PET_WAR_HORSE, SIZE_LARGE, 60);
pet_cost(PET_WAR_HORSE, 2250, 5);

add_pet(PET_HEAVY_WAR_HORSE, "a heavy war horse", 7, 44, 140, 0);
add_pet_attack(PET_HEAVY_WAR_HORSE, 0, 16, 1, 6, 4);
add_pet_attack(PET_HEAVY_WAR_HORSE, 1, 16, 1, 6, 4);
pet_mount(PET_HEAVY_WAR_HORSE, SIZE_LARGE, 60);
pet_cost(PET_HEAVY_WAR_HORSE, 3000, 7);

add_pet(PET_KHURIAN_RIDING_HORSE, "a khurian riding horse", 9, 30, 130, 0);
add_pet_attack(PET_KHURIAN_RIDING_HORSE, 0, 18, 1, 6, 1);
add_pet_attack(PET_KHURIAN_RIDING_HORSE, 1, 18, 1, 6, 1);
pet_mount(PET_KHURIAN_RIDING_HORSE, SIZE_LARGE, 65);
pet_cost(PET_KHURIAN_RIDING_HORSE, 10000, 9);

add_pet(PET_KHURIAN_WAR_HORSE, "a khurian war horse", 12, 44, 140, 0);
add_pet_attack(PET_KHURIAN_WAR_HORSE, 0, 20, 1, 6, 4);
add_pet_attack(PET_KHURIAN_WAR_HORSE, 1, 20, 1, 6, 4);
pet_mount(PET_KHURIAN_WAR_HORSE, SIZE_LARGE, 65);
pet_cost(PET_KHURIAN_WAR_HORSE, 25000, 12);

add_pet(PET_LARGE_EARTH_ELEMENTAL, "a large earth elemental", 8, 96, 180, 5);
add_pet_attack(PET_LARGE_EARTH_ELEMENTAL, 0, 16, 2, 8, 7);
add_pet_attack(PET_LARGE_EARTH_ELEMENTAL, 1, 16, 2, 8, 7);

add_pet(PET_HUGE_EARTH_ELEMENTAL, "a huge earth elemental", 16, 208, 180, 5);
add_pet_attack(PET_HUGE_EARTH_ELEMENTAL, 0, 32, 2, 10, 9);
add_pet_attack(PET_HUGE_EARTH_ELEMENTAL, 1, 32, 2, 10, 9);

add_pet(PET_GREATER_EARTH_ELEMENTAL, "a greater earth elemental", 21, 252, 200, 10);
add_pet_attack(PET_GREATER_EARTH_ELEMENTAL, 0, 42, 2, 10, 10);
add_pet_attack(PET_GREATER_EARTH_ELEMENTAL, 1, 42, 2, 10, 10);

add_pet(PET_GHOUL, "a ghoul", 2, 24, 140, 0);
add_pet_attack(PET_GHOUL, 0, 4, 1, 6, 1);
add_pet_attack(PET_GHOUL, 1, 4, 1, 3, 0);
add_pet_attack(PET_GHOUL, 2, 4, 1, 3, 0);

add_pet(PET_GHAST, "a ghast", 4, 51, 170, 0);
add_pet_attack(PET_GHAST, 0, 8, 1, 8, 3);
add_pet_attack(PET_GHAST, 1, 8, 1, 4, 1);
add_pet_attack(PET_GHAST, 2, 8, 1, 4, 1);

add_pet(PET_MUMMY, "a mummy", 8, 99, 200, 5);
add_pet_attack(PET_MUMMY, 0, 16, 1, 6, 10);

add_pet(PET_MOHRG, "a mohrg", 14, 168, 230, 0);
add_pet_attack(PET_MOHRG, 0, 28, 1, 6, 7);

add_pet(PET_ADULT_RED_DRAGON, "an adult red dragon", 30, 550, 374, 0);
add_pet_attack(PET_ADULT_RED_DRAGON, 0, 50, 2, 8, 9);
add_pet_attack(PET_ADULT_RED_DRAGON, 1, 50, 6, 6, 24);
add_pet_attack(PET_ADULT_RED_DRAGON, 2, 50, 4, 8, 18);
pet_mount(PET_ADULT_RED_DRAGON, SIZE_HUGE, 150);
pet_flying(PET_ADULT_RED_DRAGON);

add_pet(PET_ADULT_SILVER_DRAGON, "an adult silver dragon", 30, 550, 374, 0);
add_pet_attack(PET_ADULT_SILVER_DRAGON, 0, 50, 2, 8, 9);
add_pet_attack(PET_ADULT_SILVER_DRAGON, 1, 50, 6, 6, 24);
add_pet_attack(PET_ADULT_SILVER_DRAGON, 2, 50, 4, 8, 18);
pet_mount(PET_ADULT_SILVER_DRAGON, SIZE_HUGE, 150);
pet_flying(PET_ADULT_SILVER_DRAGON);

add_pet(PET_ADULT_COPPER_DRAGON, "an adult copper dragon", 30, 550, 374, 0);
add_pet_attack(PET_ADULT_COPPER_DRAGON, 0, 50, 2, 8, 9);
add_pet_attack(PET_ADULT_COPPER_DRAGON, 1, 50, 6, 6, 24);
add_pet_attack(PET_ADULT_COPPER_DRAGON, 2, 50, 4, 8, 18);
pet_mount(PET_ADULT_COPPER_DRAGON, SIZE_HUGE, 150);
pet_flying(PET_ADULT_COPPER_DRAGON);

}

void free_summon(struct char_data *ch)
{
  if (ch->player_specials->mounted == MOUNT_SUMMON)
    do_dismount(ch, 0, 0, 0);

  byte i = 0;

  ch->player_specials->summon_num = 0;
//  if (ch->player_specials->summon_desc != NULL)
//    free(ch->player_specials->summon_desc);
  ch->player_specials->summon_max_hit = 0;
  ch->player_specials->summon_cur_hit = 0;
  ch->player_specials->summon_ac = 0;
  ch->player_specials->summon_dr = 0;

  for (i = 0; i < 5; i++) {
    ch->player_specials->summon_attack_to_hit[i] = 0;
    ch->player_specials->summon_attack_ndice[i] = 0;
    ch->player_specials->summon_attack_sdice[i] = 0;
    ch->player_specials->summon_attack_dammod[i] = 0;
  }


}

void free_mount(struct char_data *ch)
{

  if (ch->player_specials->mounted == MOUNT_MOUNT)
    do_dismount(ch, 0, 0, 0);

  byte i = 0;

  ch->player_specials->mount_num = 0;
//  if (ch->player_specials->mount_desc != NULL)
//    free(ch->player_specials->mount_desc);
  ch->player_specials->mount_max_hit = 0;
  ch->player_specials->mount_cur_hit = 0;
  ch->player_specials->mount_ac = 0;
  ch->player_specials->mount_dr = 0;

  for (i = 0; i < 5; i++) {
    ch->player_specials->mount_attack_to_hit[i] = 0;
    ch->player_specials->mount_attack_ndice[i] = 0;
    ch->player_specials->mount_attack_sdice[i] = 0;
    ch->player_specials->mount_attack_dammod[i] = 0;
  }

}

void free_companion(struct char_data *ch)
{

  if (ch->player_specials->mounted == MOUNT_COMPANION)
    do_dismount(ch, 0, 0, 0);

  byte i = 0;

  ch->player_specials->companion_num = 0;
//  if (ch->player_specials->companion_desc != NULL)
//    free(ch->player_specials->companion_desc);
  ch->player_specials->companion_max_hit = 0;
  ch->player_specials->companion_cur_hit = 0;
  ch->player_specials->companion_ac = 0;
  ch->player_specials->companion_dr = 0;

  for (i = 0; i < 5; i++) {
    ch->player_specials->companion_attack_to_hit[i] = 0;
    ch->player_specials->companion_attack_ndice[i] = 0;
    ch->player_specials->companion_attack_sdice[i] = 0;
    ch->player_specials->companion_attack_dammod[i] = 0;
  }

}
