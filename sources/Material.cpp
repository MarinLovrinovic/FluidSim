#include "Material.h"
#include "Shader.h"


Material::Material(Shader *shader) {
    SetShader(shader);
}

void Material::BeforeDraw() const {
    glUniform3f(materialAmbientLocation, ambient.x, ambient.y, ambient.z);
    glUniform3f(materialDiffuseLocation, diffuse.x, diffuse.y, diffuse.z);
    glUniform3f(materialSpecularLocation, specular.x, specular.y, specular.z);
    glUniform1f(materialShininessLocation, shininess);

    glUniform3f(backgroundColorLocation, backgroundColor.r, backgroundColor.g, backgroundColor.b);
    glUniform3f(curveColorLocation, curveColor.r, curveColor.g, curveColor.b);
    glUniform1f(curvatureLocation, curvature);
    glUniform1f(curveThicknessLocation, curveThickness);
    glUniform1f(curveFrequencyLocation, curveFrequency);
}

Material::Material() = default;

void Material::SetShader(Shader *shader) {
    materialAmbientLocation = glGetUniformLocation(shader->ID, "materialAmbient");
    materialDiffuseLocation = glGetUniformLocation(shader->ID, "materialDiffuse");
    materialSpecularLocation = glGetUniformLocation(shader->ID, "materialSpecular");
    materialShininessLocation = glGetUniformLocation(shader->ID, "materialShininess");

    backgroundColorLocation = glGetUniformLocation(shader->ID, "backgroundColor");
    curveColorLocation = glGetUniformLocation(shader->ID, "curveColor");
    curvatureLocation = glGetUniformLocation(shader->ID, "curvature");
    curveThicknessLocation = glGetUniformLocation(shader->ID, "curveThickness");
    curveFrequencyLocation = glGetUniformLocation(shader->ID, "curveFrequency");
}

