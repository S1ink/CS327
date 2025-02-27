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

// VAARG MACROS
#define LOG(...) printf(__VA_ARGS__);

// in GCC:
int b = 0b10101010101000101010111010101110U;
int x = 0xAAA2AEAEU;

/*  Hexidecimal
 --> base 16!!!
 --> binary for people
 ----------------------------------
    base 10     base 16     base 2
    0           0x0         0b0000
    1           0x1         0b0001
    2           0x2         0b0010
    3           0x3         0b0011
    4           0x4         0b0100
    5           0x5         0b0101
    6           0x6         0b0110
    7           0x7         0b0111
    8           0x8         0b1000
    9           0x9         0b1001
    10          0xA         0b1010
    11          0xB         0b1011
    12          0xC         0b1100
    13          0xD         0b1101
    14          0xE         0b1110
    15          0xF         0b1111

 27 dec --> 27 = 16 + 8 + 2 + 1 --> 0b11011 --> 0x1B
*/

/* Bitwise ops
 1. AND (&):        ~bit-checking
    --> 0b10110
      & 0b00001
     -----------
        0b00001

 2. OR (|):         ~combining
    --> 0b10110
      | 0b00001
     -----------
        0b10111
 3. XOR (^):        ~toggling
    --> 0b10110
      | 0b00101
     -----------
        0b10011
 4. NOT (~):        ~inversion
    --> ~0b10110 = 0b01001
*/

#define MONSTAT_SMART       0x01
#define MONSTAT_TUNNELING   0x02
#define MONSTAT_ERRATIC     0x04
#define MONSTAT_TELEPATHIC  0x08
#define MONSTAT_MAGIC       0x10
#define MONSTAT_FIRE        0x20

struct monster
{
    int stats;
}
m;

#define MON_HAS_ATTRIB(m, atrib) (m->stats & MONSTAT_##atrib)


int main(int argc, char** argv)
{
    printf("%d\n", VALUE_MACRO);

    if(m.stats & MONSTAT_TELEPATHIC)
    {
        // check if bit is on
    }
    if(m.stats & (MONSTAT_TELEPATHIC | MONSTAT_FIRE))
    {
        // check if any of (tele/fire) are on
    }


    // MAX(func_with_side_effects(), very_expensive_function());
    // MIN(func_with_side_effects(), very_expensive_function());

    STRINGIFY(foo);

    bsearch(key, table, n_elems, sizeof(struct lookup_entry), comp)->func(0);

    CONCAT(bar_, 8)

    LOG("error in %s\n", fname, f, g, h)

    return 0;
}