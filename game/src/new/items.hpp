#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cmath>


struct RollNum
{
    uint32_t base;
    uint16_t sides;
    uint16_t rolls;

public:
    uint32_t roll();
    void serialize(std::ostream& out) const;

};

class MonDescription
{
public:
    static void parse(std::istream& f, std::vector<MonDescription>& descs);

public:
    inline MonDescription() = default;
    inline ~MonDescription() = default;

public:
    void serialize(std::ostream& out) const;

protected:
    std::string name;
    std::string desc;

    RollNum speed;
    RollNum health;
    RollNum attack;

    uint16_t abilities; // SMART | TELE | TUNNEL | ERRATIC | PASS | PICKUP | DESTROY | UNIQ | BOSS
    uint8_t colors;     // RED | GREEN | BLUE | CYAN | YELLOW | MAGENTA | WHITE | BLACK
    uint8_t rarity;     // between 1 and 100

    char symbol;

};








template<typename T>
void memToStr(const T& x, std::string& str)
{
    uintptr_t p = reinterpret_cast<uintptr_t>(&x);

    str.clear();
    str.reserve(ceil(log(p) / log(26)) + 1);

    size_t i;
    for(i = 0; p > 0; i++)
    {
        const uintptr_t q = p / 26;
        str += ('a' + static_cast<char>(p - (q * 26)));
        p = q;
    }
}
