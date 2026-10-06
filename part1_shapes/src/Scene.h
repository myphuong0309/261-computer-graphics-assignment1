// =============================================================================
// Scene.h - the collection of objects the user builds, plus the ground grid / axes
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "SceneObject.h"

class Scene {
public:
    std::vector<std::unique_ptr<SceneObject>> objects;
    int  selected   = -1;     // index into objects, -1 = none
    bool showGrid   = true;
    bool showAxes   = true;

    // ---- editing
    // Adds a shape at the next free spot of the layout. Returns nullptr (and fills `error`) if its mesh cannot be built.
    SceneObject* addShape(ShapeKind kind, std::string& error);
    SceneObject* addModel(const std::string& path, std::string& error);
    SceneObject* selectedObject();
    void removeSelected();
    void duplicateSelected();
    void clear();
    void toggleWireframe();               // selected object, or all objects when nothing is selected
    void addDemo();                       // one of every shape, in a mix of render modes

    // Loads (or reuses) the texture for `obj.st.texturePath`; falls back to the checkerboard on error.
    void applyTexture(SceneObject& obj);

    // ---- per frame
    void draw(Shader& shader);            // objects + helpers; the caller sets camera/light uniforms

private:
    void initHelpers();
    void drawHelpers(Shader& shader);
    SceneObject* adopt(std::unique_ptr<SceneObject> obj);

    TextureCache textures_;
    std::unique_ptr<ShapeMesh> gridMesh_, axesMesh_, boxMesh_;
    int counter_ = 0;
};
