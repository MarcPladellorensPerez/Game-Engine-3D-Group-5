#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleCamera.h"
#include "ModuleRender.h"
#include "Model.h"
#include "GameObject.h"
#include "EditorCommand.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <IL/il.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

ModuleEditor::ModuleEditor() {}
ModuleEditor::~ModuleEditor() {}

bool ModuleEditor::Init()
{
    ClearLog();
    ClearHistory();
    LOG("Init ImGui Editor");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;

    ImGui_ImplSDL3_InitForOpenGL(App->window->GetWindow(), App->renderer->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    return true;
}

UpdateStatus ModuleEditor::PreUpdate()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    return UPDATE_CONTINUE;
}

UpdateStatus ModuleEditor::Update()
{
    // Auto-relayout when the application window is resized
    ImGuiIO& io = ImGui::GetIO();
    float curW = io.DisplaySize.x;
    float curH = io.DisplaySize.y;
    if (curW > 0.0f && curH > 0.0f && (curW != lastDisplayW || curH != lastDisplayH))
    {
        needResetLayout = true;
        lastDisplayW = curW;
        lastDisplayH = curH;
    }

    HandleShortcuts();

    DrawMainMenuBar();

    if (showConfigWindow) DrawConfigurationWindow();
    if (showConsoleWindow) DrawConsoleWindow();
    if (showHierarchyWindow) DrawHierarchyWindow();
    if (showInspectorWindow) DrawInspectorWindow();
    if (showAboutWindow) DrawAboutWindow();

    DrawImportModelModal();

    ProcessPendingActions();

    if (needResetLayout)
    {
        needResetLayout = false;
    }

    // Keep track of FPS and MS
    float currentFps = ImGui::GetIO().Framerate;
    fpsLog.push_back(currentFps);
    if (fpsLog.size() > 100) fpsLog.erase(fpsLog.begin());

    float currentMs = currentFps > 0.0f ? (1000.0f / currentFps) : 0.0f;
    msLog.push_back(currentMs);
    if (msLog.size() > 100) msLog.erase(msLog.begin());

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleEditor::PostUpdate()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(App->window->GetWindow());
    return UPDATE_CONTINUE;
}

bool ModuleEditor::CleanUp()
{
    LOG("CleanUp ImGui Editor");
    ClearHistory();
    if (clipboardGameObject)
    {
        delete clipboardGameObject;
        clipboardGameObject = nullptr;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return true;
}

void ModuleEditor::AddLog(const char* fmt, ...)
{
    int old_size = logBuffer.size();
    va_list args;
    va_start(args, fmt);
    logBuffer.appendfv(fmt, args);
    va_end(args);
    for (int new_size = logBuffer.size(); old_size < new_size; old_size++)
    {
        if (logBuffer[old_size] == '\n')
            logLineOffsets.push_back(old_size + 1);
    }
}

void ModuleEditor::ClearLog()
{
    logBuffer.clear();
    logLineOffsets.clear();
    logLineOffsets.push_back(0);
}

void ModuleEditor::ExecuteCommand(EditorCommand* cmd, bool execute)
{
    if (!cmd) return;

    if (execute)
    {
        cmd->Execute();
    }

    redoStack.clear();
    undoStack.push_back(std::unique_ptr<EditorCommand>(cmd));

    if (undoStack.size() > maxHistorySize)
    {
        undoStack.erase(undoStack.begin());
    }

    LOG("[History] %s", cmd->GetName().c_str());
}

bool ModuleEditor::Undo()
{
    if (undoStack.empty()) return false;

    auto cmd = std::move(undoStack.back());
    undoStack.pop_back();

    cmd->Undo();
    LOG("[Undo] %s", cmd->GetName().c_str());

    redoStack.push_back(std::move(cmd));
    return true;
}

bool ModuleEditor::Redo()
{
    if (redoStack.empty()) return false;

    auto cmd = std::move(redoStack.back());
    redoStack.pop_back();

    cmd->Redo();
    LOG("[Redo] %s", cmd->GetName().c_str());

    undoStack.push_back(std::move(cmd));
    return true;
}

bool ModuleEditor::CanUndo() const
{
    return !undoStack.empty();
}

bool ModuleEditor::CanRedo() const
{
    return !redoStack.empty();
}

std::string ModuleEditor::GetUndoCommandName() const
{
    if (!undoStack.empty()) return undoStack.back()->GetName();
    return "";
}

std::string ModuleEditor::GetRedoCommandName() const
{
    if (!redoStack.empty()) return redoStack.back()->GetName();
    return "";
}

void ModuleEditor::ClearHistory()
{
    undoStack.clear();
    redoStack.clear();
}

void ModuleEditor::HandleShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;

    // Undo: Ctrl + Z
    if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        Undo();
    }
    // Redo: Ctrl + Y or Ctrl + Shift + Z
    else if ((ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) ||
             (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_Z, false)))
    {
        Redo();
    }
    // Duplicate: Ctrl + D
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
    {
        DuplicateSelectedGameObject();
    }
    // Copy: Ctrl + C
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        CopySelectedGameObject();
    }
    // Paste: Ctrl + V
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
    {
        PasteGameObject();
    }
    // Cut: Ctrl + X
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
    {
        CutSelectedGameObject();
    }
    // New / Clear Scene: Ctrl + N
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
    {
        ClearScene();
    }
    // Delete: Delete key
    else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        DeleteSelectedGameObject();
    }
    // Reset Window Layout: Ctrl + Shift + R
    else if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_R, false))
    {
        ResetWindowLayout();
    }
}

void ModuleEditor::PromptLoadModel(const std::string& path)
{
    if (path.empty()) return;

    if (dragDropAction == DragDropAction::Replace)
    {
        ClearSceneAndLoadModel(path);
    }
    else if (dragDropAction == DragDropAction::Add || !App || !App->renderer || App->renderer->GetGameObjects().empty())
    {
        LoadModelDirect(path);
    }
    else
    {
        pendingDroppedModelPath = path;
        showImportModelModal = true;
    }
}

void ModuleEditor::LoadModelDirect(const std::string& path)
{
    if (!App || !App->renderer) return;

    GameObject* go = App->renderer->LoadModel(path);
    if (go)
    {
        ExecuteCommand(new CreateGameObjectCommand(go, nullptr), false);
        App->renderer->SetSelectedGameObject(go);
        if (App->camera)
        {
            App->camera->FocusOnModel();
        }
        LOG("[Scene] Added 3D Model '%s' to scene", go->GetName().c_str());
    }
}

void ModuleEditor::ClearSceneAndLoadModel(const std::string& path)
{
    if (!App || !App->renderer) return;

    std::vector<GameObject*> oldObjects = App->renderer->GetGameObjects();
    for (GameObject* go : oldObjects)
    {
        App->renderer->DetachGameObject(go);
    }

    GameObject* newObj = App->renderer->LoadModel(path);
    ExecuteCommand(new ReplaceSceneCommand(oldObjects, newObj), false);
    if (newObj)
    {
        App->renderer->SetSelectedGameObject(newObj);
        if (App->camera)
        {
            App->camera->FocusOnModel();
        }
        LOG("[Scene] Replaced scene with model '%s'", newObj->GetName().c_str());
    }
}

void ModuleEditor::ClearScene()
{
    if (!App || !App->renderer || App->renderer->GetGameObjects().empty()) return;

    std::vector<GameObject*> oldObjects = App->renderer->GetGameObjects();
    for (GameObject* go : oldObjects)
    {
        App->renderer->DetachGameObject(go);
    }
    ExecuteCommand(new ReplaceSceneCommand(oldObjects, nullptr), false);
    App->renderer->SetSelectedGameObject(nullptr);
    LOG("[Scene] Cleared scene");
}

void ModuleEditor::DrawImportModelModal()
{
    if (showImportModelModal)
    {
        ImGui::OpenPopup("Import 3D Model##Modal");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Import 3D Model##Modal", &showImportModelModal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        size_t lastSlash = pendingDroppedModelPath.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? pendingDroppedModelPath.substr(lastSlash + 1) : pendingDroppedModelPath;

        ImGui::Text("Model: %s", filename.c_str());
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("A 3D model was dragged into the engine.\nHow would you like to load it into the scene?\n\n");

        if (ImGui::Button("Replace Scene (Delete Previous)", ImVec2(240, 0)))
        {
            ClearSceneAndLoadModel(pendingDroppedModelPath);
            pendingDroppedModelPath.clear();
            showImportModelModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Add as New Object", ImVec2(160, 0)))
        {
            LoadModelDirect(pendingDroppedModelPath);
            pendingDroppedModelPath.clear();
            showImportModelModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90, 0)))
        {
            pendingDroppedModelPath.clear();
            showImportModelModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        int currentAction = static_cast<int>(dragDropAction);
        const char* actions[] = { "Ask each time", "Always replace scene", "Always add to scene" };
        if (ImGui::Combo("On Drag & Drop", &currentAction, actions, IM_ARRAYSIZE(actions)))
        {
            dragDropAction = static_cast<DragDropAction>(currentAction);
        }

        ImGui::EndPopup();
    }
}

void ModuleEditor::DeleteSelectedGameObject()
{
    GameObject* selected = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;
    if (!selected) return;

    pendingDeleteTarget = selected;
}

void ModuleEditor::DuplicateSelectedGameObject()
{
    GameObject* selected = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;
    if (!selected) return;

    pendingDuplicateTarget = selected;
}

void ModuleEditor::CopySelectedGameObject()
{
    GameObject* selected = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;
    if (!selected) return;

    if (clipboardGameObject)
    {
        delete clipboardGameObject;
        clipboardGameObject = nullptr;
    }

    clipboardGameObject = selected->Clone(nullptr, false);
    LOG("[Clipboard] Copied '%s'", selected->GetName().c_str());
}

void ModuleEditor::PasteGameObject()
{
    if (!clipboardGameObject) return;

    pendingPaste = true;
    pendingPasteTarget = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;
}

void ModuleEditor::CutSelectedGameObject()
{
    CopySelectedGameObject();
    DeleteSelectedGameObject();
}

GameObject* ModuleEditor::CreatePrimitive(const std::string& type, GameObject* parent)
{
    if (!App || !App->renderer) return nullptr;
    GameObject* go = App->renderer->CreatePrimitive(type, parent);
    if (go)
    {
        ExecuteCommand(new CreateGameObjectCommand(go, parent), false);
    }
    return go;
}

GameObject* ModuleEditor::CreateEmptyGameObject(GameObject* parent)
{
    if (!App || !App->renderer) return nullptr;
    std::string name = "GameObject " + std::to_string(App->renderer->GetGameObjects().size() + 1);
    GameObject* go = App->renderer->CreateGameObject(name, parent);
    if (go)
    {
        ExecuteCommand(new CreateGameObjectCommand(go, parent), false);
    }
    return go;
}

void ModuleEditor::ProcessPendingActions()
{
    if (pendingDeleteTarget)
    {
        ExecuteCommand(new DeleteGameObjectCommand(pendingDeleteTarget));
        pendingDeleteTarget = nullptr;
    }

    if (pendingDuplicateTarget)
    {
        GameObject* parent = pendingDuplicateTarget->GetParent();
        GameObject* clone = pendingDuplicateTarget->Clone(parent, true);
        if (clone->transform)
        {
            clone->transform->position += glm::vec3(1.0f, 0.0f, 1.0f);
        }
        if (!parent && App && App->renderer)
        {
            App->renderer->AddGameObject(clone, nullptr);
        }
        ExecuteCommand(new CreateGameObjectCommand(clone, parent), false);
        if (App && App->renderer)
        {
            App->renderer->SetSelectedGameObject(clone);
        }
        LOG("[Hierarchy] Duplicated GameObject '%s'", clone->GetName().c_str());
        pendingDuplicateTarget = nullptr;
    }

    if (pendingPaste)
    {
        if (clipboardGameObject)
        {
            GameObject* targetParent = pendingPasteTarget ? pendingPasteTarget->GetParent() : nullptr;
            GameObject* clone = clipboardGameObject->Clone(targetParent, true);
            if (clone->transform)
            {
                clone->transform->position += glm::vec3(1.0f, 0.0f, 1.0f);
            }
            if (!targetParent && App && App->renderer)
            {
                App->renderer->AddGameObject(clone, nullptr);
            }
            ExecuteCommand(new CreateGameObjectCommand(clone, targetParent), false);
            if (App && App->renderer)
            {
                App->renderer->SetSelectedGameObject(clone);
            }
            LOG("[Hierarchy] Pasted GameObject '%s'", clone->GetName().c_str());
        }
        pendingPaste = false;
        pendingPasteTarget = nullptr;
    }
}

static void DrawAssetsModelsMenu(ModuleEditor* editor)
{
    namespace fs = std::filesystem;
    std::vector<fs::path> modelPaths;

    std::error_code ec;
    if (fs::exists("assets", ec) && fs::is_directory("assets", ec))
    {
        for (const auto& entry : fs::directory_iterator("assets", ec))
        {
            if (entry.is_regular_file(ec))
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".glb")
                {
                    modelPaths.push_back(entry.path());
                }
            }
        }
    }

    std::sort(modelPaths.begin(), modelPaths.end(), [](const fs::path& a, const fs::path& b) {
        return a.filename().string() < b.filename().string();
    });

    if (modelPaths.empty())
    {
        ImGui::TextDisabled("No 3D models in assets/");
    }
    else
    {
        for (const auto& p : modelPaths)
        {
            std::string label = p.stem().string();
            std::string fullPath = p.string();
            if (ImGui::MenuItem(label.c_str()))
            {
                editor->PromptLoadModel(fullPath);
            }
        }
    }
}

void ModuleEditor::DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New / Clear Scene", "Ctrl+N"))
            {
                ClearScene();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Load 3D Model"))
            {
                DrawAssetsModelsMenu(this);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            std::string undoLabel = "Undo";
            if (CanUndo()) undoLabel += " (" + GetUndoCommandName() + ")";
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, CanUndo()))
            {
                Undo();
            }

            std::string redoLabel = "Redo";
            if (CanRedo()) redoLabel += " (" + GetRedoCommandName() + ")";
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, CanRedo()))
            {
                Redo();
            }

            ImGui::Separator();

            GameObject* sel = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;
            bool hasSel = (sel != nullptr);

            if (ImGui::MenuItem("Cut", "Ctrl+X", false, hasSel))
            {
                CutSelectedGameObject();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSel))
            {
                CopySelectedGameObject();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboardGameObject != nullptr))
            {
                PasteGameObject();
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSel))
            {
                DuplicateSelectedGameObject();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Delete", false, hasSel))
            {
                DeleteSelectedGameObject();
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::MenuItem("Cube")) CreatePrimitive("Cube");
            if (ImGui::MenuItem("Plane / Quad")) CreatePrimitive("Plane");
            if (ImGui::MenuItem("Sphere")) CreatePrimitive("Sphere");
            if (ImGui::MenuItem("Cylinder")) CreatePrimitive("Cylinder");
            if (ImGui::MenuItem("Pyramid")) CreatePrimitive("Pyramid");
            if (ImGui::MenuItem("Triangle")) CreatePrimitive("Triangle");
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Models"))
            {
                DrawAssetsModelsMenu(this);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Empty GameObject")) CreateEmptyGameObject();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (App && App->renderer)
            {
                ImGui::MenuItem("3D Grid", NULL, &App->renderer->showGrid);
                ImGui::Separator();
            }
            ImGui::MenuItem("Configuration", NULL, &showConfigWindow);
            ImGui::MenuItem("Console", NULL, &showConsoleWindow);
            ImGui::MenuItem("Hierarchy", NULL, &showHierarchyWindow);
            ImGui::MenuItem("Inspector", NULL, &showInspectorWindow);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Window Layout", "Ctrl+Shift+R"))
            {
                ResetWindowLayout();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation"))
            {
                SDL_OpenURL("https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5/blob/main/README.md");
            }
            if (ImGui::MenuItem("Report a bug"))
            {
                SDL_OpenURL("https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5/issues");
            }
            if (ImGui::MenuItem("Download latest"))
            {
                SDL_OpenURL("https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5/releases");
            }
            ImGui::Separator();
            ImGui::MenuItem("About", NULL, &showAboutWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void GetEditorLayout(float& leftW, float& rightW, float& topH, float& posY, float& bottomH,
                            float& consoleX, float& consoleW, float& consoleH,
                            float& menuH, float& margin, float& displayW, float& displayH)
{
    ImGuiIO& io = ImGui::GetIO();
    displayW = io.DisplaySize.x > 0.0f ? io.DisplaySize.x : 1280.0f;
    displayH = io.DisplaySize.y > 0.0f ? io.DisplaySize.y : 720.0f;
    menuH = ImGui::GetFrameHeight();
    if (menuH < 18.0f) menuH = 22.0f;
    margin = 0.0f; // No gap between panels

    leftW = (displayW >= 1400.0f) ? 230.0f : ((displayW >= 1000.0f) ? 210.0f : (std::max)(170.0f, displayW * 0.18f));
    rightW = (displayW >= 1400.0f) ? 310.0f : ((displayW >= 1000.0f) ? 280.0f : (std::max)(200.0f, displayW * 0.24f));

    // Console spans the full width at the bottom
    consoleH = std::clamp(displayH * 0.25f, 140.0f, 190.0f);
    consoleX = 0.0f;
    consoleW = displayW;

    // Left panels (Hierarchy + Configuration) fill height above the console
    float usableH = displayH - menuH - consoleH;
    topH = usableH * 0.40f;
    posY = menuH + topH;
    bottomH = usableH - topH;
}

void ModuleEditor::DrawHierarchyWindow()
{
    float leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH;
    GetEditorLayout(leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH);

    ImGuiCond cond = needResetLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(0.0f, menuH), cond);
    ImGui::SetNextWindowSize(ImVec2(leftW, topH), cond);
    ImGui::SetNextWindowCollapsed(false, cond);

    if (!ImGui::Begin("Hierarchy", &showHierarchyWindow))
    {
        ImGui::End();
        return;
    }

    if (App && App->renderer)
    {
        const auto& gos = App->renderer->GetGameObjects();
        for (GameObject* go : gos)
        {
            DrawGameObjectNode(go);
        }
    }

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboardGameObject != nullptr))
        {
            PasteGameObject();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Create Empty GameObject"))
        {
            CreateEmptyGameObject(nullptr);
        }
        if (ImGui::BeginMenu("Create 3D Object"))
        {
            if (ImGui::MenuItem("Cube")) CreatePrimitive("Cube", nullptr);
            if (ImGui::MenuItem("Plane")) CreatePrimitive("Plane", nullptr);
            if (ImGui::MenuItem("Sphere")) CreatePrimitive("Sphere", nullptr);
            if (ImGui::MenuItem("Cylinder")) CreatePrimitive("Cylinder", nullptr);
            if (ImGui::MenuItem("Pyramid")) CreatePrimitive("Pyramid", nullptr);
            if (ImGui::MenuItem("Triangle")) CreatePrimitive("Triangle", nullptr);
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Models"))
            {
                DrawAssetsModelsMenu(this);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        bool hasObjects = App && App->renderer && !App->renderer->GetGameObjects().empty();
        if (ImGui::MenuItem("Clear Scene", "Ctrl+N", false, hasObjects))
        {
            ClearScene();
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
    {
        if (App && App->renderer)
        {
            App->renderer->SetSelectedGameObject(nullptr);
        }
    }

    ImGui::End();
}

void ModuleEditor::DrawGameObjectNode(GameObject* go)
{
    if (!go) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (App && App->renderer && go == App->renderer->GetSelectedGameObject())
        flags |= ImGuiTreeNodeFlags_Selected;

    if (go->GetChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool nodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(go), flags, "%s", go->GetName().c_str());
    if (ImGui::IsItemClicked(0))
    {
        if (App && App->renderer)
        {
            App->renderer->SetSelectedGameObject(go);
        }
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (App && App->renderer)
        {
            App->renderer->SetSelectedGameObject(go);
        }

        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
            CopySelectedGameObject();
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboardGameObject != nullptr))
        {
            PasteGameObject();
        }
        if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
        {
            DuplicateSelectedGameObject();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Delete"))
        {
            pendingDeleteTarget = go;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Create Empty Child"))
        {
            CreateEmptyGameObject(go);
        }
        if (ImGui::BeginMenu("Create 3D Object"))
        {
            if (ImGui::MenuItem("Cube")) CreatePrimitive("Cube", go);
            if (ImGui::MenuItem("Plane")) CreatePrimitive("Plane", go);
            if (ImGui::MenuItem("Sphere")) CreatePrimitive("Sphere", go);
            if (ImGui::MenuItem("Cylinder")) CreatePrimitive("Cylinder", go);
            if (ImGui::MenuItem("Pyramid")) CreatePrimitive("Pyramid", go);
            if (ImGui::MenuItem("Triangle")) CreatePrimitive("Triangle", go);
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    if (nodeOpen && !go->GetChildren().empty())
    {
        for (GameObject* child : go->GetChildren())
        {
            DrawGameObjectNode(child);
        }
        ImGui::TreePop();
    }
}

void ModuleEditor::DrawInspectorWindow()
{
    float leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH;
    GetEditorLayout(leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH);

    ImGuiCond cond = needResetLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(displayW - rightW, menuH), cond);
    ImGui::SetNextWindowSize(ImVec2(rightW, displayH - menuH - consoleH), cond);
    ImGui::SetNextWindowCollapsed(false, cond);

    if (!ImGui::Begin("Inspector", &showInspectorWindow))
    {
        ImGui::End();
        return;
    }

    GameObject* selected = (App && App->renderer) ? App->renderer->GetSelectedGameObject() : nullptr;

    if (selected)
    {
        char nameBuffer[128];
        strncpy(nameBuffer, selected->GetName().c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
        {
            selected->SetName(nameBuffer);
        }

        bool active = selected->IsActive();
        if (ImGui::Checkbox("Active", &active))
        {
            selected->SetActive(active);
        }

        ImGui::SameLine();
        if (ImGui::Button("Duplicate"))
        {
            DuplicateSelectedGameObject();
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("Delete"))
        {
            DeleteSelectedGameObject();
        }
        ImGui::PopStyleColor(3);

        ImGui::Separator();

        if (selected->transform && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static glm::vec3 startPos, startRot, startScale;
            static bool transformDragging = false;
            static GameObject* dragGO = nullptr;

            if (!transformDragging || dragGO != selected)
            {
                startPos = selected->transform->position;
                startRot = selected->transform->rotation;
                startScale = selected->transform->scale;
                dragGO = selected;
            }

            ImGui::DragFloat3("Position", &selected->transform->position.x, 0.1f);
            bool actPos = ImGui::IsItemActivated();
            bool deactPos = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::DragFloat3("Rotation", &selected->transform->rotation.x, 1.0f);
            bool actRot = ImGui::IsItemActivated();
            bool deactRot = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::DragFloat3("Scale", &selected->transform->scale.x, 0.1f);
            bool actScale = ImGui::IsItemActivated();
            bool deactScale = ImGui::IsItemDeactivatedAfterEdit();

            if (actPos || actRot || actScale)
            {
                transformDragging = true;
            }

            if (deactPos || deactRot || deactScale)
            {
                if (glm::length(startPos - selected->transform->position) > 0.0001f ||
                    glm::length(startRot - selected->transform->rotation) > 0.0001f ||
                    glm::length(startScale - selected->transform->scale) > 0.0001f)
                {
                    ExecuteCommand(new TransformCommand(selected, startPos, startRot, startScale,
                                                        selected->transform->position,
                                                        selected->transform->rotation,
                                                        selected->transform->scale), false);
                }
                transformDragging = false;
            }
        }

        if (selected->mesh && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Mesh: %s", selected->mesh->meshName.c_str());
            ImGui::Text("Vertices: %zu", selected->mesh->GetVertexCount());
            ImGui::Text("Triangles: %zu", selected->mesh->GetTriangleCount());
            ImGui::Text("Indices: %zu", selected->mesh->GetIndexCount());

            ImGui::Checkbox("Show Normals (Per Face)", &selected->mesh->showNormalsPerFace);
            ImGui::Checkbox("Show Normals (Per Vertex / Triangle)", &selected->mesh->showNormalsPerVertex);
        }

        if (selected->texture && ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Texture Path: %s", selected->texture->GetPath().c_str());
            ImGui::Text("Size: %d x %d (%d channels)",
                        selected->texture->GetWidth(),
                        selected->texture->GetHeight(),
                        selected->texture->GetChannels());
            ImGui::Text("Texture ID: %u", selected->texture->GetTextureID());

            ImGui::Checkbox("Checker Texture (White & Black)", &selected->texture->showChecker);
        }
    }
    else
    {
        ImGui::TextDisabled("No GameObject selected.");
    }

    ImGui::End();
}

void ModuleEditor::DrawConsoleWindow()
{
    float leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH;
    GetEditorLayout(leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH);

    ImGuiCond cond = needResetLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(consoleX, displayH - consoleH), cond);
    ImGui::SetNextWindowSize(ImVec2(consoleW, consoleH), cond);
    ImGui::SetNextWindowCollapsed(false, cond);

    if (!ImGui::Begin("Console", &showConsoleWindow))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) ClearLog();
    ImGui::SameLine();
    bool copy_to_clipboard = ImGui::Button("Copy");
    ImGui::SameLine();
    logFilter.Draw("Filter", -120.0f);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &logAutoScroll);

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (copy_to_clipboard)
        ImGui::LogToClipboard();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    const char* buf_begin = logBuffer.begin();
    const char* buf_end = logBuffer.end();

    if (logFilter.IsActive())
    {
        for (int line_no = 0; line_no < logLineOffsets.Size; line_no++)
        {
            const char* line_start = buf_begin + logLineOffsets[line_no];
            const char* line_end = (line_no + 1 < logLineOffsets.Size) ? (buf_begin + logLineOffsets[line_no + 1] - 1) : buf_end;
            if (logFilter.PassFilter(line_start, line_end))
            {
                bool isError = false;
                bool isWarning = false;
                for (const char* p = line_start; p + 4 <= line_end; ++p)
                {
                    if ((p[0] == 'E' || p[0] == 'e') && (p[1] == 'r' || p[1] == 'R') && (p[2] == 'r' || p[2] == 'R') && (p[3] == 'o' || p[3] == 'O') && (p[4] == 'r' || p[4] == 'R'))
                        isError = true;
                    else if ((p[0] == 'W' || p[0] == 'w') && (p[1] == 'a' || p[1] == 'A') && (p[2] == 'r' || p[2] == 'R') && (p[3] == 'n' || p[3] == 'N'))
                        isWarning = true;
                }

                if (isError)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                else if (isWarning)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

                ImGui::TextUnformatted(line_start, line_end);
                ImGui::PopStyleColor();
            }
        }
    }
    else
    {
        ImGuiListClipper clipper;
        clipper.Begin(logLineOffsets.Size);
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char* line_start = buf_begin + logLineOffsets[line_no];
                const char* line_end = (line_no + 1 < logLineOffsets.Size) ? (buf_begin + logLineOffsets[line_no + 1] - 1) : buf_end;

                bool isError = false;
                bool isWarning = false;
                for (const char* p = line_start; p + 4 <= line_end; ++p)
                {
                    if ((p[0] == 'E' || p[0] == 'e') && (p[1] == 'r' || p[1] == 'R') && (p[2] == 'r' || p[2] == 'R') && (p[3] == 'o' || p[3] == 'O') && (p[4] == 'r' || p[4] == 'R'))
                        isError = true;
                    else if ((p[0] == 'W' || p[0] == 'w') && (p[1] == 'a' || p[1] == 'A') && (p[2] == 'r' || p[2] == 'R') && (p[3] == 'n' || p[3] == 'N'))
                        isWarning = true;
                }

                if (isError)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                else if (isWarning)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

                ImGui::TextUnformatted(line_start, line_end);
                ImGui::PopStyleColor();
            }
        }
        clipper.End();
    }
    ImGui::PopStyleVar();

    if (logAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void ModuleEditor::DrawConfigurationWindow()
{
    float leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH;
    GetEditorLayout(leftW, rightW, topH, posY, bottomH, consoleX, consoleW, consoleH, menuH, margin, displayW, displayH);

    ImGuiCond cond = needResetLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(0.0f, posY), cond);
    ImGui::SetNextWindowSize(ImVec2(leftW, displayH - posY - consoleH), cond);
    ImGui::SetNextWindowCollapsed(false, cond);

    if (!ImGui::Begin("Configuration", &showConfigWindow))
    {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char fpsTitle[32];
        float currentFPS = fpsLog.empty() ? 0.0f : fpsLog.back();
        snprintf(fpsTitle, sizeof(fpsTitle), "Framerate %.1f", currentFPS);
        ImGui::PlotHistogram("##framerate", fpsLog.data(), static_cast<int>(fpsLog.size()), 0, fpsTitle, 0.0f, 120.0f, ImVec2(0, 80));

        char msTitle[32];
        float currentMS = msLog.empty() ? 0.0f : msLog.back();
        snprintf(msTitle, sizeof(msTitle), "Milliseconds %.1f ms", currentMS);
        ImGui::PlotHistogram("##milliseconds", msLog.data(), static_cast<int>(msLog.size()), 0, msTitle, 0.0f, 40.0f, ImVec2(0, 80));
    }

    if (ImGui::CollapsingHeader("Window"))
    {
        if (App && App->window)
        {
            bool fullscreen = App->window->IsFullscreen();
            if (ImGui::Checkbox("Fullscreen", &fullscreen))
            {
                App->window->SetFullscreen(fullscreen);
            }

            bool resizable = App->window->IsResizable();
            if (ImGui::Checkbox("Resizable", &resizable))
            {
                App->window->SetResizable(resizable);
            }

            int w = App->window->GetWidth();
            int h = App->window->GetHeight();
            if (ImGui::SliderInt("Width", &w, 640, 2560) || ImGui::SliderInt("Height", &h, 480, 1440))
            {
                App->window->SetSize(w, h);
            }
        }
    }

    if (ImGui::CollapsingHeader("Renderer"))
    {
        if (App && App->renderer)
        {
            if (ImGui::Checkbox("VSync", &App->renderer->vsync))
            {
                SDL_GL_SetSwapInterval(App->renderer->vsync ? 1 : 0);
            }

            ImGui::Checkbox("Wireframe Mode", &App->renderer->wireframe);
            ImGui::Checkbox("Depth Test", &App->renderer->depthTest);
            ImGui::Checkbox("Cull Face", &App->renderer->cullFace);

            ImGui::Separator();
            ImGui::Checkbox("Show 3D Grid", &App->renderer->showGrid);
            if (App->renderer->showGrid)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Show Axes", &App->renderer->showGridAxes);
                if (ImGui::DragFloat("Grid Size", &App->renderer->gridSize, 1.0f, 10.0f, 200.0f, "%.0f"))
                {
                    App->renderer->CreateGridBuffers();
                }
            }

            ImGui::Separator();
            ImGui::ColorEdit4("Clear Color", App->renderer->clearColor);
        }
    }

    if (ImGui::CollapsingHeader("Scene & Import", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* actions[] = { "Ask each time", "Always replace scene", "Always add to scene" };
        int actIdx = static_cast<int>(dragDropAction);
        if (ImGui::Combo("On Drag & Drop 3D Model", &actIdx, actions, IM_ARRAYSIZE(actions)))
        {
            dragDropAction = static_cast<DragDropAction>(actIdx);
        }

        if (ImGui::Button("Clear Current Scene"))
        {
            ClearScene();
        }
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (App && App->camera)
        {
            if (ImGui::DragFloat3("Position", &App->camera->Position[0], 0.1f))
            {
                App->camera->CalculateViewMatrix();
            }
            if (ImGui::DragFloat3("Reference", &App->camera->Reference[0], 0.1f))
            {
                App->camera->LookAt(App->camera->Reference);
            }

            float fov = App->camera->GetFOV();
            if (ImGui::SliderFloat("Field of View", &fov, 10.0f, 120.0f, "%.1f deg"))
            {
                App->camera->SetFOV(fov);
            }

            float nearP = App->camera->GetNearPlane();
            float farP = App->camera->GetFarPlane();
            if (ImGui::DragFloat("Near Plane", &nearP, 0.05f, 0.01f, 10.0f))
            {
                App->camera->SetPlanes(nearP, farP);
            }
            if (ImGui::DragFloat("Far Plane", &farP, 1.0f, 10.0f, 5000.0f))
            {
                App->camera->SetPlanes(nearP, farP);
            }

            ImGui::DragFloat("Speed", &App->camera->speed, 0.2f, 0.1f, 50.0f);
            ImGui::DragFloat("Sensitivity", &App->camera->sensitivity, 0.01f, 0.01f, 2.0f);

            ImGui::Separator();
            if (ImGui::Button("Focus on Model (F)"))
            {
                App->camera->FocusOnModel();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Camera"))
            {
                App->camera->Reset();
            }

            ImGui::Separator();
            ImGui::TextDisabled("Controls Help:");
            ImGui::BulletText("Hold RMB + WASD: Fly Camera (Shift = Sprint)");
            ImGui::BulletText("Hold RMB + Q/E: Move Down / Up");
            ImGui::BulletText("Alt + LMB Drag: Orbit around model/reference");
            ImGui::BulletText("MMB Drag: Pan camera");
            ImGui::BulletText("Mouse Wheel / Alt + RMB: Zoom in / out");
            ImGui::BulletText("F: Focus camera on model");
        }
    }

    if (ImGui::CollapsingHeader("Input"))
    {
        if (App && App->camera)
        {
            ImGui::SeparatorText("Mouse");
            ImGui::SliderFloat("Mouse Sensitivity", &App->camera->sensitivity,     0.01f, 2.0f,  "%.2f");
            ImGui::SliderFloat("Zoom Speed",        &App->camera->zoomSpeed,        0.1f, 10.0f, "%.1f");
            ImGui::SliderFloat("Pan Speed",         &App->camera->panSpeed,         0.5f, 30.0f, "%.1f");
            ImGui::Checkbox("Invert X Axis", &App->camera->invertX);
            ImGui::SameLine();
            ImGui::Checkbox("Invert Y Axis", &App->camera->invertY);

            ImGui::SeparatorText("Movement");
            ImGui::SliderFloat("Move Speed",        &App->camera->speed,            0.5f, 50.0f, "%.1f");
            ImGui::SliderFloat("Sprint Multiplier", &App->camera->sprintMultiplier, 1.0f, 10.0f, "%.1f");

            ImGui::SeparatorText("Live State");
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Mouse Pos: (%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
            ImGui::Text("Mouse Delta: (%.1f, %.1f)", io.MouseDelta.x, io.MouseDelta.y);
            ImGui::Text("Mouse Wheel: %.1f", io.MouseWheel);
            ImGui::Text("Buttons: [L:%s] [R:%s] [M:%s]",
                io.MouseDown[0] ? "DOWN" : "UP",
                io.MouseDown[1] ? "DOWN" : "UP",
                io.MouseDown[2] ? "DOWN" : "UP");
            ImGui::Text("Modifiers: [Shift:%s] [Ctrl:%s] [Alt:%s]",
                io.KeyShift ? "YES" : "NO",
                io.KeyCtrl ? "YES" : "NO",
                io.KeyAlt ? "YES" : "NO");
        }
    }

    if (ImGui::CollapsingHeader("Textures"))
    {
        GLint maxTexSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
        ImGui::Text("GPU Max Texture Size: %d x %d", maxTexSize, maxTexSize);
        ImGui::Separator();

        static int filterMode = 0; 
        static int wrapMode   = 0; 

        const char* filterOptions[] = { "Linear (Bilinear)", "Nearest (Pixelated)" };
        const char* wrapOptions[]   = { "Repeat", "Clamp to Edge", "Mirrored Repeat" };

        bool changed = false;
        changed |= ImGui::Combo("Filter Mode", &filterMode, filterOptions, IM_ARRAYSIZE(filterOptions));
        changed |= ImGui::Combo("Wrap Mode",   &wrapMode,   wrapOptions,   IM_ARRAYSIZE(wrapOptions));

        if (changed || ImGui::Button("Apply to All Textures"))
        {
            GLint minF = (filterMode == 0) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST;
            GLint magF = (filterMode == 0) ? GL_LINEAR               : GL_NEAREST;
            GLint wrap = (wrapMode   == 0) ? GL_REPEAT
                       : (wrapMode   == 1) ? GL_CLAMP_TO_EDGE
                                           : GL_MIRRORED_REPEAT;

            auto applyToTex = [&](unsigned int tid) {
                if (tid == 0) return;
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minF);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magF);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
                glBindTexture(GL_TEXTURE_2D, 0);
            };

            std::function<void(GameObject*)> walk = [&](GameObject* go) {
                if (!go) return;
                if (go->texture) applyToTex(go->texture->GetTextureID());
                for (auto* child : go->GetChildren()) walk(child);
            };
            if (App && App->renderer)
                for (auto* go : App->renderer->GetGameObjects()) walk(go);
        }

        ImGui::Separator();
        ImGui::Text("Loaded Textures:");

        int count = 0;
        std::function<void(GameObject*)> countWalk = [&](GameObject* go) {
            if (!go) return;
            if (go->texture && go->texture->GetTextureID() != 0)
            {
                ++count;
                ImGui::BulletText("%s (%dx%d, %dch)",
                    go->texture->GetPath().c_str(),
                    go->texture->GetWidth(), go->texture->GetHeight(),
                    go->texture->GetChannels());
            }
            for (auto* child : go->GetChildren()) countWalk(child);
        };
        if (App && App->renderer)
            for (auto* go : App->renderer->GetGameObjects()) countWalk(go);
        if (count == 0)
            ImGui::TextDisabled("No textures loaded.");
    }

    if (ImGui::CollapsingHeader("Hardware & Software", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int sdlVer = SDL_GetVersion();
        ImGui::Text("SDL Version: %d.%d.%d", SDL_VERSIONNUM_MAJOR(sdlVer), SDL_VERSIONNUM_MINOR(sdlVer), SDL_VERSIONNUM_MICRO(sdlVer));

        ILint devilVer = ilGetInteger(IL_VERSION_NUM);
        int devILMajor = devilVer / 100;
        int devILMinor = (devilVer / 10) % 10;
        int devILPatch = devilVer % 10;
        ImGui::Text("DevIL Version: %d.%d.%d", devILMajor, devILMinor, devILPatch);
        ImGui::Separator();

        ImGui::Text("CPUs: %d Cores", SDL_GetNumLogicalCPUCores());
        ImGui::Text("CPU Cache Line: %d bytes", SDL_GetCPUCacheLineSize());
        ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());

#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            ImGui::Text("Process RAM: %.2f MB", pmc.WorkingSetSize / (1024.0f * 1024.0f));
        }
#elif defined(__linux__)
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
        {
            ImGui::Text("Process RAM: %.2f MB", usage.ru_maxrss / 1024.0f);
        }
#endif
        ImGui::Separator();

        const char* glVendor = (const char*)glGetString(GL_VENDOR);
        const char* glRenderer = (const char*)glGetString(GL_RENDERER);
        const char* glVersion = (const char*)glGetString(GL_VERSION);
        const char* glslVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

        ImGui::Text("GPU: %s", glRenderer ? glRenderer : "Unknown");
        ImGui::Text("Brand: %s", glVendor ? glVendor : "Unknown");
        ImGui::Text("OpenGL: %s", glVersion ? glVersion : "Unknown");
        ImGui::Text("GLSL Version: %s", glslVersion ? glslVersion : "Unknown");
    }

    ImGui::End();
}

void ModuleEditor::DrawAboutWindow()
{
    if (!ImGui::Begin("About", &showAboutWindow))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("MotorVJ - Visualizador de Geometria (Entrega 1)");
    ImGui::Text("Un motor y visualizador 3D desarrollado para la asignatura de Motores de Videojuegos.");
    ImGui::Separator();
    
    ImGui::Text("Miembros del equipo:");
    ImGui::BulletText("Marc Pladellorens (Editor UI, Ventanas y Consola)");
    ImGui::BulletText("Montse Medina (Camara Orbital, Controles y Matematicas)");
    ImGui::BulletText("Aidan (Modelos 3D, Assimp y Componentes)");
    ImGui::Separator();

    int sdlVer = SDL_GetVersion();
    ImGui::Text("Librerias utilizadas:");
    ImGui::BulletText("SDL3 v%d.%d.%d", SDL_VERSIONNUM_MAJOR(sdlVer), SDL_VERSIONNUM_MINOR(sdlVer), SDL_VERSIONNUM_MICRO(sdlVer));
    ImGui::BulletText("OpenGL 4.6 (glad)");
    ImGui::BulletText("Dear ImGui %s", IMGUI_VERSION);
    ImGui::BulletText("DevIL (Developer's Image Library)");
    ImGui::BulletText("Assimp (Open Asset Import Library)");
    ImGui::BulletText("GLM (OpenGL Mathematics)");
    ImGui::BulletText("stb_image");
    ImGui::Separator();

    ImGui::Text("Licencia:");
    ImGui::TextWrapped("Este proyecto esta bajo la Licencia MIT. Consulta el repositorio para mas detalles.");
    if (ImGui::Button("Ver Licencia en GitHub"))
    {
        SDL_OpenURL("https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5/blob/main/LICENSE");
    }

    ImGui::End();
}

void ModuleEditor::ResetWindowLayout()
{
    needResetLayout = true;
    showHierarchyWindow = true;
    showInspectorWindow = true;
    showConfigWindow = true;
    showConsoleWindow = true;
}

