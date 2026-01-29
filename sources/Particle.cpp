//
// Created by lovri on 2/2/2025.
//
#include "Particle.h"

Particle::Particle(glm::vec3 initialPos) {
    currentPosition = initialPos;
    previousPosition = initialPos;
    nextPosition = initialPos;
    velocity = glm::vec3(0);
    mass = 1;
}

Particle::Particle(glm::vec3 initialPos, glm::vec3 previousPos) {
    currentPosition = initialPos;
    previousPosition = previousPos;
    nextPosition = initialPos;
    velocity = glm::vec3(0);
    mass = 1;
}

