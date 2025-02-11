#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "util/vec_geom.h"

#include "dungeon_config.h"


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

typedef struct DungeonCell
{
    uint8_t type : 4;
    uint8_t is_stair : 2;
    uint8_t hardness : 8;
}
DungeonCell;

typedef struct DungeonRoom
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


typedef struct Dungeon
{
    DungeonCell cells[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    DungeonRoom* rooms;
    uint16_t num_rooms;
    uint16_t num_up_stair;
    uint16_t num_down_stair;

    char printable[DUNGEON_Y_DIM][DUNGEON_X_DIM];
}
Dungeon;

int generate_dungeon(Dungeon* d, uint32_t seed);
int zero_dungeon(Dungeon* d);
int destruct_dungeon(Dungeon* d);

int print_dungeon(Dungeon* d, int border);
int serialize_dungeon(const Dungeon* d, FILE* out, const uint8_t* pc_loc);
int deserialize_dungeon(Dungeon* d, FILE* in, uint8_t* pc_loc);
