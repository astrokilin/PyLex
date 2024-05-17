#ifndef DFA_TABLE_H
#define DFA_TABLE_H

#include <stdlib.h>
#include <limits.h>

#include "dfa_table_parameters.h"

typedef struct {
    void *table_chunk;
    table_size_t *bases;
    target_num_t *acc_state;
    state_num_t *next;
    state_num_t *check;
    state_num_t states;
    table_size_t transitions_len;
} dfa_table;


#define DFA_BUILD_ERROR     0
#define DFA_BUILD_SUCCES    1

#define DFA_TABLE_SIZE(dfa_table_ptr)                            \
    (((dfa_table_ptr)->transitions_len * sizeof(state_num_t*) +  \
      (dfa_table_ptr)->states *  sizeof(table_size_t*)) * 2)

int dfa_table_init(dfa_table*, char**, target_num_t, compiler_error*);

inline static void dfa_table_deinit(dfa_table*);

inline static int dfa_table_is_state_accepting(dfa_table*, state_num_t, target_num_t*);

inline static int dfa_table_next_state(dfa_table*, state_num_t, alphabet_t, state_num_t*);


inline static void dfa_table_deinit(dfa_table *t){
    free(t -> table_chunk); 
}

#define DFA_TRANSITION_IMPOSIBLE     0
#define DFA_TRANSITION_SUCCES        1

#define DFA_STATE_NOTACCEPTING       0
#define DFA_STATE_ACCEPTING          1

inline static int dfa_table_is_state_accepting(dfa_table *t, state_num_t cur_state, target_num_t *target_num){
    if (t -> acc_state[cur_state]){
        *target_num = t -> acc_state[cur_state] - 1;
        return DFA_STATE_ACCEPTING;
    }

    return DFA_STATE_NOTACCEPTING;
}

inline static int dfa_table_next_state(dfa_table *t, state_num_t cur_state, alphabet_t sym, state_num_t *next_state){
    if (t -> bases[cur_state] + sym < t -> transitions_len && t->check[t -> bases[cur_state] + sym] == cur_state + 1){
        *next_state = t->next[t -> bases[cur_state] + sym];
        return DFA_TRANSITION_SUCCES;
    }

    return DFA_TRANSITION_IMPOSIBLE;
}

#endif
