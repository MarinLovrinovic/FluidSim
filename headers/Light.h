#ifndef IRGLAB_LIGHT_H
#define IRGLAB_LIGHT_H

#include "Transform.h"

class Light : public Transform {
public:
    glm::vec3 ambient = glm::vec3(1, 1, 1);
    glm::vec3 diffuse = glm::vec3(15, 15, 15);
    glm::vec3 specular = glm::vec3(10, 10, 10);
};

#endif //IRGLAB_LIGHT_H
