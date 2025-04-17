#include <iostream>


int count_calls();
void call_static();

class InstanceCounter
{
public:
    InstanceCounter()
    {
        InstanceCounter::instances++;
    }

public:
    static size_t getNum()
    {
        return InstanceCounter::instances;
    }

private:
    inline static size_t instances{ 0 };

};

// size_t InstanceCounter::instances = 0;   // don't actually need to assign 0 since all bits in data segment are zeroed!!!


int main(int argc, char** argv)
{
    for(size_t i = 0; i < 10; i++)
    {
        std::cout << count_calls() << std::endl;
    }

    call_static();

    std::cout << InstanceCounter::getNum() << std::endl;

    InstanceCounter x;

    std::cout << InstanceCounter::getNum() << std::endl;

    InstanceCounter y, z, w;

    std::cout << InstanceCounter::getNum() << std::endl;

    return 0;
}