// =============================================================================
// main.cpp - window, input callbacks and render loop
// Part 1: Drawing Basic Shapes  (HCMUT Computer Graphics, Assignment 1, 2026-2027)
//
// Controls (see also Help > Controls):
//   Left drag  - orbit            Right drag - pan          Wheel - zoom
//   Ctrl+Left drag - rotate the selected object
//   Arrows - orbit, Shift+Arrows - pan, +/- - zoom, R - reset camera
//   W - toggle wireframe, Del - delete selected, F12 - screenshot, Esc - quit
//
// CLI (for screenshots / testing):  --demo  --add <shape> (repeatable)  --mode <flat|gouraud|phong|texture|wire>
//   --select <index|-1> --no-grid --light x,y,z --texture <png>  --surface "<expr>"  --model <file>  --view <front|top> --distance <d>  --screenshot <file.png> [--with-gui] [--frames N]
// =============================================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "App.h"
#include "Gui.h"
#include "Paths.h"
#include "Screenshot.h"
#include "Shader.h"      // shared code: common/src

namespace {

AppState* g_app = nullptr;
const int kPanelWidth = 380;   // inspector width + margin (pixels)

// ---------------------------------------------------------------------------
// GLFW callbacks (same structure as part2_molecules/main.cpp)
// ---------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_app->winW = w; g_app->winH = h;
    glViewport(0, 0, w, h);
}

void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
    AppState& app = *g_app;
    double x, y; glfwGetCursorPos(w, &x, &y);

    if (action == GLFW_PRESS && ImGui::GetIO().WantCaptureMouse) return;   // clicking the GUI
    // (releases are always processed so a drag that ends over the GUI does not get stuck)

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) && app.scene.selectedObject()) {
            app.objectDragging = true;               // Ctrl + drag: rotate the selected object
            app.lastMouseX = x; app.lastMouseY = y;
            return;
        }
        if (action == GLFW_RELEASE && app.objectDragging) { app.objectDragging = false; return; }
    }
    app.camera.mouseButton(button, action, x, y);
}

void cursorPosCallback(GLFWwindow*, double x, double y) {
    AppState& app = *g_app;
    if (app.objectDragging) {
        if (SceneObject* o = app.scene.selectedObject()) {
            o->st.rotation.y += (float)(x - app.lastMouseX) * 0.4f;
            o->st.rotation.x += (float)(y - app.lastMouseY) * 0.4f;
        }
        app.lastMouseX = x; app.lastMouseY = y;
        return;
    }
    if (ImGui::GetIO().WantCaptureMouse && !app.camera.isDragging && !app.camera.isPanning) return;
    app.camera.mouseMove(x, y);
}

void scrollCallback(GLFWwindow*, double, double yo) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    g_app->camera.scroll(yo);
}

void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS || ImGui::GetIO().WantCaptureKeyboard) return;
    AppState& app = *g_app;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, 1); break;
        case GLFW_KEY_R:      app.resetCamera(); break;
        case GLFW_KEY_W:      app.scene.toggleWireframe(); break;
        case GLFW_KEY_H:      app.showHelp = !app.showHelp; break;
        case GLFW_KEY_DELETE: app.scene.removeSelected(); break;
        case GLFW_KEY_F12:    app.screenshotRequested = true; break;
    }
}

// Held keys: arrows orbit (Shift = pan), +/- or PageUp/PageDown zoom.
void processHeldKeys(GLFWwindow* w, AppState& app, float dt) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    Camera& cam = app.camera;
    auto down = [&](int k) { return glfwGetKey(w, k) == GLFW_PRESS; };
    bool shift = down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT);

    float dx = (down(GLFW_KEY_RIGHT) ? 1.0f : 0.0f) - (down(GLFW_KEY_LEFT) ? 1.0f : 0.0f);
    float dy = (down(GLFW_KEY_UP)    ? 1.0f : 0.0f) - (down(GLFW_KEY_DOWN) ? 1.0f : 0.0f);
    if (shift) {
        glm::vec3 front = glm::normalize(cam.target - cam.position());
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up    = glm::normalize(glm::cross(right, front));
        cam.target += (right * dx + up * dy) * (0.6f * cam.distance * dt);
    } else {
        cam.yaw   += dx * 90.0f * dt;
        cam.pitch  = std::max(-89.0f, std::min(89.0f, cam.pitch + dy * 90.0f * dt));
    }
    float zoom = (down(GLFW_KEY_MINUS) || down(GLFW_KEY_KP_SUBTRACT) || down(GLFW_KEY_PAGE_DOWN) ? 1.0f : 0.0f) -
                 (down(GLFW_KEY_EQUAL) || down(GLFW_KEY_KP_ADD)      || down(GLFW_KEY_PAGE_UP)   ? 1.0f : 0.0f);
    cam.distance = std::max(1.0f, std::min(50.0f, cam.distance + zoom * 0.8f * cam.distance * dt));
}

// ---------------------------------------------------------------------------
// Command line
// ---------------------------------------------------------------------------
struct CliOptions {
    bool demo = false, withGui = false, noGrid = false;
    std::string screenshot, view, mode, surface, model;
    std::vector<std::string> add;
    int frames = 8, select = -2;   // -2: leave the default (last added), -1: nothing selected
    float distance = 0.0f;
    bool hasLight = false;
    glm::vec3 light{0.0f};
    std::string texture;
};

std::string squash(std::string s) {   // "Truncated Cone" -> "truncatedcone"
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return !std::isalnum(c); }), s.end());
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if      (a == "--demo")       o.demo = true;
        else if (a == "--with-gui")   o.withGui = true;
        else if (a == "--no-grid")    o.noGrid = true;
        else if (a == "--screenshot") o.screenshot = next();
        else if (a == "--view")       o.view = next();
        else if (a == "--mode")       o.mode = next();
        else if (a == "--surface")    o.surface = next();
        else if (a == "--model")      o.model = next();
        else if (a == "--add")        o.add.push_back(next());
        else if (a == "--distance")   o.distance = (float)std::atof(next().c_str());
        else if (a == "--light") {   // --light x,y,z
            o.hasLight = std::sscanf(next().c_str(), "%f,%f,%f", &o.light.x, &o.light.y, &o.light.z) == 3;
        }
        else if (a == "--select")     o.select = std::atoi(next().c_str());
        else if (a == "--texture")    o.texture = next();
        else if (a == "--frames")     o.frames = std::max(1, std::atoi(next().c_str()));
        else std::cerr << "Unknown option: " << a << "\n";
    }
    return o;
}

void setupSceneFromCli(AppState& app, const CliOptions& o) {
    std::string err;
    if (o.demo) { app.scene.addDemo(); app.camera.distance = 15.0f; app.camera.target = glm::vec3(0, 1.2f, 0); }
    for (const std::string& name : o.add) {
        bool found = false;
        for (int k = 0; k < (int)ShapeKind::Count && !found; ++k) {
            if (k == (int)ShapeKind::Model) continue;
            if (squash(shapeName((ShapeKind)k)).rfind(squash(name), 0) == 0) {
                if (!app.scene.addShape((ShapeKind)k, err)) std::cerr << err << "\n";
                found = true;
            }
        }
        if (!found) std::cerr << "Unknown shape: " << name << "\n";
    }
    if (!o.surface.empty()) {
        if (SceneObject* s = app.scene.addShape(ShapeKind::Surface, err)) {
            s->st.params.surface.expression = o.surface;
            if (!s->rebuild()) std::cerr << s->error << "\n";
        }
    }
    if (!o.model.empty() && !app.scene.addModel(o.model, err)) std::cerr << err << "\n";

    if (!o.mode.empty()) {
        RenderMode m = RenderMode::Phong;
        std::string k = squash(o.mode);
        if      (k.rfind("flat", 0) == 0)    m = RenderMode::Flat;
        else if (k.rfind("gour", 0) == 0)    m = RenderMode::Gouraud;
        else if (k.rfind("tex", 0) == 0)     m = RenderMode::Texture;
        else if (k.rfind("wire", 0) == 0)    m = RenderMode::Wireframe;
        for (auto& obj : app.scene.objects) obj->st.mode = m;
    }
    if (!o.demo && app.scene.objects.size() > 1) {   // spread CLI-created shapes in a row so they do not overlap
        float n = (float)app.scene.objects.size();
        for (size_t i = 0; i < app.scene.objects.size(); ++i)
            app.scene.objects[i]->st.position.x = ((float)i - (n - 1.0f) * 0.5f) * 1.8f;
    }
    if (!o.texture.empty())
        for (auto& obj : app.scene.objects) { obj->st.texturePath = o.texture; app.scene.applyTexture(*obj); }
    if (o.select >= -1 && o.select < (int)app.scene.objects.size()) app.scene.selected = o.select;
    if (o.noGrid) app.scene.showGrid = app.scene.showAxes = false;
    if (o.view == "front") app.frontView();
    else if (o.view == "top") app.topView();
    if (o.distance > 0.0f) app.camera.distance = o.distance;
    if (o.hasLight) app.light.position = o.light;
}

std::string timestampedName() {
    char buf[64];
    std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "screenshot_%Y%m%d_%H%M%S.png", std::localtime(&t));
    return buf;
}

} // namespace

// =============================================================================
int main(int argc, char** argv) {
    CliOptions cli = parseArgs(argc, argv);
    AppState app;
    g_app = &app;
    app.resetCamera();

    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);   // MSAA

    GLFWwindow* window = glfwCreateWindow(app.winW, app.winH,
        "Part 1: Drawing Basic Shapes - HCMUT CG 2026-2027", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; return 1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);   // 2D shapes are visible from both sides; lighting is two-sided in the shader

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.FontGlobalScale = 1.1f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    int exitCode = 0;
    try {
        Shader shader(Paths::resolve("shaders/shapes.vert").c_str(),
                      Paths::resolve("shaders/shapes.frag").c_str());

        setupSceneFromCli(app, cli);

        glfwGetFramebufferSize(window, &app.winW, &app.winH);
        glViewport(0, 0, app.winW, app.winH);

        float lastTime = (float)glfwGetTime(), fpsAccum = 0.0f;
        int fpsFrames = 0, frame = 0;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            float now = (float)glfwGetTime(), dt = now - lastTime;
            lastTime = now;
            fpsAccum += dt; ++fpsFrames;
            if (fpsAccum >= 0.5f) { app.fps = fpsFrames / fpsAccum; fpsAccum = 0.0f; fpsFrames = 0; }

            processHeldKeys(window, app, dt);

            // ---- scene
            glClearColor(app.background.r, app.background.g, app.background.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // The inspector panel covers the left side of the window; draw the 3D scene to its right.
            int panelW = std::min(kPanelWidth, app.winW / 2);
            glViewport(panelW, 0, app.winW - panelW, app.winH);
            float aspect = (float)(app.winW - panelW) / (float)std::max(app.winH, 1);
            shader.use();
            shader.setMat4("view", app.camera.view());
            shader.setMat4("projection", app.camera.projection(aspect));
            shader.setVec3("viewPos", app.camera.position());
            shader.setVec3("light.position", app.light.position);
            shader.setVec3("light.ambient",  app.light.ambient);
            shader.setVec3("light.diffuse",  app.light.diffuse);
            shader.setVec3("light.specular", app.light.specular);
            app.scene.draw(shader);

            bool cliShot = !cli.screenshot.empty() && ++frame >= cli.frames;
            bool wantShot = app.screenshotRequested || cliShot;
            std::string shotPath = cliShot ? cli.screenshot : timestampedName();
            bool shotWithGui = cliShot && cli.withGui;

            if (wantShot && !shotWithGui) {   // scene only, before the GUI is drawn
                bool ok = saveScreenshot(shotPath, app.winW, app.winH);
                app.status = ok ? "Saved " + shotPath : "Could not save " + shotPath;
                std::cout << app.status << "\n";
            }

            // ---- GUI
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            Gui::draw(app);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (wantShot && shotWithGui) {
                bool ok = saveScreenshot(shotPath, app.winW, app.winH);
                std::cout << (ok ? "Saved " : "Could not save ") << shotPath << "\n";
            }
            app.screenshotRequested = false;

            glfwSwapBuffers(window);
            if (cliShot || app.quitRequested) glfwSetWindowShouldClose(window, 1);
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        exitCode = 1;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    app.scene.clear();   // free GL objects while the context is still alive
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
