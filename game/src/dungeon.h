#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "util/vec_geom.h"

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

int collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b);
int in_x_window(const DungeonRoom* a, const DungeonRoom* b);
int in_y_window(const DungeonRoom* a, const DungeonRoom* b);
void room_size(const DungeonRoom* r, Vec2u* s);
void print_room(const DungeonRoom* r);


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

int generate_dungeon_map(DungeonMap* d, uint32_t seed);
int zero_dungeon_map(DungeonMap* d);
int destruct_dungeon_map(DungeonMap* d);

int random_dungeon_map_floor_pos(DungeonMap* d, uint8_t* pos);


typedef struct
{
    DungeonMap map;
    Entity* entities[DUNGEON_Y_DIM][DUNGEON_X_DIM];

    Vec2u8 pc_position;
}
DungeonLevel;


int print_dungeon_level(DungeonMap* d, uint8_t* pc_loc, int border);
int print_dungeon_level_a3(DungeonMap* d, uint8_t* pc_loc, int border);
int serialize_dungeon_level(const DungeonMap* d, FILE* out, const uint8_t* pc_loc);
int deserialize_dungeon_level(DungeonMap* d, FILE* in, uint8_t* pc_loc);
