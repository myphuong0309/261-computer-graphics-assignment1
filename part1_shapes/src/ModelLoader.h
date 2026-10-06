// =============================================================================
// ModelLoader.h - hand-written Wavefront .obj and Stanford .ply importers
// =============================================================================
#pragma once
#include <string>
#include "Geometry.h"

namespace ModelLoader {

// Dispatches on the file extension (.obj / .ply, case-insensitive).
// The result is centred, scaled to fit a 1-unit box, and has normals, UVs, vertex colors and
// wire edges (missing attributes are generated). Returns false and fills `error` on failure.
bool load(const std::string& path, MeshData& out, std::string& error);

bool loadOBJ(const std::string& path, MeshData& out, std::string& error);
bool loadPLY(const std::string& path, MeshData& out, std::string& error);

} // namespace ModelLoader
