#include "dungeon.hpp"

#include <cstdint>
#include <limits>


static int32_t cell_path_cost_cmp(const void* k, const void* w)
{
    return reinterpret_cast<const CellPathNode*>(k)->cost - reinterpret_cast<const CellPathNode*>(w)->cost;
}

int init_pathing_buffer(PathFindingBuffer buff)
{
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff[y][x].pos.x = x;
            buff[y][x].pos.y = y;
        }
    }
    return 0;
}

int dungeon_dijkstra_single_path(
    PathFindingBuffer buff,
    const DungeonLevel::TerrainMap& map,
    void* out,
    Vec2u8 from,
    Vec2u8 to,
    int(*should_use_cell)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int32_t(*cell_weight)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    void(*on_cell_path)(void*, uint8_t x, uint8_t y),
    int use_diag )
{
    CellPathNode *p;
    Heap h;
    uint8_t x, y;

    Vec2u8 iter8;

// RESET ALL WEIGHTS TO MAX
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff[y][x].cost = std::numeric_limits<int32_t>::max();
        }
    }
// INIT SRC NODE
    buff[from.y][from.x].cost = 0;
// CREATE HEAP
    heap_init(&h, cell_path_cost_cmp, NULL);
// CREATE HEAP NODES
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(should_use_cell(map, x, y))
            {
                buff[y][x].hn = heap_insert(&h, buff[y] + x);
            }
            else
            {
                buff[y][x].hn = NULL;
            }
        }
    }
// ALGO
    while((p = static_cast<CellPathNode*>(heap_remove_min(&h))))
    {
        p->hn = NULL;   // node was deleted from the heap

        if(p->pos == to)
        {
        // EXPORT PATH
            for(iter8 = to; iter8 != from; iter8 = p->from)
            {
                on_cell_path(out, iter8.x, iter8.y);
                p = &buff[iter8.y][iter8.x];
            }
            heap_delete(&h);
            return 0;
        }

        const int32_t p_cost = p->cost + cell_weight(map, p->pos.x, p->pos.y);

        // CHECK CARDINAL DIRECTIONS
    #define CHECK_NEIGHBOR(x, y) \
        if( (buff[(y)][(x)].hn) && (buff[(y)][(x)].cost > p_cost) ) \
        { \
            buff[(y)][(x)].cost = p_cost; \
            buff[(y)][(x)].from = p->pos; \
            heap_decrease_key_no_replace(&h, buff[(y)][(x)].hn); \
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
    #undef CHECK_NEIGHBOR
    }

    return -1;
}

int dungeon_dijkstra_traverse_grid(
    PathFindingBuffer buff,
    const DungeonLevel::TerrainMap& map,
    Vec2u8 from,
    int(*should_use_cell)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int32_t(*cell_weight)(const DungeonLevel::TerrainMap&, uint8_t x, uint8_t y),
    int use_diag )
{
    CellPathNode *p;
    Heap h;
    uint8_t x, y;

// RESET ALL WEIGHTS TO MAX
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff[y][x].cost = std::numeric_limits<int32_t>::max();
        }
    }
// INIT SRC NODE
    buff[from.y][from.x].cost = 0;
// CREATE HEAP
    heap_init(&h, cell_path_cost_cmp, NULL);
// CREATE HEAP NODES
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(should_use_cell(map, x, y))
            {
                buff[y][x].hn = heap_insert(&h, buff[y] + x);
            }
            else
            {
                buff[y][x].hn = NULL;
            }
        }
    }
// ALGO
    while((p = static_cast<CellPathNode*>(heap_remove_min(&h))))
    {
        p->hn = NULL;   // node was deleted from the heap

        int32_t p_cost;

        // CHECK CARDINAL DIRECTIONS
    #define CHECK_NEIGHBOR(x, y) \
        p_cost = p->cost + cell_weight(map, x, y); \
        if( (buff[(y)][(x)].hn) && (buff[(y)][(x)].cost > p_cost) ) \
        { \
            buff[(y)][(x)].cost = p_cost; \
            buff[(y)][(x)].from = p->pos; \
            heap_decrease_key_no_replace(&h, buff[(y)][(x)].hn); \
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
    #undef CHECK_NEIGHBOR
    }

    heap_delete(&h);
    return 0;
}

Vec2u8 dungeon_dijkstra_find_nearest(
    PathFindingBuffer buff,
    const DungeonLevel& l,
    Vec2u8 from,
    int(*should_use_cell)(const DungeonLevel&, uint8_t x, uint8_t y),
    int32_t(*cell_weight)(const DungeonLevel&, uint8_t x, uint8_t y),
    bool(*does_qualify)(const DungeonLevel&, uint8_t x, uint8_t y),
    int use_diag )
{
    CellPathNode *p;
    Heap h;
    uint8_t x, y;

// RESET ALL WEIGHTS TO MAX
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            buff[y][x].cost = std::numeric_limits<int32_t>::max();
        }
    }
// INIT SRC NODE
    buff[from.y][from.x].cost = 0;
// CREATE HEAP
    heap_init(&h, cell_path_cost_cmp, NULL);
// CREATE HEAP NODES
    for(y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(should_use_cell(l, x, y))
            {
                buff[y][x].hn = heap_insert(&h, buff[y] + x);
            }
            else
            {
                buff[y][x].hn = NULL;
            }
        }
    }
// ALGO
    while((p = static_cast<CellPathNode*>(heap_remove_min(&h))))
    {
        if(does_qualify(l, p->pos.x, p->pos.y))
        {
            return p->pos;
        }

        p->hn = NULL;   // node was deleted from the heap

        int32_t p_cost;

        // CHECK CARDINAL DIRECTIONS
    #define CHECK_NEIGHBOR(x, y) \
        p_cost = p->cost + cell_weight(l, x, y); \
        if( (buff[(y)][(x)].hn) && (buff[(y)][(x)].cost > p_cost) ) \
        { \
            buff[(y)][(x)].cost = p_cost; \
            buff[(y)][(x)].from = p->pos; \
            heap_decrease_key_no_replace(&h, buff[(y)][(x)].hn); \
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
    #undef CHECK_NEIGHBOR
    }

    heap_delete(&h);
    return Vec2u8{ 0, 0 };
}





namespace reuse
{
    static PathFindingBuffer buff;
    static bool init = false;
};


static int corridor_path_should_use(const DungeonLevel::TerrainMap& map, uint8_t x, uint8_t y)
{
    return map.hardness[y][x] != 0xFF;
}
static int32_t corridor_path_cell_weight(const DungeonLevel::TerrainMap& map, uint8_t x, uint8_t y)
{
    return (int32_t)map.hardness[y][x];
}
static void corridor_path_export(void* d, uint8_t x, uint8_t y)
{
    reinterpret_cast<DungeonLevel::TerrainMap*>(d)->terrain[y][x].type = DungeonLevel::TerrainMap::CELLTYPE_CORRIDOR;
}

int dungeon_dijkstra_corridor_path(DungeonLevel::TerrainMap& map, Vec2u8 from, Vec2u8 to)
{
    if(!reuse::init)
    {
        init_pathing_buffer(reuse::buff);
        reuse::init = true;
    }

    return dungeon_dijkstra_single_path(
        reuse::buff, map, &map, from, to,
        corridor_path_should_use,
        corridor_path_cell_weight,
        corridor_path_export,
        false );
}



static int floor_traversal_should_use(const DungeonLevel::TerrainMap& map, uint8_t x, uint8_t y)
{
    return map.terrain[y][x].type != DungeonLevel::TerrainMap::CELLTYPE_ROCK;
}
static int32_t floor_traversal_cell_weight(const DungeonLevel::TerrainMap& d, uint8_t x, uint8_t y)
{
    return 1;
}

int dungeon_dijkstra_traverse_floor(DungeonLevel::TerrainMap& map, Vec2u8 from, PathFindingBuffer buff)
{
    return dungeon_dijkstra_traverse_grid(
        buff, map, from,
        floor_traversal_should_use,
        floor_traversal_cell_weight,
        true );
}



static int open_entity_cell_should_use(const DungeonLevel& l, uint8_t x, uint8_t y)
{
    return floor_traversal_should_use(l.map, x, y);
}
static int32_t open_entity_cell_weight(const DungeonLevel& l, uint8_t x, uint8_t y)
{
    return 1;
}
static bool open_entity_cell_does_qualify(const DungeonLevel& l, uint8_t x, uint8_t y)
{
    return !l.item_map[y][x];
}

Vec2u8 dungeon_dijkstra_nearest_open_drop(DungeonLevel& l, Vec2u8 from)
{
    if(!reuse::init)
    {
        init_pathing_buffer(reuse::buff);
        reuse::init = true;
    }

    return dungeon_dijkstra_find_nearest(
        reuse::buff,
        l,
        from,
        open_entity_cell_should_use,
        open_entity_cell_weight,
        open_entity_cell_does_qualify,
        true );
}



static int terrain_traversal_should_use(const DungeonLevel::TerrainMap& map, uint8_t x, uint8_t y)
{
    return map.hardness[y][x] != 0xFF;
}
static int32_t terrain_traversal_cell_weight(const DungeonLevel::TerrainMap& map, uint8_t x, uint8_t y)
{
    return map.terrain[y][x].type == DungeonLevel::TerrainMap::CELLTYPE_ROCK ? (1 + map.hardness[y][x] / 85) : 1;
}

int dungeon_dijkstra_traverse_terrain(DungeonLevel::TerrainMap& map, Vec2u8 from, PathFindingBuffer buff)
{
    return dungeon_dijkstra_traverse_grid(
        buff, map, from,
        terrain_traversal_should_use,
        terrain_traversal_cell_weight,
        true );
}



int dungeon_dijkstra_floor_path(
    DungeonLevel::TerrainMap& map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) )
{
    if(!reuse::init)
    {
        init_pathing_buffer(reuse::buff);
        reuse::init = true;
    }

    return dungeon_dijkstra_single_path(
        reuse::buff, map, out, from, to,
        floor_traversal_should_use,
        floor_traversal_cell_weight,
        on_path_cell,
        true );
}
int dungeon_dijkstra_terrain_path(
    DungeonLevel::TerrainMap& map,
    Vec2u8 from, Vec2u8 to,
    void* out, void(*on_path_cell)(void*, uint8_t x, uint8_t y) )
{
    if(!reuse::init)
    {
        init_pathing_buffer(reuse::buff);
        reuse::init = true;
    }

    return dungeon_dijkstra_single_path(
        reuse::buff, map, out, from, to,
        terrain_traversal_should_use,
        terrain_traversal_cell_weight,
        on_path_cell,
        true );
}
