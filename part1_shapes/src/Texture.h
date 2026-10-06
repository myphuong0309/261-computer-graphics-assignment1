// =============================================================================
// Texture.h - 2D texture loaded from an image file (via stb_image) + a small cache
// =============================================================================
#pragma once
#include <map>
#include <memory>
#include <string>
#include <GL/glew.h>

class Texture2D {
public:
    GLuint id = 0;
    int width = 0, height = 0;
    std::string path;   // empty for the built-in checkerboard

    Texture2D() = default;
    ~Texture2D() { if (id) glDeleteTextures(1, &id); }
    Texture2D(const Texture2D&)            = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    void bind(GLuint unit = 0) const;

    // Loads png/jpg/bmp/tga... Returns nullptr and fills `error` on failure.
    static std::shared_ptr<Texture2D> fromFile(const std::string& path, std::string& error);
    // Procedural 8x8 checkerboard, used when the user has not chosen an image.
    static std::shared_ptr<Texture2D> checkerboard();

private:
    void upload(const unsigned char* rgba, int w, int h);
};

// Keeps each image loaded once, however many objects use it.
class TextureCache {
public:
    // Empty path -> the built-in checkerboard.
    std::shared_ptr<Texture2D> get(const std::string& path, std::string& error);
private:
    std::map<std::string, std::shared_ptr<Texture2D>> cache_;
};
