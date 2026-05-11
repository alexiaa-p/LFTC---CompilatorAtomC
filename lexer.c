#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;
Token *lastTk;
int line = 1;

Token *addTk(int code) {
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk) {
		lastTk->next = tk;
	} else {
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}

char* extract(const char* begin, const char* end) {
	int len = end - begin;
	char* text = (char*)safeAlloc(len + 1);
	memcpy(text, begin, len);
	text[len] = '\0';
	return text;
}

int checkKeyword(const char* text) {
	if (strcmp(text, "char") == 0) return TYPE_CHAR;
	if (strcmp(text, "double") == 0) return TYPE_DOUBLE;
	if (strcmp(text, "else") == 0) return ELSE;
	if (strcmp(text, "if") == 0) return IF;
	if (strcmp(text, "int") == 0) return TYPE_INT;
	if (strcmp(text, "return") == 0) return RETURN;
	if (strcmp(text, "struct") == 0) return STRUCT;
	if (strcmp(text, "void") == 0) return VOID;
	if (strcmp(text, "while") == 0) return WHILE;
	return ID;
}

//aici am adaugat functii pentru procesarea secventelor de escape din char si string, precum si validarea caracterelor de escape
//pentru char, daca incepe cu \, trebuie sa fie urmat de un caracter valid de escape, altfel e eroare
//pentru string, la fel, dar pot fi secvente de escape multiple in interiorul stringului
//functia parseEscape returneaza caracterul corespunzator secventei de escape, sau -1 daca e invalid
//functia parseChar primeste textul din interiorul apostrofului (fara apostrofi) si returneaza caracterul corespunzator, procesand escape daca e cazul
//functia processEscapes primeste textul din interiorul ghilimelelor (fara ghilimele) si returneaza un nou string cu toate secventele de escape procesate
char parseEscape(char escape) {
	switch (escape) {
	case 'a': return '\a';
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	case '\\': return '\\';
	case '\'': return '\'';
	case '"': return '"';
	case '0': return '\0';
	default: return -1;	// invalid escape
	}
}

char parseChar(const char* text) {
	if (text[0] == '\\') {
		char result = parseEscape(text[1]);
		if (result == -1) err("Invalid escape sequence: \\%c at line %d", text[1], line);
		return result;
	}
	return text[0];
}

char* processEscapes(const char* text) {
	int len = strlen(text);
	char* result = (char*)malloc(len + 1);
	if (result == NULL) {
    err("Eroare: Out of memory la linia %d", line);
}
	int j = 0;
	for (int i = 0; i < len; i++) {
		if (text[i] == '\\' && i + 1 < len) {
			char escape = parseEscape(text[++i]);
			if (escape == -1) err("Invalid escape sequence: \\%c at line %d", text[i], line);
			result[j++] = escape;
		} else {
			result[j++] = text[i];
		}
	}
	result[j] = '\0';
	return result;
}

//daca caracterul dupa \ este valid pentru escape
int isValidEscapeChar(char c) {
	return c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' 
		|| c == 't' || c == 'v' || c == '\\' || c == '\'' || c == '"' || c == '0';
}

Token* tokenize(const char* pch) {
	const char* start;
	Token* tk;
	for (;;) {
		switch (*pch) {
		case ' ':
		case '\t':
			pch++;
			break;
		case '\r':
			if (pch[1] == '\n') pch++;
		case '\n':
			line++;
			pch++;
			break;
		case '\0':
			addTk(END);
			return tokens;
		case '/':
			if (pch[1] == '/') {  //LINECOMMENT
				pch += 2;
				while (*pch && *pch != '\n' && *pch != '\r') pch++;
			}
			else if (pch[1] == '*') {  //BLOCKCOMMENT
				pch += 2;
				while (*pch && !(pch[0] == '*' && pch[1] == '/')) {
					if (*pch == '\n') line++;
					pch++;
				}
				if (*pch == '\0') err("Unclosed block comment at line %d", line);
				pch += 2;
			}
			else {
				addTk(DIV);
				pch++;
			}
			break;

		// DELIMITATORI
		case ',': addTk(COMMA); pch++; break;
		case ';': addTk(SEMICOLON); pch++; break;
		case '{': addTk(LACC); pch++; break;
		case '}': addTk(RACC); pch++; break;
		case '(': addTk(LPAR); pch++; break;
		case ')': addTk(RPAR); pch++; break;
		case '[': addTk(LBRACKET); pch++; break;
		case ']': addTk(RBRACKET); pch++; break;

		// OPERATORI
		case '+': addTk(ADD); pch++; break;
		case '-': addTk(SUB); pch++; break;
		case '*': addTk(MUL); pch++; break;
		case '=':
			if (pch[1] == '=') {
				addTk(EQUAL);
				pch += 2;
			} else {
				addTk(ASSIGN);
				pch++;
			}
			break;
		case '!':
			if (pch[1] == '=') {
				addTk(NOTEQ);
				pch += 2;
			} else {
				addTk(NOT);
				pch++;
			}
			break;
		case '<':
			if (pch[1] == '=') {
				addTk(LESSEQ);
				pch += 2;
			} else {
				addTk(LESS);
				pch++;
			}
			break;
		case '>':
			if (pch[1] == '=') {
				addTk(GREATEREQ);
				pch += 2;
			} else {
				addTk(GREATER);
				pch++;
			}
			break;
		case '&':
			if (pch[1] == '&') {
				addTk(AND);
				pch += 2;
			} else {
				err("Invalid & la linia %d, trebuia &&", line);
			}
			break;
		case '|':
			if (pch[1] == '|') {
				addTk(OR);
				pch += 2;
			} else {
				err("Invalid | la linia %d, trebuia ||", line);
			}
			break;
		case '.':
			if (isdigit(pch[1])) {
				//nr ca .5 sau .5e10
				//mergi de la punct cat timp ce e cifra, apoi optional e/E, apoi optional semn, apoi obligatoriu cifra
				//5.05 sau 5. sau .5 sau 5e10 sau 5.05e-2 sau .5e+3 sau .
				start = pch;
				pch++;
				while (isdigit(*pch)) pch++;
				if (*pch == 'e' || *pch == 'E') {
					pch++;
					if (*pch == '+' || *pch == '-') pch++;
					if (!isdigit(*pch)) err("Expected digits after exponent at line %d", line);
					while (isdigit(*pch)) pch++;
				}
				char* numText = extract(start, pch);
				tk = addTk(DOUBLE);
				tk->d = atof(numText);
				free(numText);
			} else {
				addTk(DOT);
				pch++;
			}
			break;

		default:
			//IDENTIFICATORI
			//ori nume d evariabila, functie, structura (ID), ori cuvant cheie (TYPE_CHAR, IF, WHILE, etc)
			if (isalpha(*pch) || *pch == '_') {
				start = pch;
				pch++;
				while (isalnum(*pch) || *pch == '_') pch++;
				char* text = extract(start, pch);
				int code = checkKeyword(text);   //verif daca e cuvant cheie
				tk = addTk(code);
				if (code == ID) tk->text = text;
				else free(text);
			}
			//numere constante: int sau double
			else if (isdigit(*pch)) {
				start = pch;
				while (isdigit(*pch)) pch++;
				
				int isDouble = 0;
				
				//Cazul: 123.456 sau 123.
				if (*pch == '.') {
					isDouble = 1;
					pch++;  //if isdigit daca nu e cifra eroare daca e ok ma duc
					while (isdigit(*pch)) pch++;
				}
				
				//Cazul: 123e5 sau 123.456e-2, exponent cu e/E
				if (*pch == 'e' || *pch == 'E') {
					isDouble = 1;
					pch++;
					if (*pch == '+' || *pch == '-') pch++;
					if (!isdigit(*pch)) err("Expected digits after exponent at line %d", line);
					while (isdigit(*pch)) pch++;
				}
				
				char* numText = extract(start, pch);
				if (isDouble) {
					tk = addTk(DOUBLE);
					tk->d = atof(numText);
				} else {
					tk = addTk(INT);
					tk->i = atoi(numText);
				}
				free(numText);
			}
			// CHAR CONSTANT: ['] ( [^'\\] | [\\] [abfnrtv\\'"0] ) [']
			else if (*pch == '\'') {
				pch++;  // skip opening '
				start = pch;
				
				//Trebuie exact un caracter: fie normal, fie escape
				if (*pch == '\\') {
					//Escape sequence
					pch++;
					if (!isValidEscapeChar(*pch)) {
						err("Invalid escape sequence '\\%c' in CHAR at line %d", *pch, line);
					}
					pch++;
				} else if (*pch == '\'' || *pch == '\0' || *pch == '\n' || *pch == '\r') {
					err("Empty or unclosed CHAR constant at line %d", line);
				} else {
					//caracter normal (nu apostrof, nu escape, nu newline)
					pch++;
				}
				
				if (*pch != '\'') {
					err("Missing closing apostrophe for CHAR at line %d", line);
				}
				
				char* charText = extract(start, pch);
				tk = addTk(CHAR);
				tk->c = parseChar(charText);
				free(charText);
				pch++;
			}
			// STRING CONSTANT: ["] ( [^"\\] | [\\] [abfnrtv\\'"0] )* ["]
			else if (*pch == '"') {
				pch++;  // skip opening "
				start = pch;
				
				while (*pch != '"' && *pch != '\0') {
					if (*pch == '\n' || *pch == '\r') {
						err("Newline not allowed in STRING constant at line %d", line);
					}
					if (*pch == '\\') {
						pch++;
						if (!isValidEscapeChar(*pch)) {
							err("Invalid escape sequence '\\%c' in STRING at line %d", *pch, line);
						}
						pch++;
					} else {
						pch++;
					}
				}
				
				if (*pch != '"') {
					err("Missing closing quote for STRING at line %d", line);
				}
				
				char* strText = extract(start, pch);
				tk = addTk(STRING);
				tk->text = processEscapes(strText);
				free(strText);
				pch++;
			}
			else {
				err("Invalid character '%c' (ASCII %d) at line %d", *pch, (unsigned char)*pch, line);
			}
		}
	}
}

//fct de afisare tokenuri din lista, bazate pe codul lor
void showTokens(const Token* tokens) {
	for (const Token* tk = tokens; tk; tk = tk->next) {
		printf("%d\n", tk->code);
	}
}


void writeTokensToFile(Token* tokens, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Eroare la crearea fisierului de output");
        return;
    }

    //parcurgem lista inlatuita de tokeni
    Token* curr = tokens;
    while (curr != NULL) {
        fprintf(f, "%d\t", curr->line); 
        
        switch (curr->code) {
            case ID:
            case STRING:
                //text salvat in curr->text
                fprintf(f, "%s:%s\n", getTokenName(curr->code), curr->text);
                break;
            case INT:
                //valoarea salvata in curr->i
                fprintf(f, "%s:%d\n", getTokenName(curr->code), curr->i);
                break;
            case DOUBLE:
                //valoarea salvata in curr->d
                fprintf(f, "%s:%.2f\n", getTokenName(curr->code), curr->d); 
                break;
            case CHAR:
                //valoarea salvata in curr->c
                fprintf(f, "%s:%c\n", getTokenName(curr->code), curr->c);
                break;
            default:
                //pt restul (keywords, delimiters, operators) afisam doar numele
                fprintf(f, "%s\n", getTokenName(curr->code));
                break;
        }
        
        curr = curr->next;
    }
    
    fclose(f);
}

//VEZI CAZU DE EROARE DACA AI AL DOUBLE ALA CU 2.2 SAU .2 SAU 2. SI \R DACA ARE CEVA REVERIFICA 