#ifndef IRGLAB_MATERIAL_H
#define IRGLAB_MATERIAL_H

#include "glm/glm.hpp"
#include "glad/glad.h"
#include "Shader.h"

class Material {
public:
    glm::vec3 ambient = glm::vec3(0.2, 0.2, 0.2);
    glm::vec3 diffuse = glm::vec3(1, 1, 1);
    glm::vec3 specular = glm::vec3(0.3, 0.3, 0.3);
    float shininess = 1;

    GLint materialAmbientLocation;
    GLint materialDiffuseLocation;
    GLint materialSpecularLocation;
    GLint materialShininessLocation;

    glm::vec3 backgroundColor = glm::vec3(0.3, 0.3, 0.3);
    glm::vec3 curveColor = glm::vec3(0.3, 1, 0.3);
    float curvature = 10;
    float curveThickness = 4;
    float curveFrequency = 0.5;

    GLint backgroundColorLocation;
    GLint curveColorLocation;
    GLint curvatureLocation;
    GLint curveThicknessLocation;
    GLint curveFrequencyLocation;

    explicit Material(Shader *shader);
    Material();
    void SetShader(Shader* shader);
    void BeforeDraw() const;
};

#endif //IRGLAB_MATERIAL_H
