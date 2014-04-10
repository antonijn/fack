#ifndef OPTIONS_H
#define OPTIONS_H

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
extern const char * outfile;
extern const char * filenames[32];
extern enum machine target;

#endif
