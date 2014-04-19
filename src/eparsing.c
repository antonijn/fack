#include <parsing.h>
#include <codegen.h>

static struct cvariable * getvar(struct list * vars, char * id)
{
	int i;
	for (i = 0; i < vars->count; ++i) {
		void * var = vars->elements[i].element;
		struct cvariable * cvar = (struct cvariable *)var;
		if (!strcmp(cvar->id, id)) {
			return cvar;
		}
	}
	return NULL;
}

static struct asmexpression * genc_var(struct cvariable * v, int addroff)
{
	struct immediate * imm;
	if (v->ty == GLOBAL) {
		return (struct asmexpression *)new_ea8086(NULL, NULL, NULL, (struct asmexpression *)((struct cglobal *)v)->label, v->type->size, 0);
	}
	
	imm = new_imm(((struct clocal *)v)->stack_offset);
	return (struct asmexpression *)new_ea8086(&ss, &bp, NULL, (struct asmexpression *)imm, v->type->size, 1);
}

static struct asmexpression * eparser_wc(FILE * file, FILE * ofile, struct asmexpression * left, int opp, struct list * vars, int addrof)
{
	if (token.ty == SEMICOLON) {
		return left;
	}
	
	if (token.ty == IDENTIFIER) {
		struct cvariable * v = getvar(vars, token.str);
		if (!v) {
			v = getvar(&globals, token.str);
		}
		gettok(file);
		return eparser_wc(file, ofile, genc_var(v, addrof), 0, vars, 0);
	} else if (token.ty == OPERATOR) {
		
		if (!strcmp(token.str, "==")) {
			
			gettok(file);
			struct asmexpression * r = eparser_wc(file, ofile, NULL, 0, vars, 0);
			
			write_instr(ofile, "cmp", 2, left, r);
			
			left->cleanup(left);
			r->cleanup(r);
			
			return (struct asmexpression *)&f_e;
		}
	}
	return left;
}

struct asmexpression * eparser(FILE * file, FILE * ofile, struct list * vars)
{
	return eparser_wc(file, ofile, NULL, 0, vars, 0);
}
