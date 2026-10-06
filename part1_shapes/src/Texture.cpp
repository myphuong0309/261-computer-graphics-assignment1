// =============================================================================
// Texture.cpp
// =============================================================================
#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb/stb_image.h"
#include "Texture.h"
#include <vector>

void Texture2D::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2D::upload(const unsigned char* rgba, int w, int h) {
    width = w; height = h;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

std::shared_ptr<Texture2D> Texture2D::fromFile(const std::string& path, std::string& error) {
    stbi_set_flip_vertically_on_load(1);   // image row 0 is the top; OpenGL's v=0 is the bottom
    int w = 0, h = 0, comp = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!pixels) {
        error = "cannot load image '" + path + "': " + stbi_failure_reason();
        return nullptr;
    }
    auto tex = std::make_shared<Texture2D>();
    tex->path = path;
    tex->upload(pixels, w, h);
    stbi_image_free(pixels);
    error.clear();
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::checkerboard() {
    const int N = 64;   // 8x8 squares of 8 pixels
    std::vector<unsigned char> px(N * N * 4);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            bool light = ((x / 8) + (y / 8)) % 2 == 0;
            unsigned char c = light ? 235 : 60;
            unsigned char* p = &px[(y * N + x) * 4];
            p[0] = c; p[1] = c; p[2] = c; p[3] = 255;
        }
    auto tex = std::make_shared<Texture2D>();
    tex->upload(px.data(), N, N);
    return tex;
}

std::shared_ptr<Texture2D> TextureCache::get(const std::string& path, std::string& error) {
    auto it = cache_.find(path);
    if (it != cache_.end()) { error.clear(); return it->second; }
    std::shared_ptr<Texture2D> tex = path.empty() ? Texture2D::checkerboard() : Texture2D::fromFile(path, error);
    if (tex) { cache_[path] = tex; error.clear(); }
    return tex;
}
