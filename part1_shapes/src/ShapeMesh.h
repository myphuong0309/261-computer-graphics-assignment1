// =============================================================================
// ShapeMesh.h - GPU mesh (VAO/VBO/EBO) built from a MeshData
//
// Same role as part2_molecules' Mesh class, but with two extra vertex attributes
// (color, uv) and a second index buffer holding the wireframe edges.
// =============================================================================
#pragma once
#include <GL/glew.h>
#include "Geometry.h"

class ShapeMesh {
public:
    ShapeMesh() = default;
    explicit ShapeMesh(MeshData data);
    ~ShapeMesh() { destroy(); }

    ShapeMesh(const ShapeMesh&)            = delete;
    ShapeMesh& operator=(const ShapeMesh&) = delete;
    ShapeMesh(ShapeMesh&& o) noexcept { *this = std::move(o); }
    ShapeMesh& operator=(ShapeMesh&& o) noexcept;

    void draw()     const;   // filled triangles
    void drawWire() const;   // edges only (GL_LINES)

    // Re-colours the vertices and re-uploads the vertex buffer.
    void recolor(MeshUtil::ColorScheme scheme, const glm::vec3& a, const glm::vec3& b, int axis = -1);

    const MeshData& data() const { return data_; }
    void bounds(glm::vec3& lo, glm::vec3& hi) const;   // local-space bounding box
    void destroy();

private:
    void setupAttributes() const;

    MeshData data_;
    GLuint vao_ = 0, wireVao_ = 0, vbo_ = 0, ebo_ = 0, wireEbo_ = 0;
    GLsizei indexCount_ = 0, wireCount_ = 0;
};
