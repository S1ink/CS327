#include "dungeon.hpp"

#include <cstring>
#include <random>

#include <endian.h>

#include "util/perlin.hpp"


static int terrain_map_connect_rooms(DungeonLevel::TerrainMap& map, std::mt19937& gen)
{
    for(size_t i = 0; i < map.rooms.size(); i++)
    {
        const size_t i2 = (i + 1) % map.rooms.size();
        Vec2u8
            pos_r1 = Vec2u8::randomInRange(map.rooms[i].tl, map.rooms[i].br, gen),
            pos_r2 = Vec2u8::randomInRange(map.rooms[i2].tl, map.rooms[i2].br, gen);

        dungeon_dijkstra_corridor_path(map, pos_r1, pos_r2);
    }

    return 0;
}
static int terrain_map_fill_room_cells(DungeonLevel::TerrainMap& map)
{
    for(size_t r = 0; r < map.rooms.size(); r++)
    {
        const DungeonLevel::TerrainMap::Room& room = map.rooms[r];
    // #if ENABLE_DEBUG_PRINTS
    //     print_dungeon_room(room);
    // #endif
        for(size_t y = room.tl.y; y <= room.br.y; y++)
        {
            for(size_t x = room.tl.x; x <= room.br.x; x++)
            {
                map.terrain[y][x].type = DungeonLevel::TerrainMap::CELLTYPE_ROOM;
            }
        }
    }

    return 0;
}

static int terrain_map_generate_floors(DungeonLevel::TerrainMap& map, std::mt19937& gen)
{
    const size_t target = random_int<size_t>(DUNGEON_MIN_NUM_ROOMS, DUNGEON_MAX_NUM_ROOMS, gen);
    map.rooms.clear();
    map.rooms.resize(target);

    static const Vec2u8
        d_min{ 1, 1 },
        d_max
        {
            (DUNGEON_X_DIM - DUNGEON_ROOM_MIN_X - 1),
            (DUNGEON_Y_DIM - DUNGEON_ROOM_MIN_Y - 1)
        },
        r_min
        {
            (DUNGEON_ROOM_MIN_X - 1),
            (DUNGEON_ROOM_MIN_Y - 1)
        };

// 1. GENERATE AT LEAST THE MINIMUM NUMBER OF ROOMS
    size_t iter = 0;
    for(size_t i = 0; i < DUNGEON_MIN_NUM_ROOMS; iter++)
    {
        DungeonLevel::TerrainMap::Room& r = map.rooms[i];
        r.tl = Vec2u8::randomInRange(d_min, d_max, gen);
        r.br = r.tl + r_min;

        if(i == 0)
        {
            i++;
            continue;
        }

        int failed = 0;
        for(size_t j = 0; j < i; j++)
        {
            if(r.collides(map.rooms[j]))
            {
                failed = 1;
                break;
            }
        }
        if(!failed) i++;
    }
    // PRINT_DEBUG("Took %lu iterations to generate core rooms.\n", iter)

// 2. GENERATE EXTRA ROOMS
    size_t rnum = DUNGEON_MIN_NUM_ROOMS;
    for(size_t i = DUNGEON_MIN_NUM_ROOMS; i < target; i++)
    {
        DungeonLevel::TerrainMap::Room& r = map.rooms[rnum];
        r.tl = Vec2u8::randomInRange(d_min, d_max, gen);
        r.br = r.tl + r_min;

        rnum++;
        for(size_t j = 0; j < rnum; j++)
        {
            if(r.collides(map.rooms[j]))
            {
                rnum--;
                break;
            }
        }
    }
    // PRINT_DEBUG("Total dungeons generated: %lu\n", R)

    map.rooms.resize(rnum);
    // d->rooms = static_cast<DungeonRoom*>(realloc(d->rooms, sizeof(*d->rooms) * R));
    // if(!d->rooms) return -1;
    // d->num_rooms = R;

// 3. EXPAND DIMENSIONS
    static const Vec2u8
        r_min{ DUNGEON_ROOM_MIN_X, DUNGEON_ROOM_MIN_Y },
        r_max{ DUNGEON_ROOM_MAX_X, DUNGEON_ROOM_MAX_Y };

    for(size_t r = 0; r < rnum; r++)
    {
        DungeonLevel::TerrainMap::Room& room = map.rooms[r];

        Vec2u8 target_size = Vec2u8::randomInRange(r_min, r_max, gen);
        // vec2u_random_in_range(&target_size, r_min, r_max);

        int status = 0;

        Vec2u8 size = room.size();
        // dungeon_room_size(dr, &size);
        size_t iter = 0;
        for(uint8_t b = 0; status < 0b1111 && size.x < target_size.x && size.y < target_size.y; b = !b)
        {
        #define CHECK_COLLISIONS(K, reset) \
            for(size_t i = 1; i < rnum; i++) \
            {   \
                size_t rr = (r + i) % rnum; \
                if(room.collides(map.rooms[rr])) \
                {   \
                    status |= (K); \
                    reset; \
                    break; \
                } \
            }

            if((b && status < 0b11) || (status & 0b1100))
            {
                if(room.br.x >= (DUNGEON_X_DIM - 2)) status |= 0b0001;
                if(!(status & 0b0001))
                {
                    room.br.x += 1;
                    CHECK_COLLISIONS(0b0001, room.br.x -= 1)
                }
                if(room.br.y >= (DUNGEON_Y_DIM - 2)) status |= 0b0010;
                if(!(status & 0b0010))
                {
                    room.br.y += 1;
                    CHECK_COLLISIONS(0b0010, room.br.y -= 1)
                }
            }
            else
            {
                if(room.tl.x <= 2) status |= 0b0100;
                if(!(status & 0b0100) && room.tl.x > 2)
                {
                    room.tl.x -= 1;
                    CHECK_COLLISIONS(0b0100, room.tl.x += 1)
                }
                if(room.tl.y <= 2) status |= 0b1000;
                if(!(status & 0b1000) && room.tl.y > 2)
                {
                    room.tl.y -= 1;
                    CHECK_COLLISIONS(0b1000, room.tl.y += 1)
                }
            }

            size = room.size();

            iter++;
            if(iter > 10000) break;

        #undef CHECK_COLLISIONS
        }
    }

    terrain_map_connect_rooms(map, gen);

    return terrain_map_fill_room_cells(map);
}

static int terrain_map_place_stairs(DungeonLevel::TerrainMap& map, std::mt19937& gen)
{
    static const Vec2u8
        d_min{ 1, 1 },
        d_max{ (DUNGEON_X_DIM - 2), (DUNGEON_Y_DIM - 2) };

    std::uniform_int_distribution<uint16_t>
        nstair_dist{ DUNGEON_MIN_NUM_EACH_STAIR, DUNGEON_MAX_NUM_EACH_STAIR };

    map.num_up_stair = nstair_dist(gen);
    map.num_down_stair = nstair_dist(gen);

    Vec2u8 p;
    for(uint16_t i = 0; i < map.num_up_stair;)
    {
        p = Vec2u8::randomInRange(d_min, d_max, gen);
        DungeonLevel::TerrainMap::Cell& c = map.terrain[p.y][p.x];
        if(c.type > 0)
        {
            c.is_stair = DungeonLevel::TerrainMap::STAIR_UP;
            i++;
        }
    }
    for(uint16_t i = 0; i < map.num_down_stair;)
    {
        p = Vec2u8::randomInRange(d_min, d_max, gen);
        DungeonLevel::TerrainMap::Cell& c = map.terrain[p.y][p.x];
        if(c.type > 0)
        {
            c.is_stair = DungeonLevel::TerrainMap::STAIR_DOWN;
            i++;
        }
    }

    return 0;
}








char DungeonLevel::TerrainMap::Cell::getChar() const
{
    switch(this->is_stair)
    {
        default: break;
        case STAIR_UP: return '<';
        case STAIR_DOWN: return '>';
    }
    switch(this->type)
    {
        default: break;
        case CELLTYPE_CORRIDOR: return '#';
        case CELLTYPE_ROOM: return '.';
    }
    return ' ';
}

bool DungeonLevel::TerrainMap::Room::collides(const Room& other) const
{
    const int32_t
        p = (int32_t)this->br.x - (int32_t)other.tl.x + 1,
        q = (int32_t)this->tl.x - (int32_t)other.br.x - 1,
        r = (int32_t)this->br.y - (int32_t)other.tl.y + 1,
        s = (int32_t)this->tl.y - (int32_t)other.br.y - 1;

    // PRINT_DEBUG("%p, %p -- %d %d %d %d\n", a, b, p, q, r, s)

    return
        (p >= 0 && q <= 0) &&
        (r >= 0 && s <= 0) &&
        (p || r) && (q || s) && (p || s) && (q || r);
}


void DungeonLevel::TerrainMap::reset()
{
    for(size_t i = 0; i < DUNGEON_Y_DIM; i++)
    {
        memset(this->terrain[i], 0x0, sizeof(this->terrain[i]));
        this->hardness[i][0] = this->hardness[i][DUNGEON_X_DIM - 1] = 0xFF;
    }
    for(size_t i = 1; i < DUNGEON_X_DIM - 1; i++)
    {
        this->hardness[0][i] = this->hardness[DUNGEON_Y_DIM - 1][i] = 0xFF;
    }

    this->rooms.clear();
    this->num_up_stair = this->num_down_stair = 0;
}

void DungeonLevel::TerrainMap::generate(uint32_t seed)
{
    std::mt19937 rgen{ seed };

    const int r = rgen();
    const float
        rx = (float)(r & 0xFF),
        ry = (float)((r >> 8) & 0xFF);

    for(size_t y = 1; y < DUNGEON_Y_DIM - 1; y++)
    {
        for(size_t x = 1; x < DUNGEON_X_DIM - 1; x++)
        {
            const float p = perlin2f((float)x * DUNGEON_PERLIN_SCALE_X + rx, (float)y * DUNGEON_PERLIN_SCALE_Y + ry);
            this->hardness[y][x] = (uint8_t)(p * 127.f + 127.f);
        }
    }

    terrain_map_generate_floors(*this, rgen);
    terrain_map_place_stairs(*this, rgen);
}





int DungeonLevel::loadTerrain(FILE* f)
{
// marker, version, and size all unneeded for parsing
    int status;
    uint8_t scratch[20];
    status = fread(scratch, 1, 20, f);

// read PC location
    status = fread(scratch, 1, 2, f);
    *reinterpret_cast<uint16_t*>(this->pc.state.pos.data) = *reinterpret_cast<uint16_t*>(scratch);

// read grid
    uint8_t dungeon_bytes[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    status = fread(dungeon_bytes, sizeof(*dungeon_bytes[0]), (DUNGEON_X_DIM * DUNGEON_Y_DIM), f);
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            TerrainMap::Cell& c = this->map.terrain[y][x];

            if( !(this->map.hardness[y][x] = dungeon_bytes[y][x]) )
            {
                c.type = TerrainMap::CELLTYPE_CORRIDOR;
            }
        }
    }

// read number of rooms
    uint16_t num_rooms;
    status = fread(&num_rooms, sizeof(num_rooms), 1, f);
    num_rooms = be16toh(num_rooms);
    // PRINT_DEBUG("Read num of rooms: %d\n", d->num_rooms);

// read each room
    this->map.rooms.resize(num_rooms);

    for(uint16_t r = 0; r < this->map.rooms.size(); r++)
    {
        status = fread(scratch, sizeof(*scratch), 4, f);

        TerrainMap::Room& room = this->map.rooms[r];
        room.tl.x = scratch[0];
        room.tl.y = scratch[1];
        room.br.x = static_cast<uint8_t>(static_cast<int16_t>(scratch[0]) + scratch[2] - 1);
        room.br.y = static_cast<uint8_t>(static_cast<int16_t>(scratch[1]) + scratch[3] - 1);

    #if ENABLE_DEBUG_PRINTS
        print_dungeon_room(room);
    #endif
    }
    terrain_map_fill_room_cells(this->map);

// read number of upward stairs
    uint16_t num_up;
    status = fread(&num_up, sizeof(num_up), 1, f);
    this->map.num_up_stair = be16toh(num_up);

// read upward stairs
    for(uint16_t s = 0; s < this->map.num_up_stair; s++)
    {
        status = fread(scratch, sizeof(*scratch), 2, f);
        this->map.terrain[scratch[1]][scratch[0]].is_stair = TerrainMap::STAIR_UP;
    }

// read number of downward stairs
    uint16_t num_down;
    status = fread(&num_down, sizeof(num_down), 1, f);
    this->map.num_down_stair = be16toh(num_down);

// read downward stairs
    for(uint16_t s = 0; s < this->map.num_down_stair; s++)
    {
        status = fread(scratch, sizeof(*scratch), 2, f);
        this->map.terrain[scratch[1]][scratch[0]].is_stair = TerrainMap::STAIR_DOWN;
    }

    (void)status;
    return 0;
}

int DungeonLevel::saveTerrain(FILE* f)
{
// 1. Write file type marker
    fwrite("RLG327-S2025", 12, 1, f);

// 2. Write file version (0)
    const uint32_t version = 0;
    // version = htobe32(&version);
    fwrite(&version, sizeof(version), 1, f);

// 3. Write file size
    uint32_t size = (1708U + this->map.rooms.size() * 4 + this->map.num_up_stair * 2 + this->map.num_down_stair * 2);
    // PRINT_DEBUG("Writing file size of %d\n", size);
    size = htobe32(size);
    fwrite(&size, sizeof(size), 1, f);

// 4. Write X and Y position of PC
    const uint8_t* pc_loc = this->pc.state.pos.data;
    // PRINT_DEBUG("Writing PC location of (%d, %d)\n", pc_pos->x, pc_pos->y);
    fwrite(pc_loc, sizeof(*pc_loc), (sizeof(pc_loc) / sizeof(*pc_loc)), f);

    uint8_t *up_stair, *down_stair;
    up_stair = static_cast<uint8_t*>(malloc(sizeof(*up_stair) * this->map.num_up_stair * 2));
    down_stair = static_cast<uint8_t*>(malloc(sizeof(*down_stair) * this->map.num_down_stair * 2));
    if(!up_stair || !down_stair) return -1;

// 5. Write DungeonMap bytes
    uint8_t dungeon_bytes[DUNGEON_Y_DIM][DUNGEON_X_DIM];
    size_t u_idx = 0, d_idx = 0;
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            const TerrainMap::Cell c = this->map.terrain[y][x];
            switch(c.type)
            {
                case TerrainMap::CELLTYPE_ROCK: dungeon_bytes[y][x] = this->map.hardness[y][x]; break;
                case TerrainMap::CELLTYPE_ROOM:
                case TerrainMap::CELLTYPE_CORRIDOR: dungeon_bytes[y][x] = 0; break;
            }

            switch(c.is_stair)
            {
                case TerrainMap::STAIR_UP:
                {
                    if(u_idx < this->map.num_up_stair)
                    {
                        up_stair[u_idx * 2 + 0] = (uint8_t)x;
                        up_stair[u_idx * 2 + 1] = (uint8_t)y;
                        u_idx++;
                    }
                    break;
                }
                case TerrainMap::STAIR_DOWN:
                {
                    if(d_idx < this->map.num_down_stair)
                    {
                        down_stair[d_idx * 2 + 0] = (uint8_t)x;
                        down_stair[d_idx * 2 + 1] = (uint8_t)y;
                        d_idx++;
                    }
                    break;
                }
                case TerrainMap::STAIR_NONE:
                default: break;
            }
        }
    }
    fwrite(dungeon_bytes, sizeof(*dungeon_bytes[0]), (DUNGEON_X_DIM * DUNGEON_Y_DIM), f);

// 6. Write number of rooms in DungeonMap
    // PRINT_DEBUG("Writing num rooms: %x\n", d->num_rooms);
    uint16_t num_rooms = htobe16(static_cast<uint16_t>(this->map.rooms.size()));
    fwrite(&num_rooms, sizeof(num_rooms), 1, f);

// 7. Write room top left corners and sizes
    for(size_t n = 0; n < this->map.rooms.size(); n++)
    {
        const TerrainMap::Room& r = this->map.rooms[n];

        uint8_t room_data[4];
        room_data[0] = static_cast<uint8_t>(r.tl.x),
        room_data[1] = static_cast<uint8_t>(r.tl.y),
        room_data[2] = static_cast<uint8_t>(r.br.x - r.tl.x + 1),
        room_data[3] = static_cast<uint8_t>(r.br.y - r.tl.y + 1);

        // PRINT_DEBUG("Writing room %lu data: (%d, %d, %d, %d)\n", n, room_data[0], room_data[1], room_data[2], room_data[3]);

        fwrite(room_data, sizeof(*room_data), (sizeof(room_data) / sizeof(*room_data)), f);
    }

// 8. Write the number of upward staircases
    // PRINT_DEBUG("Writing number of up staircases: %d\n", this->map.num_up_stair);
    uint16_t num_up = htobe16(this->map.num_up_stair);
    fwrite(&num_up, sizeof(num_up), 1, f);

// 9. Write each upward staircase
    fwrite(up_stair, sizeof(*up_stair), this->map.num_up_stair * 2, f);

// 10. Write the number of downward staircases
    // PRINT_DEBUG("Writing number of down staircases: %d\n", this->map.num_down_stair);
    uint16_t num_down = htobe16(this->map.num_down_stair);
    fwrite(&num_down, sizeof(num_down), 1, f);

// 11. Write each downward staircase
    fwrite(down_stair, sizeof(*down_stair), this->map.num_down_stair * 2, f);

    free(up_stair);
    free(down_stair);

    return 0;
}

int DungeonLevel::generateTerrain()
{
    this->map.generate(this->rgen());
}



int DungeonLevel::updateCosts(bool both_or_only_terrain)
{
    static PathFindingBuffer buff;
    static int buff_inited = 0;
    if(!buff_inited)
    {
        init_pathing_buffer(buff);
        buff_inited = 1;
    }

    if(both_or_only_terrain)
    {
        dungeon_dijkstra_traverse_floor(this->map, this->pc.state.pos, buff);
        for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
        {
            for(size_t x = 0; x < DUNGEON_X_DIM; x++)
            {
                this->tunnel_costs[y][x] = buff[y][x].cost;
            }
        }
    }
    dungeon_dijkstra_traverse_terrain(this->map, this->pc.state.pos, buff);
    for(size_t y = 0; y < DUNGEON_Y_DIM; y++)
    {
        for(size_t x = 0; x < DUNGEON_X_DIM; x++)
        {
            this->terrain_costs[y][x] = buff[y][x].cost;
        }
    }

    return 0;
}