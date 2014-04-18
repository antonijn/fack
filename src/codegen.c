#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <codegen.h>
#include <stdarg.h>

static void reg_tostring(FILE * f, struct reg * r);
static void ea_tostring(FILE * f, struct effective_address8086 * ea);
static void imm_tostring(FILE * f, struct immediate * imm);

struct reg eax = { REGISTER, &reg_tostring, NULL, "eax", 4, INTEL_80386 };
struct reg ebx = { REGISTER, &reg_tostring, NULL, "ebx", 4, INTEL_80386 };
struct reg ecx = { REGISTER, &reg_tostring, NULL, "ecx", 4, INTEL_80386 };
struct reg edx = { REGISTER, &reg_tostring, NULL, "edx", 4, INTEL_80386 };
struct reg esi = { REGISTER, &reg_tostring, NULL, "esi", 4, INTEL_80386 };
struct reg edi = { REGISTER, &reg_tostring, NULL, "edi", 4, INTEL_80386 };
struct reg esp = { REGISTER, &reg_tostring, NULL, "esp", 4, INTEL_80386 };
struct reg ebp = { REGISTER, &reg_tostring, NULL, "ebp", 4, INTEL_80386 };

struct reg ax = { REGISTER, &reg_tostring, &eax, "ax", 2, INTEL_8086 };
struct reg bx = { REGISTER, &reg_tostring, &ebx, "bx", 2, INTEL_8086 };
struct reg cx = { REGISTER, &reg_tostring, &ecx, "cx", 2, INTEL_8086 };
struct reg dx = { REGISTER, &reg_tostring, &edx, "dx", 2, INTEL_8086 };
struct reg si = { REGISTER, &reg_tostring, &esi, "si", 2, INTEL_8086 };
struct reg di = { REGISTER, &reg_tostring, &edi, "di", 2, INTEL_8086 };
struct reg sp = { REGISTER, &reg_tostring, &esp, "sp", 2, INTEL_8086 };
struct reg bp = { REGISTER, &reg_tostring, &ebp, "bp", 2, INTEL_8086 };

struct reg ah = { REGISTER, &reg_tostring, &ax, "ah", 1, INTEL_8086 };
struct reg bh = { REGISTER, &reg_tostring, &bx, "bh", 1, INTEL_8086 };
struct reg ch = { REGISTER, &reg_tostring, &cx, "ch", 1, INTEL_8086 };
struct reg dh = { REGISTER, &reg_tostring, &dx, "dh", 1, INTEL_8086 };
struct reg al = { REGISTER, &reg_tostring, &ax, "al", 1, INTEL_8086 };
struct reg bl = { REGISTER, &reg_tostring, &bx, "bl", 1, INTEL_8086 };
struct reg cl = { REGISTER, &reg_tostring, &cx, "cl", 1, INTEL_8086 };
struct reg dl = { REGISTER, &reg_tostring, &dx, "dl", 1, INTEL_8086 };

struct reg cs = { REGISTER, &reg_tostring, NULL, "cs", 2, INTEL_8086 };
struct reg ds = { REGISTER, &reg_tostring, NULL, "ds", 2, INTEL_8086 };
struct reg ss = { REGISTER, &reg_tostring, NULL, "ss", 2, INTEL_8086 };
struct reg es = { REGISTER, &reg_tostring, NULL, "es", 2, INTEL_8086 };
struct reg fs = { REGISTER, &reg_tostring, NULL, "fs", 2, INTEL_80386 };
struct reg gs = { REGISTER, &reg_tostring, NULL, "gs", 2, INTEL_80386 };

struct immediate new_label(const char * str)
{
	struct immediate res;
	res.ty = IMMEDIATE;
	res.tostring = &imm_tostring;
	res.value = NULL;
	res.str = str;
	return res;
}

struct immediate new_imm(int * value)
{
	struct immediate imm;
	imm.ty = IMMEDIATE;
	imm.tostring = &imm_tostring;
	imm.value = value;
	imm.str = NULL;
	return imm;
}

struct immediate new_immf(float * value)
{
	return new_imm((int *)value);
}

struct effective_address8086 new_ea8086(struct reg * segment,
                                        struct reg * base,
										struct reg * index,
										struct asmexpression * displacement,
										size_t size)
{
	struct effective_address8086 ea;
	ea.ty = DEREFERENCE;
	ea.tostring = &ea_tostring;
	
	if (segment) {
		if (segment != &ss &&
		    segment != &ds &&
		    segment != &cs &&
		    segment != &ds &&
		    segment != &es &&
		    segment != &fs &&
		    segment != &gs) {
			fprintf(stderr, "error: %s is not a segment register\n", segment->name);
		}
	}
	ea.tostring = &ea_tostring;
	ea.segment = segment;
	ea.base = base;
	ea.index = index;
	ea.displacement = displacement;
	ea.size = size;
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
			fprintf(stderr, "error: invalid size\n");
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
		fprintf(f, "%d", imm->value);
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
		fprintf(stderr, "error: %s is not available in 8086\n", r->name);
	}
	fprintf(f, r->name);
}

void write_instr(FILE * f, const char * instr, size_t ops, ...)
{
	va_list ap;
	int i;
	
	fprintf(f, "\t%s ", instr);
	
	va_start(ap, ops);
	for (i = 0; i < ops; ++i) {
		struct asmexpression * e = va_arg(ap, struct asmexpression *);
		e->tostring(f, e);
		if (i != ops - 1) {
			fprintf(f, ", ");
		}
	}
	va_end(ap);
	fprintf(f, "\n");
}

void write_label(FILE * f, const char * lbl)
{
	fprintf(f, "%s:\n", lbl);
}

void write_resb(FILE * f, size_t cnt)
{
	fprintf(f, "\tresb %d\n", cnt);
}
