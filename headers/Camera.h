#ifndef IRGLAB_CAMERA_H
#define IRGLAB_CAMERA_H

#include "Transform.h"

class Camera : public Transform {
public:
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewMatrix() const;
};

#endif //IRGLAB_CAMERA_H
