#include "random.hpp"


uint32_t RollNum::roll(uint32_t seed) const
{
    return RollableNum{ *this, seed }.roll();
}
