#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "spec_procs.h"
#include "feats.h"
#include "oasis.h"

void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);

extern struct auction_house_data *auction_list;

void auction_add(int price, char *seller, long idnum, long date_sold, int active);

SPECIAL(auction_house_old)
{

  if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("sell") && !CMD_IS("try"))
    return 0;

  if (CMD_IS("list")) {

  }

  if (!auction_list) {
    send_to_char(ch, "There is nothing for sale on the auction house right now.\r\n");
    return 1;
  }

  if (CMD_IS("buy")) {

  }
  else if (CMD_IS("sell")) {

  }
  else if (CMD_IS("try")) {

  }
  return 1;
}

void load_auction_house(void) {
  FILE *fl;
  char filename[MAX_STRING_LENGTH]={'\0'};
  char buf2[MAX_STRING_LENGTH]={'\0'};
  char line[256]={'\0'};
  int t[21],danger,zwei=0;
  struct obj_data *temp;
  int j, nr,k,num_objs=0;
  struct extra_descr_data *new_descr;
  int price, active;
  long idnum, date_sold;
  char *seller = NULL;
  
  sprintf(filename, "%s", AUCTION_FILE);

  if (!(fl = fopen(filename, "rb"))) {
    return;
  }

  if(!feof(fl))
    get_line(fl, line);

  while (!feof(fl)) {
        temp=NULL;
        /* first, we get the number. Not too hard. */
    if(*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) {   /* then it is unique */
        temp = create_obj();
        temp->item_number=NOTHING;
        GET_OBJ_SIZE(temp) = SIZE_MEDIUM;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 999999) 
          continue;
        temp=read_object(nr,VIRTUAL);
        if (!temp) {
	  get_line(fl, line);
          continue;
        }
      }

      get_line(fl,line);

      sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, 
                   t + 10, t + 11, t + 12, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18, t + 19, t + 20);
      GET_OBJ_VAL(temp,0) = t[1];
      GET_OBJ_VAL(temp,1) = t[2];
      GET_OBJ_VAL(temp,2) = t[3];
      GET_OBJ_VAL(temp,3) = t[4];
      GET_OBJ_VAL(temp,4) = t[5];
      GET_OBJ_VAL(temp,5) = t[6];
      GET_OBJ_VAL(temp,6) = t[7];
      GET_OBJ_VAL(temp,7) = t[8];
      GET_OBJ_EXTRA(temp)[0] = t[9];
      GET_OBJ_EXTRA(temp)[1] = t[10];
      GET_OBJ_EXTRA(temp)[2] = t[11];
      GET_OBJ_EXTRA(temp)[3] = t[12];
      GET_OBJ_VAL(temp,8) = t[13];
      GET_OBJ_VAL(temp,9) = t[14];
      GET_OBJ_VAL(temp,10) = t[15];
      GET_OBJ_VAL(temp,11) = t[16];
      GET_OBJ_VAL(temp,12) = t[17];
      GET_OBJ_VAL(temp,13) = t[18];
      GET_OBJ_VAL(temp,14) = t[19];
      GET_OBJ_VAL(temp,15) = t[20];

      get_line(fl,line);
       /* read line check for xap. */
      if(!strcmp("XAP",line)) {  /* then this is a Xap Obj, requires
                                       special care */
        if ((temp->name = fread_string(fl, "rented object name")) == NULL) {
          temp->name = "undefined";
        }

        if ((temp->short_description = fread_string(fl, "rented object short desc")) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fl, "rented object desc")) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fl, "rented object adesc")) == NULL) {
          temp->action_description=0;
        }

        if (!get_line(fl, line) ||
           (sscanf(line, "%d %d %d %d %d %d %d %d", t,t+1,t+2,t+3,t+4,t+5,t+6,t+7) != 8)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return;
        }
        temp->type_flag = t[0];
        temp->wear_flags[0] = t[1];
        temp->wear_flags[1] = t[2];
        temp->wear_flags[2] = t[3];
        temp->wear_flags[3] = t[4];
        temp->weight = t[5];
        temp->cost = t[6];
        GET_OBJ_LEVEL(temp) = t[7];

        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          temp->affected[j].location = APPLY_NONE;
          temp->affected[j].modifier = 0;
          temp->affected[j].specific = 0;
        }

        free_extra_descriptions(temp->ex_description);
        temp->ex_description = NULL;

        get_line(fl,line);
        for (k=j=zwei=0;!zwei && !feof(fl);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              sprintf(buf2, "rented object edesc keyword for object #%d", nr);
              new_descr->keyword = fread_string(fl, buf2);
              sprintf(buf2, "rented object edesc text for object #%d keyword %s", nr, new_descr->keyword);
              new_descr->description = fread_string(fl, buf2);
              new_descr->next = temp->ex_description;
              temp->ex_description = new_descr;
              get_line(fl,line);
              break;
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                log("SYSERR: Too many object affectations in loading rent file");
                danger=1;
              }
              get_line(fl, line);
              sscanf(line, "%d %d %d", t, t + 1, t + 2);

              temp->affected[j].location = t[0];
              temp->affected[j].modifier = t[1];  
              temp->affected[j].specific = t[2];  
              j++;
              //GET_OBJ_LEVEL(temp) = set_object_level(temp);
              get_line(fl,line);
              break;
            case 'G':
              get_line(fl, line);
              sscanf(line, "%ld", (long int *)&temp->generation);
              get_line(fl, line);
              break;
            case 'U':
              get_line(fl, line);
              sscanf(line, "%lld", &temp->unique_id);
              get_line(fl, line);
              break;
            case 'S':
              if (j >= SPELLBOOK_SIZE) {
                log("SYSERR: Too many spells in spellbook loading rent file");
                danger=1;
              }
              get_line(fl, line);
              sscanf(line, "%d %d", t, t + 1);

              if (!temp->sbinfo) {
                CREATE(temp->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
                memset((char *) temp->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
              }
              temp->sbinfo[j].spellname = t[0];
              temp->sbinfo[j].pages = t[1];  
              j++;
              get_line(fl,line);
              break;
            case 'Z':
              get_line(fl, line);
              sscanf(line, "%d", (int *)&GET_OBJ_SIZE(temp));
              get_line(fl, line);
              break;
            case '$':
            case '#':
              zwei=1;
              break;
            default:
              zwei=1;
              break;
          }
        }      /* exit our for loop */
      }   /* exit our xap loop */
      if(temp != NULL) {
        num_objs++;
        check_unique_id(temp);
        add_unique_id(temp);
        if (GET_OBJ_TYPE(temp) == ITEM_DRINKCON) {
          name_from_drinkcon(temp);
          if (GET_OBJ_VAL(temp, 1) != 0 )
            name_to_drinkcon(temp, GET_OBJ_VAL(temp, 2));
        }
      } else {
        continue;
      }
      get_line(fl, line);
      sscanf(line, "%d %s %ld %ld %d", &price, seller, &idnum, &date_sold, &active);
      auction_add(price, seller, idnum, date_sold, active);

      } else {
         get_line(fl, line);
      }
    }
}

void save_auction_house(void) 
{
  char filename[50]={'\0'};
  FILE *fl;
  struct auction_house_data *ptr = NULL;

  sprintf(filename, "%s", AUCTION_FILE);

  if (!(fl = fopen(filename, "wb"))) {
    return;
  }

  ptr = auction_list;

  while (ptr != NULL) {
    my_obj_save_to_disk(fl, ptr->obj, 0);
    fprintf(fl, "%d %s %ld %ld %d\n", ptr->price, ptr->seller, ptr->idnum,
            (long int)ptr->date_sold, ptr->active);
    ptr = ptr->next;
  }

  fclose(fl);
}

void auction_add(int price, char *seller, long idnum, long date_sold, int active) {

  struct auction_house_data *ptr;

  CREATE(ptr, struct auction_house_data, sizeof(struct auction_house_data));

  ptr->price = price;
  ptr->seller = strdup(seller);
  ptr->idnum = idnum;
  ptr->date_sold = date_sold;
  ptr->active = active;
  ptr->next = auction_list;
  auction_list = ptr;
}

