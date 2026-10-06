// =============================================================================
// Mesh.cpp - Procedural mesh generation
// =============================================================================
#include "Mesh.h"
#include <cmath>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
}

// ---------------------------------------------------------------------------
// UV Sphere
// ---------------------------------------------------------------------------
Mesh Mesh::makeSphere(int sectorCount, int stackCount) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> idxs;

    float sectorStep = (float)(2.0 * M_PI / sectorCount);
    float stackStep  = (float)(M_PI / stackCount);

    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = (float)(M_PI / 2.0 - i * stackStep);
        float xy = std::cos(stackAngle);
        float z  = std::sin(stackAngle);

        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * sectorStep;
            float x = xy * std::cos(sectorAngle);
            float y = xy * std::sin(sectorAngle);
            Vertex v;
            v.position = glm::vec3(x, y, z);
            v.normal   = glm::vec3(x, y, z); // unit sphere: normal == position
            verts.push_back(v);
        }
    }

    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                idxs.push_back(k1); idxs.push_back(k2); idxs.push_back(k1 + 1);
            }
            if (i != stackCount - 1) {
                idxs.push_back(k1 + 1); idxs.push_back(k2); idxs.push_back(k2 + 1);
            }
        }
    }
    return Mesh(verts, idxs);
}

// ---------------------------------------------------------------------------
// Cylinder — Y-axis aligned: axis runs from (0,-hHalf,0) to (0,+hHalf,0)
// This matches MoleculeScene's rotation which brings Y -> bond direction.
// ---------------------------------------------------------------------------
Mesh Mesh::makeCylinder(int sectorCount, float radius, float height) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> idxs;

    float step  = (float)(2.0 * M_PI / sectorCount);
    float hHalf = height * 0.5f;

    // Side vertices: circle in XZ plane, axis along Y
    for (int i = 0; i <= sectorCount; ++i) {
        float angle = i * step;
        float cx    = radius * std::cos(angle);
        float cz    = radius * std::sin(angle);
        glm::vec3 norm(std::cos(angle), 0.0f, std::sin(angle));

        Vertex vb, vt;
        vb.position = glm::vec3(cx, -hHalf, cz); vb.normal = norm;
        vt.position = glm::vec3(cx,  hHalf, cz); vt.normal = norm;
        verts.push_back(vb);
        verts.push_back(vt);
    }

    // Side indices
    for (int i = 0; i < sectorCount; ++i) {
        int k0 = i * 2;
        idxs.push_back(k0);     idxs.push_back(k0 + 2); idxs.push_back(k0 + 1);
        idxs.push_back(k0 + 1); idxs.push_back(k0 + 2); idxs.push_back(k0 + 3);
    }

    // Cap centers (bottom -Y, top +Y)
    int baseCenterIdx = (int)verts.size();
    Vertex bc, tc;
    bc.position = glm::vec3(0, -hHalf, 0); bc.normal = glm::vec3(0, -1, 0);
    tc.position = glm::vec3(0,  hHalf, 0); tc.normal = glm::vec3(0,  1, 0);
    verts.push_back(bc);
    verts.push_back(tc);

    int capStart = (int)verts.size();
    for (int i = 0; i <= sectorCount; ++i) {
        float angle = i * step;
        float cx = radius * std::cos(angle);
        float cz = radius * std::sin(angle);
        Vertex vb2, vt2;
        vb2.position = glm::vec3(cx, -hHalf, cz); vb2.normal = glm::vec3(0, -1, 0);
        vt2.position = glm::vec3(cx,  hHalf, cz); vt2.normal = glm::vec3(0,  1, 0);
        verts.push_back(vb2);
        verts.push_back(vt2);
    }

    for (int i = 0; i < sectorCount; ++i) {
        int k = capStart + i * 2;
        // Bottom cap (winding: CCW from below = CW from outside)
        idxs.push_back(baseCenterIdx);     idxs.push_back(k + 2); idxs.push_back(k);
        // Top cap
        idxs.push_back(baseCenterIdx + 1); idxs.push_back(k + 1); idxs.push_back(k + 3);
    }

    return Mesh(verts, idxs);
}

// ---------------------------------------------------------------------------
// Torus (for orbital rings)
// ---------------------------------------------------------------------------
Mesh Mesh::makeTorus(float majorR, float minorR, int majorSeg, int minorSeg) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> idxs;

    for (int i = 0; i <= majorSeg; ++i) {
        float theta = (float)(2.0 * M_PI * i / majorSeg);
        float cosT  = std::cos(theta), sinT = std::sin(theta);

        for (int j = 0; j <= minorSeg; ++j) {
            float phi  = (float)(2.0 * M_PI * j / minorSeg);
            float cosP = std::cos(phi), sinP = std::sin(phi);

            float x = (majorR + minorR * cosP) * cosT;
            float y = (majorR + minorR * cosP) * sinT;
            float z = minorR * sinP;

            glm::vec3 pos(x, y, z);
            glm::vec3 center(majorR * cosT, majorR * sinT, 0.0f);
            glm::vec3 norm = glm::normalize(pos - center);

            Vertex v; v.position = pos; v.normal = norm;
            verts.push_back(v);
        }
    }

    for (int i = 0; i < majorSeg; ++i) {
        for (int j = 0; j < minorSeg; ++j) {
            int a = i * (minorSeg + 1) + j;
            int b = a + minorSeg + 1;
            idxs.push_back(a); idxs.push_back(b); idxs.push_back(a + 1);
            idxs.push_back(b); idxs.push_back(b + 1); idxs.push_back(a + 1);
        }
    }
    return Mesh(verts, idxs);
}
