#pragma once
#include <Eigen/Dense>

class Camera {
public:
    float theta  = 0.5f;
    float phi    = 0.5f;
    float radius = 5.0f;

    Eigen::Vector3f target{0, 0, 0};
    Eigen::Vector3f up{0, 1, 0};

    float fovY   = 60.0f;
    float aspect = 1.0f;
    float zNear  = 0.1f;
    float zFar   = 100.0f;

    Eigen::Matrix4f viewMat;
    Eigen::Matrix4f projMat;

    Camera();
    void update();
    Eigen::Vector3f pos() const;
    void orbit(float dTheta, float dPhi);
    void zoom(float delta);
    void pan(float dx, float dy);
    void reset();

private:
    void buildView();
    void buildProj();
};
