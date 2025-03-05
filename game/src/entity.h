#pragma once

#include <stdint.h>

#include "util/vec_geom.h"
#include "util/varray.h"
#include "util/heap.h"


enum MonsterStats
{
    INTELLIGENCE    = 0b0001,
    TELEPATHY       = 0b0010,
    TUNNELING       = 0b0100,
    ERRATIC         = 0b1000
};

typedef struct
{
    union
    {
        struct
        {
            uint8_t intelligence : 1;
            uint8_t telepathy : 1;
            uint8_t tunneling : 1;
            uint8_t erratic : 1;

            uint8_t using_rem_pos : 1;
        };
        struct
        {
            uint8_t stats : 4;
            uint8_t flags : 4;
        };
        uint8_t data;
    };

    Vec2u8 pc_rem_pos;
}
MonsterData;

size_t x = sizeof(MonsterData);

typedef struct
{
    uint8_t is_pc;
    uint8_t speed;
    uint8_t priority;
    Vec2u8 pos;
    MonsterData md;

    size_t next_turn;

    HeapNode* hn;

}
Entity;

size_t y = sizeof(Entity);
