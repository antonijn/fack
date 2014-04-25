#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>

enum optimise {
	O0,
	O1,
	O2,
	O3
};

enum machine {
	INTEL_8086 = 0x01,
	INTEL_80386 = 0x02
};

extern enum optimise optimiseLevel;
extern enum machine target;
extern const char * outfile;
extern const char * filenames[32];

extern int _linenum;
extern int _column;

void showerror(FILE * f, const char * type, const char * frmt, ...);

#endif
