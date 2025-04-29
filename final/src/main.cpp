#include "util/nc_wrap.hpp"
#include "util/math.hpp"
#include "util/grid.hpp"

#include <vector>
#include <fstream>
#include <type_traits>

#include <Eigen/Eigen>


static std::ofstream dbg{ "./debug.txt" };

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
        Vec2f uv{ 0.f, 0.f };
        Vec2f duv{ 0.f, 0.f };
        Vec2f prev_uv{ 0.f, 0.f };
        FloatT p{ 0.f };
        FloatT s{ 1.f };
        FloatT density{ 0.f };

        uint8_t type{ CELLTYPE_FLUID };
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
    FluidSim(
        const Vec2i& dim,
        const Vec2f& cell_size,
        FloatT particle_radius,
        FloatT density );
    inline ~FluidSim() = default;

    void resize(const Vec2i& dim);
    void simulate(
        FloatT dt,
        const Vec2f& acc,
        FloatT ratio,
        size_t n_pressure_iters,
        size_t n_particle_iters,
        FloatT over_rxn,
        bool compensate_drift );

    void render(WINDOW* w, NCGradient& grad);

protected:
    void writeBoundaryCells();

    void stepParticles(FloatT dt, const Vec2f& acc);
    void pushParticles(size_t iters);
    void handleParticleCollisions();
    void updateParticleDensity();
    template<bool ToGrid>
    void transferVelocities(FloatT ratio);
    void solveIncompressibility(
        size_t iters,
        FloatT dt,
        FloatT over_rxn,
        bool compensate_drift = true );

protected:
    FloatT density;
    FloatT particle_rad;
    FloatT particle_rest_density;

    std::vector<Particle> particles;
    std::vector<FluidCell> fluid_cells;
    std::vector<ParticleCell> particle_cells;
    AbstractGridUtil<IntT, FloatT, GridUtil::Y_MAJOR_ORDER, GridUtil::GRID_USING_VECRES> fluid_grid;
    AbstractGridUtil<IntT, FloatT, GridUtil::Y_MAJOR_ORDER, GridUtil::GRID_STANDARD> particle_grid;

};



FluidSim::FluidSim(
    const Vec2i& dim,
    const Vec2f& cell_size,
    FloatT particle_radius,
    FloatT density )
:
    density{ density },
    particle_rad{ particle_radius },
    particle_rest_density{ 0.f },
    fluid_grid{ dim, cell_size },
    particle_grid{ Vec2f{ dim.template cast<FloatT>().cwiseProduct(cell_size) }, 2.2f * particle_radius }
{
    this->particles.reserve((dim - Vec2i::Constant(2)).prod());
    for(IntT y = 1; y < dim.y() - 1; y++)
    {
        for(IntT x = 1; x < dim.x() - 1; x++)
        {
            this->particles.emplace_back(
                Particle{
                    .pos = Vec2f{ static_cast<FloatT>(x) + 0.5f, static_cast<FloatT>(y) + 0.5f }.cwiseProduct(cell_size),
                    .vel = Vec2f::Zero()
                } );

            // Vec2f& pos = this->particles.back().pos;
            // Vec2i cell = this->fluid_grid.align(pos);
            // dbg << "\t(" << pos.x() << ", " << pos.y() << ") -- ( " << cell.x() << ", " << cell.y() << " )\n";
        }
    }

    this->fluid_cells.resize(this->fluid_grid.numCells(), FluidCell{});
    // for(FluidCell& f : this->fluid_cells)
    // {
    //     f.s = 1.f;
    //     f.type = CELLTYPE_FLUID;
    // }

    this->writeBoundaryCells();

    this->particle_cells.resize(this->particle_grid.numCells());
}

void FluidSim::resize(const Vec2i& dim)
{
    std::vector<FluidCell> fcell_swap;
    fcell_swap.resize(dim.prod(), FluidCell{});

    IntT ncols = std::min(dim.x(), this->fluid_grid.dim().x());
    IntT nrows = std::min(dim.y(), this->fluid_grid.dim().y());

    for(IntT row = 1; row < nrows - 1; row++)
    {
        int64_t swap_base = GridUtil::gridIdx<GridUtil::Y_MAJOR_ORDER>(0, row, dim);
        int64_t src_base = this->fluid_grid.localIdx(0, row);

        // memcpy(
        //     fcell_swap.data() + GridUtil::gridIdx<GridUtil::Y_MAJOR_ORDER>(1, row, dim),
        //     this->fluid_cells.data() + this->fluid_grid.localIdx(1, row),
        //     (ncols - 2) * sizeof(FluidCell) );
        for(IntT col = 1; col < ncols - 1; col++)
        {
            fcell_swap[swap_base + col] = this->fluid_cells[src_base + col];
        }
    }

    this->fluid_cells.swap(fcell_swap);
    this->fluid_grid.resize(dim);
    this->particle_cells.resize(dim.prod());
    this->particle_grid.resize(dim);

    this->writeBoundaryCells();
}

void FluidSim::simulate(
    FloatT dt,
    const Vec2f& acc,
    FloatT ratio,
    size_t n_pressure_iters,
    size_t n_particle_iters,
    FloatT over_rxn,
    bool compensate_drift )
{
    constexpr size_t sub_steps = 1;
    const FloatT sdt = dt / sub_steps;

    for(size_t step = 0; step < sub_steps; step++)
    {
        dbg << "begin sim iter " << step << std::endl;
        this->stepParticles(sdt, acc);
        dbg << "stepped particles" << std::endl;
        this->pushParticles(n_particle_iters);
        dbg << "pushed particles" << std::endl;
        this->handleParticleCollisions();
        dbg << "handled particle collisions" << std::endl;
        this->transferVelocities<true>(ratio);
        dbg << "transferred velocities" << std::endl;
        this->updateParticleDensity();
        dbg << "updated particle density" << std::endl;
        this->solveIncompressibility(n_pressure_iters, sdt, over_rxn, compensate_drift);
        dbg << "solved incompressibility" << std::endl;
        this->transferVelocities<false>(ratio);
        dbg << "transferred velocities -- done with iter\n" << std::endl;
    }
}

void FluidSim::render(WINDOW* w, NCGradient& grad)
{
    const IntT mx = std::min(getmaxx(w), this->fluid_grid.dim().x());
    const IntT my = std::min(getmaxy(w), this->fluid_grid.dim().y());

    for(IntT y = 0; y < my; y++)
    {
        for(IntT x = 0; x < mx; x++)
        {
            int64_t i = this->fluid_grid.localIdx(x, y);
            const FluidCell& f = this->fluid_cells[i];
            if(f.type == CELLTYPE_SOLID)
            {
                mvwaddch(w, y, x, ' ');
            }
            else
            {
                float weight = f.density;

                grad.printChar(
                    w, y, x,
                    grad.floatToIdx(weight / 4),
                    " .+#@"[MIN_CACHED(static_cast<size_t>(weight), 4U)] );
            }
            // const auto c = COLOR_PAIR(f.type == CELLTYPE_SOLID ? COLOR_MAGENTA : COLOR_GREEN);

            // wattron(w, c);
            // mvwaddch(w, y, x, '*');
            // wattroff(w, c);
        }
    }
    wrefresh(w);

    // mvwprintw(w, 0, 0, "FLUID GRID DIM IS %d x %d", this->fluid_grid.dim().x(), this->fluid_grid.dim().y());
    // mvwprintw(w, 1, 0, "FLUID GRID SIZE IS %f x %f", this->fluid_grid.size().x(), this->fluid_grid.size().y());
    // mvwprintw(w, 2, 0, "PARTICLE GRID DIM IS %d x %d", this->particle_grid.dim().x(), this->particle_grid.dim().y());
    // mvwprintw(w, 3, 0, "PARTICLE GRID SIZE IS %f x %f", this->particle_grid.size().x(), this->particle_grid.size().y());
    // wrefresh(w);
}

void FluidSim::writeBoundaryCells()
{
    const IntT mx = this->fluid_grid.dim().x();
    const IntT my = this->fluid_grid.dim().y();

    for(IntT x = 0; x < mx; x++)
    {
        FluidCell& a = this->fluid_cells[this->fluid_grid.localIdx(x, 0)];
        FluidCell& b = this->fluid_cells[this->fluid_grid.localIdx(x, my - 1)];
        a.s = b.s = 0.f;
        a.type = b.type = CELLTYPE_SOLID;
    }
    for(IntT y = 1; y < my - 1; y++)
    {
        FluidCell& a = this->fluid_cells[this->fluid_grid.localIdx(0, y)];
        FluidCell& b = this->fluid_cells[this->fluid_grid.localIdx(mx - 1, y)];
        a.s = b.s = 0.f;
        a.type = b.type = CELLTYPE_SOLID;
    }
}

void FluidSim::stepParticles(FloatT dt, const Vec2f& acc)
{
    for(Particle& p : this->particles)
    {
        p.pos += (p.vel += (acc * dt)) * dt;

        assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));
    }
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

        assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));
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
            (--this->particle_cells[
                this->particle_grid.clampedCellIdx(p.pos) ].first_idx) ] = i;

        assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));

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

                        Vec2f pre_p = p.pos, pre_p_ = p_.pos;

                        Vec2f ds = p_.pos - p.pos;
                        const float d2 = ds.squaredNorm();

                        if(d2 > min_dist_2 || d2 == 0.f) continue;

                        const float d = std::sqrt(d2);
                        ds *= (0.5f * (min_dist - d) / d);

                        p.pos -= ds;
                        p_.pos += ds;

                        if(isnanf(p.pos.x()) || isnanf(p.pos.y()))
                        {
                            dbg << "PUSH PARTICLES ENCOUNTERED NAN:"
                                "\n\tp : (" << pre_p.x() << ", " << pre_p.y() <<
                                ")\n\tPVI: (" << pvi.x() << ", " << pvi.y() <<
                                ")\n\tv0: (" << v0.x() << ", " << v0.y() <<
                                ")\n\tv1: (" << v1.x() << ", " << v1.y() <<
                                ")\n\tpi: " << *pi <<
                                "\n\tp_: (" << pre_p_.x() << ", " << pre_p_.y() <<
                                ")\n\tds: (" << ds.x() << ", " << ds.y() <<
                                ")\n\td2: " << d2 <<
                                "\n\td: " << d << std::endl;

                            assert(false);
                        }
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

        assert(!isnanf(v.x()) && !isnanf(v.y()));
    }
}

void FluidSim::updateParticleDensity()
{
    // dbg << "\tupdating particle densities" << std::endl;
    for(FluidCell& f : this->fluid_cells) f.density = 0.f;

    const Vec2f& res = this->fluid_grid.cellSize();
    const Vec2f& inv_res = this->fluid_grid.invCellSize();
    const Vec2f half_res = res * 0.5;

    for(Particle& p : this->particles)
    {
        // dbg << "\tlooping particle at " << p.pos.x() << ", " << p.pos.y() << std::endl;

        const Vec2f v = GridUtil::clamp(p.pos, res, Vec2f{ this->fluid_grid.size() - res });
        const Vec2f v_ = v - half_res;
        const Vec2i v0 = this->fluid_grid.align(v_);
        const Vec2i v1 = (v0 + Vec2i::Ones()).cwiseMin(this->fluid_grid.maxCell());
        const Vec2f t = (v_ - v0.template cast<FloatT>().cwiseProduct(res)).cwiseProduct(inv_res);
        const Vec2f s = Vec2f::Ones() - t;

        // dbg << "v0 local idx : " << this->fluid_grid.localIdx(v0) << " (" << v0.x() << ", " << v0.y() << ")" << std::endl;

        this->fluid_cells[this->fluid_grid.localIdx(v0)].density += s.prod();
        this->fluid_cells[this->fluid_grid.localIdx(v1)].density += t.prod();
        this->fluid_cells[this->fluid_grid.localIdx(v0.x(), v1.y())].density += s.x() * t.y();
        this->fluid_cells[this->fluid_grid.localIdx(v1.x(), v0.y())].density += t.x() * s.y();

        assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));
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
void FluidSim::transferVelocities(FloatT ratio)
{
    const Vec2f& res = this->fluid_grid.cellSize();
    const Vec2f& inv_res = this->fluid_grid.invCellSize();
    const Vec2f half_res = res * 0.5;

    // dbg << "\ttransfer velocities <" << ToGrid << "> initializing" << std::endl;

    if constexpr(ToGrid)
    {
        for(FluidCell& f : this->fluid_cells)
        {
            f.prev_uv = f.uv;
            f.duv.setZero();
            f.uv.setZero();

            f.type = (f.s == 0.f) ? CELLTYPE_SOLID : CELLTYPE_AIR;
        }

        // dbg << "\ttogrid -- iterated fluid cells" << std::endl;

        for(Particle& p : this->particles)
        {
            FluidCell& f = this->fluid_cells[this->fluid_grid.clampedCellIdx(p.pos)];
            if(f.type == CELLTYPE_AIR)
            {
                f.type = CELLTYPE_FLUID;
            }

            assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));
        }

        // dbg << "\ttogrid -- iterated particles" << std::endl;
    }

    for(size_t w = 0; w < 2; w++)
    {
        // dbg << "\tbeginning iter " << w << std::endl;

        const Vec2f& size = this->fluid_grid.size();
        const Vec2i& dim = this->fluid_grid.dim();

        const float dx = w ? 0.f : half_res.x();
        const float dy = w ? half_res.y() : 0.f;

        for(Particle& p : this->particles)
        {
            assert(!isnanf(p.pos.x()) && !isnanf(p.pos.y()));

            const Vec2f v = GridUtil::clamp(p.pos, res, Vec2f{ size - res });

            // dbg << "particle (" << p.pos.x() << ", " << p.pos.y() << ") --> (" << v.x() << ", " << v.y() << ")" << std::endl;

            int x0 = std::min(static_cast<int>(std::floor((v.x() - dx) * inv_res.x())), dim.x() - 2);
            float tx = ((v.x() - dx) - (x0 * res.x())) * inv_res.x();
            int x1 = std::min(x0 + 1, dim.x() - 2);

            int y0 = std::min(static_cast<int>(std::floor((v.y() - dy) * inv_res.y())), dim.y() - 2);
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
                int64_t off = w ? -dim.y() : 1;
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

        // dbg << "\titerated particles" << std::endl;

        if constexpr(ToGrid)
        {
            for(FluidCell& f : this->fluid_cells)
            {
                if(f.duv[w] > 0.f)
                {
                    f.uv[w] /= f.duv[w];
                }
            }

            // dbg << "\ttogrid -- iterated cells" << std::endl;

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

            // dbg << "\ttogrid -- iterated cells again" << std::endl;
        }
    }

}

void FluidSim::solveIncompressibility(
    size_t iters,
    FloatT dt,
    FloatT overrxn,
    bool compensate_drift )
{
    for(FluidCell& f : this->fluid_cells)
    {
        f.p = 0.f;
        f.prev_uv = f.uv;
    }

    const Vec2i& dim = this->fluid_grid.dim();
    FloatT h = std::sqrt(this->fluid_grid.cellSize().prod());
    FloatT cp = h / dt * this->density;

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
                center.p += cp * p;

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

    int wx, wy;
    FluidSim f{
        Eigen::Vector2i{ wx = getmaxx(stdscr), wy = getmaxy(stdscr) },
        Eigen::Vector2f{ 0.9f, 1.9f },
        0.1f,
        10.f };

    {
        // f.render(stdscr, grad);
        int c = 0;
        do
        {
            const int wx_ = getmaxx(stdscr);
            const int wy_ = getmaxy(stdscr);

            if(wx_ != wx || wy_ != wy)
            {
                wx = wx_;
                wy = wy_;
                f.resize(Eigen::Vector2i{ wx, wy });
            }

            f.simulate(0.2f, Eigen::Vector2f{ 0.f, 9.8f }, 0.5f, 100, 20, 1.9, true);
            f.render(stdscr, grad);
        }
        while((c = getch()) != 03);
    }
    NCInitializer::shutdown();
}
