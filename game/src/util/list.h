#pragma once

// #include <stdint.h>
#include <stdlib.h>
// #include <string.h>


#define GENERATE_LIST_STRUCT(T, PRE, pre) \
    typedef struct PRE##List \
    { \
        T *data; \
        size_t size; \
    } \
    PRE##List;

#define GENERATE_LIST_DECLARATION(T, PRE, pre) \
    GENERATE_LIST_STRUCT(T, PRE, pre) \
    \
    int pre##_list_init(PRE##List*); \
    int pre##_list_destroy(PRE##List*); \
    \
    int pre##_list_add(PRE##List*, T); \
    int pre##_list_insert(PRE##List*, T*, size_t); \
    int pre##_list_remove(PRE##List*, size_t, T*);

GENERATE_LIST_DECLARATION(int, I32, i32)
