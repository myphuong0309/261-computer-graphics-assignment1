// =============================================================================
// MoleculeScene.cpp - Ball-and-stick molecule rendering
// =============================================================================
#include "MoleculeScene.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

void MoleculeScene::build(const MoleculeSpec& spec, Mesh* sphereMesh, Mesh* cylMesh) {
    float keepAngle = root ? root->rotAngle : 0.0f;   // keep the spin when the molecule is edited
    vibNodes.clear();
    strands.clear();
    root = std::make_unique<MolRotNode>();
    root->rotAxis   = glm::vec3(0, 1, 0);
    root->rotAngle  = keepAngle;
    root->animating = rotating;

    // Centre of mass c = sum(m_i p_i) / sum(m_i): the molecule spins and vibrates around it
    glm::vec3 c(0.0f);
    float totalMass = 0.0f;
    for (const auto& a : spec.atoms) {
        float m = AtomData::get(a.symbol).mass;
        c += m * a.position;
        totalMass += m;
    }
    centerOfMass = totalMass > 0.0f ? c / totalMass : glm::vec3(0.0f);

    // Scale factor — the farthest ball surface touches the fit sphere:
    //   s * max_i(|p_i - c| + kAtomScale * r_i) = kFitRadius
    float extent = 0.0f;
    for (const auto& a : spec.atoms)
        extent = std::max(extent, glm::length(a.position - centerOfMass) + kAtomScale * AtomData::get(a.symbol).radius);
    scale = (extent > 1e-3f) ? (kFitRadius / extent) : 1.0f;

    baseFreq = spec.stretchWavenumber > 0.0f ? spec.stretchWavenumber * kHzPerWavenumber : 2.0f;
    vibFreq  = baseFreq * vibSpeed;

    // Build atom nodes (with vibration)
    for (int i = 0; i < (int)spec.atoms.size(); ++i) {
        const auto& aspec = spec.atoms[i];
        const AtomInfo& info = AtomData::get(aspec.symbol);

        auto vibNode = std::make_unique<VibrationNode>();
        vibNode->init((aspec.position - centerOfMass) * scale);
        vibNode->vibFreq   = vibFreq;
        vibNode->vibAmp    = vibAmp;
        vibNode->animating = vibrating;

        auto atom = std::make_unique<SceneNode>();
        atom->mesh  = sphereMesh;
        atom->scale = glm::vec3(kAtomScale * info.radius * scale); // visual radius (unit sphere mesh)
        atom->material.ambient   = info.color * 0.25f;
        atom->material.diffuse   = info.color;
        atom->material.specular  = glm::vec3(0.6f);
        atom->material.shininess = 80.0f;

        vibNode->addChild(std::move(atom));
        vibNodes.push_back(vibNode.get());
        root->addChild(std::move(vibNode));
    }

    // Bond cylinders: 1, 2 or 3 parallel strands
    for (const auto& bond : spec.bonds) {
        int n = (int)spec.atoms.size();
        if (bond.a < 0 || bond.b < 0 || bond.a >= n || bond.b >= n || bond.a == bond.b) continue;

        std::vector<float> offsets;
        float radius;
        if (bond.order <= 1)      { offsets = {0.0f};                            radius = kBondRadius;   }
        else if (bond.order == 2) { offsets = {-kDoubleOffset, kDoubleOffset};    radius = kStrandRadius; }
        else                      { offsets = {-kTripleOffset, 0.0f, kTripleOffset}; radius = kStrandRadius; }

        for (float off : offsets) {
            auto bondNode = std::make_unique<SceneNode>();
            bondNode->mesh = cylMesh;

            // Bond color: grey
            bondNode->material.ambient   = glm::vec3(0.15f);
            bondNode->material.diffuse   = glm::vec3(0.55f, 0.55f, 0.60f);
            bondNode->material.specular  = glm::vec3(0.4f);
            bondNode->material.shininess = 32.0f;

            strands.push_back({bondNode.get(), bond.a, bond.b, off * scale, radius * scale});
            root->addChild(std::move(bondNode));
        }
    }
    placeStrands();
}

void MoleculeScene::placeCylinder(SceneNode& node, const glm::vec3& pa, const glm::vec3& pb, float radius) {
    glm::vec3 dir = pb - pa;
    float len = glm::length(dir);
    node.visible = len > 1e-4f;          // two atoms on top of each other: nothing to draw
    if (!node.visible) return;

    // Rotation taking the cylinder axis Y onto the bond direction d:
    //   axis = Y x d / |Y x d|,  angle = atan2(|Y x d|, Y . d)
    glm::vec3 d  = dir / len;
    glm::vec3 up(0, 1, 0);
    glm::vec3 axis = glm::cross(up, d);
    float sinA = glm::length(axis);
    float cosA = glm::dot(up, d);

    if (sinA > 1e-6f) {
        node.rotAxis  = axis / sinA;
        node.rotAngle = glm::degrees(std::atan2(sinA, cosA));
    } else if (cosA < 0.0f) {            // d = -Y: any axis perpendicular to Y, half a turn
        node.rotAxis  = glm::vec3(1, 0, 0);
        node.rotAngle = 180.0f;
    } else {                             // d = +Y: no rotation
        node.rotAxis  = up;
        node.rotAngle = 0.0f;
    }

    node.position = (pa + pb) * 0.5f;
    node.scale    = glm::vec3(2.0f * radius, len, 2.0f * radius);   // mesh radius 0.5, height 1
}

void MoleculeScene::placeStrands() {
    for (const Strand& s : strands) {
        glm::vec3 a = vibNodes[s.a]->position;
        glm::vec3 b = vibNodes[s.b]->position;

        // Strands of a multiple bond are shifted along u = d x Z (in the screen plane of the
        // default view), or d x X when the bond is (almost) parallel to Z
        glm::vec3 shift(0.0f);
        if (s.offset != 0.0f && glm::length(b - a) > 1e-6f) {
            glm::vec3 d = glm::normalize(b - a);
            glm::vec3 perp = glm::cross(d, glm::vec3(0, 0, 1));
            if (glm::length(perp) < 0.1f) perp = glm::cross(d, glm::vec3(1, 0, 0));
            shift = glm::normalize(perp) * s.offset;
        }
        placeCylinder(*s.node, a + shift, b + shift, s.radius);
    }
}

void MoleculeScene::setVibration(float amp, float speed) {
    vibAmp   = amp;
    vibSpeed = speed;
    vibFreq  = baseFreq * speed;
    for (auto* n : vibNodes) { n->vibAmp = amp; n->vibFreq = vibFreq; }
}

void MoleculeScene::update(float dt) {
    if (!root) return;
    root->updateTree(dt);   // rotation (root) and vibration (atom nodes)
    placeStrands();         // bonds follow the atoms
}

void MoleculeScene::draw(Shader& shader) const {
    if (root) root->draw(shader, glm::mat4(1.0f));
}
