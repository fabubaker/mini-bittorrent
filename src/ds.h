/********************************************/
/* @file ds.h                               */
/*                                          */
/* @brief Contains headers for ds.c         */
/*                                          */
/* @authors Fadhil Abubaker, Malek Anabtawi */
/********************************************/

#define DATA_SIZE 512

/* Linked list for storing characters */

typedef struct node {
  char  data[DATA_SIZE];
  node* next;

} node;

typedef struct linkedlist {
  node*  first;
  size_t count;
  node*  last;

} ll;

ll*   create_ll();
void  remove_ll(ll* list);

void  add_node(ll* list, char* data, size_t n);
void  remove_node(ll* list, char* data);

/* End Linked list */
