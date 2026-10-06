// =============================================================================
// Mesh.h - Procedural mesh generation for spheres and cylinders
// =============================================================================
#pragma once
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

class Mesh {
public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh() {}

    void draw() const;
    void destroy();

    // Factories
    static Mesh makeSphere(int sectorCount = 36, int stackCount = 18);
    static Mesh makeCylinder(int sectorCount = 32, float radius = 0.5f, float height = 1.0f);
    static Mesh makeTorus(float majorR = 1.0f, float minorR = 0.15f,
                          int majorSeg = 48, int minorSeg = 16);
};
