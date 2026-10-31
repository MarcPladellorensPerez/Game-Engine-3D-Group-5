#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Application.h"
#include "ModuleRender.h"
#include "GameObject.h"

class EditorCommand
{
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void Redo() { Execute(); }
    virtual std::string GetName() const = 0;
};

class CreateGameObjectCommand : public EditorCommand
{
public:
    CreateGameObjectCommand(GameObject* go, GameObject* parent = nullptr, int index = -1)
        : gameObject(go), parent(parent), index(index), inScene(true)
    {
        if (go) name = go->GetName();
    }

    virtual ~CreateGameObjectCommand()
    {
        if (!inScene && gameObject != nullptr)
        {
            delete gameObject;
            gameObject = nullptr;
        }
    }

    void Execute() override
    {
        if (!inScene && gameObject != nullptr && App && App->renderer)
        {
            App->renderer->AddGameObject(gameObject, parent, index);
            inScene = true;
            App->renderer->SetSelectedGameObject(gameObject);
        }
    }

    void Undo() override
    {
        if (inScene && gameObject != nullptr && App && App->renderer)
        {
            App->renderer->DetachGameObject(gameObject);
            inScene = false;
        }
    }

    void Redo() override
    {
        Execute();
    }

    std::string GetName() const override
    {
        return "Create " + name;
    }

private:
    GameObject* gameObject = nullptr;
    GameObject* parent = nullptr;
    int index = -1;
    bool inScene = true;
    std::string name;
};

class DeleteGameObjectCommand : public EditorCommand
{
public:
    DeleteGameObjectCommand(GameObject* go)
        : gameObject(go), inScene(true)
    {
        if (go)
        {
            name = go->GetName();
            parent = go->GetParent();
            if (parent)
            {
                index = parent->GetChildIndex(go);
            }
            else if (App && App->renderer)
            {
                index = App->renderer->GetRootGameObjectIndex(go);
            }
        }
    }

    virtual ~DeleteGameObjectCommand()
    {
        if (!inScene && gameObject != nullptr)
        {
            delete gameObject;
            gameObject = nullptr;
        }
    }

    void Execute() override
    {
        if (inScene && gameObject != nullptr && App && App->renderer)
        {
            App->renderer->DetachGameObject(gameObject);
            inScene = false;
        }
    }

    void Undo() override
    {
        if (!inScene && gameObject != nullptr && App && App->renderer)
        {
            App->renderer->AddGameObject(gameObject, parent, index);
            inScene = true;
            App->renderer->SetSelectedGameObject(gameObject);
        }
    }

    void Redo() override
    {
        Execute();
    }

    std::string GetName() const override
    {
        return "Delete " + name;
    }

private:
    GameObject* gameObject = nullptr;
    GameObject* parent = nullptr;
    int index = -1;
    bool inScene = true;
    std::string name;
};

class TransformCommand : public EditorCommand
{
public:
    TransformCommand(GameObject* go,
                     const glm::vec3& oldPos, const glm::vec3& oldRot, const glm::vec3& oldScale,
                     const glm::vec3& newPos, const glm::vec3& newRot, const glm::vec3& newScale)
        : gameObject(go),
          oldPosition(oldPos), oldRotation(oldRot), oldScale(oldScale),
          newPosition(newPos), newRotation(newRot), newScale(newScale)
    {
        if (go) name = go->GetName();
    }

    virtual ~TransformCommand() = default;

    void Execute() override
    {
        if (gameObject && gameObject->transform)
        {
            gameObject->transform->position = newPosition;
            gameObject->transform->rotation = newRotation;
            gameObject->transform->scale = newScale;
        }
    }

    void Undo() override
    {
        if (gameObject && gameObject->transform)
        {
            gameObject->transform->position = oldPosition;
            gameObject->transform->rotation = oldRotation;
            gameObject->transform->scale = oldScale;
        }
    }

    void Redo() override
    {
        Execute();
    }

    std::string GetName() const override
    {
        return "Transform " + name;
    }

private:
    GameObject* gameObject = nullptr;
    std::string name;
    glm::vec3 oldPosition, oldRotation, oldScale;
    glm::vec3 newPosition, newRotation, newScale;
};

class ReplaceSceneCommand : public EditorCommand
{
public:
    ReplaceSceneCommand(const std::vector<GameObject*>& oldObjects, GameObject* newObj)
        : oldObjects(oldObjects), newObject(newObj), replaced(true)
    {
    }

    virtual ~ReplaceSceneCommand()
    {
        if (replaced)
        {
            for (GameObject* go : oldObjects)
            {
                if (go) delete go;
            }
            oldObjects.clear();
        }
        else
        {
            if (newObject)
            {
                delete newObject;
                newObject = nullptr;
            }
        }
    }

    void Execute() override
    {
        if (!replaced && App && App->renderer)
        {
            if (newObject)
            {
                App->renderer->AddGameObject(newObject, nullptr);
            }
            for (GameObject* go : oldObjects)
            {
                if (go) App->renderer->DetachGameObject(go);
            }
            replaced = true;
            if (newObject) App->renderer->SetSelectedGameObject(newObject);
        }
    }

    void Undo() override
    {
        if (replaced && App && App->renderer)
        {
            if (newObject)
            {
                App->renderer->DetachGameObject(newObject);
            }
            for (size_t i = 0; i < oldObjects.size(); ++i)
            {
                if (oldObjects[i])
                {
                    App->renderer->AddGameObject(oldObjects[i], nullptr, static_cast<int>(i));
                }
            }
            replaced = false;
        }
    }

    void Redo() override
    {
        Execute();
    }

    std::string GetName() const override
    {
        return newObject ? "Load " + newObject->GetName() : "Clear Scene";
    }

private:
    std::vector<GameObject*> oldObjects;
    GameObject* newObject = nullptr;
    bool replaced = true;
};


