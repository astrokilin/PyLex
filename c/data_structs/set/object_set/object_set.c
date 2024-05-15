#include "object_set.h"


/***************************************************************
||                                                            ||
||                      RB tree routines                      ||
||                                                            ||
***************************************************************/

// TODO: redo all recursive functions with rb_stack

// since the maximum depth ob rb - tree with size_t_max is 2*log2(size_t_max + 1)
// we can always allocate stack for dfs

struct check_context{
    compare_func *C;
    void *object;
};

struct copy_context{
    copy_func *C;
    dealloc_func *D;
};

struct insert_context{
    int result_code;
    compare_func *comparator;
    void *object;
    void **orig_object;
};

inline static void object_rb_stack_push(struct object_rb_stack *s, struct object_rb_node *n){
    s -> buf[s -> len++] = n;
}

inline static struct object_rb_node *object_rb_stack_pop(struct object_rb_stack *s){
    return s -> buf[--(s -> len)];
}

inline static struct object_rb_node *object_rb_stack_get(struct object_rb_stack *s, size_t ind){
    return s -> buf[ind];
}

inline static void object_rb_stack_set(struct object_rb_stack *s, size_t ind, struct object_rb_node *r){
    s -> buf[ind] = r; 
}


static struct object_rb_node* new_object_rb_node(
                void *x,
                char is_red,
                struct object_rb_node *left,
                struct object_rb_node *right){
    struct object_rb_node *res = 0;

    res = (struct object_rb_node*) malloc(sizeof(struct object_rb_node));
    if (res == NULL)
        return NULL;

    res -> is_red = is_red;
    res -> left = left;
    res -> right = right;
    res -> object = x;
    return res;
}

static void object_rb_free(struct object_rb_node *top, dealloc_func *F){
    struct object_rb_stack s;
    struct object_rb_node *tmp;

    if (!top)
        return;

    s.len = 0;
    object_rb_stack_push(&s, top);

    while (s.len){
        tmp = object_rb_stack_pop(&s);

        if (tmp -> right)
            object_rb_stack_push(&s, tmp -> right);

        if (tmp -> left)
            object_rb_stack_push(&s, tmp -> left);

        if (F)
            F(tmp -> object);

        free(tmp);

    }
}

// insertion of element

#define RED(rb_ptr) (!!(rb_ptr) && (rb_ptr)->is_red) 

inline static void rot_left(struct object_rb_node **ptr){
    struct object_rb_node *x = (*ptr) -> right;
    (*ptr) -> right = x -> left;
    x -> left = *ptr;
    *ptr = x;
}

inline static void rot_right(struct object_rb_node **ptr){
    struct object_rb_node *x = (*ptr) -> left;
    (*ptr) -> left = x -> right;
    x -> right = *ptr;
    *ptr = x;
}

static void object_rb_insert(struct object_rb_node **ptr, struct insert_context *context, char sw){
    signed long cmp_res;

    if (*ptr == 0){
        context -> result_code = OBJECT_SET_INSERT_SUCCES;

        if (!(*ptr = new_object_rb_node(context -> object, 1, 0, 0)))
            context -> result_code = OBJECT_SET_INSERT_ERROR;

        return;
    }

    if (RED((*ptr) -> left) && RED((*ptr) -> right)){
        (*ptr) -> is_red = 1;
        (*ptr) -> left -> is_red = 0;
        (*ptr) -> right -> is_red = 0;
    }

    cmp_res = context -> comparator(context -> object, (*ptr) -> object);

    if (cmp_res < 0){
        object_rb_insert(&((*ptr) -> left), context, 0);

        if ((*ptr) -> is_red && RED((*ptr) -> left) && sw)
            rot_right(ptr);

        if (RED((*ptr) -> left) && RED((*ptr) -> left -> left)){
            rot_right(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> right -> is_red = 1;
        }
    }
    else if (cmp_res > 0){
        object_rb_insert(&((*ptr) -> right), context, 1);

        if ((*ptr) -> is_red && RED((*ptr) -> right) && !sw)
            rot_left(ptr);

        if (RED((*ptr) -> right) && RED((*ptr) -> right -> right)){
            rot_left(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> left -> is_red = 1;
        }
    }
    else{
        if (context -> orig_object)
            *(context -> orig_object) = (*ptr) -> object;

        context -> result_code = OBJECT_SET_INSERT_DUPLICATE;
    }
}

// iterator functions

inline static struct object_rb_node *set_left_dive(object_set_iterator *iter, struct object_rb_node *top){

    while (top -> left){
        object_rb_stack_push(&iter -> stack, top);
        top = top -> left;
    }

    return top;
}

/***************************************************************
||                                                            ||
||                      Public interface                      ||
||                                                            ||
***************************************************************/

void object_set_deinit(object_set *s, dealloc_func *F){
    object_rb_free(s -> top, F);
}

int object_set_insert(object_set *obj_set, void *obj, compare_func *comparator, void **orig_obj){
    struct insert_context context = {0, comparator, obj, orig_obj};
    
    object_rb_insert(&(obj_set -> top), &context, 0);

    if (context.result_code == OBJECT_SET_INSERT_SUCCES){
        (obj_set -> len)++;
        obj_set -> top -> is_red = 0;
    }

    return context.result_code;
}

void object_set_iterator_init(object_set_iterator* iter, object_set* s){
    iter -> stack.len = 0;
    iter -> next_ret_node = 0;
    if (s -> top){
        iter -> next_ret_node = set_left_dive(iter, s -> top);        
    }
}

int object_set_iterator_next(object_set_iterator *iter, void **res){
    struct object_rb_node *tmp;

    if (!(tmp = iter -> next_ret_node))
        return OBJECT_ITERATOR_NEXT_NOMORE;

    if (tmp -> right)
        iter -> next_ret_node = set_left_dive(iter, tmp -> right);
    else if (iter -> stack.len)
        iter -> next_ret_node = object_rb_stack_pop(&iter -> stack);
    else
        iter -> next_ret_node = 0;

    *res = tmp -> object;
    return OBJECT_ITERATOR_NEXT_SUCCES;
}

