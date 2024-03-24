#include <stddef.h>
#include <stdio.h>

#define MAX_LEN 600

typedef struct dll_node_t
{
	void *data;
	size_t address;
	struct dll_node_t* next;
	struct dll_node_t* prev;
} dll_node_t;

typedef struct {
	size_t address; // adresa headului practic;
	dll_node_t* head;
	dll_node_t* tail;
	unsigned int data_size; // marimea tipului de date din lista;
	unsigned int size; // marimea listei;
} doubly_linked_list_t;

typedef struct {
	int nlists;
	int nbytes; // pe lista
	int type_rec;
	doubly_linked_list_t **list;
} sfl_t;

typedef struct {
	int nlists;
	doubly_linked_list_t **list;
} mem_t;

doubly_linked_list_t* dll_create(unsigned int data_size)
{
	doubly_linked_list_t *ll = malloc(sizeof(doubly_linked_list_t));
	if (!ll) {
		fprintf(stderr, "malloc() failed\n");
		return NULL;
	}
	ll->head = NULL;
	ll->tail = NULL;
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

// a fost scos parametrul void *data;
void dll_add_nth_node(doubly_linked_list_t *list, unsigned int n, size_t address)
{
	if (!list || n < 0)
        return;
    if (n > list->size)
        n = list->size;
	dll_node_t *elem = malloc(sizeof(dll_node_t));
	if (!elem) {
		fprintf(stderr, "malloc() failed\n");
		return;
	}
	elem->data = malloc(sizeof(list->data_size));
	elem->address = address;
	if (n == 0) {
		if (!list->size) {
			// lista e goala
			list->head = elem;
			list->tail = elem;
			list->head->next = NULL;
			list->tail->prev = NULL;
			list->head->prev = NULL;
			list->tail->next = NULL;
			list->size++;
		} else {
			// am elemente in lista;
			elem->next = list->head;
			elem->prev = NULL;
			list->head->prev = elem;
			list->head = elem;
			list->size++;
		}
	} else if (n == list->size) {
		// adaug la final;
		elem->next = NULL;
		elem->prev = list->tail;
		list->tail->next = elem;
		list->tail = elem;
		list->size++;
	} else {
		// adaug la mijloc
		if (n > list->size / 2) {
			// parcurg de la final;
			dll_node_t* p = list->tail;
			for (int i = list->size - 1; i >= n; --i)
				p = p->prev;
			elem->next = p;
			elem->prev = p->prev;
			p->prev->next = elem;
			p->prev = elem;
			list->size++;
		} else {
			// parcurg de la inceput;
			dll_node_t* p = list->head;
			for (int i = 0; i < n - 1; ++i)
				p = p->next;
			elem->prev = p;
			elem->next = p->next;
			p->next->prev = elem;
			p->next = elem;
			list->size++;
		}
	}
}

// sterge nodul de pe pozitia n si il intoarce
dll_node_t* dll_remove_nth_node(doubly_linked_list_t *list, unsigned int n)
{
	if (!list || n < 0 || n > list->size || list->size == 0)
		return NULL;
	if (!n) {
		// scot headul;
		if (list->size == 1) {
			// head = tail, practic sterg headul si tailul;
			dll_node_t *aux = list->head;
			list->head = NULL;
			list->tail = NULL;
			list->size--;
			return aux;
		} else {
			// lista nu e goala;
			dll_node_t* aux = list->head;
			list->head = list->head->next;
			list->head->prev = NULL;
			list->size--;
			return aux;
		}
	} else if (n == list->size - 1) {
		// scot tailul;
		if (list->size == 1) {
			dll_node_t *aux = list->head;
			list->head = NULL;
			list->tail = NULL;
			list->size--;
			return aux;
		} else {
			dll_node_t *aux = list->tail;
			list->tail = list->tail->prev;
			list->tail->next = NULL;
			list->size--;
			return aux;
		}
	} else {
		// scot de la mijloc;
		if (n > list->size / 2) {
			// parcurg de la final;
			dll_node_t* p = list->tail;
			for (int i = list->size - 1; i > n && p; --i)
				p = p->prev;
			dll_node_t *aux = p->prev;
			p->prev = aux->prev;
			aux->prev->next = p;
			list->size--;
			return aux;
		} else {
			// parcurg de la inceput;
			dll_node_t* p = list->head;
			for (int i = 0; i < n - 1 && p; ++i)
				p = p->next;
			dll_node_t* aux = p->next;
			p->next = aux->next;
			aux->next->prev = p;
			list->size--;
			return aux;
		}
	}
	return NULL;
}

void dll_free(doubly_linked_list_t **list)
{
	if (!list || !*list) {
		fprintf(stderr, "List is empty\n");
		return;
	}
	while ((*list)->size) {
		dll_node_t *elem = dll_remove_nth_node(*list, 0);
		free(elem);
	}
	free(*list);
}