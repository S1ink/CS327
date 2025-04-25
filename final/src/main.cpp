#include "util/nc_wrap.hpp"
#include "util/math.hpp"
#include "util/grid.hpp"

#include <vector>


class FlipFluidSim
{
public:
    using FloatT = float;

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
        uint32_t n_particles;
        uint32_t first_idx;
    };

    struct Particle
    {
        Eigen::Vector2<FloatT> pos;
        Eigen::Vector2<FloatT> vel;

        size_t cell_id;
    };

protected:
    FloatT density;
    FloatT particle_rad;
    FloatT fcell_spacing;
    FloatT inverse_fcell_spacing;
    FloatT inverse_pcell_spacing;

    std::vector<Particle> particles;
    GridBase<FluidCell> fluid_cells;
    GridBase<ParticleCell> particle_cells;

};

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
