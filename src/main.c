#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fack/options.h>
#include <fack/parsing.h>
#include <fack/codegen.h>
#include <fack/cpus/8086.h>
#include <fack/cpus/80386.h>

const char * outfile = "out";
const char * filenames[32] = { 0 };
struct targ target;

int main(int argc, char **argv) {
	int i, fncnt = 0;
	
	--argc;
	++argv;
	
	target.cpu = m8086;
	target.form = ff_bin;
	
	for (i = 0; i < argc; ++i) {
		char *arg = argv[i];
		if (!strcmp(arg, "-o")) {
			++i;
			arg = argv[i];
			outfile = (const char *)arg;
		} else if (!strcmp(arg, "-m8086")) {
			target.cpu = m8086;
		} else if (!strcmp(arg, "-m80386")) {
			target.cpu = m80386;
		} else {
			filenames[fncnt++] = arg;
		}
	}
	
	if (!filenames[0]) {
		fprintf(stderr, "error: no input files specified\n");
		exit(1);
	}
	
	for (i = 0; filenames[i]; ++i) {
		parse(filenames[i]);
	}
	
	return 0;
}
