#include "dungeon_config.h"
#include "dungeon.h"

#include "util/vec_geom.h"
#include "util/debug.h"
#include "util/math.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <ncurses.h>


typedef struct
{
    uint8_t load : 1;
    uint8_t save : 1;
    uint8_t nmon;
    char* save_path;
    Vec2u8 pc_init;
}
RuntimeState;

int handle_level_init(DungeonLevel* d, RuntimeState* state, int argc, char** argv)
{
    int ret = 0;
    state->load = 0;
    state->save = 0;
    state->nmon = 0;
    state->save_path = NULL;

    #define MAX_ARGN 5
    int nmon_arg = 0;
    for(int n = 1; n < argc && n < MAX_ARGN; n++)
    {
        const char* arg = argv[n];
        if(!strncmp(arg, "--", 2))
        {
            state->load |= !strncmp(arg + 2, "load", 4);
            state->save |= !strncmp(arg + 2, "save", 4);
            if(!strncmp(arg + 2, "nummon", 6))
            {
                n++;
                state->nmon = atoi(argv[n]);
                nmon_arg = 1;
            }
        }
    }
    #undef MAX_ARGN

    if(state->load || state->save)
    {
        const char* home = getenv("HOME");
        const char* rel_save_dir = "/.rlg327";
        const char* rel_save_file = DUNGEON_FILE_NAME;

        const size_t home_strlen = strlen(home);
        const size_t rel_save_dir_strlen = strlen(rel_save_dir);
        const size_t rel_save_file_strlen = strlen(rel_save_file);

        state->save_path = (char*)malloc(home_strlen + rel_save_dir_strlen + rel_save_file_strlen + 1);
        if(!state->save_path) ret = -1;

        strcpy(state->save_path, home);
        strcat(state->save_path, rel_save_dir);
        mkdir(state->save_path, 0700);
        strcat(state->save_path, rel_save_file);
    }
    if(!nmon_arg)
    {
        state->nmon = RANDOM_IN_RANGE(DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS);
    }

    DungeonMap* map = &d->map;

    if(state->load)
    {
        PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", state->save_path);

        FILE* f = fopen(state->save_path, "rb");
        if(f)
        {
            deserialize_dungeon_map(map, &state->pc_init, f);
            fclose(f);
        }
        else
        {
            printf("ERROR: Failed to load dungeon from '%s' (file does not exist)\n", state->save_path);
            ret = -1;
        }
    }
    else
    {
        PRINT_DEBUG("GENERATING DUNGEON...\n")

        generate_dungeon_map(map, 0);
        random_dungeon_map_floor_pos(map, state->pc_init.data);
    }

    if(!ret) init_dungeon_level(d, state->pc_init, state->nmon);

    return ret;
}
int handle_level_deinit(DungeonLevel* d, RuntimeState* state)
{
    int ret = 0;

    if(state->save)
    {
        PRINT_DEBUG("SAVING DUNGEON TO '%s'\n", state->save_path)

        FILE* f = fopen(state->save_path, "wb");
        if(f)
        {
            serialize_dungeon_map(&d->map, &state->pc_init, f);
            fclose(f);
        }
        else
        {
            printf("ERROR: Failed to save dungeon to '%s'\n", state->save_path);
            ret = -1;
        }
    }

    free(state->save_path);

    return ret;
}



int print_win_lose(LevelStatus s, volatile int* r);

static volatile int is_running = 1;
static void handle_exit(int x)
{
    is_running = 0;
#if CURSES
    endwin();
#endif
}

inline static int one_o_four_main(int argc, char** argv)
{
    signal(SIGINT, handle_exit);

    DungeonLevel d;
    RuntimeState s;
    zero_dungeon_level(&d);
    srand(us_seed());

    if(!handle_level_init(&d, &s, argc, argv))
    {
        LevelStatus status;
        status.data = 0;
        uint64_t ntime_us = us_time();
        
        printf("\033[2J\033[1;1H");
        print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
        while(is_running && !status.data)
        {
            status = iterate_dungeon_level(&d, 1);
            printf("\033[2J\033[1;1H");
            print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
            // print_dungeon_level_costmaps(&d, DUNGEON_PRINT_BORDER);

            ntime_us += LEVEL_ITERATION_TIME_US;
            uint64_t n_us = us_time();
            if(n_us < ntime_us) usleep(ntime_us - n_us);
        }

        if(!is_running) printf("\nCaught Ctrl-C. Exitting...\n");
        if(status.data) print_win_lose(status, &is_running);
        // TODO: print win/lose screen
    }
    handle_level_deinit(&d, &s);

    destruct_dungeon_level(&d);

    return 0;
}

inline static int one_o_five_main(int argc, char** argv)
{
    DungeonLevel d;
    RuntimeState s;
    zero_dungeon_level(&d);
    srand(us_seed());

    if(!handle_level_init(&d, &s, argc, argv))
    {

    }
    handle_level_deinit(&d, &s);

    destruct_dungeon_level(&d);

    return 0;
}



int nc_configure();
int nc_print_border_backing();

int main(int argc, char** argv)
{
    initscr();
    // cbreak();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    signal(SIGINT, handle_exit);

    // nc_configure();
    nc_print_border_backing();
    refresh();

    for(;is_running;)
    {
        char pb[80];
        nodelay(stdscr, 0);
        int c = getch();
        nodelay(stdscr, 1);
        size_t i = 0;
        do
        {
            switch(c)
            {
                case 'Q':
                case 03:
                {
                    raise(SIGINT);
                    break;
                }
                case '7':
                case 'y':
                {
                    // move up + left
                    mvaddstr(i + 2, 1, "move up + left    ");
                    break;
                }
                case '8':
                case 'k':
                {
                    // move up
                    mvaddstr(i + 2, 1, "move up              ");
                    break;
                }
                case '9':
                case 'u':
                {
                    // move up + right
                    mvaddstr(i + 2, 1, "move up + right   ");
                    break;
                }
                case '6':
                case 'l':
                {
                    // move right
                    mvaddstr(i + 2, 1, "move right          ");
                    break;
                }
                case '3':
                case 'n':
                {
                    // move down + right
                    mvaddstr(i + 2, 1, "move down + right       ");
                    break;
                }
                case '2':
                case 'j':
                {
                    // move down
                    mvaddstr(i + 2, 1, "move down          ");
                    break;
                }
                case '1':
                case 'b':
                {
                    // move down + left
                    mvaddstr(i + 2, 1, "move down + left        ");
                    break;
                }
                case '4':
                case 'h':
                {
                    // move left
                    mvaddstr(i + 2, 1, "move left          ");
                    break;
                }
                case '>':
                {
                    // down stair
                    mvaddstr(i + 2, 1, "down stair         ");
                    break;
                }
                case '<':
                {
                    // up stair
                    mvaddstr(i + 2, 1, "up stair          ");
                    break;
                }
                case '5':
                case ' ':
                case '.':
                {
                    // rest
                    mvaddstr(i + 2, 1, "rest               ");
                    break;
                }
                case 'm':
                {
                    // display map
                    mvaddstr(i + 2, 1, "monsters           ");
                    break;
                }
                case KEY_UP:
                {
                    // scroll up
                    mvaddstr(i + 2, 1, "scroll up           ");
                    break;
                }
                case KEY_DOWN:
                {
                    // scroll down
                    mvaddstr(i + 2, 1, "scroll down          ");
                    break;
                }
                case 033:
                {
                    // escape
                    mvaddstr(i + 2, 1, "escape              ");
                    break;
                }
                default:
                {
                    snprintf(pb, 80, "Read input %#o          ", c);
                    mvaddstr(i + 2, 1, pb);
                }
            }
            i++;
        }
        while(is_running && ((c = getch()) != ERR) && i < 20);
    }

    return 0;
}
