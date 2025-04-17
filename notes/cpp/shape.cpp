#include <iostream>
#include <cmath>
#include <vector>


class Shape
{
public:
    virtual ~Shape() {}
    virtual double area() = 0;
    virtual double perimeter() = 0;
    virtual void print() = 0;
};

class Square : public Shape
{
public:
    Square() = default;

    virtual double area() override { return 1.; }
    virtual double perimeter() override { return 4; }
    virtual void print() override {}
};

class Circle : public Shape
{
public:
    Circle() = default;

    virtual double area() override { return 3.14; }
    virtual double perimeter() override { return 6.28; }
    virtual void print() override {}

    double circumfrance() { return 0.; }
};

int main(int argc, char** argv)
{
    Circle x;
    Square y;

    Shape* a = &x;
    Shape* b = &y;

    Circle* c;
    if((c = dynamic_cast<Circle*>(a)))
    {
        std::cout << "CIRCLE! (a)" << std::endl;
    }
    if(!(c = dynamic_cast<Circle*>(a)))
    {
        std::cout << "NOT A CIRCLE! (b)" << std::endl;
    }
}
