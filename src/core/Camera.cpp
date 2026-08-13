#include "core/Camera.h"

#include <algorithm>
#include <numbers>

namespace r3d {

namespace {

Vector3 world_up() {
    return Vector3{0.0f, 1.0f, 0.0f};
}

Matrix4 make_translation(const Vector3 &shift) {
    Matrix4 result = Matrix4::Identity();
    result.topRightCorner<3, 1>() = shift;
    return result;
}

Matrix4 make_rotation(const Vector3 &axis, float radians) {
    const Vector3 unit_axis = axis.normalized();
    Matrix4 result = Matrix4::Identity();
    result.topLeftCorner<3, 3>() = Eigen::AngleAxisf(radians, unit_axis).toRotationMatrix();
    return result;
}

Matrix4 make_projection(float fov_y, float aspect, float near_plane, float far_plane) {
    const float tan_half_fov = std::tan(fov_y * 0.5f);
    const float range = far_plane - near_plane;

    Matrix4 projection = Matrix4::Zero();
    projection(0, 0) = 1.0f / (aspect * tan_half_fov);
    projection(1, 1) = 1.0f / tan_half_fov;
    projection(2, 2) = -(far_plane + near_plane) / range;
    projection(2, 3) = -(2.0f * far_plane * near_plane) / range;
    projection(3, 2) = -1.0f;
    return projection;
}

} // namespace

Camera::Camera() {
    reset();
}

void Camera::move_forward(float distance) {
    if (!std::isfinite(distance))
        throw std::invalid_argument("Invalid movement.");
    view_ = view_ * make_translation(-forward().normalized() * distance);
}

void Camera::move_backward(float distance) {
    move_forward(-distance);
}

void Camera::move_left(float distance) {
    if (!std::isfinite(distance))
        throw std::invalid_argument("Invalid movement.");

    view_ = view_ * make_translation(right() * distance);
}

void Camera::move_right(float distance) {
    move_left(-distance);
}

void Camera::pan(float dx, float dy) {
    if (!std::isfinite(dx) || !std::isfinite(dy))
        throw std::invalid_argument("Invalid movement.");

    view_ = view_ * make_translation(Vector3{-dx, -dy, 0.0f});
}

void Camera::yaw(float radians) {
    if (!std::isfinite(radians))
        throw std::invalid_argument("Invalid rotation.");
    if (radians == 0)
        return;

    const Vector3 eye = position();
    view_.topLeftCorner<3, 3>() =
        (view_.topLeftCorner<3, 3>() * make_rotation(world_up(), -radians).topLeftCorner<3, 3>())
            .eval();
    view_.topLeftCorner<3, 3>() =
        Eigen::Quaternionf(Matrix3(view_.topLeftCorner<3, 3>())).normalized().toRotationMatrix();
    view_.topRightCorner<3, 1>() = -view_.topLeftCorner<3, 3>() * eye;
}

void Camera::pitch(float radians) {
    if (!std::isfinite(radians))
        throw std::invalid_argument("Invalid rotation.");

    constexpr float kMaxPitch = 1.5533f;
    const float previous = pitch_angle_;
    pitch_angle_ = std::clamp(pitch_angle_ + radians, -kMaxPitch, kMaxPitch);
    const float applied = pitch_angle_ - previous;
    if (applied == 0.0f) {
        return;
    }

    const Vector3 eye = position();
    const Matrix3 rotation = make_rotation(Vector3::UnitX(), -applied).topLeftCorner<3, 3>() *
                             view_.topLeftCorner<3, 3>();
    view_.topLeftCorner<3, 3>() = Eigen::Quaternionf(rotation).normalized().toRotationMatrix();
    view_.topRightCorner<3, 1>() = -view_.topLeftCorner<3, 3>() * eye;
}

void Camera::set_vertical_fov(float degrees) {
    if (!std::isfinite(degrees) || degrees < 10 || degrees > 150)
        throw std::invalid_argument("Invalid field of view.");
    fov_y_ = degrees * std::numbers::pi_v<float> / 180;
    projection_ = make_projection(fov_y_, aspect_, near_plane_, far_plane_);
}

void Camera::set_aspect_ratio(float aspect) {
    if (!std::isfinite(aspect) || aspect <= 0)
        throw std::invalid_argument("Invalid aspect ratio.");
    aspect_ = aspect;
    projection_ = make_projection(fov_y_, aspect_, near_plane_, far_plane_);
}

void Camera::reset() {

    pitch_angle_ = 0;
    view_ = make_translation(Vector3{0.0f, 0.5f, -5.0f});
    projection_ = make_projection(fov_y_, aspect_, near_plane_, far_plane_);
}

const Matrix4 &Camera::view_matrix() const {
    return view_;
}

const Matrix4 &Camera::projection_matrix() const {
    return projection_;
}

Vector3 Camera::position() const {
    const Matrix3 rotation = view_.topLeftCorner<3, 3>();
    const Vector3 translation = view_.topRightCorner<3, 1>();
    return -rotation.transpose() * translation;
}

Vector3 Camera::forward() const {

    const Matrix3 rotation = view_.topLeftCorner<3, 3>();
    return rotation.transpose() * Vector3{0.0f, 0.0f, -1.0f};
}

Vector3 Camera::right() const {
    const Matrix3 rotation = view_.topLeftCorner<3, 3>();
    return rotation.transpose() * Vector3{1.0f, 0.0f, 0.0f};
}

} // namespace r3d
