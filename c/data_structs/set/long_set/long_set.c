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

inline static void long_rb_free(struct long_rb_node *top){
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

static void long_rb_insert(struct long_rb_node **ptr, char sw, unsigned long x, int *res){

    if (*ptr == 0){
        *res = LONG_SET_INSERT_SUCCES;

        if (!(*ptr = new_long_rb_node(x , 1, 0, 0)))
            *res = LONG_SET_INSERT_ERROR;

        return;
    }

    if (RED((*ptr) -> left) && RED((*ptr) -> right)){
        (*ptr) -> is_red = 1;
        (*ptr) -> left -> is_red = 0;
        (*ptr) -> right -> is_red = 0;
    }

    if (x < (*ptr) -> num){
        long_rb_insert(&((*ptr) -> left), 0, x, res);

        if ((*ptr) -> is_red && RED((*ptr) -> left) && sw)
            rot_right(ptr);

        if (RED((*ptr) -> left) && RED((*ptr) -> left -> left)){
            rot_right(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> right -> is_red = 1;
        }
    }
    else if (x > (*ptr) -> num){
        long_rb_insert(&((*ptr) -> right), 1, x, res);

        if ((*ptr) -> is_red && RED((*ptr) -> right) && !sw)
            rot_left(ptr);

        if (RED((*ptr) -> right) && RED((*ptr) -> right -> right)){
            rot_left(ptr);
            (*ptr) -> is_red = 0;
            (*ptr) -> left -> is_red = 1;
        }
    }
    else{
        *res = LONG_SET_INSERT_DUPLICATE;
    }
}

#define is_2_node(rb_node_ptr) (!RED((rb_node_ptr)) && !RED((rb_node_ptr)->left) && !RED((rb_node_ptr)->right))

// traverse tree down, making rotations and recoloring
// to find the r34 (not root, 3 or 4 equivalent in 2-3-4 tree) node
// with given value, if no such node, return closest to it

static struct long_rb_node **rb_find_r34(struct long_rb_node **ptr, unsigned long x){
    struct long_rb_node *next_node;
    struct long_rb_node *sibling_node;


    while (1){
        // inductive rule
        // on every iteration we enter node that is not 2 node

        if ((*ptr) -> left && is_2_node((*ptr) -> left) && (*ptr) -> right && is_2_node((*ptr) -> right)){
            (*ptr) -> is_red = 0;
            (*ptr) -> left -> is_red = 1;
            (*ptr) -> right -> is_red = 1;
        }

        if (x < (*ptr) -> num){
            next_node = (*ptr) -> left;
            sibling_node = (*ptr) -> right;

            //no node with x in tree
            if (next_node == 0)
                return ptr;

            // next node is not 2 node, so traverse to it
            if (next_node -> is_red || RED(next_node -> left) || RED(next_node -> right))
                goto TRAVERSE_LEFT;

            // now we are looking at 2 node

            // sibling should be black, rotate
            if (sibling_node -> is_red){
                rot_left(ptr);
                (*ptr) -> is_red = 0;
                (*ptr) -> left -> is_red = 1;
                goto TRAVERSE_LEFT;
            }
            // after this step parent is guaranteed to be red
 
            // either sibling has one red son or no red sons
            if (!RED(sibling_node -> right)){
                // make left red son right
                rot_right(&(*ptr) -> right);
                sibling_node = (*ptr) -> right;
                sibling_node -> is_red = 0;
                sibling_node -> right -> is_red = 1;
            }

            // sibling has right red son case
            rot_left(ptr);
            (*ptr) -> is_red = (*ptr) -> left -> is_red;
            (*ptr) -> left -> is_red = 0;
            (*ptr) -> right -> is_red = 0;
            next_node -> is_red = 1;

TRAVERSE_LEFT:
            ptr = &(*ptr) -> left;
            continue;

        }else if (x > (*ptr) -> num){
            // symmetrical case
            next_node = (*ptr) -> right;
            sibling_node = (*ptr) -> left;

            if (next_node == 0)
                return ptr;

            if (next_node -> is_red || RED(next_node -> left) || RED(next_node -> right))
                goto TRAVERSE_RIGHT;

            if (sibling_node -> is_red){
                rot_right(ptr);
                (*ptr) -> is_red = 0;
                (*ptr) -> right -> is_red = 1;
                goto TRAVERSE_RIGHT;
            }
 
            if (!RED(sibling_node -> left)){
                rot_left(&(*ptr) -> left);
                sibling_node = (*ptr) -> left;
                sibling_node -> is_red = 0;
                sibling_node -> left -> is_red = 1;
            }

            rot_right(ptr);
            (*ptr) -> is_red = (*ptr) -> right -> is_red;
            (*ptr) -> right -> is_red = 0;
            (*ptr) -> left -> is_red = 0;
            next_node -> is_red = 1;

TRAVERSE_RIGHT:
            ptr = &(*ptr) -> right;
            continue;
        }

        break;
    } 
    return ptr;
}

// deletes node in 2-3-4 leaf node or return next non 2 node
// to start search for transplant node
// return 0 if deletion was succes or return next non 2 node ptr

static struct long_rb_node **long_rb_del_in_leaf(struct long_rb_node **ptr){
    struct long_rb_node *tmp;

    if ((*ptr) -> is_red){
        if ((*ptr) -> left == 0){
            free(*ptr);
            *ptr = 0;
            return 0;
        }

        if (!is_2_node((*ptr) -> left)){
            return &(*ptr) -> left;
        }else if (is_2_node((*ptr) -> right)){
        // if we ended up in two 2 subnodes nodes 
            (*ptr) -> is_red = 0;
            (*ptr) -> left -> is_red = 1;
            (*ptr) -> right -> is_red = 1;
        }

        return &(*ptr) -> right;
    }
    // we are in black node

    // black root red left sibling
    if ((*ptr) -> right == 0){
        tmp = *ptr;
        tmp -> left -> is_red = 0;
        *ptr = tmp -> left;
        free(tmp);
        return 0; 
    }else if ((*ptr) -> right -> is_red == 1){
        return &(*ptr) -> right;
    }

    // black root red right sibling
    if ((*ptr) -> left == 0){
        tmp = *ptr;
        tmp -> right -> is_red = 0;
        *ptr = tmp -> right;
        free(tmp);
        return 0; 
    }else if ((*ptr) -> left -> is_red == 1){
        return &(*ptr) -> left;
    }

    if (is_2_node((*ptr) -> left))
        return &(*ptr) -> right;

    return &(*ptr) -> left;
}

// iterator functions

inline static struct long_rb_node *set_left_dive(
        long_set_iterator *iter, 
        struct long_rb_node *top){

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
    s -> top = 0;
}


void long_set_dealloc(long_set *ptr){
    if (!ptr)
        return;
    long_rb_free(ptr -> top);
    free(ptr);
}


int long_set_insert(long_set *ptr, unsigned long item){
    int res;

    long_rb_insert(&ptr -> top, 0, item, &res);

    if (res == LONG_SET_INSERT_SUCCES){
        (ptr -> len)++;
        ptr -> top -> is_red = 0;
    }
    return res;
}

int long_set_delete(long_set *ptr, unsigned long item){
    struct long_rb_node **node_to_del_ptr;
    struct long_rb_node *node_to_del;
    struct long_rb_node **tmp;

    if (ptr -> top == 0)
        return LONG_SET_DELETE_NOITEM;

    node_to_del_ptr = rb_find_r34(&ptr -> top, item);
    node_to_del = *node_to_del_ptr;

    if (node_to_del -> num != item)
        return LONG_SET_DELETE_NOITEM;

    // node is found

    // works only if one node in set
    if (ptr -> len == 1){
        free(node_to_del); 
        *node_to_del_ptr = 0;
        goto DELETE_SUCCES;
    }

    // if tmp then search should continue
    if (!(tmp = long_rb_del_in_leaf(node_to_del_ptr)))
        goto DELETE_SUCCES;

    tmp = rb_find_r34(tmp, item);
    node_to_del -> num = (*tmp) -> num;
    long_rb_del_in_leaf(tmp);

DELETE_SUCCES:
    ptr -> len--;
    return LONG_SET_DELETE_SUCCES;
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
