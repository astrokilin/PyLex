#ifndef REG_AST_H
#define REG_AST_H

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#include "long_set.h"

#define AST_NODE_LEAF           0
#define AST_NODE_LEAF_ACC       0 // internal
#define AST_NODE_LEAF_SYM       1 // symbol
#define AST_NODE_LEAF_EMP       2 // internal

#define AST_NODE_BINOP          1
#define AST_NODE_BINOP_OR       0 // |
#define AST_NODE_BINOP_CAT      1 // concatenation

#define AST_NODE_UNOP           2
#define AST_NODE_UNOP_STAR      0 // *
#define AST_NODE_UNOP_QUEST     1 // ?
#define AST_NODE_UNOP_PLUS      2 // +

#define TO_AST_NODE(node) ((ast_node*)(node))
#define TO_LEAF_NODE(ast_node) ((ast_leaf_node*)(ast_node))
#define TO_BINOP_NODE(ast_node) ((ast_binop_node*)(ast_node))
#define TO_UNOP_NODE(ast_node) ((ast_unop_node*)(ast_node))

#define AST_NODE_GET_TYPE(ast_node) ((ast_node) -> type_info >> 2)
#define AST_NODE_GET_SUBTYPE(ast_node) ((ast_node) -> type_info & 3)

typedef struct{
    char type_info;
}ast_node;

typedef struct{
    ast_node node;
    ast_node *l_arg;
    ast_node *r_arg; 
}ast_binop_node;

typedef struct {
    ast_node node;
    ast_node *arg;
}ast_unop_node;

typedef struct {
    ast_node node;
    unsigned int sym;
}ast_leaf_node;


#define AST_ERR_MEM 0
#define AST_ERR_SYN 1
#define AST_ERR_OVF 2

//TODO extend this structure to provide more info about regex syntax error

struct error_data{
    int err_type;
    unsigned int err_ind;
    char* err_offset;
};

typedef struct{
    unsigned int acc_states;
    unsigned long states;
    ast_node* top_node;
}reg_ast;

typedef struct{
    unsigned int *state_targets;
    unsigned long states_count;
    long_set *followpos_sets;     
    unsigned char *symbols;
}nfa_table;

reg_ast* ast_build(char **patterns, unsigned int patterns_l, struct error_data *err);
void ast_dealloc(reg_ast*);

int ast_init_nfa_table(reg_ast*, nfa_table*, long_set*);

void nfa_table_deinit(nfa_table*);

#endif
