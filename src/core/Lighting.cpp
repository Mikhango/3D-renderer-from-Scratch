#include "core/Lighting.h"

#include <Eigen/Dense>

#include <algorithm>

namespace r3d {

Color blinn_phong(const std::vector<DirectionalLight> &lights, const Color &ambient_light,
                  const Material &material, const Vector3 &position, const Vector3 &normal,
                  const Vector3 &eye_position, const Color &base_color) {
    Color result{material.ambient * ambient_light.r * base_color.r,
                 material.ambient * ambient_light.g * base_color.g,
                 material.ambient * ambient_light.b * base_color.b};

    const Vector3 unit_normal = normal.normalized();
    const Vector3 view_direction = (eye_position - position).normalized();

    for (const DirectionalLight &light : lights) {

        const Vector3 light_direction = light.direction.normalized();

        const float diffuse_factor = std::max(0.0f, unit_normal.dot(light_direction));
        const Vector3 half_vector = (light_direction + view_direction).normalized();
        const float specular_factor =
            std::pow(std::max(0.0f, unit_normal.dot(half_vector)), material.shininess);

        result.r += light.intensity.r * (material.diffuse * diffuse_factor * base_color.r +
                                         material.specular * specular_factor);
        result.g += light.intensity.g * (material.diffuse * diffuse_factor * base_color.g +
                                         material.specular * specular_factor);
        result.b += light.intensity.b * (material.diffuse * diffuse_factor * base_color.b +
                                         material.specular * specular_factor);
    }

    result.r = std::clamp(result.r, 0.0f, 1.0f);
    result.g = std::clamp(result.g, 0.0f, 1.0f);
    result.b = std::clamp(result.b, 0.0f, 1.0f);
    return result;
}

} // namespace r3d
