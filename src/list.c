#include <list.h>

struct list new_list(size_t c) {
	struct list r;
	r.elements = malloc(sizeof(void*) * c);
	r.count = 0;
	r.capacity = c;
	return r;
}

void add(struct list * l, void * e, void (*c)(void *)) {
	if (l->count == l->capacity) {
		l->elements = realloc(l->elements, sizeof(struct lelement)
		                      * (l->capacity = (size_t)(l->capacity * 1.6)));
	}
	l->elements[l->count].element = e;
	l->elements[l->count++].cleanup = c;
}

int contains(struct list * l, void * e) {
	int i;
	for (i = 0; i < l->count; ++i) {
		if (l->elements[i].element == e) {
			return 1;
		}
	}
	return 0;
}

void freelist(struct list * l) {
	int i;
	for (i = 0; i < l->count; ++i) {
		struct lelement le = l->elements[i];
		le.cleanup(le.element);
	}
}
