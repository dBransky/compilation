#ifndef CLASSES_CPP
#define CLASSES_CPP
#include "classes.hpp"

string currFunc;
int amountOfCurrArgs;
regPool regsPool;
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
	if (type == "STRING") 
	{
        return "i8*";
    } 
	else if (type == "BYTE") 
	{
        return "i8";
    } 
	else if (type == "BOOL") 
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
    buffer.bpatch(buffer.makelist({first_l->loc, FIRST}), first_l->instr);
    buffer.bpatch(buffer.makelist({second_l->loc, FIRST}), secondL->instr);
    buffer.bpatch(buffer.makelist({second_l->loc, SECOND}), genLabelStr);
    buffer.bpatch(buffer.makelist({labelLoc, FIRST}), secondL->instr);
	int breakListSize = statement->breakList.size();
	int continuteListSize = statement->continueList.size();
    if (breakListSize != 0) 
	{
        buffer.bpatch(statement->breakList, genLabelStr);
    }
    if (continuteListSize != 0) 
	{
        buffer.bpatch(statement->continueList, first_l->instr);
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

            output::printID(i->name, i->offset, output::makeFunctionType(retVal, i->types));
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
    this->instr = buffer.genLabel();
}

N::N() {
    this->loc = buffer.emit("br label @");
    this->instr = buffer.genLabel();
}

P::P(Exp *left) {
    this->loc = buffer.emit("br i1 %" + left->reg + ", label @, label @");
    this->instr = buffer.genLabel();
}

Exp::Exp(Exp *exp)
{
    value = exp->value;
    type = exp->type;
    bool_val = exp->bool_val;
    reg = exp->reg;
    instrc = exp->instrc;
    trueList = exp->trueList;
    falseList = exp->falseList;
}

Exp::Exp(Type *type, Exp *exp)
{
    if ((type->value == "INT" || type->value == "BYTE") && (exp->type == "INT" || exp->type == "BYTE"))
    {
        value = exp->value;
        this->type = type->value;
        this->reg = exp->reg;
        this->instrc = exp->instrc;
        this->falseList = exp->falseList;
        this->trueList = exp->trueList;
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
    this->reg = poolregs.getReg();
    buffer.emit("%" + this->reg + " = add i1 1, %" + exp->reg);
    this->falseList = exp->trueList;
    this->trueList = exp->falseList;
}

Exp::Exp(Exp *left, Node *op, Exp *right, string str, P *shortC) 
{
    this->type = "";
    this->reg = regsPool.getReg();
    string end = "";
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
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
            if (op->value = "==") 
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
                if (isize == "i8") 
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
                    leftReg = regsPool.getReg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") 
                {
                    rightLeft = regsPool.getReg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = icmp " + relop + " " + iSize + " %" + leftReg + ", %" + rightLeft);
            if (right->instrc != "") 
            {
                end = right->instrc;
            } 
            else 
            {
                end = left->instrc;
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
            this->reg = regsPool.getReg();
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
                string backupReg = regsPool.getReg();
                if (right->type == "BYTE") 
                {
                    rightLeft = regsPool.getReg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
                if (left->type == "BYTE") 
                {
                    leftReg = regsPool.getReg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                buffer.emit("%" + backupReg + " = icmp eq i32 %" + rightLeft + ", 0");
                int first_emit = buffer.emit("br i1 %" + backupReg + ", label @, label @");
                string first_label = buffer.genLabel();
                string myregs = poolregs.getReg();
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
                end = second_left;
            }
            if (iSize == "i32") 
            {
                if (left->type == "BYTE") 
                {
                    leftReg = poolregs.getReg();
                    buffer.emit("%" + leftReg + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") 
                {
                    rightLeft = poolregs.getReg();
                    buffer.emit("%" + rightLeft + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = " + oper + " " + iSize + " %" + leftReg + ", %" + rightLeft);
            if (oper == "sdiv" && right->type == "BYTE" && left->type == "BYTE") 
            {
                string backLastRegsStr = pool.getReg();
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
            if (right->instrc != "") 
            {
                this->instrc = right->instrc;
            } else 
            {
                this->instrc = shortC->instr;
            }
            if (op->value == "and") 
            {
                int loc_bef = buffer.emit("br label @");
                string leftFalseLabel = buffer.genLabel();
                int loc_aft = buffer.emit("br label @");
                endLabel = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->instrc + "],[0, %" + leftFalseLabel + "]");
                buffer.bpatch(buffer.makelist({shortC->loc, FIRST}), shortC->instr);
                buffer.bpatch(buffer.makelist({shortC->loc, SECOND}), leftFalseLabel);
                buffer.bpatch(buffer.makelist({loc_bef, FIRST}), endLabel);
                buffer.bpatch(buffer.makelist({loc_aft, FIRST}), endLabel);
            } 
            else if (op->value.compare("or") == 0) 
            {
                int loc_bef = buffer.emit("br label @");
                string leftTrueLabel = buffer.genLabel();
                int loc_aft = buffer.emit("br label @");
                endLabel = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->instrc + "],[1, %" + leftTrueLabel + "]");
                buffer.bpatch(buffer.makelist({shortC->loc, FIRST}), leftTrueLabel);
                buffer.bpatch(buffer.makelist({shortC->loc, SECOND}), shortC->instr);
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
        this->instrc = end;
    }
}



Exp::Exp(Node *term, std::string str) : Node(term->value)
{
    vector<pair<int, BranchLabelIndex>> false_list;
    falseList = false_list;
    vector<pair<int, BranchLabelIndex>> true_list;
    trueList = true_list;
    if (str == "num")
    {
        type = "INT";
        this->reg = poolregs.getReg();
        buffer.emit("%" + this->reg + " = add i32 0," + term->value);
    }
    if (str == "STRING")
    {
        type = "STRING";
        this->reg = poolregs.getReg();
        int termSize = term->value.size();
        int lastPlace = termSize - 1;
        term->value[lastPlace] = '\00';
        buffer.emitGlobal("@" + this->reg + "= constant [" + to_string(termSize) + " x i8] c\"" + term->value + "\"");
        buffer.emit("%" + this->reg + "= getelementptr [" + to_string(termSize) + " x i8], [" + to_string(termSize) + " x i8]* @" + this->reg + ", i8 0, i8 0");
    }
    if (str == "BOOL")
    {
        type = "BOOL";
        this->reg = poolregs.getReg();
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
        this->reg = regsPool.getReg();
        buffer.emit("%" + this->reg + " = add i8 0," + term->value)
    }
}



Exp::Exp(Node *id)
{
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
    this->type = "";
    for (int i = tablesStack.size() - 1; i >= 0; i--)
    {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j)
        {
            if (tablesStack[i]->lines[j]->name == id->value)
            {
                this->value = ID->value;
                this->type = tablesStack[i]->lines[j]->types.back();
                int offset = tablesStack[i]->lines[j]->offset;
                string reg_1 = poolregs.getReg();
                string copyPtr = poolregs.getReg();
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
                    this->reg = poolregs.getReg();
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
    this->instrc = call->instrc;
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
}

Exp::Exp(Exp *exp, std::string str)
{
    if (exp->type == "BOOL") 
    {
        this->value = exp->value;
        this->type = exp->type;
        this->boolVal = exp->boolVal;
        this->reg = exp->reg;
        this->instrc = exp->instrc;
        int res = buffer.emit("br i1 %" + this->reg + ", label @, label @");
        this->trueList = buffer.makelist(pair<int, BranchLabelIndex>(res, FIRST));
        this->falseList = buffer.makelist(pair<int, BranchLabelIndex>(res, SECOND));
    }
    else
    {
        output::errorMismatch(yylineno);
        exit(0);
    }
}

Node *docompare(Exp *left) 
{
    Node *temp = new P(left);
    return temp;
}

void ifBp(M *Label1, Exp *exp) 
{
    int res = buffer.emit("br label @");
    string genLabelRes = buffer.genLabel();
    buffer.bpatch(exp->trueList, Label1->instr);
    buffer.bpatch(exp->falseList, genLabelRes);
    buffer.bpatch(buffer.makelist({res, FIRST}), genLabelRes);
}

void ifElseBp(M *Label1, N *Label2, Exp *exp) {
    int res = buffer.emit("br label @");
    string genLabelRes = buffer.genLabel();
    buffer.bpatch(exp->trueList, Label1->instr);
    buffer.bpatch(exp->falseList, Label2->instr);
    buffer.bpatch(buffer.makelist({Label2->loc, FIRST}), genLabelRes);
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
                this->reg = poolregs.getReg();
                if (getLLVMPrimitiveType(this->value) == "void")
                {
                    buffer.emit("call " + getLLVMPrimitiveType(this->value) + " @" + id->value; + "()");
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
            if (i->types.size() == 1 + list->exp_list.size())
            {
                for (int j = 0; j < list->exp_list.size(); j++)
                {
                    if (list->exp_list[j].type == "BYTE" && i->types[j] == "INT")
                    {
                        string regsStr = poolregs.getReg();
                        buffer.emit("%" + regsStr + " = zext  i8 %" + list->expList[j].reg + " to i32");
                        args += getLLVMPrimitiveType("INT") + " %" + regsStr + ",";
                        continue;
                    }
                    args += getLLVMPrimitiveType(i->types[j]) + " %" + list->expList[j].reg + ",";
                    if (list->exp_list[j].type != i->types[j])
                    {
                        i->types.pop_back();
                        output::errorPrototypeMismatch(yylineno, i->name, i->types);
                        exit(0);
                    }
                }
                args.back() = ')';
                this->value = i->types.back();
                this->reg = poolregs.getReg();
                if (getLLVMPrimitiveType(this->value) == "VOID")
                {
                    buffer.emit("call " + getLLVMPrimitiveType(this->value); + " @" + id->value; + " " + args);
                } 
                else 
                {
                    buffer.emit("%" + reg + " = call " + getLLVMPrimitiveType(this->value); + " @" + id->value; + " " + args);
                }
                int refLoc = buffer.emit("br label @");
                this->instrc = buffer.genLabel();
                buffer.bpatch(buffer.makelist({refLoc, FIRST}), this->instrc);
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
}
#endif