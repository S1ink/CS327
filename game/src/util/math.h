#pragma once

#define GENERATE_MIN_MAX_UTIL(T, L) \
    T L##_max(T a, T b) { return (a > b) ? a : b; } \
    T L##_min(T a, T b) { return (a < b) ? a : b; }
