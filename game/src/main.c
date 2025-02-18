#include "dungeon.h"
#include "dungeon_config.h"

#include "util/debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>



int handle_dungeon_init(Dungeon* d, uint8_t* pc, int argc, char** argv)
{
    int ret = 0;
    int load = 0;
    int save = 0;
    char* save_path = 0;

    if(argc > 1 && !strncmp(argv[1], "--", 2))
    {
        const char* home = getenv("HOME");
        const char* rel_save_dir = "/.rlg327";
        const char* rel_save_file = DUNGEON_FILE_NAME;

        size_t home_strlen = strlen(home);
        size_t rel_save_dir_strlen = strlen(rel_save_dir);
        size_t rel_save_file_strlen = strlen(rel_save_file);

        save_path = (char*)malloc(home_strlen + rel_save_dir_strlen + rel_save_file_strlen + 1);

        strcpy(save_path, home);
        strcat(save_path, rel_save_dir);
        mkdir(save_path, 0700);
        strcat(save_path, rel_save_file);

        // PRINT_DEBUG("SAVE PATH: %s\n", save_path);

        if((load |= !strncmp(argv[1] + 2, "load", 4))) {}
        else save |= !strncmp(argv[1] + 2, "save", 4);

        if(argc > 2 && !strncmp(argv[2], "--", 2))
        {
            if(!strncmp(argv[2] + 2, "load", 4)) load |= 1;
            else save |= !strncmp(argv[2] + 2, "save", 4);
        }
    }

    if(load)
    {
        PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", save_path);

        FILE* f = fopen(save_path, "rb");
        if(f)
        {
            deserialize_dungeon(d, f, pc);
            fclose(f);
        }
        else
        {
            printf("ERROR: Failed to load dungeon from '%s' (file does not exist)\n", save_path);
            ret = -1;
        }
    }
    else
    {
        PRINT_DEBUG("GENERATING DUNGEON...\n")

        generate_dungeon(d, us_seed());
        random_dungeon_floor_pos(d, pc);
    }
    if(save)
    {
        PRINT_DEBUG("SAVING DUNGEON TO '%s'\n", save_path)

        FILE* f = fopen(save_path, "wb");
        if(f)
        {
            serialize_dungeon(d, f, pc);
            fclose(f);
        }
        else
        {
            printf("ERROR: Failed to save dungeon to '%s'\n", save_path);
            ret = -1;
        }
    }

    if(save_path) free(save_path);

    return ret;
}

int main(int argc, char** argv)
{
    Dungeon d;
    uint8_t pc[] = { 0, 0 };
    zero_dungeon(&d);

IF_DEBUG(uint64_t t1 = us_time();)
    if(!handle_dungeon_init(&d, pc, argc, argv))
    {
    IF_DEBUG(uint64_t t2 = us_time();)
        print_dungeon(&d, pc, DUNGEON_PRINT_BORDER);
    IF_DEBUG(uint64_t t3 = us_time();)
        print_dungeon_a3(&d, pc, DUNGEON_PRINT_BORDER);
    IF_DEBUG(uint64_t t4 = us_time();)

    #if ENABLE_DEBUG_PRINTS
        printf(
            "RUNTIME: %f\n Init: %f\n Print: %f\n Weights: %f\n",
            (double)(t4 - t1) * 1e-6,
            (double)(t2 - t1) * 1e-6,
            (double)(t3 - t2) * 1e-6,
            (double)(t4 - t3) * 1e-6 );
    #endif
    }

    destruct_dungeon(&d);

    return 0;
}
