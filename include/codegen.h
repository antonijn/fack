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
	void (*cleanup)(struct asmexpression * self);
};

struct reg {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct reg * r);
	void (*cleanup)(struct reg * self);
	
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
	void (*cleanup)(struct immediate * self);
	int * value;
	char * str;
};

struct effective_address8086 {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct effective_address8086 * ea);
	void (*cleanup)(struct effective_address8086 * self);
	
	struct reg * segment;
	struct reg * base;
	struct reg * index;
	struct asmexpression * displacement;
	size_t size;
};

struct immediate * new_label(const char * str);
struct immediate * new_imm(int value);
struct immediate * new_immf(float value);
struct effective_address8086 * new_ea8086(struct reg * segment,
                                        struct reg * base,
										struct reg * index,
										struct asmexpression * displacement,
										size_t size, int cd);

void write_instr(FILE * f, const char * instr, size_t ops, ...);
void write_label(FILE * f, struct immediate * lbl);
void write_resb(FILE * f, size_t cnt);

void asm_enter_block(void);
void asm_leave_block(void);

struct immediate * get_tmp_label(void);

#endif
