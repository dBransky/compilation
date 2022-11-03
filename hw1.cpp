#include "tokens.hpp"
#include <iostream>
#include <string>
using namespace std;

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
		"STRING"

	};

	int token;
	while ((token = yylex()))
	{
		switch (token)
		{

		case CHAR_ERROR:
			cout << "Error " << yytext << std::endl;
			return 0;
		case STRING_LINE_ERROR:
			cout << "Error unclosed string\\n"<<std::endl;
			return 0;
		case STRING:
			cout << yylineno << " " << (tokens[token - 1]) << " " << yytext << std::endl;
		default:
			cout << yylineno << " " << (tokens[token - 1]) << " " << (((string)yytext).substr(0, ((string)yytext).size()-1)) << std::endl;
			break;
		}
	}
	return 0;
}