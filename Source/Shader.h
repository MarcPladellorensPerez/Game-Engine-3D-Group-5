#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    Shader();
    ~Shader();

    bool LoadFromSource(const char* vertexSource, const char* fragmentSource);
    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);

    void Use() const;
    void Unuse() const;
    void CleanUp();

    unsigned int GetProgramID() const { return programID; }

    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
    bool CheckCompileErrors(unsigned int shader, const std::string& type);

private:
    unsigned int programID = 0;
};
