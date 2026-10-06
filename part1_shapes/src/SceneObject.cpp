// =============================================================================
// SceneObject.cpp
// =============================================================================
#include "SceneObject.h"
#include "Geometry.h"
#include "ModelLoader.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
const char* SHAPE_NAMES[] = {
    "Triangle", "Rectangle", "Pentagon", "Hexagon", "Circle", "Ellipse", "Trapezoid", "Star", "Arrow",
    "Cube", "Sphere", "Cylinder", "Cone", "Truncated Cone", "Tetrahedron", "Torus", "Prism",
    "Surface z=f(x,y)", "Model",
};

// Values of the "mode" uniform in shaders/shapes.frag
enum ShaderMode { SM_FLAT = 0, SM_GOURAUD = 1, SM_PHONG = 2, SM_TEXTURE = 3, SM_WIRE = 4 };
}

const char* shapeName(ShapeKind k) { return SHAPE_NAMES[(int)k]; }
bool is2D(ShapeKind k) { return (int)k <= (int)ShapeKind::Arrow; }

const char* renderModeName(RenderMode m) {
    switch (m) {
        case RenderMode::Flat:      return "Flat color";
        case RenderMode::Gouraud:   return "Gouraud (vertex colors)";
        case RenderMode::Phong:     return "Phong";
        case RenderMode::Texture:   return "Texture";
        case RenderMode::Wireframe: return "Wireframe";
    }
    return "?";
}

bool SceneObject::rebuild() {
    const ShapeParams& p = st.params;
    MeshData md;
    std::string err;

    switch (st.kind) {
        case ShapeKind::Triangle:      md = Geometry::regularPolygon(3);  break;
        case ShapeKind::Rectangle:     md = Geometry::rectangle(p.rectWidth, p.rectHeight); break;
        case ShapeKind::Pentagon:      md = Geometry::regularPolygon(5);  break;
        case ShapeKind::Hexagon:       md = Geometry::regularPolygon(6);  break;
        case ShapeKind::Circle:        md = Geometry::regularPolygon(p.segments); break;
        case ShapeKind::Ellipse:       md = Geometry::ellipse(p.radiusX, p.radiusY, p.segments); break;
        case ShapeKind::Trapezoid:     md = Geometry::trapezoid(p.trapBottom, p.trapTop, p.trapHeight); break;
        case ShapeKind::Star:          md = Geometry::star(p.starPoints, 0.5f, p.innerRatio); break;
        case ShapeKind::Arrow:         md = Geometry::arrow(); break;
        case ShapeKind::Cube:          md = Geometry::cube(); break;
        case ShapeKind::Sphere:        md = Geometry::sphere(p.segments, p.stacks); break;
        case ShapeKind::Cylinder:      md = Geometry::frustum(0.5f, 0.5f, 1.0f, p.segments); break;
        case ShapeKind::Cone:          md = Geometry::frustum(0.5f, 0.0f, 1.0f, p.segments); break;
        case ShapeKind::TruncatedCone: md = Geometry::frustum(0.5f, p.topRadius, 1.0f, p.segments); break;
        case ShapeKind::Tetrahedron:   md = Geometry::tetrahedron(); break;
        case ShapeKind::Torus:         md = Geometry::torus(0.5f, p.minorRadius, p.segments, p.stacks); break;
        case ShapeKind::Prism:         md = Geometry::prism(p.prismSides, 0.5f, 1.0f); break;
        case ShapeKind::Surface:
            if (!Surface::build(p.surface, md, err)) { error = err; return false; }
            break;
        case ShapeKind::Model:
            if (!ModelLoader::load(p.modelPath, md, err)) { error = err; return false; }
            break;
        default: error = "unknown shape"; return false;
    }
    mesh_ = std::make_unique<ShapeMesh>(std::move(md));
    error.clear();
    if (st.vertexScheme == MeshUtil::ColorScheme::Gradient) applyVertexColors();
    return true;
}

void SceneObject::applyVertexColors() {
    if (!mesh_) return;
    // Surfaces are colored by height (Y), everything else along the bounding-box diagonal.
    int axis = (st.kind == ShapeKind::Surface && st.vertexScheme == MeshUtil::ColorScheme::Rainbow) ? 1 : -1;
    mesh_->recolor(st.vertexScheme, st.gradientA, st.gradientB, axis);
}

glm::mat4 SceneObject::modelMatrix() const {
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(st.rotation.z), glm::vec3(0, 0, 1)) *
                  glm::rotate(glm::mat4(1.0f), glm::radians(st.rotation.y), glm::vec3(0, 1, 0)) *
                  glm::rotate(glm::mat4(1.0f), glm::radians(st.rotation.x), glm::vec3(1, 0, 0));
    return glm::translate(glm::mat4(1.0f), st.position) * R * glm::scale(glm::mat4(1.0f), st.scale);
}

void SceneObject::draw(Shader& shader) const {
    if (!st.visible || !mesh_) return;

    glm::mat4 model = modelMatrix();
    shader.setMat4("model", model);
    shader.setMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(model))));
    shader.setVec3("objectColor", st.color);
    shader.setFloat("specularStrength", st.specular);
    shader.setFloat("shininess", st.shininess);
    shader.setFloat("uvScale", st.uvScale);

    int mode = SM_PHONG;
    switch (st.mode) {
        case RenderMode::Flat:      mode = SM_FLAT;    break;
        case RenderMode::Gouraud:   mode = SM_GOURAUD; break;
        case RenderMode::Phong:     mode = SM_PHONG;   break;
        case RenderMode::Texture:   mode = SM_TEXTURE; break;
        case RenderMode::Wireframe: mode = SM_WIRE;    break;
    }
    shader.setInt("mode", mode);

    if (st.mode == RenderMode::Texture && texture) {
        texture->bind(0);
        shader.setInt("tex", 0);
    }
    if (st.mode == RenderMode::Wireframe) mesh_->drawWire();
    else                                  mesh_->draw();
}

void SceneObject::drawSelection(Shader& shader, const ShapeMesh& unitBox) const {
    if (!st.visible || !mesh_) return;
    glm::vec3 lo, hi;
    mesh_->bounds(lo, hi);
    glm::vec3 ext = glm::max(hi - lo, glm::vec3(0.002f));   // flat 2D shapes get a thin box
    glm::mat4 box = modelMatrix() * glm::translate(glm::mat4(1.0f), (lo + hi) * 0.5f) *
                    glm::scale(glm::mat4(1.0f), ext * 1.04f);
    shader.setMat4("model", box);
    shader.setMat3("normalMatrix", glm::mat3(1.0f));
    shader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.2f));
    shader.setInt("mode", SM_WIRE);
    unitBox.drawWire();
}
