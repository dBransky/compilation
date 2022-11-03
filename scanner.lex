%{
#include <stdio.h>
#include "tokens.hpp"
%}

%option yylineno
%option noyywrap
digit   		  ([1-9])
letter  		  ([a-zA-Z])
whitespace	  ([\t\n\r ])
%%
void return VOID;
int return INT;
byte return BYTE;
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
( return LPAREN;
) return RPAREN;
{ return LBRACE;
} return RBRACE;
= return ASSIGN;
(==|!=|<=|=>|<|>) return RELOP;
(\+|-|\*|/) return RELOP;
//([^\n\r])* return COMMENT;
(letter)+((digit | letter))* return ID;
[1-9]+(digit)* return NUM;
"([^\\("")\n\r])*" return STRING;


%%
