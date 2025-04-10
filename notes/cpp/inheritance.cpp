#include <iostream>


#define PRINT_METHOD_NAME std::cout << __PRETTY_FUNCTION__ << std::endl;

class A
{
public:
    inline A() { PRINT_METHOD_NAME };
    inline A(int a) : a{ a } { PRINT_METHOD_NAME }
    inline A(const A& a) : a{ a.a } { PRINT_METHOD_NAME }
    inline virtual ~A() { PRINT_METHOD_NAME }

public:
    inline void print()
    {
        std::cout << "A(" << this->a << ")" << std::endl;
        PRINT_METHOD_NAME
    }

protected:
    int a{ 0 };

};

class B : virtual public A
{
public:
    inline B() { PRINT_METHOD_NAME };
    inline B(int b) : A(b), b{ b } { PRINT_METHOD_NAME }
    inline B(const B& b) : B{ b.b } { PRINT_METHOD_NAME }
    inline virtual ~B() { PRINT_METHOD_NAME }

public:
    inline void print()
    {
        std::cout << "B(" << this->b << ")" << std::endl;
        PRINT_METHOD_NAME
    }

protected:
    int b{ 0 };
};

class C : virtual public A
{
public:
    inline C() { PRINT_METHOD_NAME };
    inline C(int c) : A(c), c{ c } { PRINT_METHOD_NAME }
    inline C(const C& c) : C{ c.c } { PRINT_METHOD_NAME }
    inline virtual ~C() { PRINT_METHOD_NAME }

public:
    inline void print()
    {
        std::cout << "C(" << this->c << ")" << std::endl;
        PRINT_METHOD_NAME
    }

protected:
    int c{ 0 };
};

class D : virtual public B, virtual public C
{
    public:
    inline D() { PRINT_METHOD_NAME };
    inline D(int d) : B(d), C(d), d{ d } { PRINT_METHOD_NAME }
    inline D(const D& d) : D{ d.d } { PRINT_METHOD_NAME }
    inline virtual ~D() { PRINT_METHOD_NAME }

public:
    inline void print()
    {
        std::cout << "D(" << this->d << ")" << std::endl;
        PRINT_METHOD_NAME
    }

protected:
    int d{ 0 };

};


int main(int argc, char** argv)
{
    D a{ 1 };

    a.print();
    a.B::print();
    a.C::print();
    a.A::print();
    // dynamic_cast<B*>(&a)->A::print();

    return 0;
}
