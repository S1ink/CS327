#pragma once

#include <vector>
#include <random>
#include <cstdint>

#include "util/vec_geom.hpp"
#include "util/heap.h"
#include "util/math.h"
#include "dungeon_config.h"


class DungeonLevel
{
public:
    template<typename T>
    using DungeonGrid = T[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    using DungeonCostMap = DungeonGrid<int32_t>;

    using SeedT = uint32_t;

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
            geom::Vec2u8 tl{ 0 }, br{ 0 };

            inline geom::Vec2u8 size() const { return tl - br + geom::Vec2u8{ 1 }; }
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
        void generate(SeedT s);

    };

public:

public:
    TerrainMap map;

};
