#pragma once

#include <cmath>
#include <cstdint>
// #include <limits>
#include <algorithm>
#include <type_traits>

#include <Eigen/Core>


/** Generic grid helpers */
namespace GridUtil
{
    enum
    {
        Y_MAJOR_ORDER,
        X_MAJOR_ORDER,
    };
    enum
    {
        GRID_STANDARD = 0,
        GRID_USING_OFFSET = 1 << 0,
        GRID_USING_VECRES = 1 << 1
    };

    template<int GridAttributes>
    struct traits
    {
        static constexpr bool Using_Offset = (GridAttributes & GRID_USING_OFFSET);
        static constexpr bool Using_VecRes = (GridAttributes & GRID_USING_VECRES);
    };

    template<typename T>
    using Vec2 = Eigen::Vector2<T>;
    template<int Dim, typename T>
    using Vec = Eigen::Vector<T, Dim>;


    template<typename T>
    inline static T clamp(T v, T min, T max)
    {
        return (v < min) ? min : ((v > max) ? max : v);
    }

    template<typename T>
    inline static Vec2<T> clamp(
        const Vec2<T>& v,
        const Vec2<T>& min,
        const Vec2<T>& max )
    {
        return v.cwiseMin(max).cwiseMax(min);
    }


    /** Align a point to a box grid of the given resolution.
     * Result may be negative if lower than current offset. */
    template<typename IntT = int, typename FloatT = float>
    inline static Vec2<IntT> gridAlign(
        FloatT x,
        FloatT y,
        FloatT res )
    {
        return Vec2<IntT>{
            static_cast<IntT>( std::floor(x / res) ),
            static_cast<IntT>( std::floor(y / res) ) };
    }

    template<typename IntT = int, typename FloatT = float, int Dim = 2>
    inline static Vec2<IntT> gridAlign(
        const Vec<Dim, FloatT>& pt,
        FloatT res )
    {
        return gridAlign<IntT, FloatT>(pt.x(), pt.y(), res);
    }

    /** Align a point to a box grid of the given resolution and offset origin.
     * Result may be negative if lower than current offset. */
    template<typename IntT = int, typename FloatT = float>
    inline static Vec2<IntT> gridAlign(
        FloatT x,
        FloatT y,
        const Vec2<FloatT>& off,
        FloatT res )
    {
        // always floor since grid cells are indexed by their "bottom left" corner's raw position
        return Vec2<IntT>{
            static_cast<IntT>( std::floor((x - off.x()) / res) ),
            static_cast<IntT>( std::floor((y - off.y()) / res) ) };
    }

    template<typename IntT = int, typename FloatT = float, int Dim = 2>
    inline static Vec2<IntT> gridAlign(
        const Vec<Dim, FloatT>& pt,
        const Vec2<FloatT>& off,
        FloatT res )
    {
        return gridAlign<IntT, FloatT>(pt.x(), pt.y(), off, res);
    }

    /** Get a raw buffer idx from a 2d index and buffer size (templated on major-order) */
    template<int Ordering = Y_MAJOR_ORDER, typename IntT = int>
    inline static int64_t gridIdx(
        const IntT x,
        const IntT y,
        const Vec2<IntT>& size )
    {
        if constexpr(Ordering == X_MAJOR_ORDER)
        {
            // x-major = "contiguous blocks along [parallel to] y-axis" --> idx = (x * ymax) + y
            return static_cast<int64_t>(x) * size.y() + y;
        }
        else
        {
            // y-major = "contiguous blocks along [parallel to] x-axis" --> idx = (y * xmax) + x
            return static_cast<int64_t>(y) * size.x() + x;
        }
    }
    template<int Ordering = Y_MAJOR_ORDER, typename IntT = int>
    inline static int64_t gridIdx(
        const Vec2<IntT>& loc,
        const Vec2<IntT>& size )
    {
        return gridIdx<Ordering, IntT>(loc.x(), loc.y(), size);
    }

    /** Same as gridIdx() but clamps input x,y to be inside the given dimensions */
    template<int Ordering = Y_MAJOR_ORDER, typename IntT = int>
    inline static size_t clampedGridIdx(
        IntT x,
        IntT y,
        const Vec2<IntT>& size )
    {
        x = (x < 0) ? 0 : (x >= size.x()) ? (size.x() - 1) : x;
        y = (y < 0) ? 0 : (y >= size.y()) ? (size.y() - 1) : y;

        if constexpr(Ordering == X_MAJOR_ORDER)
        {
            // see gridIdx<>()
            return static_cast<size_t>(x) * size.y() + y;
        }
        else
        {
            // see gridIdx<>()
            return static_cast<size_t>(y) * size.x() + x;
        }
    }
    template<int Ordering = Y_MAJOR_ORDER, typename IntT = int>
    inline static size_t clampedGridIdx(
        const Vec2<IntT>& loc,
        const Vec2<IntT>& size )
    {
        return clampedGridIdx<Ordering, IntT>(loc.x(), loc.y(), size);
    }

    /** Get the 2d location corresponding to a raw buffer idx for the provded grid size (templated on major-order) */
    template<int Ordering = Y_MAJOR_ORDER, typename IntT = int>
    inline static Vec2<IntT> gridLoc(
        const size_t idx,
        const Vec2<IntT>& size )
    {
        if constexpr(Ordering == X_MAJOR_ORDER)
        {
            // x-major = "contiguous blocks along [parallel to] y-axis" --> x = idx / ymax, y = idx % ymax
            return Vec2<IntT>{
                static_cast<IntT>(idx / size.y()),
                static_cast<IntT>(idx % size.y()) };
        }
        else
        {
            // y-major = "contiguous blocks along [parallel to] x-axis" --> x = idx % xmax, y = idx / xmax
            return Vec2<IntT>{
                static_cast<IntT>(idx % size.x()),
                static_cast<IntT>(idx / size.x()) };
        }
    }

    /** Copy a 2d windows of elemens - expects element types to be POD (templated on major-order) */
    template<
        typename T,
        int Ordering = Y_MAJOR_ORDER,
        typename IntT = int,
        size_t T_Bytes = sizeof(T) >
    static void memcpyWindow(
        T* dest,
        const T* src,
        const Vec2<IntT>& dest_size,
        const Vec2<IntT>& src_size,
        const Vec2<IntT>& diff )
    {
        if constexpr(Ordering == X_MAJOR_ORDER)
        {
            // iterate through source "rows" of contiguous memory (along y -- for each x)
            for(int64_t _x = 0; _x < src_size.x(); _x++)
            {
                memcpy(
                    dest + ((_x + diff.x()) * dest_size.y() + diff.y()),
                    src + (_x * src_size.y()),
                    src_size.y() * T_Bytes );
            }
        }
        else
        {
            // iterate through source "rows" of contiguous memory (along x -- for each y)
            for(int64_t _y = 0; _y < src_size.y(); _y++)
            {
                memcpy(
                    dest + ((_y + diff.y()) * dest_size.x() + diff.x()),
                    src + (_y * src_size.x()),
                    src_size.x() * T_Bytes );
            }
        }
    }

};



namespace agu_base
{
    struct Empty{};

    template<typename T>
    struct Offset{ Eigen::Vector2<T> grid_off{ 0, 0 }; };
};

template<
    typename DimInt_T = int,
    typename DimFloat_T = float,
    int Ordering = GridUtil::Y_MAJOR_ORDER,
    int Attributes = GridUtil::GRID_STANDARD >
class AbstractGridUtil :
    protected std::conditional<
        (Attributes & GridUtil::GRID_USING_OFFSET),
        agu_base::Offset<DimFloat_T>, agu_base::Empty >::type
{
    static_assert(std::is_integral<DimInt_T>::value);
    static_assert(std::is_floating_point<DimFloat_T>::value);

public:
    using DimIntT = DimInt_T;
    using DimFloatT = DimFloat_T;
    using ThisT = AbstractGridUtil<DimIntT, DimFloatT, Ordering, Attributes>;

    using IdxT = int64_t;
    using UIdxT = size_t;

    template<typename T>
    using Vec2 = GridUtil::Vec2<T>;
    template<int Dim, typename T>
    using Vec = GridUtil::Vec<Dim, T>;

    using DimVec2i = Vec2<DimIntT>;
    using DimVec2f = Vec2<DimFloatT>;

    using SpacingT = typename std::conditional<
                            (Attributes & GridUtil::GRID_USING_VECRES),
                            DimVec2f, DimFloatT >::type;
    using SpacingArgT = typename std::conditional<
                            (sizeof(SpacingT) >= sizeof(uintptr_t)),
                            const SpacingT&,
                            SpacingT >::type;

public:
    inline AbstractGridUtil(const DimVec2i& dim, SpacingArgT res)
        { this->initialize(dim, res); }
    inline AbstractGridUtil(const DimVec2f& size, SpacingArgT res)
        { this->initialize(size, res); }
    inline AbstractGridUtil(const DimVec2i& dim, SpacingArgT res, const DimVec2f& off)
        { this->initialize(dim, res, off); }
    inline AbstractGridUtil(const DimVec2f& size, SpacingArgT res, const DimVec2f& off)
        { this->initialize(size, res, off); }

public:
    void initialize(const DimVec2i& dim, SpacingArgT res);
    void initialize(const DimVec2f& size, SpacingArgT res);
    void initialize(const DimVec2i& dim, SpacingArgT res, const DimVec2f& off);
    void initialize(const DimVec2f& size, SpacingArgT res, const DimVec2f& off);

    void resize(const DimVec2i& dim);
    void resize(const DimVec2f& size);

    // TODO: resize and realloc

public:
    inline const DimVec2i& dim() const { return this->grid_dim; }
    inline const DimVec2f& size() const { return this->grid_size; }
    inline const SpacingArgT cellSize() const { return this->grid_res; }
    inline const SpacingArgT invCellSize() const { return this->inv_grid_res; }
    inline const UIdxT numCells() const { return static_cast<UIdxT>(this->grid_dim.prod()); }
    inline const DimFloatT area() const { return this->grid_size.prod(); }
    inline const DimVec2i minCell() const { return DimVec2i::Zero(); }
    inline const DimVec2i maxCell() const { return this->grid_dim - DimVec2i::Ones(); }
    const DimVec2f minBound() const;
    inline const DimVec2f maxBound() const { return this->minBound() + this->grid_size; }

public:
    // Clamp the coordinates to the grid's bounding coordinates
    template<typename T>
    DimVec2f clampToBounds(T x, T y) const;
    template<typename T>
    inline DimVec2f clampToBounds(const Vec2<T>& p) const { return this->clampToBounds(p.x(), p.y()); }

    // Clamp the coordinates to the grid's local (cell)
    template<typename T>
    DimVec2i clampToLocalBounds(T x, T y) const;
    template<typename T>
    inline DimVec2i clampToLocalBounds(const Vec2<T>& p) const { return this->clampToLocalBounds(p.x(), p.y()); }

    // Get the cell which contains the provided (x, y) -- may be out of bounds of the grid
    template<typename T>
    DimVec2i align(T x, T y) const;
    template<typename T, int Dim = 2>
    inline DimVec2i align(const Vec<Dim, T>& p) const { return this->align(p.x(), p.y()); }

    // Get the cell which contains the clamped coordinates of the provided (x, y)
    template<typename T>
    inline DimVec2i clampedAlign(T x, T y) const { return this->align(this->clampToBounds(x, y)); }
    template<typename T, int Dim = 2>
    inline DimVec2i clampedAlign(const Vec<Dim, T>& p) const { return this->clampedAlign(p.x(), p.y()); }

    // Gets the index of the cell which contains the provided (x, y) -- may be out of bounds of the grid
    template<typename T>
    inline IdxT cellIdx(T x, T y) const { return this->localIdx(this->align(x, y)); }
    template<typename T, int Dim = 2>
    inline IdxT cellIdx(const Vec<Dim, T>& c) const { return this->cellIdx(c.x(), c.y()); }

    // Gets the index of the cell which contains the clamped coordinates of the provided (x, y)
    template<typename T>
    inline UIdxT clampedCellIdx(T x, T y) const { return this->clampedLocalIdx(this->align(x, y)); }
    template<typename T, int Dim = 2>
    inline UIdxT clampedCellIdx(const Vec<Dim, T>& c) const { return this->clampedCellIdx(c.x(), c.y()); }

    // Gets the index of the cell which contains the provided (x, y) in grid space -- may be out of bounds of the grid
    template<typename T>
    IdxT localIdx(T x, T y) const;
    template<typename T, int Dim = 2>
    inline IdxT localIdx(const Vec<Dim, T>& c) const { return this->localIdx(c.x(), c.y()); }

    // Gets the index of the cell which contains the clamped coordinates of the provided (x, y) in grid space
    template<typename T>
    UIdxT clampedLocalIdx(T x, T y) const;
    template<typename T, int Dim = 2>
    inline UIdxT clampedLocalIdx(const Vec<Dim, T>& c) const { return this->clampedLocalIdx(c.x(), c.y()); }

    // Gets the cell coordinates for the provided index
    DimVec2i cellCoords(UIdxT idx) const;

protected:
    inline DimFloatT resX() const
    {
        if constexpr(Attributes & GridUtil::GRID_USING_VECRES) return this->grid_res.x();
        else return this->grid_res;
    }
    inline DimFloatT resY() const
    {
        if constexpr(Attributes & GridUtil::GRID_USING_VECRES) return this->grid_res.y();
        else return this->grid_res;
    }
    inline DimFloatT invResX() const
    {
        if constexpr(Attributes & GridUtil::GRID_USING_VECRES) return this->inv_grid_res.x();
        else return this->inv_grid_res;
    }
    inline DimFloatT invResY() const
    {
        if constexpr(Attributes & GridUtil::GRID_USING_VECRES) return this->inv_grid_res.y();
        else return this->inv_grid_res;
    }

protected:
    DimVec2i grid_dim;
    DimVec2f grid_size;
    SpacingT grid_res, inv_grid_res;

};



template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::initialize(
    const DimVec2i& dim,
    SpacingArgT res )
{
    this->grid_dim = dim;
    this->grid_res = res;
    if constexpr(Attributes & GridUtil::GRID_USING_VECRES)
    {
        this->grid_size = dim.template cast<DimFloatT>().cwiseProduct(res);
        this->inv_grid_res = DimVec2f::Ones().cwiseQuotient(res);
    }
    else
    {
        this->grid_size = dim * res;
        this->inv_grid_res = 1 / res;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::initialize(
    const DimVec2f& size,
    SpacingArgT res )
{
    this->grid_res = res;
    if constexpr(Attributes & GridUtil::GRID_USING_VECRES)
    {
        this->inv_grid_res = DimVec2f::Ones().cwiseQuotient(res);
        this->grid_dim.x() = static_cast<DimIntT>(std::ceil(size.x() * this->inv_grid_res.x()));
        this->grid_dim.y() = static_cast<DimIntT>(std::ceil(size.y() * this->inv_grid_res.y()));
        this->grid_size = this->grid_dim.template cast<DimFloatT>().cwiseProduct(res);
    }
    else
    {
        this->inv_grid_res = 1 / res;
        this->grid_dim.x() = static_cast<DimIntT>(std::ceil(size.x() * this->inv_grid_res));
        this->grid_dim.y() = static_cast<DimIntT>(std::ceil(size.y() * this->inv_grid_res));
        this->grid_size = this->grid_dim.template cast<DimFloatT>() * res;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::initialize(
    const DimVec2i& dim,
    SpacingArgT res,
    const DimVec2f& off )
{
    this->initialize(dim, res);
    if constexpr(Attributes & GridUtil::GRID_USING_OFFSET)
    {
        this->grid_off = off;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::initialize(
    const DimVec2f& size,
    SpacingArgT res,
    const DimVec2f& off )
{
    this->initialize(size, res);
    if constexpr(Attributes & GridUtil::GRID_USING_OFFSET)
    {
        this->grid_off = off;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::resize(const DimVec2i& dim)
{
    this->grid_dim = dim;
    if constexpr(Attributes & GridUtil::GRID_USING_VECRES)
    {
        this->grid_size = dim.template cast<DimFloatT>().cwiseProduct(this->grid_res);
    }
    else
    {
        this->grid_size = dim.template cast<DimFloatT>() * this->grid_res;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
void AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::resize(const DimVec2f& size)
{
    if constexpr(Attributes & GridUtil::GRID_USING_VECRES)
    {
        this->grid_dim.x() = static_cast<DimIntT>(std::ceil(size.x() * this->inv_grid_res.x()));
        this->grid_dim.y() = static_cast<DimIntT>(std::ceil(size.y() * this->inv_grid_res.y()));
        this->grid_size = this->grid_dim.template cast<DimFloatT>().cwiseProduct(this->grid_res);
    }
    else
    {
        this->grid_dim.x() = static_cast<DimIntT>(std::ceil(size.x() * this->inv_grid_res));
        this->grid_dim.y() = static_cast<DimIntT>(std::ceil(size.y() * this->inv_grid_res));
        this->grid_size = this->grid_dim.template cast<DimFloatT>() * this->grid_res;
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
const typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2f
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::minBound() const
{
    if constexpr(Attributes & GridUtil::GRID_USING_OFFSET)
    {
        return this->grid_off;
    }
    else
    {
        return DimVec2f::Zero();
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
template<typename T>
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2f
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::clampToBounds(T x, T y) const
{
    const DimVec2f min = this->minBound();
    const DimVec2f max = this->maxBound();

    return DimVec2f{
        std::clamp(static_cast<DimFloatT>(x), min.x(), max.x()),
        std::clamp(static_cast<DimFloatT>(y), min.y(), max.y()) };
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
template<typename T>
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2i
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::clampToLocalBounds(T x, T y) const
{
    return DimVec2i{
        std::clamp(static_cast<DimIntT>(x), 0, this->grid_dim.x() - 1),
        std::clamp(static_cast<DimIntT>(y), 0, this->grid_dim.y() - 1) };
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
template<typename T>
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2i
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::align(T x, T y) const
{
    if constexpr(Attributes & GridUtil::GRID_USING_OFFSET)
    {
        return DimVec2i{
            static_cast<DimIntT>( std::floor((x - this->grid_off.x()) * this->invResX()) ),
            static_cast<DimIntT>( std::floor((y - this->grid_off.y()) * this->invResY()) ) };
    }
    else
    {
        return DimVec2i{
            static_cast<DimIntT>( std::floor(x * this->invResX()) ),
            static_cast<DimIntT>( std::floor(y * this->invResY()) ) };
    }
}

// template<
//     typename DimInt_T,
//     typename DimFloat_T,
//     int Ordering,
//     int Attributes >
// template<typename T>
// AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2i
//     AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::clampedAlign(T x, T y) const
// {
//     return this->align(this->clampToBounds(x, y));
// }

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
template<typename T>
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::IdxT
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::localIdx(T x, T y) const
{
    if constexpr(Ordering == GridUtil::X_MAJOR_ORDER)
    {
        return static_cast<IdxT>(static_cast<DimIntT>(x) * this->grid_dim.y() + static_cast<DimIntT>(y));
    }
    else
    {
        return static_cast<IdxT>(static_cast<DimIntT>(y) * this->grid_dim.x() + static_cast<DimIntT>(x));
    }
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
template<typename T>
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::UIdxT
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::clampedLocalIdx(T x, T y) const
{
    DimIntT x_ = std::clamp(static_cast<DimIntT>(x), 0, this->grid_dim.x() - 1);
    DimIntT y_ = std::clamp(static_cast<DimIntT>(y), 0, this->grid_dim.y() - 1);

    return this->localIdx(x_, y_);
}

template<
    typename DimInt_T,
    typename DimFloat_T,
    int Ordering,
    int Attributes >
typename AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::DimVec2i
    AbstractGridUtil<DimInt_T, DimFloat_T, Ordering, Attributes>::cellCoords(UIdxT idx) const
{
    if constexpr(Ordering == GridUtil::X_MAJOR_ORDER)
    {
        // x-major = "contiguous blocks along [parallel to] y-axis" --> x = idx / ymax, y = idx % ymax
        return DimVec2i{
            static_cast<DimIntT>(idx / this->grid_dim.y()),
            static_cast<DimIntT>(idx % this->grid_dim.y()) };
    }
    else
    {
        // y-major = "contiguous blocks along [parallel to] x-axis" --> x = idx % xmax, y = idx / xmax
        return DimVec2i{
            static_cast<DimIntT>(idx % this->grid_dim.x()),
            static_cast<DimIntT>(idx / this->grid_dim.x()) };
    }
}















/** GridBase contains all generic functinality for a dynamic grid cells (template type) */
template<
    typename Cell_t,
    typename Int_t = int,
    typename Float_t = float,
    bool X_Major = false,
    size_t Max_Alloc_Bytes = (1ULL << 30)>
class GridBase
{
    static_assert(std::is_integral_v<Int_t>, "");
    static_assert(std::is_floating_point_v<Float_t>, "");
    static_assert(Max_Alloc_Bytes >= sizeof(Cell_t), "");

public:
    using Cell_T = Cell_t;
    using IntT = Int_t;
    using FloatT = Float_t;
    using This_T = GridBase< Cell_T, IntT, FloatT, X_Major, Max_Alloc_Bytes >;

    using Vec2i = Eigen::Vector2<IntT>;
    using Vec2f = Eigen::Vector2<FloatT>;

    static constexpr bool
        X_Major_Order = X_Major;
    static constexpr size_t
        Bytes_Per_Cell = sizeof(Cell_T),
        Max_Alloc_NCells = Max_Alloc_Bytes / Bytes_Per_Cell;

    template<typename T>
    constexpr inline static const IntT literalI(T v) { return static_cast<IntT>(v); }
    template<typename T>
    constexpr inline static const FloatT literalF(T v) { return static_cast<FloatT>(v); }

public:
    template<typename IT = IntT>
    inline static int64_t gridIdx(
        const IT x,
        const IT y,
        const Eigen::Vector2<IT>& size )
    {
        return GridUtil::gridIdx<This_T::X_Major_Order, IT>(x, y, size);
    }

    template<typename IT = IntT>
    inline static int64_t gridIdx(
        const Eigen::Vector2<IT>& loc,
        const Eigen::Vector2<IT>& size )
    {
        return GridUtil::gridIdx<This_T::X_Major_Order, IT>(loc, size);
    }

    template<typename IT = IntT>
    inline static int64_t clampedGridIdx(
        const IT x,
        const IT y,
        const Eigen::Vector2<IT>& size )
    {
        return GridUtil::clampedGridIdx<This_T::X_Major_Order, IT>(x, y, size);
    }

    template<typename IT = IntT>
    inline static int64_t clampedGridIdx(
        const Eigen::Vector2<IT>& loc,
        const Eigen::Vector2<IT>& size )
    {
        return GridUtil::clampedGridIdx<This_T::X_Major_Order, IT>(loc, size);
    }

    template<typename IT = IntT>
    inline static Eigen::Vector2<IT> gridLoc(
        const size_t idx,
        const Eigen::Vector2<IT>& size )
    {
        return GridUtil::gridLoc<This_T::X_Major_Order, IT>(idx, size);
    }

    template<typename T = Cell_T, typename IT = IntT, size_t T_Bytes = sizeof(T)>
    inline static void memcpyWindow(
        T* dest, const T* src,
        const Eigen::Vector2<IT>& dest_size, const Eigen::Vector2<IT>& src_size,
        const Eigen::Vector2<IT>& diff )
    {
        return GridUtil::memcpyWindow<T, This_T::X_Major_Order, IT, T_Bytes>(dest, src, dest_size, src_size, diff);
    }

public:
    inline GridBase() = default;
    inline ~GridBase()
    {
        if(this->grid) delete[] this->grid;
    }

    inline GridBase(const GridBase& g) :
        grid_origin{ g.grid_origin },
        grid_size{ g.grid_size },
        cell_res{ g.cell_res }
    {
        if(g.grid)
        {
            const int64_t a = this->area();
            this->grid = new Cell_T[a];
            memcpy(this->grid, g.grid, a * Bytes_Per_Cell);
        }
    }
    inline GridBase(GridBase&& g) :
        grid_origin{ g.grid_origin },
        grid_size{ g.grid_size },
        cell_res{ g.cell_res }
    {
        this->grid = g.grid;

        g.grid_size.setZero();
        g.grid = nullptr;
    }

    inline GridBase& operator=(const GridBase& g)
    {
        this->grid_origin = g.grid_origin;
        this->gird_size = g.grid_size;
        this->cell_res = g.cell_res;

        this->deleteData();

        if(g.grid)
        {
            const int64_t a = this->area();
            this->grid = new Cell_T[a];
            memcpy(this->grid, g.grid, a * Bytes_Per_Cell);
        }
    }
    inline GridBase& operator=(GridBase&& g)
    {
        this->grid_origin = g.grid_origin;
        this->gird_size = g.grid_size;
        this->cell_res = g.cell_res;

        this->deleteData();

        this->grid = g.grid;

        g.grid_size.setZero();
        g.grid = nullptr;
    }

public:
    inline const Vec2i& size() const
    {
        return this->grid_size;
    }
    inline const int64_t area() const
    {
        return static_cast<int64_t>(this->grid_size.x()) * this->grid_size.y();
    }
    inline const Vec2f& origin() const
    {
        return this->grid_origin;
    }
    inline const Vec2f& gridSpan() const
    {
        return (this->grid_size.template cast<FloatT>() * this->cell_res);
    }
    inline const Vec2f& maxBound() const
    {
        return this->grid_origin + this->gridSpan();
    }
    inline const FloatT cellRes() const
    {
        return this->cell_res;
    }
    inline const Cell_T* gridData() const
    {
        return this->grid;
    }

    inline const Vec2i boundingLoc(FloatT x, FloatT y) const
    {
        return GridUtil::gridAlign<IntT, FloatT>(x, y, this->grid_origin, this->cell_res);
    }
    inline const int64_t cellIdxOf(FloatT x, FloatT y) const
    {
        return This_T::gridIdx<IntT>(this->boundingLoc(x, y), this->grid_size);
    }
    inline const int64_t clampedCellIdxOf(FloatT x, FloatT y) const
    {
        return This_T::clampedGridIdx<IntT>(this->boundingLoc(x, y), this->grid_size);
    }

public:
    inline Cell_T& at(size_t i)
    {
        return this->grid[i];
    }
    inline const Cell_T& at(size_t i) const
    {
        return this->grid[i];
    }
    inline Cell_T& at(IntT x, IntT y)
    {
        return this->grid[This_T::gridIdx(x, y, this->size)];
    }
    inline const Cell_T& at(IntT x, IntT y) const
    {
        return this->grid[This_T::gridIdx(x, y, this->size)];
    }
    inline Cell_T& at(const Vec2i& v)
    {
        return this->grid[This_T::gridIdx(v, this->size)];
    }
    inline const Cell_T& at(const Vec2i& v) const
    {
        return this->grid[This_T::gridIdx(v, this->size)];
    }

    inline Cell_T& operator[](size_t i)
    {
        return this->at(i);
    }
    inline const Cell_T& operator[](size_t i) const
    {
        return this->at(i);
    }
    inline Cell_T& operator()(IntT x, IntT y)
    {
        return this->at(x, y);
    }
    inline const Cell_T& operator()(IntT x, IntT y) const
    {
        return this->at(x, y);
    }
    inline Cell_T& operator[](const Vec2i& v)
    {
        return this->at(v);
    }
    inline const Cell_T& operator[](const Vec2i& v) const
    {
        return this->at(v);
    }

    inline Cell_T* begin()
    {
        return this->grid;
    }
    inline Cell_T* end()
    {
        return this->grid + static_cast<size_t>(this->area()) * Bytes_Per_Cell;
    }

public:
    void reset(
        FloatT resolution = literalF(1),
        const Vec2f origin = Vec2f::Zero() )
    {
        this->deleteData();

        this->grid_size = Vec2i::Zero();
        this->cell_res = (resolution <= literalF(0)) ? literalF(0) : resolution;
        this->grid_origin = origin;
    }


    /** Returns false if an invalid realloc was skipped */
    template<bool NeedZeroed = true, bool Trim = false>
    bool resizeToBounds(const Vec2f& min, const Vec2f& max)
    {
        static const Vec2i
            _zero = Vec2i::Zero(),
            _one = Vec2i::Ones();

        // grid cell locations containing min and max, aligned with current offsets
        const Vec2i
            _min = this->boundingLoc(min.x(), min.y()),
            _max = this->boundingLoc(max.x(), max.y());

        // if (_min.cwiseLess(_zero).any() || _max.cwiseGreater(this->grid_size).any()) {
        if( (_min.x() < _zero.x() || _min.y() < _zero.y()) ||
            (_max.x() > this->grid_size.x() || _max.y() > this->grid_size.y()) )
        {
            const Vec2i
                // new high and low bounds for the map
                _low = _min.cwiseMin(_zero),
                // need to add an additional row + col since indexing happens in the corner,
                // thus by default the difference between corners will not cover the entire size
                _high = _max.cwiseMax(this->grid_size) + _one,
                // new map size
                _size = _high - _low;

            const int64_t _area = static_cast<int64_t>(_size.x()) * _size.y();

            // less than a gigabyte of allocated buffer is ideal
            if(_area < 0LL || static_cast<size_t>(_area) > This_T::Max_Alloc_NCells) return false;

            Cell_T* _grid = new Cell_T[_area];
            if constexpr(NeedZeroed)
            {
                memset(_grid, 0x00, _area * This_T::Bytes_Per_Cell);
            }

            // by how many grid cells did the origin shift
            const Vec2i _diff = _zero - _low;
            if(this->grid)
            {
                This_T::memcpyWindow(_grid, this->grid, _size, this->grid_size, _diff);
                delete[] this->grid;
            }

            this->grid_origin -= (_diff.template cast<FloatT>() * this->cell_res);
            this->grid_size = _size;
            this->grid = _grid;
        }
        return true;
    }

protected:
    inline void deleteData()
    {
        if(this->grid)
        {
            delete[] this->grid;
            this->grid = nullptr;
        }
    }

protected:
    Vec2f grid_origin{};
    Vec2i grid_size{};
    FloatT cell_res{ static_cast<FloatT>(1) };
    Cell_T* grid{ nullptr };

};




// #include <ratio>

// /** constexpr conditional value (v1 = true val, v2 = false val) */
// template<bool _test, typename T, T tV, T fV>
// struct conditional_literal {
//     static constexpr T value = tV;
// };
// template<typename T, T tV, T fV>
// struct conditional_literal<false, T, tV, fV> {
//     static constexpr T value = fV;
// };

// template<typename>
// struct is_ratio : std::false_type{};
// template<std::intmax_t A, std::intmax_t B>
// struct is_ratio<std::ratio<A, B>> : std::true_type{};
