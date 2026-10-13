#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <cstdarg>

ModuleEditor::ModuleEditor() {}
ModuleEditor::~ModuleEditor() {}

bool ModuleEditor::Init()
{
    ClearLog();
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
    if (showConsoleWindow) DrawConsoleWindow();
    if (showAboutWindow) DrawAboutWindow();

    // Placeholder Hierarchy
    if (showHierarchyWindow) {
        ImGui::Begin("Hierarchy", &showHierarchyWindow);
        ImGui::Text("GameObject 1 (Baker House)");
        ImGui::Text("GameObject 2 (Custom Model)");
        ImGui::End();
    }

    // Placeholder Inspector
    if (showInspectorWindow) {
        ImGui::Begin("Inspector", &showInspectorWindow);
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            static float pos[3] = { 0.0f, 0.0f, 0.0f };
            static float rot[3] = { 0.0f, 0.0f, 0.0f };
            static float scale[3] = { 1.0f, 1.0f, 1.0f };
            ImGui::DragFloat3("Position", pos, 0.1f);
            ImGui::DragFloat3("Rotation", rot, 1.0f);
            ImGui::DragFloat3("Scale", scale, 0.1f);
        }
        if (ImGui::CollapsingHeader("Mesh")) {
            ImGui::Text("Vertices: 0");
            ImGui::Text("Indices: 0");
            static bool showNormals = false;
            ImGui::Checkbox("Show Normals", &showNormals);
        }
        if (ImGui::CollapsingHeader("Texture")) {
            ImGui::Text("Path: None");
            static bool checkerTexture = false;
            ImGui::Checkbox("Checker Texture", &checkerTexture);
        }
        ImGui::End();
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

void ModuleEditor::DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
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

void ModuleEditor::DrawConsoleWindow()
{
    ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiCond_FirstUseEver);
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
        ImGui::PlotHistogram("##framerate", fpsLog.data(), static_cast<int>(fpsLog.size()), 0, fpsTitle, 0.0f, 120.0f, ImVec2(310, 100));

        char msTitle[32];
        float currentMS = msLog.empty() ? 0.0f : msLog.back();
        snprintf(msTitle, sizeof(msTitle), "Milliseconds %.1f ms", currentMS);
        ImGui::PlotHistogram("##milliseconds", msLog.data(), static_cast<int>(msLog.size()), 0, msTitle, 0.0f, 40.0f, ImVec2(310, 100));
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

            ImGui::ColorEdit4("Clear Color", App->renderer->clearColor);
        }
    }

    if (ImGui::CollapsingHeader("Hardware & Software", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int sdlVer = SDL_GetVersion();
        ImGui::Text("SDL Version: %d.%d.%d", SDL_VERSIONNUM_MAJOR(sdlVer), SDL_VERSIONNUM_MINOR(sdlVer), SDL_VERSIONNUM_MICRO(sdlVer));
        ImGui::Separator();

        ImGui::Text("CPUs: %d Cores", SDL_GetNumLogicalCPUCores());
        ImGui::Text("CPU Cache Line: %d bytes", SDL_GetCPUCacheLineSize());
        ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());
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
