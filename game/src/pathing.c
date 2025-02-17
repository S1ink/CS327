#include "pathing.h"

#include <limits.h>


/* DIJKSTRA */

static int32_t cell_path_cost_cmp(const void* k, const void* w)
{
    return ((CellPathNode*)k)->cost - ((CellPathNode*)w)->cost;
}

int init_pathing_buffer(PathFindingBuffer* buff)
{
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff->nodes[y][x].pos.x = x;
            buff->nodes[y][x].pos.y = y;
        }
    }
    return 0;
}

int dungeon_dijkstra_single_path(
    PathFindingBuffer* buff,
    Dungeon* d,
    Vec2u from,
    Vec2u to,
    int(*should_use_cell)(const Dungeon*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const Dungeon*, uint32_t x, uint32_t y),
    void(*on_cell_path)(Dungeon*, uint32_t x, uint32_t y),
    int use_diag )
{
    CellPathNode *p;
    Heap h;
    uint32_t x, y;

    Vec2u8 from8, to8, iter8;
    vec2u8_assign(&from8, (uint8_t)from.x, (uint8_t)from.y);
    vec2u8_assign(&to8, (uint8_t)to.x, (uint8_t)to.y);

// RESET ALL WEIGHTS TO MAX
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff->nodes[y][x].cost = INT_MAX;
        }
    }
// INIT SRC NODE
    buff->nodes[from.y][from.x].cost = 0;
// CREATE HEAP
    heap_init(&h, cell_path_cost_cmp, NULL);
// CREATE HEAP NODES
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(should_use_cell(d, x, y))
            {
                buff->nodes[y][x].hn = heap_insert(&h, buff->nodes[y] + x);
            }
            else
            {
                buff->nodes[y][x].hn = NULL;
            }
        }
    }
// ALGO
    while((p = heap_remove_min(&h)))
    {
        p->hn = NULL;   // node was deleted from the heap

        if(vec2u8_equal(&p->pos, &to8))
        {
        // EXPORT PATH
            for(vec2u8_copy(&iter8, &to8); !vec2u8_equal(&iter8, &from8); vec2u8_copy(&iter8, &p->from))
            {
                on_cell_path(d, iter8.x, iter8.y);
                p = &buff->nodes[iter8.y][iter8.x];
            }
            heap_delete(&h);
            return 0;
        }

        const int32_t p_cost = p->cost + cell_weight(d, p->pos.x, p->pos.y);

        // CHECK CARDINAL DIRECTIONS
    #define CHECK_NEIGHBOR(x, y) \
        if( (buff->nodes[(y)][(x)].hn) && (buff->nodes[(y)][(x)].cost > p_cost) ) \
        { \
            buff->nodes[(y)][(x)].cost = p_cost; \
            vec2u8_copy(&buff->nodes[(y)][(x)].from, &p->pos); \
            heap_decrease_key_no_replace(&h, buff->nodes[(y)][(x)].hn); \
        }

        CHECK_NEIGHBOR(p->pos.x, p->pos.y - 1)
        CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y)
        CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y)
        CHECK_NEIGHBOR(p->pos.x, p->pos.y + 1)
        if(use_diag)
        {
            CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y - 1)
            CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y + 1)
            CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y - 1)
            CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y + 1)
        }
    }

    return -1;
}

int dungeon_dijkstra_traverse_grid(
    PathFindingBuffer* buff,
    Dungeon* d,
    Vec2u from,
    int(*should_use_cell)(const Dungeon*, uint32_t x, uint32_t y),
    int32_t(*cell_weight)(const Dungeon*, uint32_t x, uint32_t y),
    int use_diag )
{
    CellPathNode *p;
    Heap h;
    uint32_t x, y;

// RESET ALL WEIGHTS TO MAX
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff->nodes[y][x].cost = INT_MAX;
        }
    }
// INIT SRC NODE
    buff->nodes[from.y][from.x].cost = 0;
// CREATE HEAP
    heap_init(&h, cell_path_cost_cmp, NULL);
// CREATE HEAP NODES
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(should_use_cell(d, x, y))
            {
                buff->nodes[y][x].hn = heap_insert(&h, buff->nodes[y] + x);
            }
            else
            {
                buff->nodes[y][x].hn = NULL;
            }
        }
    }
// ALGO
    while((p = heap_remove_min(&h)))
    {
        p->hn = NULL;   // node was deleted from the heap

        const int32_t p_cost = p->cost + cell_weight(d, p->pos.x, p->pos.y);

        // CHECK CARDINAL DIRECTIONS
    #define CHECK_NEIGHBOR(x, y) \
        if( (buff->nodes[(y)][(x)].hn) && (buff->nodes[(y)][(x)].cost > p_cost) ) \
        { \
            buff->nodes[(y)][(x)].cost = p_cost; \
            vec2u8_copy(&buff->nodes[(y)][(x)].from, &p->pos); \
            heap_decrease_key_no_replace(&h, buff->nodes[(y)][(x)].hn); \
        }

        CHECK_NEIGHBOR(p->pos.x, p->pos.y - 1)
        CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y)
        CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y)
        CHECK_NEIGHBOR(p->pos.x, p->pos.y + 1)
        if(use_diag)
        {
            CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y - 1)
            CHECK_NEIGHBOR(p->pos.x - 1, p->pos.y + 1)
            CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y - 1)
            CHECK_NEIGHBOR(p->pos.x + 1, p->pos.y + 1)
        }
    }

    heap_delete(&h);
    return 0;
}



static int corridor_path_should_use(const Dungeon* d, uint32_t x, uint32_t y)
{
    return d->hardness[y][x] != 0xFF;
}
static int32_t corridor_path_cell_weight(const Dungeon* d, uint32_t x, uint32_t y)
{
    return (int32_t)d->hardness[y][x];
}
static void corridor_path_export(Dungeon* d, uint32_t x, uint32_t y)
{
    d->terrain[y][x].type = CORRIDOR;
}

int dungeon_dijkstra_corridor_path(Dungeon* d, Vec2u from, Vec2u to)
{
    static PathFindingBuffer buff;
    static uint32_t init = 0;

    if(!init) init_pathing_buffer(&buff);

    return dungeon_dijkstra_single_path(
        &buff, d, from, to,
        corridor_path_should_use,
        corridor_path_cell_weight,
        corridor_path_export,
        0 );
}



static int floor_traversal_should_use(const Dungeon* d, uint32_t x, uint32_t y)
{
    return d->terrain[y][x].type != ROCK;
}
static int32_t floor_traversal_cell_weight(const Dungeon* d, uint32_t x, uint32_t y)
{
    return 1;
}

int dungeon_dijkstra_traverse_floor(Dungeon* d, Vec2u from, PathFindingBuffer* buff)
{
    return dungeon_dijkstra_traverse_grid(
        buff, d, from,
        floor_traversal_should_use,
        floor_traversal_cell_weight,
        1 );
}



static int terrain_traversal_should_use(const Dungeon* d, uint32_t x, uint32_t y)
{
    return d->hardness[y][x] != 0xFF;
}
static int32_t terrain_traversal_cell_weight(const Dungeon* d, uint32_t x, uint32_t y)
{
    return d->terrain[y][x].type == ROCK ? (1 + d->hardness[y][x] / 85) : 1;
}

int dungeon_dijkstra_traverse_terrain(Dungeon* d, Vec2u from, PathFindingBuffer* buff)
{
    return dungeon_dijkstra_traverse_grid(
        buff, d, from,
        terrain_traversal_should_use,
        terrain_traversal_cell_weight,
        1 );
}
