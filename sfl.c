#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllfunc.h"

void initheap(sfl_t **v, mem_t **w, info_dump *id, size_t heapbase,
			  int nlists, int bytes, int type_rec)
{
	*v = (sfl_t *)calloc(1, sizeof(sfl_t));
	if (!*v) {
		fprintf(stderr, "calloc() failed\n");
		return;
	}
	(*v)->list = (dll_list_t **)calloc(nlists, sizeof(dll_list_t *));
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
	(*w)->list = (dll_list_t *)calloc(nlists, sizeof(dll_list_t));
	if (!(*w)->list) {
		fprintf(stderr, "calloc() failed\n");
		free((*v)->list);
		free(*v);
		free(*w);
		return;
	}
	(*w)->nblocks = 0;
	size_t addy = heapbase;
	id->free_blocks = 0;
	for (int i = 0; i < nlists; ++i) {
		addy = heapbase + i * bytes;
		(*v)->list[i] = dll_create(1 << (i + 3));
		if (!(*v)->list[i]) {
			fprintf(stderr, "malloc() failed\n");
			for (int j = i - 1; j >= 0; --j)
				free((*v)->list[i]);
			free((*v)->list);
			return;
		}
		size_t node_addy = addy;
		size_t node_bytes = bytes / (bytes / (1 << (i + 3)));
		for (int j = 0; j < bytes / (1 << (i + 3)); ++j) {
			node_addy = addy + j * node_bytes;
			add_in_order((*v)->list[i], node_addy, node_bytes, 0, node_bytes);
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
}

void sort_lists(sfl_t *v, int l, int r)
{
	if (l >= r)
		return;
	int mid = (l + r) / 2;
	int piv = l + rand() % (r - l + 1);
	dll_list_t *aux = v->list[piv];
	v->list[piv] = v->list[mid];
	v->list[mid] = aux;
	int i = l, j = r, k = 0;
	while (i < j) {
		if (v->list[i]->data_size > v->list[j]->data_size) {
			aux = v->list[i];
			v->list[i] = v->list[j];
			v->list[j] = aux;
			k = 1 - k;
		}
		i += k;
		j -= (1 - k);
	}
	sort_lists(v, l, i - 1);
	sort_lists(v, i + 1, r);
}

void parse_lists(sfl_t **v, size_t dim_free, size_t rest_addy, int frag,
				 int ogsize)
{
	for (int i = 0; i < (*v)->nlists; ++i) {
		if ((*v)->list[i]->data_size == dim_free) {
			add_in_order((*v)->list[i], rest_addy, dim_free, frag, ogsize);
			break;
		} else if ((*v)->list[i]->data_size > dim_free) {
			(*v)->list = (dll_list_t **)realloc((*v)->list,
						 ((*v)->nlists + 1) * sizeof(dll_list_t *));

			if (!(*v)->list) {
				fprintf(stderr, "Realloc() failed\n");
				return;
			}
			(*v)->nlists++;
			(*v)->list[(*v)->nlists - 1] = dll_create(dim_free);
			(*v)->list[(*v)->nlists - 1]->data_size = dim_free;
			// add in last list
			add_in_order((*v)->list[(*v)->nlists - 1], rest_addy,
						 dim_free, frag, ogsize);
			// sort by data_size
			sort_lists((*v), 0, (*v)->nlists - 1);
			break;
		}
	}
}

void my_malloc(mem_t **w, sfl_t **v, info_dump *id, unsigned int bytes)
{
	if (!*v || !bytes)
		return;
	int ok = 0;
	int index = -1;
	ok = -1;
	for (int i = 0; i < (*v)->nlists; ++i) {
		if ((*v)->list[i]->data_size > bytes) {
			if ((*v)->list[i]->size) {
				ok = 1;
				index = i;
				break;
			}
		} else if ((*v)->list[i]->data_size == bytes) {
			if ((*v)->list[i]->size) {
				ok = 0;
				index = i;
				break;
			}
		}
	}
	if (index == -1) {
		printf("Out of memory\n");
		return;
	}
	if (ok == 0) {
		// no fragmentation
		id->free_blocks--;
		dll_node_t *node = dll_remove_nth_node((*v)->list[index], 0);
		add_in_order((*w)->list, ((info *)node->data)->addy, bytes,
					 ((info *)node->data)->fragment,
					 ((info *)node->data)->ogsize);
		(*w)->nblocks++;
		free(((info *)node->data)->data);
		free(node->data);
		free(node);
	} else if (ok == 1) {
		id->fragmentations++;
		size_t dim_free = (*v)->list[index]->data_size - bytes;
		dll_node_t *node = dll_remove_nth_node((*v)->list[index], 0);
		size_t rest_addy = ((info *)node->data)->addy + bytes;
		parse_lists(v, dim_free, rest_addy, ((info *)node->data)->fragment + 1,
					((info *)node->data)->ogsize);
		add_in_order((*w)->list, ((info *)node->data)->addy, bytes,
					 ((info *)node->data)->fragment + 1,
					 ((info *)node->data)->ogsize);
		(*w)->nblocks++;
		free(((info *)node->data)->data);
		free(node->data);
		free(node);
	}
	id->mallocs++;
	id->allocated_memory += bytes;
	id->free_mem -= bytes;
	id->allocated_blocks++;
}

int get_max(int a, int b)
{
	return a > b ? a : b;
}

void parse(sfl_t **v, size_t addy, size_t size, info_dump *id, int frag,
		   int ogsize)
{
	int ok = 0;	// flag care imi spune cand sa opresc cautarea;
	for (int i = 0; i < (*v)->nlists; ++i) {
		// TODO: fac o regula sa nu parcurg toate listele
		dll_node_t *aux = (*v)->list[i]->head;
		for (int j = 0; aux; ++j, aux = aux->next) {
			if (addy + size == ((info *)aux->data)->addy) {
				if (!frag || !(((info *)aux->data)->fragment)) {
					continue;
				} else if (((info *)aux->data)->ogsize ==
						   ((info *)aux->data)->size) {
					((info *)aux->data)->fragment = 0;
					continue;
				}
				dll_node_t *node = dll_remove_nth_node((*v)->list[i], j);
				size = size + ((info *)node->data)->size;
				ok = 1;
				free(((info *)node->data)->data);
				free(node->data);
				free(node);
				break;
			} else if (((info *)aux->data)->addy +
					   ((size_t)((info *)aux->data)->size) == addy) {
				if (!frag || !(((info *)aux->data)->fragment)) {
					continue;
				} else if (((info *)aux->data)->ogsize ==
						   ((info *)aux->data)->size) {
					((info *)aux->data)->fragment = 0;
					continue;
				}
				dll_node_t *node = dll_remove_nth_node((*v)->list[i], j);
				size = size + ((info *)node->data)->size;
				ok = 1;
				addy = ((info *)aux->data)->addy;
				frag = get_max(((info *)aux->data)->fragment, frag);
				free(((info *)node->data)->data);
				free(node->data);
				free(node);
				break;
			}
		}
		if (ok)
			break;
	}
	if (!ok) {
		// nu s a gasit niciun nod => fac parse_lists clasic
		parse_lists(v, size, addy, frag, ogsize);
		return;
	}
	id->free_blocks--;
	parse(v, addy, size, id, frag - 1, ogsize);
}

void my_free1(mem_t **w, sfl_t **v, info_dump *id, size_t addy)
{
	if (addy == 0) {
		id->frees++;
		return;
	}
	int ok = 0;
	dll_node_t *node = (*w)->list->head;
	int i = 0;
	while (node) {
		if (((info *)node->data)->addy == addy) {
			dll_node_t *aux = dll_remove_nth_node((*w)->list, i);
			parse(v, ((info *)aux->data)->addy, ((info *)aux->data)->size, id,
				  ((info *)aux->data)->fragment, ((info *)aux->data)->ogsize);
			id->allocated_memory -= ((info *)aux->data)->size;
			id->free_mem += ((info *)aux->data)->size;
			free(((info *)aux->data)->data);
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
	else
		id->frees++;
}

void my_free(mem_t **w, sfl_t **v, info_dump *id, size_t addy)
{
	if (addy == 0) {
		id->frees++;
		return;
	}
	int ok = 0;
	dll_node_t *node = (*w)->list->head;
	int i = 0;
	while (node) {
		if (((info *)node->data)->addy == addy) {
			dll_node_t *aux = dll_remove_nth_node((*w)->list, i);
			parse_lists(v, ((info *)aux->data)->size,
						((info *)aux->data)->addy,
						((info *)aux->data)->fragment,
						((info *)aux->data)->ogsize);
			id->allocated_memory -= ((info *)aux->data)->size;
			id->free_mem += ((info *)aux->data)->size;
			free(((info *)aux->data)->data);
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
	else
		id->frees++;
}

void dump_memory(sfl_t *v, mem_t *w, info_dump *id)
{
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
			printf("Blocks with %d bytes - %d free block(s) : ",
				   v->list[i]->data_size, v->list[i]->size);
			dll_node_t *node = v->list[i]->head;
			for (int j = 0; j < (int)v->list[i]->size && node; ++j) {
				if (j)
					printf(" 0x%lx", ((info *)node->data)->addy);
				else
					printf("0x%lx", ((info *)node->data)->addy);
				node = node->next;
			}
			printf("\n");
		}
	}
	printf("Allocated blocks :");
	dll_node_t *node = w->list->head;
	while (node) {
		printf(" (0x%lx - %d)", ((info *)node->data)->addy,
			   ((info *)node->data)->size);
		node = node->next;
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

int my_read(mem_t *w, size_t start_addy, int bytes, sfl_t *v, info_dump *id)
{
	int ret = 0, ok = 0, j = 0;
	dll_node_t *node = w->list->head;
	char *output = calloc(bytes + 1, sizeof(char));
	while (node) {
		info *dt = ((info *)node->data);
		if (dt->addy == start_addy && dt->size >= bytes) {
			for (int i = 0; i < bytes; ++i)
				output[i] = *((char *)(((info *)node->data)->data) + i);
			ok = 1;
		} else if (dt->addy == start_addy) {
			// se intinde pe mai multe noduri;
			while (node->next &&  ((info *)node->next->data)->addy ==
				   dt->addy + dt->size) {
				int i = 0;
				for (; i < dt->size && bytes; ++i) {
					output[j] = *((char *)(dt->data) + i);
					++j;
					bytes--;
				}
				if (bytes <= 0) {
					ok = 1;
					break;
				}
				node = node->next;
				dt = ((info *)node->data);
			}
			// inca mai are de citit bytes din ultimul nod
			if (!ok && bytes <= dt->bytes && dt->addy ==
				((info *)node->prev->data)->addy +
				((info *)node->prev->data)->size) {
				int i = 0;
				for (; i < bytes; ++i) {
					output[j] = *((char *)(dt->data) + i);
					++j;
				}
				bytes -= i;
				if (bytes <= 0)
					ok = 1;
			}
			// In mijlocul nodului
		} else if (dt->addy < start_addy &&
				   dt->addy + dt->size - 1 >= start_addy) {
			int i = start_addy - dt->addy;
			while (node->next &&
				   ((info *)node->next->data)->addy == dt->addy + dt->size) {
				for (; i < dt->size && bytes; ++i, ++j, --bytes)
					output[j] = *((char *)(dt->data) + i);
				if (bytes <= 0) {
					ok = 1;
					break;
				}
				i = 0;
				node = node->next;
				dt = ((info *)node->data);
			}
			// inca mai are de citit bytes din ultimul nod
			if (!ok && bytes <= dt->bytes &&
				dt->addy + dt->size - 1 >= start_addy) {
				int cpy = bytes;
				for (; bytes && i + bytes < dt->bytes; ++i, ++j, --cpy)
					output[j] = *((char *)(dt->data) + i);
				bytes = cpy;
				if (bytes <= 0)
					ok = 1;
			}
		}
		if (ok)
			break;
		node = node->next;
	}
	if (ok) {
		printf("%s\n", output);
	} else {
		printf("Segmentation fault (core dumped)\n");
		dump_memory(v, w, id);
		ret = -1;
	}
	free(output);
	return ret;
}

int my_write(mem_t *w, size_t start_addy, char *string,
			 int bytes, sfl_t *v, info_dump *id)
{
	int ret = 0, ok = 0, j = 0, len = (int)strlen(string);
	dll_node_t *node = w->list->head;
	if (len < bytes)
		bytes = len;
	while (node) {
		if (((info *)node->data)->addy == start_addy &&
			((info *)node->data)->size >= bytes) {
			int i = 0;
			for (; i < bytes; ++i)
				*((char *)(((info *)node->data)->data) + i) = string[i];
			ok = 1;
			((info *)node->data)->bytes = bytes;
			// SE INTINDE PE MAI MULTE NODURI
		} else if (((info *)node->data)->addy == start_addy) {
			dll_node_t *aux = node;
			int bts = bytes;
			while (node->next && ((info *)node->next->data)->addy ==
				   ((info *)node->data)->addy + ((info *)node->data)->size) {
				bytes -= ((info *)node->data)->size;
				if (bytes <= 0) {
					ok = 1;
					break;
				}
				node = node->next;
			}
			if (!ok && node->prev && ((info *)node->data)->addy ==
				((info *)node->prev->data)->addy +
				((info *)node->prev->data)->size) {
				bytes -= ((info *)node->data)->size;
				if (bytes <= 0)
					ok = 1;
			}
			if (ok) {
				node = aux;
				bytes = bts;
				int verif = 0;
				while (node->next && ((info *)node->next->data)->addy ==
					   ((info *)node->data)->addy +
					   ((info *)node->data)->size) {
					int i = 0;
					for (; i < ((info *)node->data)->size && bytes; ++i, ++j) {
						*((char *)(((info *)node->data)->data) + i) = string[j];
						((info *)node->data)->bytes++;
						--bytes;
					}
					node = node->next;
					if (bytes <= 0) {
						verif = 1;
						break;
					}
				}
				if (!verif && node->prev && ((info *)node->data)->addy ==
					((info *)node->prev->data)->addy +
					((info *)node->prev->data)->size) {
					int i = 0;
					for (i = 0; i < bytes; ++i, ++j) {
						*((char *)(((info *)node->data)->data) + i) = string[j];
						((info *)node->data)->bytes++;
					}
					bytes -= i;
					if (bytes <= 0)
						verif = 1;
				}
			}
		} else if (((info *)node->data)->addy > start_addy) {
			break;
		}
		if (ok)
			break;
		node = node->next;
	}
	if (!ok) {
		printf("Segmentation fault (core dumped)\n");
		dump_memory(v, w, id);
		ret = -1;
	}
	return ret;
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
			if (!v->type_rec)
				my_free(&w, &v, id, heapbase);
			else
				my_free1(&w, &v, id, heapbase);
		} else if (!strcmp(buffer, "READ")) {
			scanf("%x %d", &heapbase, &nbytes);
			if (my_read(w, heapbase, nbytes, v, id) == -1)
				goto free_all;
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
			if (my_write(w, heapbase, writer, nbytes, v, id) == -1)
				goto free_all;
		} else if (!strcmp(buffer, "DUMP_MEMORY")) {
			dump_memory(v, w, id);
		} else if (!strcmp(buffer, "DESTROY_HEAP")) {
free_all:
			dll_free(&w->list);
			free(w);
			for (int i = 0; i < v->nlists; ++i)
				dll_free(&v->list[i]);
			free(v->list);
			free(v);
			free(id);
			ok = 0;
		}
	}

	return 0;
}
