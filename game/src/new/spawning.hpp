#pragma once

#include <unordered_map>
#include <string_view>
#include <functional>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <vector>
#include <cmath>

#include "util/vec_geom.hpp"
#include "util/random.hpp"
#include "util/math.hpp"


enum DisplayColor
{
    RED = 1 << 0,
    GREEN = 1 << 1,
    BLUE = 1 << 2,
    CYAN = 1 << 3,
    YELLOW = 1 << 4,
    MAGENTA = 1 << 5,
    WHITE = 1 << 6,
    BLACK =  1 << 7
};

class MonDescription
{
    friend class Entity;

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

class Entity
{
public:
    struct PCGenT {};

public:
    Entity(PCGenT);
    Entity(const MonDescription& md, std::mt19937& gen);
    inline ~Entity() = default;

public:
    inline bool isAlive() const { return this->state.health > 0; }
    inline bool isDead() const { return !this->isAlive(); }
    inline bool isPC() const { return this->config.is_pc; }
    inline char getChar() const { return this->config.symbol; }

protected:
    inline Entity() = default;
    inline Entity(const Entity&) = delete;
    Entity(Entity&&);

    Entity& operator=(const Entity&) = delete;
    Entity& operator=(Entity&&);

public:
    struct
    {
        std::string_view name{}, desc{};
        RollableNum attack_damage{ {} };
        uint32_t speed{ 0 };
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
            uint16_t ability_bits{ 0 };
        };
        uint8_t color{ 0 };
        char symbol{ ' ' };

        const MonDescription* unique_entry{ nullptr };
    }
    config;

    struct
    {
        Vec2u8 pos{ 0, 0 };
        Vec2u8 target_pos{ 0, 0 };

        uint32_t health{ 0 };
    }
    state;

};






class ItemDescription
{
    friend class Item;

public:
    static void parse(std::istream& f, std::vector<ItemDescription>& descs);
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

class Item
{
public:
    struct StackNode
    {
        Item *item{ nullptr }, *next{ nullptr };
    };

public:
    std::string_view name, desc;
    RollableNum attack_damage;
    uint32_t hit;
    uint32_t dodge;
    uint32_t defense;
    uint32_t weight;
    uint32_t speed;
    uint32_t special;
    uint32_t value;
    uint32_t type;
    uint8_t color;

    ItemDescription* artifact_entry;

public:
    char getChar();

};








// template<typename T>
// void memToStr(const T& x, std::string& str)
// {
//     uintptr_t p = reinterpret_cast<uintptr_t>(&x);

//     str.clear();
//     str.reserve(ceil(log(p) / log(26)) + 1);

//     size_t i;
//     for(i = 0; p > 0; i++)
//     {
//         const uintptr_t q = p / 26;
//         str += ('a' + static_cast<char>(p - (q * 26)));
//         p = q;
//     }
// }
