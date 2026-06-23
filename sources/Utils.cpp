#include "Utils.h"

// Raycast against plane Z = 0
glm::vec3 RaycastZ0(const glm::vec3 origin, const glm::vec3 direction)
{
    if (direction.z == 0)
    {
        return {glm::vec2(origin), 0};
    }
    const float t = -origin.z / direction.z;
    return origin + t * direction;
}
