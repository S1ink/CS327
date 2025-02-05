#include "dungeon.h"

#include "util/perlin.h"
#include "util/debug.h"
#include "util/math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#ifndef DEBUG_PRINT_HARDNESS
#define DEBUG_PRINT_HARDNESS 0
#endif


/* UTILITIES */

GENERATE_MIN_MAX_UTIL(uint32_t, u32)

uint32_t random_in_range(uint32_t min, uint32_t max)
{
    return min + (uint32_t)(rand() / (RAND_MAX / (max - min + 1)));
}
void vec2u_random(Vec2u* v, Vec2u range)
{
    v->x = (uint32_t)(rand() / (RAND_MAX / range.x));
    v->y = (uint32_t)(rand() / (RAND_MAX / range.y));
}
void vec2u_random_in_range(Vec2u* v, Vec2u min, Vec2u max)
{
    v->x = min.x + (uint32_t)(rand() / (RAND_MAX / (max.x - min.x + 1)));
    v->y = min.y + (uint32_t)(rand() / (RAND_MAX / (max.y - min.y + 1)));
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

int create_random_connection(Dungeon* d, size_t a, size_t b)
{
    const DungeonRoom* room_data = d->rooms;

    Vec2u pos_r1, pos_r2;
    vec2u_random_in_range(&pos_r1, room_data[a].tl, room_data[a].br);
    vec2u_random_in_range(&pos_r2, room_data[b].tl, room_data[b].br);

    uint32_t beg_x, end_x, beg_y, end_y, x_trav_y, y_trav_x;
    beg_x = u32_min(pos_r1.x, pos_r2.x);
    end_x = u32_max(pos_r1.x, pos_r2.x);
    beg_y = u32_min(pos_r1.y, pos_r2.y);
    end_y = u32_max(pos_r1.y, pos_r2.y);

    int32_t m;
    m = ((int32_t)pos_r2.x - (int32_t)pos_r1.x) * ((int32_t)pos_r2.y - (int32_t)pos_r1.y);

    if(rand() & 0x1)
    {
        if(m > 0)
        {
            x_trav_y = end_y;
            y_trav_x = beg_x;
        }
        else
        {
            x_trav_y = beg_y;
            y_trav_x = beg_x;
        }
    }
    else
    {
        if(m > 0)
        {
            x_trav_y = beg_y;
            y_trav_x = end_x;
        }
        else
        {
            x_trav_y = end_y;
            y_trav_x = end_x;
        }
    }

    for(uint32_t x = beg_x; x <= end_x; x++)
    {
        d->cells[x_trav_y][x].type = CORRIDOR;
    }
    for(uint32_t y = beg_y; y <= end_y; y++)
    {
        d->cells[y][y_trav_x].type = CORRIDOR;
    }

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
                d->cells[y][x].type = ROOM;
            }
        }
    }

    return 0;
}


/* GENERATE ROOMS */

int generate_rooms(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
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
        DungeonCell* c = d->cells[p.y] + p.x;
        if(c->type > 0)
        {
            c->is_stair = STAIR_UP;
            break;
        }
    }
    for(;;)
    {
        vec2u_random_in_range(&p, d_min, d_max);
        DungeonCell* c = d->cells[p.y] + p.x;
        if(c->type > 0 && c->is_stair == 0)
        {
            c->is_stair = STAIR_DOWN;
            break;
        }
    }

    return 0;
}


int fill_printable(Dungeon* d)
{
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            DungeonCell C = d->cells[y][x];
            char* c = d->printable[y] + x;

            switch(C.is_stair)
            {
                case STAIR_UP: *c = '>'; continue;
                case STAIR_DOWN: *c = '<'; continue;
                case NO_STAIR:
                default: break;
            }
            switch(C.type)
            {
            #if DEBUG_PRINT_HARDNESS
                case ROCK:
                {
                    static const char* BRIGHTNESS = ".-:=*?#$%%&@";
                    *c = BRIGHTNESS[C.hardness / (255 / 12 + 1)];
                    break;
                }
                case ROOM: *c = ' '; break;
            #else
                case ROCK: *c = ' '; break;
                case ROOM: *c = '.'; break;
            #endif
                case CORRIDOR: *c = '#'; break;
            }
        }
    }

    return 0;
}



int generate_dungeon(Dungeon* d, uint32_t seed)
{
    if(seed > 0) srand(seed);
    else srand(time(NULL));

    for(size_t i = 0; i < DUNGEON_Y_DIM; i++)
    {
        memset(d->cells[i], 0x0, sizeof(d->cells[i]));
        d->cells[i][0].hardness = d->cells[i][DUNGEON_X_DIM - 1].hardness = 0xFF;
    }
    for(size_t i = 1; i < DUNGEON_X_DIM - 1; i++)
    {
        d->cells[0][i].hardness = d->cells[DUNGEON_Y_DIM - 1][i].hardness = 0xFF;
    }

    const int32_t
        rx = rand() % 0xFF,
        ry = rand() % 0xFF;

    for(size_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        for(size_t x = 1; x < DUNGEON_X_DIM - 1; x++)
        {
            const float p = perlin2f((float)x * DUNGEON_PERLIN_SCALE_X + rx, (float)y * DUNGEON_PERLIN_SCALE_Y + ry);
            d->cells[y][x].hardness = (uint8_t)(p * 127.f + 127.f);
        }
    }

    generate_rooms(d);
    place_stairs(d);

    fill_printable(d);

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    return 0;
}

int print_dungeon(Dungeon* d, int border)
{
    char* row_fmt = "%.*s\n";

    if(border)
    {
        row_fmt = "|%.*s|\n";
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    for(uint32_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        printf(row_fmt, DUNGEON_X_DIM, d->printable[y]);
    }

    if(border)
    {
        printf("+%.*s+\n", DUNGEON_X_DIM, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    }

    return 0;
}
