#include <options.h>
#include <codegen.h>
#include <parsing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse(const char * filename)
{
	FILE * ifile, * ofile;
	char ofname[64] = { 0 };
	
	strcat(ofname, filename);
	strcat(ofname, ".asm");
	
	ifile = fopen(filename, "rb");
	ofile = fopen(ofname, "wb");
	
	fparser_init(ofile);
	while (!fparse_element(ifile))
		;
	fparser_release();
	
	fclose(ifile);
}

static int gettok_numc(int ch)
{
	return (ch >= '0' && ch <= '9');
}

static int gettok_octc(int ch)
{
	return (ch >= '0' && ch <= '7');
}

static int gettok_hexc(int ch)
{
	return gettok_numc(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

static int gettok_idc(int ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

static int gettok_idcc(int ch)
{
	return gettok_idc(ch) || (ch >= '0' && ch <= '9');
}

static int gettok_aopc(int ch)
{
	switch (ch) {
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '|':
	case '&':
	case '^':
	case '~':
	case '!':
	case '=':
	case '>':
	case '<':
		return 1;
	}
	return 0;
}

static int gettok_opc(int ch)
{
	switch (ch) {
	case '(':
	case ')':
	case '{':
	case '}':
	case ',':
		return 1;
	}
	return 0;
}

static void gettok_id(FILE * file)
{
	int ch;
	
	token.ty = IDENTIFIER;
	while ((ch = getc(file)) != EOF && gettok_idcc(ch)) {
		token.str[token.len++] = ch;
	}
	ungetc(ch, file);
	token.str[token.len] = '\0';
	
	if (!strcmp(token.str, "const") ||
		!strcmp(token.str, "unsigned") ||
		!strcmp(token.str, "signed") ||
		!strcmp(token.str, "static")) {
		token.ty = MODIFIER;
	} else if (!strcmp(token.str, "int") ||
		!strcmp(token.str, "short") ||
		!strcmp(token.str, "long") ||
		!strcmp(token.str, "char") ||
		!strcmp(token.str, "float") ||
		!strcmp(token.str, "double") ||
		!strcmp(token.str, "void")) {
		token.ty = PRIMITIVE;
	} else if (!strcmp(token.str, "if") ||
		!strcmp(token.str, "for") ||
		!strcmp(token.str, "while") ||
		!strcmp(token.str, "do")) {
		token.ty = CONTROL_STRUCTURE;
	} else if (!strcmp(token.str, "struct") ||
		!strcmp(token.str, "enum") ||
		!strcmp(token.str, "union")) {
		token.ty = TYPE_TYPE;
	}
}

static void gettok_num(FILE * file)
{
	int ch = getc(file);
	int nb = 10;
	token.str[token.len++] = ch;
	
	if (ch == '0') {
		ch = getc(file);
		token.str[token.len++] = ch;
		switch (ch) {
		case 'x':
			nb = 16;
			break;
		case 'b':
			nb = 2;
			break;
		default:
			nb = 8;
			ungetc(ch, file);
			--token.len;
			break;
		}
	}
	
	do {
		ch = getc(file);
		token.str[token.len++] = ch;
	} while (gettok_hexc(ch));
	ungetc(ch, file);
	--token.len;
	if (nb == 8 && token.len == 1) {
		nb = 10; /* zero */
	}
	
	switch (nb) {
	case 2:
		token.ty = BIN_INT_LITERAL;
		break;
	case 8:
		token.ty = OCT_INT_LITERAL;
		break;
	case 10:
		token.ty = DEC_INT_LITERAL;
		break;
	case 16:
		token.ty = HEX_INT_LITERAL;
		break;
	}
}

static void gettok_aop(FILE * file)
{
	int ch = getc(file);
	token.ty = OPERATOR;
	token.str[token.len++] = ch;
	if (gettok_aopc(ch)) {
		ch = getc(file);
		if (ch != '=') {
			ungetc(ch, file);
		} else {
			token.str[token.len++] = ch;
		}
	}
}

static void gettok_opo(FILE * file)
{
	int ch = getc(file);
	token.ty = OPERATOID;
	token.str[token.len++] = ch;
}

struct tok token = { 0 };

void gettok(FILE * file)
{
	int ch;
	/* skip spaces, newlines and other guff */
	while ((ch = getc(file)) <= 0x20) {
		if (ch == EOF) {
			token.ty = STOP;
			return;
		}
	}
	ungetc(ch, file);
	
	memset(token.str, 0, token.len + 1);
	token.len = 0;
	
	if (gettok_idc(ch)) {
		gettok_id(file);
		return;
	}
	
	if (gettok_numc(ch)) {
		gettok_num(file);
		return;
	}
	
	if (ch == ';') {
		token.ty = SEMICOLON;
		token.str[0] = getc(file);
		token.len = 1;
		return;
	}
	
	if (gettok_aopc(ch)) {
		gettok_aop(file);
		return;
	}
	
	if (gettok_opc(ch)) {
		gettok_opo(file);
		return;
	}
	
	fprintf(stderr, "error: internal tokenisation error\n");
}
