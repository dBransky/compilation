#include "classes.hpp"

string currFucn;
vector<shared_ptr<SymbolTable>> tablesStack;
vector<int> offsetsStack;
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
    for (size_t i = tablesStack.size() - 1; i >= 0; i --)
    {
        for (size_t j = 0; j < tablesStack[j]->lines.size(); j++)
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
        if(exp2->type=="BOOL")
            this->bool_val=exp1->bool_val;
    }
    else
    {
        this->value = exp3->value;
        this->type = exp3->type;
        if(exp2->type=="BOOL")
            this->bool_val=exp3->bool_val;
    }

    if (exp2->type == "INT" && exp1->type == "BYTE" || exp1->type == "INT" && exp2->type == "BYTE")
    {
        return;
    }
    else if (exp2->type != exp1->type)
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
        }
        if (str == "ADD" || str == "MUL")
        {
            this->type = (left->type == "INT" || right->type == "INT") ? "INT" : "BYTE";
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
            this->value = (bleft || bright) ? "true" : "false";
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
                for (size_t j = 0; j < list->exp_list.size(); j++)
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
Program::Program(){
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
