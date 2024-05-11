#include "queue.h"

int obj_queue_init(obj_queue *Q){

    if (!(Q -> last_node = (struct queue_node*) malloc(sizeof(struct queue_node))))
        return 0;

    Q -> last_node -> next = 0;
    Q -> first_node = Q -> last_node;
    Q -> first_empty_node = Q -> last_node;
    Q -> len = 0;
    return 1;
}

void obj_queue_deinit(obj_queue *Q){
    struct queue_node *tmp;
    for (struct queue_node *ptr = Q -> first_node; ptr;){
        tmp = ptr -> next; 
        free(ptr);
        ptr = tmp;
    }
}

int obj_queue_push(obj_queue *L, void *obj){
    struct queue_node *new_last_node;

    if (L -> len == SIZE_MAX)
        return QUEUE_PUSH_FULL;

    if (L -> first_empty_node){
        L -> first_empty_node -> obj = obj;
        new_last_node = L -> first_empty_node;
        L -> first_empty_node = L -> first_empty_node -> next;
    }else{

        if ((new_last_node = (struct queue_node*) malloc(sizeof(struct queue_node))) == NULL)
            return QUEUE_PUSH_NOMEM;

        new_last_node -> obj = obj;
        new_last_node -> next = 0;
        L -> last_node -> next = new_last_node;
    }

    L -> last_node = new_last_node;
    L -> len++;
    return QUEUE_PUSH_SUCCES;
}

void *obj_queue_pop(obj_queue *L){
    void *res;
    struct queue_node *tmp;

    if (!L -> len)
        return 0;

    res = L -> first_node -> obj;

    if (L -> last_node == L -> first_node){
        L -> first_empty_node = L -> first_node;
    }else{
        tmp = L -> first_node -> next;
        L -> last_node -> next = L -> first_node;
        L -> first_node -> next = L -> first_empty_node;
        L -> first_empty_node = L -> first_node;
        L -> first_node = tmp;
    }
    L -> len--;
    return res; 
}
