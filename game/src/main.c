#include "perlin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define PERLIN_SCALE 3


double rand_double()
{
    return (double)rand() / RAND_MAX;
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    perlin_init();

    static const char* brightness = ".-:=*?#$%%&@";

    const double
        rand_x = rand_double() * 0xFF,
        rand_y = rand_double() * 0xFF;

    char block[80];
    for(uint32_t y = 0; y < 21; y++)
    {
        for(uint32_t x = 0; x < 80; x++)
        {
            double p = perlin2d((double)(x) * PERLIN_SCALE / 80. + rand_x, (double)(y) * PERLIN_SCALE / 21. + rand_y);
            block[x] = brightness[(uint32_t)((p + 1.) * 0.5 * 12.)];
        }

        printf("%.80s\n", block);
    }

    return 0;
}