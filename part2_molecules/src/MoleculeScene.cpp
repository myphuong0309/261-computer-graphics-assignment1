// =============================================================================
// MoleculeScene.cpp - Ball-and-stick molecule rendering
// =============================================================================
#include "MoleculeScene.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void MoleculeScene::build(const MoleculeSpec& spec, Mesh* sphereMesh, Mesh* cylMesh) {
    vibNodes.clear();
    root = std::make_unique<MolRotNode>();
    root->rotAxis = glm::vec3(0, 1, 0);

    // Scale factor — map angstrom coordinates to ~1-3 world unit range
    float maxR = 0.0f;
    for (const auto& a : spec.atoms)
        maxR = std::max(maxR, glm::length(a.position));
    float scale = (maxR > 0.01f) ? (2.0f / maxR) : 1.0f;

    // Build atom nodes (with vibration)
    std::vector<glm::vec3> worldPos;
    for (int i = 0; i < (int)spec.atoms.size(); ++i) {
        const auto& aspec = spec.atoms[i];
        const AtomInfo& info = AtomData::get(aspec.symbol);

        glm::vec3 pos = aspec.position * scale;
        worldPos.push_back(pos);

        auto vibNode = std::make_unique<VibrationNode>();
        vibNode->init(pos);
        vibNode->vibAxis = i % 3;
        vibNode->vibFreq = 2.0f + i * 0.5f;
        vibNode->vibAmp  = 0.05f;
        vibNode->animating = vibrating;

        auto atom = std::make_unique<SceneNode>();
        atom->mesh  = sphereMesh;
        atom->scale = glm::vec3(info.radius * 0.5f); // visual radius
        atom->material.ambient   = info.color * 0.25f;
        atom->material.diffuse   = info.color;
        atom->material.specular  = glm::vec3(0.6f);
        atom->material.shininess = 80.0f;

        vibNode->addChild(std::move(atom));
        vibNodes.push_back(vibNode.get());
        root->addChild(std::move(vibNode));
    }

    // Bond cylinders
    static const float bondR   = 0.06f; // visual bond radius
    static const float dblOff  = 0.09f; // offset for double bond strands

    for (const auto& bond : spec.bonds) {
        glm::vec3 a = worldPos[bond.a];
        glm::vec3 b = worldPos[bond.b];

        auto addBondCyl = [&](glm::vec3 pa, glm::vec3 pb) {
            auto bondNode = std::make_unique<SceneNode>();
            bondNode->mesh = cylMesh;

            // Compute transform
            glm::vec3 dir = pb - pa;
            float len = glm::length(dir);
            glm::vec3 up(0, 1, 0);
            glm::vec3 norm = (len > 1e-6f) ? glm::normalize(dir) : up;
            glm::vec3 axis = glm::cross(up, norm);
            float sinA = glm::length(axis);
            float cosA = glm::dot(up, norm);

            glm::mat4 R(1.0f);
            if (sinA > 1e-6f)
                R = glm::rotate(glm::mat4(1.0f), std::atan2(sinA, cosA), glm::normalize(axis));
            else if (cosA < 0.0f)
                R = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1,0,0));

            glm::vec3 mid = (pa + pb) * 0.5f;

            bondNode->position = mid;
            // Encode rotation into rotAxis/rotAngle (approximate: use decomposition)
            // Instead we directly bake into a transform; use position + custom mat
            // For simplicity: use the node's rotAngle/rotAxis from R decomposition
            // and scale Y by len, XZ by bondR
            bondNode->scale = glm::vec3(bondR, len, bondR);

            // Extract axis-angle from R
            glm::mat4 Rmat = R;
            float traceVal = Rmat[0][0] + Rmat[1][1] + Rmat[2][2];
            float angle = std::acos(std::min(std::max((traceVal - 1.0f) / 2.0f, -1.0f), 1.0f));
            if (std::abs(angle) < 1e-6f) {
                bondNode->rotAxis  = glm::vec3(0,1,0);
                bondNode->rotAngle = 0.0f;
            } else {
                float inv2s = 1.0f / (2.0f * std::sin(angle));
                bondNode->rotAxis = glm::vec3(
                    (Rmat[2][1] - Rmat[1][2]) * inv2s,
                    (Rmat[0][2] - Rmat[2][0]) * inv2s,
                    (Rmat[1][0] - Rmat[0][1]) * inv2s
                );
                bondNode->rotAngle = glm::degrees(angle);
            }

            // Bond color: grey
            bondNode->material.ambient   = glm::vec3(0.15f);
            bondNode->material.diffuse   = glm::vec3(0.55f, 0.55f, 0.60f);
            bondNode->material.specular  = glm::vec3(0.4f);
            bondNode->material.shininess = 32.0f;

            root->addChild(std::move(bondNode));
        };

        if (bond.order == 1) {
            addBondCyl(a, b);
        } else if (bond.order >= 2) {
            // Two parallel strands offset perpendicular to bond
            glm::vec3 dir = glm::normalize(b - a);
            glm::vec3 perp = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));
            if (glm::length(perp) < 0.01f)
                perp = glm::normalize(glm::cross(dir, glm::vec3(1,0,0)));
            glm::vec3 off = perp * dblOff;
            addBondCyl(a + off, b + off);
            addBondCyl(a - off, b - off);
            if (bond.order == 3) {
                // Triple: add third strand perpendicular
                glm::vec3 perp2 = glm::normalize(glm::cross(dir, perp));
                addBondCyl(a + perp2 * dblOff, b + perp2 * dblOff);
            }
        }
    }
}

void MoleculeScene::update(float dt) {
    std::function<void(SceneNode*, float)> walk = [&](SceneNode* node, float dt2) {
        node->update(dt2);
        for (auto& c : node->children) walk(c.get(), dt2);
    };
    if (root) walk(root.get(), dt);
}

void MoleculeScene::draw(Shader& shader) const {
    if (root) root->draw(shader, glm::mat4(1.0f));
}
