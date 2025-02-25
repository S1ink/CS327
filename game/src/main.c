#include "dungeon.h"
#include "dungeon_config.h"

#include "util/debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>


typedef struct
{
    uint8_t load : 1;
    uint8_t save : 1;
    uint8_t nmon;
    char* save_path;
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
    Vec2u8* pc = &d->pc_position;

    if(state->load)
    {
        PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", state->save_path);

        FILE* f = fopen(state->save_path, "rb");
        if(f)
        {
            deserialize_dungeon_map(map, pc, f);
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
        random_dungeon_map_floor_pos(map, pc->data);
    }

    init_level(d, state->nmon);

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
            serialize_dungeon_map(&d->map, &d->pc_position, f);
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
    printf("\nCaught Ctrl-C. Exitting...\n");
    is_running = 0;
}

int main(int argc, char** argv)
{
    signal(SIGINT, handle_exit);

    DungeonLevel d;
    RuntimeState s;
    zero_dungeon_level(&d);
    srand(us_seed());

    // IF_DEBUG(uint64_t t1 = us_time();)
    if(!handle_level_init(&d, &s, argc, argv))
    {
        int status;
        int i = 0;
        do
        {
            i++;
            usleep(250000);
            printf("\033[2J\033[1;1H");
            print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
        }
        while(is_running && !(status = iterate_level(&d)) && i < 10);
        // TODO: print win/lose screen

    // IF_DEBUG(uint64_t t2 = us_time();)
    //     print_dungeon_level(&d, DUNGEON_PRINT_BORDER);
    // IF_DEBUG(uint64_t t3 = us_time();)
    // //     print_dungeon_level_a3(&d, DUNGEON_PRINT_BORDER);
    // IF_DEBUG(uint64_t t4 = us_time();)

    // #if ENABLE_DEBUG_PRINTS
    //     printf(
    //         "RUNTIME: %f\n Init: %f\n Print: %f\n Weights: %f\n",
    //         (double)(t4 - t1) * 1e-6,
    //         (double)(t2 - t1) * 1e-6,
    //         (double)(t3 - t2) * 1e-6,
    //         (double)(t4 - t3) * 1e-6 );
    // #endif
    }
    handle_level_deinit(&d, &s);

    destruct_dungeon_level(&d);

    return 0;
}
