// =============================================================================
// SceneNode.cpp
// =============================================================================
#include "SceneNode.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::mat4 SceneNode::localTransform() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(rotAngle), rotAxis);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    return T * R * S;
}

void SceneNode::draw(Shader& shader, const glm::mat4& parentTransform) const {
    if (!visible) return;

    glm::mat4 world = parentTransform * localTransform();

    if (mesh) {
        // Upload matrices
        shader.setMat4("model", world);
        glm::mat3 nm = glm::mat3(glm::transpose(glm::inverse(world)));
        shader.setMat3("normalMatrix", nm);

        // Upload material
        shader.setVec3("material.ambient",   material.ambient);
        shader.setVec3("material.diffuse",   material.diffuse);
        shader.setVec3("material.specular",  material.specular);
        shader.setFloat("material.shininess", material.shininess);
        shader.setBool("wireframe", wireframe);

        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        mesh->draw();

        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    for (const auto& child : children)
        child->draw(shader, world);
}

SceneNode* SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    children.push_back(std::move(child));
    return children.back().get();
}
