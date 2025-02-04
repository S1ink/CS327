#include <stdio.h>
#include <math.h>


typedef enum
{
    OP_READ_TEXT = 0b00,
    OP_READ_BINARY = 0b10,
    OP_WRITE_TEXT = 0b01,
    OP_WRITE_BINARY = 0b11,
    OP_INVALID = 0b100
}
Operation;

void usage(const char* s)
{
    fprintf(stderr, "%s [-rt|-rb|-wt-|wb]\n", s);
}

// ./fileio -"OP"
int main(int argc, char** argv)
{
    int mode = 0;

    struct
    {
        int i;
        int j;
        double d;
    }
    in, out;

    out.i = 1;
    out.j = 65537;
    out.d = M_PI;

    if(argc > 1 && argv[1][0] == '-')
    {
        switch(argv[1][1])
        {
            case 'r': break;
            case 'w': mode |= 0b01; break;
            default: mode |= OP_INVALID;
        }
        switch(argv[1][2])
        {
            case 't' : break;
            case 'b' : mode |= 0b10; break;
            default: mode |= OP_INVALID;
        }
    }

    if(mode & OP_INVALID)
    {
        usage(argv[0]);
        return -1;
    }

    FILE* f;

    switch(mode)
    {
        case OP_READ_TEXT:
        {
            f = fopen("textfile", "r");
            fscanf(f, "%d\n%d\n%lf\n", &in.i, &in.j, &in.d);
            printf("%d\n%d\n%lf\n", in.i, in.j, in.d);
        }
        case OP_READ_BINARY: break;
        case OP_WRITE_TEXT:
        {
            f = fopen("textfile", "w");   // can fail, and should check, but we aren't so whatever
            fprintf(f, "%d\n%d\n%lf\n", out.i, out.j, out.d);
            break;
        }
        case OP_WRITE_BINARY: break;
        default: break;
    }

    fclose(f);

    return 0;
}
