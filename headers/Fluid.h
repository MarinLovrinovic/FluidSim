#pragma once

#include <vector>
#include <optional>

#include "Object.h"

#include "glm/vec2.hpp"
#include "glm/ext/scalar_constants.hpp"


using namespace std;

class Fluid
{
private:
    struct SpatialLookupEntry
    {
        int particleIndex;
        unsigned int cellKey;
        bool operator<(const SpatialLookupEntry& other) const
        {
            return cellKey < other.cellKey;
        }
    };

    int particleCount;
    float smoothingRadius;
    float targetDensity;
    float pressureMultiplier;
    float nearPressureMultiplier;
    float viscosityStrength;
    vector<glm::vec3> previousPositions;
    vector<glm::vec3> currentPositions;
    vector<glm::vec3> predictedPositions;
    vector<int> particleIndices;
    vector<glm::vec3> velocities;
    vector<glm::vec3> forces;
    vector<glm::vec2> densities;
    vector<SpatialLookupEntry> spatialLookup;
    vector<int> startIndices;
    Object* object;
    glm::vec3 corner1, corner2;

    float SmoothingKernelFlat(float radius, float distance) const;
    float SmoothingKernelSpikyPow2(float radius, float distance) const;
    float SmoothingKernelSpikyPow2Derivative(float radius, float distance) const;
    float SmoothingKernelSpikyPow3(float radius, float distance) const;
    float SmoothingKernelSpikyPow3Derivative(float radius, float distance) const;

    float kernelScalingFactorFlat;
    float kernelScalingFactorSpikyPow2;
    float kernelScalingFactorSpikyPow2Derivative;
    float kernelScalingFactorSpikyPow3;
    float kernelScalingFactorSpikyPow3Derivative;
    float pi = glm::pi<float>();
public:
    Fluid(
        glm::ivec3 particleGridDimensions,
        float smoothingRadius,
        float targetDensity,
        float pressureMultiplier,
        float nearPressureMultiplier,
        float viscosityStrength,
        glm::vec3 startingPosition,
        float particleSize,
        float particleSpacing,
        glm::vec3 corner1,
        glm::vec3 corner2,
        Object* object);
    [[nodiscard]] glm::vec2 CalculateDensities(glm::vec3 samplePoint) const;
    [[nodiscard]] float DensityToPressure(float density) const;
    [[nodiscard]] float NearDensityToNearPressure(float nearDensity) const;
    [[nodiscard]] glm::vec3 CalculatePressureForce(int particleIndex) const;
    [[nodiscard]] glm::vec3 CalculateViscosityForce(int particleIndex) const;
    [[nodiscard]] glm::vec3 CalculateInteractionForce(glm::vec3 inputPos, float radius, float strength, int particleIndex) const;
    static glm::ivec3 PositionToCellCoord(glm::vec3 point, float radius);
    static unsigned int Fluid::HashCell(glm::ivec3 cell);
    [[nodiscard]] unsigned int GetKeyFromHash(unsigned int hash) const;
    void UpdateSpatialLookup();
    template<typename F>
    void ForeachPointWithinRadius(glm::vec3 samplePoint, F&& f) const;
    void Update(float dt, glm::vec3 gravity, optional<glm::vec4> interaction);
};


template <typename F>
void Fluid::ForeachPointWithinRadius(const glm::vec3 samplePoint, F&& f) const
{
    const glm::ivec3 center = PositionToCellCoord(samplePoint, smoothingRadius);
    const float sqrRadius = smoothingRadius * smoothingRadius;
    for (int offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetZ = -1; offsetZ <= 1; offsetZ++)
            {
                glm::ivec3 cell = {center.x + offsetX, center.y + offsetY, center.z + offsetZ};
                const unsigned int key = GetKeyFromHash(HashCell(cell));
                const int cellStartIndex = startIndices[key];

                for (int i = cellStartIndex; i < particleCount; i++)
                {
                    const SpatialLookupEntry spatialLookupEntry = spatialLookup[i];
                    // exit if we're no longer at the correct cell
                    if (spatialLookupEntry.cellKey != key) break;

                    const int particleIndex = spatialLookupEntry.particleIndex;
                    const auto offset = predictedPositions[particleIndex] - samplePoint;
                    const float sqrDistance = glm::dot(offset, offset);

                    // test if the point is inside the radius
                    if (sqrDistance <= sqrRadius)
                    {
                        std::forward<F>(f)(particleIndex);
                    }
                }
            }
        }
    }
}
