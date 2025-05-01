#include <stdio.h>
#include <stdint.h>

#define Y_SIZE 1024
#define X_SIZE 1024

enum
{
    R = 0,
    G = 1,
    B = 2
};

typedef uint8_t IMAGE_T[Y_SIZE][X_SIZE][3];

void write_ppm(char* fname, IMAGE_T image)
{
    FILE* f = fopen(fname, "w");

    fprintf(f, "P6\n%d\n%d\n255\n", X_SIZE, Y_SIZE);

    fwrite(image, sizeof(uint8_t), Y_SIZE * X_SIZE * 3, f);

    fclose(f);
}

void read_ppm(char* fname, IMAGE_T image)
{
    
}

int main(int argc, char** argv)
{
    int x, y;
    IMAGE_T img;

    for(y = 0; y < Y_SIZE; y++)
    {
        for(x = 0; x < X_SIZE; x++)
        {
            img[y][x][R] = (y >> 2);
            img[y][x][G] = 255 - (y >> 2);
            img[y][x][B] = (x >> 2);
        }
    }

    write_ppm("./image.ppm", img);

    return 0;
}
