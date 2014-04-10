#include <options.h>
#include <codegen.h>
#include <parsing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list functions;
struct list types;
struct list globals;

void fparser_init(void) {
	functions = new_list(32);
	types = new_list(32);
	globals = new_list(32);
}

void parse_type(FILE * file, struct tok token) {
	if (!strcmp(token.str, "struct")) {
		struct type_struct s;
		s.ty = STRUCT;
		
		gettok(file, &token);
		s.namelen = token.len;
		memcpy(s.name, token.str, token.len);
		
		
	}
}

int fparse_element(FILE * file)
{
	struct tok token;
	gettok(file, &token);
	if (token.ty == STOP) {
		return 0;
	}
	
	switch (token.ty) {
		case TYPE_TYPE:
			parse_type(file, token);
			break;
	}
	
	return 1;
}
