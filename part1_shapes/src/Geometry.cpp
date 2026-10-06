// =============================================================================
// Geometry.cpp - procedural 2D / 3D shape generators and mesh utilities
// =============================================================================
#include "Geometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

const float PI = (float)M_PI;

// Adds default per-vertex colors and the wire (edge) list.
MeshData finish(MeshData m) {
    MeshUtil::assignVertexColors(m, MeshUtil::ColorScheme::Rainbow);
    MeshUtil::buildWireIndices(m);
    return m;
}

Vertex makeVertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
    Vertex v;
    v.position = p;
    v.normal   = n;
    v.uv       = uv;
    return v;
}

// Triangle fan around `center` through a counter-clockwise 2D ring (convex / star-shaped only).
void appendFan(MeshData& m, const std::vector<glm::vec2>& ring, const glm::vec2& center) {
    const glm::vec3 nz(0.0f, 0.0f, 1.0f);
    unsigned base = (unsigned)m.vertices.size();
    m.vertices.push_back(makeVertex(glm::vec3(center, 0.0f), nz, glm::vec2(0.0f)));
    for (const auto& p : ring)
        m.vertices.push_back(makeVertex(glm::vec3(p, 0.0f), nz, glm::vec2(0.0f)));
    unsigned n = (unsigned)ring.size();
    for (unsigned i = 0; i < n; ++i) {
        m.indices.push_back(base);
        m.indices.push_back(base + 1 + i);
        m.indices.push_back(base + 1 + (i + 1) % n);
    }
}

// Flat disc cap on a Y-aligned solid. top=true faces +Y, otherwise -Y.
void appendCap(MeshData& m, float radius, float y, int segments, bool top) {
    const glm::vec3 n(0.0f, top ? 1.0f : -1.0f, 0.0f);
    unsigned center = (unsigned)m.vertices.size();
    m.vertices.push_back(makeVertex(glm::vec3(0.0f, y, 0.0f), n, glm::vec2(0.5f)));
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * PI * i / segments;
        float c = std::cos(a), s = std::sin(a);
        m.vertices.push_back(makeVertex(glm::vec3(radius * c, y, radius * s), n,
                                        glm::vec2(0.5f + 0.5f * c, 0.5f + 0.5f * s)));
    }
    for (int i = 0; i < segments; ++i) {
        unsigned p0 = center + 1 + i, p1 = p0 + 1;
        if (top) { m.indices.push_back(center); m.indices.push_back(p1); m.indices.push_back(p0); }
        else     { m.indices.push_back(center); m.indices.push_back(p0); m.indices.push_back(p1); }
    }
}

} // namespace

// =============================================================================
// 2D shapes
// =============================================================================
namespace Geometry {

MeshData regularPolygon(int sides, float radius) {
    sides = std::max(sides, 3);
    std::vector<glm::vec2> ring;
    for (int i = 0; i < sides; ++i) {
        float a = PI / 2.0f + 2.0f * PI * i / sides;   // first vertex points up
        ring.emplace_back(radius * std::cos(a), radius * std::sin(a));
    }
    MeshData m;
    appendFan(m, ring, glm::vec2(0.0f));
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

MeshData rectangle(float w, float h) {
    float x = w * 0.5f, y = h * 0.5f;
    MeshData m;
    appendFan(m, {{-x, -y}, {x, -y}, {x, y}, {-x, y}}, glm::vec2(0.0f));
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

MeshData ellipse(float rx, float ry, int segments) {
    segments = std::max(segments, 3);
    std::vector<glm::vec2> ring;
    for (int i = 0; i < segments; ++i) {
        float a = 2.0f * PI * i / segments;
        ring.emplace_back(rx * std::cos(a), ry * std::sin(a));
    }
    MeshData m;
    appendFan(m, ring, glm::vec2(0.0f));
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

MeshData trapezoid(float bottomW, float topW, float h) {
    float bx = bottomW * 0.5f, tx = topW * 0.5f, y = h * 0.5f;
    MeshData m;
    appendFan(m, {{-bx, -y}, {bx, -y}, {tx, y}, {-tx, y}}, glm::vec2(0.0f));
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

MeshData star(int points, float outerR, float innerRatio) {
    points = std::max(points, 3);
    std::vector<glm::vec2> ring;
    for (int i = 0; i < 2 * points; ++i) {
        float a = PI / 2.0f + PI * i / points;
        float r = (i % 2 == 0) ? outerR : outerR * innerRatio;
        ring.emplace_back(r * std::cos(a), r * std::sin(a));
    }
    MeshData m;
    appendFan(m, ring, glm::vec2(0.0f));   // a star is star-shaped around its centre
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

MeshData arrow() {
    // The arrow is concave, so it is triangulated by hand with shared vertices
    // (so that the shaft/head seam is recognised as an interior edge in wireframe).
    const glm::vec3 nz(0.0f, 0.0f, 1.0f);
    const glm::vec2 pts[7] = {
        {-0.5f, -0.15f},  // 0 A  shaft bottom-left
        { 0.1f, -0.15f},  // 1 B  shaft bottom-right
        { 0.1f,  0.15f},  // 2 C  shaft top-right
        {-0.5f,  0.15f},  // 3 D  shaft top-left
        { 0.1f, -0.40f},  // 4 E  head bottom
        { 0.5f,  0.00f},  // 5 F  tip
        { 0.1f,  0.40f},  // 6 G  head top
    };
    MeshData m;
    for (const auto& p : pts) m.vertices.push_back(makeVertex(glm::vec3(p, 0.0f), nz, glm::vec2(0.0f)));
    const unsigned tris[] = {0, 1, 2,  0, 2, 3,   4, 5, 1,  1, 5, 2,  2, 5, 6};
    m.indices.assign(tris, tris + 15);
    MeshUtil::planarUV(m);
    return finish(std::move(m));
}

// =============================================================================
// 3D solids
// =============================================================================
MeshData cube(float size) {
    float h = size * 0.5f;
    // normal, u axis, v axis with u x v == normal (counter-clockwise seen from outside)
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] = {
        {{ 1, 0, 0}, { 0, 0,-1}, {0, 1, 0}},
        {{-1, 0, 0}, { 0, 0, 1}, {0, 1, 0}},
        {{ 0, 1, 0}, { 1, 0, 0}, {0, 0,-1}},
        {{ 0,-1, 0}, { 1, 0, 0}, {0, 0, 1}},
        {{ 0, 0, 1}, { 1, 0, 0}, {0, 1, 0}},
        {{ 0, 0,-1}, {-1, 0, 0}, {0, 1, 0}},
    };
    MeshData m;
    for (const Face& f : faces) {
        unsigned base = (unsigned)m.vertices.size();
        m.vertices.push_back(makeVertex(h * (f.n - f.u - f.v), f.n, {0, 0}));
        m.vertices.push_back(makeVertex(h * (f.n + f.u - f.v), f.n, {1, 0}));
        m.vertices.push_back(makeVertex(h * (f.n + f.u + f.v), f.n, {1, 1}));
        m.vertices.push_back(makeVertex(h * (f.n - f.u + f.v), f.n, {0, 1}));
        const unsigned t[] = {0, 1, 2, 0, 2, 3};
        for (unsigned i : t) m.indices.push_back(base + i);
    }
    return finish(std::move(m));
}

MeshData sphere(int sectors, int stacks, float radius) {
    sectors = std::max(sectors, 3);
    stacks  = std::max(stacks, 2);
    MeshData m;
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI / 2.0f - PI * i / stacks;       // +90deg (top) .. -90deg (bottom)
        float ring = std::cos(phi), y = std::sin(phi);
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * PI * j / sectors;
            glm::vec3 n(ring * std::cos(theta), y, ring * std::sin(theta));
            m.vertices.push_back(makeVertex(n * radius, n,
                                            glm::vec2((float)j / sectors, 1.0f - (float)i / stacks)));
        }
    }
    for (int i = 0; i < stacks; ++i) {
        unsigned k1 = i * (sectors + 1), k2 = k1 + sectors + 1;
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0)          { m.indices.push_back(k1);     m.indices.push_back(k1 + 1); m.indices.push_back(k2); }
            if (i != stacks - 1) { m.indices.push_back(k1 + 1); m.indices.push_back(k2 + 1); m.indices.push_back(k2); }
        }
    }
    return finish(std::move(m));
}

MeshData frustum(float rb, float rt, float height, int segments) {
    segments = std::max(segments, 3);
    float hh = height * 0.5f;
    MeshData m;
    // Side: slanted normals so cone / truncated cone are lit correctly.
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * PI * i / segments;
        float c = std::cos(a), s = std::sin(a);
        glm::vec3 n = glm::normalize(glm::vec3(c * height, rb - rt, s * height));
        float u = (float)i / segments;
        m.vertices.push_back(makeVertex(glm::vec3(rb * c, -hh, rb * s), n, {u, 0.0f}));
        m.vertices.push_back(makeVertex(glm::vec3(rt * c,  hh, rt * s), n, {u, 1.0f}));
    }
    for (int i = 0; i < segments; ++i) {
        unsigned b0 = 2 * i, t0 = b0 + 1, b1 = b0 + 2, t1 = b0 + 3;
        m.indices.insert(m.indices.end(), {b0, t0, b1,  b1, t0, t1});
    }
    if (rb > 1e-5f) appendCap(m, rb, -hh, segments, false);
    if (rt > 1e-5f) appendCap(m, rt,  hh, segments, true);
    return finish(std::move(m));
}

MeshData tetrahedron(float edge) {
    float k = edge / (2.0f * std::sqrt(2.0f));
    const glm::vec3 p[4] = {
        k * glm::vec3( 1,  1,  1), k * glm::vec3( 1, -1, -1),
        k * glm::vec3(-1,  1, -1), k * glm::vec3(-1, -1,  1)};
    const int faces[4][3] = {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}};
    const glm::vec2 uvs[3] = {{0, 0}, {1, 0}, {0.5f, 1.0f}};
    MeshData m;
    for (auto& f : faces) {
        glm::vec3 a = p[f[0]], b = p[f[1]], c = p[f[2]];
        glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
        if (glm::dot(n, a + b + c) < 0.0f) { std::swap(b, c); n = -n; }   // make it point outwards
        unsigned base = (unsigned)m.vertices.size();
        m.vertices.push_back(makeVertex(a, n, uvs[0]));
        m.vertices.push_back(makeVertex(b, n, uvs[1]));
        m.vertices.push_back(makeVertex(c, n, uvs[2]));
        m.indices.insert(m.indices.end(), {base, base + 1, base + 2});
    }
    return finish(std::move(m));
}

MeshData torus(float R, float r, int majorSeg, int minorSeg) {
    majorSeg = std::max(majorSeg, 3);
    minorSeg = std::max(minorSeg, 3);
    MeshData m;
    for (int i = 0; i <= majorSeg; ++i) {
        float theta = 2.0f * PI * i / majorSeg;
        float ct = std::cos(theta), st = std::sin(theta);
        for (int j = 0; j <= minorSeg; ++j) {
            float phi = 2.0f * PI * j / minorSeg;
            float cp = std::cos(phi), sp = std::sin(phi);
            glm::vec3 pos((R + r * cp) * ct, r * sp, (R + r * cp) * st);
            glm::vec3 n(cp * ct, sp, cp * st);
            m.vertices.push_back(makeVertex(pos, n, {(float)i / majorSeg, (float)j / minorSeg}));
        }
    }
    for (int i = 0; i < majorSeg; ++i) {
        for (int j = 0; j < minorSeg; ++j) {
            unsigned a = i * (minorSeg + 1) + j, b = a + minorSeg + 1;
            m.indices.insert(m.indices.end(), {a, a + 1, b,  b, a + 1, b + 1});
        }
    }
    return finish(std::move(m));
}

MeshData prism(int sides, float radius, float height) {
    sides = std::max(sides, 3);
    float hh = height * 0.5f;
    MeshData m;
    for (int i = 0; i < sides; ++i) {
        float a0 = 2.0f * PI * i / sides, a1 = 2.0f * PI * (i + 1) / sides;
        glm::vec3 n(std::cos((a0 + a1) * 0.5f), 0.0f, std::sin((a0 + a1) * 0.5f));
        float u0 = (float)i / sides, u1 = (float)(i + 1) / sides;
        unsigned base = (unsigned)m.vertices.size();
        m.vertices.push_back(makeVertex({radius * std::cos(a0), -hh, radius * std::sin(a0)}, n, {u0, 0}));
        m.vertices.push_back(makeVertex({radius * std::cos(a0),  hh, radius * std::sin(a0)}, n, {u0, 1}));
        m.vertices.push_back(makeVertex({radius * std::cos(a1), -hh, radius * std::sin(a1)}, n, {u1, 0}));
        m.vertices.push_back(makeVertex({radius * std::cos(a1),  hh, radius * std::sin(a1)}, n, {u1, 1}));
        m.indices.insert(m.indices.end(), {base, base + 1, base + 2,  base + 2, base + 1, base + 3});
    }
    appendCap(m, radius, -hh, sides, false);
    appendCap(m, radius,  hh, sides, true);
    return finish(std::move(m));
}

} // namespace Geometry

// =============================================================================
// Mesh utilities
// =============================================================================
namespace MeshUtil {

namespace {

using PosKey = std::array<long long, 3>;

PosKey quantize(const glm::vec3& p) {
    return {std::llround(p.x * 1e4f), std::llround(p.y * 1e4f), std::llround(p.z * 1e4f)};
}

// Maps each vertex to an id shared by all vertices at the same position.
std::vector<unsigned> canonicalIds(const MeshData& m) {
    std::map<PosKey, unsigned> ids;
    std::vector<unsigned> out(m.vertices.size());
    for (size_t i = 0; i < m.vertices.size(); ++i) {
        auto it = ids.emplace(quantize(m.vertices[i].position), (unsigned)ids.size()).first;
        out[i] = it->second;
    }
    return out;
}

void boundingBox(const MeshData& m, glm::vec3& lo, glm::vec3& hi) {
    lo = glm::vec3(1e30f);
    hi = glm::vec3(-1e30f);
    for (const auto& v : m.vertices) {
        lo = glm::min(lo, v.position);
        hi = glm::max(hi, v.position);
    }
}

} // namespace

glm::vec3 hsv2rgb(float h, float s, float v) {
    h = h - std::floor(h);
    float c = v * s, hp = h * 6.0f;
    float x = c * (1.0f - std::fabs(std::fmod(hp, 2.0f) - 1.0f));
    glm::vec3 rgb(0.0f);
    if      (hp < 1) rgb = {c, x, 0};
    else if (hp < 2) rgb = {x, c, 0};
    else if (hp < 3) rgb = {0, c, x};
    else if (hp < 4) rgb = {0, x, c};
    else if (hp < 5) rgb = {x, 0, c};
    else             rgb = {c, 0, x};
    return rgb + glm::vec3(v - c);
}

void computeSmoothNormals(MeshData& m) {
    std::vector<unsigned> cid = canonicalIds(m);
    std::vector<glm::vec3> acc(m.vertices.size(), glm::vec3(0.0f));  // indexed by canonical id
    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        unsigned a = m.indices[t], b = m.indices[t + 1], c = m.indices[t + 2];
        glm::vec3 fn = glm::cross(m.vertices[b].position - m.vertices[a].position,
                                  m.vertices[c].position - m.vertices[a].position);  // length = 2*area
        acc[cid[a]] += fn; acc[cid[b]] += fn; acc[cid[c]] += fn;
    }
    for (size_t i = 0; i < m.vertices.size(); ++i) {
        const glm::vec3& n = acc[cid[i]];
        m.vertices[i].normal = glm::length(n) > 1e-12f ? glm::normalize(n) : glm::vec3(0, 1, 0);
    }
}

void assignVertexColors(MeshData& m, ColorScheme scheme, const glm::vec3& a, const glm::vec3& b, int axis) {
    if (m.vertices.empty()) return;
    glm::vec3 lo, hi;
    boundingBox(m, lo, hi);
    glm::vec3 ext = hi - lo;
    const float eps = 1e-6f;

    for (auto& v : m.vertices) {
        glm::vec3 q = v.position - lo;
        float t = 0.0f;
        if (axis >= 0 && axis <= 2) {
            t = ext[axis] > eps ? q[axis] / ext[axis] : 0.0f;
        } else if (scheme == ColorScheme::Rainbow) {
            int used = 0;
            for (int k = 0; k < 3; ++k)
                if (ext[k] > eps) { t += q[k] / ext[k]; ++used; }
            t = used ? t / used : 0.0f;
        } else {
            int k = ext.y > eps ? 1 : 0;
            t = ext[k] > eps ? q[k] / ext[k] : 0.0f;
        }
        v.color = (scheme == ColorScheme::Rainbow) ? hsv2rgb(t * 0.83f, 0.85f, 1.0f)
                                                   : glm::mix(a, b, t);
    }
}

void normalizeToUnit(MeshData& m, float size) {
    if (m.vertices.empty()) return;
    glm::vec3 lo, hi;
    boundingBox(m, lo, hi);
    glm::vec3 center = (lo + hi) * 0.5f, ext = hi - lo;
    float maxExt = std::max(ext.x, std::max(ext.y, ext.z));
    float s = maxExt > 1e-12f ? size / maxExt : 1.0f;
    for (auto& v : m.vertices) v.position = (v.position - center) * s;
}

void planarUV(MeshData& m) {
    if (m.vertices.empty()) return;
    glm::vec3 lo, hi;
    boundingBox(m, lo, hi);
    glm::vec3 ext = hi - lo;
    for (auto& v : m.vertices) {
        v.uv.x = ext.x > 1e-6f ? (v.position.x - lo.x) / ext.x : 0.0f;
        v.uv.y = ext.y > 1e-6f ? (v.position.y - lo.y) / ext.y : 0.0f;
    }
}

void buildWireIndices(MeshData& m) {
    struct EdgeInfo {
        unsigned i0 = 0, i1 = 0;   // original vertex indices of the first occurrence
        int count = 0;
        glm::vec3 n0{0.0f};
        bool sharp = false;
    };
    std::vector<unsigned> cid = canonicalIds(m);
    std::map<std::pair<unsigned, unsigned>, EdgeInfo> edges;

    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        const unsigned tri[3] = {m.indices[t], m.indices[t + 1], m.indices[t + 2]};
        glm::vec3 n = glm::cross(m.vertices[tri[1]].position - m.vertices[tri[0]].position,
                                 m.vertices[tri[2]].position - m.vertices[tri[0]].position);
        float len = glm::length(n);
        if (len < 1e-12f) continue;   // degenerate (e.g. sphere poles, cone apex)
        n /= len;

        for (int e = 0; e < 3; ++e) {
            unsigned u = tri[e], v = tri[(e + 1) % 3];
            unsigned cu = cid[u], cv = cid[v];
            if (cu == cv) continue;
            EdgeInfo& info = edges[{std::min(cu, cv), std::max(cu, cv)}];
            if (info.count == 0) { info.i0 = u; info.i1 = v; info.n0 = n; }
            else if (std::fabs(glm::dot(info.n0, n)) < 0.99999f) info.sharp = true;
            ++info.count;
        }
    }
    m.wireIndices.clear();
    for (const auto& kv : edges) {
        const EdgeInfo& e = kv.second;
        if (e.count == 1 || e.count > 2 || e.sharp) {
            m.wireIndices.push_back(e.i0);
            m.wireIndices.push_back(e.i1);
        }
    }
}

} // namespace MeshUtil
