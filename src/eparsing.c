#include <parsing.h>
#include <codegen.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <eparsingapi.h>

struct operator {
	int prec;
	int binary;
	int postfix_unary;
	int lefttoright;
};

static void * eparser_r(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars);

static const struct operator noop = { 0x100, 0, 0, 0 };

void cleane(struct expression * e)
{
	if (e->asme) {
		//e->asme->cleanup(e->asme);
		e->asme = NULL;
	}
	/*e.type->lcleanup(e.type);*/
}

static struct cvariable * getvar(char * name, struct list * vars)
{
	int i;
	for (i = 0; i < vars->count; ++i) {
		struct cvariable * cvar = (struct cvariable *)vars->elements[i].element;
		if (!strcmp(name, cvar->id)) {
			return cvar;
		}
	}
	return NULL;
}

static void * parse_id(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	struct asmexpression * so;
	struct expression res;
	struct clocal * loc = (struct clocal *)getvar(token.str, vars);
	
	if (!loc) {
		struct cglobal * g = (struct cglobal *)getvar(token.str, &globals);
		res.asme = (struct asmexpression *)
		    new_ea8086(NULL, NULL, NULL, (struct asmexpression *)g->label, g->type->size, 0);
		res.type = g->type;
		gettok(file);
		return eparser_r(file, ofile, pack(res), lastop, vars);
	}
	
	so = (struct asmexpression *)new_imm(loc->stack_offset);
	res.asme = (struct asmexpression *)new_ea8086(&ss, &bp, NULL, so, loc->type->size, 1);
	res.type = loc->type;
	gettok(file);
	return eparser_r(file, ofile, pack(res), lastop, vars);
}

static struct operator get_binop(char * str)
{
	struct operator res;
	if (!strcmp(str, ".") ||
	    !strcmp(str, "->")) {
		
		res.prec = 2;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "*") ||
	    !strcmp(str, "/") ||
		!strcmp(str, "%")) {
		
		res.prec = 5;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "+") ||
	    !strcmp(str, "-")) {
		
		res.prec = 6;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, ">>") ||
	    !strcmp(str, "<<")) {
		
		res.prec = 7;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, ">") ||
	    !strcmp(str, "<") ||
	    !strcmp(str, ">=") ||
	    !strcmp(str, "<=")) {
		
		res.prec = 8;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "==") ||
	    !strcmp(str, "!=")) {
		
		res.prec = 9;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "&")) {
		
		res.prec = 10;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "^")) {
		
		res.prec = 11;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "|")) {
		
		res.prec = 12;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "&&")) {
		
		res.prec = 13;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, "||")) {
		
		res.prec = 14;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	if (!strcmp(str, ",")) {
		
		res.prec = 18;
		res.binary = 1;
		res.lefttoright = 1;
		return res;
	}
	
	/* assignment */
	res.prec = 16;
	res.binary = 1;
	res.lefttoright = 0;
	return res;
}

static void * parse_bop(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	char opstr[token.len + 1];
	void * right;
	struct expression res;
	struct operator op = get_binop(token.str);
	
	if (op.prec >= lastop.prec) {
		/*gettok(file);*/
		return left;
	}
	
	memcpy(opstr, token.str, token.len + 1);
	
	gettok(file);
	right = eparser_r(file, ofile, NULL, op, vars);
	
	if (!strcmp(opstr, "==")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_e;
		res.type = &_int;
		
	} else if (!strcmp(opstr, "!=")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_ne;
		res.type = &_int;
		
	} else if (!strcmp(opstr, ">")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_g;
		res.type = &_int;
		
	} else if (!strcmp(opstr, ">=")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_ge;
		res.type = &_int;
		
	} else if (!strcmp(opstr, "<")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_l;
		res.type = &_int;
		
	} else if (!strcmp(opstr, "<=")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_le;
		res.type = &_int;
		
	} else if (!strcmp(opstr, "=")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacklvalue(left);
		r = unpacktorvalue(right);
		shield(r.asme);
		cloggflags();
		if (l.asme->ty == DEREFERENCE) {
			cloggmem(l.asme);
		}
		write_instr(ofile, "mov", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = l.asme;
		res.type = l.type;
		
	} else if (!strcmp(opstr, "+")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogpr(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "add", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = l.asme;
		res.type = l.type;
		
	} else if (!strcmp(opstr, "-")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogpr(left);
		shield(l.asme);
		r = unpacktorvalue(right);
		cloggflags();
		write_instr(ofile, "sub", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = l.asme;
		res.type = l.type;
		
	} else if (!strcmp(opstr, "*")) {
		
		struct expression l, r;
		shield_all();
		unshield(&ax);
		l = unpacktogpr(left);
		unshield_all();
		shield(&ax);
		r = unpacktogprea(right);
		clogg(&dx);
		write_instr(ofile, "mul", 1, r.asme);
		
		unshield_all();
		res.asme = &ax;
		res.type = l.type;
	} else if (!strcmp(opstr, "/")) {
		
		struct expression l, r;
		shield_all();
		unshield(&ax);
		l = unpacktogpr(left);
		unshield_all();
		shield(&ax);
		shield(&dx);
		r = unpacktogprea(right);
		clogg(&dx);
		write_instr(ofile, "xor", 2, &dx, &dx);
		write_instr(ofile, "div", 1, r.asme);
		
		unshield_all();
		res.asme = &ax;
		res.type = l.type;
	} else if (!strcmp(opstr, "%")) {
		
		struct expression l, r;
		shield_all();
		unshield(&ax);
		l = unpacktogpr(left);
		unshield_all();
		shield(&ax);
		shield(&dx);
		r = unpacktogprea(right);
		clogg(&dx);
		write_instr(ofile, "xor", 2, &dx, &dx);
		write_instr(ofile, "div", 1, r.asme);
		
		unshield_all();
		res.asme = &dx;
		res.type = l.type;
	}
	
	return eparser_r(file, ofile, pack(res), noop, vars);
}

static void * parse_intlit(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars,
	const char * format)
{
	struct expression res;
	int i;
	
	sscanf(token.str, format, &i);
	
	res.asme = (struct asmexpression *)new_imm(i);
	res.type = (struct ctype *)&_int;
	
	gettok(file);
	return eparser_r(file, ofile, pack(res), lastop, vars);
}

struct expr_internal {
	struct expression e;
	struct reg * r; /* used for storing memory values */
	unsigned int pushed : 1;
};

static void * eparser_r(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	switch (token.ty) {
	case SEMICOLON:
		break;
	case IDENTIFIER:
		left = parse_id(file, ofile, left, lastop, vars);
		break;
	case OPERATOR:
		if (left) {
			left = parse_bop(file, ofile, left, lastop, vars);
			break;
		}
		/* parse uop */
		break;
	case DEC_INT_LITERAL:
		left = parse_intlit(file, ofile, left, lastop, vars, "%d");
		break;
	case HEX_INT_LITERAL:
		left = parse_intlit(file, ofile, left, lastop, vars, "0x%x");
		break;
	case OCT_INT_LITERAL:
		left = parse_intlit(file, ofile, left, lastop, vars, "0%o");
		break;
	}
	
	return left;
}

void * eparser(FILE * file, FILE * ofile, struct list * vars)
{
	void * e;
	
	e = eparser_r(file, ofile, NULL, noop, vars);
	
	return e;
}
