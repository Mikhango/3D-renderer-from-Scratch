#pragma once
#include <vector>
#include <string>
#include "primitives/mesh.h"
#include "camera.h"
#include "lighting.h"

struct SceneObject {
    std::string name;
    Mesh mesh;
};

class Scene {
public:
    std::vector<SceneObject> objects;
    Camera camera;
    Light  light;

    Scene();
    void addSphere(float r = 1.0f);
    void addBox(float w = 1.0f, float h = 1.0f, float d = 1.0f);
    void addPyramid(float base = 1.0f, float h = 1.5f);
    void removeObject(int idx);
    void clear();
};
