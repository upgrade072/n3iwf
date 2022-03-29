#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LN_MAX_KEY_VAL 128
typedef struct link_node {
	struct link_node *prev;
	struct link_node *next;

	char key[LN_MAX_KEY_VAL + 1];

	void *data;
} link_node;

typedef struct linked_list {
	int node_num;
	link_node *start;
	link_node *last;
} linked_list;

/* ------------------------- node.c --------------------------- */
link_node       *link_node_assign(linked_list *list, size_t alloc_size);
link_node       *link_node_assign_left(linked_list *list, link_node *sibling, size_t alloc_size);
link_node       *link_node_assign_right(linked_list *list, link_node *sibling, size_t alloc_size);
int     link_node_length(linked_list *list);
link_node       *link_node_get_nth(linked_list *list, int nth);
void    *link_node_get_nth_data(linked_list *list, int nth);
void    link_node_delete(linked_list *list, link_node *node);
void    link_node_delete_all(linked_list *list, void (*cbfunc_with_node_data)());
link_node       *link_node_assign_key_order(linked_list *list, const char *key, size_t alloc_size);
link_node       *link_node_get_by_key(linked_list *list, const char *key);
void    *link_node_get_data_by_key(linked_list *list, const char *key);
