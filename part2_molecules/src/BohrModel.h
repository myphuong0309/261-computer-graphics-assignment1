// =============================================================================
// BohrModel.h - Animated Bohr atomic model scene builder
// =============================================================================
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include "SceneNode.h"
#include "Mesh.h"

// Animated orbit node: rotates its children around an axis at given speed
class OrbitNode : public SceneNode {
public:
    float baseSpeed  = 90.0f;  // degrees per second at speed multiplier 1
    float orbitSpeed = 90.0f;  // degrees per second actually used (baseSpeed * multiplier)
    glm::vec3 orbitAxis = glm::vec3(0, 0, 1);

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
    // Visual constants (world units)
    static constexpr int   kMaxShells      = 7;
    static constexpr float kNucleusRadius  = 0.45f;
    static constexpr float kElectronRadius = 0.10f;
    static constexpr float kRingTubeRadius = 0.03f;   // < kElectronRadius, so electrons stay visible

    // Per-shell layout (shell index 0 = K, 1 = L, ...)
    static float shellRadius(int shell);   // orbit radius r_k
    static float shellTilt(int shell);     // tilt of the orbit plane around X (degrees)
    static float shellSpeed(int shell);    // angular speed at multiplier 1 (degrees / s)

    std::string        symbol;
    float              animSpeed = 1.0f;   // multiplier for all orbit speeds

    std::unique_ptr<SceneNode> root;       // scene root
    std::vector<OrbitNode*>    orbits;     // for animation toggling

    bool animating = true;

    // Build from element symbol; meshes provided externally (shared).
    // ringMeshes[k] is a torus of major radius shellRadius(k) and tube kRingTubeRadius.
    void build(const std::string& sym, Mesh* sphereMesh, const std::vector<Mesh*>& ringMeshes);

    // Change the speed multiplier without rebuilding (electrons keep their phase)
    void setAnimSpeed(float s);

    // Radius of the outermost shell (for fitting the camera)
    float outerRadius() const;

    // Update animation
    void update(float dt);

    // Draw
    void draw(Shader& shader) const;

private:
    int shellCount = 0;
};
