#pragma once

#include "glm/glm.hpp"

template<glm::length_t L>
glm::vec<L, float> Mirror(
    const glm::vec<L, float>& point,
    const glm::vec<L, float>& pointOnSurface,
    const glm::vec<L, float>& surfaceNormal)
{
    const auto surfaceToPoint = point - pointOnSurface;
    return pointOnSurface + glm::reflect(surfaceToPoint, surfaceNormal);
}

glm::vec3 RaycastZ0(glm::vec3 origin, glm::vec3 direction);