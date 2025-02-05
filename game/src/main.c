#include "dungeon.h"

#include <time.h>


int main(int argc, char** argv)
{
    Dungeon d;
    generate_dungeon(&d, time(NULL));
    print_dungeon(&d, 1);
    destruct_dungeon(&d);

    return 0;
}