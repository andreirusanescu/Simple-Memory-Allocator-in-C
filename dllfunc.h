/*
 * Copyright (c) 2024, Andrei Rusanescu <andreirusanescu154gmail.com>
 */

#include <stddef.h>

#ifndef DLLFUNC
#define DLLFUNC

#define MAX_LEN 200

typedef struct info_dump {
	int total_memory;
	int allocated_memory;
	int free_mem;
	int free_blocks;
	int allocated_blocks;
	int mallocs;
	int fragmentations;
	int frees;
} info_dump;

/* Holds metadata for each memory block, including address,
 * size, and fragmentation status
 */
typedef struct info {
	size_t addy;
	int fragment;
	int size;
	int ogsize;	// size of the original node
	int bytes;
	void *data;	// actual data of the node;
} info;

/* Represents a node in a doubly linked list */
typedef struct dll_node_t {
	void *data;
	struct dll_node_t *next;
	struct dll_node_t *prev;
} dll_node_t;

/* A doubly linked list used for managing free
 * and allocated blocks
 */
typedef struct {
	dll_node_t *head;
	dll_node_t *tail;
	unsigned int data_size;
	unsigned int size; // length of list
} dll_list_t;

/* Manages multiple free lists, each corresponding
 * to different block sizes
 */
typedef struct {
	int nlists;
	int nbytes; // bytes per list
	int type_rec;
	dll_list_t **list;
} sfl_t;

/* Tracks allocated blocks */
typedef struct {
	int nblocks;
	dll_list_t *list;
} mem_t;

dll_list_t *dll_create(unsigned int data_size);
dll_node_t *dll_get_nth_node(dll_list_t *list, int n);
dll_node_t *dll_remove_nth_node(dll_list_t *list, int n);
void dll_free(dll_list_t **pp_list);
void add_in_order(dll_list_t *list, size_t address, size_t bytes,
				  int frag, int ogsize);

#endif
