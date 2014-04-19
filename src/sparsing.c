#include <parsing.h>
#include <codegen.h>

static int scope = -1;

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars);

static void sparser_if(FILE * file, FILE * ofile, struct list * vars)
{
	struct expression cond;
	struct immediate * onfalse;
	onfalse = get_tmp_label();
	
	gettok(file);
	cond = eparser(file, ofile, NULL, INFLAGS, vars);
	
	cjmp_nc(ofile, cond.asme, onfalse);
	
	cleane(cond);
	
	/* parse if body */
	sparser_stat(file, ofile, vars);
	
	gettok(file);
	if (!strcmp(token.str, "else")) {
		struct immediate * skipelse = get_tmp_label();
		
		write_instr(ofile, "jmp", 1, skipelse);
		
		write_label(ofile, onfalse);
		gettok(file);
		sparser_stat(file, ofile, vars);
		
		write_label(ofile, skipelse);
		
		skipelse->cleanup(skipelse);
	} else {
		write_label(ofile, onfalse);
	}
	
	onfalse->cleanup(onfalse);
}

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars)
{
	if (!strcmp(token.str, "{")) {
		sparser_body(file, ofile, vars);
	} else if (!strcmp(token.str, "if")) {
		sparser_if(file, ofile, vars);
	} else if (!strcmp(token.str, ";")) {
		write_instr(ofile, "nop", 0);
	} else {
		struct expression asme = eparser(file, ofile, NULL, NONE, vars);
		cleane(asme);
	}
}

void sparser_body(FILE * file, FILE * ofile, struct list * vars)
{
	int vc = vars->count;
	
	++scope;
	asm_enter_block();
	
	if (token.str[0] != '{') {
		fprintf(stderr, "error: unexpected token %s\n", token.str);
	}
	
	while (1)
	{
		gettok(file);
		if (token.str[0] == '}') {
			break;
		}
		sparser_stat(file, ofile, vars);
	}
	
	removelast(vars, vars->count - vc);
	
	asm_leave_block();
	--scope;
}
