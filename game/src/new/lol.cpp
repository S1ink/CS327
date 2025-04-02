#include <type_traits>
#include <array>

#include <limits.h>

#include <ncurses.h>

#include "../util/vec_geom.h"
#include "../dungeon_config.h"
#include "../dungeon.h"
#include "../entity.h"



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
    static void use()
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
    static void unuse()
    {
        nwin--;
        if(!nwin)
        {
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

template<class Derived_T>
class NCWindow : public NCInitializer
{
    static_assert(std::is_base_of<NCWindow<Derived_T>, Derived_T>::value);

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
        scrollok(this->win, true);
        idlok(this->win, true);
    }
    inline void printBox()
    {
        box(thiw->win, 0, 0);
    }

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



class MapWindow : public NCWindow<MapWindow>
{
    using WindowBaseT = NCWindow<MapWindow>;

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
    ~MapWindow() = default;

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
        int map_mode{ MAP_FOG };
        bool needs_rewrite{ false };
    }
    state;

    DungeonLevel* level;

    NCGradient hardness_gradient, weightmap_gradient;

};



void MapWindow::onPlayerMove(Vec2u8 a, Vec2u8 b)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        {
            // move the visible range -- need to handle clearing monsters that go out of range
            break;
        }
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            mvwaddch(this->win, a.y, a.x, get_terrain_char(this->level->map.terrain[a.y][a.x]));
            mvwaddch(this->win, b.y, b.x, get_entity_char(this->level->entities[b.y][b.x]));
            break;
        }
        case MAP_FWEIGHT :
        case MAP_TWEIGHT :
        {
            this->state.needs_rewrite = true;
            break;
        }
    }
}
void MapWindow::onMonsterMove(Vec2u8 a, Vec2u8 b, bool terrain_changed)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        {
            // update cell if in visible range
            break;
        }
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            mvwaddch(this->win, a.y, a.x, get_terrain_char(this->level->map.terrain[a.y][a.x]));
            mvwaddch(this->win, b.y, b.x, get_entity_char(this->level->entities[b.y][b.x]));
            break;
        }
        case MAP_FWEIGHT :
        case MAP_TWEIGHT :
        {
            this->state.needs_rewrite = terrain_changed;
            break;
        }
    }
}
void MapWindow::onGotoMove(Vec2u8 a, Vec2u8 b)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            mvwaddch(this->win, b.y, b.x, '*');
            mvwaddch(
                this->win,
                a.y,
                a.x,
                get_cell_char(
                    this->level->map.terrain[a.y][a.x],
                    this->level->entities[b.y][b.x] ) );
        }
        default: return;
    }
}

void MapWindow::onRefresh(bool force_rewrite)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        {
            if(force_rewrite)
            {
                this->writeFogMap();
            }
            break;
        }
        case MAP_DUNGEON :
        {
            if(force_rewrite)
            {
                this->writeDungeonMap();
            }
            break;
        }
        case MAP_HARDNESS :
        {
            if(force_rewrite)
            {
                this->writeHardnessMap();
            }
            break;
        }
        case MAP_FWEIGHT :
        case MAP_TWEIGHT :
        {
            if(force_rewrite || this->state.needs_rewrite)
            {
                this->writeWeightMap(
                    this->state.map_mode == MAP_FWEIGHT ?
                        this->level->tunnel_costs :
                        this->level->terrain_costs );
            }
            break;
        }
        default: return;
    }

    this->refresh();
}

void MapWindow::changeMap(int mmode)
{
    if(mmode != this->state.map_mode)
    {
        switch(mmode)
        {
            case MAP_FOG :
            case MAP_DUNGEON :
            case MAP_HARDNESS :
            {
                this->hardness_gradient.applyBackground();
                break;
            }
            case MAP_FWEIGHT :
            case MAP_TWEIGHT :
            {
                this->weightmap_gradient.applyForeground(COLOR_BLACK);
                break;
            }
            default: return;
        }

        this->state.map_mode = mmode;
        this->onRefresh(true);
    }
}

void MapWindow::changeLevel(DungeonLevel& l, int mmode)
{
    this->level = &l;

    if(mmode >= 0)
    {
        this->changeMap(mmode);
    }
    else
    {
        this->onRefresh(true);
    }
}


void MapWindow::writeFogMap()
{
    for(uint32_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        mvwaddnstr(this->win, y, 1, this->level->fog_map[y] + 1, DUNGEON_X_DIM - 2);
    }
}
void MapWindow::writeDungeonMap()
{
    char row[DUNGEON_X_DIM - 2];
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            row[x - 1] = get_cell_char(
                            this->level->map.terrain[y][x],
                            this->level->entities[y][x] );
        }
        mvwaddnstr(this->win, y, 1, row, DUNGEON_X_DIM - 2);
    }
}
void MapWindow::writeHardnessMap()
{
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            CellTerrain t = this->level->map.terrain[y][x];
            if(t.type)
            {
                mvwaddch(this->win, y, x, get_cell_char(t, this->level->entities[y][x]));
            }
            else
            {
                this->hardness_gradient.printChar(
                    this->win,
                    y,
                    x,
                    (this->level->map.hardness[y][x] / 2),
                    ' ' );
            }
        }
    }
}
void MapWindow::writeWeightMap(DungeonCostMap weights)
{
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            const int32_t w = weights[y][x];
            if(w == INT_MAX)
            {
                mvwaddch(this->win, y, x, ' ');
            }
            else
            {
                this->weightmap_gradient.printChar(
                    this->win,
                    y,
                    x,
                    (127 - MIN_CACHED(w * 2, 127)),
                    w == 0 ? '@' : (w % 10) + '0' );
            }
        }
    }
}














#include "../game.h"
#include "../util/math.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>

/* --- BEHAVIOR.C INTERFACE ------------------------------------------- */

/* Iterate the provided monster's AI and update the game state. */
int iterate_monster(DungeonLevel* d, Entity* e);
/* Move an entity. */
int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t x, uint8_t y);

/* -------------------------------------------------------------------- */


#define NC_PRINT(...) \
    move(0, 0); \
    clrtoeol(); \
    mvprintw(0, 0, __VA_ARGS__); \
    refresh();

enum
{
    MOVE_CMD_NONE = 0,
    MOVE_CMD_SKIP,
    MOVE_CMD_U,     // up
    MOVE_CMD_D,     // down
    MOVE_CMD_L,     // left
    MOVE_CMD_R,     // right
    MOVE_CMD_UL,
    MOVE_CMD_UR,
    MOVE_CMD_DL,
    MOVE_CMD_DR,
    MOVE_CMD_US,    // up stair
    MOVE_CMD_DS,    // down stair
    MOVE_CMD_GOTO,
    MOVE_CMD_RGOTO,
    NUM_MOVE_CMD
};
enum
{
    MLIST_CMD_NONE = 0,
    MLIST_CMD_SHOW,
    MLIST_CMD_HIDE,
    MLIST_CMD_SU,       // scroll up
    MLIST_CMD_SD,       // scroll down
    NUM_MLIST_CMD
};
enum
{
    DBG_CMD_NONE = 0,
    DBG_CMD_SHOW_TMAP,
    DBG_CMD_SHOW_FMAP,
    DBG_CMD_SHOW_HMAP,
    DBG_CMD_SHOW_DMAP,
    DBG_CMD_TOGGLE_FOG,
    NUM_DBG_CMD
};

typedef struct
{
    uint8_t move_cmd : REQUIRED_BITS32(NUM_MOVE_CMD - 1);
    uint8_t mlist_cmd : REQUIRED_BITS32(NUM_MLIST_CMD - 1);
    uint8_t dbg_cmd : REQUIRED_BITS32(NUM_DBG_CMD - 1);
    uint8_t exit : 1;
}
InputCommand;

static inline InputCommand zeroed_input()
{
    InputCommand ic;
    ic.move_cmd = 0;
    ic.mlist_cmd = 0;
    ic.dbg_cmd = 0;
    ic.exit = 0;
    return ic;
}

static inline InputCommand decode_input_command(int in)
{
    InputCommand cmd = zeroed_input();

    switch(in)
    {
    // --- EXIT ------------------
        case 'Q':
        case 03:    // Ctrl+C
        {
            cmd.exit = 1;
            break;
        }
    // --- MOVE COMMANDS ---------
        case '7':
        case 'y':
        {
            cmd.move_cmd = MOVE_CMD_UL;    // move up + left
            break;
        }
        case '8':
        case 'k':
        {
            cmd.move_cmd = MOVE_CMD_U;     // move up
            break;
        }
        case '9':
        case 'u':
        {
            cmd.move_cmd = MOVE_CMD_UR;    // move up + right
            break;
        }
        case '6':
        case 'l':
        {
            cmd.move_cmd = MOVE_CMD_R;     // move right
            break;
        }
        case '3':
        case 'n':
        {
            cmd.move_cmd = MOVE_CMD_DR;    // move down + right
            break;
        }
        case '2':
        case 'j':
        {
            cmd.move_cmd = MOVE_CMD_D;     // move down
            break;
        }
        case '1':
        case 'b':
        {
            cmd.move_cmd = MOVE_CMD_DL;    // move down + left
            break;
        }
        case '4':
        case 'h':
        {
            cmd.move_cmd = MOVE_CMD_L;     // move left
            break;
        }
        case '>':
        {
            cmd.move_cmd = MOVE_CMD_DS;    // down stair
            break;
        }
        case '<':
        {
            cmd.move_cmd = MOVE_CMD_US;    // up stair
            break;
        }
        case '5':
        case ' ':
        case '.':
        {
            cmd.move_cmd = MOVE_CMD_SKIP;  // rest
            break;
        }
        case 'g':
        {
            cmd.move_cmd = MOVE_CMD_GOTO;
            break;
        }
        case 'r':
        {
            cmd.move_cmd = MOVE_CMD_RGOTO;
            break;
        }
    // --- MONSTER LIST COMMANDS ----------
        case 'm':
        {
            cmd.mlist_cmd = MLIST_CMD_SHOW;    // display map
            break;
        }
        case KEY_UP:
        {
            cmd.mlist_cmd = MLIST_CMD_SU;      // scroll up
            break;
        }
        case KEY_DOWN:
        {
            cmd.mlist_cmd = MLIST_CMD_SD;      // scroll down
            break;
        }
        case 033:   // Esc key
        {
            cmd.mlist_cmd = MLIST_CMD_HIDE;    // escape
            break;
        }
    // --- DEBUG COMMANDS -----------------
        case 'T':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_TMAP;
            break;
        }
        case 'D':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_FMAP;
            break;
        }
        case 'H':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_HMAP;
            break;
        }
        case 's':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_DMAP;
            break;
        }
        case 'f':
        {
            cmd.dbg_cmd = DBG_CMD_TOGGLE_FOG;
            break;
        }
        default: break;
    }

    return cmd;
}

int zero_game(Game* g)
{
    zero_dungeon_level(&g->level);

    g->map_win = g->mlist_win = NULL;
    g->state.active_win = g->state.displayed_win = GWIN_NONE;

    return 0;
}

int init_game_windows(Game* g)
{
    nc_init();

    g->mlist_win = newwin(DUNGEON_Y_DIM - 2, DUNGEON_X_DIM - 2, DUNGEON_MAP_WIN_Y_OFF + 1, DUNGEON_MAP_WIN_X_OFF + 1);
    scrollok(g->mlist_win, TRUE);
    idlok(g->mlist_win, TRUE);

    return 0;
}

int deinit_game_windows(Game* g)
{
    delwin(g->map_win);
    delwin(g->mlist_win);
    nc_deinit();

    return 0;
}

int run_game(Game* g, volatile int* r)
{
    LevelStatus status;
    InputCommand ic = zeroed_input();
    int c;
    int scroll_amount = 0;

    nc_write_dungeon_map(g);
    g->state.active_win = GWIN_MAP;

    NC_PRINT("Welcome to the dungeon. Good luck! :)");

    for(ic.move_cmd = MOVE_CMD_SKIP, status.data = 0; !status.data && *r;)
    {
        const int is_currently_map = (g->state.active_win == GWIN_MAP);

        display_active_window(g);
        if(is_currently_map && ic.move_cmd && iterate_next_pc(g, &status, map_display_mode)) break;  // game done when iterate_next_pc() returns non-zero

        ic = decode_input_command((c = getch()));

        if(ic.exit)
        {
            raise(SIGINT);
        }
        else if(ic.move_cmd && is_currently_map)
        {
            iterate_pc(g, ic.move_cmd, &status, map_display_mode);
        }
        else if(ic.mlist_cmd)
        {
            handle_mlist_cmd(g, ic.mlist_cmd, &scroll_amount);
        }
        else if(ic.dbg_cmd)
        {
            handle_dbg_cmd(g, ic.dbg_cmd, &map_display_mode);
        }
        else
        {
            NC_PRINT("Unknown key: %#o", c);
        }
    }

    if(status.data)
    {
        // nc_print_win_lose(status, r);
        if(*r) getch();
    }

    return 0;
}
