#include "conf.h"
#include "sysdep.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "improved-edit.h"



ACMD(do_forum)
{
  
  char forum[100];

  half_chop(argument, forum, argument);

  if (!*forum) {
    send_to_char(ch, "Which forum would you like to interact with?\r\n"
                     "   announcements\r\n"
		     "   general\r\n"
		     "   gameplay\r\n"
		     "   ooc\r\n"
		     "   roleplay\r\n"
		     "   trade\r\n"
		     "   feedback\r\n"
		     "   bugs\r\n"
		     "   offtopic\r\n");
    if (GET_ADMLVL(ch) > 0)
      send_to_char(ch,
                     "   staffannouncements\r\n"
                     "   staffgeneral\r\n"
		     "   staffcoding\r\n"
                     "   staffworld\r\n"
                     "   staffplayers\r\n"
                     "   staffother\r\n");
    if (GET_ADMLVL(ch) >= 4)
      send_to_char(ch,
                     "   staffadmins\r\n");

                   
  }



}
