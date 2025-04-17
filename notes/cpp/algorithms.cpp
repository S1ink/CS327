#include <iostream>
#include <algorithm>
#include <vector>


template<typename T>
void print(T x)
{
    std::cout << x << std::endl;
}

void inplace_double(size_t& x)
{
    x <<= 1;
}

bool is_prime(size_t x)
{
    if(x < 2 || x == 4) return false;   // 这是什么!? (非常糟糕！)

    for(size_t j = 2; j < x / 2; j++)
    {
        if(!(x % j)) return false;
    }

    return true;
}

bool is17(size_t i)
{
    return i == 17;
}

template<size_t I>
class value_predicate
{
public:
    bool operator()(size_t x) { return x == I; }
};

int main(int argc, char** argv)
{
    std::vector<size_t> v;

    v.reserve(100);
    for(size_t i = 0; i < 100; i++)
    {
        v.emplace_back(i);
    }

    // std::for_each(v.begin(), v.end(), print<size_t>);
    // std::for_each(v.begin(), v.end(), inplace_double);
    // std::for_each(v.begin(), v.end(), print<size_t>);

    for(auto vi = v.begin(); vi != v.end(); vi++)
    {
        vi = std::find_if(vi, v.end(), is_prime);
        if(vi == v.end()) break;
        std::cout << *vi << std::endl;
    }

    std::cout << std::count_if(v.begin(), v.end(), is_prime) << std::endl;

    std::cout << std::count_if(v.begin(), v.end(), value_predicate<16>{}) << std::endl;
    std::cout << std::count_if(v.begin(), v.end(), value_predicate<18>{}) << std::endl;
    std::cout << std::count_if(v.begin(), v.end(), value_predicate<125>{}) << std::endl;
}
