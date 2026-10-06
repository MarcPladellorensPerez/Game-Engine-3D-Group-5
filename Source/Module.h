#pragma once
#include "Globals.h"

class Application;

class Module
{
public:
    Module() {}
    virtual ~Module() {}

    virtual bool Init() { return true; }
    virtual UpdateStatus PreUpdate() { return UPDATE_CONTINUE; }
    virtual UpdateStatus Update() { return UPDATE_CONTINUE; }
    virtual UpdateStatus PostUpdate() { return UPDATE_CONTINUE; }
    virtual bool CleanUp() { return true; }
};
