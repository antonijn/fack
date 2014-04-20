#include <parsing.h>
#include <codegen.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

struct operator {
	int prec;
	int binary;
	int postfix_unary;
	int lefttoright;
};

static const struct expression E_NULL = { NULL, NULL };
static const struct operator noop = { 0x100, 0, 0, 0 };

static struct expression eparser_r(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
	struct operator lastop,
	struct list * vars);
static struct expression parse_bop(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
	struct operator lastop,
	struct list * vars);
static struct expression e_fin(
	FILE * ofile,
	struct expression gen,
	struct asmexpression * dest,
	enum codegenhint hint);
static struct reg * getgpr(FILE * ofile, size_t av, ...);
static struct reg * vgetgpr(FILE * ofile, size_t av, va_list ap);

/* I know it's awful that it's global, but if we don't tell anyone,
 * it'll be okay, okay?
 */
static struct list vulnerable_gprs;

struct vulnerable_gpr {
	struct reg * gpr;
	int pushed;
};

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

static struct expression parse_id(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
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
		return eparser_r(file, ofile, out, hint, res, lastop, vars);
	}
	
	so = (struct asmexpression *)new_imm(loc->stack_offset);
	res.asme = (struct asmexpression *)new_ea8086(&ss, &bp, NULL, so, loc->type->size, 1);
	res.type = loc->type;
	gettok(file);
	return eparser_r(file, ofile, out, hint, res, lastop, vars);
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

static struct expression e_applyhint(
	FILE * ofile,
	struct expression gen,
	enum codegenhint hint)
{
	struct expression res;
	res.type = gen.type;
	
	switch (hint) {
	case NONE:
		return gen;
	case INREG:
		if (gen.asme->ty == REGISTER) {
			return gen;
		}
		return e_fin(ofile, gen, (struct asmexpression *)&ax, NONE);
	case ASRVALUE:
		if (gen.asme->ty != FLAG) {
			return gen;
		}
		return e_fin(ofile, gen, (struct asmexpression *)&ax, NONE);
	case PUSHED:
		pushasme(ofile, gen.asme);
		return res; /* res.asme isn't set */
	case INFLAGS:
		if (gen.asme->ty == FLAG) {
			return gen;
		}
		
		gen = e_applyhint(ofile, gen, INREG);
		write_instr(ofile, "test", 2, gen.asme, gen.asme);
		
		res.asme = (struct asmexpression *)&f_nz;
		return res;
	}
}

/*
 * Finalises expression, so that the result is in the appropriate place.
 * 
 * gen: the originally generated expression
 * dest: the destination as passed to eparser
 * hint: the hint as passed to eparser
 * */
static struct expression e_fin(
	FILE * ofile,
	struct expression gen,
	struct asmexpression * dest,
	enum codegenhint hint)
{
	struct expression res;
	if (!dest) {
		return e_applyhint(ofile, gen, hint);
	}
	
	res.asme = dest;
	res.type = gen.type;
	
	if (gen.asme == dest) {
		return gen;
	}
	
	if (gen.asme->ty == FLAG) {
		struct immediate * ontrue, * skip, * one, * zero;
		
		ontrue = get_tmp_label();
		skip = get_tmp_label();
		zero = new_imm(0);
		one = new_imm(1);
		
		cjmp_c(ofile, gen.asme, ontrue);
		/* not an xor to save possible memory accesses */
		write_instr(ofile, "mov", 2, dest, zero);
		write_instr(ofile, "jmp", 1, skip);
		write_label(ofile, ontrue);
		write_instr(ofile, "mov", 2, dest, one);
		write_label(ofile, skip);
		
		ontrue->cleanup(ontrue);
		skip->cleanup(skip);
		one->cleanup(one);
		zero->cleanup(zero);
		
		return res;
	}
	
	write_instr(ofile, "mov", 2, dest, gen.asme);
	return res;
}

static void save_left(FILE * ofile, struct expression e, struct vulnerable_gpr * gpr)
{
	if (e.asme->ty == REGISTER) {
		gpr->gpr = (struct reg *)e.asme;
		gpr->pushed = 0;
		add(&vulnerable_gprs, gpr, NULL);
	}
}

static struct asmexpression * restore_left(FILE * ofile, struct expression e, int inreg, size_t av, ...)
{
	struct reg * acc;
	struct vulnerable_gpr * last;
	va_list ap;
	int i;
	if (e.asme->ty != REGISTER) {
		if (!inreg) {
			return e.asme;
		}
		
		va_start(ap, av);
		acc = vgetgpr(ofile, av, ap);
		write_instr(ofile, "mov", 2, acc, e.asme);
		return (struct asmexpression *)acc;
	}
	
	last = vulnerable_gprs.elements[vulnerable_gprs.count - 1].element;
	removelast(&vulnerable_gprs, 1);
	
	if ((struct reg *)e.asme != last->gpr) {
		fprintf(stderr, "error: internal error\n");
	}
	
	if (!last->pushed) {
		return e.asme;
	}
	
	va_start(ap, av);
	acc = vgetgpr(ofile, av, ap);
	write_instr(ofile, "pop", 1, acc);
	return (struct asmexpression *)acc;
}

static int licontainsgpr(struct list * li, struct reg * r)
{
	int i;
	for (i = 0; i < li->count; ++i) {
		struct vulnerable_gpr * v = li->elements[i].element;
		if (v->gpr == r) {
			return 1;
		}
	}
	return 0;
}

static struct reg * vgetgpr(FILE * ofile, size_t av, va_list ap)
{
	int i;
	
	for (i = 0; i < av; ++i) {
		struct reg * r = va_arg(ap, struct reg *);
		if (!licontainsgpr(&vulnerable_gprs, r)) {
			va_end(ap);
			return r;
		}
	}
	
	va_end(ap);
	pushasme(ofile, (struct asmexpression *)&ax);
	for (i = 0; i < vulnerable_gprs.count; ++i) {
		if (vulnerable_gprs.elements[i].element == &ax) {
			struct vulnerable_gpr * vgpr = (struct vulnerable_gpr *)
			    vulnerable_gprs.elements[i].element;
			vgpr->pushed = 1;
			break;
		}
	}
	return &ax;
}

static struct reg * getgpr(FILE * ofile, size_t av, ...)
{
	va_list ap;
	va_start(ap, av);
	return vgetgpr(ofile, av, ap);
}

static struct expression parse_bop(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
	struct operator lastop,
	struct list * vars)
{
	char opstr[token.len + 1];
	struct expression right, res = left;
	struct vulnerable_gpr vgpr;
	struct operator op = get_binop(token.str);
	
	if (op.prec >= lastop.prec) {
		/*gettok(file);*/
		return left;
	}
	
	memcpy(opstr, token.str, token.len + 1);
	
	save_left(ofile, left, &vgpr);
	
	gettok(file);
	right = eparser_r(file, ofile, NULL, ASRVALUE, E_NULL, op, vars);
	
	if (!strcmp(opstr, "==")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, "!=")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, ">")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, ">=")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, "<")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, "<=")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "cmp", 2, l, right.asme);
		
		cleane(&left);
		cleane(&right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
	} else if (!strcmp(opstr, "=")) {
		struct asmexpression * l = restore_left(ofile, left, 0, 6, &ax, &bx, &cx, &dx, &si, &di);
		left = e_fin(ofile, right, l, NONE);
		
		cleane(&right);
		
		res = left;
	} else if (!strcmp(opstr, "+")) {
		struct asmexpression * acc = restore_left(ofile, left, 1, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "add", 2, acc, right.asme);
		
		res.type = (left.type); /* TODO: select proper type */
		
		cleane(&left);
		cleane(&right);
		
		res.asme = (struct asmexpression *)acc;
	} else if (!strcmp(opstr, "-")) {
		struct asmexpression * acc = restore_left(ofile, left, 1, 6, &ax, &bx, &cx, &dx, &si, &di);
		write_instr(ofile, "sub", 2, acc, right.asme);
		
		res.type = (left.type); /* TODO: select proper type */
		
		cleane(&left);
		cleane(&right);
		
		res.asme = (struct asmexpression *)acc;
	}
	
	return eparser_r(file, ofile, out, hint, res, noop, vars);
}

static struct expression parse_intlit(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
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
	return eparser_r(file, ofile, out, hint, res, lastop, vars);
}

static struct expression eparser_r(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
	struct operator lastop,
	struct list * vars)
{
	switch (token.ty) {
	case SEMICOLON:
		return e_fin(ofile, left, out, hint);
	case IDENTIFIER:
		return e_fin(ofile, parse_id(file, ofile, out, hint, left, lastop, vars), out, hint);
	case OPERATOR:
		if (left.asme) {
			return e_fin(ofile, parse_bop(file, ofile, out, hint, left, lastop, vars), out, hint);
		}
		/* parse uop */
		break;
	case DEC_INT_LITERAL:
		return e_fin(ofile, parse_intlit(file, ofile, out, hint, left, lastop, vars, "%d"), out, hint);
	case HEX_INT_LITERAL:
		return e_fin(ofile, parse_intlit(file, ofile, out, hint, left, lastop, vars, "0x%x"), out, hint);
	case OCT_INT_LITERAL:
		return e_fin(ofile, parse_intlit(file, ofile, out, hint, left, lastop, vars, "0%o"), out, hint);
	}
	return left;
}

struct expression eparser(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct list * vars)
{
	struct expression e;
	
	vulnerable_gprs = new_list(8);
	
	e = eparser_r(file, ofile, out, hint, E_NULL, noop, vars);
	
	freelist(&vulnerable_gprs);
	return e;
}
