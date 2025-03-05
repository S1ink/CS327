#include <stdio.h>
#include <stdlib.h>


int main(int argc, char** argv)
{
    int i;
    int *j;
    int *a[10];

    for(i = 0; i < 10; i++)
    {
        j = malloc(sizeof(int));
        *j = i;
        a[i] = j;
    }
    for(i = 0; i < 10; i++)
    {
        printf("%p\n", a[i]);
        free(a[i]);
    }
    return 0;
}
