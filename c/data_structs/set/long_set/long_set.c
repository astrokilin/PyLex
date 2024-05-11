#include "long_set.h"

/***************************************************************
||                                                            ||
||                      RB tree routines                      ||
||                                                            ||
***************************************************************/

// TODO: redo all recursive functions with rb_stack

inline static void long_rb_stack_push(struct long_rb_stack *s, struct long_rb_node *n){
    s -> buf[s -> len++] = n;
}

inline static struct long_rb_node *long_rb_stack_pop(struct long_rb_stack *s){
    return s -> buf[--(s -> len)];
}

inline static struct long_rb_node *long_rb_stack_get(struct long_rb_stack *s, size_t ind){
    return s -> buf[ind];
}

inline static void long_rb_stack_set(struct long_rb_stack *s, size_t ind, struct long_rb_node *r){
    s -> buf[ind] = r; 
}


static struct long_rb_node* new_long_rb_node(
                unsigned long x,
                char is_red,
                struct long_rb_node *left,
                struct long_rb_node *right){
    struct long_rb_node *res = 0;

    res = (struct long_rb_node*) malloc(sizeof(struct long_rb_node));
    if (res == NULL)
        return NULL;

    res -> is_red = is_red;
    res -> left = left;
    res -> right = right;
    res -> num = x;
    return res;
}

static void long_rb_free(struct long_rb_node *top){
    struct long_rb_stack s;
    struct long_rb_node *tmp;

    if (!top)
        return;

    s.len = 0;
    long_rb_stack_push(&s, top);

    while (s.len){
        tmp = long_rb_stack_pop(&s);

        if (tmp -> right)
            long_rb_stack_push(&s, tmp -> right);

        if (tmp -> left)
            long_rb_stack_push(&s, tmp -> left);

        free(tmp);

    }
}

static int long_rb_copy(struct long_rb_node *orig, struct long_rb_node **cpy){
    struct long_rb_node *left;
    struct long_rb_node *right;
    struct long_rb_node *res;

    if (!orig){
        *cpy = 0;
        return 1;
    }

    if (long_rb_copy(orig -> left, &left) == 0)
        return 0;

    if (long_rb_copy(orig -> right, &right) == 0){
        long_rb_free(left);
        return 0;
    }

    if (!(res = new_long_rb_node(orig -> num, orig -> is_red, left, right))){
        long_rb_free(left);
        long_rb_free(right);
        return 0; 
    }

    *cpy = res;
    return 1;
}


// insertion of element

#define RED(rb_ptr) (!!(rb_ptr) && (rb_ptr)->is_red) 

inline static void rot_left(struct long_rb_node **ptr){
    struct long_rb_node *x = (*ptr) -> right;
    (*ptr) -> right = x -> left;
    x -> left = *ptr;
    *ptr = x;
}

inline static void rot_right(struct long_rb_node **ptr){
    struct long_rb_node *x = (*ptr) -> left;
    (*ptr) -> left = x -> right;
    x -> right = *ptr;
    *ptr = x;
}

static int long_rb_insert(struct long_rb_node **ptr, unsigned long x, char sw){
    int res;

    if (*ptr == 0){
        if (!(*ptr = new_long_rb_node(x , 1, 0, 0)))
            return LONG_SET_INSERT_ERROR;
        return LONG_SET_INSERT_SUCCES;
    }

    if (RED((*ptr) -> left) && RED((*ptr) -> right)){
        (*ptr) -> is_red = 1;
        (*ptr) -> left -> is_red = 0;
        (*ptr) -> right -> is_red = 0;
    }

    if (x < (*ptr) -> num){
        if ((res = long_rb_insert(&((*ptr) -> left), x, 0)) != LONG_SET_INSERT_SUCCES)
            return res;

        if ((*ptr) -> is_red && RED((*ptr) -> left) && sw)
            rot_right(ptr);

        if (RED((*ptr) -> left) && RED((*ptr) -> left -> left)){
            rot_right(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> right -> is_red = 1;
        }
    }
    else if (x > (*ptr) -> num){
        if ((res = long_rb_insert(&((*ptr) -> right), x, 1)) != LONG_SET_INSERT_SUCCES)
            return res;

        if ((*ptr) -> is_red && RED((*ptr) -> right) && !sw)
            rot_left(ptr);

        if (RED((*ptr) -> right) && RED((*ptr) -> right -> right)){
            rot_left(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> left -> is_red = 1;
        }
    }
    else{
        return LONG_SET_INSERT_DUPLICATE;
    }
    return LONG_SET_INSERT_SUCCES;
}

// iterator functions

inline static struct long_rb_node *set_left_dive(long_set_iterator *iter, struct long_rb_node *top){

    while (top -> left){
        long_rb_stack_push(&iter -> stack, top);
        top = top -> left;
    }

    return top;
}

/***************************************************************
||                                                            ||
||                      Public interface                      ||
||                                                            ||
***************************************************************/

long_set *long_set_alloc(){
    long_set *res = 0;

    if (!(res = (long_set*) malloc(sizeof(long_set))))
        return 0;

    long_set_init(res);
    return res;
}

int long_set_copy_init(long_set *dst, long_set *src){
 
    if (long_rb_copy(src -> top, &dst -> top) == 0)
        return 0;

    dst -> len = src -> len;
    return 1; 
}


void long_set_deinit(long_set *s){
    long_rb_free(s -> top);
}


void long_set_dealloc(long_set *ptr){
    if (!ptr)
        return;
    long_rb_free(ptr -> top);
    free(ptr);
}


int long_set_insert(long_set *ptr, unsigned long item){
    int res = long_rb_insert(&(ptr -> top), item, 0);
    if (res == LONG_SET_INSERT_SUCCES){
        (ptr -> len)++;
        ptr -> top -> is_red = 0;
    }
    return res;
}


int long_set_insert_set(long_set *set_1, long_set *set_2, size_t *inserted){
    size_t inserted_before = set_1 -> len;
    long_set_iterator iter;
    long_set_iterator_init(&iter, set_2);
    unsigned long item;
    int res = 1;

    while(long_set_iterator_next(&iter, &item) == LONG_ITERATOR_NEXT_SUCCES){
        if (long_set_insert(set_1, item) == LONG_SET_INSERT_ERROR){
            res = 0;
            break;
        }
    }

    if (inserted)
        *inserted = set_1 -> len - inserted_before;

    return res;
}


int long_set_compare(long_set *ptr_1, long_set *ptr_2){
    long_set_iterator iter_1;
    long_set_iterator iter_2;
    unsigned long item_1;
    unsigned long item_2;

    if (ptr_1 -> len < ptr_2 -> len)
        return -1;
    else if (ptr_1 -> len > ptr_2 -> len)
        return 1;

    long_set_iterator_init(&iter_1, ptr_1);
    long_set_iterator_init(&iter_2, ptr_2);

    while (long_set_iterator_next(&iter_1, &item_1) && long_set_iterator_next(&iter_2, &item_2)){
        if (item_1 < item_2)
            return -1;
        else if(item_1 > item_2)
            return 1;
    }

    return 0; 
}


void long_set_iterator_init(long_set_iterator* iter, long_set* s){
    iter -> stack.len = 0;
    iter -> next_ret_node = 0;
    if (s -> top){
        iter -> next_ret_node = set_left_dive(iter, s -> top);        
    }
}

int long_set_iterator_next(long_set_iterator *iter, unsigned long *res){
    struct long_rb_node *tmp;

    if (!(tmp = iter -> next_ret_node))
        return LONG_ITERATOR_NEXT_NOMORE;

    if (tmp -> right)
        iter -> next_ret_node = set_left_dive(iter, tmp -> right);
    else if (iter -> stack.len)
        iter -> next_ret_node = long_rb_stack_pop(&iter -> stack);
    else
        iter -> next_ret_node = 0;

    *res = tmp -> num;
    return LONG_ITERATOR_NEXT_SUCCES;
}
