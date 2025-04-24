#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>


int main(int argc, char** argv)
{
    uint32_t val;
    int32_t count;

    val = atoi(argv[1]);

    count = 0;
    while(val != 1)
    {
        if(val & 1)
        {
            if(val >= UINT_MAX / 3)
            {
                fprintf(stderr, "OVERFLOW\n");
                return -1;
            }
            val = val * 3 + 1;
        }
        else
        {
            val = val >> 1;
        }
        count++;
    }

    printf("Iterated %d times\n", count);
    return 0;
}
