#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main() {
	FILE* f = fopen("tests/testparser.c", "r");
	if (!f) {
		perror("Nu pot deschide testparser.c");
		return 1;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* code = malloc(size + 1);
	if (!code) {
		fclose(f);
		return 1;
	}

	fread(code, 1, size, f);
	code[size] = '\0';
	fclose(f);

	//lexer
	Token* toks = tokenize(code);

	writeTokensToFile(toks, "token_output.txt");


	for(Token *t = toks; t; t = t->next){
        if(t->code == ID)
            printf("linia %d: cod %d text=%s\n", t->line, t->code, t->text);
        else
            printf("linia %d: cod %d\n", t->line, t->code);
    }

	//parser
	parse(toks);
	printf("Program corect sintactic\n");

	free(code);
	return 0;
}