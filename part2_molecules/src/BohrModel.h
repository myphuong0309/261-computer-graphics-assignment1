// =============================================================================
// BohrModel.h - Animated Bohr atomic model scene builder
// =============================================================================
#pragma once
#include <string>
#include <vector>
#include <memory>
#include "SceneNode.h"
#include "Mesh.h"

// Animated orbit node: rotates its children around an axis at given speed
class OrbitNode : public SceneNode {
public:
    float orbitSpeed = 90.0f;  // degrees per second
    glm::vec3 orbitAxis = glm::vec3(0, 1, 0);

    void update(float dt) override {
        rotAngle = std::fmod(rotAngle + orbitSpeed * dt, 360.0f);
        rotAxis  = orbitAxis;
    }
};

// Tilted shell pivot (for 3D look)
class ShellPivot : public SceneNode {
public:
    // static tilt, no update needed
};

// BohrModel: builds and owns the scene graph for one atom
class BohrAtom {
public:
    std::string        symbol;
    float              animSpeed = 1.0f;   // multiplier for all orbit speeds

    std::unique_ptr<SceneNode> root;       // scene root
    std::vector<OrbitNode*>    orbits;     // for animation toggling

    bool animating = true;

    // Build from element symbol; meshes provided externally (shared)
    void build(const std::string& sym, Mesh* sphereMesh, Mesh* torusMesh);

    // Update animation
    void update(float dt);

    // Draw
    void draw(Shader& shader) const;
};
