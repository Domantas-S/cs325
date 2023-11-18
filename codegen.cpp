// #include "codegen.hpp"
#include "astnode.hpp"


using namespace llvm;

Value* lazyAnd(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok){
    return nullptr;
}

Value* lazyOr(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok){
    return nullptr;
}

Type* getWidestType(Type* type1, Type* type2){
    // hierarchy of types: float > int > bool
    if(type1 == type2){
        return type1;
    }
    if(type1 == Type::getFloatTy(TheContext) || type2 == Type::getFloatTy(TheContext)){
        return Type::getFloatTy(TheContext);
    }
    if(type1 == Type::getInt32Ty(TheContext) || type2 == Type::getInt32Ty(TheContext)){
        return Type::getInt32Ty(TheContext);
    }
    return Type::getInt1Ty(TheContext); // bool
}

std::string typeToString(Type* type){
    if(type == Type::getFloatTy(TheContext)){
        return "float";
    }
    if(type == Type::getInt32Ty(TheContext)){
        return "int";
    }
    if(type == Type::getInt1Ty(TheContext)){
        return "bool";
    }
    if(type == Type::getVoidTy(TheContext)){
        return "void";
    }
    return "ERROR CONVERTING TYPE TO STRING";
}

Value* castToType(Value* val, Type* type, TOKEN tok){
    // check for same type
    if(val->getType() == type){
        return val;
    }
    // narrowing conversions
    if(val->getType() == Type::getFloatTy(TheContext) && type == Type::getInt32Ty(TheContext)){
        addWarning(tok, "Narrowing conversion from float to int");
        return Builder.CreateFPToSI(val, type, "FPtoSIcast");  // floating point to signed int
    }
    if(val->getType() == Type::getFloatTy(TheContext) && type == Type::getInt1Ty(TheContext)){
        addWarning(tok, "Narrowing conversion from float to bool");
        return Builder.CreateFPToSI(val, type, "FPtoBcast");  // floating point to bool
    }
    if(val->getType() == Type::getInt32Ty(TheContext) && type == Type::getInt1Ty(TheContext)){
        addWarning(tok, "Narrowing conversion from int to bool");
        return Builder.CreateIntCast(val, type, true, "SItoBcast");  // int to bool
    }

    // widening conversions
    if(val->getType() == Type::getInt32Ty(TheContext) && type == Type::getFloatTy(TheContext)){
        return Builder.CreateSIToFP(val, type, "SItoFPcast");  // signed int to float
    }
    if(val->getType() == Type::getInt1Ty(TheContext) && type == Type::getFloatTy(TheContext)){
        return Builder.CreateSIToFP(val, type, "BtoFPcast");  // bool to float
    }
    if(val->getType() == Type::getInt1Ty(TheContext) && type == Type::getInt32Ty(TheContext)){
        return Builder.CreateIntCast(val, type, true, "BtoSIcast");  // bool to int
    }
    error(tok, "Unsupported cast of " + typeToString(val->getType())+ " to " + typeToString(type));
    return nullptr;
}

// generating allocas from slides
AllocaInst * CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, Type *type) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, 0, VarName.c_str());
}