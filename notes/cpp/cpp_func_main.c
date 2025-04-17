#include "cpp_func.hpp"


int main(int argc, char** argv)
{
    print("你们怎么样!\n");

    OSTREAM* o = get_cout();
    print_with_cout(o, "在见！\n");

    return 0;
}
