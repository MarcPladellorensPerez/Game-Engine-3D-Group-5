#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Texture.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    // mesh data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(unsigned int shaderProgram) const;
    void CleanUp();

private:
    // render data
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};
