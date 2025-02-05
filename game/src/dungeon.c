#include "dungeon.h"

#include "util/perlin.h"
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

int collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b)
{
    const int32_t
        p = (int32_t)a->br.x - (int32_t)b->tl.x + 1,
        q = (int32_t)a->tl.x - (int32_t)b->br.x - 1,
        r = (int32_t)a->br.y - (int32_t)b->tl.y + 1,
        s = (int32_t)a->tl.y - (int32_t)b->br.y - 1;

    // printf("%p, %p -- %d %d %d %d\n", a, b, p, q, r, s);

    return
        (p >= 0 && q <= 0) &&
        (r >= 0 && s <= 0) &&
        (p || r) && (q || s) && (p || s) && (q || r);
}

void print_room(const DungeonRoom* room)
{
    printf("\t%p -- (%d, %d) -- (%d, %d)\n", room, room->tl.x, room->tl.y, room->br.x, room->br.y);
}

void random_room_in_range(DungeonRoom* r, Vec2u min, Vec2u max)
{
    Vec2u range, config_range, range_min, size, pos_max, pos;

    vec2u_assign( &config_range,
        (DUNGEON_ROOM_MAX_X - DUNGEON_ROOM_MIN_X),
        (DUNGEON_ROOM_MAX_Y - DUNGEON_ROOM_MIN_Y) );
    vec2u_assign( &range_min,
        (DUNGEON_ROOM_MIN_X),
        (DUNGEON_ROOM_MIN_Y) );

    vec2u_sub(&range, &max, &min);  // raw variation
    vec2u_cwise_min(&range, &range, &config_range); // minimum usable bbox for either dim

    vec2u_random(&size, range);             // random size variation component
    vec2u_add(&size, &range_min, &size);    // actual random size
    vec2u_sub(&pos_max, &max, &size);       // max corner minus size

    vec2u_random_in_range(&pos, min, pos_max);    // random starting location

    vec2u_copy(&(r->tl), &pos);
    vec2u_add(&(r->br), &pos, &size);
}

int fill_room_cells(Dungeon* d)
{
    for(size_t r = 0; r < d->rooms_list.size; r++)
    {
        const DungeonRoom* room = d->rooms_list.data + r;
        // print_room(room);
        for(size_t y = room->tl.y; y <= room->br.y; y++)
        {
            for(size_t x = room->tl.x; x <= room->br.x; x++)
            {
                d->cells[y][x].type = ROOM;
                // printf("(%lu, %lu)", x, y);
            }
        }
    }

    return 0;
}

int create_random_connection(Dungeon* d, size_t a, size_t b)
{
    const DungeonRoom* room_data = d->rooms_list.data;

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
    const size_t num_rooms = d->rooms_list.size;

    // size_t indices[DUNGEON_MAX_NUM_ROOMS];
    // // uint8_t adjacency[DUNGEON_MAX_NUM_ROOMS][DUNGEON_MAX_NUM_ROOMS];

    // for(size_t i = 0; i < num_rooms; i++)
    // {
    //     indices[i] = i;
    //     // memset(adjacency[i], 0x0, num_rooms);
    // }
    for(size_t i = 0; i < num_rooms; i++)
    {
        // const uint32_t n = random_in_range(0, num_rooms - 2);
        // const size_t r1 = i, r2i = (i + n) % num_rooms, r2 = indices[r2i];
        // indices[r2i] = indices[i];  // overwrite that index so we don't travel to it again

        // adjacency[r1][r2] = adjacency[r2][r1] = 1;

        create_random_connection(d, i, (i + 1) % num_rooms);
    }

    return 0;
}

int generate_rooms(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
    d->rooms_list.size = target;
    d->rooms_list.data = (DungeonRoom*)malloc(target * sizeof(*d->rooms_list.data));
    DungeonRoom* room_data = d->rooms_list.data;

    Vec2u d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max,
        (DUNGEON_X_DIM - DUNGEON_ROOM_MIN_X - 1),
        (DUNGEON_Y_DIM - DUNGEON_ROOM_MIN_Y - 1) );

// 1. GENERATE AT LEAST THE MINIMUM NUMBER
    size_t iter = 0;
    for(size_t i = 0; i < DUNGEON_MIN_NUM_ROOMS; iter++)
    {
        vec2u_random_in_range(&room_data[i].tl, d_min, d_max);
        room_data[i].br.x = room_data[i].tl.x + (DUNGEON_ROOM_MIN_X - 1);
        room_data[i].br.y = room_data[i].tl.y + (DUNGEON_ROOM_MIN_Y - 1);

        if(i == 0)
        {
            i++;
            continue;
        }

        int failed = 0;
        for(size_t j = 0; j < i; j++)
        {
            if(collide_or_tangent(room_data + i, room_data + j))
            {
                // printf("collision\n");
                // print_room(room_data + i);
                // print_room(room_data + j);
                failed = 1;
                break;
            }
        }
        if(!failed) i++;
    }
    printf("Took %lu iterations to generate core rooms.\n", iter);

// 2. GENERATE EXTRA ROOMS
    size_t R = DUNGEON_MIN_NUM_ROOMS;
    for(size_t i = DUNGEON_MIN_NUM_ROOMS; i < target; i++)
    {
        vec2u_random_in_range(&room_data[R].tl, d_min, d_max);
        room_data[R].br.x = room_data[R].tl.x + (DUNGEON_ROOM_MIN_X - 1);
        room_data[R].br.y = room_data[R].tl.y + (DUNGEON_ROOM_MIN_Y - 1);

        int success = 1;
        for(size_t j = 0; j < R; j++)
        {
            if(collide_or_tangent(room_data + R, room_data + j))
            {
                // printf("collision\n");
                // print_room(room_data + ri);
                // print_room(room_data + j);
                success = 0;
                break;
            }
        }
        R += success;
    }
    printf("Total dungeons generated: %lu\n", R);

// RESIZE
    d->rooms_list.size = R;
    room_data = d->rooms_list.data = (DungeonRoom*)realloc(room_data, d->rooms_list.size * sizeof(*room_data));

// 3. EXPAND DIMENSIONS
    // Vec2u r_min, r_max;
    // vec2u_assign(&r_min, DUNGEON_ROOM_MIN_X, DUNGEON_ROOM_MIN_Y);
    // vec2u_assign(&r_max, DUNGEON_ROOM_MAX_X, DUNGEON_ROOM_MAX_Y);
    // for(size_t r = 0; r < R; r++)
    // {
    //     DungeonRoom* dr = room_data + r;
    //     DungeonRoom temp;

    //     Vec2u target_size;
    //     vec2u_random_in_range(&target_size, r_min, r_max);

    //     // uint32_t up_range, down_range, left_range, right_range;
    //     vec2u_copy(&temp.tl, &dr->tl);
    //     vec2u_add(&temp.br, &temp.tl, &target_size);

    //     int failed = 0;
    //     for(size_t rr = 0; rr < R; rr++)
    //     {
    //         if(rr != r)
    //         {

    //         }
    //     }
    // }

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
    // connect_rooms(d);
    place_stairs(d);

    fill_printable(d);

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    free(d->rooms_list.data);

    return 0;
}
