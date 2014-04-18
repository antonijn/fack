#ifndef PUTIL_H
#define PUTIL_H

#include <stdlib.h>

struct lelement {
	void * element;
	void (*cleanup)(void *);
};

struct list {
	struct lelement * elements;
	size_t count;
	size_t capacity;
};

struct list new_list(size_t c);
void add(struct list * l, void * e, void (*c)(void *));
int contains(struct list * l, void * e);
void freelist(struct list * l);

#endif
