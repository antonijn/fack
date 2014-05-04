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

#define REGS_COUNT 14

void printreginfo(void)
{
	int i;
	for (i = 0; i < REGS_COUNT; ++i) {
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

static void occupy(struct reg * r)
{
	int i;
	if (r->arch != INTEL_80386 &&
		target == INTEL_8086) {
		return;
	}
	if (r->parent) {
		occupy(r->parent);
	}
	for (i = 0; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].occupied = 1;
			return;
		}
	}
}

static void freereg(struct reg * r)
{
	int i;
	if (!r) {
		return;
	}
	if (r->arch != INTEL_80386 &&
		target == INTEL_8086) {
		return;
	}
	if (r->parent) {
		freereg(r->parent);
	}
	for (i = 0; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].occupied = 0;
			return;
		}
	}
}

static int isoccupied(struct reg * r)
{
	int i;
	for (i = 0; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			return regs[i].occupied;
		}
	}
}

static int isshielded(struct reg * r)
{
	int i;
	for (i = 0; i < REGS_COUNT; ++i) {
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
	
	for (i = 0; i < REGS_COUNT; ++i) {
		if (isavailable(regs[i].r) && regs[i].r->size == sz) {
			return regs[i].r;
		}
	}
	
	for (i = 0; i < REGS_COUNT; ++i) {
		if (!isshielded(regs[i].r) && regs[i].r->size == sz) {
			clogg(regs[i].r);
			return regs[i].r;
		}
	}
}

void clogg(struct reg * r)
{
	int i;
	if (r->arch != INTEL_80386 &&
		target == INTEL_8086) {
		return;
	}
	if (r->parent) {
		clogg(r->parent);
	}
	
	for (i = 0; i < packed.count; ++i) {
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
	
	for (i = packed.count - 1; i >= 0; --i) {
		struct expr_internal * ei = packed.elements[i].element;
		if (ei->e.asme->ty == DEREFERENCE && !ei->r && !ei->pushed) {
			write_instr(ofile, "push", 1, ei->e.asme);
			ei->pushed = 1;
		}
	}
	
	clogg(&ax);
	clogg(&bx);
	clogg(&cx);
	clogg(&dx);
	clogg(&si);
	clogg(&di);
	cloggflags();
}

void cloggmem(struct effective_address8086 * ea)
{
	int i;
	for (i = 0; i < packed.count; ++i) {
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
	for (i = 0; i < packed.count; ++i) {
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
		if (((struct effective_address8086 *)e.asme)->base != &bp &&
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

struct expression unpack(void * p)
{
	struct expression expr;
	struct expr_internal * ei = p;
	if (!p) {
		return expr;
	}
	
	if (ei->pushed) {
		return unpacktogpr(p);
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
		return expr;
	}
	expr = ei->e;
	if (expr.asme->ty != IMMEDIATE)
	{
		removefromlist(&packed, p);
	}
	return expr;
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
	struct expr_internal * ei = p;
	if (ei->e.asme->ty == FLAG) {
		return unpacktogpr(p);
	}
	if (e.asme->ty == DEREFERENCE && ei->e.asme->ty == DEREFERENCE
		&& !ei->pushed && !ei->r) {
		return unpacktogpr(p);
	}
	return unpack(p);
}

struct expression unpacktogpr(void * p)
{
	struct expression expr;
	struct expr_internal * ei = p;
	if (ei->pushed) {
		expr.asme = getgpr(ei->e.type->size);
		expr.type = ei->e.type;
		write_instr(ofile, "pop", 1, expr.asme); /* temporary bad code */
		removefromlist(&packed, p);
		return expr;
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
		return expr;
	}
	if (ei->e.asme->ty == REGISTER) {
		expr = ei->e;
		if (isshielded(expr.asme)) {
			expr.asme = getgpr(expr.type->size);
			write_instr(ofile, "mov", 2, expr.asme, ei->e.asme);
		}
		removefromlist(&packed, p);
		return expr;
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
		return expr;
	}
	expr.asme = getgpr(ei->e.type->size);
	expr.type = ei->e.type;
	write_instr(ofile, "mov", 2, expr.asme, ei->e.asme);
	if (expr.asme->ty != IMMEDIATE)
	{
		removefromlist(&packed, p);
	}
	return expr;
}

struct expression unpacktogprea(void * p)
{
	struct expr_internal * ei = p;
	if (ei->e.asme->ty == DEREFERENCE && !ei->pushed && !ei->r) {
		freelist(&ei->rlist);
		return ei->e;
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
	unshield(&ax);
	unshield(&bx);
	unshield(&cx);
	unshield(&dx);
	unshield(&si);
	unshield(&di);
}

void unshield(struct asmexpression * r)
{
	int i;
	
	if (r->ty != REGISTER) {
		return;
	}
	
	for (i = 0; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].shielded = 0;
			return;
		}
	}
}

void shield_all(void)
{
	shield(&ax);
	shield(&bx);
	shield(&cx);
	shield(&dx);
	shield(&si);
	shield(&di);
}

void shield(struct asmexpression * r)
{
	int i;
	
	if (r->ty != REGISTER) {
		return;
	}
	
	for (i = 0; i < REGS_COUNT; ++i) {
		if (regs[i].r == r) {
			regs[i].shielded = 1;
			return;
		}
	}
}

struct ctype * unpackty(void * p)
{
	struct expr_internal * ei = p;
	return ei->e.type;
}
