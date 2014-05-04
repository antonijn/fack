#include <fack/cpus/8086.h>
#include <fack/codegen.h>
#include <fack/eparsingapi.h>

struct cconv cdecl8086 = {
	&prepcall_cdecl8086,
	&cleancall_cdecl8086,
	&call_cdecl8086,
	&loadparam_cdecl8086,
	&retrieveret_cdecl8086,
	&retrieveparam_cdecl8086,
	&loadret_cdecl8086,
};

struct arch m8086 = {
	2,
	&ax, &bx, &cx, &dx,
	&si, &di, &sp, &bp,
	
	&cdecl8086, NULL, NULL,
	
	6, "8086"
};

void prepcall_cdecl8086(FILE * ofile, struct cfunctiontype * cft)
{
	growstack(ofile, cft->paramdepth);
}

void cleancall_cdecl8086(FILE * ofile, struct cfunctiontype * cft)
{
	shrinkstack(ofile, cft->paramdepth);
}

void call_cdecl8086(FILE * ofile, void * cft)
{
	clogg(target.cpu.acc);
	clogg(target.cpu.count);
	clogg(target.cpu.data);
	write_instr(ofile, "call", 1, unpack(cft).asme);
}

void loadparam_cdecl8086(FILE * ofile, struct cfunctiontype * cft, int param, void * p)
{
	struct expression parame;
	int depth = 0, i;
	for (i = cft->paramtypes->count - 1; param <= i; ++i) {
		struct ctype * pty = cft->paramtypes->elements[i].element;
		depth -= pty->size;
	}
	parame.asme = new_ea8086(&ss, target.cpu.basep, depth, NULL, new_imm(depth), 1);
	parame.type = cft->paramtypes->elements[param].element;
	
	write_instr(ofile, "mov", 2, parame.asme, unpacktorvalue(p, parame).asme);
}

void * retrieveret_cdecl8086(FILE * ofile, struct cfunctiontype * cft)
{
	struct expression e;
	e.asme = target.cpu.acc;
	e.type = cft->ret;
	return pack(e);
}

void * retrieveparam_cdecl8086(FILE * ofile, struct cfunctiontype * cft, int param)
{
	struct expression parame;
	int i, depth = 0;
	for (i = cft->paramtypes->count - 1; param <= i; ++i) {
		struct ctype * pty = cft->paramtypes->elements[i].element;
		depth -= pty->size;
	}
	parame.asme = new_ea8086(&ss, target.cpu.basep, depth, NULL, new_imm(depth - 2), 1);
	parame.type = cft->paramtypes->elements[param].element;
	return pack(parame);
}

void loadret_cdecl8086(FILE * ofile, struct cfunctiontype * cft, void * ret)
{
	shield_all();
	unshield(target.cpu.acc);
	unpacktogpr(ret);
}
