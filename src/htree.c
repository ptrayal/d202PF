/***************************************************************************
 *   File: htree.c                                                         *
 *  Usage: Generalized hash tree code for fast lookups                     *
 *                                                                         *
 * This code is released under the CircleMud License                       *
 * Written by Elie Rosenblum <fnord@cosanostra.net>                        *
 * Copyright (c) 7-Oct-2004                                                *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

SVNHEADER("$Id: htree.c 55 2009-03-20 17:58:56Z pladow $");

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "htree.h"

#undef HTREE_TEST_CYCLES

struct htree_node *HTREE_NULL = NULL;
int htree_total_nodes = 0;
int htree_depth_used = 0;

void htree_shutdown()
{
  free(HTREE_NULL);
  HTREE_NULL = NULL;
}

struct htree_node *htree_init()
{
  struct htree_node *newnode;
  int i;

  if (! HTREE_NULL) {
    htree_total_nodes++;
    CREATE(HTREE_NULL, struct htree_node, 1);
    for (i = 0; i < HTREE_NODE_SUBS; i++) {
      HTREE_NULL->subs[i] = HTREE_NULL;
    }
    HTREE_NULL->content = NOWHERE;
    HTREE_NULL->parent = NULL;
  }

  if (! htree_depth_used)
    htree_depth_used = 1;

  htree_total_nodes++;
  CREATE(newnode, struct htree_node, 1);
  memcpy(newnode->subs, HTREE_NULL->subs, HTREE_NODE_SUBS * sizeof(struct htree_node *));
  newnode->content = NOWHERE;
  newnode->parent = HTREE_NULL;

  return newnode;
}

void htree_free(struct htree_node *root)
{
  int i;

  if (! root || root == HTREE_NULL)
    return;

  for (i = 0; i < HTREE_NODE_SUBS; i++)
    htree_free(root->subs[i]);

  free(root);
}

void htree_add(struct htree_node *root, IDXTYPE index, IDXTYPE content)
{
  struct htree_node *tmp;
  int i, depth;

  if (! root)
    return;

  tmp = root;
  depth = 0;
  while (index) {
    depth++;
    i = index & HTREE_NODE_MASK;
    index >>= HTREE_NODE_BITS;
    if (tmp->subs[i] == HTREE_NULL) {
      htree_total_nodes++;
      CREATE(tmp->subs[i], struct htree_node, 1);
      memcpy(tmp->subs[i]->subs, HTREE_NULL->subs, HTREE_NODE_SUBS * sizeof(struct htree_node *));
      tmp->subs[i]->content = NOWHERE;
      tmp->subs[i]->parent = HTREE_NULL;
    }
    tmp = tmp->subs[i];
  }

  if (tmp == HTREE_NULL) /* We fell off somehow! Time to crap our pants */
    return;

  if (depth > htree_depth_used)
    htree_depth_used = depth;

  tmp->content = content;
}

struct htree_node *htree_find_node(struct htree_node *root, IDXTYPE index)
{
  struct htree_node *tmp;
  int i;
  
  tmp = root;
  while (index) {
    i = index & HTREE_NODE_MASK;
    index >>= HTREE_NODE_BITS;
    tmp = tmp->subs[i];
  }

  return tmp;
}

void htree_del(struct htree_node *root, IDXTYPE index)
{
  struct htree_node *tmp;

  tmp = htree_find_node(root, index);
  tmp->content = NOWHERE;
}

IDXTYPE htree_find(struct htree_node *root, IDXTYPE index)
{
  struct htree_node *tmp;

  tmp = htree_find_node(root, index);
  return tmp->content;
}

room_rnum real_room_old(room_vnum vnum)
{
  room_rnum bot, top, mid;
  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return (mid);

    if (bot >= top)
      return (NOWHERE);
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

void htree_test()
{
#ifdef HTREE_TEST_CYCLES
  int i, n, l;
  struct timeval start, finish;
  float t1, t2;

  if (gettimeofday(&start, NULL)) {
    log("error getting time: gettimeofday(): %s", strerror(errno));
  }
  for (i = 0; i < HTREE_TEST_CYCLES; i++) {
    n = rand_number(1, top_of_world);
    l = real_room_old(world[n].number);
  }
  if (gettimeofday(&finish, NULL)) {
    log("error getting time: gettimeofday(): %s", strerror(errno));
  }
  log("old start: %2d.%06d", start.tv_sec, start.tv_usec);
  log("old   end: %2d.%06d", finish.tv_sec, finish.tv_usec);
  t1 = ((float)finish.tv_sec + ((float)finish.tv_usec) / 1000000) -
       ((float)start.tv_sec + ((float)start.tv_usec) / 1000000);
  if (gettimeofday(&start, NULL)) {
    log("error getting time: gettimeofday(): %s", strerror(errno));
  }
  for (i = 0; i < HTREE_TEST_CYCLES; i++) {
    n = rand_number(1, top_of_world);
    l = real_room(world[n].number);
  }
  if (gettimeofday(&finish, NULL)) {
    log("error getting time: gettimeofday(): %s", strerror(errno));
  }
  log("new start: %2d.%06d", start.tv_sec, start.tv_usec);
  log("new   end: %2d.%06d", finish.tv_sec, finish.tv_usec);
  t2 = ((float)finish.tv_sec + ((float)finish.tv_usec) / 1000000) -
       ((float)start.tv_sec + ((float)start.tv_usec) / 1000000);
  log("htree_test: htree speedup factor: %.0f%% (%.2f/%.2f)", t1 * 100 / t2, t1, t2);
#endif /* HTREE_TEST_CYCLES */
  log("htree stats (global): %d nodes, %lu bytes (depth %d/%lu used/possible)", htree_total_nodes, (long unsigned int)(htree_total_nodes * sizeof(struct htree_node)), htree_depth_used, (long unsigned int)HTREE_MAX_DEPTH);
}
