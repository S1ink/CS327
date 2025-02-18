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


/* UTILITIES */

GENERATE_MIN_MAX_UTIL(uint32_t, u32)

static inline uint32_t random_in_range(uint32_t min, uint32_t max)
{
    return min + (uint32_t)(rand() % (max - min + 1));
}
static inline void vec2u_random(Vec2u* v, Vec2u range)
{
    v->x = (uint32_t)(rand() % range.x);
    v->y = (uint32_t)(rand() % range.y);
}
static inline void vec2u_random_in_range(Vec2u* v, Vec2u min, Vec2u max)
{
    v->x = min.x + (uint32_t)(rand() % (max.x - min.x + 1));
    v->y = min.y + (uint32_t)(rand() % (max.y - min.y + 1));
}

static inline char get_cell_char(CellTerrain c)
{
    switch(c.is_stair)
    {
        case STAIR_UP: return '<';
        case STAIR_DOWN: return '>';
        default: break;
    }
    switch(c.type)
    {
        case CORRIDOR: return '#';
        case ROOM: return '.';
        default: break;
    }
    return ' ';
}


/* DUNGEON ROOM UTILIES */

int collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b)
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
int in_x_window(const DungeonRoom* a, const DungeonRoom* b)
{
    const int32_t
        p = (int32_t)a->br.x - (int32_t)b->tl.x + 1,
        q = (int32_t)a->tl.x - (int32_t)b->br.x - 1;

    return (p >= 0 && q <= 0);
}
int in_y_window(const DungeonRoom* a, const DungeonRoom* b)
{
    const int64_t
        r = (int32_t)a->br.y - (int32_t)b->tl.y + 1,
        s = (int32_t)a->tl.y - (int32_t)b->br.y - 1;

    return (r >= 0 && s <= 0);
}

void room_size(const DungeonRoom* r, Vec2u* s)
{
    s->x = r->br.x - r->tl.x + 1;
    s->y = r->br.y - r->tl.y + 1;
}
void print_room(const DungeonRoom* room)
{
    printf("\t%p -- (%d, %d) -- (%d, %d)\n", room, room->tl.x, room->tl.y, room->br.x, room->br.y);
}



/* GENERATION HELPERS */

int zero_cells(Dungeon* d)
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

int create_random_connection(Dungeon* d, size_t a, size_t b)
{
    const DungeonRoom* room_data = d->rooms;

    Vec2u pos_r1, pos_r2;
    vec2u_random_in_range(&pos_r1, room_data[a].tl, room_data[a].br);
    vec2u_random_in_range(&pos_r2, room_data[b].tl, room_data[b].br);

    dungeon_dijkstra_corridor_path(d, pos_r1, pos_r2);

    return 0;
}

int connect_rooms(Dungeon* d)
{
    for(size_t i = 0; i < d->num_rooms; i++)
    {
        create_random_connection(d, i, (i + 1) % d->num_rooms);
    }

    return 0;
}

int fill_room_cells(Dungeon* d)
{
    for(size_t r = 0; r < d->num_rooms; r++)
    {
        const DungeonRoom* room = d->rooms + r;
    #if ENABLE_DEBUG_PRINTS
        print_room(room);
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


/* GENERATE ROOMS */

int generate_rooms(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
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
            if(collide_or_tangent(d->rooms + i, d->rooms + j))
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
            if(collide_or_tangent(d->rooms + R, d->rooms + j))
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
        room_size(dr, &size);
        size_t iter = 0;
        for(uint8_t b = 0; status < 0b1111 && size.x < target_size.x && size.y < target_size.y; b = !b)
        {
        #define CHECK_COLLISIONS(K, reset) \
            for(size_t i = 1; i < R; i++) \
            {   \
                size_t rr = (r + i) % R; \
                if(collide_or_tangent(dr, d->rooms + rr)) \
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

            room_size(dr, &size);

            iter++;
            if(iter > 10000) break;

        #undef CHECK_COLLISIONS
        }
    }

    connect_rooms(d);

    return fill_room_cells(d);
}


int place_stairs(Dungeon* d)
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



int generate_dungeon(Dungeon* d, uint32_t seed)
{
    if(seed > 0) srand(seed);
    else srand(time(NULL));

    zero_cells(d);

    const int32_t
        rx = rand() % 0xFF,
        ry = rand() % 0xFF;

    for(size_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        for(size_t x = 1; x < DUNGEON_X_DIM - 1; x++)
        {
            const float p = perlin2f((float)x * DUNGEON_PERLIN_SCALE_X + rx, (float)y * DUNGEON_PERLIN_SCALE_Y + ry);
            d->hardness[y][x] = (uint8_t)(p * 127.f + 127.f);
        }
    }

    generate_rooms(d);
    place_stairs(d);

    return 0;
}

int zero_dungeon(Dungeon* d)
{
    d->num_rooms = 0;
    d->num_up_stair = 0;
    d->num_down_stair = 0;
    d->rooms = 0;

    zero_cells(d);

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    free(d->rooms);

    return 0;
}



int random_dungeon_floor_pos(Dungeon* d, uint8_t* pos)
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



/* SERIALIZATION/DESERIALIZATION */

int print_dungeon(Dungeon* d, uint8_t* pc_loc, int border)
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
            row_chars[x] = get_cell_char(d->terrain[y][x]);
        }

        if(pc_loc && pc_loc[1] == y)
        {
            row_chars[pc_loc[0]] = '@';
        }

        uint32_t i = 0;
    #if DUNGEON_PRINT_HARDNESS
        char row[20 * DUNGEON_X_DIM + 5];   // "\033[48;2;<3>;127;127m<1>" for each cell + reset code + null termination
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            uint8_t w = d->hardness[y][x];
            if(d->terrain[y][x].type) w = 0;
            i += sprintf(row + i, "\033[48;2;127;%d;127m%c", w, row_chars[x]);     // does not export null termination
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
static int print_trav_weights(PathFindingBuffer* buff, int border)
{
    char* row_fmt = "%.*s\n";

    if(border)
    {
        row_fmt = "|%.*s|\n";
        // printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    for(uint32_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        char row_chars[DUNGEON_X_DIM];
        for(uint32_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            const int32_t w = buff->nodes[y][x].cost;
            row_chars[x] = w == INT_MAX ? ' ' : (w % 10) + '0';
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
                uint8_t w = (uint8_t)u32_min((buff->nodes[y][x].cost * 4), 0xFF);
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
int print_dungeon_a3(Dungeon* d, uint8_t* pc_loc, int border)
{
    if(!pc_loc) return -1;

    Vec2u pos;
    vec2u_assign(&pos, pc_loc[0], pc_loc[1]);

    PathFindingBuffer pbuff;
    init_pathing_buffer(&pbuff);

IF_DEBUG(const uint64_t t1 = us_time();)
    dungeon_dijkstra_traverse_floor(d, pos, &pbuff);
IF_DEBUG(const uint64_t t2 = us_time();)
    print_trav_weights(&pbuff, border);

IF_DEBUG(const uint64_t t3 = us_time();)
    dungeon_dijkstra_traverse_terrain(d, pos, &pbuff);
IF_DEBUG(const uint64_t t4 = us_time();)
    print_trav_weights(&pbuff, border);
IF_DEBUG(const uint64_t t5 = us_time();)

#if ENABLE_DEBUG_PRINTS
    printf(
        "WEIGHTMAP GENERATION:\n Floor traversal: %f\n Floor trav print: %f\n Grid traversal: %f\n Grid trav print: %f\n",
        (double)(t2 - t1) * 1e-6,
        (double)(t3 - t2) * 1e-6,
        (double)(t4 - t3) * 1e-6,
        (double)(t5 - t4) * 1e-6 );
#endif

    return 0;
}

int serialize_dungeon(const Dungeon* d, FILE* out, const uint8_t* pc)
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
    if(pc) (*(uint16_t*)pc_loc) = (*(const uint16_t*)pc);
    PRINT_DEBUG("Writing PC location of (%d, %d)\n", pc_loc[0], pc_loc[1]);
    fwrite(pc_loc, sizeof(*pc_loc), (sizeof(pc_loc) / sizeof(*pc_loc)), out);

    uint8_t *up_stair, *down_stair;
    up_stair = malloc(sizeof(*up_stair) * d->num_up_stair * 2);
    down_stair = malloc(sizeof(*down_stair) * d->num_down_stair * 2);
    if(!up_stair || !down_stair) return -1;

// 5. Write dungeon bytes
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

// 6. Write number of rooms in dungeon
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

int deserialize_dungeon(Dungeon* d, FILE* in, uint8_t* pc)
{
// marker, version, and size all unneeded for parsing
    int status;
    uint8_t scratch[20];
    status = fread(scratch, 1, 20, in);

// read PC location
    status = fread(scratch, 1, 2, in);
    if(pc) (*(uint16_t*)pc) = (*(uint16_t*)scratch);

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
        print_room(room);
    #endif
    }
    fill_room_cells(d);

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
