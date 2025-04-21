#pragma once

#include <type_traits>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>
#include <queue>

#include <ncurses.h>

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

    template<typename T, typename I>
    static inline T& accessGridElem(DungeonLevel::DungeonGrid<T>& grid, const geom::Vec2_<I>& p)
    {
        static_assert(std::is_integral<I>::value);

        return grid[p.y][p.x];
    }

    static inline constexpr int8_t VIS_OFFSETS[21][2] =   // (Y, X)
    {
        { -2, -1 },
        { -2, 0 },
        { -2, 1 },

        { -1, -2 },
        { -1, -1 },
        { -1, 0 },
        { -1, 1 },
        { -1, 2 },

        { 0, -2 },
        { 0, -1 },
        { 0, 0 },
        { 0, 1 },
        { 0, 2 },

        { 1, -2 },
        { 1, -1 },
        { 1, 0 },
        { 1, 1 },
        { 1, 2 },

        { 2, -1 },
        { 2, 0 },
        { 2, 1 },
    };
    static inline constexpr uint8_t VIS_RADSQ = 5;

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
            inline bool isRock() const { return !this->isFloor(); }
        };
        struct Room
        {
            Vec2u8 tl{ 0, 0 }, br{ 0, 0 };

            inline Vec2u8 size() const { return br - tl + Vec2u8{ 1, 1 }; }
            bool collides(const Room& r) const;
        };

    public:
        DungeonGrid<Cell> terrain;
        DungeonGrid<uint8_t> hardness;

        std::vector<Room> rooms;

        uint16_t num_up_stair{ 0 }, num_down_stair{ 0 };

    public:
        inline TerrainMap() = default;
        inline ~TerrainMap() = default;

        void reset();
        void generate(uint32_t seed);
        inline void generateClean(uint32_t seed)
        {
            this->reset();
            this->generate(seed);
        }

        template<typename G = std::mt19937>
        inline Vec2u8 randomRoomFloorPos(G& gen)
        {
            std::uniform_int_distribution<size_t>
                room_idx_distribution{ 0, this->rooms.size() - 1 };

            const Room& room = this->rooms[room_idx_distribution(gen)];
            return Vec2u8::randomInRange(room.tl, room.br, gen);
        }

    };

    struct EntityQueueNode
    {
        inline EntityQueueNode(Entity* e, size_t n, uint8_t p) :
            e{ e }, next_turn{ n }, priority{ p }
        {}

        Entity* e{ nullptr };
        size_t next_turn{ 0 };
        uint8_t priority{ 0 };

        inline bool operator<(const EntityQueueNode& other) const
        {
            return !((this->next_turn < other.next_turn) ||
                (this->next_turn == other.next_turn && this->priority < other.priority));
        }
    };

public:
    inline DungeonLevel() :
        pc{ Entity::PCGenT{} },
        rroll{ std::random_device{}() }
    {
        this->reset();
    }

    inline void setSeed(uint32_t s) { this->rgen.seed(s); }
    inline int getWinLose()
    {
        return (this->pc.state.health <= 0) ? -1 : (this->npcs_remaining > 0 ? 0 : 1);
    }

    void reset();

    int loadTerrain(FILE* f);
    int saveTerrain(FILE* f);
    int generateTerrain();

    int updateCosts(bool both_or_only_terrain = true);
    int copyVisCells();

    int handlePCMove(Vec2u8 to, bool is_goto);
    int iterateNPC(Entity& e);

    void writeChar(WINDOW* win, Vec2u8 loc);

public:
    TerrainMap map;
    DungeonCostMap tunnel_costs, terrain_costs;
    DungeonGrid<char> visibility_map;

    DungeonGrid<Entity*> entity_map;
    DungeonGrid<Item*> item_map;

    std::priority_queue<EntityQueueNode> entity_queue;

    Entity pc;
    std::vector<Entity> npcs;
    std::vector<Item> items;

    size_t npcs_remaining;

    // uint32_t seed{ 0 };
    std::mt19937 rgen;
    std::mt19937 rroll;

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
    DungeonLevel::TerrainMap& map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) );
int dungeon_dijkstra_terrain_path(
    DungeonLevel::TerrainMap& map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) );
