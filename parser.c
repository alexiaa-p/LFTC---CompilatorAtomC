#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "utils.h"
#include "ad.h"

Token* itk;        // iteratorul in lista de atomi
Token* consumedtk; // ultimul atom consumat
Symbol *owner = NULL; // simbolul domeniului curent (NULL pentru domeniul global)

void tkerr(const char* fmt, ...) {
    fprintf(stderr, "error in line %d: ", itk->line);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

bool consume(int code) {
    if (itk->code == code) {
        consumedtk = itk;
        itk = itk->next;
        return true;
    }
    return false;
}

// forward declarations
bool expr();
bool stm();
bool stmcompound(bool newDomain);

// typebase: type_int | type_double | type_char | struct id
// MODIFICARE ANALIZA DOMENIU
// daca tipul de baza este o structura, ea trebuie sa fie deja definita anterior
bool typebase(Type *t) {
    t->n = -1; // nu este vector
    Token *start = itk;
    if (consume(TYPE_INT)) 
    { 
        t->tb = TB_INT; 
          return true;
    }
    if (consume(TYPE_DOUBLE)) 
    {
        t->tb = TB_DOUBLE; 
        return true;
    }
    if (consume(TYPE_CHAR))  
    {
        t->tb = TB_CHAR;
        return true;
    }
    if (consume(STRUCT)) {
        if (consume(ID)) 
        {
            Token *tkName = consumedtk;
            t->tb = TB_STRUCT;
            t->s  = findSymbol(tkName->text);
            if (!t->s)              
              tkerr("structure %s is not defined", tkName->text);
            return true;
        }
        else tkerr("expected struct name");
    }
    itk = start;
    return false;
}

// arraydecl: lbracket int? rbracket
// MODIFICARE ANALIZA DOMENIU
bool arrayDecl(Type *t){
    Token *start = itk;
    if(consume(LBRACKET)){
        if(consume(INT)){
            Token *tkSize = consumedtk;
            t->n = tkSize->i;  
        }else{
            t->n = 0; //array fara dimensiune specificata
        }
        if(consume(RBRACKET))
            return true;
        else
            tkerr("expected ']' after array declaration or invalid expression in array declaration");
    }
    itk = start;
    return false;
}

// vardef: typebase id arraydecl? semicolon
// MODIFICARE ANALIZA DOMENIU
// numele variabilei trebuie sa fie unic in domeniu
// variabilele de tip vector trebuie sa aiba dimensiunea data (nu se accepta: int v[])
bool vardef() {
    Token* start = itk;
    Type t;
    if (typebase(&t)) {
        if (consume(ID)) {
            Token *tkName = consumedtk;
            if (arrayDecl(&t)) {
                if(t.n == 0)
                    tkerr("a vector variable must have a specified dimension");
            }
            if (consume(SEMICOLON))
            {
                Symbol *var = findSymbolInDomain(symTable, tkName->text);
                if (var)
                    tkerr("symbol redefinition: %s", tkName->text);
                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;
                addSymbolToDomain(symTable, var);
                if(owner)
                {
                    switch(owner->kind)
                    {
                        case SK_FN:
                            var->varIdx = symbolsLen(owner->fn.locals);
                            addSymbolToList(&owner->fn.locals, dupSymbol(var));
                            break;
                        case SK_STRUCT:
                            var->varIdx = typeSize(&owner->type);
                            addSymbolToList(&owner->structMembers, dupSymbol(var));
                            break;
                    }
                }
                else
                {
                    var->varMem = safeAlloc(typeSize(&t));
                }
                return true;
            }
            else
                tkerr("expected ';' after variable definition");
        }
        else
            tkerr("expected variable name or function identifier or missing { for struct definition");
    }
    return false;
}

// structdef: struct id lacc vardef* racc semicolon
//MODIFICARE ANALIZA DOMENIU
// numele structurii trebuie sa fie unic in domeniu
// in interiorul structurii nu pot exista doua variabile cu acelasi nume
bool structdef() {
    Token* start = itk;
    if (consume(STRUCT)) {
        if (consume(ID)) {
            Token *tkName = consumedtk;
            if (consume(LACC)) {
                Symbol *s=findSymbolInDomain(symTable,tkName->text);
                if(s)
                    tkerr("symbol redefinition: %s", tkName->text);
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.tb=TB_STRUCT;
                s->type.s=s;
                s->type.n=-1;
                pushDomain();
                owner = s;
                
                for (;;) {
                    if (vardef());
                    else break;
                }
                if (consume(RACC)) {
                    if (consume(SEMICOLON))
                    {
                        owner = NULL;
                        dropDomain();
                        return true;
                    }
                    else
                        tkerr("expected ';' after struct definition");
                }
                else
                    tkerr("expected '}' after struct definition or invalid expression in struct definition");
            }
        }
    }
    itk = start;
    return false;
}

// fnparam: typebase id arraydecl?
// numele parametrului trebuie sa fie unic in domeniu
// parametrii pot fi vectori cu dimensiune data, dar in acest caz li se sterge
//dimensiunea ( int v[10] -> int v[] )

bool fnparam() {
    Token *start = itk;
    Type t;
    if (typebase(&t)) {
        if (consume(ID)) {
                Token *tkName = consumedtk;
            if (arrayDecl(&t)) {
                t.n = 0;
            }
            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if (param)
                tkerr("symbol redefinition: %s", tkName->text);
            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx = symbolsLen(owner->fn.params);
            // parametrul este adaugat atat la domeniul curent, cat si la parametrii fn
            addSymbolToDomain(symTable, param);
            addSymbolToList(&owner->fn.params, dupSymbol(param));
            return true;
        }
        else
            tkerr("expected parameter name");
    }
    itk = start;
    return false;
}

// fndef: ( typebase | void ) id lpar ( fnparam ( comma fnparam )* )? rpar stmcompound
// MODIFICARE ANALIZA DOMENIU
// numele functiei trebuie sa fie unic in domeniu
// domeniul local functiei incepe imediat dupa LPAR
// corpul functiei {...} nu defineste un nou subdomeniu in domeniul local functiei
bool fndef() {
    Token* start = itk;
    Type t;
    bool consumedVoid = false;
    if (typebase(&t) || (consumedVoid = consume(VOID))) {
        if (consumedVoid)
            t.tb = TB_VOID;
        if (consume(ID)) {
            Token *tkName = consumedtk;
            if (consume(LPAR)) {
                Symbol *fn = findSymbolInDomain(symTable, tkName->text);
                if (fn)
                    tkerr("symbol redefinition: %s", tkName->text);
                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);
                owner = fn;
                pushDomain();
                if (fnparam()) {
                    while (consume(COMMA)) {
                        if (!fnparam())
                            tkerr("missing or invalid parameter after ',' in function definition");
                    }
                }
                if (consume(RPAR)) {
                    if (stmcompound(false))
                    {
                        dropDomain();
                        owner = NULL;
                        return true;
                    }
                    else
                        tkerr("expected compound statement after function declaration or invalid expression in function definition");
                }
                else
                    tkerr("expected ')' after function parameters or invalid expression in function definition");
            }
        }
        // Daca nu gasim ID sau LPAR, facem doar backtrack
    }
    itk = start;
    return false;
}
// exprprimary: id ( lpar ( expr ( comma expr )* )? rpar )?
//            | int | double | char | string
//            | lpar expr rpar
bool exprprimary() {
    if (consume(ID)) {
        if (consume(LPAR)) {
            if (expr()) {
                while (consume(COMMA)) {
                    if (!expr())
                        tkerr("expected expression after ',' in function call");
                }
            }
            if (consume(RPAR)) {
                return true;
            }
            else {
                tkerr("expected ')' after function call arguments or invalid expression in function call");
            }
        }
        return true;
    }
    if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)) {
        return true;
    }
    if (consume(LPAR)) {
        if (expr()) {
            if (consume(RPAR)) {
                return true;
            }
            else {
                tkerr("expected ')' after expression or invalid expression in parentheses");
            }
        }
        else {
            tkerr("invalid or missing expression after '('");
        }
    }
    return false;
}

// exprpostfixprim: lbracket expr rbracket exprpostfixprim | dot id exprpostfixprim | ε
bool exprpostfixprim() {
    if (consume(LBRACKET)) {
        if (expr()) {
            if (consume(RBRACKET)) {
                if (exprpostfixprim()) {
                    return true;
                }
                else {
                    tkerr("invalid or missing expression after array access");
                }
            }
            else {
                tkerr("expected ']' after array access or invalid expression in array access");
            }
        }
        else {
            tkerr("invalid or missing expression after '[' in array access");
        }
    }
    else if (consume(DOT)) {
        if (consume(ID)) {
            if (exprpostfixprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after member access");
            }
        }
        else {
            tkerr("expected member name after '.' in member access");
        }
    }
    return true;
}

// exprpostfix: exprprimary exprpostfixprim
bool exprpostfix() {
    if (exprprimary()) {
        if (exprpostfixprim()) {
            return true;
        }
    }
    return false;
}

// exprunary: ( sub | not ) exprunary | exprpostfix
bool exprunary() {
    if (consume(SUB) || consume(NOT)) {
        if (exprunary()) {
            return true;
        }
        else {
            tkerr("invalid or missing expression after unary operator");
        }
    }
    else {
        if (exprpostfix()) {
            return true;
        }
    }
    return false;
}

// exprcast: lpar typebase arraydecl? rpar exprcast | exprunary
// MODIFICARE ANALIZA DOMENIU
// deoarece typeBase si arrayDecl au nevoie de un argument, se adauga acesta
// t va fi folosit ulterior

bool exprcast() {
    if (consume(LPAR)) {
        Type t;
        if (typebase(&t)) {
            if (arrayDecl(&t)) {}
            if (consume(RPAR)) {
                if (exprcast()) {
                    return true;
                }
                else {
                    tkerr("invalid or missing expression after cast");
                }
            }
            else {
                tkerr("expected ')' after cast type or invalid expression in cast");
            }
        }
        else {
            tkerr("expected type in cast");
        }
    }
    else {
        if (exprunary()) {
            return true;
        }
    }
    return false;
}

// exprmul: exprcast exprMulPrim
bool exprmulprim() {
    if (consume(MUL)) {
        if (exprcast()) {
            if (exprmulprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '*'");
            }
        }
        else {
            tkerr("invalid or missing expression after '*'");
        }
    }
    if (consume(DIV)) {
        if (exprcast()) {
            if (exprmulprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '/'");
            }
        }
        else {
            tkerr("invalid or missing expression after '/'");
        }
    }
    return true;
}
bool exprmul() {
    if (exprcast()) {
        if (exprmulprim()) {
            return true;
        }
    }
    return false;
}

// expradd: exprmul exprAddPrim
bool expraddprim() {
    if (consume(ADD)) {
        if (exprmul()) {
            if (expraddprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '+'");
            }
        }
        else {
            tkerr("invalid or missing expression after '+'");
        }
    }
    if (consume(SUB)) {
        if (exprmul()) {
            if (expraddprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '-'");
            }
        }
        else {
            tkerr("invalid or missing expression after '-'");
        }
    }
    return true;
}

bool expradd() {
    if (exprmul()) {
        if (expraddprim()) {
            return true;
        }
    }
    return false;
}

// exprrel: expradd exprRelPrim
bool exprRelprim() {
    if (consume(LESS)) {
        if (expradd()) {
            if (exprRelprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '<'");
            }
        }
        else {
            tkerr("invalid or missing expression after '<'");
        }
    }
    if (consume(LESSEQ)) {
        if (expradd()) {
            if (exprRelprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '<='");
            }
        }
        else {
            tkerr("invalid or missing expression after '<='");
        }
    }
    if (consume(GREATER)) {
        if (expradd()) {
            if (exprRelprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '>'");
            }
        }
        else {
            tkerr("invalid or missing expression after '>'");
        }
    }
    if (consume(GREATEREQ)) {
        if (expradd()) {
            if (exprRelprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '>='");
            }
        }
        else {
            tkerr("invalid or missing expression after '>='");
        }
    }
    return true;
}
bool exprrel() {
    if (expradd()) {
        if (exprRelprim()) {
            return true;
        }
    }
    return false;
}

// expreq: exprrel exprEqPrim
bool exprEqprim() {
    if (consume(EQUAL) || consume(NOTEQ)) {
        if (exprrel()) {
            if (exprEqprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '==' or '!='");
            }
        }
        else {
            tkerr("invalid or missing expression after '==' or '!='");
        }
    }
    return true;
}

bool expreq() {
    if (exprrel()) {
        if (exprEqprim()) {
            return true;
        }
    }
    return false;
}

// exprand: expreq exprAndPrim
bool exprandprim() {
    if (consume(AND)) {
        if (expreq()) {
            if (exprandprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '&&'");
            }
        }
        else {
            tkerr("invalid or missing expression after '&&'");
        }
    }
    return true;
}

bool exprand() {
    if (expreq()) {
        if (exprandprim()) {
            return true;
        }
    }
    return false;
}

// expror: exprand exprOrPrim
bool exprOrprim() {
    if (consume(OR)) {
        if (exprand()) {
            if (exprOrprim()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '||'");
            }
        }
        else {
            tkerr("invalid or missing expression after '||'");
        }
    }
    return true;
}

bool expror() {
    if (exprand()) {
        if (exprOrprim()) {
            return true;
        }
    }
    return false;
}

// exprassign: exprunary assign exprassign | expror
bool exprassign() {
    Token* start = itk;
    if (exprunary()) {
        if (consume(ASSIGN)) {
            if (exprassign()) {
                return true;
            }
            else {
                tkerr("invalid or missing expression after '='");
            }
        }
        itk = start;
    }
    if (expror()) {
        return true;
    }
    return false;
}

// expr: exprassign
bool expr() {
    if (exprassign()) {
        return true;
    }
    return false;
}

// stmcompound: lacc ( vardef | stm )* racc
// MODIFICARE ANALIZA DOMENIU
// se defineste un nou domeniu doar la cerere

bool stmcompound(bool newDomain) {
    Token* start = itk;
    if (consume(LACC)) {
        if(newDomain)
             pushDomain();
        for (;;) {
            if (vardef()) {}
            else if (stm()) {}
            else break;
        }
        if (consume(RACC)) {
            if(newDomain)
                dropDomain();
            return true;
        }
        else {
            tkerr("expected '}' after compound statement or invalid expression in compound statement");
        }
    }
    itk = start;
    return false;
}

// stm: stmcompound | if lpar expr rpar stm (else stm)?
//    | while lpar expr rpar stm | return expr? semicolon | expr? semicolon
// MODIFICARE ANALIZA DOMENIU
// corpul compus {...} al instructiunilor defineste un nou domeniu

bool stm() {
    if (stmcompound(true)) {
        return true;
    }
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            if (stm())
                                return true;
                            else
                                tkerr("expected statement after 'else' or invalid expression in statement");
                            return true;
                        }
                        return true;
                    }
                    else {
                        tkerr("expected statement after 'if' condition or invalid expression in statement");
                    }
                }
                else {
                    tkerr("expected ')' after 'if' condition or invalid expression in statement");
                }
            }
            else {
                tkerr("missing condition for 'if' statement");
            }
        }
        else {
            tkerr("expected '(' after 'if'");
        }
    }
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        return true;
                    }
                    else {
                        tkerr("missing statement after 'while' condition");
                    }
                }
                else {
                    tkerr("expected ')' after 'while' condition or invalid expression in statement");
                }
            }
            else {
                tkerr("missing condition for 'while' statement");
            }
        }
        else {
            tkerr("expected '(' after 'while'");
        }
    }
    if (consume(RETURN)) {
        if (expr()) {}
        if (consume(SEMICOLON)) {
            return true;
        }
        else {
            tkerr("expected ';' after return statement");
        }
    }
    if (expr()) {
        if (consume(SEMICOLON)) {
            return true;
        }
        else {
            tkerr("expected ';' after expression statement");
        }
    }
    if (consume(SEMICOLON)) {
        return true; // empty statement
    }
    return false;
}

// unit: ( structdef | fndef | vardef )* end
bool unit() {
    for (;;) {
        if (structdef()) {}
        else if (fndef()) {}
        else if (vardef()) {}
        else break;
    }
    if (consume(END))
        return true;
    return false;
}

void parse(Token* tokens) {
    itk = tokens;
    if (!unit())
        tkerr("syntax error");
}