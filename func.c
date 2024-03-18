#include <stddef.h>
#include <stdio.h>

#define MAX_LEN 600

typedef struct dll_node_t
{
    size_t address;
	void* data;
	struct dll_node_t* next;
    struct dll_node_t* prev;
} dll_node_t;

typedef struct {
    size_t address; // adresa headului practic;
    dll_node_t* head;
	unsigned int data_size; // marimea tipului de date din lista;
	unsigned int size; // marimea listei;
} doubly_linked_list_t;

typedef struct {
    int nlists;
    int nbytes; // pe lista
    int type_rec;
    doubly_linked_list_t **list;
} SegregatedFreeLists;

typedef struct {
    size_t address;
    unsigned int data_size;
	void* data;
	struct weirdnode* next;
    struct weirdnode* prev;
} weirdnode;

typedef struct {
    unsigned int size;
    weirdnode* head;
} OccupiedMemory;

doubly_linked_list_t* dll_create(unsigned int data_size)
{
    doubly_linked_list_t *ll = malloc(sizeof(doubly_linked_list_t));
    if (!ll) {
        fprintf(stderr, "malloc() failed\n");
        return NULL;
    }
    ll->head = NULL;
    ll->data_size = data_size;
    ll->size = 0;
    return ll;
}

dll_node_t* dll_get_nth_node(doubly_linked_list_t* list, unsigned int n)
{
    if (!list || !list->head || n < 0)
        return NULL;
    n = n % list->size;
    dll_node_t *node = list->head;
    for (int i = 0; i < n; ++i)
        node = node->next;
    return node;
}

void dll_add_nth_node(doubly_linked_list_t* list, unsigned int n, size_t address)
{
    if (!list || n < 0)
        return;
    if (n > list->size)
        n = list->size;
    if (n == 0) {
        dll_node_t *node = malloc(sizeof(dll_node_t));
        node->data = malloc(sizeof(list->data_size));
        node->address = address;
        node->prev = NULL;
        node->next = list->head;
        list->head = node;
        list->size++;
    } else {
        dll_node_t *add = malloc(sizeof(dll_node_t));
        add->data = malloc(sizeof(list->data_size));
        dll_node_t *node = list->head;
        for (int i = 0; i < n - 1; ++i)
            node = node->next;
        add->next = node->next;
        add->prev = node;
        node->next = add;
        add->address = address;
        list->size++;
    }
}

dll_node_t* dll_remove_nth_node(doubly_linked_list_t* list, unsigned int n)
{
    if (!list || n < 0)
        return NULL;
    if (n == 0) {
        dll_node_t *aux = list->head;
        list->head = list->head->next;
        if (list->head->next)
            list->head->next->prev = NULL;
        list->size--;
        return aux;
    } else {
        if (n > list->size - 1)
            n = list->size - 1;
        dll_node_t *node = list->head;
        for (int i = 0; i < n - 1; ++i)
            node = node->next;
        dll_node_t *aux = node->next;
        aux->prev = node;
        node->next = aux->next;
        list->size--;
        return aux;
    }
}

void dll_free(doubly_linked_list_t** pp_list)
{
    if (!pp_list || !*pp_list)
        return;
    for (int i = 0; i < (*pp_list)->size; ++i) {
        dll_node_t *aux = dll_remove_nth_node(*pp_list, 0);
        free(aux->data);
        free(aux);
    }
    free(*pp_list);
    *pp_list = NULL;
}