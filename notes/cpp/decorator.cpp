#include <iostream>


class Shape
{
public:
    virtual void draw() = 0;

};

class Circle : public Shape
{
public:
    void draw() override
    {
        std::cout << "O";
    }
};

class Square : public Shape
{
public:
    void draw() override
    {
        std::cout << "[]";
    }
};

class BigCircle : public Circle
{
public:
    void draw() override
    {
        std::cout << "BIG-";
        this->Circle::draw();
    }
};

class GreenCircle : public Circle
{
public:
    void draw()
    {
        std::cout << "GREEN-";
        this->Circle::draw();
    }
};

class ShapeDecorator : public Shape
{
public:
    inline ShapeDecorator(Shape* s) : s{ s } {}

    void draw() override
    {
        s->draw();
    }

protected:
    Shape* s;

};

class BigShape : public ShapeDecorator
{
public:
    inline BigShape(Shape* s) : ShapeDecorator(s) {}

    void draw() override
    {
        std::cout << "BIG-";
        this->s->draw();
    }
};

class GreenShape : public ShapeDecorator
{
public:
    inline GreenShape(Shape* s) : ShapeDecorator(s) {}

    void draw() override
    {
        std::cout << "GREEN-";
        this->s->draw();
    }
};

int main(int argc, char** argv)
{
    Circle c;
    c.draw();
    std::cout << std::endl;

    BigShape b{ &c };
    b.draw();
    std::cout << std::endl;

    GreenShape g{ &c };
    g.draw();
    std::cout << std::endl;

    GreenShape bg{ &b };
    bg.draw();
    std::cout << std::endl;

    return 0;
}
