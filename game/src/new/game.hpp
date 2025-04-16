#pragma once

#include <unordered_map>
#include <fstream>
#include <atomic>
#include <random>
#include <string>
#include <vector>
#include <cstdio>

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "util/vec_geom.hpp"
#include "util/nc_wrap.hpp"

#include "dungeon_config.h"

#include "spawning.hpp"
#include "dungeon.hpp"


class GameState
{
    friend class GameApplication;

public:
    inline GameState() :
        level{},
        map_win{ this->level },
        mlist_win{ this->level }
    {}

public:
    inline void initRuntimeArgs(uint32_t seed, int nmon)
    {
        this->state.seed = seed;
        this->state.nmon = nmon;

        this->procedural.rgen.seed(seed);
    }
    inline bool initMonDescriptions(std::istream& i)
    {
        return MonDescription::parse(i, this->mon_desc);
    }
    inline bool initItemDescriptions(std::istream& i)
    {
        return ItemDescription::parse(i, this->item_desc);
    }
    bool initDungeonFile(FILE* f);
    bool initDungeonRandom();

    bool exportDungeonFile(FILE* f);

protected:
    enum
    {
        GWIN_NONE = 0,
        GWIN_MAP,
        GWIN_MLIST,
        NUM_GWIN
    };

protected:
    class MListWindow : public NCWindow
    {
        using WindowBaseT = NCWindow;

    public:
        inline MListWindow(DungeonLevel& l)
            : WindowBaseT(
                MONLIST_WIN_Y_DIM,
                MONLIST_WIN_X_DIM,
                MONLIST_WIN_Y_OFF,
                MONLIST_WIN_X_OFF ),
            level{ &l },
            prox_gradient{ MONLIST_PROXIMITY_GRADIENT }
        {
            this->setScrollable(true);
        }
        inline virtual ~MListWindow() {};

    public:
        void onShow();
        void onScrollUp();
        void onScrollDown();

        void changeLevel(DungeonLevel& l);

    protected:
        void printEntry(Entity* m, int line);

    protected:
        DungeonLevel* level;
        NCGradient16<16> prox_gradient;

        int scroll_amount{ 0 };

    };

    class MapWindow : public NCWindow
    {
        using WindowBaseT = NCWindow;

    public:
        enum
        {
            MAP_FOG = 0,
            MAP_DUNGEON,
            MAP_HARDNESS,
            MAP_FWEIGHT,
            MAP_TWEIGHT
        };

    public:
        inline MapWindow(DungeonLevel& l)
            : WindowBaseT(
                DUNGEON_MAP_WIN_Y_DIM,
                DUNGEON_MAP_WIN_X_DIM,
                DUNGEON_MAP_WIN_Y_OFF,
                DUNGEON_MAP_WIN_X_OFF ),
            level{ &l },
            hardness_gradient{ DUNGEON_HARDNESS_GRADIENT },
            weightmap_gradient{ DUNGEON_WEIGHTMAP_GRADIENT }
        {
            this->printBox();
        }
        inline virtual ~MapWindow() {};

    public:
        void onPlayerMove(Vec2u8 a, Vec2u8 b);
        void onMonsterMove(Vec2u8 a, Vec2u8 b, bool terrain_changed = false);
        void onGotoMove(Vec2u8 a, Vec2u8 b);
        void onRefresh(bool force_rewrite = false);

        void changeMap(int mmode);
        void changeLevel(DungeonLevel& l, int mmode = -1);

    protected:
        void writeFogMap();
        void writeDungeonMap();
        void writeHardnessMap();
        void writeWeightMap(DungeonLevel::DungeonCostMap);

    protected:
        struct
        {
            int map_mode{ MAP_FOG }, fogless_map_mode{ MAP_DUNGEON };
            bool needs_rewrite{ false };
        }
        state;

        DungeonLevel* level;

        NCGradient hardness_gradient, weightmap_gradient;

    };

protected:
    DungeonLevel level;

    MapWindow map_win;
    MListWindow mlist_win;

    std::vector<MonDescription> mon_desc;
    std::vector<ItemDescription> item_desc;

    std::unordered_map<MonDescription*, bool> unique_availability;
    std::unordered_map<ItemDescription*, bool> artifact_availability;

    struct
    {
        uint8_t active_win : REQUIRED_BITS32(NUM_GWIN - 1);
        uint8_t displayed_win : REQUIRED_BITS32(NUM_GWIN - 1);
        bool is_goto_ctrl{ false };

        uint32_t seed;
        int nmon;
    }
    state;

    struct
    {
        std::mt19937 rgen;
    }
    procedural;

};



class GameApplication
{
public:
    inline GameApplication(int argc, char** argv, const std::atomic<bool>& r) :
        runtime_args
        {
            .load{ false },
            .save{ false }
        },
        game{},
        is_running{ r }
    {
        this->initialize(argc, argv);
    }

    void run();

protected:
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

protected:
    inline GameApplication(const GameApplication&) = delete;
    inline GameApplication(GameApplication&&) = delete;

    void initialize(int argc, char** argv);
    void shutdown();

protected:
    GameState game;

    const std::atomic<bool>& is_running;

    struct
    {
        bool load;
        bool save;
    }
    runtime_args;

};
