#pragma once

#include "dungeon.h"

#include "util/heap.h"
#include "util/vec_geom.h"

#include <stdint.h>


typedef struct
{
    HeapNode* hn;
    Vec2u8 pos;
    Vec2u8 from;
    int32_t cost;
}
CellPathNode;

typedef CellPathNode PathFindingBuffer[DUNGEON_Y_DIM][DUNGEON_X_DIM];

int init_pathing_buffer(PathFindingBuffer buff);

int dungeon_dijkstra_single_path(
    PathFindingBuffer buff,
    DungeonMap* d,
    void* out,
    Vec2u from,
    Vec2u to,
    int(*should_use_cell)(const DungeonMap*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const DungeonMap*, uint32_t x, uint32_t y),
    void(*on_path_cell)(void*, uint32_t x, uint32_t y),
    int use_diag );
int dungeon_dijkstra_traverse_grid(
    PathFindingBuffer buff,
    DungeonMap* d,
    Vec2u from,
    int(*should_use_cell)(const DungeonMap*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const DungeonMap*, uint32_t x, uint32_t y),
    int use_diag );

int dungeon_dijkstra_corridor_path(DungeonMap* d, Vec2u from, Vec2u to);
int dungeon_dijkstra_traverse_floor(DungeonMap* d, Vec2u from, PathFindingBuffer buff);
int dungeon_dijkstra_traverse_terrain(DungeonMap* d, Vec2u from, PathFindingBuffer buff);

int dungeon_dijkstra_floor_path(
    DungeonMap* d,
    Vec2u from, Vec2u to,
    void* out, void(*on_path_cell)(void*, uint32_t x, uint32_t y) );
int dungeon_dijkstra_terrain_path(
    DungeonMap* d,
    Vec2u from, Vec2u to,
    void* out, void(*on_path_cell)(void*, uint32_t x, uint32_t y) );
