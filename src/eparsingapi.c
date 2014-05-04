#include <stdlib.h>

#include <fack/options.h>
#include <fack/eparsingapi.h>

struct expr_internal {
	struct expression e;
	struct reg * r; /* used for storing memory values */
	struct list rlist;
	unsigned int pushed : 1;
};

struct reginfo {
	struct reg * r;
	unsigned int occupied : 1;
	unsigned int shielded : 1;
};

static struct list packed;
static FILE * ofile;

static struct reginfo regs[] = {
	{ &eax, 0, 0 },
	{ &ebx, 0, 0 },
	{ &ecx, 0, 0 },
	{ &edx, 0, 0 },
	{ &esi, 0, 0 },
	{ &edi, 0, 0 },
	
	{ &ax, 0, 0 },
	{ &bx, 0, 0 },
	{ &cx, 0, 0 },
	{ &dx, 0, 0 },
	{ &si, 0, 0 },
	{ &di, 0, 0 },
	
	{ &al, 0, 0 },
	{ &bl, 0, 0 },
	{ &cl, 0, 0 },
	{ &dl, 0, 0 },
	
	{ &ah, 0, 0 },
	{ &bh, 0, 0 },
	{ &ch, 0, 0 },
	{ &dh, 0, 0 },
};

#define REGS_COUNT (sizeof(regs)/sizeof(regs[0]))

void printreginfo(void)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		printf("{ %s, %d, %d }\n", regs[i].r->name, regs[i].occupied, regs[i].shielded);
	}
}

int packedinflags(void * p)
{
	struct expr_internal * ei = p;
	
	if (!ei->pushed && ei->e.asme->ty == FLAG) {
		return 1;
	}
	return 0;
}

static void occup(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].occupied = 1;
			return;
		}
	}
}

static void occupydown(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r->parent == r) {
			occupydown(regs[i].r);
		}
	}
	occup(r);
}

static void occupyup(struct reg * r)
{
	if (r->parent) {
		occupyup(r->parent);
	}
	occup(r);
}

static void occupy(struct reg * r)
{
	if (!r) {
		return;
	}
	occupyup(r);
	occupydown(r);
}

static void freer(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].occupied = 0;
			return;
		}
	}
}

static void freeregdown(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r->parent == r) {
			freeregdown(regs[i].r);
		}
	}
	freer(r);
}

static void freeregup(struct reg * r)
{
	if (r->parent) {
		freeregup(r->parent);
	}
	freer(r);
}

static void freereg(struct reg * r)
{
	if (!r) {
		return;
	}
	freeregup(r);
	freeregdown(r);
}

void shieldr(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].shielded = 1;
			return;
		}
	}
}

void shielddown(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r->parent == r) {
			shielddown(regs[i].r);
		}
	}
	shieldr(r);
}

void shieldup(struct reg * r)
{
	if (r->parent) {
		shieldup(r->parent);
	}
	shieldr(r);
}

void shield(struct asmexpression * r)
{
	if (!r || r->ty != REGISTER) {
		return;
	}
	shieldup(r);
	shielddown(r);
}

void unshieldr(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].shielded = 0;
			return;
		}
	}
}

void unshielddown(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r->parent == r) {
			unshielddown(regs[i].r);
		}
	}
	unshieldr(r);
}

void unshieldup(struct reg * r)
{
	if (r->parent) {
		unshieldup(r->parent);
	}
	unshieldr(r);
}

void unshield(struct asmexpression * r)
{
	if (!r || r->ty != REGISTER) {
		return;
	}
	unshieldup(r);
	unshielddown(r);
}

static int isoccupied(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			return regs[i].occupied;
		}
	}
}

static int isshielded(struct reg * r)
{
	int i;
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			return regs[i].shielded;
		}
	}
}

static int isavailable(struct reg * r)
{
	return !isoccupied(r) && !isshielded(r);
}

void enter_exprenv(FILE * outfile)
{
	packed = new_list(16);
	ofile = outfile;
}

void leave_exprenv(void)
{
	freelist(&packed);
}

struct reg * getgpr(size_t sz)
{
	int i;
	
	/*printf("GPR requested \n");
	print_regs();*/
	
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (isavailable(regs[i].r) && regs[i].r->size == sz) {
			return regs[i].r;
		}
	}
	
	for (i = target.cpu.offs; i < REGS_COUNT; ++i) {
		if (!isshielded(regs[i].r) && regs[i].r->size == sz) {
			clogg(regs[i].r);
			return regs[i].r;
		}
	}
}

void clogg(struct reg * r)
{
	int i;
	if (r->parent) {
		clogg(r->parent);
	}
	
	for (i = target.cpu.offs; i < packed.count; ++i) {
		struct expr_internal * ei = packed.elements[i].element;
		if (!ei->pushed) {
			if (ei->e.asme == r) {
				write_instr(ofile, "push", 1, ei->e.asme);
				ei->pushed = 1;
				freereg(ei->e.asme);
				return;
			} else if (ei->r == r) {
				write_instr(ofile, "push", 1, ei->r);
				ei->pushed = 1;
				freereg(ei->r);
				return;
			}
		}
	}
}

void cloggall(void)
{
	int i;
	
	for (i = packed.count - 1; i >= target.cpu.offs; --i) {
		struct expr_internal * ei = packed.elements[i].element;
		if (ei->e.asme->ty == DEREFERENCE && !ei->r && !ei->pushed) {
			write_instr(ofile, "push", 1, ei->e.asme);
			ei->pushed = 1;
		}
	}
	
	clogg(target.cpu.acc);
	clogg(target.cpu.base);
	clogg(target.cpu.count);
	clogg(target.cpu.data);
	clogg(target.cpu.sourcei);
	clogg(target.cpu.desti);
	cloggflags();
}

void cloggmem(struct effective_address8086 * ea)
{
	int i;
	for (i = target.cpu.offs; i < packed.count; ++i) {
		struct expr_internal * ei = packed.elements[i].element;
		/* reference equality for now */
		if (ei->e.asme == ea && !ei->r && !ei->pushed) {
			ei->r = getgpr(ei->e.type->size);
			occupy(ei->r);
			write_instr(ofile, "mov", 2, ei->r, ei->e.asme);
			return;
		}
	}
}

void cloggflags(void)
{
	int i;
	for (i = target.cpu.offs; i < packed.count; ++i) {
		struct expr_internal * ei = packed.elements[i].element;
		if (!ei->pushed && ei->e.asme->ty == FLAG) {
			struct immediate * skip;
			struct reg * gpr;
			
			gpr = getgpr(2);
			
			skip = get_tmp_label();
			
			write_instr(ofile, "xor", 2, gpr, gpr);
			cjmp_nc(ofile, ei->e.asme, skip);
			write_instr(ofile, "inc", 1, gpr);
			write_label(ofile, skip);
			write_instr(ofile, "push", 1, gpr);
			ei->pushed = 1;
			
			skip->cleanup(skip);
		}
	}
}

static void free_expr_internal(void * p)
{
	struct expr_internal * ei = p;
	if (!ei->pushed) {
		if (ei->e.asme->ty == REGISTER) {
			freereg(ei->e.asme);
		} else if (ei->e.asme->ty == DEREFERENCE) {
			freereg(ei->r);
		}
	}
	free(p);
}

struct packedderefreg
{
	void * p;
	struct effective_address8086 * ea;
};

static void cleanderefreg(struct packedderefreg * p)
{
	/* gaah! is this allowed? D: */
	shield_all();
	unshield(p->ea->base);
	
	unpacktogpr(p->p);
	
	unshield_all();
	
	free(p);
}

void * pack(struct expression e)
{
	struct expr_internal * ei = malloc(sizeof(struct expr_internal));
	
	ei->e = e;
	if (e.asme->ty == DEREFERENCE) {
		ei->r = NULL;
		ei->rlist = new_list(4);
		if (((struct effective_address8086 *)e.asme)->base != target.cpu.basep &&
			((struct effective_address8086 *)e.asme)->base) {
			struct expression pdre;
			struct packedderefreg * pdr = malloc(sizeof(struct packedderefreg));
			pdre.asme = ((struct effective_address8086 *)e.asme)->base;
			pdre.type = &_int;
			pdr->p = pack(pdre);
			pdr->ea = e.asme;
			add(&ei->rlist, pdr, &cleanderefreg);
		}
		/*ei->r = getgpr();
		occupy(ei->r);
		write_instr(ofile, "mov", 2, ei->r, e.asme);*/
	}
	ei->pushed = 0;
	if (e.asme->ty == REGISTER) {
		occupy(e.asme);
	}
	if (e.asme->ty != IMMEDIATE) {
		add(&packed, ei, &free_expr_internal);
	}
	return ei;
}

struct expression promote(struct expression e, int size)
{
	struct reg * assigned;
	int i;
	
	if (e.type->size == size) {
		return e;
	}
	
	/* assume only signed stuff for now */
	assigned = getgpr(size);
	if (size > e.type->size) {
		write_instr(ofile, "movsx", 2, assigned, e.asme);
		e.asme = assigned;
		return e;
	}
	
	if (e.asme == &eax) {
		switch (size) {
			case 1:
				e.asme = &al;
				return e;
			case 2:
				e.asme = &ax;
				return e;
		}
	} else if (e.asme == &ebx) {
		switch (size) {
			case 1:
				e.asme = &bl;
				return e;
			case 2:
				e.asme = &bx;
				return e;
		}
	} else if (e.asme == &ecx) {
		switch (size) {
			case 1:
				e.asme = &cl;
				return e;
			case 2:
				e.asme = &cx;
				return e;
		}
	} else if (e.asme == &edx) {
		switch (size) {
			case 1:
				e.asme = &dl;
				return e;
			case 2:
				e.asme = &dx;
				return e;
		}
	}
	
	if (e.asme->ty == DEREFERENCE) {
		/* don't we all love little endian? <3 */
		((struct effective_address8086 *)e.asme)->size = size;
	}
	return e;
}

struct expression unpack(void * p)
{
	if (!p) {
		struct expression e;
		e.asme = NULL;
		e.type = NULL;
		return e;
	}
	return unpackx(p, unpackty(p)->size);
}

struct expression unpackx(void * p, int size)
{
	struct expression expr;
	struct expr_internal * ei = p;
	if (!p) {
		return expr;
	}
	
	if (ei->pushed) {
		return unpacktogprx(p, size);
	}
	if (ei->e.asme->ty == DEREFERENCE) {
		freelist(&ei->rlist);
		if (ei->r) {
			expr.asme = ei->r;
		} else {
			expr.asme = ei->e.asme;
		}
		expr.type = ei->e.type;
		removefromlist(&packed, p);
		return promote(expr, size);
	}
	expr = ei->e;
	if (expr.asme->ty != IMMEDIATE)
	{
		removefromlist(&packed, p);
	}
	return promote(expr, size);
}

struct expression unpacklvalue(void * p)
{
	struct expression expr;
	struct expr_internal * ei = p;
	expr = ei->e;
	removefromlist(&packed, p);
	return expr;
}

struct expression unpacktorvalue(void * p, struct expression e)
{
	return unpacktorvaluex(p, e, unpackty(p)->size);
}

struct expression unpacktorvaluex(void * p, struct expression e, int size)
{
	struct expr_internal * ei = p;
	if (ei->e.asme->ty == FLAG) {
		return unpacktogprx(p, size);
	}
	if (e.asme->ty == DEREFERENCE && ei->e.asme->ty == DEREFERENCE
		&& !ei->pushed && !ei->r) {
		return unpacktogprx(p, size);
	}
	return unpackx(p, size);
}

struct expression unpacktogpr(void * p)
{
	return unpacktogprx(p, unpackty(p)->size);
}

struct expression unpacktogprx(void * p, int size)
{
	struct expression expr;
	struct expr_internal * ei = p;
	if (ei->pushed) {
		expr.asme = getgpr(ei->e.type->size);
		expr.type = ei->e.type;
		write_instr(ofile, "pop", 1, expr.asme); /* temporary bad code */
		removefromlist(&packed, p);
		return promote(expr, size);
	}
	if (ei->e.asme->ty == FLAG) {
		struct immediate * skip;
		
		expr.asme = getgpr(ei->e.type->size);
		expr.type = ei->e.type;
		
		skip = get_tmp_label();
		
		write_instr(ofile, "xor", 2, expr.asme, expr.asme);
		cjmp_nc(ofile, ei->e.asme, skip);
		write_instr(ofile, "inc", 1, expr.asme);
		write_label(ofile, skip);
		
		skip->cleanup(skip);
		
		removefromlist(&packed, p);
		return promote(expr, size);
	}
	if (ei->e.asme->ty == REGISTER) {
		expr = ei->e;
		if (isshielded(expr.asme)) {
			expr.asme = getgpr(expr.type->size);
			write_instr(ofile, "mov", 2, expr.asme, ei->e.asme);
		}
		removefromlist(&packed, p);
		return promote(expr, size);
	}
	if (ei->e.asme->ty == DEREFERENCE) {
		freelist(&ei->rlist);
		if (ei->r) {
			expr.asme = ei->r;
		} else {
			expr.asme = getgpr(ei->e.type->size);
			write_instr(ofile, "mov", 2, expr.asme, ei->e.asme);
		}
		expr.type = ei->e.type;
		removefromlist(&packed, p);
		return promote(expr, size);
	}
	expr.asme = getgpr(ei->e.type->size);
	expr.type = ei->e.type;
	write_instr(ofile, "mov", 2, expr.asme, ei->e.asme);
	if (expr.asme->ty != IMMEDIATE)
	{
		removefromlist(&packed, p);
	}
	return promote(expr, size);
}

struct expression unpacktogprea(void * p)
{
	return unpacktogpreax(p, unpackty(p)->size);
}

struct expression unpacktogpreax(void * p, int size)
{
	struct expr_internal * ei = p;
	if (ei->e.asme->ty == DEREFERENCE && !ei->pushed && !ei->r) {
		freelist(&ei->rlist);
		return promote(ei->e, size);
	}
	return unpacktogpr(p);
}

struct expression unpacktoflags(void * p)
{
	struct expression expr;
	struct expr_internal * ei = p;
	
	if (ei->e.asme->ty == FLAG && !ei->pushed) {
		expr = ei->e;
		removefromlist(&packed, p);
		return expr;
	}
	
	expr = unpacktogpr(p);
	write_instr(ofile, "test", 2, expr.asme, expr.asme);
	return expr;
}

void unshield_all(void)
{
	unshield(target.cpu.acc);
	unshield(target.cpu.base);
	unshield(target.cpu.count);
	unshield(target.cpu.data);
	unshield(target.cpu.sourcei);
	unshield(target.cpu.desti);
}

void shield_all(void)
{
	shield(target.cpu.acc);
	shield(target.cpu.base);
	shield(target.cpu.count);
	shield(target.cpu.data);
	shield(target.cpu.sourcei);
	shield(target.cpu.desti);
}

struct ctype * unpackty(void * p)
{
	struct expr_internal * ei = p;
	return ei->e.type;
}
