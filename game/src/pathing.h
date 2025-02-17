#pragma once

#include "dungeon.h"

#include "util/heap.h"
#include "util/vec_geom.h"

#include <stdint.h>


GENERATE_VEC2_TYPE(uint8_t, u8)

typedef struct
{
    HeapNode* hn;
    Vec2u8 pos;
    Vec2u8 from;
    int32_t cost;
}
CellPathNode;

typedef struct
{
    CellPathNode nodes[DUNGEON_Y_DIM][DUNGEON_X_DIM];
}
PathFindingBuffer;

int init_pathing_buffer(PathFindingBuffer* buff);

int dungeon_dijkstra_single_path(
    PathFindingBuffer* buff,
    Dungeon* d,
    Vec2u from,
    Vec2u to,
    int(*should_use_cell)(const Dungeon*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const Dungeon*, uint32_t x, uint32_t y),
    void(*on_path_cell)(Dungeon*, uint32_t x, uint32_t y),
    int use_diag );
int dungeon_dijkstra_traverse_grid(
    PathFindingBuffer* buff,
    Dungeon* d,
    Vec2u from,
    int(*should_use_cell)(const Dungeon*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const Dungeon*, uint32_t x, uint32_t y),
    int use_diag );

int dungeon_dijkstra_corridor_path(Dungeon* d, Vec2u from, Vec2u to);
int dungeon_dijkstra_traverse_floor(Dungeon* d, Vec2u from, PathFindingBuffer* buff);
int dungeon_dijkstra_traverse_terrain(Dungeon* d, Vec2u from, PathFindingBuffer* buff);
