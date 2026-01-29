//
// Created by lovri on 2/2/2025.
//

#ifndef CLOTHSIM_PARTICLE_H
#define CLOTHSIM_PARTICLE_H

#include "glm/vec3.hpp"

class Particle {
public:
    glm::vec3 previousPosition;
    glm::vec3 currentPosition;
    glm::vec3 nextPosition;
    glm::vec3 velocity;
    float mass;
    glm::vec3 force = glm::vec3(0);
    explicit Particle(glm::vec3 initialPos);
    Particle(glm::vec3 initialPos, glm::vec3 previousPos);
};

#endif //CLOTHSIM_PARTICLE_H
