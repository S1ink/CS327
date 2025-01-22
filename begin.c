#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"

#define TABLE_MAX_DIM 20
#define TABLE_BASE_LEN (TABLE_MAX_DIM * (TABLE_MAX_DIM * 2 + 1) + 1)
#define TABLE_TEMP_LEN (TABLE_MAX_DIM * TABLE_MAX_DIM)

// #define DIR_DU 0
// #define DIR_R 1
// #define DIR_DD 2
// #define DIR_D 3

const int32_t DIR_X_SCALAR[] = { 1, 1, 1, 0 };
const int32_t DIR_Y_SCALAR[] = { -1, 0, 1, 1 };     // array y-axis goes down

typedef struct{ int32_t x, y; } Vec2i;
typedef struct{ uint32_t x, y; } Vec2u;

char table_base[TABLE_BASE_LEN];
uint32_t availablity_status[TABLE_TEMP_LEN];

uint32_t resolve_unbuffered_idx_2d(uint32_t x, uint32_t y, uint32_t dim)
{
    return (x * (dim + 1) + y);
}
uint32_t resolve_buffered_idx_2d(uint32_t x, uint32_t y, uint32_t dim)
{
    return (x * (dim * 2 + 2) + y * 2);
}
uint32_t resolve_buffered_idx_1d(uint32_t x, uint32_t dim)
{
    return x + (x / dim);
}

uint32_t rand_direction()
{
    return (uint32_t)rand() % 4;
}
uint32_t next_direction(uint32_t dir)
{
    return (dir + 1) % 4;
}

Vec2u rand_position(uint32_t dim)
{
    Vec2u v;
    v.x = (uint32_t)rand() % dim;
    v.y = (uint32_t)rand() % dim;
    return v;
}

MAKE_TYPED_MAX(uint32)
MAKE_TYPED_MAX(int32)
MAKE_TYPED_MIN(uint32)
MAKE_TYPED_MIN(int32)



Vec2i offset_position_2d(
    uint32_t x,
    uint32_t y,
    uint32_t n,
    uint32_t dir )
{
    Vec2i v;
    v.x = (int32_t)x + (int32_t)n * DIR_X_SCALAR[dir];
    v.y = (int32_t)y + (int32_t)n * DIR_Y_SCALAR[dir];
    return v;
}

bool resolve_does_fit(
    uint32_t x,
    uint32_t y,
    uint32_t n,
    uint32_t dir,
    uint32_t dim )
{
    const Vec2i v = offset_position_2d(x, y, n, dir);
    return (v.x >= 0 && v.x <= dim && v.y >= 0 && v.y <= dim);
}

uint32_t get_buffered_nth_letter_idx(
    uint32_t x,
    uint32_t y,
    uint32_t n,
    uint32_t dir,
    uint32_t dim )
{
    const Vec2i v = offset_position_2d(x, y, n, dir);
    return resolve_buffered_idx_2d(v.x, v.y, dim);
}

uint32_t get_unbuffered_nth_letter_idx(
    uint32_t x,
    uint32_t y,
    uint32_t n,
    uint32_t dir,
    uint32_t dim )
{
    const Vec2i v = offset_position_2d(x, y, n, dir);
    return resolve_unbuffered_idx_2d(v.x, v.y, dim);
}

uint32_t get_max_length(uint32_t x, uint32_t y, uint32_t dir, uint32_t dim)
{
    const int32_t x_scalar = DIR_X_SCALAR[dir];
    const int32_t y_scalar = DIR_Y_SCALAR[dir];

    if(x >= dir || y >= dir || x < 0 || y < 0) return 0;

    switch(dir % 4)
    {
        case 0: return uint32_min(dim - x, y + 1);
        case 1: return dim - x;
        case 2: return uint32_min(dim - x, dim - y);
        case 3: return dim - y;
        default: return 0;
    }
}

uint8_t get_avail_len(uint32_t avail, uint32_t dir)
{
    return avail & (0xFF << (dir & 0x4) * 8);
}
void set_avail_len(uint32_t* avail, uint32_t dir, uint8_t len)
{
    const uint32_t shift = (dir & 0x4) * 8;
    *avail = (*avail & !(0xFF << shift)) | ((uint32_t)(len & 0xFF) << shift);
}





void initialize(uint32_t dim)
{
    srand(time(NULL));

    // for(int i = 0; i < TABLE_BASE_LEN; i++)
    // {
    //     table_base[i] = 'a' + (char)(rand() % 26);
    // }
    memset(table_base, 0, TABLE_BASE_LEN);

    table_base[dim * (dim + 1)] = '\0';
    for(int i = 0; i < dim; i++)
    {
        table_base[resolve_unbuffered_idx_2d(i, dim, dim)] = '\n';
    }

    for(uint32_t x = 0; x < dim; x++)
    {
        for(uint32_t y = 0; y < dim; y++)
        {
            uint32_t* avail = availablity_status + (uint32_t)resolve_unbuffered_idx_2d(x, y, dim);
            for(uint32_t d = 0; d < 4; d++)
            {
                set_avail_len(avail, d, (uint8_t)get_max_length(x, y, d, dim));
            }
        }
    }
}

bool try_insert_string(const char* str, uint32_t dim)
{
    const size_t len = strlen(str);
    // printf("trying to insert: %s -- len: %lu\n", str, len);
    if(len > dim || len == 0) return false;

    Vec2u begin_loc = rand_position(dim);
    uint32_t begin_dir = rand_direction();
    // printf("rand pos: (%u, %u), rand dir: %u\n", begin_loc.x, begin_loc.y, begin_dir);
    for(uint32_t _d = 0; _d < 4; _d++)
    {
        const uint32_t d = (_d + begin_dir) % 4;
        for(uint32_t _x = 0; _x < dim; _x++)
        {
            const uint32_t x = (_x + begin_loc.x) % dim;
            for(uint32_t _y = 0; _y < dim; _y++)
            {
                const uint32_t y = (_y + begin_loc.y) % dim;
                const uint32_t* avail = availablity_status + resolve_unbuffered_idx_2d(x, y, dim);
                const uint32_t avail_len = get_avail_len(*avail, d);
                // printf("trying to insert at position: (%u, %u) -- dir: %u -- avail: %u\n", x, y, d, avail_len);
                if(len <= avail_len)  // if can fit at this pose
                {
                    uint32_t n;
                    for(n = 0; n < len; n++)   // check all characters to find collisions
                    {
                        const uint32_t idx = get_buffered_nth_letter_idx(x, y, n, d, dim);
                        if(table_base[idx] != 0 && table_base[idx] != str[n]) break;
                    }
                    if(n == len)
                    {
                        for(n = 0; n < len; n++)
                        {
                            const uint32_t idx = get_buffered_nth_letter_idx(x, y, n, d, dim);
                            table_base[idx] = str[n];
                        }
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void fill_random(uint32_t dim)
{
    for(uint32_t i = 0; i < (dim * dim); i++)
    {
        const uint32_t idx = resolve_buffered_idx_1d(i, dim);
        if(!table_base[idx]) table_base[idx] = 'a' + (char)(rand() % 26);
    }
}

void print_table()
{
    printf("%s", table_base);
}


int main(int argc, char** argv)
{
    if(argc < 2) return -1;

    int32_t table_dim = atoi(argv[1]);
    if(table_dim > TABLE_MAX_DIM) table_dim = 20;
    if(table_dim < 0) table_dim = 5;

    initialize(table_dim);
    for(int s = 2; s < argc; s++)
    {
        if(!try_insert_string(argv[s], table_dim))
        {
            printf("Failed to insert string: %s\n", argv[s]);
            return -2;
        }
    }
    fill_random(table_dim);
    print_table();

    return 0;
}
