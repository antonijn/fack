#include <fack/options.h>
#include <fack/codegen.h>
#include <fack/parsing.h>
#include <fack/eparsingapi.h>

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
	char cpudir[16] = { 0 };
	char bitsdir[16] = { 0 };
	
	functions = new_list(32);
	types = new_list(32);
	globals = new_list(32);
	ofile = outfile;
	
	sprintf(cpudir, "cpu %s", target.cpu.cpuid);
	sprintf(bitsdir, "bits %d", target.cpu.bytes * 8);
	
	write_directive(outfile, cpudir);
	write_directive(outfile, bitsdir);
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
	self->type->lcleanup(self->type);
	free(self);
}

struct cpointer * new_cpointer(struct ctype * type)
{
	struct cpointer * res = malloc(sizeof(struct cpointer));
	res->ty = POINTER;
	res->size = target.cpu.bytes;
	res->cleanup = &cleanup_cpointer;
	res->lcleanup = &cleanup_cpointer;
	res->type = type;
	return res;
}

static void cleanup_cfunctionptr(struct cfunctiontype * self)
{
	self->ret->lcleanup(self->ret);
	freelist(self->paramtypes);
	free(self->paramtypes);
	free(self);
}

struct cfunctiontype * new_cfunctiontype(struct ctype * ret, struct list * paramtypes, size_t paramdepth)
{
	struct cfunctiontype * res = malloc(sizeof(struct cfunctiontype));
	res->ty = FUNCTION_PTR;
	res->size = target.cpu.bytes;
	res->cleanup = &cleanup_cfunctionptr;
	res->lcleanup = &cleanup_cfunctionptr;
	res->ret = ret;
	res->paramtypes = paramtypes;
	res->paramdepth = paramdepth;
	return res;
}

static void cleanup_var(struct cvariable * self)
{
	if (self->ty == GLOBAL) {
		struct cglobal * g = (struct cglobal *)self;
		g->label->cleanup(g->label);
	}
	self->type->lcleanup(self->type);
	free(self);
}

struct cglobal * new_global(struct ctype * type, char * id)
{
	size_t idl = strlen(id);
	struct cglobal * res = malloc(sizeof(struct cglobal));
	res->ty = GLOBAL;
	res->cleanup = (void (*)(struct cglobal *))&cleanup_var;
	res->type = type;
	res->label = new_label(id);
	res->id = res->label->str;
	res->isarray = 0;
	return res;
}
struct clocal * new_local(struct ctype * type, char * id, int stack_offset)
{
	size_t idl = strlen(id);
	struct clocal * res = malloc(sizeof(struct cvariable));
	res->ty = LOCAL;
	res->cleanup = (void (*)(struct clocal *))&cleanup_var;
	res->type = type;
	res->id = malloc(idl + 1);
	memcpy(res->id, id, idl + 1);
	res->stack_offset = stack_offset;
	res->isarray = 0;
	return res;
}
struct cparam * new_param(struct ctype * type, char * id, int stack_offset)
{
	size_t idl = strlen(id);
	struct cparam * res = malloc(sizeof(struct cvariable));
	res->ty = PARAM;
	res->cleanup = (void (*)(struct cparam *))&cleanup_var;
	res->type = type;
	res->id = malloc(idl + 1);
	memcpy(res->id, id, idl + 1);
	res->stack_offset = stack_offset;
	return res;
}

static void cleanup_cfunction(struct cfunction * self)
{
	//free(self->id);
	free(self->label);
	self->ret->lcleanup(self->ret);
	self->ty->lcleanup(self->ty);
	freelist(&self->params);
	free(self);
}

struct cfunction * new_function(struct ctype * ret, char * id)
{
	struct cfunction * res = malloc(sizeof(struct cfunction));
	res->cleanup = &cleanup_cfunction;
	res->label = new_label(id);
	res->id = res->label->str;
	res->ret = ret;
	res->params = new_list(16);
	return res;
}

/* leaves token at identifier */
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
		gettok(file);
		return (struct ctype *)new_cpointer(prev);
	}
	
	return prev;
}

void fparse_func(FILE * file, struct ctype * ty, char * id)
{
	/* token is now ( */
	int stack_offset;
	struct cfunction * func;
	struct list * paramtys = malloc(sizeof(struct list));
	
	*paramtys = new_list(16);
	
	stack_offset = target.cpu.bytes;
	func = new_function(ty, id);
	
	while (token.str[0] != ')')
	{
		char * pid;
		struct cparam * param;
		struct ctype * pty;
		
		pty = f_readty(file, NULL);
		/* ) char, for example */
		if (!pty) {
			if (token.str[0] != ')') {
				showerror(stderr, "error", "unexpected token %s", token.str);
			}
			break;
		}
		
		pid = malloc(token.len + 1);
		pid[token.len] = '\0';
		memcpy(pid, token.str, token.len);
		
		/* go to , */
		gettok(file);
		if (token.str[0] != ',' && token.str[0] != ')') {
			showerror(stderr, "error", "expected ','");
		}
		
		stack_offset += pty->size > target.cpu.bytes ? pty->size : target.cpu.bytes;
		if (target.cpu.bytes > pty->size) {
			pty = &_int;
		}
		param = new_param(pty, pid, stack_offset);
		free(pid);
		add(&func->params, (void *)param, (void (*)(void *))param->cleanup);
		add(paramtys, (void *)pty, NULL);
	}
	
	func->paramdepth = stack_offset - 4;
	func->ty = new_cfunctiontype(ty, paramtys, stack_offset - 4);
	
	gettok(file);
	
	/* ; or { */
	add(&functions, (void *)func, (void (*)(void *))func->cleanup);
	if (token.str[0] != ';') {
		to_section(ofile, ".text");
		
		write_label(ofile, func->label);
		write_instr(ofile, "push", 1, target.cpu.stackp);
		write_instr(ofile, "mov", 2, target.cpu.basep, target.cpu.stackp);
		sparser_body(file, ofile, &func->params);
		write_instr(ofile, "pop", 1, target.cpu.basep);
		write_instr(ofile, "ret", 0);
	}
}

int fparse_afterty(FILE * file, struct ctype * ty)
{
	/* token is now id */
	char * id;
	
	id = malloc(token.len + 1);
	memcpy(id, token.str, token.len + 1);
	
	/* next token: ';', '=' => var, '(' => func*/
	gettok(file);
	
	if (token.str[0] == ';' || token.str[0] == '=' || token.str[0] == ',' ||
	    token.str[0] == '[') {
		
		/* TODO: var init vals */
		
		struct cglobal * g = new_global(ty, id);
		g->label->str = g->id;
		free(id);
		
		if (token.str[0] == '=') {
			void * right;
			struct list livars;
			struct expression e;
			
			to_section(ofile, ".data");
			write_label(ofile, g->label);
			
			livars = new_list(1);
			
			gettok(file);
			enter_exprenv(ofile);
			
			right = eparser(file, ofile, &livars);
			write_dx(ofile, ty->size, 1, unpack(right).asme);
			
			leave_exprenv();
		} else if (token.str[0] == '[') {
			void * count;
			struct list livars;
			struct expression e;
			
			//g->isarray = 1;
			ty = new_cpointer(ty);
			g->type = ty;
			
			livars = new_list(1);
			gettok(file);
			enter_exprenv(ofile);
			
			count = eparser(file, ofile, &livars);
			gettok(file);
			
			e = unpack(count);
			to_section(ofile, ".bss");
			write_label(ofile, g->label);
			write_resb(ofile, ty->size * *((struct immediate *)e.asme)->value);
			
			leave_exprenv();
			
			g->isarray = 1;
			
		} else {
			to_section(ofile, ".bss");
			write_label(ofile, g->label);
			write_resb(ofile, ty->size);
		}
		
		add(&globals, (void *)g, (void (*)(void *))g->cleanup);
		
		if (token.str[0] == ',') {
			/* advance to next id */
			gettok(file);
			fparse_afterty(file, ty);
			
		}
		return 0;
		
	} else if (token.str[0] != '(') {
		free(id);
		showerror(stderr, "error", "unexpected token %s", token.str);
		return 0;
	}
	
	/* function */
	fparse_func(file, ty, id);
	free(id);
	return 0;
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
