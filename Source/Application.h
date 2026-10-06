#pragma once
#include "Globals.h"
#include <vector>

class Module;
class ModuleWindow;
class ModuleRender;

class Application
{
public:
    Application();
    ~Application();

    bool Init();
    UpdateStatus Update();
    bool CleanUp();

public:
    ModuleWindow* window = nullptr;
    ModuleRender* renderer = nullptr;

private:
    std::vector<Module*> modules;
};

extern Application* App;
