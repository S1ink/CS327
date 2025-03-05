#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


int main(int argc, char** argv)
{
    Queue q;
    int i, j;

    if(queue_init(&q)) return -1;

    for(i = 0; i < 10; i++)
    {
        if(queue_enqueue(&q, i)) return -1;
    }

    queue_front(&q, &j);
    printf("length: %d, is empty: %d, front: %d\n", queue_size(&q), queue_is_empty(&q), j);

    while(!queue_dequeue(&q, &j))
    {
        printf("%d\n", j);
    }

    printf("length: %d, is empty: %d\n", queue_size(&q), queue_is_empty(&q));

    queue_destroy(&q);

    return 0;
}