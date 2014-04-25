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

static void * parse_func(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	int i;
	struct expression res;
	struct cfunction * f = NULL;
	
	for (i = 0; i < functions.count; ++i) {
		struct cfunction * cf = functions.elements[i].element;
		if (!strcmp(token.str, cf->id)) {
			f = cf;
			break;
		}
	}
	
	if (!f) {
		showerror(stderr, "error", "invalid identifier '%s'", token.str);
	}
	
	res.asme = f->label;
	res.type = f->ty;
	
	gettok(file);
	return eparser_r(file, ofile, pack(res), lastop, vars);
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
		if (!g) {
			return parse_func(file, ofile, left, lastop, vars);
		}
		
		res.asme = (struct asmexpression *)
		    new_ea8086(NULL, NULL, NULL, (struct asmexpression *)g->label, g->type->size, 0);
		if (g->isarray) {
			res.asme = new_label(g->label->str);
		}
		res.type = g->type;
		gettok(file);
		return eparser_r(file, ofile, pack(res), lastop, vars);
	}
	
	so = (struct asmexpression *)new_imm(loc->stack_offset);
	res.asme = (struct asmexpression *)new_ea8086(&ss, &bp, NULL, so, loc->type->size, 1);
	if (loc->isarray) {
		so = getgpr();
		write_instr(ofile, "lea", 2, so, res.asme);
		res.asme = so;
	}
	res.type = loc->type;
	gettok(file);
	return eparser_r(file, ofile, pack(res), lastop, vars);
}

static struct operator get_binop(char * str)
{
	struct operator res;
	if (!strcmp(str, ".") ||
	    !strcmp(str, "->") ||
	    !strcmp(str, "++") ||
	    !strcmp(str, "--")) {
		
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

static struct operator get_unop(char * str)
{
	struct operator res;
	if (!strcmp(str, "++") ||
	    !strcmp(str, "--") ||
	    !strcmp(str, "+") ||
	    !strcmp(str, "-") ||
	    !strcmp(str, "!") ||
	    !strcmp(str, "~") ||
	    !strcmp(str, "&") ||
	    !strcmp(str, "*") ||
	    !strcmp(str, "sizeof")) {
		
		res.prec = 3;
		res.binary = 0;
		res.lefttoright = 0;
		return res;
	}
	
	/* assignment */
	res.prec = 16;
	res.binary = 1;
	res.lefttoright = 0;
	return res;
}

static void * parse_uop(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	char opstr[token.len + 1];
	void * right;
	struct expression res;
	struct operator op = get_unop(token.str);
	
	if (op.prec >= lastop.prec) {
		return left;
	}
	
	memcpy(opstr, token.str, token.len + 1);
	
	gettok(file);
	right = eparser_r(file, ofile, NULL, op, vars);
	
	if (!strcmp(opstr, "++")) {
		
		struct expression r;
		unshield_all();
		r = unpacklvalue(right);
		write_instr(ofile, "inc", 1, r.asme);
		
		res = r;
		
	} else if (!strcmp(opstr, "--")) {
		
		struct expression r;
		unshield_all();
		r = unpacklvalue(right);
		write_instr(ofile, "dec", 1, r.asme);
		
		res = r;
		
	} else if (!strcmp(opstr, "-")) {
		
		struct expression r;
		unshield_all();
		r = unpacktogpr(right);
		write_instr(ofile, "neg", 1, r.asme);
		
		res = r;
		
	} else if (!strcmp(opstr, "!")) {
		
		struct expression r;
		unshield_all();
		
		if (packedinflags(right)) {
			
			r = unpacktoflags(right);
			res.asme = invflag(r.asme);
			res.type = r.type;
			
		} else {
			
			r = unpacktogpr(right);
			write_instr(ofile, "test", 2, r.asme, r.asme);
			res.asme = &f_z;
			res.type = r.type;
			
		}
		
	} else if (!strcmp(opstr, "~")) {
		
		struct expression r;
		struct immediate * imm = new_imm(-1);
		unshield_all();
		r = unpacktogpr(right);
		write_instr(ofile, "xor", 2, r.asme, imm);
		
		res = r;
		
		imm->cleanup(imm);
		
	} else if (!strcmp(opstr, "&")) {
		
		struct expression r;
		unshield_all();
		r = unpack(right);
		if (r.asme->ty == DEREFERENCE) {
			unshield_all();
			res.asme = getgpr();
			res.type = r.type;
			
			write_instr(ofile, "lea", 2, res.asme, r.asme);
		}
	} else if (!strcmp(opstr, "*")) {
		
		struct expression r;
		unshield_all();
		r = unpack(right);
		if (r.asme->ty == DEREFERENCE) {
			shield_all();
			unshield(&bx);
			unshield(&si);
			unshield(&di);
			res.asme = getgpr();
			res.type = ((struct cpointer *)r.type)->type;
			unshield_all();
			
			write_instr(ofile, "mov", 2, res.asme, r.asme);
			res.asme = new_ea8086(NULL, &bx, NULL, NULL, res.type->size, 0);
		}
	}
	
	return eparser_r(file, ofile, pack(res), op, vars);
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
	if (strcmp(opstr, "++") &&
		strcmp(opstr, "--")) {
		
		right = eparser_r(file, ofile, NULL, op, vars);
	}
	
	if (!strcmp(opstr, "++")) {
		
		struct expression r;
		unshield_all();
		r = unpacklvalue(left);
		res.asme = getgpr();
		write_instr(ofile, "mov", 2, res.asme, r.asme);
		write_instr(ofile, "inc", 1, r.asme);
		
		res.type = r.type;
		
	} else if (!strcmp(opstr, "--")) {
		
		struct expression r;
		unshield_all();
		r = unpacklvalue(left);
		res.asme = getgpr();
		write_instr(ofile, "mov", 2, res.asme, r.asme);
		write_instr(ofile, "dec", 1, r.asme);
		
		res.type = r.type;
		
	} else if (!strcmp(opstr, "==")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacktogprea(left);
		shield(l.asme);
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
		cloggflags();
		write_instr(ofile, "cmp", 2, l.asme, r.asme);
		
		unshield_all();
		res.asme = &f_le;
		res.type = &_int;
		
	} else if (!strcmp(opstr, "=")) {
		
		struct expression l, r;
		unshield_all();
		l = unpacklvalue(left);
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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
		r = unpacktorvalue(right, l);
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

static void * eparser_fcall(
	FILE * file, FILE * ofile,
	void * left,
	struct operator lastop,
	struct list * vars)
{
	int depth = 0, i = 0;
	char tokencpy[32] = { 0 };
	struct expression ret;
	struct cfunctiontype * fptr = unpackty(left);
	growstack(ofile, fptr->paramdepth);
	
	gettok(file);
	strcpy(tokencpy, token.str);
	while (strcmp(tokencpy, ")")) {
		void * param;
		struct expression parame;
		size_t bpoffs;
		struct immediate * offs;
		struct effective_address8086 * pea;
		struct ctype * paramty;
		
		paramty = fptr->paramtypes->elements[i++].element;
		depth += paramty->size;
		
		param = eparser(file, ofile, vars);
		strcpy(tokencpy, token.str);
		gettok(file);
		
		bpoffs = -depth;
		offs = new_imm(bpoffs);
		pea = new_ea8086(&ss, &bp, NULL, offs, paramty->size <= 2 ? 2 : paramty->size, 1);
		
		parame.asme = pea;
		parame.type = fptr->paramtypes->elements[i].element;
		parame = unpacktorvalue(param, parame);
		write_instr(ofile, "mov", 2, pea, parame.asme);
		
		pea->cleanup(pea);
	}
	
	cloggall();
	write_instr(ofile, "call", 1, unpack(left).asme);
	shrinkstack(ofile, fptr->paramdepth);
	
	/* TODO: proper handling of return stuffs */
	if (fptr->ret == &_void) {
		gettok(file);
		return eparser_r(file, ofile, NULL, lastop, vars);
	}
	
	ret.asme = &ax;
	ret.type = &_int;
	gettok(file);
	return eparser_r(file, ofile, pack(ret), lastop, vars);
}

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
		left = parse_uop(file, ofile, left, lastop, vars);
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
	case OPERATOID:
		if (!strcmp(token.str, "(")) {
			if (left) {
				left = eparser_fcall(file, ofile, left, lastop, vars);
				break;
			}
			gettok(file);
			left = eparser_r(file, ofile, NULL, noop, vars);
			gettok(file);
			left = eparser_r(file, ofile, left, lastop, vars);
		} else if (!strcmp(token.str, "[")) {
			void * idx;
			struct expression l, r, ea;
			
			gettok(file);
			idx = eparser_r(file, ofile, NULL, noop, vars);
			
			shield_all();
			unshield(&bx);
			unshield(&si);
			unshield(&di);
			l = unpacktogpr(left);
			unshield_all();
			shield(l.asme);
			r = unpacktorvalue(idx, l);
			write_instr(ofile, "add", 2, l.asme, r.asme);
			ea.type = ((struct cpointer *)l.type)->type;
			ea.asme = new_ea8086(NULL, l.asme, NULL, NULL, ea.type->size, 0);
			unshield_all();
			
			left = pack(ea);
			
			gettok(file);
			left = eparser_r(file, ofile, left, lastop, vars);
		}
	}
	
	return left;
}

void * eparser(FILE * file, FILE * ofile, struct list * vars)
{
	void * e;
	
	e = eparser_r(file, ofile, NULL, noop, vars);
	
	return e;
}
