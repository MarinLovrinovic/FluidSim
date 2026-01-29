#include "Transform.h"

#include <glm/glm.hpp>
#include "glm/ext.hpp"



glm::mat4 Transform::RotationMatrix() const {
    return {localX, localY, localZ, glm::vec4(0, 0, 0, 1)};
}

glm::mat4 Transform::ScaleMatrix() const {
    return glm::scale(glm::mat4(1), glm::vec3(scale));
}

glm::mat4 Transform::TranslationMatrix() const {
    return glm::translate(glm::mat4(1), glm::vec3(offset));
}

glm::mat4 Transform::GetModelMatrix() const {
    return TranslationMatrix() * RotationMatrix() * ScaleMatrix();
}

void Transform::SetScale(glm::vec3 value) {
    scale = glm::vec4(value, 1);
}
void Transform::Scale(glm::vec3 by) {
    scale *= glm::vec4(by, 1);
}

void Transform::SetPosition(glm::vec3 value) {
    offset = glm::vec4(value, 1);
}
void Transform::Move(glm::vec3 by) {
    offset += glm::vec4(by, 0);
}

void Transform::SetRotation(float angle, glm::vec3 axis) {
    auto rotation = glm::rotate(glm::mat4(1), angle, axis);;
    localX = rotation[0];
    localY = rotation[1];
    localZ = rotation[2];
}
void Transform::Rotate(float angle, glm::vec3 axis) {
    auto rotation = glm::rotate(glm::mat4(1), angle, axis);;
    localX = rotation * localX;
    localY = rotation * localY;
    localZ = rotation * localZ;
}

glm::mat4 Transform::LocalToGlobalDir() const {
    return RotationMatrix();
}
glm::mat4 Transform::GlobalToLocalDir() const {
    return glm::inverse(LocalToGlobalDir());
}

glm::mat4 Transform::LocalToGlobalPos() const {
    return GetModelMatrix();
}
glm::mat4 Transform::GlobalToLocalPos() const {
    return glm::inverse(LocalToGlobalPos());
}

glm::vec3 Transform::GetPosition() const {
    return offset;
}
glm::vec3 Transform::GetLocalX() const {
    return localX;
}
glm::vec3 Transform::GetLocalY() const {
    return localY;
}
glm::vec3 Transform::GetLocalZ() const {
    return localZ;
}
