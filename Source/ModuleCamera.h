#pragma once
#include <glm/glm.hpp>

class ModuleCamera {
public:
    ModuleCamera();
    ~ModuleCamera();

    bool Init();
    bool Update(float deltaTime);
    bool CleanUp();

    glm::mat4 GetViewMatrix() const { return ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return ProjectionMatrix; }

private:
    void CalculateViewMatrix();

public:
    glm::vec3 Position;
    glm::vec3 Reference;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

private:
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;

    float yaw;
    float pitch;
};