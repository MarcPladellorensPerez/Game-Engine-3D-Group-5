#pragma once
#include "Module.h"
#include <vector>
#include <string>
#include <memory>
#include <imgui.h>

class EditorCommand;
class GameObject;

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

    void ExecuteCommand(EditorCommand* cmd, bool execute = true);
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    std::string GetUndoCommandName() const;
    std::string GetRedoCommandName() const;
    void ClearHistory();

    void DeleteSelectedGameObject();
    void DuplicateSelectedGameObject();
    void CopySelectedGameObject();
    void PasteGameObject();
    void CutSelectedGameObject();
    GameObject* CreatePrimitive(const std::string& type, GameObject* parent = nullptr);
    GameObject* CreateEmptyGameObject(GameObject* parent = nullptr);

    // Drag & Drop / Model Import
    enum class DragDropAction { Ask, Replace, Add };
    DragDropAction dragDropAction = DragDropAction::Ask;

    void PromptLoadModel(const std::string& path);
    void LoadModelDirect(const std::string& path);
    void ClearSceneAndLoadModel(const std::string& path);
    void ClearScene();

private:
    void HandleShortcuts();
    void ProcessPendingActions();

    void DrawMainMenuBar();
    void DrawConfigurationWindow();
    void DrawConsoleWindow();
    void DrawHierarchyWindow();
    void DrawInspectorWindow();
    void DrawAboutWindow();
    void DrawImportModelModal();

    bool showConfigWindow = true;
    bool showAboutWindow = false;
    bool showConsoleWindow = true;
    bool showHierarchyWindow = true;
    bool showInspectorWindow = true;

    // Drag & Drop Modal state
    std::string pendingDroppedModelPath = "";
    bool showImportModelModal = false;

    // Hierarchy & Inspector
    void DrawGameObjectNode(GameObject* go);

    std::vector<std::unique_ptr<EditorCommand>> undoStack;
    std::vector<std::unique_ptr<EditorCommand>> redoStack;
    size_t maxHistorySize = 50;

    GameObject* clipboardGameObject = nullptr;
    GameObject* pendingDeleteTarget = nullptr;
    GameObject* pendingDuplicateTarget = nullptr;
    bool pendingPaste = false;
    GameObject* pendingPasteTarget = nullptr;

    // Configuration / Performance stats
    std::vector<float> fpsLog;
    std::vector<float> msLog;

    // Console
    ImGuiTextBuffer logBuffer;
    ImVector<int> logLineOffsets;
    ImGuiTextFilter logFilter;
    bool logAutoScroll = true;
};
