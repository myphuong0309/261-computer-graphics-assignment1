// =============================================================================
// test_part2.cpp - CPU checks of Part 2 (make test). No window / GPU needed:
// scenes are built with dummy meshes and only their transforms are inspected.
// =============================================================================
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "SceneNode.h"
#include "AtomData.h"
#include "BohrModel.h"
#include "MoleculeScene.h"

static int g_checks = 0, g_failed = 0;

#define CHECK(cond, ...) do { ++g_checks; if (!(cond)) { ++g_failed; \
    if (g_failed <= 30) { std::printf("FAIL %s:%d: ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } } while (0)

static glm::vec3 point(const glm::mat4& M, const glm::vec3& p) { return glm::vec3(M * glm::vec4(p, 1.0f)); }
static glm::vec3 dirOf(const glm::mat4& M, const glm::vec3& v) { return glm::vec3(M * glm::vec4(v, 0.0f)); }

// world matrix of every node (parents first)
struct Item { const SceneNode* node; const SceneNode* parent; glm::mat4 world; };
static void collect(const SceneNode* n, const SceneNode* parent, const glm::mat4& P, std::vector<Item>& out) {
    glm::mat4 W = P * n->localTransform();
    out.push_back({n, parent, W});
    for (const auto& c : n->children) collect(c.get(), n, W, out);
}

// -----------------------------------------------------------------------------
// Meshes: closed-form counts, valid indices, unit normals, counter-clockwise winding
// -----------------------------------------------------------------------------
static void checkMesh(const char* name, const MeshData& d, size_t V, size_t I) {
    CHECK(d.vertices.size() == V, "%s: V = %zu, expected %zu", name, d.vertices.size(), V);
    CHECK(d.indices.size() == I, "%s: I = %zu, expected %zu", name, d.indices.size(), I);
    bool inRange = true, unit = true;
    for (unsigned idx : d.indices) inRange &= idx < d.vertices.size();
    for (const auto& v : d.vertices) unit &= std::fabs(glm::length(v.normal) - 1.0f) < 1e-5f;
    CHECK(inRange, "%s: index out of range", name);
    CHECK(unit, "%s: normal not unit length", name);
    if (!inRange) return;
    int wrong = 0;
    for (size_t t = 0; t + 2 < d.indices.size(); t += 3) {
        const Vertex& a = d.vertices[d.indices[t]];
        const Vertex& b = d.vertices[d.indices[t + 1]];
        const Vertex& c = d.vertices[d.indices[t + 2]];
        glm::vec3 n = glm::cross(b.position - a.position, c.position - a.position);
        if (glm::length(n) < 1e-9f) continue;                       // degenerate
        if (glm::dot(n, a.normal + b.normal + c.normal) <= 0.0f) ++wrong;
    }
    CHECK(wrong == 0, "%s: %d triangle(s) wound clockwise from outside", name, wrong);
}

static void testMeshes() {
    const int S = 36, K = 18, N = 32, M = 128, m = 8;
    MeshData sphere = Mesh::sphereData(S, K);
    checkMesh("sphere", sphere, (size_t)(S + 1) * (K + 1), (size_t)6 * S * (K - 1));
    for (const auto& v : sphere.vertices)
        CHECK(std::fabs(glm::length(v.position) - 1.0f) < 1e-5f, "sphere vertex off the unit sphere");

    checkMesh("cylinder", Mesh::cylinderData(N, 0.5f, 1.0f), (size_t)4 * N + 6, (size_t)12 * N);

    for (int k = 0; k < BohrAtom::kMaxShells; ++k) {
        float R = BohrAtom::shellRadius(k), rho = BohrAtom::kRingTubeRadius;
        MeshData torus = Mesh::torusData(R, rho, M, m);
        checkMesh("torus", torus, (size_t)(M + 1) * (m + 1), (size_t)6 * M * m);
        float worst = 0.0f;   // every vertex lies at distance rho from the centre circle of radius R
        for (const auto& v : torus.vertices) {
            float q = std::sqrt(v.position.x * v.position.x + v.position.y * v.position.y) - R;
            worst = std::max(worst, std::fabs(std::sqrt(q * q + v.position.z * v.position.z) - rho));
        }
        CHECK(worst < 1e-5f, "torus %d: tube radius error %g", k, worst);
    }
}

// -----------------------------------------------------------------------------
// Bohr atom: data, electrons on their ring at every time, no overlaps
// -----------------------------------------------------------------------------
static void checkBohrScene(const std::string& sym, const BohrAtom& atom, const AtomInfo& info, const char* when) {
    std::vector<Item> items;
    collect(atom.root.get(), nullptr, glm::mat4(1.0f), items);

    std::vector<glm::vec3> electrons;
    std::vector<int> shellOf;
    int shell = -1;
    glm::vec3 normal(0.0f);
    for (const Item& it : items) {
        if (dynamic_cast<const ShellPivot*>(it.node)) {
            ++shell;
            normal = glm::normalize(dirOf(it.world, glm::vec3(0, 0, 1)));   // n_k = R_x(tilt) z
            float tilt = glm::radians(BohrAtom::shellTilt(shell));
            CHECK(glm::length(normal - glm::vec3(0, -std::sin(tilt), std::cos(tilt))) < 1e-5f,
                  "%s shell %d: orbit-plane normal", sym.c_str(), shell);
            continue;
        }
        if (!it.node->mesh || shell < 0) continue;
        float r = BohrAtom::shellRadius(shell);
        if (dynamic_cast<const OrbitNode*>(it.parent)) {                 // electron
            glm::vec3 c = point(it.world, glm::vec3(0.0f));
            CHECK(std::fabs(glm::length(c) - r) < 1e-4f, "%s (%s): electron at |c| = %g, ring r = %g",
                  sym.c_str(), when, glm::length(c), r);
            CHECK(std::fabs(glm::dot(c, normal)) < 1e-4f, "%s (%s): electron off the orbit plane (%g)",
                  sym.c_str(), when, glm::dot(c, normal));
            electrons.push_back(c);
            shellOf.push_back(shell);
        } else {                                                          // ring: rigid, centre circle = orbit
            glm::mat3 A(it.world);
            CHECK(std::fabs(glm::determinant(A) - 1.0f) < 1e-4f, "%s: ring is scaled", sym.c_str());
            glm::vec3 p = point(it.world, glm::vec3(r, 0, 0)), q = point(it.world, glm::vec3(0, r, 0));
            CHECK(std::fabs(glm::length(p) - r) < 1e-4f && std::fabs(glm::dot(q, normal)) < 1e-4f,
                  "%s: ring centre circle differs from the orbit", sym.c_str());
        }
    }
    int total = 0;
    for (int n : info.shells) total += n;
    CHECK((int)electrons.size() == total, "%s: %zu electron nodes, %d electrons", sym.c_str(), electrons.size(), total);

    float minPair = 1e9f, minNucleus = 1e9f;
    for (size_t i = 0; i < electrons.size(); ++i) {
        minNucleus = std::min(minNucleus, glm::length(electrons[i]));
        for (size_t j = i + 1; j < electrons.size(); ++j) {
            float d = glm::length(electrons[i] - electrons[j]);
            minPair = std::min(minPair, d);
            if (shellOf[i] == shellOf[j]) {   // same shell: chord >= 2 r sin(pi / n)
                int n = info.shells[shellOf[i]];
                float chord = 2.0f * BohrAtom::shellRadius(shellOf[i]) * std::sin((float)M_PI / n);
                CHECK(d > chord - 1e-3f, "%s: electrons of one shell closer than the even spacing", sym.c_str());
            }
        }
    }
    CHECK(minPair > 2.0f * BohrAtom::kElectronRadius, "%s: electrons overlap (%g)", sym.c_str(), minPair);
    CHECK(minNucleus > BohrAtom::kNucleusRadius + BohrAtom::kElectronRadius, "%s: electron inside the nucleus", sym.c_str());
}

static void testBohr() {
    CHECK(BohrAtom::kRingTubeRadius < BohrAtom::kElectronRadius, "ring tube thicker than an electron");
    Mesh dummy;
    std::vector<Mesh> rings(BohrAtom::kMaxShells);
    std::vector<Mesh*> ringPtrs;
    for (auto& r : rings) ringPtrs.push_back(&r);

    for (const std::string& sym : AtomData::symbolsByZ()) {
        const AtomInfo& info = AtomData::get(sym);
        int total = 0;
        for (size_t n = 0; n < info.shells.size(); ++n) {
            total += info.shells[n];
            int cap = 2 * (int)(n + 1) * (int)(n + 1);
            CHECK(info.shells[n] <= cap, "%s: shell %zu holds %d > 2n^2 = %d", sym.c_str(), n + 1, info.shells[n], cap);
        }
        CHECK(total == info.atomicNumber, "%s: %d electrons but Z = %d", sym.c_str(), total, info.atomicNumber);
        CHECK((int)info.shells.size() <= BohrAtom::kMaxShells, "%s: too many shells", sym.c_str());

        BohrAtom atom;
        atom.build(sym, &dummy, ringPtrs);
        checkBohrScene(sym, atom, info, "t=0");
        const float steps[] = {0.016f, 0.5f, 1.7f, 13.2f};
        for (float dt : steps) { atom.update(dt); checkBohrScene(sym, atom, info, "animated"); }

        // angular speed: one step of dt turns every orbit node by omega * dt
        std::vector<float> before;
        for (auto* o : atom.orbits) before.push_back(o->rotAngle);
        atom.setAnimSpeed(2.0f);
        atom.update(0.25f);
        for (size_t i = 0; i < atom.orbits.size(); ++i) {
            float expect = std::fmod(before[i] + 2.0f * atom.orbits[i]->baseSpeed * 0.25f, 360.0f);
            CHECK(std::fabs(atom.orbits[i]->rotAngle - expect) < 1e-3f, "%s: orbit step", sym.c_str());
        }

        // built while paused (no update yet): electrons must already be on their rings
        BohrAtom paused;
        paused.animating = false;
        paused.build(sym, &dummy, ringPtrs);
        checkBohrScene(sym, paused, info, "paused");
    }
}

// -----------------------------------------------------------------------------
// Molecules: reference geometry, normalisation, bonds joining the atoms, vibration, rotation
// -----------------------------------------------------------------------------
struct Reference { const char* formula; float bond; float angle; };   // angle < 0: none

static float angleDeg(glm::vec3 u, glm::vec3 v) {
    float c = glm::dot(u, v) / (glm::length(u) * glm::length(v));
    return glm::degrees(std::acos(std::max(-1.0f, std::min(1.0f, c))));
}

static void checkMoleculeScene(const MoleculeSpec& spec, const MoleculeScene& mol, const char* when, float lambda) {
    std::vector<Item> items;
    collect(mol.root.get(), nullptr, glm::mat4(1.0f), items);
    glm::mat4 R = mol.root->localTransform();
    int n = (int)spec.atoms.size();

    // world atom centres and ball radii
    std::vector<glm::vec3> x(n);
    std::vector<float> rho(n);
    glm::vec3 com(0.0f);
    float mass = 0.0f;
    for (int i = 0; i < n; ++i) {
        x[i] = point(R, mol.vibNodes[i]->position);
        rho[i] = mol.vibNodes[i]->children[0]->scale.x;
        float m = AtomData::get(spec.atoms[i].symbol).mass;
        com += m * x[i];
        mass += m;
        // similarity: equilibrium world position = s (p - c), current one = lambda times that
        glm::vec3 expect = point(R, lambda * mol.scale * (spec.atoms[i].position - mol.centerOfMass));
        CHECK(glm::length(x[i] - expect) < 1e-4f, "%s (%s): atom %d position", spec.formula.c_str(), when, i);
    }
    CHECK(glm::length(com / mass) < 1e-4f, "%s (%s): centre of mass moved", spec.formula.c_str(), when);

    // every strand joins its two atoms (shifted sideways), with the right radius
    int strand = 0;
    for (const auto& b : spec.bonds) {
        int count = b.order <= 1 ? 1 : (b.order == 2 ? 2 : 3);
        float r = mol.scale * (b.order <= 1 ? MoleculeScene::kBondRadius : MoleculeScene::kStrandRadius);
        std::vector<glm::vec3> axisPoints;
        for (int s = 0; s < count; ++s, ++strand) {
            const Item* it = nullptr;
            int seen = -1;
            for (const Item& cand : items)
                if (cand.parent == mol.root.get() && !dynamic_cast<const VibrationNode*>(cand.node) && ++seen == strand) it = &cand;
            CHECK(it != nullptr, "%s: missing strand", spec.formula.c_str());
            if (!it) return;
            glm::vec3 e0 = point(it->world, glm::vec3(0, -0.5f, 0)), e1 = point(it->world, glm::vec3(0, 0.5f, 0));
            glm::vec3 shift = e0 - x[b.a];
            CHECK(glm::length((e1 - e0) - (x[b.b] - x[b.a])) < 1e-4f, "%s (%s): strand does not join atoms %d-%d",
                  spec.formula.c_str(), when, b.a, b.b);
            CHECK(std::fabs(glm::dot(shift, glm::normalize(x[b.b] - x[b.a]))) < 1e-4f, "%s: shift not perpendicular", spec.formula.c_str());
            CHECK(glm::length(shift) + r < std::min(rho[b.a], rho[b.b]), "%s: strand end sticks out of a ball", spec.formula.c_str());
            float radius = glm::length(point(it->world, glm::vec3(0.5f, 0, 0)) - point(it->world, glm::vec3(0.0f)));
            CHECK(std::fabs(radius - r) < 1e-4f, "%s: strand radius %g, expected %g", spec.formula.c_str(), radius, r);

            // normal matrix (M^-1)^T keeps the side normal perpendicular to the (non-uniformly scaled) surface
            glm::mat3 Nm = glm::transpose(glm::inverse(glm::mat3(it->world)));
            glm::vec3 nW = Nm * glm::vec3(1, 0, 0);
            CHECK(std::fabs(glm::dot(nW, dirOf(it->world, glm::vec3(0, 1, 0)))) < 1e-4f &&
                  std::fabs(glm::dot(nW, dirOf(it->world, glm::vec3(0, 0, 1)))) < 1e-4f, "%s: normal matrix", spec.formula.c_str());
            axisPoints.push_back(e0);
        }
        for (size_t p = 0; p < axisPoints.size(); ++p)   // parallel strands never touch
            for (size_t q = p + 1; q < axisPoints.size(); ++q)
                CHECK(glm::length(axisPoints[p] - axisPoints[q]) > 2.0f * r, "%s: strands overlap", spec.formula.c_str());
    }
}

static void testMolecules() {
    const Reference refs[] = {
        {"H2O", 0.957f, 104.5f}, {"CO2", 1.160f, 180.0f}, {"NH3", 1.012f, 106.7f}, {"CH4", 1.087f, 109.47f},
        {"O2", 1.2075f, -1.0f}, {"N2", 1.0977f, -1.0f}, {"HCl", 1.2746f, -1.0f}, {"NaCl", 2.361f, -1.0f},
    };
    Mesh sphere, cyl;
    for (const Reference& ref : refs) {
        const MoleculeSpec* spec = MoleculeData::find(ref.formula);
        CHECK(spec != nullptr, "missing molecule %s", ref.formula);
        if (!spec) continue;

        // reference geometry (experimental values)
        for (const auto& b : spec->bonds) {
            float len = glm::length(spec->atoms[b.a].position - spec->atoms[b.b].position);
            CHECK(std::fabs(len - ref.bond) < 2e-3f, "%s: bond length %.4f, reference %.4f", ref.formula, len, ref.bond);
        }
        if (ref.angle > 0.0f)
            for (size_t i = 0; i < spec->bonds.size(); ++i)
                for (size_t j = i + 1; j < spec->bonds.size(); ++j) {
                    const auto& p = spec->atoms;
                    float a = angleDeg(p[spec->bonds[i].b].position - p[0].position, p[spec->bonds[j].b].position - p[0].position);
                    CHECK(std::fabs(a - ref.angle) < 0.1f, "%s: bond angle %.2f, reference %.2f", ref.formula, a, ref.angle);
                }

        // distance-based bond detection finds exactly the bonds of the preset
        auto found = MoleculeData::detectBonds(spec->atoms);
        CHECK(found.size() == spec->bonds.size(), "%s: detectBonds found %zu bonds, preset has %zu",
              ref.formula, found.size(), spec->bonds.size());

        MoleculeScene mol;
        mol.build(*spec, &sphere, &cyl);
        CHECK((int)mol.vibNodes.size() == (int)spec->atoms.size(), "%s: atom nodes", ref.formula);

        // normalisation: the farthest ball surface touches the fit sphere
        float extent = 0.0f;
        for (auto* v : mol.vibNodes) extent = std::max(extent, glm::length(v->position) + v->children[0]->scale.x);
        CHECK(std::fabs(extent - MoleculeScene::kFitRadius) < 1e-4f, "%s: fitted radius %g", ref.formula, extent);
        checkMoleculeScene(*spec, mol, "rest", 1.0f);

        // vibration: lambda(t) = 1 + A sin(2 pi f t) for every atom, bonds follow, centre of mass fixed
        mol.setVibrating(true);
        float t = 0.0f;
        const float steps[] = {0.031f, 0.11f, 0.37f, 1.3f};
        for (float dt : steps) {
            mol.update(dt);
            t += dt;
            float lambda = 1.0f + mol.vibAmp * std::sin(2.0f * (float)M_PI * mol.vibFreq * t);
            checkMoleculeScene(*spec, mol, "vibrating", lambda);
        }

        // rotation of the whole molecule (root node) keeps everything attached
        mol.setRotating(true);
        mol.update(0.75f);
        t += 0.75f;
        CHECK(std::fabs(mol.root->rotAngle - 30.0f * 0.75f) < 1e-3f, "%s: rotation angle", ref.formula);
        checkMoleculeScene(*spec, mol, "rotating", 1.0f + mol.vibAmp * std::sin(2.0f * (float)M_PI * mol.vibFreq * t));
    }

    CHECK(MoleculeData::hillFormula(MoleculeData::find("CH4")->atoms) == "CH4", "Hill formula CH4");
    CHECK(MoleculeData::hillFormula(MoleculeData::find("H2O")->atoms) == "H2O", "Hill formula H2O");
    CHECK(MoleculeData::hillFormula(MoleculeData::find("NH3")->atoms) == "H3N", "Hill formula NH3");
}

// -----------------------------------------------------------------------------
// placeCylinder: M (0, -1/2, 0) = a and M (0, 1/2, 0) = b for any direction, also d = +-Y
// -----------------------------------------------------------------------------
static void testCylinderPlacement() {
    std::srand(7);
    auto rnd = []() { return (float)std::rand() / RAND_MAX * 4.0f - 2.0f; };
    std::vector<std::pair<glm::vec3, glm::vec3>> cases = {
        {{0, 0, 0}, {0, 1, 0}}, {{0, 0, 0}, {0, -1, 0}}, {{1, 2, 3}, {1, -5, 3}}, {{0, 0, 0}, {1, 0, 0}},
        {{0, 0, 0}, {0, 0, -2}}, {{0, 0, 0}, {1e-4f, 1, 0}}, {{0, 0, 0}, {1e-4f, -1, 0}},
    };
    for (int i = 0; i < 300; ++i) cases.push_back({{rnd(), rnd(), rnd()}, {rnd(), rnd(), rnd()}});
    for (const auto& c : cases) {
        SceneNode node;
        MoleculeScene::placeCylinder(node, c.first, c.second, 0.1f);
        if (glm::length(c.second - c.first) < 1e-3f) continue;
        glm::mat4 M = node.localTransform();
        float err = std::max(glm::length(point(M, glm::vec3(0, -0.5f, 0)) - c.first),
                             glm::length(point(M, glm::vec3(0, 0.5f, 0)) - c.second));
        CHECK(err < 1e-4f * (1.0f + glm::length(c.second - c.first)), "placeCylinder endpoint error %g", err);
    }
}

int main() {
    testMeshes();
    testBohr();
    testMolecules();
    testCylinderPlacement();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
