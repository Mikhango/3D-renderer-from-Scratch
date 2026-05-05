#include "scene.h"
#include "primitives/sphere.h"
#include "primitives/box.h"
#include "primitives/pyramid.h"

Scene::Scene() {
    addSphere(1.0f);
}

void Scene::addSphere(float r) {
    SceneObject obj;
    obj.name = "Sphere";
    obj.mesh = Primitives::createSphere(r, 24, 24);
    objects.push_back(std::move(obj));
}

void Scene::addBox(float w, float h, float d) {
    SceneObject obj;
    obj.name = "Box";
    obj.mesh = Primitives::createBox(w, h, d);
    objects.push_back(std::move(obj));
}

void Scene::addPyramid(float base, float h) {
    SceneObject obj;
    obj.name = "Pyramid";
    obj.mesh = Primitives::createPyramid(base, h);
    objects.push_back(std::move(obj));
}

void Scene::removeObject(int idx) {
    if (idx >= 0 && idx < (int)objects.size())
        objects.erase(objects.begin() + idx);
}

void Scene::clear() {
    objects.clear();
}
