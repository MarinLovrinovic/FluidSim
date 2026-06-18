#include "TriangleMesh.h"
#include "Renderable.h"

#include <utility>

TriangleMesh::TriangleMesh(vector<glm::vec3> vertices, vector<glm::vec3> normals, vector<int> indices, vector<glm::vec3> uvCoords) :
        vertices(std::move(vertices)),
        normals(std::move(normals)),
        indices(std::move(indices)),
        uvCoords(std::move(uvCoords)) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &verticesVBO);
    glGenBuffers(1, &normalsVBO);
    glGenBuffers(1, &uvCoordsVBO);
    glGenBuffers(1, &EBO);
}

void TriangleMesh::SendToGpu() {
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normals.size(), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, uvCoordsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * uvCoords.size(), &uvCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void TriangleMesh::Bind() {
    glBindVertexArray(VAO);
}

void TriangleMesh::Draw() {
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
}

TriangleMesh::~TriangleMesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &verticesVBO);
    glDeleteBuffers(1, &normalsVBO);
    glDeleteBuffers(1, &EBO);
}

pair<glm::vec3, glm::vec3> TriangleMesh::GetBoundingBox() {
    glm::vec3 min = vertices[0];
    glm::vec3 max = vertices[1];
    for (auto vertex : vertices) {
        min.x = std::min(min.x, vertex.x);
        min.y = std::min(min.y, vertex.y);
        min.z = std::min(min.z, vertex.z);
        max.x = std::max(max.x, vertex.x);
        max.y = std::max(max.y, vertex.y);
        max.z = std::max(max.z, vertex.z);
    }
    return {min, max};
}


void TriangleMesh::ApplyTransform(glm::mat4 transform) {
    for (glm::vec3& vertex : vertices) {
        vertex = transform * glm::vec4(vertex, 1);
    }
}

void TriangleMesh::Normalize() {
    auto boundingBox = GetBoundingBox();
    auto minCorner = boundingBox.first;
    auto maxCorner = boundingBox.second;

    auto center = (minCorner + maxCorner) / 2.0f;
    auto translate = glm::mat4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            -center.x, -center.y, -center.z, 1
    );

    auto diff = maxCorner - minCorner;
    float maxDiff = max(max(diff.x, diff.y), diff.z);
    float scaleFactor = 2.0f / maxDiff;
    auto scale = glm::mat4(
            scaleFactor, 0,           0,           0,
            0,           scaleFactor, 0,           0,
            0,           0,           scaleFactor, 0,
            0,           0,           0,           1
    );

    ApplyTransform(scale * translate);
}
