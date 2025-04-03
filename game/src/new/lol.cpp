#include "lol.hpp"



void MListWindow::onShow()
{
    this->prox_gradient.applyForeground(COLOR_BLACK);

    werase(this->win);
    this->scroll_amount = 0;

    size_t line = 0;
    size_t mon = 0;
    Entity* m = this->level->entity_alloc + 1;
    for(;mon < this->level->num_monsters && line < DUNGEON_Y_DIM - 2; m++)
    {
        if(m->hn)
        {
            this->printEntry(m, line);

            mon++;
            line++;
        }
    }

    this->refresh();

    // NC_PRINT("%d monster(s) remain.", this->level->num_monsters);
}

void MListWindow::onScrollUp()
{
    if((int)this->level->num_monsters - this->scroll_amount > (DUNGEON_Y_DIM - 2))
    {
        this->scroll_amount += 1;
        wscrl(this->win, 1);

        const int target_mnum = DUNGEON_Y_DIM - 3 + this->scroll_amount;
        Entity* m = this->level->entity_alloc + 1;
        for(int mon = 0; !m->hn || mon < target_mnum; m++)
        {
            if(m->hn) mon++;
        }

        this->printEntry(m, DUNGEON_Y_DIM - 3);
        this->refresh();
    }
}

void MListWindow::onScrollDown()
{
    if(this->scroll_amount > 0)
    {
        this->scroll_amount -= 1;
        wscrl(this->win, -1);

        const int target_mnum = this->scroll_amount;
        Entity* m = this->level->entity_alloc + 1;
        for(int mon = 0; !m->hn || mon < target_mnum; m++)
        {
            if(m->hn) mon++;
        }

        this->printEntry(m, 0);
        this->refresh();
    }
}

void MListWindow::changeLevel(DungeonLevel& l)
{
    this->level = &l;
}

void MListWindow::printEntry(Entity* m, int line)
{
    const int
        dx = (int)this->level->pc->pos.x - (int)m->pos.x,
        dy = (int)this->level->pc->pos.y - (int)m->pos.y;
        // ds = m->md.tunneling ? this->level->terrain_costs[m->pos.y][m->pos.x] : this->level->tunnel_costs[m->pos.y][m->pos.x];

    wmove(this->win, line, 0);
    wclrtoeol(this->win);

    // if(ds < 5)
    // {
    //     wattron(this->win, COLOR_PAIR(COLOR_RED));
    // }

    mvwprintw( this->win, line, 0, "[0x%c] : %d %s, %d %s",
        ("0123456789ABCDEF")[m->md.stats],
        abs(dx),
        dx > 0 ? "West" : "East",
        abs(dy),
        dy > 0 ? "North" : "South" );

    // if(ds < 5)
    // {
    //     wattroff(this->win, COLOR_PAIR(COLOR_RED));
    // }
}





void MapWindow::onPlayerMove(Vec2u8 a, Vec2u8 b)
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
        {
            mvwaddch(this->win, a.y, a.x, this->level->fog_map[a.y][a.x]);
            mvwaddch(this->win, b.y, b.x, '*');
            break;
        }
        case MAP_DUNGEON :
        case MAP_HARDNESS :
        {
            mvwaddch(
                this->win,
                a.y,
                a.x,
                get_cell_char(
                    this->level->map.terrain[a.y][a.x],
                    this->level->entities[b.y][b.x] ) );
            mvwaddch(this->win, b.y, b.x, '*');
            break;
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

    this->state.needs_rewrite = false;

    this->refresh();
}

void MapWindow::changeMap(int mmode)
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
    if(this->level->pc)
    {
        for(size_t i = 0; i < 21; i++)
        {
            const auto v = VIS_OFFSETS[i];
            const int8_t y = static_cast<int8_t>(this->level->pc->pos.y) + v[0];
            const int8_t x = static_cast<int8_t>(this->level->pc->pos.x) + v[1];

            if( (y >= 0 && y < DUNGEON_Y_DIM && x >= 0 && x < DUNGEON_X_DIM) &&
                (this->level->entities[y][x]) )
            {
                mvwaddch(this->win, y, x, get_entity_char(this->level->entities[y][x]));
            }
        }
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















/* --- BEHAVIOR.C INTERFACE ------------------------------------------- */

/* Iterate the provided monster's AI and update the game state. */
int iterate_monster(DungeonLevel* d, Entity* e);
/* Move an entity. */
int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t x, uint8_t y, bool is_goto);

/* -------------------------------------------------------------------- */


static int nc_print_win_lose(LevelStatus s, volatile int* r)
{
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

    if(s.has_lost)
    {
        mvaddstr(0, 0, lose);

        uint8_t p = 0;
        for(; p <= 100 && *r; p = MIN_CACHED(p + RANDOM_IN_RANGE(MIN_PERCENT_CHUNK, MAX_PERCENT_CHUNK), 100))
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
        if(*r)
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
    DBG_CMD_TOGGLE_FOG,
    DBG_CMD_SHOW_DUNGEON,
    DBG_CMD_SHOW_HARDNESS,
    DBG_CMD_SHOW_FWEIGHTS,
    DBG_CMD_SHOW_TWEIGHTS,
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


int Game::overwrite_changes()
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

int Game::iterate_next_pc(LevelStatus& s)
{
    Heap* q = &this->level.entity_q;
    Entity* e;

    s.data = 0;
    do
    {
        e = static_cast<Entity*>(heap_remove_min(q));
        if(e->hn)   // null heap node means the entity has perished
        {
            e->next_turn += e->speed;
            e->hn = heap_insert(q, e);

            if(!e->is_pc)
            {
                Vec2u8 pre;
                vec2u8_copy(&pre, &e->pos);
                if(iterate_monster(&this->level, e))
                {
                    this->map_win.onMonsterMove(pre, e->pos);
                }
            }
        }
    }
    while(!(s = get_dungeon_level_status(&this->level)).data && !e->is_pc);

    this->map_win.onRefresh(true);

    return s.data;
}

int Game::iterate_pc_cmd(int move_cmd, LevelStatus& s)
{
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

    Entity* pc = this->level.pc;
    const CellTerrain t = this->level.map.terrain[pc->pos.y][pc->pos.x];
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
            const int8_t
                dx = off[(move_cmd - MOVE_CMD_U) * 2 + 0],
                dy = off[(move_cmd - MOVE_CMD_U) * 2 + 1];

            if(this->state.is_goto_ctrl)
            {
                vec2u8_copy(&from, &pc->md.pc_rem_pos);
                pc->md.pc_rem_pos.x = static_cast<uint8_t>(static_cast<int8_t>(pc->md.pc_rem_pos.x) + dx),
                pc->md.pc_rem_pos.y = static_cast<uint8_t>(static_cast<int8_t>(pc->md.pc_rem_pos.y) + dy);

                this->map_win.onGotoMove(from, pc->md.pc_rem_pos);
            }
            else
            {
                vec2u8_copy(&from, &pc->pos);
                if( handle_entity_move(
                        &this->level,
                        pc,
                        static_cast<uint8_t>(static_cast<int8_t>(pc->pos.x) + dx),
                        static_cast<uint8_t>(static_cast<int8_t>(pc->pos.y) + dy), false ) )
                {
                    this->map_win.onPlayerMove(from, pc->pos);
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

                destruct_dungeon_level(&this->level);
                zero_dungeon_level(&this->level);
                generate_dungeon_map(&this->level.map, 0);
                random_dungeon_map_floor_pos(&this->level.map, pc_pos.data);
                init_dungeon_level(&this->level, pc_pos, RANDOM_IN_RANGE(DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS));

                this->map_win.changeLevel(this->level);
                this->mlist_win.changeLevel(this->level);
            }
            break;
        }
        case MOVE_CMD_GOTO:
        {
            if(this->state.is_goto_ctrl)
            {
                Vec2u8 from;
                vec2u8_copy(&from, &pc->pos);
                if( handle_entity_move( &this->level, pc, pc->md.pc_rem_pos.x, pc->md.pc_rem_pos.y, true ) )
                {
                    this->map_win.onPlayerMove(from, pc->pos);
                }
                this->state.is_goto_ctrl = false;
            }
            else
            {
                vec2u8_copy(&pc->md.pc_rem_pos, &pc->pos);
                this->map_win.onGotoMove(pc->pos, pc->md.pc_rem_pos);
                this->state.is_goto_ctrl = true;
            }
            break;
        }
        case MOVE_CMD_RGOTO:
        {
            if(this->state.is_goto_ctrl)
            {
                Vec2u8 r;
                random_dungeon_map_floor_pos(&this->level.map, r.data);
                Vec2u8 from;
                vec2u8_copy(&from, &pc->pos);
                if( handle_entity_move( &this->level, pc, r.x, r.y, true ) )
                {
                    this->map_win.onPlayerMove(from, pc->pos);
                }
                this->state.is_goto_ctrl = false;
            }
            break;
        }
        case MOVE_CMD_SKIP:
        default: break;
    }

    this->map_win.onRefresh(!this->state.is_goto_ctrl);

    return (s = get_dungeon_level_status(&this->level)).data;
}

int Game::handle_mlist_cmd(int mlist_cmd)
{
    const int is_currently_mlist = (this->state.active_win == GWIN_MLIST);
    switch(mlist_cmd)
    {
        case MLIST_CMD_SHOW:
        {
            this->state.active_win = GWIN_MLIST;
            this->mlist_win.onShow();
            NC_PRINT("%d monster(s) remain.", this->level.num_monsters);
            break;
        }
        case MLIST_CMD_HIDE:
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
        }
        case MLIST_CMD_SD:
        {
            this->mlist_win.onScrollDown();
        }
        default: break;
    }

    return 0;
}

int Game::handle_dbg_cmd(int dbg_cmd)
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


int Game::run(volatile int* r)
{
    LevelStatus status;
    InputCommand ic = zeroed_input();
    int c;

    this->state.active_win = GWIN_MAP;
    this->map_win.onRefresh(true);

    NC_PRINT("Welcome to the dungeon. Good luck! :)");

    for(ic.move_cmd = MOVE_CMD_SKIP, status.data = 0; !status.data && *r;)
    {
        const int is_currently_map = (this->state.active_win == GWIN_MAP);

        this->overwrite_changes();
        if(is_currently_map && ic.move_cmd && !this->state.is_goto_ctrl && this->iterate_next_pc(status)) break;  // game done when iterate_next_pc() returns non-zero

        ic = decode_input_command((c = getch()));

        if(ic.exit)
        {
            raise(SIGINT);
        }
        else if(ic.move_cmd && is_currently_map)
        {
            this->iterate_pc_cmd(ic.move_cmd, status);
        }
        else if(ic.mlist_cmd)
        {
            this->handle_mlist_cmd(ic.mlist_cmd);
        }
        else if(ic.dbg_cmd)
        {
            this->handle_dbg_cmd(ic.dbg_cmd);
        }
        else
        {
            NC_PRINT("Unknown key: %#o", c);
        }
    }

    if(status.data)
    {
        nc_print_win_lose(status, r);
        if(*r) getch();
    }

    return 0;
}
