#pragma once

#include <cmath>
#include <cstdint>
// #include <limits>
#include <type_traits>

#include <Eigen/Core>


/** Generic grid helpers */
namespace GridUtil
{
    /** Align a point to a box grid of the given resolution and offset origin.
     * Result may be negative if lower than current offset. */
    template<typename IntT = int, typename FloatT = float>
    inline static Eigen::Vector2<IntT> gridAlign(
        FloatT x,
        FloatT y,
        const Eigen::Vector2<FloatT>& off,
        FloatT res )
    {
        // always floor since grid cells are indexed by their "bottom left" corner's raw position
        return Eigen::Vector2<IntT>{
            static_cast<IntT>( std::floor((x - off.x()) / res) ),
            static_cast<IntT>( std::floor((y - off.y()) / res) ) };
    }

    template<typename IntT = int, typename FloatT = float>
    inline static Eigen::Vector2<IntT> gridAlign(
        const Eigen::Vector4<FloatT>& pt,
        const Eigen::Vector2<FloatT>& off,
        FloatT res )
    {
        return gridAlign<IntT, FloatT>(pt.x(), pt.y(), off, res);
    }

    /** Get a raw buffer idx from a 2d index and buffer size (templated on major-order) */
    template<bool X_Major = false, typename IntT = int>
    inline static int64_t gridIdx(
        const IntT x,
        const IntT y,
        const Eigen::Vector2<IntT>& size )
    {
        if constexpr(X_Major)
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
    template<bool X_Major = false, typename IntT = int>
    inline static int64_t gridIdx(
        const Eigen::Vector2<IntT>& loc,
        const Eigen::Vector2<IntT>& size )
    {
        return gridIdx<X_Major, IntT>(loc.x(), loc.y(), size);
    }

    /** Same as gridIdx() but clamps input x,y to be inside the given dimensions */
    template<bool X_Major = false, typename IntT = int>
    inline static size_t clampedGridIdx(
        IntT x,
        IntT y,
        const Eigen::Vector2<IntT>& size )
    {
        x = (x < 0) ? 0 : (x >= size.x()) ? (size.x() - 1) : x;
        y = (y < 0) ? 0 : (y >= size.y()) ? (size.y() - 1) : y;

        if constexpr(X_Major)
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
    template<bool X_Major = false, typename IntT = int>
    inline static size_t clampedGridIdx(
        const Eigen::Vector2<IntT>& loc,
        const Eigen::Vector2<IntT>& size )
    {
        return clampedGridIdx<X_Major, IntT>(loc.x(), loc.y(), size);
    }

    /** Get the 2d location corresponding to a raw buffer idx for the provded grid size (templated on major-order) */
    template<bool X_Major = false, typename IntT = int>
    inline static Eigen::Vector2<IntT> gridLoc(
        const size_t idx,
        const Eigen::Vector2<IntT>& size )
    {
        if constexpr(X_Major)
        {
            // x-major = "contiguous blocks along [parallel to] y-axis" --> x = idx / ymax, y = idx % ymax
            return Eigen::Vector2<IntT>{
                static_cast<IntT>(idx / size.y()),
                static_cast<IntT>(idx % size.y()) };
        }
        else
        {
            // y-major = "contiguous blocks along [parallel to] x-axis" --> x = idx % xmax, y = idx / xmax
            return Eigen::Vector2<IntT>{
                static_cast<IntT>(idx % size.x()),
                static_cast<IntT>(idx / size.x()) };
        }
    }

    /** Copy a 2d windows of elemens - expects element types to be POD (templated on major-order) */
    template<typename T, bool X_Major = false, typename IntT = int, size_t T_Bytes = sizeof(T)>
    static void memcpyWindow(
        T* dest,
        const T* src,
        const Eigen::Vector2<IntT>& dest_size,
        const Eigen::Vector2<IntT>& src_size,
        const Eigen::Vector2<IntT>& diff )
    {
        if constexpr(X_Major)
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
