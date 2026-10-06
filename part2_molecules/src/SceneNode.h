// =============================================================================
// SceneNode.h - Hierarchical scene graph node
// =============================================================================
#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"
#include "Shader.h"

struct Material {
    glm::vec3 ambient   = glm::vec3(0.2f);
    glm::vec3 diffuse   = glm::vec3(0.8f);
    glm::vec3 specular  = glm::vec3(0.5f);
    float     shininess = 64.0f;
};

class SceneNode {
public:
    // Local transform components
    glm::vec3 position    = glm::vec3(0.0f);
    glm::vec3 scale       = glm::vec3(1.0f);
    glm::vec3 rotAxis     = glm::vec3(0.0f, 1.0f, 0.0f);
    float     rotAngle    = 0.0f; // degrees

    Material  material;
    Mesh*     mesh    = nullptr;  // non-owning
    bool      visible = true;
    bool      wireframe = false;

    std::vector<std::unique_ptr<SceneNode>> children;

    SceneNode() = default;
    virtual ~SceneNode() = default;

    // Called each frame with the elapsed time delta
    virtual void update(float dt) {}

    // Draw this node and all children
    void draw(Shader& shader, const glm::mat4& parentTransform) const;

    glm::mat4 localTransform() const;

    SceneNode* addChild(std::unique_ptr<SceneNode> child);
};
