#pragma once

#include <type_traits>
#include <array>

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>
#include <ncurses.h>

#include "util/vec_geom.hpp"
#include "util/math.h"
#include "dungeon_config.h"
#include "dungeon.h"
#include "entity.h"


template<NCURSES_COLOR_T GNum = 128, NCURSES_COLOR_T PairOff = 128>
class NCGradient_
{
    static_assert((GNum <= (256 - 8)) && (PairOff >= 8) && (PairOff + GNum <= 256));

public:
    using ColorElemT = NCURSES_COLOR_T;
    using ArrColorT = std::array<ColorElemT, 3>;
    using GradientT = std::array<ArrColorT, GNum>;

public:
    static void generate(
        GradientT& grad,
        const ArrColorT& a,
        const ArrColorT& b )
    {
        const ColorElemT
            dr = b[0] - a[0],
            dg = b[1] - a[1],
            db = b[2] - a[2];

        for(ColorElemT i = 0; i < static_cast<ColorElemT>(GNum); i++)
        {
            grad[i][0] = a[0] + ((dr * i) / static_cast<ColorElemT>(GNum - 1));
            grad[i][1] = a[1] + ((dg * i) / static_cast<ColorElemT>(GNum - 1));
            grad[i][2] = a[2] + ((db * i) / static_cast<ColorElemT>(GNum - 1));
        }
    }

public:
    inline NCGradient_(
        ColorElemT ar,
        ColorElemT ag,
        ColorElemT ab,
        ColorElemT br,
        ColorElemT bg,
        ColorElemT bb ) : NCGradient_( {ar, ag, ab}, {br, bg, bb} )
    {}
    inline NCGradient_(const ArrColorT& a, const ArrColorT& b)
    {
        generate(this->grad, a, b);
    }
    ~NCGradient_() = default;

public:
    void initialize()
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            init_color(PairOff + i, this->grad[i][0], this->grad[i][1], this->grad[i][2]);
        }
    }
    void applyForeground(ColorElemT bg)
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            const ColorElemT idx = (PairOff + i);

            init_color(idx, this->grad[i][0], this->grad[i][1], this->grad[i][2]);
            init_pair(idx, idx, bg);
        }
    }
    void applyBackground()
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            const ColorElemT idx = (PairOff + i);

            init_color(idx, grad[i][0], grad[i][1], grad[i][2]);
            init_pair(idx, (grad[i][0] + grad[i][1] + grad[i][2] > 1500 ? COLOR_BLACK : COLOR_WHITE), idx);
        }
    }

    void printChar(WINDOW* w, int y, int x, NCURSES_PAIRS_T idx, chtype c)
    {
        wattron(w, COLOR_PAIR(PairOff + idx));
        mvwaddch(w, y, x, c);
        wattroff(w, COLOR_PAIR(PairOff + idx));
    }

protected:
    GradientT grad;

};

using NCGradient = NCGradient_<>;

template<NCURSES_COLOR_T PairOff>
using NCGradient8 = NCGradient_<8, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient16 = NCGradient_<16, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient32 = NCGradient_<32, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient64 = NCGradient_<64, PairOff>;



class NCInitializer
{
public:
    static inline void use()
    {
        if(!nwin)
        {
            initscr();

            raw();
            noecho();
            curs_set(0);
            keypad(stdscr, TRUE);
            start_color();
            set_escdelay(0);
        }
        nwin++;
    }
    static inline void unuse()
    {
        if(nwin)
        {
            nwin--;
            if(!nwin)
            {
                endwin();
            }
        }
    }
    static inline void shutdown()
    {
        if(nwin)
        {
            nwin = 0;
            endwin();
        }
    }

protected:
    inline NCInitializer()
    {
        NCInitializer::use();
    }
    inline virtual ~NCInitializer()
    {
        NCInitializer::unuse();
    }

protected:
    inline static size_t nwin{ 0 };

};

// template<class Derived_T>
class NCWindow : public NCInitializer
{
    // static_assert(std::is_base_of<NCWindow<Derived_T>, Derived_T>::value);

public:
    inline NCWindow( int szy, int szx, int y, int x )
        : NCInitializer(), win{ newwin(szy, szx, y, x) }
    {}
    inline virtual ~NCWindow()
    {
        delwin(this->win);
    }

public:
    inline void setScrollable(bool s)
    {
        scrollok(this->win, s);
        idlok(this->win, s);
    }
    inline void printBox()
    {
        box(this->win, 0, 0);
    }

    // touches and refreshes
    inline void overwrite()
    {
        touchwin(this->win);
        wrefresh(this->win);
    }
    inline void refresh()
    {
        wrefresh(this->win);
    }

public:
    WINDOW* const win;

};



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
    void writeWeightMap(DungeonCostMap);

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





enum
{
    GWIN_NONE = 0,
    GWIN_MAP,
    GWIN_MLIST,
    NUM_GWIN
};

class Game
{
public:
    inline Game()
        : level{},
          map_win{ this->level },
          mlist_win{ this->level }
    {
        zero_dungeon_level(&this->level);
        this->state.active_win = this->state.displayed_win = GWIN_NONE;
    }
    inline ~Game() = default;

public:
    int run(volatile int* r);

    int overwrite_changes();

    int iterate_next_pc(LevelStatus& s);
    int iterate_pc_cmd(int move_cmd, LevelStatus& s);
    int handle_mlist_cmd(int mlist_cmd);
    int handle_dbg_cmd(int dbg_cmd);

public:
    DungeonLevel level;

    MapWindow map_win;
    MListWindow mlist_win;

    struct
    {
        uint8_t active_win : REQUIRED_BITS32(NUM_GWIN - 1);
        uint8_t displayed_win : REQUIRED_BITS32(NUM_GWIN - 1);
        bool is_goto_ctrl{ false };
    }
    state;
};
