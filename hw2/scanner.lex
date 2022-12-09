%{
#include <stdio.h>
#define YYSTYPE int
#include "parser.tab.hpp"
#include "output.hpp"
%}

%option yylineno
%option noyywrap

digit   		  ([1-9])
digit_zero   		  ([0-9])
letter  		  ([a-zA-Z])
whitespace	  ([\t\n\r ])

%%
not return NOT;
void return VOID;
int return INT;
byte return BYTE;
(b) return B;
bool return BOOL;
and return AND;
or return OR;
true return TRUE;
false return FALSE;
return return RETURN;
if return IF;
else return ELSE;
while return WHILE;
break return BREAK;
continue return CONTINUE;
(\;) return SC;
(\,) return COMMA;
(\() return LPAREN;
(\)) return RPAREN;
(\{) return LBRACE;
(\}) return RBRACE;
(\=) return ASSIGN;
(==|!=|<=|>=|<|>) return RELOP;
(\+|\-) return ADD;
(\*|\/) return MUL;
{letter}({letter}|{digit_zero})* return ID;
0|{digit}{digit_zero}* return NUM;
(\")([^(\n)(\r)(\")(\\)]|(\))|(\()|(\\)[rnt(\")(\\)])+(\") {return STRING;};
\/\/[^\r\n]*[\r|\n|\r\n]? ;
{whitespace}
. {output::errorLex(yylineno); exit(0);};

%%
