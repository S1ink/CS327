#include <stdio.h>
#include <math.h>
#include <endian.h>
#include <stdint.h>


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
    int mode;

    struct
    {
        int i;
        int j;
        double d;
    }
    in, out;
    int32_t i32_temp;
    int64_t i64_temp;

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
            break;
        }
        case OP_READ_BINARY:
        {
            f = fopen("binaryfile", "rb");
            // fread(&in, sizeof(in), 1, f); // should return 1 and SHOULD CHECK IT!
            fread(&i32_temp, 4, 1, f);
            in.i = be32toh(i32_temp);
            fread(&i32_temp, 4, 1, f);
            in.j = be32toh(i32_temp);
            fread(&i64_temp, 8, 1, f);
            i64_temp = be64toh(i64_temp);
            in.d = *(double*)(&i64_temp);

            printf("%d\n%d\n%lf\n", in.i, in.j, in.d);
            break;
        }
        case OP_WRITE_TEXT:
        {
            f = fopen("textfile", "w");   // can fail, and should check, but we aren't so whatever
            fprintf(f, "%d\n%d\n%lf\n", out.i, out.j, out.d);
            break;
        }
        case OP_WRITE_BINARY:
        {
            f = fopen("binaryfile", "wb");  // don't need 'b' unless on windows

            i32_temp = htobe32(out.i);
            fwrite(&i32_temp, sizeof(i32_temp), 1, f);

            i32_temp = htobe32(out.j);
            fwrite(&i32_temp, sizeof(i32_temp), 1, f);

            i64_temp = htobe64(*(int64_t*)(&out.d));
            fwrite(&i64_temp, sizeof(i64_temp), 1, f);
            break;
        }
        default: break;
    }

    fclose(f);

    return 0;
}

/*
LITTLE ENDIAN: "little-est" byte first
BIG ENDIAN:  biggest byte first

"think of each byte as a digit" -- ordering the the digits (bytes) -- NOT BIT ORDER!!!

htobeN --> "host to big endian for N bits" -- convert from host to big endian
beNtoh --> "big endian N bits to host" -- convert from big endian to host
*/
