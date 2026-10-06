// =============================================================================
// Shader.cpp - Implementation
// =============================================================================
#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

Shader::Shader(const char* vertPath, const char* fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    ID = glCreateProgram();
    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);

    GLint ok = 0;
    glGetProgramiv(ID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(ID, 1024, nullptr, log);
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Shader::use()  const { glUseProgram(ID); }

void Shader::setMat4(const std::string& n, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::setMat3(const std::string& n, const glm::mat3& m) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::setVec3(const std::string& n, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v));
}
void Shader::setFloat(const std::string& n, float val) const {
    glUniform1f(glGetUniformLocation(ID, n.c_str()), val);
}
void Shader::setInt(const std::string& n, int val) const {
    glUniform1i(glGetUniformLocation(ID, n.c_str()), val);
}
void Shader::setBool(const std::string& n, bool val) const {
    glUniform1i(glGetUniformLocation(ID, n.c_str()), (int)val);
}

std::string Shader::readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::string("Cannot open shader: ") + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return s;
}
