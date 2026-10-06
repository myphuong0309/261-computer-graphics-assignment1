// =============================================================================
// Screenshot.h - save the current framebuffer as a PNG
// =============================================================================
#pragma once
#include <string>

// Reads the back buffer (width x height) and writes a PNG. Returns false on failure.
bool saveScreenshot(const std::string& path, int width, int height);
