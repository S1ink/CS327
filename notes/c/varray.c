#include "varray.h"

#include <stdlib.h>
#include <string.h>


#ifndef VARRAY_DEFAULT_CAPACATY
#define VARRAY_DEFAULT_CAPACATY 10
#endif

struct varray
{
    char* data;
    uint32_t cap;
    uint32_t size;
    uint32_t esz;
};

static int varray_resize(VArray* v)
{
    void* x;
    if(!(x = realloc(v->data, v->cap * v->esz * 2))) return -1;

    v->data = x;
    v->cap *= 2;

    return 0;
}

int varray_init(VArray** v, uint32_t esz)
{
    if( !(*v = malloc(sizeof(**v))) ||
        !((*v)->data = malloc(VARRAY_DEFAULT_CAPACATY * esz)) )
        return -1;

    (*v)->cap = VARRAY_DEFAULT_CAPACATY;
    (*v)->size = 0;
    (*v)->esz = esz;

    return 0;
}
int varray_destroy(VArray* v)
{
    free(v->data);
    free(v);

    return 0;
}
int varray_append(VArray* v, void* data)
{
    if(v->size >= v->cap && varray_resize(v)) return -1;

    memcpy(v->data + (v->size * v->esz), data, v->esz);
    v->size++;

    return 0;
}
int varray_at(VArray* v, uint32_t idx, void* data)
{
    if(idx >= v->size) return -1;
    memcpy(data, v->data + (idx * v->esz), v->esz);

    return 0;
}
int varray_pop(VArray* v, void* data)
{
    if(!v->size) return -1;

    v->size--;
    memcpy(data, v->data + (v->size * v->esz), v->esz);

    return 0;
}
int varray_remove(VArray* v, uint32_t idx, void* data)
{
    if(!v->size || idx >= v->size) return -1;

    memcpy(data, v->data + (idx * v->esz), v->esz);

    v->size--;
    memmove(
        v->data + (idx * v->esz),
        v->data + ((idx + 1) * v->esz),
        v->esz * (v->size - idx) );

    return 0;
}
uint32_t varray_size(VArray* v)
{
    return v->size;
}



#ifdef VARRAY_MAIN
#include <stdio.h>

int main(int argc, char** argv)
{
    VArray* v;
    varray_init(&v, sizeof(int));

    for(int i = 0; i < 1000; i++)
    {
        varray_append(v, &i);
    }

    int x;
    varray_at(v, 10, &x);
    printf("%d\n", x);
    varray_at(v, 500, &x);
    printf("%d\n", x);
    varray_at(v, 999, &x);
    printf("%d\n", x);

    varray_remove(v, 500, &x);
    varray_at(v, 500, &x);
    printf("%d\n", x);
    varray_at(v, 998, &x);
    printf("%d\n", x);

    varray_destroy(v);

    return 0;
}

#endif
