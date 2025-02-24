#pragma once

#include <stdint.h>


#define GENERATE_VEC2_STRUCT(T, L) \
    typedef union Vec2##L { struct{ T x, y; }; T data[2]; } Vec2##L;
#define GENERATE_VEC3_STRUCT(T, L) \
    typedef union Vec3##L { struct{ T x, y, z; }; T data[3]; } Vec3##L;


#define DEFINE_VEC2_UTILS(T, L) \
    inline static void vec2##L##_zero(Vec2##L* v) { v->x = 0; v->y = 0; } \
    inline static void vec2##L##_copy(Vec2##L* v, Vec2##L* a) { v->x = a->x; v->y = a->y; } \
    inline static void vec2##L##_assign(Vec2##L* v, T x, T y) { v->x = x; v->y = y; }
#define DEFINE_VEC3_UTILS(T, L) \
    inline static void vec3##L##_zero(Vec3##L* v) { v->x = 0; v->y = 0; v->z = 0; } \
    inline static void vec3##L##_copy(Vec3##L* v, Vec3##L* a) { v->x = a->x; v->y = a->y; v->z = a->z; } \
    inline static void vec3##L##_assign(Vec3##L* v, T x, T y, T z) { v->x = x; v->y = y; v->z = z; }

#define DEFINE_VEC2_EQUALITY(T, L) \
    inline static int vec2##L##_equal(Vec2##L* a, Vec2##L* b) { return (a->x == b->x && a->y == b->y); }
#define DEFINE_VEC3_EQUALITY(T, L) \
    inline static int vec3##L##_equal(Vec3##L* a, Vec3##L* b) { return (a->x == b->x && a->y == b->y && a->z == b->z); }

#define DEFINE_VEC2_ADD_SUB(T, L) \
    inline static void vec2##L##_add(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) { v->x = a->x + b->x; v->y = a->y + b->y; } \
    inline static void vec2##L##_sub(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) { v->x = a->x - b->x; v->y = a->y - b->y; }
#define DEFINE_VEC3_ADD_SUB(T, L) \
    inline static void vec3##L##_add(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) { v->x = a->x + b->x; v->y = a->y + b->y; v->z = a->z + b->z; } \
    inline static void vec3##L##_sub(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) { v->x = a->x - b->x; v->y = a->y - b->y; v->z = a->z - b->z; }

#define DEFINE_VEC2_CWISE_MIN_MAX(T, L) \
    inline static void vec2##L##_cwise_min(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) \
    { \
        v->x = a->x < b->x ? a->x : b->x; \
        v->y = a->y < b->y ? a->y : b->y; \
    } \
    inline static void vec2##L##_cwise_max(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) \
    { \
        v->x = a->x > b->x ? a->x : b->x; \
        v->y = a->y > b->y ? a->y : b->y; \
    }
#define DEFINE_VEC3_CWISE_MIN_MAX(T, L) \
    inline static void vec3##L##_cwise_min(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) \
    { \
        v->x = a->x < b->x ? a->x : b->x; \
        v->y = a->y < b->y ? a->y : b->y; \
        v->z = a->z < b->z ? a->z : b->z; \
    } \
    inline static void vec3##L##_cwise_max(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) \
    { \
        v->x = a->x > b->x ? a->x : b->x; \
        v->y = a->y > b->y ? a->y : b->y; \
        v->z = a->z > b->z ? a->z : b->z; \
    }


#define BUILD_FOR_VEC_GEOM_DEFAULT_TYPES \
    BUILD_FOR_TYPE(int8_t, i8) \
    BUILD_FOR_TYPE(uint8_t, u8) \
    BUILD_FOR_TYPE(int16_t, i16) \
    BUILD_FOR_TYPE(uint16_t, u16) \
    BUILD_FOR_TYPE(int32_t, i) \
    BUILD_FOR_TYPE(uint32_t, u) \
    BUILD_FOR_TYPE(int64_t, l) \
    BUILD_FOR_TYPE(uint64_t, ul) \
    BUILD_FOR_TYPE(float, f) \
    BUILD_FOR_TYPE(double, d)

#define DEFINE_VEC2_TYPE(T, t) \
    GENERATE_VEC2_STRUCT(T, t) \
    DEFINE_VEC2_UTILS(T, t) \
    DEFINE_VEC2_EQUALITY(T, t) \
    DEFINE_VEC2_ADD_SUB(T, t) \
    DEFINE_VEC2_CWISE_MIN_MAX(T, t)
#define GENERATE_VEC2_TYPE(T, t) \
    DEFINE_VEC2_TYPE(T, t)

#define DEFINE_VEC3_TYPE(T, t) \
    GENERATE_VEC3_STRUCT(T, t) \
    DEFINE_VEC3_UTILS(T, t) \
    DEFINE_VEC3_EQUALITY(T, t) \
    DEFINE_VEC3_ADD_SUB(T, t) \
    DEFINE_VEC3_CWISE_MIN_MAX(T, t)
#define GENERATE_VEC3_TYPE(T, t) \
    DEFINE_VEC3_TYPE(T, t)

#define BUILD_FOR_TYPE(x, y) DEFINE_VEC2_TYPE(x, y)
BUILD_FOR_VEC_GEOM_DEFAULT_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DEFINE_VEC3_TYPE(x, y)
BUILD_FOR_VEC_GEOM_DEFAULT_TYPES
#undef BUILD_FOR_TYPE
