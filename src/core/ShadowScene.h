#pragma once
#include "core/Scene.h"
#include <array>
#include <vector>

namespace r3d {

class ShadowScene {
public:
    ShadowScene(const Scene &scene, const Matrix4 &view);
    float visibility(const Vector3 &origin, const Vector3 &direction, float distance) const;

private:
    struct Triangle {
        Vector3 a, b, c;
        float opacity;
    };

    struct Node {
        Vector3 minimum, maximum;
        std::size_t begin = 0, end = 0;
        int left = -1, right = -1;
    };

    int build(std::size_t begin, std::size_t end);
    float trace(int node, const Vector3 &origin, const Vector3 &direction, float distance) const;
    std::vector<Triangle> triangles_;
    std::vector<Node> nodes_;
};
} // namespace r3d
