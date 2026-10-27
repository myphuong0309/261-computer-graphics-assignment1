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
//   W                - Toggle wireframe
//   F12              - Screenshot of the 3D view (Shift+F12: whole window, GUI included)
//   ESC              - Quit
//
// CLI (for screenshots / testing):
//   --element <symbol> | --molecule <formula>   --wireframe   --paused   --no-vibrate   --rotate
//   --speed <x>  --amplitude <A>  --time <seconds>  --yaw <deg>  --pitch <deg>  --distance <d>
//   --light x,y,z  --editor  --screenshot <file.png> [--with-gui] [--frames N]
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
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <set>
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
#include "Screenshot.h"

// ===============================
// Global application state: prevent var name conflicts
// ===============================

namespace App {

    // Window dimensions
    GLFWwindow* window    = nullptr; // pointer for GLFW window
    int         winW      = 1280;
    int         winH      = 800;
    const int   panelW    = 330;     // control panel width + margins: the 3D view is drawn to its right

    // Camera (handle 3D math and view/projection matrices)
    Camera camera;

    // Shaders & Meshes (shared)
    Shader* shader        = nullptr;
    Mesh*   sphereMesh    = nullptr;
    Mesh*   cylMesh       = nullptr;
    std::vector<Mesh*> ringMeshes;   // one torus per Bohr shell (constant tube radius)

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
    MoleculeSpec editMol;      // working copy of the selected molecule: the editor changes this one
    bool molEdited = false;    // editMol differs from the preset
    // when the molecule selection changes, we need to rebuild the scene
    bool molDirty  = true;
    bool openEditor = false;   // expand the editor once (CLI --editor)

    // Light source (Phone lighting)
    glm::vec3 lightPos   = glm::vec3(5.0f, 8.0f, 5.0f);
    glm::vec3 lightAmbient = glm::vec3(0.15f);  // make backsides of atoms visible -> create white light over all object equally
    glm::vec3 lightDiffuse = glm::vec3(1.0f);   // directional light from lightPos (facing towards get full light)
    glm::vec3 lightSpecular = glm::vec3(1.0f);  // shiny part on the surface

    // Rendering mode
    bool wireframe = false;

    // Screenshots
    bool shotRequested = false;
    bool shotWithGui   = false;
    std::string status;

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
// Camera helpers
// ===========================================================
// Distance at which a sphere of radius R just fits in the vertical field of view:
// the view cone of half-angle fov/2 is tangent to the sphere when sin(fov/2) = R / d.
static void fitCamera() {
    float R = (App::mode == App::ViewMode::Bohr)
            ? App::bohrAtom.outerRadius() + BohrAtom::kElectronRadius
            : MoleculeScene::kFitRadius;
    float halfFov = glm::radians(App::camera.fov * 0.5f);
    App::camera.distance = std::min(50.0f, 1.25f * R / std::sin(halfFov));   // 25 % margin
}

// Default view of Part 2: camera on +Z looking at the origin (yaw 90 deg), so that X points right,
// Y up and Z towards the viewer, and the default light (5, 8, 5) is in front of the scene.
static void resetView() {
    App::camera.reset();
    App::camera.yaw = 90.0f;
    fitCamera();
}

static int viewportX() { return std::min(App::panelW, App::winW / 2); }

// ===========================================================
// Callbacks: GLFW window events (resize, mouse, keyboard)
// ===========================================================
static void framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    App::winW = width; App::winH = height;  // update new window size
    glViewport(0, 0, width, height);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {

    // is the current user interaction with the GUI -> If yes, ignore mouse presses for camera control
    // (releases always reach the camera, so a drag that ends over the GUI does not get stuck)
    if (action == GLFW_PRESS && ImGui::GetIO().WantCaptureMouse) return;

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
    // while typing in a GUI text field (e.g. "Ar") the keys belong to the GUI, not to the shortcuts
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    // listen for specific keys for camera reset, wireframe toggle, screenshot and exit
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1); // exit
        if (key == GLFW_KEY_R)      resetView();    // reset camera
        if (key == GLFW_KEY_W)      App::wireframe = !App::wireframe;
        if (key == GLFW_KEY_F12) {
            App::shotRequested = true;
            App::shotWithGui   = (mods & GLFW_MOD_SHIFT) != 0;
        }
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

// ================================================
// Molecule helpers
// ================================================
static void selectMolecule(int idx) {
    App::molIdx    = idx;
    App::editMol   = MoleculeData::all()[idx];
    App::molEdited = false;
    App::molDirty  = true;
}

// Bond lengths |p_a - p_b| and bond angles acos(u.v / |u||v|) of the current molecule
static void drawGeometry(const MoleculeSpec& mol) {
    const auto& atoms = mol.atoms;
    int n = (int)atoms.size();
    if (mol.bonds.empty()) { ImGui::TextDisabled("  (no bonds)"); return; }

    for (const auto& b : mol.bonds) {
        if (b.a < 0 || b.b < 0 || b.a >= n || b.b >= n) continue;
        float len = glm::length(atoms[b.a].position - atoms[b.b].position);
        ImGui::Text("  %s%d-%s%d: %.3f A", atoms[b.a].symbol.c_str(), b.a, atoms[b.b].symbol.c_str(), b.b, len);
    }
    // angle i-j-k for every pair of bonds sharing the atom j
    std::vector<std::vector<int>> nb(n);
    for (const auto& b : mol.bonds)
        if (b.a >= 0 && b.b >= 0 && b.a < n && b.b < n && b.a != b.b) { nb[b.a].push_back(b.b); nb[b.b].push_back(b.a); }
    for (int j = 0; j < n; ++j)
        for (size_t p = 0; p < nb[j].size(); ++p)
            for (size_t q = p + 1; q < nb[j].size(); ++q) {
                int i = nb[j][p], k = nb[j][q];
                glm::vec3 u = atoms[i].position - atoms[j].position;
                glm::vec3 v = atoms[k].position - atoms[j].position;
                float lu = glm::length(u), lv = glm::length(v);
                if (lu < 1e-6f || lv < 1e-6f) continue;
                float c = std::max(-1.0f, std::min(1.0f, glm::dot(u, v) / (lu * lv)));
                ImGui::Text("  %s%d-%s%d-%s%d: %.1f deg", atoms[i].symbol.c_str(), i, atoms[j].symbol.c_str(), j,
                            atoms[k].symbol.c_str(), k, glm::degrees(std::acos(c)));
            }
}

// Editor of the working molecule: element and position of every atom, bonds and bond orders
static void drawMoleculeEditor() {
    static const std::vector<std::string> symbols = AtomData::symbolsByZ();
    std::vector<const char*> items;
    for (const auto& s : symbols) items.push_back(s.c_str());

    MoleculeSpec& mol = App::editMol;
    bool changed = false;

    ImGui::TextDisabled("Atoms: element, position (Angstrom)");
    int removeAtom = -1;
    for (int i = 0; i < (int)mol.atoms.size(); ++i) {
        ImGui::PushID(i);
        int cur = (int)(std::find(symbols.begin(), symbols.end(), mol.atoms[i].symbol) - symbols.begin());
        ImGui::Text("%d", i); ImGui::SameLine();
        ImGui::SetNextItemWidth(56);
        if (ImGui::Combo("##el", &cur, items.data(), (int)items.size())) {
            mol.atoms[i].symbol = symbols[cur]; changed = true;
        }
        ImGui::SameLine(); ImGui::SetNextItemWidth(150);
        if (ImGui::DragFloat3("##pos", &mol.atoms[i].position.x, 0.005f, -10.0f, 10.0f, "%.3f")) changed = true;
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) removeAtom = i;
        ImGui::PopID();
    }
    if (removeAtom >= 0) {   // drop the atom, its bonds, and renumber the remaining bonds
        mol.atoms.erase(mol.atoms.begin() + removeAtom);
        std::vector<BondSpec> kept;
        for (BondSpec b : mol.bonds) {
            if (b.a == removeAtom || b.b == removeAtom) continue;
            if (b.a > removeAtom) --b.a;
            if (b.b > removeAtom) --b.b;
            kept.push_back(b);
        }
        mol.bonds = kept;
        changed = true;
    }
    if (ImGui::Button("Add atom")) {   // new H atom 1 A beyond the farthest atom on +X
        float maxX = 0.0f;
        for (const auto& a : mol.atoms) maxX = std::max(maxX, a.position.x);
        mol.atoms.push_back({"H", glm::vec3(maxX + 1.0f, 0.0f, 0.0f)});
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto bonds")) { mol.bonds = MoleculeData::detectBonds(mol.atoms); changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) selectMolecule(App::molIdx);

    ImGui::TextDisabled("Bonds: atoms, order (1-3)");
    int removeBond = -1;
    for (int k = 0; k < (int)mol.bonds.size(); ++k) {
        BondSpec& b = mol.bonds[k];
        ImGui::PushID(1000 + k);
        ImGui::Text("%d-%d", b.a, b.b); ImGui::SameLine(60);
        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderInt("##ord", &b.order, 1, 3)) changed = true;
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) removeBond = k;
        ImGui::PopID();
    }
    if (removeBond >= 0) { mol.bonds.erase(mol.bonds.begin() + removeBond); changed = true; }

    static int newA = 0, newB = 1;
    ImGui::SetNextItemWidth(80); ImGui::InputInt("##na", &newA); ImGui::SameLine();
    ImGui::SetNextItemWidth(80); ImGui::InputInt("##nb", &newB); ImGui::SameLine();
    if (ImGui::Button("Add bond")) {
        int n = (int)mol.atoms.size();
        bool valid = newA >= 0 && newB >= 0 && newA < n && newB < n && newA != newB;
        for (const auto& b : mol.bonds)
            if ((b.a == newA && b.b == newB) || (b.a == newB && b.b == newA)) valid = false;
        if (valid) { mol.bonds.push_back({newA, newB, 1}); changed = true; }
    }

    if (changed) {
        App::molEdited = true;
        mol.stretchWavenumber = 0.0f;   // the experimental value belongs to the preset only
        App::molDirty = true;
    }
}

// ======================
// GUI from Dear ImGui
// ======================
static void drawGUI() {
    // Main control panel: fixed on the left, grows with its content up to the window height (then scrolls)
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(310, 0), ImVec2(310, (float)App::winH - 20.0f));

    // user cannot drag or resize the control panel, and it will auto-resize to fit content
    ImGui::Begin("Control Panel", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    // FPS
    float fps = App::fps > 0.0f ? App::fps : (App::fpsAccum > 0.0f ? App::fpsFrames / App::fpsAccum : 0.0f);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Separator();

    // --- View Mode ---
    ImGui::Text("View Mode");   // choose between Bohr atom model or ball-and-stick molecule
    bool isMol  = (App::mode == App::ViewMode::Molecule);
    bool isBohr = (App::mode == App::ViewMode::Bohr);

    // adjust camera distance and reset when switching between modes
    if (ImGui::RadioButton("Bohr Atom Model", isBohr) && !isBohr) {
        App::mode = App::ViewMode::Bohr;
        resetView();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Ball-and-Stick", isMol) && !isMol) {
        App::mode = App::ViewMode::Molecule;
        resetView();
    }
    ImGui::Separator();

    // ---- Bohr Atom Section ----
    if (App::mode == App::ViewMode::Bohr) {
        ImGui::TextColored(ImVec4(0.4f,0.9f,1.0f,1.0f), "Bohr Atom Model");

        // Element vec Symbols (list of all elements, sorted by atomic number)
        static const std::vector<std::string> symbols = AtomData::symbolsByZ();

        // Find current index
        int curIdx = 0;
        for (int i = 0; i < (int)symbols.size(); ++i)
            if (symbols[i] == App::bohrSymbol) { curIdx = i; break; }

        static char elementBuf[8] = "C";
        if (App::bohrSymbol != elementBuf && !ImGui::IsAnyItemActive())   // e.g. set from the command line
            strncpy(elementBuf, App::bohrSymbol.c_str(), sizeof(elementBuf) - 1);
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
        ImGui::SameLine(); ImGui::TextDisabled("(Enter)");

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
            ImGui::Text("Shells (electrons / capacity 2n^2):");
            static const char* shellNames = "KLMNOPQ";
            for (int i = 0; i < (int)info.shells.size(); ++i)
                ImGui::Text("  %c (n=%d): %2d / %2d   r = %.1f", shellNames[i], i + 1,
                            info.shells[i], 2 * (i + 1) * (i + 1), BohrAtom::shellRadius(i));
        } catch (...) {}

        ImGui::Separator();
        // Animation
        bool anim = App::bohrAtom.animating;
        if (ImGui::Checkbox("Animate Electrons", &anim))
            App::bohrAtom.animating = anim;
        // modify animation speed (applied to every orbit, the electrons keep their phase)
        float spd = App::bohrAtom.animSpeed;
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("Speed", &spd, 0.1f, 5.0f))
            App::bohrAtom.setAnimSpeed(spd);
    }

    // ---- Molecule Section ----
    if (App::mode == App::ViewMode::Molecule) {
        ImGui::TextColored(ImVec4(1.0f,0.8f,0.3f,1.0f), "Ball-and-Stick Molecules");

        const auto& mols = MoleculeData::all();
        std::vector<const char*> molNames;
        for (const auto& m : mols) molNames.push_back(m.name.c_str());

        ImGui::Text("Select Molecule:");
        ImGui::SetNextItemWidth(200);
        int sel = App::molIdx;
        if (ImGui::ListBox("##mols", &sel, molNames.data(), (int)molNames.size(), 6))
            selectMolecule(sel);

        const MoleculeSpec& spec = App::editMol;
        if (App::molEdited)
            ImGui::Text("Formula: %s (edited)", MoleculeData::hillFormula(spec.atoms).c_str());
        else
            ImGui::Text("Formula: %s", spec.formula.c_str());
        ImGui::Text("Atoms: %d  Bonds: %d", (int)spec.atoms.size(), (int)spec.bonds.size());

        ImGui::Separator();
        // Bond order legend
        ImGui::Text("Bond types:");
        for (const auto& b : spec.bonds) {
            if (b.a < 0 || b.b < 0 || b.a >= (int)spec.atoms.size() || b.b >= (int)spec.atoms.size()) continue;
            const char* ord[] = {"?","Single","Double","Triple"};
            ImGui::Text("  %s-%s: %s", spec.atoms[b.a].symbol.c_str(),
                        spec.atoms[b.b].symbol.c_str(), ord[std::max(0, std::min(b.order,3))]);
        }

        // Geometry measured from the atom positions
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Geometry (lengths, angles)"))
            drawGeometry(spec);

        // Atom legend (CPK colors), one line per element
        ImGui::Text("Atom colors (CPK):");
        std::set<std::string> shown;
        for (const auto& a : spec.atoms) {
            if (!shown.insert(a.symbol).second) continue;
            try {
                const AtomInfo& info = AtomData::get(a.symbol);
                ImVec4 col(info.color.r, info.color.g, info.color.b, 1.0f);
                ImGui::ColorButton(("##col"+a.symbol).c_str(), col,
                                   ImGuiColorEditFlags_NoTooltip, ImVec2(14,14));
                ImGui::SameLine();
                ImGui::Text("%s (%s)", a.symbol.c_str(), info.name.c_str());
            } catch (...) {}
        }

        // Configuration of atom types and positions
        if (App::openEditor) { ImGui::SetNextItemOpen(true, ImGuiCond_Always); App::openEditor = false; }
        if (ImGui::CollapsingHeader("Edit atoms & bonds"))
            drawMoleculeEditor();

        ImGui::Separator();
        // Animation
        bool vib = App::molScene.vibrating;
        if (ImGui::Checkbox("Molecular Vibration", &vib))
            App::molScene.setVibrating(vib);
        if (spec.stretchWavenumber > 0.0f)
            ImGui::Text("  sym. stretch %.0f cm^-1 -> %.2f Hz", spec.stretchWavenumber, App::molScene.vibFreq);
        else
            ImGui::Text("  stretch shown at %.2f Hz", App::molScene.vibFreq);
        float amp = App::molScene.vibAmp, vspd = App::molScene.vibSpeed;
        ImGui::SetNextItemWidth(150);
        bool vChanged = ImGui::SliderFloat("Amplitude", &amp, 0.0f, 0.25f, "%.2f");
        ImGui::SetNextItemWidth(150);
        vChanged |= ImGui::SliderFloat("Vib. speed", &vspd, 0.1f, 3.0f, "%.1fx");
        if (vChanged) App::molScene.setVibration(amp, vspd);
        bool rot = App::molScene.rotating;
        if (ImGui::Checkbox("Molecular Rotation", &rot))
            App::molScene.setRotating(rot);
    }

    ImGui::Separator();
    // Rendering options -> wireframe toggle
    ImGui::TextColored(ImVec4(0.7f,1.0f,0.7f,1.0f), "Rendering");
    ImGui::Checkbox("Wireframe (W)", &App::wireframe);

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
        ImGui::SliderFloat("Yaw",      &App::camera.yaw,   -180.0f, 180.0f);
        ImGui::SliderFloat("Pitch",    &App::camera.pitch,  -89.0f,  89.0f);
        ImGui::SliderFloat("FOV",      &App::camera.fov,      15.0f, 120.0f);
        ImGui::SliderFloat("Distance", &App::camera.distance,  1.0f,  50.0f);
        if (ImGui::Button("Reset Camera")) resetView();
    }

    ImGui::Separator();
    // Screenshot: the 3D view only, or the whole window with this panel
    static bool shotGui = false;
    if (ImGui::Button("Screenshot (F12)")) { App::shotRequested = true; App::shotWithGui = shotGui; }
    ImGui::SameLine();
    ImGui::Checkbox("with GUI", &shotGui);
    if (!App::status.empty()) ImGui::TextWrapped("%s", App::status.c_str());
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
    App::bohrAtom.build(App::bohrSymbol, App::sphereMesh, App::ringMeshes);
    App::bohrDirty = false;
}

static void rebuildMolecule() {
    App::molScene.build(App::editMol, App::sphereMesh, App::cylMesh);
    App::molDirty = false;
}

// ==========================================
// Command line (scripted screenshots for the report)
// ==========================================
struct CliOptions {
    std::string element, molecule, screenshot;
    bool withGui = false, wireframe = false, paused = false, noVibrate = false, rotate = false, editor = false;
    int   frames = 8;
    float time = 0.0f, speed = 0.0f, amplitude = -1.0f, distance = 0.0f;
    bool  hasYaw = false, hasPitch = false, hasLight = false;
    float yaw = 0.0f, pitch = 0.0f;
    glm::vec3 light{0.0f};
};

static std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static CliOptions parseArgs(int argc, char** argv) {
    CliOptions o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if      (a == "--element")    o.element = next();
        else if (a == "--molecule")   o.molecule = next();
        else if (a == "--screenshot") o.screenshot = next();
        else if (a == "--with-gui")   o.withGui = true;
        else if (a == "--frames")     o.frames = std::max(1, std::atoi(next().c_str()));
        else if (a == "--wireframe")  o.wireframe = true;
        else if (a == "--paused")     o.paused = true;
        else if (a == "--no-vibrate") o.noVibrate = true;
        else if (a == "--rotate")     o.rotate = true;
        else if (a == "--editor")     o.editor = true;
        else if (a == "--time")       o.time = (float)std::atof(next().c_str());
        else if (a == "--speed")      o.speed = (float)std::atof(next().c_str());
        else if (a == "--amplitude")  o.amplitude = (float)std::atof(next().c_str());
        else if (a == "--distance")   o.distance = (float)std::atof(next().c_str());
        else if (a == "--yaw")        { o.hasYaw = true;   o.yaw = (float)std::atof(next().c_str()); }
        else if (a == "--pitch")      { o.hasPitch = true; o.pitch = (float)std::atof(next().c_str()); }
        else if (a == "--light") {   // --light x,y,z
            o.hasLight = std::sscanf(next().c_str(), "%f,%f,%f", &o.light.x, &o.light.y, &o.light.z) == 3;
        }
        else std::cerr << "Unknown option: " << a << "\n";
    }
    return o;
}

// Applied once the meshes exist (before the first frame)
static void setupFromCli(const CliOptions& o) {
    if (!o.element.empty()) {
        std::string sym = lower(o.element);
        if (!sym.empty()) sym[0] = (char)std::toupper((unsigned char)sym[0]);
        try { AtomData::get(sym); App::bohrSymbol = sym; App::bohrDirty = true; }
        catch (...) { std::cerr << "Unknown element: " << o.element << "\n"; }
    }
    if (!o.molecule.empty()) {
        const auto& mols = MoleculeData::all();
        int found = -1;
        for (int i = 0; i < (int)mols.size(); ++i)
            if (lower(mols[i].formula) == lower(o.molecule) || lower(mols[i].name) == lower(o.molecule)) found = i;
        if (found >= 0) { selectMolecule(found); App::mode = App::ViewMode::Molecule; }
        else std::cerr << "Unknown molecule: " << o.molecule << "\n";
    }
    App::wireframe  = o.wireframe;
    App::openEditor = o.editor;
    if (o.paused) App::bohrAtom.animating = false;
    if (o.paused || o.noVibrate) App::molScene.vibrating = false;
    if (o.rotate) App::molScene.rotating = true;
    if (o.speed > 0.0f) App::bohrAtom.animSpeed = o.speed;
    if (o.amplitude >= 0.0f) App::molScene.vibAmp = o.amplitude;
    if (o.hasLight) App::lightPos = o.light;

    if (App::bohrDirty) rebuildBohr();
    if (App::molDirty)  rebuildMolecule();

    // advance the animations to time t (all motions are linear in dt, so one step is exact)
    if (o.time > 0.0f) {
        if (App::mode == App::ViewMode::Bohr) App::bohrAtom.update(o.time);
        else                                  App::molScene.update(o.time);
    }

    resetView();
    if (o.hasYaw)   App::camera.yaw   = o.yaw;
    if (o.hasPitch) App::camera.pitch = std::max(-89.0f, std::min(89.0f, o.pitch));
    if (o.distance > 0.0f) App::camera.distance = o.distance;
}

static std::string timestampedName() {
    char buf[64];
    std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "screenshot_%Y%m%d_%H%M%S.png", std::localtime(&t));
    return buf;
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char** argv) {
    CliOptions cli = parseArgs(argc, argv);

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
    io.IniFilename = nullptr;   // the panel layout is fixed, no imgui.ini needed
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
    std::vector<Mesh> ringMeshes;
    for (int k = 0; k < BohrAtom::kMaxShells; ++k)
        ringMeshes.push_back(Mesh::makeTorus(BohrAtom::shellRadius(k), BohrAtom::kRingTubeRadius, 128, 8));

    App::sphereMesh = &sphereMesh;
    App::cylMesh    = &cylMesh;
    for (auto& m : ringMeshes) App::ringMeshes.push_back(&m);

    // -------------------------------------------------------------------------
    // Initial scene build
    // -------------------------------------------------------------------------
    selectMolecule(0);
    setupFromCli(cli);   // also builds both scenes and fits the camera
    glfwGetFramebufferSize(App::window, &App::winW, &App::winH);
    App::lastTime = (float)glfwGetTime();
    int frame = 0;

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
        // scripted screenshots show exactly the state at --time (no extra motion)
        float animDt = cli.screenshot.empty() ? App::dt : 0.0f;

        // Rebuild if dirty
        if (App::bohrDirty)  { rebuildBohr(); fitCamera(); }
        if (App::molDirty)   rebuildMolecule();

        // Update animation
        if (App::mode == App::ViewMode::Bohr)
            App::bohrAtom.update(animDt);
        else
            App::molScene.update(animDt);

        // ---- Render ----
        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // the control panel covers the left side of the window: draw the 3D view to its right
        int vx = viewportX();
        int vw = std::max(1, App::winW - vx), vh = std::max(1, App::winH);
        glViewport(vx, 0, vw, vh);
        float aspect = (float)vw / (float)vh;
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
        if (App::mode == App::ViewMode::Bohr) {
            if (App::bohrAtom.root) App::bohrAtom.root->setWireframe(App::wireframe);
            App::bohrAtom.draw(shader);
        } else {
            if (App::molScene.root) App::molScene.root->setWireframe(App::wireframe);
            App::molScene.draw(shader);
        }

        // Screenshot of the 3D view (before the GUI is drawn)
        bool cliShot  = !cli.screenshot.empty() && ++frame >= cli.frames;
        bool wantShot = App::shotRequested || cliShot;
        bool withGui  = cliShot ? cli.withGui : App::shotWithGui;
        std::string shotPath = cliShot ? cli.screenshot : timestampedName();
        if (wantShot && !withGui) {
            bool ok = saveScreenshot(shotPath, vx, 0, vw, vh);
            App::status = ok ? "Saved " + shotPath : "Could not save " + shotPath;
            std::cout << App::status << "\n";
        }

        // ---- ImGui ----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawGUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (wantShot && withGui) {   // whole window, GUI included
            bool ok = saveScreenshot(shotPath, 0, 0, App::winW, App::winH);
            App::status = ok ? "Saved " + shotPath : "Could not save " + shotPath;
            std::cout << App::status << "\n";
        }
        App::shotRequested = false;

        glfwSwapBuffers(App::window);
        if (cliShot) glfwSetWindowShouldClose(App::window, 1);
    }

    // -------------------------------------------------------------------------
    // Cleanup
    // -------------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    sphereMesh.destroy();
    cylMesh.destroy();
    for (auto& m : ringMeshes) m.destroy();
    glDeleteProgram(shader.ID);

    glfwDestroyWindow(App::window);
    glfwTerminate();
    return 0;
}
