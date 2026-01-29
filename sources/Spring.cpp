//
// Created by lovri on 2/2/2025.
//
#include "Spring.h"
#include <glm/glm.hpp>

Spring::Spring(Particle *first, Particle *second, float springConstant, float springDampingCoefficient) : springConstant(springConstant), springDampingCoefficient(springDampingCoefficient) {
    a = first;
    b = second;
    equilibriumLength = glm::distance(a->currentPosition, b->currentPosition);
}

