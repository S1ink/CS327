#pragma once

#include <type_traits>
#include <cstdint>
#include <random>

#include "math.hpp"


namespace geom
{

namespace traits
{
    template<typename X, typename Y>
    struct larger
    {
        using type = typename std::conditional<(sizeof(X) > sizeof(Y)), X, Y>::type;
    };

    template<typename X, typename Y>
    struct autopromote
    {
        static_assert(std::is_arithmetic<X>::value && std::is_arithmetic<Y>::value);

        using type = typename std::conditional<
            (std::is_floating_point<X>::value && std::is_floating_point<Y>::value) ||
                (std::is_integral<X>::value && std::is_integral<Y>::value),
            typename larger<X, Y>::type,   // if both floating or both int, use largest
            typename std::conditional<
                (std::is_floating_point<X>::value), X, Y>::type >::type;  // otherwise use the floating point type
    };
};

template<typename T>
struct Vec2_
{
    static_assert(std::is_arithmetic<T>::value);

    using VecT = Vec2_<T>;
    using ArgT = typename std::conditional<(sizeof(T) > sizeof(std::uintptr_t)), const T&, T>::type;
    // using VecArgT = typename std::conditional<(sizeof(VecT) > sizeof(std::uintptr_t)), const VecT&, VecT>::type;

    union
    {
        struct
        {
            T x{ 0 }, y{ 0 };
        };
        T data[2];
    };

public:
    inline Vec2_() {}
    inline Vec2_(ArgT c) : x{ c }, y{ c } {}
    inline Vec2_(ArgT x, ArgT y) : x{ x }, y{ y } {}
    // inline Vec2_(const T v[2]) : x{ v[0] }, y{ v[1] } {}
    inline Vec2_(const T* v) : x{ v[0] }, y{ v[1] } {}
    inline Vec2_(const VecT& v) : Vec2_(v.x, v.y) {}
    inline Vec2_(VecT&& v) : Vec2_(v.x, v.y) {}
    inline ~Vec2_() = default;

public:
    inline VecT& assign(ArgT c)
    {
        this->x = this->y = c;
        return *this;
    }
    inline VecT& assign(ArgT x, ArgT y)
    {
        this->x = x;
        this->y = y;
        return *this;
    }
    inline VecT& assign(const T* v)
    {
        this->x = v[0];
        this->y = v[1];
        return *this;
    }
    inline VecT& operator=(const VecT& v) { return this->assign(v.data); }
    inline VecT& operator=(VecT&& v) { return this->assign(v.data); }
    template<typename U>
    inline VecT& operator=(const Vec2_<U>& v)
    {
        this->x = static_cast<T>(v.x);
        this->y = static_cast<T>(v.y);
        return *this;
    }

    template<typename U>
    inline Vec2_<U> cast() const
    {
        return Vec2_<U>{ static_cast<U>(this->x), static_cast<U>(this->y) };
    }
    template<typename U>
    inline operator Vec2_<U>() const { return this->cast<U>(); }

public:
    #define BUILD_AUTOPROMOTED_DISTRIBUTED_OPS(op) \
        template<typename U> \
        inline Vec2_<typename traits::autopromote<T, U>::type> operator op (const Vec2_<U>& v) const \
        { \
            using A = typename traits::autopromote<T, U>::type; \
            return Vec2_<A>{ \
                static_cast<A>(this->x) op static_cast<A>(v.x), \
                static_cast<A>(this->y) op static_cast<A>(v.y) }; \
        } \
        template<typename U> \
        inline VecT& operator op##= (const Vec2_<U>& v) \
        { \
            if constexpr(!std::is_same<T, U>::value) \
            { \
                using A = typename traits::autopromote<T, U>::type; \
                this->x = static_cast<T>(static_cast<A>(this->x) op static_cast<A>(v.x)); \
                this->y = static_cast<T>(static_cast<A>(this->y) op static_cast<A>(v.y)); \
            } \
            else \
            { \
                this->x op##= v.x; \
                this->y op##= v.y; \
            } \
            return *this; \
        }

    BUILD_AUTOPROMOTED_DISTRIBUTED_OPS(+)
    BUILD_AUTOPROMOTED_DISTRIBUTED_OPS(-)
    BUILD_AUTOPROMOTED_DISTRIBUTED_OPS(*)
    BUILD_AUTOPROMOTED_DISTRIBUTED_OPS(/)

    #undef BUILD_AUTOPROMOTED_DISTRIBUTED_OPS

public:
    template<typename U>
    inline bool operator==(const Vec2_<U>& v) const
    {
        using A = typename traits::autopromote<T, U>::type;
        return ( static_cast<A>(this->x) == static_cast<A>(v.x)
            && static_cast<A>(this->y) == static_cast<A>(v.y) );
    }
    template<typename U>
    inline bool operator!=(const Vec2_<U>& v) const
    {
        using A = typename traits::autopromote<T, U>::type;
        return ( static_cast<A>(this->x) != static_cast<A>(v.x)
            || static_cast<A>(this->y) != static_cast<A>(v.y) );
    }

    inline VecT cwiseMin(const VecT& v) const
    {
        return VecT{ MIN(this->x, v.x), MIN(this->y, v.y) };
    }
    inline VecT cwiseMax(const VecT& v) const
    {
        return VecT{ MAX(this->x, v.x), MAX(this->y, v.y) };
    }
    inline VecT& cwiseMinEq(const VecT& v)
    {
        if(v.x < this->x) this->x = v.x;
        if(v.y < this->y) this->y = v.y;
        return *this;
    }
    inline VecT& cwiseMaxEq(const VecT& v)
    {
        if(v.x > this->x) this->x = v.x;
        if(v.y > this->y) this->y = v.y;
        return *this;
    }

public:
    inline T sum() const
    {
        return this->x + this->y;
    }
    template<typename U>
    inline typename traits::autopromote<T, U>::type dot(const Vec2_<U>& v) const
    {
        using A = typename traits::autopromote<T, U>::type;
        return (
            (static_cast<A>(this->x) * static_cast<A>(v.x)) +
            (static_cast<A>(this->y) * static_cast<A>(v.y)) );
    }

    inline T lensquared() const
    {
        return this->dot<T>(*this);
    }
    inline double length() const
    {
        return std::sqrt(static_cast<double>(this->lensquared()));
    }

public:
    template<typename G>
    static inline VecT randomInRange(VecT min, VecT max, G& gen)
    {
        if constexpr(std::is_floating_point<T>::value)
        {
            std::uniform_real_distribution<T>
                xdist{ min.x, max.x },
                ydist{ min.y, max.y };

            return VecT{ xdist(gen), ydist(gen) };
        }
        else
        {
            std::uniform_int_distribution<T>
                xdist{ min.x, max.x },
                ydist{ min.y, max.y };

            return VecT{ xdist(gen), ydist(gen) };
        }
    }

};

};

#define DEFINE_VEC2_TYPE(T, t) using Vec2##t = geom::Vec2_<T>;

DEFINE_VEC2_TYPE(int8_t, i8)
DEFINE_VEC2_TYPE(uint8_t, u8)
DEFINE_VEC2_TYPE(int16_t, i16)
DEFINE_VEC2_TYPE(uint16_t, u16)
DEFINE_VEC2_TYPE(int32_t, i)
DEFINE_VEC2_TYPE(uint32_t, u)
DEFINE_VEC2_TYPE(int64_t, l)
DEFINE_VEC2_TYPE(uint64_t, ul)
DEFINE_VEC2_TYPE(float, f)
DEFINE_VEC2_TYPE(double, d)

#undef DEFINE_VEC2_TYPE
