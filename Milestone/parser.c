/* 
  Author: John Ko
  Course: CSC-453
  Purpose: For milestone 4-2, It will create the parser as full C-- version of grammar.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "symbol_table.h"
#include "ast.h"


Token cur_token;  // Global variable to store each tokens
extern int get_token();  // used for getting tokens from scanner
extern int line_num;  // checks line number for stderr
extern int chk_decl_flag;
extern int print_ast_flag;
extern int gen_code_flag;
int count_scope = 0;  // When the value is 0, which means global scope, and when the value is 1, which means local scope.
extern char* lexeme;  // Used to store the name of the identifier.
char* cur_lexeme;  // Store current identifier to the global variable
int args_num = 0;  // Counts number of parameters in function definition or function call.
char* func_name;  // Stores the name of the function.
extern Scope* globalScope;  // This header of linked list of symbol table.
int attempt = 0;
int temps = 0;
int addGlobalVar = 0;
int assgNum = 0;

/* Function Prototypes */
void prog();
void var_decl();
void id_list();
void type();
ASTnode* func_defn();
ASTnode* formals();
void opt_var_decls();
ASTnode* opt_stmt_list();
ASTnode* stmt();
ASTnode* if_stmt();
ASTnode* while_stmt();
ASTnode* return_stmt();
ASTnode* assg_stmt();
ASTnode* fn_call();
ASTnode* opt_expr_list();
ASTnode* expr_list();
ASTnode* bool_exp();
ASTnode* arith_exp();
ASTnode* relop();
ASTnode* arithop();
ASTnode* logical_op();

/* Code Generation Prototypes */
symtab_entry *newtemp(char *t);
Quad *newlabel();
Quad *newinstr(OpType op, Operand src1, Operand src2, Operand dest);

void postOrderTraversal(ASTnode* node);
void processNode(ASTnode* node);

void codeGen_func_def(ASTnode *f);
void codeGen_stmt(ASTnode *s);
void codeGen_expr(ASTnode *e);
void codeGen_bool(ASTnode *b, Quad* thenLabel, Quad* elseLabel);

void generateMips(ASTnode* node);
void generateMipsCode(Quad* quad);
void printQuadChain(Quad* quad);

void increment_scope() {  // Increament value to change scope to local.
  count_scope++;
}

void decrement_scope() {  // Decreament value to change scope to global
  if (count_scope > 0) {
    count_scope--;
  }
}

// This function will remove all the symbols from symbol table.
void clean_symbol_table() {
  if (chk_decl_flag != 0  || gen_code_flag != 0) {  // Clean symbol table
    cleanupGlobalScope();
  }
}

// Prints out error message with exit code 1.
void error_case() {
  fprintf(stderr, "ERROR LINE %d\n", line_num);
  exit(1);
}

/*
  This function does semantic checking by following the rule of specification.
  func_call parameter checks its current grammar rule is fn_call or not.
*/
int semantic_check(int func_call, int assg_call) {
  if (func_call) {  // When the rule is on fn_call
    Scope* next_scope = globalScope->next;  // Move to the local scope to check any other variable is assigned with the same name.
    if (assg_call) {  // When standard input assigns another value to a variable
      // When the variable exists in symbol table, it will check the type of that symbol.
      if (findSymbol(globalScope, cur_lexeme) != NULL || findSymbol(next_scope, cur_lexeme) != NULL) {
        if (findSymbol(next_scope, cur_lexeme) != NULL) {  // NEED FIX to assignment op
          Symbol* target = findSymbol(next_scope, cur_lexeme);
          if (strcmp(target->type, "var") == 0) {
            return 1;
          }
        } else {
          Symbol* target = findSymbol(globalScope, cur_lexeme);
          if (strcmp(target->type, "var") == 0) {
            return 1;
          }
        }
      }
    } else {  // When the function call is generated
      if (findSymbol(globalScope, cur_lexeme) != NULL) {  // defined function should be on global scope
        Symbol* func_symbol = findSymbol(globalScope, cur_lexeme);
        if (findSymbol(next_scope, cur_lexeme) == NULL) {  // Check local variable with the same name exists
          Symbol* target = findSymbol(globalScope, cur_lexeme);
          if (strcmp(target->type, "func") == 0) {  // Check found symbol has the type "function"
            if (func_symbol->args_num == args_num) {
              return 1;
            }
          }
        }
      }
    }
  } else {  // When the rule is not on fn_call
    if (count_scope == 1) {  // When the scope is on local scope
      if (globalScope->next != NULL) {  // It will check the local scope is assigned or not.
        Scope* cur_scope = globalScope->next;  // Move to the next scope to check the name of other variables that have the same name.
        if (findSymbol(cur_scope, cur_lexeme) == NULL) {
          return 1;
        }
      }
    } else {  // When the scope is on global scope
      if (findSymbol(globalScope, cur_lexeme) == NULL) {  // It will check the name of other variables or functions with the same name.
        return 1;
      } 
    }
  }

  return 0;  // Otherwise, it will cause an error because of semantically invalid to send 0.
}

/*
  This function will add symbol to the symbol table.
  It will handle error and exit with status code 1 by semantic_check function.
*/
void add_symbol(int func_call, int assg_call, int params_num, char* type) {
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is on the command-line argument, it will implemented.
    if (count_scope == 0) {  // When the scope is on global scope
      if (semantic_check(func_call, assg_call)) {  // It will check the symbol is semantically valid.
        if (func_call == 0) {
          insertSymbol(globalScope, cur_lexeme, type, params_num);  // If it is, it will add symbol to the table.
        }
        // printSymbolsInScope(globalScope);
      } else {  // Otherwise, it will cause an error.
        clean_symbol_table();
        error_case();
      }
    } else {  // When the scope is on local scope.
      if (func_call == 1 && semantic_check(func_call, assg_call)) {
        // Do not need to store function call into the symbol table
      }
      // When the symbol is valid on local scope, it will add symbol to the table.
      else if (func_call == 0 && semantic_check(func_call, assg_call)) {  // ??
        Scope* newScope = globalScope->next;
        insertSymbol(newScope, cur_lexeme, type, params_num);
        // printSymbolsInScope(globalScope->next);
      } else {  // ERROR HANDLING
        clean_symbol_table();
        error_case();
      }
    }
  }
}

// This function matches the tokens that are valid or not from grammar rule
void match(Token expected) {
  if (cur_token == expected) {  // When current token is matched to expected token, move to the next token
    cur_token = get_token();
  } else {  // When t he token is not matched, it will show an error with line number and exit code 1.
    error_case();
  }
}

ASTnode* arithop() {
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Create ASTnode when print_ast flas is on.
    ast = malloc(sizeof(ASTnode));
  }
  if (cur_token == opADD) {
    match(opADD);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = ADD;
    }
  } else if (cur_token == opSUB) {
    match(opSUB);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = SUB;
    }
  } else if (cur_token == opMUL) {
    match(opMUL);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = MUL;
    }
  } else if (cur_token == opDIV) { 
    match(opDIV);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = DIV;
    }
  } else { 
    error_case();
  }
  return ast;
}

ASTnode* logical_op() {
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Create ASTnode when print_ast flas is on.
    ast = malloc(sizeof(ASTnode));
  }
  if (cur_token == opAND) {
    match(opAND);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = AND;
    }
  } else if (cur_token == opOR) {
    match(opOR);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = OR;
    }
  } else { 
    error_case();
  }
  return ast;
}

// This function checks the operators for boolean checking.
ASTnode* relop() {
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Create ASTnode when print_ast flas is on.
    ast = malloc(sizeof(ASTnode));
  }
  if (cur_token == opEQ) {
    match(opEQ);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = EQ;
    }
  } else if (cur_token == opNE) {
    match(opNE);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = NE;
    }
  } else if (cur_token == opLE) {
    match(opLE);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = LE;
    }
  } else if (cur_token == opLT) { 
    match(opLT);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = LT;
    }
  } else if (cur_token == opGE) {
    match(opGE);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = GE;
    }
  } else if (cur_token == opGT) { 
    match(opGT);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = GT;
    }
  } else { 
    error_case();
  }
  return ast;
}

ASTnode* primary_exp() {  // ID, INTCON, FN_CALL, or ( exp )
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast = malloc(sizeof(ASTnode));
  }
  if (cur_token == ID) {
    char* cur_id = lexeme;
    match(ID);

    if (cur_token == LPAREN) {  // Function call
      ASTnode* call = NULL;
      int original_args_num = args_num;
      args_num = 0;
      cur_lexeme = cur_id;
      call = fn_call();
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {
        cur_lexeme = cur_id;
        add_symbol(1, 0, args_num, "func");
        args_num = original_args_num;
      }
      if (print_ast_flag != 0 || gen_code_flag != 0) {
        free(ast);
        ast = call;

        ast->num = 1;
        char str[100];
        sprintf(str, "%s%d", "temp", temps);
        insertSymbol(globalScope->next, str, "var", 0);

        temps++;
      }
    } else {  // ID
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {
        cur_lexeme = cur_id;
        add_symbol(1, 1, 0, "var");
      }

      if (print_ast_flag != 0 || gen_code_flag != 0) {
        ast->name = cur_id;
        ast->ntype = IDENTIFIER;
      }
    }
  } else if (cur_token == INTCON) {
    int val = atoi(lexeme);
    match(INTCON);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->ntype = INTCONST;
      ast->num = val;

      char str[100];
      sprintf(str, "%s%d", "temp", temps);
      insertSymbol(globalScope->next, str, "var", 0);

      temps++;
    }
  } else if (cur_token == LPAREN) {
    match(LPAREN);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast = arith_exp();
    } else {
      arith_exp();
    }
    match(RPAREN);
  } else {
    error_case();
  }

  return ast;
}

ASTnode* unary_exp() {  // Unary
  if (cur_token == opSUB) {
    match(opSUB);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ASTnode* node = unary_exp();
      ASTnode* new_node = malloc(sizeof(ASTnode));
      new_node->ntype = UMINUS;
      new_node->child0 = node;

      char str[100];
      sprintf(str, "%s%d", "temp", temps);
      insertSymbol(globalScope->next, str, "var", 0);

      temps++;

      return new_node;
    } else {
      unary_exp();
      
      return NULL;
    }
  }

  return primary_exp();
}

ASTnode* mul_exp() {  // MUL or DIV
  ASTnode* left = unary_exp();
  while (cur_token == opMUL || cur_token == opDIV) {
    ASTnode* new_node = NULL;
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      new_node = malloc(sizeof(ASTnode));
      new_node->ntype = arithop()->ntype;
      ASTnode* right = unary_exp();

      new_node->child0 = left;
      new_node->child1 = right;
      left = new_node;
      
      char str[100];
      sprintf(str, "%s%d", "temp", temps);
      insertSymbol(globalScope->next, str, "var", 0);

      temps++;
    } else {
      arithop();
      unary_exp();
    }
  }

  return left;
}

ASTnode* add_exp() {  // Addition or Subtraction
  ASTnode* left = mul_exp();
  while (cur_token == opADD || cur_token == opSUB) {
    ASTnode* new_node = NULL;
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      new_node = malloc(sizeof(ASTnode));
      new_node->ntype = arithop()->ntype;
      ASTnode* right = mul_exp();

      new_node->child0 = left;
      new_node->child1 = right;
      left = new_node;

      char str[100];
      sprintf(str, "%s%d", "temp", temps);
      insertSymbol(globalScope->next, str, "var", 0);

      temps++;
    } else {
      arithop();
      mul_exp();
    }
  }

  return left;
}

// This function checks the identifiers or integer constants from statement or function call
ASTnode* arith_exp() {
  return add_exp();
}

ASTnode* rel_exp() {
  ASTnode* left = arith_exp();
  while (cur_token == opEQ || cur_token == opNE || cur_token == opLE || cur_token == opLT || cur_token == opGE || cur_token == opGT) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ASTnode* new_node = malloc(sizeof(ASTnode));
      new_node->child0 = left;
      new_node->ntype = relop()->ntype;
      new_node->child1 = arith_exp();
      left = new_node;
    } else {
      relop();
      arith_exp();
    }
  }
  return left;
}

ASTnode* and_exp() {
  ASTnode* left = rel_exp();
  while (cur_token == opAND) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ASTnode* new_node = malloc(sizeof(ASTnode));
      new_node->child0 = left;
      new_node->ntype = logical_op()->ntype;
      new_node->child1 = rel_exp();
      left = new_node;
    } else {
      logical_op();
      rel_exp();
    }
  }
  return left;
}

// This function checks boolean expression for if-statement or while-statement
ASTnode* bool_exp() {
  ASTnode* left = and_exp();
  while (cur_token == opOR) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ASTnode* new_node = malloc(sizeof(ASTnode));
      new_node->child0 = left;
      new_node->ntype = logical_op()->ntype;
      new_node->child1 = and_exp();
      left = new_node;
    } else {
      logical_op();
      and_exp();
    }
  }
  return left;
}

// This function checks if statement is generated correctly.
ASTnode* if_stmt() {
  ASTnode* ast = NULL;
  match(kwIF);
  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast = malloc(sizeof(ASTnode));
    ast->ntype = IF;
  }
  match(LPAREN);
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // if-stmt boolean expression
    ast->child0 = bool_exp();
  } else {
    bool_exp();
  }
  match(RPAREN);
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // if-then including statement
    ast->child1 = stmt();
  } else {
    stmt();
  }
  if (cur_token == kwELSE) {
    match(kwELSE);
    if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add else part
      ast->child2 = stmt();
    } else {
      stmt();
    }
  }

  return ast;
}

// This function checks while statement is generated correctly.
ASTnode* while_stmt() {
  ASTnode* ast = NULL;
  match(kwWHILE);
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add while-statement boolean expression 
    ast = malloc(sizeof(ASTnode));
    ast->ntype = WHILE;
  }
  match(LPAREN);
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add while-statement body
    ast->child0 = bool_exp();
  } else {
    bool_exp();
  }
  match(RPAREN);
  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast->child1 = stmt();
  } else {
    stmt();
  }

  return ast;
}

// This function checks return statement is generated correctly.
ASTnode* return_stmt() {
  ASTnode* ast = NULL;
  match(kwRETURN);
  if (cur_token == ID || cur_token == INTCON || cur_token == LPAREN || cur_token == opSUB) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add return value when it exists
      ast = malloc(sizeof(ASTnode));
      ast->ntype = RETURN;
      ast->child0 = arith_exp();
    } else {
      arith_exp();
    }
    match(SEMI);
  } else if (cur_token == SEMI) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add return as node type
      ast = malloc(sizeof(ASTnode));
      ast->ntype = RETURN;
    }
    match(SEMI);
  } else {
    error_case();
  }

  return ast;
}

// This function checks the identifier assigns new variable or value correctly.
ASTnode* assg_stmt() {
  ASTnode* ast = NULL;
  match(opASSG);
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add assignment statement
    ast = malloc(sizeof(ASTnode));
    ast->ntype = ASSG;
    ast->child1 = arith_exp();
  } else {
    arith_exp();
  }
  match(SEMI);

  return ast;
}

// This function matches the type of token is INT.
void type() {
  match(kwINT);
}

// This is newly added function for this milestone to add list of ids
void id_list() {
  while (cur_token == COMMA) {  // When comma is found, it will match another id.
    match(COMMA);
    cur_lexeme = lexeme;
    match(ID);
    if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will add id to the symbol table.
      add_symbol(0, 0, 0, "var");
    }
  }
}

// This function will match the tokens for parameters.
ASTnode* formals() {
  ASTnode* ast = NULL;
  type();
  cur_lexeme = lexeme;
  match(ID);
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will add id to the symbol table.
    add_symbol(0, 0, 0, "var");
  }
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add Identifier for each arguments for function definition
    ast = malloc(sizeof(ASTnode));
    ast->ntype = IDENTIFIER;
    ast->name = cur_lexeme;
    ast->st_ref = findSymbol(globalScope->next, cur_lexeme);
    
    findSymbol(globalScope->next, cur_lexeme)->isArg = 1;
  }
  args_num++;
  if (cur_token == COMMA) {  // When comma is found, it will match another id.
    match(COMMA);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->child0 = formals();  // Recursive call
    } else {
      formals();  // Recursive call
    }
  }

  return ast;
}

// This function checks for opt_formals, which returns epsilon for now.
ASTnode* opt_formals() {
  ASTnode* ast = NULL;
  args_num = 0;

  if (cur_token == kwINT) {
    ast = formals();
  }

  return ast;
}

// This function will match variable assigning
void var_decl() {
  type();
  cur_lexeme = lexeme;
  match(ID);
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will add id to the symbol table.
    add_symbol(0, 0, 0, "var");
  }
  id_list();  // function call to add more ids if there are more ids to add
  match(SEMI);
  addGlobalVar = 1;
}

// This function checks for opt_var_decls, which returns epsilon for now.
void opt_var_decls() {
  while (cur_token == kwINT) {
    var_decl();
  }
}

// This function checks the parameters of function call
ASTnode* expr_list() {
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add expression list to store arguments of a function call
    ast = malloc(sizeof(ASTnode));
    ast->ntype = EXPR_LIST;
    ast->child0 = arith_exp();
  } else  {
    arith_exp();
  }
  args_num++;
  
  if (cur_token == COMMA) {
    match(COMMA);
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast->child1 = expr_list();
    } else {
      expr_list();
    }
  }

  return ast;
}

// This function checks for opt_expr_list, which returns epsilon for now.
ASTnode* opt_expr_list() {
  ASTnode* ast = NULL;
  if (cur_token == ID || cur_token == INTCON || cur_token == LPAREN || cur_token == opSUB) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      ast = expr_list();
    } else {
      expr_list();
    }
  }

  return ast;
}

// This function will follow the rule of fn_call 
ASTnode* fn_call() {
  ASTnode* ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add the name of callee to ASTnode
    ast = malloc(sizeof(ASTnode));
    ast->name = cur_lexeme;
    ast->ntype = FUNC_CALL;
    ast->st_ref = findSymbol(globalScope, cur_lexeme);
  }
  match(LPAREN);
  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast->child0 = opt_expr_list();
  } else {
    opt_expr_list();
  }
  match(RPAREN);
  return ast;
}

// This function will follow the grammar rule of stmt
ASTnode* stmt() {
  ASTnode* ast = NULL;
  if (cur_token == ID) {  // Checks the token is ID for assg_stmt and fn_call
    char* func_id = lexeme;  // Store ID
    match(ID);
    if (cur_token == LPAREN) {  // When fn_call is generated
      args_num = 0;
      cur_lexeme = func_id;
      ast = fn_call();
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {
        cur_lexeme = func_id;
        add_symbol(1, 0, args_num, "func");
      }
      if (gen_code_flag != 0) {
        ast->num = 0;
      }
      match(SEMI);
    } else if (cur_token == opASSG) {  // When assg_stmt is generated
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {
        cur_lexeme = func_id;
        add_symbol(1, 1, 0, "var");
      }
      ast = assg_stmt();
      if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add left-hand side of assignment statement
        ASTnode* lhs = malloc(sizeof(ASTnode));
        lhs->name = func_id;
        lhs->ntype = IDENTIFIER;
        ast->child0 = lhs;
      }
    } else {
      error_case();
    }
  } else if (cur_token == kwWHILE) {  // When while_stmt is generated.
    ast = while_stmt();
  } else if (cur_token == kwIF) {  // When if_stmt is generated
    ast = if_stmt();
  } else if (cur_token == kwRETURN) {  // When return_stmt is generated
    ast = return_stmt();
  } else if (cur_token == LBRACE) {  // When the tokens are inputed after if or while statement is generated
    match(LBRACE);
    ast = opt_stmt_list();
    match(RBRACE);
  } else if (cur_token == SEMI) {  // Checking semi-colon
    match(SEMI);
  } else {
    if (print_ast_flag != 0 || gen_code_flag != 0) {
      return NULL;
    } else {
      error_case();
    }
  }

  return ast;
}

// This function will follow the grammar rule of opt_stmt_list with tail recursion to match ID
ASTnode* opt_stmt_list() {
  ASTnode* ast = NULL;
  //tail recursion optimization
  while (cur_token == ID || cur_token == kwWHILE || cur_token == kwIF || cur_token == kwRETURN || cur_token == LBRACE || cur_token == SEMI) {
    if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add statement list to the child of function definition
      ast = malloc(sizeof(ASTnode));
      ast->ntype = STMT_LIST;
      ast->child0 = stmt();
      ast->child1 = opt_stmt_list();
    } else {
      stmt();
      opt_stmt_list();
    }
  }
  return ast;
}

// This function will follow the grammar rule of func_defn that is derived from prog non-terminal.
ASTnode* func_defn() {
  ASTnode *ast = NULL;
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Create the root of AST
    ast = malloc(sizeof(ASTnode));
    ast->name = func_name;
    ast->ntype = FUNC_DEF;
  }
  match(LPAREN);
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will create local scope from the starting symbol.
    increment_scope();  // Change the scope to local
    createScope(count_scope, globalScope);
  }
  if (print_ast_flag != 0 || gen_code_flag != 0) {  // Add arguments as a child of function definition to get the arguments when print
    ast->child1 = opt_formals();
  } else {
    opt_formals();
  }
  
  match(RPAREN);
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // Add defined function to the symbol table.
    Symbol* func_sym = findSymbol(globalScope, func_name);
    func_sym->args_num = args_num;
    args_num = 0;
  }
  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast->st_ref = globalScope->symbols;
  }
  match(LBRACE);
  opt_var_decls();

  if (print_ast_flag != 0 || gen_code_flag != 0) {
    ast->child0 = opt_stmt_list();
  } else {
    opt_stmt_list();
  }

  match(RBRACE);
  if (print_ast_flag != 0) {
    print_ast(ast);
  }
  if (gen_code_flag != 0) {
    processNode(ast);
    generateMips(ast);
  }
  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {
    decrement_scope();  // Change back scope to global
    removeSymbolsInScope(globalScope->next);
    temps = 0;
  }

  return ast;
}

void addPrint() {
  if (gen_code_flag != 0) {
    char name[50] = "println";
    cur_lexeme = name;
    add_symbol(0, 0, 0, "func");
    Symbol* sym = findSymbol(globalScope, "println");
    sym->args_num = 1;
  }
}

// This function will follow the grammar rule of prog that starts with kwINT and it has tail recursion.
void prog() {
  addPrint();
  //tail recursion optimization
  while (cur_token == kwINT) {
    type();
    cur_lexeme = lexeme;
    match(ID);

    // Checking the current token to generate two different grammar rules
    if (cur_token == LPAREN) {
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will add id to the symbol table.
        add_symbol(0, 0, 0, "func");
        func_name = cur_lexeme;
      }
      func_defn();
    } else {
      if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, it will add id to the symbol table.
        add_symbol(0, 0, 0, "var");
        func_name = cur_lexeme;
      }
      id_list();
      match(SEMI);
    }
  }

  return;  // Epsilon
}

// This function will generate the parser from standard inputs by scanner.
int parse() {
  cur_token = get_token();

  if (chk_decl_flag != 0 || print_ast_flag != 0 || gen_code_flag != 0) {  // When --chk_decl is declared, create symbol table with the scope.
    initGlobalScope();
  }

  prog();

  match(EOF);  // Check the standard input is empty
  clean_symbol_table();
  exit(0);  // When the program generated successfully, it will exit the program with exit code 0.
}

// Global variables for Code generation
static int temp_num = 0;
static int label_num = 0;
static int varLoc = 4;
static int argLoc = 8;

symtab_entry *newtemp(char* t) {
  struct symtab_entry *ntmp = malloc(sizeof(struct symtab_entry));
  char tempName[20];
  sprintf(tempName, "temp%d", temp_num++);
  ntmp->name = strdup(tempName);
  ntmp->type = strdup(t);
  // ntmp->num = val;
  ntmp->info = strdup("temp");

  findSymbol(globalScope->next, tempName)->memLoc = varLoc;
  varLoc += 4;
  return ntmp;
}

Quad* newlabel() {
  char labelName[50];
  sprintf(labelName, "_label%d", label_num++);

  symtab_entry* sym = malloc(sizeof(symtab_entry));
  sym->info = strdup("label");
  sym->name = strdup(labelName);
  Operand src1;
  src1.operand_type = SYMBOL_PTR;
  src1.val.stptr = sym;

  return newinstr(LABEL, src1, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED});
}

Quad *newinstr(OpType op, Operand src1, Operand src2, Operand dest)
{
  Quad *ninstr = malloc(sizeof(Quad));
  ninstr->op = op;
  ninstr->src1 = src1;
  ninstr->src2 = src2;
  ninstr->dest = dest;
  ninstr->next = NULL;
  return ninstr;
}

void processNode(ASTnode* node) {
  codeGen_func_def(node);
}

void codeGen_func_def(ASTnode* f) {
  temp_num = 0;
  if (f == NULL || f->ntype != FUNC_DEF) {
    return;
  }
  ASTnode* curNode = f;

  Quad* enterQuad = (Quad*)malloc(sizeof(Quad));
  enterQuad->op = ENTER;
  enterQuad->src1.operand_type = SYMBOL_PTR;
  enterQuad->src1.val.name = f->st_ref->name;
  enterQuad->src2.operand_type = UNUSED;
  enterQuad->dest.operand_type = UNUSED;
  enterQuad->next = NULL;

  f->code = enterQuad;

  curNode = curNode->child0;
  if (curNode != NULL) {
    codeGen_stmt(curNode);
  } else {
    return;
  }

  // // When there are more stmts
  // if (curNode->child1 != NULL) {
  //   curNode = curNode->child1;  // Move to the next stmt
  //   while (curNode != NULL) {
  //     codeGen_stmt(curNode);

  //     curNode = curNode->child1;
  //   }
  // }

  varLoc = 4;
  argLoc = 8;
}

int enterWhile = 0;

void codeGen_stmt(ASTnode *s) {
  if (s == NULL) return;
  Operand src1, src2, dest;
  Quad* labelThen = NULL;
  Quad* labelElse = NULL;
  Quad* labelTop = NULL;
  Quad* labelBody = NULL;
  Quad* labelAfter = NULL;
  Quad* head = NULL;
  Quad* newQuad = NULL;
  switch (s->ntype) {
    case FUNC_CALL:
      codeGen_expr(s);
      break;
    
    case STMT_LIST:
      codeGen_stmt(s->child0);
      codeGen_stmt(s->child1);
      break;

    case ASSG:
      codeGen_expr(s->child0);  // LHS
      codeGen_expr(s->child1);  // RHS

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = s->child1->place;
      src2.operand_type = UNUSED;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = s->child0->place;

      s->code = newinstr(ASSG, src1, src2, dest);

      assgNum++;

      // Test Code
      // printf("**ASSG**\n");
      // printf("opType A : %d\n", s->code->op);
      // printf("assg lhs : %s\n", s->code->dest.val.stptr->name);
      // printf("assg rhs : %s\n", s->code->src1.val.stptr->name);
      // if (findSymbol(globalScope->next, s->code->dest.val.stptr->name) != NULL) {
      //   printf("assg lhs mem : %d\n", findSymbol(globalScope->next, s->code->dest.val.stptr->name)->memLoc);
      // }
      // if (findSymbol(globalScope->next, s->code->src1.val.stptr->name) != NULL) {
      //   printf("assg rhs mem : %d\n", findSymbol(globalScope->next, s->code->src1.val.stptr->name)->memLoc);
      // }
      break;

    case RETURN:
      if (s->child0) {
        codeGen_expr(s->child0);
        src1.operand_type = SYMBOL_PTR;
        src1.val.stptr = s->child0->place;

        s->code = newinstr(RETURN_STMT, src1, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED});
      } else {
        s->code = newinstr(RETURN_STMT, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED});
      }
      break;
    
    case IF:
      labelThen = newlabel();
      labelElse = newlabel();
      labelAfter = newlabel();

      codeGen_bool(s->child0, labelThen, labelElse);
      codeGen_stmt(s->child1);
      codeGen_stmt(s->child2);

      head = labelAfter;
      
      labelElse->next = labelAfter;
      head = labelElse;

      newQuad = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, labelAfter->src1);
      newQuad->next = labelElse;
      head = newQuad;

      labelThen->next = newQuad;
      head = labelThen;

      // Store boolean expression to quad

      s->code = head;

      // Test
      // printf("boolean op : %d\n", s->child0->code->op);
      // printf("goto : %s\n", s->child0->code->next->dest.val.stptr->name);
      // printf("labels %s, %s, %s\n", labelThen->src1.val.stptr->name, labelElse->src1.val.stptr->name, labelAfter->src1.val.stptr->name);
      // while (head != NULL) {
      //   if (head->op == LABEL) {
      //     printf("label : %s\n", head->src1.val.stptr->name);
      //   } else {
      //     printf("goto : %s\n", head->dest.val.stptr->name);
      //   }

      //   head = head->next;
      // }

      break;

    case WHILE:
      labelTop = newlabel();
      labelBody = newlabel();
      labelAfter = newlabel();

      enterWhile = 1;
      codeGen_bool(s->child0, labelBody, labelAfter);
      enterWhile = 0;

      codeGen_stmt(s->child1);

      head = labelAfter;

      newQuad = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, labelTop->src1);
      newQuad->next = labelAfter;
      head = newQuad;

      labelBody->next = newQuad;
      head = labelBody;

      labelTop->next = labelBody;
      head = labelTop;

      s->code = head;

      break;

    default:
      break;
  }
}

void codeGen_expr(ASTnode *e) {
  if (e == NULL) return;

  Quad* headQuad = NULL;
  Operand src1, src2, dest;
  ASTnode* expr_list;
  switch (e->ntype) {
    case FUNC_CALL:
      if (e->num == 1) {
        e->place = newtemp("arith");  // retrieve
      }
      expr_list = e->child0;
      codeGen_expr(expr_list);  // Arg list

      // Add CALL to the head of quad.
      src1.operand_type = SYMBOL_PTR;
      src1.val.name = e->name;
      src2.operand_type = INTCONST;
      src2.val.iconst = findSymbol(globalScope, e->name)->args_num;
      dest.operand_type = UNUSED;
      headQuad = newinstr(CALL, src1, src2, dest);

      if (e->num == 1) {
        src1.operand_type = UNUSED;
        src2.operand_type = UNUSED;
        dest.operand_type = SYMBOL_PTR;
        if (findSymbol(globalScope->next, e->place->name) != NULL) {
          if (findSymbol(globalScope->next, e->place->name)->isAssigned == 0) {
            updateSymbol(globalScope->next, e->place->name, varLoc);
            varLoc += 4;
          }
        }
        dest.val.stptr = e->place;
        headQuad->next = newinstr(RETRIEVE, src1, src2, dest);
      }
      // Add PARAM
      while (expr_list != NULL) {
        src1.operand_type = SYMBOL_PTR;
        src1.val.stptr = expr_list->child0->place;
        src2.operand_type = UNUSED;
        dest.operand_type = UNUSED;
        
        Quad* new_quad = newinstr(PARAM, src1, src2, dest);

        new_quad->next = headQuad;
        headQuad = new_quad;

        expr_list = expr_list->child1;
      }

      e->code = headQuad;
      
      // test out fn_call
      // expr_list = e->child0;
      // while (expr_list != NULL) {
      //   if (expr_list->child0->code != NULL) {
      //     printf("opType : %d\n", expr_list->child0->code->op);
      //     printf("args : %d\n", expr_list->child0->code->op);
      //     printf("val : %d\n", expr_list->child0->code->src1.val.iconst);
      //   } else {
            
      //   }
      //   expr_list = expr_list->child1;
      // }
      // while (headQuad != NULL) {
      //   if (headQuad->op == CALL) {
      //     printf("optype : %d, name : %s\n", headQuad->op, headQuad->src1.val.name);
      //   } else {
      //     printf("info : %s\n", headQuad->src1.val.stptr->info);
      //     printf("optype : %d, name : %s\n", headQuad->op, headQuad->src1.val.stptr->name);
      //     if (findSymbol(globalScope->next, headQuad->src1.val.stptr->name) == NULL) {
      //       printf("global : _%s\n", headQuad->src1.val.stptr->name);
      //     } else {
      //       printf("memory : %d\n", findSymbol(globalScope->next, headQuad->src1.val.stptr->name)->memLoc);
      //     }
      //   }
      //   headQuad = headQuad->next;
      // }
      // printf("-----------------\n");

      break;

    case EXPR_LIST:
      expr_list = e;
      while (expr_list != NULL) {
        codeGen_expr(expr_list->child0);

        expr_list = expr_list->child1;
      }
      break;

    case INTCONST:
      e->place = newtemp("int");
      src1.operand_type = INTEGER_CONST;
      src1.val.iconst = e->num;
      src2.operand_type = UNUSED;
      dest.operand_type = SYMBOL_PTR;
      e->place->num = e->num;
      dest.val.stptr = e->place;

      e->code = newinstr(ASSG, src1, src2, dest);
      break;

    case IDENTIFIER:
      if (e->place == NULL) {
        e->place = malloc(sizeof(symtab_entry));
      }
      e->place->type = strdup("int");
      e->place->name = e->name;
      if (findSymbol(globalScope->next, e->name) != NULL) {
        e->place->scope = 1;
        if (findSymbol(globalScope->next, e->name)->isAssigned == 0) {  // When location is not already set
          if (findSymbol(globalScope->next, e->name)->isArg == 0) {  // When it is variables or temporaries
            updateSymbol(globalScope->next, e->name, varLoc);
            e->place->info = "var";
            varLoc += 4;
          } else {
            updateSymbol(globalScope->next, e->name, argLoc);
            e->place->info = "arg";
            argLoc += 4;
          }
        } else {
          if (findSymbol(globalScope->next, e->name)->isArg == 0) {
            e->place->info = "var";
          } else {
            e->place->info = "arg";
          }
        }
      } else {
        e->place->scope = 0;
        e->place->info = "global";
      }

      e->code = NULL;
      break;
    
    case ADD:
      codeGen_expr(e->child0);
      codeGen_expr(e->child1);
      e->place = newtemp("arith");
      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = e->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = e->child1->place;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = e->place;

      e->code = newinstr(PLUS, src1, src2, dest);

      break;

    case SUB:
      codeGen_expr(e->child0);
      codeGen_expr(e->child1);
      e->place = newtemp("arith");
      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = e->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = e->child1->place;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = e->place;
      e->code = newinstr(MINUS, src1, src2, dest);

      break;

    case MUL:
      codeGen_expr(e->child0);
      codeGen_expr(e->child1);
      e->place = newtemp("arith");
      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = e->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = e->child1->place;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = e->place;
      e->code = newinstr(MULTIPLE, src1, src2, dest);

      break;
    
    case DIV:
      codeGen_expr(e->child0);
      codeGen_expr(e->child1);
      e->place = newtemp("arith");
      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = e->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = e->child1->place;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = e->place;
      e->code = newinstr(DIVIDE, src1, src2, dest);

      break;

    case UMINUS:
      codeGen_expr(e->child0);
      e->place = newtemp("arith");
      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = e->child0->place;
      src2.operand_type = UNUSED;
      dest.operand_type = SYMBOL_PTR;
      dest.val.stptr = e->place;
      e->code = newinstr(UNARY, src1, src2, dest);
      break;

    default:
      break;
  }
}

void codeGen_bool(ASTnode* b, Quad* thenLabel, Quad* elseLabel) {
  Operand src1, src2, dest;
  Quad* head;
  Quad* label;
  switch (b->ntype)
  {
    case EQ:  // ...
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;
      
      if (enterWhile == 1) {
        head = newinstr(IF_EQ, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_EQ, src1, src2, thenLabel->src1);
      }
      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;

    case NE:
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;

      if (enterWhile == 1) {
        head = newinstr(IF_NE, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_NE, src1, src2, thenLabel->src1);
      }

      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;

    case LE:
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;

      if (enterWhile == 1) {
        head = newinstr(IF_LE, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_LE, src1, src2, thenLabel->src1);
      }

      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;

    case LT:
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;

      if (enterWhile == 1) {
        head = newinstr(IF_LT, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_LT, src1, src2, thenLabel->src1);
      }

      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;

    case GE:
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;

      if (enterWhile == 1) {
        head = newinstr(IF_GE, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_GE, src1, src2, thenLabel->src1);
      }

      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;
    
    case GT:
      codeGen_expr(b->child0);
      codeGen_expr(b->child1);

      src1.operand_type = SYMBOL_PTR;
      src1.val.stptr = b->child0->place;
      src2.operand_type = SYMBOL_PTR;
      src2.val.stptr = b->child1->place;
      if (enterWhile == 1) {
        head = newinstr(IF_GT, src1, src2, elseLabel->src1);
      } else {
        head = newinstr(IF_GT, src1, src2, thenLabel->src1);
      }

      head->next = newinstr(GOTO, (Operand){.operand_type=UNUSED}, (Operand){.operand_type=UNUSED}, elseLabel->src1);

      b->code = head;

      break;

    case AND:
      label = newlabel();
      codeGen_bool(b->child0, label, elseLabel);
      codeGen_bool(b->child1, thenLabel, elseLabel);

      head = b->child0->code;
      label->next = b->child1->code;
      head->next = label;
      b->code = head;

      break;

    case OR:
      label = newlabel();
      codeGen_bool(b->child0, thenLabel, label);
      codeGen_bool(b->child1, thenLabel, elseLabel);

      head = b->child0->code;
      label->next = b->child1->code;
      head->next = label; 
      b->code = head;
      
      break;

    default:
      break;
  }
}

void printConditionalId(Quad* quad) {
  // load LHS
  if (strcmp(quad->src1.val.stptr->info, "var") == 0 || strcmp(quad->src1.val.stptr->info, "temp") == 0) {
    fprintf(stdout, "    lw $t0, -%d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
  } else if (strcmp(quad->src1.val.stptr->info, "global") == 0) {
    fprintf(stdout, "    lw $t0, _%s\n", quad->src1.val.stptr->name);
  } else if (strcmp(quad->src1.val.stptr->info, "arg") == 0) {
    fprintf(stdout, "    lw $t0, %d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
  }

  if (quad->op == PARAM) {
    return;
  }

  if (quad->src2.operand_type == UNUSED) {
    return;
  }

  // load RHS
  if (strcmp(quad->src2.val.stptr->info, "var") == 0 || strcmp(quad->src2.val.stptr->info, "temp") == 0) {
    fprintf(stdout, "    lw $t1, -%d($fp)\n", findSymbol(globalScope->next, quad->src2.val.stptr->name)->memLoc);
  } else if (strcmp(quad->src2.val.stptr->info, "arg") == 0) {
    fprintf(stdout, "    lw $t1, %d($fp)\n", findSymbol(globalScope->next, quad->src2.val.stptr->name)->memLoc);
  } else if (strcmp(quad->src2.val.stptr->info, "global") == 0) {
    fprintf(stdout, "    lw $t1, _%s\n", quad->src2.val.stptr->name);
  }
}

int passInReturn = 0;
int passInWhile = 0;

void generateMipsCode(Quad* quad) {
  switch (quad->op)
  {
  case INTEGER_CONST:
    /* Unused */
    break;
  
  case SYMBOL_PTR:
    /* Unused */
    break;

  case PARAM:
    fprintf(stdout, "    # PARAM _%s\n", quad->src1.val.stptr->name);

    printConditionalId(quad);

    fprintf(stdout, "    la $sp, -4($sp)\n");
    fprintf(stdout, "    sw $t0, 0($sp)\n\n");

    generateMipsCode(quad->next);
    break;

  case CALL:
    fprintf(stdout, "    # CALL\n");
    fprintf(stdout, "    jal _%s\n", quad->src1.val.name);
    fprintf(stdout, "    la $sp %d($sp)\n\n", quad->src2.val.iconst*4);

    if (quad->next != NULL) {  // To call Retrieve
      generateMipsCode(quad->next);
    }
    break;

  case RETRIEVE:
    fprintf(stdout, "    # RETRIEVE\n");
    fprintf(stdout, "    sw $v0, -%d($fp) \n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc); 
    break;

  
  case ASSG_STMT:  // Integer arguments,
    if (strcmp(quad->dest.val.stptr->info, "temp") == 0) {
      fprintf(stdout, "    # ASSG _%s = %d\n", quad->dest.val.stptr->name, quad->dest.val.stptr->num);
      fprintf(stdout, "    li $t0, %d\n", quad->dest.val.stptr->num);
      fprintf(stdout, "    sw $t0, -%d($fp)\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    } else {
      fprintf(stdout, "    # ASSG _%s = _%s\n", quad->dest.val.stptr->name, quad->src1.val.stptr->name);
      // load RHS
      if (strcmp(quad->src1.val.stptr->info, "var") == 0) {
        fprintf(stdout, "    lw $t0, -%d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
      } else if (strcmp(quad->src1.val.stptr->info, "arg") == 0) {
        fprintf(stdout, "    lw $t0, %d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
      } else if (strcmp(quad->src1.val.stptr->info, "global") == 0) {
        fprintf(stdout, "    lw $t0, _%s\n", quad->src1.val.stptr->name);
      } 
      else if (strcmp(quad->src1.val.stptr->info, "temp") == 0) {
        if (strcmp(quad->src1.val.stptr->type, "arith") != 0) {
          fprintf(stdout, "    li $t0, %d\n", quad->src1.val.stptr->num);
        } else {
          fprintf(stdout, "    lw $t0, -%d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
        }
      }

      // load LHS
      if (strcmp(quad->dest.val.stptr->info, "var") == 0) {
        fprintf(stdout, "    sw $t0, -%d($fp)\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
      } else if (strcmp(quad->dest.val.stptr->info, "arg") == 0) {
        fprintf(stdout, "    sw $t0, %d($fp)\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
      } else if (strcmp(quad->dest.val.stptr->info, "global") == 0) {
        fprintf(stdout, "    sw $t0, _%s\n", quad->dest.val.stptr->name);
      }
    }
    
    fprintf(stdout, "\n");
    break;

  case ENTER:
    fprintf(stdout, "# Enter function\n");
    fprintf(stdout, ".align 2\n");
    fprintf(stdout, ".text\n");
    fprintf(stdout, "_%s:\n", quad->src1.val.name);
    fprintf(stdout, "    la $sp, -8($sp) # allocate space for old $fp and $ra\n");
    fprintf(stdout, "    sw $fp, 4($sp) # save old $fp\n");
    fprintf(stdout, "    sw $ra, 0($sp) # save return address\n");
    fprintf(stdout, "    la $fp, 0($sp) # set up frame pointer\n");
    fprintf(stdout, "    la $sp, -%d($sp) # allocate stack frame\n\n", (temp_num + assgNum + 3) * 4);
    break;
  
  case LEAVE:
    /* Unused */
    break;
  
  case RETURN_STMT:
    passInReturn = 1;
    
    if (quad->src1.operand_type != UNUSED) {
      fprintf(stdout, "    # Return _%s;\n", quad->src1.val.stptr->name);
    } else {
      fprintf(stdout, "    # Return;\n");
    }

    if (quad->src1.operand_type != UNUSED) {
      if (strcmp(quad->src1.val.stptr->info, "var") == 0 || strcmp(quad->src1.val.stptr->info, "temp") == 0) {
        fprintf(stdout, "    lw $v0, -%d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
      } else if (strcmp(quad->src1.val.stptr->info, "global") == 0) {
        fprintf(stdout, "    lw $v0, _%s\n", quad->src1.val.stptr->name);
      } else if (strcmp(quad->src1.val.stptr->info, "arg") == 0) {
        fprintf(stdout, "    lw $v0, %d($fp)\n", findSymbol(globalScope->next, quad->src1.val.stptr->name)->memLoc);
      }
    }

    fprintf(stdout, "    la $sp, 0($fp)\n");
    fprintf(stdout, "    lw $ra, 0($sp)\n");
    fprintf(stdout, "    lw $fp, 4($sp)\n");
    fprintf(stdout, "    la $sp, 8($sp)\n");
    fprintf(stdout, "    jr $ra\n\n");

    break;

  case UNUSED:
    /* Unused */
    break;

  case LABEL:
    fprintf(stdout, "%s:\n", quad->src1.val.stptr->name);
    break;

  case IF_EQ:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s == %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s == %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }

    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) {
      fprintf(stdout, "    bne $t0, $t1, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    beq $t0, $t1, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }
    break;

  case IF_NE:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s != %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s != %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }
    
    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) {
      fprintf(stdout, "    beq $t0, $t1, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    bne $t0, $t1, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }
    break;

  case IF_GT:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s > %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s > %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }
    
    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) {
      fprintf(stdout, "    slt $t2, $t1, $t0\n");
      fprintf(stdout, "    beq $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    slt $t2, $t1, $t0\n");
      fprintf(stdout, "    bne $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }
    break;

  case IF_GE:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s >= %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s >= %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }
    
    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) {
      fprintf(stdout, "    slt $t2, $t0, $t1\n");
      fprintf(stdout, "    bne $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    slt $t2, $t0, $t1\n");
      fprintf(stdout, "    beq $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }

    break;

  case IF_LT:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s < %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s < %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }
    
    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) { 
      fprintf(stdout, "    slt $t2, $t0, $t1\n");
      fprintf(stdout, "    beq $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    slt $t2, $t0, $t1\n");
      fprintf(stdout, "    bne $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }

    break;

  case IF_LE:
    if (passInWhile == 1) {
      fprintf(stdout, "    # WHILE (%s <= %s) otherwise %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    # IF (%s <= %s) goto %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name, quad->dest.val.stptr->name);
    }
    printConditionalId(quad);

    // Conditional Check
    if (passInWhile == 1) {
      fprintf(stdout, "    slt $t2, $t1, $t0\n");
      fprintf(stdout, "    bne $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
    } else {
      fprintf(stdout, "    slt $t2, $t1, $t0\n");
      fprintf(stdout, "    beq $t2, $zero, %s\n\n", quad->dest.val.stptr->name);
      generateMipsCode(quad->next);
    }
    break;

  case GOTO:
    fprintf(stdout, "    # Jump to %s\n", quad->dest.val.stptr->name);
    fprintf(stdout, "    j %s\n\n", quad->dest.val.stptr->name);
    break;

  case PLUS:
    fprintf(stdout, "    # PLUS %s + %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name);

    printConditionalId(quad);
    fprintf(stdout, "    add $t2, $t0, $t1\n");
    fprintf(stdout, "    sw $t2, -%d($fp)\n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    break;

  case MINUS:
    fprintf(stdout, "    # SUB %s - %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name);

    printConditionalId(quad);
    fprintf(stdout, "    sub $t2, $t0, $t1\n");
    fprintf(stdout, "    sw $t2, -%d($fp)\n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    break;

  case MULTIPLE:
    fprintf(stdout, "    # MULT %s * %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name);

    printConditionalId(quad);
    fprintf(stdout, "    mul $t2, $t0, $t1\n");
    fprintf(stdout, "    sw $t2, -%d($fp)\n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    break;

  case DIVIDE:
    fprintf(stdout, "    # DIV %s / %s\n", quad->src1.val.stptr->name, quad->src2.val.stptr->name);

    printConditionalId(quad);
    fprintf(stdout, "    div $t2, $t0, $t1\n");
    fprintf(stdout, "    sw $t2, -%d($fp)\n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    break;

  case UNARY:
    fprintf(stdout, "    # Unary %s\n", quad->src1.val.stptr->name);

    printConditionalId(quad);
    fprintf(stdout, "    neg $t1, $t0\n");
    fprintf(stdout, "    sw $t1, -%d($fp)\n\n", findSymbol(globalScope->next, quad->dest.val.stptr->name)->memLoc);
    break;

  default:
    break;
  }
}

// It generate MIPS code for global variables
void addGlobalVars(Symbol* st_ref) {
  Symbol* curSym = st_ref;
  int count = 0;
  while (curSym != NULL) {
    if (curSym->globalVar == 0) {  // Means current global variable is already assigned
      if (strcmp(curSym->type, "var") == 0) {
        if (count == 0) {
          fprintf(stdout, ".data\n");
        }
        fprintf(stdout, "_%s: .space 4\n", curSym->name);
        curSym->globalVar = 1;
        count++;
      }
    }

    curSym = curSym->next;
  }

  fprintf(stdout, "\n");
}

// It prints temporaries as the statements needed
void printRAFChild(ASTnode* node) {
  if (node->ntype == FUNC_CALL) {
    ASTnode* expr_list = node->child0;
    while (expr_list != NULL) {
      if (expr_list->child0->code != NULL) {
        generateMipsCode(expr_list->child0->code);
      }
      expr_list = expr_list->child1;
    }
  } else if (node->ntype == RETURN) {
    if (node->child0 != NULL && node->child0->code != NULL) {
      generateMipsCode(node->child0->code);
    }
  }
}

void generateMips(ASTnode* node) {
  int passedDef = 0;
  passInReturn = 0;
  if (attempt == 0) {
    const char* printFn =
      ".align 2\n"
      ".data\n"
      "nl: .asciiz \"\\n\"\n"
      ".align 2\n"
      ".text\n"
      "_println:\n"
      "    li $v0, 1\n"
      "    lw $a0, 0($sp)\n"
      "    syscall\n"
      "    li $v0, 4\n"
      "    la $a0, nl\n"
      "    syscall\n"
      "    jr $ra\n\n";

    fprintf(stdout, "%s", printFn);
    attempt += 1;
  }
  addGlobalVars(node->st_ref);

  ASTnode* curNode = node;
  while (curNode != NULL) {
    passInWhile = 0;
    // Code to read through AST
    if (curNode->ntype == FUNC_DEF) {
      generateMipsCode(curNode->code);
      passedDef = 1;
      curNode = curNode->child0;
    } else if (curNode->ntype == STMT_LIST) {
      if (curNode->child0 == NULL) {  // Checks current node is semi-colon
        curNode = curNode->child1;
        continue;
      }

      if (curNode->child0->ntype == STMT_LIST) {  // Case for { opt_stmt_list }
        generateMips(curNode->child0);  // Recursive call to get into
        curNode = curNode->child1;  // Keeping through other opt_stmt_list
        continue;
      }

      if (curNode->child0->ntype == IF) {
        ASTnode* if_st = curNode->child0;
        ASTnode* if_then = if_st->child1;
        ASTnode* if_else = if_st->child2;
        if (if_st->child0->child0->code != NULL) {  // variable for LHS
          generateMipsCode(if_st->child0->child0->code);
        }
        if (if_st->child0->child1->code != NULL) {  // variable for RHS
          generateMipsCode(if_st->child0->child1->code);
        }
        generateMipsCode(if_st->child0->code);  // Boolean expression (IF_OP)

        Quad* curQuad = if_st->code;
        int count = 0;
        while (curQuad != NULL) {
          generateMipsCode(curQuad);  // Generating MIPS code on labels and goto statement
          if (count == 0) {
            if (if_then != NULL) {
              if (if_then->ntype == STMT_LIST || if_then->ntype == IF || if_then->ntype == WHILE) {
                generateMips(if_then);
              } else {  // RETURN, ASSG, FN_CALL
                printRAFChild(if_then);  // It prints temporaries as the statements needed
                generateMipsCode(if_then->code);
              }
            }
          } else if (count == 2) {
            if (if_else != NULL) {
              if (if_else->ntype == STMT_LIST || if_else->ntype == IF || if_else->ntype == WHILE) {
                generateMips(if_else);
              } else {  // RETURN, ASSG, FN_CALL
                printRAFChild(if_else);  // It prints temporaries as the statements needed
                generateMipsCode(if_else->code);
              }
            }
          }
          count++;
          curQuad = curQuad->next;
        }

      } else if (curNode->child0->ntype == WHILE) {  // Case while
        passInWhile = 1;
        ASTnode* while_st = curNode->child0;
        ASTnode* while_body = while_st->child1;

        if (while_st->child0->child0->code != NULL) {  // variable for LHS
          generateMipsCode(while_st->child0->child0->code);
        }
        if (while_st->child0->child1->code != NULL) {  // variable for RHS
          generateMipsCode(while_st->child0->child1->code);
        }

        Quad* curQuad = while_st->code;
        int count = 0;
        while (curQuad != NULL) {
          generateMipsCode(curQuad);  // Mips code for labels
          if (count == 0) {
            generateMipsCode(while_st->child0->code);  // Boolean Expression
          } else if (count == 1) {
            if (while_body != NULL) {
              if (while_body->ntype == STMT_LIST || while_body->ntype == IF || while_body->ntype == WHILE) {
                generateMips(while_body);
              } else {  // RETURN, ASSG, FN_CALL
                printRAFChild(while_body);  // It prints temporaries as the statements needed
                generateMipsCode(while_body->code);
              }
            }
          }

          count++;
          curQuad = curQuad->next;
        }

      } else if (curNode->child0->ntype == FUNC_CALL) {  // Case function call
        // Assign integer constants from arguments of the function call.
        ASTnode* expr_list = curNode->child0->child0;
        while (expr_list != NULL) {  // Read expr_list to set up arguments as needed
          if (expr_list->child0->code != NULL) {
            generateMipsCode(expr_list->child0->code);
          }
          expr_list = expr_list->child1;
        }

        generateMipsCode(curNode->child0->code);  // Write PARAMs and CALL
      } else {  // only for ASSG & RETURN
        if (curNode->child0->ntype == RETURN) {  // Case return
          ASTnode* return_ = curNode->child0;
          if (return_->child0 != NULL && return_->child0->code != NULL) {  // Checks return has value to return
            if (return_->child0->child0 != NULL && return_->child0->child0->code != NULL) {
              generateMips(return_->child0->child0);
            }
            if (return_->child0->child1 != NULL && return_->child0->child1->code != NULL) {
              generateMips(return_->child0->child1);

            }
            generateMipsCode(return_->child0->code);
          }
          generateMipsCode(curNode->child0->code);
        } else {  // For ASSG
          ASTnode* assg = curNode->child0;
          if (assg->child1 != NULL && assg->child1->code != NULL) {
            if (assg->child1->child0 != NULL && assg->child1->child0->code != NULL) {
              generateMips(assg->child1->child0);
            }
            if (assg->child1->child1 != NULL && assg->child1->child1->code != NULL) {
              generateMips(assg->child1->child1);
            }
            generateMipsCode(assg->child1->code);  // Will generate code if needed (RHS)
          }
          generateMipsCode(assg->code);
        }
      }
      curNode = curNode->child1;

    } else if (curNode->ntype == IF) {
      ASTnode* if_st = curNode;
      ASTnode* if_then = if_st->child1;
      ASTnode* if_else = if_st->child2;
      if (if_st->child0->child0->code != NULL) {  // variable for LHS
        generateMipsCode(if_st->child0->child0->code);
      }
      if (if_st->child0->child1->code != NULL) {  // variable for RHS
        generateMipsCode(if_st->child0->child1->code);
      }
      generateMipsCode(if_st->child0->code);  // Boolean expression (IF_OP)

      Quad* curQuad = if_st->code;
      int count = 0;
      while (curQuad != NULL) {
        generateMipsCode(curQuad);  // Generating MIPS code on labels and goto statement
        if (count == 0) {
          if (if_then != NULL) {
            if (if_then->ntype == STMT_LIST || if_then->ntype == IF || if_then->ntype == WHILE) {
              generateMips(if_then);
            } else {  // RETURN, ASSG, FN_CALL
              printRAFChild(if_then);  // It prints temporaries as the statements needed
              generateMipsCode(if_then->code);

            }
          }
        } else if (count == 2) {
          if (if_else != NULL) {
            if (if_else->ntype == STMT_LIST || if_else->ntype == IF || if_else->ntype == WHILE) {
              generateMips(if_else);
            } else {  // RETURN, ASSG, FN_CALL
              printRAFChild(if_else);  // It prints temporaries as the statements needed
              generateMipsCode(if_else->code);
            }
          }
        }
        count++;
        curQuad = curQuad->next;
      }
      break;

    } else if (curNode->ntype == WHILE) {
      passInWhile = 1;
      ASTnode* while_st = curNode;
      ASTnode* while_body = while_st->child1;

      if (while_st->child0->child0->code != NULL) {  // variable for LHS
        generateMipsCode(while_st->child0->child0->code);
      }
      if (while_st->child0->child1->code != NULL) {  // variable for RHS
        generateMipsCode(while_st->child0->child1->code);
      }

      Quad* curQuad = while_st->code;
      int count = 0;
      while (curQuad != NULL) {
        generateMipsCode(curQuad);  // Mips code for labels
        if (count == 0) {
          generateMipsCode(while_st->child0->code);  // Boolean Expression
        } else if (count == 1) {
          if (while_body != NULL) {
            if (while_body->ntype == STMT_LIST || while_body->ntype == IF || while_body->ntype == WHILE) {
              generateMips(while_body);
            } else {  // RETURN, ASSG, FN_CALL
              printRAFChild(while_body);  // It prints temporaries as the statements needed
              generateMipsCode(while_body->code);
            }
          }
        }

        count++;
        curQuad = curQuad->next;
      }
      break;
    } else {  // When expression is not only on integer or identifier
      if (curNode->ntype == FUNC_CALL) {
        ASTnode* expr_list = curNode->child0;
        while (expr_list != NULL) {  // Read expr_list to set up arguments as needed
          if (expr_list->child0->code != NULL) {
            if (expr_list->child0->child0 != NULL && expr_list->child0->child0->code != NULL) {
              generateMips(expr_list->child0->child1);
            }
            if (expr_list->child0->child1 != NULL && expr_list->child0->child1->code != NULL) {
              generateMips(expr_list->child0->child1);
            }
            generateMipsCode(expr_list->child0->code);
          }
          expr_list = expr_list->child1;
        }
        if (curNode->code == NULL) {
        }

        generateMipsCode(curNode->code);
      } else {  // Arithmetic expressions
        if (curNode->child0 != NULL && curNode->child0->code != NULL) {
          generateMips(curNode->child0);
        }
        if (curNode->child1 != NULL && curNode->child1->code != NULL) {
          generateMips(curNode->child1);
        }
        generateMipsCode(curNode->code);
      }
      break;
    }

  }

  if (passInReturn == 0 && passedDef == 1) {
    fprintf(stdout,
        "    li $t0, 0\n"
        "    sw $t0, -4($fp)\n\n"
        "# RETURN\n"
        "    lw $v0, -4($fp)\n"
        "    la $sp, 0($fp)\n"
        "    lw $ra, 0($sp)\n"
        "    lw $fp, 4($sp)\n"
        "    la $sp, 8($sp)\n"
        "    jr $ra\n\n");
  }

  if (node->name != NULL && strcmp(node->name, "main") == 0) {
    fprintf(stdout, ".align 2\n");
    fprintf(stdout, ".text\n");
    fprintf(stdout, "main:\n");
    fprintf(stdout, "    j _main\n\n");
  }
}