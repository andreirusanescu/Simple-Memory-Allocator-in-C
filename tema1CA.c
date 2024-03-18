#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllfunc.h"

/*pt 8 am 128 de noduri; 8 * 128 = 1024 de bytes.. start: 0 ... final: 1016;
pt 16 am: 64 de noduri; .. start: 1024;
pt 32 am: 32 de noduri
pt 64 am: 16 noduri
pt 128 am 8 noduri
pt 256 am 4 noduri
pt 512 am 2 noduri
pt 1024 am 1 nod */

void printaddress(SegregatedFreeLists *v)
{
    for (int i = 0; i < v->nlists; ++i) {
        dll_node_t *node = v->list[i]->head;
        for (int j = 0; j < v->list[i]->size; ++j) {
            printf("%lx ", node->address);
            node = node->next;
        }
        printf("\n");
    }
}

// merge bine functia asta
void initheap(SegregatedFreeLists **v, size_t heapbase, int nlists, int bytes, int type_rec)
{
    *v = (SegregatedFreeLists*)malloc(sizeof(SegregatedFreeLists));
    if (!*v) {
        fprintf(stderr, "malloc() failed\n");
        return;
    }
    (*v)->list = (doubly_linked_list_t**)malloc(nlists * sizeof(doubly_linked_list_t*));
    if (!(*v)->list) {
        fprintf(stderr, "malloc() failed\n");
        free(*v);
        return;
    }
    size_t address = heapbase;
    for (int i = 0; i < nlists; ++i) {
        address = address + i * bytes;
        (*v)->list[i] = dll_create(1 << (i + 3)); // tre sa fac altfel heapbaseul asta;
        (*v)->list[i]->address = address;
        if (!(*v)->list[i]) {
            fprintf(stderr, "malloc() failed\n");
            for (int j = i - 1; j >= 0; --j) {
                free((*v)->list[i]);
            }
            free((*v)->list);
            return;
        }
        // aloc bytes / (1 << (i + 3)) noduri;
        size_t node_address = address;
        for (int j = 0; j < bytes / (1 << (i + 3)); ++j) {
            // aloc gol;
            node_address += j * bytes / (bytes / (1 << (i + 3)));
            dll_add_nth_node((*v)->list[i], bytes / (1 << (i + 3)), node_address);
        }
    }
    (*v)->nbytes = bytes;
    (*v)->nlists = nlists;
    (*v)->type_rec = type_rec;
    // printaddress(*v);
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
        printf("ceva\n");
    }
}

int main(void)
{
    char buffer[MAX_LEN];
    SegregatedFreeLists *v = NULL;
    OccupiedMemory *w = NULL;
    int heapbase, nlists, nbytes, type_rec;
    int ok = 1;
    while (ok) {
        scanf("%s", buffer);
        if (!strcmp(buffer, "INIT_HEAP")) {
            scanf("%x %d %d %d", &heapbase, &nlists, &nbytes, &type_rec);
            initheap(&v, heapbase, nlists, nbytes, type_rec);
        } else if (!strcmp(buffer, "MALLOC")) {
            scanf("%d", &nbytes);
            my_malloc(w, v, nbytes);
        } else if (!strcmp(buffer, "FREE")) {
            
        } else if (!strcmp(buffer, "READ")) {
            
        } else if (!strcmp(buffer, "WRITE")) {
            
        } else if (!strcmp(buffer, "DUMP_MEMORY")) {
            
        } else if (!strcmp(buffer, "DESTROY_HEAP")) {
            // free
            ok = 0;
        }
    }

    return 0;
}