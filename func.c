/*
 * Copyright (c) 2024, Andrei Rusanescu <andreirusanescu154gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "dllfunc.h"

/* Creates a doubly linked list of data_size elements */
dll_list_t *dll_create(unsigned int data_size)
{
	dll_list_t *ll = calloc(1, sizeof(dll_list_t));
	ll->head = NULL;
	ll->tail = NULL;
	ll->data_size = data_size;
	ll->size = 0;
	return ll;
}

/* Returns the nth node in the list*/
dll_node_t *dll_get_nth_node(dll_list_t *list, int n)
{
	if (!list || !list->head || n < 0)
		return NULL;
	n = n % list->size;
	dll_node_t *node = list->head;
	for (int i = 0; i < n; ++i)
		node = node->next;
	return node;
}

/* Adds nodes in order */
void add_in_order(dll_list_t *list, size_t addy, size_t bytes, int frag,
				  int ogsize)
{
	if (!list)
		return;
	dll_node_t *elem = calloc(1, sizeof(dll_node_t));
	elem->data = calloc(1, sizeof(info));
	info *node = elem->data;
	node->ogsize = ogsize;
	node->addy = addy;
	node->fragment = 0;
	node->data = calloc(1, bytes);
	node->size = bytes;
	node->fragment = frag;
	if (!list->size) {
		// list empty
		list->head = elem;
		list->tail = elem;
		list->head->next = NULL;
		list->tail->prev = NULL;
		list->head->prev = NULL;
		list->tail->next = NULL;
		list->size++;
	} else {
		dll_node_t *p = list->head;
		if (((info *)list->head->data)->addy > ((info *)elem->data)->addy) {
			elem->prev = NULL;
			elem->next = list->head;
			list->head->prev = elem;
			list->head = elem;
			list->size++;
			return;
		}

		while (p->next) {
			if ((((info *)p->data)->addy <= addy) &&
				(((info *)p->next->data)->addy >= addy)) {
				elem->next = p->next;
				elem->prev = p;
				p->next->prev = elem;
				p->next = elem;
				list->size++;
				return;
			}
			p = p->next;
		}
		elem->next = NULL;
		elem->prev = list->tail;
		list->tail->next = elem;
		list->tail = elem;
		list->size++;
	}
}

/* Removes and returns the nth node, free is done by the caller */
dll_node_t *dll_remove_nth_node(dll_list_t *list, int n)
{
	if (!list || n < 0 || (unsigned int)n > list->size || list->size == 0)
		return NULL;
	if (!n) {
		// remove head
		if (list->size == 1) {
			// head = tail
			dll_node_t *aux = list->head;
			list->head = NULL;
			list->tail = NULL;
			list->size--;
			return aux;
		}
		// list not empty
		dll_node_t *aux = list->head;
		list->head = list->head->next;
		list->head->prev = NULL;
		list->size--;
		return aux;
	} else if ((unsigned int)n == list->size - 1) {
		// remove tail;
		if (list->size == 1) {
			dll_node_t *aux = list->head;
			list->head = NULL;
			list->tail = NULL;
			list->size--;
			return aux;
		}
		dll_node_t *aux = list->tail;
		list->tail = list->tail->prev;
		list->tail->next = NULL;
		list->size--;
		return aux;
	}
	if ((unsigned int)n > (list->size >> 1)) {
		dll_node_t *p = list->tail;
		for (int i = list->size - 1; i > n + 1 && p; --i)
			p = p->prev;
		dll_node_t *aux = p->prev;
		p->prev = aux->prev;
		aux->prev->next = p;
		list->size--;
		return aux;
	}
	dll_node_t *p = list->head;
	for (int i = 0; i < n - 1 && p; ++i)
		p = p->next;
	dll_node_t *aux = p->next;
	p->next = aux->next;
	aux->next->prev = p;
	list->size--;
	return aux;
}

/* Frees memory */
void dll_free(dll_list_t **list)
{
	if (!list || !*list) {
		fprintf(stderr, "List is empty\n");
		return;
	}
	while ((*list)->size) {
		dll_node_t *elem = dll_remove_nth_node(*list, 0);
		free(((info *)elem->data)->data);
		free(elem->data);
		free(elem);
	}
	free(*list);
}
