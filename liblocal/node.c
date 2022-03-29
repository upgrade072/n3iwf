#include "libnode.h"

link_node *link_node_assign(linked_list *list, size_t alloc_size)
{
	if (list == NULL) return NULL;

	link_node *new = NULL;

	if (list->last == NULL) {
		list->start = list->last = calloc(1, sizeof(link_node));
		new = list->start;
		new->next = NULL;
		new->prev = NULL;
		new->data = calloc(1, alloc_size);
	} else {
		link_node *last = list->last;
		new = calloc(1, sizeof(link_node));
		new->data = calloc(1, alloc_size);
		last->next = new;
		new->prev = last;
		new->next = NULL;
		list->last = new;
	}

	list->node_num++;

	return new;
}

link_node *link_node_assign_left(linked_list *list, link_node *sibling, size_t alloc_size)
{
	if (list == NULL || sibling == NULL) return NULL;

	link_node *new = calloc(1, sizeof(link_node));
	new->data = calloc(1, alloc_size);

	new->next = sibling;
	new->prev = sibling->prev;
	if (new->prev != NULL) {
		link_node *left = new->prev;
		left->next = new;
	}
	sibling->prev = new;

	if (list->start == sibling)
		list->start = new;

	list->node_num++;

	return new;
}

link_node *link_node_assign_right(linked_list *list, link_node *sibling, size_t alloc_size)
{
	if (list == NULL || sibling == NULL) return NULL;

	link_node *new = calloc(1, sizeof(link_node));
	new->data = calloc(1, alloc_size);

	new->next = sibling->next;
	if (new->next != NULL) {
		link_node *right = new->next;
		right->prev = new;
	}
	new->prev = sibling;
	sibling->next = new;

	if (list->last == sibling)
		list->last = new;

	list->node_num++;

	return new;
}

int link_node_length(linked_list *list)
{
	return list->node_num;
}

link_node *link_node_get_nth(linked_list *list, int nth)
{
	if (nth >= list->node_num) {
		return NULL;
	} else {
		link_node *curr = list->start;
		int pos = 0;
		for (; pos < nth && curr != NULL; pos++) {
			link_node *next = curr->next;
			curr = next;
		}
		if (pos == nth)
			return curr;
		else
			return NULL;
	}
}

void *link_node_get_nth_data(linked_list *list, int nth)
{
	if (nth >= list->node_num) {
		return NULL;
	} else {
		link_node *curr = list->start;
		int pos = 0;
		for (; pos < nth && curr != NULL; pos++) {
			link_node *next = curr->next;
			curr = next;
		}
		if (pos == nth)
			return curr->data;
		else {
			return NULL;
		}
	}
}

void link_node_delete(linked_list *list, link_node *node)
{
	if (list == NULL || node == NULL) return;

	link_node *prev = node->prev;
	link_node *next = node->next;

	if (prev != NULL)
		prev->next = next;
	if (next != NULL)
		next->prev = prev;

	if (list->start == node)
		list->start = next;

	if (list->last == node)
		list->last = prev;

	free(node->data);
	free(node);

	list->node_num--;

	return;
}

void link_node_delete_all(linked_list *list, void (*cbfunc_with_node_data)())
{
	link_node *node = list->last;
	while (node != NULL) {
		if (cbfunc_with_node_data != NULL) {
			cbfunc_with_node_data(node->data);
		}
		link_node_delete(list, node);
		node = list->last;
	}
}

link_node *link_node_assign_key_order(linked_list *list, const char *key, size_t alloc_size)
{
	if (strlen(key) > LN_MAX_KEY_VAL) {
		fprintf(stderr, "%s() key_len=(%ld) exceed max=(%d)\n", __func__, strlen(key), LN_MAX_KEY_VAL);
		return NULL;
	}

	if (link_node_length(list) == 0) {
		link_node *new = link_node_assign(list, alloc_size);
		strcpy(new->key, key);
		return new;
	}

	int low = 0;
	int high = (link_node_length(list) - 1);
	int nth = 0;
	link_node *node = NULL;
	int compare_res = 0;
	link_node *new = NULL;

	while (low <= high) {
		nth = (low + high) / 2;
		node = link_node_get_nth(list, nth);

		compare_res = strcmp(key, node->key);
		if (compare_res == 0) {
			return node; // we find return
		} else if (compare_res < 0) {
			high = nth - 1;
		} else {
			low = nth + 1;
		}
	}

	if (compare_res < 0) {
		new = link_node_assign_left(list, node, alloc_size);
	} else {
		new = link_node_assign_right(list, node, alloc_size);
	}
	strcpy(new->key, key);
	return new;
}

link_node *link_node_get_by_key(linked_list *list, const char *key)
{
	if (strlen(key) > LN_MAX_KEY_VAL) {
		return NULL;
	}

	if (link_node_length(list) == 0) {
		return NULL;
	}

	int low = 0;
	int high = (link_node_length(list) - 1);
	int nth = 0;
	link_node *node = NULL;
	int compare_res = 0;

	while (low <= high) {
		nth = (low + high) / 2;
		node = link_node_get_nth(list, nth);

		compare_res = strcmp(key, node->key);
		if (compare_res == 0) {
			return node; // we find return
		} else if (compare_res < 0) {
			high = nth - 1;
		} else {
			low = nth + 1;
		}
	}

	return NULL;
}

void *link_node_get_data_by_key(linked_list *list, const char *key)
{
	if (strlen(key) > LN_MAX_KEY_VAL) {
		return NULL;
	}

	if (link_node_length(list) == 0) {
		return NULL;
	}

	int low = 0;
	int high = (link_node_length(list) - 1);
	int nth = 0;
	link_node *node = NULL;
	int compare_res = 0;

	while (low <= high) {
		nth = (low + high) / 2;
		node = link_node_get_nth(list, nth);

		compare_res = strcmp(key, node->key);
		if (compare_res == 0) {
			return node->data; // we find return
		} else if (compare_res < 0) {
			high = nth - 1;
		} else {
			low = nth + 1;
		}
	}

	return NULL;
}
