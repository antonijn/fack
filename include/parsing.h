#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>
#include <list.h>

enum tokenty {
	STOP,
	IDENTIFIER,
	MODIFIER,
	OPERATOR,
	CONTROL_STRUCTURE,
	TYPE_TYPE,
	PRIMITIVE,
	SEMICOLON,
	
	DEC_INT_LITERAL,
	BIN_INT_LITERAL,
	OCT_INT_LITERAL,
	HEX_INT_LITERAL,
	
	CHAR_LITERAL,
	STRING_LITERAL,
};

struct tok {
	enum tokenty ty;
	size_t len;
	char str[256];
};

enum ctype_type {
	STRUCT,
	UNION,
	ENUM,
	
	POINTER,
	PRIMITIVE_TYPE,
};

struct ctype {
	enum ctype_type ty;
	void (*cleanup)(struct ctype * self);
	void (*lcleanup)(struct ctype * self);
	size_t size;
};

struct cpointer {
	enum ctype_type ty;
	void (*cleanup)(struct cpointer * self);
	void (*lcleanup)(struct cpointer * self);
	size_t size;
	
	struct ctype * type;
};

struct cstruct {
	enum ctype_type ty;
	void (*cleanup)(struct cstruct * self);
	void (*lcleanup)(struct cstruct * self);
	size_t size;
	
	size_t namelen;
	char * name;
	struct list fields;
};

struct cprimitive {
	enum ctype_type ty;
	void (*cleanup)(struct cprimitive * self);
	void (*lcleanup)(struct cprimitive * self);
	size_t size;
	
	const char * name;
};

enum cvariable_type {
	GLOBAL,
	LOCAL,
	PARAM,
};

struct cvariable {
	enum cvariable_type ty;
	void (*cleanup)(struct cvariable * self);
	
	struct ctype * type;
	char * id;
};

struct cvariable * new_global(struct ctype * type, char * id);
struct cvariable * new_local(struct ctype * type, char * id);
struct cvariable * new_param(struct ctype * type, char * id);

extern struct cprimitive _char, _int, _long, _float, _double, _void;

struct cstruct * new_cstruct(size_t namelen, char * name);
struct cpointer * new_cpointer(struct ctype * type);

extern struct tok token;

void gettok(FILE * file);

void parse(const char * filename);
void fparser_init(FILE * ofile);
void fparser_release(void);

#endif
