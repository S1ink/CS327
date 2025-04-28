#include "util/nc_wrap.hpp"
#include "util/math.hpp"
#include "util/grid.hpp"

#include <vector>
#include <type_traits>

#include <Eigen/Eigen>


class FluidSim
{
public:
    using IntT = int;
    using FloatT = float;
    using Vec2i = Eigen::Vector2<IntT>;
    using Vec2f = Eigen::Vector2<FloatT>;

    enum
    {
        CELLTYPE_FLUID = 0,
        CELLTYPE_AIR,
        CELLTYPE_SOLID
    };

    struct FluidCell
    {
        Vec2f uv;
        Vec2f duv;
        Vec2f prev_uv;
        FloatT p;
        FloatT s;
        FloatT density;

        uint8_t type;
    };

    struct ParticleCell
    {
        size_t n_particles;
        size_t first_idx;
    };

    struct Particle
    {
        Vec2f pos;
        Vec2f vel;
    };

public:
    FluidSim();
    inline ~FluidSim() = default;

protected:
    void stepParticles(float dt, const Vec2f& acc);
    void pushParticles(size_t iters);
    void handleParticleCollisions();
    void updateParticleDensity();
    template<bool ToGrid>
    void transferVelocities(float ratio);
    void solveIncompressibility(
        size_t iters,
        float dt,
        float overrxn,
        bool compensate_drift = true );

protected:
    FloatT density;
    FloatT particle_rad;
    FloatT fcell_spacing;
    FloatT inverse_fcell_spacing;
    FloatT inverse_pcell_spacing;
    FloatT particle_rest_density;

    std::vector<Particle> particles;
    // GridBase<FluidCell, IntT, FloatT> fluid_cells;
    // GridBase<ParticleCell, IntT, FloatT> particle_cells;
    std::vector<FluidCell> fluid_cells;
    std::vector<ParticleCell> particle_cells;
    AbstractGridUtil<IntT, FloatT, GridUtil::Y_MAJOR_ORDER, GridUtil::GRID_USING_VECRES> fluid_grid;
    AbstractGridUtil<IntT, FloatT, GridUtil::Y_MAJOR_ORDER, GridUtil::GRID_STANDARD> particle_grid;

};



void FluidSim::stepParticles(float dt, const Vec2f& acc)
{
    for(Particle& p : this->particles) p.pos += (p.vel += acc * dt) * dt;
}

void FluidSim::pushParticles(size_t iters)
{
    thread_local std::vector<size_t> cell_particle_ids;
    cell_particle_ids.resize(this->particles.size());

    // reset count
    for(ParticleCell& c : this->particle_cells)
    {
        c.n_particles = 0;
    }

    // count particles in each cell
    for(Particle& p : this->particles)
    {
        this->particle_cells[
            this->particle_grid.clampedCellIdx(p.pos) ].n_particles++;
    }

    // get max indices for each particle cell
    size_t n = 0;
    for(ParticleCell& c : this->particle_cells)
    {
        c.first_idx = (n += c.n_particles);
    }

    size_t i = 0;
    for(Particle& p : this->particles)
    {
        // assign indices of particles into slots within accumulated ranges for each particle cell
        cell_particle_ids[
            (this->particle_cells[
                this->particle_grid.clampedCellIdx(p.pos) ].first_idx--) ] = i;

        i++;
    }

    const float min_dist = this->particle_rad * 2.f;
    const float min_dist_2 = min_dist * min_dist;

    for(size_t iter = 0; iter < iters; iter++)
    {
        i = 0;
        for(Particle& p : this->particles)
        {
            // iterate over surrounding cells (3x3)

            Vec2i pvi = this->particle_grid.clampedAlign(p.pos);
            const Vec2i
                v0 = (pvi - Vec2i::Ones()).cwiseMax(this->particle_grid.minCell()),
                v1 = (pvi + Vec2i::Ones()).cwiseMin(this->particle_grid.maxCell());

            for(IntT y = v0.y(); y < v1.y(); y++)
            {
                for(IntT x = v0.x(); x < v1.x(); x++)
                {
                    const int64_t idx = this->particle_grid.localIdx(x, y);

                    size_t beg = this->particle_cells[idx].first_idx;
                    size_t end;
                    if(i == this->particle_grid.numCells() - 1) end = n;
                    else end = this->particle_cells[idx + 1].first_idx;

                    for( auto pi = cell_particle_ids.begin() + beg;
                        pi < cell_particle_ids.begin() + end;
                        pi++ )
                    {
                        if(*pi == i) continue;

                        Particle& p_ = this->particles[*pi];

                        Vec2f ds = p_.pos - p.pos;
                        const float d2 = ds.squaredNorm();

                        if(d2 < min_dist_2 || d2 == 0.f) continue;

                        const float d = std::sqrt(d2);
                        ds *= (0.5f * (min_dist - d) / d);

                        p.pos -= ds;
                        p_.pos += ds;
                    }
                }
            }

            i++;
        }
    }
}

void FluidSim::handleParticleCollisions()
{
    const Vec2f min_bound =
        this->fluid_grid.cellSize() + Vec2f::Constant(this->particle_rad);
    const Vec2f max_bound =
        this->fluid_grid.size() - this->fluid_grid.cellSize() - Vec2f::Constant(this->particle_rad);

    for(Particle& p : this->particles)
    {
        Vec2f& v = p.pos;

        if(v.x() < min_bound.x())
        {
            v.x() = min_bound.x();
            p.vel.x() = 0.f;
        }
        else
        if(v.x() > max_bound.x())
        {
            v.x() = max_bound.x();
            p.vel.x() = 0.f;
        }
        if(v.y() < min_bound.y())
        {
            v.y() = min_bound.y();
            p.vel.y() = 0.f;
        }
        else
        if(v.y() > max_bound.y())
        {
            v.y() = max_bound.y();
            p.vel.y() = 0.f;
        }
    }
}

void FluidSim::updateParticleDensity()
{
    for(FluidCell& f : this->fluid_cells) f.density = 0.f;

    const Vec2f& res = this->fluid_grid.cellSize();
    const Vec2f& inv_res = this->fluid_grid.invCellSize();
    const Vec2f half_res = res * 0.5;

    for(Particle& p : this->particles)
    {
        const Vec2f v = GridUtil::clamp(p.pos, res, Vec2f{ this->fluid_grid.size() - res });
        const Vec2f v_ = v - half_res;
        const Vec2i v0 = this->fluid_grid.align(v_);
        const Vec2i v1 = (v0 + Vec2i::Ones()).cwiseMin(this->fluid_grid.maxCell());
        const Vec2f t = (v_ - v0.template cast<FloatT>().cwiseProduct(res)).cwiseProduct(inv_res);
        const Vec2f s = Vec2f::Ones() - t;

        this->fluid_cells[this->fluid_grid.localIdx(v0)].density += s.prod();
        this->fluid_cells[this->fluid_grid.localIdx(v1)].density += t.prod();
        this->fluid_cells[this->fluid_grid.localIdx(v0.x(), v1.y())].density += s.x() * t.y();
        this->fluid_cells[this->fluid_grid.localIdx(v1.x(), v0.y())].density += t.x() * s.y();
    }

    if(this->particle_rest_density == 0.f)
    {
        double sum = 0.;
        size_t n_fluid_cells = 0;

        for(FluidCell& f : this->fluid_cells)
        {
            if(f.type == CELLTYPE_FLUID)
            {
                sum += static_cast<double>(f.density);
                n_fluid_cells++;
            }
        }

        if(n_fluid_cells > 0)
        {
            this->particle_rest_density = static_cast<FloatT>(sum / n_fluid_cells);
        }
    }
}

template<bool ToGrid>
void FluidSim::transferVelocities<ToGrid>(float ratio)
{
    const Vec2f& res = this->fluid_grid.cellSize();
    const Vec2f& inv_res = this->fluid_grid.invCellSize();
    const Vec2f half_res = res * 0.5;

    if constexpr(ToGrid)
    {
        for(FluidCell& f : this->fluid_cells)
        {
            f.prev_uv = f.uv;
            f.duv.setZero();
            f.uv.setZero();

            f.type = (f.s == 0.f) ? CELLTYPE_SOLID : CELLTYPE_AIR;
        }

        for(Particle& p : this->particles)
        {
            FluidCell& f = this->fluid_cells[this->fluid_grid.clampedCellIdx(p.pos)];
            if(f.type == CELLTYPE_AIR)
            {
                f.type = CELLTYPE_FLUID;
            }
        }
    }

    for(size_t w = 0; w < 2; w++)
    {
        const Vec2f& size = this->fluid_grid.size();
        const Vec2i& dim = this->fluid_grid.dim();

        const float dx = (float[]{ 0.f, inv_res.x() })[w];
        const float dy = (float[]{ inv_res.y(), 0.f })[w];

        for(Particle& p : this->particles)
        {
            const Vec2f v = GridUtil::clamp(p.pos, res, Vec2f{ size - res });

            int x0 = std::min(std::floor((v.x() - dx) * inv_res.x()), dim.x() - 2);
            float tx = ((v.x() - dx) - (x0 * res.x())) * inv_res.x();
            int x1 = std::min(x0 + 1, dim.x() - 2);

            int y0 = std::min(std::floor((v.y() - dy) * inv_res.y()), dim.y() - 2);
            float ty = ((v.y() - dy) - (y0 * res.y())) * inv_res.y();
            int y1 = std::min(y0 + 1, dim.y() - 2);

            float sx = 1.f - tx;
            float sy = 1.f - ty;

            std::array<float, 4> d{
                {
                    (sx * sy),
                    (tx * sy),
                    (tx * ty),
                    (sx * ty)
                } };

            std::array<int64_t, 4> nr{
                {
                    this->fluid_grid.localIdx(x0, y0),
                    this->fluid_grid.localIdx(x1, y0),
                    this->fluid_grid.localIdx(x1, y1),
                    this->fluid_grid.localIdx(x0, y1)
                } };

            if constexpr(ToGrid)
            {
                for(size_t i = 0; i < 4; i++)
                {
                    FluidCell& f = this->fluid_cells[nr[i]];
                    f.uv[w] += p.vel[w] * d[i];
                    f.duv[w] += d[i];
                }
            }
            else
            {
                int64_t off = (int64_t[]{ -dim.y(), 1 })[w];
                float ds = 0.;
                for(size_t i = 0; i < 4; i++)
                {
                    if( this->fluid_cells[nr[i]].type == CELLTYPE_AIR &&
                        this->fluid_cells[nr[i] + off].type == CELLTYPE_AIR )
                    {
                        d[i] = 0;
                    }

                    ds += d[i];
                }

                if(ds > 0.f)
                {
                    float pic_v = 0.f;
                    float corr = 0.f;
                    for(size_t i = 0; i < 4; i++)
                    {
                        const FluidCell& f = this->fluid_cells[nr[i]];

                        pic_v += d[i] * f.uv[w];
                        corr += d[i] * (f.uv[w] - f.prev_uv[w]);
                    }
                    pic_v /= ds;
                    corr /= ds;
                    float flip_v = p.vel[w] + corr;

                    p.vel[w] = pic_v * (1.f - ratio) + flip_v * ratio;
                }
            }
        }

        if constexpr(ToGrid)
        {
            for(FluidCell& f : this->fluid_cells)
            {
                if(f.duv[w] > 0.f)
                {
                    f.uv[w] /= f.duv[w];
                }
            }

            for(int y = 0; y < dim.y(); y++)
            {
                for(int x = 0; x < dim.x(); x++)
                {
                    int64_t i = this->fluid_grid.localIdx(x, y);
                    int64_t i2 = this->fluid_grid.localIdx(x - 1, y);
                    int64_t i3 = this->fluid_grid.localIdx(x, y + 1);

                    FluidCell& f = this->fluid_cells[i];

                    if( f.type == CELLTYPE_SOLID || ((x > 0) &&
                        this->fluid_cells[i2].type == CELLTYPE_SOLID) )
                    {
                        f.uv[0] = f.prev_uv[0];
                    }
                    if( f.type == CELLTYPE_SOLID || ((y < dim.y()) &&
                        this->fluid_cells[i3].type == CELLTYPE_SOLID) )
                    {
                        f.uv[1] = f.prev_uv[1];
                    }
                }
            }
        }
    }

}

void FluidSim::solveIncompressibility(
    size_t iters,
    float dt,
    float overrxn,
    bool compensate_drift )
{
    for(FluidCell& f : this->fluid_cells)
    {
        f.p = 0.f;
        f.prev_uv = f.uv;
    }

    const Vec2i& dim = this->fluid_grid.dim();
    Vec2f cp = this->fluid_grid.cellSize() / dt * this->density;

    for(size_t iter = 0; iter < iters; iter++)
    {
        for(int y = 1; y < dim.y() - 1; y++)
        {
            for(int x = 1; x < dim.x() - 1; x++)
            {
                FluidCell& center = this->fluid_cells[this->fluid_grid.localIdx(x, y)];
                FluidCell& left = this->fluid_cells[this->fluid_grid.localIdx(x - 1, y)];
                FluidCell& right = this->fluid_cells[this->fluid_grid.localIdx(x + 1, y)];
                FluidCell& bottom = this->fluid_cells[this->fluid_grid.localIdx(x, y + 1)];
                FluidCell& top = this->fluid_cells[this->fluid_grid.localIdx(x, y - 1)];

                if(center.type != CELLTYPE_FLUID) continue;

                float s = left.s + right.s + bottom.s + top.s;
                if(s == 0.f) continue;

                float div = right.uv[0] - center.uv[0] + top.uv[1] - center.uv[1];

                if(this->particle_rest_density > 0.f && compensate_drift)
                {
                    constexpr float k = 1.f;
                    float cmp = center.density - this->particle_rest_density;
                    if(cmp > 0.f) div = div - k * cmp;
                }

                float p = -div / s;
                p *= overrxn;
                center.p += (cp * p).norm();

                center.uv[0] -= left.s * p;
                right.uv[0] += right.s * p;
                center.uv[1] -= left.s * p;
                top.uv[1] += right.s * p;
            }
        }
    }
}



int main(int argc, char** argv)
{
    NCInitializer::use();

    NCGradient grad{{0, 0, 0}, {100, 1000, 500}};
    grad.applyBackground();

    // GridBase<double> g;

    {
        int c = 0;
        do
        {
            const int wx = getmaxx(stdscr);
            const int wy = getmaxy(stdscr);

            // g.resizeToBounds(Eigen::Vector2f::Zero(), Eigen::Vector2f{ wx, wy });

            const float mrel = static_cast<float>(wx * wy);
            for(int y = 0; y < wy; y++)
            {
                for(int x = 0; x < wx; x++)
                {
                    float rel = static_cast<float>(x * y) / mrel;
                    grad.printChar(stdscr, y, x, grad.floatToIdx(rel), ' ');
                }
            }
            refresh();

            // move(0, 0);
            // clrtoeol();
            mvprintw(0, 0, "Window size is %d x %d", wx, wy);
            mvprintw(1, 0, "Normalized window size is %d x %d", wx, wy * 19 / 9);
            // move(1, 0);
            // clrtoeol();
            mvprintw(2, 0, "Getch is %o", c);
            // mvprintw(2, 0, "Grid alloc is 0x%lx", reinterpret_cast<uintptr_t>(g.gridData()));

            refresh();
        }
        while((c = getch()) != 03);
    }
    NCInitializer::shutdown();
}
