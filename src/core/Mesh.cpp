#include "core/Mesh.h"

namespace r3d {

void Mesh::reserve_vertices(Index count) {
    if (count < 0)
        throw std::invalid_argument("Invalid vertex capacity.");
    if (count <= positions_.cols())
        return;
    Positions positions(4, count);
    Normals normals(3, count);
    positions.leftCols(vertex_count_) = positions_.leftCols(vertex_count_);
    normals.leftCols(vertex_count_) = normals_.leftCols(vertex_count_);
    positions_.swap(positions);
    normals_.swap(normals);
}

void Mesh::add_vertex(const Vector3 &position, const Vector3 &normal) {
    if (!position.allFinite() || !normal.allFinite())
        throw std::invalid_argument("Invalid vertex.");
    const Index count = vertex_count_;
    reserve_vertices(
        std::max<Index>(count + 1, positions_.cols() * (count == positions_.cols() ? 2 : 1)));
    ++vertex_count_;
    positions_.col(count) = Vector4(position.x(), position.y(), position.z(), 1.0f);
    normals_.col(count) = normal;
}

void Mesh::add_face(Index a, Index b, Index c) {
    const Index count = vertex_count_;
    if (a < 0 || b < 0 || c < 0 || a >= count || b >= count || c >= count)
        throw std::out_of_range("Invalid face index.");
    faces_.push_back(Face{a, b, c});
}

void Mesh::set_color(const Color &color) {
    if (!valid_color(color))
        throw std::invalid_argument("Invalid color.");
    color_ = color;
}

void Mesh::set_opacity(float opacity) {
    if (!std::isfinite(opacity) || opacity < 0 || opacity > 1)
        throw std::invalid_argument("Invalid opacity.");
    opacity_ = opacity;
}

float Mesh::opacity() const {
    return opacity_;
}

void Mesh::set_model_matrix(const Matrix4 &model) {
    if (!model.allFinite() || !model.row(3).isApprox(Vector4(0, 0, 0, 1).transpose()) ||
        std::abs(model.topLeftCorner<3, 3>().determinant()) < 1e-8f)
        throw std::invalid_argument("Invalid model transform.");
    model_ = model;
}

void Mesh::recalculate_normals() {
    const Index count = vertex_count_;
    normals_.setZero();
    for (const Face &face : faces_) {
        const Vector3 a = positions_.col(face.a).head<3>();
        const Vector3 b = positions_.col(face.b).head<3>();
        const Vector3 c = positions_.col(face.c).head<3>();
        const Vector3 face_normal = (b - a).cross(c - a);
        normals_.col(face.a) += face_normal;
        normals_.col(face.b) += face_normal;
        normals_.col(face.c) += face_normal;
    }
    for (Index i = 0; i < count; ++i) {
        const Vector3 normal = normals_.col(i);
        if (normal.norm() > 1e-6f) {
            normals_.col(i) = normal.normalized();
        }
    }
}

auto Mesh::positions() const -> PositionView {
    return positions_.leftCols(vertex_count_);
}

auto Mesh::normals() const -> NormalView {
    return normals_.leftCols(vertex_count_);
}

const std::vector<Face> &Mesh::faces() const {
    return faces_;
}

const Color &Mesh::color() const {
    return color_;
}

const Matrix4 &Mesh::model_matrix() const {
    return model_;
}

} // namespace r3d
