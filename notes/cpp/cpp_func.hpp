#pragma once

#ifdef __cplusplus
#include <iostream>
#define OSTREAM std::ostream

extern "C" {
#else
#define OSTREAM void
#endif

void print(const char* s);

OSTREAM* get_cout();
void print_with_cout(OSTREAM*, const char*);

#ifdef __cplusplus
}
#endif
