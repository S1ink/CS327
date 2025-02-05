#pragma once

#include <stdio.h>

#ifndef ENABLE_DEBUG_PRINTS
#define ENABLE_DEBUG_PRINTS 0
#endif

#if ENABLE_DEBUG_PRINTS
    #define PRINT_DEBUG(...) printf(__VA_ARGS__);
#else
    #define PRINT_DEBUG(...)
#endif
