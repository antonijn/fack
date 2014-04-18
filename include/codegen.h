#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <options.h>

enum asmexpressiontype {
	IMMEDIATE,
	REGISTER,
	DEREFERENCE,
};

struct asmexpression {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct asmexpression * asme);
};

struct reg {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct reg * r);
	
	struct reg * parent;
	const char * name;
	size_t size;
	enum machine arch;
};

extern struct reg eax, ebx, ecx, edx, esi, edi, ebp, esp;
extern struct reg ax, bx, cx, dx, si, di, bp, sp;
extern struct reg al, ah, bl, bh, cl, ch, dl, dh;
extern struct reg cs, ds, ss, es, fs, gs;

struct immediate {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct immediate * i);
	int * value;
	const char * str;
};

struct effective_address8086 {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct effective_address8086 * ea);
	
	struct reg * segment;
	struct reg * base;
	struct reg * index;
	struct asmexpression * displacement;
	size_t size;
};

struct immediate new_label(const char * str);
struct immediate new_imm(int * value);
struct immediate new_immf(float * value);
struct effective_address8086 new_ea8086(struct reg * segment,
                                        struct reg * base,
										struct reg * index,
										struct asmexpression * displacement,
										size_t size);

void write_instr(FILE * f, const char * instr, size_t ops, ...);
void write_label(FILE * f, const char * lbl);
void write_resb(FILE * f, size_t cnt);

#endif
