//
// Created by lovri on 2/2/2025.
//

#ifndef CLOTHSIM_SOFTBODY_H
#define CLOTHSIM_SOFTBODY_H

#include "TriangleMesh.h"
#include "glm/vec3.hpp"
#include "Particle.h"
#include "Spring.h"
#include "Collider.h"
#include "Object.h"
#include <vector>

using namespace std;

class SoftBody {
private:
    vector<Particle> particles;
    vector<Spring> springs;

    TriangleMesh* mesh;
    vector<int> vertexToParticleMap;
    void UpdateMesh();
public:
    Object* object;

    explicit SoftBody(TriangleMesh* mesh, Shader* shader, glm::vec3 initialPosition, glm::vec3 initialVelocity,
                      float dt, float springConstant, float springDampingCoefficient);
    void Update(float dt, glm::vec3 gravity, glm::vec3 airflow, const vector<Collider*>& colliders);
};

#endif //CLOTHSIM_SOFTBODY_H
