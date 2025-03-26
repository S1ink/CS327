#pragma once

#include <stdint.h>

#include "util/vec_geom.h"
#include "util/heap.h"


enum
{
    MONSTAT_INTELLIGENCE    = 0b0001,
    MONSTAT_TELEPATHY       = 0b0010,
    MONSTAT_TUNNELING       = 0b0100,
    MONSTAT_ERRATIC         = 0b1000
};

class MonsterData
{
public:
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
};

class Entity
{
public:
    uint8_t is_pc;
    uint8_t speed;
    uint8_t priority;
    Vec2u8 pos;
    MonsterData md;

    size_t next_turn;

    HeapNode* hn;
};





// ------------------------------------------------
static inline char get_entity_char(const Entity* e)
{
    return e->is_pc ? '@' : ("0123456789ABCDEF")[e->md.stats];
}
