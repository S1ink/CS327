#include "util/nc_wrap.hpp"
#include "util/math.hpp"
#include "util/grid.hpp"

#include <vector>


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
        FloatT u;
        FloatT v;
        FloatT du;
        FloatT dv;
        FloatT prev_u;
        FloatT prev_v;
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
    inline FluidSim() = default;

protected:
    void stepParticles(float dt, const Vec2f& acc);
    void pushParticles(size_t iters);
    void handleParticleCollisions();
    void updateParticleDensity();
    void transferVelocities(bool direction, float ratio);
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
    GridBase<FluidCell, IntT, FloatT> fluid_cells;
    GridBase<ParticleCell, IntT, FloatT> particle_cells;

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
            this->particle_cells.clampedCellIdxOf(p.pos.x(), p.pos.y()) ].n_particles++;
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
                this->particle_cells.clampedCellIdxOf(p.pos.x(), p.pos.y()) ].first_idx--) ] = i;

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

            Vec2i pvi = this->particle_cells.boundingLoc(p.pos.x(), p.pos.y());
            const Vec2i
                v0 = (pvi - Vec2i::Ones()).cwiseMax(Vec2i::Zero()),
                v1 = (pvi + Vec2i::Ones()).cwiseMin(this->particle_cells.size() - Vec2i::Ones());

            for(IntT y = v0.y(); y < v1.y(); y++)
            {
                for(IntT x = v0.x(); x < v1.x(); x++)
                {
                    const int64_t idx = this->particle_cells.gridIdx(x, y, this->particle_cells.size());

                    size_t beg = this->particle_cells[idx].first_idx;
                    size_t end;
                    if(i == this->particle_cells.area() - 1) end = n;
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
    const Vec2f min_bound = Vec2f::Constant(this->fluid_cells.cellRes() + this->particle_rad);
    const Vec2f max_bound =
        (this->fluid_cells.size() - Vec2i::Ones()).template cast<FloatT>() *
        Vec2f::Constant(this->fluid_cells.cellRes() - this->particle_rad);

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

    for(Particle& p : this->particles)
    {
        
    }
}


int main(int argc, char** argv)
{
    NCInitializer::use();

    NCGradient grad{{0, 0, 0}, {100, 1000, 500}};
    grad.applyBackground();

    GridBase<double> g;

    {
        int c = 0;
        do
        {
            const int wx = getmaxx(stdscr);
            const int wy = getmaxy(stdscr);

            g.resizeToBounds(Eigen::Vector2f::Zero(), Eigen::Vector2f{ wx, wy });

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
            // move(1, 0);
            // clrtoeol();
            mvprintw(1, 0, "Getch is %o", c);
            mvprintw(2, 0, "Grid alloc is 0x%lx", reinterpret_cast<uintptr_t>(g.gridData()));

            refresh();
        }
        while((c = getch()) != 03);
    }
    NCInitializer::shutdown();
}
