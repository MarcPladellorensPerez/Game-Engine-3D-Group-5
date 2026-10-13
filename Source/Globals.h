#pragma once
#include <SDL3/SDL.h>

void Log(const char* format, ...);

#define LOG(format, ...) Log(format, ##__VA_ARGS__)

enum UpdateStatus
{
    UPDATE_CONTINUE = 1,
    UPDATE_STOP,
    UPDATE_ERROR
};
