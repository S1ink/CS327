#include <iostream>


int count_calls()
{
    static int num_calls = 0;   // not actually on the stack --> in the "data" segment

    return ++num_calls;
}

int* return_local_address()
{
    static int i;               // scope vs lifetime --> visibility vs "does the data still exist"

    return &i;
}

static void static_func()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void call_static()
{
    static_func();
}