#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cmath>
#include <random>
#include <string_view>


struct RollNum
{
    uint32_t base;
    uint16_t sides;
    uint16_t rolls;

public:
    uint32_t roll(uint32_t seed = std::mt19937::default_seed);
    void serialize(std::ostream& out) const;

};

class Entity
{
public:
    std::string_view name, desc;
    RollNum attack;
    uint32_t speed, health;
    union
    {
        struct
        {
            uint8_t is_smart : 1;
            uint8_t is_tele : 1;
            uint8_t can_tunnel : 1;
            uint8_t is_erratic : 1;
            uint8_t is_ghost : 1;
            uint8_t can_pickup : 1;
            uint8_t can_destroy : 1;
            uint8_t is_unique : 1;
            uint8_t is_boss : 1;
            uint8_t is_pc : 1;
        };
        uint16_t ability_bits;
    };
    uint8_t color;
    char symbol;
};

class Item
{

};

class MonDescription
{
public:
    static void parse(std::istream& f, std::vector<MonDescription>& descs);
    static bool verifyHeader(std::istream& f);

    static std::string& Name(MonDescription& m) { return m.name; }
    static std::string& Desc(MonDescription& m) { return m.desc; }
    static RollNum& Speed(MonDescription& m) { return m.speed; }
    static RollNum& Health(MonDescription& m) { return m.health; }
    static RollNum& Attack(MonDescription& m) { return m.attack; }
    static uint16_t& Abilities(MonDescription& m) { return m.abilities; }
    static uint8_t& Colors(MonDescription& m) { return m.colors; }
    static uint8_t& Rarity(MonDescription& m) { return m.rarity; }
    static char& Symbol(MonDescription& m) { return m.symbol; }

    enum
    {
        CLR_RED = 1 << 0,
        CLR_GREEN = 1 << 1,
        CLR_BLUE = 1 << 2,
        CLR_CYAN = 1 << 3,
        CLR_YELLOW = 1 << 4,
        CLR_MAGENTA = 1 << 5,
        CLR_WHITE = 1 << 6,
        CLR_BLACK =  1 << 7
    };
    enum
    {
        ABILITY_SMART = 1 << 0,
        ABILITY_TELE = 1 << 1,
        ABILITY_TUNNEL = 1 << 2,
        ABILITY_ERRATIC = 1 << 3,
        ABILITY_PASS = 1 << 4,
        ABILITY_PICKUP = 1 << 5,
        ABILITY_DESTROY = 1 << 6,
        ABILITY_UNIQ = 1 << 7,
        ABILITY_BOSS = 1 << 8,
    };

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

class ItemDescription
{
public:
    static bool verifyHeader(std::istream& f);

    static std::string& Name(ItemDescription& i) { return i.name; }
    static std::string& Desc(ItemDescription& i) { return i.desc; }
    static RollNum& Hit(ItemDescription& i) { return i.hit; }
    static RollNum& Damage(ItemDescription& i) { return i.damage; }
    static RollNum& Dodge(ItemDescription& i) { return i.dodge; }
    static RollNum& Defense(ItemDescription& i) { return i.defense; }
    static RollNum& Weight(ItemDescription& i) { return i.weight; }
    static RollNum& Speed(ItemDescription& i) { return i.speed; }
    static RollNum& Special(ItemDescription& i) { return i.special; }
    static RollNum& Value(ItemDescription& i) { return i.value; }
    static uint32_t& Types(ItemDescription& i) { return i.types; }
    static uint8_t& Colors(ItemDescription& i) { return i.colors; }
    static uint8_t& Rarity(ItemDescription& i) { return i.rarity; }
    static bool& Artifact(ItemDescription& i) { return i.artifact; }

    enum
    {
        CLR_RED = 1 << 0,
        CLR_GREEN = 1 << 1,
        CLR_BLUE = 1 << 2,
        CLR_CYAN = 1 << 3,
        CLR_YELLOW = 1 << 4,
        CLR_MAGENTA = 1 << 5,
        CLR_WHITE = 1 << 6,
        CLR_BLACK =  1 << 7
    };
    enum
    {
        TYPE_WEAPON = 1 << 0,
        TYPE_OFFHAND = 1 << 1,
        TYPE_RANGED = 1 << 2,
        TYPE_ARMOR = 1 << 3,
        TYPE_HELMET = 1 << 4,
        TYPE_CLOAK = 1 << 5,
        TYPE_GLOVES = 1 << 6,
        TYPE_BOOTS = 1 << 7,
        TYPE_RING = 1 << 8,
        TYPE_AMULET = 1 << 9,
        TYPE_LIGHT = 1 << 10,
        TYPE_SCROLL = 1 << 11,
        TYPE_BOOK = 1 << 12,
        TYPE_FLASK = 1 << 13,
        TYPE_GOLD = 1 << 14,
        TYPE_AMMUNITION = 1 << 15,
        TYPE_FOOD = 1 << 16,
        TYPE_WAND = 1 << 17,
        TYPE_CONTAINER = 1 << 18
    };

public:
    inline ItemDescription() = default;
    inline ~ItemDescription() = default;

public:
    void serialize(std::ostream& out) const;

protected:
    std::string name;
    std::string desc;

    RollNum hit;
    RollNum damage;
    RollNum dodge;
    RollNum defense;
    RollNum weight;
    RollNum speed;
    RollNum special;
    RollNum value;

    uint32_t types;
    uint8_t colors;
    uint8_t rarity;
    bool artifact;

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
