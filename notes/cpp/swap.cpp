#include <iostream>


template<typename T>
void swap_c(T* a, T* b)
{
    T tmp = *a;
    *a = *b;
    *b = tmp;
}
template<typename T>
void swap_cpp(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

int main(int argc, char** argv)
{
    using namespace std;

    int i, j;

    i = 0; j = 1;

    std::cout << i << ' ' << j << std::endl;
    swap_c(&i, &j);
    std::cout << i << ' ' << j << std::endl;
    swap_cpp(i, j);
    std::cout << i << ' ' << j << std::endl;

    return 0;
}
