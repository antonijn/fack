#ifndef PUTIL_H
#define PUTIL_H

#include <stdlib.h>

struct list {
	void ** elements;
	size_t count;
	size_t capacity;
};

struct list new_list(size_t c) {
	struct list r;
	r.elements = malloc(sizeof(void*) * c);
	r.count = 0;
	r.capacity = c;
	return r;
}

void add(struct list * l, void * e) {
	if (l->count == l->capacity) {
		l->elements = realloc(l->elements, sizeof(e)
		                      * (l->capacity = (size_t)(l->capacity * 1.6)));
	}
	l->elements[l->count++] = e;
}

int contains(struct list * l, void * e) {
	int i;
	for (i = 0; i < l->count; ++i) {
		if (l->elements[i] == e) {
			return 1;
		}
	}
	return 0;
}

#endif
