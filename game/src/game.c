#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>

#include <ncurses.h>

#include "util/debug.h"
#include "util/math.h"

#include "dungeon_config.h"
#include "dungeon.h"
#include "entity.h"


#if CURSES
int nc_configure()
{
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    // init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);

    return 0;
}

int nc_print_border_backing()
{
    mvaddstr(1, 0,
        "+------------------------------------------------------------------------------+\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "+------------------------------------------------------------------------------+" );

    return 0;
}

int nc_print_dungeon_level(DungeonLevel* d)
{
    return 0;
}

int nc_print_win_lose(LevelStatus s, volatile int* r)
{
    return 0;
}


// #else
int print_win_lose(LevelStatus s, volatile int* r)
{
    char lose[] =
        "\033[2J\033[1;1H"
        "+------------------------------------------------------------------------------+\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                .@       You encountered a problem and need to restart.       |\n"
        "|               @@                                                             |\n"
        "|      @@      @@         We're just collecting some error info, and then      |\n"
        "|             .@+                we'll restart for you (no we won't lol).      |\n"
        "|             @@                                                               |\n"
        "|             @@                                                               |\n"
        "|             @@                                                               |\n"
        "|             *@+                                                              |\n"
        "|      @@      @@                                                              |\n"
        "|               @@                                                             |\n"
        "|                *@                                                            |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|     [                                                    ]    % complete     |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "+------------------------------------------------------------------------------+\n";

    char win[] =
        "\033[2J\033[1;1H"
        "+------------------------------------------------------------------------------+\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|             @.           Congratulations. You are victorious.                |\n"
        "|              @@                                                              |\n"
        "|      @@       @@                                                             |\n"
        "|               +@.                                                            |\n"
        "|                @@                                                            |\n"
        "|                @@                                                            |\n"
        "|                @@                                                            |\n"
        "|               +@*                                                            |\n"
        "|      @@       @@                                                             |\n"
        "|              @@                                                              |\n"
        "|             @*                                                               |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "+------------------------------------------------------------------------------+\n";

    #define MIN_PERCENT_CHUNK       12
    #define MAX_PERCENT_CHUNK       37
    #define LOADING_BAR_START_IDX   1637
    #define LOADING_BAR_LEN         51
    #define PERCENT_START_IDX       1691
    #define MIN_PAUSE_MS            200
    #define MAX_PAUSE_MS            700

    if(s.has_lost)
    {
        uint8_t p = 0;
        for(; p <= 100 && *r; p = MIN_CACHED(p + RANDOM_IN_RANGE(MIN_PERCENT_CHUNK, MAX_PERCENT_CHUNK), 100))
        {
            uint8_t px = (p * LOADING_BAR_LEN) / 100;
            for(int i = LOADING_BAR_START_IDX; i <= LOADING_BAR_START_IDX + px; i++)
            {
                lose[i] = '#';
            }
            lose[PERCENT_START_IDX + 0] = p >= 100 ? '1' : ' ';
            lose[PERCENT_START_IDX + 1] = " 1234567890"[(p / 10)];
            lose[PERCENT_START_IDX + 2] = '0' + (p % 10);

            printf("%s", lose);
            if(p >= 100) break;
            usleep(1000 * RANDOM_IN_RANGE(MIN_PAUSE_MS, MAX_PAUSE_MS));
        }
    }
    else
    {
        printf("%s", win);
    }

    return 0;

    #undef MIN_PERCENT_CHUNK
    #undef MAX_PERCENT_CHUNK
    #undef LOADING_BAR_START_IDX
    #undef LOADING_BAR_LEN
    #undef PERCENT_START_IDX
    #undef MIN_PAUSE_MS
    #undef MAX_PAUSE_MS
}
#endif
