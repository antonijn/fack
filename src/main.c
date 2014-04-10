#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <options.h>
#include <parsing.h>
#include <codegen.h>

enum optimise optimiseLevel = O0;
const char * outfile = "out";
const char * filenames[32] = { 0 };
enum machine target = INTEL_8086;

static void print_options(void)
{
	const char *fname;
	int i = 0;
	
	printf("Optimisation: ");
	switch (optimiseLevel) {
	case O0:
		printf("O0\n");
		break;
	case O1:
		printf("O1\n");
		break;
	case O2:
		printf("O2\n");
		break;
	}
	
	printf("Output: %s\n", outfile);
	printf("Input:\n");
	while (fname = filenames[i++]) {
		printf("\t%s\n", fname);
	}
	
	printf("Target: ");
	switch (target) {
	case INTEL_8086:
		printf("Intel 8086 (16-bit)\n");
		break;
	case INTEL_80386:
		printf("Intel 80386 (32-bit)\n");
		break;
	}
}

int main(int argc, char **argv) {
	int i, fncnt = 0;
	
	--argc;
	++argv;
	
	for (i = 0; i < argc; ++i) {
		char *arg = argv[i];
		if (!strcmp(arg, "-O0")) {
			optimiseLevel = O0;
		} else if (!strcmp(arg, "-O1")) {
			optimiseLevel = O1;
		} else if (!strcmp(arg, "-O2")) {
			optimiseLevel = O2;
		} else if (!strcmp(arg, "-o")) {
			++i;
			arg = argv[i];
			outfile = (const char *)arg;
		} else if (!strcmp(arg, "-m8086")) {
			target = INTEL_8086;
		} else if (!strcmp(arg, "-m80386")) {
			target = INTEL_80386;
		} else {
			filenames[fncnt++] = arg;
		}
	}
	
	for (i = 0; filenames[i]; ++i) {
		parse(filenames[i]);
	}
	
	return 0;
}
