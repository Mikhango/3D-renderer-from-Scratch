#include "core/Scene.h"

namespace r3d {

void Scene::add_object(std::string name, Mesh mesh) {
    objects_.push_back(SceneObject{std::move(name), std::move(mesh), next_object_id_++});
}

bool Scene::remove_object(const std::string &name) {
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        if (objects_[i].name == name) {
            objects_.erase(objects_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool Scene::remove_object_by_id(std::uint64_t id) {
    for (auto it = objects_.begin(); it != objects_.end(); ++it) {
        if (it->id == id) {
            objects_.erase(it);
            return true;
        }
    }
    return false;
}

void Scene::add_light(const DirectionalLight &light) {
    if (!light.direction.allFinite() || light.direction.stableNorm() < 1e-6f ||
        !valid_color(light.intensity))
        throw std::invalid_argument("Invalid direction or intensity.");
    lights_.push_back(DirectionalLight{light.direction.stableNormalized(), light.intensity});
}

void Scene::add_point_light(const PointLight &light) {
    if (!light.position.allFinite() || !valid_color(light.intensity))
        throw std::invalid_argument("Invalid point light.");
    point_lights_.push_back(light);
}

void Scene::set_ambient_light(const Color &light) {
    if (!valid_color(light))
        throw std::invalid_argument("Invalid ambient light.");
    ambient_light_ = light;
}

const std::vector<PointLight> &Scene::point_lights() const {
    return point_lights_;
}

const Color &Scene::ambient_light() const {
    return ambient_light_;
}

void Scene::clear() {
    objects_.clear();
    lights_.clear();
    point_lights_.clear();
}

const std::vector<SceneObject> &Scene::objects() const {
    return objects_;
}

const std::vector<DirectionalLight> &Scene::lights() const {
    return lights_;
}

} // namespace r3d
