#include "game.hpp"

#include <cstring>


void GameApplication::initialize(int argc, char** argv)
{
// 1. Parse args
    #define MAX_ARGN 7
    int nmon = -1;
    uint32_t seed = 0;
    bool seed_arg = false;
    for(int n = 1; n < argc && n < MAX_ARGN; n++)
    {
        const char* arg = argv[n];
        if(!strncmp(arg, "--", 2))
        {
            this->runtime_args.load |= !strncmp(arg + 2, "load", 4);
            this->runtime_args.save |= !strncmp(arg + 2, "save", 4);
            if(!strncmp(arg + 2, "nummon", 6))
            {
                n++;
                nmon = atoi(argv[n]);
            }
            if(!strncmp(arg + 2, "seed", 4))
            {
                n++;
                seed = static_cast<uint32_t>(atoi(argv[n]));
                seed_arg = true;
            }
        }
    }
    #undef MAX_ARGN

    if(!seed_arg)
    {
        seed = static_cast<uint32_t>(std::random_device{}());
    }

    this->game.initRuntimeArgs(seed, nmon);

// 2. Load descriptions
    {
        std::ifstream f = DungeonFIO::openMonDescriptions();
        if(!f.is_open() || !this->game.initMonDescriptions(f))
        {
            // error
        }
        f.close();

        f = DungeonFIO::openObjDescriptions();
        if(!f.is_open() || !this->game.initItemDescriptions(f))
        {
            // error
        }
    }

// 3. Generate / load terrain
    if(this->runtime_args.load)
    {
        // PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", state->save_path);

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "rb");
        if(f)
        {
            this->game.initDungeonFile(f);
            fclose(f);
        }
        else
        {
            // fprintf(
            //     stderr,
            //     "ERROR: Failed to load dungeon from '%s' (file does not exist)\n",
            //     DungeonFIO::getLevelSaveFileName().c_str() );
        }
    }
    else
    {
        // PRINT_DEBUG("GENERATING DUNGEON...\n")

        this->game.initDungeonRandom();
    }
}

void GameApplication::shutdown()
{
    if(this->runtime_args.save)
    {
        // PRINT_DEBUG("SAVING DUNGEON TO '%s'\n", state->save_path)

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "wb");
        if(f)
        {
            this->game.exportDungeonFile(f);
            fclose(f);
        }
        else
        {
            // fprintf(
            //     stderr,
            //     "ERROR: Failed to save dungeon to '%s'\n", 
            //     DungeonFIO::getLevelSaveFileName().c_str() );
        }
    }
}
