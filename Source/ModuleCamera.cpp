#include "ModuleCamera.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

ModuleCamera::ModuleCamera()
{
    Reset();
}

ModuleCamera::~ModuleCamera()
{
}

bool ModuleCamera::Init()
{
    LOG("Init ModuleCamera");
    Reset();
    return true;
}

void ModuleCamera::Reset()
{
    Position = glm::vec3(8.0f, 6.0f, 12.0f);
    Reference = glm::vec3(0.0f, 2.0f, 0.0f);

    fov = 45.0f;
    nearPlane = 0.1f;
    farPlane = 500.0f;
    speed = 8.0f;
    sprintMultiplier = 2.5f;
    sensitivity = 0.15f;
    zoomSpeed = 1.5f;
    panSpeed = 6.0f;

    LookAt(Reference);

    if (App && App->window && App->window->GetHeight() > 0)
    {
        aspectRatio = static_cast<float>(App->window->GetWidth()) / static_cast<float>(App->window->GetHeight());
    }
    else
    {
        aspectRatio = 1280.0f / 720.0f;
    }

    CalculateProjectionMatrix();
}

void ModuleCamera::LookAt(const glm::vec3& target)
{
    Reference = target;
    glm::vec3 dir = Reference - Position;
    if (glm::length(dir) < 0.0001f)
    {
        dir = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    Front = glm::normalize(dir);
    RecalculateAnglesFromVectors();
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));
    CalculateViewMatrix();
}

void ModuleCamera::Focus(const glm::vec3& target, float distance)
{
    Reference = target;
    glm::vec3 dir = Position - Reference;
    if (glm::length(dir) < 0.0001f)
    {
        dir = glm::vec3(0.6f, 0.4f, 0.8f);
    }
    Position = Reference + glm::normalize(dir) * distance;
    LookAt(Reference);
}

void ModuleCamera::FocusOnModel()
{
    glm::vec3 target(0.0f, 2.0f, 0.0f);
    if (App && App->renderer)
    {
        GameObject* targetGO = App->renderer->GetSelectedGameObject();
        if (!targetGO && !App->renderer->GetGameObjects().empty())
        {
            targetGO = App->renderer->GetGameObjects().front();
        }

        if (targetGO && targetGO->transform)
        {
            target = targetGO->transform->position + glm::vec3(0.0f, 2.0f, 0.0f);
        }
    }
    Focus(target, 14.0f);
    LOG("Camera focused on model at (%.1f, %.1f, %.1f)", target.x, target.y, target.z);
}

void ModuleCamera::CalculateViewMatrix()
{
    ViewMatrix = glm::lookAt(Position, Position + Front, Up);
}

void ModuleCamera::CalculateProjectionMatrix()
{
    ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void ModuleCamera::RecalculateAnglesFromVectors()
{
    pitch = glm::degrees(asin(glm::clamp(Front.y, -0.999f, 0.999f)));
    yaw = glm::degrees(atan2(Front.z, Front.x));
}

void ModuleCamera::RecalculateVectorsFromAngles()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    Front = glm::normalize(newFront);
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));
}

void ModuleCamera::SetPosition(const glm::vec3& pos)
{
    Position = pos;
    CalculateViewMatrix();
}

void ModuleCamera::SetReference(const glm::vec3& ref)
{
    Reference = ref;
    LookAt(Reference);
}

void ModuleCamera::SetFOV(float fovDeg)
{
    fov = glm::clamp(fovDeg, 5.0f, 160.0f);
    CalculateProjectionMatrix();
}

void ModuleCamera::SetPlanes(float nearP, float farP)
{
    nearPlane = glm::max(nearP, 0.01f);
    farPlane = glm::max(farP, nearPlane + 1.0f);
    CalculateProjectionMatrix();
}

void ModuleCamera::SetAspectRatio(float aspect)
{
    if (aspect > 0.0f)
    {
        aspectRatio = aspect;
        CalculateProjectionMatrix();
    }
}

UpdateStatus ModuleCamera::Update()
{
    static uint64_t lastCounter = 0;
    uint64_t nowCounter = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    float deltaTime = 0.016f;
    if (lastCounter != 0)
    {
        deltaTime = static_cast<float>(nowCounter - lastCounter) / static_cast<float>(freq);
        deltaTime = glm::clamp(deltaTime, 0.0001f, 0.1f);
    }
    lastCounter = nowCounter;

    if (App && App->window)
    {
        int w = App->window->GetWidth();
        int h = App->window->GetHeight();
        if (w > 0 && h > 0)
        {
            float currentAspect = static_cast<float>(w) / static_cast<float>(h);
            if (glm::abs(currentAspect - aspectRatio) > 0.001f)
            {
                aspectRatio = currentAspect;
                CalculateProjectionMatrix();
            }
        }
    }

    const bool* keyboard = SDL_GetKeyboardState(nullptr);
    float mouseX = 0.0f, mouseY = 0.0f;
    SDL_MouseButtonFlags mouseButtons = SDL_GetRelativeMouseState(&mouseX, &mouseY);
    ImGuiIO& io = ImGui::GetIO();

    bool altHeld = (keyboard[SDL_SCANCODE_LALT] || keyboard[SDL_SCANCODE_RALT]);
    bool shiftHeld = (keyboard[SDL_SCANCODE_LSHIFT] || keyboard[SDL_SCANCODE_RSHIFT]);
    bool rmbPressed = (mouseButtons & SDL_BUTTON_RMASK) != 0;
    bool lmbPressed = (mouseButtons & SDL_BUTTON_LMASK) != 0;
    bool mmbPressed = (mouseButtons & SDL_BUTTON_MMASK) != 0;

    if (!isFreeLooking && !isOrbiting && !isPanning)
    {
        if (io.WantCaptureMouse)
        {
            rmbPressed = false;
            lmbPressed = false;
            mmbPressed = false;
        }
    }

    if (altHeld && lmbPressed)
    {
        isOrbiting = true;
        isFreeLooking = false;
        isPanning = false;
    }
    else if (mmbPressed || (altHeld && mmbPressed))
    {
        isPanning = true;
        isOrbiting = false;
        isFreeLooking = false;
    }
    else if (rmbPressed && !altHeld)
    {
        isFreeLooking = true;
        isOrbiting = false;
        isPanning = false;
    }
    else if (altHeld && rmbPressed)
    {
        float zoomDelta = mouseY * sensitivity * 0.5f;
        Position += Front * zoomDelta;
        CalculateViewMatrix();
        isFreeLooking = false;
        isOrbiting = false;
        isPanning = false;
    }
    else
    {
        isFreeLooking = false;
        isOrbiting = false;
        isPanning = false;
    }

    bool needRelativeMouse = (isFreeLooking || isOrbiting);
    if (needRelativeMouse != relativeMouseActive)
    {
        if (App && App->window && App->window->GetWindow())
        {
            SDL_SetWindowRelativeMouseMode(App->window->GetWindow(), needRelativeMouse);
            relativeMouseActive = needRelativeMouse;
        }
    }

    if (isFreeLooking)
    {
        yaw += mouseX * sensitivity;
        pitch -= mouseY * sensitivity;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        RecalculateVectorsFromAngles();

        float currentSpeed = speed;
        if (shiftHeld)
        {
            currentSpeed *= sprintMultiplier;
        }

        if (keyboard[SDL_SCANCODE_W]) Position += Front * currentSpeed * deltaTime;
        if (keyboard[SDL_SCANCODE_S]) Position -= Front * currentSpeed * deltaTime;
        if (keyboard[SDL_SCANCODE_A]) Position -= Right * currentSpeed * deltaTime;
        if (keyboard[SDL_SCANCODE_D]) Position += Right * currentSpeed * deltaTime;
        if (keyboard[SDL_SCANCODE_Q]) Position -= glm::vec3(0.0f, 1.0f, 0.0f) * currentSpeed * deltaTime;
        if (keyboard[SDL_SCANCODE_E]) Position += glm::vec3(0.0f, 1.0f, 0.0f) * currentSpeed * deltaTime;

        Reference = Position + Front * 10.0f;
        CalculateViewMatrix();
    }
    else if (isOrbiting)
    {
        float angleX = -mouseX * sensitivity;
        float angleY = -mouseY * sensitivity;

        glm::vec3 offset = Position - Reference;
        float dist = glm::length(offset);
        if (dist < 0.001f)
        {
            offset = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), glm::vec3(0.0f, 1.0f, 0.0f));
        offset = glm::vec3(rotY * glm::vec4(offset, 1.0f));

        glm::vec3 curRight = glm::normalize(glm::cross(glm::normalize(offset), glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), curRight);
        glm::vec3 newOffset = glm::vec3(rotX * glm::vec4(offset, 1.0f));

        glm::vec3 norm = glm::normalize(newOffset);
        if (glm::abs(glm::dot(norm, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.98f)
        {
            offset = newOffset;
        }

        Position = Reference + offset;
        Front = glm::normalize(Reference - Position);
        Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        Up = glm::normalize(glm::cross(Right, Front));
        RecalculateAnglesFromVectors();
        CalculateViewMatrix();
    }
    else if (isPanning)
    {
        glm::vec3 panDelta = (-Right * mouseX + Up * mouseY) * (panSpeed * 0.005f);
        Position += panDelta;
        Reference += panDelta;
        CalculateViewMatrix();
    }

    if (!io.WantCaptureMouse && io.MouseWheel != 0.0f)
    {
        float wheelDelta = io.MouseWheel * zoomSpeed;
        glm::vec3 toRef = Reference - Position;
        float dist = glm::length(toRef);

        if (wheelDelta > 0.0f) 
        {
            if (dist > 1.0f)
            {
                float step = glm::min(wheelDelta, dist - 0.5f);
                Position += Front * step;
            }
            else
            {
                Position += Front * wheelDelta;
            }
        }
        else 
        {
            Position += Front * wheelDelta;
        }
        CalculateViewMatrix();
    }

    static bool fKeyPressedLast = false;
    bool fKeyPressed = keyboard[SDL_SCANCODE_F];
    if (fKeyPressed && !fKeyPressedLast && !io.WantTextInput)
    {
        FocusOnModel();
    }
    fKeyPressedLast = fKeyPressed;

    return UPDATE_CONTINUE;
}

bool ModuleCamera::CleanUp()
{
    LOG("CleanUp ModuleCamera");
    if (relativeMouseActive && App && App->window && App->window->GetWindow())
    {
        SDL_SetWindowRelativeMouseMode(App->window->GetWindow(), false);
        relativeMouseActive = false;
    }
    return true;
}
