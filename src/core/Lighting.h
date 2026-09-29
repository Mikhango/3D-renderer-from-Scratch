#pragma once

#include "core/Types.h"

#include <vector>

namespace r3d {

struct DirectionalLight {
    Vector3 direction{0.0f, 1.0f, 1.0f};
    Color intensity{1.0f, 1.0f, 1.0f};
};

struct PointLight {
    Vector3 position{0.0f, 3.0f, 3.0f};
    Color intensity{1.0f, 1.0f, 1.0f};
};

struct Material {
    float ambient = 0.4f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    float shininess = 32.0f;
};

Color blinn_phong(const std::vector<DirectionalLight> &lights, const Color &ambient_light,
                  const Material &material, const Vector3 &position, const Vector3 &normal,
                  const Vector3 &eye_position, const Color &base_color);

} // namespace r3d
