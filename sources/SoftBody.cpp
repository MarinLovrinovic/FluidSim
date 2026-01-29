//
// Created by lovri on 2/2/2025.
//
#include "SoftBody.h"
#include <map>
#include <set>
#include <random>

SoftBody::SoftBody(TriangleMesh *mesh, Shader* shader, glm::vec3 initialPosition, glm::vec3 initialVelocity, float dt, float springConstant, float springDampingCoefficient) : mesh(mesh) {

    glm::vec3 previousPosition = initialPosition - initialVelocity * dt;

    vector<glm::vec3> uniqueVertexPositions;
    int particleIndex = 0;
    for (auto vertex : mesh->vertices) {
        bool found = false;
        for (int i = 0; i < uniqueVertexPositions.size(); i++) {
            if (vertex == uniqueVertexPositions[i]) {
                vertexToParticleMap.emplace_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            uniqueVertexPositions.push_back(vertex);
            particles.emplace_back(vertex + initialPosition, vertex + previousPosition);
            vertexToParticleMap.emplace_back(particleIndex);
            particleIndex++;
        }
    }

    vector<int> remappedIndices;
    for (int index : mesh->indices) {
        remappedIndices.push_back(vertexToParticleMap[index]);
    }
    map<int, vector<int>> neighborMap;
    for (int i = 0; i < particles.size(); i++) {
        neighborMap[i] = vector<int>();
    }

    set<pair<int, int>> edges;
    for (int i = 0; i < remappedIndices.size(); i += 3) {
        int i1 = remappedIndices[i];
        int i2 = remappedIndices[i + 1];
        int i3 = remappedIndices[i + 2];

        // insert all edges (smaller index first)
        if (i1 < i2)
            edges.emplace(i1, i2);
        else
            edges.emplace(i2, i1);
        if (i2 < i3)
            edges.emplace(i2, i3);
        else
            edges.emplace(i3, i2);
        if (i3 < i1)
            edges.emplace(i3, i1);
        else
            edges.emplace(i1, i3);

        neighborMap[i1].push_back(i2);
        neighborMap[i2].push_back(i3);
        neighborMap[i3].push_back(i1);
    }

    // add connections to second neighbors
    for (int i = 0; i < particles.size(); i++) {
        for (auto neighbor : neighborMap[i]) {
            for (auto secondNeighbor : neighborMap[neighbor]) {
                if (secondNeighbor == i) continue;

                if (i < secondNeighbor)
                    edges.emplace(i, secondNeighbor);
                else
                    edges.emplace(secondNeighbor, i);
            }
        }
    }

    // add random connections
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, particles.size() - 1);
    for (int i = 0; i < particles.size(); i++) {
        for (int j = 0; j < 3; j++) {
            int randomIndex = dist(gen);
            if (randomIndex == i) {
                j--;
                continue;
            }
            if (i < randomIndex)
                edges.emplace(i, randomIndex);
            else
                edges.emplace(randomIndex, i);
        }
    }

    for (auto edge : edges) {
        springs.emplace_back(&particles[edge.first], &particles[edge.second], springConstant, springDampingCoefficient);
    }

    mesh->SendToGpu();
    object = new Object(mesh, shader);
}

void SoftBody::Update(float dt, glm::vec3 gravity, glm::vec3 airflow, const vector<Collider *> &colliders) {
    for (auto& particle : particles) {
        particle.force = gravity;
    }

    for (auto& spring : springs) {
        Particle* p1 = spring.a;
        Particle* p2 = spring.b;
        glm::vec3 pos1 = p1->currentPosition;
        glm::vec3 pos2 = p2->currentPosition;
        glm::vec3 v1to2 = pos2 - pos1;

        float length = glm::length(v1to2);
        float dx = length - spring.equilibriumLength;

        glm::vec3 dir1to2 = v1to2 / length;
        p1->force += spring.springConstant * dx * dir1to2;
        p2->force += -spring.springConstant * dx * dir1to2;
    }

    for (auto& particle : particles) {
        glm::vec3 acceleration = particle.force / particle.mass;
        glm::vec3 nextPosition = 2.0f * particle.currentPosition - particle.previousPosition + acceleration * dt * dt;
        // calculate collisions
        for (Collider *const collider : colliders) {
            collider->Displace(particle.currentPosition, nextPosition);
        }
        particle.previousPosition = particle.currentPosition;
        particle.currentPosition = nextPosition;
    }

    UpdateMesh();
}

void SoftBody::UpdateMesh() {
    for (int i = 0; i < mesh->vertices.size(); i++) {
        mesh->vertices[i] = particles[vertexToParticleMap[i]].currentPosition;
    }
    mesh->SendToGpu();
}









// layering
//        int particleIndexOffset = particleIndex;
// intrude a new particle for each particle and connect it to the original
//    int particleCount = particles.size();
//    for (int i = 0; i < particleCount; i++) {
//        Particle* original = &particles[i];
//        Particle copy = Particle(original->currentPosition * 0.5f);
//        particles.push_back(copy);
//        springs.emplace_back(original, &particles[particleIndex]);
//        particleIndex++;
//    }
////
////    // connect the intruded particles together
//    for (int& index : remappedIndices) {
//        index += particleIndexOffset;
//    }
//    for (int i = 0; i < remappedIndices.size(); i += 3) {
//        int i1 = remappedIndices[i];
//        int i2 = remappedIndices[i + 1];
//        int i3 = remappedIndices[i + 2];
//
//        springs.emplace_back(&particles[i1], &particles[i2]);
//        springs.emplace_back(&particles[i2], &particles[i3]);
//        springs.emplace_back(&particles[i3], &particles[i1]);
//    }