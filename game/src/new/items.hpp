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
    uintptr_t p = static_cast<const uintptr_t>(&x);

    str.clear();
    str.reserve(ceil(log(p) / log(26)) + 1);

    size_t i;
    for(i = 0; p > 0; i++)
    {
        const uintptr_t q = p / 26;
        str += ('a' + static_cast<char>(p - q));
        p = q;
    }
}

#include <iostream>

void MonDescription::parse(std::istream& f, std::vector<MonDescription>& descs)
{
    static constexpr char const
        *MONDESC_HEADER = "RLG327 MONSTER DESCRIPTION 1",
        *MONDESC_STARTMON = "BEGIN MONSTER";
        // *MONDESC_ENDMON = "END";

    // MONDESC_STARTMON;
    // (void)MONDESC_ENDMON;

    descs.clear();

    std::string s1;
    std::getline(f, s1);
    if(s1 != MONDESC_HEADER) return;

    enum
    {
        NAME = 1 << 0,
        DESC = 1 << 1,
        COLOR = 1 << 2,
        SPEED = 1 << 3,
        ABIL = 1 << 4,
        HP = 1 << 5,
        DAM = 1 << 6,
        SYMB = 1 << 7,
        RRTY = 1 << 8
    };
    #define SET_AND_CHECK(x, b) (((x) ^= (b)) & b)

    #define MON_ENDED 1
    #define MON_FAILED -1

    std::unordered_map<std::string, std::function<int(std::istream&, uint16_t&, MonDescription&)>> token_map;
    token_map["NAME"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, NAME))
            {
                std::getline(initial, mon.name);
                return 0;
            }
            return MON_FAILED;
        };
    token_map["DESC"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, DESC))
            {
                std::string s;
                for(std::getline(f, s); !f.eof() && s != "."; std::getline(f, s))
                {
                    mon.desc += s += '\n';
                }
                mon.desc.pop_back();
                return 0;
            }
            return MON_FAILED;
        };
    token_map["COLOR"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            static std::unordered_map<std::string, uint8_t> colors{
                { "RED", 1 << 0 },
                { "GREEN", 1 << 1 },
                { "BLUE", 1 << 2 },
                { "CYAN", 1 << 3 },
                { "YELLOW", 1 << 4 },
                { "MAGENTA", 1 << 5 },
                { "WHITE", 1 << 6 },
                { "BLACK", 1 << 7 }
            };

            if(SET_AND_CHECK(ps, COLOR))
            {
                mon.colors = 0;
                std::string c;
                do
                {
                    std::getline(initial, c, ' ');
                    auto search = colors.find(c);
                    if(search != colors.end())
                    {
                        mon.colors |= search->second;
                    }
                }
                while(!initial.eof());

                return 0;
            }
            return MON_FAILED;
        };
    token_map["SPEED"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, SPEED))
            {
                std::string x;
                std::getline(initial, x, '+');
                mon.speed.base = atoi(x.c_str());

                std::getline(initial, x, 'd');
                mon.speed.rolls = atoi(x.c_str());

                initial >> mon.speed.sides;

                return 0;
            }
            return MON_FAILED;
        };
    token_map["ABIL"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            static std::unordered_map<std::string, uint16_t> abilities{
                { "SMART", 1 << 0 },
                { "TELE", 1 << 1 },
                { "TUNNEL", 1 << 2 },
                { "ERRATIC", 1 << 3 },
                { "PASS", 1 << 4 },
                { "PICKUP", 1 << 5 },
                { "DESTROY", 1 << 6 },
                { "UNIQ", 1 << 7 },
                { "BOSS", 1 << 8 },
            };

            if(SET_AND_CHECK(ps, ABIL))
            {
                mon.abilities = 0;
                std::string a;
                do
                {
                    std::getline(initial, a, ' ');
                    auto search = abilities.find(a);
                    if(search != abilities.end())
                    {
                        mon.abilities |= search->second;
                    }
                }
                while(!initial.eof());

                return 0;
            }
            return MON_FAILED;
        };
    token_map["HP"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, HP))
            {
                std::string x;
                std::getline(initial, x, '+');
                mon.health.base = atoi(x.c_str());

                std::getline(initial, x, 'd');
                mon.health.rolls = atoi(x.c_str());

                initial >> mon.health.sides;

                return 0;
            }
            return MON_FAILED;
        };
    token_map["DAM"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, DAM))
            {
                std::string x;
                std::getline(initial, x, '+');
                mon.attack.base = atoi(x.c_str());

                std::getline(initial, x, 'd');
                mon.attack.rolls = atoi(x.c_str());

                initial >> mon.attack.sides;

                return 0;
            }
            return MON_FAILED;
        };
    token_map["SYMB"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, SYMB))
            {
                initial >> mon.symbol;

                return 0;
            }
            return MON_FAILED;
        };
    token_map["RRTY"] =
        [&f](std::istream& initial, uint16_t& ps, MonDescription& mon)
        {
            if(SET_AND_CHECK(ps, RRTY))
            {
                int r;
                initial >> r;
                mon.rarity = static_cast<uint8_t>(r);
                return 0;
            }
            return MON_FAILED;
        };
    token_map["END"] = [&f](std::istream& initial, uint16_t& ps, MonDescription& mon) { return MON_ENDED; };

    #undef SET_AND_CHECK

    bool in_mon = false;
    uint16_t ps = 0;
    for(size_t i = 0; !f.eof(); std::getline(f, s1))
    {
        // TODO: TRIM WHITESPACE!!!

        if(!in_mon && s1 == MONDESC_STARTMON)  // VERIFY START TOKEN!
        {
            if(i >= descs.size())
            {
                descs.emplace_back();
            }
            in_mon = true;
            ps = 0;
            continue;
        }

        if(in_mon)
        {
            std::stringstream s2{ s1 };
            std::getline(s2, s1, ' ');

            auto token_func = token_map.find(s1);
            if(token_func != token_map.end())
            {
                if(int v = token_func->second(s2, ps, descs.back()); v)
                {
                    in_mon = false;
                    i += (v == MON_ENDED);
                }
            }
            else
            {
                in_mon = false;
            }
        }
    }

    // if(!s1.empty()) std::cout << s1 << std::endl;
}

void MonDescription::serialize(std::ostream& out) const
{
    std::cout << "MonDescription@0x" << std::hex << reinterpret_cast<uintptr_t>(this) << std::dec
        << "\n\tName : " << this->name
        << "\n\tDesc : \"" << this->desc
        << "\"\n\tSpeed : " << this->speed.base << '+' << this->speed.rolls << 'd' << this->speed.sides
        << "\n\tHealth : " << this->health.base << '+' << this->health.rolls << 'd' << this->health.sides
        << "\n\tAttack : " << this->attack.base << '+' << this->attack.rolls << 'd' << this->attack.sides
        << "\n\tAbilities : 0x" << std::hex << this->abilities << std::dec
        << "\n\tColors : 0x" << std::hex << static_cast<int>(this->colors) << std::dec
        << "\n\tRarity : " << static_cast<int>(this->rarity)
        << "\n\tSymbol : " << this->symbol << '\n' << std::endl;
}
