#include <iostream>


class Singleton
{
public:
    static Singleton& get()
    {
        if(!inst)
        {
            inst = new Singleton();
        }
        return *inst;
    }

    void print()
    {
        std::cout << "YOU HAVE REACHED THE SINGLETON INSTANCE!" << std::endl;
    }

private:
    Singleton() = default;
    Singleton(const Singleton&) = delete;

    Singleton& operator=(const Singleton&) = delete;

private:
    static inline Singleton* inst{ nullptr };

};

int main(int argc, char** argcv)
{
    Singleton& s = Singleton::get();

    s.print();

    std::cout << &s << std::endl;

    return 0;
}
