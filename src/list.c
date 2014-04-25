#include <list.h>

struct list new_list(size_t c) {
	struct list r;
	r.elements = malloc(sizeof(void*) * c);
	r.count = 0;
	r.capacity = c;
	return r;
}

void add(struct list * l, void * e, void (*c)(void *))
{
	if (l->count == l->capacity) {
		l->elements = realloc(l->elements, sizeof(struct lelement)
		                      * (l->capacity = (size_t)(l->capacity * 1.6)));
	}
	l->elements[l->count].element = e;
	l->elements[l->count++].cleanup = c;
}

void removefromlist(struct list * l, void * e)
{
	int i;
	for (i = 0; i < l->count; ++i) {
		if (l->elements[i].element == e) {
			l->elements[i].cleanup(e);
			memmove(&l->elements[i], &l->elements[i + 1], (--l->count - i) * sizeof(struct lelement));
			return;
		}
	}
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
		if (!le.cleanup) {
			continue;
		}
		le.cleanup(le.element);
	}
}

void removelast(struct list * l, size_t cnt)
{
	int i;
	for (i = 0; i < cnt; ++i) {
		struct lelement le = l->elements[--l->count];
		if (!le.cleanup) {
			continue;
		}
		le.cleanup(le.element);
	}
}
