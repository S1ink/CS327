#include <stdio.h>
#include <stdint.h>


int main(int argc, char** argv)
{
    int i;
    uint64_t prod, prod2;
    uint64_t r = 1, oo = 2, Ke = 5, in = 5, n = 5, v = 3251, a = 3371, p = 51287, G = 52027;

    // prod = Ke * v * in * (G * n * a * p * oo + r);

    printf("%#lx\n", r);
    printf("%#lx\n", oo);
    printf("%#lx\n", Ke);
    printf("%#lx\n", in);
    printf("%#lx\n", n);
    printf("%#lx\n", v);
    printf("%#lx\n", a);
    printf("%#lx\n", p);
    printf("%#lx\n-------------\n", G);

    prod = Ke * v;
    printf("%#lx\n", prod);
    prod *= in;
    printf("%#lx\n", prod);

    prod2 = G * n;
    printf("%#lx\n", prod2);
    prod2 *= a;
    printf("%#lx\n", prod2);
    prod2 *= p;
    printf("%#lx\n", prod2);
    prod2 *= oo;
    printf("%#lx\n", prod2);
    prod2 += r;
    printf("%#lx\n", prod2);

    prod *= prod2;
    printf("%#lx\n", prod);

    for(size_t i = 0; i < 8; i++)
    {
        printf("%#x\t", (int)((char*)&prod)[i]);
    }
    putchar('\n');
}