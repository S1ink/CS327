#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "util/vec_geom.h"
#include "util/heap.h"

#include "dungeon_config.h"
#include "entity.h"


enum CellType
{
    ROCK = 0,
    ROOM,
    CORRIDOR
};
enum StairType
{
    NO_STAIR = 0,
    STAIR_UP,
    STAIR_DOWN
};

typedef struct
{
    uint8_t type : 4;
    uint8_t is_stair : 2;
}
CellTerrain;

typedef struct
{
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
}
DungeonRoom;

int dungeon_room_collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b);
void dungeon_room_size(const DungeonRoom* r, Vec2u* s);
void print_dungeon_room(const DungeonRoom* r);


typedef struct
{
    CellTerrain terrain[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    uint8_t hardness[DUNGEON_Y_DIM][DUNGEON_X_DIM];

    DungeonRoom* rooms;

    uint16_t num_rooms;
    uint16_t num_up_stair;
    uint16_t num_down_stair;
}
DungeonMap;

int zero_dungeon_map(DungeonMap* d);
int generate_dungeon_map(DungeonMap* d, uint32_t seed);
int destruct_dungeon_map(DungeonMap* d);

int random_dungeon_map_floor_pos(DungeonMap* d, uint8_t* pos);

int serialize_dungeon_map(const DungeonMap* d, const Vec2u8* pc_pos, FILE* out);
int deserialize_dungeon_map(DungeonMap* d, Vec2u8* pc_pos, FILE* in);


typedef int32_t DungeonCostMap[DUNGEON_Y_DIM][DUNGEON_X_DIM];

typedef struct
{
    DungeonMap map;
    Heap entity_q;

    Entity* entities[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    DungeonCostMap tunnel_costs, terrain_costs;

    Entity* pc;
    Entity* entity_alloc;
    uint8_t num_monsters;
}
DungeonLevel;

int zero_dungeon_level(DungeonLevel* d);
int destruct_dungeon_level(DungeonLevel* d);

int init_dungeon_level(DungeonLevel* d, Vec2u8 pc_pos, size_t nmon);
int iterate_dungeon_level(DungeonLevel* d);

int print_dungeon_level(DungeonLevel* d, int border);
int print_dungeon_level_costmaps(DungeonLevel* d, int border);
