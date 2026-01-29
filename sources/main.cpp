// Local Headers


// System Headers
#include <glad/glad.h>
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
    if (!scene->HasMeshes()){
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
    if (mesh->mNormals != nullptr){
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

    bool pressed = action == GLFW_PRESS;
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
    glfwSetInputMode(renderer->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    shader = LoadShader(argv[0], "scene");
    Light light;


    // FALLING ONTO CUBE
//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 1;
//    float dragCoefficient = 0.7;
//    glm::vec3 clothPosition(0, 1.5, 0);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = { };
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.01f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0, 0, 0);
//    vector<Collider*> colliders = {
//            AddCubeCollider(glm::vec3(0, 0, 0), 1.8, 1, glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, 0.5))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 3, 8));


    // FULLY CONNECTED FLAG
//    int dim = 15;
//    float springConstant = 40;
//    float springDampingCoefficient = 5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed;
//    for (int i = 0; i < dim; ++i) {
//        fixed.emplace_back(0, i);
//    }
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(1.5, 0, 0.3);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 8));



// HANGING TWO ATTACHMENT POINT FLAG (WIND)
//    int dim = 20;
//    float springConstant = 100;
//    float springDampingCoefficient = 1;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed = {
//            {0, dim - 1},
//            {1, dim - 1},
//            {dim - 2, dim - 1},
//            {dim - 1, dim - 1}
//    };
//    glm::vec3 clothTexBackgroundColor(0.2078,0.0039,0.2471);
//    glm::vec3 clothTexCurveColor(0.9412,0.9294,0.8);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 4;
//    float clothTexCurveFrequency = 0.4;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0, 0, -1);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 9));


// HANGING TWO ATTACHMENT POINT FLAG (NO WIND)
//    int dim = 15;
//    float springConstant = 60;
//    float springDampingCoefficient = 25;
//    float dragCoefficient = 0.001;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed = {
//            {0, dim - 1},
////            {1, dim - 1},
////            {dim - 2, dim - 1},
//            {dim - 1, dim - 1}
//    };
//    glm::vec3 clothTexBackgroundColor(0.4078,0.0069,0.4471);
//    glm::vec3 clothTexCurveColor(0.9412,0.9294,0.8);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 4;
//    float clothTexCurveFrequency = 0.2;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0, 0, 0);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 9));


// TWO ATTACHMENT POINT FLAG
//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed = {
//            {0, 0},
//            {0, 1},
//            {0, dim - 2},
//            {0, dim - 1}
//    };
//    glm::vec3 clothTexBackgroundColor(0.4, 0.4, 0.4);
//    glm::vec3 clothTexCurveColor(0.4, 1, 0.4);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 4;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(1.5, 0, 0.3);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 9));

// THREE ATTACHMENT POINT FLAG
//    int dim = 15;
//    float springConstant = 40;
//    float springDampingCoefficient = 5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed = {
//            {0, 0},
//            {0, 1},
//            {0, dim / 2},
//            {0, dim / 2 + 1},
//            {0, dim - 2},
//            {0, dim - 1}
//    };
//    glm::vec3 clothTexBackgroundColor(0.0078,0.2039,0.2471);
//    glm::vec3 clothTexCurveColor(0.9412,0.9294,0.8);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 4;
//    float clothTexCurveFrequency = 0.2;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(1.5, 0, 0.3);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 9));



// FULLY CONNECTED FLAG HORIZONTAL
//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 0.5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed;
//    for (int i = 0; i < dim; ++i) {
//        fixed.emplace_back(0, i);
//    }
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(1.5, 0, 0.3);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 8));

// HORIZONTAL CORNERS CONNECTED
//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 10;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0.3);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = {
//            {0, 0},
//            {0, dim - 1},
//            {dim - 1, 0},
//            {dim - 1, dim - 1}
//    };
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0.5, 0, 0.3);
//    vector<Collider*> colliders = { };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 8));


// WIND BLOWING INTO A SPHERE
//    int dim = 20;
//    float springConstant = 60;
//    float springDampingCoefficient = 0.5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 6, 0);
//    float sideLength = 4;
//    bool vertical = true;
//    vector<glm::ivec2> fixed = {
//            {0, dim - 2},
//            {0, dim - 1},
//            {1, dim - 2},
//            {1, dim - 1},
//            {dim / 2, dim - 2},
//            {dim / 2, dim - 1},
//            {dim / 2 - 1, dim - 2},
//            {dim / 2 - 1, dim - 1},
//            {dim - 2, dim - 2},
//            {dim - 2, dim - 1},
//            {dim - 1, dim - 2},
//            {dim - 1, dim - 1},
//    };
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0.4, 0, -1);
//    vector<Collider*> colliders = {
//            AddSphereCollider(glm::vec3(0, 6, -1), 1, 5, glm::vec3(1, 0.3, 0), glm::vec3(0, 0, 1), 0, 5),
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 5, 8));


// FALLING ONTO SPHERE WINDY
//    int dim = 20;
//    float springConstant = 60;
//    float springDampingCoefficient = 0.5;
//    float dragCoefficient = 0.005;
//    glm::vec3 clothPosition(0, 1.5, 0);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = { };
//    glm::vec3 clothTexBackgroundColor(0.5, 0, 0.5);
//    glm::vec3 clothTexCurveColor(1, 0, 0.3);
//    float clothTexCurvature = 5;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(1, 1, 2);
//    vector<Collider*> colliders = {
//            AddSphereCollider(glm::vec3(0, 0, 0), 1, 1, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 3, 8));


// FALLING ONTO SPHERE WITH FLOOR
//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 0.5;
//    float dragCoefficient = 0.01;
//    glm::vec3 clothPosition(0, 1.5, 0);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = { };
//    glm::vec3 clothTexBackgroundColor(0.3, 0.3, 0.3);
//    glm::vec3 clothTexCurveColor(1, 0.3, 0.3);
//    float clothTexCurvature = 1;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0, 0, 0);
//    vector<Collider*> colliders = {
//            AddSphereCollider(glm::vec3(0, 0, 0), 1, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3)),
//            AddCubeCollider(glm::vec3(0, -5, 0), 10, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));


// FALLING ONTO SPHERE
//    int dim = 20;
//    float springConstant = 0;
//    float springDampingCoefficient = 0;
//    float dragCoefficient = 0.0;
//    glm::vec3 clothPosition(0, 1.5, 0);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = { };
    glm::vec3 clothTexBackgroundColor(0.3, 0.3, 0.3);
    glm::vec3 clothTexCurveColor(1, 0.3, 0.3);
    float clothTexCurvature = 1;
    float clothTexCurveThickness = 1;
    float clothTexCurveFrequency = 1;

//    float dt = 0.001f;
//    glm::vec3 gravity(0, -0.4, 0);
    glm::vec3 airflow(0, 0, 0);
//    vector<Collider*> colliders = {
////            AddSphereCollider(glm::vec3(0, 0, 0), 1, 0, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3)),
//            AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));




//    int dim = 20;
//    float springConstant = 40;
//    float springDampingCoefficient = 0.5;
//    float dragCoefficient = 0.1;
//    glm::vec3 clothPosition(0, 1.5, 0);
//    float sideLength = 4;
//    bool vertical = false;
//    vector<glm::ivec2> fixed = { };
//    glm::vec3 clothTexBackgroundColor(0.3, 0.3, 0.3);
//    glm::vec3 clothTexCurveColor(1, 0.3, 0.3);
//    float clothTexCurvature = 1;
//    float clothTexCurveThickness = 1;
//    float clothTexCurveFrequency = 1;
//
//    float dt = 0.005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    glm::vec3 airflow(0, 0, 0);
//
//    vector<Collider*> colliders = {
//            AddSphereCollider(glm::vec3(-1.2, 0, 0), 1, 0),
//            AddSphereCollider(glm::vec3(1.2, 0, 0), 1, 0)
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));

//    Cloth cloth(dim, springConstant, springDampingCoefficient, dragCoefficient, shader, clothPosition, sideLength, vertical, fixed);
//    cloth.object->material->backgroundColor = clothTexBackgroundColor;
//    cloth.object->material->curveColor = clothTexCurveColor;
//    cloth.object->material->curvature = clothTexCurvature;
//    cloth.object->material->curveThickness = clothTexCurveThickness;
//    cloth.object->material->curveFrequency = clothTexCurveFrequency;
//
//    renderer->RegisterRenderable(cloth.object);


//ICOSAHEDRON
//    float dt = 0.001f;
//    glm::vec3 gravity(0, -0.4, 0);
//    float springConstant = 100;
//    float springDampingCoefficient = 40;
//    vector<Collider*> colliders = {
//        AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));
//    TriangleMesh* softBodyMesh = ImportMesh(argv0, "ico");
//    glm::vec3 initialPosition = glm::vec3(0, 1.5, 0);
//    glm::vec3 initialVelocity = glm::vec3(2, -2, 0);


//CUBE
//    float dt = 0.001f;
//    glm::vec3 gravity(0, -0.4, 0);
//    float springConstant = 20;
//    float springDampingCoefficient = 100;
//    vector<Collider*> colliders = {
//        AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 3, 8));
//    TriangleMesh* softBodyMesh = ImportMesh(argv0, "cube");
//    glm::vec3 initialPosition = glm::vec3(0, 3.5, 0);
//    glm::vec3 initialVelocity = glm::vec3(3, -2, 3);

//TORUS
//    float dt = 0.001f;
//    glm::vec3 gravity(0, -0.4, 0);
//    float springConstant = 100;
//    float springDampingCoefficient = 200;
//    vector<Collider*> colliders = {
//    AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 3, 8));
//    TriangleMesh* softBodyMesh = ImportMesh(argv0, "torus");
//    glm::vec3 initialPosition = glm::vec3(0, 0.6, 0);
//    glm::vec3 initialVelocity = glm::vec3(0, 0, 0);


//ICO SPHERE
//    float dt = 0.0005f;
//    glm::vec3 gravity(0, -0.4, 0);
//    float springConstant = 10;
//    float springDampingCoefficient = 5;
//    vector<Collider*> colliders = {
//    //    AddSphereCollider(glm::vec3(0, 0, 0), 1, 0, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3)),
//        AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));
//    TriangleMesh* softBodyMesh = ImportMesh(argv0, "icosphere");
//    glm::vec3 initialPosition = glm::vec3(0, 1.5, 0);
//    glm::vec3 initialVelocity = glm::vec3(0.5, 0, 0);

// ICO SMOOTH
    float dt = 0.002f;
    glm::vec3 gravity(0, -0.4, 0);
    float springConstant = 40;
    float springDampingCoefficient = 500;
    vector<Collider*> colliders = {
        AddSphereCollider(glm::vec3(2, 0, 0), 1, 0, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3)),
        AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
    };
    light.SetPosition(glm::vec3(10, 10, 10));
    camera.SetPosition(glm::vec3(0, 2, 8));
    TriangleMesh* softBodyMesh = ImportMesh(argv0, "icosmooth");
    glm::vec3 initialPosition = glm::vec3(0, 1.5, 0);
    glm::vec3 initialVelocity = glm::vec3(1, 0, 0);

// TEDDY
//    float dt = 0.001f;
//    glm::vec3 gravity(0, -0.4, 0);
//    float springConstant = 50;
//    float springDampingCoefficient = 40;
//    vector<Collider*> colliders = {
//    //    AddSphereCollider(glm::vec3(0, 0, 0), 1, 0, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3)),
//        AddCubeCollider(glm::vec3(0, -10, 0), 20, 5, glm::vec3(0.7, 0.5, 0.3), glm::vec3(0.7, 0.5, 0.3))
//    };
//    light.SetPosition(glm::vec3(10, 10, 10));
//    camera.SetPosition(glm::vec3(0, 2, 8));
//    TriangleMesh* softBodyMesh = ImportMesh(argv0, "teddy");
//    glm::vec3 initialPosition = glm::vec3(0, 1, 0);
//    glm::vec3 initialVelocity = glm::vec3(0, 0, 0);


    softBodyMesh->Normalize();
    SoftBody softBody(softBodyMesh, shader, initialPosition, initialVelocity, dt, springConstant, springDampingCoefficient);
    softBody.object->material->backgroundColor = clothTexBackgroundColor;
    softBody.object->material->curveColor = clothTexCurveColor;
    softBody.object->material->curvature = clothTexCurvature;
    softBody.object->material->curveThickness = clothTexCurveThickness;
    softBody.object->material->curveFrequency = clothTexCurveFrequency;

    renderer->RegisterRenderable(softBody.object);

    while (!glfwWindowShouldClose(renderer->window)) {
        if (moveVector != glm::vec3(0.0f)) {
            camera.Move(dt * camera.LocalToGlobalDir() * glm::vec4(moveVector, 0.0f));
        }

//        cloth.Update(dt, gravity, airflow, colliders);

        softBody.Update(dt, gravity, airflow, colliders);

        renderer->Render(camera, light);

        glfwPollEvents();
        if (glfwGetKey(renderer->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(renderer->window, true);
        glfwSetCursorPos(renderer->window, renderer->width * 0.5, renderer->height * 0.5);
    }
    delete renderer;
    for (auto collider : colliders) {
        delete collider;
    }
    return EXIT_SUCCESS;
}
