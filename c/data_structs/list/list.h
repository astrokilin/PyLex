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

// converts list to regular fixed size array
inline static unsigned long *long_list_to_array(long_list*);

// inline functions

inline static void long_list_deinit(long_list *list){
    free(list -> arr);
}

inline static unsigned long *long_list_to_array(long_list *list){
    unsigned long *b; 

    if (!(b = (unsigned long*) reallocarray(list -> arr, list->len, sizeof(unsigned long))))
        return 0;
    
    return b;
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

inline static unsigned int *int_list_to_array(int_list*);

// inline functions

inline static void int_list_deinit(int_list *list){
    free(list -> arr);
}

inline static void int_list_write(int_list *list, unsigned int x, size_t ind){
    list -> arr[ind] = x;
}

inline static unsigned int *int_list_to_array(int_list *list){
    unsigned int *b; 

    if (!(b = (unsigned int*) reallocarray(list -> arr, list->len, sizeof(unsigned int))))
        return 0;
    
    return b;
}

#endif
