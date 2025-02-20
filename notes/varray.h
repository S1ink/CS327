#pragma once

#include <stdint.h>

struct varray;      // fwd declaration
typedef struct varray VArray;

int varray_init(VArray** v, uint32_t esz);
int varray_destroy(VArray* v);
int varray_append(VArray* v, void* data);
int varray_at(VArray* v, uint32_t idx, void* data);
int varray_pop(VArray* v, void* data);
int varray_remove(VArray* v, uint32_t idx, void* data);
// int varray_resize(VArray* v);    // make this reserve
uint32_t varray_size(VArray* v);
