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

#include <ncurses.h>

#include "util/vec_geom.hpp"
#include "util/random.hpp"
#include "util/math.hpp"


enum DisplayColor
{
    BLACK =  1 << COLOR_BLACK,
    RED = 1 << COLOR_RED,
    GREEN = 1 << COLOR_GREEN,
    YELLOW = 1 << COLOR_YELLOW,
    BLUE = 1 << COLOR_BLUE,
    MAGENTA = 1 << COLOR_MAGENTA,
    CYAN = 1 << COLOR_CYAN,
    WHITE = 1 << COLOR_WHITE
};

class MonDescription
{
    friend class Entity;

public:
    static bool parse(std::istream& f, std::vector<MonDescription>& descs);
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
    Entity(Entity&&);
    inline ~Entity() = default;

    Entity& operator=(Entity&&);

public:
    inline bool isAlive() const { return this->state.health > 0; }
    inline bool isDead() const { return !this->isAlive(); }
    inline bool isPC() const { return this->config.is_pc; }
    inline bool isBoss() const { return this->config.is_boss; }
    inline char getChar() const { return this->config.symbol; }
    short getColor() const;

    void print(std::ostream&);

protected:
    inline Entity() = default;
    inline Entity(const Entity&) = delete;

    Entity& operator=(const Entity&) = delete;

public:
    struct
    {
        std::string_view name{}, desc{};
        RollableNum attack_damage{ {} };
        int32_t speed{ 0 };
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

        int32_t health{ 0 };
    }
    state;

};






class ItemDescription
{
    friend class Item;

public:
    static bool parse(std::istream& f, std::vector<ItemDescription>& descs);
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
    Item(const ItemDescription& id, std::mt19937& gen);
    Item(Item&&);
    inline ~Item() = default;

    Item& operator=(Item&&);

    char getChar() const;
    short getColor() const;

    void print(std::ostream&);

protected:
    inline Item() = default;
    inline Item(const Item&) = delete;

    Item& operator=(const Item&) = delete;

public:
    // struct StackNode
    // {
    //     Item* item{ nullptr };
    //     StackNode* next{ nullptr };
    // };

public:
    std::string_view name{}, desc{};
    RollableNum attack_damage{ {} };
    uint32_t hit{ 0 };
    uint32_t dodge{ 0 };
    uint32_t defense{ 0 };
    uint32_t weight{ 0 };
    int32_t speed{ 0 };
    uint32_t special{ 0 };
    uint32_t value{ 0 };
    uint32_t type{ 0 };
    uint8_t color{ 0 };

    const ItemDescription* artifact_entry{ nullptr };
    // Item* stack_next{ nullptr };

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
