#pragma once

#include <stdint.h>


#define GENERATE_VEC2_STRUCT(T, L) \
    typedef struct Vec2##L { T x, y; } Vec2##L;
#define GENERATE_VEC3_STRUCT(T, L) \
    typedef struct Vec3##L { T x, y, z; } Vec3##L;

#define DECLARE_VEC2_UTILS(T, L) \
    void vec2##L##_zero(Vec2##L* v); \
    void vec2##L##_copy(Vec2##L* v, Vec2##L* a); \
    void vec2##L##_assign(Vec2##L* v, T x, T y);
#define DECLARE_VEC3_UTILS(T, L) \
    void vec3##L##_zero(Vec3##L* v); \
    void vec3##L##_copy(Vec3##L* v, Vec3##L* a); \
    void vec3##L##_assign(Vec3##L* v, T x, T y, T z);

#define DECLARE_VEC2_ADD_SUB(T, L) \
    void vec2##L##_add(Vec2##L* v, const Vec2##L* a, const Vec2##L* b); \
    void vec2##L##_sub(Vec2##L* v, const Vec2##L* a, const Vec2##L* b);
#define DECLARE_VEC3_ADD_SUB(T, L) \
    void vec3##L##_add(Vec3##L* v, const Vec3##L* a, const Vec3##L* b); \
    void vec3##L##_sub(Vec3##L* v, const Vec3##L* a, const Vec3##L* b);

#define DECLARE_VEC2_CWISE_MIN_MAX(T, L) \
    void vec2##L##_cwise_min(Vec2##L* v, const Vec2##L* a, const Vec2##L* b); \
    void vec2##L##_cwise_max(Vec2##L* v, const Vec2##L* a, const Vec2##L* b);
#define DECLARE_VEC3_CWISE_MIN_MAX(T, L) \
    void vec3##L##_cwise_min(Vec3##L* v, const Vec3##L* a, const Vec3##L* b); \
    void vec3##L##_cwise_max(Vec3##L* v, const Vec3##L* a, const Vec3##L* b);


#define DEFINE_VEC2_UTILS(T, L) \
    void vec2##L##_zero(Vec2##L* v) { v->x = 0; v->y = 0; } \
    void vec2##L##_copy(Vec2##L* v, Vec2##L* a) { v->x = a->x; v->y = a->y; } \
    void vec2##L##_assign(Vec2##L* v, T x, T y) { v->x = x; v->y = y; }
#define DEFINE_VEC3_UTILS(T, L) \
    void vec3##L##_zero(Vec3##L* v) { v->x = 0; v->y = 0; v->z = 0; } \
    void vec3##L##_copy(Vec3##L* v, Vec3##L* a) { v->x = a->x; v->y = a->y; v->z = a->z; } \
    void vec3##L##_assign(Vec3##L* v, T x, T y, T z) { v->x = x; v->y = y; v->z = z; }

#define DEFINE_VEC2_ADD_SUB(T, L) \
    void vec2##L##_add(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) { v->x = a->x + b->x; v->y = a->y + b->y; } \
    void vec2##L##_sub(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) { v->x = a->x - b->x; v->y = a->y - b->y; }
#define DEFINE_VEC3_ADD_SUB(T, L) \
    void vec3##L##_add(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) { v->x = a->x + b->x; v->y = a->y + b->y; v->z = a->z + b->z; } \
    void vec3##L##_sub(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) { v->x = a->x - b->x; v->y = a->y - b->y; v->z = a->z - b->z; }

#define DEFINE_VEC2_CWISE_MIN_MAX(T, L) \
    void vec2##L##_cwise_min(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) \
    { \
        v->x = a->x < b->x ? a->x : b->x; \
        v->y = a->y < b->y ? a->y : b->y; \
    } \
    void vec2##L##_cwise_max(Vec2##L* v, const Vec2##L* a, const Vec2##L* b) \
    { \
        v->x = a->x > b->x ? a->x : b->x; \
        v->y = a->y > b->y ? a->y : b->y; \
    }
#define DEFINE_VEC3_CWISE_MIN_MAX(T, L) \
    void vec3##L##_cwise_min(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) \
    { \
        v->x = a->x < b->x ? a->x : b->x; \
        v->y = a->y < b->y ? a->y : b->y; \
        v->z = a->z < b->z ? a->z : b->z; \
    } \
    void vec3##L##_cwise_max(Vec3##L* v, const Vec3##L* a, const Vec3##L* b) \
    { \
        v->x = a->x > b->x ? a->x : b->x; \
        v->y = a->y > b->y ? a->y : b->y; \
        v->z = a->z > b->z ? a->z : b->z; \
    }


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

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC2_UTILS(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC3_UTILS(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC2_ADD_SUB(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC3_ADD_SUB(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC2_CWISE_MIN_MAX(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#define BUILD_FOR_TYPE(x, y) DECLARE_VEC3_CWISE_MIN_MAX(x, y)
BUILD_FOR_TYPES
#undef BUILD_FOR_TYPE

#undef BUILD_FOR_TYPES
