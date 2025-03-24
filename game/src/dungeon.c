#include "dungeon.h"
#include "dungeon_config.h"
#include "pathing.h"

#include "util/perlin.h"
#include "util/debug.h"
#include "util/math.h"
#include "util/heap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>
#include <limits.h>
#include <time.h>
#include <math.h>


/* --- PARETER FUNCTIONS ---------------------------------------------- */

static int32_t entity_priority_comp(const void* k, const void* w)
{
    Entity *a, *b;
    a = (Entity*)k;
    b = (Entity*)w;
    return (a->next_turn == b->next_turn) ? (int32_t)a->priority - (int32_t)b->priority : (int32_t)a->next_turn - (int32_t)b->next_turn;
}

/* --- UTILITIES ------------------------------------------------------ */

static inline void vec2u_random_in_range(Vec2u* v, Vec2u min, Vec2u max)
{
    v->x = (uint32_t)RANDOM_IN_RANGE(min.x, max.x);
    v->y = (uint32_t)RANDOM_IN_RANGE(min.y, max.y);
}

static int dungeon_map_zero_cells(DungeonMap* d)
{
    for(size_t i = 0; i < DUNGEON_Y_DIM; i++)
    {
        memset(d->terrain[i], 0x0, sizeof(d->terrain[i]));
        d->hardness[i][0] = d->hardness[i][DUNGEON_X_DIM - 1] = 0xFF;
    }
    for(size_t i = 1; i < DUNGEON_X_DIM - 1; i++)
    {
        d->hardness[0][i] = d->hardness[DUNGEON_Y_DIM - 1][i] = 0xFF;
    }

    return 0;
}
static int dungeon_map_connect_rooms(DungeonMap* d)
{
    for(uint16_t i = 0; i < d->num_rooms; i++)
    {
        const uint16_t i2 = (i + 1) % d->num_rooms;
        Vec2u pos_r1, pos_r2;

        vec2u_random_in_range(&pos_r1, d->rooms[i].tl,  d->rooms[i].br );
        vec2u_random_in_range(&pos_r2, d->rooms[i2].tl, d->rooms[i2].br);

        dungeon_dijkstra_corridor_path(d, pos_r1, pos_r2);
    }

    return 0;
}
static int dungeon_map_fill_room_cells(DungeonMap* d)
{
    for(size_t r = 0; r < d->num_rooms; r++)
    {
        const DungeonRoom* room = d->rooms + r;
    #if ENABLE_DEBUG_PRINTS
        print_dungeon_room(room);
    #endif
        for(size_t y = room->tl.y; y <= room->br.y; y++)
        {
            for(size_t x = room->tl.x; x <= room->br.x; x++)
            {
                d->terrain[y][x].type = ROOM;
            }
        }
    }

    return 0;
}

static int dungeon_map_generate_rooms(DungeonMap* d)
{
    const size_t target = (size_t)RANDOM_IN_RANGE(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
    if(d->num_rooms && d->rooms)
        d->rooms = realloc(d->rooms, sizeof(*d->rooms) * target);
    else
        d->rooms = malloc(sizeof(*d->rooms) * target);

    if(!d->rooms)
        return -1;

    d->num_rooms = target;

    Vec2u d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max,
        (DUNGEON_X_DIM - DUNGEON_ROOM_MIN_X - 1),
        (DUNGEON_Y_DIM - DUNGEON_ROOM_MIN_Y - 1) );

// 1. GENERATE AT LEAST THE MINIMUM NUMBER OF ROOMS
    size_t iter = 0;
    for(size_t i = 0; i < DUNGEON_MIN_NUM_ROOMS; iter++)
    {
        vec2u_random_in_range(&d->rooms[i].tl, d_min, d_max);
        d->rooms[i].br.x = d->rooms[i].tl.x + (DUNGEON_ROOM_MIN_X - 1);
        d->rooms[i].br.y = d->rooms[i].tl.y + (DUNGEON_ROOM_MIN_Y - 1);

        if(i == 0)
        {
            i++;
            continue;
        }

        int failed = 0;
        for(size_t j = 0; j < i; j++)
        {
            if(dungeon_room_collide_or_tangent(d->rooms + i, d->rooms + j))
            {
                failed = 1;
                break;
            }
        }
        if(!failed) i++;
    }
    PRINT_DEBUG("Took %lu iterations to generate core rooms.\n", iter)

// 2. GENERATE EXTRA ROOMS
    size_t R = DUNGEON_MIN_NUM_ROOMS;
    for(size_t i = DUNGEON_MIN_NUM_ROOMS; i < target; i++)
    {
        vec2u_random_in_range(&d->rooms[R].tl, d_min, d_max);
        d->rooms[R].br.x = d->rooms[R].tl.x + (DUNGEON_ROOM_MIN_X - 1);
        d->rooms[R].br.y = d->rooms[R].tl.y + (DUNGEON_ROOM_MIN_Y - 1);

        int success = 1;
        for(size_t j = 0; j < R; j++)
        {
            if(dungeon_room_collide_or_tangent(d->rooms + R, d->rooms + j))
            {
                success = 0;
                break;
            }
        }
        R += success;
    }
    PRINT_DEBUG("Total dungeons generated: %lu\n", R)

    d->rooms = realloc(d->rooms, sizeof(*d->rooms) * R);
    if(!d->rooms) return -1;
    d->num_rooms = R;

// 3. EXPAND DIMENSIONS
    Vec2u r_min, r_max;
    vec2u_assign(&r_min, DUNGEON_ROOM_MIN_X, DUNGEON_ROOM_MIN_Y);
    vec2u_assign(&r_max, DUNGEON_ROOM_MAX_X, DUNGEON_ROOM_MAX_Y);
    for(size_t r = 0; r < R; r++)
    {
        DungeonRoom* dr = d->rooms + r;

        Vec2u target_size;
        vec2u_random_in_range(&target_size, r_min, r_max);

        int status = 0;

        Vec2u size;
        dungeon_room_size(dr, &size);
        size_t iter = 0;
        for(uint8_t b = 0; status < 0b1111 && size.x < target_size.x && size.y < target_size.y; b = !b)
        {
        #define CHECK_COLLISIONS(K, reset) \
            for(size_t i = 1; i < R; i++) \
            {   \
                size_t rr = (r + i) % R; \
                if(dungeon_room_collide_or_tangent(dr, d->rooms + rr)) \
                {   \
                    status |= (K); \
                    reset; \
                    break; \
                } \
            }

            if((b && status < 0b11) || (status & 0b1100))
            {
                if(dr->br.x >= (DUNGEON_X_DIM - 2)) status |= 0b0001;
                if(!(status & 0b0001))
                {
                    dr->br.x += 1;
                    CHECK_COLLISIONS(0b0001, dr->br.x -= 1)
                }
                if(dr->br.y >= (DUNGEON_Y_DIM - 2)) status |= 0b0010;
                if(!(status & 0b0010))
                {
                    dr->br.y += 1;
                    CHECK_COLLISIONS(0b0010, dr->br.y -= 1)
                }
            }
            else
            {
                if(dr->tl.x <= 2) status |= 0b0100;
                if(!(status & 0b0100) && dr->tl.x > 2)
                {
                    dr->tl.x -= 1;
                    CHECK_COLLISIONS(0b0100, dr->tl.x += 1)
                }
                if(dr->tl.y <= 2) status |= 0b1000;
                if(!(status & 0b1000) && dr->tl.y > 2)
                {
                    dr->tl.y -= 1;
                    CHECK_COLLISIONS(0b1000, dr->tl.y += 1)
                }
            }

            dungeon_room_size(dr, &size);

            iter++;
            if(iter > 10000) break;

        #undef CHECK_COLLISIONS
        }
    }

    dungeon_map_connect_rooms(d);

    return dungeon_map_fill_room_cells(d);
}
static int dungeon_map_place_stairs(DungeonMap* d)
{
    Vec2u p, d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max, (DUNGEON_X_DIM - 2), (DUNGEON_Y_DIM - 2) );

    for(;;)
    {
        vec2u_random_in_range(&p, d_min, d_max);
        CellTerrain* c = d->terrain[p.y] + p.x;
        if(c->type > 0)
        {
            c->is_stair = STAIR_UP;
            break;
        }
    }
    for(;;)
    {
        vec2u_random_in_range(&p, d_min, d_max);
        CellTerrain* c = d->terrain[p.y] + p.x;
        if(c->type > 0 && c->is_stair == 0)
        {
            c->is_stair = STAIR_DOWN;
            break;
        }
    }

    d->num_up_stair = 1;
    d->num_down_stair = 1;

    return 0;
}

static int dungeon_level_print_costs(DungeonCostMap costs, int border)
{
    char* row_fmt = "%.*s\n";

    if(border)
    {
        row_fmt = "|%.*s|\n";
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    for(uint32_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        char row_chars[DUNGEON_X_DIM];
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            const int32_t w = costs[y][x];
            row_chars[x] = w == INT_MAX ? ' ' : w == 0 ? '@' : (w % 10) + '0';
        }

        uint32_t i = 0;
    #if DUNGEON_PRINT_HARDNESS
        char row[20 * DUNGEON_X_DIM + 5];   // "\033[48;2;<3>;127;127m<1>" for each cell + reset code + null termination
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            if(row_chars[x] == ' ') // dont print a color
            {
                i += sprintf(row + i, "\033[0m ");
            }
            else
            {
                const uint8_t w = (uint8_t)MIN_CACHED((costs[y][x] * 4), 0xFF);
                i += sprintf(row + i, "\033[38;2;127;%d;%dm%c", w, 0xFF - w, row_chars[x]);     // does not export null termination
            }
        }
        strcpy(row + i, "\033[0m");     // null termination is copied as well
        i += 4;
    #else
        char* row = row_chars;
        i = DUNGEON_X_DIM;
    #endif
        printf(row_fmt, i, row);
    }

    if(border)
    {
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    return 0;
}
int dungeon_level_update_costs(DungeonLevel* d, int both_or_only_terrain)
{
    static PathFindingBuffer buff;
    static int buff_inited = 0;
    if(!buff_inited)
    {
        init_pathing_buffer(&buff);
        buff_inited = 1;
    }

    Vec2u pos;
    vec2u_assign(&pos, d->pc->pos.x, d->pc->pos.y);

    if(both_or_only_terrain)
    {
        dungeon_dijkstra_traverse_floor(&d->map, pos, &buff);
        for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
        {
            for(size_t x = 0; x < DUNGEON_X_DIM; x++)
            {
                d->tunnel_costs[y][x] = buff.nodes[y][x].cost;
            }
        }
    }
    dungeon_dijkstra_traverse_terrain(&d->map, pos, &buff);
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            d->terrain_costs[y][x] = buff.nodes[y][x].cost;
        }
    }

    return 0;
}



/* --- PUBLIC INTERFACE ------------------------------------------------ */

/* DungeonRoom Public Utilites */

int dungeon_room_collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b)
{
    const int32_t
        p = (int32_t)a->br.x - (int32_t)b->tl.x + 1,
        q = (int32_t)a->tl.x - (int32_t)b->br.x - 1,
        r = (int32_t)a->br.y - (int32_t)b->tl.y + 1,
        s = (int32_t)a->tl.y - (int32_t)b->br.y - 1;

    // PRINT_DEBUG("%p, %p -- %d %d %d %d\n", a, b, p, q, r, s)

    return
        (p >= 0 && q <= 0) &&
        (r >= 0 && s <= 0) &&
        (p || r) && (q || s) && (p || s) && (q || r);
}
void dungeon_room_size(const DungeonRoom* r, Vec2u* s)
{
    s->x = r->br.x - r->tl.x + 1;
    s->y = r->br.y - r->tl.y + 1;
}
void print_dungeon_room(const DungeonRoom* room)
{
    printf("\t%p -- (%d, %d) -- (%d, %d)\n", room, room->tl.x, room->tl.y, room->br.x, room->br.y);
}


/* DungeonMap Public Interface */

int zero_dungeon_map(DungeonMap* d)
{
    d->num_rooms = 0;
    d->num_up_stair = 0;
    d->num_down_stair = 0;
    d->rooms = NULL;

    dungeon_map_zero_cells(d);

    return 0;
}
int generate_dungeon_map(DungeonMap* d, uint32_t seed)
{
    if(seed > 0) srand(seed);
    // else srand(time(NULL));

    dungeon_map_zero_cells(d);

    const int r = rand();
    const float
        rx = (float)(r & 0xFF),
        ry = (float)((r >> 8) & 0xFF);

    for(size_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        for(size_t x = 1; x < DUNGEON_X_DIM - 1; x++)
        {
            const float p = perlin2f((float)x * DUNGEON_PERLIN_SCALE_X + rx, (float)y * DUNGEON_PERLIN_SCALE_Y + ry);
            d->hardness[y][x] = (uint8_t)(p * 127.f + 127.f);
        }
    }

    dungeon_map_generate_rooms(d);
    dungeon_map_place_stairs(d);

    return 0;
}
int destruct_dungeon_map(DungeonMap* d)
{
    if(d->rooms) free(d->rooms);

    return 0;
}

int random_dungeon_map_floor_pos(DungeonMap* d, uint8_t* pos)
{
    Vec2u p, d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max, (DUNGEON_X_DIM - 2), (DUNGEON_Y_DIM - 2) );

    for(;;)
    {
        vec2u_random_in_range(&p, d_min, d_max);
        CellTerrain* c = d->terrain[p.y] + p.x;
        if(c->type > 0)
        {
            break;
        }
    }

    pos[0] = p.x;
    pos[1] = p.y;

    return 0;
}

int serialize_dungeon_map(const DungeonMap* d, const Vec2u8* pc_pos, FILE* out)
{
// 1. Write file type marker
    fwrite("RLG327-S2025", 12, 1, out);

// 2. Write file version (0)
    const uint32_t version = 0;
    // version = htobe32(&version);
    fwrite(&version, sizeof(version), 1, out);

// 3. Write file size
    uint32_t size = (1708U + d->num_rooms * 4 + d->num_up_stair * 2 + d->num_down_stair * 2);
    PRINT_DEBUG("Writing file size of %d\n", size);
    size = htobe32(size);
    fwrite(&size, sizeof(size), 1, out);

// 4. Write X and Y position of PC
    const uint8_t pc_loc[2] = { 0, 0 };
    if(pc_pos) (*(uint16_t*)pc_loc) = (*(const uint16_t*)pc_pos);
    PRINT_DEBUG("Writing PC location of (%d, %d)\n", pc_pos->x, pc_pos->y);
    fwrite(pc_loc, sizeof(*pc_loc), (sizeof(pc_loc) / sizeof(*pc_loc)), out);

    uint8_t *up_stair, *down_stair;
    up_stair = malloc(sizeof(*up_stair) * d->num_up_stair * 2);
    down_stair = malloc(sizeof(*down_stair) * d->num_down_stair * 2);
    if(!up_stair || !down_stair) return -1;

// 5. Write DungeonMap bytes
    uint8_t dungeon_bytes[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    size_t u_idx = 0, d_idx = 0;
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            const CellTerrain c = d->terrain[y][x];
            switch(c.type)
            {
                case ROCK: dungeon_bytes[y][x] = d->hardness[y][x]; break;
                case ROOM:
                case CORRIDOR: dungeon_bytes[y][x] = 0; break;
            }

            switch(c.is_stair)
            {
                case STAIR_UP:
                {
                    if(u_idx < d->num_up_stair)
                    {
                        up_stair[u_idx * 2 + 0] = (uint8_t)x;
                        up_stair[u_idx * 2 + 1] = (uint8_t)y;
                        u_idx++;
                    }
                    break;
                }
                case STAIR_DOWN:
                {
                    if(d_idx < d->num_down_stair)
                    {
                        down_stair[d_idx * 2 + 0] = (uint8_t)x;
                        down_stair[d_idx * 2 + 1] = (uint8_t)y;
                        d_idx++;
                    }
                    break;
                }
                case NO_STAIR:
                default: break;
            }
        }
    }
    fwrite(dungeon_bytes, sizeof(*dungeon_bytes[0]), (DUNGEON_X_DIM * DUNGEON_Y_DIM), out);

// 6. Write number of rooms in DungeonMap
    PRINT_DEBUG("Writing num rooms: %x\n", d->num_rooms);
    uint16_t num_rooms = htobe16(d->num_rooms);
    fwrite(&num_rooms, sizeof(num_rooms), 1, out);

// 7. Write room top left corners and sizes
    for(size_t n = 0; n < d->num_rooms; n++)
    {
        const DungeonRoom* r = d->rooms + n;

        uint8_t room_data[4];
        room_data[0] = (uint8_t)r->tl.x,
        room_data[1] = (uint8_t)r->tl.y,
        room_data[2] = (uint8_t)(r->br.x - r->tl.x + 1),
        room_data[3] = (uint8_t)(r->br.y - r->tl.y + 1);

        PRINT_DEBUG("Writing room %lu data: (%d, %d, %d, %d)\n", n, room_data[0], room_data[1], room_data[2], room_data[3]);

        fwrite(room_data, sizeof(*room_data), (sizeof(room_data) / sizeof(*room_data)), out);
    }

// 8. Write the number of upward staircases
    PRINT_DEBUG("Writing number of up staircases: %d\n", d->num_up_stair);
    uint16_t num_up = htobe16(d->num_up_stair);
    fwrite(&num_up, sizeof(num_up), 1, out);

// 9. Write each upward staircase
    fwrite(up_stair, sizeof(*up_stair), d->num_up_stair * 2, out);

// 10. Write the number of downward staircases
    PRINT_DEBUG("Writing number of down staircases: %d\n", d->num_down_stair);
    uint16_t num_down = htobe16(d->num_down_stair);
    fwrite(&num_down, sizeof(num_down), 1, out);

// 11. Write each downward staircase
    fwrite(down_stair, sizeof(*down_stair), d->num_down_stair * 2, out);

    free(up_stair);
    free(down_stair);

    return 0;
}
int deserialize_dungeon_map(DungeonMap* d, Vec2u8* pc_pos, FILE* in)
{
// marker, version, and size all unneeded for parsing
    int status;
    uint8_t scratch[20];
    status = fread(scratch, 1, 20, in);

// read PC location
    status = fread(scratch, 1, 2, in);
    if(pc_pos) (*(uint16_t*)pc_pos) = (*(uint16_t*)scratch);

// read grid
    uint8_t dungeon_bytes[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    status = fread(dungeon_bytes, sizeof(*dungeon_bytes[0]), (DUNGEON_X_DIM * DUNGEON_Y_DIM), in);
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            CellTerrain* c = d->terrain[y] + x;

            if( !(d->hardness[y][x] = dungeon_bytes[y][x]) )
            {
                c->type = CORRIDOR;
            }
        }
    }

// read number of rooms
    const uint16_t prev_num_rooms = d->num_rooms;
    uint16_t num_rooms;
    status = fread(&num_rooms, sizeof(num_rooms), 1, in);
    d->num_rooms = be16toh(num_rooms);
    PRINT_DEBUG("Read num of rooms: %d\n", d->num_rooms);

// read each room
    if(prev_num_rooms && d->rooms)
        d->rooms = realloc(d->rooms, sizeof(*d->rooms) * d->num_rooms);
    else
        d->rooms = malloc(sizeof(*d->rooms) * d->num_rooms);
    if(!d->rooms)
        return -1;

    for(uint16_t r = 0; r < d->num_rooms; r++)
    {
        status = fread(scratch, sizeof(*scratch), 4, in);

        DungeonRoom* room = d->rooms + r;
        room->tl.x = scratch[0];
        room->tl.y = scratch[1];
        room->br.x = (uint32_t)scratch[0] + scratch[2] - 1U;
        room->br.y = (uint32_t)scratch[1] + scratch[3] - 1U;

    #if ENABLE_DEBUG_PRINTS
        print_dungeon_room(room);
    #endif
    }
    dungeon_map_fill_room_cells(d);

// read number of upward stairs
    uint16_t num_up;
    status = fread(&num_up, sizeof(num_up), 1, in);
    d->num_up_stair = be16toh(num_up);

// read upward stairs
    for(uint16_t s = 0; s < d->num_up_stair; s++)
    {
        status = fread(scratch, sizeof(*scratch), 2, in);
        d->terrain[scratch[1]][scratch[0]].is_stair = STAIR_UP;
    }

// read number of downward stairs
    uint16_t num_down;
    status = fread(&num_down, sizeof(num_down), 1, in);
    d->num_down_stair = be16toh(num_down);

// read downward stairs
    for(uint16_t s = 0; s < d->num_down_stair; s++)
    {
        status = fread(scratch, sizeof(*scratch), 2, in);
        d->terrain[scratch[1]][scratch[0]].is_stair = STAIR_DOWN;
    }

    (void)status;
    return 0;
}


/* DungeonLevel Public Interface */

int zero_dungeon_level(DungeonLevel* d)
{
    int ret = zero_dungeon_map(&d->map);

    memset(d->entities, 0x0, sizeof(Entity*) * DUNGEON_Y_DIM * DUNGEON_X_DIM);
    memset(d->tunnel_costs, INT_MAX, sizeof(int32_t) * DUNGEON_Y_DIM * DUNGEON_X_DIM);
    memset(d->terrain_costs, INT_MAX, sizeof(int32_t) * DUNGEON_Y_DIM * DUNGEON_X_DIM);

    d->pc = NULL;
    d->entity_alloc = NULL;
    d->num_monsters = 0;

    return ret;
}
int destruct_dungeon_level(DungeonLevel* d)
{
    int ret = destruct_dungeon_map(&d->map);

    heap_delete(&d->entity_q);

    if(d->entity_alloc) free(d->entity_alloc);

    return ret;
}

int init_dungeon_level(DungeonLevel* d, Vec2u8 pc_pos, size_t nmon)
{
// 1. init priority q
    heap_init(&d->entity_q, entity_priority_comp, NULL);    // queue should not delete entities - we handle this

// 2. init pc (alloc for all entities)
    if( !(d->pc = d->entity_alloc = malloc((nmon + 1) * sizeof(Entity))) ) return -1;     // allocate for all entities --> pc gets first element so reuse ptr as address for subsequent monster elements as well
    d->pc->is_pc = 1;
    d->pc->speed = 100;
    d->pc->priority = 0;
    d->pc->next_turn = 0;
    vec2u8_copy(&d->pc->pos, &pc_pos);
    d->pc->hn = heap_insert(&d->entity_q, d->pc);
    d->pc->md.flags = 0;
    d->entities[pc_pos.y][pc_pos.x] = d->pc; // assign positional pointer

// 3. allocate monster buffers
    PRINT_DEBUG("Allocating %lu monsters\n", nmon)
    // if( !(d->monster_alloc = malloc(nmon * sizeof(MonsterData))) ) return -1;

// 4. init monsters
    uint8_t x, y;
    x = pc_pos.x;
    y = pc_pos.y;
    for(size_t m = 0; m < nmon; m++)
    {
        for(uint32_t trav = RANDOM_IN_RANGE(30, 150); trav > 0;)
        {
            x++;
            y += (x / DUNGEON_X_DIM);
            x %= DUNGEON_X_DIM;
            y %= DUNGEON_Y_DIM;
            trav -= (d->map.terrain[y][x].type && !d->entities[y][x]);
        }

        Entity* me = d->entities[y][x] = d->entity_alloc + m + 1;

        me->is_pc = 0;
        me->speed = (uint8_t)RANDOM_IN_RANGE(50, 200);
        me->priority = m + 1;
        me->next_turn = 0;
        vec2u8_assign(&me->pos, x, y);
        me->hn = heap_insert(&d->entity_q, me);

        me->md.stats = (uint8_t)RANDOM_IN_RANGE(0, 15);
        me->md.flags = 0;
        vec2u8_zero(&me->md.pc_rem_pos);

        PRINT_DEBUG( "Initialized monster {%d, %d, (%d, %d), %#x}\n",
            me->speed, me->priority, x, y, me->md.stats );
    }
    d->num_monsters = nmon;

// 5. init cost maps
    dungeon_level_update_costs(d, 1);

    return 0;
}

LevelStatus get_dungeon_level_status(DungeonLevel* d)
{
    LevelStatus s;
    s.has_won = !d->num_monsters;
    s.has_lost = !d->pc;
    return s;
}

int print_dungeon_level(DungeonLevel* d, int border)
{
    char* row_fmt = "%.*s\n";

    if(border)
    {
        row_fmt = "|%.*s|\n";
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    for(uint32_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        char row_chars[DUNGEON_X_DIM];
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            row_chars[x] = get_cell_char(d->map.terrain[y][x], d->entities[y][x]);
        }

    #if DUNGEON_PRINT_HARDNESS
        uint32_t i = 0;
        char row[20 * DUNGEON_X_DIM + 5];   // "\033[48;2;<3>;127;127m<1>" for each cell + reset code + null termination
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            uint8_t w = d->map.hardness[y][x];
            if(d->map.terrain[y][x].type) w = 0;
            i += sprintf(row + i, "\033[48;2;127;%d;127m%c", w, row_chars[x]);     // does not export null termination
        }
        strcpy(row + i, "\033[0m");     // null termination is copied as well
        i += 4;
        printf(row_fmt, i, row);
    #else
        printf(row_fmt, DUNGEON_X_DIM, row_chars);
    #endif
    }

    if(border)
    {
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    fflush(stdout);

    return 0;
}
int print_dungeon_level_costmaps(DungeonLevel* d, int border)
{
    dungeon_level_print_costs(d->tunnel_costs, border);
    dungeon_level_print_costs(d->terrain_costs, border);

    return 0;
}
