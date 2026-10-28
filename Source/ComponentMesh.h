#pragma once
#include "Component.h"
#include "Mesh.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

class ComponentMesh : public Component {
public:
    ComponentMesh(GameObject* owner);
    virtual ~ComponentMesh();

    void InitBuffers(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds);
    void Draw(unsigned int shaderProgram);
    void DrawNormals(unsigned int shaderProgram, const glm::mat4& modelMatrix);
    void CleanUp() override;

    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }
    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }
    size_t GetTriangleCount() const { return indices.empty() ? (vertices.size() / 3) : (indices.size() / 3); }

    std::string meshName = "Mesh";
    bool showNormalsPerFace = false;
    bool showNormalsPerVertex = false;

    // Basic procedural geometries
    void CreateCube(float size = 1.0f);
    void CreatePlane(float width = 2.0f, float height = 2.0f);
    void CreateSphere(float radius = 1.0f, unsigned int rings = 16, unsigned int sectors = 16);
    void CreateCylinder(float radius = 0.5f, float height = 2.0f, unsigned int sectors = 16);
    void CreatePyramid(float baseSize = 1.0f, float height = 1.5f);
    void CreateTriangle(float size = 1.0f);

private:
    void GenerateNormalsBuffers();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO = 0, VBO = 0, EBO = 0;

    // Normal visualization buffers
    unsigned int vertexNormalsVAO = 0, vertexNormalsVBO = 0;
    size_t vertexNormalsCount = 0;

    unsigned int faceNormalsVAO = 0, faceNormalsVBO = 0;
    size_t faceNormalsCount = 0;
};
