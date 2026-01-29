#include "Camera.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 200.0f);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(glm::vec3(offset),
                glm::vec3(offset + -localZ),
                glm::vec3(0, 1, 0));
}