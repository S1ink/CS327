#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// string static allocation:
// 1. char a[] = "foo"          --> a char array (literal is copied in, so doesn't exist in ROM) -- ARRAY IS MUTABLE!
// 2. const char* p = "bar"     --> a pointer to a string literal -- the pointer can change but the literal CANNOT!

// All strings in C are null-terminated ('\0') -- automagically added by the compiler for literals
// It is an error to use string function on non-null-terminated char arrays

int strcmp_2(const char* s1, const char* s2)
{
    size_t i;
    for(i = 0; s1[i] && s1[i] == s2[i]; i++);

    return s1[i] - s2[i];
}
int strcmp_idiomatic(const char* s1, const char* s2)    // "the better version"
{
    while(*s1 && *s1 == *s2)
    {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

int strlen_2(const char* s)
{
    int32_t i;
    for(i = 0; s[i]; i++);

    return i;
}
char* strcpy_2(char* dest, const char* src)
{
    size_t i;
    for(i = 0; (dest[i] = src[i]); i++);

    return dest;

    // {
    //     int sz = strlen(src);
    //     memcpy(dest, src, sz + 1);
    //     return dest;
    // }
}

int main(int argc, char** argv)
{
    char a[] = "Hello";
    char b[] = "Goodbye";
    char* c = "Hello";

    printf("%d\t%d\n", strcmp_idiomatic(a, b), strcmp_idiomatic(a, c));
    b[0] = 'H';
    printf("%d\t%d\n", strcmp_idiomatic(a, b), strcmp_idiomatic(a, c));

    strcpy_2(b, a);
    printf("%s\n", b);
    printf("%s\n", b + strlen_2(b) + 1);

    printf("%d\t%d\n", strcmp_2(a, b), strcmp_2(b, c));
    // printf("%d\t%d\t%d\n", strlen_2(a), strlen_2(b), strlen_2(c));
}
