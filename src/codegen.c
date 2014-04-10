#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <codegen.h>
#include <stdarg.h>

struct reg eax = { REGISTER, NULL, "eax", 4, INTEL_80386 };
struct reg ebx = { REGISTER, NULL, "ebx", 4, INTEL_80386 };
struct reg ecx = { REGISTER, NULL, "ecx", 4, INTEL_80386 };
struct reg edx = { REGISTER, NULL, "edx", 4, INTEL_80386 };
struct reg esi = { REGISTER, NULL, "esi", 4, INTEL_80386 };
struct reg edi = { REGISTER, NULL, "edi", 4, INTEL_80386 };
struct reg esp = { REGISTER, NULL, "esp", 4, INTEL_80386 };
struct reg ebp = { REGISTER, NULL, "ebp", 4, INTEL_80386 };

struct reg ax = { REGISTER, &eax, "ax", 2, INTEL_8086 };
struct reg bx = { REGISTER, &ebx, "bx", 2, INTEL_8086 };
struct reg cx = { REGISTER, &ecx, "cx", 2, INTEL_8086 };
struct reg dx = { REGISTER, &edx, "dx", 2, INTEL_8086 };
struct reg si = { REGISTER, &esi, "si", 2, INTEL_8086 };
struct reg di = { REGISTER, &edi, "di", 2, INTEL_8086 };
struct reg sp = { REGISTER, &esp, "sp", 2, INTEL_8086 };
struct reg bp = { REGISTER, &ebp, "bp", 2, INTEL_8086 };

struct reg ah = { REGISTER, &ax, "ah", 1, INTEL_8086 };
struct reg bh = { REGISTER, &bx, "bh", 1, INTEL_8086 };
struct reg ch = { REGISTER, &cx, "ch", 1, INTEL_8086 };
struct reg dh = { REGISTER, &dx, "dh", 1, INTEL_8086 };
struct reg al = { REGISTER, &ax, "al", 1, INTEL_8086 };
struct reg bl = { REGISTER, &bx, "bl", 1, INTEL_8086 };
struct reg cl = { REGISTER, &cx, "cl", 1, INTEL_8086 };
struct reg dl = { REGISTER, &dx, "dl", 1, INTEL_8086 };

struct reg cs = { REGISTER, NULL, "cs", 2, INTEL_8086 };
struct reg ds = { REGISTER, NULL, "ds", 2, INTEL_8086 };
struct reg ss = { REGISTER, NULL, "ss", 2, INTEL_8086 };
struct reg es = { REGISTER, NULL, "es", 2, INTEL_8086 };
struct reg fs = { REGISTER, NULL, "fs", 2, INTEL_80386 };
struct reg gs = { REGISTER, NULL, "gs", 2, INTEL_80386 };

struct immediate new_label(const char * str)
{
	struct immediate res;
	res.ty = IMMEDIATE;
	res.value = NULL;
	res.str = str;
	return res;
}

struct immediate new_imm(int * value)
{
	struct immediate imm;
	imm.ty = IMMEDIATE;
	imm.value = value;
	imm.str = NULL;
	return imm;
}

struct immediate new_immf(float * value)
{
	struct immediate imm;
	imm.ty = IMMEDIATE;
	imm.value = (int *)value;
	return imm;
}

struct effective_address8086 new_ea8086(struct reg * segment,
                                        struct reg * base,
										struct reg * index,
										struct asmexpression * displacement,
										size_t size)
{
	struct effective_address8086 ea;
	ea.ty = DEREFERENCE;
	
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
		if (ea->base || ea->index) {
			fprintf(f, " + ");
			asme_tostring(f, (struct asmexpression *)ea->displacement);
		} else {
			asme_tostring(f, (struct asmexpression *)ea->displacement);
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

void asme_tostring(FILE * f, struct asmexpression * asme)
{
	switch (asme->ty) {
		case DEREFERENCE:
			ea_tostring(f, (struct effective_address8086 *)asme);
			break;
		case IMMEDIATE:
			imm_tostring(f, (struct immediate *)asme);
			break;
		case REGISTER:
			reg_tostring(f, (struct reg *)asme);
			break;
	}
}

void write_instr(FILE * f, const char * instr, size_t ops, ...)
{
	va_list ap;
	int i;
	
	fprintf(f, "\t%s ", instr);
	
	va_start(ap, ops);
	for (i = 0; i < ops; ++i) {
		asme_tostring(f, va_arg(ap, struct asmexpression *));
		if (i != ops - 1) {
			fprintf(f, ", ");
		}
	}
	va_end(ap);
	fprintf(f, "\n");
}
