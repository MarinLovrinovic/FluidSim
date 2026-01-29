#include "Object.h"
#include "Renderable.h"

Object::Object(Renderable *renderable, Shader *shader) :
        renderable(renderable),
        shader(shader) {
    modelMatrixLocation = glGetUniformLocation(shader->ID, "model");
    viewProjectionMatrixLocation = glGetUniformLocation(shader->ID, "viewProjection");

    eyePositionLocation = glGetUniformLocation(shader->ID, "eye");

    lightPositionLocation = glGetUniformLocation(shader->ID, "lightPosition");
    lightAmbientLocation = glGetUniformLocation(shader->ID, "lightAmbient");
    lightDiffuseLocation = glGetUniformLocation(shader->ID, "lightDiffuse");
    lightSpecularLocation = glGetUniformLocation(shader->ID, "lightSpecular");

    material = new Material(shader);

    transforms.emplace_back();
}


Object::~Object() {
    delete renderable;
    delete shader;
}
void Object::SendToGpu() {
    renderable->SendToGpu();
}

void Object::Draw(glm::mat4 viewProjectionMatrix, glm::vec3 eyePosition, const Light &light) {
    glUseProgram(shader->ID);
    glUniformMatrix4fv(viewProjectionMatrixLocation, 1, GL_FALSE, &viewProjectionMatrix[0][0]);
    glUniform3f(eyePositionLocation, eyePosition.x, eyePosition.y, eyePosition.z);

    glm::vec3 lightPos = light.GetPosition();
    glUniform3f(lightPositionLocation, lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(lightAmbientLocation, light.ambient.x, light.ambient.y, light.ambient.z);
    glUniform3f(lightDiffuseLocation, light.diffuse.x, light.diffuse.y, light.diffuse.z);
    glUniform3f(lightSpecularLocation, light.specular.x, light.specular.y, light.specular.z);

    material->BeforeDraw();

    renderable->Bind();
    for (auto transform : transforms) {
        auto modelMatrix = transform.GetModelMatrix();
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        renderable->Draw();
    }
    glBindVertexArray(0);
}
