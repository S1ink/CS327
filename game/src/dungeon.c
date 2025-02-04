#include "dungeon.h"

#include "util/perlin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#ifndef DEBUG_PRINT_HARDNESS
#define DEBUG_PRINT_HARDNESS 0
#endif


uint32_t random_in_range(uint32_t min, uint32_t max)
{
    return min + (uint32_t)(rand() / (RAND_MAX / (max - min)));
}

void vec2u_random(Vec2u* v, Vec2u range)
{
    v->x = (uint32_t)(rand() / (RAND_MAX / range.x));
    v->y = (uint32_t)(rand() / (RAND_MAX / range.y));
}

int collide_or_tangent(DungeonRoom* a, DungeonRoom* b)
{
    return (
        (a->br.x - b->tl.x >= -2) && (a->tl.x - b->br.x <= 2) &&
        (a->br.y - b->tl.y >= -2) && (a->tl.y - b->br.y <= 2) );
}

void random_room_in_range(DungeonRoom* r, Vec2u min, Vec2u max)
{
    Vec2u range, config_range, range_min, size, pos_range, pos;

    vec2u_assign( &config_range,
        (DUNGEON_ROOM_MAX_X - DUNGEON_ROOM_MIN_X),
        (DUNGEON_ROOM_MAX_Y - DUNGEON_ROOM_MIN_Y) );
    vec2u_assign( &range_min,
        (DUNGEON_ROOM_MIN_X),
        (DUNGEON_ROOM_MIN_Y) );

    vec2u_sub(&range, &max, &min);
    vec2u_cwise_min(&range, &range, &config_range);

    vec2u_random(&size, range);
    vec2u_add(&size, &range_min, &size);
    vec2u_sub(&pos_range, &max, &size);

    vec2u_random(&pos, pos_range);
    vec2u_add(&pos, &min, &pos);

    vec2u_copy(&(r->tl), &pos);
    vec2u_add(&(r->br), &pos, &size);
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
            for(j = 0; j < i - 1; j++)
            {
                if(collide_or_tangent(d->rooms_list.data + i, d->rooms_list.data + j))
                {
                    printf("collision\n");
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
        printf("(%u, %u), (%u, %u)\n", room->tl.x, room->tl.y, room->br.x, room->br.y);
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

// void generate_rooms2(Dungeon* d)
// {
//     // random roll for first room tl, size
//     // calculate bounds for remaining 5 rooms -- random rolls within bounds for each
//     // random roll for max number of additional rooms -- try to add if possible up to this amount

// #define DUNGEON_MAX_NUM_ROOMS_TREE_SIZE (DUNGEON_MAX_NUM_ROOMS * 2 - 1)

//     Vec2u bboxes[DUNGEON_MAX_NUM_ROOMS_TREE_SIZE][2];
//     vec2u_zero(bboxes[0] + 0);
//     vec2u_assign(bboxes[0] + 1, DUNGEON_X_DIM - 1, DUNGEON_Y_DIM - 1);

//     for(size_t i = 1; i < DUNGEON_MAX_NUM_ROOMS_TREE_SIZE; i += 2)
//     {
//         const size_t level = (size_t)floorf(log2f((float)i + 1));
//         // const size_t i2 = i + 1;
//         // const size_t parent = i / 2;
//         const int32_t rd = rand();

//         Vec2u* parent = bboxes[i / 2];
//         Vec2u* a = bboxes[i];
//         Vec2u* b = bboxes[i + 1];

//         if(level & 0x1)
//         {
//             const uint32_t min_x = parent[0].x;
//             const uint32_t max_x = parent[1].x;
//         }
//         else
//         {
//             const uint32_t min_y = parent[1].y;
//             const uint32_t max_y = parent[0].y;
//         }
//     }

//     DungeonRoom r1;
//     // random_in_range(&r1, , )
// }

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
            #else
                case ROCK: *c = ' '; break;
            #endif
                case ROOM: *c = '.'; break;
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

    fill_printable(d);

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    free(d->rooms_list.data);

    return 0;
}
