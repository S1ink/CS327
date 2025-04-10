#include "cpp_func.hpp"

#include <iostream>


void print(const char* s)
{
    std::cout << s;
}

OSTREAM* get_cout()
{
    return &std::cout;
}

void print_with_cout(OSTREAM* o, const char* s)
{
    *o << s;
}