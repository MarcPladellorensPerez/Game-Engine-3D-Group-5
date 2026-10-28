#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class ComponentTransform : public Component {
public:
    ComponentTransform(GameObject* owner);
    virtual ~ComponentTransform();

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // In degrees
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 GetLocalMatrix() const;
    glm::mat4 GetGlobalMatrix() const;

    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetRotation(const glm::vec3& rot) { rotation = rot; }
    void SetScale(const glm::vec3& sc) { scale = sc; }
};
