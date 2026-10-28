#pragma once
#include "Globals.h"
#include <vector>

class Module;
class ModuleWindow;
class ModuleCamera;
class ModuleRender;
class ModuleEditor;

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
    ModuleCamera* camera = nullptr;
    ModuleRender* renderer = nullptr;
    ModuleEditor* editor = nullptr;

private:
    std::vector<Module*> modules;
};

extern Application* App;
