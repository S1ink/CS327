#pragma once

typedef struct queue_node
{
    struct queue_node* next;
    int data;
}
QueueNode;

typedef struct queue
{
    struct queue_node
        *front,
        *back;
    int length;
}
Queue;

int queue_init(Queue* q);
int queue_destroy(Queue* q);

int queue_enqueue(Queue* q, int value);
int queue_dequeue(Queue* q, int* value);

int queue_front(Queue* q, int* value);
int queue_size(Queue* q);
int queue_is_empty(Queue* q);
