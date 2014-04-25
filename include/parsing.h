#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>
#include <list.h>
#include <codegen.h>
#include <options.h>

enum tokenty {
	STOP,
	IDENTIFIER,
	MODIFIER,
	OPERATOR,
	OPERATOID,
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
	FUNCTION_PTR,
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

struct cfunctiontype {
	enum ctype_type ty;
	void (*cleanup)(struct cprimitive * self);
	void (*lcleanup)(struct cprimitive * self);
	size_t size;
	
	size_t paramdepth;
	struct ctype * ret;
	struct list * paramtypes;
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

struct cglobal {
	enum cvariable_type ty;
	void (*cleanup)(struct cglobal * self);
	
	struct ctype * type;
	char * id;
	struct immediate * label;
};

struct clocal {
	enum cvariable_type ty;
	void (*cleanup)(struct clocal * self);
	
	struct ctype * type;
	char * id;
	int stack_offset;
	int isarray;
};

struct cparam {
	enum cvariable_type ty;
	void (*cleanup)(struct cparam * self);
	
	struct ctype * type;
	char * id;
	int stack_offset;
};

struct cfunction {
	void (*cleanup)(struct cfunction * self);
	
	struct ctype * ret;
	char * id;
	size_t paramdepth;
	struct immediate * label;
	struct list params;
	struct cfunctiontype * ty;
};

struct expression {
	struct asmexpression * asme;
	struct ctype * type;
};

enum codegenhint {
	NONE,
	INREG,
	ASRVALUE,
	PUSHED,
	INFLAGS,
};

/*
 * Cleans up struct expression.
 * */
void cleane(struct expression * e);

struct cfunction * new_function(struct ctype * ret, char * id);

struct cglobal * new_global(struct ctype * type, char * id);
struct clocal * new_local(struct ctype * type, char * id, int stack_offset);
struct cparam * new_param(struct ctype * type, char * id, int stack_offset);

extern struct cprimitive _char, _int, _long, _float, _double, _void;

extern struct list functions, types, globals;

struct cstruct * new_cstruct(size_t namelen, char * name);
struct cpointer * new_cpointer(struct ctype * type);
struct cfunctiontype * new_cfunctiontype(struct ctype * ret, struct list * paramtypes, size_t paramdepth);

extern struct tok token;

void gettok(FILE * file);

void parse(const char * filename);
void fparser_init(FILE * ofile);
void fparser_release(void);

void sparser_body(FILE * file, FILE * ofile, struct list * vars);

void * eparser(FILE * file, FILE * ofile, struct list * vars);

void printreginfo(void);

size_t stackdepth(void);
void growstack(FILE * f, size_t x);
void shrinkstack(FILE * f, size_t x);

struct ctype * f_readty(FILE * file, struct ctype * prev);

#endif
