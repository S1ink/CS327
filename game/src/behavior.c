#include "dungeon.h"
#include "pathing.h"

#include "util/debug.h"
#include "util/math.h"
#include "util/heap.h"

int dungeon_level_update_costs(DungeonLevel* d);    // from dungeon.c

/* No header since these are only used in dungeon.c -- declarations are internal */

static inline int entity_has_tunneling(Entity* e)
{
    return !e->is_pc && (e->md.tunneling);
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

static int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t x, uint8_t y)
{
    Entity** prev_slot = d->entities[e->pos.y] + e->pos.x;

    enum
    {
        FLAG_TERRAIN_UPDATED = 0b001,
        FLAG_PC_MOVED        = 0b010,
        FLAG_ENTITY_MOVED    = 0b100
    };
    int flags = 0;

    if(entity_has_tunneling(e) && !d->map.terrain[y][x].type)
    {
        uint8_t* h =  d->map.hardness[y] + x;
        *h = (*h > 85 ? *h - 85 : 0);
        if(!*h)
        {
            d->map.terrain[y][x].type = CORRIDOR;
            flags |= FLAG_ENTITY_MOVED;
        }
        flags |= FLAG_TERRAIN_UPDATED;
    }
    else
    {
        flags |= FLAG_ENTITY_MOVED;
    }

    if(flags & FLAG_ENTITY_MOVED)
    {
        e->pos.x = x;
        e->pos.y = y;
        if(e->is_pc) flags |= FLAG_PC_MOVED;

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

    if(flags & (FLAG_TERRAIN_UPDATED | FLAG_PC_MOVED))
    {
        PRINT_DEBUG("UPDATING TERRAIN COSTS\n");
        dungeon_level_update_costs(d);
    }

    return 0;
}
static int handle_entity_move_dir(DungeonLevel* d, Entity* e, uint8_t dir_idx)
{
    return handle_entity_move( d, e, (e->pos.x + OFF_DIRECTIONS[dir_idx][0]), (e->pos.y + OFF_DIRECTIONS[dir_idx][1]) );
}

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
    #define A e->pos
    #define B d->pc->pos

    PRINT_DEBUG("BRESENHAM: calculating from (%d, %d) to (%d, %d)\n", A.x, A.y, B.x, B.y);

    const int32_t dx = abs((int32_t)B.x - (int32_t)A.x);
    const int32_t dy = -abs((int32_t)B.y - (int32_t)A.y);
    const int32_t sx = A.x < B.x ? 1 : -1;
    const int32_t sy = A.y < B.y ? 1 : -1;
    int32_t err = dx + dy;

    int32_t x = A.x;
    int32_t y = A.y;
    for(int i = 0; !(x == B.x && y == B.y); i++)
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

        PRINT_DEBUG("BRESENHAM ITERATION %d : (%d, %d) -- terrain: %d\n", i, x, y, d->map.terrain[y][x].type);

        if(!i)
        {
            trav_cell->x = (uint8_t)x;
            trav_cell->y = (uint8_t)y;
        }
        if(!d->map.terrain[y][x].type)
        {
            return i + 1;
        }
    }

    return 0;

    #undef A
    #undef B
}

static void pathing_export_vec2u8(void* v, uint32_t x, uint32_t y)
{
    vec2u8_assign((Vec2u8*)v, x, y);
}

static int iterate_monster(DungeonLevel* d, Entity* e)
{
    struct
    {
        uint8_t can_see_pc : 1;
        uint8_t computed_can_see_pc : 1;
    }
    flags;
    Vec2u8 move_pos;

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
        if((flags.can_see_pc = e->md.using_rem_pos = !bresenham_check_los(d, e, &move_pos)))    // set flag and entity state
        {
            PRINT_DEBUG("Monster can see PC -- updated rem\n");
            vec2u8_copy(&e->md.pc_rem_pos, &d->pc->pos);    // update known pc location
        }
        flags.computed_can_see_pc = 1;
    }
    else
    {
        flags.can_see_pc = 0;
        flags.computed_can_see_pc = 0;
    }

    const int r = rand();
    if(e->md.erratic && (r % 2))
    {
        PRINT_DEBUG("Moving erratic...\n");
        return move_random(d, e, r);
    }
    else
    {
        if(e->md.telepathy)
        {
            if(e->md.intelligence)
            {
                if(e->md.tunneling)
                {
                    PRINT_DEBUG("Moving to lowest cost terrain cell\n");
                    GET_MIN_COST_NEIGHBOR(move_pos, d->terrain_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y);
                }
                else
                {
                    PRINT_DEBUG("Moving to lowest cost floor cell\n");
                    GET_MIN_COST_NEIGHBOR(move_pos, d->tunnel_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y);
                }
            }
            else
            {
                PRINT_DEBUG("Moving telepathically to PC using bresenham iteration\n");
                if(!flags.computed_can_see_pc) flags.can_see_pc = !bresenham_check_los(d, e, &move_pos);

                if(!e->md.tunneling && !d->map.terrain[move_pos.y][move_pos.x].type)
                {
                    return 0;   // trying to move into rock, but cannot tunnel --> no movement.
                }

                // return handle_entity_move(d, e, move_pos.x, move_pos.y);
            }
        }
        else
        {
            if(!flags.computed_can_see_pc) flags.can_see_pc = !bresenham_check_los(d, e, &move_pos);

            if(flags.can_see_pc)
            {
                PRINT_DEBUG("Moving directly towards PC (sighting) using precomputed bresenham\n");
                // return handle_entity_move(d, e, move_pos.x, move_pos.y);
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
                        PRINT_DEBUG("Computing efficient path to PC's LKL over all terrain\n");
                        dungeon_dijkstra_terrain_path(&d->map, a, b, &move_pos, pathing_export_vec2u8);
                    }
                    else
                    {
                        PRINT_DEBUG("Computing efficient path to PC's LKL over floors only\n");
                        dungeon_dijkstra_floor_path(&d->map, a, b, &move_pos, pathing_export_vec2u8);
                    }
                }
                else
                {
                    PRINT_DEBUG("Moving randomly since no other moves are possible\n");
                    // move random, or none
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


int iterate_entity(DungeonLevel* d, Entity* e)
{
    PRINT_DEBUG("Iterating %s!\n", (e->is_pc ? "player" : "monster"));

    // e->hn = heap_insert(&d->entity_q, e);
    if(!e->is_pc) iterate_monster(d, e);
    else move_random(d, e, 0);
    return (d->pc) ? (d->num_monsters ? 0 : 1) : -1;

    return 0;
}
