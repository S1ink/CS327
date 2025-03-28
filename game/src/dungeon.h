#pragma once

#include <stdint.h>
#include <stdio.h>

#include "util/vec_geom.h"
#include "util/heap.h"
#include "util/math.h"

#include "dungeon_config.h"
#include "entity.h"


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

class CellTerrain
{
public:
    uint8_t type : REQUIRED_BITS32(CELLTYPE_MAX_VALUE - 1);
    uint8_t is_stair : REQUIRED_BITS32(STAIR_MAX_VALUE - 1);
};

class DungeonRoom
{
public:
    Vec2u tl;
    Vec2u br;

    /* `tl` denotes the TOP LEFT cell that is contained in the room, and
     * `br` denotes the BOTTOM RIGHT cell that is contained in the room -- ex.
     * 
     *   TL.....
     *    ......
     *    .....BR
     * 
     *    ^ A 6x3-cell room! */
};

int dungeon_room_collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b);
void dungeon_room_size(const DungeonRoom* r, Vec2u* s);
void print_dungeon_room(const DungeonRoom* r);


class DungeonMap
{
public:
    CellTerrain terrain[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    uint8_t hardness[DUNGEON_Y_DIM][DUNGEON_X_DIM];

    DungeonRoom* rooms;

    uint16_t num_rooms;
    uint16_t num_up_stair;
    uint16_t num_down_stair;
};

int zero_dungeon_map(DungeonMap* d);
int generate_dungeon_map(DungeonMap* d, uint32_t seed);
int destruct_dungeon_map(DungeonMap* d);

int random_dungeon_map_floor_pos(DungeonMap* d, uint8_t* pos);

int serialize_dungeon_map(const DungeonMap* d, const Vec2u8* pc_pos, FILE* out);
int deserialize_dungeon_map(DungeonMap* d, Vec2u8* pc_pos, FILE* in);


using DungeonCostMap = int32_t[DUNGEON_Y_DIM][DUNGEON_X_DIM];

union LevelStatus
{
public:
    struct
    {
        uint8_t has_won : 1;
        uint8_t has_lost : 1;
    };
    uint8_t data : 2;
};

class DungeonLevel
{
public:
    DungeonMap map;
    Heap entity_q;

    Entity* entities[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    DungeonCostMap tunnel_costs, terrain_costs;

    Entity* pc;             // the PC's entity address -- set to null when eaten (lose)
    Entity* entity_alloc;   // all the entities are allocated contiguously
    uint8_t num_monsters;   // counts the number of remaining monsters (0 = win)
};

int zero_dungeon_level(DungeonLevel* d);
int init_dungeon_level(DungeonLevel* d, Vec2u8 pc_pos, size_t nmon);
int destruct_dungeon_level(DungeonLevel* d);

// LevelStatus iterate_dungeon_level(DungeonLevel* d, int until_next_pc_move);
static inline LevelStatus get_dungeon_level_status(DungeonLevel* d)
{
    LevelStatus s;
    s.has_won = !d->num_monsters;
    s.has_lost = !d->pc;
    return s;
}





// ------------------------------------------------------
static inline char get_terrain_char(CellTerrain c)
{
    switch(c.is_stair)
    {
        default: break;
        case STAIR_UP: return '<';
        case STAIR_DOWN: return '>';
    }
    switch(c.type)
    {
        default: break;
        case CELLTYPE_CORRIDOR: return '#';
        case CELLTYPE_ROOM: return '.';
    }
    return ' ';
}
static inline char get_cell_char(CellTerrain c, const Entity* e)
{
    if(e) return get_entity_char(e);
    return get_terrain_char(c);
}
