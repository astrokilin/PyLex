#ifndef OBJECT_SET_H
#define OBJECT_SET_H

#include <stdlib.h>

// max size of dfs stack during rb tree traversal
// without putting non existing leafs (zero pointers) on stack
#define OBJECT_SET_DFS_STACK_MAX_LEN (sizeof(size_t) * 16)

struct object_rb_stack{
    size_t len;
    struct object_rb_node *buf[OBJECT_SET_DFS_STACK_MAX_LEN];
};

struct object_rb_node{
    char is_red;
    void *object;
    struct object_rb_node *left;
    struct object_rb_node *right;
};

typedef struct {
    struct object_rb_node *top;
    size_t len;
}object_set;

// returns -1 if obj_1 < obj_2, 0 on equal, 1 otherwise
typedef int compare_func(void *obj_1, void *obj_2);
// used for copying internal objects
typedef void* copy_func(void *origin);
// used for deallocating internal objects
typedef void dealloc_func(void *obj);

#define object_set_get_len(obj_set_ptr) ((obj_set_ptr)->len)

inline static void object_set_init(object_set*);
void object_set_deinit(object_set*, dealloc_func*);  

#define OBJECT_SET_INSERT_ERROR        0
#define OBJECT_SET_INSERT_SUCCES       1
#define OBJECT_SET_INSERT_DUPLICATE    2

int object_set_insert(object_set*, void*, compare_func*, void**);

//inline functions

inline static void object_set_init(object_set *set){
    set -> top = 0;
    set -> len = 0;
}


// iterator

typedef struct{
    struct object_rb_stack stack;
    struct object_rb_node *next_ret_node;
}object_set_iterator;

#define OBJECT_ITERATOR_NEXT_NOMORE    0
#define OBJECT_ITERATOR_NEXT_SUCCES    1

void object_set_iterator_init(object_set_iterator*, object_set*);

int object_set_iterator_next(object_set_iterator*, void**);


#endif
