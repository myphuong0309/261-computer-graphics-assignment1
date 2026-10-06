// =============================================================================
// ShapeMesh.cpp
// =============================================================================
#include "ShapeMesh.h"
#include <cstddef>
#include <utility>

ShapeMesh::ShapeMesh(MeshData data) : data_(std::move(data)) {
    indexCount_ = (GLsizei)data_.indices.size();
    wireCount_  = (GLsizei)data_.wireIndices.size();

    glGenVertexArrays(1, &vao_);
    glGenVertexArrays(1, &wireVao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glGenBuffers(1, &wireEbo_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, data_.vertices.size() * sizeof(Vertex), data_.vertices.data(), GL_DYNAMIC_DRAW);

    // Triangle VAO
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_.indices.size() * sizeof(unsigned int), data_.indices.data(), GL_STATIC_DRAW);
    setupAttributes();

    // Wire VAO (shares the vertex buffer)
    glBindVertexArray(wireVao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_.wireIndices.size() * sizeof(unsigned int), data_.wireIndices.data(), GL_STATIC_DRAW);
    setupAttributes();

    glBindVertexArray(0);
}

void ShapeMesh::setupAttributes() const {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
}

ShapeMesh& ShapeMesh::operator=(ShapeMesh&& o) noexcept {
    if (this != &o) {
        destroy();
        data_ = std::move(o.data_);
        vao_ = o.vao_; wireVao_ = o.wireVao_; vbo_ = o.vbo_; ebo_ = o.ebo_; wireEbo_ = o.wireEbo_;
        indexCount_ = o.indexCount_; wireCount_ = o.wireCount_;
        o.vao_ = o.wireVao_ = o.vbo_ = o.ebo_ = o.wireEbo_ = 0;
        o.indexCount_ = o.wireCount_ = 0;
    }
    return *this;
}

void ShapeMesh::draw() const {
    if (!vao_) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ShapeMesh::drawWire() const {
    if (!wireVao_) return;
    glBindVertexArray(wireVao_);
    glDrawElements(GL_LINES, wireCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ShapeMesh::recolor(MeshUtil::ColorScheme scheme, const glm::vec3& a, const glm::vec3& b, int axis) {
    if (!vbo_) return;
    MeshUtil::assignVertexColors(data_, scheme, a, b, axis);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data_.vertices.size() * sizeof(Vertex), data_.vertices.data());
}

void ShapeMesh::bounds(glm::vec3& lo, glm::vec3& hi) const {
    lo = glm::vec3(0.0f); hi = glm::vec3(0.0f);
    if (data_.vertices.empty()) return;
    lo = hi = data_.vertices[0].position;
    for (const auto& v : data_.vertices) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
}

void ShapeMesh::destroy() {
    if (vao_)     { glDeleteVertexArrays(1, &vao_);     vao_ = 0; }
    if (wireVao_) { glDeleteVertexArrays(1, &wireVao_); wireVao_ = 0; }
    if (vbo_)     { glDeleteBuffers(1, &vbo_);          vbo_ = 0; }
    if (ebo_)     { glDeleteBuffers(1, &ebo_);          ebo_ = 0; }
    if (wireEbo_) { glDeleteBuffers(1, &wireEbo_);      wireEbo_ = 0; }
}
