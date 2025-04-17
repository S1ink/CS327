#include <iostream>
#include <functional>


typedef void*(*fptr)();

fptr f_()
{
    std::cout << "F CALL!!!" << std::endl;

    return (fptr)f_;
}

class F
{
public:
    F& operator()()
    {
        std::cout << "PARENTHESIS!!!" << std::endl;

        return *this;
    }
};

int main(int argc, char** argv)
{
    F a;
    a()()()()()()()()()();

    return 0;
}
