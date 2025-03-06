#pragma once

#include <iostream>
#include <cstring>


class string2
{
    friend std::ostream& operator<<(std::ostream&, const string2&); // doesn't need but ahh it looks nicer here
    friend std::istream& operator>>(std::istream&, string2&);

public:
    string2();
    string2(const char*);
    string2(const string2&);
    string2(string2&&);
    ~string2();

    string2& operator = (const string2&);
    string2& operator = (const char*);
    string2& operator = (string2&&);

    string2& operator += (const string2&);
    string2& operator += (const char*);
    string2  operator +  (const string2&);
    string2  operator +  (const char*);

    bool operator == (const string2&);
    bool operator != (const string2&);
    bool operator >= (const string2&);
    bool operator <= (const string2&);
    bool operator >  (const string2&);
    bool operator <  (const string2&);

    inline const char* c_str() const { return this->str; }
    inline operator const char*() const { return this->str; }

    inline char operator[](size_t i) const { return this->str[i]; }
    inline char& operator[](size_t i) { return this->str[i]; }

    inline size_t length() const { return strlen(this->str); }

protected:
    char* str;

};
