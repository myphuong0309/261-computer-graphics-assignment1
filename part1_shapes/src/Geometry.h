// =============================================================================
// Geometry.h - CPU-side mesh data + procedural generators for 2D / 3D shapes
//
// Every generator returns a MeshData (positions, normals, per-vertex colors,
// texture coordinates, triangle indices and a "wire" edge list). The data is
// uploaded to the GPU by ShapeMesh. Nothing in here touches OpenGL, so the
// generators can be unit-tested without a window.
//
// The sphere / cylinder / torus loops are adapted from part2_molecules/Mesh.cpp
// (Y-up here, plus texture coordinates and vertex colors, which Part 2's Mesh
// does not store).
// =============================================================================
#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    glm::vec3 color{1.0f};
    glm::vec2 uv{0.0f};
};

struct MeshData {
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;      // triangle list
    std::vector<unsigned int> wireIndices;  // line list (pairs) - the "edges" of the shape
};

namespace Geometry {

// ---------------------------------------------------------------- 2D shapes
// All 2D shapes lie in the XY plane (normal +Z), roughly 1 unit wide, centred at the origin.
MeshData regularPolygon(int sides, float radius = 0.5f);   // triangle, pentagon, hexagon, circle...
MeshData rectangle(float width, float height);
MeshData ellipse(float radiusX, float radiusY, int segments);
MeshData trapezoid(float bottomWidth, float topWidth, float height);
MeshData star(int points, float outerRadius, float innerRatio);
MeshData arrow();

// ---------------------------------------------------------------- 3D solids
// All solids are centred at the origin, Y is "up", roughly 1 unit in size.
MeshData cube(float size = 1.0f);
MeshData sphere(int sectors, int stacks, float radius = 0.5f);
MeshData frustum(float bottomRadius, float topRadius, float height, int segments); // cylinder / cone / truncated cone
MeshData tetrahedron(float edge = 1.2f);
MeshData torus(float majorRadius, float minorRadius, int majorSegments, int minorSegments);
MeshData prism(int sides, float radius, float height);     // regular n-gon extruded along Y

} // namespace Geometry

namespace MeshUtil {

enum class ColorScheme { Rainbow, Gradient };

// Area-weighted smooth normals (vertices at the same position are merged, so UV seams stay smooth).
void computeSmoothNormals(MeshData& m);

// Fills Vertex::color. axis: 0/1/2 = x/y/z, -1 = diagonal of the bounding box.
void assignVertexColors(MeshData& m, ColorScheme scheme,
                        const glm::vec3& a = glm::vec3(1.0f, 0.2f, 0.2f),
                        const glm::vec3& b = glm::vec3(0.2f, 0.4f, 1.0f),
                        int axis = -1);

// Recentres on the bounding-box centre and scales so the largest extent equals `size`.
void normalizeToUnit(MeshData& m, float size = 1.0f);

// Builds wireIndices: boundary edges + edges between non-coplanar triangles
// (so the invisible diagonals of quads, caps and flat faces are not drawn).
void buildWireIndices(MeshData& m);

// Planar UV mapping of the XY bounding box (used by the 2D shapes).
void planarUV(MeshData& m);

glm::vec3 hsv2rgb(float h, float s, float v);

} // namespace MeshUtil
