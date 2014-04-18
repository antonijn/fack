#include <options.h>
#include <codegen.h>
#include <parsing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list functions;
struct list types;
struct list globals;

static void pcleanup(struct cprimitive * self)
{
}

struct cprimitive _char = { PRIMITIVE, &pcleanup, &pcleanup, 1, "char" };
struct cprimitive _short = { PRIMITIVE, &pcleanup, &pcleanup, 2, "short" };
struct cprimitive _int = { PRIMITIVE, &pcleanup, &pcleanup, 2, "int" };
struct cprimitive _long = { PRIMITIVE, &pcleanup, &pcleanup, 4, "long" };
struct cprimitive _float = { PRIMITIVE, &pcleanup, &pcleanup, 4, "float" };
struct cprimitive _double = { PRIMITIVE, &pcleanup, &pcleanup, 8, "double" };
struct cprimitive _void = { PRIMITIVE, &pcleanup, &pcleanup, 0, "void" };

static FILE * ofile;

void fparser_init(FILE * outfile) {
	functions = new_list(32);
	types = new_list(32);
	globals = new_list(32);
	ofile = outfile;
}

void fparser_release() {
	freelist(&functions);
	freelist(&types);
	freelist(&globals);
	fclose(ofile);
}

static void cleanup_cstruct(struct cstruct * self)
{
	freelist(&self->fields);
	free(self->name);
	free(self);
}
static void lcleanup_cstruct(struct cstruct * self)
{
	if (self->name != NULL)
	{
		return;
	}
	self->cleanup(self);
}

struct cstruct * new_cstruct(size_t namelen, char * name)
{
	int i;
	struct cstruct * res = malloc(sizeof(struct cstruct));
	res->ty = STRUCT;
	res->cleanup = &cleanup_cstruct;
	res->lcleanup = &lcleanup_cstruct;
	res->namelen = namelen;
	res->name = malloc(namelen);
	for (i = 0; i < namelen; ++i) {
		res->name[i] = name[i];
	}
	res->fields = new_list(16);
	return res;
}

static void cleanup_cpointer(struct cpointer * self)
{
	self->type->cleanup(self->type);
	free(self);
}

struct cpointer * new_cpointer(struct ctype * type)
{
	struct cpointer * res = malloc(sizeof(struct cpointer));
	res->ty = POINTER;
	res->cleanup = &cleanup_cpointer;
	res->lcleanup = &cleanup_cpointer;
	res->type = type;
	return res;
}

static void cleanup_var(struct cvariable * self)
{
	free(self->id);
	self->type->lcleanup(self->type);
	free(self);
}

struct cvariable * new_global(struct ctype * type, char * id)
{
	size_t idl = strlen(id);
	struct cvariable * res = malloc(sizeof(struct cvariable));
	res->ty = GLOBAL;
	res->cleanup = &cleanup_var;
	res->type = type;
	res->id = malloc(idl + 1);
	memcpy(res->id, id, idl + 1);
	return res;
}
struct cvariable * new_local(struct ctype * type, char * id)
{
	size_t idl = strlen(id);
	struct cvariable * res = malloc(sizeof(struct cvariable));
	res->ty = LOCAL;
	res->cleanup = &cleanup_var;
	res->type = type;
	res->id = malloc(idl + 1);
	memcpy(res->id, id, idl + 1);
	return res;
}
struct cvariable * new_param(struct ctype * type, char * id)
{
	size_t idl = strlen(id);
	struct cvariable * res = malloc(sizeof(struct cvariable));
	res->ty = PARAM;
	res->cleanup = &cleanup_var;
	res->type = type;
	res->id = malloc(idl + 1);
	memcpy(res->id, id, idl + 1);
	return res;
}

struct ctype * f_readty(FILE * file, struct ctype * prev)
{
	gettok(file);
	
	if (token.ty == STOP)
	{
		return NULL;
	}
	
	if (token.ty == PRIMITIVE) {
		if (!strcmp(token.str, "char")) {
			return f_readty(file, (struct ctype *)&_char);
		} else if (!strcmp(token.str, "short")) {
			return f_readty(file, (struct ctype *)&_short);
		} else if (!strcmp(token.str, "int")) {
			return f_readty(file, (struct ctype *)&_int);
		} else if (!strcmp(token.str, "long")) {
			return f_readty(file, (struct ctype *)&_long);
		} else if (!strcmp(token.str, "float")) {
			return f_readty(file, (struct ctype *)&_float);
		} else if (!strcmp(token.str, "double")) {
			return f_readty(file, (struct ctype *)&_double);
		} else if (!strcmp(token.str, "void")) {
			return f_readty(file, (struct ctype *)&_void);
		}
		return NULL;
	}
	
	if (token.str[0] == '*') {
		return (struct ctype *)new_cpointer(prev);
	}
	
	return prev;
}

int fparse_afterty(FILE * file, struct ctype * ty)
{
	/* token is now id */
	char * id;
	
	id = malloc(token.len + 1);
	id[token.len] = '\0';
	memcpy(id, token.str, token.len);
	
	/* next token: ';', '=' => var, '(' => func*/
	gettok(file);
	
	if (token.str[0] == ';' || token.str[0] == '=' || token.str[0] == ',') {
		
		/* TODO: var init vals */
		
		write_label(ofile, id);
		write_resb(ofile, ty->size);
		
		struct cvariable * g = new_global(ty, id);
		free(id);
		add(&globals, (void *)g, (void (*)(void *))g->cleanup);
		
		if (token.str[0] == ',') {
			/* advance to next id */
			gettok(file);
			fparse_afterty(file, ty);
		}
		return 0;
		
	} else if (token.str[0] != ')') {
		free(id);
		fprintf(stderr, "error: unexpected token %s\n", token.str);
		return 0;
	}
}

int fparse_element(FILE * file)
{
	struct ctype * ty;
	
	/* read typename */
	ty = f_readty(file, NULL);
	if (!ty) {
		return 1;
	}
	
	/* typename, semicolon if no global */
	if (!strcmp(token.str, ";")) {
		return 0;
	}
	return fparse_afterty(file, ty);
}
