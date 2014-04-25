#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <codegen.h>
#include <stdarg.h>
#include <string.h>

static void reg_tostring(FILE * f, struct reg * r);
static void ea_tostring(FILE * f, struct effective_address8086 * ea);
static void imm_tostring(FILE * f, struct immediate * imm);
static void flag_tostring(FILE * f, struct flags_flag * imm);

static const char * section = "";

static void rcu(void * self)
{
}

struct reg eax = { REGISTER, &reg_tostring, &rcu, NULL, "eax", 4, INTEL_80386 };
struct reg ebx = { REGISTER, &reg_tostring, &rcu, NULL, "ebx", 4, INTEL_80386 };
struct reg ecx = { REGISTER, &reg_tostring, &rcu, NULL, "ecx", 4, INTEL_80386 };
struct reg edx = { REGISTER, &reg_tostring, &rcu, NULL, "edx", 4, INTEL_80386 };
struct reg esi = { REGISTER, &reg_tostring, &rcu, NULL, "esi", 4, INTEL_80386 };
struct reg edi = { REGISTER, &reg_tostring, &rcu, NULL, "edi", 4, INTEL_80386 };
struct reg esp = { REGISTER, &reg_tostring, &rcu, NULL, "esp", 4, INTEL_80386 };
struct reg ebp = { REGISTER, &reg_tostring, &rcu, NULL, "ebp", 4, INTEL_80386 };

struct reg ax = { REGISTER, &reg_tostring, &rcu, &eax, "ax", 2, INTEL_8086 };
struct reg bx = { REGISTER, &reg_tostring, &rcu, &ebx, "bx", 2, INTEL_8086 };
struct reg cx = { REGISTER, &reg_tostring, &rcu, &ecx, "cx", 2, INTEL_8086 };
struct reg dx = { REGISTER, &reg_tostring, &rcu, &edx, "dx", 2, INTEL_8086 };
struct reg si = { REGISTER, &reg_tostring, &rcu, &esi, "si", 2, INTEL_8086 };
struct reg di = { REGISTER, &reg_tostring, &rcu, &edi, "di", 2, INTEL_8086 };
struct reg sp = { REGISTER, &reg_tostring, &rcu, &esp, "sp", 2, INTEL_8086 };
struct reg bp = { REGISTER, &reg_tostring, &rcu, &ebp, "bp", 2, INTEL_8086 };

struct reg ah = { REGISTER, &reg_tostring, &rcu, &ax, "ah", 1, INTEL_8086 };
struct reg bh = { REGISTER, &reg_tostring, &rcu, &bx, "bh", 1, INTEL_8086 };
struct reg ch = { REGISTER, &reg_tostring, &rcu, &cx, "ch", 1, INTEL_8086 };
struct reg dh = { REGISTER, &reg_tostring, &rcu, &dx, "dh", 1, INTEL_8086 };
struct reg al = { REGISTER, &reg_tostring, &rcu, &ax, "al", 1, INTEL_8086 };
struct reg bl = { REGISTER, &reg_tostring, &rcu, &bx, "bl", 1, INTEL_8086 };
struct reg cl = { REGISTER, &reg_tostring, &rcu, &cx, "cl", 1, INTEL_8086 };
struct reg dl = { REGISTER, &reg_tostring, &rcu, &dx, "dl", 1, INTEL_8086 };

struct reg cs = { REGISTER, &reg_tostring, &rcu, NULL, "cs", 2, INTEL_8086 };
struct reg ds = { REGISTER, &reg_tostring, &rcu, NULL, "ds", 2, INTEL_8086 };
struct reg ss = { REGISTER, &reg_tostring, &rcu, NULL, "ss", 2, INTEL_8086 };
struct reg es = { REGISTER, &reg_tostring, &rcu, NULL, "es", 2, INTEL_8086 };
struct reg fs = { REGISTER, &reg_tostring, &rcu, NULL, "fs", 2, INTEL_80386 };
struct reg gs = { REGISTER, &reg_tostring, &rcu, NULL, "gs", 2, INTEL_80386 };

struct flags_flag f_e  = { FLAG, &flag_tostring, &rcu, "e",  INTEL_8086 };
struct flags_flag f_ne = { FLAG, &flag_tostring, &rcu, "ne", INTEL_8086 };
struct flags_flag f_g  = { FLAG, &flag_tostring, &rcu, "g",  INTEL_8086 };
struct flags_flag f_ge = { FLAG, &flag_tostring, &rcu, "ge", INTEL_8086 };
struct flags_flag f_l  = { FLAG, &flag_tostring, &rcu, "l",  INTEL_8086 };
struct flags_flag f_le = { FLAG, &flag_tostring, &rcu, "le", INTEL_8086 };
struct flags_flag f_a  = { FLAG, &flag_tostring, &rcu, "a",  INTEL_8086 };
struct flags_flag f_ae = { FLAG, &flag_tostring, &rcu, "ae", INTEL_8086 };
struct flags_flag f_b  = { FLAG, &flag_tostring, &rcu, "b",  INTEL_8086 };
struct flags_flag f_be = { FLAG, &flag_tostring, &rcu, "be", INTEL_8086 };
struct flags_flag f_s  = { FLAG, &flag_tostring, &rcu, "s",  INTEL_8086 };
struct flags_flag f_ns = { FLAG, &flag_tostring, &rcu, "ns", INTEL_8086 };
struct flags_flag f_z  = { FLAG, &flag_tostring, &rcu, "z",  INTEL_8086 };
struct flags_flag f_nz = { FLAG, &flag_tostring, &rcu, "nz", INTEL_8086 };

static void imm_cleanup(struct immediate * self)
{
	if (self->str) {
		free(self->str);
	}
	if (self->value) {
		free(self->value);
	}
	free(self);
}

struct immediate * new_label(const char * str)
{
	size_t strl = strlen(str);
	struct immediate * res = malloc(sizeof(struct immediate));
	res->ty = IMMEDIATE;
	res->tostring = &imm_tostring;
	res->cleanup = &imm_cleanup;
	res->value = NULL;
	res->str = malloc(strl + 1);
	memcpy(res->str, str, strl + 1);
	return res;
}

struct immediate * new_imm(int value)
{
	struct immediate * res = malloc(sizeof(struct immediate));
	res->ty = IMMEDIATE;
	res->tostring = &imm_tostring;
	res->cleanup = &imm_cleanup;
	res->value = malloc(sizeof(int));
	*res->value = value;
	res->str = NULL;
	return res;
}

union ftoi {
	int i;
	float f;
};

struct immediate * new_immf(float value)
{
	union ftoi cast;
	cast.f = value;
	return new_imm(cast.i);
}

static void ea_cleanup_displ(struct effective_address8086 * self)
{
	self->displacement->cleanup(self->displacement);
	free(self);
}

static void ea_cleanup(struct effective_address8086 * self)
{
	free(self);
}

struct effective_address8086 * new_ea8086(struct reg * segment,
                                        struct reg * base,
										struct reg * index,
										struct asmexpression * displacement,
										size_t size, int cleandisp)
{
	struct effective_address8086 * ea = malloc(sizeof(struct effective_address8086));
	ea->ty = DEREFERENCE;
	ea->tostring = &ea_tostring;
	ea->cleanup = cleandisp ? ea_cleanup_displ : ea_cleanup;
	
	if (segment != NULL &&
		segment != &ss &&
		segment != &ds &&
		segment != &cs &&
		segment != &ds &&
		segment != &es &&
		segment != &fs &&
		segment != &gs) {
		showerror(stderr, "error", "%s is not a segment register", segment->name);
	}
	ea->tostring = &ea_tostring;
	ea->segment = segment;
	ea->base = base;
	ea->index = index;
	ea->displacement = displacement;
	ea->size = size;
	return ea;
}

static void ea_tostring(FILE * f, struct effective_address8086 * ea)
{
	switch (ea->size) {
		case 1:
			fprintf(f, "byte");
			break;
		case 2:
			fprintf(f, "word");
			break;
		case 4:
			fprintf(f, "dword");
			break;
		default:
			showerror(stderr, "error", "invalid size");
			break;
	}
	
	fprintf(f, " [");
	if (ea->segment) {
		fprintf(f, "%s:", ea->segment->name);
	}
	if (ea->base) {
		fprintf(f, "%s", ea->base->name);
	}
	if (ea->index) {
		if (ea->base) {
			fprintf(f, " + %s", ea->index->name);
		} else {
			fprintf(f, "%s", ea->index->name);
		}
	}
	if (ea->displacement) {
		struct asmexpression * d = (struct asmexpression *)ea->displacement;
		if (ea->base || ea->index) {
			fprintf(f, " + ");
			d->tostring(f, d);
		} else {
			d->tostring(f, d);
		}
	}
	fprintf(f, "]");
}

static void imm_tostring(FILE * f, struct immediate * imm)
{
	if (imm->value) {
		fprintf(f, "%d", *imm->value);
	}
	if (imm->str) {
		if (imm->value) {
			fprintf(f, "+");
		}
		fprintf(f, "%s", imm->str);
	}
}

static void reg_tostring(FILE * f, struct reg * r)
{
	if (r->arch == INTEL_80386 && target == INTEL_8086) {
		showerror(stderr, "error", "%s is not available in 8086", r->name);
	}
	fprintf(f, "%s", r->name);
}

static void flag_tostring(FILE * f, struct flags_flag * fl)
{
	fprintf(f, "%s", fl->name);
}

void write_instr(FILE * f, const char * instr, size_t ops, ...)
{
	va_list ap;
	int i;
	
	fprintf(f, "\t%s", instr);
	if (ops) {
		fprintf(f, " ");
	}
	
	va_start(ap, ops);
	for (i = 0; i < ops; ++i) {
		struct asmexpression * e = va_arg(ap, struct asmexpression *);
		if (!e) {
			showerror(stderr, "error", "assembly expression null");
			continue;
		}
		e->tostring(f, e);
		if (i != ops - 1) {
			fprintf(f, ", ");
		}
	}
	va_end(ap);
	fprintf(f, "\n");
	fflush(f);
}

void to_section(FILE * f, const char * sec)
{
	if (strcmp(section, sec)) {
		fprintf(f, "\nsection %s", (section = sec));
	}
}

void write_dx(FILE * f, int dsize, int ac, ...)
{
	va_list ap;
	int i;
	
	switch (dsize) {
	case 1:
		fprintf(f, "\tdd ");
		break;
	case 2:
		fprintf(f, "\tdw ");
		break;
	case 4:
		fprintf(f, "\tdd ");
		break;
	case 8:
		fprintf(f, "\tdq ");
		break;
	}
	
	va_start(ap, ac);
	for (i = 0; i < ac; ++i) {
		struct immediate * e = va_arg(ap, struct immediate *);
		e->tostring(f, e);
		if (i != ac - 1) {
			fprintf(f, ", ");
		}
	}
	va_end(ap);
	fprintf(f, "\n");
	fflush(f);
}

void write_comment(FILE * f, int ident, const char * str)
{
	fprintf(f, "\n");
	while (ident--) {
		fprintf(f, "\t");
	}
	fprintf(f, "; %s\n", str);
	fflush(f);
}

struct reg * toreg(FILE * f, struct asmexpression * x, struct list exclude)
{
	struct reg * r;
	
	if (x->ty != REGISTER) {
		r = &ax;
	}
	
	while (contains(&exclude, r)) {
		if (r == &ax) {
			r = &bx;
		} else if (r == &bx) {
			r = &cx;
		} else if (r == &cx) {
			r = &dx;
		} else if (r == &dx) {
			r = &ax;
		}
	}
	
	if (x != (struct asmexpression *)r) {
		write_instr(f, "mov", 2, r, x);
	}
	return r;
}

/* clogs ax */
void pushasme(FILE * f, struct asmexpression * x)
{
	if (x->ty == FLAG) {
		struct immediate * fa = get_tmp_label(), * skip = get_tmp_label();
		struct immediate * t = new_imm(1);
		
		cjmp_c(f, x, fa);
		write_instr(f, "xor", 2, &ax, &ax);
		write_instr(f, "jmp", 1, skip);
		write_label(f, fa);
		write_instr(f, "mov", 2, &ax, t);
		write_label(f, skip);
		write_instr(f, "push", 1, &ax);
		
		skip->cleanup(skip);
		t->cleanup(t);
		fa->cleanup(fa);
		
		return;
	} else if (x->ty == IMMEDIATE) {
		write_instr(f, "mov", 2, &ax, x);
		write_instr(f, "push", 1, &ax);
	} else {
		write_instr(f, "push", 1, x);
	}
}

void cjmp_c(FILE * f, struct asmexpression * x, struct immediate * jmp)
{
	if (x->ty == FLAG) {
		char instr[4] = { 0 };
		instr[0] = 'j';
		strcat(instr, ((struct flags_flag *)x)->name);
		write_instr(f, instr, 1, jmp);
		return;
	}
	
	write_instr(f, "test", 2, x, x);
	write_instr(f, "jnz", 1, jmp);
}

struct flags_flag * invflag(struct flags_flag * fl)
{
	if (fl == &f_e) {
		return &f_ne;
	} else if (fl == &f_ne) {
		return &f_e;
	} else if (fl == &f_g) {
		return &f_le;
	} else if (fl == &f_ge) {
		return &f_l;
	} else if (fl == &f_l) {
		return &f_ge;
	} else if (fl == &f_le) {
		return &f_g;
	} else if (fl == &f_a) {
		return &f_be;
	} else if (fl == &f_ae) {
		return &f_b;
	} else if (fl == &f_b) {
		return &f_ae;
	} else if (fl == &f_be) {
		return &f_a;
	} else if (fl == &f_ns) {
		return &f_s;
	} else if (fl == &f_s) {
		return &f_ns;
	} else if (fl == &f_z) {
		return &f_nz;
	} else if (fl == &f_nz) {
		return &f_z;
	}
}

void cjmp_nc(FILE * f, struct asmexpression * x, struct immediate * jmp)
{
	if (x->ty == FLAG) {
		char instr[4] = { 0 };
		instr[0] = 'j';
		strcat(instr, invflag((struct flags_flag *)x)->name);
		write_instr(f, instr, 1, jmp);
		return;
	}
	
	write_instr(f, "test", 2, x, x);
	write_instr(f, "jz", 1, jmp);
}

void write_label(FILE * f, struct immediate * lbl)
{
	fprintf(f, "\n");
	lbl->tostring(f, lbl);
	fprintf(f, ":\n");
}

void write_resb(FILE * f, size_t cnt)
{
	fprintf(f, "\tresb %d\n", cnt);
}

void write_directive(FILE * f, const char * dir)
{
	fprintf(f, "[%s]\n", dir);
}

static int stacktop = -1;
static int labelstack[32];

void asm_enter_block(void)
{
	++stacktop;
	labelstack[stacktop] = 0;
}

void asm_leave_block(void)
{
	--stacktop;
}

struct immediate * get_tmp_label(void)
{
	char str[32] = { 0 };
	sprintf(str, ".L%d.%d", stacktop, labelstack[stacktop]++);
	
	return new_label(str);
}
