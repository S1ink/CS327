#include "util/debug.h"
#include "util/math.h"

#include "dungeon_config.h"
#include "dungeon.h"
#include "entity.h"
#include "game.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>

#include <ncurses.h>    // https://invisible-island.net/ncurses/man/ncurses.3x.html

/* --- BEHAVIOR.C INTERFACE ------------------------------------------- */

/* Iterate the provided monster's AI and update the game state. */
int iterate_monster(Game* d, Entity* e);
/* Move an entity. */
int handle_entity_move(Game* g, Entity* e, uint8_t x, uint8_t y);

/* -------------------------------------------------------------------- */


#define DUNGEON_WIN_OFFSET_X 0
#define DUNGEON_WIN_OFFSET_Y 1

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

typedef struct
{
    uint8_t move_cmd : REQUIRED_BITS32(NUM_MOVE_CMD - 1);
    uint8_t mlist_cmd : REQUIRED_BITS32(NUM_MLIST_CMD - 1);
    uint8_t exit : 1;
}
InputCommand;

static inline InputCommand get_zeroed_ic()
{
    InputCommand ic;
    ic.move_cmd = 0;
    ic.mlist_cmd = 0;
    ic.exit = 0;
    return ic;
}

static inline InputCommand decode_input_command(int in)
{
    InputCommand cmd;
    cmd.move_cmd = MOVE_CMD_NONE;
    cmd.mlist_cmd = MLIST_CMD_NONE;
    cmd.exit = 0;

    switch(in)
    {
        case 'Q':
        case 03:    // Ctrl+C
        {
            cmd.exit = 1;
            break;
        }
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
        default:
        {
            // char pb[32];
            // snprintf(pb, 80, "Read unknown input %#o   ", in);
            // mvaddstr(0, 0, pb);
        }
    }

    return cmd;
}


int nc_print_win_lose(LevelStatus s, volatile int* r)
{
    char lose[] =
        "+------------------------------------------------------------------------------+\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                .@       You encountered a problem and need to restart.       |\n"
        "|               @@                                                             |\n"
        "|      @@      @@         We're just collecting some error info, and then      |\n"
        "|             .@+                we'll restart for you (no we won't lol).      |\n"
        "|             @@                                                               |\n"
        "|             @@                                                               |\n"
        "|             @@                                                               |\n"
        "|             *@+                                                              |\n"
        "|      @@      @@                                                              |\n"
        "|               @@                                                             |\n"
        "|                *@                                                            |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|     [                                                    ]    % complete     |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "+------------------------------------------------------------------------------+\n";

    char win[] =
        "+------------------------------------------------------------------------------+\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|             @.           Congratulations. You are victorious.                |\n"
        "|              @@                                                              |\n"
        "|      @@       @@                                                             |\n"
        "|               +@.                                                            |\n"
        "|                @@                                                            |\n"
        "|                @@                                                            |\n"
        "|                @@                                                            |\n"
        "|               +@*                                                            |\n"
        "|      @@       @@                                                             |\n"
        "|              @@                                                              |\n"
        "|             @*                                                               |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "|                                                                              |\n"
        "+------------------------------------------------------------------------------+\n";

    #define MIN_PERCENT_CHUNK       3
    #define MAX_PERCENT_CHUNK       41
    #define LOADING_BAR_START_IDX   1627
    #define LOADING_BAR_LEN         51
    #define PERCENT_START_IDX       1681
    #define MIN_PAUSE_MS            200
    #define MAX_PAUSE_MS            700

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

        mvaddstr(14, 38, "Press any key to continue.");
    }
    else
    {
        mvaddstr(0, 0, win);
    }

    return 0;

    #undef MIN_PERCENT_CHUNK
    #undef MAX_PERCENT_CHUNK
    #undef LOADING_BAR_START_IDX
    #undef LOADING_BAR_LEN
    #undef PERCENT_START_IDX
    #undef MIN_PAUSE_MS
    #undef MAX_PAUSE_MS
}



static inline int nc_init()
{
    initscr();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    // color

    refresh();

    return 0;
}
static inline int nc_deinit()
{
    endwin();

    return 0;
}

static inline int nc_print_border(WINDOW* w)
{
    return wborder(w, '|', '|', '-', '-', '+', '+', '+', '+');
}
static inline int nc_overwrite_window(WINDOW* w)
{
    touchwin(w);
    return wrefresh(w);
}



/* --- GRADIENT COLOR GENERATOR ------------------------- */

typedef NCURSES_COLOR_T NCColorT;
typedef NCColorT NCGradient[128][3];

void generate_color_gradient(
    NCGradient grad,
    NCColorT ra,
    NCColorT ga,
    NCColorT ba,
    NCColorT rb,
    NCColorT gb,
    NCColorT bb )
{
    // ra = MIN_CACHED(MAX(ra, 0), 1000);
    // ga = MIN_CACHED(MAX(ga, 0), 1000);
    // ba = MIN_CACHED(MAX(ba, 0), 1000);
    // rb = MIN_CACHED(MAX(rb, 0), 1000);
    // gb = MIN_CACHED(MAX(gb, 0), 1000);
    // bb = MIN_CACHED(MAX(bb, 0), 1000);

    const NCColorT rd = rb - ra;
    const NCColorT gd = gb - ga;
    const NCColorT bd = bb - ba;

    for(size_t i = 0; i < 128; i++)
    {
        grad[i][0] = ra + ((rd * i) / 127);
        grad[i][1] = ga + ((gd * i) / 127);
        grad[i][2] = ba + ((bd * i) / 127);
    }
}
void nc_apply_gradient(NCGradient grad)
{
    for(NCColorT i = 0; i < 128; i++)
    {
        init_color(128 + i, grad[i][0], grad[i][1], grad[i][2]);
    }
}
void nc_apply_gradient_pairs_fg(NCGradient grad, NCColorT bg)
{
    for(NCColorT i = 0; i < 128; i++)
    {
        init_color(128 + i, grad[i][0], grad[i][1], grad[i][2]);
        init_pair(128 + i, 128 + i, bg);
    }
}
void nc_apply_gradient_pairs_bg(NCGradient grad)
{
    for(NCColorT i = 0; i < 128; i++)
    {
        init_color(128 + i, grad[i][0], grad[i][1], grad[i][2]);
        init_pair(128 + i, (grad[i][0] + grad[i][1] + grad[i][2] > 1500 ? COLOR_BLACK : COLOR_WHITE), 128 + i);
    }
}
inline void nc_print_grad_char(int y, int x, NCURSES_PAIRS_T v, chtype c)
{
    attron(COLOR_PAIR(128 + v));
    mvaddch(y, x, c);
    attroff(COLOR_PAIR(128 + v));
}



static inline int write_dungeon_map(Game* g)
{
    char row[DUNGEON_X_DIM - 2];
    for(uint32_t y = 1; y < (DUNGEON_Y_DIM - 1); y++)
    {
        for(uint32_t x = 1; x < (DUNGEON_X_DIM - 1); x++)
        {
            row[x - 1] = get_cell_char(g->level.map.terrain[y][x], g->level.entities[y][x]);
        }
        mvwaddnstr(g->map_win, y, 1, row, DUNGEON_X_DIM - 2);
    }

    return 0;
}

static inline int display_active_window(Game* g)
{
    if(g->state.active_win != g->state.displayed_win)
    {
        switch(g->state.active_win)
        {
            case GWIN_MAP:      nc_overwrite_window(g->map_win);   break;
            case GWIN_MLIST:    nc_overwrite_window(g->mlist_win); break;
            case GWIN_NONE:
            default: return 0;
        }
        g->state.displayed_win = g->state.active_win;
    }

    return 0;
}

static inline int iterate_next_pc(Game* g, LevelStatus* s)
{
    Heap* q = &g->level.entity_q;
    Entity* e;

    s->data = 0;
    do
    {
        e = heap_remove_min(q);
        if(e->hn)   // null heap node means the entity has perished
        {
            e->next_turn += e->speed;
            e->hn = heap_insert(q, e);

            if(!e->is_pc)
            {
                iterate_monster(g, e);
            }
        }
    }
    while(!(*s = get_dungeon_level_status(&g->level)).data && !e->is_pc);

    wrefresh(g->map_win);

    return s->data;
}

static inline int iterate_pc(Game* g, int move_cmd, LevelStatus* s)
{
    Entity* pc = g->level.pc;
    const CellTerrain t = g->level.map.terrain[pc->pos.y][pc->pos.x];
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
            const uint8_t off[] = {
                 0, -1,
                 0, +1,
                -1,  0,
                +1,  0,
                -1, -1,
                +1, -1,
                -1, +1,
                +1, +1
            };
            const uint8_t
                x = pc->pos.x + off[(move_cmd - MOVE_CMD_U) * 2 + 0],
                y = pc->pos.y + off[(move_cmd - MOVE_CMD_U) * 2 + 1];

            handle_entity_move(g, pc, x, y);

            break;
        }
        case MOVE_CMD_US:
        case MOVE_CMD_DS:
        {
            if((move_cmd - 9) == (int)t.is_stair)
            {
                Vec2u8 pc_pos;

                destruct_dungeon_level(&g->level);
                zero_dungeon_level(&g->level);
                generate_dungeon_map(&g->level.map, 0);
                random_dungeon_map_floor_pos(&g->level.map, pc_pos.data);
                init_dungeon_level(&g->level, pc_pos, RANDOM_IN_RANGE(DUNGEON_MIN_NUM_MONSTERS, DUNGEON_MAX_NUM_MONSTERS));
                write_dungeon_map(g);
            }
            break;
        }
        case MOVE_CMD_SKIP:
        default: break;
    }

    wrefresh(g->map_win);

    return (*s = get_dungeon_level_status(&g->level)).data;
}

static int handle_mlist_cmd(Game* g, int mlist_cmd, int* scroll_amount)
{
    const int is_currently_mlist = (g->state.active_win == GWIN_MLIST);

    switch(mlist_cmd)
    {
        case MLIST_CMD_SHOW:
        {
            g->state.active_win = GWIN_MLIST;

            werase(g->mlist_win);
            *scroll_amount = 0;

            size_t line = 0;
            size_t mon = 0;
            Entity* m = g->level.entity_alloc + 1;
            for(;mon < g->level.num_monsters && line < DUNGEON_Y_DIM - 2; m++)
            {
                if(m->hn)
                {
                    const int
                        dx = (int)g->level.pc->pos.x - (int)m->pos.x,
                        dy = (int)g->level.pc->pos.y - (int)m->pos.y;

                    mvwprintw( g->mlist_win, line, 0, "[0x%c] : %d %s, %d %s",
                        ("0123456789ABCDEF")[m->md.stats],
                        abs(dx),
                        dx > 0 ? "West" : "East",
                        abs(dy),
                        dy > 0 ? "North" : "South");

                    mon++;
                    line++;
                }
            }
            // *window handler will refresh when it overwrites*
            break;
        }
        case MLIST_CMD_HIDE:
        {
            if(is_currently_mlist)
            {
                g->state.active_win = GWIN_MAP;
            }
            break;
        }
        case MLIST_CMD_SU:
        {
            if(is_currently_mlist && (int)g->level.num_monsters - *scroll_amount > (DUNGEON_Y_DIM - 2))
            {
                *scroll_amount += 1;
                wscrl(g->mlist_win, 1);
                // wclrtoeol(g->mlist_win);

                const int target_mnum = DUNGEON_Y_DIM - 3 + *scroll_amount;
                Entity* m = g->level.entity_alloc + 1;
                for(int mon = 0; !m->hn || mon < target_mnum; m++)
                {
                    if(m->hn) mon++;
                }

                const int
                    dx = (int)g->level.pc->pos.x - (int)m->pos.x,
                    dy = (int)g->level.pc->pos.y - (int)m->pos.y;

                mvwprintw( g->mlist_win, DUNGEON_Y_DIM - 3, 0, "[0x%c] : %d %s, %d %s           ",
                    ("0123456789ABCDEF")[m->md.stats],
                    abs(dx),
                    dx > 0 ? "West" : "East",
                    abs(dy),
                    dy > 0 ? "North" : "South");

                wrefresh(g->mlist_win);
            }
            break;
        }
        case MLIST_CMD_SD:
        {
            if(is_currently_mlist && *scroll_amount > 0)
            {
                *scroll_amount -= 1;
                wscrl(g->mlist_win, -1);
                // wclrtoeol(g->mlist_win);

                const int target_mnum = *scroll_amount;
                Entity* m = g->level.entity_alloc + 1;
                for(int mon = 0; !m->hn || mon < target_mnum; m++)
                {
                    if(m->hn) mon++;
                }

                const int
                    dx = (int)g->level.pc->pos.x - (int)m->pos.x,
                    dy = (int)g->level.pc->pos.y - (int)m->pos.y;

                mvwprintw( g->mlist_win, 0, 0, "[0x%c] : %d %s, %d %s             ",
                    ("0123456789ABCDEF")[m->md.stats],
                    abs(dx),
                    dx > 0 ? "West" : "East",
                    abs(dy),
                    dy > 0 ? "North" : "South");

                wrefresh(g->mlist_win);
            }
            break;
        }
        default: break;
    }

    return 0;
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

    g->map_win = newwin(DUNGEON_Y_DIM, DUNGEON_X_DIM, DUNGEON_WIN_OFFSET_Y, DUNGEON_WIN_OFFSET_X);
    g->mlist_win = newwin(DUNGEON_Y_DIM - 2, DUNGEON_X_DIM - 2, DUNGEON_WIN_OFFSET_Y + 1, DUNGEON_WIN_OFFSET_X + 1);
    scrollok(g->mlist_win, TRUE);
    idlok(g->mlist_win, TRUE);

    if(DUNGEON_PRINT_BORDER)
    {
        nc_print_border(g->map_win);
        // nc_print_border(g->mlist_win);
    }
    // wrefresh(g->map_win);
    // wrefresh(g->mlist_win);

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
    // NCGradient t_grad, t_grad2;
    // generate_color_gradient(t_grad, 500, 0, 500, 500, 1000, 500);
    // generate_color_gradient(t_grad2, 0, 0, 0, 500, 700, 1000);
    // nc_apply_gradient_pairs_fg(t_grad, COLOR_WHITE);

    // for(size_t i = 0; i < 128; i++)
    // {
    //     nc_print_grad_char(0, i, i, 'x');
    // }

    // getch();

    LevelStatus status;
    InputCommand ic = get_zeroed_ic();
    int c;
    int scroll_amount = 0;

    write_dungeon_map(g);
    g->state.active_win = GWIN_MAP;

    for(status.data = 0; !status.data && *r;)
    {
        const int is_currently_map = (g->state.active_win == GWIN_MAP);

        display_active_window(g);
        if(is_currently_map && ic.move_cmd && iterate_next_pc(g, &status)) break;  // game done when iterate_next_pc() returns non-zero

        ic = decode_input_command((c = getch()));

        if(ic.exit)
        {
            raise(SIGINT);
        }
        else if(ic.move_cmd && is_currently_map)
        {
            iterate_pc(g, ic.move_cmd, &status);
        }
        else if(ic.mlist_cmd)
        {
            handle_mlist_cmd(g, ic.mlist_cmd, &scroll_amount);
        }
        else
        {
            mvprintw(0, 0, "Unknown command : %#o                  ", c);
            refresh();
        }
    }

    if(status.data)
    {
        nc_print_win_lose(status, r);
        if(*r) getch();
    }

    return 0;
}
