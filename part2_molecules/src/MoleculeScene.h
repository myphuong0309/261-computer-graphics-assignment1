// =============================================================================
// MoleculeScene.h - Ball-and-stick molecule scene builder
// =============================================================================
#pragma once
#include <string>
#include <memory>
#include <functional>
#include <cmath>
#include "SceneNode.h"
#include "AtomData.h"
#include "Mesh.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Node that oscillates its children (molecular vibration).
// Symmetric stretching ("breathing") mode: the atom moves on the line through the centre of mass,
//   x(t) = (1 + A sin(2 pi f t)) x0,   x0 = equilibrium position measured from the centre of mass,
// so all atoms move in phase and the centre of mass never moves.
class VibrationNode : public SceneNode {
public:
    float   vibFreq     = 2.0f;   // Hz (displayed frequency)
    float   vibAmp      = 0.08f;  // relative amplitude A
    float   vibTime     = 0.0f;
    bool    animating   = true;
    glm::vec3 basePos   = glm::vec3(0.0f);

    void init(const glm::vec3& base) { basePos = base; position = base; }

    // Current stretch factor lambda(t) = 1 + A sin(2 pi f t)
    float stretch() const {
        return animating ? 1.0f + vibAmp * std::sin(2.0f * (float)M_PI * vibFreq * vibTime) : 1.0f;
    }

    void update(float dt) override {
        if (animating) vibTime += dt;
        position = basePos * stretch();
    }
};

// RotationNode: spins the whole molecule
class MolRotNode : public SceneNode {
public:
    float  rotSpeed   = 30.0f; // deg/s
    bool   animating  = false;

    void update(float dt) override {
        if (animating)
            rotAngle = std::fmod(rotAngle + rotSpeed * dt, 360.0f);
    }
};

class MoleculeScene {
public:
    // Layout constants. Lengths marked (A) are in Angstrom and are multiplied by `scale`.
    static constexpr float kFitRadius     = 2.2f;   // world radius of the sphere the molecule is fitted in
    static constexpr float kAtomScale     = 0.5f;   // ball radius = kAtomScale * covalent radius
    static constexpr float kBondRadius    = 0.07f;  // single bond (A)
    static constexpr float kStrandRadius  = 0.04f;  // one strand of a double / triple bond (A)
    static constexpr float kDoubleOffset  = 0.07f;  // strands of a double bond at +-offset (A)
    static constexpr float kTripleOffset  = 0.11f;  // strands of a triple bond at 0, +-offset (A)
    static constexpr float kHzPerWavenumber = 1e-3f; // shown vibration: 1000 cm^-1 -> 1 Hz

    std::unique_ptr<MolRotNode>        root;
    std::vector<VibrationNode*>        vibNodes;  // one per atom, for animation control

    bool  vibrating = true;
    bool  rotating  = false;
    float vibAmp    = 0.08f;   // relative amplitude of the stretch
    float vibSpeed  = 1.0f;    // multiplier of the displayed frequency
    float vibFreq   = 2.0f;    // displayed frequency (Hz) of the current molecule

    // Normalisation of the last build: world = scale * (p - centerOfMass)
    glm::vec3 centerOfMass = glm::vec3(0.0f);   // Angstrom
    float     scale        = 1.0f;              // world units per Angstrom

    // Build from molecule spec; meshes provided externally
    void build(const MoleculeSpec& spec, Mesh* sphereMesh, Mesh* cylMesh);

    void update(float dt);
    void draw(Shader& shader) const;

    void setVibrating(bool v) {
        vibrating = v;
        for (auto* n : vibNodes) n->animating = v;
    }
    void setRotating(bool r) {
        rotating = r;
        if (root) root->animating = r;
    }
    void setVibration(float amp, float speed);   // amplitude A and frequency multiplier

    // Place the unit cylinder mesh (radius 0.5, height 1, axis Y) so that it joins pa and pb
    static void placeCylinder(SceneNode& node, const glm::vec3& pa, const glm::vec3& pb, float radius);

private:
    // One cylinder of a bond: between atoms a and b, shifted sideways by `offset` (world units)
    struct Strand { SceneNode* node; int a, b; float offset; float radius; };
    std::vector<Strand> strands;
    float baseFreq = 2.0f;

    void placeStrands();   // re-place every bond cylinder from the current atom positions
};
