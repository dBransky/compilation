#include "tokens.h"
#include <iostream>
#include <string>
using namespace std;

void printString(string str)
{

}



int main()
{
	string tokens[] = {
		"VOID",
		"INT",
		"BYTE",
		"B",
		"BOOL",
		"AND",
		"OR",
		"NOT",
		"TRUE",
		"FALSE",
		"RETURN",
		"IF",
		"ELSE",
		"WHILE",
		"BREAK",
		"CONTINUE",
		"SC",
		"COMMA",
		"LPAREN",
		"RPAREN",
		"LBRACE",
		"RBRACE",
		"ASSIGN",
		"RELOP",
		"BINOP",
		"COMMENT",
		"ID",
		"NUM",
		"STRING",
        "CHAR_ERROR",
        "STRING_LINE_ERROR",
        "STRING_ESCAPE_ERROR"
	};

	int token;
	while ((token = yylex()))
	{
		switch (token)
		{
		case CHAR_ERROR:
			cout << "Error " << yytext << std::endl;
            exit(0);
		case STRING_LINE_ERROR:
			cout << "Error unclosed string\\n"<<std::endl;
			return 0;
		case STRING:
			cout << yylineno << " " << (tokens[token - 1]) << " " << yytext << std::endl;
        case COMMENT:
            cout << yylineno << " " << (tokens[token - 1]) << " " << "//" << std::endl;
        default:
			cout << yylineno << " " << (tokens[token - 1]) << " " << (((string)yytext).substr(0, ((string)yytext).size()-1)) << std::endl;
			break;
		}
	}
	return 0;
}