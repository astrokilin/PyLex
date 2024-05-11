#include "ast.h"

static ast_node* new_node(int type, int subtype, size_t node_type_size){
    ast_node *res;

    if (!(res = (ast_node*) malloc(node_type_size)))
        return 0;

    res -> type_info = (type << 2) | subtype;
    return res;
}

static void ast_free(ast_node* node){
    if (!node)
        return;

    switch(AST_NODE_GET_TYPE(node)){
        case AST_NODE_LEAF:
            break;

        case AST_NODE_BINOP:
            ast_free(TO_BINOP_NODE(node) -> l_arg);
            ast_free(TO_BINOP_NODE(node) -> r_arg);
            break;

        case AST_NODE_UNOP:
            ast_free(TO_UNOP_NODE(node) -> arg);
            break;
    }

    free(node);
}

void ast_dealloc(reg_ast *ast){
    if (!ast)
        return;

    ast_free(ast -> top_node);
    free(ast);
}


struct parsing_context{
    char *cur_str;
    unsigned int cur_accept_state;
    size_t states;
    struct error_data* err;
};

// TODO: redo this ugly parsing

static ast_node* parse_ere_content(struct parsing_context*);
static ast_node* parse_ere_exp(struct parsing_context*);
static ast_node* parse_ere_branch(struct parsing_context*);
static ast_node* parse_reg_subexp(struct parsing_context*);
static ast_node* parse_reg_exp(struct parsing_context*);

static ast_node* parse_ere_content(struct parsing_context* context){
    ast_node *top;
    char cur_sym;

    top = 0;
    cur_sym = *(context -> cur_str);

    if (cur_sym == '('){
        context -> cur_str++;

        if (!(top = parse_reg_subexp(context)))
            goto ERR_EXIT;

        if (*(context -> cur_str) != ')'){
            context -> err -> err_type = AST_ERR_SYN;
            goto ERR_EXIT;
        }

        context -> cur_str++;

    }else if (cur_sym == ')' || cur_sym == '|'){
        if (!(top = new_node(AST_NODE_LEAF, AST_NODE_LEAF_EMP, sizeof(ast_leaf_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

    }else if (cur_sym == '+' || cur_sym == '?' || cur_sym == '*' || cur_sym == 0){
        context -> err -> err_type = AST_ERR_SYN;
        goto ERR_EXIT;

    }else{
        if (!(top = new_node(AST_NODE_LEAF, AST_NODE_LEAF_SYM, sizeof(ast_leaf_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }
        TO_LEAF_NODE(top) -> sym = cur_sym;
        context -> cur_str++;
    }
 
    if (context -> states == ULONG_MAX){
        context -> err -> err_type = AST_ERR_OVF;
        goto ERR_EXIT;
    }

    context -> states++;

    return top;

ERR_EXIT:
    ast_free(top);
    return 0;
}

static ast_node* parse_ere_exp(struct parsing_context* context){
    ast_node *ere_content;
    ast_node *ere_dupl_sym;
    char next_sym;

    ere_content = 0;

    if (!(ere_content = parse_ere_content(context)))
        goto ERR_EXIT;

    next_sym = *context -> cur_str;

    if (next_sym == '*'){

        if (!(ere_dupl_sym = new_node(AST_NODE_UNOP, AST_NODE_UNOP_STAR, sizeof(ast_unop_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

        context -> cur_str++;
        TO_UNOP_NODE(ere_dupl_sym) -> arg = ere_content;
        ere_content = ere_dupl_sym;

    }else if (next_sym == '?'){

        if (!(ere_dupl_sym = new_node(AST_NODE_UNOP, AST_NODE_UNOP_QUEST, sizeof(ast_unop_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

        context -> cur_str++;
        TO_UNOP_NODE(ere_dupl_sym) -> arg = ere_content;
        ere_content = ere_dupl_sym;

    }else if (next_sym == '+'){

        if (!(ere_dupl_sym = new_node(AST_NODE_UNOP, AST_NODE_UNOP_PLUS, sizeof(ast_unop_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

        context -> cur_str++;
        TO_UNOP_NODE(ere_dupl_sym) -> arg = ere_content;
        ere_content = ere_dupl_sym;
    }

    return ere_content; 

ERR_EXIT:
    ast_free(ere_content);
    return 0;
}

static ast_node* parse_ere_branch(struct parsing_context* context){
    ast_node *top_ere_exp;
    ast_node *ere_exp;
    ast_node *tmp;

    top_ere_exp = 0;
    ere_exp = 0;

    if (!(top_ere_exp = parse_ere_exp(context)))
        goto ERR_EXIT;

    while (*context -> cur_str != '|' && *context -> cur_str != ')' && *context -> cur_str != 0){
        if (!(ere_exp = parse_ere_exp(context)))
            goto ERR_EXIT;

        if (!(tmp = new_node(AST_NODE_BINOP, AST_NODE_BINOP_CAT, sizeof(ast_binop_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

        TO_BINOP_NODE(tmp) -> l_arg = top_ere_exp;
        TO_BINOP_NODE(tmp) -> r_arg = ere_exp;

        top_ere_exp = tmp;
    }
    return top_ere_exp;

ERR_EXIT:
    ast_free(top_ere_exp);
    ast_free(ere_exp);
    return 0;
}

static ast_node* parse_reg_subexp(struct parsing_context* context){ 
    ast_node *top_ere_branch;
    ast_node *ere_branch;
    ast_node *tmp;

    top_ere_branch = 0;
    ere_branch = 0;

    if (!(top_ere_branch = parse_ere_branch(context)))
        goto ERR_EXIT;

    while(*context -> cur_str == '|'){
        context -> cur_str++;

        if (!(ere_branch = parse_ere_branch(context)))
            goto ERR_EXIT;

        if (!(tmp = new_node(AST_NODE_BINOP, AST_NODE_BINOP_OR, sizeof(ast_binop_node)))){
            context -> err -> err_type = AST_ERR_MEM;
            goto ERR_EXIT;
        }

        TO_BINOP_NODE(tmp) -> l_arg = top_ere_branch;
        TO_BINOP_NODE(tmp) -> r_arg = ere_branch;

        top_ere_branch = tmp;
    }

    return top_ere_branch;

ERR_EXIT:
    ast_free(top_ere_branch);
    ast_free(ere_branch);
    return 0;
}

static ast_node* parse_reg_exp(struct parsing_context* context){
    ast_node *reg_node;
    ast_node *acc_node;
    ast_node *cat_node;

    reg_node = 0;
    acc_node = 0;
    cat_node = 0;

    if (!(reg_node = parse_reg_subexp(context)))
        goto ERR_EXIT;

    if (!(acc_node = new_node(AST_NODE_LEAF, AST_NODE_LEAF_ACC, sizeof(ast_leaf_node)))){
        context -> err -> err_type = AST_ERR_MEM;
        goto ERR_EXIT;
    }

    if (context -> states == ULONG_MAX){
        context -> err -> err_type = AST_ERR_OVF;
        goto ERR_EXIT;
    }

    context -> states++;

    TO_LEAF_NODE(acc_node) -> sym = context -> cur_accept_state;

    if (!(cat_node = new_node(AST_NODE_BINOP, AST_NODE_BINOP_CAT, sizeof(ast_binop_node)))){
        context -> err -> err_type = AST_ERR_MEM;
        goto ERR_EXIT;
    }

    TO_BINOP_NODE(cat_node) -> l_arg = reg_node;
    TO_BINOP_NODE(cat_node) -> r_arg = acc_node;
    return cat_node;

ERR_EXIT:
    ast_free(reg_node);
    ast_free(acc_node);
    ast_free(cat_node);
    return 0;
}


reg_ast* ast_build(char **patterns, unsigned int patterns_l, struct error_data *err){
    struct parsing_context context;
    ast_node *prev_ast_top;
    ast_node *cur_ast_top;
    ast_node *tmp;
    reg_ast *res;

    res = 0;
    context.err = err;
    context.cur_accept_state = 0;
    context.states = 0;
    prev_ast_top = 0;
    cur_ast_top = 0;

    if (patterns_l > 0){

        context.cur_str = patterns[0];

        if (!(cur_ast_top = parse_reg_exp(&context)))
            goto ERR_EXIT;

        context.cur_accept_state++;

        for (;context.cur_accept_state < patterns_l; context.cur_accept_state++){
            context.cur_str = patterns[context.cur_accept_state];
            prev_ast_top = cur_ast_top;

            if (!(cur_ast_top = parse_reg_exp(&context)))
                goto ERR_EXIT;

            if (!(tmp = new_node(AST_NODE_BINOP, AST_NODE_BINOP_OR, sizeof(ast_binop_node)))){
                err -> err_type = AST_ERR_MEM;
                goto ERR_EXIT;
            }

            TO_BINOP_NODE(tmp) -> r_arg = cur_ast_top;
            TO_BINOP_NODE(tmp) -> l_arg = prev_ast_top;

            cur_ast_top = tmp; 
        }
    }

    if ((res = (reg_ast*) malloc(sizeof(reg_ast))) == NULL){
        err -> err_type = AST_ERR_MEM;
        goto ERR_EXIT;
    }

    res -> acc_states = patterns_l;
    res -> top_node = cur_ast_top;
    res -> states = context.states;
    return res;

ERR_EXIT:
    ast_free(prev_ast_top);
    ast_free(cur_ast_top);
    free(res);

    err -> err_ind = context.cur_accept_state;
    err -> err_offset = context.cur_str;
    return 0;
}

