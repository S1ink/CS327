#include "dungeon.h"
#include "util/debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#ifndef PRINT_BOARDER
#define PRINT_BOARDER 1
#endif


int handle_dungeon_init(Dungeon* d, int argc, char** argv)
{
    int load = 0;
    int save = 0;
    char* save_path = 0;

    if(argc > 1 && !strncmp(argv[1], "--", 2))
    {
        const char* home = getenv("HOME");
        const char* rel_save_dir = "/.rlg327";
        const char* rel_save_file = "/dungeon";

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

        uint8_t pc[2];
        FILE* f = fopen(save_path, "rb");
        deserialize_dungeon(d, f, pc);
        d->printable[pc[1]][pc[0]] = '@';
        fclose(f);
    }
    else
    {
        PRINT_DEBUG("GENERATING DUNGEON...\n")

        generate_dungeon(d, time(NULL));
    }
    if(save)
    {
        PRINT_DEBUG("SAVING DUNGEON TO '%s'\n", save_path)

        FILE* f = fopen(save_path, "wb");
        serialize_dungeon(d, f, NULL);
        fclose(f);
    }

    if(save_path) free(save_path);

    return 0;
}

int main(int argc, char** argv)
{
    Dungeon d;
    zero_dungeon(&d);

    handle_dungeon_init(&d, argc, argv);
    print_dungeon(&d, PRINT_BOARDER);

    destruct_dungeon(&d);

    return 0;
}
