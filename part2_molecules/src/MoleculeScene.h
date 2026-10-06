// =============================================================================
// MoleculeScene.h - Ball-and-stick molecule scene builder
// =============================================================================
#pragma once
#include <string>
#include <memory>
#include <functional>
#include "SceneNode.h"
#include "AtomData.h"
#include "Mesh.h"

// Node that oscillates its children (molecular vibration)
class VibrationNode : public SceneNode {
public:
    float   vibFreq     = 2.0f;   // Hz
    float   vibAmp      = 0.06f;  // amplitude in world units
    float   vibTime     = 0.0f;
    int     vibAxis     = 0;      // 0=x,1=y,2=z
    bool    animating   = true;
    glm::vec3 basePos;

    void init(const glm::vec3& base) { basePos = base; position = base; }

    void update(float dt) override {
        vibTime += dt;
        if (animating) {
            float disp = vibAmp * std::sin(2.0f * (float)M_PI * vibFreq * vibTime);
            position = basePos;
            position[vibAxis] += disp;
        } else {
            position = basePos;
        }
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
    std::unique_ptr<MolRotNode>        root;
    std::vector<VibrationNode*>        vibNodes;  // for animation control

    bool vibrating = true;
    bool rotating  = false;

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
};
