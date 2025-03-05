#include <stdlib.h>
#include "queue.h"


int queue_init(Queue* q)
{
    q->front = q->back = NULL;
    q->length = 0;

    return 0;
}
int queue_destroy(Queue* q)
{
    QueueNode *node;
    while( (node = q->front) )
    {
        q->front = q->front->next;
        free(node);
    }

    q->back = NULL;
    q->length = 0;

    return 0;
}

int queue_enqueue(Queue* q, int value)
{
    QueueNode* temp;

    if( !(temp = malloc(sizeof(*temp))) ) return -1;
    temp->data = value;
    temp->next = NULL;

    if(q->front)
    {
        q->back->next = temp;
    }
    else
    {
        q->front = temp;
    }
    q->back = temp;
    q->length++;

    return 0;
}
int queue_dequeue(Queue* q, int* value)
{
    if(!q->front) return -1;

    *value = q->front->data;

    QueueNode* temp;
    temp = q->front;

    q->front = q->front->next;
    if(!q->front) q->back = NULL;
    q->length--;

    free(temp);

    return 0;
}

int queue_front(Queue* q, int* value)
{
    if(!q->front) return -1;

    *value = q->front->data;

    return 0;
}
int queue_size(Queue* q)
{
    return q->length;
}
int queue_is_empty(Queue* q)
{
    return !q->front;
}
