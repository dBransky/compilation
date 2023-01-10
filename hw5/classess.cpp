#ifndef CLASSES_CPP
#define CLASSES_CPP
#include "classes.hpp"

string currFunc;
int amountOfCurrArgs;
RegPool regsPool;
vector<shared_ptr<SymbolTable>> tablesStack;
vector<int> offsetsStack;
vector<string> primitiveTypes = {"VOID", "INT", "BYTE", "BOOL", "STRING"};
CodeBuffer &buffer = CodeBuffer::instance();

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
void enterArguments(Formals *fm)
{
    for (int i = 0; i < fm->list.size(); i++)
    {
        auto entr = shared_ptr<SBEntry>(new SBEntry(fm->list[i]->value, fm->list[i]->type, -i - 1));
        tablesStack.back()->lines.push_back(entr);
    }
}

string getLLVMPrimitiveType(string prmType) 
{
    if (prmType == "VOID") 
	{
        return "void";
	} 
	else 
	if (prmType == "STRING") 
	{
        return "i8*";
    } 
	else if (prmType == "BYTE") 
	{
        return "i8";
    } 
	else if (prmType == "BOOL") 
	{
        return "i1";
    }
	else 
	{
		return "i32"; 
	}
}

void endFunc(RetType* type) 
{
	currFunc = "";
	amountOfCurrArgs = 0; 
    if (type->value == "VOID") 
    {
        buffer.emit("ret void");
    } 
    else 
    {
        string returnType = getLLVMPrimitiveType(type->value);
        buffer.emit("ret " + returnType + " 0");
    }
	buffer.emit("}");
}

int loopCount = 0;
void inLoop()
{
    loopCount++;
}

void outLoop(N* first_l, P* second_l, Statment* statement)
{
	int labelLoc = buffer.emit("br label @");
    string genLabelStr = buffer.genLabel();
    buffer.bpatch(buffer.makelist({first_l->location, FIRST}), first_l->inst);
    buffer.bpatch(buffer.makelist({second_l->location, FIRST}), second_l->inst);
    buffer.bpatch(buffer.makelist({second_l->location, SECOND}), genLabelStr);
    buffer.bpatch(buffer.makelist({labelLoc, FIRST}), second_l->inst);
	int breakListSize = statement->break_list.size();
	int continuteListSize = statement->continue_list.size();
    if (breakListSize != 0) 
	{
        buffer.bpatch(statement->break_list, genLabelStr);
    }
    if (continuteListSize != 0) 
	{
        buffer.bpatch(statement->continue_list, first_l->inst);
    }
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
    // output::endScope();
    auto scope = tablesStack.back();
    for (auto i : scope->lines)
    {
        if (i->types.size() == 1)
        {
            // output::printID(i->name, i->offset, i->types[0]);
        }
        else
        {
            auto retVal = i->types.back();
            i->types.pop_back();
            if (i->types.front() == "VOID")
            {
                i->types.pop_back();
            }

           // output::printID(i->name, i->offset, output::makeFunctionType(retVal, i->types));
        }
    }
    while (scope->lines.size() != 0)
    {
        scope->lines.pop_back();
    }
    tablesStack.pop_back();
    offsetsStack.pop_back();
}

void endProgram()
{
    auto global = tablesStack.front()->lines;
    bool mainFound = false;
    for (int i = 0; i < global.size(); ++i)
    {
        if (global[i]->name == "main")
        {
            if (global[i]->types.size() == 2)
            {
                if (global[i]->types[0] == "VOID" && global[i]->types[1] == "VOID")
                {
                    mainFound = true;
                }
            }
        }
    }
    if (!mainFound)
    {
        output::errorMainMissing();
        exit(0);
    }
    closeScope();
    buffer.printGlobalBuffer();
    buffer.printCodeBuffer();
}

M::M() {
    this->inst = buffer.genLabel();
}

N::N() {
    this->location = buffer.emit("br label @");
    this->inst = buffer.genLabel();
}

P::P(Exp *left) {
    this->location = buffer.emit("br i1 %" + left->reg + ", label @, label @");
    this->inst = buffer.genLabel();
}

Exp::Exp(Exp *exp)
{
    value = exp->value;
    type = exp->type;
    bool_val = exp->bool_val;
    reg = exp->reg;
    inst = exp->inst;
    truelist = exp->truelist;
    falselist = exp->falselist;
}

Exp::Exp(Type *type, Exp *exp)
{
    if ((type->value == "INT" || type->value == "BYTE") && (exp->type == "INT" || exp->type == "BYTE"))
    {
        value = exp->value;
        this->type = type->value;
        this->reg = exp->reg;
        this->inst = exp->inst;
        this->falselist = exp->falselist;
        this->truelist = exp->truelist;
    }
    else
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
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
    this->reg = regsPool.get_reg();
    buffer.emit("%" + this->reg + " = add i1 1, %" + exp->reg);
    this->falselist = exp->truelist;
    this->truelist = exp->falselist;
}

Exp::Exp(Exp *left, Node *op, Exp *right, string str, P *shortC) 
{
    this->type = "";
    this->reg = regsPool.get_reg();
    string end = "";
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falselist = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->truelist = listTrue;
    if ((left->type == "BYTE" || left->type == "INT") && (right->type == "BYTE" || right->type == "INT")) 
    {
        if (str == "RELOPL" || str == "RELOPN")
        {
            this->type = "BOOL";
            string iSize = "i8";
            if (left->type == "INT" || right->type == "INT") 
            {
                iSize = "i32";
            }
            string relop;
            if (op->value == "==") 
            {
                relop = "eq";
            }
            else if (op->value == "!=") 
             {
                relop = "ne";
            } 
            else if (op->value == "<") 
            {
                relop = "slt";
                if (iSize == "i8") 
                {
                    relop = "ult";
                }
            } 
            else if (op->value == ">") 
            {
                relop = "sgt";
                if (iSize == "i8") 
                {
                    relop = "ugt";
                }
            } 
            else if (op->value == "<=") 
            {
                relop = "sle";
                if (iSize == "i8") 
                {
                    relop = "ule";
                }
            } 
            else if (op->value == ">=") 
            {
                relop = "sge";
                if (iSize == "i8") 
                {
                    relop = "uge";
                }
            }
            string leftReg = left->reg;
            string rightLeft = right->reg;
            if (iSize == "i32") 
            {
                if (left->type == "BYTE") 
                {
                    leftReg = regsPool.get_reg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") 
                {
                    rightLeft = regsPool.get_reg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = icmp " + relop + " " + iSize + " %" + leftReg + ", %" + rightLeft);
            if (right->inst != "") 
            {
                end = right->inst;
            } 
            else 
            {
                end = left->inst;
            }
        }
        if (str == "ADD" || str == "MUL") 
        {
            this->type = "BYTE";
            string iSize = "i8";
            if (left->type == "INT" || right->type == "INT") 
            {
                this->type = "INT";
                iSize = "i32";
            }
            this->reg = regsPool.get_reg();
            string oper;
            string rightLeft = right->reg;
            string leftReg = left->reg;
            if (op->value == "+") 
            {
                oper = "add";
            }
            else if (op->value.compare("-") == 0) 
            {
                oper = "sub";
            } 
            else if (op->value == "*") 
            {
                oper = "mul";
            } 
            else if (op->value == "/") 
            {
                string backupReg = regsPool.get_reg();
                if (right->type == "BYTE") 
                {
                    rightLeft = regsPool.get_reg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
                if (left->type == "BYTE") 
                {
                    leftReg = regsPool.get_reg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                buffer.emit("%" + backupReg + " = icmp eq i32 %" + rightLeft + ", 0");
                int first_emit = buffer.emit("br i1 %" + backupReg + ", label @, label @");
                string first_label = buffer.genLabel();
                string myregs = regsPool.get_reg();
                buffer.emit("%" + myregs + " = getelementptr [22 x i8], [22 x i8]* @DavidThrowsZeroExcp, i32 0, i32 0");
                buffer.emit("call void @print(i8* %" + myregs + ")");
                buffer.emit("call void @exit(i32 0)");
                int second_emit = buffer.emit("br label @");
                string second_label = buffer.genLabel();
                buffer.bpatch(buffer.makelist({first_emit, FIRST}), first_label);
                buffer.bpatch(buffer.makelist({first_emit, SECOND}), second_label);
                buffer.bpatch(buffer.makelist({second_emit, FIRST}), second_label);
                iSize = "i32";
                oper = "sdiv";
                end = second_label;
            }
            if (iSize == "i32") 
            {
                if (left->type == "BYTE") 
                {
                    leftReg = regsPool.get_reg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") 
                {
                    rightLeft = regsPool.get_reg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = " + oper + " " + iSize + " %" + leftReg + ", %" + rightLeft);
            if (oper == "sdiv" && right->type == "BYTE" && left->type == "BYTE") 
            {
                string backLastRegsStr = regsPool.get_reg();
                buffer.emit("%" + backLastRegsStr + " = trunc i32 %" + this->reg + " to i8");
                this->reg = backLastRegsStr;
            }
        }
    }
    else if ((left->type == "BOOL") && (right->type == "BOOL")) 
    {
        this->type = "BOOL";
        if (str == "AND" || str == "OR") 
        {
            if (right->inst != "") 
            {
                this->inst = right->inst;
            } else 
            {
                this->inst = shortC->inst;
            }
            if (op->value == "and") 
            {
                int loc_bef = buffer.emit("br label @");
                string leftFalseLabel = buffer.genLabel();
                int loc_aft = buffer.emit("br label @");
                string endLabel = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->inst + "],[0, %" + leftFalseLabel + "]");
                buffer.bpatch(buffer.makelist({shortC->location, FIRST}), shortC->inst);
                buffer.bpatch(buffer.makelist({shortC->location, SECOND}), leftFalseLabel);
                buffer.bpatch(buffer.makelist({loc_bef, FIRST}), endLabel);
                buffer.bpatch(buffer.makelist({loc_aft, FIRST}), endLabel);
            } 
            else if (op->value.compare("or") == 0) 
            {
                int loc_bef = buffer.emit("br label @");
                string leftTrueLabel = buffer.genLabel();
                int loc_aft = buffer.emit("br label @");
                string endLabel = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->inst + "],[1, %" + leftTrueLabel + "]");
                buffer.bpatch(buffer.makelist({shortC->location, FIRST}), leftTrueLabel);
                buffer.bpatch(buffer.makelist({shortC->location, SECOND}), shortC->inst);
                buffer.bpatch(buffer.makelist({loc_bef, FIRST}), endLabel);
                buffer.bpatch(buffer.makelist({loc_aft, FIRST}), endLabel);
            }
        }
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
    if (end != "") 
    {
        this->inst = end;
    }
}



Exp::Exp(Node *term, std::string str) : Node(term->value)
{
    vector<pair<int, BranchLabelIndex>> falselist;
    falselist = falselist;
    vector<pair<int, BranchLabelIndex>> truelist;
    truelist = truelist;
    if (str == "num")
    {
        type = "INT";
        this->reg = regsPool.get_reg();
        buffer.emit("%" + this->reg + " = add i32 0," + term->value);
    }
    if (str == "STRING")
    {
        type = "STRING";
        this->reg = regsPool.get_reg();
        int termSize = term->value.size();
        int lastPlace = termSize - 1;
        term->value[lastPlace] = '\00';
        buffer.emitGlobal("@" + this->reg + "= constant [" + to_string(termSize-1) + " x i8] c" + term->value + "\"");
        buffer.emit("%" + this->reg + "= getelementptr [" + to_string(termSize-1) + " x i8], [" + to_string(termSize-1) + " x i8]* @" + this->reg + ", i8 0, i8 0");
    }
    if (str == "BOOL")
    {
        type = "BOOL";
        this->reg = regsPool.get_reg();
        if (term->value == "true")
        {
            this->bool_val = true;
            buffer.emit("%" + this->reg + " = add i1 0,1");
        }
        else
        {
            this->bool_val = false;
            buffer.emit("%" + this->reg + " = add i1 0,0");
        }
    }
    if (str == "B")
    {
        if (stoi(term->value) > 255)
        {
            output::errorByteTooLarge(yylineno, term->value);
            exit(0);
        }
        type = "BYTE";
        this->reg = regsPool.get_reg();
        buffer.emit("%" + this->reg + " = add i8 0," + term->value);
    }
}



Exp::Exp(Node *id)
{
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falselist = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->truelist = listTrue;
    this->type = "";
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == id->value)
            {
                this->value = id->value;
                this->type = tablesStack[i]->lines[j]->types.back();
                int offset = tablesStack[i]->lines[j]->offset;
                string reg_1 = regsPool.get_reg();
                string copyPtr = regsPool.get_reg();
                if (offset >= 0) 
                {
                    buffer.emit("%" + copyPtr +" = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " + to_string(offset));
                } 
                else if (amountOfCurrArgs >= 1 && offset <= -1) 
                { 
                    buffer.emit("%" + copyPtr + " = getelementptr [ " + to_string(amountOfCurrArgs) +" x i32], [ " +to_string(amountOfCurrArgs) + " x i32]* %args, i32 0, i32 "+ to_string(amountOfCurrArgs + offset));
                } 
                else 
                {
                    cout << "SHIIT" << endl;
                }
                buffer.emit("%" + reg_1 + "= load i32, i32* %" + copyPtr);
                string id = getLLVMPrimitiveType(this->type);
                this->reg = reg_1;
                if (id != "i32") 
                {
                    this->reg = regsPool.get_reg();
                    buffer.emit("%" + this->reg + " = trunc i32 %" + reg_1 + " to " + id);
                }
                return;
            }
        }
    }
    output::errorUndef(yylineno, id->value);
    exit(0);
}
Exp::Exp(Call *call)
{
    this->type = call->value;
    this->reg = call->reg;
    this->inst = call->inst;
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falselist = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->truelist = listTrue;
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
    }
    else
    {
        this->value = exp3->value;
    }

    if (exp3->type == "INT" && exp1->type == "BYTE" || exp1->type == "INT" && exp3->type == "BYTE")
    {   
        this->type="INT";
        return;
    }
    else if (exp1->type != exp3->type)
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
    this->type=exp1->type;
}

Exp::Exp(Exp *exp, std::string str)
{
    if (exp->type == "BOOL") 
    {
        this->value = exp->value;
        this->type = exp->type;
        this->bool_val = exp->bool_val;
        this->reg = exp->reg;
        this->inst = exp->inst;
        int res = buffer.emit("br i1 %" + this->reg + ", label @, label @");
        this->truelist = buffer.makelist(pair<int, BranchLabelIndex>(res, FIRST));
        this->falselist = buffer.makelist(pair<int, BranchLabelIndex>(res, SECOND));
    }
    else
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
}

Node *doc_compare(Exp *left) 
{
    Node *temp = new P(left);
    return temp;
}

void if_bp(M *Label1, IfStart *if_start) 
{
    int res = buffer.emit("br label @");
    string genLabelRes = buffer.genLabel();
    buffer.bpatch(if_start->truelist, Label1->inst);
    buffer.bpatch(if_start->falselist, genLabelRes);
    buffer.bpatch(buffer.makelist({res, FIRST}), genLabelRes);
}

void if_else_bp(M *Label1, N *Label2, IfStart *if_start) {
    int res = buffer.emit("br label @");
    string genLabelRes = buffer.genLabel();
    buffer.bpatch(if_start->truelist, Label1->inst);
    buffer.bpatch(if_start->falselist, Label2->inst);
    buffer.bpatch(buffer.makelist({Label2->location, FIRST}), genLabelRes);
    buffer.bpatch(buffer.makelist({res, FIRST}), genLabelRes);
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
                this->reg = regsPool.get_reg();
                if (getLLVMPrimitiveType(this->value) == "void")
                {
                    buffer.emit("call " + getLLVMPrimitiveType(this->value) + " @" + id->value + "()");
                } 
                else 
                {
                    buffer.emit("%" + reg + " = call " + getLLVMPrimitiveType(this->value) + " @" + id->value + "()");
                }
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
            string args="";
            if (i->types.size() == 1 + list->exp_list.size())
            {
                for (int j = 0; j < list->exp_list.size(); j++)
                {
                    if (list->exp_list[j].type == "BYTE" && i->types[j] == "INT")
                    {
                        string regsStr = regsPool.get_reg();
                        buffer.emit("%" + regsStr + " = zext  i8 %" + list->exp_list[j].reg + " to i32");
                        args += getLLVMPrimitiveType("INT") + " %" + regsStr + ",";
                        continue;
                    }
                    args += getLLVMPrimitiveType(i->types[j]) + " %" + list->exp_list[j].reg + ",";
                    if (list->exp_list[j].type != i->types[j])
                    {
                        i->types.pop_back();
                        output::errorPrototypeMismatch(yylineno, i->name, i->types);
                        exit(0);
                    }
                }
                args.back() = ')';
                this->value = i->types.back();
                this->reg = regsPool.get_reg();
                if (getLLVMPrimitiveType(this->value) == "void")
                {
                    buffer.emit("call " + getLLVMPrimitiveType(this->value) + " @" + id->value + " (" + args);
                } 
                else 
                {
                    buffer.emit("%" + reg + " = call " + getLLVMPrimitiveType(this->value) + " @" + id->value + " (" + args);
                }
                int refLoc = buffer.emit("br label @");
                this->inst = buffer.genLabel();
                buffer.bpatch(buffer.makelist({refLoc, FIRST}), this->inst);
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
    buffer.emitGlobal("declare i32 @printf(i8*, ...)");
    buffer.emitGlobal("declare void @exit(i32)");
    buffer.emitGlobal("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    buffer.emitGlobal("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    buffer.emitGlobal("@DavidThrowsZeroExcp = constant [22 x i8] c\"Error division by zero\"");
    buffer.emitGlobal("define void @printi(i32) {");
    buffer.emitGlobal(
        "call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0), i32 %0)");
    buffer.emitGlobal("ret void");
    buffer.emitGlobal("}");
    buffer.emitGlobal("define void @print(i8*) {");
    buffer.emitGlobal(
        "call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0), i8* %0)");
    buffer.emitGlobal("ret void");
    buffer.emitGlobal("}");
}
Statment::Statment(Node *term)
{
    vector<pair<int, BranchLabelIndex>> list_break;
    vector<pair<int, BranchLabelIndex>> list_continue;
    this->break_list = list_break;
    this->continue_list = list_continue;
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
    int location = buffer.emit("br label @");
    if (term->value == "break")
        this->break_list = buffer.makelist({location, FIRST});
    else
        this->continue_list = buffer.makelist({location, FIRST});
    data = "break";
}
Statment *add_else_statment(Statment *if_statment, Statment *else_statment)
{
    if_statment->break_list = buffer.merge(if_statment->break_list, else_statment->break_list);
    if_statment->continue_list = buffer.merge(if_statment->continue_list, else_statment->continue_list);
    return if_statment;
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
Statments::Statments(Statment *statment)
{
    this->break_list = statment->break_list;
    this->continue_list = statment->continue_list;
}
Statments::Statments(Statments *statments, Statment *statment)
{
    this->break_list = buffer.merge(statments->break_list, statment->break_list);
    this->continue_list = buffer.merge(statments->continue_list, statment->break_list);
}
string emitting(string data, string type, int offset)
{
    string reg = regsPool.get_reg();
    string data_reg = data;
    string arg_type = getLLVMPrimitiveType(type);
    if (arg_type != "i32")
    {
        data_reg = regsPool.get_reg();
        buffer.emit(
            "%" + data_reg + " = zext " + arg_type + " %" + data + " to i32");
    }
    buffer.emit("%" + reg + " = add i32 0,%" + data_reg);
    string ptr = regsPool.get_reg();
    if (offset >= 0)
    {
        buffer.emit(
            "%" + ptr +
            " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " +
            to_string(offset));
    }
    else if (offset < 0 && amountOfCurrArgs > 0)
    {
        buffer.emit(
            "%" + ptr + " = getelementptr [ " + to_string(amountOfCurrArgs) +
            " x i32], [ " +
            to_string(amountOfCurrArgs) +
            " x i32]* %args, i32 0, i32 " +
            to_string(amountOfCurrArgs + offset));
    }
    buffer.emit("store i32 %" + reg + ", i32* %" + ptr);
    return reg;
}
Statment::Statment(std::string str)
{
    vector<pair<int, BranchLabelIndex>> list_break;
    vector<pair<int, BranchLabelIndex>> list_continue;
    this->break_list = list_break;
    this->continue_list = list_continue;
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
                    buffer.emit("ret void");
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
    vector<pair<int, BranchLabelIndex>> list_break;
    vector<pair<int, BranchLabelIndex>> list_continue;
    this->break_list = list_break;
    this->continue_list = list_continue;
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == id->value)
            {
                if (tablesStack[i]->lines[j]->types.size() == 1)
                {
                    if ((tablesStack[i]->lines[j]->types[0] == "INT" && exp->type == "BYTE") || (tablesStack[i]->lines[j]->types[0] == exp->type))
                    {
                        data = exp->value;
                        this->inst = exp->inst;
                        this->reg = emitting(exp->reg, exp->type, tablesStack[i]->lines[j]->offset);
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
    }
    output::errorUndef(yylineno, id->value);
    exit(0);
}
Statment::Statment(Type *type, Node *id, Exp *exp)
{
    vector<pair<int, BranchLabelIndex>> list_break;
    vector<pair<int, BranchLabelIndex>> list_continue;
    this->break_list = list_break;
    this->continue_list = list_continue;
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
    this->reg = regsPool.get_reg();
    string expType = getLLVMPrimitiveType(type->value);
    string date_reg = exp->reg;
    if (type->value == "INT" && exp->type == "BYTE")
    {

        date_reg = regsPool.get_reg();
        buffer.emit("%" + date_reg + " = zext i8 %" + exp->reg + " to i32");
    }
    buffer.emit("%" + this->reg + " = add " + expType + " 0,%" +
                date_reg);
    string ptr = regsPool.get_reg();
    buffer.emit("%" + ptr +
                " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    date_reg = reg;
    if (expType != "i32")
    {

        date_reg = regsPool.get_reg();
        buffer.emit(
            "%" + date_reg + " = zext " + expType + " %" + reg + " to i32");
    }
    buffer.emit("store i32 %" + date_reg + ", i32* %" + ptr);
}
Statment::Statment(Type *type, Node *id)
{
    vector<pair<int, BranchLabelIndex>> list_break;
    vector<pair<int, BranchLabelIndex>> list_continue;
    this->break_list = list_break;
    this->continue_list = list_continue;
    if (idExists(id->value))
    {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    data = "type id";
    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<SBEntry>(new SBEntry(id->value, type->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
    this->reg = regsPool.get_reg();
    string exp_type = getLLVMPrimitiveType(type->value);
    buffer.emit("%" + this->reg + " = add " + exp_type +
                " 0,0");
    string ptr = regsPool.get_reg();

    buffer.emit("%" + ptr +
                " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    string date_reg = reg;
    if (exp_type != "i32")
    {

        date_reg = regsPool.get_reg();
        buffer.emit(
            "%" + date_reg + " = zext " + exp_type + " %" + reg + " to i32");
    }
    buffer.emit("store i32 %" + date_reg + ", i32* %" + ptr);
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
        if (formals->list[i]->value == id->value)
        {
            output::errorDef(yylineno, id->value);
            exit(0);
        }
        if (idExists(formals->list[i]->value))
        {
            output::errorDef(yylineno, formals->list[i]->value);
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
    amountOfCurrArgs = formals->list.size();
    string arg_string = ("(");
    if (formals->list.size() != 0)
    {
        for (int i = 0; i < formals->list.size(); i++)
        {
            arg_string += getLLVMPrimitiveType(formals->list[i]->type) + ",";
        }
        arg_string.back() = ')';
    }
    else
    {
        arg_string.append(")");
    }
    string ret_type_string = getLLVMPrimitiveType(ret_type->value);
    buffer.emit(
        "define " + ret_type_string + " @" + this->value + arg_string + " {");

    buffer.emit("%stack = alloca [50 x i32]");
    buffer.emit("%args = alloca [" + to_string(formals->list.size()) +
                " x i32]");
    int size = formals->list.size();
    for (int i = 0; i < size; i++)
    {
        string ptr_reg = regsPool.get_reg();

        buffer.emit(
            "%" + ptr_reg + " = getelementptr [" + to_string(size) +
            " x i32], [" + to_string(size) +
            " x i32]* %args, i32 0, i32 " +
            to_string(amountOfCurrArgs - i - 1));
        string date_reg = to_string(i);
        string arg_type = getLLVMPrimitiveType(formals->list[i]->type);
        if (arg_type != "i32")
        {

            date_reg = regsPool.get_reg();
            buffer.emit("%" + date_reg + " = zext " + arg_type + " %" + to_string(i) + " to i32");
        }

        buffer.emit("store i32 %" + date_reg + ", i32* %" + ptr_reg);
    }
}
#endif