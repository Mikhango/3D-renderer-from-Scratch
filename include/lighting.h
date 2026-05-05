#pragma once
#include <Eigen/Dense>

struct Light {
    Eigen::Vector3f pos{2.0f, 4.0f, 3.0f};
    Eigen::Vector3f color{1.0f, 1.0f, 1.0f};
    float ka = 0.15f;
    float kd = 0.8f;
    float ks = 0.5f;
    float shininess = 32.0f;
};

class Lighting {
public:
    static Eigen::Vector3f phong(
        const Light& light,
        const Eigen::Vector3f& fragPos,
        const Eigen::Vector3f& normal,
        const Eigen::Vector3f& viewPos,
        const Eigen::Vector3f& objColor
    );
};
