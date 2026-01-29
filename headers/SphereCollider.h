
#ifndef CLOTHSIM_SPHERECOLLIDER_H
#define CLOTHSIM_SPHERECOLLIDER_H

#include "glm/vec3.hpp"
#include "Collider.h"
#include "glm/vec4.hpp"

class SphereCollider : public Collider {
public:
    glm::vec3 position;
    float radius;
    float frictionCoefficient;
    SphereCollider(glm::vec3 position, float radius, float frictionCoefficient);
    glm::vec4 Displace(glm::vec3 particlePosition, glm::vec3& particleNextPosition) const override;
};

#endif //CLOTHSIM_SPHERECOLLIDER_H
