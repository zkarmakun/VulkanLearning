#pragma once
#include <iostream>
#include <stdio.h>
#include <SDL2/SDL_log.h>

#define check(condition) \
    do { \
    if (!(condition)) { \
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Check failed"); \
    std::terminate(); \
    } \
    } while (false)

#define checkf(condition, text) \
    do { \
    if (!(condition)) { \
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, text); \
    std::terminate(); \
    } \
    } while (false)
