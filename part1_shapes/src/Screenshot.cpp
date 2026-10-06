// =============================================================================
// Screenshot.cpp
// =============================================================================
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/stb/stb_image_write.h"
#include "Screenshot.h"
#include <vector>
#include <GL/glew.h>

bool saveScreenshot(const std::string& path, int width, int height) {
    std::vector<unsigned char> px((size_t)width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    stbi_flip_vertically_on_write(1);   // OpenGL rows start at the bottom
    return stbi_write_png(path.c_str(), width, height, 3, px.data(), width * 3) != 0;
}
