#include "dungeon.h"

#include "util/perlin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#ifndef DEBUG_PRINT_HARDNESS
#define DEBUG_PRINT_HARDNESS 1
#endif


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

int generate_rooms(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
    d->rooms_list.size = target;
    d->rooms_list.data = (DungeonRoom*)malloc(target * sizeof(*d->rooms_list.data));

    Vec2u origin, max;
    vec2u_zero(&origin);
    vec2u_assign(&max, DUNGEON_X_DIM - 1, DUNGEON_Y_DIM - 1);

    size_t i = 0;
    for(size_t failed = 0; failed < 10000 && i < target; )
    {
        random_room_in_range(d->rooms_list.data + i, origin, max);

        if(i > 0)
        {
            size_t j;
            for(j = 0; j < i; j++)
            {
                if(collide_or_tangent(d->rooms_list.data + i, d->rooms_list.data + j))
                {
                    // printf("collision\n");
                    // print_room(d->rooms_list.data + i);
                    // print_room(d->rooms_list.data + j);
                    failed++;
                    break;
                }
            }
            if(j < i - 1)
            {
                continue;
            }
        }
        i++;
    }

    printf("generated %lu rooms\n", i);

    if(i < DUNGEON_MIN_NUM_ROOMS) return -1;

    d->rooms_list.size = i;
    d->rooms_list.data = (DungeonRoom*)realloc(d->rooms_list.data, i * sizeof(*d->rooms_list.data));

    for(size_t r = 0; r < i; r++)
    {
        const DungeonRoom* room = d->rooms_list.data + r;
        print_room(room);
        for(size_t y = room->tl.y; y < room->br.y; y++)
        {
            for(size_t x = room->tl.x; x < room->br.x; x++)
            {
                d->cells[y][x].type = ROOM;
                // printf("(%lu, %lu)", x, y);
            }
        }
    }

    return 0;
}

int generate_rooms3(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
    d->rooms_list.size = target;
    d->rooms_list.data = (DungeonRoom*)malloc(target * sizeof(*d->rooms_list.data));
    DungeonRoom* room_data = d->rooms_list.data;

    // {
    //     printf("-----------------------------\n");
    //     vec2u_assign(&room_data[0].tl, 5, 5);
    //     vec2u_assign(&room_data[0].br, 8, 7);
    //     vec2u_assign(&room_data[1].tl, 2, 1);
    //     vec2u_assign(&room_data[1].br, 7, 2);
    //     vec2u_assign(&room_data[2].tl, 8, 3);
    //     vec2u_assign(&room_data[2].br, 12, 4);
    //     vec2u_assign(&room_data[3].tl, 2, 7);
    //     vec2u_assign(&room_data[3].br, 4, 10);
    //     int
    //         ab = collide_or_tangent(&room_data[0], &room_data[1]),
    //         ac = collide_or_tangent(&room_data[0], &room_data[2]),
    //         ad = collide_or_tangent(&room_data[0], &room_data[3]),
    //         bc = collide_or_tangent(&room_data[1], &room_data[2]),
    //         bd = collide_or_tangent(&room_data[1], &room_data[3]),
    //         cd = collide_or_tangent(&room_data[2], &room_data[3]);
    //     printf("AB : %d\nAC : %d\nAD : %d\nBC : %d\nBD : %d\nCD : %d\n", ab, ac, ad, bc, bd, cd);
    //     printf("-----------------------------\n");

    //     d->rooms_list.size = 4;
    //     return fill_room_cells(d);
    // }

    Vec2u d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max,
        (DUNGEON_X_DIM - DUNGEON_ROOM_MIN_X - 1),
        (DUNGEON_Y_DIM - DUNGEON_ROOM_MIN_Y - 1) );

    size_t iter = 0;
    for(size_t i = 0; i < DUNGEON_MIN_NUM_ROOMS; iter++)  // generate at least the minimum number of rooms
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

    size_t r = DUNGEON_MIN_NUM_ROOMS;
    for(size_t i = DUNGEON_MIN_NUM_ROOMS; i < target; i++)
    {
        vec2u_random_in_range(&room_data[r].tl, d_min, d_max);
        room_data[r].br.x = room_data[r].tl.x + (DUNGEON_ROOM_MIN_X - 1);
        room_data[r].br.y = room_data[r].tl.y + (DUNGEON_ROOM_MIN_Y - 1);

        int success = 1;
        for(size_t j = 0; j < r; j++)
        {
            if(collide_or_tangent(room_data + r, room_data + j))
            {
                // printf("collision\n");
                // print_room(room_data + ri);
                // print_room(room_data + j);
                success = 0;
                break;
            }
        }
        r += success;
    }
    printf("Total dungeons generated: %lu\n", r);

    d->rooms_list.size = r;
    d->rooms_list.data = (DungeonRoom*)realloc(d->rooms_list.data, d->rooms_list.size * sizeof(*d->rooms_list.data));

    return fill_room_cells(d);
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

    generate_rooms3(d);

    fill_printable(d);

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    free(d->rooms_list.data);

    return 0;
}
