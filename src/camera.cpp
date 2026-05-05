#include "camera.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Camera::Camera() {
    update();
}

Eigen::Vector3f Camera::pos() const {
    return {
        target.x() + radius * sinf(phi) * cosf(theta),
        target.y() + radius * cosf(phi),
        target.z() + radius * sinf(phi) * sinf(theta)
    };
}

void Camera::update() {
    buildView();
    buildProj();
}

void Camera::orbit(float dTheta, float dPhi) {
    theta += dTheta;
    phi = std::clamp(phi + dPhi, 0.05f, (float)M_PI - 0.05f);
    update();
}

void Camera::zoom(float delta) {
    radius = std::clamp(radius - delta, 0.5f, 200.0f);
    update();
}

void Camera::pan(float dx, float dy) {
    Eigen::Vector3f eye = pos();
    Eigen::Vector3f fwd = (target - eye).normalized();
    Eigen::Vector3f right = fwd.cross(up).normalized();
    Eigen::Vector3f u = right.cross(fwd).normalized();
    target += right * (-dx) + u * dy;
    update();
}

void Camera::reset() {
    theta  = 0.5f;
    phi    = 0.5f;
    radius = 5.0f;
    target = Eigen::Vector3f::Zero();
    update();
}

void Camera::buildView() {
    Eigen::Vector3f eye = pos();
    Eigen::Vector3f f = (target - eye).normalized();
    Eigen::Vector3f r = f.cross(up).normalized();
    Eigen::Vector3f u = r.cross(f);

    viewMat = Eigen::Matrix4f::Identity();
    viewMat(0,0) =  r.x(); viewMat(0,1) =  r.y(); viewMat(0,2) =  r.z();
    viewMat(1,0) =  u.x(); viewMat(1,1) =  u.y(); viewMat(1,2) =  u.z();
    viewMat(2,0) = -f.x(); viewMat(2,1) = -f.y(); viewMat(2,2) = -f.z();
    viewMat(0,3) = -r.dot(eye);
    viewMat(1,3) = -u.dot(eye);
    viewMat(2,3) =  f.dot(eye);
}

void Camera::buildProj() {
    float fovRad  = fovY * (float)M_PI / 180.0f;
    float tanHalf = tanf(fovRad * 0.5f);
    float range   = zFar - zNear;

    projMat = Eigen::Matrix4f::Zero();
    projMat(0,0) = 1.0f / (aspect * tanHalf);
    projMat(1,1) = 1.0f / tanHalf;
    projMat(2,2) = -(zFar + zNear) / range;
    projMat(2,3) = -(2.0f * zFar * zNear) / range;
    projMat(3,2) = -1.0f;
}
