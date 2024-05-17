#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdint.h>

#define list_get(list_ptr, ind) ((list_ptr) -> arr[(ind)])
#define list_len(list_ptr) ((list_ptr) -> len)

typedef struct {
    unsigned long *arr;
    size_t len;
    size_t capacity;
}long_list;

int long_list_init(long_list*);
inline static void long_list_deinit(long_list*);

int long_list_append(long_list*, unsigned long);

// inline functions

inline static void long_list_deinit(long_list *list){
    free(list -> arr);
}



typedef struct {
    unsigned int *arr;
    size_t len;
    size_t capacity;
}int_list;

int int_list_init(int_list*);
inline static void int_list_deinit(int_list*);


inline static void int_list_write(int_list *, unsigned int, size_t);
int int_list_append(int_list*, unsigned int);

// inline functions

inline static void int_list_deinit(int_list *list){
    free(list -> arr);
}

inline static void int_list_write(int_list *list, unsigned int x, size_t ind){
    list -> arr[ind] = x;
}

#endif
