#include <condition_variable>
#include <algorithm>
// #include <fstream>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
#include <cmath>
#include <mutex>

#include <unistd.h>
#include <sys/select.h>

#include <ncurses.h>

#include "util/nc_wrap.hpp"


#ifndef USE_OMP
#define USE_OMP 0
#endif

#if USE_OMP
#include <omp.h>
#endif

// static std::ofstream dbg{ "./debug.txt" };

/** Core functionality converted from https://github.com/matthias-research/pages/blob/master/tenMinutePhysics/18-flip.html */
class FlipFluid
{
public:
    enum CellType
    {
        FLUID_CELL = 0,
        AIR_CELL = 1,
        SOLID_CELL = 2
    };

    // Fluid parameters
    float density;
    int fNumX, fNumY, fNumCells;
    float h, fInvSpacing;
    std::vector<float> u, v, du, dv, prevU, prevV, p, s, cellColor;
    std::vector<int> cellType;

    // Particle parameters
    int maxParticles;
    int numParticles = 0;
    float particleRadius, pInvSpacing;
    int pNumX, pNumY, pNumCells;
    float particleRestDensity = 0.0f;
    std::vector<float> particlePos, particleVel, particleDensity; //, particleColor;
    std::vector<int> numCellParticles, firstCellParticle, cellParticleIds;

public:
    FlipFluid(
        float density_,
        float width,
        float height,
        float spacing,
        float particleRadius_,
        int maxParticles_ );
    inline ~FlipFluid() = default;

    void simulate(
        float dt,
        float x_acc,
        float y_acc,
        float flipRatio,
        int numPressureIters,
        int numParticleIters,
        float overRelaxation,
        bool compensateDrift,
        bool separateParticles );

    void resize(float width, float height, float spacing = 1.f);
    void render(WINDOW* w, NCGradient& grad);

public:
    void integrateParticles(float dt, float x_acc, float y_acc);
    void pushParticlesApart(int numIters);
    void handleParticleCollisions(/*float obstacleX, float obstacleY, float obstacleRadius*/);
    void updateParticleDensity();
    void transferVelocities(bool toGrid, float flipRatio);
    void solveIncompressibility(
        int numIters,
        float dt,
        float overRelaxation,
        bool compensateDrift = true );
    // void updateParticleColors();
    void updateCellColors();
    void setSciColor(int cellNr, float val, float minVal, float maxVal);

private:
    inline float clamp(float x, float minVal, float maxVal)
    {
        return std::max(minVal, std::min(x, maxVal));
    }

};



FlipFluid::FlipFluid(
    float density_,
    float width,
    float height,
    float spacing,
    float particleRadius_,
    int maxParticles_ )
:
    density(density_),
    maxParticles(maxParticles_),
    particleRadius(particleRadius_)
{

    fNumX = static_cast<int>(std::ceil(width / spacing));
    fNumY = static_cast<int>(std::ceil(height / spacing));
    h = std::max(width / fNumX, height / fNumY);
    fInvSpacing = 1.0f / h;
    fNumCells = fNumX * fNumY;

    u.resize(fNumCells, 0);
    v.resize(fNumCells, 0);
    du.resize(fNumCells, 0);
    dv.resize(fNumCells, 0);
    prevU.resize(fNumCells, 0);
    prevV.resize(fNumCells, 0);
    p.resize(fNumCells, 0);
    s.resize(fNumCells, 1);
    cellType.resize(fNumCells, FLUID_CELL);
    cellColor.resize(3 * fNumCells, 0);

    particlePos.resize(2 * maxParticles, 0);
    particleVel.resize(2 * maxParticles, 0);
    // particleColor.resize(3 * maxParticles, 0);
    // for(int i = 0; i < maxParticles; ++i)
    // {
    //     particleColor[3 * i + 2] = 1.0f;
    // }

    particleDensity.resize(fNumCells, 0);

    pInvSpacing = 1.0f / (2.2f * particleRadius);
    pNumX = static_cast<int>(width * pInvSpacing) + 1;
    pNumY = static_cast<int>(height * pInvSpacing) + 1;
    pNumCells = pNumX * pNumY;

    numCellParticles.resize(pNumCells, 0);
    firstCellParticle.resize(pNumCells + 1, 0);
    cellParticleIds.resize(maxParticles, 0);

    #if USE_OMP
    int tt = std::thread::hardware_concurrency() - 1;
    if(tt > 1)
    {
        omp_set_num_threads(tt);
    }
    #endif
}

void FlipFluid::simulate(
    float dt,
    float x_acc,
    float y_acc,
    float flipRatio,
    int numPressureIters,
    int numParticleIters,
    float overRelaxation,
    bool compensateDrift,
    bool separateParticles )
{
    constexpr int numSubSteps = 1;
    float sdt = dt / numSubSteps;

    for(int step = 0; step < numSubSteps; step++)
    {
        this->integrateParticles(sdt, x_acc, y_acc);
        if(separateParticles)
        {
            this->pushParticlesApart(numParticleIters);
        }
        this->handleParticleCollisions();
        this->transferVelocities(true, flipRatio);
        this->updateParticleDensity();
        this->solveIncompressibility(numPressureIters, sdt, overRelaxation, compensateDrift);
        this->transferVelocities(false, flipRatio);
    }

    // this->updateParticleColors();
    this->updateCellColors();
}

void FlipFluid::resize(float width, float height, float spacing)
{
    FlipFluid f{ density, width, height, spacing, particleRadius, maxParticles };

    int mx = std::min(fNumX, f.fNumX) - 1;
    int my = std::min(fNumY, f.fNumY) - 1;

    // copy fluid cells in range
    for(int x = 1; x < mx; x++)
    {
        for(int y = 1; y < my; y++)
        {
            int i_src = x * fNumY + y;
            int i_dest = x * f.fNumY + y;

            f.u[i_dest] = u[i_src];
            f.v[i_dest] = v[i_src];
            f.du[i_dest] = du[i_src];
            f.dv[i_dest] = dv[i_src];
            f.prevU[i_dest] = prevU[i_src];
            f.prevV[i_dest] = prevV[i_src];
            f.s[i_dest] = s[i_src];
            f.p[i_dest] = p[i_src];
            f.cellType[i_dest] = cellType[i_src];   // <-- ideally fill all with AIR_CELL by default
            // f.cellColor[i_dest] = cellColor[i_src];
            // f.particleDensity[i_dest] = particleDensity[i_src];
        }
    }

    for(int x = 0; x < f.fNumX; x++)
    {
        f.s[x * f.fNumY + 0] = 0.f;
        f.s[(x + 1) * f.fNumY - 1] = 0.f;
    }
    for(int y = 1; y < f.fNumY - 1; y++)
    {
        f.s[y] = 0.f;
        f.s[(f.fNumX - 1) * f.fNumY + y] = 0.f;
    }

    fNumX = f.fNumX;
    fNumY = f.fNumY;
    h = f.h;
    fInvSpacing = f.fInvSpacing;
    fNumCells = f.fNumCells;

    u.swap(f.u);
    v.swap(f.v);
    du.swap(f.du);
    dv.swap(f.dv);
    prevU.swap(f.prevU);
    prevV.swap(f.prevV);
    p.swap(f.p);
    s.swap(f.s);
    cellType.swap(f.cellType);
    cellColor.swap(f.cellColor);
    pNumX = f.pNumX;
    pNumY = f.pNumY;
    pNumCells = f.pNumCells;
    numCellParticles.swap(f.numCellParticles);
    firstCellParticle.swap(f.firstCellParticle);
    cellParticleIds.swap(f.cellParticleIds);
    particleDensity.swap(f.particleDensity);

    // particleRestDensity = 0.f;
}

void FlipFluid::render(WINDOW* w, NCGradient& grad)
{
    const int mx = std::min(getmaxx(w), fNumX - 2);
    const int my = std::min(getmaxy(w), fNumY / 2 - 2);

    for(int y = my - 1; y >= 0; y--)
    {
        for(int x = 0; x < mx; x++)
        {
            int i0 = (x + 1) * fNumY + ((y + 1) * 2);
            int i1 = (x + 1) * fNumY + ((y + 1) * 2 + 1);
            if( (cellType[i0] == SOLID_CELL && cellType[i1]) ||
                (cellType[i0] == AIR_CELL && cellType[i1]) )
            {
                attron(COLOR_PAIR(COLOR_BLACK));
                mvwaddch(w, my - y - 1, x, ' ');
                attron(COLOR_PAIR(COLOR_BLACK));
            }
            else
            {
                // float density = (particleDensity[i0] + particleDensity[i1]) / (particleRestDensity * 2);

                float density =
                    (cellColor[i0 * 3] + cellColor[i0 * 3 + 1] + cellColor[i0 * 3 + 2] +
                        cellColor[i1 * 3] + cellColor[i1 * 3 + 1] + cellColor[i1 * 3 + 2]) / 6.f;

                // // float u = (this->du[i0] + this->du[i1]);
                // // float v = (this->dv[i0] + this->dv[1]);
                // float weight = std::abs((p[i0] + p[i1]) * 0.5f);

                grad.printChar(
                    w, my - y - 1, x,
                    grad.floatToIdx(density * 3.f - 1.f),    // <-- density: use d * 3 - 1
                    " .,:+#@"[MIN_CACHED(static_cast<size_t>((1.f - density) * 15.f - 4.f), 6U)] );
            }
        }
    }
    // wrefresh(w);
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
    // float colorDiffusionCoeff = 0.001f;
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

                        // for(int k = 0; k < 3; ++k)
                        // {
                        //     float color0 = particleColor[3 * i + k];
                        //     float color1 = particleColor[3 * id + k];
                        //     float color = 0.5f * (color0 + color1);
                        //     particleColor[3 * i + k] += (color - color0) * colorDiffusionCoeff;
                        //     particleColor[3 * id + k] += (color - color1) * colorDiffusionCoeff;
                        // }
                    }
                }
            }
        }
    }
}

void FlipFluid::handleParticleCollisions(
    /*float obstacleX,
    float obstacleY,
    float obstacleRadius*/ )
{
    float hLocal = 1.0f / fInvSpacing;
    float r = particleRadius;
    // float or2 = obstacleRadius * obstacleRadius;
    // float minDist = obstacleRadius + r;
    // float minDist2 = minDist * minDist;

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

        // float dx = x - obstacleX;
        // float dy = y - obstacleY;
        // float d2 = dx * dx + dy * dy;

        // if(d2 < minDist2)
        // {
        //     particleVel[2 * i] = 0.0f;  // Assume obstacle velocity is 0 for simplicity
        //     particleVel[2 * i + 1] = 0.0f;
        // }

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

// void FlipFluid::updateParticleColors()
// {
//     float s = 0.01f;
//     float h1 = fInvSpacing;

//     #if USE_OMP
//     #pragma omp parallel for
//     #endif
//     for(int i = 0; i < numParticles; ++i)
//     {
//         particleColor[3 * i] = clamp(particleColor[3 * i] - s, 0.0f, 1.0f);
//         particleColor[3 * i + 1] = clamp(particleColor[3 * i + 1] - s, 0.0f, 1.0f);
//         particleColor[3 * i + 2] = clamp(particleColor[3 * i + 2] + s, 0.0f, 1.0f);

//         float x = particlePos[2 * i];
//         float y = particlePos[2 * i + 1];
//         int xi = static_cast<int>(clamp(std::floor(x * h1), 1.0f, static_cast<float>(fNumX - 1)));
//         int yi = static_cast<int>(clamp(std::floor(y * h1), 1.0f, static_cast<float>(fNumY - 1)));
//         int cellNr = xi * fNumY + yi;

//         if(particleRestDensity > 0.0f)
//         {
//             float relDensity = particleDensity[cellNr] / particleRestDensity;
//             if(relDensity < 0.7f)
//             {
//                 float c = 0.8f;
//                 particleColor[3 * i] = c;
//                 particleColor[3 * i + 1] = c;
//                 particleColor[3 * i + 2] = 1.0f;
//             }
//         }
//     }
// }

void FlipFluid::updateCellColors()
{
    #if USE_OMP
    #pragma omp parallel for
    #endif
    for(int i = 0; i < fNumCells; ++i)
    {
        if(cellType[i] == SOLID_CELL)
        {
            cellColor[3 * i] = 0.5f;
            cellColor[3 * i + 1] = 0.5f;
            cellColor[3 * i + 2] = 0.5f;
        }
        else
        if(cellType[i] == FLUID_CELL)
        {
            float d = particleDensity[i];
            if(particleRestDensity > 0.0f)
            {
                d /= particleRestDensity;
            }
            setSciColor(i, d, 0.0f, 2.0f);
        }
    }
}

void FlipFluid::setSciColor(int cellNr, float val, float minVal, float maxVal)
{
    val = std::min(std::max(val, minVal), maxVal - 0.0001f);
    float d = maxVal - minVal;
    val = d == 0.0f ? 0.5f : (val - minVal) / d;
    int num = static_cast<int>(val / 0.25f);
    float s = (val - num * 0.25f) / 0.25f;
    float r, g, b;

    switch(num)
    {
        case 0:
        {
            r = 0.0f;
            g = s;
            b = 1.0f;
            break;
        }
        case 1:
        {
            r = 0.0f;
            g = 1.0f;
            b = 1.0f - s;
            break;
        }
        case 2:
        {
            r = s;
            g = 1.0f;
            b = 0.0f;
            break;
        }
        case 3:
        {
            r = 1.0f;
            g = 1.0f - s;
            b = 0.0f;
            break;
        }
        default:
        {
            r = g = b = 0.0f;
            break;
        }
    }

    cellColor[3 * cellNr] = r;
    cellColor[3 * cellNr + 1] = g;
    cellColor[3 * cellNr + 2] = b;
}



int main(int argc, char** argv)
{
    NCInitializer::use();
    attron(A_BOLD);
    
    // {500, 100, 800}, {100, 1000, 500}
    // {700, 0, 600}, {200, 1000, 400}
    // {400, 100, 1000}, {400, 1000, 100}
    NCGradient grad{{100, 800, 400}, {100, 200, 800}};
    grad.applyForeground(COLOR_BLACK);

    int wx = getmaxx(stdscr);
    int wy = getmaxy(stdscr);

    constexpr float realtime_factor = 10.f;
    constexpr float default_dt = 0.05f;
    constexpr float max_sim_dt = 0.5f;
    constexpr float flipRatio = 0.5;
    constexpr int numPressureIters = 50;
    constexpr int numParticleIters = 2;
    constexpr float overRelaxation = 1.9;
    constexpr float compensateDrift = true;
    constexpr float separateParticles = true;

    // constexpr float res = 100.f;
    constexpr float density = 1000.f;
    float tankHeight = static_cast<float>(wy * 2 + 4);
    float tankWidth = static_cast<float>(wx + 2);
    constexpr float h = 1;

    constexpr float relWaterHeight = 1.f;
    constexpr float relWaterWidth = 0.3f;

    float r = 0.3f * h;
    float dx = 2.f * r;
    float dy = std::sqrt(3.f) / 2.f * dx;

    int numX = static_cast<int>(std::floor((relWaterWidth * tankWidth - 2.f * h - 2.f * r) / dx));
    int numY = static_cast<int>(std::floor((relWaterHeight * tankHeight - 2.f * h - 2.f * r) / dy));
    // int numX = static_cast<int>(tankWidth - 2);
    // int numY = static_cast<int>(tankHeight - 2);
    int maxParticles = numX * numY;

    FlipFluid f{ density, tankWidth, tankHeight, h, r, maxParticles };

    f.numParticles = numX * numY;
    int p = 0;
    for(int i = 0; i < numX; i++)
    {
        for(int j = 0; j < numY; j++)
        {
            f.particlePos[p++] = h + r + dx * i + (j % 2 == 0 ? 0.f : r);
            f.particlePos[p++] = h + r + dy * j;
            // f.particlePos[p++] = (i + 1.5f) * h;
            // f.particlePos[p++] = (j + 1.5f) * h;
        }
    }

    for(int x = 0; x < f.fNumX; x++)
    {
        f.s[x * f.fNumY + 0] = 0.f;
        f.s[(x + 1) * f.fNumY - 1] = 0.f;
    }
    for(int y = 1; y < f.fNumY - 1; y++)
    {
        f.s[y] = 0.f;
        f.s[(f.fNumX - 1) * f.fNumY + y] = 0.f;
    }

    std::atomic<bool> running = true;
    std::atomic<bool> sim_can_continue = true;
    std::condition_variable sim_notifier;

    fd_set set;
    struct timeval tv;

    std::atomic<float> x_acc = 0.f;
    std::atomic<float> y_acc = 0.f;
    std::atomic<float> sim_dt = default_dt * realtime_factor;
    std::mutex sim_mtx, tmp_mtx;

    std::thread sim_thread{
        [&]()
        {
            // size_t iter = 0;
            while(running)
            {
                using clock_t = std::chrono::system_clock;

                clock_t::time_point beg = clock_t::now();
                sim_mtx.lock();
                f.simulate(
                    sim_dt.load(),
                    x_acc,
                    y_acc,
                    flipRatio,
                    numPressureIters,
                    numParticleIters,
                    overRelaxation,
                    compensateDrift,
                    separateParticles );
                sim_mtx.unlock();
                sim_dt = std::min(
                    max_sim_dt,
                    std::chrono::duration<float>(clock_t::now() - beg).count() * realtime_factor );

                if(!sim_can_continue)
                {
                    // wait for signal to continue;
                    std::unique_lock<std::mutex> l{ tmp_mtx };
                    while(running && !sim_can_continue)
                    {
                        sim_notifier.wait(l);
                    }
                    sim_can_continue = true;
                }

                // iter++;
            }
        } };

    struct
    {
        uint8_t paused : 1;
        uint8_t show_stats : 1;
        uint8_t resized : 1;
    }
    state;
    state.paused = state.resized = 0;
    state.show_stats = 1;

    // bool paused = false;
    // bool show_stats = true;
    while(running)
    {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        bool acc_updated = false;

        int x = select(1, &set, NULL, NULL, &tv);
        sim_can_continue = false;   // signal sim thread to wait for render after next iteration

        if(x)
        {
            int c = getch();

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
                case ' ' :
                {
                    state.paused = !state.paused;
                    if(state.paused)
                    {
                        sim_can_continue = false;
                    }
                    else
                    {
                        sim_can_continue = true;
                        sim_notifier.notify_all();
                    }
                    break;
                }
                case 's' :
                {
                    state.show_stats = !state.show_stats;
                    break;
                }
                case KEY_RESIZE :
                {
                    wx = getmaxx(stdscr);
                    wy = getmaxy(stdscr);
                    state.resized = true;
                }
            }
        }

        if(!state.paused)
        {
            if(!acc_updated)
            {
                x_acc = x_acc * 0.75f;
                y_acc = y_acc * 0.75f;
            }

            // render
            sim_mtx.lock();
            f.render(stdscr, grad);
            if(state.show_stats)
            {
                mvprintw(0, 0, "Sim Timestep : %.3fs (%.1fms)", sim_dt.load(), sim_dt.load() / realtime_factor * 1000.f);
                mvprintw(1, 0, "Acceleration : <%.3f, %.3f>", x_acc.load(), y_acc.load());
                mvprintw(2, 0, "Win Size : (%d, %d)", wx, wy);
            }
            refresh();
            if(state.resized)
            {
                f.resize(static_cast<float>(wx + 2), static_cast<float>(wy * 2 + 4));
            }
            sim_mtx.unlock();

            sim_can_continue = true;
            sim_notifier.notify_all();
        }
    }

    sim_can_continue = true;
    sim_notifier.notify_all();
    sim_thread.join();

    attroff(A_BOLD);
    NCInitializer::shutdown();
}
