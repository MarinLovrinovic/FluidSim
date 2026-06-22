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
        unsigned int cellKey;
        glm::vec2 previousPosition;
        glm::vec2 currentPosition;
        glm::vec2 predictedPosition;
        glm::vec2 velocity;
        glm::vec2 force;
        float density;
        Transform* transform;
        bool operator<(const SpatialLookupEntry& other) const
        {
            return cellKey < other.cellKey;
        }
    };

    int particleCount;
    float smoothingRadius;
    float targetDensity;
    float pressureMultiplier;
    float viscosityStrength;
    vector<int> particleIndices;
    vector<SpatialLookupEntry> spatialLookup;
    vector<int> startIndices;
    Object* object;
    glm::vec2 corner1, corner2;
public:
    Fluid(int particlesX, int particlesY, float smoothingRadius, float targetDensity, float pressureMultiplier, float viscosityStrength, glm::vec2 startingPosition, float particleSize, float particleSpacing, glm::vec2 corner1, glm::vec2 corner2, Object* object);
    void CalculateDensity(SpatialLookupEntry& particle) const;
    [[nodiscard]] float ConvertDensityToPressure(float density) const;
    [[nodiscard]] float CalculateSharedPressure(float densityA, float densityB) const;
    void CalculatePressureForce(SpatialLookupEntry& particle) const;
    void CalculateViscosityForce(SpatialLookupEntry& particle) const;
    void CalculateInteractionForce(glm::vec2 inputPos, float radius, float strength, SpatialLookupEntry& particle) const;
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
                const SpatialLookupEntry& spatialLookupEntry = spatialLookup[i];
                // exit if we're no longer at the correct cell
                if (spatialLookupEntry.cellKey != key) break;

                const auto offset = spatialLookupEntry.previousPosition - samplePoint;
                const float sqrDistance = glm::dot(offset, offset);

                // test if the point is inside the radius
                if (sqrDistance <= sqrRadius)
                {
                    std::forward<F>(f)(spatialLookupEntry);
                }
            }
        }
    }
}
