#include "ComponentTransform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>

ComponentTransform::ComponentTransform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM),
      position(0.0f, 0.0f, 0.0f),
      rotation(0.0f, 0.0f, 0.0f),
      scale(1.0f, 1.0f, 1.0f)
{
}

ComponentTransform::~ComponentTransform()
{
}

glm::mat4 ComponentTransform::GetLocalMatrix() const
{
    glm::mat4 local = glm::mat4(1.0f);
    local = glm::translate(local, position);
    local = glm::rotate(local, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    local = glm::rotate(local, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    local = glm::rotate(local, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    local = glm::scale(local, scale);
    return local;
}

glm::mat4 ComponentTransform::GetGlobalMatrix() const
{
    glm::mat4 local = GetLocalMatrix();
    if (owner && owner->GetParent() && owner->GetParent()->transform)
    {
        return owner->GetParent()->transform->GetGlobalMatrix() * local;
    }
    return local;
}
