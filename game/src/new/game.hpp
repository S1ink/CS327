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
        mlist_win{ this->level },
        inv_win{ this->level }
    {}

public:
    void initRuntimeArgs(uint32_t seed, int nmon);
    bool initMonDescriptions(std::istream& i);
    bool initItemDescriptions(std::istream& i);
    bool initDungeonFile(FILE* f);
    bool initDungeonRandom();

    void run(const std::atomic<bool>& r);

    bool exportDungeonFile(FILE* f);

protected:
    inline uint32_t nextSeed()
    {
        return static_cast<uint32_t>(this->state.rgen());
    }

    bool initializeEntities();

    int overwrite_changes();

    int iterate_next_pc();
    int iterate_pc_cmd(int move_cmd, bool& was_nop);
    int handle_action_cmd(int action_cmd);
    int handle_mlist_cmd(int mlist_cmd);
    int handle_dbg_cmd(int dbg_cmd);

protected:
    enum
    {
        GWIN_NONE = 0,
        GWIN_MAP,
        GWIN_MLIST,
        GWIN_INVENTORY,
        NUM_GWIN
    };
    enum
    {
        UMODE_MOVE = 0,
        UMODE_LOOK,
        UMODE_GOTO,
        NUM_UMODE
    };

protected:
    class MapWindow : public NCWindow
    {
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
            : NCWindow(
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

    class MListWindow : public NCWindow
    {
    public:
        inline MListWindow(DungeonLevel& l)
            : NCWindow(
                MONLIST_WIN_Y_DIM,
                MONLIST_WIN_X_DIM,
                MONLIST_WIN_Y_OFF,
                MONLIST_WIN_X_OFF ),
            level{ &l },
            prox_gradient{ MONLIST_PROXIMITY_GRADIENT }
        {
            this->setScrollable(true);
        }
        inline virtual ~MListWindow() {}

    public:
        void onShow();
        void onScrollUp();
        void onScrollDown();

        void changeLevel(DungeonLevel& l);

    protected:
        void printEntry(const Entity& m, int line);

    protected:
        DungeonLevel* level;
        NCGradient16<16> prox_gradient;

        int scroll_amount{ 0 };

    };

    class InventoryWindow : public NCWindow
    {
    public:
        inline InventoryWindow(DungeonLevel& l)
            : NCWindow(
                INVENTORY_WIN_Y_DIM,
                INVENTORY_WIN_X_DIM,
                INVENTORY_WIN_Y_OFF,
                INVENTORY_WIN_X_OFF ),
            level{ &l }
        {}
        inline virtual ~InventoryWindow() {}

    public:
        void showEquipment();
        void showInventory();
        void showDescription(const Item* i);
        void showDescription(const Entity* e);

        void changeLevel(DungeonLevel& l);

    protected:
        DungeonLevel* level;

    };

protected:
    DungeonLevel level;

    MapWindow map_win;
    MListWindow mlist_win;
    InventoryWindow inv_win;

    std::vector<MonDescription> mon_desc;
    std::vector<ItemDescription> item_desc;

    std::unordered_map<const MonDescription*, bool> unique_availability;
    std::unordered_map<const ItemDescription*, bool> artifact_availability;

    struct
    {
        uint8_t active_win : REQUIRED_BITS32(NUM_GWIN - 1);
        uint8_t displayed_win : REQUIRED_BITS32(NUM_GWIN - 1);
        bool is_goto_ctrl{ false }; // TODO: remove
        uint8_t user_mode : REQUIRED_BITS32(NUM_UMODE - 1);

        uint32_t seed;
        int nmon;

        std::mt19937 rgen;
    }
    state;

};





class GameApplication
{
public:
    inline GameApplication(int argc, char** argv, const std::atomic<bool>& r) :
        game{},
        is_running{ r },
        runtime_args
        {
            .load{ false },
            .save{ false }
        }
    {
        this->initialize(argc, argv);
    }
    inline ~GameApplication() { this->shutdown(); }

    void run();

protected:
    inline GameApplication(const GameApplication&) = delete;
    inline GameApplication(GameApplication&&) = delete;

    void initialize(int argc, char** argv);
    void shutdown();

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
    GameState game;

    const std::atomic<bool>& is_running;

    struct
    {
        bool load;
        bool save;
    }
    runtime_args;

};
