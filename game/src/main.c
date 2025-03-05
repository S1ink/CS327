#include "dungeon_config.h"
#include "dungeon.h"

#include "util/vec_geom.h"
#include "util/debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>


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

    #define MAX_ARGN 4
    int nmon_arg = 0;
    for(int n = 1; n < argc && n < (MAX_ARGN + 1); n++)
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
        state->nmon = (uint8_t)(rand() % DUNGEON_MAX_NUM_MONSTERS);
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

    init_dungeon_level(d, state->pc_init, state->nmon);

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



static volatile int is_running = 1;
static void handle_exit(int x)
{
    is_running = 0;
}

int main(int argc, char** argv)
{
    signal(SIGINT, handle_exit);

    DungeonLevel d;
    RuntimeState s;
    zero_dungeon_level(&d);
    srand(us_seed());

    if(!handle_level_init(&d, &s, argc, argv))
    {
        int status = 0;
        for( size_t i = 0;
            (i < 200000) && (is_running && !status);
            i++ )
        {
            printf("\033[2J\033[1;1H");
            print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
            // print_dungeon_level_costmaps(&d, DUNGEON_PRINT_BORDER);
            status = iterate_dungeon_level(&d);
            usleep(100000);
        }
        printf("\033[2J\033[1;1H");
        print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
        if(!is_running) printf("\nCaught Ctrl-C. Exitting...\n");
        if(status) printf("GAME %s!!!\n", status > 0 ? "WON" : "LOST");
        // TODO: print win/lose screen
    }
    handle_level_deinit(&d, &s);

    destruct_dungeon_level(&d);

    return 0;
}
