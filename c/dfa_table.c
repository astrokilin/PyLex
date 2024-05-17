#include <string.h>

#include "dfa_table.h"
#include "nfa_table.h"
#include "list.h"
#include "long_set.h"
#include "object_set.h"
#include "queue.h"

// stuff for building intermediate dfa table
// which is just a list of i_dfa_entries

struct nfa_to_dfa_state {
    long_set nfa_states;
    state_num_t state_num;
};

static struct nfa_to_dfa_state *nfa_to_dfa_state_alloc(state_num_t state_num)
{
    struct nfa_to_dfa_state *res;

    if (!(res = ((struct nfa_to_dfa_state *)malloc(sizeof(struct nfa_to_dfa_state)))))
        return 0;

    long_set_init((long_set *)res);
    res->state_num = state_num;
    return res;
}

struct calc_frame {
    alphabet_t maximum_ind;
    alphabet_t minimum_ind;
    alphabet_t transitions_len;
    alphabet_t transitions_ind[ALPHABET_MAX_SIZE];
    struct nfa_to_dfa_state *transitions[ALPHABET_MAX_SIZE];
};

#define FRAME_TRANSITIONS_SIZE (sizeof(((struct calc_frame *)0)->transitions))
// index of i'th nonempty entry in frame
#define frame_ind_nonempty(frame, i) ((frame)->transitions_ind[(i)])
// pointer to i'th nonempty set in frame
#define frame_nonempty(frame, i) \
    ((frame)->transitions[(frame)->transitions_ind[(i)]])

inline static void frame_clear(struct calc_frame *f)
{
    memset(f->transitions, 0, FRAME_TRANSITIONS_SIZE);
    f->transitions_len = 0;
    f->minimum_ind = ALPHABET_MAX_SIZE;
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

inline static void frame_dealloc_internal(struct calc_frame *f, state_num_t d_states_len)
{
    struct nfa_to_dfa_state *state;

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

static int frame_add(struct calc_frame *f, alphabet_t sym, long_set *state_followpos,
          size_t d_states_len)
{
    struct nfa_to_dfa_state *followpos_union;

    followpos_union = f->transitions[sym];

    if (followpos_union) {
        if (!long_set_insert_set((long_set *)followpos_union, state_followpos, 0))
            return 0;
        return 1;
    }

    if (!(followpos_union = nfa_to_dfa_state_alloc(d_states_len)))
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

struct i_dfa_transition{
    alphabet_t symbol;
    state_num_t next_state;
};

struct i_dfa_table_entry_header{
    alphabet_t entry_len;
    alphabet_t max_sym;
    alphabet_t min_sym;
    target_num_t acc_target;
};

// entry in intermediate dfa table looks like this
// { {entry_header}{i_dfa_transition}...{i_dfa_transition} }

// since entry len is uin8 this will fit in uint32 so no overflows are possible

#define I_DFA_TABLE_ENTRY_SIZE(entry_len)  \
    (sizeof(struct i_dfa_table_entry_header) + sizeof(struct i_dfa_transition) * (entry_len))

#define I_DFA_TABLE_ENTRY_GET_TRANSITIONS(entry_ptr) \
    ((struct i_dfa_transition*)((char*)(entry_ptr) + sizeof(struct i_dfa_table_entry_header)))


static int i_dfa_table_add_state(long_list *i_table, struct calc_frame *f, target_num_t target){
    struct i_dfa_table_entry_header *entry;    
    struct i_dfa_transition *transitions;

    if (!(entry = (struct i_dfa_table_entry_header*) malloc(I_DFA_TABLE_ENTRY_SIZE(f -> transitions_len))))
        return 0; 

    if (!(long_list_append(i_table, (size_t) entry))){
        free(entry);
        return 0;
    }

    entry -> entry_len = f -> transitions_len;
    entry -> max_sym = f -> maximum_ind;
    entry -> min_sym = f -> minimum_ind;
    entry -> acc_target = target;
    
    transitions = I_DFA_TABLE_ENTRY_GET_TRANSITIONS(entry);

    for (alphabet_t i = 0; i < f -> transitions_len; i++){
        transitions[i].symbol = frame_ind_nonempty(f, i);
        transitions[i].next_state = frame_nonempty(f, i) -> state_num;
    }

    return 1;
}

inline static void i_dfa_table_clear(long_list *i_table){
    for (size_t i = 0; i < list_len(i_table); i++)
        free((void*)list_get(i_table, i));
}


static int i_dfa_table_init(
        long_list *i_table,
        obj_queue *Q,
        char **patterns, 
        target_num_t patterns_l,
        compiler_error *err){

    int result_code;
    struct calc_frame *frame;
    struct nfa_to_dfa_state *tmp_state;
    struct nfa_to_dfa_state *cur_state;
    nfa_table nfa_t;
    object_set d_states;
    state_num_t state;
    target_num_t acc_state;
    size_t old_d_state_len;
    long_set_iterator states_iterator;

    result_code = 0;
    err -> err_type = ERROR_NO_MEMORY;
    frame = 0;
    object_set_init(&d_states);

    // structures init

    if (!(frame = frame_alloc()))
        goto BUILD_CLEANUP_0;

    if (!(cur_state = nfa_to_dfa_state_alloc(0)))
        goto BUILD_CLEANUP_1;

    if (!object_set_insert(
                &d_states, 
                cur_state,
                (compare_func *)long_set_compare, 
                0)) {
        long_set_dealloc((long_set *)cur_state);
        goto BUILD_CLEANUP_1;
    }

    if (!(nfa_table_init(&nfa_t, patterns, patterns_l, err)))
        goto BUILD_CLEANUP_1;

    // start of building
    nfa_table_take_first_state(&nfa_t, (long_set*) cur_state);
    obj_queue_push(Q, cur_state);

    while (Q -> len) {
        cur_state = (struct nfa_to_dfa_state *) obj_queue_pop(Q);
        acc_state = 0;
        // fill frame

        long_set_iterator_init(&states_iterator, (long_set *)cur_state);

        while (long_set_iterator_next(&states_iterator, (unsigned long*) &state) ==
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

        // all newly created dfa_states sets in frame will have state_num =
        // current d_states len
        old_d_state_len = d_states.len;

        for (unsigned char i = 0; i < frame->transitions_len; i++) {
            tmp_state = frame_nonempty(frame, i);

            // check for d_states membership
            switch (object_set_insert(&d_states, tmp_state,
                                      (compare_func *)long_set_compare,
                                      (void *)&tmp_state)) {
                case OBJECT_SET_INSERT_SUCCES:
                    tmp_state->state_num = d_states.len - 1;

                    if (!obj_queue_push(Q, tmp_state))
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
        if (!i_dfa_table_add_state(
                    i_table, 
                    frame, 
                    acc_state))
            goto BUILD_CLEANUP_FULL;

        // clear frame
        frame_clear(frame);
    }

    result_code = 1; 

BUILD_CLEANUP_FULL:
    nfa_table_deinit(&nfa_t);
BUILD_CLEANUP_1:
    frame_dealloc(frame);
BUILD_CLEANUP_0:
    object_set_deinit(&d_states, (dealloc_func*) long_set_dealloc);
    return result_code;
}



static int i_dfa_table_to_dfa(long_list *i_table, dfa_table *final_table){
    int_list next; 
    int_list check;
    table_size_t *bases;
    target_num_t *targets;
    table_size_t *transitions_arr_size;
    state_num_t *states_number;
    struct i_dfa_table_entry_header *transition_header;
    struct i_dfa_transition *transitions;
    alphabet_t transition_min_sym;
    alphabet_t transition_max_sym;
    alphabet_t transitions_len;
    table_size_t base;
    table_size_t linear_dfa_chunk_size;
    void *linear_dfa_chunk;

    // overflow check
    if (list_len(i_table) > (TABLE_MAX_SIZE - sizeof(table_size_t) - sizeof(target_num_t)) / (sizeof(table_size_t) + sizeof(target_num_t)))
        goto ERROR_EXIT_0;

    linear_dfa_chunk_size = sizeof(table_size_t) + sizeof(target_num_t) + list_len(i_table) * (sizeof(table_size_t) + sizeof(target_num_t));

    if (!(transitions_arr_size = (table_size_t*) malloc(linear_dfa_chunk_size)))
        goto ERROR_EXIT_0;

    bases = (table_size_t*) ((char*)transitions_arr_size + sizeof(table_size_t) + sizeof(target_num_t));
    targets = (target_num_t*) (bases + list_len(i_table));
  
    if (!int_list_init(&next))
        goto ERROR_EXIT_1;

    if (!int_list_init(&check))
        goto ERROR_EXIT_2;

    for (state_num_t cur_state_num = 0; cur_state_num < list_len(i_table); cur_state_num++){

        // decode transition for current state
        transition_header = (struct i_dfa_table_entry_header*) list_get(i_table, cur_state_num); 
        transitions = I_DFA_TABLE_ENTRY_GET_TRANSITIONS(transition_header);
        transitions_len = transition_header -> entry_len;
        transition_max_sym = transition_header -> max_sym;
        transition_min_sym = transition_header -> min_sym;


        base = 0;
        targets[cur_state_num] = transition_header -> acc_target;

        if (!transitions_len) {
            bases[cur_state_num] = 0;
            continue;
        }

        // searh for space to insert states
        for (; base + transition_min_sym < list_len(&next); base++) {
            if (list_get(&next, base + transition_min_sym) != 0) {
                CONTINUE_SEARCH:
                continue;
            }

            for (alphabet_t j = 0; j < transitions_len; j++) {
                if (base + transitions[cur_state_num].symbol < list_len(&next) &&
                    list_get(&next, base + transitions[cur_state_num].symbol))
                        goto CONTINUE_SEARCH;
            }
            // at this point base is correct
            break;
        }

        if (base > TABLE_MAX_SIZE - transition_max_sym)
            goto ERROR_EXIT_3;

        // extend transitions arrays
        while (list_len(&next) <= base + transition_max_sym) {
            if (!int_list_append(&next, 0) || !int_list_append(&check, 0))
            return 0;
        }

        for (alphabet_t i = 0; i < transitions_len; i++) {
            int_list_write(
                    &next, 
                    transitions[i].next_state + 1,
                    base + transitions[i].symbol);

            int_list_write(
                    &check, 
                    cur_state_num + 1,
                    base + transitions[i].symbol);
        }

        bases[cur_state_num] = base;
    }

    for (table_size_t i = 0; i < list_len(&next); i++) {
        if (list_get(&next, i))
            int_list_write(&next, list_get(&next, i) - 1, i);
    }

    // make table a single buffer in memory
    // overflow check
    if (list_len(&next) > (TABLE_MAX_SIZE - linear_dfa_chunk_size) / (sizeof(state_num_t) * 2))
        goto ERROR_EXIT_3;

    linear_dfa_chunk_size = list_len(&next) * sizeof(state_num_t) * 2 + linear_dfa_chunk_size;

    if (!(linear_dfa_chunk = malloc(linear_dfa_chunk_size)))
        goto ERROR_EXIT_3;

    final_table -> table_chunk = linear_dfa_chunk;

    states_number = (target_num_t*)(transitions_arr_size + 1);
    final_table -> bases = (table_size_t*) linear_dfa_chunk;
    final_table -> acc_state = (target_num_t*) (final_table -> bases + list_len(i_table));
    final_table -> next = (state_num_t*) (final_table -> acc_state + list_len(i_table));
    final_table -> check = (state_num_t*) (final_table -> next + list_len(&next));

    *states_number = (state_num_t) list_len(i_table);
    memcpy(final_table -> bases, bases, list_len(i_table) * sizeof(table_size_t));
    memcpy(final_table -> acc_state, targets, list_len(i_table) * sizeof(target_num_t));
    final_table -> states = (state_num_t) list_len(i_table);
    free(transitions_arr_size);

    *transitions_arr_size = list_len(&next);
    memcpy(final_table -> next, &list_get(&next, 0), list_len(&next) * sizeof(target_num_t));
    int_list_deinit(&next);
    memcpy(final_table -> check, &(list_get(&check, 0)), list_len(&check) * sizeof(target_num_t));
    final_table -> transitions_len = list_len(&check);
    int_list_deinit(&check);

    return 1;

ERROR_EXIT_3:
    int_list_deinit(&check);
ERROR_EXIT_2:
    int_list_deinit(&next);
ERROR_EXIT_1:
    free(bases);
ERROR_EXIT_0:
    return 0;
}




int dfa_table_init(
        dfa_table *d, 
        char **patterns, 
        target_num_t patterns_l,
        compiler_error *err){
    long_list i_table;
    obj_queue Q;
    int result_code;

    err -> err_type = ERROR_NO_MEMORY;
    result_code = DFA_BUILD_ERROR;

    if (!(long_list_init(&i_table)))
        goto CLEAN_UP_EXIT_0;

    if (!obj_queue_init(&Q))
        goto CLEAN_UP_EXIT_1;

    if (!(i_dfa_table_init(&i_table, &Q, patterns, patterns_l, err)))
        goto CLEAN_UP_EXIT_2;

    if (!(i_dfa_table_to_dfa(&i_table, d)))
        goto CLEAN_UP_EXIT_3;

    result_code = DFA_BUILD_SUCCES;

CLEAN_UP_EXIT_3:
    i_dfa_table_clear(&i_table);
CLEAN_UP_EXIT_2:
    obj_queue_deinit(&Q);
CLEAN_UP_EXIT_1:
    long_list_deinit(&i_table);
CLEAN_UP_EXIT_0:
    return result_code;
}

