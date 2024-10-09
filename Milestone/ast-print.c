/*
 * File: ast-print.c
 * Author: Saumya Debray
 * Purpose: Code to print syntax trees
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

char *opname(NodeType ntype);
static void print_ast_formatted(void *tree, int n, int nl);

/*
 * print_ast(tree) takes a pointer to an AST node and uses the getter
 * functions supplied by the user to traverse and print the tree below
 * that node.
 */
void print_ast(void *tree) {
  print_ast_formatted(tree, 0, 1);
}


/*******************************************************************************
 *                                                                             *
 *                                   HELPERS                                   *
 *                                                                             *
 ******************************************************************************/

/*
 * indent(n) : print out n spaces
 */
static void indent(int n) {
  assert(n >= 0);
  while (n-- > 0) putchar(' ');
}

#define SPACES_PER_INDENTATION_LEVEL  4

/*
 * print_ast_formatted(tree, n) takes a pointer to an AST node and uses the 
 * getter functions supplied by the user to traverse and print the tree.  The
 * second argument specifies a left-indentation level.  The third argument
 * specifies whether a newline should be printed at the end.
 */
static void print_ast_formatted(void *tree, int n, int nl) {
  NodeType ntype;
  char *name;
  void *list_hd, *list_tl;
  int i, nargs;

  int indent_amt = n * SPACES_PER_INDENTATION_LEVEL;

  if (tree == NULL) {
    return;
  }

  ntype = ast_node_type(tree);
  
  switch (ntype) {
  case FUNC_DEF:
    name = func_def_name(tree);
    printf("func_def: %s\n", name);  /* print the function's name */
    printf("  formals: ");           /* print the function's formals */
    nargs = func_def_nargs(tree);
    for (i = 1; i <= nargs; i++) {
      printf("%s", func_def_argname(tree, i));
      if (i < nargs) printf(", ");
    }

    printf("\n  body:\n");           /* print the function's body */
    print_ast_formatted(func_def_body(tree), n+1, 1);
    printf("/* func_def: %s */\n\n", name);
    break;

  case FUNC_CALL:
    indent(indent_amt);
    name = func_call_callee(tree);
    printf("%s(", name);  /* print the callee's name */
    print_ast_formatted(func_call_args(tree), 0, 0);   /* print the argument list */
    printf(")");
    if (nl != 0) {
      printf("\n");
    }
    break;

  case STMT_LIST:
    indent(indent_amt);
    printf("{\n");
    while (tree != NULL) {
      list_hd = stmt_list_head(tree);
      tree = stmt_list_rest(tree);
      print_ast_formatted(list_hd, n+1, nl);
    }
    indent(indent_amt);
    printf("}\n");
    break;

  case IF:
    indent(indent_amt); printf("if (");
    print_ast_formatted(stmt_if_expr(tree), 0, 0);
    printf("):\n");
    indent(indent_amt); printf("then:\n");
    print_ast_formatted(stmt_if_then(tree), n+1, nl);
    indent(indent_amt); printf("else:\n");
    print_ast_formatted(stmt_if_else(tree), n+1, nl);
    indent(indent_amt);
    printf("end_if\n");
    break;

  case ASSG:
    indent(indent_amt);
    printf("%s = ", stmt_assg_lhs(tree));
    print_ast_formatted(stmt_assg_rhs(tree), 0, 0);
    printf("\n");
    break;

  case WHILE:
    indent(indent_amt); printf("while (");
    print_ast_formatted(stmt_while_expr(tree), 0, 0);
    printf("):\n");
    print_ast_formatted(stmt_while_body(tree), n+1, 1);
    indent(indent_amt);
    printf("end_while\n");
    break;

  case RETURN:
    indent(indent_amt);
    printf("return: ");
    print_ast_formatted(stmt_return_expr(tree), 0, 0);
    printf("\n");
    break;

  case EXPR_LIST:
    list_tl = expr_list_rest(tree);
    print_ast_formatted(expr_list_head(tree), 0, 0);
    if (list_tl != NULL) {
      printf(", ");
    }
    print_ast_formatted(list_tl, 0, 0);
    break;

  case IDENTIFIER:
    printf("%s", expr_id_name(tree));
    break;

  case INTCONST:
    printf("%d", expr_intconst_val(tree));
    break;
    
  case UMINUS:
    printf("-(");
    print_ast_formatted(expr_operand_1(tree), 0, 0);
    printf(")");
    break;

  case EQ:
  case NE:
  case LE:
  case LT:
  case GE:
  case GT:
    print_ast_formatted(expr_operand_1(tree), 0, 0);
    printf(" %s ", opname(ntype));
    print_ast_formatted(expr_operand_2(tree), 0, 0);
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
    printf("(");
    print_ast_formatted(expr_operand_1(tree), 0, 0);
    printf(" %s ", opname(ntype));
    print_ast_formatted(expr_operand_2(tree), 0, 0);
    printf(")");
    break;

  case AND:
  case OR:
    printf("(");
    print_ast_formatted(expr_operand_1(tree), 0, 0);
    printf(") %s (", opname(ntype));
    print_ast_formatted(expr_operand_2(tree), 0, 0);
    printf(")");
    break;

  default:
    fprintf(stderr, "*** [%s] Unrecognized syntax tree node type %d\n",
	    __func__, ntype);
  }
}

/*
 * opname: return a string that is a readable representation of an
 * operator.
 */
char *opname(NodeType ntype) {
  switch (ntype) {
  case EQ:
    return "==";
  case NE:
    return "!=";
  case LE:
    return "<=";
  case LT:
    return "<";
  case GE:
    return ">=";
  case GT:
    return ">";
  case ADD:
    return "+";
  case SUB:        /* fall through */
  case UMINUS:
    return "-";
  case MUL:
    return "*";
  case DIV:
    return "/";
  case AND:
    return "&&";
  case OR:
    return "||";
    
  default:
      fprintf(stderr, "*** [%s] Unrecognized syntax tree node type %d\n", __func__, ntype);
      return NULL;
  }
}

// -------------------------------------------

NodeType ast_node_type(void *ptr) {
  ASTnode *node = (ASTnode *)ptr;
  assert(node != NULL);
  return node->ntype;
}

char * func_def_name(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is a function definition.
    if (node != NULL && node->ntype == FUNC_DEF) {
        return node->name; // Return the function name directly from the AST node.
    }
    return NULL; // Return NULL if the conditions are not met.
}

int func_def_nargs(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node == NULL && node->ntype != FUNC_DEF) {
        return 0; // Not a function definition
    }
    Symbol *symbol = node->st_ref; // Assuming child0 points to the first parameter
    while (symbol != NULL) {
        if (strcmp(symbol->name, node->name) == 0) {
          return symbol->args_num;
        }
        symbol = symbol->next; // Move to the next symbol in the list
    }

    return 0;
}

char *func_def_argname(void *ptr, int n) {
    ASTnode *node = (ASTnode *)ptr;
    if (node->ntype != FUNC_DEF || n <= 0) {
        return NULL; // Error handling or invalid 'n'
    }

    ASTnode* cur_node = node->child1;
    for (int i = 1; cur_node != NULL && i < n; i++) {
        cur_node = cur_node->child0;
    }

    if (cur_node != NULL && cur_node->ntype == IDENTIFIER) {
        return cur_node->name; // Found the nth parameter
    }

    return NULL; // Not found
}

void * func_def_body(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Verify that the node is a function definition.
    if (node != NULL && node->ntype == FUNC_DEF) {
        // Assuming child1 points to the function body, return it.
        return node->child0;
    }
    // If the conditions are not met, return NULL.
    return NULL;
}

// 
char *func_call_callee(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is a function call.
    if (node != NULL && node->ntype == FUNC_CALL) {
        return node->name;
        // Or, if the function name is stored as a string in this node (or its child), adjust accordingly.
    }
    return NULL; // Return NULL if conditions are not met.
}

void * func_call_args(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is a function call.
    if (node != NULL && node->ntype == FUNC_CALL) {
        return node->child0;
    }
    return NULL; // Return NULL if conditions are not met.
}

void * stmt_list_head(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is part of a statement list.
    if (node != NULL && node->ntype == STMT_LIST) {
        return node->child0; // Return the first statement in the list.
    }
    return NULL; // Return NULL if conditions are not met or the structure differs.
}

void * stmt_list_rest(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is part of a statement list.
    if (node != NULL && node->ntype == STMT_LIST) {
        return node->child1; // Return the next node in the list.
    }
    return NULL; // Return NULL if conditions are not met or the structure differs.
}

void * expr_list_head(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is part of an expression list.
    if (node != NULL && node->ntype == EXPR_LIST) {
        return node->child0; // Return the first expression in the list.
    }
    return NULL; // Return NULL if conditions are not met.
}

void * expr_list_rest(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is part of an expression list.
    if (node != NULL && node->ntype == EXPR_LIST) {
        return node->child1; // Return the rest of the list.
    }
    return NULL; // Return NULL if conditions are not met.
}


char *expr_id_name(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == IDENTIFIER) {
        // Assuming the identifier's name is stored in the symbol table reference
        return node->name;
    }
    return NULL; // If the node is not an IDENTIFIER or ptr is NULL
}

int expr_intconst_val(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == INTCONST) {
        return node->num; // Assuming the integer constant value is stored in 'num'
    }
    return 0; // If the node is not an INTCONST or ptr is NULL
}

void *expr_operand_1(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL) {
        return node->child0; // Assuming the first operand is stored in child0
    }
    return NULL; // If the conditions are not met
}

void *expr_operand_2(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL) {
        return node->child1; // Assuming the second operand is stored in child1
    }
    return NULL; // If the conditions are not met
}


void * stmt_if_expr(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == IF) {
      if (node->child0->ntype == EQ) {
      }
        return node->child0; // Assuming the expression is stored in child0
    }
    return NULL; // Return NULL if the node is not an IF statement or if ptr is NULL
}


void * stmt_if_then(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == IF) {

        return node->child1; // Assuming the then-part is stored in child1
    }
    return NULL; // Return NULL if the node is not an IF statement or if ptr is NULL
}


void * stmt_if_else(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == IF) {
        return node->child2; // Assuming the else-part is stored in child2
    }
    return NULL; // Return NULL if the node is not an IF statement, if ptr is NULL, or if there is no else-part
}


char *stmt_assg_lhs(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is an assignment statement.
    if (node != NULL && node->ntype == ASSG) {
        return node->child0->name;
    }
    return NULL; // Return NULL if conditions are not met.
}


void *stmt_assg_rhs(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    // Ensure the node is an assignment statement.
    if (node != NULL && node->ntype == ASSG) {
        // Assuming the right-hand side expression is stored in child0.
        // Adjust this according to your AST structure if it uses a different child.
        return node->child1;
    }
    return NULL; // Return NULL if conditions are not met.
}


void *stmt_while_expr(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == WHILE) {
        // Assuming the condition expression is stored in child0
        return node->child0;
    }
    return NULL; // Return NULL if conditions are not met.
}


void *stmt_while_body(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == WHILE) {
        // Assuming the body of the while statement is stored in child1
        return node->child1;
    }
    return NULL; // Return NULL if conditions are not met.
}

void *stmt_return_expr(void *ptr) {
    ASTnode *node = (ASTnode *)ptr;
    if (node != NULL && node->ntype == RETURN) {
        // Assuming the return expression is stored in child0
        return node->child0;
    }
    return NULL; // Return NULL if conditions are not met.
}
