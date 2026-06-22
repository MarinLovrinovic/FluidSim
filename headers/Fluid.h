#pragma once

#include <vector>
#include <optional>

#include "Object.h"

#include "glm/vec2.hpp"


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
    vector<glm::vec2> previousPositions;
    vector<glm::vec2> currentPositions;
    vector<glm::vec2> predictedPositions;
    vector<int> particleIndices;
    vector<glm::vec2> velocities;
    vector<glm::vec2> forces;
    vector<glm::vec2> densities;
    vector<SpatialLookupEntry> spatialLookup;
    vector<int> startIndices;
    Object* object;
    glm::vec2 corner1, corner2;
public:
    Fluid(int particlesX, int particlesY, float smoothingRadius, float targetDensity, float pressureMultiplier, float nearPressureMultiplier, float viscosityStrength, glm::vec2 startingPosition, float particleSize, float particleSpacing, glm::vec2 corner1, glm::vec2 corner2, Object* object);
    [[nodiscard]] glm::vec2 CalculateDensities(glm::vec2 samplePoint) const;
    [[nodiscard]] float DensityToPressure(float density) const;
    [[nodiscard]] float NearDensityToNearPressure(float nearDensity) const;
    [[nodiscard]] float CalculateSharedPressure(float densityA, float densityB) const;
    [[nodiscard]] glm::vec2 CalculatePressureForce(int particleIndex) const;
    [[nodiscard]] glm::vec2 CalculateViscosityForce(int particleIndex) const;
    [[nodiscard]] glm::vec2 CalculateInteractionForce(glm::vec2 inputPos, float radius, float strength, int particleIndex) const;
    static glm::vec<2, int> PositionToCellCoord(glm::vec2 point, float radius);
    static unsigned int Fluid::HashCell(int cellX, int cellY);
    [[nodiscard]] unsigned int GetKeyFromHash(unsigned int hash) const;
    void UpdateSpatialLookup();
    template<typename F>
    void ForeachPointWithinRadius(glm::vec2 samplePoint, F&& f) const;
    void Update(float dt, glm::vec2 gravity, optional<glm::vec3> interaction);
};


template <typename F>
void Fluid::ForeachPointWithinRadius(const glm::vec2 samplePoint, F&& f) const
{
    const glm::vec<2, int> center = PositionToCellCoord(samplePoint, smoothingRadius);
    const float sqrRadius = smoothingRadius * smoothingRadius;
    for (int offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            const unsigned int key = GetKeyFromHash(HashCell(center.x + offsetX, center.y + offsetY));
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
