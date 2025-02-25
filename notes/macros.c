#include <stdio.h>

// VALUE-type MACROS
#define VALUE_MACRO 100

// FUNCTION-type MACROS
#define MIN(x, y) (x < y ? x : y)   // have to be mindful of invoking functions as paramters as this will call them twice!
// "basic blocks" have a value, which is the value of their last line!? - however, they are not expressions. The trick is to wrap BB in parenthesis, which transforms it into an expression
#define MAX(x, y) \
({                          \
    typeof(x) _x = (x);     /* the function is not called in the use of typeof() */ \
    typeof(y) _y = (y);     \
    (_x < _y ? _x : _y);    \
})

// MACRO OPS
#define STRINGIFY(x) #x
#define CONCAT(x, y) x##y

int cmd_bar(int);
int cmd_baz(int);
int cmd_buz(int);
int cmd_fiz(int);
int cmd_foo(int);
int cmd_zip(int);

typedef int (*lu_func)(int);
struct lookup_entry
{
    const char* name;
    lu_func func;
};
#define LU_VALUE(n) { STRINGIFY(n), CONCAT(cmd_, n) }
struct lookup_entry table[] =
{
    LU_VALUE(bar),
    LU_VALUE(baz),
    LU_VALUE(buz),
    LU_VALUE(fiz),
    LU_VALUE(foo),
    LU_VALUE(zip)
};

int main(int argc, char** argv)
{
    printf("%d\n", VALUE_MACRO);

    // MAX(func_with_side_effects(), very_expensive_function());
    // MIN(func_with_side_effects(), very_expensive_function());

    STRINGIFY(foo);

    bsearch(key, table, n_elems, sizeof(struct lookup_entry), comp)->func(0);

    CONCAT(bar_, 8)

    return 0;
}