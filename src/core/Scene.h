#pragma once

#include "core/Lighting.h"
#include "core/Mesh.h"
#include "core/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace r3d {

struct SceneObject {
    std::string name;
    Mesh mesh;
    std::uint64_t id = 0;
};

class Scene {
public:
    void add_object(std::string name, Mesh mesh);

    bool remove_object(const std::string &name);
    bool remove_object_by_id(std::uint64_t id);
    void add_light(const DirectionalLight &light);
    void add_point_light(const PointLight &light);
    void set_ambient_light(const Color &light);
    const std::vector<PointLight> &point_lights() const;
    const Color &ambient_light() const;
    void clear();

    const std::vector<SceneObject> &objects() const;
    const std::vector<DirectionalLight> &lights() const;

private:
    std::vector<SceneObject> objects_;
    std::uint64_t next_object_id_ = 1;
    std::vector<DirectionalLight> lights_;
    std::vector<PointLight> point_lights_;
    Color ambient_light_{0.5f, 0.5f, 0.5f};
};

} // namespace r3d
