#pragma once

#include "util/math.h"

#include "dungeon.h"

#include <stdint.h>
#include <ncurses.h>


enum
{
    GWIN_NONE = 0,
    GWIN_MAP,
    GWIN_MLIST,
    NUM_GWIN
};

typedef struct
{
    WINDOW* map_win;
    WINDOW* mlist_win;

    DungeonLevel level;

    struct
    {
        uint8_t active_win : REQUIRED_BITS32(NUM_GWIN - 1);
        uint8_t displayed_win : REQUIRED_BITS32(NUM_GWIN - 1);
    }
    state;
}
Game;

int zero_game(Game* g);
int init_game_windows(Game* g);
int deinit_game_windows(Game* g);

int run_game(Game* g, volatile int* r);
