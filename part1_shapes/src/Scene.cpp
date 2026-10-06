// =============================================================================
// Scene.cpp
// =============================================================================
#include "Scene.h"
#include <algorithm>

SceneObject* Scene::adopt(std::unique_ptr<SceneObject> obj) {
    applyTexture(*obj);
    objects.push_back(std::move(obj));
    selected = (int)objects.size() - 1;
    return objects.back().get();
}

SceneObject* Scene::addShape(ShapeKind kind, std::string& error) {
    ObjectState st;
    st.kind = kind;
    st.name = std::string(shapeName(kind)) + " " + std::to_string(++counter_);
    st.position = glm::vec3(0.0f, 0.5f, 0.0f);   // new shapes appear at the centre of the view
    if (is2D(kind)) st.mode = RenderMode::Gouraud;   // flat shapes show the vertex-color blend best
    auto obj = std::make_unique<SceneObject>(st);
    if (!obj->rebuild()) { error = obj->error; --counter_; return nullptr; }
    error.clear();
    return adopt(std::move(obj));
}

SceneObject* Scene::addModel(const std::string& path, std::string& error) {
    ObjectState st;
    st.kind = ShapeKind::Model;
    st.params.modelPath = path;
    size_t slash = path.find_last_of("/\\");
    st.name = path.substr(slash == std::string::npos ? 0 : slash + 1);
    st.position = glm::vec3(0.0f, 0.5f, 0.0f);   // new shapes appear at the centre of the view
    st.scale = glm::vec3(1.5f);
    auto obj = std::make_unique<SceneObject>(st);
    if (!obj->rebuild()) { error = obj->error; return nullptr; }
    error.clear();
    return adopt(std::move(obj));
}

SceneObject* Scene::selectedObject() {
    return (selected >= 0 && selected < (int)objects.size()) ? objects[selected].get() : nullptr;
}

void Scene::removeSelected() {
    if (!selectedObject()) return;
    objects.erase(objects.begin() + selected);
    selected = std::min(selected, (int)objects.size() - 1);
}

void Scene::duplicateSelected() {
    SceneObject* src = selectedObject();
    if (!src) return;
    ObjectState st = src->st;
    st.name += " copy";
    st.position.x += 0.6f;
    auto obj = std::make_unique<SceneObject>(st);
    if (!obj->rebuild()) return;
    adopt(std::move(obj));
}

void Scene::clear() {
    objects.clear();
    selected = -1;
    counter_ = 0;
}

void Scene::toggleWireframe() {
    auto toggle = [](ObjectState& st) {
        if (st.mode == RenderMode::Wireframe) {
            st.mode = st.lastSolidMode;
        } else {
            st.lastSolidMode = st.mode;
            st.mode = RenderMode::Wireframe;
        }
    };
    if (SceneObject* o = selectedObject()) toggle(o->st);
    else for (auto& obj : objects) toggle(obj->st);
}

void Scene::addDemo() {
    clear();
    std::string err;
    const RenderMode modes[] = {RenderMode::Phong, RenderMode::Gouraud, RenderMode::Texture,
                                RenderMode::Flat, RenderMode::Wireframe};
    // 2D shapes: one row standing up at the back; 3D solids: two rows on the ground.
    int n2 = 0, n3 = 0;
    for (int k = 0; k < (int)ShapeKind::Surface; ++k) {
        SceneObject* o = addShape((ShapeKind)k, err);
        if (!o) continue;
        if (is2D((ShapeKind)k)) {
            o->st.position = glm::vec3((n2 - 4) * 1.6f, 2.6f, -2.0f);
            o->st.mode = (n2 % 2 == 0) ? RenderMode::Gouraud : RenderMode::Phong;
            ++n2;
        } else {
            o->st.position = glm::vec3((n3 % 4 - 1.5f) * 2.0f, 0.5f, (n3 / 4) * 2.0f);
            o->st.mode = modes[n3 % 5];
            ++n3;
        }
    }
    if (SceneObject* s = addShape(ShapeKind::Surface, err)) {
        s->st.position = glm::vec3(4.5f, 0.5f, 1.0f);
        s->st.scale = glm::vec3(2.0f);
        s->st.mode = RenderMode::Gouraud;
    }
    selected = -1;
}

void Scene::applyTexture(SceneObject& obj) {
    std::string err;
    obj.texture = textures_.get(obj.st.texturePath, err);
    if (!obj.texture) {                         // bad path: fall back to the checkerboard and tell the user
        obj.error = err;
        obj.texture = textures_.get("", err);
    } else if (obj.hasMesh()) {
        obj.error.clear();
    }
}

void Scene::draw(Shader& shader) {
    initHelpers();
    drawHelpers(shader);
    for (auto& o : objects) o->draw(shader);
    if (SceneObject* sel = selectedObject()) sel->drawSelection(shader, *boxMesh_);
}

// ---------------------------------------------------------------------------
// Ground grid (XZ plane) and XYZ axes - plain colored line meshes
// ---------------------------------------------------------------------------
void Scene::initHelpers() {
    if (gridMesh_) return;

    auto line = [](MeshData& m, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        unsigned base = (unsigned)m.vertices.size();
        Vertex va, vb;
        va.position = a; va.color = c;
        vb.position = b; vb.color = c;
        m.vertices.push_back(va);
        m.vertices.push_back(vb);
        m.wireIndices.push_back(base);
        m.wireIndices.push_back(base + 1);
    };

    MeshData grid, axes, box;
    const int N = 10;
    for (int i = -N; i <= N; ++i) {
        if (i == 0) continue;   // the axes are drawn separately
        glm::vec3 c(0.22f);
        line(grid, {(float)i, 0, -N}, {(float)i, 0, N}, c);
        line(grid, {-N, 0, (float)i}, {N, 0, (float)i}, c);
    }
    line(axes, {-N, 0, 0}, {N, 0, 0}, {0.85f, 0.25f, 0.25f});   // X red
    line(axes, {0, 0, -N}, {0, 0, N}, {0.30f, 0.45f, 0.95f});   // Z blue
    line(axes, {0, 0, 0},  {0, N, 0}, {0.30f, 0.85f, 0.35f});   // Y green
    // unit cube (side 1) used as the selection box
    for (int i = 0; i < 8; ++i) {
        Vertex v;
        v.position = glm::vec3((i & 1) ? 0.5f : -0.5f, (i & 2) ? 0.5f : -0.5f, (i & 4) ? 0.5f : -0.5f);
        box.vertices.push_back(v);
    }
    for (int i = 0; i < 8; ++i)
        for (int bit = 1; bit <= 4; bit <<= 1)
            if (!(i & bit)) { box.wireIndices.push_back(i); box.wireIndices.push_back(i | bit); }
    boxMesh_  = std::make_unique<ShapeMesh>(std::move(box));
    gridMesh_ = std::make_unique<ShapeMesh>(std::move(grid));
    axesMesh_ = std::make_unique<ShapeMesh>(std::move(axes));
}

void Scene::drawHelpers(Shader& shader) {
    if (!showGrid && !showAxes) return;
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat3("normalMatrix", glm::mat3(1.0f));
    shader.setInt("mode", 5);   // unlit vertex color
    if (showGrid) gridMesh_->drawWire();
    if (showAxes) axesMesh_->drawWire();
}
