#pragma once

#include <stdlib.h>


#define GENERATE_MIN_MAX_UTIL(T, L) \
    static inline T L##_max(T a, T b) { return (a > b) ? a : b; } \
    static inline T L##_min(T a, T b) { return (a < b) ? a : b; }

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN_CACHED(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); (_a < _b) ? _a : _b; })
#define MAX_CACHED(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); (_a > _b) ? _a : _b; })

#define RANDOM_IN_RANGE(min, max) ((min) + (typeof(min))(rand() % ((max) - (min) + 1)))
#define RANDOM_IN_RANGE_CACHED(min, max) ({ typeof(min) _m = (min); typeof(max) _M = (max); (_m + (typeof(_m))(rand() % (_M - _m + 1))); })

