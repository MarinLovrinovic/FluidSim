#include "Fluid.h"

#include <chrono>
#include <iostream>
#include <random>
#include <limits>
#include <optional>
#include <execution>
#include <algorithm>

#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/ext/scalar_constants.hpp"
#include <glm/gtc/random.hpp>

constexpr auto executionPolicy = std::execution::par;

float Fluid::SmoothingKernelFlat(const float radius, const float distance) const
{
    if (distance >= radius) return 0;

    const float value = radius * radius - distance * distance;
    return value * value * value * kernelScalingFactorFlat;
}

float Fluid::SmoothingKernelSpikyPow2(const float radius, const float distance) const
{
    if (distance >= radius) return 0;

    const float value = radius - distance;
    return value * value * kernelScalingFactorSpikyPow2;
}

float Fluid::SmoothingKernelSpikyPow2Derivative(const float radius, const float distance) const
{
    if (distance >= radius) return 0;

    const float value = radius - distance;
    return -value * kernelScalingFactorSpikyPow2Derivative;
}

float Fluid::SmoothingKernelSpikyPow3(const float radius, const float distance) const
{
    if (distance >= radius) return 0;

    const float value = radius - distance;
    return value * value * value * kernelScalingFactorSpikyPow3;
}

float Fluid::SmoothingKernelSpikyPow3Derivative(const float radius, const float distance) const
{
    if (distance >= radius) return 0;

    const float value = radius - distance;
    return -value * value * kernelScalingFactorSpikyPow3Derivative;
}

glm::ivec3 Fluid::PositionToCellCoord(const glm::vec3 point, const float radius)
{
    const int cellX = static_cast<int>(point.x / radius);
    const int cellY = static_cast<int>(point.y / radius);
    const int cellZ = static_cast<int>(point.z / radius);
    return {cellX, cellY, cellZ};
}

unsigned int Fluid::HashCell(const glm::ivec3 cell)
{
    constexpr unsigned int blockSize = 50;
    const glm::uvec3 ucell = cell + glm::ivec3(blockSize / 2);

    const glm::uvec3 localCell = ucell % blockSize;
    const glm::uvec3 blockID = ucell / blockSize;
    const unsigned int blockHash = blockID.x * 15823 + blockID.y * 9737333 + blockID.z * 440817757;
    return localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash;
}

Fluid::Fluid(
    const glm::ivec3 particleGridDimensions,
    const float smoothingRadius,
    const float targetDensity,
    const float pressureMultiplier,
    const float nearPressureMultiplier,
    const float viscosityStrength,
    const glm::vec3 startingPosition,
    const float particleSize,
    const float particleSpacing,
    const glm::vec3 corner1,
    const glm::vec3 corner2,
    Object* object) :
particleCount(particleGridDimensions.x * particleGridDimensions.y * particleGridDimensions.z),
smoothingRadius(smoothingRadius),
targetDensity(targetDensity),
pressureMultiplier(pressureMultiplier),
nearPressureMultiplier(nearPressureMultiplier),
viscosityStrength(viscosityStrength),
object(object),
corner1(corner1),
corner2(corner2)
{
    previousPositions.resize(particleCount);
    currentPositions.resize(particleCount);
    predictedPositions.resize(particleCount);
    particleIndices.resize(particleCount);
    velocities.resize(particleCount);
    forces.resize(particleCount);
    densities.resize(particleCount);
    spatialLookup.resize(particleCount);
    startIndices.resize(particleCount);
    object->transforms.resize(particleCount);

    for (int x = 0; x < particleGridDimensions.x; ++x)
    {
        for (int y = 0; y < particleGridDimensions.y; ++y)
        {
            for (int z = 0; z < particleGridDimensions.z; ++z)
            {
                const int index = z * particleGridDimensions.x * particleGridDimensions.y + y * particleGridDimensions.x + x;
                const glm::vec3 particlePosition = startingPosition + glm::vec3(particleSpacing * x, particleSpacing * y, particleSpacing * z);
                previousPositions[index] = particlePosition;
                currentPositions[index] = particlePosition;
            }
        }
    }

    for (int i = 0; i < particleCount; i++)
    {
        object->transforms[i].SetPosition(currentPositions[i]);
        object->transforms[i].SetScale(glm::vec3(particleSize));
        particleIndices[i] = i;
    }

    // pre-calculate kernel scaling factors
    kernelScalingFactorFlat = 4 / (pi * glm::pow(smoothingRadius, 8));
    kernelScalingFactorSpikyPow3 = 10 / (pi * glm::pow(smoothingRadius, 5));
    kernelScalingFactorSpikyPow2 = 6 / (pi * glm::pow(smoothingRadius, 4));
    kernelScalingFactorSpikyPow3Derivative = 30 / (pi * glm::pow(smoothingRadius, 5));
    kernelScalingFactorSpikyPow2Derivative = 12 / (pi * glm::pow(smoothingRadius, 4));
}

glm::vec2 Fluid::CalculateDensities(const glm::vec3 samplePoint) const
{
    float density = 0;
    float nearDensity = 0;

    ForeachPointWithinRadius(samplePoint, [&](const int pointIndex)
    {
        const glm::vec3 position = predictedPositions[pointIndex];
        const float distance = glm::length(position - samplePoint);
        density += SmoothingKernelSpikyPow2(smoothingRadius, distance);
        nearDensity += SmoothingKernelSpikyPow3(smoothingRadius, distance);
    });
    return {density, nearDensity};
}

float Fluid::DensityToPressure(const float density) const
{
    const float densityError = density - targetDensity;
    return densityError * pressureMultiplier;
}

float Fluid::NearDensityToNearPressure(const float nearDensity) const
{
    return nearDensity * nearPressureMultiplier;
}

glm::vec3 Fluid::CalculatePressureForce(const int particleIndex) const
{
    glm::vec3 pressureForce(0);
    const float density = densities[particleIndex].x;
    const float nearDensity = densities[particleIndex].y;
    const float pressure = DensityToPressure(density);
    const float nearPressure = NearDensityToNearPressure(nearDensity);

    const glm::vec3 particlePosition = predictedPositions[particleIndex];
    ForeachPointWithinRadius(particlePosition, [&](const int otherParticleIndex)
    {
        if (otherParticleIndex != particleIndex)
        {
            const glm::vec3 offset = predictedPositions[otherParticleIndex] - particlePosition;
            const float distance = glm::length(offset);
            // pick a random direction in case two particles share the same position
            const glm::vec3 direction = distance == 0 ? glm::sphericalRand(1.0f) : offset / distance;
            const float otherDensity = densities[otherParticleIndex].x;
            const float otherNearDensity = densities[otherParticleIndex].y;
            const float otherPressure = DensityToPressure(otherDensity);
            const float otherNearPressure = NearDensityToNearPressure(otherNearDensity);

            const float sharedPressure = (pressure + otherPressure) * 0.5f;
            const float sharedNearPressure = (nearPressure + otherNearPressure) * 0.5f;
            pressureForce += sharedPressure * direction
                * SmoothingKernelSpikyPow2Derivative(smoothingRadius, distance) / otherDensity;
            pressureForce += sharedNearPressure * direction
                * SmoothingKernelSpikyPow3Derivative(smoothingRadius, distance) / otherNearDensity;
        }
    });
    return pressureForce;
}

glm::vec3 Fluid::CalculateViscosityForce(const int particleIndex) const
{
    glm::vec3 viscosityForce(0);
    const glm::vec3 particlePosition = predictedPositions[particleIndex];

    ForeachPointWithinRadius(particlePosition, [&](const int otherParticleIndex)
    {
        if (otherParticleIndex != particleIndex)
        {
            const float distance = glm::length(particlePosition - predictedPositions[otherParticleIndex]);
            const float influence = SmoothingKernelFlat(smoothingRadius, distance);
            viscosityForce += (velocities[otherParticleIndex] - velocities[particleIndex]) * influence;
        }
    });
    return viscosityForce * viscosityStrength;
}

glm::vec3 Fluid::CalculateInteractionForce(const glm::vec3 inputPos, const float radius, const float strength, const int particleIndex) const
{
    glm::vec3 interactionForce = glm::vec3(0);
    const glm::vec3 offset = inputPos - currentPositions[particleIndex];
    const float sqrDst = glm::dot(offset, offset);

    if (sqrDst < radius * radius)
    {
        const float dst = glm::sqrt(sqrDst);
        const glm::vec3 dirToInputPoint = dst <= glm::epsilon<float>() ? glm::vec3(0) : offset / dst;
        // influence is 1 when particle is at input point, 0 when at the edge of input radius
        const float influence = 1 - dst / radius;
        // velocity is subtracted to slow the particle down
        interactionForce += (dirToInputPoint * strength - velocities[particleIndex]) * influence;
    }
    return interactionForce;
}

unsigned int Fluid::GetKeyFromHash(unsigned int hash) const
{
    return hash % static_cast<unsigned int>(particleCount);
}

void Fluid::UpdateSpatialLookup()
{
    // create unordered spatial lookup
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        const glm::ivec3 cell = PositionToCellCoord(predictedPositions[i], smoothingRadius);
        const unsigned int cellKey = GetKeyFromHash(HashCell(cell));
        spatialLookup[i] = SpatialLookupEntry {
            i,
            cellKey
        };
        startIndices[i] = std::numeric_limits<int>::max();
    });

    // sort by cell key
    std::sort(executionPolicy, spatialLookup.begin(), spatialLookup.end());

    // calculate start indices of each unique cell key in the spatial lookup
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        const unsigned int key = spatialLookup[i].cellKey;
        if (i == 0 || spatialLookup[i - 1].cellKey != key)
        {
            startIndices[key] = i;
        }
    });
}

void Fluid::Update(const float dt, const glm::vec3 gravity, const optional<glm::vec4> interaction)
{
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        forces[i] = gravity;
        predictedPositions[i] = currentPositions[i] + currentPositions[i] - previousPositions[i];
    });

    if (interaction.has_value())
    {
        for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
        {
            forces[i] += CalculateInteractionForce(interaction.value(), 2, 200 * interaction.value().w, i);
        });
    }

    auto spatialLookupStart = std::chrono::high_resolution_clock::now();
    UpdateSpatialLookup();
    auto spatialLookupEnd = std::chrono::high_resolution_clock::now();

    auto densityStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        densities[i] = CalculateDensities(predictedPositions[i]);
    });
    auto densityEnd = std::chrono::high_resolution_clock::now();

    auto forcesStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        forces[i] += CalculatePressureForce(i);
    });
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        forces[i] += CalculateViscosityForce(i);
    });
    auto forcesEnd = std::chrono::high_resolution_clock::now();

    auto collisionsStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        glm::vec3 acceleration = forces[i] / densities[i].x;
        glm::vec3 currentPosition = currentPositions[i];
        glm::vec3 nextPosition = 2.0f * currentPosition - previousPositions[i] + acceleration * dt * dt;


        // calculate collisions
        constexpr bool bounce = true;
        const float bounceFactor = 0.05f;
        const glm::vec3 minPos = glm::vec3(min(corner1.x, corner2.x), min(corner1.y, corner2.y), min(corner1.z, corner2.z));
        const glm::vec3 maxPos = glm::vec3(max(corner1.x, corner2.x), max(corner1.y, corner2.y), max(corner1.z, corner2.z));
        if (bounce)
        {
            if (nextPosition.x < minPos.x)
            {
                nextPosition = Mirror(nextPosition, minPos, glm::vec3(1, 0, 0));
                currentPosition = Mirror(currentPosition, minPos, glm::vec3(1, 0, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            } else if (maxPos.x < nextPosition.x)
            {
                nextPosition = Mirror(nextPosition, maxPos, glm::vec3(-1, 0, 0));
                currentPosition = Mirror(currentPosition, maxPos, glm::vec3(-1, 0, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            }
            if (nextPosition.y < minPos.y)
            {
                nextPosition = Mirror(nextPosition, minPos, glm::vec3(0, 1, 0));
                currentPosition = Mirror(currentPosition, minPos, glm::vec3(0, 1, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            } else if (maxPos.y < nextPosition.y)
            {
                nextPosition = Mirror(nextPosition, maxPos, glm::vec3(0, -1, 0));
                currentPosition = Mirror(currentPosition, maxPos, glm::vec3(0, -1, 0));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            }
            if (nextPosition.z < minPos.z)
            {
                nextPosition = Mirror(nextPosition, minPos, glm::vec3(0, 0, 1));
                currentPosition = Mirror(currentPosition, minPos, glm::vec3(0, 0, 1));
                currentPosition += (nextPosition - currentPosition) * (1 - bounceFactor);
            } else if (maxPos.z < nextPosition.z)
            {
                nextPosition = Mirror(nextPosition, maxPos, glm::vec3(0, 0, -1));
                currentPosition = Mirror(currentPosition, maxPos, glm::vec3(0, 0, -1));
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
        object->transforms[i].SetPosition(currentPositions[i]);
    });
    auto collisionsEnd = std::chrono::high_resolution_clock::now();

    double spatialLookupMs = std::chrono::duration<double, std::milli>(spatialLookupEnd - spatialLookupStart).count();
    double densityMs = std::chrono::duration<double, std::milli>(densityEnd - densityStart).count();
    double forcesMs = std::chrono::duration<double, std::milli>(forcesEnd - forcesStart).count();
    double collisionsMs = std::chrono::duration<double, std::milli>(collisionsEnd - collisionsStart).count();
    std::cout << "Spatial lookup: " << spatialLookupMs << " ms, Density: " << densityMs << " ms, Forces: " << forcesMs << " ms, Integration and collisions: " << collisionsMs << " ms\n";
}
