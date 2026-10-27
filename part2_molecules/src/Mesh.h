// =============================================================================
// Mesh.h - Procedural mesh generation for spheres, cylinders and tori
// =============================================================================
#pragma once
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// CPU-side geometry (no OpenGL): what the factories upload, also checked by the tests.
// Triangles are wound counter-clockwise when seen from outside (needed by back-face culling).
struct MeshData {
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
};

class Mesh {
public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    explicit Mesh(const MeshData& data) : Mesh(data.vertices, data.indices) {}
    ~Mesh() {}

    void draw() const;
    void destroy();

    // Geometry generators (pure CPU)
    static MeshData sphereData(int sectorCount, int stackCount);
    static MeshData cylinderData(int sectorCount, float radius, float height);
    static MeshData torusData(float majorR, float minorR, int majorSeg, int minorSeg);

    // Factories (generate + upload to the GPU)
    static Mesh makeSphere(int sectorCount = 36, int stackCount = 18);
    static Mesh makeCylinder(int sectorCount = 32, float radius = 0.5f, float height = 1.0f);
    static Mesh makeTorus(float majorR = 1.0f, float minorR = 0.15f,
                          int majorSeg = 48, int minorSeg = 16);
};
