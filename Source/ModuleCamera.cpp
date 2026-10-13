#include "ModuleCamera.h"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

ModuleCamera::ModuleCamera() {
    Position = glm::vec3(0.0f, 0.0f, 3.0f);
    Reference = glm::vec3(0.0f, 0.0f, 0.0f);
    Front = glm::normalize(Reference - Position);
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));
    
    yaw = -90.0f;
    pitch = 0.0f;

    CalculateViewMatrix();
    ProjectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
}

ModuleCamera::~ModuleCamera() {}

bool ModuleCamera::Init() {
    return true;
}

void ModuleCamera::CalculateViewMatrix() {
    ViewMatrix = glm::lookAt(Position, Position + Front, Up);
}

bool ModuleCamera::Update(float deltaTime) {
    float speed = 5.0f; // Velocidad de movimiento

    const bool* keyboard = SDL_GetKeyboardState(NULL);
    float mouseX, mouseY;
    SDL_MouseButtonFlags mouseButtons = SDL_GetRelativeMouseState(&mouseX, &mouseY);

    float sensitivity = 0.1f;

    // Cámara Libre (Noclip)
    if (mouseButtons & SDL_BUTTON_RMASK) {
        yaw += mouseX * sensitivity;
        pitch -= mouseY * sensitivity;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        Front = glm::normalize(newFront);
        Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        Up = glm::normalize(glm::cross(Right, Front));

        if (keyboard[SDL_SCANCODE_W]) Position += Front * speed * deltaTime;
        if (keyboard[SDL_SCANCODE_S]) Position -= Front * speed * deltaTime;
        if (keyboard[SDL_SCANCODE_A]) Position -= Right * speed * deltaTime;
        if (keyboard[SDL_SCANCODE_D]) Position += Right * speed * deltaTime;
        if (keyboard[SDL_SCANCODE_Q]) Position += Up * speed * deltaTime;
        if (keyboard[SDL_SCANCODE_E]) Position -= Up * speed * deltaTime;
        
        Reference = Position + Front;
        CalculateViewMatrix();
    }
    // Cámara Orbital
    else if (keyboard[SDL_SCANCODE_LALT]) {
        float angleX = -mouseX * sensitivity;
        float angleY = -mouseY * sensitivity;

        glm::vec3 offset = Position - Reference;
        
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), glm::vec3(0.0f, 1.0f, 0.0f));
        offset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
        
        glm::vec3 curRight = glm::normalize(glm::cross(glm::normalize(-offset), glm::vec3(0.0f, 1.0f, 0.0f)));
        
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), curRight);
        glm::vec3 newOffset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
        
        glm::vec3 curFront = glm::normalize(-newOffset);
        float dot = glm::dot(curFront, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::abs(dot) < 0.99f) {
            offset = newOffset;
        }

        Position = Reference + offset;
        Front = glm::normalize(Reference - Position);
        Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        Up = glm::normalize(glm::cross(Right, Front));

        CalculateViewMatrix();
    }

    return true;
}

bool ModuleCamera::CleanUp() {
    return true;
}
