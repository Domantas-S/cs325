#include "mccomp.hpp"
#include "token.hpp"
#include "astnode.hpp"
// #include "codegen.hpp"



//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//


static TOKEN funcRetType;


// program ::= extern_list decl_list | decl_list
static std::unique_ptr<ProgramASTnode> parseProgram() {
  std::vector<std::unique_ptr<ExternASTnode>> externs = parseExternList();
  std::vector<std::unique_ptr<ASTnode>> decls = parseDeclList();
  return std::make_unique<ProgramASTnode>(std::move(externs), std::move(decls));
};


// extern_list ::= extern extern_list | extern
static std::vector<std::unique_ptr<ExternASTnode>> parseExternList() {
  std::vector<std::unique_ptr<ExternASTnode>> externs;
  while (CurTok.type == EXTERN) {
    externs.push_back(parseExtern());
  }
  return externs;
};

// extern ::= "extern" type_spec IDENT "(" params ")" ";"
static std::unique_ptr<ExternASTnode> parseExtern() {
  getNextToken(); // eat extern

  std::unique_ptr<TypeASTnode> type = parseTypeSpec();  // eat type_spec

  if (CurTok.type != IDENT) error(CurTok, "Expected identifier in extern declaration");
  std::string externName = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  if (CurTok.type != LPAR) error(CurTok, "Expected ( in extern declaration");
  getNextToken(); // eat (
  
  std::vector<std::unique_ptr<ParamASTnode>> params = parseParams();

  if (CurTok.type != RPAR) error(CurTok, "Expected ) in extern declaration");
  getNextToken(); // eat )

  if (CurTok.type != SC) error(CurTok, "Expected ; in extern declaration");
  getNextToken(); // eat ;
  return std::make_unique<ExternASTnode>(std::move(type), externName, std::move(params), saveToken);
};

// decl_list ::= decl decl_list | decl
static std::vector<std::unique_ptr<ASTnode>> parseDeclList() {
  std::vector<std::unique_ptr<ASTnode>> decls;
  while (CurTok.type != EOF_TOK) {
    decls.push_back(parseDecl());
  }
  return decls;
};

// decl ::= var_decl | fun_decl
static std::unique_ptr<ASTnode> parseDecl() {
  std::unique_ptr<TypeASTnode> type = parseTypeSpec();
  std::string name = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  if (CurTok.type == SC){
    getNextToken(); // eat ;
    return std::make_unique<VarDeclASTnode>(std::move(type), name, saveToken);
  } else if (CurTok.type == LPAR) {
    // fun_decl ::= type_spec IDENT "(" params ")" block
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ParamASTnode>> params = parseParams();
    if (CurTok.type != RPAR) 
      error(CurTok, "Expected ) in function declaration");
    getNextToken(); // eat )
    std::unique_ptr<ASTnode> body = parseBlock();
    return std::make_unique<FunctionASTnode>(std::move(type), name, std::move(params), std::move(body), saveToken);
  } else {
    error(CurTok, "Expected ; or ( in declaration");
    return nullptr;
  }
};

// var_decl ::= var_type IDENT ";"
static std::unique_ptr<VarDeclASTnode> parseVarDecl() {
  std::unique_ptr<TypeASTnode> type = parseVarType();

  if (CurTok.type != IDENT) error(CurTok, "Expected identifier in variable declaration");
  std::string varName = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  if (CurTok.type != SC) error(CurTok, "Expected ; in variable declaration");
  getNextToken(); // eat ;
  return std::make_unique<VarDeclASTnode>(std::move(type), varName, saveToken);
};

// type_spec ::= "void" | var_type
static std::unique_ptr<TypeASTnode> parseTypeSpec() {
  if (CurTok.type == VOID_TOK) {
    TOKEN saveToken = CurTok;
    getNextToken(); // eat void
    return std::make_unique<TypeASTnode>("void", saveToken);
  } else {
    return parseVarType();
  }
};

// var_type ::= "int" | "float" | "bool"
static std::unique_ptr<TypeASTnode> parseVarType() {
  TOKEN saveToken = CurTok;
  switch (CurTok.type)
  {
  case INT_TOK:
    getNextToken(); // eat int
    return std::make_unique<TypeASTnode>("int", saveToken);
    break;
  case FLOAT_TOK:
    getNextToken(); // eat float
    return std::make_unique<TypeASTnode>("float", saveToken);
    break;
  case BOOL_TOK:
    getNextToken(); // eat bool
    return std::make_unique<TypeASTnode>("bool", saveToken);
    break;
  default:
    error(CurTok, "Expected a type here"); // TODO: add difference between variable declaration and function declaration (void)
    return nullptr;
    break;
  }
};

// fun_decl ::= type_spec IDENT "(" params ")" block
static std::unique_ptr<FunctionASTnode> parseFunctionDecl() {
  std::unique_ptr<TypeASTnode> type = parseTypeSpec();

  if (CurTok.type != IDENT) error(CurTok, "Expected identifier in function declaration");
  std::string funcName = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  if (CurTok.type != LPAR) error(CurTok, "Expected ( in function declaration");
  getNextToken(); // eat (

  std::vector<std::unique_ptr<ParamASTnode>> params = parseParams();

  if (CurTok.type != RPAR) error(CurTok, "Expected ) in function declaration");
  getNextToken(); // eat )

  std::unique_ptr<ASTnode> body = parseBlock();

  return std::make_unique<FunctionASTnode>(std::move(type), funcName, std::move(params), std::move(body), saveToken);
};

// params ::= param_list | "void" | empty
static std::vector<std::unique_ptr<ParamASTnode>> parseParams() {
  std::vector<std::unique_ptr<ParamASTnode>> params;
  if (CurTok.type == VOID_TOK) {
    getNextToken(); // eat void
    return params;
  } else if (CurTok.type == RPAR) {
    return params;
  } else {
    params = parseParamList();
    return params;
  }
};

// param_list ::= param "," param_list | param
static std::vector<std::unique_ptr<ParamASTnode>> parseParamList() {
  std::vector<std::unique_ptr<ParamASTnode>> params;
  params.push_back(parseParam());
  while (CurTok.type == COMMA) {
    getNextToken(); // eat ,
    params.push_back(parseParam());
  }
  return params;
};

// param ::= var_type IDENT
static std::unique_ptr<ParamASTnode> parseParam() {
  std::unique_ptr<TypeASTnode> type = parseVarType();

  if (CurTok.type != IDENT) error(CurTok, "Expected identifier in parameter declaration");
  std::string paramName = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  return std::make_unique<ParamASTnode>(std::move(type), paramName, saveToken);
};

// block ::= "{" local_decls stmt_list "}"
static std::unique_ptr<BlockASTnode> parseBlock() {
  if (CurTok.type != LBRA) error(CurTok, "Expected { in block");
  TOKEN saveToken = CurTok;
  getNextToken(); // eat {

  std::vector<std::unique_ptr<ASTnode>> local_decls = parseLocalDecls();
  std::vector<std::unique_ptr<ASTnode>> stmt_list = parseStmtList();

  if (CurTok.type != RBRA) error(CurTok, "Expected } in block");
  getNextToken(); // eat }

  return std::make_unique<BlockASTnode>(std::move(local_decls), std::move(stmt_list), saveToken);
};

// local_decls ::= local_decl local_decls | empty
static std::vector<std::unique_ptr<ASTnode>> parseLocalDecls() {
  std::vector<std::unique_ptr<ASTnode>> local_decls;
  while (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    local_decls.push_back(parseLocalDecl());
  }
  return local_decls;
};

// local_decl ::= var_type IDENT ";"
static std::unique_ptr<ASTnode> parseLocalDecl() {
  std::unique_ptr<TypeASTnode> type = parseVarType();

  if (CurTok.type != IDENT) error(CurTok, "Expected identifier in local declaration");
  std::string declName = CurTok.lexeme;
  TOKEN saveToken = CurTok;
  getNextToken(); // eat IDENT

  if (CurTok.type != SC) error(CurTok, "Expected ; at the end of a local declaration");
  getNextToken(); // eat ;
  return std::make_unique<VarDeclASTnode>(std::move(type), declName, saveToken);
};

// stmt_list ::= stmt stmt_list | empty
static std::vector<std::unique_ptr<ASTnode>> parseStmtList() {
  std::vector<std::unique_ptr<ASTnode>> stmt_list;
  while (CurTok.type != RBRA) {
    stmt_list.push_back(parseStmt());
  }
  return stmt_list;
};

// stmt ::= expr_stmt | block | if_stmt | while_stmt | return_stmt
static std::unique_ptr<ASTnode> parseStmt() {
  switch (CurTok.type)
  {
  case LBRA:
    return parseBlock();
    break;
  case IF:
    return parseIfStmt();
    break;
  case WHILE:
    return parseWhileStmt();
    break;
  case RETURN:
    return parseReturnStmt();
    break;
  default:
    return parseExprStmt();
    break;
  }
};

// expr_stmt ::= expr ";"
static std::unique_ptr<ASTnode> parseExprStmt() {
  std::unique_ptr<ASTnode> expr = parseExpr();

  if (CurTok.type != SC) error(CurTok, "Expected ; in expression statement");
  getNextToken(); // eat ;
  return expr;
};

// while_stmt ::= "while" "(" expr ")" stmt
static std::unique_ptr<ASTnode> parseWhileStmt() {
  if (CurTok.type != WHILE) error(CurTok, "Expected while in while statement"); // maybe not needed
  TOKEN saveToken = CurTok;
  getNextToken(); // eat while

  if (CurTok.type != LPAR) error(CurTok, "Expected ( in while statement");
  getNextToken(); // eat (

  std::unique_ptr<ASTnode> expr = parseExpr();

  if (CurTok.type != RPAR) error(CurTok, "Expected ) in while statement");
  getNextToken(); // eat )

  std::unique_ptr<ASTnode> stmt = parseStmt();

  return std::make_unique<WhileASTnode>(std::move(expr), std::move(stmt), saveToken);
};

// if_stmt ::= "if" "(" expr ")" block else_stmt
static std::unique_ptr<ASTnode> parseIfStmt() {
  if (CurTok.type != IF) error(CurTok, "Expected if in if statement"); // maybe not needed
  TOKEN saveToken = CurTok;
  getNextToken(); // eat if

  if (CurTok.type != LPAR) error(CurTok, "Expected ( in if statement");
  getNextToken(); // eat (

  std::unique_ptr<ASTnode> expr = parseExpr();

  if (CurTok.type != RPAR) error(CurTok, "Expected ) in if statement");
  getNextToken(); // eat )

  std::unique_ptr<ASTnode> block = parseBlock();

  std::unique_ptr<ASTnode> else_stmt = parseElseStmt();

  return std::make_unique<IfASTnode>(std::move(expr), std::move(block), std::move(else_stmt), saveToken);
};

// else_stmt ::= "else" block | empty
static std::unique_ptr<ASTnode> parseElseStmt() {
  if (CurTok.type == ELSE) {
    getNextToken(); // eat else
    return parseBlock();
  } else {
    return nullptr;
  }
};

// return_stmt ::= "return" ";" | "return" expr ";"
static std::unique_ptr<ASTnode> parseReturnStmt() {
  TOKEN saveToken = CurTok;
  getNextToken(); // eat return

  if (CurTok.type == SC) {
    getNextToken(); // eat ;
    return std::make_unique<ReturnASTnode>(nullptr, saveToken);
  } else {
    std::unique_ptr<ASTnode> expr = parseExpr();

    if (CurTok.type != SC) error(CurTok, "Expected ; in return statement");
    getNextToken(); // eat ;
    return std::make_unique<ReturnASTnode>(std::move(expr), saveToken);
  }
};

// expr ::= IDENT "=" expr | op1
static std::unique_ptr<ASTnode> parseExpr() {
  if (CurTok.type == IDENT) {
    std::string identName = CurTok.lexeme;
    TOKEN saveToken = CurTok;
    getNextToken(); // eat IDENT

    if (CurTok.type == ASSIGN) {
      getNextToken(); // eat =
      return std::make_unique<AssignASTnode>(identName, parseExpr(), saveToken);
    } else {
      putBackToken(CurTok);
      CurTok = saveToken;
      return parseOp1();  // parse operation with IDENT as first operand
    }
  } else {
    return parseOp1();
  }
};

// op1 ::= op2 op1'
static std::unique_ptr<ASTnode> parseOp1() {
  std::unique_ptr<ASTnode> op2 = parseOp2();
  return parseOp1Prime(std::move(op2));
};

// op1' ::= "||" op2 op1' | empty
static std::unique_ptr<ASTnode> parseOp1Prime(std::unique_ptr<ASTnode> op2) {
  if (CurTok.type == OR) {
    TOKEN saveToken = CurTok;
    getNextToken(); // eat ||
    std::unique_ptr<ASTnode> op2_prime = parseOp2();
    return parseOp1Prime(std::make_unique<BinOpNode>("||", std::move(op2), std::move(op2_prime), saveToken));
  } else {
    return op2;
  }
};

// op2 ::= op3 op2'
static std::unique_ptr<ASTnode> parseOp2() {
  std::unique_ptr<ASTnode> op3 = parseOp3();
  return parseOp2Prime(std::move(op3));
};

// op2' ::= "&&" op3 op2' | empty
static std::unique_ptr<ASTnode> parseOp2Prime(std::unique_ptr<ASTnode> op3) {
  if (CurTok.type == AND) {
    TOKEN saveToken = CurTok;
    getNextToken(); // eat &&
    std::unique_ptr<ASTnode> op3_prime = parseOp3();
    return parseOp2Prime(std::make_unique<BinOpNode>("&&", std::move(op3), std::move(op3_prime), saveToken));
  } else {
    return op3;
  }
};

// op3 ::= op4 op3'
static std::unique_ptr<ASTnode> parseOp3() {
  std::unique_ptr<ASTnode> op4 = parseOp4();
  return parseOp3Prime(std::move(op4));
};

// op3' ::= "==" op4 op3' | "!=" op4 op3' | empty
static std::unique_ptr<ASTnode> parseOp3Prime(std::unique_ptr<ASTnode> op4) {
  TOKEN saveToken = CurTok;
  if (CurTok.type == EQ) {
    getNextToken(); // eat ==
    std::unique_ptr<ASTnode> op4_prime = parseOp4();
    return parseOp3Prime(std::make_unique<BinOpNode>("==", std::move(op4), std::move(op4_prime), saveToken));
  } else if (CurTok.type == NE) {
    getNextToken(); // eat !=
    std::unique_ptr<ASTnode> op4_prime = parseOp4();
    return parseOp3Prime(std::make_unique<BinOpNode>("!=", std::move(op4), std::move(op4_prime), saveToken));
  } else {
    return op4;
  }
};

// op4 ::= op5 op4'
static std::unique_ptr<ASTnode> parseOp4() {
  std::unique_ptr<ASTnode> op5 = parseOp5();
  return parseOp4Prime(std::move(op5));
};

// op4' ::= "<=" op5 op4' | "<" op5 op4' | ">=" op5 op4' | ">" op5 op4' | empty
static std::unique_ptr<ASTnode> parseOp4Prime(std::unique_ptr<ASTnode> op5) {
  TOKEN saveToken = CurTok;
  if (CurTok.type == LE) {
    getNextToken(); // eat <=
    std::unique_ptr<ASTnode> op5_prime = parseOp5();
    return parseOp4Prime(std::make_unique<BinOpNode>("<=", std::move(op5), std::move(op5_prime), saveToken));
  } else if (CurTok.type == LT) {
    getNextToken(); // eat <
    std::unique_ptr<ASTnode> op5_prime = parseOp5();
    return parseOp4Prime(std::make_unique<BinOpNode>("<", std::move(op5), std::move(op5_prime), saveToken));
  } else if (CurTok.type == GE) {
    getNextToken(); // eat >=
    std::unique_ptr<ASTnode> op5_prime = parseOp5();
    return parseOp4Prime(std::make_unique<BinOpNode>(">=", std::move(op5), std::move(op5_prime), saveToken));
  } else if (CurTok.type == GT) {
    getNextToken(); // eat >
    std::unique_ptr<ASTnode> op5_prime = parseOp5();
    return parseOp4Prime(std::make_unique<BinOpNode>(">", std::move(op5), std::move(op5_prime), saveToken));
  } else {
    return op5;
  }
};

// op5 ::= op6 op5'
static std::unique_ptr<ASTnode> parseOp5() {
  std::unique_ptr<ASTnode> op6 = parseOp6();
  return parseOp5Prime(std::move(op6));
};

// op5' ::= "+" op6 op5' | "-" op6 op5' | empty
static std::unique_ptr<ASTnode> parseOp5Prime(std::unique_ptr<ASTnode> op6) {
  TOKEN saveToken = CurTok;
  if (CurTok.type == PLUS) {
    getNextToken(); // eat +
    std::unique_ptr<ASTnode> op6_prime = parseOp6();
    return parseOp5Prime(std::make_unique<BinOpNode>("+", std::move(op6), std::move(op6_prime), saveToken));
  } else if (CurTok.type == MINUS) {
    getNextToken(); // eat -
    std::unique_ptr<ASTnode> op6_prime = parseOp6();
    return parseOp5Prime(std::make_unique<BinOpNode>("-", std::move(op6), std::move(op6_prime), saveToken));
  } else {
    return op6;
  }
};

// op6 ::= op7 op6'
static std::unique_ptr<ASTnode> parseOp6() {
  std::unique_ptr<ASTnode> op7 = parseOp7();
  return parseOp6Prime(std::move(op7));
};

// op6' ::= "*" op7 op6' | "/" op7 op6' | "%" op7 op6' | empty
static std::unique_ptr<ASTnode> parseOp6Prime(std::unique_ptr<ASTnode> op7) {
  TOKEN saveToken = CurTok;
  if (CurTok.type == ASTERIX) {
    getNextToken(); // eat *
    std::unique_ptr<ASTnode> op7_prime = parseOp7();
    return parseOp6Prime(std::make_unique<BinOpNode>("*", std::move(op7), std::move(op7_prime), saveToken));
  } else if (CurTok.type == DIV) {
    getNextToken(); // eat /
    std::unique_ptr<ASTnode> op7_prime = parseOp7();
    return parseOp6Prime(std::make_unique<BinOpNode>("/", std::move(op7), std::move(op7_prime), saveToken));
  } else if (CurTok.type == MOD) {
    getNextToken(); // eat %
    std::unique_ptr<ASTnode> op7_prime = parseOp7();
    return parseOp6Prime(std::make_unique<BinOpNode>("%", std::move(op7), std::move(op7_prime), saveToken));
  } else {
    return op7;
  }
};

// op7 ::= "-" op7 | "!" op7 | op8
static std::unique_ptr<ASTnode> parseOp7() {
  TOKEN saveToken = CurTok;
  if (CurTok.type == MINUS) {
    getNextToken(); // eat -
    std::unique_ptr<ASTnode> op7 = parseOp7();
    return std::make_unique<UnaryOpNode>("-", std::move(op7), saveToken);
  } else if (CurTok.type == NOT) {
    getNextToken(); // eat !
    std::unique_ptr<ASTnode> op7 = parseOp7();
    return std::make_unique<UnaryOpNode>("!", std::move(op7), saveToken);
  } else {
    return parseOp8();
  }
};

// op8 ::= "(" expr ")" | op9
static std::unique_ptr<ASTnode> parseOp8() {
  if (CurTok.type == LPAR) {
    getNextToken(); // eat (
    std::unique_ptr<ASTnode> expr = parseExpr();
    if (CurTok.type != RPAR) error(CurTok, "Expected ) in expression");
    getNextToken(); // eat )
    return expr;
  } else {
    return parseOp9();
  }
};

// op9 ::= IDENT | IDENT "(" args ")" | op10
static std::unique_ptr<ASTnode> parseOp9() {
  TOKEN saveToken = CurTok;
  if (CurTok.type == IDENT) {
    std::string identName = CurTok.lexeme;
    getNextToken(); // eat IDENT
    if (CurTok.type == LPAR) {
      getNextToken(); // eat (
      std::vector<std::unique_ptr<ASTnode>> args = parseArgs();
      if (CurTok.type != RPAR) error(CurTok, "Expected ) in function call");
      getNextToken(); // eat )
      return std::make_unique<CallASTnode>(identName, std::move(args), saveToken);
    } else {
      return std::make_unique<IdentASTnode>(identName, saveToken);
    }
  } else {
    return parseOp10();
  }
};

// op10 ::= INT_LIT | FLOAT_LIT | BOOL_LIT
static std::unique_ptr<ASTnode> parseOp10() {
  TOKEN saveToken = CurTok;
  if (CurTok.type == INT_LIT) {
    int intVal = std::stoi(CurTok.lexeme);
    getNextToken(); // eat INT_LIT
    return std::make_unique<IntASTnode>(intVal, saveToken);
  } else if (CurTok.type == FLOAT_LIT) {
    float floatVal = std::stof(CurTok.lexeme);
    getNextToken(); // eat FLOAT_LIT
    return std::make_unique<FloatASTnode>(floatVal, saveToken);
  } else if (CurTok.type == BOOL_LIT) {
    bool boolVal = (CurTok.lexeme == "true");
    getNextToken(); // eat BOOL_LIT
    return std::make_unique<BoolASTnode>(boolVal, saveToken);
  } else {
    error(CurTok, "Expected an expression here");
    return nullptr;
  }
};


// args ::= arg_list | empty
static std::vector<std::unique_ptr<ASTnode>> parseArgs() {
  std::vector<std::unique_ptr<ASTnode>> args;
  if (CurTok.type == RPAR) {
    return args;
  } else {
    args = parseArgList();
    return std::move(args);
  }
};

// arg_list ::= expr "," arg_list | expr
static std::vector<std::unique_ptr<ASTnode>> parseArgList() {
  std::vector<std::unique_ptr<ASTnode>> args;
  args.push_back(parseExpr());
  while (CurTok.type == COMMA) {
    getNextToken(); // eat ,
    args.push_back(parseExpr());
  }
  return std::move(args);
};

static void parser() {
  getNextToken();
  std::unique_ptr<ProgramASTnode> tree = parseProgram();
  fprintf(stdout, "Parsed program\n");
  fprintf(stdout, "Printing AST\n");
  fprintf(stdout, "%s", tree->to_tree().c_str());
  tree->codegen();
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ASTnode &ast) {
  os << ast.to_string();
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // get the first token
  // getNextToken();
  // while (CurTok.type != EOF_TOK) {
  //   fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
  //           CurTok.type);
  //   getNextToken();
  // }
  // fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);

  // Run the parser now.
  parser();

  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************

  fclose(pFile); // close the file that contains the code that was parsed
  printWarnings();
  return 0;
}
