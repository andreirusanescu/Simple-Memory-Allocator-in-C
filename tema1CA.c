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
	dll_node_t *node = v->list->head;
	for (int j = 0; j < v->nblocks && node; ++j) {
		printf("%lx ", ((info_node*)node->data)->address);
		node = node->next;
	}
	printf("\n");
}

// merge bine functia asta
void initheap(sfl_t **v, mem_t **w, info_dump *id, size_t heapbase, int nlists, int bytes, int type_rec)
{
	*v = (sfl_t *)calloc(1, sizeof(sfl_t));
	if (!*v) {
		fprintf(stderr, "calloc() failed\n");
		return;
	}
	(*v)->list = (doubly_linked_list_t **)calloc(nlists, sizeof(doubly_linked_list_t*));
	if (!(*v)->list) {
		fprintf(stderr, "calloc() failed\n");
		free(*v);
		return;
	}
	*w = (mem_t *)calloc(1, sizeof(mem_t));
	if (!*w) {
		fprintf(stderr, "calloc() failed\n");
		free((*v)->list);
		free(*v);
		return;
	}
	(*w)->list = (doubly_linked_list_t *)calloc(nlists, sizeof(doubly_linked_list_t));
	if (!(*w)->list) {
		fprintf(stderr, "calloc() failed\n");
		free((*v)->list);
		free(*v);
		free(*w);
		return;
	}
	(*w)->nblocks = 0;
	size_t address = heapbase;
	id->free_blocks = 0;
	for (int i = 0; i < nlists; ++i) {
		address = heapbase + i * bytes;
		(*v)->list[i] = dll_create(1 << (i + 3));
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
			add_in_order((*v)->list[i], node_address, node_bytes);
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
			add_in_order((*v)->list[i], rest_address, dim_free);
			break;
		} else if ((*v)->list[i]->data_size > dim_free) {
			(*v)->list = (doubly_linked_list_t **)realloc((*v)->list, ((*v)->nlists + 1) * sizeof(doubly_linked_list_t*));
			if (!(*v)->list) {
				fprintf(stderr, "Realloc() failed\n");
				return;
			}
			(*v)->nlists++;
			(*v)->list[(*v)->nlists - 1] = dll_create(dim_free);
			(*v)->list[(*v)->nlists - 1]->data_size = dim_free;
			// adaug in ultima lista;
			add_in_order((*v)->list[(*v)->nlists - 1], rest_address, dim_free);
			// sortez dupa data_size;
			sort_lists((*v));
			break;
		}
	}
}

// are memory leak uri functia asta !!!
void my_malloc(mem_t **w, sfl_t **v, info_dump *id, int bytes)
{
	if (!*v || !bytes)
		return;
	int ok = 0;
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
			// free((*v)->list[(*v)->nlists - 1]);
			(*v)->nlists--;
			(*v)->list = (doubly_linked_list_t**)realloc((*v)->list, ((*v)->nlists) * sizeof(doubly_linked_list_t*));
		}
		add_in_order((*w)->list, ((info_node*)node->data)->address, bytes);
		(*w)->nblocks++;
		free(((info_node *)node->data)->data);
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
		add_in_order((*w)->list, ((info_node*)node->data)->address, bytes);
		(*w)->nblocks++;
		free(((info_node *)node->data)->data);
		free(node->data);
		free(node);
	}
	id->mallocs++;
	id->allocated_memory += bytes;
	id->free_mem -= bytes;
	id->allocated_blocks++;
}

void my_free(mem_t **w, sfl_t **v, info_dump *id, size_t address)
{
	if (address == 0) {
		id->frees++;
		return;
	}
	int ok = 0;
	
	dll_node_t *node = (*w)->list->head;
	int i = 0;
	while (node) {
		if (((info_node *)node->data)->address == address) {
			dll_node_t *aux = dll_remove_nth_node((*w)->list, i);
			parse_lists(v, ((info_node *)aux->data)->size, ((info_node *)aux->data)->address);
			id->allocated_memory -= ((info_node *)aux->data)->size;
			id->free_mem += ((info_node *)aux->data)->size;
			free(((info_node*)aux->data)->data);
			free(aux->data);
			free(aux);
			ok = 1;
			id->allocated_blocks--;
			id->free_blocks++;
			break;
		}
		node = node->next;
		++i;
	}
	if (!ok)
		printf("Invalid free\n");
	else id->frees++;
}

void dump_memory(sfl_t *v, mem_t *w, info_dump *id)
{
// Allocated blocks : (0x<adresă_start_bloc_1> - <dimensiune_bloc_1>) (0x<adresă_start_bloc_2> - <dimensiune_bloc_2>) ... (0x<adresă_start_bloc_n> - <dimensiune_n>)
	printf("+++++DUMP+++++\n");
	printf("Total memory: %d bytes\n", id->total_memory);
	printf("Total allocated memory: %d bytes\n", id->allocated_memory);
	printf("Total free memory: %d bytes\n", id->free_mem);
	printf("Free blocks: %d\n", id->free_blocks);
	printf("Number of allocated blocks: %d\n", id->allocated_blocks);
	printf("Number of malloc calls: %d\n", id->mallocs);
	printf("Number of fragmentations: %d\n", id->fragmentations);
	printf("Number of free calls: %d\n", id->frees);
	for (int i = 0; i < v->nlists; ++i) {
		if (v->list[i]->size) {
			printf("Blocks with %d bytes - %d free block(s) : ", v->list[i]->data_size, v->list[i]->size);
			dll_node_t *node = v->list[i]->head;
			for (int j = 0; j < v->list[i]->size && node; ++j) {
				printf("0x%lx ", ((info_node*)node->data)->address);
				node = node->next;
			}
			printf("\n");
		}
	}
	printf("Allocated blocks : ");
	dll_node_t *node = w->list->head;
	while (node) {
		printf("(0x%lx - %d) ", ((info_node *)node->data)->address, ((info_node *)node->data)->size);
		node = node->next;
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

void my_read(mem_t *w, size_t start_address, size_t bytes, sfl_t *v, info_dump *id)
{
	dll_node_t *node = w->list->head;
	char *output = calloc(bytes + 1, sizeof(char));
	int ok = 0;
	int j = 0;
	while (node) {
		info_node *date = ((info_node *)node->data);
		if (date->address == start_address && date->size >= bytes) {
			for (int i = 0; i < bytes; ++i)
				// printf("%c", *((char *)(((info_node *)node->data)->data) + i));
				output[i] = *((char *)(((info_node *)node->data)->data) + i);
			// printf("\n");
			ok = 1;
		} else if (date->address == start_address) {
			// se intinde pe mai multe noduri;
			while (node->next &&  ((info_node *)node->next->data)->address == date->address + date->size) {
				int i = 0;
				for (; i < date->size; ++i) {
					output[j] = *((char *)(date->data) + i);
					++j;
				}
				bytes -= i;
				
				if (bytes == 0) {
					ok = 1; 
					break;
				}
				node = node->next;
				date = ((info_node *)node->data);
			}
			// inca mai are de citit bytes din ultimul nod
			if (!ok && bytes <= date->bytes && date->address == ((info_node *)node->prev->data)->address + ((info_node *)node->prev->data)->size) {
				int i = 0;
				// dupa \0 se opreste
				for (i = 0; i < bytes; ++i) {
					output[j] = *((char *)(date->data) + i);
					++j;
				}
				bytes -= i;
				if (bytes == 0) {
					ok = 1; 
				}
			}
			// In mijlocul nodului
		} else if (date->address < start_address && date->address + date->size - 1 >= start_address) {
			int i = start_address - date->address;
			while (node->next &&  ((info_node *)node->next->data)->address == date->address + date->size) {
				for (; i < date->size && bytes; ++i) {
					output[j] = *((char *)(date->data) + i);
					++j;
					bytes--;
				}
				if (bytes <= 0) {
					ok = 1; 
					break;
				}
				i = 0;
				node = node->next;
				date = ((info_node *)node->data);
			}
			// inca mai are de citit bytes din ultimul nod
			if (!ok && bytes <= date->bytes && date->address + date->size - 1 >= start_address) {
				// dupa \0 se opreste
				for (i; bytes && i + bytes < date->bytes; ++i) {
					output[j] = *((char *)(date->data) + i);
					++j;
					bytes--;
				}
				// printf("%d\n", bytes);
				if (bytes == 0)
					ok = 1; 
			}
		}
		if (ok) break;
		node = node->next;
	}
	if (ok) {
		printf("%s\n", output);
	} else {
		printf("Segmentation fault (core dumped)\n");
		dump_memory(v, w, id);
		// exit(-1);
	}
	free(output);
}

void my_write(mem_t *w, size_t start_address, char *string, int bytes, sfl_t *v, info_dump *id)
{
	dll_node_t *node = w->list->head;
	int ok = 0;
	int j = 0;
	if (strlen(string) < bytes)
		bytes = strlen(string);
	
	while (node) {
		// printf("%ld %d %ld\n",((info_node *)node->data)->address, ((info_node *)node->data)->size, start_address);
		if (((info_node *)node->data)->address == start_address && ((info_node *)node->data)->size >= bytes) {
			int i = 0;

			for (i = 0; i < bytes; ++i)
				*((char *)(((info_node *)node->data)->data) + i) = string[i];
			ok = 1;
			((info_node *)node->data)->bytes = bytes;

			// MAI MULTE NODURI
		} else if (((info_node *)node->data)->address == start_address) {
			dll_node_t *aux = node;
			int bts = bytes;
			while (node->next && ((info_node *)node->next->data)->address == ((info_node *)node->data)->address + ((info_node *)node->data)->size) {
				bytes -= ((info_node *)node->data)->size;
				if (bytes <= 0) {
					ok = 1;
					break;
				}
				node = node->next;
			}
			if (!ok && node->prev && ((info_node *)node->data)->address == ((info_node *)node->prev->data)->address + ((info_node *)node->prev->data)->size) {
				bytes -= ((info_node *)node->data)->size;
				// printf("%d\n", bytes);
				if (bytes <= 0)
					ok = 1;
			}
			if (ok) {
				node = aux;
				bytes = bts;
				int verif = 0;
				while (node->next && ((info_node *)node->next->data)->address == ((info_node *)node->data)->address + ((info_node *)node->data)->size) {
					int i = 0;
					for (; i < ((info_node *)node->data)->size && bytes; ++i) {
						*((char *)(((info_node *)node->data)->data ) + i) = string[j++];
						((info_node *)node->data)->bytes++;
						bytes--;
					}
					node = node->next;
					if (bytes <= 0) {
						verif = 1;
						break;
					}
				}
				if (!verif && node->prev && ((info_node *)node->data)->address == ((info_node *)node->prev->data)->address + ((info_node *)node->prev->data)->size) {
					int i = 0;
					for (i = 0; i < bytes; ++i) {
						*((char *)(((info_node *)node->data)->data) + i) = string[j++];
						((info_node *)node->data)->bytes++;
					}
					bytes -= i;
					if (bytes <= 0) verif = 1;
				}
			}
		} else if (((info_node *)node->data)->address > start_address) break;
		if (ok) break;
		node = node->next;
	}

	if (!ok) {
		printf("Segmentation fault (core dumped)\n");
		dump_memory(v, w, id);
		exit(-1);
	}
}

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
			my_read(w, heapbase, nbytes, v, id);
		} else if (!strcmp(buffer, "WRITE")) {
			char writer[100];
			scanf("%x", &heapbase);
			scanf("%*c%*c");
			int i = 0;
			while (1) {
				scanf("%c", &writer[i]);
				if (writer[i] == '"') {
					writer[i] = '\0';
					break;
				}
				i++;
			}
			scanf("%d", &nbytes);
			my_write(w, heapbase, writer, nbytes, v, id);
		} else if (!strcmp(buffer, "DUMP_MEMORY")) {
			dump_memory(v, w, id);
		} else if (!strcmp(buffer, "DESTROY_HEAP")) {
			// free
			dll_free(&w->list);
			free(w);
			for (int i = 0; i < v->nlists; ++i) {
				dll_free(&v->list[i]);
			}
			free(v->list);
			free(v);
			free(id);
			ok = 0;
		}
	}

	return 0;
}
