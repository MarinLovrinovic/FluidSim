//
// Created by lovri on 9/6/2024.
//

#ifndef CLOTHSIM_COLLIDER_H
#define CLOTHSIM_COLLIDER_H

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

class Collider {
public:
    virtual glm::vec4 Displace(glm::vec3 particlePosition, glm::vec3& particleNextPosition) const = 0;
    virtual ~Collider() = default;
};

#endif //CLOTHSIM_COLLIDER_H
