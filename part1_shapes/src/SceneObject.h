// =============================================================================
// SceneObject.h - one drawable shape in the scene: geometry + transform + appearance
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Shader.h"          // shared code: common/src
#include "ShapeMesh.h"
#include "Surface.h"
#include "Texture.h"

enum class ShapeKind {
    // 2D
    Triangle, Rectangle, Pentagon, Hexagon, Circle, Ellipse, Trapezoid, Star, Arrow,
    // 3D
    Cube, Sphere, Cylinder, Cone, TruncatedCone, Tetrahedron, Torus, Prism,
    Surface, Model,
    Count
};

const char* shapeName(ShapeKind k);
bool        is2D(ShapeKind k);

enum class RenderMode { Flat, Gouraud, Phong, Texture, Wireframe };
const char* renderModeName(RenderMode m);

// Editable shape parameters (only the ones relevant to the object's kind are used).
struct ShapeParams {
    int   segments    = 48;     // circle, ellipse, sphere, cylinder, cone, truncated cone, torus (around the ring)
    int   stacks      = 24;     // sphere (latitude), torus (tube)
    int   prismSides  = 6;      // prism
    int   starPoints  = 5;      // star
    float rectWidth   = 1.0f,  rectHeight = 0.7f;           // rectangle
    float trapBottom  = 1.0f,  trapTop = 0.5f, trapHeight = 0.7f;   // trapezoid
    float radiusX     = 0.5f,  radiusY = 0.3f;              // ellipse
    float innerRatio  = 0.4f;   // star: inner / outer radius
    float topRadius   = 0.25f;  // truncated cone (bottom radius is 0.5)
    float minorRadius = 0.18f;  // torus tube radius (ring radius is 0.5)
    SurfaceParams surface;      // z = f(x, y)
    std::string   modelPath;    // imported .obj / .ply
};

// Everything the user can edit about an object; copyable (used for "duplicate").
struct ObjectState {
    std::string name;
    ShapeKind   kind = ShapeKind::Cube;
    ShapeParams params;

    // Transform
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};      // Euler angles in degrees (applied X, then Y, then Z)
    glm::vec3 scale{1.0f};

    // Appearance
    RenderMode  mode      = RenderMode::Phong;
    RenderMode  lastSolidMode = RenderMode::Phong;   // restored when wireframe is toggled off
    glm::vec3   color{0.90f, 0.55f, 0.20f};   // flat / Phong / wireframe color
    float       specular  = 0.5f;
    float       shininess = 64.0f;
    MeshUtil::ColorScheme vertexScheme = MeshUtil::ColorScheme::Rainbow;   // for Gouraud
    glm::vec3   gradientA{1.0f, 0.25f, 0.2f}, gradientB{0.2f, 0.4f, 1.0f};
    std::string texturePath;       // empty = built-in checkerboard
    float       uvScale   = 1.0f;

    bool visible = true;
};

class SceneObject {
public:
    ObjectState st;
    std::string error;                       // last build / texture error (shown in the GUI)
    std::shared_ptr<Texture2D> texture;      // set by Scene (shared through TextureCache)

    explicit SceneObject(ObjectState state) : st(std::move(state)) {}

    // (Re)generates the mesh from st.kind / st.params. On failure the previous mesh is kept.
    bool rebuild();
    // Re-applies the vertex color scheme (Rainbow / Gradient) to the mesh.
    void applyVertexColors();

    // T * R * S
    glm::mat4 modelMatrix() const;
    void draw(Shader& shader) const;
    // Yellow bounding box around the object (shown for the selected one). `unitBox` is a wire cube of side 1.
    void drawSelection(Shader& shader, const ShapeMesh& unitBox) const;

    bool hasMesh() const { return mesh_ != nullptr; }
    const ShapeMesh* mesh() const { return mesh_.get(); }

private:
    std::unique_ptr<ShapeMesh> mesh_;
};
