#pragma once
#include <vector>
#include <QImage>
#include <Eigen/Dense>
#include "primitives/mesh.h"
#include "camera.h"
#include "lighting.h"

class Rasterizer {
public:
    int  width     = 800;
    int  height    = 600;
    bool wireframe = false;

    Rasterizer(int w, int h);
    void resize(int w, int h);

    QImage render(const std::vector<Mesh>& meshes, const Camera& cam, const Light& light);

private:
    std::vector<float> zbuf;

    void clear(QImage& img);
    bool project(const Eigen::Vector4f& clip, Eigen::Vector3f& screen) const;

    void fillTriangle(QImage& img,
        const Eigen::Vector3f& s0, const Eigen::Vector3f& s1, const Eigen::Vector3f& s2,
        const Eigen::Vector3f& w0, const Eigen::Vector3f& w1, const Eigen::Vector3f& w2,
        const Eigen::Vector3f& n0, const Eigen::Vector3f& n1, const Eigen::Vector3f& n2,
        const Eigen::Vector3f& color, const Light& light, const Eigen::Vector3f& viewPos);

    void wireTriangle(QImage& img,
        const Eigen::Vector3f& s0, const Eigen::Vector3f& s1, const Eigen::Vector3f& s2);

    void drawLine(QImage& img, int x0, int y0, int x1, int y1, const QColor& c);

    static float edge(const Eigen::Vector2f& a, const Eigen::Vector2f& b, const Eigen::Vector2f& p);
};
