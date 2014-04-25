#include <parsing.h>
#include <codegen.h>
#include <eparsingapi.h>

static int scope = -1;

static struct immediate * break_label;
static struct immediate * continue_label;

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars);

static void sparser_if(FILE * file, FILE * ofile, struct list * vars)
{
	struct expression cond;
	void * condpacked;
	struct immediate * onfalse;
	onfalse = get_tmp_label();
	
	enter_exprenv(ofile);
	gettok(file);
	if (strcmp(token.str, "(")) {
		showerror(stderr, "error", "expected '('");
	}
	gettok(file);
	condpacked = eparser(file, ofile, vars);
	cond = unpacktoflags(condpacked);
	cjmp_nc(ofile, cond.asme, onfalse);
	cleane(&cond);
	leave_exprenv();
	
	/* parse if body */
	if (strcmp(token.str, ")")) {
		showerror(stderr, "error", "expected ')'");
	}
	gettok(file);
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
	struct immediate * repeat, * continuebackup, * skipinc, * breakbackup;
	struct expression cond;
	
	continuebackup = continue_label;
	breakbackup = break_label;
	
	repeat = get_tmp_label();
	continue_label = get_tmp_label();
	skipinc = get_tmp_label();
	break_label = get_tmp_label();
	
	/* first statement */
	gettok(file);
	if (strcmp(token.str, "(")) {
		showerror(stderr, "error", "expected '('");
	}
	gettok(file);
	sparser_stat(file, ofile, vars);
	
	write_label(ofile, repeat);
	
	/* condition */
	enter_exprenv(ofile);
	gettok(file);
	cond = unpacktoflags(eparser(file, ofile, vars));
	cjmp_nc(ofile, cond.asme, break_label);
	write_instr(ofile, "jmp", 1, skipinc);
	write_label(ofile, continue_label);
	leave_exprenv();
	
	/* "increment" */
	gettok(file);
	sparser_stat(file, ofile, vars);
	
	write_instr(ofile, "jmp", 1, repeat);
	write_label(ofile, skipinc);
	
	if (strcmp(token.str, ")")) {
		showerror(stderr, "error", "expected ')'");
	}
	gettok(file);
	sparser_stat(file, ofile, vars);
	write_instr(ofile, "jmp", 1, continue_label);
	write_label(ofile, break_label);
	
	continue_label->cleanup(continue_label);
	repeat->cleanup(repeat);
	skipinc->cleanup(skipinc);
	break_label->cleanup(break_label);
	
	continue_label = continuebackup;
	break_label = breakbackup;
}

static void sparser_while(FILE * file, FILE * ofile, struct list * vars)
{
	struct immediate * continuebackup, * breakbackup;
	struct expression cond;
	
	continuebackup = continue_label;
	breakbackup = break_label;
	
	continue_label = get_tmp_label();
	break_label = get_tmp_label();
	
	write_label(ofile, continue_label);
	
	/* condition */
	gettok(file);
	if (strcmp(token.str, "(")) {
		showerror(stderr, "error", "expected '('");
	}
	gettok(file);
	enter_exprenv(ofile);
	cond = unpacktoflags(eparser(file, ofile, vars));
	cjmp_nc(ofile, cond.asme, break_label);
	leave_exprenv();
	
	if (strcmp(token.str, ")")) {
		showerror(stderr, "error", "expected ')'");
	}
	gettok(file);
	sparser_stat(file, ofile, vars);
	write_instr(ofile, "jmp", 1, continue_label);
	write_label(ofile, break_label);
	
	continue_label->cleanup(continue_label);
	break_label->cleanup(break_label);
	
	continue_label = continuebackup;
	break_label = breakbackup;
}

static void sparser_do(FILE * file, FILE * ofile, struct list * vars)
{
	struct immediate * repeat, * breakbackup, * continuebackup;
	struct expression cond;
	
	breakbackup = break_label;
	continuebackup = continue_label;
	
	repeat = get_tmp_label();
	continue_label = get_tmp_label();
	break_label = get_tmp_label();
	
	write_label(ofile, repeat);
	
	/* body */
	gettok(file);
	sparser_stat(file, ofile, vars);
	write_label(ofile, continue_label);
	
	/* condition */
	gettok(file);
	if (strcmp(token.str, "while")) {
		showerror(stderr, "error", "expected 'while'");
	}
	gettok(file);
	if (strcmp(token.str, "(")) {
		showerror(stderr, "error", "expected '('");
	}
	gettok(file);
	
	enter_exprenv(ofile);
	cond = unpacktoflags(eparser(file, ofile, vars));
	cjmp_c(ofile, cond.asme, repeat);
	leave_exprenv();
	
	if (strcmp(token.str, ")")) {
		showerror(stderr, "error", "expected ')'");
	}
	gettok(file);
	if (strcmp(token.str, ";")) {
		showerror(stderr, "error", "expected ';'");
	}
	
	write_label(ofile, break_label);
	
	repeat->cleanup(repeat);
	break_label->cleanup(break_label);
	continue_label->cleanup(continue_label);
	
	continue_label = continuebackup;
	break_label = breakbackup;
}

static void sparser_asm(FILE * file, FILE * ofile)
{
	char stopc;
	int ch;
	int lastcc = 0;
	
	gettok(file);
	write_comment(ofile, 1, "begin user generated asm");
	
	stopc = strcmp(token.str, "{") ? ';' : '}';
	
	if (stopc == ';') {
		fprintf(ofile, "\t%s", token.str);
	}
	
	while ((ch = getc(file)) != stopc) {
		if (!lastcc || ch > ' ') {
			if (ch == '\n') {
				lastcc = 1;
				fprintf(ofile, "\n\t");
				continue;
			}
			putc(ch, ofile);
			lastcc = 0;
		}
	}
	
	write_comment(ofile, 1, "end user generated asm");
	
	/*gettok(file);*/
}

static void sparser_stat(FILE * file, FILE * ofile, struct list * vars)
{
	if (!strcmp(token.str, "{")) {
		sparser_body(file, ofile, vars);
	} else if (!strcmp(token.str, "if")) {
		sparser_if(file, ofile, vars);
	} else if (!strcmp(token.str, "for")) {
		sparser_for(file, ofile, vars);
	} else if (!strcmp(token.str, "while")) {
		sparser_while(file, ofile, vars);
	} else if (!strcmp(token.str, "do")) {
		sparser_do(file, ofile, vars);
	} else if (!strcmp(token.str, "break")) {
		
		write_instr(ofile, "jmp", 1, break_label);
		gettok(file);
		
	} else if (!strcmp(token.str, "continue")) {
		
		write_instr(ofile, "jmp", 1, continue_label);
		gettok(file);
		
	} else if (!strcmp(token.str, "asm")) {
		
		sparser_asm(file, ofile);
		
	} else if (!strcmp(token.str, ";")) {
		/*write_instr(ofile, "nop", 0);*/
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
		showerror(stderr, "error", "unexpected token %s", token.str);
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
