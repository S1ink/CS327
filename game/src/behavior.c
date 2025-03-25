#include "dungeon.h"
#include "pathing.h"

#include "util/debug.h"
#include "util/math.h"
#include "util/heap.h"

#ifndef BRESENHAM_DEBUG
#define BRESENHAM_DEBUG 0
#endif


/* --- DUNGEON.C INTERNAL INTERFACE --------------------------------------- */

int dungeon_level_update_costs(DungeonLevel* d, int both_or_only_terrain);

/* No header since these are only used in dungeon.c -- declarations are internal */

static inline int entity_has_tunneling(Entity* e)
{
    return !e->is_pc && (e->md.tunneling);
}
static inline int terrain_is_rock(CellTerrain t)
{
    return !t.type;
}

static const int8_t OFF_DIRECTIONS[8][2] =
{
    { +1,  0 },
    {  0, -1 },
    { -1,  0 },
    {  0, +1 },
    { +1, +1 },
    { +1, -1 },
    { -1, -1 },
    { -1, +1 },
};

// returns 1 if the entity successfully moved, 0 otherwise
int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t x, uint8_t y)
{
    Entity** prev_slot = d->entities[e->pos.y] + e->pos.x;
    struct
    {
        uint8_t terrain_updated : 1;
        uint8_t floor_updated : 1;
        uint8_t has_entity_moved : 1;
        uint8_t pc_moved : 1;
    }
    flags;

    *(uint8_t*)(&flags) = 0;

    if(terrain_is_rock(d->map.terrain[y][x]))
    {
        if(entity_has_tunneling(e))
        {
            uint8_t* h =  d->map.hardness[y] + x;
            *h = (*h > 85 ? *h - 85 : 0);
            if(!*h)
            {
                d->map.terrain[y][x].type = CELLTYPE_CORRIDOR;
                flags.has_entity_moved = 1;
                flags.floor_updated = 1;
            }
            flags.terrain_updated = 1;
        }
        // else... entity isnt allowed to move
    }
    else
    {
        flags.has_entity_moved = 1;
    }

    if(flags.has_entity_moved)
    {
        e->pos.x = x;
        e->pos.y = y;

        Entity** slot = d->entities[y] + x;
        if(*slot)   // previous entity
        {
            (*slot)->hn = NULL; // this marks that the entity will no longer be iterated -- TODO: immediate heap removal!
            if(!(*slot)->is_pc)
            {
                d->num_monsters -= 1;
            }
            else d->pc = NULL;
        }
        *slot = e;
        *prev_slot = NULL;
    }

    flags.pc_moved = (flags.has_entity_moved && e->is_pc);
    if(flags.terrain_updated || flags.pc_moved)
    {
        PRINT_DEBUG("UPDATING TERRAIN %sCOSTS\n", flags.floor_updated ? "(and floor) " : "");
        dungeon_level_update_costs(d, flags.floor_updated || flags.pc_moved);
    }

    return flags.has_entity_moved;
}
// returns the result of handle_entity_move()
static int handle_entity_move_dir(DungeonLevel* d, Entity* e, uint8_t dir_idx)
{
    return handle_entity_move( d, e, (e->pos.x + OFF_DIRECTIONS[dir_idx][0]), (e->pos.y + OFF_DIRECTIONS[dir_idx][1]) );
}

// returns the result of handle_entity_move_dir() a valid direction was detected, otherwise 0
static int move_random(DungeonLevel* d, Entity* e, int r)
{
    const int has_tunneling = entity_has_tunneling(e);

    uint8_t valid_dirs[8];
    uint8_t n_valid_dirs = 0;
    uint8_t x, y;
    for(size_t i = 0; i < 8; i++)
    {
        x = e->pos.x + OFF_DIRECTIONS[i][0];
        y = e->pos.y + OFF_DIRECTIONS[i][1];
        if(d->map.terrain[y][x].type || (has_tunneling && d->map.hardness[y][x] < 0xFF))
        {
            valid_dirs[n_valid_dirs] = i;
            n_valid_dirs++;
        }
    }

    return n_valid_dirs ? handle_entity_move_dir(d, e, valid_dirs[(r ? r : rand()) % n_valid_dirs]) : 0;
}

static int bresenham_check_los(DungeonLevel* d, Entity* e, Vec2u8* trav_cell)
{
    #define ENTITY e->pos
    #define PLAYER d->pc->pos

#if BRESENHAM_DEBUG
    PRINT_DEBUG("BRESENHAM: calculating from (%d, %d) to (%d, %d)\n", ENTITY.x, ENTITY.y, PLAYER.x, PLAYER.y);
#endif

    const int32_t dx = abs((int32_t)PLAYER.x - (int32_t)ENTITY.x);
    const int32_t dy = -abs((int32_t)PLAYER.y - (int32_t)ENTITY.y);
    const int32_t sx = ENTITY.x < PLAYER.x ? 1 : -1;
    const int32_t sy = ENTITY.y < PLAYER.y ? 1 : -1;
    int32_t err = dx + dy;

    int32_t x = ENTITY.x;
    int32_t y = ENTITY.y;
    for(int i = 0; !(x == PLAYER.x && y == PLAYER.y); i++)
    {
        const int32_t err2 = err * 2;
        if(err2 >= dy)
        {
            err = err + dy;
            x += sx;
        }
        if(err2 <= dx)
        {
            err = err + dx;
            y += sy;
        }

    #if BRESENHAM_DEBUG
        PRINT_DEBUG("BRESENHAM ITERATION %d : (%d, %d) -- terrain: %d\n", i, x, y, d->map.terrain[y][x].type);
    #endif

        if(!i)  // set on first iteration
        {
            trav_cell->x = (uint8_t)x;
            trav_cell->y = (uint8_t)y;
        }
        if(terrain_is_rock(d->map.terrain[y][x]))  // exit on rock intersection
        {
            return i + 1;
        }
    }

    return 0;

    #undef ENTITY
    #undef PLAYER
}

static void pathing_export_vec2u8(void* v, uint32_t x, uint32_t y)
{
    vec2u8_assign((Vec2u8*)v, x, y);
}

// returns 0 if no movement occurred, otherwise returns the result of move_random() or handle_entity_move()
int iterate_monster(DungeonLevel* d, Entity* e)
{
    struct
    {
        uint8_t can_see_pc : 1;
        uint8_t computed_can_see_pc : 1;
    }
    flags;
    Vec2u8 move_pos;
    // vec2u8_copy(&move_pos, &e->pos);    // ensure save no-op so we don't use garbage

#define GET_MIN_COST_NEIGHBOR(vout, map) \
    vec2u8_assign(&vout, e->pos.x + OFF_DIRECTIONS[0][0], e->pos.y + OFF_DIRECTIONS[0][1]); \
    int32_t min_cost = map[vout.y][vout.x]; \
    for(uint8_t i = 1; i < 8; i++) \
    { \
        const uint8_t x = e->pos.x + OFF_DIRECTIONS[i][0]; \
        const uint8_t y = e->pos.y + OFF_DIRECTIONS[i][1]; \
        const int32_t c = map[y][x]; \
        if(c < min_cost) \
        { \
            vout.x = x; \
            vout.y = y; \
            min_cost = c; \
        } \
    }

    if(e->md.intelligence && !e->md.telepathy)  // check LOS if can remember for the future and not telepathic
    {
        if(e->md.using_rem_pos && vec2u8_equal(&e->pos, &e->md.pc_rem_pos)) e->md.using_rem_pos = 0;

        if((flags.can_see_pc = !bresenham_check_los(d, e, &move_pos)))    // set flag and entity state
        {
            PRINT_DEBUG("(%#x) : Monster can see PC - updating known location to (%d, %d).\n", e->md.stats, d->pc->pos.x, d->pc->pos.y);
            vec2u8_copy(&e->md.pc_rem_pos, &d->pc->pos);    // update known pc location
            e->md.using_rem_pos |= 1;
        }
        flags.computed_can_see_pc = 1;
    }
    else
    {
        flags.can_see_pc = 0;
        flags.computed_can_see_pc = 0;
    }

    const int r = rand();
    if(e->md.erratic && (r & 1))
    {
        PRINT_DEBUG("(%#x) : Moving erraticly.\n", e->md.stats);
        return move_random(d, e, (r >> 1));
    }
    else
    {
        if(e->md.telepathy)
        {
            if(e->md.intelligence)
            {
                if(e->md.tunneling)
                {
                    PRINT_DEBUG("(%#x) : Telepathically moving towards PC using the optimal TUNNELING path.\n", e->md.stats);
                    GET_MIN_COST_NEIGHBOR(move_pos, d->terrain_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
                }
                else
                {
                    PRINT_DEBUG("(%#x) : Telepathically moving towards PC using the optimal FLOOR path\n", e->md.stats);
                    GET_MIN_COST_NEIGHBOR(move_pos, d->tunnel_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
                }
            }
            else
            {
                PRINT_DEBUG("(%#x) : Telpathically moving towards the PC using the DIRECT path\n", e->md.stats);
                if(!flags.computed_can_see_pc)
                {
                    bresenham_check_los(d, e, &move_pos);   // get the next cell directly toward the PC
                }

                if(!e->md.tunneling && terrain_is_rock(d->map.terrain[move_pos.y][move_pos.x]))
                {
                    PRINT_DEBUG("(%#x) : Not moving since monster is NON-TUNNELING.\n", e->md.stats);
                    return 0;
                }

                // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
            }
        }
        else
        {
            if(!flags.computed_can_see_pc) flags.can_see_pc = !bresenham_check_los(d, e, &move_pos);

            if(flags.can_see_pc)
            {
                PRINT_DEBUG("(%#x) : Moving directly towards the PC (LINE OF SIGHT).\n", e->md.stats);
                // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
            }
            else
            {
                if(e->md.intelligence && e->md.using_rem_pos)
                {
                    Vec2u a, b;
                    vec2u_assign(&a, e->pos.x, e->pos.y);
                    vec2u_assign(&b, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y);

                    if(e->md.tunneling)
                    {
                        PRINT_DEBUG("(%#x) : Moving towards the PC's last known location (%d, %d) using the optimal TUNNELING path.\n",
                            e->md.stats, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y );
                        dungeon_dijkstra_terrain_path(&d->map, a, b, &move_pos, pathing_export_vec2u8);
                    }
                    else
                    {
                        PRINT_DEBUG("(%#x) : Moving towards the PC's last known location (%d, %d) using the optimal FLOOR path.\n",
                            e->md.stats, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y );
                        dungeon_dijkstra_floor_path(&d->map, a, b, &move_pos, pathing_export_vec2u8);
                    }
                }
                else
                {
                    PRINT_DEBUG("(%#x) : Wandering since no obvious moves are possible.\n", e->md.stats);
                    // return move_random(d, e, r);
                    return 0;
                }
            }
        }
    }

    PRINT_DEBUG("MOVING TO: (%d, %d)\n", move_pos.x, move_pos.y);
    return handle_entity_move(d, e, move_pos.x, move_pos.y);

#undef GET_MIN_COST_NEIGHBOR
}

// static int iterate_pc(DungeonLevel* d, Entity* e)
// {
//     uint8_t x, y;
//     for(uint8_t i = 0; i < 8; i++)
//     {
//         x = e->pos.x + OFF_DIRECTIONS[i][0];
//         y = e->pos.y + OFF_DIRECTIONS[i][1];
//         if(d->entities[y][x])
//         {
//             PRINT_DEBUG("(PC) : Attacking monster in direction <%d, %d>.\n", OFF_DIRECTIONS[i][0], OFF_DIRECTIONS[i][0]);
//             return handle_entity_move(d, e, x, y);
//         }
//     }

//     if(!e->md.using_rem_pos || vec2u8_equal(&e->pos, &e->md.pc_rem_pos))
//     {
//         PRINT_DEBUG("(PC) : Recalculating target position...\n");
//         random_dungeon_map_floor_pos(&d->map, e->md.pc_rem_pos.data);
//         e->md.using_rem_pos = 1;
//     }

//     Vec2u8 mv;
//     Vec2u a, b;
//     vec2u_assign(&a, e->pos.x, e->pos.y);
//     vec2u_assign(&b, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y);
//     dungeon_dijkstra_floor_path(&d->map, a, b, &mv, pathing_export_vec2u8);

//     PRINT_DEBUG("(PC) : Moving to (%d, %d).\n", mv.x, mv.y);
//     return handle_entity_move(d, e, mv.x, mv.y);
// }

// int iterate_entity(DungeonLevel* d, Entity* e)
// {
//     PRINT_DEBUG("Iterating %s! --------------------------------\n", (e->is_pc ? "player" : "monster"));

//     if(e->is_pc) iterate_pc(d, e);
//     else iterate_monster(d, e);

//     return 0;
// }
