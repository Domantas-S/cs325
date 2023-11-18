#ifndef CODEGEN_HPP
#define CODEGEN_HPP

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
#include "astnode.hpp"

using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, AllocaInst*> NamedValues; // local var table(s)
static std::map<std::string, Value *> GlobalNamedValues; //global var table

Value* lazyAnd(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok);

Value* lazyOr(std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, TOKEN tok);

Type* getWidestType(Type* type1, Type* type2);

std::string typeToString(Type* type);

Value* castToType(Value* val, Type* type, TOKEN tok);

AllocaInst * CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, Type *type);

#endif