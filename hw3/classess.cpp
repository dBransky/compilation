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
    for (size_t i = tablesStack.size() - 1; i >= 0; i -)
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
    if (exp2->type == "INT" && exp1->type == "BYTE" || exp1->type == "INT" && exp2->type == "BYTE")
    {
        this->type = "INT";
    }
    else
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
}
Exp::Exp(Exp *left, Node *op, Exp *right, std::string str){

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