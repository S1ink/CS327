#include "dungeon.h"

#include "util/debug.h"
#include "util/math.h"
#include "util/heap.h"

int dungeon_level_update_costs(DungeonLevel* d);    // from dungeon.c

/* No header since these are only used in dungeon.c -- declarations are internal */

static inline int entity_has_tunneling(Entity* e)
{
    return e->md && (e->md->stats & TUNNELING);
}

static const int8_t OFF_DIRECTIONS[8][2] =
{
    { +1, +1 },
    { +1,  0 },
    { +1, -1 },
    {  0, -1 },
    { -1, -1 },
    { -1,  0 },
    { -1, +1 },
    {  0, +1 },
};

static int handle_entity_move(DungeonLevel* d, Entity* e, uint8_t dir_idx)
{
    Entity** prev_slot = d->entities[e->pos.y] + e->pos.x;

    uint8_t x, y;
    x = (e->pos.x + OFF_DIRECTIONS[dir_idx][0]);
    y = (e->pos.y + OFF_DIRECTIONS[dir_idx][1]);

    enum
    {
        FLAG_TERRAIN_UPDATED = 0b001,
        FLAG_PC_MOVED        = 0b010,
        FLAG_COMPLETED_MOVE  = 0b100
    };
    int flags = 0;

    if(entity_has_tunneling(e) && !d->map.terrain[y][x].type)
    {
        uint8_t* h =  d->map.hardness[y] + x;
        *h = (*h > 85 ? *h - 85 : 0);
        if(!*h)
        {
            d->map.terrain[y][x].type = CORRIDOR;
            flags |= FLAG_COMPLETED_MOVE;
        }
        flags |= FLAG_TERRAIN_UPDATED;
    }
    else
    {
        flags |= FLAG_COMPLETED_MOVE;
    }

    if(flags & FLAG_COMPLETED_MOVE)
    {
        e->pos.x = x;
        e->pos.y = y;
        if(!e->md) flags |= FLAG_PC_MOVED;

        Entity** slot = d->entities[y] + x;
        if(*slot)   // previous entity
        {
            (*slot)->hn = NULL; // this marks that the entity will no longer be iterated -- TODO: immediate heap removal!
            if((*slot)->md)
            {
                // varray_destroy(&(*slot)->md->path);
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

static int move_random(DungeonLevel* d, Entity* e)
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

    return n_valid_dirs ? handle_entity_move(d, e, valid_dirs[rand() % n_valid_dirs]) : 0;
}


int iterate_entity(DungeonLevel* d, Entity* e)
{
    PRINT_DEBUG("Iterating %s!\n", (e->md ? "monster" : "player"));

    // e->hn = heap_insert(&d->entity_q, e);
    move_random(d, e);
    return (d->pc) ? (d->num_monsters ? 0 : 1) : -1;

    return 0;
}
