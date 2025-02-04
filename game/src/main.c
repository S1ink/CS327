#include "dungeon.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>


int main(int argc, char** argv)
{
    Dungeon d;
    generate_dungeon(&d, time(NULL));

    for(uint32_t y = 0; y < 21; y++)
    {
        printf("%.80s\n", d.printable[y]);
    }

    destruct_dungeon(&d);

    return 0;
}