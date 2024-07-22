#include <stddef.h>
#include "func.c"
#ifndef DLLFUNC
#define DLLFUNC

dll_list_t *dll_create(unsigned int data_size);
dll_node_t *dll_get_nth_node(dll_list_t *list, int n);
dll_node_t *dll_remove_nth_node(dll_list_t *list, int n);
void dll_free(dll_list_t **pp_list);
void add_in_order(dll_list_t *list, size_t address, size_t bytes,
				  int frag, int ogsize);

#endif
