#include "dungeon_config.h"
#include "dungeon.h"
// #include "game.h"
#include "new/lol.hpp"

#include "util/vec_geom.h"
#include "util/debug.h"
#include "util/math.h"

#include <string>
#include <fstream>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <ncurses.h>


class DungeonFIO
{
public:
    static inline const std::string& getDirectory()
    {
        if(DungeonFIO::directory.empty()) DungeonFIO::init();
        return DungeonFIO::directory;
    }

    static inline const std::string& getLevelSaveFileName()
    {
        if(DungeonFIO::directory.empty()) DungeonFIO::init();
        return DungeonFIO::level_save_fn;
    }
    static inline const std::string& getMonDescriptionsFileName()
    {
        if(DungeonFIO::directory.empty()) DungeonFIO::init();
        return DungeonFIO::mon_desc_fn;
    }
    static inline const std::string& openObjDescriptionsFileName()
    {
        if(DungeonFIO::directory.empty()) DungeonFIO::init();
        return DungeonFIO::obj_desc_fn;
    }

    static inline std::fstream openLevelSave()
    {
        return std::fstream{ DungeonFIO::getLevelSaveFileName() };
    }
    static inline std::ifstream openMonDescriptions()
    {
        return std::ifstream{ DungeonFIO::getMonDescriptionsFileName() };
    }
    static inline std::ifstream openObjDescriptions()
    {
        return std::ifstream{ DungeonFIO::openObjDescriptionsFileName() };
    }

protected:
    static void init()
    {
        (DungeonFIO::directory = getenv("HOME")) += "/.rlg327";
        mkdir(DungeonFIO::directory.c_str(), 0700);

        DungeonFIO::level_save_fn = DungeonFIO::directory + "/" DUNGEON_FILE_NAME;
        DungeonFIO::mon_desc_fn = DungeonFIO::directory + "/" MOSNTER_DESC_FILE_NAME;
        DungeonFIO::obj_desc_fn = DungeonFIO::directory + "/" OBJECT_DESC_FILE_NAME;
    }

protected:
    static inline std::string directory;
    static inline std::string level_save_fn;
    static inline std::string mon_desc_fn;
    static inline std::string obj_desc_fn;

};


class RuntimeState
{
public:
    uint8_t load : 1;
    uint8_t save : 1;
    uint8_t nmon;
    Vec2u8 pc_init;
};

static inline int handle_level_init(DungeonLevel* d, RuntimeState* state, int argc, char** argv)
{
    int ret = 0;
    state->load = 0;
    state->save = 0;
    state->nmon = 0;

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

    if(!nmon_arg)
    {
        state->nmon = RANDOM_IN_RANGE(DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS);
    }

    DungeonMap* map = &d->map;

    if(state->load)
    {
        PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", state->save_path);

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "rb");
        if(f)
        {
            deserialize_dungeon_map(map, &state->pc_init, f);
            fclose(f);
        }
        else
        {
            fprintf(
                stderr,
                "ERROR: Failed to load dungeon from '%s' (file does not exist)\n",
                DungeonFIO::getLevelSaveFileName().c_str() );

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

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "wb");
        if(f)
        {
            serialize_dungeon_map(&d->map, &state->pc_init, f);
            fclose(f);
        }
        else
        {
            fprintf(
                stderr,
                "ERROR: Failed to save dungeon to '%s'\n", 
                DungeonFIO::getLevelSaveFileName().c_str() );

            ret = -1;
        }
    }

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



static inline int main_106(int argc, char** argv)
{
    Game g;
    RuntimeState s;

    srand(us_seed());
    init_sig();

    if( !handle_level_init(&g.level, &s, argc, argv) )
    {
        g.run(&is_running);
    }

    handle_level_deinit(&g.level, &s);
    destruct_dungeon_level(&g.level);

    return 0;
}



#include "new/items.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    // return main_106(argc, argv);

    auto f = DungeonFIO::openMonDescriptions();

    std::vector<MonDescription> md;
    MonDescription::parse(f, md);

    for(const MonDescription& m : md)
    {
        std::string mm;
        memToStr(m, mm);
        std::cout << "[[ " << mm << " ]]\n" << std::endl;

        m.serialize(std::cout);
        std::cout << std::endl;
    }
}
