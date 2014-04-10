#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>
#include <putil.h>

enum tokenty {
	STOP,
	IDENTIFIER,
	MODIFIER,
	LITERAL,
	OPERATOR,
	CONTROL_STRUCTURE,
	TYPE_TYPE,
	PRIMITIVE,
};

struct tok {
	enum tokenty ty;
	size_t len;
	char str[32];
};

enum type_type {
	STRUCT,
	UNION,
	ENUM
};
struct type_struct {
	enum type_type ty;
	
	size_t namelen;
	char name[32];
	struct list fields;
};

void gettok(FILE * file, struct tok * token);

void parse(const char * filename);
void fparser_init(void);

#endif
