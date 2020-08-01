/* ************************************************************************
*  file:  showplay.c                                  Part of CircleMud   *
*  Usage: list a diku playerfile                                          *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"


void show(char *filename)
{
  FILE *fl;
  struct player_index_element player;
  long size;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  fseek(fl, 0L, SEEK_END);
  size = ftell(fl);
  rewind(fl);
  if (size % sizeof(player)) {
    fprintf(stderr, "\aWARNING:  File size does not match structure, recompile showplay.\n");
    fclose(fl);
    exit(1);
  }

  for (;;) {
    fread(&player, sizeof(player), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      exit(0);
    }
  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    show(argv[1]);

  return (0);
}
