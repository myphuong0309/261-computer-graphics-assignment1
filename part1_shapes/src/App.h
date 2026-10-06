// =============================================================================
// App.h - state shared between main.cpp (window / input / render loop) and Gui.cpp
// =============================================================================
#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Camera.h"      // shared code: common/src
#include "Scene.h"

struct LightState {
    glm::vec3 position {5.0f, 8.0f, 6.0f};
    glm::vec3 ambient  {0.25f};
    glm::vec3 diffuse  {1.0f};
    glm::vec3 specular {1.0f};
};

struct AppState {
    Scene      scene;
    Camera     camera;
    LightState light;
    glm::vec3  background {0.08f, 0.10f, 0.14f};

    int   winW = 1280, winH = 800;
    float fps  = 0.0f;
    bool  showHelp = false;
    bool  quitRequested = false;
    bool  openImportPopup = false;          // set by the menu, consumed by Gui::draw
    bool  screenshotRequested = false;      // set by the menu / F12, consumed by main loop
    std::string status;                     // one-line message shown in the panel

    // Mouse state for Ctrl + left-drag (rotate the selected object)
    bool   objectDragging = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    // Camera presets. Camera::reset() from Part 2 looks at the scene from -Z, we prefer +Z
    // (so 2D shapes, whose front face points to +Z, are seen unmirrored).
    void resetCamera() {
        camera.yaw = 90.0f; camera.pitch = 25.0f; camera.distance = 11.0f;
        camera.target = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    void frontView() {   // looking straight at the XY plane: ideal for 2D shapes
        camera.yaw = 90.0f; camera.pitch = 0.0f; camera.distance = 8.0f;
        camera.target = glm::vec3(0.0f, 0.8f, 0.0f);
    }
    void topView() {
        camera.yaw = 90.0f; camera.pitch = 89.0f; camera.distance = 12.0f;
        camera.target = glm::vec3(0.0f);
    }
};
