#include <parsing.h>
#include <codegen.h>
#include <string.h>
#include <assert.h>

struct operator {
	int prec;
	int binary;
	int postfix_unary;
	int lefttoright;
};

static const struct expression E_NULL = { NULL, NULL };

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

void cleane(struct expression e)
{
	if (e.asme) {
		e.asme->cleanup(e.asme);
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
		res.asme = new_ea8086(NULL, NULL, NULL, g->label, g->type->size, 0);
		res.type = g->type;
		gettok(file);
		return eparser_r(file, ofile, out, hint, res, lastop, vars);
	}
	
	so = (struct asmexpression *)new_imm(loc->stack_offset);
	res.asme = new_ea8086(&ss, &bp, NULL, so, loc->type->size, 1);
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
		one = new_imm(1);
		zero = new_imm(0);
		
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

static void save_left(FILE * ofile, struct expression e)
{
	if (e.asme->ty == REGISTER) {
		pushasme(ofile, e.asme);
	}
}

static struct asmexpression * restore_left(FILE * ofile, struct reg * r, struct expression e)
{
	if (e.asme->ty == REGISTER) {
		write_instr(ofile, "pop", 1, r);
		return (struct asmexpression *)r;
	}
	return e.asme;
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
	struct expression right, res;
	struct operator op = get_binop(token.str);
	
	if (op.prec > lastop.prec) {
		return left;
	}
	
	memcpy(opstr, token.str, token.len + 1);
	
	save_left(ofile, left);
	
	gettok(file);
	right = eparser_r(file, ofile, NULL, ASRVALUE, E_NULL, op, vars);
	
	if (!strcmp(opstr, "==")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, "!=")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, ">")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, ">=")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, "<")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, "<=")) {
		write_instr(ofile, "cmp", 2, restore_left(ofile, &ax, left), right.asme);
		
		cleane(left);
		cleane(right);
		
		res.type = (struct ctype *)&_int;
		res.asme = (struct asmexpression *)&f_e;
		return res;
	}
	if (!strcmp(opstr, "=")) {
		left = e_fin(ofile, right, restore_left(ofile, &ax, left), NONE);
		
		cleane(right);
		
		return left;
	}
	if (!strcmp(opstr, "+")) {
		left.asme = restore_left(ofile, &ax, left);
		left = e_fin(ofile, left, (struct asmexpression *)&ax, NONE);
		write_instr(ofile, "add", 2, &ax, right.asme);
		
		res.type = (left.type); /* TODO: select proper type */
		
		cleane(left);
		cleane(right);
		
		res.asme = (struct asmexpression *)&ax;
		return res;
	}
	if (!strcmp(opstr, "-")) {
		left.asme = restore_left(ofile, &ax, left);
		left = e_fin(ofile, left, (struct asmexpression *)&ax, NONE);
		write_instr(ofile, "sub", 2, &ax, right.asme);
		
		res.type = (left.type); /* TODO: select proper type */
		
		cleane(left);
		cleane(right);
		
		res.asme = (struct asmexpression *)&ax;
		return res;
	}
	
	return left;
}

static struct expression eparser_r(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct expression left,
	struct operator lastop,
	struct list * vars)
{
	if (token.ty == SEMICOLON) {
		return e_fin(ofile, left, out, hint);
	}
	
	if (token.ty == IDENTIFIER) {
		return e_fin(ofile, parse_id(file, ofile, out, hint, left, lastop, vars), out, hint);
	}
	if (token.ty == OPERATOR) {
		if (left.asme) {
			return e_fin(ofile, parse_bop(file, ofile, out, hint, left, lastop, vars), out, hint);
		}
		/* parse uop */
	}
	
	return left;
}

struct expression eparser(
	FILE * file, FILE * ofile,
	struct asmexpression * out,
	enum codegenhint hint,
	struct list * vars)
{
	struct operator noop;
	noop.prec = 0x100;
	
	return eparser_r(file, ofile, out, hint, E_NULL, noop, vars);
}
