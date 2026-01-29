#ifndef IRGLAB_TRIANGLEMESH_H
#define IRGLAB_TRIANGLEMESH_H

#include "glm/glm.hpp"
#include "glad/glad.h"
#include "Renderable.h"

#include <vector>

using namespace std;

class TriangleMesh : public Renderable {
private:
    GLuint verticesVBO;
    GLuint normalsVBO;
    GLuint uvCoordsVBO;
    GLuint EBO;
    GLuint VAO;
    pair<glm::vec3, glm::vec3> GetBoundingBox();
public:
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    vector<int> indices;
    vector<glm::vec3> uvCoords;
    TriangleMesh(vector<glm::vec3> vertices, vector<glm::vec3> normals, vector<int> indices, vector<glm::vec3> uvCoords);
    ~TriangleMesh() override;
    void SendToGpu() override;
    void Draw() override;
    void Bind() override;
    void ApplyTransform(glm::mat4 transform);
    void Normalize();
};

#endif //IRGLAB_TRIANGLEMESH_H
