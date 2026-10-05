#include "core/ShadowScene.h"
#include <algorithm>
#include <limits>

namespace r3d {
namespace {
constexpr float kRayBias = 0.001f;

bool hits_box(const Vector3 &low, const Vector3 &high, const Vector3 &origin,
              const Vector3 &direction, float distance) {
    float near = kRayBias;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < 1e-8f) {
            if (origin[axis] < low[axis] || origin[axis] > high[axis])
                return false;
            continue;
        }
        float a = (low[axis] - origin[axis]) / direction[axis];
        float b = (high[axis] - origin[axis]) / direction[axis];
        if (a > b)
            std::swap(a, b);
        near = std::max(near, a);
        distance = std::min(distance, b);
        if (near > distance)
            return false;
    }
    return true;
}

bool hits_triangle(const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &origin,
                   const Vector3 &direction, float distance) {
    const Vector3 e1 = b - a, e2 = c - a;
    const Vector3 cross = direction.cross(e2);
    const float det = e1.dot(cross);
    if (std::abs(det) < 1e-8f)
        return false;
    const Vector3 offset = origin - a;
    const float u = offset.dot(cross) / det;
    if (u < 0 || u > 1)
        return false;
    const Vector3 q = offset.cross(e1);
    const float v = direction.dot(q) / det;
    if (v < 0 || u + v > 1)
        return false;
    const float t = e2.dot(q) / det;
    return t > kRayBias && t < distance - kRayBias;
}
} // namespace

ShadowScene::ShadowScene(const Scene &scene, const Matrix4 &view) {
    for (const auto &object : scene.objects()) {
        if (object.mesh.opacity() == 0)
            continue;
        const Positions positions = view * object.mesh.model_matrix() * object.mesh.positions();
        for (const auto &face : object.mesh.faces()) {
            triangles_.push_back({positions.col(face.a).head<3>(), positions.col(face.b).head<3>(),
                                  positions.col(face.c).head<3>(), object.mesh.opacity()});
        }
    }
    if (!triangles_.empty())
        build(0, triangles_.size());
}

float ShadowScene::visibility(const Vector3 &origin, const Vector3 &direction,
                              float distance) const {
    return nodes_.empty() ? 1.0f : trace(0, origin, direction, distance);
}

int ShadowScene::build(std::size_t begin, std::size_t end) {
    Node node;
    node.begin = begin;
    node.end = end;
    node.minimum = Vector3::Constant(std::numeric_limits<float>::infinity());
    node.maximum = -node.minimum;
    for (auto i = begin; i < end; ++i) {
        const auto &t = triangles_[i];
        node.minimum = node.minimum.cwiseMin(t.a).cwiseMin(t.b).cwiseMin(t.c);
        node.maximum = node.maximum.cwiseMax(t.a).cwiseMax(t.b).cwiseMax(t.c);
    }
    const int index = static_cast<int>(nodes_.size());
    nodes_.push_back(node);
    if (end - begin <= 8)
        return index;
    Index axis = 0;
    (node.maximum - node.minimum).maxCoeff(&axis);
    const auto middle = begin + (end - begin) / 2;
    std::nth_element(triangles_.begin() + begin, triangles_.begin() + middle,
                     triangles_.begin() + end, [axis](const Triangle &a, const Triangle &b) {
                         return (a.a[axis] + a.b[axis] + a.c[axis]) <
                                (b.a[axis] + b.b[axis] + b.c[axis]);
                     });
    const int left = build(begin, middle);
    const int right = build(middle, end);
    nodes_[index].left = left;
    nodes_[index].right = right;
    return index;
}

float ShadowScene::trace(int index, const Vector3 &origin, const Vector3 &direction,
                         float distance) const {
    const auto &node = nodes_[index];
    if (!hits_box(node.minimum, node.maximum, origin, direction, distance))
        return 1;
    if (node.left >= 0) {
        const float left = trace(node.left, origin, direction, distance);
        return left <= 0 ? 0 : left * trace(node.right, origin, direction, distance);
    }
    float visibility = 1;
    for (auto i = node.begin; i < node.end; ++i) {
        const auto &t = triangles_[i];
        if (hits_triangle(t.a, t.b, t.c, origin, direction, distance))
            visibility *= 1 - t.opacity;
        if (visibility <= 0)
            break;
    }
    return visibility;
}
} // namespace r3d
