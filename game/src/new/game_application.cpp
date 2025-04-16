#include "game.hpp"

#include <cstring>


void GameApplication::initialize(int argc, char** argv)
{
    int ret = 0;

    #define MAX_ARGN 7
    bool nmon_arg = false, seed_arg = false;
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
                this->runtime_args.nmon = static_cast<uint8_t>(atoi(argv[n]));
                nmon_arg = true;
            }
            if(!strncmp(arg + 2, "seed", 4))
            {
                n++;
                this->runtime_args.seed = static_cast<uint32_t>(atoi(argv[n]));
                seed_arg = true;
            }
        }
    }
    #undef MAX_ARGN

    if(!seed_arg)
    {
        this->runtime_args.seed = static_cast<uint32_t>(std::random_device{}());
    }
    this->game.procedural.rgen.seed(this->runtime_args.seed);

    if(nmon_arg)
    {
        this->game.procedural.nmon_distribution = std::uniform_int_distribution<uint32_t>{ 1, this->runtime_args.nmon };
    }

    // DungeonMap* map = &d->map;

    if(this->runtime_args.load)
    {
        // PRINT_DEBUG("LOADING DUNGEON FROM '%s'\n", state->save_path);

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "rb");
        if(f)
        {
            // deserialize_dungeon_map(map, &state->pc_init, f);
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
        // PRINT_DEBUG("GENERATING DUNGEON...\n")

        // generate_dungeon_map(map, 0);
        // random_dungeon_map_floor_pos(map, state->pc_init.data);
    }

    // if(!ret) init_dungeon_level(d, state->pc_init, state->nmon);

    // return ret;
}

void GameApplication::shutdown()
{
    int ret = 0;

    if(this->runtime_args.save)
    {
        // PRINT_DEBUG("SAVING DUNGEON TO '%s'\n", state->save_path)

        FILE* f = fopen(DungeonFIO::getLevelSaveFileName().c_str(), "wb");
        if(f)
        {
            // serialize_dungeon_map(&d->map, &state->pc_init, f);
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

    // return ret;
}
