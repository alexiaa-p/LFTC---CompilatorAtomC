#pragma once

enum{
	ID,
	// keywords
	TYPE_CHAR, TYPE_DOUBLE, ELSE, IF, TYPE_INT, RETURN, STRUCT, VOID, WHILE,
	// delimiters
	SEMICOLON, COMMA, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC, END,
	// constants
	INT, DOUBLE, CHAR, STRING,
	// operators
	ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ
	};

typedef struct Token{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union{
		char *text;		// the chars for ID, STRING (dynamically allocated)
		int i;		// the value for INT
		char c;		// the value for CHAR
		double d;		// the value for DOUBLE
		};
	struct Token *next;		// next token in a simple linked list
	}Token;

int isValidEscapeChar(char c);
char parseEscape(char escape);	
char parseChar(const char* text);
char* processEscapes(const char* text);
Token *tokenize(const char *pch);
void showTokens(const Token *tokens);
void writeTokensToFile(Token* tokens, const char* filename);