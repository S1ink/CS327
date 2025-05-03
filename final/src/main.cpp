#include <condition_variable>
#include <algorithm>
// #include <fstream>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>
#include <cmath>
#include <mutex>

#include <signal.h>
#include <unistd.h>
#include <sys/select.h>

#include <ncurses.h>

#include "util/nc_wrap.hpp"
#include "util/stats.hpp"


#ifndef USE_OMP
#define USE_OMP 0
#endif

#if USE_OMP
#include <omp.h>
#endif

// static std::ofstream dbg{ "./debug.txt" };

/** Core functionality converted from
 * https://github.com/matthias-research/pages/blob/master/tenMinutePhysics/18-flip.html */
class FlipFluid
{
public:
    enum CellType
    {
        FLUID_CELL = 0,
        AIR_CELL = 1,
        SOLID_CELL = 2
    };

public:
    FlipFluid(
        int inner_x_dim,
        int inner_y_dim,
        float cell_res,
        float particle_radius,
        float fill_percent,
        float density );
    inline ~FlipFluid() = default;

    void resize(
        int inner_x_dim,
        int inner_y_dim,
        float cell_res = 1.f );

    template<NCURSES_COLOR_T GNum, NCURSES_COLOR_T Off>
    void render(WINDOW* w, const NCGradient_<GNum, Off>& grad);
    template<NCURSES_COLOR_T GNum, NCURSES_COLOR_T Off>
    void overlay(WINDOW* w, int y, int x, const char* str, const NCGradient_<GNum, Off>& grad);

    template<bool Separate_Particles = true>
    void simulate(
        float dt,
        float x_acc,
        float y_acc,
        float flipRatio,
        int numPressureIters,
        int numParticleIters,
        float overRelaxation,
        bool compensateDrift );

protected:
    FlipFluid() = default;

    void initializeFluidCells(
        float tank_width,
        float tank_height,
        float cell_res );
    void initializeParticleCells(
        float tank_width,
        float tank_height,
        float particle_radius );

    void integrateParticles(float dt, float x_acc, float y_acc);
    void pushParticlesApart(int numIters);
    void handleParticleCollisions();
    void updateParticleDensity();
    void transferVelocities(bool toGrid, float flipRatio);
    void solveIncompressibility(
        int numIters,
        float dt,
        float overRelaxation,
        bool compensateDrift = true );

protected:
    // Fluid parameters
    float density;
    int fNumX, fNumY, fNumCells;
    float h, fInvSpacing;
    std::vector<float> u, v, du, dv, prevU, prevV, p, s;
    std::vector<int> cellType;

    // Particle parameters
    int maxParticles;
    int numParticles = 0;
    float particleRadius, pInvSpacing;
    int pNumX, pNumY, pNumCells;
    float particleRestDensity = 0.0f;
    std::vector<float> particlePos, particleVel, particleDensity;
    std::vector<int> numCellParticles, firstCellParticle, cellParticleIds;

private:
    inline float clamp(float x, float minVal, float maxVal)
    {
        return std::max(minVal, std::min(x, maxVal));
    }

};



FlipFluid::FlipFluid(
    int inner_x_dim,
    int inner_y_dim,
    float cell_res,
    float particle_radius,
    float fill_percent,
    float density )
:
    density{ density },
    particleRadius{ particle_radius }
{
    const float tank_width = static_cast<float>(inner_x_dim + 2);
    const float tank_height = static_cast<float>(inner_y_dim + 2);

    this->initializeFluidCells(tank_width, tank_height, cell_res);

    const float dx = 2.f * particle_radius;
    const float dy = std::sqrt(3.f) / 2.f * dx;
    const int x_num_particles =
        static_cast<int>(std::floor(
            (fill_percent * tank_width - 2.f * h - 1.f * particle_radius) / dx) );
    const int y_num_particles =
        static_cast<int>(std::floor(
            (tank_height - 2.f * h - 1.f * particle_radius) / dy) );

    this->maxParticles = this->numParticles = x_num_particles * y_num_particles;
    this->particlePos.resize(2 * this->maxParticles, 0);
    this->particleVel.resize(2 * this->maxParticles, 0);

    int p = 0;
    for(int i = 0; i < x_num_particles; i++)
    {
        for(int j = 0; j < y_num_particles; j++)
        {
            this->particlePos[p++] = h + particle_radius + dx * i + (j % 2 == 0 ? 0.f : particle_radius);
            this->particlePos[p++] = h + particle_radius + dy * j;
        }
    }

    this->initializeParticleCells(tank_width, tank_height, particle_radius);

    #if USE_OMP
    int tt = std::thread::hardware_concurrency() - 1;
    if(tt > 1)
    {
        omp_set_num_threads(tt);
    }
    #endif
}


void FlipFluid::initializeFluidCells(
    float tank_width,
    float tank_height,
    float cell_res )
{
    this->fNumX = static_cast<int>(std::ceil(tank_width / cell_res));
    this->fNumY = static_cast<int>(std::ceil(tank_height / cell_res));
    this->fNumCells = fNumX * fNumY;
    this->h = std::max(
        tank_width / this->fNumX,
        tank_height / this->fNumY );
    this->fInvSpacing = 1.0f / h;

    this->u.resize(this->fNumCells, 0);
    this->v.resize(this->fNumCells, 0);
    this->du.resize(this->fNumCells, 0);
    this->dv.resize(this->fNumCells, 0);
    this->prevU.resize(this->fNumCells, 0);
    this->prevV.resize(this->fNumCells, 0);
    this->p.resize(this->fNumCells, 0);
    this->s.resize(this->fNumCells, 1);
    this->cellType.resize(this->fNumCells, FLUID_CELL);
    this->particleDensity.resize(this->fNumCells, 0);

    for(int x = 0; x < this->fNumX; x++)
    {
        this->s[x * this->fNumY + 0] = 0.f;
        this->s[(x + 1) * this->fNumY - 1] = 0.f;
    }
    for(int y = 1; y < this->fNumY - 1; y++)
    {
        this->s[y] = 0.f;
        this->s[(this->fNumX - 1) * this->fNumY + y] = 0.f;
    }
}

void FlipFluid::initializeParticleCells(
    float tank_width,
    float tank_height,
    float particle_radius )
{
    this->pInvSpacing = 1.0f / (2.2f * particle_radius);
    this->pNumX = static_cast<int>(std::ceil(tank_width * this->pInvSpacing));
    this->pNumY = static_cast<int>(std::ceil(tank_height * this->pInvSpacing));
    this->pNumCells = this->pNumX * this->pNumY;

    this->numCellParticles.resize(pNumCells);
    this->firstCellParticle.resize(pNumCells + 1);
    this->cellParticleIds.resize(maxParticles);
}



void FlipFluid::resize(
    int inner_x_dim,
    int inner_y_dim,
    float cell_res )
{
    const float tank_width = static_cast<float>(inner_x_dim + 2);
    const float tank_height = static_cast<float>(inner_y_dim + 2);

    FlipFluid f_swap{};
    f_swap.initializeFluidCells(tank_width, tank_height, cell_res);

    const int mx = std::min(fNumX, f_swap.fNumX) - 1;
    const int my = std::min(fNumY, f_swap.fNumY) - 1;
    const int y_diff = f_swap.fNumY - fNumY;
    const int src_y_off = std::max(-y_diff, 0);
    const int dest_y_off = std::max(y_diff, 0);

    // copy fluid cells in range
    for(int x = 1; x < mx; x++)
    {
        int i_src = x * fNumY + src_y_off;
        int i_dest = x * f_swap.fNumY + dest_y_off;

        for(int y = 1; y < my; y++)
        {
            i_src++;
            i_dest++;

            f_swap.u[i_dest] = u[i_src];
            f_swap.v[i_dest] = v[i_src];
            f_swap.du[i_dest] = du[i_src];
            f_swap.dv[i_dest] = dv[i_src];
            f_swap.prevU[i_dest] = prevU[i_src];
            f_swap.prevV[i_dest] = prevV[i_src];
            f_swap.s[i_dest] = s[i_src];
            f_swap.p[i_dest] = p[i_src];
            f_swap.cellType[i_dest] = cellType[i_src];   // <-- ideally fill all with AIR_CELL by default
        }
    }

    fNumX = f_swap.fNumX;
    fNumY = f_swap.fNumY;
    fNumCells = f_swap.fNumCells;
    h = f_swap.h;
    fInvSpacing = f_swap.fInvSpacing;

    u.swap(f_swap.u);
    v.swap(f_swap.v);
    du.swap(f_swap.du);
    dv.swap(f_swap.dv);
    prevU.swap(f_swap.prevU);
    prevV.swap(f_swap.prevV);
    p.swap(f_swap.p);
    s.swap(f_swap.s);
    cellType.swap(f_swap.cellType);
    particleDensity.swap(f_swap.particleDensity);

    const float particle_y_off = static_cast<float>(y_diff) * h;
    for(int i = 0; i < numParticles; i++)
    {
        particlePos[i * 2 + 1] += particle_y_off;
    }

    this->initializeParticleCells(tank_width, tank_height, this->particleRadius);
}

template<NCURSES_COLOR_T GNum, NCURSES_COLOR_T Off>
void FlipFluid::render(WINDOW* w, const NCGradient_<GNum, Off>& grad)
{
    const int wx = getmaxx(w);
    const int wy = getmaxy(w);
    const int mx = std::min(wx, fNumX - 2);
    const int my = std::min(wy, (fNumY - 2) / 2);
    // const int y_diff =  wy - ((fNumY - 2) / 2);
    // const int src_y_off = std::max(-y_diff, 0);
    // const int dest_y_off = std::max(y_diff, 0);

    for(int x = 0; x < mx; x++)
    {
        int i0 = (x + 1) * fNumY + (((my - 1) * 2 + 1)); // + (src_y_off * 2);
        int i1 = (x + 1) * fNumY + (((my - 1) * 2 + 1) + 1); // + (src_y_off * 2);

        for(int y = 0; y < my; y++)
        {
            if( (cellType[i0] == SOLID_CELL && cellType[i1] == SOLID_CELL) ||
                (cellType[i0] == AIR_CELL && cellType[i1] == AIR_CELL) )
            {
                attron(COLOR_PAIR(COLOR_BLACK));
                mvwaddch(w, y, x, ' ');
                attron(COLOR_PAIR(COLOR_BLACK));
            }
            else
            {
                float density = (particleDensity[i0] + particleDensity[i1]) / (particleRestDensity * 2);

                grad.printChar(
                    w, y, x,
                    grad.floatToIdx(density),
                    " .,:+#@"[std::min<size_t>(density * 6, 6U)] );
            }

            i0 -= 2;
            i1 -= 2;
        }
    }
    // wrefresh(w);
}

template<NCURSES_COLOR_T GNum, NCURSES_COLOR_T Off>
void FlipFluid::overlay(
    WINDOW* w,
    int y,
    int x,
    const char* str,
    const NCGradient_<GNum, Off>& grad )
{
    const int my = (fNumY - 2) / 2;
    if(y < my)
    {
        int i0 = (x + 1) * fNumY + (((my - 1 - y) * 2 + 1));
        int i1 = (x + 1) * fNumY + (((my - 1 - y) * 2 + 1) + 1);

        for(const char* c = str; *c && x < (fNumX - 2); c++)
        {
            if( (cellType[i0] == SOLID_CELL && cellType[i1] == SOLID_CELL) ||
                (cellType[i0] == AIR_CELL && cellType[i1] == AIR_CELL) )
            {
                attron(COLOR_PAIR(COLOR_BLACK));
                mvwaddch(w, y, x, *c);
                attron(COLOR_PAIR(COLOR_BLACK));
            }
            else
            {
                float density = (particleDensity[i0] + particleDensity[i1]) / (particleRestDensity * 2);

                grad.printChar(
                    w, y, x,
                    grad.floatToIdx(density),
                    *c );
            }

            x++;
            i0 += fNumY;
            i1 += fNumY;
        }
    }
}






template<bool Separate_Particles>
void FlipFluid::simulate(
    float dt,
    float x_acc,
    float y_acc,
    float flipRatio,
    int numPressureIters,
    int numParticleIters,
    float overRelaxation,
    bool compensateDrift )
{
    constexpr int numSubSteps = 1;
    float sdt = dt / numSubSteps;

    for(int step = 0; step < numSubSteps; step++)
    {
        this->integrateParticles(sdt, x_acc, y_acc);
        if constexpr(Separate_Particles)
        {
            this->pushParticlesApart(numParticleIters);
        }
        this->handleParticleCollisions();
        this->transferVelocities(true, flipRatio);
        this->updateParticleDensity();
        this->solveIncompressibility(numPressureIters, sdt, overRelaxation, compensateDrift);
        this->transferVelocities(false, flipRatio);
    }
}

void FlipFluid::integrateParticles(float dt, float x_acc, float y_acc)
{
    #if USE_OMP
    #pragma omp parallel for
    #endif
    for(int i = 0; i < numParticles; ++i)
    {
        particleVel[2 * i] += dt * x_acc;
        particleVel[2 * i + 1] += dt * y_acc;
        particlePos[2 * i] += particleVel[2 * i] * dt;
        particlePos[2 * i + 1] += particleVel[2 * i + 1] * dt;
    }
}

void FlipFluid::pushParticlesApart(int numIters)
{
    std::fill(numCellParticles.begin(), numCellParticles.end(), 0);

    for(int i = 0; i < numParticles; ++i)
    {
        float x = particlePos[2 * i];
        float y = particlePos[2 * i + 1];
        int xi = static_cast<int>(clamp(std::floor(x * pInvSpacing), 0.0f, static_cast<float>(pNumX - 1)));
        int yi = static_cast<int>(clamp(std::floor(y * pInvSpacing), 0.0f, static_cast<float>(pNumY - 1)));
        int cellNr = xi * pNumY + yi;
        numCellParticles[cellNr]++;
    }

    int first = 0;
    for(int i = 0; i < pNumCells; ++i)
    {
        first += numCellParticles[i];
        firstCellParticle[i] = first;
    }
    firstCellParticle[pNumCells] = first;

    for(int i = 0; i < numParticles; ++i)
    {
        float x = particlePos[2 * i];
        float y = particlePos[2 * i + 1];
        int xi = static_cast<int>(clamp(std::floor(x * pInvSpacing), 0.0f, static_cast<float>(pNumX - 1)));
        int yi = static_cast<int>(clamp(std::floor(y * pInvSpacing), 0.0f, static_cast<float>(pNumY - 1)));
        int cellNr = xi * pNumY + yi;
        firstCellParticle[cellNr]--;
        cellParticleIds[firstCellParticle[cellNr]] = i;
    }

    float minDist = 2.0f * particleRadius;
    float minDist2 = minDist * minDist;

    for(int iter = 0; iter < numIters; ++iter)
    {
        for(int i = 0; i < numParticles; ++i)
        {
            float px = particlePos[2 * i];
            float py = particlePos[2 * i + 1];
            int pxi = static_cast<int>(px * pInvSpacing);
            int pyi = static_cast<int>(py * pInvSpacing);
            int x0 = std::max(pxi - 1, 0);
            int y0 = std::max(pyi - 1, 0);
            int x1 = std::min(pxi + 1, pNumX - 1);
            int y1 = std::min(pyi + 1, pNumY - 1);

            for(int xi = x0; xi <= x1; ++xi)
            {
                for(int yi = y0; yi <= y1; ++yi)
                {
                    int cellNr = xi * pNumY + yi;
                    int first = firstCellParticle[cellNr];
                    int last = firstCellParticle[cellNr + 1];

                    for(int j = first; j < last; ++j)
                    {
                        int id = cellParticleIds[j];
                        if(id == i) continue;

                        float qx = particlePos[2 * id];
                        float qy = particlePos[2 * id + 1];
                        float dx = qx - px;
                        float dy = qy - py;
                        float d2 = dx * dx + dy * dy;
                        if(d2 > minDist2 || d2 == 0.0f) continue;

                        float d = std::sqrt(d2);
                        float s = 0.5f * (minDist - d) / d;
                        dx *= s;
                        dy *= s;

                        particlePos[2 * i] -= dx;
                        particlePos[2 * i + 1] -= dy;
                        particlePos[2 * id] += dx;
                        particlePos[2 * id + 1] += dy;
                    }
                }
            }
        }
    }
}

void FlipFluid::handleParticleCollisions()
{
    float hLocal = 1.0f / fInvSpacing;
    float r = particleRadius;

    float minX = hLocal + r;
    float maxX = (fNumX - 1) * hLocal - r;
    float minY = hLocal + r;
    float maxY = (fNumY - 1) * hLocal - r;

    #if USE_OMP
    #pragma omp parallel for
    #endif
    for(int i = 0; i < numParticles; ++i)
    {
        float& x = particlePos[2 * i];
        float& y = particlePos[2 * i + 1];

        if(x < minX)
        {
            x = minX;
            particleVel[2 * i] = 0.0f;
        }
        if(x > maxX)
        {
            x = maxX;
            particleVel[2 * i] = 0.0f;
        }
        if(y < minY)
        {
            y = minY;
            particleVel[2 * i + 1] = 0.0f;
        }
        if(y > maxY)
        {
            y = maxY;
            particleVel[2 * i + 1] = 0.0f;
        }
    }
}

void FlipFluid::updateParticleDensity()
{
    int n = fNumY;
    float h2 = 0.5f * h;
    float h1 = fInvSpacing;

    std::fill(particleDensity.begin(), particleDensity.end(), 0.0f);

    for(int i = 0; i < numParticles; ++i)
    {
        float x = clamp(particlePos[2 * i], h, (fNumX - 1) * h);
        float y = clamp(particlePos[2 * i + 1], h, (fNumY - 1) * h);

        int x0 = static_cast<int>((x - h2) * h1);
        float tx = ((x - h2) - x0 * h) * h1;
        int x1 = std::min(x0 + 1, fNumX - 1);

        int y0 = static_cast<int>((y - h2) * h1);
        float ty = ((y - h2) - y0 * h) * h1;
        int y1 = std::min(y0 + 1, fNumY - 1);

        float sx = 1.0f - tx;
        float sy = 1.0f - ty;

        if(x0 < fNumX && y0 < fNumY) particleDensity[x0 * n + y0] += sx * sy;
        if(x1 < fNumX && y0 < fNumY) particleDensity[x1 * n + y0] += tx * sy;
        if(x1 < fNumX && y1 < fNumY) particleDensity[x1 * n + y1] += tx * ty;
        if(x0 < fNumX && y1 < fNumY) particleDensity[x0 * n + y1] += sx * ty;
    }

    if(particleRestDensity == 0.0f)
    {
        float sum = 0.0f;
        int numFluidCells = 0;
        for(int i = 0; i < fNumCells; ++i)
        {
            if(cellType[i] == FLUID_CELL)
            {
                sum += particleDensity[i];
                ++numFluidCells;
            }
        }

        if(numFluidCells > 0)
        {
            particleRestDensity = sum / numFluidCells;
        }
    }
}

void FlipFluid::transferVelocities(bool toGrid, float flipRatio)
{
    int n = fNumY;
    float h1 = fInvSpacing;
    float h2 = 0.5f * h;

    if(toGrid)
    {
        prevU = u;
        prevV = v;
        std::fill(du.begin(), du.end(), 0.0f);
        std::fill(dv.begin(), dv.end(), 0.0f);
        std::fill(u.begin(), u.end(), 0.0f);
        std::fill(v.begin(), v.end(), 0.0f);

        for(int i = 0; i < fNumCells; ++i)
        {
            cellType[i] = (s[i] == 0.0f) ? SOLID_CELL : AIR_CELL;
        }

        for(int i = 0; i < numParticles; ++i)
        {
            int xi = static_cast<int>(clamp(std::floor(particlePos[2 * i] * h1), 0.0f, static_cast<float>(fNumX - 1)));
            int yi = static_cast<int>(clamp(std::floor(particlePos[2 * i + 1] * h1), 0.0f, static_cast<float>(fNumY - 1)));
            int cellNr = xi * n + yi;
            if(cellType[cellNr] == AIR_CELL)
            {
                cellType[cellNr] = FLUID_CELL;
            }
        }
    }

    for(int component = 0; component < 2; ++component)
    {
        float dx = (component == 0) ? 0.0f : h2;
        float dy = (component == 0) ? h2 : 0.0f;

        std::vector<float>& f = (component == 0) ? u : v;
        std::vector<float>& prevF = (component == 0) ? prevU : prevV;
        std::vector<float>& d = (component == 0) ? du : dv;

        for(int i = 0; i < numParticles; ++i)
        {
            float x = clamp(particlePos[2 * i], h, (fNumX - 1) * h);
            float y = clamp(particlePos[2 * i + 1], h, (fNumY - 1) * h);

            int x0 = std::min(static_cast<int>((x - dx) * h1), fNumX - 2);
            float tx = ((x - dx) - x0 * h) * h1;
            int x1 = std::min(x0 + 1, fNumX - 2);

            int y0 = std::min(static_cast<int>((y - dy) * h1), fNumY - 2);
            float ty = ((y - dy) - y0 * h) * h1;
            int y1 = std::min(y0 + 1, fNumY - 2);

            float sx = 1.0f - tx;
            float sy = 1.0f - ty;

            float d0 = sx * sy;
            float d1 = tx * sy;
            float d2 = tx * ty;
            float d3 = sx * ty;

            int nr0 = x0 * n + y0;
            int nr1 = x1 * n + y0;
            int nr2 = x1 * n + y1;
            int nr3 = x0 * n + y1;

            if(toGrid)
            {
                float pv = particleVel[2 * i + component];
                f[nr0] += pv * d0;  d[nr0] += d0;
                f[nr1] += pv * d1;  d[nr1] += d1;
                f[nr2] += pv * d2;  d[nr2] += d2;
                f[nr3] += pv * d3;  d[nr3] += d3;
            }
            else
            {
                int offset = (component == 0) ? n : 1;
                float valid0 = (cellType[nr0] != AIR_CELL || cellType[nr0 - offset] != AIR_CELL) ? 1.0f : 0.0f;
                float valid1 = (cellType[nr1] != AIR_CELL || cellType[nr1 - offset] != AIR_CELL) ? 1.0f : 0.0f;
                float valid2 = (cellType[nr2] != AIR_CELL || cellType[nr2 - offset] != AIR_CELL) ? 1.0f : 0.0f;
                float valid3 = (cellType[nr3] != AIR_CELL || cellType[nr3 - offset] != AIR_CELL) ? 1.0f : 0.0f;

                float v0 = particleVel[2 * i + component];
                float dSum = valid0 * d0 + valid1 * d1 + valid2 * d2 + valid3 * d3;

                if(dSum > 0.0f)
                {
                    float picV = (valid0 * d0 * f[nr0] + valid1 * d1 * f[nr1] +
                                  valid2 * d2 * f[nr2] + valid3 * d3 * f[nr3]) / dSum;
                    float corr = (valid0 * d0 * (f[nr0] - prevF[nr0]) + valid1 * d1 * (f[nr1] - prevF[nr1]) +
                                  valid2 * d2 * (f[nr2] - prevF[nr2]) + valid3 * d3 * (f[nr3] - prevF[nr3])) / dSum;
                    float flipV = v0 + corr;
                    particleVel[2 * i + component] = (1.0f - flipRatio) * picV + flipRatio * flipV;
                }
            }
        }

        if(toGrid)
        {
            for(size_t i = 0; i < f.size(); ++i)
            {
                if(d[i] > 0.0f)
                {
                    f[i] /= d[i];
                }
            }

            for(int i = 0; i < fNumX; ++i)
            {
                for(int j = 0; j < fNumY; ++j)
                {
                    bool solid = (cellType[i * n + j] == SOLID_CELL);
                    if(solid || (i > 0 && cellType[(i - 1) * n + j] == SOLID_CELL))
                    {
                        u[i * n + j] = prevU[i * n + j];
                    }
                    if(solid || (j > 0 && cellType[i * n + j - 1] == SOLID_CELL))
                    {
                        v[i * n + j] = prevV[i * n + j];
                    }
                }
            }
        }
    }
}

void FlipFluid::solveIncompressibility(
    int numIters,
    float dt,
    float overRelaxation,
    bool compensateDrift )
{
    std::fill(p.begin(), p.end(), 0.0f);
    prevU = u;
    prevV = v;

    int n = fNumY;
    float cp = density * h / dt;

    for(int iter = 0; iter < numIters; ++iter)
    {
        for(int i = 1; i < fNumX - 1; ++i)
        {
            for(int j = 1; j < fNumY - 1; ++j)
            {
                int center = i * n + j;
                if (cellType[center] != FLUID_CELL) continue;

                int left = (i - 1) * n + j;
                int right = (i + 1) * n + j;
                int bottom = i * n + j - 1;
                int top = i * n + j + 1;

                float sx0 = s[left];
                float sx1 = s[right];
                float sy0 = s[bottom];
                float sy1 = s[top];

                float sumS = sx0 + sx1 + sy0 + sy1;
                if(sumS == 0.0f) continue;

                float div = u[right] - u[center] + v[top] - v[center];

                if(particleRestDensity > 0.0f && compensateDrift)
                {
                    float compression = particleDensity[center] - particleRestDensity;
                    if(compression > 0.0f)
                    {
                        div -= compression;
                    }
                }

                float pressure = -div / sumS * overRelaxation;
                p[center] += cp * pressure;

                u[center] -= sx0 * pressure;
                u[right] += sx1 * pressure;
                v[center] -= sy0 * pressure;
                v[top] += sy1 * pressure;
            }
        }
    }
}










constexpr char const HELP_MESSAGE[][58] =
{
    "+-CONTROLS----------------------------------------------+",
    "| Arrow Keys    : Hold to apply acceleration            |",
    "| SPACE or \'p\'  : Play/pause the simulation             |",
    "| \'h\' or \'?\'    : Show/hide this help message           |",
    "| \'s\'           : Show/hide statistics                  |",
    "| \'S\'           : Show/hide advanced statistics         |",
    "| \'l\'           : Toggle light/dark mode (for weirdos)  |",
    "| Ctrl+C or \'q\' : Quit                                  |",
    "|                                                       |",
    "+-USAGE-------------------------------------------------+",
    "| * The fluid is simluated \"free-floating\" by default.  |",
    "| * Directional acceleration can be applied by pressing |",
    "|   (and holding) the arrow keys.                       |",
    "| * The window can be resized to make the fluid         |",
    "|   interact with the updated boundaries.               |",
    "+-------------------------------------------------------+"
};
constexpr int HELP_MESSAGE_COLS = sizeof(*HELP_MESSAGE) / sizeof(**HELP_MESSAGE) - 1;
constexpr int HELP_MESSAGE_ROWS = sizeof(HELP_MESSAGE) / (sizeof(*HELP_MESSAGE) / sizeof(**HELP_MESSAGE));

int main(int argc, char** argv)
{
    using clock_t = std::chrono::system_clock;

    NCInitializer::use();
    attron(A_BOLD);

    // {500, 100, 800}, {100, 1000, 500}
    // {700, 0, 600}, {200, 1000, 400}
    // {400, 100, 1000}, {400, 1000, 100}
    // {100, 800, 400}, {100, 200, 800}
    NCGradient fluid_grad{ {100, 200, 800}, {100, 800, 400} };
    NCGradient64<64> overlay_grad{ {60, 120, 480}, {60, 480, 240} };
    NCGradient32<32> overlay_dark_grad{ {30, 60, 240}, {30, 240, 120} };
    fluid_grad.applyForeground(COLOR_BLACK);
    overlay_grad.applyBackground();
    overlay_dark_grad.applyBackground();

    int wx = getmaxx(stdscr);
    int wy = getmaxy(stdscr);

    constexpr float SIM_REALTIME_FACTOR = 10.f;
    constexpr float SIM_DEFAULT_DT = 0.05f;
    constexpr float MAX_SIM_DT = 0.25f;
    constexpr float MIN_UI_DT = 0.01f;
    constexpr float MAX_UI_DT = 0.05f;
    constexpr float FLIP_RATIO = 0.5;
    constexpr float PARTICLE_RADIUS = 0.3f;
    constexpr float FILL_PERCENT = 0.3f;
    constexpr int NUM_PRESSURE_ITERS = 50;
    constexpr int NUM_PARTICLE_ITERS = 2;
    constexpr float OVER_RELAXATION_FACTOR = 1.9;
    constexpr bool USE_COMPENSATE_DRIFT = true;
    constexpr bool USE_SEPARATE_PARTICLES = true;
    constexpr float DEFAULT_DENSITY = 1000.f;

    FlipFluid f{ wx, wy * 2, 1.f, PARTICLE_RADIUS, FILL_PERCENT, DEFAULT_DENSITY };

    std::atomic<bool> running = true;
    std::atomic<bool> sim_can_continue = true;
    std::condition_variable sim_notifier;
    std::mutex sim_mtx, tmp_mtx;

    std::atomic<float> x_acc = 0.f;
    std::atomic<float> y_acc = 0.f;
    std::atomic<float> sim_dt = SIM_DEFAULT_DT * SIM_REALTIME_FACTOR;

    std::thread sim_thread{
        [&]()
        {
            while(running)
            {
                clock_t::time_point beg = clock_t::now();
                sim_mtx.lock();
                f.simulate<USE_SEPARATE_PARTICLES>(
                    std::min(sim_dt.load(), MAX_SIM_DT),
                    x_acc,
                    y_acc,
                    FLIP_RATIO,
                    NUM_PRESSURE_ITERS,
                    NUM_PARTICLE_ITERS,
                    OVER_RELAXATION_FACTOR,
                    USE_COMPENSATE_DRIFT );
                sim_mtx.unlock();
                sim_dt = std::chrono::duration<float>(clock_t::now() - beg).count() * SIM_REALTIME_FACTOR;

                if(!sim_can_continue)
                {
                    // wait for signal to continue
                    std::unique_lock<std::mutex> l{ tmp_mtx };
                    while(running && !sim_can_continue)
                    {
                        sim_notifier.wait(l);
                    }
                    sim_can_continue = true;
                }
            }
        } };

    fd_set set;
    struct timeval tv;

    struct
    {
        uint8_t paused : 1;
        uint8_t show_stats : 2;
        uint8_t resized : 1;
        uint8_t show_usage : 1;
        uint8_t is_reversed : 1;
        uint8_t is_random : 1;
    }
    state;
    state.paused = state.resized = state.is_reversed = state.is_random = 0;
    state.show_stats = state.show_usage = 1;

    struct
    {
        std::mt19937 twister{ std::random_device{}() };
        std::uniform_int_distribution<int> i_gen{ 0, 3 };
        std::uniform_real_distribution<float> f_gen{ -0.05f, 0.05f };
    }
    rando;


    util::proc::ProcessMetrics process_metrics;
    clock_t::time_point ui_ts = clock_t::now();
    float ui_dt = 0.f;
    while(running)
    {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        tv.tv_sec = 0;
        tv.tv_usec = static_cast<suseconds_t>(MAX_UI_DT * 1e6f);
        bool acc_updated = false;

        int x = select(1, &set, NULL, NULL, &tv);
        sim_can_continue = false;   // signal sim thread to wait for render after next iteration

        if(x)
        {
            int c;
            nodelay(stdscr, true);
            while((c = getch()) != ERR)
            {
                switch(c)
                {
                    case 03 :
                    case 'q' :
                    {
                        running = false;
                        continue;
                    }
                    case KEY_UP :
                    {
                        if(!state.paused)
                        {
                            x_acc = 0.f;
                            y_acc = 10.f;
                            acc_updated = true;
                        }
                        break;
                    }
                    case KEY_DOWN :
                    {
                        if(!state.paused)
                        {
                            x_acc = 0.f;
                            y_acc = -10.f;
                            acc_updated = true;
                        }
                        break;
                    }
                    case KEY_RIGHT :
                    {
                        if(!state.paused)
                        {
                            x_acc = 10.f;
                            y_acc = 0.f;
                            acc_updated = true;
                        }
                        break;
                    }
                    case KEY_LEFT :
                    {
                        if(!state.paused)
                        {
                            x_acc = -10.f;
                            y_acc = 0.;
                            acc_updated = true;
                        }
                        break;
                    }
                    case 'm' :
                    {
                        state.is_random = !state.is_random;
                        break;
                    }
                    case 'p' :
                    case ' ' :
                    {
                        if((state.paused = !state.paused))
                        {
                            sim_can_continue = false;
                        }
                        // else
                        // {
                        //     sim_can_continue = true;
                        //     sim_notifier.notify_all();
                        // }
                        break;
                    }
                    case 's' :
                    {
                        switch(state.show_stats)
                        {
                            case 1 : state.show_stats = 0; break;
                            case 0 :
                            case 2 :
                            default : state.show_stats = 1; break;
                        }
                        break;
                    }
                    case 'S' :
                    {
                        switch(state.show_stats)
                        {
                            case 2 : state.show_stats = 0; break;
                            case 0 :
                            case 1 :
                            default : state.show_stats = 2; break;
                        }
                        break;
                    }
                    case '?' :
                    case 'h' :
                    {
                        state.show_usage = !state.show_usage;
                        break;
                    }
                    case 'l' :
                    {
                        if((state.is_reversed = !state.is_reversed))
                        {
                            attron(A_REVERSE);
                        }
                        else
                        {
                            attroff(A_REVERSE);
                        }
                        break;
                    }
                    case KEY_RESIZE :
                    {
                        wx = getmaxx(stdscr);
                        wy = getmaxy(stdscr);
                        state.resized = true;
                        break;
                    }
                }
            }
            nodelay(stdscr, false);
        }

        process_metrics.update();

        clock_t::time_point ts = clock_t::now();
        ui_dt = std::chrono::duration<float>(ts - ui_ts).count();
        ui_ts = ts;

        // if(!state.paused)
        {
            if(acc_updated)
            {
                state.is_random = false;
            }
            else
            if(!state.is_random)
            {
                x_acc = x_acc * 0.75f;
                y_acc = y_acc * 0.75f;
            }
            else
            {
                if(!rando.i_gen(rando.twister))
                {
                    x_acc = x_acc + rando.f_gen(rando.twister);
                    y_acc = y_acc + rando.f_gen(rando.twister);
                }
                else
                {
                    x_acc = x_acc * 0.9f;
                    y_acc = y_acc * 0.9f;
                }
            }

            // render
            sim_mtx.lock();
            f.render(stdscr, fluid_grad);
            if(state.show_stats > 0)
            {
                char buff[40];
                snprintf(buff, 40, "Sim Timestep : %.1fms (%.0fms)", sim_dt.load() / (SIM_REALTIME_FACTOR * 1e-3f), std::min(sim_dt.load(), MAX_SIM_DT) * 1e3f);
                f.overlay(stdscr, 0, 0, buff, overlay_grad);
                snprintf(buff, 40, "UI Timestep  : %.1fms", ui_dt * 1e3f);
                f.overlay(stdscr, 1, 0, buff, overlay_grad);
                snprintf(buff, 40, "Acceleration : <%.3f, %.3f>", x_acc.load(), y_acc.load());
                f.overlay(stdscr, 2, 0, buff, overlay_grad);
                snprintf(buff, 40, "Window Size  : %d x %d", wx, wy);
                f.overlay(stdscr, 3, 0, buff, overlay_grad);

                if(state.show_stats > 1)
                {
                    double mem;
                    size_t nthreads;
                    util::proc::getProcessStats(mem, nthreads);

                    snprintf(buff, 40, "CPU Usage    : %.1f%%", process_metrics.last_cpu_percent);
                    f.overlay(stdscr, 4, 0, buff, overlay_grad);
                    snprintf(buff, 40, "Mem Alloc    : %.0f MB", mem);
                    f.overlay(stdscr, 5, 0, buff, overlay_grad);
                }
            }
            if(state.show_usage)
            {
                int beg_x = (wx - HELP_MESSAGE_COLS) / 2;
                int beg_y = (wy - HELP_MESSAGE_ROWS) / 2;
                for(int y = 0; y < HELP_MESSAGE_ROWS; y++)
                {
                    f.overlay(stdscr, beg_y + y, beg_x, HELP_MESSAGE[y], overlay_dark_grad);
                }
            }
            refresh();
            sim_mtx.unlock();

            if(!state.paused)
            {
                if(state.resized)
                {
                    f.resize(wx, wy * 2);
                    state.resized = false;
                }

                sim_can_continue = true;
                sim_notifier.notify_all();
            }
        }

        if(tv.tv_usec)
        {
            usleep(std::min(tv.tv_usec, static_cast<suseconds_t>(MIN_UI_DT * 1e6f)));
        }
    }

    sim_can_continue = true;
    sim_notifier.notify_all();
    sim_thread.join();

    attroff(A_BOLD);
    NCInitializer::shutdown();
}
