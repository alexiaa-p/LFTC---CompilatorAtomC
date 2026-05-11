
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utils.h"
#include "lexer.h"

void err(const char *fmt,...){
	fprintf(stderr,"error: ");
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

void *safeAlloc(size_t nBytes){
	void *p=malloc(nBytes);
	if(!p)err("not enough memory");
	return p;
	}

char *loadFile(const char *fileName){
	FILE *fis=fopen(fileName,"rb");
	if(!fis)err("unable to open %s",fileName);
	fseek(fis,0,SEEK_END);
	size_t n=(size_t)ftell(fis);
	fseek(fis,0,SEEK_SET);
	char *buf=(char*)safeAlloc((size_t)n+1);
	size_t nRead=fread(buf,sizeof(char),(size_t)n,fis);
	fclose(fis);
	if(n!=nRead)err("cannot read all the content of %s",fileName);
	buf[n]='\0';
	return buf;
	}

const char* getTokenName(int code) {
    switch (code) {
        case ID: return "ID";
        case TYPE_CHAR: return "TYPE_CHAR";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case ELSE: return "ELSE";
        case IF: return "IF";
        case TYPE_INT: return "TYPE_INT";
        case RETURN: return "RETURN";
        case STRUCT: return "STRUCT";
        case VOID: return "VOID";
        case WHILE: return "WHILE";
        case SEMICOLON: return "SEMICOLON";
        case COMMA: return "COMMA";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case END: return "END";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case DOT: return "DOT";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        default: return "UNKNOWN";
    }
}
