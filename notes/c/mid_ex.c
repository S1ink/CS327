#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


static inline void bonus()
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

#define TEST_NULL_MALLOC 0
char* strdup_(const char* s)
{
    char* e;
    for(e = s; *e; e++) {}
#if TEST_NULL_MALLOC
    char* m = NULL;
#else
    char* m = malloc((e - s) + 1);
#endif
    if(m)
    {
        for(int i = 0; (m[i] = s[i]); i++) {}
    }
    return m;
}

char* strcat_(char* dst, const char* src)
{
    char* c;
    for(c = dst; *c; c++) {}
    for(int i = 0; (c[i] = src[i]); i++) {}
    return dst;
}


int main(int argc, char** argv)
{
    printf("------------- strdup() #1 ---\n");
    {
        const char* x = "hi mom";
        char* y = strdup_(x);
        printf("\"%s\"\n\"%s\"\n", x, y);
        free(y);
    }
    printf("------------- strdup() #2 ---\n");
    {
        const char* x = "";
        char* y = strdup_(x);
        printf("\"%s\"\n\"%s\"\n", x, y);
        free(y);
    }
    printf("------------- strcat() #1 ---\n");
    {
        char buff[64] = "hi mom";
        printf("\"%s\"\n", buff);
        printf("\"%s\"\n", strcat_(buff, " how are you?"));
        printf("\"%s\"\n", strcat_(buff, " i'm pretty good ig."));
    }
}
