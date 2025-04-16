#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "util/vec_geom.hpp"
#include "util/math.hpp"

#include "dungeon_config.h"

#include "spawning.hpp"


class DungeonLevel
{
public:
    template<typename T>
    using DungeonGrid = T[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    using DungeonCostMap = DungeonGrid<int32_t>;

    inline static std::uniform_int_distribution<uint32_t>
        NMON_DISTRIBUTION{ DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS };

public:
    struct TerrainMap
    {
    public:
        enum
        {
            CELLTYPE_ROCK = 0,
            CELLTYPE_ROOM,
            CELLTYPE_CORRIDOR,
            CELLTYPE_MAX_VALUE
        };
        enum
        {
            STAIR_NONE = 0,
            STAIR_UP,
            STAIR_DOWN,
            STAIR_MAX_VALUE
        };

    public:
        struct Cell
        {
            uint8_t type : REQUIRED_BITS32(CELLTYPE_MAX_VALUE - 1);
            uint8_t is_stair : REQUIRED_BITS32(STAIR_MAX_VALUE - 1);

            char getChar() const;
            inline bool isFloor() const { return static_cast<bool>(this->type); }
            inline bool isStair() const { return static_cast<bool>(this->is_stair); }
        };
        struct Room
        {
            Vec2u8 tl{ 0, 0 }, br{ 0, 0 };

            inline Vec2u8 size() const { return tl - br + Vec2u8{ 1 }; }
            bool collides(const Room& r) const;
        };

    public:
        DungeonGrid<Cell> terrain;
        DungeonGrid<uint8_t> hardness;

        std::vector<Room> rooms;

        uint8_t num_up_stair{ 0 }, num_down_stair{ 0 };

    public:
        inline TerrainMap() { this->reset(); }
        inline ~TerrainMap() = default;

        void reset();
        void generate(uint32_t s);

    };

public:
    inline DungeonLevel() :
        pc{ Entity::PCGenT{} }
    {}

    int loadTerrain(FILE* f);
    int saveTerrain(FILE* f);
    int generateTerrain(uint32_t seed);

public:
    TerrainMap map;
    DungeonCostMap tunnel_costs, terrain_costs;
    DungeonGrid<char> visibility_map;

    DungeonGrid<Entity*> entity_map;
    DungeonGrid<Item::StackNode> items;

    Entity pc;
    std::vector<Entity> npcs;
    std::vector<Item> items;

    uint32_t seed{ 0 };

};
