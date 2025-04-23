#include "random.hpp"


int32_t RollNum::roll(uint32_t seed) const
{
    return RollableNum{ *this, seed }.roll();
}



RollableNum& RollableNum::operator=(RollableNum&& r)
{
    base = r.base;
    rolls = r.rolls;
    distribution = std::move(r.distribution);
    generator = std::move(r.generator);

    return *this;
}

inline RollableNum& RollableNum::reset(RollNumArgT n)
{
    this->base = n.base;
    this->rolls = n.rolls;
    this->distribution = std::uniform_int_distribution<uint32_t>(1, n.sides);

    return *this;
}

inline RollableNum& RollableNum::setSeed(uint32_t s)
{
    this->generator.seed(s);

    return *this;
}

inline int32_t RollableNum::roll()
{
    return this->roll(this->generator);
}

inline int32_t RollableNum::rollArg(RollNumArgT n)
{
    std::uniform_int_distribution<uint32_t> dist{ 1, n.sides };
    int32_t x;
    for(uint32_t r = 0; r < n.rolls; r++) x += static_cast<int32_t>(dist(this->generator));
    return x;
}


std::ostream& operator<<(std::ostream& out, const RollableNum& rn)
{
    return out << rn.base << '+' << rn.rolls << 'd' << rn.distribution.b();
}
