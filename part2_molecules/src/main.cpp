// =============================================================================
// main.cpp - Entry point & application loop
// Part 2: Visualization of Atoms and Molecules
// HCMUT Computer Graphics Assignment 1 – Semester I, AY 2026-2027
//
// Controls:
//   Left Mouse Drag  - Rotate view
//   Right Mouse Drag - Pan
//   Scroll Wheel     - Zoom
//   R                - Reset camera
//   ESC              - Quit
// =============================================================================

// ----- ImGui (dung cho GUI) -----

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_glfw.h"
#include "../vendor/imgui/imgui_impl_opengl3.h"

// ----- OpenGL, GLFW -----
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ----- GLM (calculation lib) -----
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ----------
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

// ----- Additional import -----
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "SceneNode.h"
#include "AtomData.h"
#include "BohrModel.h"
#include "MoleculeScene.h"

// ===============================
// Global application state: prevent var name conflicts
// ===============================

namespace App {

    // Window dimensions
    GLFWwindow* window    = nullptr; // pointer for GLFW window
    int         winW      = 1280;
    int         winH      = 800;

    // Camera (handle 3D math and view/projection matrices)
    Camera camera;

    // Shaders & Meshes (shared)
    Shader* shader        = nullptr;
    Mesh*   sphereMesh    = nullptr;
    Mesh*   cylMesh       = nullptr;
    Mesh*   torusMesh     = nullptr;

    // View mode -> track whether we are in Bohr model or Molecule scene
    enum class ViewMode { Bohr, Molecule } 
    mode = ViewMode::Bohr;

    // Bohr model (option 1)
    BohrAtom bohrAtom;
    std::string bohrSymbol = "C";
    // when the element changes, we need to rebuild the scene
    bool bohrDirty = true; 

    // Molecule scene (option 2)
    MoleculeScene molScene;
    int  molIdx    = 0;
    // when the molecule selection changes, we need to rebuild the scene
    bool molDirty  = true;

    // Light source (Phone lighting)
    glm::vec3 lightPos   = glm::vec3(5.0f, 8.0f, 5.0f);
    glm::vec3 lightAmbient = glm::vec3(0.15f);  // make backsides of atoms visible -> create white light over all object equally
    glm::vec3 lightDiffuse = glm::vec3(1.0f);   // directional light from lightPos (facing towards get full light)
    glm::vec3 lightSpecular = glm::vec3(1.0f);  // shiny part on the surface

    // Rendering mode
    bool wireframe = false;

    // Timing
    float lastTime = 0.0f;
    float dt       = 0.0f;  // delta time between frames (ensure animation speed is consistent)

    // FPS
    float fpsAccum = 0.0f;
    int   fpsFrames = 0;
    float fps = 0.0f;

    // GUI state
    bool showHelp = false;

} // namespace App

// ===========================================================
// Callbacks: GLFW window events (resize, mouse, keyboard)
// ===========================================================
static void framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    App::winW = width; App::winH = height;  // update new window size
    glViewport(0, 0, width, height);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {

    // is the current user interaction with the GUI -> If yes, ignore mouse events for camera control
    if (ImGui::GetIO().WantCaptureMouse) return;

    // user clicked on the window -> get cursor position and pass to camera
    double x, y; glfwGetCursorPos(w, &x, &y);
    App::camera.mouseButton(button, action, x, y);
}

static void cursorPosCallback(GLFWwindow* w, double x, double y) {
    // if the user is interacting with the GUI -> ignore mouse events for camera control
    if (ImGui::GetIO().WantCaptureMouse) return;

    // if user dragging or panning -> pass mouse movement to camera
    App::camera.mouseMove(x, y);
}

static void scrollCallback(GLFWwindow* w, double xo, double yo) {
    // scroll by mouse wheel -> zoom in/out
    if (ImGui::GetIO().WantCaptureMouse) return;
    App::camera.scroll(yo);
}

static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
    // listen for specific keys for camera reset, wireframe toggle, and exit 
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1); // exit
        if (key == GLFW_KEY_R)      App::camera.reset();    // reset camera
        if (key == GLFW_KEY_W)      App::wireframe = !App::wireframe;
    }
}

// ================================================
// Shader path -> load shader from relative path
// ================================================
static std::string shaderPath(const std::string& name) {
    // Try relative to cwd first, then relative to executable
    namespace fs = std::filesystem;
    fs::path rel = fs::path("shaders") / name;
    if (fs::exists(rel)) return rel.string();
    // Fallback: relative to source
    return std::string("../shaders/") + name;
}

// ======================
// GUI from Dear ImGui
// ======================
static void drawGUI() {
    // Main control panel
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(310, 0), ImGuiCond_Always);

    // user cannot drag or resize the control panel, and it will auto-resize to fit content
    ImGui::Begin("Control Panel", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    // FPS
    ImGui::Text("FPS: %.1f", App::fps);
    ImGui::Separator();

    // --- View Mode ---
    ImGui::Text("View Mode");   // choose between Bohr atom model or ball-and-stick molecule
    bool isMol  = (App::mode == App::ViewMode::Molecule);
    bool isBohr = (App::mode == App::ViewMode::Bohr);

    // adjust camera distance and reset when switching between modes
    if (ImGui::RadioButton("Bohr Atom Model", isBohr)) {
        App::mode = App::ViewMode::Bohr;
        App::camera.distance = 8.0f;
        App::camera.reset();
    }
    ImGui::SameLine();
    // if (ImGui::RadioButton("Ball-and-Stick Molecule", isMol)) {
    //     App::mode = App::ViewMode::Molecule;
    //     App::camera.distance = 6.0f;
    //     App::camera.reset();
    // }
    ImGui::Separator();

    // ---- Bohr Atom Section ----
    if (App::mode == App::ViewMode::Bohr) {
        ImGui::TextColored(ImVec4(0.4f,0.9f,1.0f,1.0f), "Bohr Atom Model");

        // Element vec Symbols (list of all elements)
        const auto& allAtoms = AtomData::all();
        static std::vector<std::string> symbols;
        if (symbols.empty()) {
            for (const auto& kv : allAtoms) symbols.push_back(kv.first);
            // sort by atomic number
            std::sort(symbols.begin(), symbols.end(), [&](const std::string& a, const std::string& b){
                return allAtoms.at(a).atomicNumber < allAtoms.at(b).atomicNumber;
            });
        }

        // Find current index
        int curIdx = 0;
        for (int i = 0; i < (int)symbols.size(); ++i)
            if (symbols[i] == App::bohrSymbol) { curIdx = i; break; }

        static char elementBuf[8] = "C";
        ImGui::Text("Element:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(60);

        // allow user to type in an element symbol (VD: "H", "He", etc.) and 
        // press Enter to update the Bohr model
        if (ImGui::InputText("##elem", elementBuf, sizeof(elementBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string sym(elementBuf); 
            // validate symbol exists in AtomData -> ignore it
            try { AtomData::get(sym); App::bohrSymbol = sym; App::bohrDirty = true; }
            catch (...) {}
        }
        
        // Dropdown list of all elements -> user can select from the list
        ImGui::Text("Select:");
        ImGui::SetNextItemWidth(180);
        std::vector<const char*> symCStrs;
        for (auto& s : symbols) symCStrs.push_back(s.c_str());
        if (ImGui::ListBox("##atoms", &curIdx, symCStrs.data(), (int)symCStrs.size(), 6)) {
            App::bohrSymbol = symbols[curIdx];
            strncpy(elementBuf, App::bohrSymbol.c_str(), sizeof(elementBuf)-1);
            App::bohrDirty = true;
        }

        // Show atom info (current displayed element)
        try {
            const AtomInfo& info = AtomData::get(App::bohrSymbol);
            ImGui::Text("Name: %s  Z=%d", info.name.c_str(), info.atomicNumber);
            ImGui::Text("Shells:");
            for (int i = 0; i < (int)info.shells.size(); ++i)
                ImGui::Text("  Shell %d: %d electron(s)", i+1, info.shells[i]);
        } catch (...) {}

        ImGui::Separator();
        // Animation
        bool anim = App::bohrAtom.animating;
        if (ImGui::Checkbox("Animate Electrons", &anim))
            App::bohrAtom.animating = anim;
        // modify animation speed -> rebuild the Bohr model to apply the new speed
        float spd = App::bohrAtom.animSpeed;
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("Speed", &spd, 0.1f, 5.0f)) {
            App::bohrAtom.animSpeed = spd;
            // rebuild to apply speed 
            App::bohrDirty = true;
        }
    }

    // ---- Molecule Section ----
    if (App::mode == App::ViewMode::Molecule) {
        ImGui::TextColored(ImVec4(1.0f,0.8f,0.3f,1.0f), "Ball-and-Stick Molecules");

        const auto& mols = MoleculeData::all();
        std::vector<const char*> molNames;
        for (const auto& m : mols) molNames.push_back(m.name.c_str());

        ImGui::Text("Select Molecule:");
        ImGui::SetNextItemWidth(200);
        if (ImGui::ListBox("##mols", &App::molIdx, molNames.data(), (int)molNames.size(), 6))
            App::molDirty = true;

        const auto& spec = mols[App::molIdx];
        ImGui::Text("Formula: %s", spec.formula.c_str());
        ImGui::Text("Atoms: %d  Bonds: %d", (int)spec.atoms.size(), (int)spec.bonds.size());

        ImGui::Separator();
        // Bond order legend
        ImGui::Text("Bond types:");
        for (const auto& b : spec.bonds) {
            const char* ord[] = {"?","Single","Double","Triple"};
            ImGui::Text("  %s-%s: %s", spec.atoms[b.a].symbol.c_str(),
                        spec.atoms[b.b].symbol.c_str(), ord[std::min(b.order,3)]);
        }

        ImGui::Separator();
        // Atom legend (CPK colors)
        ImGui::Text("Atom colors (CPK):");
        for (const auto& a : spec.atoms) {
            try {
                const AtomInfo& info = AtomData::get(a.symbol);
                ImVec4 col(info.color.r, info.color.g, info.color.b, 1.0f);
                ImGui::ColorButton(("##col"+a.symbol).c_str(), col,
                                   ImGuiColorEditFlags_NoTooltip, ImVec2(14,14));
                ImGui::SameLine();
                ImGui::Text("%s (%s)", a.symbol.c_str(), info.name.c_str());
            } catch (...) {}
        }

        ImGui::Separator();
        // Animation
        bool vib = App::molScene.vibrating;
        if (ImGui::Checkbox("Molecular Vibration", &vib))
            App::molScene.setVibrating(vib);
        bool rot = App::molScene.rotating;
        if (ImGui::Checkbox("Molecular Rotation", &rot))
            App::molScene.setRotating(rot);
    }

    ImGui::Separator();
    // Rendering options -> wireframe toggle (develop trong tuong lai)
    ImGui::TextColored(ImVec4(0.7f,1.0f,0.7f,1.0f), "Rendering");
    if (ImGui::Checkbox("Wireframe (W)", &App::wireframe)) {}

    // Light controls
    if (ImGui::CollapsingHeader("Lighting")) {
        // moving the source of light 
        ImGui::SliderFloat3("Light Pos", &App::lightPos.x, -15.0f, 15.0f);

        // adjust light color components (ambient, diffuse, specular) for Phong lighting
        ImGui::ColorEdit3("Ambient",  &App::lightAmbient.x);
        ImGui::ColorEdit3("Diffuse",  &App::lightDiffuse.x);
        ImGui::ColorEdit3("Specular", &App::lightSpecular.x);
    }

    // Camera controls
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat("FOV",      &App::camera.fov,      15.0f, 120.0f);
        ImGui::SliderFloat("Distance", &App::camera.distance,  1.0f,  50.0f);
        if (ImGui::Button("Reset Camera")) App::camera.reset();
    }

    ImGui::Separator();
    // if (ImGui::Button("Help")) App::showHelp = !App::showHelp;

    ImGui::End();

    // // Help window
    // if (App::showHelp) {
    //     ImGui::SetNextWindowSize(ImVec2(380, 250), ImGuiCond_Always);
    //     ImGui::Begin("Help", &App::showHelp);
    //     ImGui::Text("Controls:");
    //     ImGui::Separator();
    //     ImGui::Text("Left Mouse Drag  - Rotate view");
    //     ImGui::Text("Right Mouse Drag - Pan view");
    //     ImGui::Text("Scroll Wheel     - Zoom in/out");
    //     ImGui::Text("R                - Reset camera");
    //     ImGui::Text("W                - Toggle wireframe");
    //     ImGui::Text("ESC              - Quit");
    //     ImGui::Separator();
    //     ImGui::Text("Bohr Model:");
    //     ImGui::Text("  Electrons revolve around nucleus");
    //     ImGui::Text("  in quantized shells (K, L, M...)");
    //     ImGui::Separator();
    //     ImGui::Text("Ball-and-Stick:");
    //     ImGui::Text("  Atoms = colored spheres (CPK)");
    //     ImGui::Text("  Bonds = cylinders (1/2/3 strands)");
    //     ImGui::Text("  Toggle vibration or rotation");
    //     ImGui::End();
    // }
}

// ==========================================
// Rebuild scenes when selection changes
// ==========================================
// calculate number of election, orbit radius, and linking parent/child nodes-> rebuild the Bohr model scene
static void rebuildBohr() {
    App::bohrAtom.build(App::bohrSymbol, App::sphereMesh, App::torusMesh);
    App::bohrDirty = false;
}

static void rebuildMolecule() {
    const auto& mols = MoleculeData::all();
    App::molScene.build(mols[App::molIdx], App::sphereMesh, App::cylMesh);
    App::molDirty = false;
}

// =============================================================================
// Main
// =============================================================================
int main() {
    // -------------------------------------------------------------------------
    // GLFW init
    // -------------------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n"; return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    App::window = glfwCreateWindow(App::winW, App::winH,
        "Part 2: Atoms & Molecules Visualizer — HCMUT CG 2026-2027", nullptr, nullptr);
    if (!App::window) {
        std::cerr << "Window creation failed\n"; glfwTerminate(); return 1;
    }
    glfwMakeContextCurrent(App::window);
    glfwSwapInterval(1); // VSync

    // Callbacks
    glfwSetFramebufferSizeCallback(App::window, framebufferSizeCallback);
    glfwSetMouseButtonCallback    (App::window, mouseButtonCallback);
    glfwSetCursorPosCallback      (App::window, cursorPosCallback);
    glfwSetScrollCallback         (App::window, scrollCallback);
    glfwSetKeyCallback            (App::window, keyCallback);

    // -------------------------------------------------------------------------
    // GLEW init
    // -------------------------------------------------------------------------
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n"; return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // -------------------------------------------------------------------------
    // ImGui init
    // -------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.1f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(App::window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // -------------------------------------------------------------------------
    // Load shaders
    // -------------------------------------------------------------------------
    Shader shader(shaderPath("phong.vert").c_str(),
                  shaderPath("phong.frag").c_str());
    App::shader = &shader;

    // -------------------------------------------------------------------------
    // Build shared meshes
    // -------------------------------------------------------------------------
    Mesh sphereMesh = Mesh::makeSphere(36, 18);
    Mesh cylMesh    = Mesh::makeCylinder(32, 0.5f, 1.0f);
    Mesh torusMesh  = Mesh::makeTorus(1.0f, 0.025f, 64, 8);

    App::sphereMesh = &sphereMesh;
    App::cylMesh    = &cylMesh;
    App::torusMesh  = &torusMesh;

    // -------------------------------------------------------------------------
    // Initial scene build
    // -------------------------------------------------------------------------
    rebuildBohr();
    rebuildMolecule();

    // -------------------------------------------------------------------------
    // Main render loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(App::window)) {
        glfwPollEvents();

        // Timing
        float curTime   = (float)glfwGetTime();
        App::dt         = curTime - App::lastTime;
        App::lastTime   = curTime;
        App::fpsAccum  += App::dt;
        App::fpsFrames++;
        if (App::fpsAccum >= 0.5f) {
            App::fps     = App::fpsFrames / App::fpsAccum;
            App::fpsAccum  = 0.0f;
            App::fpsFrames = 0;
        }

        // Rebuild if dirty
        if (App::bohrDirty)  rebuildBohr();
        if (App::molDirty)   rebuildMolecule();

        // Update animation
        if (App::mode == App::ViewMode::Bohr)
            App::bohrAtom.update(App::dt);
        else
            App::molScene.update(App::dt);

        // ---- Render ----
        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)App::winW / (float)App::winH;
        glm::mat4 view = App::camera.view();
        glm::mat4 proj = App::camera.projection(aspect);
        glm::vec3 camPos = App::camera.position();

        shader.use();
        shader.setMat4("view",       view);
        shader.setMat4("projection", proj);
        shader.setVec3("viewPos",    camPos);

        // Light uniforms
        shader.setVec3("light.position", App::lightPos);
        shader.setVec3("light.ambient",  App::lightAmbient);
        shader.setVec3("light.diffuse",  App::lightDiffuse);
        shader.setVec3("light.specular", App::lightSpecular);

        // Draw scene
        if (App::mode == App::ViewMode::Bohr)
            App::bohrAtom.draw(shader);
        else
            App::molScene.draw(shader);

        // ---- ImGui ----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawGUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(App::window);
    }

    // -------------------------------------------------------------------------
    // Cleanup
    // -------------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    sphereMesh.destroy();
    cylMesh.destroy();
    torusMesh.destroy();
    glDeleteProgram(shader.ID);

    glfwDestroyWindow(App::window);
    glfwTerminate();
    return 0;
}
