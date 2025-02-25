#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


// "VAriatic ARGumentS"
void printf2(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);   // pass in the last known parameters

    while(*fmt)
    {
        switch(*fmt)
        {
            case 'd' :
            {
                printf("%d\t", va_arg(ap, int));
                break;
            }
            case 'f' :
            {
                printf("%f\t", va_arg(ap, double));
                break;
            }
            case 'c' :
            {
                printf("%c\t", va_arg(ap, int));
                break;
            }
            case 's' :
            {
                printf("'%s'\t", va_arg(ap, char*));
                break;
            }
        }
        fmt++;
    }
    printf("\n");

    va_end(ap);
}

int main(int argc, char** argv)
{
    printf2("dscf", 2180, "hello world", 't', 0.9f);

    return 0;
}
