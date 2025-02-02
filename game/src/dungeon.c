#include "dungeon.h"

#include "util/perlin.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifndef DEBUG_PRINT_HARDNESS
#define DEBUG_PRINT_HARDNESS 1
#endif


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

    fill_printable(d);

    return 0;
}
