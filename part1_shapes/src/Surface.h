// =============================================================================
// Surface.h - mesh for the mathematical surface z = f(x, y)
// =============================================================================
#pragma once
#include <string>
#include "Geometry.h"
#include "ExprParser.h"

struct SurfaceParams {
    std::string expression = "sin(x) * cos(y)";
    float xMin = -3.0f, xMax = 3.0f;
    float yMin = -3.0f, yMax = 3.0f;
    int   resolution = 64;     // grid cells per axis
};

namespace Surface {

// The surface is centred in x/y and uniformly scaled so its footprint is ~1 unit wide (z uses the same scale).
// Mathematical z becomes world "up" (+Y); mathematical y becomes world -Z. Cells where f is not finite are skipped.
// Returns false (and fills `error`) when the expression or the ranges are invalid.
bool build(const SurfaceParams& p, MeshData& out, std::string& error);

} // namespace Surface
