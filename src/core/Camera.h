#pragma once

#include "core/Types.h"
#include <numbers>

namespace r3d {

class Camera {
public:
    Camera();

    void move_forward(float distance);
    void move_backward(float distance);
    void move_left(float distance);
    void move_right(float distance);

    void pan(float dx, float dy);

    void yaw(float radians);

    void pitch(float radians);
    void set_vertical_fov(float degrees);
    void set_aspect_ratio(float aspect);
    void reset();

    const Matrix4 &view_matrix() const;
    const Matrix4 &projection_matrix() const;
    Vector3 position() const;
    Vector3 forward() const;
    Vector3 right() const;

private:
    Matrix4 view_ = Matrix4::Identity();
    Matrix4 projection_ = Matrix4::Identity();
    float fov_y_ = std::numbers::pi_v<float> / 3;
    float aspect_ = 1.0f;
    float near_plane_ = 0.1f;
    float far_plane_ = 100.0f;

    float pitch_angle_ = 0.0f;
};

} // namespace r3d
