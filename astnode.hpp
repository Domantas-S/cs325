#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "token.hpp"

using namespace llvm;

class ASTnode
{
public:
    virtual ~ASTnode(){};
    virtual llvm::Value *codegen() = 0;
    virtual std::string to_string() const;
    virtual std::string to_tree(std::string prefix, bool end) const = 0;
};

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::vector<std::map<std::string, AllocaInst *>> NamedValues;  // local var tables, cleared at end of blocks
static std::map<std::string, GlobalVariable*> GlobalNamedValues; // global var table
AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, Type *type);
Value *castToType(Value *val, Type *type, TOKEN tok);
Type *getWidestType(Type *type1, Type *type2);
std::string typeToString(Type *type);
Type* getLLVMType(std::string Val);
Value* lazyAnd(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok);
Value* lazyOr(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok);




// Generate LHS, if false, immediately jump to end and return false, otherwise generate RHS, and return true if RHS is true
// It's lazy because there is NO LEFT RECURSION so only RHS would be "nested" in this case
Value *lazyAnd(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok)
{
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* LHSBB = BasicBlock::Create(TheContext, "lhs", TheFunction);
    BasicBlock* RHSBB = BasicBlock::Create(TheContext, "rhs", TheFunction);
    BasicBlock* EndBB = BasicBlock::Create(TheContext, "end", TheFunction);
    BasicBlock* SetFalseBB = BasicBlock::Create(TheContext, "setfalse", TheFunction);
    BasicBlock* SetTrueBB = BasicBlock::Create(TheContext, "settrue", TheFunction);

    // temp variable to store result of lazy and
    AllocaInst* temp = CreateEntryBlockAlloca(TheFunction, "andtmp", Type::getInt1Ty(TheContext));

    Builder.CreateBr(LHSBB);
    Builder.SetInsertPoint(LHSBB);
    Value* L = LHS->codegen();
    if(!L){
        error(tok, "Error in lazyAnd(): L is nullptr");
    }
    if(L->getType()->isVoidTy()){
        error(tok, "LHS is void! Cannot perform operation");
    }
    // convert result to bool
    L = castToType(L, Type::getInt1Ty(TheContext), tok);
    
    // branch to RHS if L is true, otherwise branch to set temp variable to false and end
    Builder.CreateCondBr(L, RHSBB, SetFalseBB);
    Builder.SetInsertPoint(RHSBB);
    Value* R = RHS->codegen();
    if(!R){
        error(tok, "Error in lazyAnd(): R is nullptr");
    }
    if(R->getType()->isVoidTy()){
        error(tok, "RHS is void! Cannot perform operation");
    }
    // convert result to bool
    R = castToType(R, Type::getInt1Ty(TheContext), tok);

    Builder.CreateCondBr(R, SetTrueBB, SetFalseBB);

    // set temp variable to true
    Builder.SetInsertPoint(SetTrueBB);
    Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 1, true)), temp);
    Builder.CreateBr(EndBB);

    // set temp variable to false
    Builder.SetInsertPoint(SetFalseBB);
    Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 0, true)), temp);
    Builder.CreateBr(EndBB);

    // exit and return temp variable
    Builder.SetInsertPoint(EndBB);
    return Builder.CreateLoad(Type::getInt1Ty(TheContext), temp, "andtmp");
}

// Same principle as lazy and
Value *lazyOr(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok)
{
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* LHSBB = BasicBlock::Create(TheContext, "lhs", TheFunction);
    BasicBlock* RHSBB = BasicBlock::Create(TheContext, "rhs", TheFunction);
    BasicBlock* EndBB = BasicBlock::Create(TheContext, "end", TheFunction);
    BasicBlock* SetTrueBB = BasicBlock::Create(TheContext, "settrue", TheFunction);
    BasicBlock* SetFalseBB = BasicBlock::Create(TheContext, "setfalse", TheFunction);

    // temp variable to store result of lazy or
    AllocaInst* temp = CreateEntryBlockAlloca(TheFunction, "ortmp", Type::getInt1Ty(TheContext));

    Builder.CreateBr(LHSBB);
    Builder.SetInsertPoint(LHSBB);
    Value* L = LHS->codegen();
    if(!L){
        error(tok, "Error in lazyOr(): L is nullptr");
    }
    if(L->getType()->isVoidTy()){
        error(tok, "LHS is void! Cannot perform operation");
    }
    // convert result to bool
    L = castToType(L, Type::getInt1Ty(TheContext), tok);
    
    // branch to set temp variable to true and end if L is true, otherwise branch to RHS
    Builder.CreateCondBr(L, SetTrueBB, RHSBB);
    Builder.SetInsertPoint(RHSBB);
    Value* R = RHS->codegen();
    if(!R){
        error(tok, "Error in lazyOr(): R is nullptr");
    }
    if(R->getType()->isVoidTy()){
        error(tok, "RHS is void! Cannot perform operation");
    }
    // convert result to bool
    R = castToType(R, Type::getInt1Ty(TheContext), tok);

    Builder.CreateCondBr(R, SetTrueBB, SetFalseBB);

    // set temp variable to true
    Builder.SetInsertPoint(SetTrueBB);
    Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 1, true)), temp);
    Builder.CreateBr(EndBB);

    // set temp variable to false
    Builder.SetInsertPoint(SetFalseBB);
    Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 0, true)), temp);
    Builder.CreateBr(EndBB);

    // exit and return temp variable
    Builder.SetInsertPoint(EndBB);
    return Builder.CreateLoad(Type::getInt1Ty(TheContext), temp, "ortmp");
}

Type *getWidestType(Type *type1, Type *type2)
{
    // hierarchy of types: float > int > bool
    if (type1 == type2)
    {
        return type1;
    }
    if (type1 == Type::getFloatTy(TheContext) || type2 == Type::getFloatTy(TheContext))
    {
        return Type::getFloatTy(TheContext);
    }
    if (type1 == Type::getInt32Ty(TheContext) || type2 == Type::getInt32Ty(TheContext))
    {
        return Type::getInt32Ty(TheContext);
    }
    return Type::getInt1Ty(TheContext); // bool
}

std::string typeToString(Type *type)
{
    if (type == Type::getFloatTy(TheContext))
    {
        return "float";
    }
    if (type == Type::getInt32Ty(TheContext))
    {
        return "int";
    }
    if (type == Type::getInt1Ty(TheContext))
    {
        return "bool";
    }
    if (type == Type::getVoidTy(TheContext))
    {
        return "void";
    }
    fprintf(stdout, "Type: %u\n", type->getTypeID());
    return "ERROR CONVERTING LLVM TYPE TO STRING";
}

Type* getLLVMType(std::string Val) {
        if (Val == "int")
        {
            return Type::getInt32Ty(TheContext);
        }
        else if (Val == "float")
        {
            return Type::getFloatTy(TheContext);
        }
        else if (Val == "bool")
        {
            return Type::getInt1Ty(TheContext);
        }
        else if (Val == "void")
        {
            return Type::getVoidTy(TheContext);
        }
        else
        {
            return nullptr;
        }
    }

Value *castToType(Value *val, Type *type, TOKEN tok)
{
    // check for same type
    if (val->getType() == type)
    {
        return val;
    }
    // narrowing conversions
    if (val->getType() == Type::getFloatTy(TheContext) && type == Type::getInt32Ty(TheContext))
    {
        addWarning(tok, "Narrowing conversion from float to int");
        return Builder.CreateFPToSI(val, type, "FPtoSIcast"); // floating point to signed int
    }
    if (val->getType() == Type::getFloatTy(TheContext) && type == Type::getInt1Ty(TheContext))
    {
        addWarning(tok, "Narrowing conversion from float to bool");
        return Builder.CreateFPToSI(val, type, "FPtoBcast"); // floating point to bool
    }
    if (val->getType() == Type::getInt32Ty(TheContext) && type == Type::getInt1Ty(TheContext))
    {
        addWarning(tok, "Narrowing conversion from int to bool");
        return Builder.CreateIntCast(val, type, true, "SItoBcast"); // int to bool
    }

    // widening conversions
    if (val->getType() == Type::getInt32Ty(TheContext) && type == Type::getFloatTy(TheContext))
    {
        return Builder.CreateSIToFP(val, type, "SItoFPcast"); // signed int to float
    }
    if (val->getType() == Type::getInt1Ty(TheContext) && type == Type::getFloatTy(TheContext))
    {
        return Builder.CreateSIToFP(val, type, "BtoFPcast"); // bool to float
    }
    if (val->getType() == Type::getInt1Ty(TheContext) && type == Type::getInt32Ty(TheContext))
    {
        return Builder.CreateIntCast(val, type, false, "BtoSIcast"); // bool to int
    }
    error(tok, "Unsupported cast of " + typeToString(val->getType()) + " to " + typeToString(type));
    return nullptr;
}

// generating allocas from slides
AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, Type *type)
{
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, 0, VarName.c_str());
}


class IntASTnode : public ASTnode
{
    int Val;
    TOKEN Tok;

public:
    IntASTnode(int val, TOKEN tok) : Val(val), Tok(tok) {}
    virtual ~IntASTnode() {}
    Value *codegen() { return ConstantInt::get(TheContext, APInt(32, Val, true)); };

    std::string to_string() const
    {
        return "Int: " + std::to_string(Val) + "\n";
    }

    // L for last child, T otherwise (more siblings)
    std::string to_tree(std::string prefix, bool end) const
    {
        return prefix + (end ? "└── " : "├── ") + this->to_string();
    }
};

class FloatASTnode : public ASTnode
{
    float Val;
    TOKEN Tok;
    std::string Name;

public:
    FloatASTnode(float val, TOKEN tok) : Val(val), Tok(tok) {}
    virtual ~FloatASTnode() {}
    Value *codegen() { return ConstantFP::get(TheContext, APFloat(Val)); };

    std::string to_string() const
    {
        return "Float: " + std::to_string(Val) + "\n";
    }

    std::string to_tree(std::string prefix, bool end) const
    {
        return prefix + (end ? "└── " : "├── ") + this->to_string();
    }
};

class BoolASTnode : public ASTnode
{
    bool Val;
    TOKEN Tok;
    std::string Name;

public:
    BoolASTnode(bool val, TOKEN tok) : Val(val), Tok(tok) {}
    virtual ~BoolASTnode() {}
    Value *codegen() { return ConstantInt::get(TheContext, APInt(1, Val, true)); };
    std::string to_string() const
    {
        return "Bool: " + std::to_string(Val) + "\n";
    }

    std::string to_tree(std::string prefix, bool end) const
    {
        return prefix + (end ? "└── " : "├── ") + this->to_string();
    }
};

class TypeASTnode : public ASTnode
{
    TOKEN Tok;

public:
    std::string Val;
    TypeASTnode(std::string val, TOKEN tok) : Val(val), Tok(tok) {}
    virtual ~TypeASTnode() {}
    Value *codegen() { return nullptr; };
    Type *getType() { return getLLVMType(Val); }
    std::string to_string() const { return "Type: " + Val + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        return prefix + (end ? "└── " : "├── ") + this->to_string();
    }
};

class BinOpNode : public ASTnode
{
    TOKEN Tok;
    std::string Op;
    std::unique_ptr<ASTnode> LHS, RHS;

public:
    BinOpNode(std::string op, std::unique_ptr<ASTnode> lhs, std::unique_ptr<ASTnode> rhs, TOKEN tok) : Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)), Tok(tok) {}
    virtual ~BinOpNode() {}
    Value *codegen()
    {
        // lazy operations handled separately
        if (Op == "||")
        {
            return lazyOr(std::move(LHS), std::move(RHS), Tok);
        }
        else if (Op == "&&")
        {
            return lazyAnd(std::move(LHS), std::move(RHS), Tok);
        }

        Value *L = LHS->codegen();
        Value *R = RHS->codegen(); // RHS IS NULL IN ADDITION TEST!!!!!!!!

        // check for void types
        if (L->getType()->isVoidTy())
        {
            error(Tok, "LHS is void! Cannot perform operation");
        }
        if (R->getType()->isVoidTy())
        {
            error(Tok, "RHS is void! Cannot perform operation");
        }

        // check for mismatched types
        if (L->getType() != R->getType())
        {
            // convert to widest
            Type *widest = getWidestType(L->getType(), R->getType());
            L = castToType(L, widest, Tok);
            R = castToType(R, widest, Tok);
        }

        switch (Tok.type)
        {
        case PLUS:
            if(L->getType()->isIntegerTy())
                return Builder.CreateAdd(L, R, "addtmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFAdd(L, R, "addtmp");
            else
                error(Tok, "Cannot add " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case MINUS:
            if(L->getType()->isIntegerTy())
                return Builder.CreateSub(L, R, "subtmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFSub(L, R, "subtmp");
            else
                error(Tok, "Cannot subtract " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case ASTERIX:
            if(L->getType()->isIntegerTy())
                return Builder.CreateMul(L, R, "multmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFMul(L, R, "multmp");
            else
                error(Tok, "Cannot multiply " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case DIV:
            // division by zero error
            if(ConstantFP* floatVal = dyn_cast<ConstantFP>(R)){
                if(floatVal->isZero()){
                    error(Tok, "Division by zero");
                }
            }
            else if(ConstantInt* intVal = dyn_cast<ConstantInt>(R)){
                if(intVal->isZero()){
                    error(Tok, "Division by zero");
                }
            }

            if(L->getType()->isIntegerTy())
                return Builder.CreateSDiv(L, R, "divtmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFDiv(L, R, "divtmp");
            else
                error(Tok, "Cannot divide " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;

        case MOD:
            // division by zero error
            if(ConstantFP* floatVal = dyn_cast<ConstantFP>(R)){
                if(floatVal->isZero()){
                    error(Tok, "Division by zero");
                }
            }
            else if(ConstantInt* intVal = dyn_cast<ConstantInt>(R)){
                if(intVal->isZero()){
                    error(Tok, "Division by zero");
                }
            }

            if(L->getType()->isIntegerTy())
                return Builder.CreateSRem(L, R, "modtmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFRem(L, R, "modtmp");
            else
                error(Tok, "Cannot mod " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;

        case LT:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpSLT(L, R, "lttmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpOLT(L, R, "lttmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;

        case GT:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpSGT(L, R, "gttmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpOGT(L, R, "gttmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case LE:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpSLE(L, R, "letmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpOLE(L, R, "letmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case GE:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpSGE(L, R, "getmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpOGE(L, R, "getmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        case EQ:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpEQ(L, R, "eqtmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpOEQ(L, R, "eqtmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;

        case NE:
            if(L->getType()->isIntegerTy())
                return Builder.CreateICmpNE(L, R, "netmp");
            else if(L->getType()->isFloatingPointTy())
                return Builder.CreateFCmpONE(L, R, "netmp");
            else
                error(Tok, "Cannot compare " + typeToString(L->getType()) + " and " + typeToString(R->getType()));
            break;
        
        default:
            error(Tok, "Unknown binary operator");
        }
        return nullptr;
    };

    std::string to_string() const { return "BinOp: " + Op + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        return out + LHS->to_tree(new_prefix, false) + RHS->to_tree(new_prefix, true);
    }
};

class UnaryOpNode : public ASTnode
{
    TOKEN Tok;
    std::string Op;
    std::unique_ptr<ASTnode> RHS;

public:
    UnaryOpNode(std::string op, std::unique_ptr<ASTnode> rhs, TOKEN tok) : Op(op), RHS(std::move(rhs)), Tok(tok) {}
    virtual ~UnaryOpNode() {}
    
    Value *codegen() {
        Value* R = RHS->codegen();
        if(!R){
            error(Tok, "Error in UnaryOpNode::codegen(): R is nullptr");
        }
        if(R->getType()->isVoidTy()){
            error(Tok, "RHS is void! Cannot perform operation");
        }
        if(Op == "!"){
            if(R->getType()->isIntegerTy()){
                return Builder.CreateNot(R, "nottmp");  // hope behaviour is same as C: 0 -> 1 and non-zero -> 0
            }
            else if(R->getType()->isFloatingPointTy()){
                // cast to bool
                Value* casted = castToType(R, Type::getInt1Ty(TheContext), Tok);
                return Builder.CreateNot(casted, "nottmp"); // hope behaviour is same as C: 0.0 -> 1 and non-zero -> 0
            }
            else{
                error(Tok, "Unknown type for ! operator");
                return nullptr;
            }
        }
        else if(Op == "-"){
            if(R->getType()->isIntegerTy()){
                return Builder.CreateNeg(R, "negtmp");
            }
            else if(R->getType()->isFloatingPointTy()){
                return Builder.CreateFNeg(R, "negtmp");
            }
            else{   // cast bool to int and then negate
                Value* casted = castToType(R, Type::getInt32Ty(TheContext), Tok);
                return Builder.CreateNeg(casted, "negtmp");
            }
        }
        else{
            error(Tok, "Unknown unary operator");
        }
        return nullptr;
    };
    
    std::string to_string() const { return "UnaryOp: " + Op + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        return out + RHS->to_tree(prefix + (end ? "    " : "│   "), true);
    }
};

class ParamASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<TypeASTnode> TypeNode;
    std::string Name;

public:
    ParamASTnode(std::unique_ptr<TypeASTnode> type, std::string name, TOKEN tok) : TypeNode(std::move(type)), Name(name), Tok(tok) {}
    virtual ~ParamASTnode() {}
    Value *codegen() { 
        // codegen for parameters are handled in FunctionASTnode::codegen() and ExternASTnode::codegen() 
        // since they need to be generated in the context of the function/extern
        return nullptr; 
    };
    Type* getType() { return TypeNode->getType(); }
    std::string getName() { return Name; }
    std::string to_string() const { return "Param: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        return out + TypeNode->to_tree(prefix + (end ? "    " : "│   "), true);
    }
};

// defining a function (with a body)
class FunctionASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<TypeASTnode> TypeNode;
    std::string Name;
    std::vector<std::unique_ptr<ParamASTnode>> Params;
    std::unique_ptr<ASTnode> Body;

public:
    FunctionASTnode(std::unique_ptr<TypeASTnode> type,
                    std::string name,
                    std::vector<std::unique_ptr<ParamASTnode>> params,
                    std::unique_ptr<ASTnode> body, TOKEN tok) : TypeNode(std::move(type)),
                                                                Name(name),
                                                                Params(std::move(params)),
                                                                Body(std::move(body)),
                                                                Tok(tok) {}

    virtual ~FunctionASTnode() {}
    
    Value *codegen() {
        // check if function already exists, prevent function overloading
        Function *F = TheModule->getFunction(Name);
        if (F)
        {
            error(Tok, "Function " + Name + " already exists or trying to overload the function which is not allowed.");
            return nullptr;
        }

        // create param types
        std::vector<Type *> paramTypes;
        for (auto &p : Params)
        {
            paramTypes.push_back(p->getType());
        }

        // create function type
        FunctionType *FT = FunctionType::get(TypeNode->getType(), paramTypes, false);
        F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
        Builder.SetInsertPoint(BasicBlock::Create(TheContext, "entry", F));

        // set var table for function
        NamedValues.push_back(std::map<std::string, AllocaInst *>());

        // fprintf(stdout, "DEBUG: FunctionASTnode::codegen(): Function parameters handled\n");
        // handle function parameters
        unsigned Idx = 0;
        for (auto &Arg : F->args()){
            Arg.setName(Params[Idx]->getName());
            // fprintf(stdout, "DEBUG: %s\n", Arg.getName().str().c_str());
            AllocaInst *Alloca = CreateEntryBlockAlloca(F, Arg.getName().str(), Arg.getType());
            Builder.CreateStore(&Arg, Alloca);
            NamedValues.back()[Arg.getName().str()] = Alloca;
            Idx++;
        }

        // generate function body
        Value* RetVal = Body->codegen();
        // check if function ends with a return
        if(verifyFunction(*F)){ // If there are no errors, the verifyFunction returns false
            // fprintf(stdout, "DEBUG: FunctionASTnode::codegen(): Function verified\n");
            // create return instruction
            if (TypeNode.get()->getType() == Type::getVoidTy(TheContext))
            {
                // fprintf(stdout, "DEBUG: FunctionASTnode::codegen(): Void function\n");
                Builder.CreateRetVoid();
            }
            else
            {
                // fprintf(stdout, "DEBUG: Return value type: %s\n", typeToString(RetVal->getType()).c_str());
                Builder.CreateRet(Constant::getNullValue(TypeNode->getType()));
            }
        }

        // clear local vars
        NamedValues.pop_back();
        return F;
    };
    
    std::string to_string() const { return "Function: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        out += TypeNode->to_tree(new_prefix, false);
        for (auto &p : Params)
        {
            out += p->to_tree(new_prefix, false);
        }
        return out + Body->to_tree(new_prefix, true);
    }
};

class ExternASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<TypeASTnode> TypeNode;
    std::string Name;
    std::vector<std::unique_ptr<ParamASTnode>> Params;

public:
    ExternASTnode(std::unique_ptr<TypeASTnode> type,
                  std::string name,
                  std::vector<std::unique_ptr<ParamASTnode>> params,
                  TOKEN tok) : TypeNode(std::move(type)),
                               Name(name),
                               Params(std::move(params)),
                               Tok(tok) {}

    virtual ~ExternASTnode() {}
    Value *codegen() { 
        // check if function already exists, prevent function overloading
        Function *F = TheModule->getFunction(Name);
        if (F)
        {
            error(Tok, "Function " + Name + " already exists or trying to overload the function which is not allowed.");
            return nullptr;
        }

        // create param types
        std::vector<Type *> paramTypes;
        for (auto &p : Params)
        {
            paramTypes.push_back(p->getType());
        }

        // create function type
        FunctionType *FT = FunctionType::get(TypeNode->getType(), paramTypes, false);
        F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

        // set names for all arguments
        unsigned Idx = 0;
        for (auto &Arg : F->args())
        {
            Arg.setName(Params[Idx++]->getName());
        }

        return F;    
    };

    std::string to_string() const { return "Extern: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        out += TypeNode->to_tree(new_prefix, false);
        for (auto &p : Params)
        {
            out += p->to_tree(new_prefix, p == Params.back());
        }
        return out;
    }
};

class IfASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<ASTnode> Cond;
    std::unique_ptr<ASTnode> Then;
    std::unique_ptr<ASTnode> Else;

public:
    IfASTnode(std::unique_ptr<ASTnode> cond, std::unique_ptr<ASTnode> then, std::unique_ptr<ASTnode> else_, TOKEN tok) : Cond(std::move(cond)), Then(std::move(then)), Else(std::move(else_)), Tok(tok) {}
    Value *codegen() { 
        Value* CondV = Cond->codegen();
        if(!CondV){
            error(Tok, "Error in IfASTnode::codegen(): CondV is nullptr");
        }
        else if (CondV->getType()->isVoidTy()){
            error(Tok, "Condition is void which cannot be used for if condition!");
        }

        // convert condition to bool
        castToType(CondV, Type::getInt1Ty(TheContext), Tok);

        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        BasicBlock* thenBlock = BasicBlock::Create(TheContext, "then", TheFunction);
        BasicBlock* elseBlock = BasicBlock::Create(TheContext, "else");
        BasicBlock* mergeBlock = BasicBlock::Create(TheContext, "ifcont");

        // generate else branching
        if (Else){
            Builder.CreateCondBr(CondV, thenBlock, elseBlock);
        }
        else{
            Builder.CreateCondBr(CondV, thenBlock, mergeBlock);
        }

        // generate then block
        Builder.SetInsertPoint(thenBlock);
        Value* ThenV = Then->codegen();
        //  if (!ThenV)
        //     return nullptr;

        Builder.CreateBr(mergeBlock);
        thenBlock = Builder.GetInsertBlock();

        // generate else block
        if (Else){
            TheFunction->insert(TheFunction->end(), elseBlock);
            Builder.SetInsertPoint(elseBlock);
            Value* ElseV = Else->codegen();
            // if (!ElseV)
            //     return nullptr;
        }
        Builder.CreateBr(mergeBlock);

        TheFunction->insert(TheFunction->end(), mergeBlock);
        Builder.SetInsertPoint(mergeBlock);
        return nullptr;
    };

    std::string to_string() const { return "If \n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        out += Cond->to_tree(new_prefix, false);
        if (Else)
        {
            out += Then->to_tree(new_prefix, false);
            return out + Else->to_tree(new_prefix, true);
        }
        else
        {
            return out + Then->to_tree(new_prefix, true);
        }
    }
};

class WhileASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<ASTnode> Cond;
    std::unique_ptr<ASTnode> Body;

public:
    WhileASTnode(std::unique_ptr<ASTnode> cond, std::unique_ptr<ASTnode> body, TOKEN tok) : Cond(std::move(cond)), Body(std::move(body)), Tok(tok) {}
    Value *codegen() { 
        // fprintf(stdout, "DEBUG: WhileASTnode::codegen()\n");
        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        BasicBlock* condBlock = BasicBlock::Create(TheContext, "cond", TheFunction);
        BasicBlock* bodyBlock = BasicBlock::Create(TheContext, "body", TheFunction);
        BasicBlock* exitBlock = BasicBlock::Create(TheContext, "exitwhile", TheFunction);

        // generate condition block
        Builder.CreateBr(condBlock);
        Builder.SetInsertPoint(condBlock);
        
        Value* CondV = Cond->codegen();
        if(!CondV){
            error(Tok, "Error in WhileASTnode::codegen(): CondV is nullptr");
        }
        else if (CondV->getType()->isVoidTy()){
            error(Tok, "Condition is void which cannot be used for while condition!");
        }

        // convert condition to bool
        castToType(CondV, Type::getInt1Ty(TheContext), Tok);

        // generate body branching
        Builder.CreateCondBr(CondV, bodyBlock, exitBlock);
        Builder.SetInsertPoint(bodyBlock);
        Value* BodyV = Body->codegen();
        Builder.CreateBr(condBlock);    // go back to condition block
        Builder.SetInsertPoint(exitBlock); // set insert point to exit loop
        return nullptr;
    };
    
    std::string to_string() const { return "While \n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        out += Cond->to_tree(new_prefix, false);
        return out + Body->to_tree(new_prefix, true);
    }
};

class ReturnASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<ASTnode> Val;

public:
    ReturnASTnode(std::unique_ptr<ASTnode> val, TOKEN tok) : Val(std::move(val)), Tok(tok) {}
    virtual ~ReturnASTnode() {}
    Value *codegen() { // TODO CHECK IF RETURN TYPE MATCHES FUNCTION SIGNATURE
        Function* TheFunction = Builder.GetInsertBlock()->getParent();

        if(!Val){
            return Builder.CreateRetVoid();
        }
        Value* V = Val->codegen();
        // if (V->getType() != TheFunction->getReturnType())
        // {
        //     error(Tok, "Return type of function " + TheFunction->getName().str() + " does not match function signature");
        //     return nullptr;
        // }
        return Builder.CreateRet(V);
    };
    
    std::string to_string() const { return "Return: \n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└──" : "├── ") + this->to_string();
        if (!Val)
        { // checking for nullptr (nothing returned)
            return out + prefix + (end ? "    " : "│   ") + "└──" + "NoRetVal\n";
        }
        else
        {
            return out + Val->to_tree(prefix + (end ? "    " : "│   "), true);
        }
    }
};

class CallASTnode : public ASTnode
{
    TOKEN Tok;
    std::string Callee;
    std::vector<std::unique_ptr<ASTnode>> Args;

public:
    CallASTnode(std::string callee, std::vector<std::unique_ptr<ASTnode>> args, TOKEN tok) : Callee(callee), Args(std::move(args)), Tok(tok) {}
    virtual ~CallASTnode() {}
    Value *codegen() { 
        // check if function exists
        Function *CalleeF = TheModule->getFunction(Callee);
        if (!CalleeF)
        {
            error(Tok, "Unknown function referenced");
            return nullptr;
        }

        // check if number of args match
        if (CalleeF->arg_size() != Args.size())
        {
            error(Tok, "Incorrect number of arguments passed to function " + Callee);
            return nullptr;
        }



        // generate code for args and check if types match
        unsigned Idx = 0;
        std::vector<Value *> ArgsV;
        for (auto &a : Args)
        {
            Value *argVal = a->codegen();
            if (argVal->getType() != CalleeF->getArg(Idx)->getType())
            {
                error(Tok, "Incorrect type of argument index " + std::to_string(Idx) + " passed to function " + Callee + "\n\tExpected: " + typeToString(CalleeF->getArg(Idx)->getType()) + " but got: " + typeToString(argVal->getType()));
                return nullptr;
            }
            ArgsV.push_back(argVal);
        }

        return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
    };
    std::string to_string() const { return "FuncCall: " + Callee + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        for (auto &a : Args)
        {
            out += a->to_tree(prefix + (end ? "    " : "│   "), a == Args.back()); // only true when arg is last one
        }
        return out;
    }
};

// declaring a new variable for the first time
class VarDeclASTnode : public ASTnode
{
    TOKEN Tok;
    std::unique_ptr<TypeASTnode> Type;
    std::string Name;
    

public:
    VarDeclASTnode(std::unique_ptr<TypeASTnode> type, std::string name, TOKEN tok) : Type(std::move(type)), Name(name), Tok(tok) {}
    virtual ~VarDeclASTnode() {}
    Value *codegen() { 
        // global variables are allowed to be declared once. they are declared at the start of the file
        // re-declaration of a global variable within a local scope is allowed
        
        // check if global variable already exists
        if (NamedValues.size() == 0){   // no local context available
            if (GlobalNamedValues.find(Name) != GlobalNamedValues.end())
            {
                error(Tok, "Global variable " + Name + " already exists");
                return nullptr;
            }
            // create global variable
            GlobalNamedValues[Name] = new GlobalVariable(*TheModule, Type->getType(), false, GlobalValue::CommonLinkage, Constant::getNullValue(Type->getType()), Name);
            return GlobalNamedValues[Name];
        }
        
        Function *TheFunction = Builder.GetInsertBlock()->getParent();
        // check if local variable already exists in CURRENT context
        if (NamedValues.back().find(Name) != NamedValues.back().end())
        {
            error(Tok, "Variable " + Name + " already exists in current context");
            return nullptr;
        }
        
        // create local variable
        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Name, Type->getType());
        NamedValues.back()[Name] = Alloca;
        return Alloca;
    };
    
    std::string to_string() const { return "Decl: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        return out + Type->to_tree(prefix + (end ? "    " : "│   "), true);
    }
};

class ProgramASTnode : public ASTnode
{
    std::vector<std::unique_ptr<ExternASTnode>> externs;
    std::vector<std::unique_ptr<ASTnode>> decls;

public:
    ProgramASTnode(std::vector<std::unique_ptr<ExternASTnode>> externs, std::vector<std::unique_ptr<ASTnode>> decls) : externs(std::move(externs)), decls(std::move(decls)) {}
    virtual ~ProgramASTnode() {}
    Value *codegen()
    {
        for (auto &e : externs)
        {
            e->codegen();
        }
        for (auto &d : decls)
        {
            d->codegen();
        }
        return nullptr;
    };

    std::string to_string() const { return "Program \n"; }

    std::string to_tree(std::string prefix = "", bool end = true) const
    {
        std::string out = this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        for (auto &e : externs)
        {
            out += e->to_tree(new_prefix, false);
        }
        for (auto &d : decls)
        {
            out += d->to_tree(new_prefix, d == decls.back());
        }
        return out;
    }
};

class BlockASTnode : public ASTnode
{
    TOKEN Tok;
    std::vector<std::unique_ptr<ASTnode>> local_decls;
    std::vector<std::unique_ptr<ASTnode>> stmt_list;

public:
    BlockASTnode(std::vector<std::unique_ptr<ASTnode>> local_decls, std::vector<std::unique_ptr<ASTnode>> stmt_list, TOKEN tok) : local_decls(std::move(local_decls)), stmt_list(std::move(stmt_list)), Tok(tok) {}
    virtual ~BlockASTnode() {}

    Value *codegen() { 
        // fprintf(stdout, "DEBUG: BlockASTnode::codegen()\n");
        // check if in a function
        Function *TheFunction = Builder.GetInsertBlock()->getParent();

        // create new local context
        NamedValues.push_back(std::map<std::string, AllocaInst *>());

        // generate code for local declarations
        for (auto &l : local_decls)
        {
            l->codegen();
        }
        // generate code for statements
        for (auto &s : stmt_list)
        {
            Value* val = s->codegen();
            // check if statement is a return, if it is, then don't generate code for the rest of the block
            if (val && isa<ReturnInst>(val))
            {
                break;
            }
        }
        
        // clear local context
        NamedValues.pop_back();
        return nullptr;
    };

    std::string to_string() const { return "Block: \n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        for (auto &l : local_decls)
        {
            out += l->to_tree(new_prefix, false);
        }
        for (auto &s : stmt_list)
        {
            out += s->to_tree(new_prefix, s == stmt_list.back());
        }
        return out;
    }
};

// using a variable that has already been declared
class IdentASTnode : public ASTnode
{
    TOKEN Tok;
    std::string Name;

public:
    IdentASTnode(std::string name, TOKEN tok) : Name(name), Tok(tok) {}
    virtual ~IdentASTnode() {}
    Value *codegen() { 
        // fprintf(stdout, "DEBUG: IdentASTnode::codegen()\n");
        // fprintf(stdout, "DEBUG: %s\n", NamedValues[0][Name]->getName().str().c_str());
        // check local contexts first
        if (NamedValues.size() != 0){
            for (int i = NamedValues.size() - 1; i >= 0; i--){
                if (NamedValues[i].find(Name) != NamedValues[i].end())
                {
                    // fprintf(stdout, "DEBUG: Type: %s\n", typeToString(NamedValues[i][Name]->getAllocatedType()).c_str());
                    return Builder.CreateLoad(NamedValues[i][Name]->getAllocatedType(), NamedValues[i][Name], Name.c_str());
                }
            }
        }
        if (GlobalNamedValues.find(Name) != GlobalNamedValues.end())
        {
            return Builder.CreateLoad(GlobalNamedValues[Name]->getValueType(), GlobalNamedValues[Name], Name.c_str());
        }
        error(Tok, "Unknown variable name");
        return nullptr;
    };
    
    std::string to_string() const { return "Ident: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        return prefix + (end ? "└── " : "├── ") + this->to_string();
    }
};

// same as variable but without the type (since it's already been declared)
class AssignASTnode : public ASTnode
{
    TOKEN Tok;
    std::string Name;
    std::unique_ptr<ASTnode> Expr;

public:
    AssignASTnode(std::string name, std::unique_ptr<ASTnode> expr, TOKEN tok) : Name(name), Expr(std::move(expr)), Tok(tok) {}
    virtual ~AssignASTnode() {}
    Value *codegen() {
        // fprintf(stdout, "DEBUG: AssignASTnode::codegen()\n");
        Value* val = Expr->codegen();
        if(!val){
            error(Tok, "Error in AssignASTnode::codegen(): val is nullptr");
        }
        if(val->getType()->isVoidTy()){
            error(Tok, "Cannot assign a void value to a variable!");
        }
        
        // fprintf(stdout, "DEBUG1: %s\n", Name.c_str());
        // check if local variable exists
        if (NamedValues.size() != 0){
            for (int i = NamedValues.size() - 1; i >= 0; i--){
                if (NamedValues[i].find(Name) != NamedValues[i].end())
                {
                    // cast to correct type (will warn if narrowing conversion)
                    val = castToType(val, NamedValues[i][Name]->getAllocatedType(), Tok);
                    Builder.CreateStore(val, NamedValues[i][Name]);
                    return val;
                }
            }
        }
        if (GlobalNamedValues.find(Name) != GlobalNamedValues.end())
        {
            // cast to correct type (will warn if narrowing conversion)
            val = castToType(val, GlobalNamedValues[Name]->getValueType(), Tok);
            Builder.CreateStore(val, GlobalNamedValues[Name]);
            return val;
        }
        error(Tok, "Unknown variable name");
        return nullptr;
    };
    
    std::string to_string() const { return "Assign: " + Name + "\n"; }

    std::string to_tree(std::string prefix, bool end) const
    {
        std::string out = prefix + (end ? "└── " : "├── ") + this->to_string();
        std::string new_prefix = prefix + (end ? "    " : "│   ");
        return out + Expr->to_tree(new_prefix, true);
    }
};

#endif