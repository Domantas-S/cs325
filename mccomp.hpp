#ifndef MCCOMP_HPP
#define MCCOMP_HPP

#include <algorithm>
#include <cassert> 
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "astnode.hpp"

static std::unique_ptr<ProgramASTnode> parseProgram();
static std::vector<std::unique_ptr<ExternASTnode>> parseExternList();
static std::unique_ptr<ExternASTnode> parseExtern();
static std::vector<std::unique_ptr<ASTnode>> parseDeclList();
static std::unique_ptr<ASTnode> parseDecl();
static std::unique_ptr<VarDeclASTnode> parseVarDecl();
static std::unique_ptr<TypeASTnode> parseTypeSpec();
static std::unique_ptr<TypeASTnode> parseVarType();
static std::unique_ptr<FunctionASTnode> parseFunctionDecl();
static std::unique_ptr<ParamASTnode> parseParam();
static std::vector<std::unique_ptr<ParamASTnode>> parseParams();
static std::vector<std::unique_ptr<ParamASTnode>> parseParamList();
static std::unique_ptr<BlockASTnode> parseBlock();
static std::vector<std::unique_ptr<ASTnode>> parseLocalDecls();
static std::unique_ptr<ASTnode> parseLocalDecl();
static std::vector<std::unique_ptr<ASTnode>> parseStmtList();
static std::unique_ptr<ASTnode> parseStmt();
static std::unique_ptr<ASTnode> parseExprStmt();
static std::unique_ptr<ASTnode> parseWhileStmt();
static std::unique_ptr<ASTnode> parseIfStmt();
static std::unique_ptr<ASTnode> parseElseStmt();
static std::unique_ptr<ASTnode> parseReturnStmt();
static std::unique_ptr<ASTnode> parseExpr();
static std::unique_ptr<ASTnode> parseOp1();
static std::unique_ptr<ASTnode> parseOp1Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp2();
static std::unique_ptr<ASTnode> parseOp2Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp3();
static std::unique_ptr<ASTnode> parseOp3Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp4();
static std::unique_ptr<ASTnode> parseOp4Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp5();
static std::unique_ptr<ASTnode> parseOp5Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp6();
static std::unique_ptr<ASTnode> parseOp6Prime(std::unique_ptr<ASTnode>);
static std::unique_ptr<ASTnode> parseOp7();
static std::unique_ptr<ASTnode> parseOp8();
static std::unique_ptr<ASTnode> parseOp9();
static std::unique_ptr<ASTnode> parseOp10();
static std::vector<std::unique_ptr<ASTnode>> parseArgs();
static std::vector<std::unique_ptr<ASTnode>> parseArgList();

#endif
