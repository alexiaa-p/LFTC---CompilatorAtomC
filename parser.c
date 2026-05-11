#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;        // iteratorul in lista de atomi
Token *consumedTk; // ultimul atom consumat

void tkerr(const char *fmt, ...) {
    fprintf(stderr, "error in line %d: ", iTk->line);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

bool consume(int code) {
    if (iTk->code == code) {
        consumedTk = iTk;
        iTk = iTk->next;
        return true;
    }
    return false;
}

// Forward declarations
bool expr();
bool stm();
bool stmCompound();



// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase() {
    Token *start = iTk;

    if (consume(TYPE_INT))    return true;
    if (consume(TYPE_DOUBLE)) return true;
    if (consume(TYPE_CHAR))   return true;

    if (consume(STRUCT)) {
        if (consume(ID)) return true;
        tkerr("lipseste numele structurii dupa struct");
    }

    iTk = start;
    return false;
}



// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl() {
    Token *start = iTk;

    if (consume(LBRACKET)) {
        consume(INT); // optional — ex: char s[]
        if (consume(RBRACKET)) return true;
        tkerr("lipseste ] in declaratia de array");
    }

    iTk = start;
    return false;
}



// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef() {
    Token *start = iTk;

    if (!typeBase()) {
        iTk = start;
        return false;
    }

    if (!consume(ID)) {
        iTk = start;
        return false;
    }

    arrayDecl(); // optional

    if (!consume(SEMICOLON))
        tkerr("lipseste ; dupa definitia variabilei");

    return true;
}


// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef() {
    Token *start = iTk;

    if (!consume(STRUCT)) return false;

    if (!consume(ID)) {
        iTk = start;
        return false;
    }

    if (!consume(LACC)) {
        iTk = start;  // ← return false, nu tkerr!
        return false; // poate fi varDef de tip struct
    }

    while (varDef()) {}

    if (!consume(RACC))
        tkerr("lipseste } in definitia structurii");

    if (!consume(SEMICOLON))
        tkerr("lipseste ; dupa definitia structurii");

    return true;
}


// fnParam: typeBase ID arrayDecl?
bool fnParam() {
    Token *start = iTk;

    if (!typeBase()) {
        iTk = start;
        return false;
    }

    if (!consume(ID)) {
        iTk = start;
        return false;
    }

    arrayDecl(); // optional

    return true;
}



// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef() {
    Token *start = iTk;

    if (!typeBase()) {
        if (!consume(VOID)) {
            iTk = start;
            return false;
        }
    }

    if (!consume(ID)) {
        iTk = start;
        return false;
    }

    if (!consume(LPAR)) {
        // Poate fi varDef, nu fnDef — refacem tot
        iTk = start;
        return false;
    }

    if (fnParam()) {
        while (consume(COMMA)) {
            if (!fnParam())
                tkerr("parametru invalid dupa ,");
        }
    }

    if (!consume(RPAR))
        tkerr("lipseste ) in definitia functiei");

    if (!stmCompound())
        tkerr("lipseste corpul { } al functiei");

    return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//            | INT | DOUBLE | CHAR | STRING
//            | LPAR expr RPAR
bool exprPrimary() {
    Token *start = iTk;

    if (consume(ID)) {
        if (consume(LPAR)) {
            if (expr()) {
                while (consume(COMMA)) {
                    if (!expr())
                        tkerr("expresie invalida dupa , in apelul de functie");
                }
            }
            if (!consume(RPAR))
                tkerr("lipseste ) in apelul de functie");
        }
        return true;
    }

    if (consume(INT))    return true;
    if (consume(DOUBLE)) return true;
    if (consume(CHAR))   return true;
    if (consume(STRING)) return true;

    if (consume(LPAR)) {
        if (expr()) {
            if (!consume(RPAR))
                tkerr("lipseste ) in expresie");
            return true;
        }
        iTk = start;  // ← backtrack, lasa exprCast sa incerce
        return false;
    }

    iTk = start;
    return false;
}
// exprPostfix (recursivitate stanga eliminata):
// exprPostfix:    exprPrimary exprPostfixPrim
// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | ε
bool exprPostfixPrim() {
    if (consume(LBRACKET)) {
        if (!expr())
            tkerr("expresie invalida in indexare");
        if (!consume(RBRACKET))
            tkerr("lipseste ] in indexare");
        exprPostfixPrim();
        return true;
    }
    if (consume(DOT)) {
        if (!consume(ID))
            tkerr("lipseste numele campului dupa .");
        exprPostfixPrim();
        return true;
    }
    return true; // epsilon
}

bool exprPostfix() {
    Token *start = iTk;
    if (!exprPrimary()) {
        iTk = start;
        return false;
    }
    exprPostfixPrim();
    return true;
}



// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary() {
    if (consume(SUB) || consume(NOT)) {
        if (!exprUnary())
            tkerr("expresie invalida dupa operator unar");
        return true;
    }
    return exprPostfix();
}



// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast() {
    Token *start = iTk;

    if (consume(LPAR)) {
        if (typeBase()) {
            arrayDecl();
            if (consume(RPAR)) {
                if (!exprCast())
                    tkerr("expresie invalida dupa cast");
                return true;
            }
        }
        iTk = start;  // nu e cast
    }

    return exprUnary();  // exprUnary → exprPostfix → exprPrimary va incerca (expr)
}


// exprMul: exprCast ( ( MUL | DIV ) exprCast )*
bool exprMul() {
    Token *start = iTk;
    if (!exprCast()) { iTk = start; return false; }
    while (consume(MUL) || consume(DIV)) {
        if (!exprCast())
            tkerr("expresie invalida dupa * sau /");
    }
    return true;
}


// exprAdd: exprMul ( ( ADD | SUB ) exprMul )*
bool exprAdd() {
    Token *start = iTk;
    if (!exprMul()) { iTk = start; return false; }
    while (consume(ADD) || consume(SUB)) {
        if (!exprMul())
            tkerr("expresie invalida dupa + sau -");
    }
    return true;
}


// exprRel: exprAdd ( ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd )*
bool exprRel() {
    Token *start = iTk;
    if (!exprAdd()) { iTk = start; return false; }
    while (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        if (!exprAdd())
            tkerr("expresie invalida dupa operator relational");
    }
    return true;
}


// exprEq: exprRel ( ( EQUAL | NOTEQ ) exprRel )*
bool exprEq() {
    Token *start = iTk;
    if (!exprRel()) { iTk = start; return false; }
    while (consume(EQUAL) || consume(NOTEQ)) {
        if (!exprRel())
            tkerr("expresie invalida dupa == sau !=");
    }
    return true;
}


// exprAnd: exprEq ( AND exprEq )*
bool exprAnd() {
    Token *start = iTk;
    if (!exprEq()) { iTk = start; return false; }
    while (consume(AND)) {
        if (!exprEq())
            tkerr("expresie invalida dupa &&");
    }
    return true;
}


// exprOr: exprAnd ( OR exprAnd )*
bool exprOr() {
    Token *start = iTk;
    if (!exprAnd()) { iTk = start; return false; }
    while (consume(OR)) {
        if (!exprAnd())
            tkerr("expresie invalida dupa ||");
    }
    return true;
}


// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign() {
    Token *start = iTk;

    // Salvăm pozitia, incercam exprUnary ASSIGN exprAssign
    Token *startAssign = iTk;
    if (exprUnary()) {
        if (consume(ASSIGN)) {
            if (!exprAssign())
                tkerr("expresie invalida dupa =");
            return true;
        }
    }
    // Nu a fost assignment — resetam si incercam exprOr de la inceput
    iTk = startAssign;
    return exprOr();
}


// expr: exprAssign
bool expr() {
    return exprAssign();
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound() {
    Token *start = iTk;

    if (!consume(LACC)) { iTk = start; return false; }

    for (;;) {
        if (varDef()) continue;
        if (stm())    continue;
        break;
    }

    if (!consume(RACC))
        tkerr("lipseste } la sfarsitul blocului");

    return true;
}

// stm: stmCompound
//    | IF LPAR expr RPAR stm ( ELSE stm )?
//    | WHILE LPAR expr RPAR stm
//    | RETURN expr? SEMICOLON
//    | expr? SEMICOLON
bool stm() {
    Token *start = iTk;

    if (stmCompound()) return true;

    if (consume(IF)) {
        if (!consume(LPAR))
            tkerr("lipseste ( dupa if");
        if (!expr())
            tkerr("conditie invalida in if");
        if (!consume(RPAR))
            tkerr("lipseste ) dupa conditia if");
        if (!stm())
            tkerr("lipseste instructiunea din if");
        if (consume(ELSE)) {
            if (!stm())
                tkerr("lipseste instructiunea din else");
        }
        return true;
    }

    if (consume(WHILE)) {
        if (!consume(LPAR))
            tkerr("lipseste ( dupa while");
        if (!expr())
            tkerr("conditie invalida in while");
        if (!consume(RPAR))
            tkerr("lipseste ) dupa conditia while");
        if (!stm())
            tkerr("lipseste instructiunea din while");
        return true;
    }

    if (consume(RETURN)) {
        expr(); // optional
        if (!consume(SEMICOLON))
            tkerr("lipseste ; dupa return");
        return true;
    }

    // expr? SEMICOLON
    expr(); // optional
    if (consume(SEMICOLON)) return true;

    iTk = start;
    return false;
}


// unit: ( structDef | fnDef | varDef )* END
bool unit() {
    for (;;) {
        if (structDef()) continue;
        if (fnDef())     continue;
        if (varDef())    continue;
        break;
    }
    if (!consume(END))
        tkerr("eroare de sintaxa");
    return true;
}

void parse(Token *tokens) {
    iTk = tokens;
   //for(Token *t = tokens; t; t = t->next)
        //printf("linia %d: cod %d\n", t->line, t->code);
    unit();
}