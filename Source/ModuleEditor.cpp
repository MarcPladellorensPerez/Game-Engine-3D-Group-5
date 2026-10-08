#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

ModuleEditor::ModuleEditor() {}
ModuleEditor::~ModuleEditor() {}

bool ModuleEditor::Init()
{
    LOG("Init ImGui Editor");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

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
    DrawMainMenuBar();

    if (showConfigWindow) DrawConfigurationWindow();
    if (showAboutWindow) DrawAboutWindow();

    // Placeholder Console Window
    if (showConsoleWindow) {
        ImGui::Begin("Console", &showConsoleWindow);
        ImGui::Text("Console output goes here...");
        ImGui::End();
    }
    // Placeholder Hierarchy
    if (showHierarchyWindow) {
        ImGui::Begin("Hierarchy", &showHierarchyWindow);
        ImGui::Text("GameObject 1");
        ImGui::Text("GameObject 2");
        ImGui::End();
    }
    // Placeholder Inspector
    if (showInspectorWindow) {
        ImGui::Begin("Inspector", &showInspectorWindow);
        ImGui::Text("Transform component info");
        ImGui::End();
    }

    // Keep track of FPS
    fpsLog.push_back(ImGui::GetIO().Framerate);
    if (fpsLog.size() > 100) fpsLog.erase(fpsLog.begin());

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleEditor::PostUpdate()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return UPDATE_CONTINUE;
}

bool ModuleEditor::CleanUp()
{
    LOG("CleanUp ImGui Editor");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return true;
}

void ModuleEditor::DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                // To exit, we could handle an event or set a flag in Application
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Configuration", NULL, &showConfigWindow);
            ImGui::MenuItem("Console", NULL, &showConsoleWindow);
            ImGui::MenuItem("Hierarchy", NULL, &showHierarchyWindow);
            ImGui::MenuItem("Inspector", NULL, &showInspectorWindow);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation")) { /* Open browser */ }
            if (ImGui::MenuItem("Report a bug")) { /* Open browser */ }
            if (ImGui::MenuItem("Download latest")) { /* Open browser */ }
            ImGui::Separator();
            ImGui::MenuItem("About", NULL, &showAboutWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ModuleEditor::DrawConfigurationWindow()
{
    ImGui::Begin("Configuration", &showConfigWindow);
    if (ImGui::CollapsingHeader("Application"))
    {
        char title[25];
        sprintf(title, "Framerate %.1f", fpsLog.back());
        ImGui::PlotHistogram("##framerate", &fpsLog[0], fpsLog.size(), 0, title, 0.0f, 100.0f, ImVec2(310, 100));
    }
    if (ImGui::CollapsingHeader("Hardware"))
    {
        ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());
        ImGui::Text("CPU Cores: %d", SDL_GetSystemLogicalCoreCount());
    }
    ImGui::End();
}

void ModuleEditor::DrawAboutWindow()
{
    ImGui::Begin("About", &showAboutWindow);
    ImGui::Text("MotorVJ - Entrega 1");
    ImGui::Text("Authors: Marc, Montse, Aidan");
    ImGui::Separator();
    ImGui::Text("Dependencies:");
    ImGui::Text("- SDL3");
    ImGui::Text("- OpenGL 4.6");
    ImGui::Text("- ImGui");
    ImGui::Text("- Assimp");
    ImGui::Text("- DevIL");
    ImGui::Separator();
    ImGui::Text("MIT License");
    ImGui::End();
}
