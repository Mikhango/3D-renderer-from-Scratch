#pragma once

#include "core/Types.h"

#include <vector>

namespace r3d {

struct Face {
    Index a = 0;
    Index b = 0;
    Index c = 0;
};

class Mesh {
public:
    void reserve_vertices(Index count);
    void add_vertex(const Vector3 &position, const Vector3 &normal);
    void add_face(Index a, Index b, Index c);
    void set_color(const Color &color);
    void set_opacity(float opacity);
    float opacity() const;
    void set_model_matrix(const Matrix4 &model);

    void recalculate_normals();

    auto positions() const -> PositionView;
    auto normals() const -> NormalView;
    const std::vector<Face> &faces() const;
    const Color &color() const;
    const Matrix4 &model_matrix() const;

private:
    Index vertex_count_ = 0;
    Positions positions_{4, 0};
    Normals normals_{3, 0};
    std::vector<Face> faces_;
    Color color_{0.7f, 0.7f, 0.7f};
    float opacity_ = 1.0f;
    Matrix4 model_ = Matrix4::Identity();
};

} // namespace r3d
