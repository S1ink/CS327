#include "string.h"

#include <stdlib.h>
#include <exception>    // std::exception exports abstract method const char* what() (must implement)


// exceptions "unwind" when uncaught --> runtime catches all unhandled exceptions when they reach main()
// *anything* can be used as an exception
// C --> __FUNCTION__ to get function name
// C++ -> __FUNCTION__ can be ambiguous so need to use __PRETTY_FUNCTION__ (entire function signature)

string2::string2()
{
    this->str = static_cast<char*>(calloc(1, 1));
}

string2::string2(const char* s)
{
    if( !(this->str = strdup(s)) )
    {
        throw __PRETTY_FUNCTION__;
    }
}

string2::string2(const string2& s)
{
    if( !(this->str = strdup(s.str)) )
    {
        throw __PRETTY_FUNCTION__;
    }
}

string2::string2(string2&& s)
{
    this->str = s.str;
    if( !(s.str = static_cast<char*>(calloc(1, 1))) )
    {
        throw __PRETTY_FUNCTION__;
    }
}

string2::~string2()
{
    free(this->str);
}


string2& string2::operator = (const string2& s)
{
    free(this->str);
    if( !(this->str = strdup(s.str)) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return *this;
}
string2& string2::operator = (const char* str)
{
    free(this->str);
    if( !(this->str = strdup(str)) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return *this;
}
string2& string2::operator = (string2&& s)
{
    std::swap(this->str, s.str);

    return *this;
}

string2& string2::operator += (const string2& s)
{
    const size_t l = this->length();

    char* x = static_cast<char*>(realloc(this->str, l + s.length() + 1));
    if(!x)
    {
        throw __PRETTY_FUNCTION__;
    }

    this->str = x;
    if( !(strcpy(this->str + l, s.str)) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return *this;
}
string2& string2::operator += (const char* str)
{
    const size_t l = this->length();

    char* x = static_cast<char*>(realloc(this->str, l + strlen(str) + 1));
    if(!x) {} // error!

    this->str = x;
    strcpy(this->str + l, str);

    return *this;
}
string2 string2::operator + (const string2& s) const
{
    // return string2{ *this } += s;

    string2 x{};
    free(x.str);

    if( !(x.str = static_cast<char*>(malloc(this->length() + s.length() + 1))) )
    {
        throw __PRETTY_FUNCTION__;
    }
    if( !(strcpy(stpcpy(x.str, this->str), s.str)) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return x;
}
string2 string2::operator + (const char* str) const
{
    // return string2{ *this } += str;

    string2 x{};
    free(x.str);

    if( !(x.str = static_cast<char*>(malloc(this->length() + strlen(str) + 1))) )
    {
        throw __PRETTY_FUNCTION__;
    }
    if( !(strcpy(stpcpy(x.str, this->str), str)) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return x;
}

bool string2::operator == (const string2& s) const
{
    return static_cast<bool>(!strcmp(this->str, s.str));
}
bool string2::operator != (const string2& s) const
{
    return static_cast<bool>(strcmp(this->str, s.str));
}
bool string2::operator >= (const string2& s) const
{
    return strcmp(this->str, s.str) >= 0;
}
bool string2::operator <= (const string2& s) const
{
    return strcmp(this->str, s.str) <= 0;
}
bool string2::operator >  (const string2& s) const
{
    return strcmp(this->str, s.str) > 0;
}
bool string2::operator <  (const string2& s) const
{
    return strcmp(this->str, s.str) < 0;
}


std::ostream& operator<<(std::ostream& o, const string2& s)
{
    return (o << s.str);
}

std::istream& operator>>(std::istream& i, string2& s)
{
    free(s.str);
    if( !(s.str = static_cast<char*>(malloc(80))) )
    {
        throw __PRETTY_FUNCTION__;
    }

    return i.getline(s.str, s.length(), '\n');
}





#ifndef STRING2_MAIN
#define STRING2_MAIN 0
#endif

#if STRING2_MAIN
int main(int argc, char** argv)
{
    string2 x = { "Hello World" };

    std::cout << x << std::endl;

    // x = "foo";

    x[6] = 'E';
    x[7] = 'a';
    x[8] = 'r';
    x[9] = 'l';
    x[10] = '!';

    std::cout << x << std::endl;

    try
    {
        // throw 1;
        std::cout << x[11] << std::endl;
    }
    catch(char* e)
    {
        std::cout << "took the char* option" << std::endl;
    }
    catch(int e)
    {
        std::cout << e << std::endl;
    }
    catch(const char* e)
    {
        std::cout << "Failed to access index 11 in function " << e << std::endl;
    }
    catch(...)
    {
        std::cout << "Foo" << std::endl;
    }

    return 0;
}
#endif
