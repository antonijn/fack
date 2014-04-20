#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <list.h>

enum asmexpressiontype {
	IMMEDIATE,
	REGISTER,
	DEREFERENCE,
	FLAG,
};

struct asmexpression {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct asmexpression * asme);
	void (*cleanup)(struct asmexpression * self);
};

struct reg {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct reg * r);
	void (*cleanup)(void * self);
	
	struct reg * parent;
	const char * name;
	size_t size;
	enum machine arch;
};

struct flags_flag {
	enum asmexpressiontype ty;
	void (*tostring)(FILE * f, struct flags_flag * r);
	void (*cleanup)(void * self);
	
	const char * name;
	enum machine arch;
};

extern struct reg eax, ebx, ecx, edx, esi, edi, ebp, esp;
extern struct reg ax, bx, cx, dx, si, di, bp, sp;
extern struct reg al, ah, bl, bh, cl, ch, dl, dh;
extern struct reg cs, ds, ss, es, fs, gs;
extern struct flags_flag
	f_e, f_ne, f_g, f_ge, f_l, f_le,
	f_b, f_be, f_a, f_ae,
	f_s, f_ns, f_z, f_nz;

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
void write_directive(FILE * f, const char * dir);

/*
 * Enter an asm block, helps the nomenclature for get_tmp_label()
 * */
void asm_enter_block(void);
/*
 * Leave an asm block.
 * */
void asm_leave_block(void);

/*
 * Moves an expression into a register, baring in mind the illegal registers.
 * */
struct reg * toreg(FILE * f, struct asmexpression * x, struct list exclude);
/*
 * Pushes an asm expression onto the stack.
 * */
void pushasme(FILE * f, struct asmexpression * x);
/*
 * Conditional jumps if condition x.
 * */
void cjmp_c(FILE * f, struct asmexpression * x, struct immediate * jmp);
/*
 * Conditional jumps if not condition x.
 * */
void cjmp_nc(FILE * f, struct asmexpression * x, struct immediate * jmp);

/*
 * Get a new function-scoped label.
 * */
struct immediate * get_tmp_label(void);

#endif
