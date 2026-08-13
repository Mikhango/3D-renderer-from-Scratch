#pragma once

#include "core/Mesh.h"
#include "core/Types.h"

#include <string>

namespace r3d {

struct ObjectTransform {
    Vector3 rotation_axis{0.0f, 1.0f, 0.0f};
    float rotation_angle_degrees = 0.0f;
    Vector3 translation{0.0f, 0.0f, 0.0f};
    Color color{0.7f, 0.7f, 0.7f};
    float opacity = 1.0f;
};

void apply_object_transform(Mesh &mesh, const ObjectTransform &transform);

Mesh load_obj(const std::string &path, const ObjectTransform &transform);

} // namespace r3d
