/* ************************************************************************
*   File: boards.h                                      Part of CircleMUD *
*  Usage: header file for bulletin boards                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* New Dynamic boards v2.4 -PjD (dughi@imaxx.net) */
#define BOARD_DIRECTORY                "etc/boards"
#define MAX_MESSAGE_LENGTH	4096	/* arbitrary -- change if needed */

#define BOARD_MAGIC	1048575	/* arbitrary number - see modify.c */

/* Provides individual message structure */
/* doubly linked so forward or back is relatively simple */

struct board_msg {
  long poster;
  time_t timestamp;
  char *subject;
  char *data;
  struct board_msg *next;
  struct board_msg *prev;
  char *name;
};

/* Defines what we require to generate a hash for lookup
   of a message given a reader */

struct board_memory {
  int timestamp;
  int reader;
  struct board_memory *next;
  char *name;
};

struct board_info {
   int	read_lvl;	/* min level to read messages on this board */
   int	write_lvl;	/* min level to write messages on this board */
   int	remove_lvl;	/* min level to remove messages from this board */
  int  num_messages;           /* num messages of this board */
  int  vnum;
  struct board_info *next;
  struct board_msg *messages;
  int  version;
  
  /* why 301? why not?  It might not be the greatest, but if you really
     know what a hash is, you'll realize that in this case, I didn't even
     work on the algorithm, so it shouldn't make a bit of difference */
  
  struct board_memory *memory[301];
};

#define READ_LVL(i) (i->read_lvl)
#define WRITE_LVL(i) (i->write_lvl)
#define REMOVE_LVL(i) (i->remove_lvl)
#define BOARD_MNUM(i) (i->num_messages)
#define BOARD_VNUM(i) (i->vnum)
#define BOARD_NEXT(i) (i->next)
#define BOARD_MESSAGES(i) (i->messages)
#define BOARD_MEMORY(i,j) (i->memory[j])
#define BOARD_VERSION(i) 	 (i->version)
#define CURRENT_BOARD_VER	 2

#define MESG_POSTER(i) (i->poster)
#define MESG_TIMESTAMP(i) (i->timestamp)
#define MESG_SUBJECT(i) (i->subject)
#define MESG_DATA(i) (i->data)
#define MESG_NEXT(i) (i->next)
#define MESG_PREV(i) (i->prev)
#define MESG_POSTER_NAME(i)	 (i->name)

#define MEMORY_TIMESTAMP(i) (i->timestamp)
#define MEMORY_READER(i) (i->reader)
#define MEMORY_NEXT(i) (i->next)
#define MEMORY_READER_NAME(i)	 (i->name)

void init_boards(void);
struct board_info *create_new_board(obj_vnum board_vnum);
struct board_info *load_board(obj_vnum board_vnum);
int save_board(struct board_info *temp_board);
void clear_boards();
void clear_one_board(struct board_info *temp_board);
int parse_message( FILE *fl, struct board_info *temp_board);
void look_at_boards(void);
void show_board(obj_vnum board_vnum, struct char_data *ch);
void board_display_msg(obj_vnum board_vnum, struct char_data * ch, int arg);
int mesglookup(struct board_msg *message,struct char_data *ch,
	       struct board_info *board);

void write_board_message(obj_vnum board_vnum, struct char_data *ch, char *arg);
void board_respond(long board_vnum, struct char_data *ch, int mnum);

struct board_info *locate_board(obj_vnum board_vnum);

void remove_board_msg(obj_vnum board_vnum, struct char_data * ch, int arg);
