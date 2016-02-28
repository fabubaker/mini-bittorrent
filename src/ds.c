/****************************************************************/
/* @file ds.c                                                   */
/*                                                              */
/* @brief Contains custom data structures built by the authors. */
/*                                                              */
/* @authors Fadhil Abubaker, Malek Anabtawi                     */
/****************************************************************/

#include "ds.h"

/* Linked list for storing characters */

ll* create_ll()
{
  ll* new = malloc(sizeof(ll));

  new->first = NULL;
  new->last  = NULL;
  new->count = 0;

  return new;
}

ll* add_node(ll* list, char* data, size_t n)
{
  if()

  node* new = malloc(sizeof(node));
  bzero(new, sizeof(node));



}

/* End Linked list */
