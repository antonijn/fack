#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>

struct fform {
    int dynlink;
};

extern struct fform ff_bin, ff_com, ff_mz, ff_ne, ff_pe, ff_elf;

struct arch {
	int bytes;
	struct reg * acc, * base, * count, * data;
	struct reg * sourcei, * desti;
	struct reg * stackp, * basep;

	struct cconv * cdecl, * stdcall, * fastcall;
	int offs;
};

extern struct arch m80386;

struct targ {
	struct arch cpu;
	struct fform form;
};

extern struct targ target;

extern const char * outfile;
extern const char * filenames[32];

extern int _linenum;
extern int _column;

void showerror(FILE * f, const char * type, const char * frmt, ...);

#endif
