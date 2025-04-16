#pragma once

#include <type_traits>
#include <iostream>
#include <cstdlib>
#include <random>


struct RollNum
{
    uint32_t base{ 0 };
    uint16_t sides{ 1 };
    uint16_t rolls{ 0 };

public:
    inline void serialize(std::ostream& out) const
    {
        out << this->base << '+' << this->rolls << 'd' << this->sides;
    }

    uint32_t roll(uint32_t seed = std::mt19937::default_seed) const;

    template<typename G = std::mt19937>
    inline uint32_t roll(G& gen) const
    {
        std::uniform_int_distribution<uint32_t> dist{ 1, this->sides };
        uint32_t x;
        for(uint32_t r = 0; r < this->rolls; r++) x += dist(gen);

        return x;
    }

};

struct RollableNum
{
    using RollNumArgT = std::conditional<(sizeof(RollNum) > sizeof(std::uintptr_t)), const RollNum&, RollNum>::type;

public:
    inline RollableNum(RollNumArgT n, uint32_t seed = std::mt19937::default_seed) :
        base{ n.base }, rolls{ n.rolls }, distribution{ 1, n.sides }, generator{ seed }
    {}

    inline RollableNum& reset(RollNumArgT n)
    {
        this->base = n.base;
        this->rolls = n.rolls;
        this->distribution = std::uniform_int_distribution<uint32_t>(1, n.sides);

        return *this;
    }

    inline RollableNum& setSeed(uint32_t s)
    {
        this->generator.seed(s);

        return *this;
    }

    template<typename G = std::mt19937>
    inline uint32_t roll(G& generator) const
    {
        uint32_t x = this->base;
        for(uint32_t r = 0; r < this->rolls; r++) x += this->distribution(generator);
        return x;
    }

    inline uint32_t roll()
    {
        this->roll(this->generator);
    }

    inline uint32_t rollArg(RollNumArgT n)
    {
        std::uniform_int_distribution<uint32_t> dist{ 1, n.sides };
        uint32_t x;
        for(uint32_t r = 0; r < n.rolls; r++) x += dist(this->generator);
        return x;
    }

protected:
    uint32_t base, rolls;
    std::uniform_int_distribution<uint32_t> distribution;
    std::mt19937 generator;

};
