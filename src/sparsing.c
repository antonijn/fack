#include <parsing.h>
#include <codegen.h>
#include <eparsingapi.h>

static int scope = -1;

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars);

static void sparser_if(FILE * file, FILE * ofile, struct list * vars)
{
	struct expression cond;
	void * condpacked;
	struct immediate * onfalse;
	onfalse = get_tmp_label();
	
	enter_exprenv(ofile);
	gettok(file);
	condpacked = eparser(file, ofile, vars);
	cond = unpacktoflags(condpacked);
	
	cjmp_nc(ofile, cond.asme, onfalse);
	
	cleane(&cond);
	
	leave_exprenv();
	
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

static void sparser_for(FILE * file, FILE * ofile, struct list * vars)
{
	struct immediate * repeat, * increment, * skipinc, * _break;
	struct expression cond;
	
	repeat = get_tmp_label();
	increment = get_tmp_label();
	skipinc = get_tmp_label();
	_break = get_tmp_label();
	
	/* first statement */
	gettok(file);
	gettok(file);
	sparser_stat(file, ofile, vars);
	
	write_label(ofile, repeat);
	
	/* condition */
	enter_exprenv(ofile);
	gettok(file);
	cond = unpacktoflags(eparser(file, ofile, vars));
	cjmp_nc(ofile, cond.asme, _break);
	write_instr(ofile, "jmp", 1, skipinc);
	write_label(ofile, increment);
	leave_exprenv();
	
	/* "increment" */
	gettok(file);
	sparser_stat(file, ofile, vars);
	
	write_instr(ofile, "jmp", 1, repeat);
	write_label(ofile, skipinc);
	
	gettok(file);
	sparser_stat(file, ofile, vars);
	write_instr(ofile, "jmp", 1, increment);
	write_label(ofile, _break);
	
	repeat->cleanup(repeat);
	increment->cleanup(increment);
	skipinc->cleanup(skipinc);
	_break->cleanup(_break);
}

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars)
{
	if (!strcmp(token.str, "{")) {
		sparser_body(file, ofile, vars);
	} else if (!strcmp(token.str, "if")) {
		sparser_if(file, ofile, vars);
	} else if (!strcmp(token.str, "for")) {
		sparser_for(file, ofile, vars);
	} else if (!strcmp(token.str, ";")) {
		write_instr(ofile, "nop", 0);
	} else {
		enter_exprenv(ofile);
		struct expression asme = unpack(eparser(file, ofile, vars));
		cleane(&asme);
		leave_exprenv();
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
