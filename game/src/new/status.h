#pragma once

#include <ncurses.h>

#include "dungeon_config.h"


#define NC_PRINT(...) \
    move(0, 0); \
    clrtoeol(); \
    mvprintw(0, 0, __VA_ARGS__); \
    refresh();

#define NC_PRINT2(...) \
    move((DUNGEON_Y_DIM + 1), 0); \
    clrtoeol(); \
    mvprintw((DUNGEON_Y_DIM + 1), 0, __VA_ARGS__); \
    refresh();

#define NC_PRINT3(...) \
    move((DUNGEON_Y_DIM + 2), 0); \
    clrtoeol(); \
    mvprintw((DUNGEON_Y_DIM + 2), 0, __VA_ARGS__); \
    refresh();
