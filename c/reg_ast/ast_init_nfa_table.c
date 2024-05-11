#include "ast.h"


// for every state i in s_1 inserts s_2 into followpos(i)
static int followpos_inserter(long_set *s_1, long_set *s_2, nfa_table *t){
    long_set_iterator iter;  
    unsigned long state;

    long_set_iterator_init(&iter, s_1);

    while (long_set_iterator_next(&iter, &state) == LONG_ITERATOR_NEXT_SUCCES){
        if (!long_set_insert_set(&(t -> followpos_sets[state]), s_2, 0)) 
            return 0;
    }

    return 1;
}

// union(s_2, s_1) will be stored in s_2, s_1 will be min(s_2, s_1)
static int set_joiner(long_set *s_1, long_set *s_2){
    long_set tmp;

    if (long_set_get_len(s_1) > long_set_get_len(s_2)){
        tmp = *s_2;
        *s_2 = *s_1;
        *s_1 = tmp;
    }

    if (!(long_set_insert_set(s_2, s_1, 0)))
        return 0;

    return 1;
}

struct calc_res{
    char nullable;
    long_set firstpos; 
    long_set lastpos; 
};

static int calc_node(ast_node*, struct calc_res*, nfa_table*);

inline static int calc_leaf_node(ast_node *n, struct calc_res *r, nfa_table *t){
    switch (AST_NODE_GET_SUBTYPE(n)){
        case AST_NODE_LEAF_ACC:
            
            if (!long_set_insert(&r -> firstpos, t -> states_count))
                goto CALC_FAILED;

            if (!long_set_insert(&r -> lastpos, t -> states_count))
                goto CALC_FAILED;
 
            r -> nullable = 1;
            t -> state_targets[t -> states_count] = TO_LEAF_NODE(n) -> sym + 1;
            break;

        case AST_NODE_LEAF_SYM:

            if (!long_set_insert(&r -> firstpos, t -> states_count))
                goto CALC_FAILED;

            if (!long_set_insert(&r -> lastpos, t -> states_count))
                goto CALC_FAILED;

            r -> nullable = 0;
            t -> symbols[t -> states_count] = TO_LEAF_NODE(n) -> sym;    
            break;

        case AST_NODE_LEAF_EMP:
            r -> nullable = 1;
            break;
    }

    t -> states_count++;
    return 1;

CALC_FAILED:
    long_set_deinit(&r -> firstpos);
    long_set_deinit(&r -> lastpos);
    return 0;
}

inline static int calc_unop_node(ast_node *n, struct calc_res *r, nfa_table *t){
    struct calc_res sub_res;

    long_set_init(&sub_res.firstpos);
    long_set_init(&sub_res.lastpos);

    if (!(calc_node(TO_UNOP_NODE(n) -> arg, &sub_res, t)))
        goto CALC_FAILED;

    switch (AST_NODE_GET_SUBTYPE(n)){
        case AST_NODE_UNOP_STAR:
            
            if (!(followpos_inserter(&sub_res.lastpos, &sub_res.firstpos, t)))
                goto CALC_FAILED;

            r -> nullable = 1;
            r -> firstpos = sub_res.firstpos;
            r -> lastpos = sub_res.lastpos;
            break;

        case AST_NODE_UNOP_QUEST:
            r -> nullable = 1;
            r -> firstpos = sub_res.firstpos;
            r -> lastpos = sub_res.lastpos;
            break;
           
        case AST_NODE_UNOP_PLUS:
  
            if (!(followpos_inserter(&sub_res.lastpos, &sub_res.firstpos, t)))
                goto CALC_FAILED;

            r -> nullable = 0;
            r -> firstpos = sub_res.firstpos;
            r -> lastpos = sub_res.lastpos;
            break;
    }

    return 1;

CALC_FAILED:
    long_set_deinit(&sub_res.firstpos);
    long_set_deinit(&sub_res.lastpos);
    return 0;
}

inline static int calc_binop_node(ast_node *n, struct calc_res *r, nfa_table *t){
    struct calc_res sub_res_1;
    struct calc_res sub_res_2;

    long_set_init(&sub_res_1.firstpos);
    long_set_init(&sub_res_1.lastpos);
    long_set_init(&sub_res_2.firstpos);
    long_set_init(&sub_res_2.lastpos);

    if (!(calc_node(TO_BINOP_NODE(n) -> l_arg, &sub_res_1, t)))
        goto CALC_FAILED;

    if (!(calc_node(TO_BINOP_NODE(n) -> r_arg, &sub_res_2, t)))
        goto CALC_FAILED;

    switch (AST_NODE_GET_SUBTYPE(n)){
        case AST_NODE_BINOP_OR:
            // &sub_res_2.firstpos holds the union result
            if (!(set_joiner(&sub_res_1.firstpos, &sub_res_2.firstpos)))
                goto CALC_FAILED;

            // &sub_res_2.lastpos holds the union result
            if (!(set_joiner(&sub_res_1.lastpos, &sub_res_2.lastpos)))
                goto CALC_FAILED;

   
            long_set_deinit(&sub_res_1.firstpos);
            long_set_deinit(&sub_res_1.lastpos);

            r -> nullable = sub_res_1.nullable || sub_res_2.nullable;
            r -> firstpos = sub_res_2.firstpos;
            r -> lastpos = sub_res_2.lastpos;
            break;

        case AST_NODE_BINOP_CAT:
 
            if (!(followpos_inserter(&sub_res_1.lastpos, &sub_res_2.firstpos, t)))
                goto CALC_FAILED;

            if (sub_res_1.nullable && !(set_joiner(&sub_res_2.firstpos, &sub_res_1.firstpos)))
                goto CALC_FAILED;

            if (sub_res_2.nullable && !(set_joiner(&sub_res_1.lastpos, &sub_res_2.lastpos)))
                goto CALC_FAILED;

            long_set_deinit(&sub_res_2.firstpos);
            long_set_deinit(&sub_res_1.lastpos);

            r -> nullable = sub_res_1.nullable && sub_res_2.nullable;
            r -> firstpos = sub_res_1.firstpos;
            r -> lastpos = sub_res_2.lastpos;
            break;
    }

    return 1;

CALC_FAILED:
    long_set_deinit(&sub_res_1.firstpos);
    long_set_deinit(&sub_res_1.lastpos);
    long_set_deinit(&sub_res_2.firstpos);
    long_set_deinit(&sub_res_2.lastpos);
    return 0;
}


static int calc_node(ast_node *n, struct calc_res *r, nfa_table *t){

    switch (AST_NODE_GET_TYPE(n)){
        case AST_NODE_LEAF:
            return calc_leaf_node(n, r, t);

        case AST_NODE_UNOP:
            return calc_unop_node(n, r, t);

        case AST_NODE_BINOP:
            return calc_binop_node(n, r, t);
    }

    return 1;
}

int ast_init_nfa_table(reg_ast *ast, nfa_table *t, long_set *firstpos){
    struct calc_res res;

    long_set_init(&res.firstpos);
    long_set_init(&res.lastpos);
    t -> state_targets = 0;
    t -> states_count = 0;
    t -> symbols = 0;
    t -> followpos_sets = 0;

    if (!(t->followpos_sets = (long_set*) calloc(ast -> states, sizeof(long_set))))
        goto CALC_FAILED;

    if (!(t->symbols = (unsigned char*) malloc(ast -> states)))
        goto CALC_FAILED;

    if (!(t->state_targets = (unsigned int*) calloc(ast -> states, sizeof(unsigned int))))
        goto CALC_FAILED;

    if (!(calc_node(ast -> top_node, &res, t)))
        goto CALC_FAILED;

    *firstpos = res.firstpos;

    long_set_deinit(&res.lastpos);
    return 1;

CALC_FAILED:
    free(t -> followpos_sets);
    free(t -> symbols);
    free(t -> state_targets);
    return 0;
}

void nfa_table_deinit(nfa_table *t){
    for (size_t i = 0; i < t -> states_count; i++)
        long_set_deinit(&(t -> followpos_sets[i]));

    free(t -> state_targets);
    free(t -> followpos_sets);
    free(t -> symbols);
}
