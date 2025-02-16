#pragma once

/* Fibinacci heap implementation copied and reformatted
 *  from assignment 1.01 public solution */

# ifdef __cplusplus
extern "C" {
# endif

# include <stdint.h>


struct heap_node;
typedef struct heap_node HeapNode;

typedef struct heap
{
    HeapNode *min;
    uint32_t size;
    int32_t (*compare)(const void *key, const void *with);
    void (*datum_delete)(void *);
}
Heap;

void heap_init(
    Heap *h,
    int32_t (*compare)(const void *key, const void *with),
    void (*datum_delete)(void *) );
void heap_delete(Heap *h);
HeapNode *heap_insert(Heap *h, void *v);
void *heap_peek_min(Heap *h);
void *heap_remove_min(Heap *h);
int heap_combine(Heap *h, Heap *h1, Heap *h2);
int heap_decrease_key(Heap *h, HeapNode *n, void *v);
int heap_decrease_key_no_replace(Heap *h, HeapNode *n);


# ifdef __cplusplus
}
# endif
