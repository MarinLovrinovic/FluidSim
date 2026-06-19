#include "Fluid.h"

#include <chrono>
#include <iostream>
#include <random>
#include <limits>
#include <optional>

#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/ext/scalar_constants.hpp"
#include <glm/gtc/random.hpp>

float pi = glm::pi<float>();

float SmoothingKernelFlat(const float radius, const float distance)
{
    if (distance >= radius) return 0;

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

glm::vec<2, int> Fluid::PositionToCellCoord(const glm::vec2 point, const float radius)
{
    const int cellX = static_cast<int>(point.x / radius);
    const int cellY = static_cast<int>(point.y / radius);
    return {cellX, cellY};
}

unsigned int Fluid::HashCell(const int cellX, const int cellY)
{
    const unsigned int a = static_cast<unsigned int>(cellX) * 15823;
    const unsigned int b = static_cast<unsigned int>(cellY) * 9737333;
    return a + b;
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
    const float viscosityStrength,
    const glm::vec2 startingPosition,
    const float particleSize,
    const glm::vec2 corner1,
    const glm::vec2 corner2,
    Object* object) :
particleCount(particlesX * particlesY),
smoothingRadius(smoothingRadius),
targetDensity(targetDensity),
pressureMultiplier(pressureMultiplier),
viscosityStrength(viscosityStrength),
object(object),
corner1(corner1),
corner2(corner2)
{
    previousPositions.resize(particleCount);
    currentPositions.resize(particleCount);
    velocities.resize(particleCount);
    forces.resize(particleCount);
    densities.resize(particleCount);
    spatialLookup.resize(particleCount);
    startIndices.resize(particleCount);
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

    ForeachPointWithinRadius(samplePoint, [&](const int pointIndex)
    {
        const glm::vec2 position = currentPositions[pointIndex];
        const float distance = glm::length(position - samplePoint);
        const float influence = SmoothingKernel(smoothingRadius, distance);
        density += mass * influence;
    });
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

    const glm::vec2 particlePosition = currentPositions[particleIndex];
    ForeachPointWithinRadius(particlePosition, [&](const int otherParticleIndex)
    {
        if (otherParticleIndex != particleIndex)
        {
            const glm::vec2 offset = currentPositions[otherParticleIndex] - particlePosition;
            const float distance = glm::length(offset);
            // pick a random direction in case two particles share the same position
            const glm::vec2 direction = distance == 0 ? glm::circularRand(1.0f) : offset / distance;
            const float slope = SmoothingKernelDerivative(smoothingRadius, distance);
            const float otherParticleDensity = densities[otherParticleIndex];
            const float sharedPressure = CalculateSharedPressure(particleDensity, otherParticleDensity);
            pressureForce += sharedPressure * direction * slope * mass / otherParticleDensity;
        }
    });
    return pressureForce;
}

glm::vec2 Fluid::CalculateViscosityForce(int particleIndex) const
{
    glm::vec2 viscosityForce(0);
    const glm::vec2 particlePosition = currentPositions[particleIndex];

    ForeachPointWithinRadius(particlePosition, [&](const int otherParticleIndex)
    {
        if (otherParticleIndex != particleIndex)
        {
            const float distance = glm::length(particlePosition - currentPositions[otherParticleIndex]);
            const float influence = SmoothingKernelFlat(smoothingRadius, distance);
            viscosityForce += (velocities[otherParticleIndex] - velocities[particleIndex]) * influence;
        }
    });
    return viscosityForce * viscosityStrength;
}

unsigned int Fluid::GetKeyFromHash(unsigned int hash) const
{
    return hash % static_cast<unsigned int>(particleCount);
}

void Fluid::UpdateSpatialLookup()
{
    // create unordered spatial lookup
    for (int i = 0; i < particleCount; i++)
    {
        const glm::vec<2, int> cell = PositionToCellCoord(currentPositions[i], smoothingRadius);
        const unsigned int cellKey = GetKeyFromHash(HashCell(cell.x, cell.y));
        spatialLookup[i] = SpatialLookupEntry {
            i,
            cellKey
        };
        startIndices[i] = std::numeric_limits<int>::max();
    }

    // sort by cell key
    std::sort(spatialLookup.begin(), spatialLookup.end());

    // calculate start indices of each unique cell key in the spatial lookup
    for (int i = 0; i < particleCount; i++)
    {
        const unsigned int key = spatialLookup[i].cellKey;
        if (i == 0 || spatialLookup[i - 1].cellKey != key)
        {
            startIndices[key] = i;
        }
    }
}

void Fluid::Update(const float dt, const glm::vec2 gravity)
{
    for (int i = 0; i < particleCount; i++)
    {
        forces[i] = gravity;
    }

    UpdateSpatialLookup();

    auto densityStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < particleCount; i++)
    {
        densities[i] = CalculateDensity(currentPositions[i]);
    }
    auto densityEnd = std::chrono::high_resolution_clock::now();

    auto pressureStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < particleCount; i++)
    {
        forces[i] += CalculatePressureForce(i);
    }
    auto pressureEnd = std::chrono::high_resolution_clock::now();

    auto viscosityStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < particleCount; i++)
    {
        forces[i] += CalculateViscosityForce(i);
    }
    auto viscosityEnd = std::chrono::high_resolution_clock::now();

    auto collisionsStart = std::chrono::high_resolution_clock::now();
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
        }
        else
        {
            nextPosition = glm::clamp(nextPosition, minPos, maxPos);
        }

        previousPositions[i] = currentPosition;
        currentPositions[i] = nextPosition;
        velocities[i] = (currentPositions[i] - previousPositions[i]) / dt;
        object->transforms[i].SetPosition(glm::vec3(currentPositions[i], 0));
    }
    auto collisionsEnd = std::chrono::high_resolution_clock::now();

    double densityMs = std::chrono::duration<double, std::milli>(densityEnd - densityStart).count();
    double pressureMs = std::chrono::duration<double, std::milli>(pressureEnd - pressureStart).count();
    double viscosityMs = std::chrono::duration<double, std::milli>(viscosityEnd - viscosityStart).count();
    double collisionsMs = std::chrono::duration<double, std::milli>(collisionsEnd - collisionsStart).count();
    std::cout << "Density: " << densityMs << " ms, Pressure: " << pressureMs << " ms, Viscosity: " << viscosityMs << " ms, Integration and collisions: " << collisionsMs << " ms\n";

}
