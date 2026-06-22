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

float pi = glm::pi<float>();
constexpr auto executionPolicy = std::execution::par;

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
    const float particleSpacing,
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
    spatialLookup.resize(particleCount);
    particleIndices.resize(particleCount);
    startIndices.resize(particleCount);
    object->transforms.resize(particleCount);

    for (int x = 0; x < particlesX; ++x)
    {
        for (int y = 0; y < particlesY; ++y)
        {
            int index = y * particlesX + x;
            glm::vec2 particlePosition = startingPosition + glm::vec2(particleSpacing * x, particleSpacing * y);
            spatialLookup[index].previousPosition = particlePosition;
            spatialLookup[index].currentPosition = particlePosition;
            spatialLookup[index].transform = &object->transforms[index];
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
        object->transforms[i].SetPosition(glm::vec3(spatialLookup[i].currentPosition, 0));
        object->transforms[i].SetScale(glm::vec3(particleSize));
        particleIndices[i] = i;
    }
}

void Fluid::CalculateDensity(SpatialLookupEntry& particle) const
{
    float density = 0;
    constexpr float mass = 1;

    ForeachPointWithinRadius(particle.predictedPosition, [&](const SpatialLookupEntry& otherParticle)
    {
        const glm::vec2 position = otherParticle.predictedPosition;
        const float distance = glm::length(position - particle.predictedPosition);
        const float influence = SmoothingKernel(smoothingRadius, distance);
        density += mass * influence;
    });
    particle.density = density;
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

void Fluid::CalculatePressureForce(SpatialLookupEntry& particle) const
{
    glm::vec2 pressureForce(0);
    constexpr float mass = 1;
    const float particleDensity = particle.density;

    const glm::vec2 particlePosition = particle.predictedPosition;
    ForeachPointWithinRadius(particlePosition, [&](const SpatialLookupEntry& otherParticle)
    {
        if (&otherParticle != &particle)
        {
            const glm::vec2 offset = otherParticle.predictedPosition - particlePosition;
            const float distance = glm::length(offset);
            // pick a random direction in case two particles share the same position
            const glm::vec2 direction = distance == 0 ? glm::circularRand(1.0f) : offset / distance;
            const float slope = SmoothingKernelDerivative(smoothingRadius, distance);
            const float otherParticleDensity = otherParticle.density;
            const float sharedPressure = CalculateSharedPressure(particleDensity, otherParticleDensity);
            pressureForce += sharedPressure * direction * slope * mass / otherParticleDensity;
        }
    });
    particle.force += pressureForce;
}

void Fluid::CalculateViscosityForce(SpatialLookupEntry& particle) const
{
    glm::vec2 viscosityForce(0);
    const glm::vec2 particlePosition = particle.predictedPosition;

    ForeachPointWithinRadius(particlePosition, [&](const SpatialLookupEntry& otherParticle)
    {
        if (&otherParticle != &particle)
        {
            const float distance = glm::length(particlePosition - otherParticle.predictedPosition);
            const float influence = SmoothingKernelFlat(smoothingRadius, distance);
            viscosityForce += (otherParticle.velocity - particle.velocity) * influence;
        }
    });
    particle.force += viscosityForce * viscosityStrength;
}

void Fluid::CalculateInteractionForce(const glm::vec2 inputPos, const float radius, const float strength, SpatialLookupEntry& particle) const
{
    glm::vec2 interactionForce = glm::vec2(0);
    const glm::vec2 offset = inputPos - particle.currentPosition;
    const float sqrDst = glm::dot(offset, offset);

    if (sqrDst < radius * radius)
    {
        const float dst = glm::sqrt(sqrDst);
        const glm::vec2 dirToInputPoint = dst <= glm::epsilon<float>() ? glm::vec2(0) : offset / dst;
        // influence is 1 when particle is at input point, 0 when at the edge of input radius
        const float influence = 1 - dst / radius;
        // velocity is subtracted to slow the particle down
        interactionForce += (dirToInputPoint * strength - particle.velocity) * influence;
    }
    particle.force += interactionForce;
}

unsigned int Fluid::GetKeyFromHash(unsigned int hash) const
{
    return hash % static_cast<unsigned int>(particleCount);
}

void Fluid::UpdateSpatialLookup()
{
    // create unordered spatial lookup
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        const glm::vec<2, int> cell = PositionToCellCoord(particle.predictedPosition, smoothingRadius);
        const unsigned int cellKey = GetKeyFromHash(HashCell(cell.x, cell.y));
        particle.cellKey = cellKey;
    });

    // sort by cell key
    std::sort(executionPolicy, spatialLookup.begin(), spatialLookup.end());

    // calculate start indices of each unique cell key in the spatial lookup
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        startIndices[i] = std::numeric_limits<int>::max();
    });
    for_each(executionPolicy, particleIndices.begin(), particleIndices.end(), [&](auto i)
    {
        const unsigned int key = spatialLookup[i].cellKey;
        if (i == 0 || spatialLookup[i - 1].cellKey != key)
        {
            startIndices[key] = i;
        }
    });
}

void Fluid::Update(const float dt, const glm::vec2 gravity, const optional<glm::vec3> interaction)
{
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        particle.force = gravity;
        particle.predictedPosition = particle.currentPosition + particle.currentPosition - particle.predictedPosition;
    });

    if (interaction.has_value())
    {
        for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
        {
            CalculateInteractionForce(interaction.value(), 2, 200 * interaction.value().z, particle);
        });
    }

    auto spatialLookupStart = std::chrono::high_resolution_clock::now();
    UpdateSpatialLookup();
    auto spatialLookupEnd = std::chrono::high_resolution_clock::now();

    auto densityStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        CalculateDensity(particle);
    });
    auto densityEnd = std::chrono::high_resolution_clock::now();

    auto forcesStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        CalculatePressureForce(particle);
    });
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        CalculateViscosityForce(particle);
    });
    auto forcesEnd = std::chrono::high_resolution_clock::now();

    auto collisionsStart = std::chrono::high_resolution_clock::now();
    for_each(executionPolicy, spatialLookup.begin(), spatialLookup.end(), [&](SpatialLookupEntry& particle)
    {
        glm::vec2 acceleration = particle.force / particle.density;
        glm::vec2 currentPosition = particle.currentPosition;
        glm::vec2 nextPosition = 2.0f * currentPosition - particle.previousPosition + acceleration * dt * dt;

        // calculate collisions
        constexpr bool bounce = true;
        const float bounceFactor = 0.05f;
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

        particle.previousPosition = currentPosition;
        particle.currentPosition = nextPosition;
        particle.velocity = (particle.currentPosition - particle.predictedPosition) / dt;
        particle.transform->SetPosition(glm::vec3(particle.currentPosition, 0));
    });
    auto collisionsEnd = std::chrono::high_resolution_clock::now();

    double spatialLookupMs = std::chrono::duration<double, std::milli>(spatialLookupEnd - spatialLookupStart).count();
    double densityMs = std::chrono::duration<double, std::milli>(densityEnd - densityStart).count();
    double forcesMs = std::chrono::duration<double, std::milli>(forcesEnd - forcesStart).count();
    double collisionsMs = std::chrono::duration<double, std::milli>(collisionsEnd - collisionsStart).count();
    std::cout << "Spatial lookup: " << spatialLookupMs << " ms, Density: " << densityMs << " ms, Forces: " << forcesMs << " ms, Integration and collisions: " << collisionsMs << " ms\n";
}
