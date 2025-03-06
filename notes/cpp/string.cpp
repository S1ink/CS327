#include "string.h"

#include <stdlib.h>


string2::string2()
{
    this->str = static_cast<char*>(calloc(1, 1));
}

string2::string2(const char* s)
{
    this->str = strdup(s);
}

string2::string2(const string2& s)
{
    this->str = strdup(s.str);
}

string2::~string2()
{
    free(this->str);
}


string2& string2::operator = (const string2& s)
{
    free(this->str);
    this->str = strdup(s.str);

    return *this;
}
string2& string2::operator = (const char* str)
{
    free(this->str);
    this->str = strdup(str);

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
    if(!x) {} // error!

    this->str = x;
    strcpy(this->str + l, s.str);

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
string2 string2::operator + (const string2& s)
{
    // return string2{ *this } += s;

    string2 x{};
    free(x.str);

    x.str = static_cast<char*>(malloc(this->length() + s.length() + 1));
    strcpy(stpcpy(x.str, this->str), s.str);

    return x;
}
string2 string2::operator + (const char* str)
{
    // return string2{ *this } += str;

    string2 x{};
    free(x.str);

    x.str = static_cast<char*>(malloc(this->length() + strlen(str) + 1));
    strcpy(stpcpy(x.str, this->str), str);

    return x;
}

bool string2::operator == (const string2& s)
{
    return static_cast<bool>(!strcmp(this->str, s.str));
}
bool string2::operator != (const string2& s)
{
    return static_cast<bool>(strcmp(this->str, s.str));
}
bool string2::operator >= (const string2& s)
{
    return strcmp(this->str, s.str) >= 0;
}
bool string2::operator <= (const string2& s)
{
    return strcmp(this->str, s.str) <= 0;
}
bool string2::operator >  (const string2& s)
{
    return strcmp(this->str, s.str) > 0;
}
bool string2::operator <  (const string2& s)
{
    return strcmp(this->str, s.str) < 0;
}


std::ostream& operator<<(std::ostream& o, const string2& s)
{
    return (o << s.str);
}

std::istream& operator>>(std::istream& i, string2& s)
{
    return (i >> s.str);
}