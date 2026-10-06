// =============================================================================
// Surface.cpp
// =============================================================================
#include "Surface.h"
#include <algorithm>
#include <cmath>

namespace Surface {

bool build(const SurfaceParams& p, MeshData& out, std::string& error) {
    Expression expr;
    if (!expr.compile(p.expression, error)) return false;
    if (!(p.xMax > p.xMin) || !(p.yMax > p.yMin)) { error = "range max must be greater than min"; return false; }
    int n = std::min(std::max(p.resolution, 2), 400);

    const double xMin = p.xMin, xMax = p.xMax, yMin = p.yMin, yMax = p.yMax;
    const double dx = (xMax - xMin) / n, dy = (yMax - yMin) / n;
    const double scale = 1.0 / std::max(xMax - xMin, yMax - yMin);
    const double cx = 0.5 * (xMin + xMax), cy = 0.5 * (yMin + yMax);
    const double zLimit = 1e3;

    MeshData m;
    std::vector<char> ok((size_t)(n + 1) * (n + 1), 1);
    m.vertices.reserve(ok.size());

    for (int j = 0; j <= n; ++j) {
        for (int i = 0; i <= n; ++i) {
            double x = xMin + i * dx, y = yMin + j * dy;
            double z = expr.eval(x, y);
            bool finite = std::isfinite(z) && std::fabs(z) < zLimit;
            ok[(size_t)j * (n + 1) + i] = finite;

            Vertex v;
            if (finite) {
                // central differences of f (evaluated slightly outside the range at the border)
                double h = 1e-4 * std::max(xMax - xMin, yMax - yMin);
                double fx = (expr.eval(x + h, y) - expr.eval(x - h, y)) / (2.0 * h);
                double fy = (expr.eval(x, y + h) - expr.eval(x, y - h)) / (2.0 * h);
                if (!std::isfinite(fx) || !std::isfinite(fy)) fx = fy = 0.0;
                // math normal (-fx, -fy, 1)  ->  world (x, z, -y): (-fx, 1, fy)
                v.normal = glm::normalize(glm::vec3((float)-fx, 1.0f, (float)fy));
                v.position = glm::vec3((float)((x - cx) * scale), (float)(z * scale), (float)(-(y - cy) * scale));
            } else {
                v.position = glm::vec3((float)((x - cx) * scale), 0.0f, (float)(-(y - cy) * scale));
                v.normal   = glm::vec3(0, 1, 0);
            }
            v.uv = glm::vec2((float)i / n, (float)j / n);
            m.vertices.push_back(v);
        }
    }

    auto idx = [n](int i, int j) { return (unsigned)(j * (n + 1) + i); };
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            unsigned a = idx(i, j), b = idx(i + 1, j), c = idx(i, j + 1), d = idx(i + 1, j + 1);
            if (!(ok[a] && ok[b] && ok[c] && ok[d])) continue;
            // y_math grows towards -Z, so (a, b, c) is counter-clockwise seen from +Y
            m.indices.insert(m.indices.end(), {a, b, c,  b, d, c});
        }
    }
    if (m.indices.empty()) { error = "f(x, y) is not finite anywhere in this range"; return false; }

    // Colour by height (rainbow along Y), then the edge list.
    MeshUtil::assignVertexColors(m, MeshUtil::ColorScheme::Rainbow, glm::vec3(1), glm::vec3(1), 1);
    MeshUtil::buildWireIndices(m);
    out = std::move(m);
    error.clear();
    return true;
}

} // namespace Surface
