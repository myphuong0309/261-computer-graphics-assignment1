// =============================================================================
// Paths.h - locating shaders / assets whether we run from part1_shapes/ or build/
// =============================================================================
#pragma once
#include <string>
#include <vector>

namespace Paths {

// "shaders/x.vert" -> the first existing of ./shaders/x.vert, ../shaders/x.vert
std::string resolve(const std::string& relative);

// Files in a directory (resolved like above) whose extension is in `extensions` (lower-case, with dot), sorted.
// Returned paths are directly loadable (directory prefix included).
std::vector<std::string> listFiles(const std::string& relativeDir, const std::vector<std::string>& extensions);

} // namespace Paths
