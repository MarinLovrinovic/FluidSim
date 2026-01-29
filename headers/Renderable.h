#ifndef IRGLAB_RENDERABLE_H
#define IRGLAB_RENDERABLE_H

#include "glm/glm.hpp"
#include "glad/glad.h"

#include <vector>


class Renderable {
public:
    virtual void SendToGpu() = 0;
    virtual void Draw() = 0;
    virtual void Bind() = 0;
    virtual ~Renderable() = default;
};


#endif //IRGLAB_RENDERABLE_H
