#include "dungeon_config.h"
#include "dungeon.h"
#include "game.h"

#include "util/vec_geom.h"
#include "util/debug.h"
#include "util/math.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <ncurses.h>


class RuntimeState
{
public:
    uint8_t load : 1;
    uint8_t save : 1;
    uint8_t nmon;
    char* save_path;
    Vec2u8 pc_init;
};

static inline int handle_level_init(DungeonLevel* d, RuntimeState* state, int argc, char** argv)
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
static inline int handle_level_deinit(DungeonLevel* d, RuntimeState* state)
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

static volatile int is_running = 1;
static void handle_exit(int x)
{
    is_running = 0;
}
static inline void init_sig()
{
    signal(SIGINT, handle_exit);
    signal(SIGQUIT, handle_exit);
    signal(SIGILL, handle_exit);
    signal(SIGABRT, handle_exit);
    signal(SIGBUS, handle_exit);
    signal(SIGFPE, handle_exit);
    signal(SIGSEGV, handle_exit);
    signal(SIGTERM, handle_exit);
    signal(SIGSTKFLT, handle_exit);
    signal(SIGXCPU, handle_exit);
    signal(SIGXFSZ, handle_exit);
    signal(SIGPWR, handle_exit);
}



static inline int main_105(int argc, char** argv)
{
    Game g;
    RuntimeState s;

    zero_game(&g);
    srand(us_seed());
    init_sig();

    if( !handle_level_init(&g.level, &s, argc, argv) &&
        !init_game_windows(&g) )
    {
        run_game(&g, &is_running);
        deinit_game_windows(&g);
    }

    handle_level_deinit(&g.level, &s);
    destruct_dungeon_level(&g.level);

    return 0;
}



int main(int argc, char** argv)
{
    return main_105(argc, argv);
}
