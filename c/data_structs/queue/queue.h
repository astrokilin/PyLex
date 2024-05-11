#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdint.h>

#define QUEUE_PUSH_SUCCES   1
#define QUEUE_PUSH_NOMEM    0
#define QUEUE_PUSH_FULL     -1

struct queue_node{
    struct queue_node *next;
    void *obj;
};

typedef struct {
    struct queue_node *last_node;
    struct queue_node *first_empty_node;
    struct queue_node *first_node;
    size_t len;
}obj_queue;

#define obj_queue_len(queue_ptr) ((queue_ptr) -> len)

int obj_queue_init(obj_queue*);

void obj_queue_deinit(obj_queue*);

int obj_queue_push(obj_queue*, void *obj);
void *obj_queue_pop(obj_queue*);

#endif
