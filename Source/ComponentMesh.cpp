#include "ComponentMesh.h"
#include <glad/glad.h>
#include <cmath>

ComponentMesh::ComponentMesh(GameObject* owner)
    : Component(owner, ComponentType::MESH)
{
}

ComponentMesh::~ComponentMesh()
{
    CleanUp();
}

void ComponentMesh::CleanUp()
{
    if (VAO != 0) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO != 0) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (EBO != 0) { glDeleteBuffers(1, &EBO); EBO = 0; }

    if (vertexNormalsVAO != 0) { glDeleteVertexArrays(1, &vertexNormalsVAO); vertexNormalsVAO = 0; }
    if (vertexNormalsVBO != 0) { glDeleteBuffers(1, &vertexNormalsVBO); vertexNormalsVBO = 0; }

    if (faceNormalsVAO != 0) { glDeleteVertexArrays(1, &faceNormalsVAO); faceNormalsVAO = 0; }
    if (faceNormalsVBO != 0) { glDeleteBuffers(1, &faceNormalsVBO); faceNormalsVBO = 0; }

    vertices.clear();
    indices.clear();
    vertexNormalsCount = 0;
    faceNormalsCount = 0;
}

void ComponentMesh::InitBuffers(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds)
{
    CleanUp();

    vertices = verts;
    indices = inds;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty())
    {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    // Vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Vertex UV coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);

    GenerateNormalsBuffers();
}

void ComponentMesh::GenerateNormalsBuffers()
{
    if (vertices.empty()) return;

    const float normalLength = 0.2f;

    // 1. Vertex Normals (Cyan lines)
    std::vector<glm::vec3> vertNormalLines;
    vertNormalLines.reserve(vertices.size() * 2);
    for (const auto& v : vertices)
    {
        vertNormalLines.push_back(v.Position);
        vertNormalLines.push_back(v.Position + v.Normal * normalLength);
    }

    if (!vertNormalLines.empty())
    {
        glGenVertexArrays(1, &vertexNormalsVAO);
        glGenBuffers(1, &vertexNormalsVBO);

        glBindVertexArray(vertexNormalsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vertexNormalsVBO);
        glBufferData(GL_ARRAY_BUFFER, vertNormalLines.size() * sizeof(glm::vec3), vertNormalLines.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindVertexArray(0);
        vertexNormalsCount = vertNormalLines.size();
    }

    // 2. Face Normals (Yellow lines)
    std::vector<glm::vec3> faceNormalLines;
    size_t numTriangles = indices.empty() ? (vertices.size() / 3) : (indices.size() / 3);
    faceNormalLines.reserve(numTriangles * 2);

    for (size_t t = 0; t < numTriangles; ++t)
    {
        glm::vec3 p0, p1, p2;
        if (!indices.empty())
        {
            p0 = vertices[indices[t * 3 + 0]].Position;
            p1 = vertices[indices[t * 3 + 1]].Position;
            p2 = vertices[indices[t * 3 + 2]].Position;
        }
        else
        {
            p0 = vertices[t * 3 + 0].Position;
            p1 = vertices[t * 3 + 1].Position;
            p2 = vertices[t * 3 + 2].Position;
        }

        glm::vec3 center = (p0 + p1 + p2) / 3.0f;
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::cross(edge1, edge2);
        float len = glm::length(normal);
        if (len > 0.0001f)
            normal = normal / len;
        else
            normal = glm::vec3(0.0f, 1.0f, 0.0f);

        faceNormalLines.push_back(center);
        faceNormalLines.push_back(center + normal * normalLength);
    }

    if (!faceNormalLines.empty())
    {
        glGenVertexArrays(1, &faceNormalsVAO);
        glGenBuffers(1, &faceNormalsVBO);

        glBindVertexArray(faceNormalsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, faceNormalsVBO);
        glBufferData(GL_ARRAY_BUFFER, faceNormalLines.size() * sizeof(glm::vec3), faceNormalLines.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindVertexArray(0);
        faceNormalsCount = faceNormalLines.size();
    }
}

void ComponentMesh::Draw(unsigned int shaderProgram)
{
    if (VAO == 0 || vertices.empty()) return;

    glBindVertexArray(VAO);
    if (!indices.empty())
    {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    }
    glBindVertexArray(0);
}

void ComponentMesh::DrawNormals(unsigned int shaderProgram, const glm::mat4& modelMatrix)
{
    if (!showNormalsPerVertex && !showNormalsPerFace) return;

    // Use shader in flat color unlit mode
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useLighting"), 0);

    // Draw Vertex Normals in Cyan
    if (showNormalsPerVertex && vertexNormalsVAO != 0 && vertexNormalsCount > 0)
    {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 1.0f, 1.0f);
        glBindVertexArray(vertexNormalsVAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexNormalsCount));
        glBindVertexArray(0);
    }

    // Draw Face Normals in Yellow
    if (showNormalsPerFace && faceNormalsVAO != 0 && faceNormalsCount > 0)
    {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f);
        glBindVertexArray(faceNormalsVAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(faceNormalsCount));
        glBindVertexArray(0);
    }
}

// Procedural primitives
void ComponentMesh::CreateCube(float size)
{
    float half = size * 0.5f;
    meshName = "Cube";

    std::vector<Vertex> verts = {
        // Front face (+Z)
        { {-half, -half,  half}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },
        { { half, -half,  half}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
        { { half,  half,  half}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f} },
        { {-half,  half,  half}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f} },
        // Back face (-Z)
        { { half, -half, -half}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f} },
        { {-half, -half, -half}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f} },
        { {-half,  half, -half}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} },
        { { half,  half, -half}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} },
        // Left face (-X)
        { {-half, -half, -half}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },
        { {-half, -half,  half}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
        { {-half,  half,  half}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
        { {-half,  half, -half}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
        // Right face (+X)
        { { half, -half,  half}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },
        { { half, -half, -half}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
        { { half,  half, -half}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
        { { half,  half,  half}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
        // Top face (+Y)
        { {-half,  half,  half}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} },
        { { half,  half,  half}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f} },
        { { half,  half, -half}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f} },
        { {-half,  half, -half}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f} },
        // Bottom face (-Y)
        { {-half, -half, -half}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f} },
        { { half, -half, -half}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f} },
        { { half, -half,  half}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f} },
        { {-half, -half,  half}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f} },
    };

    std::vector<unsigned int> inds = {
        0, 1, 2,  0, 2, 3,        // Front
        4, 5, 6,  4, 6, 7,        // Back
        8, 9, 10, 8, 10, 11,      // Left
        12, 13, 14, 12, 14, 15,   // Right
        16, 17, 18, 16, 18, 19,   // Top
        20, 21, 22, 20, 22, 23    // Bottom
    };

    InitBuffers(verts, inds);
}

void ComponentMesh::CreatePlane(float width, float height)
{
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    meshName = "Plane";

    std::vector<Vertex> verts = {
        { {-halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
        { { halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
        { { halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { {-halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    };

    std::vector<unsigned int> inds = {
        0, 1, 2,  0, 2, 3
    };

    InitBuffers(verts, inds);
}

void ComponentMesh::CreateSphere(float radius, unsigned int rings, unsigned int sectors)
{
    meshName = "Sphere";
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (unsigned int r = 0; r < rings; ++r)
    {
        for (unsigned int s = 0; s < sectors; ++s)
        {
            float const y = sin(-M_PI_2 + M_PI * r * R);
            float const x = cos(2.0f * M_PI * s * S) * sin(M_PI * r * R);
            float const z = sin(2.0f * M_PI * s * S) * sin(M_PI * r * R);

            Vertex v;
            v.Position = glm::vec3(x * radius, y * radius, z * radius);
            v.Normal = glm::vec3(x, y, z);
            v.TexCoords = glm::vec2(s * S, r * R);
            verts.push_back(v);
        }
    }

    for (unsigned int r = 0; r < rings - 1; ++r)
    {
        for (unsigned int s = 0; s < sectors - 1; ++s)
        {
            unsigned int cur = r * sectors + s;
            unsigned int next = (r + 1) * sectors + s;

            inds.push_back(cur);
            inds.push_back(next);
            inds.push_back(next + 1);

            inds.push_back(cur);
            inds.push_back(next + 1);
            inds.push_back(cur + 1);
        }
    }

    InitBuffers(verts, inds);
}

void ComponentMesh::CreateCylinder(float radius, float height, unsigned int sectors)
{
    meshName = "Cylinder";
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    float halfH = height * 0.5f;

    // Side vertices
    for (unsigned int i = 0; i <= sectors; ++i)
    {
        float theta = (float)i / (float)sectors * 2.0f * M_PI;
        float x = cos(theta);
        float z = sin(theta);
        float u = (float)i / (float)sectors;

        // Top
        verts.push_back({ {x * radius, halfH, z * radius}, {x, 0.0f, z}, {u, 1.0f} });
        // Bottom
        verts.push_back({ {x * radius, -halfH, z * radius}, {x, 0.0f, z}, {u, 0.0f} });
    }

    for (unsigned int i = 0; i < sectors; ++i)
    {
        unsigned int top1 = i * 2;
        unsigned int bottom1 = top1 + 1;
        unsigned int top2 = (i + 1) * 2;
        unsigned int bottom2 = top2 + 1;

        inds.push_back(top1);
        inds.push_back(bottom1);
        inds.push_back(top2);

        inds.push_back(top2);
        inds.push_back(bottom1);
        inds.push_back(bottom2);
    }

    InitBuffers(verts, inds);
}

void ComponentMesh::CreatePyramid(float baseSize, float height)
{
    meshName = "Pyramid";
    float half = baseSize * 0.5f;

    std::vector<Vertex> verts = {
        // Front
        { { 0.0f, height,  0.0f}, { 0.0f, 0.5f,  1.0f}, {0.5f, 1.0f} },
        { {-half,   0.0f,  half}, { 0.0f, 0.5f,  1.0f}, {0.0f, 0.0f} },
        { { half,   0.0f,  half}, { 0.0f, 0.5f,  1.0f}, {1.0f, 0.0f} },

        // Right
        { { 0.0f, height,  0.0f}, { 1.0f, 0.5f,  0.0f}, {0.5f, 1.0f} },
        { { half,   0.0f,  half}, { 1.0f, 0.5f,  0.0f}, {0.0f, 0.0f} },
        { { half,   0.0f, -half}, { 1.0f, 0.5f,  0.0f}, {1.0f, 0.0f} },

        // Back
        { { 0.0f, height,  0.0f}, { 0.0f, 0.5f, -1.0f}, {0.5f, 1.0f} },
        { { half,   0.0f, -half}, { 0.0f, 0.5f, -1.0f}, {0.0f, 0.0f} },
        { {-half,   0.0f, -half}, { 0.0f, 0.5f, -1.0f}, {1.0f, 0.0f} },

        // Left
        { { 0.0f, height,  0.0f}, {-1.0f, 0.5f,  0.0f}, {0.5f, 1.0f} },
        { {-half,   0.0f, -half}, {-1.0f, 0.5f,  0.0f}, {0.0f, 0.0f} },
        { {-half,   0.0f,  half}, {-1.0f, 0.5f,  0.0f}, {1.0f, 0.0f} },

        // Base (2 triangles)
        { {-half,   0.0f, -half}, { 0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
        { { half,   0.0f, -half}, { 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
        { { half,   0.0f,  half}, { 0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-half,   0.0f,  half}, { 0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }
    };

    std::vector<unsigned int> inds = {
        0, 1, 2,      // Front
        3, 4, 5,      // Right
        6, 7, 8,      // Back
        9, 10, 11,    // Left
        12, 13, 14, 12, 14, 15 // Base
    };

    InitBuffers(verts, inds);
}

void ComponentMesh::CreateTriangle(float size)
{
    meshName = "Triangle";
    float half = size * 0.5f;

    std::vector<Vertex> verts = {
        { { 0.0f,  half, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f} },
        { {-half, -half, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
        { { half, -half, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }
    };

    std::vector<unsigned int> inds = { 0, 1, 2 };

    InitBuffers(verts, inds);
}
