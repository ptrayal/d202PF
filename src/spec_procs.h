/*
  spec_procs.h

  defines etc for spec_procs.c
*/


#ifndef SPECIAL
#error structs.h must be included!
#else
typedef SPECIAL(*proctype);
char *get_spec_name(SPECIAL(func));
proctype get_spec_proc(char *name);
void list_spec_procs(struct char_data *ch);
#endif

#define SPEC_PROC_NONE		0
#define MIN_MOB_SPECS           (SPEC_PROC_NONE + 1)
#define MAX_MOB_SPECS           12
#define MIN_OBJ_SPECS           MIN_MOB_SPECS
#define MAX_OBJ_SPECS           MAX_MOB_SPECS
#define MIN_ROOM_SPECS          MIN_MOB_SPECS
#define MAX_ROOM_SPECS          MAX_MOB_SPECS

struct spec_list {
  char name[25]; 
  SPECIAL(*func);
};

