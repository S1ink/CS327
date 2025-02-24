#pragma once

#include <stdint.h>

#include "util/vec_geom.h"


enum MonsterStats
{
    INTELLIGENCE    = 0b0001,
    TELEPATHY       = 0b0010,
    TUNNELING       = 0b0100,
    ERRATIC         = 0b1000
};

typedef struct
{
    uint8_t speed;
    uint8_t stats : 4;
    uint8_t is_pc : 1;
}
Entity;
