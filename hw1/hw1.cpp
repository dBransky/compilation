#include "tokens.hpp"
#include <iostream>
#include <vector>
#include <string>
using namespace std;

void printString()
{
	string str = yytext;
	vector<int> to_del;
	vector<int> ignore;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str.substr(i,2)=="\\\\"){
			str.replace(i, 2, "\\");
			continue;
		}
		if (str.substr(i,2)=="\\n"){
			str.replace(i, 2, "\n");
			continue;
		}
		if (str.substr(i,2)=="\\t"){
			str.replace(i, 2, "\t");
			continue;
		}
		if (str.substr(i,2)=="\\r"){
			str.replace(i, 2, "\r");
			continue;
		}
		if (str.substr(i,2)=="\\\""&&(i+2)!=str.length()){
			str.replace(i, 2, "\"");
			continue;
		}
		if (str.substr(i,2)=="\\0"||str.substr(i,4)=="\\x00"){
			str=str.substr(0,i+1);
			break;
		}
		if (str.substr(i,2)=="\\x"){
			string hex_val = str.substr(i + 2, 2);
			char hex_char = (char)(int)strtol(hex_val.c_str(), NULL, 16);
			str.replace(i, 4, string(1, hex_char));
			continue;
		}

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
		string str = (string)yytext;
		string::size_type index = 0;
		switch (token)
		{
		case CHAR_ERROR:
			std::cout << "Error " << yytext << std::endl;
			exit(0);
		case STRING_LINE_ERROR:
			std::cout << "Error unclosed string" << std::endl;
			exit(0);
		case STRING_ESCAPE_ERROR:
			while ((index = str.find("\\", index)) != string::npos)
			{
				if (str.substr(index + 1, 1) != "n" || str.substr(index + 1, 1) != "t" || str.substr(index + 1, 1) != "0" || str.substr(index + 1, 1) != "r")
				{
					break;
				}
			}
			std::cout << "Error undefined escape sequence " << str.substr(index + 1, 1) << std::endl;
			exit(0);
		case STRING_ESCAPE_ERROR_HEX:

			if (str.substr(string(yytext).size() - 1) == "\"")
			{
				if (str.substr(string(yytext).size() - 2,1) == "x")
				{
					std::cout << "Error undefined escape sequence " << ((string(yytext)).substr(string(yytext).size() - 2, 1)) << std::endl;
				}
				else
				{
					std::cout << "Error undefined escape sequence " << ((string(yytext)).substr(string(yytext).size() - 3, 2)) << std::endl;
				}
			}
			else
			{
				std::cout << "Error undefined escape sequence " << ((string(yytext)).substr(string(yytext).size() - 3)) << std::endl;
			}
			exit(0);
		case STRING:
			printString();
			break;
		case COMMENT:
			std::cout << yylineno << " "
					  << "COMMENT " << ((string(yytext)).substr(0, 2)) << std::endl;
			break;
		default:
			std::cout << yylineno << " " << (tokens[token - 1]) << " " << yytext << std::endl;
			break;
		}
	}
	return 0;
}