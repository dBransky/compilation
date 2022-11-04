#include "tokens.hpp"
#include <iostream>
#include <string>
using namespace std;

void printString()
{
	string str = yytext;

	string::size_type index = 0;
	while ((index = str.find("\\n", index)) != string::npos)
	{
		str.replace(index, 2, "\n");
		++index;
	}
	index = 0;
	while ((index = str.find("\\t", index)) != string::npos)
	{
		str.replace(index, 2, "\t");
		++index;
	}
	index = 0;
	while ((index = str.find("\\r", index)) != string::npos)
	{
		str.replace(index, 2, "\r");
		++index;
	}
	index = 0;
	while ((index = str.find("\\\\", index)) != string::npos)
	{
		str.replace(index, 2, "\\");
		++index;
	}
	index = 0;
	while ((index = str.find("\\\"", index)) != string::npos && (index + 2 != str.length()))
	{
		str.replace(index, 2, "\"");
		++index;
	}
	index = 0;
	while ((index = str.find("\\x", index)) != string::npos)
	{
		string hex_val = str.substr(index + 2, 2);
		char hex_char = (char)(int)strtol(hex_val.c_str(), NULL, 16);
		str.replace(index, 4, string(1, hex_char));
		++index;
	}
	std::cout << yylineno << " "
			  << "STRING"
			  << " " << ((str).substr(0, (str).size() - 1)) << std::endl;
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
		"STRING_ESCAPE_ERROR",
		"STRING_ESCAPE_ERROR_HEX"};

	int token;
	while ((token = yylex()))
	{
		switch (token)
		{
		case CHAR_ERROR:
			std::cout << "Error " << yytext << std::endl;
			exit(0);
		case STRING_LINE_ERROR:
			std::cout << "Error unclosed string" << std::endl;
			exit(0);
		case STRING_ESCAPE_ERROR:
			std::cout << "Error undefined escape sequence " << (string(yytext)).back() << std::endl;
			exit(0);
		case STRING_ESCAPE_ERROR_HEX:
			std::cout << "Error undefined escape sequence " << ((string(yytext)).substr(string(yytext).size() - 3)) << std::endl;
			exit(0);
		case STRING:
			printString();
			break;
		case COMMENT:
			std::cout << yylineno << " "
					  << "COMMENT"
					  << " " << yytext << std::endl;
			break;
		default:
			std::cout << yylineno << " " << (tokens[token - 1]) << " " << yytext << std::endl;
			break;
		}
	}
	return 0;
}