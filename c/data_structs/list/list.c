#include "list.h"

#define START_SIZE 32

inline static int expand_long(long_list *list){
    unsigned long *new_arr;
    size_t to_alloc;

    if (list -> len == list -> capacity){

        if (list -> capacity > SIZE_MAX / 2)
            to_alloc = SIZE_MAX;
        else
            to_alloc = list -> capacity * 2;

        if ((new_arr = (unsigned long*) reallocarray(list -> arr, to_alloc, sizeof(unsigned long))) == NULL)
            return 0;

        list -> arr = new_arr;
        list -> capacity = to_alloc;
    }

    return 1;
}

inline static int expand_int(int_list *list){
    unsigned int *new_arr;
    size_t to_alloc;

    if (list -> len == list -> capacity){

        if (list -> capacity > SIZE_MAX / 2)
            to_alloc = SIZE_MAX;
        else
            to_alloc = list -> capacity * 2;

        if ((new_arr = (unsigned int*) reallocarray(list -> arr, to_alloc, sizeof(unsigned int))) == NULL)
            return 0;

        list -> arr = new_arr;
        list -> capacity = to_alloc;
    }

    return 1;
}

inline static void *list_alloc(size_t list_size, size_t list_type_size){
    void *res;

    if (!(res = malloc(list_size)))
        return 0;

    if (!(*((void **)res) = calloc(START_SIZE, list_type_size))){
        free(res);
        return 0;
    }

    return res;
}






int long_list_init(long_list *list){

    if (!(list -> arr = (unsigned long*) calloc(START_SIZE, sizeof(unsigned long))))
        return 0;

    list -> len = 0;
    list -> capacity = START_SIZE;
    return 1;
}

int long_list_append(long_list *list, unsigned long item){

    if (!expand_long(list))
        return 0;

    list -> arr[list -> len] = item;
    list -> len++;

    return 1;
}

int int_list_init(int_list *list){

    if (!(list -> arr = (unsigned int*) calloc(START_SIZE, sizeof(unsigned int))))
        return 0;

    list -> len = 0;
    list -> capacity = START_SIZE;
    return 1;
}

int int_list_append(int_list *list, unsigned int item){

    if (!expand_int(list))
        return 0;

    list -> arr[list -> len] = item;
    list -> len++;

    return 1;
}
