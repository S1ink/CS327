#pragma once

#include <fstream>

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#ifndef ENABLE_DEBUG_PRINTS
#define ENABLE_DEBUG_PRINTS 0
#endif

#if ENABLE_DEBUG_PRINTS && 0
    #define PRINT_DEBUG(...) fprintf(stderr, __VA_ARGS__); fflush(stdout);
    #define IF_DEBUG(x) x
#else
    #define PRINT_DEBUG(...)
    #define IF_DEBUG(...)
#endif


class FileDebug
{
public:
    static std::ofstream& get() { return of; }

protected:
    inline static std::ofstream of{ "debug.txt" };

};


static inline uint32_t us_seed()
{
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xFFFFFFFFU;
}
static inline uint64_t us_time()
{
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)tv.tv_usec;
}
