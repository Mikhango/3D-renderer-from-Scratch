#include "lighting.h"
#include <algorithm>
#include <cmath>

Eigen::Vector3f Lighting::phong(
    const Light& light,
    const Eigen::Vector3f& fragPos,
    const Eigen::Vector3f& normal,
    const Eigen::Vector3f& viewPos,
    const Eigen::Vector3f& objColor)
{
    Eigen::Vector3f n = normal.normalized();
    Eigen::Vector3f l = (light.pos - fragPos).normalized();
    Eigen::Vector3f v = (viewPos - fragPos).normalized();
    Eigen::Vector3f r = (2.0f * n.dot(l) * n - l).normalized();

    Eigen::Vector3f ambient  = light.ka * light.color;
    float diff = std::max(n.dot(l), 0.0f);
    Eigen::Vector3f diffuse  = light.kd * diff * light.color;
    float spec = powf(std::max(v.dot(r), 0.0f), light.shininess);
    Eigen::Vector3f specular = light.ks * spec * light.color;

    Eigen::Vector3f res = (ambient + diffuse + specular).cwiseProduct(objColor);
    return res.cwiseMin(1.0f).cwiseMax(0.0f);
}
