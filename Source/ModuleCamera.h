#pragma once
#include "Module.h"
#include <glm/glm.hpp>

class ModuleCamera : public Module
{
public:
    ModuleCamera();
    virtual ~ModuleCamera();

    bool Init() override;
    UpdateStatus Update() override;
    bool CleanUp() override;

    glm::mat4 GetViewMatrix() const { return ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return ProjectionMatrix; }

    void LookAt(const glm::vec3& target);
    void Focus(const glm::vec3& target, float distance = 14.0f);
    void FocusOnModel();
    void Reset();

    void SetPosition(const glm::vec3& pos);
    void SetReference(const glm::vec3& ref);
    void SetFOV(float fovDeg);
    void SetPlanes(float nearP, float farP);
    void SetAspectRatio(float aspect);

    float GetFOV() const { return fov; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }
    float GetAspectRatio() const { return aspectRatio; }

    void CalculateViewMatrix();
    void CalculateProjectionMatrix();

public:
    glm::vec3 Position = glm::vec3(8.0f, 6.0f, 12.0f);
    glm::vec3 Reference = glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);

    float speed = 8.0f;
    float sprintMultiplier = 2.5f;
    float sensitivity = 0.15f;
    float zoomSpeed = 1.5f;
    float panSpeed = 6.0f;

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 500.0f;
    float aspectRatio = 1.777f;

private:
    void RecalculateVectorsFromAngles();
    void RecalculateAnglesFromVectors();

    glm::mat4 ViewMatrix = glm::mat4(1.0f);
    glm::mat4 ProjectionMatrix = glm::mat4(1.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;

    bool isFreeLooking = false;
    bool isOrbiting = false;
    bool isPanning = false;
    bool relativeMouseActive = false;
};