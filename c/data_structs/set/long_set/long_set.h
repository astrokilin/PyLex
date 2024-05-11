#ifndef LONG_SET_H
#define LONG_SET_H

#include <stdlib.h>

// max size of stack during rb tree deep first traversal
// without putting non existing leafs (zero pointers) on stack
#define LONG_DFS_STACK_MAX_LEN (sizeof(size_t) * 16)

struct long_rb_stack{
    size_t len;
    struct long_rb_node *buf[LONG_DFS_STACK_MAX_LEN];
};

struct long_rb_node{
    char is_red;
    unsigned long num;
    struct long_rb_node *left;
    struct long_rb_node *right;
};

typedef struct {
    struct long_rb_node *top;
    size_t len;
}long_set;


long_set *long_set_alloc(void);
inline static void long_set_init(long_set*);

int long_set_copy_init(long_set*, long_set*);

void long_set_deinit(long_set*);
void long_set_dealloc(long_set*);

#define long_set_get_len(set_ptr) ((set_ptr) -> len)

#define LONG_SET_INSERT_ERROR        0
#define LONG_SET_INSERT_SUCCES       1
#define LONG_SET_INSERT_DUPLICATE    2

// returns 0 on error, 1 on success, 2 if item is already in num_set
int long_set_insert(long_set *, unsigned long);

// return 0 if there was an error during inseretion items from set_2 to set_1
// if inserted is not 0, write number of inserted elements
int long_set_insert_set(long_set *num_set_1, long_set *num_set_2, size_t *inserted);

// returns -1, 0, 1 if set_1 < set_2; set_1 = set_2; set_1 > set_2
int long_set_compare(long_set *set_1, long_set *set_2);

//inline functions

inline static void long_set_init(long_set *s){
    s -> top = 0;
    s -> len = 0;
}

// set iterator

typedef struct{
    struct long_rb_stack stack;
    struct long_rb_node *next_ret_node;
}long_set_iterator;

#define LONG_ITERATOR_NEXT_NOMORE    0
#define LONG_ITERATOR_NEXT_SUCCES    1

void long_set_iterator_init(long_set_iterator*, long_set*);
int long_set_iterator_next(long_set_iterator*, unsigned long*);

#endif
