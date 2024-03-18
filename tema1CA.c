#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*pt 8 am 128 de noduri; 8 * 128 = 1024 de bytes.. start: 0 ... final: 1016;
pt 16 am: 64 de noduri; .. start: 1024;
pt 32 am: 32 de noduri
pt 64 am: 16 noduri
pt 128 am 8 noduri
pt 256 am 4 noduri
pt 512 am 2 noduri
pt 1024 am 1 nod */

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

size_t hexachar(char *s)
{
    size_t n = 0;
    int len = strlen(s);
    for (int i = len - 1; i >= 2; --i) {
        size_t t = 1;
        for (int j = 0; j < len - (i + 1); j++)
            t *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            n += ((size_t)(s[i] - '0')) * t;
        } else if (s[i] >= 'A' && s[i] <= 'F') {
            n += ((size_t)(s[i] - 'A')) * t;
        }
    }
    return n;
}

// fac un numar dec in string in hexa;
void hexa(size_t n, char *s)
{
    for (int i = 0; n; i++, n /= 16) {
        if (n % 16 < 10) {
            s[i] = (char)(n % 16) + '0';
        } else {
            s[i] = (char)(n % 16 - 10) + 'A'; 
        }
    }
    strrev(s);
}

SegregatedFreeLists *initheap(char *heapbase, int nlists, int bytes, int type_rec)
{
    SegregatedFreeLists *v = (SegregatedFreeLists*)malloc(sizeof(SegregatedFreeLists));
    if (!v) {
        fprintf(stderr, "malloc() failed\n");
        return NULL;
    }
    v->list = (doubly_linked_list_t**)malloc(nlists * sizeof(doubly_linked_list_t*));
    if (!v->list) {
        fprintf(stderr, "malloc() failed\n");
        free(v);
        return NULL;
    }
    size_t start_address = hexachar(heapbase);
    size_t address = start_address;
    for (int i = 0; i < nlists; ++i) {
        address = start_address + i * bytes;
        v->list[i]->address = address;
        v->list[i] = ll_create(1 << (i + 3)); // tre sa fac altfel heapbaseul asta;
        if (!v->list[i]) {
            fprintf(stderr, "malloc() failed\n");
            for (int j = i - 1; j >= 0; --j) {
                free(v->list[i]);
            }
            free(v->list);
            return NULL;
        }
        // trebuie sa aloc si nodurile tot aici de dimensiunea aferenta;
        /*aloc noduri: */
        // aloc bytes / (1 << (i + 3)) noduri;

        size_t node_address = 0;
        for (int j = 0; j < bytes / (1 << (i + 3)); ++j) {
            // aloc gol;
            dll_add_nth_node(v->list[i], bytes / (1 << (i + 3)), node_address);
            // tre sa adaug nodurilor si o adresa;
        }
    }
    v->nbytes = bytes;
    v->nlists = nlists;
    v->type_rec = type_rec;
    return v;
}

void my_malloc(doubly_linked_list_t *w, SegregatedFreeLists *v, int bytes)
{
    if (!v)
        return;
    int index = -1, ok = -1;
    for (int i = 0; i < v->nlists; ++i) {
        if (v->list[i]->data_size > bytes) {
            if (v->list[i]->head) {
                ok = 1;
                index = i;
                break;
            }
        } else if (v->list[i]->data_size == bytes) {
            if (v->list[i]->head) {
                ok = 0;
                index = i;
                break;
            }
        }
    }
    if (index = -1) {
        fprintf(stderr, "Out of memory\n");
        return;
    }
    // am gasit indexul;
    if (ok == 0) {
        // nu fragmentez;
        dll_node_t *node = dll_remove_nth_node(v->list[index], 0);
        if (!w)
            w = dll_create(v->list[index]->data_size);
        dll_add_nth_node(w, 0, node->address);
        free(node);
    } else if (ok == 1) {
        // fragmentez;
        // fac dimensiunea noua
        size_t dim = v->list[index]->data_size - bytes;
        // trebuie sa aloc un bloc de memorie de fix bytes, si unul de dim bytes si pe ala de dim bytes sa il bag in vectorul de liste unde trebuie;
        // pe ala de dim bytes il adaug in lista mea
    }
}

int main(void)
{


}