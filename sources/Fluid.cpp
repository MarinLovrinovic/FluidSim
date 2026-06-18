#include "Fluid.h"

#include <random>

#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/ext/scalar_constants.hpp"
#include <glm/gtc/random.hpp>

float pi = glm::pi<float>();

float SmoothingKernelFlat(const float radius, const float distance)
{
    const float volume = pi * glm::pow(radius, 8) / 4;
    const float value = max(0.0f, radius * radius - distance * distance);
    return value * value * value / volume;
}

float SmoothingKernelDerivativeFlat(const float radius, const float distance)
{
    if (distance >= radius) return 0;
    const float f = radius * radius - distance * distance;
    const float scale = -24 / (pi * glm::pow(radius, 8));
    return scale * distance * f * f;
}

float SmoothingKernel(const float radius, const float distance)
{
    if (distance >= radius) return 0;

    const float volume = (pi * glm::pow(radius, 4)) / 6;
    return (radius - distance) * (radius - distance) / volume;
}

float SmoothingKernelDerivative(const float radius, const float distance)
{
    if (distance >= radius) return 0;

    const float scale = 12 / (glm::pow(radius, 4) * pi);
    return (distance - radius) * scale;
}

void UpdateTransforms()
{

}

Fluid::Fluid(
    const int particlesX,
    const int particlesY,
    const float smoothingRadius,
    const float targetDensity,
    const float pressureMultiplier,
    const glm::vec2 startingPosition,
    const float particleSize,
    const glm::vec2 corner1,
    const glm::vec2 corner2,
    Object* object) :
particleCount(particlesX * particlesY),
smoothingRadius(smoothingRadius),
targetDensity(targetDensity),
pressureMultiplier(pressureMultiplier),
object(object),
corner1(corner1),
corner2(corner2)
{
    previousPositions.resize(particleCount);
    currentPositions.resize(particleCount);
    forces.resize(particleCount);
    densities.resize(particleCount);
    object->transforms.resize(particleCount);

    for (int x = 0; x < particlesX; ++x)
    {
        for (int y = 0; y < particlesY; ++y)
        {
            int index = y * particlesX + x;
            glm::vec2 particlePosition = startingPosition + glm::vec2(particleSize * x, particleSize * y);
            previousPositions[index] = particlePosition;
            currentPositions[index] = particlePosition;
        }
    }

    const float maxOffset = 1;

    // add random offsets
    std::random_device rd;
    std::mt19937 gen(rd());
    uniform_real_distribution<float> dist(-maxOffset, maxOffset);
    for (int i = 0; i < particleCount; i++)
    {
        // const glm::vec2 offset = glm::vec2(dist(gen), dist(gen));
        // currentPositions[i] += offset;
        // previousPositions[i] += offset;
        object->transforms[i].SetPosition(glm::vec3(currentPositions[i], 0));
        object->transforms[i].SetScale(glm::vec3(particleSize));
    }
}

float Fluid::CalculateDensity(const glm::vec2 samplePoint) const
{
    float density = 0;
    constexpr float mass = 1;

    for (glm::vec2 position : currentPositions)
    {
        const float distance = glm::length(position - samplePoint);
        const float influence = SmoothingKernel(smoothingRadius, distance);
        density += mass * influence;
    }
    return density;
}

float Fluid::ConvertDensityToPressure(const float density) const
{
    const float densityError = density - targetDensity;
    const float pressure = densityError * pressureMultiplier;
    return pressure;
}

float Fluid::CalculateSharedPressure(float densityA, float densityB) const
{
    float pressureA = ConvertDensityToPressure(densityA);
    float pressureB = ConvertDensityToPressure(densityB);
    return (pressureA + pressureB) / 2;
}

glm::vec2 Fluid::CalculatePressureForce(const int particleIndex) const
{
    glm::vec2 pressureForce(0);
    constexpr float mass = 1;
    const float particleDensity = densities[particleIndex];

    for (int otherParticleIndex = 0; otherParticleIndex < particleCount; otherParticleIndex++)
    {
        if (otherParticleIndex == particleIndex) continue;
        glm::vec2 offset = currentPositions[otherParticleIndex] - currentPositions[particleIndex];
        const float distance = glm::length(offset);
        // pick a random direction in case two particles share the same position
        glm::vec2 direction = distance == 0 ? glm::circularRand(1.0f) : offset / distance;
        const float slope = SmoothingKernelDerivative(smoothingRadius, distance);
        const float otherParticleDensity = densities[otherParticleIndex];
        const float sharedPressure = CalculateSharedPressure(particleDensity, otherParticleDensity);
        pressureForce += sharedPressure * direction * slope * mass / otherParticleDensity;
    }
    return pressureForce;
}

void Fluid::Update(const float dt, const glm::vec2 gravity)
{
    for (int i = 0; i < particleCount; i++)
    {
        forces[i] = gravity;
    }

    for (int i = 0; i < particleCount; i++)
    {
        densities[i] = CalculateDensity(currentPositions[i]);
    }

    // TODO: accumulate SPH forces for each particle
    for (int i = 0; i < particleCount; i++)
    {
        forces[i] += CalculatePressureForce(i);
    }


    // for (int i = 0; i < particleCount; i++)
    // {
    //     for (int j = 0; j < particleCount; j++)
    //     {
    //         if (i == j) continue;
    //
    //         // this gives us a repelling force with magnitude = 1 / distance
    //         glm::vec2 fromOtherParticle = currentPositions[i] - currentPositions[j];
    //         float distance = glm::length(fromOtherParticle);
    //         if (distance < 0.1f || distance > 0.5f) continue;
    //
    //         forces[i] += 0.2f * fromOtherParticle / (distance * distance);
    //     }
    // }

    for (int i = 0; i < particleCount; i++)
    {
        glm::vec2 acceleration = forces[i] / densities[i];
        glm::vec2 currentPosition = currentPositions[i];
        glm::vec2 nextPosition = 2.0f * currentPosition - previousPositions[i] + acceleration * dt * dt;


        // calculate collisions
        constexpr bool bounce = true;
        const float bounceFactor = 0.3f;
        const glm::vec2 minPos = glm::vec2(min(corner1.x, corner2.x), min(corner1.y, corner2.y));
        const glm::vec2 maxPos = glm::vec2(max(corner1.x, corner2.x), max(corner1.y, corner2.y));
        if (bounce)
        {
            if (nextPosition.x < minPos.x)
            {
                nextPosition = Mirror(nextPosition, minPos, glm::vec2(1, 0));
                currentPosition = Mirror(currentPosition, minPos, glm::vec2(1, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            } else if (maxPos.x < nextPosition.x)
            {
                nextPosition = Mirror(nextPosition, maxPos, glm::vec2(-1, 0));
                currentPosition = Mirror(currentPosition, maxPos, glm::vec2(-1, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            }
            if (nextPosition.y < minPos.y)
            {
                nextPosition = Mirror(nextPosition, minPos, glm::vec2(0, 1));
                currentPosition = Mirror(currentPosition, minPos, glm::vec2(0, 1));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            } else if (maxPos.y < nextPosition.y)
            {
                nextPosition = Mirror(nextPosition, maxPos, glm::vec2(0, -1));
                currentPosition = Mirror(currentPosition, maxPos, glm::vec2(0, -1));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            }
        } else
        {
            nextPosition = glm::clamp(nextPosition, minPos, maxPos);
        }

        previousPositions[i] = currentPosition;
        currentPositions[i] = nextPosition;
        object->transforms[i].SetPosition(glm::vec3(currentPositions[i], 0));
    }
}
