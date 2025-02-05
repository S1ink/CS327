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
    return min + (uint32_t)(rand() / (RAND_MAX / (max - min)));
}

void vec2u_random(Vec2u* v, Vec2u range)
{
    v->x = (uint32_t)(rand() / (RAND_MAX / range.x));
    v->y = (uint32_t)(rand() / (RAND_MAX / range.y));
}
void vec2u_random_in_range(Vec2u* v, Vec2u min, Vec2u max)
{
    v->x = min.x + (uint32_t)(rand() / (RAND_MAX / (max.x - min.x)));
    v->y = min.y + (uint32_t)(rand() / (RAND_MAX / (max.y - min.y)));
}

int collide_or_tangent(const DungeonRoom* a, const DungeonRoom* b)
{
    return (
        ((a->br.x + 1 >= b->tl.x) && (a->tl.x <= b->br.x + 1)) &&
        ((a->br.y + 1 >= b->tl.y) && (a->tl.y <= b->br.y + 1)) );
}

void collide_or_tangent_test()
{
    printf("-----------------------------\n");
    DungeonRoom a, b, c, d;
    vec2u_assign(&a.tl, 5, 5);
    vec2u_assign(&a.br, 9, 8);
    vec2u_assign(&b.tl, 2, 2);
    vec2u_assign(&b.br, 3, 3);
    vec2u_assign(&c.tl, 9, 6);
    vec2u_assign(&c.br, 12, 9);
    vec2u_assign(&d.tl, 2, 7);
    vec2u_assign(&d.br, 4, 10);
    int
        ab = collide_or_tangent(&a, &b),
        ac = collide_or_tangent(&a, &c),
        ad = collide_or_tangent(&a, &d),
        bc = collide_or_tangent(&b, &c),
        bd = collide_or_tangent(&b, &d),
        cd = collide_or_tangent(&c, &d);
    printf("AB : %d\nAC : %d\nAD : %d\nBC : %d\nBD : %d\nCD : %d\n", ab, ac, ad, bc, bd, cd);
    printf("-----------------------------\n");
}

void print_room(const DungeonRoom* room)
{
    printf("\t(%d, %d) -- (%d, %d)\n", room->tl.x, room->tl.y, room->br.x, room->br.y);
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

int generate_rooms3(Dungeon* d)
{
    const size_t target = (size_t)random_in_range(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS);
    d->rooms_list.size = target;
    d->rooms_list.data = (DungeonRoom*)malloc(target * sizeof(*d->rooms_list.data));
    DungeonRoom* room_data = d->rooms_list.data;

    Vec2u d_min, d_max;
    vec2u_assign(&d_min, 1, 1);
    vec2u_assign(&d_max,
        DUNGEON_X_DIM - DUNGEON_ROOM_MIN_X - 1,
        DUNGEON_Y_DIM - DUNGEON_ROOM_MIN_Y - 1 );

    size_t ri;
    for(ri = 0; ri < DUNGEON_MIN_NUM_ROOMS; )  // generate at least the minimum number of rooms
    {
        vec2u_random_in_range(&room_data[ri].tl, d_min, d_max);
        room_data[ri].br.x = room_data[ri].tl.x + DUNGEON_ROOM_MIN_X;
        room_data[ri].br.y = room_data[ri].tl.y + DUNGEON_ROOM_MIN_Y;

        if(ri == 0)
        {
            ri++;
            continue;
        }

        int failed = 0;
        for(size_t j = 0; j < ri - 1; j++)
        {
            if(collide_or_tangent(room_data + ri, room_data + j))
            {
                printf("collision\n");
                print_room(room_data + ri);
                print_room(room_data + j);
                failed = 1;
                break;
            }
        }
        if(!failed) ri++;
    }
    printf("Took %lu iterations to generate core rooms.\n", ri);
    // for(size_t i = DUNGEON_MIN_NUM_ROOMS; i < target; i++)
    // {
    //     vec2u_random_in_range(&room_data[i].tl, d_min, d_max);
    //     room_data[i].br.x = room_data[i].tl.x + DUNGEON_ROOM_MIN_X;
    //     room_data[i].br.y = room_data[i].tl.y + DUNGEON_ROOM_MIN_Y;
    // }

    for(size_t r = 0; r < ri; r++)
    {
        const DungeonRoom* room = room_data + r;
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

    generate_rooms3(d);

    fill_printable(d);

    collide_or_tangent_test();

    return 0;
}

int destruct_dungeon(Dungeon* d)
{
    free(d->rooms_list.data);

    return 0;
}
