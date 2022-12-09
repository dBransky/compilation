#ifndef CLASSES_HPP
#define CLASSES_HPP
#include <string>
#include <algorithm>
#include <vector>
#include <stack>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include "hw3_output.hpp"
extern int yylineno;
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
class FuncDecl : public Node
{
public:
    vector<FormalDecl *> formals;
    FuncDecl(RetType *ret_type, Node *id, Formals *formals);
};
class RetType : public Node
{
public:
    RetType(Node *type);
};
class Formals : public Node
{
public:
    Formals();
    Formals(FormalsList *formals_lst);
};

class FormalsList : public Node
{
    vector<Formals> list;
    FormalsList(FormalDecl *formal_decl);
    FormalsList(FormalDecl *formal_decl, FormalsList *formals_lst);
};
class FormalDecl : public Node
{
public:
    std::string type;
    FormalDecl(Type *type, Node *id);
};
class Statments : public Node
{
public:
    Statments(Statment *statment);
    Statments(Statments *statments, Statment *statment);
};

class Statment : public Node
{
public:
    Statment(Statments *statments);
    Statment(Type *type, Node *id);
    Statment(Node *term);
    Statment(Type *type, Node *id, Exp *exp);
    Statment(Call *call);
    Statment(std::string str);
    Statment(Exp *exp);
    Statment(std::string str, Exp *exp);
};
class Call : public Node
{
public:
    Call(Node *id, ExpList *list);
    Call(Node *id);
};
// #define YYSTYPE yystype
class Type : public Node
{
public:
    Type(Node *type);
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
class ExpList : public Node
{
public:
    vector<Exp> exp_list;
    ExpList(Exp *exp);
    ExpList(Exp *exp, ExpList *exp_lst);
};
#endif // CLASSES_HPP