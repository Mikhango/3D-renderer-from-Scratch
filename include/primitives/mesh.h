#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

struct Vertex {
    Eigen::Vector3f pos;
    Eigen::Vector3f norm;
    Eigen::Vector2f uv;
};

struct Triangle {
    int a, b, c;
};

class Mesh {
public:
    std::vector<Vertex>   verts;
    std::vector<Triangle> tris;
    Eigen::Vector3f       color{0.7f, 0.7f, 0.7f};
    Eigen::Matrix4f       modelMat = Eigen::Matrix4f::Identity();

    void recalcNormals();
};
