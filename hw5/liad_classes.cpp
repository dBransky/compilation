//
// Created by Liad on 18/12/2019.
//todo: 1)Tests for Numeric Slide of arithmetic exp's
//      2)where to declere the print functions(that are present in the symbol table from the start)
//

#include "classes.hpp"


/// global variables and functions
string currFucn;
int currFuncArgs;
regPool pool;
CodeBuffer &buffer = CodeBuffer::instance();
vector<shared_ptr<SymbolTable>> tablesStack;
vector<int> offsetsStack;
vector<shared_ptr<EnumTable>> enumsStack;//will hold all the enums that were defined
vector<string> TYPES = {"VOID", "INT", "BYTE", "BOOL", "STRING"};

void endFunc(RetType *retType) {
    if (retType->value == "VOID") {
        buffer.emit("ret void");
    } else {
        string returnType = get_LLVM_Type(retType->value);
        buffer.emit("ret " + returnType + " 0");
    }
    buffer.emit("}");
    currFucn = "";
    currFuncArgs = 0;
}

int loopCount = 0;

void inLoop() {
    loopCount++;
}

void outLoop(N *firstL, P *secondL, Statement *st) {
    loopCount--;
    int loc = buffer.emit("br label @");
    string str = buffer.genLabel();
    buffer.bpatch(buffer.makelist({firstL->loc, FIRST}), firstL->instr);
    buffer.bpatch(buffer.makelist({secondL->loc, FIRST}), secondL->instr);
    buffer.bpatch(buffer.makelist({secondL->loc, SECOND}), str);
    buffer.bpatch(buffer.makelist({loc, FIRST}), firstL->instr);
    if (st->breakList.size() != 0) {
        buffer.bpatch(st->breakList, str);
    }
    if (st->continueList.size() != 0) {
        buffer.bpatch(st->continueList, firstL->instr);
    }
}

void openScope() {
    auto newScope = shared_ptr<SymbolTable>(new SymbolTable);
    tablesStack.emplace_back(newScope);
    auto newEnumScope = shared_ptr<EnumTable>(new EnumTable);
    enumsStack.emplace_back(newEnumScope);
    offsetsStack.push_back(offsetsStack.back());
}

void closeScope() {
//    output::endScope();
    auto scope = tablesStack.back();
    for (auto i:scope->lines) {//printing all the variables(not enumDef) and functions
        if (i->types.size() == 1) {
//            output::printID(i->name, i->offset, i->types[0]);//variables
        } else {
            auto retVal = i->types.back();
            i->types.pop_back();
            if (i->types.front() == "VOID") {
                i->types.pop_back();
            }

//            output::printID(i->name, i->offset, output::makeFunctionType(retVal,
//                                                                         i->types));//functions
        }
    }
    auto enumScope = enumsStack.back();
    for (int j = 0; j < enumScope->enumLines.size(); ++j) {
//        output::printEnumType(enumScope->enumLines[j]->name, enumScope->enumLines[j]->values);
    }
    while (scope->lines.size() != 0) {
        scope->lines.pop_back();
    }
    tablesStack.pop_back();
    offsetsStack.pop_back();
    while (enumScope->enumLines.size() != 0) {
        enumScope->enumLines.pop_back();
    }
    enumsStack.pop_back();
}

void endProgram() {
    auto global = tablesStack.front()->lines;
    bool mainFound = false;
    for (int i = 0; i < global.size(); ++i) {
        if (global[i]->name == "main") {
            if (global[i]->types.size() == 2) {
                if (global[i]->types[0] == "VOID" &&
                    global[i]->types[1] == "VOID") {
                    mainFound = true;
                }
            }
        }
    }
    if (!mainFound) {//no main function
        output::errorMainMissing();
        exit(0);
    }
    closeScope();
    buffer.printGlobalBuffer();
    buffer.printCodeBuffer();
}

bool identifierExists(string str) {
    for (int i = tablesStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j) {
            if (tablesStack[i]->lines[j]->name == str)
                return true;
        }
    }
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            if (enumsStack[i]->enumLines[j]->name == str) {
                return true;
            }
            for (auto k:enumsStack[i]->enumLines[j]->values) {
                if (k == str) {
                    return true;
                }
            }
        }
    }
    return false;
}

/// gets a type(BOOL,BYTE,ENUM,INT) and returns the matching llvm type
string get_LLVM_Type(string type) {
    //the only avialble types are BOOL,BYTE,INT,ENUM
    if (type == "VOID") {
        return "void";
    } else if (type == "BOOL") {
        return "i1";
    } else if (type == "BYTE") {
        return "i8";
    } else if (type == "STRING") {
        return "i8*";
    } else return "i32"; //INT and ENUM fall here
}

void enterArguments(Formals *fm) {
//    for (int i = fm->formals.size() - 1; i >= 0; i--) {
//        auto temp = shared_ptr<Entry>(new Entry(fm->formals[i]->value, fm->formals[i]->type,
//                                                0 - (fm->formals.size() - i)));
    for (int i = 0; i < fm->formals.size(); i++) {
        auto temp = shared_ptr<Entry>(
                new Entry(fm->formals[i]->value, fm->formals[i]->type, -i - 1));
        tablesStack.back()->lines.push_back(temp);
    }
}
///EXP implamtation

Exp::Exp(Node *terminal, string
str) : Node(terminal->value) {
//    this->startLabel = buffer.genLabel();
    this->type = "";
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
    if (str.compare("num") == 0) {
        type = "INT";
        this->reg = pool.getReg();
        buffer.emit("%" + this->reg + " = add i32 0," + terminal->value);
    }
    if (str.compare("STRING") == 0) {
        type = "STRING";
        this->reg = pool.getReg();
        terminal->value[terminal->value.size() - 1] = '\00';
        int size = terminal->value.size();
        buffer.emitGlobal("@" + reg + "= constant [" + to_string(size) + " x i8] c\"" + terminal->value + "\"");
        buffer.emit("%" + reg + "= getelementptr [" + to_string(size) + " x i8], [" + to_string(size) + " x i8]* @" + reg + ", i8 0, i8 0");
        //%t3 = getelementptr [10 x i8], [10 x i8]* @t3, i8 0, i8 0
        //@name = constant [4 x i8] c"blabal"
    }
    if (str.compare("BOOL") == 0) {
        type = "BOOL";
        this->reg = pool.getReg();
        if (value.compare("true") == 0) {
            boolVal = true;
//            int loc = buffer.emit("br label @");
//            this->trueList = buffer.makelist({loc, FIRST});
            buffer.emit("%" + this->reg + " = add i1 0,1");
        } else {
            boolVal = false;
//            int loc = buffer.emit("br label @");
//            this->falseList = buffer.makelist({loc, FIRST});
            buffer.emit("%" + this->reg + " = add i1 0,0");
        }
    }
    if (str.compare("B") == 0) {
        if (stoi(terminal->value) > 255) {
            output::errorByteTooLarge(yylineno, terminal->value);
            exit(0);
        }
        type = "BYTE";
        this->reg = pool.getReg();
        buffer.emit("%" + this->reg + " = add i8 0," + terminal->value);
    }
}

Exp::Exp(Node *Not, Exp *exp) {
//    this->startLabel = buffer.genLabel();
    this->type = "";
    if (exp->type != "BOOL") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    this->type = "BOOL";
    this->reg = pool.getReg();
    buffer.emit("%" + this->reg + " = add i1 1, %" + exp->reg);
    this->falseList = exp->trueList;
    this->trueList = exp->falseList;
}

string relo(string s) {
    string ss = s;
    return s;
}

Exp::Exp(Exp *left, Node *op, Exp *right, string str, P *shortC) {
    //this->startLabel = buffer.genLabel();
    this->type = "";
    this->reg = pool.getReg();
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
    string end = "";
    if ((left->type.compare("BYTE") == 0 || left->type.compare("INT") == 0) &&
        (right->type.compare("BYTE") == 0 || right->type.compare("INT") == 0)) {// both operands must be numbers

        if (str.compare("RELOPL") == 0 || str.compare("RELOPN") == 0) {
            this->type = "BOOL";
            string isize = "i8";
            if (left->type == "INT" || right->type == "INT") {
                isize = "i32";
            }
            string relop;
            if (op->value.compare("==") == 0) {
                relop = "eq";
            } else if (op->value.compare("!=") == 0) {
                relop = "ne";
            } else if (op->value.compare("<") == 0) {
                relop = "slt";
                if (isize == "i8") {
                    relop = "ult";
                }
            } else if (op->value.compare(">") == 0) {
                relop = "sgt";
                if (isize == "i8") {
                    relop = "ugt";
                }
            } else if (op->value.compare("<=") == 0) {
                relop = "sle";
                if (isize == "i8") {
                    relop = "ule";
                }
            } else if (op->value.compare(">=") == 0) {
                relop = "sge";
                if (isize == "i8") {
                    relop = "uge";
                }
            }
            string dataRegL = left->reg;
            string dataRegR = right->reg;
            if (isize == "i32") {
                if (left->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegL = pool.getReg();
                    buffer.emit("%" + dataRegL + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegR = pool.getReg();
                    buffer.emit("%" + dataRegR + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = icmp " + relop + " " + isize + " %" + dataRegL + ", %" + dataRegR);
            if (right->instrc != "") {
                end = right->instrc;
            } else end = left->instrc;

//            int loc = buffer.emit("br i1 " + this->reg + ", label @, label @");
//            this->trueList = buffer.makelist({loc, FIRST});
//            this->falseList = buffer.makelist({loc, SECOND});
        }
        if (str.compare("ADD") == 0 || str.compare("MUL") == 0) {
            this->type = "BYTE";
            string isize = "i8";
            if (left->type == "INT" || right->type == "INT") {
                this->type = "INT";
                isize = "i32";
            }
            this->reg = pool.getReg();
            string operation;
            string dataRegR = right->reg;
            string dataRegL = left->reg;
            if (op->value.compare("+") == 0) {
                operation = "add";
            } else if (op->value.compare("-") == 0) {
                operation = "sub";
            } else if (op->value.compare("*") == 0) {
                operation = "mul";
            } else if (op->value.compare("/") == 0) {
                string cond = pool.getReg();
                if (right->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegR = pool.getReg();
                    buffer.emit("%" + dataRegR + " = zext i8 %" + right->reg + " to i32");
                }
                if (left->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegL = pool.getReg();
                    buffer.emit("%" + dataRegL + " = zext i8 %" + left->reg + " to i32");
                }
                buffer.emit("%" + cond + " = icmp eq i32 %" + dataRegR + ", 0");
                int Bfirst = buffer.emit("br i1 %" + cond + ", label @, label @");//label %zeroflag, label %dodiv
                string Lfirst = buffer.genLabel();//zeroflag
                string zero = pool.getReg();
                buffer.emit("%" + zero + " = getelementptr [22 x i8], [22 x i8]* @DavidThrowsZeroExcp, i32 0, i32 0");
                buffer.emit("call void @print(i8* %" + zero + ")");//  call void (i8*)* @print(i8* %str)
                buffer.emit("call void @exit(i32 0)");
                int Bsecond = buffer.emit("br label @");//dodiv
                string Lsecond = buffer.genLabel();//zeroflag
                buffer.bpatch(buffer.makelist({Bfirst, FIRST}), Lfirst);
                buffer.bpatch(buffer.makelist({Bfirst, SECOND}), Lsecond);
                buffer.bpatch(buffer.makelist({Bsecond, FIRST}), Lsecond);
                isize = "i32";
                operation = "sdiv";
                end = Lsecond;
            }
            if (isize == "i32") {
                if (left->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegL = pool.getReg();
                    buffer.emit("%" + dataRegL + " = zext i8 %" + left->reg + " to i32");
                }
                if (right->type == "BYTE") {
                    //%X = zext i8 %t3 to i32
                    dataRegR = pool.getReg();
                    buffer.emit("%" + dataRegR + " = zext i8 %" + right->reg + " to i32");
                }
            }
            buffer.emit("%" + this->reg + " = " + operation + " " + isize + " %" + dataRegL + ", %" + dataRegR);
            if (operation == "sdiv" && right->type == "BYTE" && left->type == "BYTE") {
                //%X = zext i8 %t3 to i32
                string dataReg = pool.getReg();
                buffer.emit("%" + dataReg + " = trunc i32 %" + this->reg + " to i8");
                this->reg = dataReg;
            }
        }
    } else if ((left->type.compare("BOOL") == 0 &&
                right->type.compare("BOOL") == 0)) {//both are bool
        //handiling AND OR
        this->type = "BOOL";

        if (str.compare("AND") == 0 || str.compare("OR") == 0) {
            string boolop;
            if (right->instrc != "") {
                this->instrc = right->instrc;
            } else {
                this->instrc = shortC->instr;
            }
            if (op->value.compare("and") == 0) {
//                buffer.bpatch(left->trueList, shortC->instr);
//                this->trueList = right->trueList;
//                this->falseList = buffer.merge(left->falseList, right->falseList);
                int loc2 = buffer.emit("br label @");//label is end
                string leftFalse = buffer.genLabel();
                int loc3 = buffer.emit("br label @");//label is end
                end = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->instrc + "],[0, %" + leftFalse + "]");
                buffer.bpatch(buffer.makelist({shortC->loc, FIRST}), shortC->instr);
                buffer.bpatch(buffer.makelist({shortC->loc, SECOND}), leftFalse);
                buffer.bpatch(buffer.makelist({loc2, FIRST}), end);
                buffer.bpatch(buffer.makelist({loc3, FIRST}), end);
            } else if (op->value.compare("or") == 0) {
//                buffer.bpatch(left->falseList, shortC->instr);
//                this->falseList = right->falseList;
//                this->trueList = buffer.merge(left->trueList, right->trueList);
                int loc2 = buffer.emit("br label @");//label is end
                string leftTrue = buffer.genLabel();
                int loc3 = buffer.emit("br label @");//label is end
                end = buffer.genLabel();
                buffer.emit("%" + this->reg + " = phi i1 [%" + right->reg + ", %" + this->instrc + "],[1, %" + leftTrue + "]");
                buffer.bpatch(buffer.makelist({shortC->loc, FIRST}), leftTrue);
                buffer.bpatch(buffer.makelist({shortC->loc, SECOND}), shortC->instr);
                buffer.bpatch(buffer.makelist({loc2, FIRST}), end);
                buffer.bpatch(buffer.makelist({loc3, FIRST}), end);
            }
        } else {
            output::errorMismatch(yylineno);
            exit(0);
        }
    } else {
        output::errorMismatch(yylineno);
        exit(0);
    }
    if (end != "") {
        this->instrc = end;
    }

}

Exp::Exp(Exp *exp) {
//    this->startLabel = buffer.genLabel();
    this->value = exp->value;
    this->type = exp->type;
    this->boolVal = exp->boolVal;
    this->reg = exp->reg;
    this->instrc = exp->instrc;
    this->trueList = exp->trueList;
    this->falseList = exp->falseList;
}

Exp::Exp(Type *type, Exp *exp) {//cant see type because it is announced later
//    this->startLabel = buffer.genLabel();
    this->type = "";
    if (exp->type.compare(0, 5, "enum ") == 0) {//exp type is enum
        if (type->value == "INT") {//casting into int
            value = exp->value;
            this->type = "INT";
            this->reg = exp->reg;
            this->instrc = exp->instrc;
            this->falseList = exp->falseList;
            this->trueList = exp->trueList;
        }
    }
}

string loadVariable(int offset, string type) {
    //%val = load i32* %ptr
    string reg = pool.getReg();
    string ptrReg = pool.getReg();
    if (offset >= 0) {//checking if data is in the stack or args
        buffer.emit(
                "%" + ptrReg +
                " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    } else if (offset < 0 && currFuncArgs > 0) { //data is in args
//                %t5= add offset+sizeofargs
        buffer.emit(
                "%" + ptrReg + " = getelementptr [ " + to_string(currFuncArgs) +
                " x i32], [ " +
                to_string(currFuncArgs) + " x i32]* %args, i32 0, i32 "
                + to_string(currFuncArgs + offset));
    } else {
        cout << "SHIIT" << endl;
    }
    //%val = load i32, i32* %ptr
    buffer.emit("%" + reg + "= load i32, i32* %" + ptrReg);
    string idType = get_LLVM_Type(type);
    string dataReg = reg;
    if (idType != "i32") {
        //%X= trunc i32 %Y to i8
        dataReg = pool.getReg();
        buffer.emit("%" + dataReg + " = trunc i32 %" + reg + " to " + idType);
    }
    return dataReg;

}

Exp::Exp(Node *ID) {
//    this->startLabel = buffer.genLabel();
    this->type = "";
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;
    for (int i = tablesStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j) {
            if (tablesStack[i]->lines[j]->name == ID->value) {
                this->value = ID->value;
                this->type = tablesStack[i]->lines[j]->types.back();
                this->reg = loadVariable(tablesStack[i]->lines[j]->offset, this->type);
                return;
            }
        }
    }
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            for (int k = 0;
                 k < enumsStack[i]->enumLines[j]->values.size(); ++k) {
                if (enumsStack[i]->enumLines[j]->values[k] == ID->value) {
                    this->value = ID->value;
                    string XX = "enum";
                    this->type = XX + " " + enumsStack[i]->enumLines[j]->name;
                    this->reg = pool.getReg();
                    buffer.emit("%" + this->reg + "= add i32 0, " + to_string(k));
                    return;
                }
            }
        }
    }
    output::errorUndef(yylineno, ID->value);
    exit(0);
}

Exp::Exp(Call *call) {
//    this->startLabel = buffer.genLabel();
    this->type = call->value;
    this->reg = call->reg;
    this->instrc = call->instrc;
    vector<pair<int, BranchLabelIndex>> listFalse;
    this->falseList = listFalse;
    vector<pair<int, BranchLabelIndex>> listTrue;
    this->trueList = listTrue;


}

Exp::Exp(Exp *exp, string str) {//checking for if and short circuit
//    this->startLabel = buffer.genLabel();
    if (exp->type != "BOOL") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    value = exp->value;
    type = exp->type;
    boolVal = exp->boolVal;
    this->reg = exp->reg;
    this->instrc = exp->instrc;
    int loc = buffer.emit("br i1 %" + this->reg + ", label @, label @");
    trueList = buffer.makelist(pair<int, BranchLabelIndex>(loc, FIRST));
    falseList = buffer.makelist(pair<int, BranchLabelIndex>(loc, SECOND));
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

Node *docompare(Exp *left) {
    Node *temp = new P(left);
    return temp;
}

void ifBp(M *Label1, Exp *exp) {
    int loc = buffer.emit("br label @");
    string end = buffer.genLabel();
    buffer.bpatch(exp->trueList, Label1->instr);
    buffer.bpatch(exp->falseList, end);
    buffer.bpatch(buffer.makelist({loc, FIRST}), end);
}

void ifElseBp(M *Label1, N *Label2, Exp *exp) {
    int loc2 = buffer.emit("br label @");
    string end = buffer.genLabel();
    buffer.bpatch(exp->trueList, Label1->instr);
    buffer.bpatch(exp->falseList, Label2->instr);
    buffer.bpatch(buffer.makelist({Label2->loc, FIRST}), end);
    buffer.bpatch(buffer.makelist({loc2, FIRST}), end);
}

EnumType::EnumType(Node *Enum, Node *id) {
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            if (enumsStack[i]->enumLines[j]->name == id->value) {
                value = Enum->value + " " + id->value;
                return;
            }
        }
    }
    output::errorUndefEnum(yylineno, id->value);
    exit(0);
}

Call::Call(Node *ID, ExpList *list) {
    auto global = tablesStack.front()->lines;
    for (auto i:global) {
        if (i->name == ID->value) {// id found
            if (i->types.size() == 1) {
                output::errorUndefFunc(yylineno, ID->value);
                exit(0);
            }
            string args = "(";//will hold the passed args themselves (i32 %x,i32 %y)
            if (i->types.size() ==
                1 + list->expList.size()) {//checking the number of arguments
                for (int j = 0; j < list->expList.size(); j++) {
                    if (list->expList[j].type == "BYTE" && i->types[j] == "INT") {
                        string reg = pool.getReg();
                        buffer.emit("%" + reg + " = zext  i8 %" + list->expList[j].reg + " to i32");
                        args += get_LLVM_Type("INT") + " %" + reg + ",";
                        continue;
                    }
                    args += get_LLVM_Type(i->types[j]) + " %" + list->expList[j].reg + ",";
//                    if (list->expList[j].type == "BYTE" &&i->types[j] == "INT") {
//                        continue;
//                    }
                    if (list->expList[j].type != i->types[j]) {
                        i->types.pop_back();
                        output::errorPrototypeMismatch(yylineno, i->name, i->types);
                        exit(0);
                    }
                }
                args.back() = ')';
                this->value = i->types.back();
                string funcName = ID->value;
                string retType = get_LLVM_Type(this->value);
                this->reg = pool.getReg();
                if (retType == "void") {
                    buffer.emit("call " + retType + " @" + funcName + " " + args);
                } else {
                    buffer.emit("%" + reg + " = call " + retType + " @" + funcName + " " + args);
                    /// this is a call with no parameters
                    ///     %call = call i32  @foo(i32 %whhh,i32 %whhh)
                }
                int loc = buffer.emit("br label @");
                this->instrc = buffer.genLabel();
                buffer.bpatch(buffer.makelist({loc, FIRST}), this->instrc);
                return;//if we got here without errors, we found our function
            } else {
                i->types.pop_back();
                output::errorPrototypeMismatch(yylineno, i->name, i->types);
                exit(0);
            }
        }
    }
    output::errorUndefFunc(yylineno,
                           ID->value);//if we found our function we're not supposed to get here
    exit(0);
}

Call::Call(Node *ID) {
    auto global = tablesStack.front()->lines;
    for (auto i:global) {
        if (i->name == ID->value) {
            if (i->types.size() == 1) {
                output::errorUndefFunc(yylineno, ID->value);
                exit(0);
            }
            if (i->types.size() == 2) {
                this->value = i->types.back();
                string funcName = ID->value;
                string retType = get_LLVM_Type(this->value);
                this->reg = pool.getReg();
                if (retType == "void") {
                    buffer.emit("call " + retType + " @" + funcName + "()");
                } else {
                    buffer.emit("%" + reg + " = call " + retType + " @" + funcName + "()");
                    /// this is a call with no parameters
                    ///            %call = call i32 @foo()
                }
                return;//if we got here without errors, we found our function
            } else {
                vector<string> temp = {""};
                output::errorPrototypeMismatch(yylineno, i->name, temp);
                exit(0);
            }
        }
    }
    output::errorUndefFunc(yylineno,
                           ID->value);//if we found our function we're not supposed to get here
    exit(0);
}

bool FormalDecl::checkingTypes(string str) {
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            int len = str.length();
            if (str.compare(5, len - 5,
                            enumsStack[i]->enumLines[j]->name) == 0) {
                return true;
            }
        }
    }
    return false;
}

EnumDecl::EnumDecl(Node *id, EnumeratorList *lst) {
    if (identifierExists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    for (int i = 0; i < lst->enumerators.size(); ++i) {
        if (identifierExists(lst->enumerators[i]) ||
            lst->enumerators[i] == id->value) {
            output::errorDef(yylineno, lst->enumerators[i]);
            exit(0);
        }
        for (int j = i + 1; j < lst->enumerators.size(); ++j) {
            if (lst->enumerators[i] == lst->enumerators[j]) {
                output::errorDef(yylineno, lst->enumerators[i]);
                exit(0);
            }
        }
    }
    this->value = id->value;
    enumerators = vector<string>(lst->enumerators);
    auto enumline = shared_ptr<Enum>(new Enum(id->value, lst->enumerators));
    enumsStack.back()->enumLines.emplace_back(enumline);
}


FuncDecl::FuncDecl(RetType *retType, Node *ID, Formals *args) {
    if (identifierExists(ID->value)) {
        output::errorDef(yylineno, ID->value);
        exit(0);
    }
    for (int i = 0; i < args->formals.size(); ++i) {
        if (identifierExists(args->formals[i]->value) ||
            args->formals[i]->value == ID->value) {
            output::errorDef(yylineno, args->formals[i]->value);
            exit(0);
        }
        for (int j = i + 1; j < args->formals.size(); ++j) {
            if (args->formals[i]->value == args->formals[j]->value) {
                output::errorDef(yylineno, args->formals[i]->value);
                exit(0);
            }
        }
    }
    value = ID->value;
    if (args->formals.size() != 0) {// this will be used for the symbol table
        for (int i = 0; i < args->formals.size(); i++) {
            types.push_back(args->formals[i]->type);
        }
    } else {
        types.emplace_back("VOID");
    }
    types.emplace_back(retType->value);//emplacing the return type
    auto temp = shared_ptr<Entry>(new Entry(this->value, this->types, 0));
    tablesStack.back()->lines.push_back(temp);
    currFucn = ID->value;
    currFuncArgs = args->formals.size();
    string argString = ("(");//will be printed in the llvm command
///symbol table finished, starting to emit the LLVM
    if (args->formals.size() != 0) {//this is for the LLVM command
        for (int i = 0; i <
                        args->formals.size(); i++) {//the only avialble types are BOOL,BYTE,INT,ENUM
            argString += get_LLVM_Type(args->formals[i]->type) +
                         ",";  //using this for to make the argString
        }
        argString.back() = ')'; // args is "(type name,type name)"
    } else {//no args
        argString.append(")");// args is "()"
    }
    string retTypeString = get_LLVM_Type(retType->value);
    buffer.emit(
            "define " + retTypeString + " @" + this->value + argString + " {");
    //  define i32 @foo(i32,i32)

    ///initializing args and stack

    buffer.emit("%stack = alloca [50 x i32]");
    buffer.emit("%args = alloca [" + to_string(args->formals.size()) +
                " x i32]");//%args= alloca [10 x i32]
    int size = args->formals.size();
    for (int i = 0; i < size; i++) {
        string ptrReg = pool.getReg();//gets a new register to hold the ptr
        //this is the syntax from class %first = getelementptr [10 x i32],[10 x i32]* %MyArr, i32 0, i32 0
        buffer.emit(
                "%" + ptrReg + " = getelementptr [" + to_string(size) +
                " x i32], [" + to_string(size) +
                " x i32]* %args, i32 0, i32 " +
                to_string(currFuncArgs - i - 1));//               4
        string dataReg = to_string(i);//                    0
        string argtype = get_LLVM_Type(args->formals[i]->type);
        if (argtype != "i32") {
            //%X = zext i8 %t3 to i32
            dataReg = pool.getReg();
            buffer.emit("%" + dataReg + " = zext " + argtype + " %" + to_string(i) + " to i32");
        }
        //store i32 %t3, i32* %ptr
        buffer.emit("store i32 %" + dataReg + ", i32* %" + ptrReg);
    }

}

//void pushStack(string reg){
//    buffer.emit("")
//}

///Statemant implamtation

Statement::Statement(Type *type, Node *id) {
//checking if the id already defined
    if (identifierExists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<Entry>(new Entry(id->value, type->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
    data = "type id";
    this->reg = pool.getReg();
    string expType = get_LLVM_Type(type->value);
    buffer.emit("%" + this->reg + " = add " + expType +
                " 0,0");//%reg= add i8 r3, r3
    string ptr = pool.getReg();
    //%first = getelementptr [10 x i32],[10 x i32]* %MyArr, i32 0, i32 0
    buffer.emit("%" + ptr +
                " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    string dataReg = reg;
    if (expType != "i32") {
        //%X = zext i8 %t3 to i32
        dataReg = pool.getReg();
        buffer.emit(
                "%" + dataReg + " = zext " + expType + " %" + reg + " to i32");
    }
    buffer.emit("store i32 %" + dataReg + ", i32* %" + ptr);
    //%ptr = getelementptr [10 x i32]*, [10 x i32]* %args, i32 0, i32 0
    //store i32 %t3, i32* %ptr

}

Statement::Statement(EnumType *enumType, Node *id) {
    bool enumID_Found = false;
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            int len = enumType->value.length();
            if (enumType->value.compare(5, len - 5,
                                        enumsStack[i]->enumLines[j]->name) ==
                0) {
                enumID_Found = true;
                break;
            }
        }
    }
    if (enumID_Found == false) {
        output::errorUndefEnum(yylineno, enumType->value.substr(5,
                                                                enumType->value.size() -
                                                                5));
        exit(0);
    }
    if (identifierExists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    data = "enumType id";
    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<Entry>(
            new Entry(id->value, enumType->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
    this->reg = pool.getReg();
    buffer.emit("%" + this->reg + " = add i32 0,0");//%reg= add i8 r3, r3
    string ptr = pool.getReg();
    buffer.emit("%" + ptr +
                " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    string dataReg = reg;
    buffer.emit("store i32 %" + dataReg + ", i32* %" + ptr);
    //%ptr = getelementptr [10 x i32]*, [10 x i32]* %args, i32 0, i32 0
    //store i32 %t3, i32* %ptr
}

//int x= y;
Statement::Statement(Type *type, Node *id, Exp *exp) {
    //checking if the id already defined
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    if (exp->type != type->value) {
        if (type->value != "INT" || exp->type != "BYTE") {
            output::errorMismatch(yylineno);
            exit(0);
        }
    }
    if (identifierExists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }

    data = exp->value;

    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<Entry>(new Entry(id->value, type->value, offset));
    tablesStack.back()->lines.emplace_back(temp);

    this->reg = pool.getReg();
    string expType = get_LLVM_Type(type->value);
    string dataReg = exp->reg;
    if (type->value == "INT" && exp->type == "BYTE") {
        //%X = zext i8 %t3 to i32
        dataReg = pool.getReg();
        buffer.emit("%" + dataReg + " = zext i8 %" + exp->reg + " to i32");
    }
    buffer.emit("%" + this->reg + " = add " + expType + " 0,%" +
                dataReg);//%reg= add i8 %r3, %r3
    string ptr = pool.getReg();
    buffer.emit("%" + ptr +
                " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    dataReg = reg;
    if (expType != "i32") {
        //%X = zext i8 %t3 to i32
        dataReg = pool.getReg();
        buffer.emit(
                "%" + dataReg + " = zext " + expType + " %" + reg + " to i32");
    }
    buffer.emit("store i32 %" + dataReg + ", i32* %" + ptr);
    //%ptr = getelementptr [10 x i32]*, [10 x i32]* %args, i32 0, i32 0
    //store i32 %t3, i32* %ptr
}


Statement::Statement(EnumType *enumType, Node *id, Exp *exp) {
    bool flag = false;
    vector<string> enumVals;
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    for (int i = enumsStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
            int len = enumType->value.length();
            if (enumType->value.compare(5, len - 5,enumsStack[i]->enumLines[j]->name) == 0) {
                flag = true;
                enumVals = enumsStack[i]->enumLines[j]->values;
                break;
            }
        }
    }

    if (flag == false) {
        output::errorUndefEnum(yylineno,
                enumType->value.substr(5,enumType->value.size() -5));
        exit(0);
    }
    if (identifierExists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    data = "null";
    for (int k = 0; k < enumVals.size(); ++k) {
        if (enumVals[k] == exp->value) {
            data = to_string(k);
            break;
        }
    }
if(exp->type==enumType->value){
    data = "X";
}
    if (data == "null" ) {
        for (int i = enumsStack.size() - 1; i >= 0; i--) {
            for (int j = 0; j < enumsStack[i]->enumLines.size(); ++j) {
                for (auto k:enumsStack[i]->enumLines[j]->values) {
                    if (k == exp->value) {
                        output::errorUndefEnumValue(yylineno, id->value);
                        exit(0);
                    }
                }
            }
        }
        output::errorUndefEnumValue(yylineno, id->value);
        exit(0);
    }

    int offset = offsetsStack.back()++;
    auto temp = shared_ptr<Entry>(new Entry(id->value, enumType->value, offset));
    tablesStack.back()->lines.emplace_back(temp);
    ///emitting code
    this->reg = pool.getReg();
    buffer.emit("%" + this->reg + " = add i32 0, %" + exp->reg);//%reg= add i8 r3, r3
    string ptr = pool.getReg();
    buffer.emit("%" + ptr + " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    string dataReg = reg;
    buffer.emit("store i32 %" + dataReg + ", i32* %" + ptr);
    //%ptr = getelementptr [10 x i32]*, [10 x i32]* %args, i32 0, i32 0
    //store i32 %t3, i32* %ptr
}

string doEmitting(string data, string type, int offset) {
    ///emitting code
    string reg = pool.getReg();
    string datareg = data;
    string argtype = get_LLVM_Type(type);
    if (argtype != "i32") {
        //%X = zext  i8 %t6 to i32
        datareg = pool.getReg();
        buffer.emit(
                "%" + datareg + " = zext " + argtype + " %" + data + " to i32");
    }
    buffer.emit("%" + reg + " = add i32 0,%" + datareg);//%reg= add i8 r3, r3
    string ptr = pool.getReg();
    if (offset >= 0) {
        buffer.emit(
                "%" + ptr +
                " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " +
                to_string(offset));
    } else if (offset < 0 && currFuncArgs > 0) {
        buffer.emit(
                "%" + ptr + " = getelementptr [ " + to_string(currFuncArgs) +
                " x i32], [ " +
                to_string(currFuncArgs) +
                " x i32]* %args, i32 0, i32 " +
                to_string(currFuncArgs + offset));
    } else cout << "FUCK" << endl;
    buffer.emit("store i32 %" + reg + ", i32* %" + ptr);
    return reg;
    //%ptr = getelementptr inbounds[10 x i32]* %args, i32 0, i32 0
    //store i32 %t3, i32* %ptr
}

//x= 15
Statement::Statement(Node *id, Exp *exp) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    for (int i = tablesStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j < tablesStack[i]->lines.size(); ++j) {//finding in symbol table
            if (tablesStack[i]->lines[j]->name == id->value) {
                if (tablesStack[i]->lines[j]->types.size() ==
                    1) {//making sure this is not a function
                    if ((tablesStack[i]->lines[j]->types[0] == "INT" && exp->type == "BYTE") ||
                        tablesStack[i]->lines[j]->types[0] == exp->type) {//checking types
                        data = exp->value;
                        this->instrc = exp->instrc;
                        this->reg = doEmitting(exp->reg, exp->type, tablesStack[i]->lines[j]->offset);
                        return;
                    } else {
                        if (tablesStack[i]->lines[j]->types[0].compare(0, 4, "enum") == 0) {
                            output::errorUndefEnumValue(yylineno, id->value);
                            exit(0);
                        }
                        output::errorMismatch(yylineno);
                        exit(0);
                    }
                } else {
                    output::errorUndef(yylineno, id->value);
                    exit(0);
                }
            }
        }
    }
    output::errorUndef(yylineno,
                       id->value);//id not found in the symbol table
    exit(0);
}

//%X = trunc i32 257 to i8
Statement::Statement(string retType) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    for (int i = tablesStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j <
                        tablesStack[i]->lines.size(); ++j) {//finding in symbol table
            if (tablesStack[i]->lines[j]->name == currFucn) {
                int size = tablesStack[i]->lines[j]->types.size();
                if (tablesStack[i]->lines[j]->types[size - 1] ==
                    retType) {//checking types
                    data = "ret val of void";
                    buffer.emit("ret void");
                    return;
                } else {
                    output::errorMismatch(yylineno);
                    exit(0);
                }
            }
        }
    }
    output::errorUndef(yylineno, "this is crazy");//should no reach this
    exit(0);
}

Statement::Statement(Exp *exp) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    if (exp->type == "VOID") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    string retType = exp->type;
    for (int i = tablesStack.size() - 1; i >= 0; i--) {
        for (int j = 0; j <
                        tablesStack[i]->lines.size(); ++j) {//finding in symbol table
            if (tablesStack[i]->lines[j]->name == currFucn) {
                int size = tablesStack[i]->lines[j]->types.size();
                if (tablesStack[i]->lines[j]->types[size - 1] ==
                    retType) {//checking types
                    data = exp->value;
                    string LLVMRetype = get_LLVM_Type(retType);
                    buffer.emit("ret " + LLVMRetype + " %" + exp->reg);
                    return;
                } else if (retType == "BYTE" &&
                           tablesStack[i]->lines[j]->types[size - 1] == "INT") {
                    data = exp->value; //allowing the case of retType to be byte in case it was int
                    string dataReg = pool.getReg();
                    buffer.emit("%" + dataReg + " = zext i8 %" + exp->reg +
                                " to i32");
                    buffer.emit("ret i32 %" + dataReg);
                    return;
                } else {
                    output::errorMismatch(yylineno);
                    exit(0);
                }
            }
        }
    }
    output::errorUndef(yylineno, "this is crazy2");//should no reach this
    exit(0);
}

Statement::Statement(Node *word) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    if (loopCount == 0) {
        if (word->value == "break") {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
        output::errorUnexpectedContinue(yylineno);
        exit(0);
    }
    int loc = buffer.emit("br label @");
    if (word->value == "break") {
        this->breakList = buffer.makelist({loc, FIRST});
    } else {
        this->continueList = buffer.makelist({loc, FIRST});
    }
    data = "this was a break or continue";
}

//    LBRACE Statements RBRACE
Statement::Statement(Statements *sts) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    data = "this was a block";
    this->continueList = sts->continueList;
    this->breakList = sts->breakList;
}


// handels if, if else, while
Statement::Statement(string str, Exp *exp, Statement *st) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    if (exp->type != "BOOL") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    data = "this was an if/ if else /while";
    if (st != nullptr) {
        this->continueList = st->continueList;
        this->breakList = st->breakList;
    }
}

Statement *addElseStatement(Statement *stIf, Statement *stElse) {
    stIf->breakList = buffer.merge(stIf->breakList, stElse->breakList);
    stIf->continueList = buffer.merge(stIf->continueList, stElse->continueList);
    return stIf;
}


//    EnumDecl
Statement::Statement(EnumDecl *enumDecl) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    value = "was enumdecl";
}

Statement::Statement(Call *call) {
    vector<pair<int, BranchLabelIndex>> listBreak;
    this->breakList = listBreak;
    vector<pair<int, BranchLabelIndex>> listContinue;
    this->continueList = listContinue;
    data = "this was a call for a function";
}

Statements::Statements(Statement *st) {
    this->breakList = st->breakList;
    this->continueList = st->continueList;
}

Statements::Statements(Statements *sts, Statement *st) {
    this->breakList = buffer.merge(sts->breakList, st->breakList);
    this->continueList = buffer.merge(sts->continueList, st->continueList);
}

Program::Program() {
    shared_ptr<SymbolTable> global = shared_ptr<SymbolTable>(new SymbolTable);
    auto globalEnum = shared_ptr<EnumTable>(new EnumTable());
    const vector<string> temp = {"STRING", "VOID"};
    auto print = shared_ptr<Entry>(new Entry("print", temp, 0));
    const vector<string> temp2 = {"INT", "VOID"};//to do arg of byte also
    auto printi = shared_ptr<Entry>(new Entry("printi", temp2, 0));
    global->lines.emplace_back(print);
    global->lines.emplace_back(printi);
    tablesStack.emplace_back(global);
    enumsStack.emplace_back(globalEnum);
    offsetsStack.emplace_back(0);
    ///finished symbol table, handling the declarations and defines

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



/*target triple = "i386-pc-linux-gnu"
@.str = private constant [15 x i8] c"hello, world!\0A\00"
define i32 @foo(i32, i32) {
%args= alloca [10 x i32]
%ptr = getelementptr inbounds[10 x i32]* %args, i32 0, i32 0
%t3= add i8 5,2
%X = zext i8 %t3 to i32
store i32 %X, i32* %ptr
%val = load i32* %ptr
  %str = getelementptr inbounds [15 x i8]* @.str, i32 0, i32 %val
  %call = call i32 (i8*, ...)* @printf(i8* %str)
ret i32 1
}
define i32 @main() {
entry:
  %whhh= add i32 0,1
  %call = call i32 (i32,i32)* @foo(i32 %whhh,i32 %whhh)
  ret i32 1
}
declare i32 @printf(i8*, ...)
*/