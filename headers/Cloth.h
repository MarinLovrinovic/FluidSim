
#ifndef CLOTHSIM_CLOTH_H
#define CLOTHSIM_CLOTH_H

#include <vector>
#include <map>
#include "glm/vec3.hpp"
#include "Renderable.h"
#include "TriangleMesh.h"
#include "Object.h"
#include "SphereCollider.h"

using namespace std;

class Cloth {
private:
    int dim;
    vector<vector<glm::vec3>> previousPositions;
    vector<vector<glm::vec3>> currentPositions;
    vector<vector<glm::vec3>> nextPositions;
    vector<vector<glm::vec3>> currentNormals;
    vector<vector<glm::vec3>> currentVelocities;
    vector<vector<glm::vec4>> colliderSurfaces;
    vector<vector<bool>> fixed;
    float springConstant;
    float springDampingCoefficient;
    float dragCoefficient;

    float equilibriumDistance;
    vector<glm::ivec2> neighborOffsets;
    map<int, map<int, float>> neighborDistances;

    float timeElapsed;

    TriangleMesh* mesh;
public:
    Object* object;

    Cloth(int dimension, float springConstant, float springDampingCoefficient, float dragCoefficient, Shader* shader,
          glm::vec3 position, float sideLength, bool vertical, vector<glm::ivec2> fixed);
    void Update(float dt, glm::vec3 gravity, glm::vec3 airflow, const vector<Collider*>& colliders);
};

#endif //CLOTHSIM_CLOTH_H
