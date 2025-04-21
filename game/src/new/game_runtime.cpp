#include "game.hpp"

#include "util/debug.hpp"


#define NC_PRINT(...) \
    move(0, 0); \
    clrtoeol(); \
    mvprintw(0, 0, __VA_ARGS__); \
    refresh();

#define NC_PRINT2(...) \
    move((DUNGEON_Y_DIM + 1), 0); \
    clrtoeol(); \
    mvprintw((DUNGEON_Y_DIM + 1), 0, __VA_ARGS__); \
    refresh();


void GameState::MListWindow::onShow()
{
    this->prox_gradient.applyForeground(COLOR_BLACK);

    werase(this->win);
    this->scroll_amount = 0;

    size_t alive_i = 0;
    for( auto m = this->level->npcs.begin();
        m != this->level->npcs.end() && alive_i < MONLIST_WIN_Y_DIM;
        m++ )
    {
        if(m->state.health > 0)
        {
            this->printEntry(*m, alive_i);

            alive_i++;
        }
    }

    this->refresh();
}

void GameState::MListWindow::onScrollUp()
{
    if((int)this->level->npcs_remaining - this->scroll_amount > MONLIST_WIN_Y_DIM)
    {
        this->scroll_amount += 1;
        wscrl(this->win, 1);

        const int target_mnum = (MONLIST_WIN_Y_DIM - 1) + this->scroll_amount;
        auto m = this->level->npcs.begin();
        for(int mon = 0; m->state.health <= 0 || mon < target_mnum; m++)
        {
            if(m->state.health) mon++;
        }

        this->printEntry(*m, (MONLIST_WIN_Y_DIM - 1));
        this->refresh();
    }
}

void GameState::MListWindow::onScrollDown()
{
    if(this->scroll_amount > 0)
    {
        this->scroll_amount -= 1;
        wscrl(this->win, -1);

        auto m = this->level->npcs.begin();
        for(int mon = 0; !m->state.health || mon < this->scroll_amount; m++)
        {
            if(m->state.health) mon++;
        }

        this->printEntry(*m, 0);
        this->refresh();
    }
}

void GameState::MListWindow::changeLevel(DungeonLevel& l)
{
    this->level = &l;
}

void GameState::MListWindow::printEntry(const Entity& m, int line)
{
    const int
        dx = (int)this->level->pc.state.pos.x - (int)m.state.pos.x,
        dy = (int)this->level->pc.state.pos.y - (int)m.state.pos.y,
        ds = m.config.can_tunnel ?
            DungeonLevel::accessGridElem(this->level->terrain_costs, m.state.pos) :
            DungeonLevel::accessGridElem(this->level->tunnel_costs, m.state.pos),
        ci = (MIN(63, ds) >> 2);

    // NC_PRINT("trav weight is %d", ds);

    wmove(this->win, line, 0);
    wclrtoeol(this->win);

    this->prox_gradient.printf(
        this->win, line, 0, ci, "[%s] : %d %s, %d %s",
        m.config.name.data(),
        abs(dx),
        dx > 0 ? "West" : "East",
        abs(dy),
        dy > 0 ? "North" : "South" );

}





void GameState::MapWindow::onPlayerMove(Vec2u8 a, Vec2u8 b)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        {
            // move the visible range -- need to handle clearing monsters that go out of range
            this->writeFogMap();
            break;
        }
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            this->level->writeChar(this->win, a);
            this->level->writeChar(this->win, b);
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
void GameState::MapWindow::onMonsterMove(Vec2u8 a, Vec2u8 b, bool terrain_changed)
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
            this->level->writeChar(this->win, a);
            this->level->writeChar(this->win, b);
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
void GameState::MapWindow::onGotoMove(Vec2u8 a, Vec2u8 b)
{
    switch(this->state.map_mode)
    {
        case MAP_FOG :
        {
            mvwaddch(this->win, a.y, a.x, this->level->visibility_map[a.y][a.x]);   // TODO: doesn't handle items/monsters in current range
            mvwaddch(this->win, b.y, b.x, '*');
            break;
        }
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            this->level->writeChar(this->win, a);
            mvwaddch(this->win, b.y, b.x, '*');
            break;
        }
        default: return;
    }
}

void GameState::MapWindow::onRefresh(bool force_rewrite)
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

    this->state.needs_rewrite = false;

    this->refresh();
}

void GameState::MapWindow::changeMap(int mmode)
{
    if(this->state.map_mode == MAP_FOG && mmode == MAP_FOG)
    {
        mmode = this->state.fogless_map_mode;
    }
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

        if(mmode != MAP_FOG)
        {
            this->state.fogless_map_mode = mmode;
        }
        this->state.map_mode = mmode;
        this->onRefresh(true);
    }
}

void GameState::MapWindow::changeLevel(DungeonLevel& l, int mmode)
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


void GameState::MapWindow::writeFogMap()
{
    for(uint32_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        mvwaddnstr(this->win, y, 1, this->level->visibility_map[y] + 1, DUNGEON_X_DIM - 2);
    }
    // if(this->level->pc)
    // {
        for(size_t i = 0; i < 21; i++)
        {
            const auto v = DungeonLevel::VIS_OFFSETS[i];
            const int8_t y = static_cast<int8_t>(this->level->pc.state.pos.y) + v[0];
            const int8_t x = static_cast<int8_t>(this->level->pc.state.pos.x) + v[1];

            if( (y >= 1 && y < DUNGEON_Y_DIM - 1 && x >= 1 && x < DUNGEON_X_DIM - 1) )
                // (this->level->entity_map[y][x] || this->level->item_map[y][x]) )
            {
                this->level->writeChar(this->win, {x, y});
            }
        }
    // }
}
void GameState::MapWindow::writeDungeonMap()
{
    // char row[DUNGEON_X_DIM - 2];
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            this->level->writeChar(this->win, {x, y});
            // row[x - 1] = get_cell_char(
            //                 this->level->map.terrain[y][x],
            //                 this->level->entities[y][x] );
        }
        // mvwaddnstr(this->win, y, 1, row, DUNGEON_X_DIM - 2);
    }
}
void GameState::MapWindow::writeHardnessMap()
{
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            DungeonLevel::TerrainMap::Cell t = this->level->map.terrain[y][x];
            if(t.type)
            {
                this->level->writeChar(this->win, {x, y});
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
void GameState::MapWindow::writeWeightMap(DungeonLevel::DungeonCostMap weights)
{
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            const int32_t w = weights[y][x];
            if(w == std::numeric_limits<int32_t>::max())
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















/* --- BEHAVIOR.C INTERFACE ------------------------------------------- */

/* Iterate the provided monster's AI and update the game state. */
int iterate_monster(DungeonLevel* d, Entity* e);
/* Move an entity. */
int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t x, uint8_t y, bool is_goto);

/* -------------------------------------------------------------------- */


static int nc_print_win_lose(int s, const std::atomic<bool>& r)
{
    if(!s) return 0;

    char lose[] =
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                 .@       You encountered a problem and need to restart.        \n"
        "                @@                                                              \n"
        "       @@      @@         We're just collecting some error info, and then       \n"
        "              .@+                we'll restart for you (no we won't lol).       \n"
        "              @@                                                                \n"
        "              @@                                                                \n"
        "              @@                                                                \n"
        "              *@+                                                               \n"
        "       @@      @@                                                               \n"
        "                @@                                                              \n"
        "                 *@                                                             \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "      [                                                    ]    % complete      \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n";

    char win[] =
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "              @.               Congratulations. You are victorious.             \n"
        "               @@                                                               \n"
        "       @@       @@                                                              \n"
        "                +@.                                                             \n"
        "                 @@                                                             \n"
        "                 @@                                                             \n"
        "                 @@                                                             \n"
        "                +@*                                                             \n"
        "       @@       @@                                                              \n"
        "               @@                                                               \n"
        "              @*                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n"
        "                                                                                \n";

    #define MIN_PERCENT_CHUNK       3
    #define MAX_PERCENT_CHUNK       41
    #define LOADING_BAR_START_IDX   1627
    #define LOADING_BAR_LEN         51
    #define PERCENT_START_IDX       1681
    #define MIN_PAUSE_MS            200
    #define MAX_PAUSE_MS            700

    init_color(8, 200, 400, 800);
    init_pair(8, COLOR_WHITE, 8);
    attron(COLOR_PAIR(8));

    if(s < 0)
    {
        mvaddstr(0, 0, lose);

        uint8_t p = 0;
        for(; p <= 100 && r; p = MIN_CACHED(p + RANDOM_IN_RANGE(MIN_PERCENT_CHUNK, MAX_PERCENT_CHUNK), 100))
        {
            uint8_t px = (p * LOADING_BAR_LEN) / 100;
            for(int i = LOADING_BAR_START_IDX; i <= LOADING_BAR_START_IDX + px; i++)
            {
                lose[i] = '#';
            }
            lose[PERCENT_START_IDX + 0] = p >= 100 ? '1' : ' ';
            lose[PERCENT_START_IDX + 1] = " 1234567890"[(p / 10)];
            lose[PERCENT_START_IDX + 2] = '0' + (p % 10);

            mvaddnstr(20, 7, lose + LOADING_BAR_START_IDX, 70);
            refresh();

            if(p >= 100) break;
            usleep(1000 * RANDOM_IN_RANGE(MIN_PAUSE_MS, MAX_PAUSE_MS));
        }

        attron(WA_BLINK);
        mvaddstr(14, 38, "Press any key to continue.");
        attroff(WA_BLINK);
        refresh();
    }
    else
    {
        mvaddstr(0, 0, win);
        refresh();
        if(r)
        {
            usleep(1000000);
            attron(WA_BLINK);
            mvaddstr(14, 38, "Press any key to continue.");
            attroff(WA_BLINK);
            refresh();
        }
    }

    attroff(COLOR_PAIR(8));

    return 0;

    #undef MIN_PERCENT_CHUNK
    #undef MAX_PERCENT_CHUNK
    #undef LOADING_BAR_START_IDX
    #undef LOADING_BAR_LEN
    #undef PERCENT_START_IDX
    #undef MIN_PAUSE_MS
    #undef MAX_PAUSE_MS
}

// ENUMS: base commands are MOVE..., ACTION..., and DBG...
// Mode specific commands are listed for sub-ctrl loops: MLIST, LOOK, GOTO
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
    MOVE_CMD_GOTO,  // TODO: delete
    MOVE_CMD_RGOTO, // TODO: delete
    NUM_MOVE_CMD
};
enum
{
    ACTION_CMD_NONE = 0,
    ACTION_CMD_WEAR,        // --> change to INVENTORY window
    ACTION_CMD_UNWEAR,      // --> change to EQUIPMENT window
    ACTION_CMD_DROP,        // --> change to INVENTORY window
    ACTION_CMD_EXPUNGE,     // --> change to INVENTORY window
    ACTION_CMD_INVENTORY,   // --> change to INVENTORY window
    ACTION_CMD_EQUIPMENT,   // --> change to EQUIPMENT window
    ACTION_CMD_INSPECT,     // --> change to INVENTORY window
    ACTION_CMD_MLIST,       // --> change to MLIST window
    ACTION_CMD_LOOK,        // --> change to LOOK mode
    ACTION_CMD_GOTO,        // --> change to GOTO mode
    NUM_ACTION_CMD
};
enum
{
    DBG_CMD_NONE = 0,
    DBG_CMD_TOGGLE_FOG,
    DBG_CMD_SHOW_DUNGEON,
    DBG_CMD_SHOW_HARDNESS,
    DBG_CMD_SHOW_FWEIGHTS,
    DBG_CMD_SHOW_TWEIGHTS,
    NUM_DBG_CMD
};

enum
{
    MLIST_CMD_NONE = 0,
    MLIST_CMD_SHOW,
    MLIST_CMD_ESCAPE,
    MLIST_CMD_SU,       // scroll up
    MLIST_CMD_SD,       // scroll down
    NUM_MLIST_CMD
};
enum
{
    LOOK_CMD_NONE = 0,
    LOOK_CMD_ESCAPE,
    LOOK_CMD_CONFIRM,
    NUM_LOOK_CMD
};
enum
{
    GOTO_CMD_NONE = 0,
    GOTO_CMD_ESCAPE,
    GOTO_CMD_CONFIRM,
    GOTO_CMD_RANDOM,
    NUM_GOTO_CMD
};

class UserInput
{
public:
    static bool checkExit(int in);
    static uint8_t checkMoveDir(int in);
    static uint8_t checkMove(int in);
    static uint8_t checkAction(int in);
    static uint8_t checkDbg(int in);
    static uint8_t checkMlist(int in);
    static uint8_t checkLook(int in);
    static uint8_t checkGoto(int in);
    static uint8_t checkEquipSlot(int in);
    static uint8_t checkCarrySlot(int in);
};

bool UserInput::checkExit(int in)
{
    return in == 'Q' || in == 03;   // Ctrl+C
}
uint8_t UserInput::checkMoveDir(int in)
{
    switch(in)
    {
        case '7':
        case 'y': return MOVE_CMD_UL;    // move up + left
        case '8':
        case 'k': return MOVE_CMD_U;     // move up
        case '9':
        case 'u': return MOVE_CMD_UR;    // move up + right
        case '6':
        case 'l': return MOVE_CMD_R;     // move right
        case '3':
        case 'n': return MOVE_CMD_DR;    // move down + right
        case '2':
        case 'j': return MOVE_CMD_D;     // move down
        case '1':
        case 'b': return MOVE_CMD_DL;    // move down + left
        case '4':
        case 'h': return MOVE_CMD_L;     // move left
    }
    return 0;
}
uint8_t UserInput::checkMove(int in)
{
    if(uint8_t u = checkMoveDir(in); u) return u;
    switch(in)
    {
        case '>': return MOVE_CMD_DS;    // down stair
        case '<': return MOVE_CMD_US;    // up stair
        case '5':
        case ' ':
        case '.': return MOVE_CMD_SKIP;  // rest
        case 'g': return MOVE_CMD_GOTO;
        case 'r': return MOVE_CMD_RGOTO;
    }
    return 0;
}
uint8_t UserInput::checkAction(int in)
{
    switch(in)
    {
        case 'w': return ACTION_CMD_WEAR;
        case 't': return ACTION_CMD_UNWEAR;
        case 'd': return ACTION_CMD_DROP;
        case 'x': return ACTION_CMD_EXPUNGE;
        case 'i': return ACTION_CMD_INVENTORY;
        case 'e': return ACTION_CMD_EQUIPMENT;
        case 'I': return ACTION_CMD_INSPECT;
        case 'm': return ACTION_CMD_MLIST;
        case 'L': return ACTION_CMD_LOOK;
        case 'g': return ACTION_CMD_GOTO;
    }
    return 0;
}
uint8_t UserInput::checkDbg(int in)
{
    switch(in)
    {
        case 'f': return DBG_CMD_TOGGLE_FOG;
        case 's': return DBG_CMD_SHOW_DUNGEON;
        case 'H': return DBG_CMD_SHOW_HARDNESS;
        case 'D': return DBG_CMD_SHOW_FWEIGHTS;
        case 'T': return DBG_CMD_SHOW_TWEIGHTS;
    }
    return 0;
}
uint8_t UserInput::checkMlist(int in)
{
    switch(in)
    {
        case 'm': return MLIST_CMD_SHOW;    // display map
        case 033: return MLIST_CMD_ESCAPE;    // escape
        case KEY_UP: return MLIST_CMD_SU;      // scroll up
        case KEY_DOWN: return MLIST_CMD_SD;      // scroll down
    }
    return 0;
}
uint8_t UserInput::checkLook(int in)
{
    switch(in)
    {
        case 033: return LOOK_CMD_ESCAPE;
        case 't': return LOOK_CMD_CONFIRM;
    }
    return 0;
}
uint8_t UserInput::checkGoto(int in)
{
    switch(in)
    {
        case 033: return GOTO_CMD_ESCAPE;
        case 'g': return GOTO_CMD_CONFIRM;
        case 'r': return GOTO_CMD_RANDOM;
    }
    return 0;
}
uint8_t UserInput::checkEquipSlot(int in)
{
    if(in >= 'a' && in <= 'l') return static_cast<uint8_t>(in - 'a') + 1;
    return 0;
}
uint8_t UserInput::checkCarrySlot(int in)
{
    if(in >= '0' && in <= '9') return static_cast<uint8_t>(in - '0') + 1;
    return 0;
}

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
            cmd.mlist_cmd = MLIST_CMD_ESCAPE;    // escape
            break;
        }
    // --- DEBUG COMMANDS -----------------
        case 'f':
        {
            cmd.dbg_cmd = DBG_CMD_TOGGLE_FOG;
            break;
        }
        case 's':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_DUNGEON;
            break;
        }
        case 'H':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_HARDNESS;
            break;
        }
        case 'D':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_FWEIGHTS;
            break;
        }
        case 'T':
        {
            cmd.dbg_cmd = DBG_CMD_SHOW_TWEIGHTS;
            break;
        }
        default: break;
    }

    return cmd;
}


int GameState::overwrite_changes()
{
    if(this->state.active_win != this->state.displayed_win)
    {
        switch(this->state.active_win)
        {
            case GWIN_MAP:      this->map_win.overwrite();   break;
            case GWIN_MLIST:    this->mlist_win.overwrite(); break;
            case GWIN_NONE:
            default: return 0;
        }
        this->state.displayed_win = this->state.active_win;
    }

    return 0;
}

int GameState::iterate_next_pc()
{
    // Heap* q = &this->level.entity_q;
    Entity* e;
    int s;
    do
    {
        // e = static_cast<Entity*>(heap_remove_min(q));
        DungeonLevel::EntityQueueNode qn = this->level.entity_queue.top();
        this->level.entity_queue.pop();
        e = qn.e;
        // FileDebug::get() << "Popped node with entity : " << std::hex << e << std::dec
        //     << ", next turn : " << qn.next_turn
        //     << ", priority : " << (int)qn.priority << std::endl;
        if(e->state.health > 0)
        {
            qn.next_turn += (1000 / e->config.speed);
            this->level.entity_queue.push(qn);

            if(!e->config.is_pc)
            {
                Vec2u8 pre = e->state.pos;
                if(this->level.iterateNPC(*e))
                {
                    // FileDebug::get() << "\tEntity has been iterated.\n";

                    // this->map_win.onMonsterMove(pre, e->state.pos);

                    // FileDebug::get() << "Entity 0x"
                    //     << std::hex << e << std::dec
                    //     << " moved from (" << pre.x << ", " << pre.y
                    //     << ") to (" << e->state.pos.x << ", " << e->state.pos.y << ")\n";
                }
                // else
                // {
                //     FileDebug::get() << "\tNo entity movement occurred.\n";
                // }
            }
        }
        else if(e->config.unique_entry)
        {
            this->unique_availability[e->config.unique_entry] = false;
        }
    }
    while(!(s = this->level.getWinLose()) && (!e || !e->config.is_pc));

    this->map_win.onRefresh(true);

    return s;
}

int GameState::iterate_pc_cmd(int move_cmd, bool& was_nop)
{
    was_nop = true;

    static const int8_t off[] = {
        0, -1,
        0, +1,
       -1,  0,
       +1,  0,
       -1, -1,
       +1, -1,
       -1, +1,
       +1, +1
   };

    Entity& pc = this->level.pc;
    const DungeonLevel::TerrainMap::Cell t = this->level.map.terrain[pc.state.pos.y][pc.state.pos.x];
    switch(move_cmd)
    {
        case MOVE_CMD_U:
        case MOVE_CMD_D:
        case MOVE_CMD_L:
        case MOVE_CMD_R:
        case MOVE_CMD_UL:
        case MOVE_CMD_UR:
        case MOVE_CMD_DL:
        case MOVE_CMD_DR:
        {
            Vec2u8 from;
            const Vec2i8 d{
                off[(move_cmd - MOVE_CMD_U) * 2 + 0],
                off[(move_cmd - MOVE_CMD_U) * 2 + 1] };

            if(this->state.is_goto_ctrl)
            {
                from = pc.state.target_pos;
                pc.state.target_pos += d;

                pc.state.target_pos.clamp(Vec2u8{1, 1}, Vec2u8{DUNGEON_X_DIM - 2, DUNGEON_Y_DIM - 2});

                this->map_win.onGotoMove(from, pc.state.target_pos);
            }
            else
            {
                from = pc.state.pos;
                if( this->level.handlePCMove(static_cast<Vec2i8>(pc.state.pos) + d, false) )
                {
                    this->map_win.onPlayerMove(from, pc.state.pos);
                    was_nop = false;
                }
            }

            break;
        }
        case MOVE_CMD_US:
        case MOVE_CMD_DS:
        {
            if(!this->state.is_goto_ctrl && (move_cmd - 9) == (int)t.is_stair)
            {
                Vec2u8 pc_pos;

                this->level.reset();
                this->initDungeonRandom();  // TODO: handle unique item resets

                this->map_win.changeLevel(this->level);
                this->mlist_win.changeLevel(this->level);
            }
            break;
        }
        case MOVE_CMD_GOTO:
        {
            if(this->state.is_goto_ctrl)
            {
                Vec2u8 from = pc.state.pos;
                this->level.handlePCMove(pc.state.target_pos, true);
                this->map_win.onPlayerMove(from, pc.state.pos);
                this->state.is_goto_ctrl = false;
            }
            else
            {
                pc.state.target_pos = pc.state.pos;
                this->map_win.onGotoMove(pc.state.pos, pc.state.target_pos);
                this->state.is_goto_ctrl = true;
            }
            break;
        }
        case MOVE_CMD_RGOTO:
        {
            if(this->state.is_goto_ctrl)
            {
                Vec2u8 from = pc.state.pos;
                if( this->level.handlePCMove(this->level.map.randomRoomFloorPos(this->state.rgen), true) )
                {
                    this->map_win.onPlayerMove(from, pc.state.pos);
                }
                this->state.is_goto_ctrl = false;
            }
            break;
        }
        case MOVE_CMD_SKIP: was_nop = false;
        default: break;
    }

    this->map_win.onRefresh(!this->state.is_goto_ctrl);

    return this->level.getWinLose();
}

int GameState::handle_mlist_cmd(int mlist_cmd)
{
    const int is_currently_mlist = (this->state.active_win == GWIN_MLIST);
    switch(mlist_cmd)
    {
        case MLIST_CMD_SHOW:
        {
            this->state.active_win = GWIN_MLIST;
            this->mlist_win.onShow();
            NC_PRINT("%lu monster(s) remain.", this->level.npcs_remaining);
            break;
        }
        case MLIST_CMD_ESCAPE:
        {
            if(is_currently_mlist)
            {
                this->state.active_win = GWIN_MAP;
                NC_PRINT(" ");
            }
            break;
        }
        case MLIST_CMD_SU:
        {
            this->mlist_win.onScrollUp();
            break;
        }
        case MLIST_CMD_SD:
        {
            this->mlist_win.onScrollDown();
            break;
        }
        default: break;
    }

    return 0;
}

int GameState::handle_dbg_cmd(int dbg_cmd)
{
    if(this->state.active_win == GWIN_MAP)
    {
        switch(dbg_cmd)
        {
            case DBG_CMD_TOGGLE_FOG:
            case DBG_CMD_SHOW_DUNGEON:
            case DBG_CMD_SHOW_FWEIGHTS:
            case DBG_CMD_SHOW_HARDNESS:
            case DBG_CMD_SHOW_TWEIGHTS:
            {
                this->map_win.changeMap(MapWindow::MAP_FOG + (dbg_cmd - DBG_CMD_TOGGLE_FOG));
            }
            default: break;
        }
    }

    return 0;
}





// GAMESTATE PUBLIC INTERFACE -----------------------------------------------------------------------------------

void GameState::initRuntimeArgs(uint32_t seed, int nmon)
{
    this->state.seed = seed;
    this->state.nmon = nmon;

    this->state.rgen.seed(seed);
}

bool GameState::initMonDescriptions(std::istream& i)
{
    return MonDescription::parse(i, this->mon_desc);
}

bool GameState::initItemDescriptions(std::istream& i)
{
    return ItemDescription::parse(i, this->item_desc);
}

bool GameState::initDungeonFile(FILE* f)
{
    this->level.setSeed(this->nextSeed());
    int r = this->level.loadTerrain(f);
    this->initializeEntities();

    return static_cast<bool>(r);
}

bool GameState::initDungeonRandom()
{
    this->level.setSeed(this->nextSeed());
    int r = this->level.generateTerrain();
    this->initializeEntities();

    return static_cast<bool>(r);
}

bool GameState::exportDungeonFile(FILE* f)
{
    return !this->level.saveTerrain(f);
}

bool GameState::initializeEntities()
{
    #define PC_POS this->level.pc.state.pos
    #define TERRAIN_MAP this->level.map
    #define ENTITY_MAP this->level.entity_map
    #define ITEM_MAP this->level.item_map

     this->level.pc.print(FileDebug::get());
     FileDebug::get() << "\n\n";

// 1. generate monsters ---------------------------------------------------------------------
    if(this->state.nmon < 0)
    {
        this->level.npcs_remaining = random_int(DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS, this->state.rgen);
    }
    else
    {
        this->state.rgen.discard(1);
        this->level.npcs_remaining = static_cast<size_t>(this->state.nmon);
    }

    std::uniform_int_distribution<size_t>
        mon_desc_idx_distribution{ 0, this->mon_desc.size() - 1 };
    std::uniform_int_distribution<uint8_t>
        rarity_required_distribution{ 0, 99 };

    for(size_t i = 0; i < this->level.npcs_remaining;)
    {
        MonDescription& mdesc = this->mon_desc[mon_desc_idx_distribution(this->state.rgen)];

        auto search = this->unique_availability.find(&mdesc);
        if(search != this->unique_availability.end() && !search->second) continue;

        const uint8_t rr = rarity_required_distribution(this->state.rgen);
        if(MonDescription::Rarity(mdesc) <= rr) continue;

        this->level.npcs.emplace_back(std::cref(mdesc), std::ref(this->state.rgen));

        if(this->level.npcs.back().config.is_unique)
        {
            this->unique_availability[&mdesc] = false;
        }

        this->level.npcs.back().print(FileDebug::get());
        FileDebug::get() << "\n\n";

        i++;
    }

// 2. assign entity floor positions -------------------------------------------------------
    if(PC_POS == Vec2u8{ 0, 0 })
    {
        this->level.pc.state.pos = TERRAIN_MAP.randomRoomFloorPos(this->state.rgen);
    }
    else
    {
        this->state.rgen.discard(2);
    }
    DungeonLevel::accessGridElem(ENTITY_MAP, PC_POS) = &this->level.pc;

    std::uniform_int_distribution<uint32_t>
        spawn_off_distribution{ 30, 150 };

    uint8_t x = PC_POS.x, y = PC_POS.y;
    for(size_t m = 0; m < this->level.npcs_remaining; m++)
    {
        size_t attempts = 0;
        for( uint32_t trav = spawn_off_distribution(this->state.rgen);
            trav > 0 && attempts < DUNGEON_TOTAL_CELLS;
            attempts++ )
        {
            // TODO: skip checking border cells
            x++;
            y += (x / DUNGEON_X_DIM);
            x %= DUNGEON_X_DIM;
            y %= DUNGEON_Y_DIM;
            trav -= (TERRAIN_MAP.terrain[y][x].type && !ENTITY_MAP[y][x]);
        }

        this->level.npcs[m].state.pos.assign(x, y);
        ENTITY_MAP[y][x] = &this->level.npcs[m];

        // PRINT_DEBUG( "Initialized monster {%d, %d, (%d, %d), %#x}\n",
        //     me->speed, me->priority, x, y, me->md.stats );
    }

// 3. generate items
    size_t num_items = random_int(DUNGEON_MIN_NUM_ITEMS, DUNGEON_MAX_NUM_ITEMS, this->state.rgen);
    std::uniform_int_distribution<size_t>
        item_desc_idx_distribution{ 0, this->item_desc.size() - 1 };

    for(size_t i = 0; i < num_items;)
    {
        ItemDescription& idesc = this->item_desc[item_desc_idx_distribution(this->state.rgen)];

        auto search = this->artifact_availability.find(&idesc);
        if(search != this->artifact_availability.end() && !search->second) continue;

        const uint8_t rr = rarity_required_distribution(this->state.rgen);
        if(ItemDescription::Rarity(idesc) <= rr) continue;

        this->level.items.emplace_back(idesc, this->state.rgen);

        if(ItemDescription::Artifact(idesc))
        {
            this->artifact_availability[&idesc] = false;
        }

        this->level.items.back().print(FileDebug::get());
        FileDebug::get() << "\n\n";

        i++;
    }

// 4. assign item floor positions
    for(size_t i = 0; i < num_items; i++)
    {
        size_t attempts = 0;
        for( uint32_t trav = spawn_off_distribution(this->state.rgen);
            trav > 0 && attempts < DUNGEON_TOTAL_CELLS;
            attempts++ )
        {
            // TODO: skip checking border cells
            x++;
            y += (x / DUNGEON_X_DIM);
            x %= DUNGEON_X_DIM;
            y %= DUNGEON_Y_DIM;
            trav -= (TERRAIN_MAP.terrain[y][x].type && !ITEM_MAP[y][x]);
        }

        ITEM_MAP[y][x] = &this->level.items[i];

        // PRINT_DEBUG( "Initialized monster {%d, %d, (%d, %d), %#x}\n",
        //     me->speed, me->priority, x, y, me->md.stats );
    }

// 5. add entities to priority queue
    this->level.entity_queue.emplace( &this->level.pc, 0, 0 );
    for(size_t i = 0; i < this->level.npcs.size(); i++)
    {
        this->level.entity_queue.emplace( &this->level.npcs[i], 0, static_cast<uint8_t>(i + 1) );
    }

// 6. update traversal costmaps
    this->level.updateCosts(true);

// 7. update visibility maps
    this->level.copyVisCells();

    return true;
}

void GameState::run(const std::atomic<bool>& r)
{
    int status = 0;
    // InputCommand ic = zeroed_input();
    bool pc_nop = false;

    this->state.active_win = GWIN_MAP;
    this->map_win.onRefresh(true);

    NC_PRINT("Welcome to the dungeon. Good luck! :)");

    while(!status && r) // not won/lost, not exit
    {
        const int is_currently_map = (this->state.active_win == GWIN_MAP);

    // 1. update window if previously changed
        this->overwrite_changes();
    // 2. iterate monsters if necessary, break on
        if( !pc_nop &&
            is_currently_map &&
            (status = this->iterate_next_pc()) ) break;  // game done when iterate_next_pc() returns non-zero

        NC_PRINT2("HEALTH: %d", this->level.pc.state.health)

    // 3. accept user input
        int c = getch();
        uint8_t d = 0;
        pc_nop = false;

    // 4. process input
        if(UserInput::checkExit(c))
        {
            raise(SIGINT);
        }
        else
        if((d = UserInput::checkMove(c)) && is_currently_map)
        {
            status = this->iterate_pc_cmd(d, pc_nop);
        }
        else
        if((d = UserInput::checkMlist(c)))
        {
            this->handle_mlist_cmd(d);
            pc_nop = true;
        }
        else
        if((d = UserInput::checkDbg(c)))
        {
            this->handle_dbg_cmd(d);
            pc_nop = true;
        }
        else
        {
            NC_PRINT("Unknown key: %#o", c);
            pc_nop = true;
        }
    }

    if(status)
    {
        nc_print_win_lose(status, r);
        if(r) getch();
    }
}
