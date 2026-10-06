// =============================================================================
// Gui.cpp
// =============================================================================
#include "Gui.h"
#include <cstdio>
#include <cstring>
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include "Paths.h"

namespace {

// ---------------------------------------------------------------------------
// Text buffers for InputText widgets. They are re-filled whenever another object is selected.
// ---------------------------------------------------------------------------
struct EditBuffers {
    const SceneObject* owner = nullptr;
    char name[128]    = "";
    char expr[256]    = "";
    char texture[512] = "";
    char model[512]   = "";
};

void copyTo(char* dst, size_t cap, const std::string& s) {
    std::snprintf(dst, cap, "%s", s.c_str());
}

void syncBuffers(EditBuffers& b, const SceneObject& o) {
    b.owner = &o;
    copyTo(b.name, sizeof(b.name), o.st.name);
    copyTo(b.expr, sizeof(b.expr), o.st.params.surface.expression);
    copyTo(b.texture, sizeof(b.texture), o.st.texturePath);
    copyTo(b.model, sizeof(b.model), o.st.params.modelPath);
}

struct SurfacePreset { const char* label; const char* expr; float range; };
const SurfacePreset SURFACE_PRESETS[] = {
    {"Wave: sin(x)*cos(y)",         "sin(x) * cos(y)",                     3.0f},
    {"Saddle: x^2 - y^2",           "x^2 - y^2",                           2.0f},
    {"Paraboloid: x^2 + y^2",       "x^2 + y^2",                           2.0f},
    {"Ripple: sin(sqrt(x^2+y^2))",  "sin(sqrt(x^2 + y^2))",                6.0f},
    {"Gaussian bump",               "2*exp(-(x^2 + y^2))",                 2.5f},
    {"Mexican hat: sin(r)/r",       "sin(sqrt(x^2+y^2)+0.001)/(sqrt(x^2+y^2)+0.001)*3", 10.0f},
    {"Monkey saddle: x^3 - 3xy^2",  "x^3 - 3*x*y^2",                       1.5f},
};

// ---------------------------------------------------------------------------
// Menu bar
// ---------------------------------------------------------------------------
void addShapeMenuItem(AppState& app, ShapeKind kind) {
    if (ImGui::MenuItem(shapeName(kind))) {
        std::string err;
        if (!app.scene.addShape(kind, err)) app.status = "Error: " + err;
        else app.status = std::string("Added ") + shapeName(kind);
    }
}

void drawMenuBar(AppState& app) {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Import model (.obj / .ply)...")) app.openImportPopup = true;
        if (ImGui::MenuItem("Save screenshot", "F12")) app.screenshotRequested = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Esc")) app.quitRequested = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add Shape")) {
        if (ImGui::BeginMenu("2D shapes")) {
            for (int k = (int)ShapeKind::Triangle; k <= (int)ShapeKind::Arrow; ++k) addShapeMenuItem(app, (ShapeKind)k);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("3D shapes")) {
            for (int k = (int)ShapeKind::Cube; k <= (int)ShapeKind::Prism; ++k) addShapeMenuItem(app, (ShapeKind)k);
            ImGui::EndMenu();
        }
        addShapeMenuItem(app, ShapeKind::Surface);
        if (ImGui::MenuItem("Import model (.obj / .ply)...")) app.openImportPopup = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Duplicate selected", nullptr, false, app.scene.selectedObject() != nullptr)) app.scene.duplicateSelected();
        if (ImGui::MenuItem("Delete selected", "Del", false, app.scene.selectedObject() != nullptr)) app.scene.removeSelected();
        if (ImGui::MenuItem("Clear all")) app.scene.clear();
        ImGui::Separator();
        if (ImGui::MenuItem("Load demo scene (all shapes)")) { app.scene.addDemo(); app.camera.distance = 15.0f; app.camera.target = glm::vec3(0, 1.2f, 0); }
        ImGui::Separator();
        if (ImGui::BeginMenu("Set all objects to")) {
            for (int m = 0; m <= (int)RenderMode::Wireframe; ++m)
                if (ImGui::MenuItem(renderModeName((RenderMode)m)))
                    for (auto& o : app.scene.objects) o->st.mode = (RenderMode)m;
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Reset camera", "R")) app.resetCamera();
        if (ImGui::MenuItem("Front view (2D)")) app.frontView();
        if (ImGui::MenuItem("Top view")) app.topView();
        ImGui::Separator();
        ImGui::MenuItem("Ground grid", nullptr, &app.scene.showGrid);
        ImGui::MenuItem("Axes", nullptr, &app.scene.showAxes);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("Controls", "H", &app.showHelp);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void drawImportPopup(AppState& app) {
    if (app.openImportPopup) { ImGui::OpenPopup("Import model"); app.openImportPopup = false; }
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Import model", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    static char path[512] = "assets/models/torusknot.obj";
    static std::string error;
    static std::vector<std::string> files;
    if (files.empty()) files = Paths::listFiles("assets/models", {".obj", ".ply"});

    ImGui::Text("Models found in assets/models:");
    if (ImGui::BeginListBox("##models", ImVec2(-1, 110))) {
        for (const auto& f : files)
            if (ImGui::Selectable(f.c_str(), std::strcmp(f.c_str(), path) == 0)) copyTo(path, sizeof(path), f);
        ImGui::EndListBox();
    }
    ImGui::Text("or type a path to any .obj / .ply file:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##path", path, sizeof(path));
    if (!error.empty()) ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", error.c_str());

    if (ImGui::Button("Load", ImVec2(120, 0))) {
        if (app.scene.addModel(path, error)) { error.clear(); app.status = "Imported model"; ImGui::CloseCurrentPopup(); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Rescan folder")) files = Paths::listFiles("assets/models", {".obj", ".ply"});
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) { error.clear(); ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

void drawHelpWindow(AppState& app) {
    if (!app.showHelp) return;
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
    ImGui::Begin("Controls", &app.showHelp);
    ImGui::Text("Mouse");
    ImGui::BulletText("Left drag: orbit the camera");
    ImGui::BulletText("Right drag: pan");
    ImGui::BulletText("Wheel: zoom");
    ImGui::BulletText("Ctrl + left drag: rotate the selected object");
    ImGui::Text("Keyboard");
    ImGui::BulletText("Arrows: orbit    Shift+Arrows: pan");
    ImGui::BulletText("+ / - (or PgUp / PgDn): zoom");
    ImGui::BulletText("R: reset camera    W: toggle wireframe on all");
    ImGui::BulletText("Del: delete selected    F12: screenshot");
    ImGui::BulletText("H: this window    Esc: quit");
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Inspector sections
// ---------------------------------------------------------------------------
bool shapeParameters(SceneObject& o, EditBuffers& buf) {
    ShapeParams& p = o.st.params;
    bool changed = false;
    switch (o.st.kind) {
        case ShapeKind::Rectangle:
            changed |= ImGui::SliderFloat("Width", &p.rectWidth, 0.1f, 2.0f);
            changed |= ImGui::SliderFloat("Height", &p.rectHeight, 0.1f, 2.0f);
            break;
        case ShapeKind::Trapezoid:
            changed |= ImGui::SliderFloat("Bottom width", &p.trapBottom, 0.1f, 2.0f);
            changed |= ImGui::SliderFloat("Top width", &p.trapTop, 0.0f, 2.0f);
            changed |= ImGui::SliderFloat("Height", &p.trapHeight, 0.1f, 2.0f);
            break;
        case ShapeKind::Circle:
            changed |= ImGui::SliderInt("Segments", &p.segments, 3, 128);
            break;
        case ShapeKind::Ellipse:
            changed |= ImGui::SliderFloat("Radius X", &p.radiusX, 0.1f, 1.0f);
            changed |= ImGui::SliderFloat("Radius Y", &p.radiusY, 0.1f, 1.0f);
            changed |= ImGui::SliderInt("Segments", &p.segments, 3, 128);
            break;
        case ShapeKind::Star:
            changed |= ImGui::SliderInt("Points", &p.starPoints, 3, 12);
            changed |= ImGui::SliderFloat("Inner radius ratio", &p.innerRatio, 0.1f, 0.9f);
            break;
        case ShapeKind::Sphere:
            changed |= ImGui::SliderInt("Sectors", &p.segments, 3, 96);
            changed |= ImGui::SliderInt("Stacks", &p.stacks, 2, 64);
            break;
        case ShapeKind::Cylinder:
        case ShapeKind::Cone:
            changed |= ImGui::SliderInt("Segments", &p.segments, 3, 96);
            break;
        case ShapeKind::TruncatedCone:
            changed |= ImGui::SliderFloat("Top radius", &p.topRadius, 0.02f, 0.5f);
            changed |= ImGui::SliderInt("Segments", &p.segments, 3, 96);
            break;
        case ShapeKind::Torus:
            changed |= ImGui::SliderFloat("Tube radius", &p.minorRadius, 0.03f, 0.45f);
            changed |= ImGui::SliderInt("Ring segments", &p.segments, 3, 96);
            changed |= ImGui::SliderInt("Tube segments", &p.stacks, 3, 64);
            break;
        case ShapeKind::Prism:
            changed |= ImGui::SliderInt("Base sides", &p.prismSides, 3, 16);
            break;
        case ShapeKind::Surface: {
            SurfaceParams& s = p.surface;
            ImGui::Text("z = f(x, y)");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##expr", buf.expr, sizeof(buf.expr), ImGuiInputTextFlags_EnterReturnsTrue)) {
                s.expression = buf.expr;
                changed = true;
            }
            ImGui::TextDisabled("Enter to apply. Vars x, y; sin cos sqrt exp abs pow ^ pi ...");
            if (ImGui::BeginCombo("Preset", "Choose...")) {
                for (const auto& pr : SURFACE_PRESETS) {
                    if (ImGui::Selectable(pr.label)) {
                        s.expression = pr.expr;
                        s.xMin = s.yMin = -pr.range;
                        s.xMax = s.yMax = pr.range;
                        copyTo(buf.expr, sizeof(buf.expr), s.expression);
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            changed |= ImGui::DragFloatRange2("x range", &s.xMin, &s.xMax, 0.1f, -50.0f, 50.0f);
            changed |= ImGui::DragFloatRange2("y range", &s.yMin, &s.yMax, 0.1f, -50.0f, 50.0f);
            changed |= ImGui::SliderInt("Resolution", &s.resolution, 8, 200);
            break;
        }
        case ShapeKind::Model:
            ImGui::TextWrapped("File: %s", p.modelPath.c_str());
            if (ImGui::Button("Reload file")) changed = true;
            break;
        default:
            ImGui::TextDisabled("No parameters for this shape.");
    }
    return changed;
}

void transformSection(SceneObject& o) {
    ObjectState& s = o.st;
    ImGui::DragFloat3("Translate", glm::value_ptr(s.position), 0.02f);
    ImGui::SliderFloat3("Rotate (deg)", glm::value_ptr(s.rotation), -180.0f, 180.0f);

    static bool uniform = true;
    ImGui::Checkbox("Uniform scale", &uniform);
    if (uniform) {
        float v = s.scale.x;
        if (ImGui::DragFloat("Scale", &v, 0.01f, 0.01f, 20.0f)) s.scale = glm::vec3(v);
    } else {
        ImGui::DragFloat3("Scale", glm::value_ptr(s.scale), 0.01f, 0.01f, 20.0f);
    }
    if (ImGui::Button("Reset transform")) {
        s.position = glm::vec3(0.0f, 0.5f, 0.0f);
        s.rotation = glm::vec3(0.0f);
        s.scale    = glm::vec3(1.0f);
    }

    // The matrix the vertex shader multiplies every vertex with: M = T * Rz * Ry * Rx * S
    if (ImGui::TreeNode("Model matrix  M = T * R * S")) {
        glm::mat4 M = o.modelMatrix();
        for (int r = 0; r < 4; ++r)
            ImGui::Text("%7.3f %7.3f %7.3f %7.3f", M[0][r], M[1][r], M[2][r], M[3][r]);
        ImGui::TreePop();
    }
}

void textureControls(AppState& app, SceneObject& o, EditBuffers& buf) {
    static std::vector<std::string> files;
    if (files.empty()) files = Paths::listFiles("assets/textures", {".png", ".jpg", ".jpeg", ".bmp", ".tga"});

    auto load = [&](const std::string& path) {
        o.st.texturePath = path;
        copyTo(buf.texture, sizeof(buf.texture), path);
        app.scene.applyTexture(o);
    };

    if (ImGui::BeginCombo("Image", o.st.texturePath.empty() ? "(built-in checkerboard)" : o.st.texturePath.c_str())) {
        if (ImGui::Selectable("(built-in checkerboard)", o.st.texturePath.empty())) load("");
        for (const auto& f : files)
            if (ImGui::Selectable(f.c_str(), f == o.st.texturePath)) load(f);
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(-60);
    bool enter = ImGui::InputText("##texpath", buf.texture, sizeof(buf.texture), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Load") || enter) load(buf.texture);
    ImGui::TextDisabled("Type any png/jpg/bmp/tga path, then Load.");
    if (ImGui::Button("Rescan assets/textures")) files = Paths::listFiles("assets/textures", {".png", ".jpg", ".jpeg", ".bmp", ".tga"});
    ImGui::SliderFloat("Tiling", &o.st.uvScale, 0.25f, 8.0f);
}

void appearanceSection(AppState& app, SceneObject& o, EditBuffers& buf) {
    ObjectState& s = o.st;
    ImGui::Text("Render mode");
    int mode = (int)s.mode;
    for (int m = 0; m <= (int)RenderMode::Wireframe; ++m)
        ImGui::RadioButton(renderModeName((RenderMode)m), &mode, m);
    s.mode = (RenderMode)mode;

    switch (s.mode) {
        case RenderMode::Flat:
            ImGui::ColorEdit3("Color", glm::value_ptr(s.color));
            break;
        case RenderMode::Wireframe:
            ImGui::ColorEdit3("Line color", glm::value_ptr(s.color));
            break;
        case RenderMode::Phong:
            ImGui::ColorEdit3("Color", glm::value_ptr(s.color));
            break;
        case RenderMode::Gouraud: {
            bool changed = false;
            int scheme = (int)s.vertexScheme;
            changed |= ImGui::RadioButton("Rainbow vertex colors", &scheme, 0);
            changed |= ImGui::RadioButton("Two-color gradient", &scheme, 1);
            s.vertexScheme = (MeshUtil::ColorScheme)scheme;
            if (s.vertexScheme == MeshUtil::ColorScheme::Gradient) {
                changed |= ImGui::ColorEdit3("Vertex color A", glm::value_ptr(s.gradientA));
                changed |= ImGui::ColorEdit3("Vertex color B", glm::value_ptr(s.gradientB));
            }
            if (changed) o.applyVertexColors();
            break;
        }
        case RenderMode::Texture:
            textureControls(app, o, buf);
            break;
    }
    if (s.mode == RenderMode::Phong || s.mode == RenderMode::Gouraud || s.mode == RenderMode::Texture) {
        ImGui::SliderFloat("Specular", &s.specular, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &s.shininess, 2.0f, 256.0f);
    }
}

void drawInspector(AppState& app) {
    static EditBuffers buf;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(360, io.DisplaySize.y - 40), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text("FPS: %.1f   Objects: %d", app.fps, (int)app.scene.objects.size());
    if (!app.status.empty()) ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1), "%s", app.status.c_str());
    ImGui::Separator();

    // ---- object list
    if (ImGui::CollapsingHeader("Scene objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginListBox("##objects", ImVec2(-1, 7 * ImGui::GetTextLineHeightWithSpacing()))) {
            for (int i = 0; i < (int)app.scene.objects.size(); ++i) {
                ImGui::PushID(i);
                if (ImGui::Selectable(app.scene.objects[i]->st.name.c_str(), app.scene.selected == i))
                    app.scene.selected = i;
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }
        bool has = app.scene.selectedObject() != nullptr;
        if (ImGui::Button("Duplicate") && has) app.scene.duplicateSelected();
        ImGui::SameLine();
        if (ImGui::Button("Delete") && has) app.scene.removeSelected();
        ImGui::SameLine();
        if (ImGui::Button("Clear all")) app.scene.clear();
    }

    // ---- selected object
    if (SceneObject* o = app.scene.selectedObject()) {
        if (buf.owner != o) syncBuffers(buf, *o);
        ImGui::PushID(o);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Selected: %s", shapeName(o->st.kind));
        ImGui::SetNextItemWidth(-60);
        if (ImGui::InputText("Name", buf.name, sizeof(buf.name))) o->st.name = buf.name;
        ImGui::Checkbox("Visible", &o->st.visible);

        if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (shapeParameters(*o, buf)) o->rebuild();
        }
        if (ImGui::CollapsingHeader("Transform (translate / rotate / scale)", ImGuiTreeNodeFlags_DefaultOpen))
            transformSection(*o);
        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
            appearanceSection(app, *o, buf);
        if (!o->error.empty())
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", o->error.c_str());

        ImGui::PopID();
    } else {
        ImGui::Separator();
        ImGui::TextDisabled("Use the 'Add Shape' menu to create an object,");
        ImGui::TextDisabled("then select it here to edit it.");
    }

    // ---- global settings
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::SliderFloat3("Light pos", glm::value_ptr(app.light.position), -15.0f, 15.0f);
        ImGui::ColorEdit3("Ambient",  glm::value_ptr(app.light.ambient));
        ImGui::ColorEdit3("Diffuse",  glm::value_ptr(app.light.diffuse));
        ImGui::ColorEdit3("Specular", glm::value_ptr(app.light.specular));
    }
    if (ImGui::CollapsingHeader("Camera / View")) {
        ImGui::SliderFloat("FOV", &app.camera.fov, 15.0f, 120.0f);
        ImGui::SliderFloat("Distance", &app.camera.distance, 1.0f, 50.0f);
        if (ImGui::Button("Reset")) app.resetCamera();
        ImGui::SameLine();
        if (ImGui::Button("Front (2D)")) app.frontView();
        ImGui::SameLine();
        if (ImGui::Button("Top")) app.topView();
        ImGui::Checkbox("Grid", &app.scene.showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &app.scene.showAxes);
        ImGui::ColorEdit3("Background", glm::value_ptr(app.background));
    }
    ImGui::End();
}

} // namespace

void Gui::draw(AppState& app) {
    drawMenuBar(app);
    drawInspector(app);
    drawImportPopup(app);
    drawHelpWindow(app);
}
