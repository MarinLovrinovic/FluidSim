#include "Utils.h"

glm::vec2 Mirror(const glm::vec2 point, const glm::vec2 pointOnSurface, const glm::vec2 surfaceNormal)
{
    const glm::vec2 surfaceToPoint = point - pointOnSurface;
    return pointOnSurface + glm::reflect(surfaceToPoint, surfaceNormal);
}