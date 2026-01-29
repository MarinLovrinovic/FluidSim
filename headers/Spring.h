//
// Created by lovri on 2/2/2025.
//

#ifndef CLOTHSIM_SPRING_H
#define CLOTHSIM_SPRING_H

#include "Particle.h"

class Spring {
public:
    Particle* a;
    Particle* b;
    float springConstant = 100;
    float springDampingCoefficient = 40;
    float equilibriumLength;
    Spring(Particle* first, Particle* second, float springConstant, float springDampingCoefficient);
};

#endif //CLOTHSIM_SPRING_H
