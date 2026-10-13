#pragma once
#include "Module.h"
#include <vector>
#include <imgui.h>

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

    void AddLog(const char* fmt, ...);
    void ClearLog();

private:
    void DrawMainMenuBar();
    void DrawConfigurationWindow();
    void DrawConsoleWindow();
    void DrawAboutWindow();

    bool showConfigWindow = true;
    bool showAboutWindow = false;
    bool showConsoleWindow = true;
    bool showHierarchyWindow = true;
    bool showInspectorWindow = true;

    // Configuration / Performance stats
    std::vector<float> fpsLog;
    std::vector<float> msLog;

    // Console
    ImGuiTextBuffer logBuffer;
    ImVector<int> logLineOffsets;
    ImGuiTextFilter logFilter;
    bool logAutoScroll = true;
};
