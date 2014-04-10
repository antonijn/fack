#include <options.h>
#include <codegen.h>
#include <parsing.h>

#include <stdio.h>
#include <stdlib.h>

void parse(const char * filename)
{
	FILE * file = fopen(filename, "rb");
	struct tok token;
	
	while (fparse_element(file))
		;
	
	fclose(file);
}

static int gettok_idc(int ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

static int gettok_idcc(int ch)
{
	return gettok_idc(ch) || (ch >= '0' && ch <= '9');
}

static void gettok_id(FILE * file, struct tok * token)
{
	int ch;
	
	token->ty = IDENTIFIER;
	while ((ch = getc(file)) != EOF && gettok_idcc(ch)) {
		token->str[token->len++] = ch;
	}
	
	if (!strcmp(token->str, "const") ||
		!strcmp(token->str, "unsigned") ||
		!strcmp(token->str, "signed") ||
		!strcmp(token->str, "static")) {
		token->ty = MODIFIER;
	} else if (!strcmp(token->str, "int") ||
		!strcmp(token->str, "short") ||
		!strcmp(token->str, "long") ||
		!strcmp(token->str, "char") ||
		!strcmp(token->str, "float") ||
		!strcmp(token->str, "double") ||
		!strcmp(token->str, "void")) {
		token->ty = PRIMITIVE;
	} else if (!strcmp(token->str, "if") ||
		!strcmp(token->str, "for") ||
		!strcmp(token->str, "while") ||
		!strcmp(token->str, "do")) {
		token->ty = CONTROL_STRUCTURE;
	} else if (!strcmp(token->str, "struct") ||
		!strcmp(token->str, "enum") ||
		!strcmp(token->str, "union")) {
		token->ty = TYPE_TYPE;
	}
}

void gettok(FILE * file, struct tok * token)
{
	int ch = getc(file);
	ungetc(ch, file);
	if (ch == EOF) {
		token->ty = STOP;
		return;
	}
	
	token->len = 0;
	
	if (gettok_idc(ch)) {
		gettok_id(file, token);
		return;
	}
}
