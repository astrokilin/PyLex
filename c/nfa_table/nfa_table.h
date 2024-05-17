#ifndef NFA_TABLE_H
#define NFA_TABLE_H

#include <string.h>

#include "dfa_table_parameters.h"
#include "long_set.h"

typedef struct{
    state_num_t states_count;
    target_num_t *state_targets;
    long_set *followpos_sets;     
    alphabet_t *symbols;
    long_set  first_state;
}nfa_table;

#define NFA_INIT_SUCCES     1
#define NFA_INIT_ERROR      0

int nfa_table_init(nfa_table*, char**, unsigned int, compiler_error*);

void nfa_table_deinit(nfa_table*);

inline static void nfa_table_take_first_state(nfa_table *t, long_set *dst){
    *dst = t -> first_state;
    memset(&t -> first_state, 0, sizeof(long_set));
}

#endif
