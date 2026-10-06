// =============================================================================
// test_core.cpp - GL-free unit tests (run with `make test` from part1_shapes/)
//   * expression parser used by the z = f(x, y) surface
//   * every shape generator: valid indices, unit normals, outward winding, edge counts
//   * surface builder
//   * .obj / .ply loaders on the bundled sample models
// =============================================================================
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "../src/ExprParser.h"
#include "../src/Geometry.h"
#include "../src/ModelLoader.h"
#include "../src/Surface.h"

static int g_failures = 0, g_checks = 0;

#define CHECK(cond, ...)                                                      \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(cond)) {                                                        \
            ++g_failures;                                                     \
            std::printf("  FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond);     \
            std::printf(__VA_ARGS__);                                         \
            std::printf("\n");                                                \
        }                                                                     \
    } while (0)

static double evalExpr(const std::string& text, double x = 0, double y = 0, bool* ok = nullptr) {
    Expression e; std::string err;
    bool good = e.compile(text, err);
    if (ok) *ok = good;
    CHECK(good || ok, "compile '%s': %s", text.c_str(), err.c_str());
    return good ? e.eval(x, y) : NAN;
}

static void testParser() {
    std::printf("[parser]\n");
    CHECK(std::fabs(evalExpr("1+2*3") - 7) < 1e-12, "precedence");
    CHECK(std::fabs(evalExpr("(1+2)*3") - 9) < 1e-12, "parentheses");
    CHECK(std::fabs(evalExpr("-x^2", 3) + 9) < 1e-12, "-x^2 == -(x^2)");
    CHECK(std::fabs(evalExpr("2^3^2") - 512) < 1e-9, "right associative power");
    CHECK(std::fabs(evalExpr("2^-1") - 0.5) < 1e-12, "negative exponent");
    CHECK(std::fabs(evalExpr("sin(pi/2)") - 1) < 1e-12, "sin(pi/2)");
    CHECK(std::fabs(evalExpr("2x", 4) - 8) < 1e-12, "implicit multiplication");
    CHECK(std::fabs(evalExpr("3 sin(x)", 0) - 0) < 1e-12, "implicit multiplication with function");
    CHECK(std::fabs(evalExpr("z = x*y", 3, 4) - 12) < 1e-12, "leading z =");
    CHECK(std::fabs(evalExpr("f(x,y) = x - y", 3, 4) + 1) < 1e-12, "leading f(x,y) =");
    CHECK(std::fabs(evalExpr("SQRT(X^2+Y^2)", 3, 4) - 5) < 1e-12, "case-insensitive");
    CHECK(std::fabs(evalExpr("max(x, y) + min(x, y)", 3, 4) - 7) < 1e-12, "two-argument functions");
    CHECK(std::fabs(evalExpr("atan2(1, 1)") - std::atan(1.0) ) < 1e-12, "atan2");
    CHECK(std::fabs(evalExpr("e") - std::exp(1.0)) < 1e-12, "constant e");
    CHECK(std::fabs(evalExpr("1e2 + 1") - 101) < 1e-12, "scientific notation");

    const char* bad[] = {"", "1+", "(1+2", "foo(1)", "x +* y", "sin(1,2)", "pow(2)", "2 $ 3", "w + 1"};
    for (const char* b : bad) {
        bool ok = true;
        evalExpr(b, 0, 0, &ok);
        CHECK(!ok, "expected a compile error for '%s'", b);
    }
}

// Valid indices, finite unit normals, wire list, and triangles wound consistently with their normals.
static void checkMesh(const char* name, const MeshData& m, int expectedEdges = -1, bool windingOutward = true) {
    std::printf("[mesh] %s: %zu vertices, %zu triangles, %zu edges\n", name, m.vertices.size(),
                m.indices.size() / 3, m.wireIndices.size() / 2);
    CHECK(!m.vertices.empty() && !m.indices.empty(), "%s: empty", name);
    CHECK(m.indices.size() % 3 == 0, "%s: index count", name);
    CHECK(m.wireIndices.size() % 2 == 0 && !m.wireIndices.empty(), "%s: wire list", name);
    for (unsigned i : m.indices)     CHECK(i < m.vertices.size(), "%s: triangle index out of range", name);
    for (unsigned i : m.wireIndices) CHECK(i < m.vertices.size(), "%s: wire index out of range", name);

    for (const auto& v : m.vertices) {
        float len = glm::length(v.normal);
        CHECK(std::isfinite(len) && std::fabs(len - 1.0f) < 1e-3f, "%s: normal length %f", name, len);
        CHECK(std::isfinite(v.position.x + v.position.y + v.position.z), "%s: position", name);
        CHECK(v.color.r >= 0 && v.color.r <= 1.0001f && v.color.g >= 0 && v.color.g <= 1.0001f &&
              v.color.b >= 0 && v.color.b <= 1.0001f, "%s: color range", name);
    }
    if (windingOutward) {
        int bad = 0;
        for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
            const Vertex &a = m.vertices[m.indices[t]], &b = m.vertices[m.indices[t + 1]], &c = m.vertices[m.indices[t + 2]];
            glm::vec3 g = glm::cross(b.position - a.position, c.position - a.position);
            if (glm::length(g) < 1e-9f) continue;   // degenerate (poles, apex)
            if (glm::dot(g, a.normal + b.normal + c.normal) <= 0.0f) ++bad;
        }
        CHECK(bad == 0, "%s: %d triangles wound against their normals", name, bad);
    }
    if (expectedEdges >= 0)
        CHECK((int)m.wireIndices.size() / 2 == expectedEdges, "%s: expected %d edges, got %zu", name, expectedEdges,
              m.wireIndices.size() / 2);
}

static void testGeometry() {
    std::printf("[geometry]\n");
    using namespace Geometry;
    // 2D (edges = outline only)
    checkMesh("triangle",  regularPolygon(3), 3);
    checkMesh("rectangle", rectangle(1, 0.7f), 4);
    checkMesh("pentagon",  regularPolygon(5), 5);
    checkMesh("hexagon",   regularPolygon(6), 6);
    checkMesh("circle",    regularPolygon(48), 48);
    checkMesh("ellipse",   ellipse(0.5f, 0.3f, 40), 40);
    checkMesh("trapezoid", trapezoid(1, 0.5f, 0.7f), 4);
    checkMesh("star",      star(5, 0.5f, 0.4f), 10);
    checkMesh("arrow",     arrow(), 7);
    // 3D
    checkMesh("cube",         cube(), 12);
    checkMesh("tetrahedron",  tetrahedron(), 6);
    checkMesh("prism(6)",     prism(6, 0.5f, 1.0f), 18);
    checkMesh("sphere",       sphere(36, 18));
    checkMesh("cylinder",     frustum(0.5f, 0.5f, 1.0f, 32));
    checkMesh("cone",         frustum(0.5f, 0.0f, 1.0f, 32));
    checkMesh("trunc. cone",  frustum(0.5f, 0.25f, 1.0f, 32));
    checkMesh("torus",        torus(0.5f, 0.18f, 48, 24));

    // Unit-size sanity: shapes should be roughly one unit wide.
    MeshData s = sphere(36, 18);
    glm::vec3 lo(1e9f), hi(-1e9f);
    for (auto& v : s.vertices) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
    CHECK(std::fabs((hi.y - lo.y) - 1.0f) < 1e-3f, "sphere diameter is 1");
}

static void testSurface() {
    std::printf("[surface]\n");
    SurfaceParams p;
    MeshData m; std::string err;
    CHECK(Surface::build(p, m, err), "default surface: %s", err.c_str());
    checkMesh("surface sin*cos", m);

    p.expression = "x^2 - y^2";
    CHECK(Surface::build(p, m, err), "saddle: %s", err.c_str());
    checkMesh("surface saddle", m);

    p.expression = "log(x)";   // not finite for x <= 0: those cells are skipped
    CHECK(Surface::build(p, m, err), "log(x): %s", err.c_str());
    p.expression = "1/0";
    CHECK(!Surface::build(p, m, err) && !err.empty(), "1/0 should be rejected");
    p.expression = "sin(x";
    CHECK(!Surface::build(p, m, err) && !err.empty(), "syntax error should be rejected");
    p.expression = "x"; p.xMax = p.xMin;
    CHECK(!Surface::build(p, m, err), "empty range should be rejected");
}

static void testModels() {
    std::printf("[models]\n");
    struct Case { const char* file; int minVerts; int edges; };
    const Case cases[] = {
        {"assets/models/octahedron.obj", 6, 12},
        {"assets/models/cube_binary.ply", 8, 12},
        {"assets/models/icosphere_colored.ply", 42, -1},
        {"assets/models/torusknot.obj", 1000, -1},
    };
    for (const Case& c : cases) {
        MeshData m; std::string err;
        bool ok = ModelLoader::load(c.file, m, err);
        CHECK(ok, "%s: %s", c.file, err.c_str());
        if (!ok) continue;
        CHECK((int)m.vertices.size() >= c.minVerts, "%s: %zu vertices", c.file, m.vertices.size());
        // normalised to a unit box centred at the origin
        glm::vec3 lo(1e9f), hi(-1e9f);
        for (auto& v : m.vertices) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
        glm::vec3 ext = hi - lo;
        CHECK(std::fabs(std::max(ext.x, std::max(ext.y, ext.z)) - 1.0f) < 1e-3f, "%s: not normalised", c.file);
        CHECK(glm::length((lo + hi) * 0.5f) < 1e-3f, "%s: not centred", c.file);
        checkMesh(c.file, m, c.edges, /*windingOutward=*/false);
    }
    // ply with colours keeps them (icosphere colours are not the rainbow default)
    MeshData ico; std::string err;
    ModelLoader::load("assets/models/icosphere_colored.ply", ico, err);
    CHECK(!ico.vertices.empty() && ico.vertices[0].color != glm::vec3(1.0f), "PLY vertex colors preserved");

    MeshData m;
    CHECK(!ModelLoader::load("assets/models/does_not_exist.obj", m, err) && !err.empty(), "missing file");
    CHECK(!ModelLoader::load("assets/textures/checker.png", m, err) && !err.empty(), "unsupported extension");
}

int main() {
    testParser();
    testGeometry();
    testSurface();
    testModels();
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
