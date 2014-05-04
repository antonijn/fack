#ifndef EPARSINGAPI_H
#define EPARSINGAPI_H

#include <fack/parsing.h>
#include <stdio.h>

/*
 * Enters a new expression.
 */
void enter_exprenv(FILE * ofile);

/*
 * Leaves expression.
 */
void leave_exprenv(void);

int packedinflags(void * p);

/*
 * Packs an expression, protecting its value until unpacking.
 */
void * pack(struct expression e);

/*
 * Unpacks an expression, regaining a usable value,
 * leaving its value open to be changed.
 */
struct expression unpack(void * p);
struct expression unpackx(void * p, int size);

/*
 * Unpacks an expression as an lvalue, so the result
 * is guaranteed to be an effective address.
 */
struct expression unpacklvalue(void * p);

/*
 * Unpacks an expression into a general purpose register.
 */
struct expression unpacktogpr(void * p);
struct expression unpacktogprx(void * p, int size);

/*
 * Unpacks an expression into a general purpose register,
 * unless the expression is an effective address.
 */
struct expression unpacktogprea(void * p);
struct expression unpacktogpreax(void * p, int size);

/*
 * Unpacks an expression into an rvalue.
 */
struct expression unpacktorvalue(void * p, struct expression e);
struct expression unpacktorvaluex(void * p, struct expression e, int size);

/*
 * Unpacks an expression into the flags register, so it's
 * available for jumping.
 */
struct expression unpacktoflags(void * p);

/*
 * Picks first clean general purpose register from the list
 * or cloggs one if none are clean.
 * Won't return registers marked as forbidden by disallow_gpr.
 */
struct reg * getgpr(size_t sz);

/*
 * Cloggs an effective address.
 */
void cloggmem(struct effective_address8086 * ea);

/*
 * Cloggs a register.
 * If a packed expression is in the register, it's value is
 * saved on the stack.
 * The value will then be retrieved off the stack on unpacking.
 */
void clogg(struct reg * r);

/*
 * Cloggs the flags register.
 */
void cloggflags(void);

/*
 * Cloggs all registers.
 */
void cloggall(void);

/*
 * Disallow a general purpose register for unpacking.
 * Useful for pinning down a register used for the result value.
 *
 * note: if r is not a struct reg *, this function does nothing
 */
void shield(struct asmexpression * r);

/*
 * Disallows all general purpose registers to be used.
 */
void shield_all(void);

/*
 * Allows a general purpose register to be used.
 * Opposite of disallow_gpr.
 */
void unshield(struct asmexpression * r);

/*
 * Enters an operation, allows all registers to be used for unpacking.
 * Basically an undo operation for disallow_gpr.
 */
void unshield_all(void);

/*
 * Performs an integer size cast.
 * Name is misleading, promoting isn't necessarily involved.
 */
struct expression promote(struct expression e, int size);

struct ctype * unpackty(void * p);

#endif
