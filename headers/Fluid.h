#pragma once

#include <vector>

#include "Object.h"

#include "glm/vec2.hpp"


using namespace std;

class Fluid
{
private:
    int particleCount;
    float smoothingRadius;
    float targetDensity;
    float pressureMultiplier;
    vector<glm::vec2> previousPositions;
    vector<glm::vec2> currentPositions;
    vector<glm::vec2> forces;
    vector<float> densities;
    Object* object;
    glm::vec2 corner1, corner2;
public:
    Fluid(int particlesX, int particlesY, float smoothingRadius, float targetDensity, float pressureMultiplier, glm::vec2 startingPosition, float particleSize, glm::vec2 corner1, glm::vec2 corner2, Object* object);
    float CalculateDensity(glm::vec2 samplePoint) const;
    float ConvertDensityToPressure(float density) const;
    float CalculateSharedPressure(float densityA, float densityB) const;
    glm::vec2 CalculatePressureForce(int particleIndex) const;
    void Update(float dt, glm::vec2 gravity);
};
