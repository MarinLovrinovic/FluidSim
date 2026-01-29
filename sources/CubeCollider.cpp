//
// Created by lovri on 9/6/2024.
//
#include "CubeCollider.h"
#include "glm/geometric.hpp"
#include <limits>

CubeCollider::CubeCollider(glm::vec3 position, float sideLength, float frictionCoefficient) :
position(position),
sideLength(sideLength),
frictionCoefficient(frictionCoefficient) {

}

glm::vec4 CubeCollider::Displace(glm::vec3 particlePosition, glm::vec3 &particleNextPosition) const {
    float halfSide = sideLength / 2;

    glm::vec3 fromCenter = particleNextPosition - position;
    if (abs(fromCenter.x) >= halfSide || abs(fromCenter.y) >= halfSide || abs(fromCenter.z) >= halfSide) {
        return glm::vec4(0);
    }

    // raycast against 3 planes from inside of cube
    glm::vec3 pos = particleNextPosition;
    glm::vec3 dir = particlePosition - particleNextPosition;
    glm::vec3 dirSign = glm::vec3(copysign(1, dir.x), copysign(1, dir.y), copysign(1, dir.z));

    glm::vec3 normal;
    float t = std::numeric_limits<float>::infinity();

    if (dir.x != 0) {
        float faceX = position.x + halfSide * dirSign.x;
        float tx = (faceX - pos.x) / dir.x;
        if (tx < t) {
            t = tx;
            normal = glm::vec3(dirSign.x, 0, 0);
        }
    }

    if (dir.y != 0) {
        float faceY = position.y + halfSide * dirSign.y;
        float ty = (faceY - pos.y) / dir.y;
        if (ty < t) {
            t = ty;
            normal = glm::vec3(0, dirSign.y, 0);
        }
    }

    if (dir.z != 0) {
        float faceZ = position.z + halfSide * dirSign.z;
        float tz = (faceZ - pos.z) / dir.z;
        if (tz < t) {
            t = tz;
            normal = glm::vec3(0, 0, dirSign.z);
        }
    }

    if (t == std::numeric_limits<float>::infinity()) {
        return glm::vec4(0);
    }

    particleNextPosition = pos + dir * t;

    return { normal, frictionCoefficient };
}
