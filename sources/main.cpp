// Local Headers


// System Headers


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <glad/glad.h>
#ifdef min
#pragma message("min macro is defined")
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//nekima mozda ne radi primjerASSIMP zbog ponovnih definicija stbi funkcija.
//Jedno od mogucih rjesenja je da se zakomentira linija #define STB_IMAGE_IMPLEMENTATION.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "TriangleMesh.h"
#include "Object.h"
#include "FPSManager.h"
#include "Renderer.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "Camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "Light.h"
#include "Cloth.h"
#include "CubeCollider.h"
#include "SoftBody.h"

#define _USE_MATH_DEFINES

// Standard Headers
#include <iostream>
#include <cstdlib>
#include <vector>

#include "Fluid.h"
#include "Fluid2D.h"
#include "Utils.h"

using namespace std;

const double pi = 3.14159265358979323846;

//malo je nespretno napravljeno jer ne koristimo c++17, a treba podrzati i windows i linux,
//slobodno pozivajte new Shader(...); direktno

Shader* LoadShader(char* path, char* naziv) {
    std::string sPath(path);
    std::string pathVert;
    std::string pathFrag;

    pathVert.append(path, sPath.find_last_of("\\/") + 1);
    pathFrag.append(path, sPath.find_last_of("\\/") + 1);
    if (pathFrag[pathFrag.size() - 1] == '/') {
        pathVert.append("shaders/");
        pathFrag.append("shaders/");
    }
    else if (pathFrag[pathFrag.size() - 1] == '\\') {
        pathVert.append("shaders\\");
        pathFrag.append("shaders\\");
    }
    else {
        std::cerr << "nepoznat format pozicije shadera";
        exit(1);
    }

    pathVert.append(naziv);
    pathVert.append(".vert");
    pathFrag.append(naziv);
    pathFrag.append(".frag");

    return new Shader(pathVert.c_str(), pathFrag.c_str());
}


TriangleMesh* ImportMesh(const string& path, const string& model) {
    Assimp::Importer importer;

    std::string dirPath(path, 0, path.find_last_of("\\/"));
    std::string resPath(dirPath);
    resPath.append("\\resources"); //za linux pretvoriti u forwardslash
    std::string objPath(resPath);
    objPath.append("\\" + model + "\\" + model + ".obj"); //za linux pretvoriti u forwardslash

    const aiScene* scene = importer.ReadFile(objPath.c_str(),
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_SortByPType |
                                             aiProcess_FlipUVs |
                                             aiProcess_GenNormals
    );
    if (!scene) {
        cerr << importer.GetErrorString();
        exit(-1);
    }
    if (!scene->HasMeshes()) {
        cerr << "Scene has no meshes." << endl;
        exit(-1);
    }
    aiMesh* mesh = scene->mMeshes[0];

    vector<glm::vec3> vertices;
    for (int i = 0; i < mesh->mNumVertices; i++) {
        auto vertex = mesh->mVertices[i];
        vertices.emplace_back(vertex.x, vertex.y, vertex.z);
    }

    vector<glm::vec3> normals;
    if (mesh->mNormals != nullptr) {
        for (int i = 0; i < mesh->mNumVertices; i++) {
            auto normal = mesh->mNormals[i];
            normals.emplace_back(normal.x, normal.y, normal.z);
        }
    }


    vector<glm::vec3> uvCoords;
    if (mesh->mTextureCoords[0] == nullptr) {
        for (auto vertex : vertices) {
            uvCoords.push_back(vertex);
        }
    } else {
        for (int i = 0; i < mesh->mNumVertices; i++) {
            auto uv = mesh->mTextureCoords[0][i];
            uvCoords.emplace_back(uv.x, uv.y, uv.z);
        }
    }

    vector<int> indices;
    for (int i = 0; i < mesh->mNumFaces; i++) {
        auto face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            std::cerr << "Error: Only triangular faces are supported" << std::endl;
            exit(-1);
        }
        for (int j = 0; j < face.mNumIndices; j++){
            indices.emplace_back(face.mIndices[j]);
        }
    }
    return new TriangleMesh(vertices, normals, indices, uvCoords);
}

Material* ImportMaterial(const string& path, const string& model) {
    Assimp::Importer importer;

    std::string dirPath(path, 0, path.find_last_of("\\/"));
    std::string resPath(dirPath);
    resPath.append("\\resources"); //za linux pretvoriti u forwardslash
    std::string objPath(resPath);
    objPath.append("\\" + model + "\\" + model + ".obj"); //za linux pretvoriti u forwardslash

    const aiScene* scene = importer.ReadFile(objPath.c_str(),
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_SortByPType |
                                             aiProcess_FlipUVs |
                                             aiProcess_GenNormals

    );

    if (!scene) {
        cerr << importer.GetErrorString();
        exit(-1);
    }
    if (!scene->HasMaterials()) {
        cerr << "Scene has no materials." << endl;
        exit(-1);
    }
    aiMaterial* material = scene->mMaterials[scene->mNumMaterials - 1];

    aiColor3D ambientK, diffuseK, specularK;
    auto* output = new Material;

    material->Get(AI_MATKEY_COLOR_AMBIENT, ambientK);
    output->ambient = glm::vec3(ambientK.r, ambientK.g, ambientK.g);

    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseK);
    output->diffuse = glm::vec3(diffuseK.r, diffuseK.g, diffuseK.g);

    material->Get(AI_MATKEY_COLOR_SPECULAR, specularK);
    output->specular = glm::vec3(specularK.r, specularK.g, specularK.g);

    material->Get(AI_MATKEY_SHININESS, output->shininess);
    return output;
}

Renderer* renderer;

void FramebufferSizeCallback(GLFWwindow* window, int Width, int Height)
{
    renderer->width = Width;
    renderer->height = Height;
    glViewport(0, 0, Width, Height);
}

Camera camera;
double pitch;
double yaw;

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    glm::vec2 newPos(xpos, ypos);
    auto delta = newPos - glm::vec2(renderer->width * 0.5, renderer->height * 0.5);
    delta *= -0.001;

    pitch += delta.y;
    pitch = std::max(-pi * 0.5 + 0.01, std::min(pitch, pi * 0.5 - 0.01));

    yaw += delta.x;

    camera.SetRotation(yaw, glm::vec3(0, 1, 0));
    camera.Rotate(pitch, camera.LocalToGlobalDir() * glm::vec4(1, 0, 0, 0));
}

bool forwardPressed;
bool backPressed;
bool leftPressed;
bool rightPressed;
bool upPressed;
bool downPressed;

glm::vec3 moveVector;

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT) return;

    const bool pressed = action == GLFW_PRESS;
    if (key == GLFW_KEY_W) {
        forwardPressed = pressed;
    } else if (key == GLFW_KEY_A) {
        leftPressed = pressed;
    } else if (key == GLFW_KEY_S) {
        backPressed = pressed;
    } else if (key == GLFW_KEY_D) {
        rightPressed = pressed;
    } else if (key == GLFW_KEY_E) {
        downPressed = pressed;
    } else if (key == GLFW_KEY_Q) {
        upPressed = pressed;
    } else {
        return;
    }

    moveVector = glm::vec3((rightPressed ? 1.0f : 0.0f) + (leftPressed ? -1.0f : 0.0f),
                           (upPressed ? 1.0f : 0.0f) + (downPressed ? -1.0f : 0.0f),
                           (backPressed ? 1.0f : 0.0f) + (forwardPressed ? -1.0f : 0.0f));
}

bool pullPressed;
bool pushPressed;

float interactionStrength;

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_REPEAT) return;

    const bool pressed = action == GLFW_PRESS;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        pullPressed = pressed;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        pushPressed = pressed;
    } else {
        return;
    }

    interactionStrength = (pullPressed ? 1.0f : 0.0f) + (pushPressed ? -1.0f : 0.0f);
}

string argv0;
Shader* shader;

SphereCollider* AddSphereCollider(glm::vec3 position, float radius, float frictionCoefficient,
                                  glm::vec3 backgroundColor = glm::vec3(0.3, 0.3, 0.3),
                                  glm::vec3 curveColor = glm::vec3(1, 0.3, 0.3),
                                  float curvature = 1, float curveThickness = 1, float curveFrequency = 1) {
    auto* triangleMesh = ImportMesh(argv0, "sphere");
    triangleMesh->Normalize();

    auto* collider = new Object(triangleMesh, shader);
    collider->SendToGpu();
    collider->transforms[0].SetPosition(position);
    collider->transforms[0].SetScale(glm::vec3(1.0f * radius));
    collider->material = ImportMaterial(argv0, "sphere");
    collider->material->SetShader(shader);
    collider->material->backgroundColor = backgroundColor;
    collider->material->curveColor = curveColor;
    collider->material->curvature = curvature;
    collider->material->curveThickness = curveThickness;
    collider->material->curveFrequency = curveFrequency;

    renderer->RegisterRenderable(collider);

    return new SphereCollider(position, radius, frictionCoefficient);
}


CubeCollider* AddCubeCollider(glm::vec3 position, float sideLength, float frictionCoefficient,
                              glm::vec3 backgroundColor = glm::vec3(0.3, 0.3, 0.3),
                              glm::vec3 curveColor = glm::vec3(1, 0.3, 0.3),
                              float curvature = 1, float curveThickness = 1, float curveFrequency = 1) {
    auto* triangleMesh = ImportMesh(argv0, "cube");
    triangleMesh->Normalize();

    auto* collider = new Object(triangleMesh, shader);
    collider->SendToGpu();
    collider->transforms[0].SetPosition(position);
    collider->transforms[0].SetScale(glm::vec3(0.5f * sideLength));
    collider->material = ImportMaterial(argv0, "cube");
    collider->material->SetShader(shader);
    collider->material->backgroundColor = backgroundColor;
    collider->material->curveColor = curveColor;
    collider->material->curvature = curvature;
    collider->material->curveThickness = curveThickness;
    collider->material->curveFrequency = curveFrequency;

    renderer->RegisterRenderable(collider);

    return new CubeCollider(position, sideLength, frictionCoefficient);
}

bool operator<(const glm::ivec2& a, const glm::ivec2& b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}

int main(int argc, char* argv[]) {
    argv0 = string(argv[0]);
    renderer = new Renderer(1000, 1000);

    glfwSetFramebufferSizeCallback(renderer->window, FramebufferSizeCallback); //funkcija koja se poziva prilikom mijenjanja velicine prozora
    glfwSetCursorPosCallback(renderer->window, CursorPosCallback);
    glfwSetKeyCallback(renderer->window, KeyCallback);
    glfwSetMouseButtonCallback(renderer->window, MouseButtonCallback);
    glfwSetInputMode(renderer->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    shader = LoadShader(argv[0], "scene");
    Light light;

    // NICE 2D SCENARIO
    // bool threeD = false;
    // float dt = 0.01f;
    // vector<Collider*> colliders = { };
    // light.SetPosition(glm::vec3(10, 10, 10));
    // camera.SetPosition(glm::vec3(0, 0, 16));
    // auto gravity = glm::vec2(0, -20);
    //
    // TriangleMesh* fluidParticleMesh = ImportMesh(argv0, "ico");
    // Object fluidObject(fluidParticleMesh, shader);
    // fluidObject.SendToGpu();
    // Fluid2D fluid(glm::ivec2(40, 40), 0.35, 32.2, 47.44, 1, 0.2 , glm::vec2(0), 0.05f, 0.07, glm::vec2(-6, -6), glm::vec2(6, 6), &fluidObject);
    // renderer->RegisterRenderable(&fluidObject);

    // NICE 3D SCENARIO
    bool threeD = true;
    float dt = 0.01f;
    vector<Collider*> colliders = { };
    light.SetPosition(glm::vec3(10, 10, 5));
    camera.SetPosition(glm::vec3(0, 0, 8));
    auto gravity = glm::vec3(0, -20, 0);

    TriangleMesh* fluidParticleMesh = ImportMesh(argv0, "ico");
    Object fluidObject(fluidParticleMesh, shader);
    fluidObject.material->backgroundColor = glm::vec3(0, 0, 1);
    fluidObject.material->curveColor = glm::vec3(0, 0, 1);
    fluidObject.SendToGpu();
    Fluid fluid(glm::ivec3(13, 13, 13), 0.35, 102.2, 37.44, 1, 0.2 , glm::vec3(0), 0.05f, 0.1, glm::vec3(-2, -2, -2), glm::vec3(2, 2, 2), &fluidObject);
    renderer->RegisterRenderable(&fluidObject);

    Object interactionIndicator(fluidParticleMesh, shader);
    interactionIndicator.material->backgroundColor = glm::vec3(1, 0, 0);
    interactionIndicator.transforms[0].SetScale(glm::vec3(0.1));
    interactionIndicator.SendToGpu();
    renderer->RegisterRenderable(&interactionIndicator);

    while (!glfwWindowShouldClose(renderer->window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        if (moveVector != glm::vec3(0.0f)) {
            camera.Move(dt * camera.LocalToGlobalDir() * glm::vec4(moveVector, 0.0f));
        }
        optional<glm::vec4> interaction = nullopt;
        if (interactionStrength != 0) {

            glm::vec3 interactionPoint = threeD
                ? camera.GetPosition() - 6.0f * camera.GetLocalZ() // 3D
                : RaycastZ0(camera.GetPosition(), camera.GetLocalZ()); // 2D

            interactionIndicator.transforms[0].SetPosition(interactionPoint);

            interaction = threeD
                ? glm::vec4(interactionPoint, interactionStrength) // 3D
                : glm::vec4(interactionPoint.x, interactionPoint.y, interactionStrength, 0); // 2D
        }

        auto simStart = std::chrono::high_resolution_clock::now();
        fluid.Update(dt, gravity, interaction);
        fluid.Update(dt, gravity, interaction);
        auto simEnd = std::chrono::high_resolution_clock::now();

        auto renderStart = std::chrono::high_resolution_clock::now();
        renderer->Render(camera, light);
        auto renderEnd = std::chrono::high_resolution_clock::now();

        glfwPollEvents();
        if (glfwGetKey(renderer->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(renderer->window, true);
        glfwSetCursorPos(renderer->window, renderer->width * 0.5, renderer->height * 0.5);
        auto frameEnd = std::chrono::high_resolution_clock::now();

        double simMs = std::chrono::duration<double, std::milli>(simEnd - simStart).count();
        double renderMs = std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();
        double frameMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        std::cout << "Sim: " << simMs << " ms, Render: " << renderMs << " ms, Frame: " << frameMs << " ms\n";
    }
    delete renderer;
    for (auto collider : colliders) {
        delete collider;
    }
    return EXIT_SUCCESS;
}
