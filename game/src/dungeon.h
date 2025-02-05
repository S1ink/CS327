#pragma once

#include <stdint.h>

#include "util/list.h"
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

GENERATE_LIST_STRUCT(DungeonRoom, Room, room)


typedef struct Dungeon
{
    DungeonCell cells[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    RoomList rooms_list;

    char printable[DUNGEON_Y_DIM][DUNGEON_X_DIM];
}
Dungeon;

int generate_dungeon(Dungeon* d, uint32_t seed);
int destruct_dungeon(Dungeon* d);
