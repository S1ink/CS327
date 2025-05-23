#pragma once

#include <type_traits>
#include <iostream>
#include <cstdlib>
#include <random>


template<typename I = uint32_t, typename G = std::mt19937>
static inline I random_int(I min, I max, G& gen)
{
    std::uniform_int_distribution<I> dist{ min, max };
    return dist(gen);
}

template<typename F = float, typename G = std::mt19937>
static inline F random_float(F min, F max, G& gen)
{
    std::uniform_real_distribution<F> dist{ min, max };
    return dist(gen);
}


struct RollNum
{
    int32_t base{ 0 };
    uint16_t sides{ 1 };
    uint16_t rolls{ 0 };

public:
    inline void serialize(std::ostream& out) const
    {
        out << this->base << '+' << this->rolls << 'd' << this->sides;
    }

    int32_t roll(uint32_t seed = std::mt19937::default_seed) const;

    template<typename G = std::mt19937>
    inline int32_t roll(G& gen) const
    {
        if(!this->sides) return this->base;
        std::uniform_int_distribution<uint32_t> dist{ 1, this->sides };
        int32_t x = this->base;
        for(uint32_t r = 0; r < this->rolls; r++) x += static_cast<int32_t>(dist(gen));

        return x;
    }

};

struct RollableNum
{
    using RollNumArgT = std::conditional<(sizeof(RollNum) > sizeof(std::uintptr_t)), const RollNum&, RollNum>::type;

    friend std::ostream& operator<<(std::ostream&, const RollableNum&);

public:
    inline RollableNum(RollNumArgT n, uint32_t seed = std::mt19937::default_seed) :
        base{ n.base },
        rolls{ n.rolls },
        distribution{ n.sides ? 1 : 0, n.sides ? n.sides : 0 },
        generator{ seed }
    {}

    inline RollableNum(RollableNum&& r) :
        base{ r.base },
        rolls{ r.rolls },
        distribution{ std::move(r.distribution) },
        generator{ std::move(r.generator) }
    {}
    RollableNum& operator=(RollableNum&& r);

    RollableNum& reset(RollNumArgT n);
    RollableNum& setSeed(uint32_t s);

    template<typename G = std::mt19937>
    inline int32_t roll(G& generator)
    {
        int32_t x = this->base;
        for(uint32_t r = 0; r < this->rolls; r++) x += static_cast<int32_t>(this->distribution(generator));
        return x;
    }

    int32_t roll();
    int32_t rollArg(RollNumArgT n);

    inline bool isStatic() const { return this->rolls == 0; }
    inline int32_t getBase() const { return this->base; }
    inline uint32_t getRolls() const { return this->rolls; }
    inline uint32_t getSides() const { return this->distribution.b(); }

protected:
    int32_t base;
    uint32_t rolls;
    std::uniform_int_distribution<uint32_t> distribution;
    std::mt19937 generator;

};
