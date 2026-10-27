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
// UV Sphere (unit radius, poles on the Z axis)
// ---------------------------------------------------------------------------
MeshData Mesh::sphereData(int sectorCount, int stackCount) {
    MeshData d;
    std::vector<Vertex>&       verts = d.vertices;
    std::vector<unsigned int>& idxs  = d.indices;

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
            if (i != 0) {                 // the first stack touches the north pole (degenerate quad)
                idxs.push_back(k1); idxs.push_back(k2); idxs.push_back(k1 + 1);
            }
            if (i != stackCount - 1) {    // the last stack touches the south pole
                idxs.push_back(k1 + 1); idxs.push_back(k2); idxs.push_back(k2 + 1);
            }
        }
    }
    return d;
}

// ---------------------------------------------------------------------------
// Cylinder — Y-axis aligned: axis runs from (0,-hHalf,0) to (0,+hHalf,0)
// This matches MoleculeScene's rotation which brings Y -> bond direction.
// ---------------------------------------------------------------------------
MeshData Mesh::cylinderData(int sectorCount, float radius, float height) {
    MeshData d;
    std::vector<Vertex>&       verts = d.vertices;
    std::vector<unsigned int>& idxs  = d.indices;

    float step  = (float)(2.0 * M_PI / sectorCount);
    float hHalf = height * 0.5f;

    // Side vertices: circle in XZ plane, axis along Y (bottom at 2i, top at 2i+1)
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

    // Side indices: (b_i, t_i, b_i+1) and (t_i, t_i+1, b_i+1) -> counter-clockwise from outside
    for (int i = 0; i < sectorCount; ++i) {
        int k0 = i * 2;
        idxs.push_back(k0);     idxs.push_back(k0 + 1); idxs.push_back(k0 + 2);
        idxs.push_back(k0 + 1); idxs.push_back(k0 + 3); idxs.push_back(k0 + 2);
    }

    // Cap centers (bottom -Y, top +Y)
    int baseCenterIdx = (int)verts.size();
    Vertex bc, tc;
    bc.position = glm::vec3(0, -hHalf, 0); bc.normal = glm::vec3(0, -1, 0);
    tc.position = glm::vec3(0,  hHalf, 0); tc.normal = glm::vec3(0,  1, 0);
    verts.push_back(bc);
    verts.push_back(tc);

    // Cap rims get their own vertices because their normal is +-Y, not the side normal
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
        // Bottom cap: counter-clockwise seen from below (-Y)
        idxs.push_back(baseCenterIdx);     idxs.push_back(k);     idxs.push_back(k + 2);
        // Top cap: counter-clockwise seen from above (+Y)
        idxs.push_back(baseCenterIdx + 1); idxs.push_back(k + 3); idxs.push_back(k + 1);
    }

    return d;
}

// ---------------------------------------------------------------------------
// Torus in the XY plane (for orbital rings)
// ---------------------------------------------------------------------------
MeshData Mesh::torusData(float majorR, float minorR, int majorSeg, int minorSeg) {
    MeshData d;
    std::vector<Vertex>&       verts = d.vertices;
    std::vector<unsigned int>& idxs  = d.indices;

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
    return d;
}

Mesh Mesh::makeSphere(int sectorCount, int stackCount) {
    return Mesh(sphereData(sectorCount, stackCount));
}

Mesh Mesh::makeCylinder(int sectorCount, float radius, float height) {
    return Mesh(cylinderData(sectorCount, radius, height));
}

Mesh Mesh::makeTorus(float majorR, float minorR, int majorSeg, int minorSeg) {
    return Mesh(torusData(majorR, minorR, majorSeg, minorSeg));
}
