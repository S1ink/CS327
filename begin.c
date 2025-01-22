#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define TABLE_MAX_DIM 20
#define TABLE_BASE_LEN (TABLE_MAX_DIM * (TABLE_MAX_DIM + 1) + 1)
#define TABLE_TEMP_LEN (TABLE_MAX_DIM * TABLE_MAX_DIM)

#define DIR_DU 0
#define DIR_R 1
#define DIR_DD 2
#define DIR_D 3

const int32_t DIR_X_SCALAR[] = { 1, 1, -1, 0 };
const int32_t DIR_Y_SCALAR[] = { -1, 0, 1, 1 };     // array y-axis goes down

typedef struct{ int32_t x, y; } Vec2i;

char table_base[TABLE_BASE_LEN];
char table_temp[TABLE_TEMP_LEN];
// uint32_t availablity_status[TABLE_TEMP_LEN];

uint32_t resolve_unbuffered_idx_2d(uint32_t x, uint32_t y, uint32_t dim)
{
    return (x * (dim + 1) + y);
}
uint32_t resolve_buffered_idx_2d(uint32_t x, uint32_t y, uint32_t dim)
{
    return (x * (dim + 2) + y);
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

// uint32_t get_avail_len(uint64_t avail, uint32_t dir)
// {
//     return avail & (0xFFFF << (dir & 0x4) * 16);
// }
// void set_avail_len(uint64_t* avail, uint32_t dir, uint32_t len)
// {
//     const uint32_t shift = (dir & 0x4) * 16;
//     *avail = (*avail & !(0xFFFF << shift)) | ((uint64_t)(len & 0xFFFF) << shift);
// }





void initialize(uint32_t dim)
{
    srand(time(NULL));

    for(int i = 0; i < TABLE_BASE_LEN; i++)
    {
        table_base[i] = 'a' + (char)(rand() % 26);
    }
    memset(table_temp, 0, TABLE_TEMP_LEN);

    table_base[dim * (dim + 1)] = '\0';
    for(int i = 0; i < dim; i++)
    {
        table_base[resolve_unbuffered_idx_2d(i, dim, dim)] = '\n';
    }
}

bool try_insert_string(const char* str, uint32_t dim)
{
    const size_t len = strlen(str);
    if(len > dim) return false;

    return false;
}

void write_temp_table(uint32_t dim)
{
    for(uint32_t i = 0; i < (dim * dim); i++)
    {
        if(table_temp[i])
        {
            table_base[resolve_buffered_idx_1d(i, dim)] = table_temp[i];
        }
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
    write_temp_table(table_dim);
    print_table();

    return 0;
}
