#pragma once
#include <SDL3/SDL.h>

#define LOG(format, ...) SDL_Log(format, __VA_ARGS__)

enum UpdateStatus
{
    UPDATE_CONTINUE = 1,
    UPDATE_STOP,
    UPDATE_ERROR
};
