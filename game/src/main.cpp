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
#include "new/generic_parse.hpp"
#include <iostream>

static inline int main_107(int argc, char** argv)
{
    auto f = DungeonFIO::openMonDescriptions();
    if(!f.is_open())
    {
        std::cout << "Monster description file does not exist!" << std::endl;
        return 0;
    }

    SequentialParser<MonDescription> monparser;
    monparser.addStartToken("BEGIN MONSTER");
    monparser.addEndToken("END");
    monparser.addStringToken("NAME", MonDescription::Name);
    monparser.addParagraphToken("DESC", MonDescription::Desc);
    monparser.addAttributeToken<uint8_t>(
        "COLOR",
        MonDescription::Colors,
        {
            { "RED", 1 << 0 },
            { "GREEN", 1 << 1 },
            { "BLUE", 1 << 2 },
            { "CYAN", 1 << 3 },
            { "YELLOW", 1 << 4 },
            { "MAGENTA", 1 << 5 },
            { "WHITE", 1 << 6 },
            { "BLACK", 1 << 7 }
        } );
    monparser.addRollableToken("SPEED", MonDescription::Speed);
    monparser.addAttributeToken<uint16_t>(
        "ABIL",
        MonDescription::Abilities,
        {
            { "SMART", 1 << 0 },
            { "TELE", 1 << 1 },
            { "TUNNEL", 1 << 2 },
            { "ERRATIC", 1 << 3 },
            { "PASS", 1 << 4 },
            { "PICKUP", 1 << 5 },
            { "DESTROY", 1 << 6 },
            { "UNIQ", 1 << 7 },
            { "BOSS", 1 << 8 },
        } );
    monparser.addRollableToken("HP", MonDescription::Health);
    monparser.addRollableToken("DAM", MonDescription::Attack);
    monparser.addPrimitiveToken<char>("SYMB", MonDescription::Symbol);
    monparser.addPrimitiveToken<uint8_t, int>("RRTY", MonDescription::Rarity);

    std::vector<MonDescription> md;
    monparser.parse(f, md);

    std::cout << "----------- MONSTER DESCRIPTIONS -----------\n" << std::endl;

    for(const MonDescription& m : md)
    {
        m.serialize(std::cout);
        std::cout << std::endl;
    }

    f.close();

    // -----------------

    f = DungeonFIO::openObjDescriptions();
    if(!f.is_open())
    {
        std::cout << "Item description file does not exist!" << std::endl;
        return 0;
    }

    SequentialParser<ItemDescription> itemparser;
    itemparser.addStartToken("BEGIN OBJECT");
    itemparser.addEndToken("END");
    itemparser.addStringToken("NAME", ItemDescription::Name);
    itemparser.addParagraphToken("DESC", ItemDescription::Desc);
    itemparser.addAttributeToken<uint32_t>(
        "TYPE",
        ItemDescription::Types,
        {
            { "WEAPON", ItemDescription::TYPE_WEAPON },
            { "OFFHAND", ItemDescription::TYPE_OFFHAND },
            { "RANGED", ItemDescription::TYPE_RANGED },
            { "ARMOR", ItemDescription::TYPE_ARMOR },
            { "HELMET", ItemDescription::TYPE_HELMET },
            { "CLOAK", ItemDescription::TYPE_CLOAK },
            { "GLOVES", ItemDescription::TYPE_GLOVES },
            { "BOOTS", ItemDescription::TYPE_BOOTS },
            { "RING", ItemDescription::TYPE_RING },
            { "AMULET", ItemDescription::TYPE_AMULET },
            { "LIGHT", ItemDescription::TYPE_LIGHT },
            { "SCROLL", ItemDescription::TYPE_SCROLL },
            { "BOOK", ItemDescription::TYPE_BOOK },
            { "FLASK", ItemDescription::TYPE_FLASK },
            { "GOLD", ItemDescription::TYPE_GOLD },
            { "AMMUNITION", ItemDescription::TYPE_AMMUNITION },
            { "FOOD", ItemDescription::TYPE_FOOD },
            { "WAND", ItemDescription::TYPE_WAND },
            { "CONTAINER", ItemDescription::TYPE_CONTAINER },
        } );
    itemparser.addAttributeToken<uint8_t>(
        "COLOR",
        ItemDescription::Colors,
        {
            { "RED", 1 << 0 },
            { "GREEN", 1 << 1 },
            { "BLUE", 1 << 2 },
            { "CYAN", 1 << 3 },
            { "YELLOW", 1 << 4 },
            { "MAGENTA", 1 << 5 },
            { "WHITE", 1 << 6 },
            { "BLACK", 1 << 7 }
        } );
    itemparser.addRollableToken("HIT", ItemDescription::Hit);
    itemparser.addRollableToken("DAM", ItemDescription::Damage);
    itemparser.addRollableToken("DODGE", ItemDescription::Dodge);
    itemparser.addRollableToken("DEF", ItemDescription::Defense);
    itemparser.addRollableToken("WEIGHT", ItemDescription::Weight);
    itemparser.addRollableToken("SPEED", ItemDescription::Speed);
    itemparser.addRollableToken("ATTR", ItemDescription::Special);
    itemparser.addRollableToken("VAL", ItemDescription::Value);
    itemparser.addAttributeToken<bool>("ART", ItemDescription::Artifact, { { "TRUE", true }, { "FALSE", false } });
    itemparser.addPrimitiveToken<uint8_t, int>("RRTY", ItemDescription::Rarity);

    std::vector<ItemDescription> id;
    itemparser.parse(f, id);

    std::cout << "----------- ITEM DESCRIPTIONS -----------\n" << std::endl;

    for(const ItemDescription& i : id)
    {
        i.serialize(std::cout);
        std::cout << std::endl;
    }

    return 0;
}

int main(int argc, char** argv)
{
    return main_107(argc, argv);
}
