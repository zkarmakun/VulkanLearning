#pragma once
#include <iostream>
#include <SDL2/SDL_log.h>

#define LOG_Info(...) \
SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

#define LOG_Warning(...) \
SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

#define LOG_Error(...) \
SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)