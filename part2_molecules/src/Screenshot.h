// =============================================================================
// Screenshot.h - save (part of) the current framebuffer as a PNG
// =============================================================================
#pragma once
#include <string>

// Reads the rectangle (x, y, width, height) of the back buffer (origin bottom-left) and writes a PNG.
// Returns false on failure.
bool saveScreenshot(const std::string& path, int x, int y, int width, int height);
