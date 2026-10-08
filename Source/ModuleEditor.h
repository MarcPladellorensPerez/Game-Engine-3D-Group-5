#pragma once
#include "Module.h"
#include <vector>

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    virtual ~ModuleEditor();

    bool Init() override;
    UpdateStatus PreUpdate() override;
    UpdateStatus Update() override;
    UpdateStatus PostUpdate() override;
    bool CleanUp() override;

private:
    void DrawMainMenuBar();
    void DrawConfigurationWindow();
    void DrawAboutWindow();

    bool showConfigWindow = true;
    bool showAboutWindow = false;
    bool showConsoleWindow = true;
    bool showHierarchyWindow = true;
    bool showInspectorWindow = true;
    
    std::vector<float> fpsLog;
};
