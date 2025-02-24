#pragma once

#define GENERATE_MIN_MAX_UTIL(T, L) \
    static inline T L##_max(T a, T b) { return (a > b) ? a : b; } \
    static inline T L##_min(T a, T b) { return (a < b) ? a : b; }
