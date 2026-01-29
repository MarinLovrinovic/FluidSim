//
// Created by lovri on 9/6/2024.
//
#include "SphereCollider.h"
#include "glm/geometric.hpp"

SphereCollider::SphereCollider(glm::vec3 position, float radius, float frictionCoefficient) :
        position(position),
        radius(radius),
        frictionCoefficient(frictionCoefficient) {

}

glm::vec4 SphereCollider::Displace(glm::vec3 particlePosition, glm::vec3 &particleNextPosition) const {
    float distanceFromCenter = glm::distance(particleNextPosition, position);
    if (distanceFromCenter >= radius) {
        return glm::vec4(0);
    }

    glm::vec3 normal = (particleNextPosition - position) / distanceFromCenter;

    particleNextPosition = position + normal * radius;

    return { normal, frictionCoefficient };
}

