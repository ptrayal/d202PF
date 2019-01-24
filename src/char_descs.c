/* Copywright 2007 Stephen Squires */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "dg_scripts.h"
#include "char_descs.h"

extern const char *genders[];
extern char *pc_race_types[];
const char *eye_descriptions[];
char *nose_descriptions[];
char *ear_descriptions[];
char *face_descriptions[];
char *scar_descriptions[];
char *hair_descriptions[];
char *build_descriptions[];
char *complexion_descriptions[];


const char *eye_descriptions[] = 
{
  "undefined",
  "blue",
  "green",
  "brown",
  "hazel",
  "aquamarine",
  "emerald",
  "sapphire",
  "chocolate",
  "honey-tinged",
  "wild-looking",
  "crazed",
  "gleaming",
  "intelligent",
  "wise-looking",
  "shimmering",
  "stern",
  "calm-looking",
  "serene",
  "serious-looking",
  "sad",
  "cheerful",
  "intense",
  "fierce",
  "wild",
  "kind-looking",
  "lavender",
  "tired-looking",
  "bored-looking",
  "bright",
  "alert",
  "angry-looking",
  "mischevious",
  "glaring",
  "appraising",
  "mysterious",
  "emotionless",
  "blank",
  "disapproving",
  "caring",
  "compassionate",
  "fearful",
  "fearless",
  "searching",
  "\n",
  NULL
};

char *nose_descriptions[] = 
{
  "undefined",
  "big",
  "large",
  "small",
  "button",
  "crooked",
  "hooked",
  "hawk-like",
  "bulbous",
  "thin",
  "long",
  "misshapen",
  "broken",
  "noble-looking",
  "short and thin",
  "short and wide",
  "long and wide",
  "long and thin",
  "perfect-looking",
  "rosy-red",
  "runny",
  "hairy",
  "fair",
  "pretty",
  "handsome",
  "\n",
  NULL
};

char *ear_descriptions[] = 
{
  "undefined",
  "large",
  "big",
  "long",
  "small",
  "prominent",
  "misshapen",
  "pointed",
  "scarred",
  "tattooed",
  "pierced",
  "tattooed",
  "multiply pierced",
  "astute",
  "alert",
  "scarred",
  "\n",
  NULL
};


char *face_descriptions[] = 
{
  "undefined",
  "handsome",
  "pretty",
  "ugly",
  "attractive",
  "comely",
  "unattractive",
  "scarred",
  "monstrous",
  "fierce",
  "mouse-like",
  "hawk-like",
  "rat-like",
  "horse-like",
  "bullish",
  "fine",
  "noble",
  "chubby",
  "fair",
  "bull-like",
  "pig-like",
  "masculine",
  "feminine",
  "boyish",
  "girlish",
  "garish",
  "frightening",
  "demon-like",
  "angellic",
  "celestial",
  "infernal",
  "\n",
  NULL
};

char *scar_descriptions[] = 
{
  "undefined",
  "a scar on the right cheek",
  "a scar on the left cheek",
  "a scar across the left eye",
  "a scar across the right eye",
  "a scar across the chin",
  "a scar across the nose",
  "a scar across the neck",
  "scars all over the face",
  "a scar on the left hand",
  "a scar on the right hand",
  "a scar on the left arm",
  "a scar on the right arm",
  "a scar across the chest",
  "a scar across the torso",
  "scars all over the body",
  "a scar on the back",
  "scars all over the back",  
  "a scar on the left leg",
  "a scar on the right leg",
  "a scar on the left foot",
  "a scar on the right foot",
  "scars all over the legs",
  "\n",
  NULL
};

char *hair_descriptions[] = 
{
  "undefined",
  "short red hair",
  "long red hair",
  "cropped red hair",
  "a red pony-tail",
  "a red topknot",
  "a red crown of hair",

  "short auburn hair",
  "long auburn hair",
  "cropped auburn hair",
  "an auburn pony-tail",
  "an auburn topknot",
  "an auburn crown of hair",

  "short blonde hair",
  "long blonde hair",
  "cropped blonde hair",
  "a blonde pony-tail",
  "a blonde topknot",
  "a blonde crown of hair",

  "short sandy-blonde hair",
  "long sandy-blonde hair",
  "cropped sandy-blonde hair",
  "a sandy-blonde pony-tail",
  "a sandy-blonde topknot",
  "a sandy-blonde crown of hair",

  "short brown hair",
  "long brown hair",
  "cropped brown hair",
  "a brown pony-tail",
  "a brown topknot",
  "a brown crown of hair",

  "short black hair",
  "long black hair",
  "cropped black hair",
  "a black pony-tail",
  "a black topknot",
  "a black crown of hair",

  "short white hair",
  "long white hair",
  "cropped white hair",
  "a white pony-tail",
  "a white topknot",
  "a white crown of hair",

  "short silver hair",
  "long silver hair",
  "cropped silver hair",
  "a silver pony-tail",
  "a silver topknot",
  "a white crown of silver",

  "long dreadlocks",
  "long, braided hair",
  "braided corn-rows",
  "a shaven, tattooed scalp",
  "a bald, misshapen head",
  "a single, long braid",
  "a shaven head and braided topknot",
  "a bald head",
  "a cleanly-shaven head",
  "a pair of short horns",
  "a pair of long horns",
  "a pair of curved horns",
  
  "\n",
  NULL
};

char *build_descriptions[] = 
{
  "undefined",
  "tall",
  "short",
  "stocky",
  "average",
  "huge",
  "monstrous",
  "gigantic",
  "tiny",
  "delicate",
  "frail",
  "towering",
  "snake-like",
  "wiry",
  "bulging",
  "obese",
  "chubby",
  "muscular",
  "thin",
  "athletic",
  "barrel-chested",
  "sinewy",
  "sculpted",
  "solid",
  "normal",
  "lithe",
  "curvy",
  "\n",
  NULL
};

char *complexion_descriptions[] = 
{
  "undefined",
  "white",
  "pale",
  "dark",
  "tanned",
  "bronzed",
  "freckled",
  "weathered",
  "brown",
  "black",
  "chocolate",
  "olive",
  "fair",
  "jet black",
  "\n",
  NULL
};

char *current_short_desc(struct char_data *ch) 
{
  int i = 0;
  char desc[100]={'\0'};
  char *tempDesc;

  int race = GET_RACE(ch);
  int sex = GET_SEX(ch);
  int pcd1 = GET_PC_DESCRIPTOR_1(ch);
  int pca1 = GET_PC_ADJECTIVE_1(ch);
  int pcd2 = GET_PC_DESCRIPTOR_2(ch);
  int pca2 = GET_PC_ADJECTIVE_2(ch);

  if (AFF_FLAGGED(ch, AFF_DISGUISED) && !DISGUISE_SEEN(ch)) 
  {
      race = GET_DISGUISE_RACE(ch);
      sex  = GET_DISGUISE_SEX(ch);
      pcd1 = GET_DISGUISE_DESC_1(ch);
      pca1 = GET_DISGUISE_ADJ_1(ch);
      pcd2 = GET_DISGUISE_DESC_2(ch);
      pca2 = GET_DISGUISE_ADJ_2(ch);
  }

  sprintf(desc, "a %s %s", genders[(int) sex], pc_race_types[(int) race]);

  switch (pcd1) 
  {
  case FEATURE_TYPE_EYES:
    sprintf(desc, "%s with %s eyes", desc, eye_descriptions[pca1]);
    break;
  case FEATURE_TYPE_NOSE:
    sprintf(desc, "%s with %s %s nose", desc, AN(nose_descriptions[pca1]), nose_descriptions[pca1]);
    break;
  case FEATURE_TYPE_EARS:
    sprintf(desc, "%s with %s ears", desc, ear_descriptions[pca1]);
    break;
  case FEATURE_TYPE_FACE:
    sprintf(desc, "%s with %s features", desc, face_descriptions[pca1]);
    break;
  case FEATURE_TYPE_SCAR:
    sprintf(desc, "%s with %s", desc, scar_descriptions[pca1]);
    break;
  case FEATURE_TYPE_HAIR:
    sprintf(desc, "%s with %s", desc, hair_descriptions[pca1]);
    break;
  case FEATURE_TYPE_BUILD:
    sprintf(desc, "%s with %s %s frame", desc, AN(build_descriptions[pca1]), build_descriptions[pca1]);
    break;
  case FEATURE_TYPE_COMPLEXION:
    sprintf(desc, "%s with %s %s complexion", desc, AN(complexion_descriptions[pca1]), complexion_descriptions[GET_PC_ADJECTIVE_1(ch)]);
    break;
  }
  
  switch (pcd2) 
  {
  case FEATURE_TYPE_EYES:
    sprintf(desc, "%s and %s eyes", desc, eye_descriptions[pca2]);
    break;
  case FEATURE_TYPE_NOSE:
    sprintf(desc, "%s and %s %s nose", desc, AN(nose_descriptions[pca2]), nose_descriptions[pca2]);
    break;
  case FEATURE_TYPE_EARS:
    sprintf(desc, "%s and %s ears", desc, ear_descriptions[pca2]);
    break;
  case FEATURE_TYPE_FACE:
    sprintf(desc, "%s and %s features", desc, face_descriptions[pca2]);
    break;
  case FEATURE_TYPE_SCAR:
    sprintf(desc, "%s and %s", desc, scar_descriptions[pca2]);
    break;
  case FEATURE_TYPE_HAIR:
    sprintf(desc, "%s and %s", desc, hair_descriptions[pca2]);
    break;
  case FEATURE_TYPE_BUILD:
    sprintf(desc, "%s and %s %s frame", desc, AN(build_descriptions[pca2]), build_descriptions[pca2]);
    break;
  case FEATURE_TYPE_COMPLEXION:
    sprintf(desc, "%s and %s %s complexion", desc, AN(complexion_descriptions[pca2]), complexion_descriptions[pca2]);
    break;
  
  }

  if (DISGUISE_SEEN(ch) && GET_DISGUISE_ROLL(ch) > 0) {
      sprintf(desc, "%s (disguised)", desc);
  }

  for (i = 0; i < strlen(desc); i++)
    desc[i] = tolower(desc[i]);

  if (!IS_HUMANOID_RACE(race)) {
    tempDesc = strdup(desc);
    replace_string(tempDesc, "hair", "fur");
    return tempDesc;
  }
  
  return strdup(desc);

}

void short_desc_descriptors_menu(struct char_data *ch) 
{

  SEND_TO_Q("Please choose a descriptor from the list.  This will determine what kind of feature\r\n"
               "you wish to add to your short description.  Once chosen you will choose a specific\r\n"
               "describing adjective for the feature you chose.\r\n\r\n", ch->desc);

  SEND_TO_Q("1) Describe Eyes\r\n"
            "2) Describe Nose\r\n"
            "3) Describe Ears\r\n"
            "4) Describe Face\r\n"
            "5) Describe Scars\r\n"
            "6) Describe Hair\r\n"
            "7) Describe Build\r\n"
            "8) Describe Complexion\r\n\r\n", ch->desc);

}

void short_desc_adjectives_menu(struct char_data *ch, int which_desc) 
{
  char buf[100]={'\0'};
  int i = 0;

  SEND_TO_Q("Please choose an adjective to describe the descriptor you just chose.\r\n\r\n", ch->desc);


  switch (which_desc) 
  {
    case FEATURE_TYPE_EYES:
    while (i < NUM_EYE_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, eye_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_NOSE:
    while (i < NUM_NOSE_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, nose_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_EARS:
    while (i < NUM_EAR_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s n", i, ear_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_FACE:      
    while (i < NUM_FACE_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, face_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_SCAR:      
    while (i < NUM_SCAR_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, scar_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_HAIR:      
    while (i < NUM_HAIR_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, hair_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_BUILD:     
    while (i < NUM_BUILD_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, build_descriptions[i]); 
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
    case FEATURE_TYPE_COMPLEXION:
    while (i < NUM_COMPLEXION_DESCRIPTORS) 
    {
      sprintf(buf, "%d) %-30s ", i, complexion_descriptions[i]);
      if (i % 2 == 1)
        sprintf(buf, "%s\r\n", buf);
      SEND_TO_Q(buf, ch->desc);
      i++;
    }
    break;
  }
  
  if (i % 2 == 0)
    SEND_TO_Q("\r\n", ch->desc); 

  SEND_TO_Q("\r\n", ch->desc);

}


int count_adjective_types(int which_desc) 
{

  int i = 0;

  switch (which_desc) 
  {
    case FEATURE_TYPE_EYES:
    while (i < NUM_EYE_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_NOSE:
    while (i < NUM_NOSE_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_EARS:
    while (i < NUM_EAR_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_FACE:      
    while (i < NUM_FACE_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_SCAR:      
    while (i < NUM_SCAR_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_HAIR:      
    while (i < NUM_HAIR_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_BUILD:     
    while (i < NUM_BUILD_DESCRIPTORS)
      i++;
    break;
    case FEATURE_TYPE_COMPLEXION:
    while (i < NUM_COMPLEXION_DESCRIPTORS)
      i++;
    break;
  }

  return i;
}
