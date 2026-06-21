#pragma once

#include "glm/glm.hpp"

glm::vec2 Mirror(glm::vec2 point, glm::vec2 pointOnSurface, glm::vec2 surfaceNormal);

glm::vec3 RaycastZ0(glm::vec3 origin, glm::vec3 direction);