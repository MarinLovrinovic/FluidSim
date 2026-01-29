#ifndef IRGLAB_TRANSFORM_H
#define IRGLAB_TRANSFORM_H


#include "glm/glm.hpp"

class Transform {
protected:
    glm::vec4 localX = glm::vec4(1, 0, 0, 0);
    glm::vec4 localY = glm::vec4(0, 1, 0, 0);
    glm::vec4 localZ = glm::vec4(0, 0, 1, 0);
    glm::vec4 offset = glm::vec4(0, 0, 0, 1);
    glm::vec4 scale = glm::vec4(1, 1, 1, 1);
public:
    glm::mat4 GetModelMatrix() const;
    glm::mat4 RotationMatrix() const;
    glm::mat4 ScaleMatrix() const;
    glm::mat4 TranslationMatrix() const;

    void SetScale(glm::vec3 value);
    void Scale(glm::vec3 by);

    void SetPosition(glm::vec3 value);
    void Move(glm::vec3 by);

    void SetRotation(float angle, glm::vec3 axis);
    void Rotate(float angle, glm::vec3 axis);

    glm::mat4 GlobalToLocalDir() const;
    glm::mat4 LocalToGlobalDir() const;

    glm::mat4 GlobalToLocalPos() const;
    glm::mat4 LocalToGlobalPos() const;

    glm::vec3 GetPosition() const;

    glm::vec3 GetLocalX() const;
    glm::vec3 GetLocalY() const;
    glm::vec3 GetLocalZ() const;
};


#endif //IRGLAB_TRANSFORM_H
