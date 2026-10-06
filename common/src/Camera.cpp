// =============================================================================
// Camera.cpp
// =============================================================================
#include "Camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

glm::vec3 Camera::position() const {
    float yawR   = glm::radians(yaw);
    float pitchR = glm::radians(pitch);
    float x = distance * std::cos(pitchR) * std::cos(yawR);
    float y = distance * std::sin(pitchR);
    float z = distance * std::cos(pitchR) * std::sin(yawR);
    return target + glm::vec3(x, y, z);
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, near_, far_);
}

void Camera::mouseButton(int button, int action, double x, double y) {
    bool pressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_LEFT)  { isDragging = pressed; }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) { isPanning  = pressed; }
    if (pressed) { lastX = x; lastY = y; }
}

void Camera::mouseMove(double x, double y) {
    float dx = (float)(x - lastX);
    float dy = (float)(y - lastY);
    lastX = x; lastY = y;

    if (isDragging) {
        yaw   += dx * sensitivity;
        pitch -= dy * sensitivity;
        pitch  = std::max(-89.0f, std::min(89.0f, pitch));
    }
    if (isPanning) {
        // Compute right and up vectors from camera orientation
        glm::vec3 front = glm::normalize(target - position());
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        glm::vec3 up    = glm::normalize(glm::cross(right, front));
        target -= right * (dx * panSpeed * distance);
        target += up    * (dy * panSpeed * distance);
    }
}

void Camera::scroll(double yOffset) {
    distance -= (float)yOffset * zoomSpeed;
    distance  = std::max(1.0f, std::min(50.0f, distance));
}

void Camera::reset() {
    yaw = -90.0f; pitch = 30.0f; distance = 8.0f;
    target = glm::vec3(0.0f);
}
