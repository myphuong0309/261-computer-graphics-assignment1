// =============================================================================
// BohrModel.cpp - Build animated Bohr atom from AtomData
// =============================================================================
#include "BohrModel.h"
#include "AtomData.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Shell radii increase: 1.2, 2.1, 3.2, 4.5, ...  (r_k = 0.1k^2 + 0.6k + 0.5 for k = 1..4)
static const float shellRadii[] = {1.2f, 2.1f, 3.2f, 4.5f, 5.9f, 7.4f, 9.0f};
// Tilt angles for 3D look (so shells aren't all flat); in the default view (pitch 30 deg) ring k appears
// as an ellipse of axis ratio |cos(tilt + 30 deg)|: K 0.87, L 0.57, M 0.34, N 0.71 (none edge-on)
static const float tiltAngles[] = {0.0f, 25.0f, 40.0f, 15.0f, 45.0f, 70.0f, 30.0f};
// Orbit speeds (degrees/sec): inner shells turn faster
static const float baseSpeeds[] = {90.0f, 60.0f, 45.0f, 35.0f, 28.0f, 22.0f, 18.0f};

float BohrAtom::shellRadius(int shell) { return shellRadii[std::min(std::max(shell, 0), kMaxShells - 1)]; }
float BohrAtom::shellTilt(int shell)   { return tiltAngles[std::min(std::max(shell, 0), kMaxShells - 1)]; }
float BohrAtom::shellSpeed(int shell)  { return baseSpeeds[std::min(std::max(shell, 0), kMaxShells - 1)]; }

void BohrAtom::build(const std::string& sym, Mesh* sphereMesh, const std::vector<Mesh*>& ringMeshes) {
    symbol = sym;
    orbits.clear();
    const AtomInfo& info = AtomData::get(sym);

    root = std::make_unique<SceneNode>();

    // ----- Nucleus -----
    auto nucleus = std::make_unique<SceneNode>();
    nucleus->mesh      = sphereMesh;
    nucleus->scale     = glm::vec3(kNucleusRadius);
    nucleus->material.ambient   = info.color * 0.3f;
    nucleus->material.diffuse   = info.color;
    nucleus->material.specular  = glm::vec3(0.8f);
    nucleus->material.shininess = 128.0f;
    root->addChild(std::move(nucleus));

    // ----- Shells -----
    const ShellConfig& shells = info.shells;
    shellCount = std::min((int)shells.size(), kMaxShells);

    for (int shellIdx = 0; shellIdx < shellCount; ++shellIdx) {
        int   nElectrons = shells[shellIdx];
        float r          = shellRadii[shellIdx];
        float tilt       = tiltAngles[shellIdx];

        // ---- Create a tiltPivot that owns BOTH the ring AND all electron orbits ----
        // This ensures electrons travel in the same tilted plane as the torus.
        auto tiltPivot = std::make_unique<ShellPivot>();
        tiltPivot->rotAxis  = glm::vec3(1, 0, 0); // tilt around world X
        tiltPivot->rotAngle = tilt;

        // ---- Torus ring ----
        // Each shell has its own torus built with major radius r and a constant tube radius.
        // (Scaling one unit torus by r would also scale its tube: with tube 0.025 the N shell
        //  ring would be 0.1125 thick and completely hide the 0.1 electrons inside it.)
        Mesh* ringMesh = shellIdx < (int)ringMeshes.size() ? ringMeshes[shellIdx] : nullptr;
        if (ringMesh) {
            auto ring = std::make_unique<SceneNode>();
            ring->mesh  = ringMesh;
            // torus lies in local XY plane — perfect, OrbitNode will spin in XY too.
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
            orbitPivot->rotAxis    = orbitPivot->orbitAxis; // correct even before the first update (paused)
            orbitPivot->baseSpeed  = baseSpeeds[shellIdx];
            orbitPivot->orbitSpeed = baseSpeeds[shellIdx] * animSpeed;
            orbitPivot->rotAngle   = phase; // initial phase so electrons are spread out
            orbits.push_back(orbitPivot.get());

            // Electron sphere: placed at (r, 0, 0) in local space.
            // When orbitPivot rotates around Z, it sweeps the XY circle of radius r.
            auto electron = std::make_unique<SceneNode>();
            electron->mesh      = sphereMesh;
            electron->scale     = glm::vec3(kElectronRadius);
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

void BohrAtom::setAnimSpeed(float s) {
    animSpeed = s;
    for (OrbitNode* o : orbits) o->orbitSpeed = o->baseSpeed * s;
}

float BohrAtom::outerRadius() const {
    return shellCount > 0 ? shellRadii[shellCount - 1] : kNucleusRadius;
}

void BohrAtom::update(float dt) {
    if (!animating) return;
    if (root) root->updateTree(dt);
}

void BohrAtom::draw(Shader& shader) const {
    if (root) root->draw(shader, glm::mat4(1.0f));
}
