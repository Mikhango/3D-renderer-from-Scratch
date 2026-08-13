#include "core/Primitives.h"

#include <numbers>

namespace r3d::primitives {

namespace {

void add_flat_triangle(Mesh *mesh, const Vector3 &p0, const Vector3 &p1, const Vector3 &p2) {
    const Vector3 normal = (p1 - p0).cross(p2 - p0).normalized();
    const Index base = mesh->positions().cols();
    mesh->add_vertex(p0, normal);
    mesh->add_vertex(p1, normal);
    mesh->add_vertex(p2, normal);
    mesh->add_face(base, base + 1, base + 2);
}

} // namespace

Mesh create_sphere(Radius radius_value, Stacks stack_count, Slices slice_count) {
    const float radius = radius_value.value();
    const int stacks = static_cast<int>(stack_count);
    const int slices = static_cast<int>(slice_count);
    if (!std::isfinite(radius) || radius <= 0 || stacks < 2 || slices < 3 || stacks > 512 ||
        slices > 512)
        throw std::invalid_argument("Invalid sphere dimensions.");
    Mesh mesh;
    mesh.set_color(Color{0.3f, 0.6f, 1.0f});

    constexpr float pi = std::numbers::pi_v<float>;
    for (int i = 0; i <= stacks; ++i) {
        const float phi = pi * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j <= slices; ++j) {
            const float theta = 2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
            const Vector3 position{
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta),
            };
            mesh.add_vertex(position, position.normalized());
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const Index r0 = static_cast<Index>(i) * (slices + 1);
            const Index r1 = static_cast<Index>(i + 1) * (slices + 1);

            mesh.add_face(r0 + j, r0 + j + 1, r1 + j);
            mesh.add_face(r0 + j + 1, r1 + j + 1, r1 + j);
        }
    }
    return mesh;
}

Mesh create_box(const BoxSize &size) {
    if (!std::isfinite(size.width.value() + size.height.value() + size.depth.value()) ||
        size.width.value() <= 0 || size.height.value() <= 0 || size.depth.value() <= 0)
        throw std::invalid_argument("Invalid box dimensions.");
    Mesh mesh;
    mesh.set_color(Color{0.2f, 0.8f, 0.3f});

    const float hw = size.width.value() / 2.0f;
    const float hh = size.height.value() / 2.0f;
    const float hd = size.depth.value() / 2.0f;

    struct FaceSpec {
        Vector3 p[4];
        Vector3 n;
    };

    const FaceSpec faces[6] = {
        {{{hw, -hh, -hd}, {hw, hh, -hd}, {hw, hh, hd}, {hw, -hh, hd}}, {1, 0, 0}},
        {{{-hw, -hh, hd}, {-hw, hh, hd}, {-hw, hh, -hd}, {-hw, -hh, -hd}}, {-1, 0, 0}},
        {{{-hw, hh, -hd}, {-hw, hh, hd}, {hw, hh, hd}, {hw, hh, -hd}}, {0, 1, 0}},
        {{{-hw, -hh, hd}, {-hw, -hh, -hd}, {hw, -hh, -hd}, {hw, -hh, hd}}, {0, -1, 0}},
        {{{-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd}}, {0, 0, 1}},
        {{{hw, -hh, -hd}, {-hw, -hh, -hd}, {-hw, hh, -hd}, {hw, hh, -hd}}, {0, 0, -1}},
    };

    for (const FaceSpec &face : faces) {
        const Index base = mesh.positions().cols();
        for (const Vector3 &p : face.p) {
            mesh.add_vertex(p, face.n);
        }
        mesh.add_face(base, base + 1, base + 2);
        mesh.add_face(base, base + 2, base + 3);
    }
    return mesh;
}

Mesh create_pyramid(const PyramidSize &size) {
    if (!std::isfinite(size.base.value() + size.height.value()) || size.base.value() <= 0 ||
        size.height.value() <= 0)
        throw std::invalid_argument("Invalid pyramid dimensions.");
    Mesh mesh;
    mesh.set_color(Color{1.0f, 0.5f, 0.1f});

    const float h = size.base.value() / 2.0f;
    const Vector3 apex{0.0f, size.height.value(), 0.0f};
    const Vector3 bl{-h, 0.0f, -h};
    const Vector3 br{h, 0.0f, -h};
    const Vector3 fr{h, 0.0f, h};
    const Vector3 fl{-h, 0.0f, h};

    add_flat_triangle(&mesh, br, bl, apex);
    add_flat_triangle(&mesh, fr, br, apex);
    add_flat_triangle(&mesh, fl, fr, apex);
    add_flat_triangle(&mesh, bl, fl, apex);

    add_flat_triangle(&mesh, bl, br, fr);
    add_flat_triangle(&mesh, bl, fr, fl);
    return mesh;
}

} // namespace r3d::primitives
