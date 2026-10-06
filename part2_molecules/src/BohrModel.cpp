// =============================================================================
// BohrModel.cpp - Build animated Bohr atom from AtomData
// =============================================================================
#include "BohrModel.h"
#include "AtomData.h"
#include <cmath>
#include <stdexcept>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void BohrAtom::build(const std::string& sym, Mesh* sphereMesh, Mesh* torusMesh) {
    symbol = sym;
    orbits.clear();
    const AtomInfo& info = AtomData::get(sym);

    root = std::make_unique<SceneNode>();

    // ----- Nucleus -----
    auto nucleus = std::make_unique<SceneNode>();
    nucleus->mesh      = sphereMesh;
    nucleus->scale     = glm::vec3(0.45f);
    nucleus->material.ambient   = info.color * 0.3f;
    nucleus->material.diffuse   = info.color;
    nucleus->material.specular  = glm::vec3(0.8f);
    nucleus->material.shininess = 128.0f;
    root->addChild(std::move(nucleus));

    // ----- Shells -----
    const ShellConfig& shells = info.shells;

    // Shell radii increase: 1.2, 2.1, 3.2, 4.5, ...
    static const float shellRadii[] = {1.2f, 2.1f, 3.2f, 4.5f, 5.9f, 7.4f, 9.0f};
    // Tilt angles for 3D look (so shells aren't all flat)
    static const float tiltAngles[] = {0.0f, 25.0f, 55.0f, 15.0f, 45.0f, 70.0f, 30.0f};
    // Orbit speeds (degrees/sec)
    static const float baseSpeeds[] = {90.0f, 60.0f, 45.0f, 35.0f, 28.0f, 22.0f, 18.0f};

    for (int shellIdx = 0; shellIdx < (int)shells.size(); ++shellIdx) {
        int   nElectrons = shells[shellIdx];
        float r          = shellRadii[shellIdx];
        float tilt       = tiltAngles[shellIdx];

        // ---- Create a tiltPivot that owns BOTH the ring AND all electron orbits ----
        // This ensures electrons travel in the same tilted plane as the torus.
        auto tiltPivot = std::make_unique<SceneNode>();
        tiltPivot->rotAxis  = glm::vec3(1, 0, 0); // tilt around world X
        tiltPivot->rotAngle = tilt;

        // ---- Torus ring ----
        if (torusMesh) {
            auto ring = std::make_unique<SceneNode>();
            ring->mesh  = torusMesh;
            // The torus was built with majorR=1, so scale uniformly to shell radius r.
            // torus lies in local XY plane — perfect, OrbitNode will spin in XY too.
            ring->scale = glm::vec3(r);
            ring->material.ambient   = glm::vec3(0.05f);
            ring->material.diffuse   = glm::vec3(0.3f, 0.4f, 0.5f);
            ring->material.specular  = glm::vec3(0.1f);
            ring->material.shininess = 16.0f;
            tiltPivot->addChild(std::move(ring));
        }

        // ---- Electrons, equally spaced with initial phase ----
        // Inside tiltPivot's local space:
        //   • torus lies in the local XY plane (theta = angle in XY)
        //   • OrbitNode rotates around local Z → electron sweeps XY plane → on the torus ✓
        for (int eIdx = 0; eIdx < nElectrons; ++eIdx) {
            float phase = (float)(360.0 * eIdx / nElectrons); // degrees, evenly spaced

            auto orbitPivot = std::make_unique<OrbitNode>();
            // Rotate around local Z (perpendicular to the torus plane)
            orbitPivot->orbitAxis  = glm::vec3(0.0f, 0.0f, 1.0f);
            orbitPivot->orbitSpeed = baseSpeeds[shellIdx] * animSpeed;
            orbitPivot->rotAngle   = phase; // initial phase so electrons are spread out
            orbits.push_back(orbitPivot.get());

            // Electron sphere: placed at (r, 0, 0) in local space.
            // When orbitPivot rotates around Z, it sweeps the XY circle of radius r.
            auto electron = std::make_unique<SceneNode>();
            electron->mesh      = sphereMesh;
            electron->scale     = glm::vec3(0.1f);
            electron->position  = glm::vec3(r, 0.0f, 0.0f);
            electron->material.ambient   = glm::vec3(0.3f, 0.3f, 0.0f);
            electron->material.diffuse   = glm::vec3(1.0f, 1.0f, 0.0f);
            electron->material.specular  = glm::vec3(1.0f);
            electron->material.shininess = 128.0f;

            orbitPivot->addChild(std::move(electron));
            tiltPivot->addChild(std::move(orbitPivot));  // ← child of tiltPivot, not root!
        }

        root->addChild(std::move(tiltPivot));
    }
}


void BohrAtom::update(float dt) {
    if (!animating) return;
    // Walk all nodes recursively
    std::function<void(SceneNode*, float)> walk = [&](SceneNode* node, float dt) {
        node->update(dt);
        for (auto& c : node->children) walk(c.get(), dt);
    };
    if (root) walk(root.get(), dt);
}

void BohrAtom::draw(Shader& shader) const {
    if (root) root->draw(shader, glm::mat4(1.0f));
}
