#include <options.h>
#include <codegen.h>
#include <parsing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const char * _fname;
int _linenum;
int _column;

void showerror(FILE * f, const char * type, const char * frmt, ...)
{
	va_list ap;
	va_start(ap, frmt);
	fprintf(f, "%s:%d:%d: %s: ", _fname, _linenum, _column, type);
	vfprintf(f, frmt, ap);
	fprintf(f, "\n");
	fflush(f);
	va_end(ap);
}

void parse(const char * filename)
{
	FILE * ifile, * ofile;
	size_t flen;
	char ofname[64] = { 0 };
	
	flen = strlen(filename);
	strcat(ofname, filename);
	if (ofname[flen - 1] == 'c' &&
	    ofname[flen - 2] == '.') {
		
		ofname[flen - 2] = '\0';
	}
	strcat(ofname, ".asm");
	
	ifile = fopen(filename, "rb");
	ofile = fopen(ofname, "wb");
	
	_fname = filename;
	_linenum = 1;
	_column = -1;
	
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
	case '[':
	case ']':
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
		++_column;
		token.str[token.len++] = ch;
	}
	ungetc(ch, file);
	token.str[token.len] = '\0';
	
	if (!strcmp(token.str, "const") ||
		!strcmp(token.str, "unsigned") ||
		!strcmp(token.str, "signed") ||
		!strcmp(token.str, "static") ||
		!strcmp(token.str, "far")) {
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
	} else if (!strcmp(token.str, "sizeof")) {
		token.ty = OPERATOR;
	}
}

static void gettok_num(FILE * file)
{
	int ch = getc(file);
	int nb = 10;
	
	++_column;
	token.str[token.len++] = ch;
	
	if (ch == '0') {
		ch = getc(file);
		++_column;
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
			--_column;
			break;
		}
	}
	
	do {
		ch = getc(file);
		++_column;
		token.str[token.len++] = ch;
	} while (gettok_hexc(ch));
	ungetc(ch, file);
	--token.len;
	--_column;
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
	int chpr = 0;
	int ch = getc(file);
	token.ty = OPERATOR;
	++_column;
	token.str[token.len++] = ch;
	if (gettok_aopc(ch)) {
		ch = getc(file);
		if (token.str[0] == '+' && ch == '+') {
			++_column;
			token.str[token.len++] = ch;
			return;
		}
		if (token.str[0] == '-' && ch == '-') {
			++_column;
			token.str[token.len++] = ch;
			return;
		}
		if (token.str[0] == '/' && ch == '*') {
			while (chpr != '*' || ch != '/') {
				chpr = ch;
				ch = getc(file);
				++_column;
				if (ch == '\n') {
					++_linenum;
					_column = 0;
				}
			}
			gettok(file);
			return;
		}
		
		if (ch != '=') {
			ungetc(ch, file);
		} else {
			++_column;
			token.str[token.len++] = ch;
		}
	}
}

static void gettok_opo(FILE * file)
{
	int ch = getc(file);
	token.ty = OPERATOID;
	++_column;
	token.str[token.len++] = ch;
}

struct tok token = { 0 };

void gettok(FILE * file)
{
	int ch;
	/* skip spaces, newlines and other guff */
	while ((ch = getc(file)) <= 0x20) {
		++_column;
		if (ch == '\n') {
			++_linenum;
			_column = 0;
		}
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
		++_column;
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
	
	showerror(stderr, "error", "internal tokenisation error");
}
