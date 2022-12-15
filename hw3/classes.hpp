#ifndef CLASSES_HPP
#define CLASSES_HPP
#include <string.h>
#include <algorithm>
#include <vector>
#include <stack>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include "hw3_output.hpp"
extern int yylineno;
void inLoop();
void outLoop();
void openScope();
void closeScope();
void endProgram();
bool idExists(string str);

class SBEntry
{
public:
    std::string name;
    vector<std::string> types;
    int offset;

    SBEntry(std::string name, vector<std::string> types, int offset)
    {
        this->name = name;
        this->types = types;
        this->offset = offset;
    }

    SBEntry(std::string name, std::string types, int offset)
    {
        this->name = name;
        this->types.emplace_back(types);
        this->offset = offset;
    }
};
class FormalDecl;
class Type;
class Statment;
class RetType;
class Formals;
class Statments;
class Exp;
class Call;
class ExpList;
void enterArguments(Formals *fm);
void endFunc();
class SymbolTable
{
public:
    vector<shared_ptr<SBEntry>> lines;
    SymbolTable() = default;
};
class Node
{
public:
    std::string value;
    Node(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        this->value = str;
    }
    Node()
    {
        value = "";
    }
    virtual ~Node(){};
};
#define YYSTYPE Node *
class Type : public Node {
public:
    Type(Node *type) : Node(type->value) {};
};
class FuncDecl : public Node
{
public:
    vector<string> types;
    FuncDecl(RetType *ret_type, Node *id, Formals *formals);
};
class RetType : public Node
{
public:
    RetType(Node *type) : Node(type->value){};
};
class Funcs : public Node
{
public:
    Funcs(){};
};
class Program : public Node
{
public:
    Program();
};

class FormalsList : public Node
{
public:
    vector<FormalDecl *> list;
    FormalsList(FormalDecl *formal_decl)
    {
        list.insert(list.begin(), formal_decl);
    }
    FormalsList(FormalsList *formals_lst,FormalDecl *formal_decl)
    {
        list = vector<FormalDecl *>(formals_lst->list);
        list.insert(list.begin(), formal_decl);
    }
};
class Formals : public Node
{
public:
    vector<FormalDecl *> list;
    Formals(){};
    Formals(FormalsList *formals_lst)
    {
        list = vector<FormalDecl *>(formals_lst->list);
    };
};
class FormalDecl : public Node
{
public:
    std::string type;
    FormalDecl(Type *type, Node *id) : Node(id->value), type(type->value) {}
};
class Statments : public Node
{
public:
    Statments(Statment *statment){};
    Statments(Statments *statments, Statment *statment){};
};

class Exp : public Node
{
public:
    std::string type;
    bool bool_val;
    Exp(Exp *exp);
    Exp(Type *type, Exp *exp);
    Exp(Node *_not, Exp *exp);
    Exp(Exp *left, Node *op, Exp *right, std::string str);
    Exp(Exp *exp1, Exp *exp2, Exp *exp3);
    Exp(Exp *exp, std::string str);
    Exp(Node *id);
    Exp(Call *call);
    Exp(Node *term, std::string str);
};
class Statment : public Node
{
public:
    std::string data;
    Statment(Statments *statments)
    {
        this->data = "black";
    };
    Statment(Type *type, Node *id);
    Statment(Node *term);
    Statment(Type *type, Node *id, Exp *exp);
    Statment(Node *id, Exp *exp);
    Statment(Call *call)
    {
        this->data = "call";
    }
    Statment(std::string str);
    Statment(Exp *exp);
    Statment(std::string str, Exp *exp)
    {
        if (exp->type != "BOOL")
        {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->data = "if/else";
    }
};
class Call : public Node
{
public:
    Call(Node *id, ExpList *list);
    Call(Node *id);
};
// #define YYSTYPE yystype
class ExpList : public Node
{
public:
    vector<Exp> exp_list;
    ExpList(Exp *exp)
    {
        exp_list.emplace(exp_list.begin(), exp);
    }
    ExpList(Exp *exp, ExpList *exp_lst)
    {
        this->exp_list = vector<Exp>(exp_lst->exp_list);
        this->exp_list.emplace(exp_list.begin(), exp);
    }
};
#endif // CLASSES_HPP