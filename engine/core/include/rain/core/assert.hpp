#pragma once

#include<cstdlib>

#include<rain/core/log.hpp>

#define rain_assert(expr)                                            \
    do                                                              \
    {                                                               \
        if (!(expr))                                                 \
        {                                                           \
            ::rain::log_error("assert failed: " #expr);             \
            std::abort();                                           \
        }                                                           \
    }while(false) 
