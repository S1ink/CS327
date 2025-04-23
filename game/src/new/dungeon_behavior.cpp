#include "dungeon.hpp"

#include "status.h"
#include "util/debug.hpp"
#include "util/math.hpp"
#include "util/heap.h"

#ifndef BRESENHAM_DEBUG
#define BRESENHAM_DEBUG 0
#endif


static constexpr int8_t OFF_DIRECTIONS[8][2] =
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

static uint8_t filter_valid_terrain_directions(
    DungeonLevel::TerrainMap& map,
    Vec2u8 pos,
    bool tunneling,
    uint8_t valid_dirs[8])
{
    uint8_t n_valid_dirs = 0;
    uint8_t x, y;
    for(size_t i = 0; i < 8; i++)
    {
        x = pos.x + OFF_DIRECTIONS[i][0];
        y = pos.y + OFF_DIRECTIONS[i][1];
        if(map.terrain[y][x].type || (tunneling && map.hardness[y][x] < 0xFF))
        {
            valid_dirs[n_valid_dirs] = i;
            n_valid_dirs++;
        }
    }
    return n_valid_dirs;
}
static uint8_t filter_open_cells(
    DungeonLevel& d,
    Vec2u8 pos,
    uint8_t valid_dirs[8] )
{
    const uint8_t n = filter_valid_terrain_directions(d.map, pos, false, valid_dirs);

    uint8_t x, y, r = 0;
    for(uint8_t i = 0; i < n; i++)
    {
        x = pos.x + OFF_DIRECTIONS[valid_dirs[i]][0];
        y = pos.y + OFF_DIRECTIONS[valid_dirs[i]][1];
        if(!d.entity_map[y][x])
        {
            valid_dirs[r] = valid_dirs[i];
            r++;
        }
    }
    return r;
}

// returns 1 if the entity successfully moved, 0 otherwise
static int handle_entity_move(DungeonLevel& d, Entity& e, Vec2u8 to)
{
    Entity*& prev_slot = DungeonLevel::accessGridElem(d.entity_map, e.state.pos);
    struct
    {
        uint8_t terrain_updated : 1;
        uint8_t floor_updated : 1;
        uint8_t has_entity_moved : 1;
    }
    flags;

    *(uint8_t*)(&flags) = 0;

    if(DungeonLevel::accessGridElem(d.map.terrain, to).isRock())
    {
        if(e.config.can_tunnel)
        {
            uint8_t& h =  DungeonLevel::accessGridElem(d.map.hardness, to);
            h = (h > 85 ? h - 85 : 0);
            if(!h)
            {
                DungeonLevel::accessGridElem(d.map.terrain, to).type = DungeonLevel::TerrainMap::CELLTYPE_CORRIDOR;
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
        Entity*& slot = DungeonLevel::accessGridElem(d.entity_map, to);
        if(slot)   // previous entity
        {
            if(slot->config.is_pc)
            {
                slot->state.health -= e.config.attack_damage.roll(d.rroll);
                if(slot->state.health <= 0)
                {
                    e.state.pos = to;
                    d.win_lose = -1;
                }
                else
                {
                    flags.has_entity_moved = false;
                }
            }
            else
            {
                Entity* x = slot;
                e.state.pos = to;
                slot = &e;
                prev_slot = nullptr;

                uint8_t valid_dirs[8];
                const uint8_t n_dirs = filter_open_cells(d, x->state.pos, valid_dirs);
                const uint8_t ri = RANDOM_IN_RANGE(0, n_dirs - 1);
                x->state.pos.x += OFF_DIRECTIONS[valid_dirs[ri]][0];
                x->state.pos.y += OFF_DIRECTIONS[valid_dirs[ri]][1];
                DungeonLevel::accessGridElem(d.entity_map, x->state.pos) = x;
            }
        }
        else
        {
            e.state.pos = to;
            slot = &e;
            prev_slot = nullptr;
        }
    }

    if(flags.terrain_updated)
    {
        // PRINT_DEBUG("UPDATING TERRAIN %sCOSTS\n", flags.floor_updated ? "(and floor) " : "");
        d.updateCosts(flags.floor_updated);
    }

    return flags.has_entity_moved;
}
// returns the result of handle_entity_move()
static int handle_entity_move_dir(DungeonLevel& d, Entity& e, uint8_t dir_idx)
{
    return handle_entity_move( d, e, e.state.pos + Vec2u8{ OFF_DIRECTIONS[dir_idx][0], OFF_DIRECTIONS[dir_idx][1] } );
}

// returns the result of handle_entity_move_dir() a valid direction was detected, otherwise 0
static int move_random(DungeonLevel& d, Entity& e, int r)
{
    const bool has_tunneling = e.config.can_tunnel;

    uint8_t valid_dirs[8];
    uint8_t n_valid_dirs = filter_valid_terrain_directions(d.map, e.state.pos, has_tunneling, valid_dirs);

    return n_valid_dirs ? handle_entity_move_dir(d, e, valid_dirs[(r ? r : rand()) % n_valid_dirs]) : 0;
}

static int bresenham_check_los(DungeonLevel& d, Entity& e, Vec2u8& trav_cell)
{
    #define ENTITY e.state.pos
    #define PLAYER d.pc.state.pos

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
            trav_cell.x = (uint8_t)x;
            trav_cell.y = (uint8_t)y;
        }
        if(d.map.terrain[y][x].isRock())  // exit on rock intersection
        {
            return i + 1;
        }
    }

    return 0;

    #undef ENTITY
    #undef PLAYER
}

static void pathing_export_vec2u8(void* v, uint8_t x, uint8_t y)
{
    reinterpret_cast<Vec2u8*>(v)->assign(x, y);
}




int DungeonLevel::handlePCMove(Vec2u8 to, bool is_goto)
{
    if(to == this->pc.state.pos) return false;

    Entity*& prev_slot = DungeonLevel::accessGridElem(this->entity_map, this->pc.state.pos);
    bool has_moved = false;

    if(DungeonLevel::accessGridElem(this->map.terrain, to).isRock())
    {
        if(is_goto)
        {
            DungeonLevel::accessGridElem(this->map.hardness, to) = 0;
            DungeonLevel::accessGridElem(this->map.terrain, to).type = DungeonLevel::TerrainMap::CELLTYPE_CORRIDOR;
            has_moved = true;
        }
    }
    else
    {
        has_moved = true;
    }

    if(has_moved)
    {
        Entity*& slot = DungeonLevel::accessGridElem(this->entity_map, to);
        if(slot)   // previous entity
        {
            int32_t a = this->rollPCDamage();
            slot->state.health -= a;

            if(slot->state.health <= 0)
            {
                NC_PRINT("Dealt %d damage to [%s (dead)]", a, slot->config.name.data());

                this->npcs_remaining--;
                if(slot->config.is_boss) this->win_lose = 1;

                slot = &this->pc;
                this->pc.state.pos = to;
                prev_slot = nullptr;
            }
            else
            {
                NC_PRINT("Dealt %d damage to [%s (H: %d health)]", a, slot->config.name.data(), slot->state.health);
            }
        }
        else
        {
            slot = &this->pc;
            this->pc.state.pos = to;
            prev_slot = nullptr;
        }

        // PRINT_DEBUG("UPDATING TERRAIN %sCOSTS\n", flags.floor_updated ? "(and floor) " : "");
        this->copyVisCells();
        this->updateCosts(true);
    }

    return has_moved;
}

// returns 0 if no movement occurred, otherwise returns the result of move_random() or handle_entity_move()
int DungeonLevel::iterateNPC(Entity& e)
{
    // FileDebug::get()
    //     << "\tSM : " << (int)e.config.is_smart
    //     << ", TE : " << (int)e.config.is_tele
    //     << ", TN : " << (int)e.config.can_tunnel
    //     << ", ER : " << (int)e.config.is_erratic << '\n';

    struct
    {
        uint8_t can_see_pc : 1;
        uint8_t computed_can_see_pc : 1;
    }
    flags;
    Vec2u8 move_pos;
    // vec2u8_copy(&move_pos, &e->pos);    // ensure save no-op so we don't use garbage

#define GET_MIN_COST_NEIGHBOR(vout, map) \
    vout.assign(e.state.pos.x + OFF_DIRECTIONS[0][0], e.state.pos.y + OFF_DIRECTIONS[0][1]); \
    int32_t min_cost = map[vout.y][vout.x]; \
    for(uint8_t i = 1; i < 8; i++) \
    { \
        const uint8_t x = e.state.pos.x + OFF_DIRECTIONS[i][0]; \
        const uint8_t y = e.state.pos.y + OFF_DIRECTIONS[i][1]; \
        const int32_t c = map[y][x]; \
        if(c < min_cost) \
        { \
            vout.x = x; \
            vout.y = y; \
            min_cost = c; \
        } \
    }

    if(e.config.is_smart && !e.config.is_tele)  // check LOS if can remember for the future and not telepathic
    {
        if(e.state.target_pos != Vec2u8{ 0, 0 } && e.state.pos == e.state.target_pos) e.state.target_pos.assign(0, 0);

        if((flags.can_see_pc = !bresenham_check_los(*this, e, move_pos)))    // set flag and entity state
        {
            // PRINT_DEBUG("(%#x) : Monster can see PC - updating known location to (%d, %d).\n", e->md.stats, d->pc->pos.x, d->pc->pos.y);
            e.state.target_pos = this->pc.state.pos;    // update known pc location
        }
        flags.computed_can_see_pc = 1;
    }
    else
    {
        flags.can_see_pc = 0;
        flags.computed_can_see_pc = 0;
    }

    // FileDebug::get() << "\tflags.can_see_pc : " << (int)flags.can_see_pc
    //     << ", flags.computed_can_see_pc : " << (int)flags.computed_can_see_pc << '\n';

    const int r = rand();
    if(e.config.is_erratic && (r & 0x1))
    {
        // PRINT_DEBUG("(%#x) : Moving erraticly.\n", e->md.stats);
        return move_random(*this, e, (r >> 1));
    }
    else
    {
        if(e.config.is_tele)
        {
            if(e.config.is_smart)
            {
                if(e.config.can_tunnel)
                {
                    // PRINT_DEBUG("(%#x) : Telepathically moving towards PC using the optimal TUNNELING path.\n", e->md.stats);
                    GET_MIN_COST_NEIGHBOR(move_pos, this->terrain_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
                }
                else
                {
                    // PRINT_DEBUG("(%#x) : Telepathically moving towards PC using the optimal FLOOR path\n", e->md.stats);
                    GET_MIN_COST_NEIGHBOR(move_pos, this->tunnel_costs)
                    // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
                }
            }
            else
            {
                // PRINT_DEBUG("(%#x) : Telpathically moving towards the PC using the DIRECT path\n", e->md.stats);
                if(!flags.computed_can_see_pc)
                {
                    bresenham_check_los(*this, e, move_pos);   // get the next cell directly toward the PC
                }

                if(!e.config.can_tunnel && this->map.terrain[move_pos.y][move_pos.x].isRock())
                {
                    // PRINT_DEBUG("(%#x) : Not moving since monster is NON-TUNNELING.\n", e->md.stats);
                    return 0;
                }

                // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
            }
        }
        else
        {
            if(!flags.computed_can_see_pc) flags.can_see_pc = !bresenham_check_los(*this, e, move_pos);

            if(flags.can_see_pc)
            {
                // PRINT_DEBUG("(%#x) : Moving directly towards the PC (LINE OF SIGHT).\n", e->md.stats);
                // return handle_entity_move(d, e, move_pos.x, move_pos.y); (END)
            }
            else
            {
                if(e.config.is_smart && e.state.target_pos != Vec2u8{ 0, 0 })
                {
                    if(e.config.can_tunnel)
                    {
                        // PRINT_DEBUG("(%#x) : Moving towards the PC's last known location (%d, %d) using the optimal TUNNELING path.\n",
                        //     e->md.stats, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y );
                        dungeon_dijkstra_terrain_path(this->map, e.state.pos, e.state.target_pos, &move_pos, pathing_export_vec2u8);
                    }
                    else
                    {
                        // PRINT_DEBUG("(%#x) : Moving towards the PC's last known location (%d, %d) using the optimal FLOOR path.\n",
                        //     e->md.stats, e->md.pc_rem_pos.x, e->md.pc_rem_pos.y );
                        dungeon_dijkstra_floor_path(this->map,  e.state.pos, e.state.target_pos, &move_pos, pathing_export_vec2u8);
                    }
                }
                else
                {
                    // PRINT_DEBUG("(%#x) : Wandering since no obvious moves are possible.\n", e->md.stats);
                    // return move_random(d, e, r);
                    return 0;
                }
            }
        }
    }

    // FileDebug::get() << "\tNPC attempting to move to : (" << move_pos.x << ", " << move_pos.y << ")\n";

    // PRINT_DEBUG("MOVING TO: (%d, %d)\n", move_pos.x, move_pos.y);
    return handle_entity_move(*this, e, move_pos);

#undef GET_MIN_COST_NEIGHBOR
}



size_t DungeonLevel::getEquipmentSlotIdx(const Item& i)
{
    constexpr uint32_t EQUIP_MASK =
        ItemDescription::TYPE_WEAPON |
        ItemDescription::TYPE_OFFHAND |
        ItemDescription::TYPE_RANGED |
        ItemDescription::TYPE_ARMOR |
        ItemDescription::TYPE_HELMET |
        ItemDescription::TYPE_CLOAK |
        ItemDescription::TYPE_GLOVES |
        ItemDescription::TYPE_BOOTS |
        ItemDescription::TYPE_RING |
        ItemDescription::TYPE_AMULET |
        ItemDescription::TYPE_LIGHT;

    switch(i.type & EQUIP_MASK)
    {
        case ItemDescription::TYPE_WEAPON: return 0;
        case ItemDescription::TYPE_OFFHAND: return 1;
        case ItemDescription::TYPE_RANGED: return 2;
        case ItemDescription::TYPE_ARMOR: return 3;
        case ItemDescription::TYPE_HELMET: return 4;
        case ItemDescription::TYPE_CLOAK: return 5;
        case ItemDescription::TYPE_GLOVES: return 6;
        case ItemDescription::TYPE_BOOTS: return 7;
        case ItemDescription::TYPE_AMULET: return 8;
        case ItemDescription::TYPE_LIGHT: return 9;
        case ItemDescription::TYPE_RING:
        {
            return (!this->pc_equipment[10] || (this->pc_equipment[10] && this->pc_equipment[11])) ? 10 : 11;
        }
        default: return 12;
    }
}

int32_t DungeonLevel::rollPCDamage()
{
    int32_t ret = 0;

    bool no_equip = true;
    for(Item* i : this->pc_equipment)
    {
        if(!i) continue;
        no_equip = false;
        if(i->attack_damage.isStatic())
        {
            ret += i->attack_damage.getBase();
        }
        else
        {
            ret += i->attack_damage.roll(this->rroll);
        }
    }

    return no_equip ? this->pc.config.attack_damage.roll(this->rroll) : ret;
}

int32_t DungeonLevel::getPCSpeed()
{
    int32_t ret = this->pc.config.speed;

    for(const Item* i : this->pc_equipment)
    {
        if(i) ret += i->speed;
    }

    return ret;
}

size_t DungeonLevel::getOpenCarrySlot()
{
    for(size_t i = 0; i < this->pc_carry.size(); i++)
    {
        if(!this->pc_carry[i]) return i;
    }
    return this->pc_carry.size();
}

void DungeonLevel::handleItemDrop(Item& i)
{
    if(!DungeonLevel::accessGridElem(this->item_map, this->pc.state.pos))
    {
        DungeonLevel::accessGridElem(this->item_map, this->pc.state.pos) = &i;
    }
    else
    {
        DungeonLevel::accessGridElem(
            this->item_map,
            dungeon_dijkstra_nearest_open_drop(*this, this->pc.state.pos) ) = &i;
    }
}

void DungeonLevel::handleItemDelete(size_t idx)
{
    delete this->pc_carry[idx];
    this->pc_carry[idx] = nullptr;
}
