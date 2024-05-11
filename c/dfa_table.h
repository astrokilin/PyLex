#ifndef DFA_TABLE_H
#define DFA_TABLE_H

#include <stdlib.h>
#include <limits.h>

typedef struct {
    char err_str[64];
} dfa_table_syntax_error;

typedef struct {
    unsigned int *acc_state;
    unsigned long *bases;
    unsigned int *next;
    unsigned int *check;
    unsigned int states;
    size_t transitions_len;
} dfa_table;


#define DFA_BUILD_SUCCES            0
#define DFA_BUILD_ERR_NOMEM         -1
#define DFA_BUILD_ERR_SYNTAX        -2
#define DFA_BUILD_ERR_OVERFLOW      -3

#define DFA_TRANSITION_IMPOSIBLE     0
#define DFA_TRANSITION_SUCCES        1

#define DFA_STATE_NOTACCEPTING       0
#define DFA_STATE_ACCEPTING          1

#define DFA_TABLE_SIZE(dfa_table_ptr) ((dfa_table_ptr)->transitions_len * sizeof(unsigned int*) * 2+ (dfa_table_ptr)->states *  sizeof(unsigned long*) * 2)

int dfa_table_init(dfa_table*, char**, unsigned int, dfa_table_syntax_error*);

inline static void dfa_table_deinit(dfa_table*);

inline static unsigned int dfa_table_is_state_accepting(dfa_table*, unsigned int, unsigned int*);

inline static unsigned int dfa_table_next_state(dfa_table*, unsigned int, unsigned char, unsigned int*);


inline static void dfa_table_deinit(dfa_table *t){
    free(t -> acc_state); 
    free(t -> bases); 
    free(t -> next); 
    free(t -> check); 
}

inline static unsigned int dfa_table_is_state_accepting(dfa_table *t, unsigned int cur_state, unsigned int *target_num){
    if (t -> acc_state[cur_state]){
        *target_num = t -> acc_state[cur_state] - 1;
        return DFA_STATE_ACCEPTING;
    }

    return DFA_STATE_NOTACCEPTING;
}

inline static unsigned int dfa_table_next_state(dfa_table *t, unsigned int cur_state, unsigned char sym, unsigned int *next_state){
    if (t -> bases[cur_state] + sym < t -> transitions_len && t->check[t -> bases[cur_state] + sym] == cur_state + 1){
        *next_state = t->next[t -> bases[cur_state] + sym];
        return DFA_TRANSITION_SUCCES;
    }

    return DFA_TRANSITION_IMPOSIBLE;
}

#endif
