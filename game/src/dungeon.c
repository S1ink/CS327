#include "dungeon.h"

#include "util/perlin.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifndef DEBUG_PRINT_HARDNESS
#define DEBUG_PRINT_HARDNESS 1
#endif


void vec2u_random(Vec2u* v, Vec2u range)
{
    v->x = (uint32_t)(rand() / (RAND_MAX / range.x));
    v->y = (uint32_t)(rand() / (RAND_MAX / range.y));
}

int collide_or_tangent(DungeonRoom* a, DungeonRoom* b)
{
    return (
        (a->br.x - b->tl.x >= -1) && (a->tl.x - b->br.x <= 1) &&
        (a->br.y - b->tl.y <= 1) && (a->tl.y - b->br.y >= -1) );
}

void random_in_range(DungeonRoom* r, Vec2u min, Vec2u max)
{
    Vec2u range, config_range, size, pos_range, pos;

    vec2u_assign( &config_range,
        (DUNGEON_ROOM_MAX_X - DUNGEON_ROOM_MIN_X),
        (DUNGEON_ROOM_MAX_Y - DUNGEON_ROOM_MIN_Y) );

    vec2u_sub(&range, &max, &min);
    vec2u_cwise_min(&range, &range, &config_range);

    vec2u_random(&size, range);
    vec2u_sub(&pos_range, &range, &size);

    vec2u_random(&pos, pos_range);

    vec2u_copy(&(r->tl), &pos);
    vec2u_add(&(r->br), &pos, &size);
}

void generate_rooms(Dungeon* d)
{
    // random roll for first room tl, size
    // calculate bounds for remaining 5 rooms -- random rolls within bounds for each
    // random roll for max number of additional rooms -- try to add if possible up to this amount

    DungeonRoom r1;
    random_in_range(&r1, , )
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

    generate_rooms(d);

    fill_printable(d);

    return 0;
}
