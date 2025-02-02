#pragma once

#include <stdint.h>


#define GENERATE_VEC2_STRUCT(T, L) \
    typedef struct Vec2##L { T x, y; } Vec2##L;
#define GENERATE_VEC3_STRUCT(T, L) \
    typedef struct Vec3##L { T x, y, z; } Vec3##L;


#define BUILD_FOR_TYPES \
    BUILD_FOR_TYPE(int32_t, i) \
    BUILD_FOR_TYPE(uint32_t, u) \
    BUILD_FOR_TYPE(int64_t, l) \
    BUILD_FOR_TYPE(uint64_t, ul) \
    BUILD_FOR_TYPE(float, f) \
    BUILD_FOR_TYPE(double, d)

#define BUILD_FOR_TYPE(x, y) GENERATE_VEC2_STRUCT(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) GENERATE_VEC3_STRUCT(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE
