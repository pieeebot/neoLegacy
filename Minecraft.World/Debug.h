#pragma once
#include <csignal>

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define DEBUG_BREAK() __builtin_trap()
#elif defined(SIGTRAP)
    #define DEBUG_BREAK() std::raise(SIGTRAP)
#else
    #define DEBUG_BREAK() ((void)0)
#endif