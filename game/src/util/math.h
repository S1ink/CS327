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

#define NUM_BITS32_UCOUNT(n) \
    ((n) - (((n) >> 1) & 033333333333) - (((n) >> 2) & 011111111111))
#define NUM_BITS32(n) \
    (((NUM_BITS32_UCOUNT(n) + (NUM_BITS32_UCOUNT(n) >> 3)) & 030707070707) % 63)

#define REQUIRED_BITS32(n) \
    (1 + \
    ((n) >= 0x2) + \
    ((n) >= 0x4) + \
    ((n) >= 0x8) + \
    ((n) >= 0x10) + \
    ((n) >= 0x20) + \
    ((n) >= 0x40) + \
    ((n) >= 0x80) + \
    ((n) >= 0x100) + \
    ((n) >= 0x200) + \
    ((n) >= 0x400) + \
    ((n) >= 0x800) + \
    ((n) >= 0x1000) + \
    ((n) >= 0x2000) + \
    ((n) >= 0x4000) + \
    ((n) >= 0x8000) + \
    ((n) >= 0x10000) + \
    ((n) >= 0x20000) + \
    ((n) >= 0x40000) + \
    ((n) >= 0x80000) + \
    ((n) >= 0x100000) + \
    ((n) >= 0x200000) + \
    ((n) >= 0x400000) + \
    ((n) >= 0x800000) + \
    ((n) >= 0x1000000) + \
    ((n) >= 0x2000000) + \
    ((n) >= 0x4000000) + \
    ((n) >= 0x8000000) + \
    ((n) >= 0x10000000) + \
    ((n) >= 0x20000000) + \
    ((n) >= 0x40000000) + \
    ((n) >= 0x80000000) )
