#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"

#define CALC_MATRIX_BUFFERED_ROW_LEN(x) (x * 2)
#define CALC_MATRIX_BUFFERED_SIZE(cols, rows) (cols * rows + 1)

#define MATRIX_MAX_DIM          20
#define MATRIX_STRING_ROW_LEN   CALC_MATRIX_BUFFERED_ROW_LEN(MATRIX_MAX_DIM)
#define MATRIX_STRING_COL_LEN   (MATRIX_MAX_DIM)
#define MATRIX_STRING_LEN       CALC_MATRIX_BUFFERED_SIZE(MATRIX_STRING_COL_LEN, MATRIX_STRING_ROW_LEN)

char string_matrix_1d[MATRIX_STRING_LEN];       // base buffer that can be directly printed
char* string_matrix_2d[MATRIX_STRING_COL_LEN];  // index to each row
uint32_t distance_field[MATRIX_MAX_DIM][MATRIX_MAX_DIM];

MAKE_TYPED_MAX(uint32)
MAKE_TYPED_MAX(int32)
MAKE_TYPED_MIN(uint32)
MAKE_TYPED_MIN(int32)


typedef struct{ int32_t x, y; } Vec2i;
typedef struct{ uint32_t x, y; } Vec2u;


void initilize_2d_pointers(uint32_t row_len)
{
    for(size_t i = 0; i < MATRIX_STRING_COL_LEN; i++)
    {
        string_matrix_2d[i] = string_matrix_1d + (i * row_len * sizeof(char));
    }
}

uint32_t buffered_spaced_row_len(uint32_t dim)
{
    return CALC_MATRIX_BUFFERED_ROW_LEN(dim);
}
uint32_t buffered_string_matrix_len(uint32_t dim)
{
    return CALC_MATRIX_BUFFERED_SIZE(dim, buffered_spaced_row_len(dim));
}

uint32_t rand_direction()
{
    return (uint32_t)rand() % 4;
}

Vec2u rand_position(uint32_t dim)
{
    Vec2u v;
    v.x = (uint32_t)rand() % dim;
    v.y = (uint32_t)rand() % dim;
    return v;
}



size_t get_longest_word(int32_t n, char** words)
{
    size_t max = 0;
    for(n--; n >= 0; n--)
    {
        const size_t l = strlen(words[n]);
        if(l > max) max = l;
    }
    return max;
}

void initialize(uint32_t dim)
{
    srand(time(NULL));
    const uint32_t
        matx_size = buffered_string_matrix_len(dim),
        matx_row_size = buffered_spaced_row_len(dim);

    memset(string_matrix_1d, ' ', matx_size);
    string_matrix_1d[matx_size - 1] = '\0';
    initilize_2d_pointers(matx_row_size);

    for(uint32_t i = 0; i < dim; i++)
    {
        string_matrix_2d[i][matx_row_size - 1] = '\n';
    }

    for(uint32_t y = 0; y < dim; y++)
    {
        for(uint32_t x = 0; x < dim; x++)
        {
            distance_field[y][x] =
                ((uint32_min(dim - x, y + 1)    & 0xFF) <<  0) |
                (((dim - x)                     & 0xFF) <<  8) |
                ((uint32_min(dim - x, dim - y)  & 0xFF) << 16) |
                (((dim - y)                     & 0xFF) << 24);
        }
    }
}

bool try_insert_string(const char* str, uint32_t dim)
{
    const size_t len = strlen(str);
    // printf("trying to insert: %s -- len: %lu\n", str, len);
    if(len > dim || len == 0)
    {
        printf("Failed to insert string: \"%s\" (invalid length)\n", str);
        return false;
    }

    const Vec2u begin_loc = rand_position(dim);
    const uint32_t begin_dir = rand_direction();
    // printf("rand pos: (%u, %u), rand dir: %u\n", begin_loc.x, begin_loc.y, begin_dir);
    for(uint32_t _y = 0; _y < dim; _y++)
    {
        const uint32_t y = (_y + begin_loc.y) % dim;
        for(uint32_t _x = 0; _x < dim; _x++)
        {
            const uint32_t x = (_x + begin_loc.x) % dim;
            for(uint32_t _d = 0; _d < 4; _d++)
            {
                const uint32_t d = (_d + begin_dir) % 4;
                const uint32_t avail_len = (distance_field[y][x] >> (d * 8)) & 0xFF;
                // printf("trying to insert at position: (%u, %u) -- dir: %u -- avail: %u\n", x, y, d, avail_len);
                if(len <= avail_len)  // if can fit at this pose
                {
                    static const int32_t DIR_X_SCALAR[] = { 1, 1, 1, 0 };
                    static const int32_t DIR_Y_SCALAR[] = { -1, 0, 1, 1 };     // array y-axis goes down

                    uint32_t n;
                    for(n = 0; n < len; n++)   // check all characters to find collisions
                    {
                        const int32_t vy = (int32_t)y + (int32_t)n * DIR_Y_SCALAR[d];
                        const int32_t vx = (int32_t)x + (int32_t)n * DIR_X_SCALAR[d];

                        const char v = string_matrix_2d[vy][vx * 2];
                        if(v != ' ' && v != str[n]) break;
                    }
                    if(n == len)
                    {
                        for(n = 0; n < len; n++)
                        {
                            const int32_t vy = (int32_t)y + (int32_t)n * DIR_Y_SCALAR[d];
                            const int32_t vx = (int32_t)x + (int32_t)n * DIR_X_SCALAR[d];

                            string_matrix_2d[vy][vx * 2] = str[n];
                        }
                        return true;
                    }
                }
            }
        }
    }

    printf("Failed to insert string: \"%s\" (could not find compatible position)\n", str);
    return false;
}

void fill_random(uint32_t dim)
{
    for(uint32_t y = 0; y < dim; y++)
    {
        for(uint32_t x = 0; x < dim; x++)
        {
            char* v = string_matrix_2d[y] + x * 2;
            if(*v == ' ') *v = 'a' + (char)(rand() % 26);
        }
    }
}

void print_word_search(uint32_t n, char** words, uint32_t dim)
{
    const uint32_t row_chars = dim * 2 - 1;
    printf("+%.*s+\n", row_chars + 2, "-----------------------------------------");
    for(uint32_t y = 0; y < dim; y++)
    {
        printf("| %.*s |\n", row_chars, string_matrix_2d[y]);
    }
    printf("+%.*s+\n", row_chars + 2, "-----------------------------------------");
    printf("Words to find:\n");
    for(uint32_t s = 0; s < n; s++)
    {
        printf("%d. %s\n", s + 1, words[s]);
    }
}


int main(int argc, char** argv)
{
    if(argc < 2) return -1;

    int32_t table_dim = atoi(argv[1]);
    if(table_dim > MATRIX_MAX_DIM) table_dim = 20;
    if(table_dim < 0) table_dim = (int32_t)get_longest_word(argc - 2, argv + 2);

    initialize(table_dim);
    for(int s = 2; s < argc; s++)
    {
        if(!try_insert_string(argv[s], table_dim))
        {
            return -2;
        }
    }
    fill_random(table_dim);

    print_word_search(argc - 2, argv + 2, table_dim);

    return 0;
}
