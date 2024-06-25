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

void printaddress(sfl_t *v)
{
	if (!v)
		return;
	for (int i = 0; i < v->nlists; ++i) {
		dll_node_t *node = v->list[i]->head;
		for (int j = 0; j < v->list[i]->size && node; ++j) {
			printf("%lx ", ((info_node*)node->data)->address);
			node = node->next;
		}
		printf("\n");
	}
}

void printaddress2(mem_t *v)
{
	if (!v)
		return;
	for (int i = 0; i < v->nlists; ++i) {
		dll_node_t *node = v->list[i]->head;
		for (int j = 0; j < v->list[i]->size && node; ++j) {
			printf("%lx ", ((info_node*)node->data)->address);
			node = node->next;
		}
		printf("\n");
	}
}

// merge bine functia asta
void initheap(sfl_t **v, mem_t **w, info_dump *id, size_t heapbase, int nlists, int bytes, int type_rec)
{
	*v = (sfl_t *)malloc(sizeof(sfl_t));
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
	*w = (mem_t*)malloc(sizeof(mem_t));
	if (!*w) {
		fprintf(stderr, "malloc() failed\n");
		free((*v)->list);
		free(*v);
		return;
	}
	(*w)->list = (doubly_linked_list_t**)malloc(nlists * sizeof(doubly_linked_list_t*));
	if (!(*w)->list) {
		fprintf(stderr, "malloc() failed\n");
		free((*v)->list);
		free(*v);
		free(*w);
		return;
	}
	(*w)->nlists = nlists;
	size_t address = heapbase;
	id->free_blocks = 0;
	for (int i = 0; i < nlists; ++i) {
		address = heapbase + i * bytes;
		(*v)->list[i] = dll_create(1 << (i + 3));
		// (*v)->list[i]->address = address;
		(*w)->list[i] = dll_create(0);
		if (!(*v)->list[i]) {
			fprintf(stderr, "malloc() failed\n");
			for (int j = i - 1; j >= 0; --j)
				free((*v)->list[i]);
			free((*v)->list);
			return;
		}
		size_t node_address = address;
		size_t node_bytes = bytes / (bytes / (1 << i + 3));
		for (int j = 0; j < bytes / (1 << (i + 3)); ++j) {
			node_address = address + j * node_bytes;
			add_in_order((*v)->list[i], node_address);
			id->free_blocks++;
		}
	}
	id->allocated_blocks = 0;
	id->total_memory = nlists * bytes;
	id->free_mem = id->total_memory;
	id->allocated_memory = 0;
	id->fragmentations = 0;
	id->mallocs = 0;
	id->frees = 0;
	
	(*v)->nbytes = bytes;
	(*v)->nlists = nlists;
	(*v)->type_rec = type_rec;
	// printaddress(*v);
}

void sort_lists(sfl_t *v)
{
	for (int j = v->nlists - 1; j >= 1; --j) {
		if (v->list[j - 1]->data_size > v->list[j]->data_size) {
			doubly_linked_list_t* aux = v->list[j - 1];
			v->list[j - 1] = v->list[j];
			v->list[j] = aux;
		}
	}
}

void parse_lists(sfl_t **v, size_t dim_free, size_t rest_address)
{
	for (int i = 0; i < (*v)->nlists; ++i) {
		if ((*v)->list[i]->data_size == dim_free) {
			add_in_order((*v)->list[i], rest_address);
			break;
		} else if ((*v)->list[i]->size > dim_free) {
			(*v)->list = (doubly_linked_list_t**)realloc((*v)->list, ((*v)->nlists + 1) * sizeof(doubly_linked_list_t*));
			if (!(*v)->list) {
				fprintf(stderr, "Realloc() failed\n");
				return;
			}
			(*v)->nlists++;
			(*v)->list[(*v)->nlists - 1] = dll_create(dim_free);
			// (*v)->list[(*v)->nlists - 1]->address = rest_address;
			(*v)->list[(*v)->nlists - 1]->data_size = dim_free;
			// adaug in ultima lista;
			add_in_order((*v)->list[(*v)->nlists - 1], rest_address);
			// sortez dupa data_size;
			sort_lists((*v));
			break;
		}
	}
}

// w are aceeasi structura cu segregated asta dar fac listele pe parcurs si dimensiunile o sa fie tot pe un rand, fix cat are primul nod din lista respectiva;
void my_malloc(mem_t **w, sfl_t **v, info_dump *id, int bytes)
{
	if (!*v || !bytes)
		return;
	int ok = 0;
	for (int i = 0; i < (*w)->nlists; ++i) {
		if ((*w)->list[i]->data_size == 0) {
			// aloc aici;
			ok = 1;
			break;
		}
	}
	if (!ok) {
		// n am gasit loc liber, deci realoc w cu memorie dubla;
		(*w)->list = realloc((*w)->list, 2 * (*w)->nlists * sizeof(doubly_linked_list_t));
		for (int i = (*w)->nlists; i < 2 * (*w)->nlists; ++i)
			(*w)->list[i] = dll_create(0);
		(*w)->nlists *= 2;
	}

	int index = -1;
	ok = -1;
	for (int i = 0; i < (*v)->nlists; ++i) {
		if ((*v)->list[i]->data_size > bytes) {
			if ((*v)->list[i]->head) {
				ok = 1;
				index = i;
				break;
			}
		} else if ((*v)->list[i]->data_size == bytes) {
			if ((*v)->list[i]->head) {
				ok = 0;
				index = i;
				break;
			}
		}
	}
	if (index == -1) {
		fprintf(stderr, "Out of memory\n");
		return;
	}
	// am gasit indexul;
	if (ok == 0) {
		// nu fragmentez;
		id->free_blocks--;
		dll_node_t *node = dll_remove_nth_node((*v)->list[index], 0);
		// verific sa n am liste goale;
		if (!(*v)->list[index]->size) {
			for (int i = index; i < (*v)->nlists - 1; ++i)
				(*v)->list[i] = (*v)->list[i + 1];
			(*v)->nlists--;
			(*v)->list = (doubly_linked_list_t**)realloc((*v)->list, ((*v)->nlists) * sizeof(doubly_linked_list_t*));
		}
		for (int i = 0; i < (*w)->nlists; ++i) {
			if ((*w)->list[i]->data_size == bytes) {
				add_in_order((*w)->list[i], ((info_node*)node->data)->address);
				// add_used_node(node, w, i);
				break;
			} else if ((*w)->list[i]->data_size == 0) {
				add_in_order((*w)->list[i], ((info_node*)node->data)->address);
				// add_used_node(node, w, i);
				(*w)->list[i]->data_size = bytes;
				break;
			}
		}
		free(node->data);
		free(node);
	} else if (ok == 1) {
		id->fragmentations++;
		size_t dim_free = (*v)->list[index]->data_size - bytes;
		// trebuie sa aloc un bloc de memorie de fix bytes, si unul de dim bytes si pe ala de dim bytes sa il bag in vectorul de liste unde trebuie;
		// pe ala de dim bytes il adaug in lista mea
		// trebuie sa cresc fragmentarea la noduri;
		dll_node_t *node = dll_remove_nth_node((*v)->list[index], 0);
		size_t rest_address = ((info_node*)node->data)->address + bytes;

		parse_lists(v, dim_free, rest_address);
		// adaugarea nodului in lista:
		for (int i = 0; i < (*w)->nlists; ++i) {
			if ((*w)->list[i]->data_size == bytes) {
				add_in_order((*w)->list[i], ((info_node*)node->data)->address);
				// add_used_node(node, w, i);
				break;
			} else if ((*w)->list[i]->data_size == 0) {
				add_in_order((*w)->list[i], ((info_node*)node->data)->address);
				// add_used_node(node, w, i);
				(*w)->list[i]->data_size = bytes;
				break;
			}
		}
		
		free(node->data);
		free(node);
	}
	id->mallocs++;
	id->allocated_memory += bytes;
	id->free_mem -= bytes;
	id->allocated_blocks++;
	// printf("Lista goala:\n");
	// printaddress(*v);
	// printf("Lista ocupata\n");
	// printaddress2(*w);
}

void my_free(mem_t **w, sfl_t **v, info_dump *id, size_t address)
{
	id->frees++;
	if (address == 0)
		return;
	int ok = 0;
	for (int i = 0; i < (*w)->nlists; ++i) {
		dll_node_t *node = (*w)->list[i]->head;
		for (int j = 0; j < (*w)->list[i]->size && node; ++j) {
			if (((info_node*)node->data)->address == address) {
				ok = 1;
				// tre sa l scot si sa creez pt v o lista de dimensiunea aia daca nu are deja;
				dll_node_t *aux = dll_remove_nth_node((*w)->list[i], j);
				if (!(*w)->list[i]->size) {
					for (int i = 0; i < (*w)->nlists - 1; ++i)
						(*w)->list[i] = (*w)->list[i + 1];
					(*w)->nlists--;
					(*w)->list = (doubly_linked_list_t**)realloc((*w)->list, (*w)->nlists * sizeof(doubly_linked_list_t *));
				}
				// fac o functie prin care sa parsez v dupa dimensiune si daca gasesc dimensiunea bine adaug in order acolo, daca nu realloc si sortez;
				parse_lists(v, (*w)->list[i]->data_size, ((info_node*)aux->data)->address);
				free(aux->data);
				free(aux);
				break;
			}
			node = node->next;
		}
	}
	// printaddress2(*w);
	if (!ok)
		printf("Invalid free\n");
}

void dump_memory(sfl_t *v, mem_t *w, info_dump *id)
{
// Allocated blocks : (0x<adresă_start_bloc_1> - <dimensiune_bloc_1>) (0x<adresă_start_bloc_2> - <dimensiune_bloc_2>) ... (0x<adresă_start_bloc_n> - <dimensiune_n>)
	printf("+++++DUMP+++++\n");
	printf("Total memory: %d bytes\n", id->total_memory);
	printf("Total allocated memory: %d bytes\n", id->allocated_memory);
	printf("Total free memory: %d bytes\n", id->free_mem);
	printf("Number of free blocks: %d\n", id->free_blocks);
	printf("Number of allocated blocks: %d\n", id->allocated_blocks);
	printf("Number of malloc calls: %d\n", id->mallocs);
	printf("Number of fragmentations: %d\n", id->fragmentations);
	printf("Number of free calls: %d\n", id->frees);
	for (int i = 0; i < v->nlists; ++i) {
		printf("Blocks with %d bytes - %d free block(s) : ", v->list[i]->data_size, v->list[i]->size);
		dll_node_t *node = v->list[i]->head;
		for (int j = 0; j < v->list[i]->size && node; ++j) {
			printf("0x%lx ", ((info_node*)node->data)->address);
			node = node->next;
		}
		printf("\n");
	}
	printf("Allocated blocks : ");
	for (int i = 0; i < w->nlists; ++i) {
		
		dll_node_t *node = w->list[i]->head;
		while (node) {
			printf("(0x%lx - %d) ", ((info_node *)node->data)->address, w->list[i]->data_size);
			node = node->next;
		}
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

void my_read(mem_t *w, int heapbase, int nbytes);

int main(void)
{
	char buffer[MAX_LEN];
	sfl_t *v = NULL;
	mem_t *w = NULL;
	int heapbase, nlists, nbytes, type_rec;
	int ok = 1;
	info_dump *id = calloc(1, sizeof(info_dump));
	while (ok) {
		scanf("%s", buffer);
		if (!strcmp(buffer, "INIT_HEAP")) {
			scanf("%x %d %d %d", &heapbase, &nlists, &nbytes, &type_rec);
			initheap(&v, &w, id, heapbase, nlists, nbytes, type_rec);
		} else if (!strcmp(buffer, "MALLOC")) {
			scanf("%d", &nbytes);
			my_malloc(&w, &v, id, nbytes);
		} else if (!strcmp(buffer, "FREE")) {
			scanf("%x", &heapbase);
			my_free(&w, &v, id, heapbase);
		} else if (!strcmp(buffer, "READ")) {
			scanf("%x %d", &heapbase, &nbytes);
			// my_read(w, heapbase, nbytes);
		} else if (!strcmp(buffer, "WRITE")) {
			
		} else if (!strcmp(buffer, "DUMP_MEMORY")) {
			dump_memory(v, w, id);
		} else if (!strcmp(buffer, "DESTROY_HEAP")) {
			// free

			ok = 0;
		}
	}

	return 0;
}
