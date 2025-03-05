#include <stdio.h>

int main(int argc, char** argv)
{
    const int i = 0;

    printf("%d\n", i);

    int* b = &i;
    *(int*)b = 1; // ALL L-VALUES MUST HAVE AN ADDRESS

    printf("%d\n", i);

    return 0;
}