/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"
#include "dg_olc.h"
#include "htree.h"

SVNHEADER("$Id: genobj.c 55 2009-03-20 17:58:56Z pladow $");


static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);

extern struct obj_data *obj_proto;
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct shop_data *shop_index;
extern zone_rnum top_of_zone_table;
extern obj_rnum top_of_objt;
extern int top_shop;

obj_rnum add_object(struct obj_data *newobj, obj_vnum ovnum)
{
  int found = NOTHING;
  zone_rnum rznum = real_zone_by_thing(ovnum);

  /*
   * Write object to internal tables.
   */
  if ((newobj->item_number = real_object(ovnum)) != NOTHING) {
    copy_object(&obj_proto[newobj->item_number], newobj);
    update_objects(&obj_proto[newobj->item_number]);
    add_to_save_list(zone_table[rznum].number, SL_OBJ);
    return newobj->item_number;
  }

  found = insert_object(newobj, ovnum);
  adjust_objects(found);
  add_to_save_list(zone_table[rznum].number, SL_OBJ);
  return found;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
int update_objects(struct obj_data *refobj)
{
  struct obj_data *obj, swap;
  int count = 0;

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != refobj->item_number)
      continue;

    count++;

    /* Update the existing object but save a copy for private information. */
    swap = *obj;
    *obj = *refobj;

    /* Copy game-time dependent variables over. */
    IN_ROOM(obj) = swap.in_room;
    obj->carried_by = swap.carried_by;
    obj->worn_by = swap.worn_by;
    obj->worn_on = swap.worn_on;
    obj->in_obj = swap.in_obj;
    obj->contains = swap.contains;
    obj->next_content = swap.next_content;
    obj->next = swap.next;
  }

  return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Adjust the internal values of other objects as if something was inserted at the given array index.
 * Might also be useful to make 'holes' in the array for some reason.
 */
obj_rnum adjust_objects(obj_rnum refpt)
{
  int shop, i, zone, cmd_no;
  struct obj_data *obj;

#if CIRCLE_UNSIGNED_INDEX
  if (refpt == NOTHING || refpt > top_of_objt)
#else
  if (refpt < 0 || refpt > top_of_objt)
#endif
    return NOTHING;

  /*
   * Renumber live objects.
   */
  for (obj = object_list; obj; obj = obj->next)
    GET_OBJ_RNUM(obj) += (GET_OBJ_RNUM(obj) >= refpt);

  /*
   * Renumber zone table.
   */
  for (zone = 0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++) {
      switch (ZCMD(zone, cmd_no).command) {
      case 'P':
        ZCMD(zone, cmd_no).arg3 += (ZCMD(zone, cmd_no).arg3 >= refpt);
         /*
          * No break here - drop into next case.
          */
      case 'O':
      case 'G':
      case 'E':
        ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= refpt);
        break;
      case 'R':
        ZCMD(zone, cmd_no).arg2 += (ZCMD(zone, cmd_no).arg2 >= refpt);
        break;
      }
    }
  }

  /*
   * Renumber shop produce.
   */
  for (shop = 0; shop <= top_shop - top_shop_offset; shop++)
    for (i = 0; SHOP_PRODUCT(shop, i) != NOTHING; i++)
      SHOP_PRODUCT(shop, i) += (SHOP_PRODUCT(shop, i) >= refpt);

  return refpt;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Function handle the insertion of an object within the prototype framework.  Note that this does not adjust internal values
 * of other objects, use add_object() for that.
 */
obj_rnum insert_object(struct obj_data *obj, obj_vnum ovnum)
{
  obj_rnum i;

  top_of_objt++;
  RECREATE(obj_index, struct index_data, top_of_objt + 1);
  RECREATE(obj_proto, struct obj_data, top_of_objt + 1);

  /*
   * Start counting through both tables.
   */
  for (i = top_of_objt; i > 0; i--) {
    /*
     * Check if current virtual is bigger than our virtual number.
     */
    if (ovnum > obj_index[i - 1].vnum)
      return index_object(obj, ovnum, i);

    /* Copy over the object that should be here. */
    obj_index[i] = obj_index[i - 1];
    obj_proto[i] = obj_proto[i - 1];
    obj_proto[i].item_number = i;
    htree_add(obj_htree, obj_index[i].vnum, i);
  }

  /* Not found, place at 0. */
  return index_object(obj, ovnum, 0);
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

obj_rnum index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum)
{
#if CIRCLE_UNSIGNED_INDEX
  if (obj == NULL || ornum == NOTHING || ornum > top_of_objt)
#else
  if (obj == NULL || ovnum < 0 || ornum < 0 || ornum > top_of_objt)
#endif
    return NOWHERE;

  obj->item_number = ornum;
  obj_index[ornum].vnum = ovnum;
  obj_index[ornum].number = 0;
  obj_index[ornum].func = NULL;

  copy_object_preserve(&obj_proto[ornum], obj);
  obj_proto[ornum].in_room = NOWHERE;

  htree_add(obj_htree, obj_index[ornum].vnum, ornum);

  return ornum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

int save_objects(zone_rnum zone_num)
{
  char fname[128], buf[MAX_STRING_LENGTH];
  char ebuf1[MAX_STRING_LENGTH], ebuf2[MAX_STRING_LENGTH];
  char ebuf3[MAX_STRING_LENGTH], ebuf4[MAX_STRING_LENGTH];
  char wbuf1[MAX_STRING_LENGTH], wbuf2[MAX_STRING_LENGTH];
  char wbuf3[MAX_STRING_LENGTH], wbuf4[MAX_STRING_LENGTH];
  char pbuf1[MAX_STRING_LENGTH], pbuf2[MAX_STRING_LENGTH];
  char pbuf3[MAX_STRING_LENGTH], pbuf4[MAX_STRING_LENGTH];
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table) {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table) {
#endif
    log("SYSERR: OasisOLC: save_objects: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  snprintf(fname, sizeof(fname), "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(fname, "w+"))) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: OLC: Cannot open objects file %s!", fname);
    return FALSE;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = genolc_zone_bottom(zone_num); counter <= zone_table[zone_num].top; counter++) {
    if ((realcounter = real_object(counter)) != NOTHING) {
      if ((obj = &obj_proto[realcounter])->action_description) {
	strncpy(buf, obj->action_description, sizeof(buf) - 1);
	strip_cr(buf);
      } else
	*buf = '\0';

      fprintf(fp,
	      "#%d\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n",

	      GET_OBJ_VNUM(obj),
	      (obj->name && *obj->name) ? obj->name : "undefined",
	      (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
	      (obj->description && *obj->description) ?	obj->description : "undefined",
	      buf);

      sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[0]);
      sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[1]);
      sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[2]);
      sprintascii(ebuf4, GET_OBJ_EXTRA(obj)[3]);
      sprintascii(wbuf1, GET_OBJ_WEAR(obj)[0]);
      sprintascii(wbuf2, GET_OBJ_WEAR(obj)[1]);
      sprintascii(wbuf3, GET_OBJ_WEAR(obj)[2]);
      sprintascii(wbuf4, GET_OBJ_WEAR(obj)[3]);
      sprintascii(pbuf1, GET_OBJ_PERM(obj)[0]);
      sprintascii(pbuf2, GET_OBJ_PERM(obj)[1]);
      sprintascii(pbuf3, GET_OBJ_PERM(obj)[2]);
      sprintascii(pbuf4, GET_OBJ_PERM(obj)[3]);

      fprintf(fp,
                "%d %s %s %s %s %s %s %s %s %s %s %s %s\n"
                "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"
	        "%d %d %d %d\n",

	      GET_OBJ_TYPE(obj), 
              ebuf1, ebuf2, ebuf3, ebuf4,
              wbuf1, wbuf2, wbuf3, wbuf4,
              pbuf1, pbuf2, pbuf3, pbuf4,
              GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), 
              GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5),
              GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7), GET_OBJ_VAL(obj, 8),
              GET_OBJ_VAL(obj, 9), GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11),
              GET_OBJ_VAL(obj, 12), GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14),
              GET_OBJ_VAL(obj, 15),
	      GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj)
      );

      /*
       * Do we have script(s) attached ? 
       */
      script_save_to_disk(fp, obj, OBJ_TRIGGER);
      fprintf(fp,
              "V\n"
              "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
              GET_OBJ_VAL(obj, 16), GET_OBJ_VAL(obj, 17), GET_OBJ_VAL(obj, 18),
              GET_OBJ_VAL(obj, 19), GET_OBJ_VAL(obj, 20), GET_OBJ_VAL(obj, 21),
              GET_OBJ_VAL(obj, 22), GET_OBJ_VAL(obj, 23), GET_OBJ_VAL(obj, 24),
              GET_OBJ_VAL(obj, 25), GET_OBJ_VAL(obj, 26), GET_OBJ_VAL(obj, 27),
              GET_OBJ_VAL(obj, 28), GET_OBJ_VAL(obj, 29), GET_OBJ_VAL(obj, 30),
              GET_OBJ_VAL(obj, 31)
      );      
      fprintf(fp, "Z\n%d\n", GET_OBJ_SIZE(obj));
      fprintf(fp, "D\n%ld\n", (obj)->date_sold);

      /*
       * Do we have extra descriptions? 
       */
      if (obj->ex_description) {	/* Yes, save them too. */
	for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
	  /*
	   * Sanity check to prevent nasty protection faults.
	   */
	  if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description) {
	    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
	    continue;
	  }
	  strncpy(buf, ex_desc->description, sizeof(buf) - 1);
	  strip_cr(buf);
	  fprintf(fp, "E\n"
		  "%s~\n"
		  "%s~\n", ex_desc->keyword, buf);
	}
      }
      /*
       * Do we have affects? 
       */
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
	if (obj->affected[counter2].modifier)
	  fprintf(fp, "A\n"
		  "%d %d %d\n", obj->affected[counter2].location,
		  obj->affected[counter2].modifier, obj->affected[counter2].specific);
      /* Do we have spells? */
        if (obj->sbinfo) {        /*. Yep, save them too . */
          for (counter2=0; counter2 < SKILL_TABLE_SIZE; counter2++) {
            if (obj->sbinfo[counter2].spellname == 0) {
              break;
            }
            fprintf(fp, "S\n" "%d %d\n", obj->sbinfo[counter2].spellname, obj->sbinfo[counter2].pages);
            continue;
          }
        }
    }
  }

  /*
   * Write the final line, close the file.
   */
  fprintf(fp, "$~\n");
  fclose(fp);
  snprintf(buf, sizeof(buf), "%s%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
  remove(buf);
  rename(fname, buf);

  if (in_save_list(zone_table[zone_num].number, SL_OBJ)) {
    remove_from_save_list(zone_table[zone_num].number, SL_OBJ);
    log("OasisOLC: Saving objects '%s'", buf);
  }
  return TRUE;
}

/*
 * Free all, unconditionally.
 */
void free_object_strings(struct obj_data *obj)
{
#if 0 /* Debugging, do not enable. */
  extern struct obj_data *object_list;
  struct obj_data *t;
  int i = 0;

  for (t = object_list; t; t = t->next) {
    if (t == obj) {
      i++;
      continue;
    }
    assert(obj->name != t->name);
    assert(obj->description != t->description);
    assert(obj->short_description != t->short_description);
    assert(obj->action_description != t->action_description);
    assert(obj->ex_description != t->ex_description);
  }
  assert(i <= 1);
#endif

  if (obj->name)
    free(obj->name);
  if (obj->description)
    free(obj->description);
  if (obj->short_description)
    free(obj->short_description);
  if (obj->action_description)
    free(obj->action_description);
  if (obj->ex_description)
    free_ex_descriptions(obj->ex_description);
}

/*
 * For object instances that are not the prototype.
 */
void free_object_strings_proto(struct obj_data *obj)
{
  int robj_num = GET_OBJ_RNUM(obj);

  if (obj->name && obj->name != obj_proto[robj_num].name)
    free(obj->name);
  if (obj->description && obj->description != obj_proto[robj_num].description)
    free(obj->description);
  if (obj->short_description && obj->short_description != obj_proto[robj_num].short_description)
    free(obj->short_description);
  if (obj->action_description && obj->action_description != obj_proto[robj_num].action_description)
    free(obj->action_description);
  if (obj->ex_description) {
    struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
    int ok_key, ok_desc, ok_item;
    for (thised = obj->ex_description; thised; thised = next_one) {
      next_one = thised->next;
      for (ok_item = ok_key = ok_desc = 1, plist = obj_proto[robj_num].ex_description; plist; plist = plist->next) {
        if (plist->keyword == thised->keyword)
          ok_key = 0;
        if (plist->description == thised->description)
          ok_desc = 0;
        if (plist == thised)
          ok_item = 0;
      }
      if (thised->keyword && ok_key)
        free(thised->keyword);
      if (thised->description && ok_desc)
        free(thised->description);
      if (ok_item)
        free(thised);
    }
  }
}

void copy_object_strings(struct obj_data *to, struct obj_data *from)
{
  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;

  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
}

int copy_object(struct obj_data *to, struct obj_data *from)
{
  free_object_strings(to);
  return copy_object_main(to, from, TRUE);
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, FALSE);
}

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object)
{
  *to = *from;
  copy_object_strings(to, from);
  return TRUE;
}
