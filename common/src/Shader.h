// Shader.h - GLSL shader program loader and manager

#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    GLuint ID;  // idenitfier for the shader program

    Shader() : ID(0) {}
    Shader(const char* vertPath, const char* fragPath);
    ~Shader() {}

    // activate the shader program for use in rendering
    void use() const;

    // setters for uniform variables in the shader
    void setMat4 (const std::string& name, const glm::mat4& mat) const;
    void setMat3 (const std::string& name, const glm::mat3& mat) const;
    void setVec3 (const std::string& name, const glm::vec3& v)   const;
    void setFloat(const std::string& name, float val)            const;
    void setInt  (const std::string& name, int val)              const;
    void setBool (const std::string& name, bool val)             const;

private:
    static std::string readFile(const char* path);
    static GLuint compileShader(GLenum type, const std::string& src);
};
