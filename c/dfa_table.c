#include "dfa_table.h"

#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "list.h"
#include "long_set.h"
#include "object_set.h"
#include "queue.h"

struct dfa_state {
    long_set nfa_states;
    unsigned long state_num;
};

static struct dfa_state *dfa_state_alloc(unsigned long state_num)
{
    struct dfa_state *res;

    if (!(res = ((struct dfa_state *)malloc(sizeof(struct dfa_state)))))
        return 0;

    long_set_init((long_set *)res);
    res->state_num = state_num;
    return res;
}

struct calc_frame {
    unsigned char maximum_ind;
    unsigned char minimum_ind;
    unsigned char transitions_len;
    unsigned char transitions_ind[UCHAR_MAX];
    struct dfa_state *transitions[UCHAR_MAX];
};

#define FRAME_TRANSITION_LEN (sizeof(((struct calc_frame *)0)->transitions))
// index of i'th nonempty entry in frame
#define frame_ind_nonempty(frame, i) ((frame)->transitions_ind[(i)])
// pointer to i'th nonempty set in frame
#define frame_nonempty(frame, i) \
    ((frame)->transitions[(frame)->transitions_ind[(i)]])

inline static void frame_clear(struct calc_frame *f)
{
    memset(f->transitions, 0, FRAME_TRANSITION_LEN);
    f->transitions_len = 0;
    f->minimum_ind = UCHAR_MAX;
    f->maximum_ind = 0;
}

inline static struct calc_frame *frame_alloc(void)
{
    struct calc_frame *res;

    if (!(res = (struct calc_frame *)malloc(sizeof(struct calc_frame))))
        return 0;

    frame_clear(res);
    return res;
}

// very carefull when freeing frame, it may have pointers from d_states

inline static void frame_dealloc_internal(struct calc_frame *f, unsigned long d_states_len)
{
    struct dfa_state *state;

    if (!f)
        return;

    for (unsigned char i = 0; i < f->transitions_len; i++) {
        state = f->transitions[f->transitions_ind[i]];
        if (state->state_num >= d_states_len)
            long_set_dealloc(
                (long_set *)f->transitions[f->transitions_ind[i]]);
    }
}

inline static void frame_dealloc(struct calc_frame *f)
{
    free(f);
}

static int frame_add(struct calc_frame *f, unsigned char sym, long_set *state_followpos,
          unsigned long d_states_len)
{
    struct dfa_state *followpos_union;

    followpos_union = f->transitions[sym];

    if (followpos_union) {
        if (!long_set_insert_set((long_set *)followpos_union, state_followpos,
                                 0))
            return 0;
        return 1;
    }

    if (!(followpos_union = dfa_state_alloc(d_states_len)))
        return 0;

    if (!(long_set_copy_init((long_set *)followpos_union, state_followpos))) {
        long_set_dealloc((long_set *)followpos_union);
        return 0;
    }

    if (sym < f->minimum_ind)
        f->minimum_ind = sym;

    if (sym > f->maximum_ind)
        f->maximum_ind = sym;

    f->transitions[sym] = followpos_union;
    f->transitions_ind[f->transitions_len++] = sym;
    return 1;
}

struct unfinished_dfa_table {
    unsigned char done;
    int_list acc_states;
    long_list bases;
    int_list next;
    int_list check;
};

static int unf_dfa_table_init(struct unfinished_dfa_table *t)
{
    if (!int_list_init(&t->acc_states))
        goto INIT_FAIL_0;

    if (!long_list_init(&t->bases))
        goto INIT_FAIL_1;

    if (!int_list_init(&t->next))
        goto INIT_FAIL_2;

    if (!int_list_init(&t->check))
        goto INIT_FAIL_3;

    t->done = 0;
    return 1;

INIT_FAIL_3:
    int_list_deinit(&t->next);
INIT_FAIL_2:
    long_list_deinit(&t->bases);
INIT_FAIL_1:
    int_list_deinit(&t->acc_states);
INIT_FAIL_0:
    return 0;
}

static void unf_dfa_table_deinit(struct unfinished_dfa_table *t){
    if (t->done)
        return;

    int_list_deinit(&t->next);
    long_list_deinit(&t->bases);
    int_list_deinit(&t->acc_states);
    int_list_deinit(&t->check);
}

// returns 0 on memory failure, -1 on overflow, 1 on succes
static int unf_dfa_table_add_state(
        struct unfinished_dfa_table *t, 
        unsigned int cur_state,
        unsigned int acc_target, 
        struct calc_frame *f){
    unsigned long base;
    size_t next_check_len;

    base = 0;
    next_check_len = list_len(&t->next);

    if (!int_list_append(&t->acc_states, acc_target))
        return 0;

    if (!f->transitions_len) {
        if (!long_list_append(&t->bases, base))
            return 0;

        return 1;
    }

    for (; base + f->minimum_ind < next_check_len; base++) {
        if (list_get(&t->next, base + f->minimum_ind) != 0) {
        CONTINUE_NESTED:
            continue;
        }

        for (unsigned char j = 0; j < f->transitions_len; j++) {
            if (base + frame_ind_nonempty(f, j) < next_check_len &&
                list_get(&t->next, base + frame_ind_nonempty(f, j)))
                goto CONTINUE_NESTED;
        }
        // at this point base is correct
        break;
    }

    if (base > SIZE_MAX - f->maximum_ind)
        return -1;

    // extend transitions arrays
    while (list_len(&t->next) <= base + f->maximum_ind) {
        if (!int_list_append(&t->next, 0) || !int_list_append(&t->check, 0))
            return 0;
    }

    for (unsigned char i = 0; i < f->transitions_len; i++) {
        int_list_write(
                &t->next, 
                frame_nonempty(f, i)->state_num + 1,
                base + frame_ind_nonempty(f, i));

        int_list_write(
                &t->check, 
                cur_state + 1,
                base + frame_ind_nonempty(f, i));
    }

    if (!(long_list_append(&t->bases, base)))
        return 0;

    return 1;
}

static int unf_dfa_table_complete(struct unfinished_dfa_table *t, dfa_table *result)
{
    t->done = 1;
    result->states = list_len(&t->bases);
    result->transitions_len = list_len(&t->next);

    for (size_t i = 0; i < list_len(&t->next); i++) {
        if (list_get(&t->next, i))
            int_list_write(&t->next, list_get(&t->next, i) - 1, i);
    }

    if (!(result->acc_state = int_list_to_array(&t->acc_states)))
        goto COMPLETE_FAIL;

    if (!(result->bases = long_list_to_array(&t->bases)))
        goto COMPLETE_FAIL;

    if (!(result->next = int_list_to_array(&t->next)))
        goto COMPLETE_FAIL;

    if (!(result->check = int_list_to_array(&t->check)))
        goto COMPLETE_FAIL;

    return 1;

COMPLETE_FAIL:
    int_list_deinit(&t->next);
    long_list_deinit(&t->bases);
    int_list_deinit(&t->acc_states);
    int_list_deinit(&t->check);
    return 0;
}

static int handle_ast_error(struct error_data *ast_err, dfa_table_syntax_error *err_syn)
{
    switch (ast_err->err_type) {
        case AST_ERR_MEM:
            return DFA_BUILD_ERR_NOMEM;
            break;

        case AST_ERR_SYN:
            snprintf(err_syn->err_str, 64,
                     "error in pattern %d near symbol %c\n", ast_err->err_ind,
                     *(ast_err->err_offset));
            return DFA_BUILD_ERR_SYNTAX;
            break;

        case AST_ERR_OVF:
            return DFA_BUILD_ERR_OVERFLOW;
            break;
    }
    return 0;
}

int dfa_table_init(
        dfa_table *d, 
        char **patterns, 
        unsigned int patterns_l,
        dfa_table_syntax_error *err)
{
    int result_code;
    struct error_data ast_err;
    struct calc_frame *frame;
    reg_ast *ast;
    struct dfa_state *tmp_state;
    struct dfa_state *cur_state;
    nfa_table nfa_t;
    object_set d_states;
    obj_queue Q;
    struct unfinished_dfa_table dfa_t;
    unsigned long state;
    unsigned int acc_state;
    long_set_iterator states_iterator;
    unsigned long old_d_state_len;

    result_code = DFA_BUILD_ERR_NOMEM;
    ast = 0;
    frame = 0;
    object_set_init(&d_states);

    // structures init

    if (!(frame = frame_alloc()))
        goto BUILD_CLEANUP_0;

    if (!(cur_state = dfa_state_alloc(0)))
        goto BUILD_CLEANUP_0;

    if (!object_set_insert(&d_states, cur_state,
                           (compare_func *)long_set_compare, 0)) {
        long_set_dealloc((long_set *)cur_state);
        goto BUILD_CLEANUP_0;
    }

    if (!unf_dfa_table_init(&dfa_t))
        goto BUILD_CLEANUP_1;

    if (!obj_queue_init(&Q))
        goto BUILD_CLEANUP_2;

    if (!(ast = ast_build(patterns, patterns_l, &ast_err))) {
        result_code = handle_ast_error(&ast_err, err);
        goto BUILD_CLEANUP_2;
    }

    if (!(ast_init_nfa_table(ast, &nfa_t, (long_set *)cur_state)))
        goto BUILD_CLEANUP_2;

    ast_dealloc(ast);
    ast = 0;

    // start of building

    obj_queue_push(&Q, cur_state);

    while (Q.len) {
        cur_state = (struct dfa_state *)obj_queue_pop(&Q);
        acc_state = 0;
        // fill frame

        long_set_iterator_init(&states_iterator, (long_set *)cur_state);

        while (long_set_iterator_next(&states_iterator, &state) ==
               LONG_ITERATOR_NEXT_SUCCES) {
            // accepting states dont go into frame
            if (nfa_t.state_targets[state]) {
                if (acc_state < nfa_t.state_targets[state])
                    acc_state = nfa_t.state_targets[state];
                continue;
            }

            if (!frame_add(
                        frame, 
                        nfa_t.symbols[state],
                        &nfa_t.followpos_sets[state], 
                        d_states.len)) {
                frame_dealloc_internal(frame, d_states.len);
                goto BUILD_CLEANUP_FULL;
            }
        }

        // all newly created dfa_states sets in frame will have state_num >=
        // current d_states len
        old_d_state_len = d_states.len;

        for (unsigned char i = 0; i < frame->transitions_len; i++) {
            tmp_state = frame_nonempty(frame, i);

            // check for d_states membership
            switch (object_set_insert(&d_states, tmp_state,
                                      (compare_func *)long_set_compare,
                                      (void **)&tmp_state)) {
                case OBJECT_SET_INSERT_SUCCES:
                    tmp_state->state_num = d_states.len - 1;

                    if (!obj_queue_push(&Q, tmp_state))
                        goto BUILD_CLEANUP_FULL;

                    break;

                case OBJECT_SET_INSERT_DUPLICATE:
                    long_set_dealloc((long_set *)frame_nonempty(frame, i));
                    frame_nonempty(frame, i) = tmp_state;
                    break;

                case OBJECT_SET_INSERT_ERROR:
                    frame_dealloc_internal(frame, old_d_state_len);
                    goto BUILD_CLEANUP_FULL;
                    break;
            }
        }

        // frame is ready to insert
        if (!unf_dfa_table_add_state(
                    &dfa_t, 
                    cur_state->state_num, 
                    acc_state,
                    frame))
            goto BUILD_CLEANUP_FULL;

        // clear frame
        frame_clear(frame);
    }

    // build is done
    if (!(unf_dfa_table_complete(&dfa_t, d)))
        goto BUILD_CLEANUP_FULL;

    result_code = DFA_BUILD_SUCCES;

BUILD_CLEANUP_FULL:
    nfa_table_deinit(&nfa_t);
BUILD_CLEANUP_2:
    obj_queue_deinit(&Q);
BUILD_CLEANUP_1:
    unf_dfa_table_deinit(&dfa_t);
BUILD_CLEANUP_0:
    frame_dealloc(frame);
    object_set_deinit(&d_states, (dealloc_func *)long_set_dealloc);
    ast_dealloc(ast);
    return result_code;
}
