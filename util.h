#pragma once

#define MAKE_TYPED_MAX(type) \
    type##_t type##_max(type##_t a, type##_t b) { return (a > b) ? a : b; }

#define MAKE_TYPED_MIN(type) \
    type##_t type##_min(type##_t a, type##_t b) { return (a < b) ? a : b; }
