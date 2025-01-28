#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


struct foo
{
    int32_t i;
    float f;
    char* s;
};

void make_foo(struct foo* f_, int i, float f)
{
    f_->i = i;
    f_->f = f;
}

int main(int argc, char** argv)
{
    struct foo f;
    make_foo(&f, 42, 3.1415926f);

    printf("%d, %f\n", f.i, f.f);

    return 0;
}
