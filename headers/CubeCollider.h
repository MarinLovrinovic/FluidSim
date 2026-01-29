//
// Created by lovri on 9/6/2024.
//

#ifndef CLOTHSIM_CUBECOLLIDER_H
#define CLOTHSIM_CUBECOLLIDER_H

#include "Collider.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

class CubeCollider : public Collider {
public:
    glm::vec3 position;
    float sideLength;
    float frictionCoefficient;
    CubeCollider(glm::vec3 position, float sideLength, float frictionCoefficient);
    glm::vec4 Displace(glm::vec3 particlePosition, glm::vec3& particleNextPosition) const override;
};

#endif //CLOTHSIM_CUBECOLLIDER_H
