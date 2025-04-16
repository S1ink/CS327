#pragma once

#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include "util/vec_geom.hpp"
#include "util/math.hpp"
#include "util/heap.h"

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

            inline Vec2u8 size() const { return br - tl + Vec2u8{ 1 }; }
            bool collides(const Room& r) const;
        };

    public:
        DungeonGrid<Cell> terrain;
        DungeonGrid<uint8_t> hardness;

        std::vector<Room> rooms;

        uint16_t num_up_stair{ 0 }, num_down_stair{ 0 };

    public:
        inline TerrainMap() { this->reset(); }
        inline ~TerrainMap() = default;

        void reset();
        void generate(uint32_t seed);
        inline void generateClean(uint32_t seed)
        {
            this->reset();
            this->generate(seed);
        }

    };

public:
    inline DungeonLevel() :
        pc{ Entity::PCGenT{} }
    {}

    inline void setSeed(uint32_t s) { this->rgen.seed(s); }

    int loadTerrain(FILE* f);
    int saveTerrain(FILE* f);
    int generateTerrain();

    int updateCosts(bool both_or_only_terrain);

public:
    TerrainMap map;
    DungeonCostMap tunnel_costs, terrain_costs;
    DungeonGrid<char> visibility_map;

    DungeonGrid<Entity*> entity_map;
    DungeonGrid<Item::StackNode> items;

    Entity pc;
    std::vector<Entity> npcs;
    std::vector<Item> items;

    // uint32_t seed{ 0 };
    std::mt19937 rgen;

};




class CellPathNode
{
public:
    HeapNode* hn;
    Vec2u8 pos;
    Vec2u8 from;
    int32_t cost;
};

using PathFindingBuffer = DungeonLevel::DungeonGrid<CellPathNode>;

int init_pathing_buffer(PathFindingBuffer buff);

int dungeon_dijkstra_single_path(
    PathFindingBuffer buff,
    DungeonLevel::TerrainMap& map,
    void* out,
    Vec2u8 from,
    Vec2u8 to,
    int(*should_use_cell)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int32_t(*cell_weight)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    void(*on_cell_path)(void*, uint8_t x, uint8_t y),
    int use_diag );
int dungeon_dijkstra_traverse_grid(
    PathFindingBuffer buff,
    DungeonLevel::TerrainMap& map,
    Vec2u8 from,
    int(*should_use_cell)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int32_t(*cell_weight)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int use_diag );

int dungeon_dijkstra_corridor_path(DungeonLevel::TerrainMap& map, Vec2u8 from, Vec2u8 to);
int dungeon_dijkstra_traverse_floor(DungeonLevel::TerrainMap& map, Vec2u8 from, PathFindingBuffer buff);
int dungeon_dijkstra_traverse_terrain(DungeonLevel::TerrainMap& map, Vec2u8 from, PathFindingBuffer buff);

int dungeon_dijkstra_floor_path(
    DungeonLevel::TerrainMap* map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) );
int dungeon_dijkstra_terrain_path(
    DungeonLevel::TerrainMap* map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) );
