// =============================================================================
// Camera.h - Arcball / orbit camera with zoom, pan, rotate
// =============================================================================
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // Spherical coordinates around target
    float yaw      = -90.0f; // degrees
    float pitch    =  30.0f; // degrees
    float distance =   8.0f; // from target
    glm::vec3 target = glm::vec3(0.0f);

    float fov   = 45.0f;
    float near_ = 0.1f;
    float far_  = 100.0f;

    // Interaction state
    bool  isDragging  = false;
    bool  isPanning   = false;
    double lastX = 0.0, lastY = 0.0;
    float sensitivity = 0.25f;
    float panSpeed    = 0.005f;
    float zoomSpeed   = 0.5f;

    glm::vec3 position() const;
    glm::mat4 view()       const;
    glm::mat4 projection(float aspect) const;

    void mouseButton(int button, int action, double x, double y);
    void mouseMove(double x, double y);
    void scroll(double yOffset);
    void reset();
};
