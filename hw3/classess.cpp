#ifndef CLASSES_CPP
#define CLASSES_CPP
#include "classes.hpp"

string currFunc;
vector<shared_ptr<SymbolTable>> tablesStack;
vector<int> offsetsStack;
bool idExists(string str)
{
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == str)
                return true;
        }
    }
    return false;
}   
void enterArguments(Formals *fm){
    for (int i = 0; i < fm->list.size(); i++)
    {
        auto entr=shared_ptr<SBEntry>(new SBEntry(fm->list[i]->value,fm->list[i]->type,-i-1));
        tablesStack.back()->lines.push_back(entr);

    }
    
}
void endFunc()
{
    currFunc = "";
}
int loopCount = 0;
void inLoop()
{
    loopCount++;
}
void outLoop()
{
    loopCount--;
}
void openScope()
{
    auto newScope = shared_ptr<SymbolTable>(new SymbolTable);
    tablesStack.emplace_back(newScope);
    offsetsStack.push_back(offsetsStack.back());
}
void closeScope()
{
    output::endScope();
    auto scope = tablesStack.back();
    for (auto i : scope->lines)
    {
        if (i->types.size() == 1)
        {
            output::printID(i->name, i->offset, i->types[0]);
        }
        else
        {
            auto retVal = i->types.back();
            i->types.pop_back();
            if (i->types.front() == "VOID")
            {
                i->types.pop_back();
            }

            output::printID(i->name, i->offset, output::makeFunctionType(retVal,
                                                                         i->types)); 
        }
    }

    while (scope->lines.size() != 0)
    {
        scope->lines.pop_back();
    }
    tablesStack.pop_back();
    offsetsStack.pop_back();
}
void endProgram(){
    auto global = tablesStack.front()->lines;
    bool mainFound = false;
    for (int i = 0; i < global.size(); ++i) {
        if (global[i]->name == "main") {
            if (global[i]->types.size() == 2) {
                if (global[i]->types[0] == "VOID" && global[i]->types[1] == "VOID") {
                    mainFound = true;
                }
            }
        }
    }
    if (!mainFound) {
        output::errorMainMissing();
        exit(0);
    }
    closeScope();
}
Exp::Exp(Exp *exp)
{
    value = exp->value;
    type = exp->type;
    bool_val = exp->bool_val;
}
Exp::Exp(Type *type, Exp *exp)
{
    value = exp->value;
    this->type = type->value;
    this->bool_val = exp->bool_val;
}
Exp::Exp(Node *_not, Exp *exp)
{
    this->type = "";
    if (exp->type != "BOOL")
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
    this->type = "BOOL";
    this->bool_val = !(exp->bool_val);
}
Exp::Exp(Node *term, std::string str) : Node(term->value)
{
    if (str == "num")
        type = "INT";
    if (str == "STRING")
        type = "STRING";
    if (str == "BOOL")
    {
        type = "BOOL";
        if (term->value == "true")
            this->bool_val = true;
        else
            this->bool_val = false;
    }
    if (str == "B")
    {
        if (stoi(term->value) > 255)
        {
            output::errorByteTooLarge(yylineno, term->value);
            exit(0);
        }
        type = "BYTE";
    }
}
Exp::Exp(Node *id)
{
    this->type = "";
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == id->value)
            {
                this->type = id->value;
                this->type = tablesStack[i]->lines[j]->types.back();
                return;
            }
        }
    }
    output::errorUndef(yylineno, id->value);
}
Exp::Exp(Call *call)
{
    this->type = call->value;
}
Exp::Exp(Exp *exp1, Exp *exp2, Exp *exp3)
{   
    if (exp2->type != "BOOL")
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
    if (exp2->bool_val)
    {
        this->value = exp1->value;
        this->type = exp1->type;
    }
    else
    {
        this->value = exp3->value;
        this->type = exp3->type;
    }

    if (exp2->type == "INT" && exp1->type == "BYTE" || exp1->type == "INT" && exp2->type == "BYTE")
    {
        return;
    }
    else if (exp1->type != exp3->type)
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
}
Exp::Exp(Exp *left, Node *op, Exp *right, std::string str)
{
    this->type = "";
    if ((left->type == "BYTE" || left->type == "INT") && (right->type == "INT" || right->type == "BYTE"))
    {
        if (str == "RELOPL" || str == "RELOPN")
        {
            this->type = "BOOL";
            return;
        }
        if (str == "ADD" || str == "MUL")
        {
            this->type = (left->type == "INT" || right->type == "INT") ? "INT" : "BYTE";
            return;
        }
    }
    else if (left->type == "BOOL" && right->type == "BOOL")
    {
        bool bleft = left->value == "true";
        bool bright = right->value == "true";
        if (str == "AND")
        {
            if (bleft && bright)
                this->value = "true";
            else
                this->value = "false";
            return;
        }
        if (str == "OR")
            {this->value = (bleft || bright) ? "true" : "false";
            return;}
        else
        {
            output::errorMismatch(yylineno);
            exit(0);
        }
    }
    else
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
}
Exp::Exp(Exp *exp, std::string str)
{
    if (exp->type != "BOOL")
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
    value = exp->value;
    bool_val = exp->bool_val;
    type = exp->type;
}
Call::Call(Node *id)
{
    auto global = tablesStack.front()->lines;
    for (auto i : global)
    {
        if (i->name == id->value)
        {
            if (i->types.size() == 1)
            {
                output::errorUndefFunc(yylineno, id->value);
                exit(0);
            }
            if (i->types.size() == 2)
            {
                this->value = i->types.back();
                return;
            }
            else
            {
                vector<string> temp = {""};
                output::errorPrototypeMismatch(yylineno, i->name, temp);
                exit(0);
            }
        }
    }
    output::errorUndefFunc(yylineno, id->value);
    exit(0);
}
Call::Call(Node *id, ExpList *list)
{
    auto global = tablesStack.front()->lines;
    for (auto i : global)
    {
        if (i->name == id->value)
        {   
            if (i->types.size() == 1)
            {
                output::errorUndefFunc(yylineno, id->value);
                exit(0);
            }
            if (i->types.size() == 1 + list->exp_list.size())
            {
                for (int j = 0; j < list->exp_list.size(); j++)
                {
                    if (list->exp_list[j].type == "BYTE" && i->types[j] == "INT")
                        continue;
                    if (list->exp_list[j].type != i->types[j])
                    {
                        i->types.pop_back();
                        output::errorPrototypeMismatch(yylineno, i->name, i->types);
                        exit(0);
                    }
                }
                this->value=i->types.back();
                return;
            }
            else
            {
                i->types.pop_back();
                output::errorPrototypeMismatch(yylineno, i->name, i->types);
                exit(0);
            }
        }
    }
    output::errorUndefFunc(yylineno, id->value);
    exit(0);
}
Program::Program()
{
    shared_ptr<SymbolTable> global = shared_ptr<SymbolTable>(new SymbolTable);
    const vector<string> temp = {"STRING", "VOID"};
    auto print = shared_ptr<SBEntry>(new SBEntry("print", temp, 0));
    const vector<string> temp2 = {"INT", "VOID"};
    auto printi = shared_ptr<SBEntry>(new SBEntry("printi", temp2, 0));
    global->lines.emplace_back(print);
    global->lines.emplace_back(printi);
    tablesStack.emplace_back(global);
    offsetsStack.emplace_back(0);
}
Statment::Statment(Node *term)
{
    if (loopCount == 0)
    {
        if (term->value == "break")
        {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
        output::errorUnexpectedContinue(yylineno);
        exit(0);
    }
    data = "break";
}

Statment::Statment(Exp *exp)
{
    if (exp->type == "VOID")
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
    string ret_type = exp->type;
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == currFunc)
            {
                int size = tablesStack[i]->lines[j]->types.size();
                if (tablesStack[i]->lines[j]->types[size - 1] == ret_type)
                {
                    data = exp->value;
                    return;
                }
                else if (ret_type == "BYTE" && tablesStack[i]->lines[j]->types[size - 1] == "INT")
                {
                    data = exp->value;
                    return;
                }
                else
                {
                    output::errorMismatch(yylineno);
                    exit(0);
                }
            }
        }
    }
    output::errorUndef(yylineno, "");
    exit(0);
}
Statment::Statment(std::string str)
{
    for (int i = tablesStack.size() - 1; i >= 0; i--)
        {
            for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
            {
                if (tablesStack[i]->lines[j]->name == currFunc)
                {
                    int size = tablesStack[i]->lines[j]->types.size();
                    if (tablesStack[i]->lines[j]->types[size - 1] == str)
                    {
                        data = "ret void";
                        return;
                    }
                    else
                    {
                        output::errorMismatch(yylineno);
                        exit(0);
                    }
                }
            }
        }
    output::errorUndef(yylineno, "");
    exit(0);
}
Statment::Statment(Node *id, Exp *exp)
{
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == id->value)
            {
                if (tablesStack[i]->lines[j]->types.size() == 1)
                {
                    if ((tablesStack[i]->lines[j]->types[0] == "INT" && exp->type == "BYTE") || (tablesStack[i]->lines[j]->types[0] == exp->type))
                        data = exp->value;
                    return;
                }
                else
                {
                    output::errorMismatch(yylineno);
                    exit(0);
                }
            }
            else
            {
                output::errorUndef(yylineno, id->value);
                exit(0);
            }
        }
    }
    output::errorUndef(yylineno, id->value);
    exit(0);
}
Statment::Statment(Type *type, Node *id, Exp *exp)
{
    if (exp->type != type->value)
    {
        if (type->value != "INT" || exp->type != "BYTE")
        {
            output::errorMismatch(yylineno);
            exit(0);
        }
    }
    if (idExists(id->value))
    {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    data = exp->value;
    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<SBEntry>(new SBEntry(id->value, type->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
}
Statment::Statment(Type *type, Node *id)
{
    if (idExists(id->value))
    {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    data = "type id";
    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<SBEntry>(new SBEntry(id->value, type->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
}
FuncDecl::FuncDecl(RetType *ret_type, Node *id, Formals *formals)
{
    if (idExists(id->value))
    {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    for (int i = 0; i < formals->list.size(); i++)
    {
        if (idExists(formals->list[i]->value) || formals->list[i]->value == id->value)
        {
            output::errorDef(yylineno, id->value);
            exit(0);
        }
        for (int j = i + 1; j < formals->list.size(); ++j)
        {
            if (formals->list[i]->value == formals->list[j]->value)
            {
                output::errorDef(yylineno, formals->list[i]->value);
                exit(0);
            }
        }
    }
    value = id->value;
    if (formals->list.size() != 0)
    {
        for (int i = 0; i < formals->list.size(); i++)
        {
            this->types.push_back(formals->list[i]->type);
        }
    }
    else
    {
        this->types.emplace_back("VOID");
    }
    this->types.emplace_back(ret_type->value);

    auto temp = shared_ptr<SBEntry>(new SBEntry(this->value, this->types, 0));
    tablesStack.back()->lines.push_back(temp);
    currFunc = id->value;
}
#endif