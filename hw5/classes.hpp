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
#include "bp.hpp"
#include "reg_pool.hpp"
extern int yylineno;
void inLoop();
void outLoop(N *first_l, P *second_l, Statment *Statement);
void openScope();
void closeScope();
void endProgram();
bool idExists(string str);
string getLLVMPrimitiveType(string type);
Node *doc_compare(Exp *left);
void if_bp(M *label1, Exp *exp);
void if_else_bp(M *label1, M *label2, Exp *exp);

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
class P;
void enterArguments(Formals *fm);
void endFunc();
Statment *add_else_statment(Statment *if_statment, Statment *else_statment);
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
    std::string inst;
    std::string reg;

    Node(std::string str)
    {
        inst = "";
        if (str == "void" || str == "bool" || str == "int" || str == "byte")
            std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        this->value = str;
    }
    Node()
    {
        inst = "";
        value = "";
    }
    virtual ~Node(){};
};
#define YYSTYPE Node *
class M : public Node
{
public:
    string inst;
    M();
};
class N : public Node
{
public:
    int location;
    N();
};

class Type : public Node
{
public:
    Type(Node *type) : Node(type->value){};
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
    RetType(Node *type) : Node(type->value)
    {
        if (type->value != "VOID" && type->value != "INT" && type->value != "BYTE" && type->value != "BOOL")
        {
            output::errorSyn(yylineno);
            exit(0);
        }
    };
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
    FormalsList(FormalsList *formals_lst, FormalDecl *formal_decl)
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
    vector<pair<int, BranchLabelIndex>> break_list;
    vector<pair<int, BranchLabelIndex>> continue_list;
    Statments(Statment *statment);
    Statments(Statments *statments, Statment *statment);
};

class Exp : public Node
{
public:
    std::string type;
    std::string start_label;
    int location;
    bool bool_val;
    vector<pair<int, BranchLabelIndex>> truelist;
    vector<pair<int, BranchLabelIndex>> falselist;
    Exp(Exp *exp);
    Exp(Type *type, Exp *exp);
    Exp(Node *_not, Exp *exp);
    Exp(Exp *left, Node *op, Exp *right, std::string str, P *short_c = nullptr);
    Exp(Exp *exp1, Exp *exp2, Exp *exp3);
    Exp(Exp *exp, std::string str);
    Exp(Node *id);
    Exp(Call *call);
    Exp(Node *term, std::string str);
};
class P : public Node
{
public:
    string inst;
    int location;
    P(Exp *left);
};
class IfStart : public Node
{
public:
    std::string data;
    IfStart(std::string str, Exp *exp)
    {
        if (exp->type != "BOOL")
        {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->data = "if/else";
    }
};
class Statment : public Node
{
public:
    std::string data;
    std::string reg;
    vector<pair<int, BranchLabelIndex>> break_list;
    vector<pair<int, BranchLabelIndex>> continue_list;
    Statment(Statments *statments)
    {
        vector<pair<int, BranchLabelIndex>> list_break;
        vector<pair<int, BranchLabelIndex>> list_continue;
        this->break_list = list_break;
        this->continue_list = list_continue;
        this->continue_list = statments->continue_list;
        this->break_list=statments->break_list;
        this->data = "black";
    };
    Statment(Type *type, Node *id);
    Statment(Node *term);
    Statment(Type *type, Node *id, Exp *exp);
    Statment(Node *id, Exp *exp);
    Statment(Call *call)
    {
        vector<pair<int, BranchLabelIndex>> list_break;
        vector<pair<int, BranchLabelIndex>> list_continue;
        this->break_list = list_break;
        this->continue_list = list_continue;
        this->data = "call";
    }
    Statment(std::string str);
    Statment(Exp *exp);
    Statment(std::string str, Exp *exp, Statment *statment)
    {
        vector<pair<int, BranchLabelIndex>> list_break;
        vector<pair<int, BranchLabelIndex>> list_continue;
        this->break_list = list_break;
        this->continue_list = list_continue;
        if (exp->type != "BOOL")
        {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->data = "if/else";
        if (statment != nullptr)
        {
            this->continue_list = statment->continue_list;
            this->break_list=statment->break_list;
        }
    }
    Statment(IfStart *if_start)
    {
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