#include <stddef.h>
#include "func.c"
#ifndef DLLFUNC
#define DLLFUNC

doubly_linked_list_t* dll_create(unsigned int data_size);
dll_node_t* dll_get_nth_node(doubly_linked_list_t* list, unsigned int n);
void dll_add_nth_node(doubly_linked_list_t* list, unsigned int n, size_t address);
dll_node_t* dll_remove_nth_node(doubly_linked_list_t* list, unsigned int n);
void dll_free(doubly_linked_list_t** pp_list);
void add_in_order(doubly_linked_list_t *list, size_t address, size_t bytes);

#endif