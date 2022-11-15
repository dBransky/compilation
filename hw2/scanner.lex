%{
#include <stdio.h>
#include "tokens.hpp"
%}

%option yylineno
%option noyywrap

digit   		  ([1-9])
digit_zero   		  ([0-9])
letter  		  ([a-zA-Z])
whitespace	  ([\t\n\r ])
hex             (\\x[0-7][0-9A-Fa-f])
escapechars     ([\\"nrt0])



%x PARENTHESIS

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
(\+|\-|\*|\/) return BINOP;
\/\/([^\n\r])* return COMMENT;
{letter}({letter}|{digit_zero})* ID
0 | {digit}{digit_zero}* NUM
"([^\n\r\"\\]|\\[rnt"\\])+" STRING
{whitespace}
. return CHAR_ERROR;

%%
