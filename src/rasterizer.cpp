#include "rasterizer.h"
#include "lighting.h"
#include <algorithm>
#include <cmath>
#include <limits>

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
    zbuf.resize(w * h, std::numeric_limits<float>::infinity());
}

void Rasterizer::resize(int w, int h) {
    width  = w;
    height = h;
    zbuf.resize(w * h, std::numeric_limits<float>::infinity());
}

void Rasterizer::clear(QImage& img) {
    img.fill(QColor(30, 30, 40));
    std::fill(zbuf.begin(), zbuf.end(), std::numeric_limits<float>::infinity());
}

QImage Rasterizer::render(const std::vector<Mesh>& meshes, const Camera& cam, const Light& light) {
    QImage img(width, height, QImage::Format_RGB32);
    clear(img);

    Eigen::Matrix4f vp = cam.projMat * cam.viewMat;
    Eigen::Vector3f viewPos = cam.pos();

    for (const auto& mesh : meshes) {
        Eigen::Matrix4f mvp = vp * mesh.modelMat;
        Eigen::Matrix3f normalMat = mesh.modelMat.topLeftCorner<3,3>().inverse().transpose();

        for (const auto& tri : mesh.tris) {
            const Vertex& v0 = mesh.verts[tri.a];
            const Vertex& v1 = mesh.verts[tri.b];
            const Vertex& v2 = mesh.verts[tri.c];

            Eigen::Vector4f c0 = mvp * Eigen::Vector4f(v0.pos.x(), v0.pos.y(), v0.pos.z(), 1.0f);
            Eigen::Vector4f c1 = mvp * Eigen::Vector4f(v1.pos.x(), v1.pos.y(), v1.pos.z(), 1.0f);
            Eigen::Vector4f c2 = mvp * Eigen::Vector4f(v2.pos.x(), v2.pos.y(), v2.pos.z(), 1.0f);

            if (c0.w() <= 0 && c1.w() <= 0 && c2.w() <= 0) continue;

            Eigen::Vector3f s0, s1, s2;
            if (!project(c0, s0)) continue;
            if (!project(c1, s1)) continue;
            if (!project(c2, s2)) continue;

            Eigen::Vector4f wh0 = mesh.modelMat * Eigen::Vector4f(v0.pos.x(), v0.pos.y(), v0.pos.z(), 1.0f);
            Eigen::Vector4f wh1 = mesh.modelMat * Eigen::Vector4f(v1.pos.x(), v1.pos.y(), v1.pos.z(), 1.0f);
            Eigen::Vector4f wh2 = mesh.modelMat * Eigen::Vector4f(v2.pos.x(), v2.pos.y(), v2.pos.z(), 1.0f);

            Eigen::Vector3f wp0 = wh0.head<3>();
            Eigen::Vector3f wp1 = wh1.head<3>();
            Eigen::Vector3f wp2 = wh2.head<3>();

            Eigen::Vector3f n0 = (normalMat * v0.norm).normalized();
            Eigen::Vector3f n1 = (normalMat * v1.norm).normalized();
            Eigen::Vector3f n2 = (normalMat * v2.norm).normalized();

            if (wireframe)
                wireTriangle(img, s0, s1, s2);
            else
                fillTriangle(img, s0, s1, s2, wp0, wp1, wp2, n0, n1, n2, mesh.color, light, viewPos);
        }
    }

    return img;
}

bool Rasterizer::project(const Eigen::Vector4f& clip, Eigen::Vector3f& screen) const {
    if (clip.w() <= 0) return false;
    float iw = 1.0f / clip.w();
    screen.x() = (clip.x() * iw + 1.0f) * 0.5f * width;
    screen.y() = (1.0f - clip.y() * iw) * 0.5f * height;
    screen.z() = clip.z() * iw;
    return true;
}

float Rasterizer::edge(const Eigen::Vector2f& a, const Eigen::Vector2f& b, const Eigen::Vector2f& p) {
    return (p.x() - a.x()) * (b.y() - a.y()) - (p.y() - a.y()) * (b.x() - a.x());
}

void Rasterizer::fillTriangle(QImage& img,
    const Eigen::Vector3f& s0, const Eigen::Vector3f& s1, const Eigen::Vector3f& s2,
    const Eigen::Vector3f& w0, const Eigen::Vector3f& w1, const Eigen::Vector3f& w2,
    const Eigen::Vector3f& n0, const Eigen::Vector3f& n1, const Eigen::Vector3f& n2,
    const Eigen::Vector3f& color, const Light& light, const Eigen::Vector3f& viewPos)
{
    Eigen::Vector2f p0{s0.x(), s0.y()};
    Eigen::Vector2f p1{s1.x(), s1.y()};
    Eigen::Vector2f p2{s2.x(), s2.y()};

    int minX = std::max(0,         (int)std::floor(std::min({p0.x(), p1.x(), p2.x()})));
    int maxX = std::min(width - 1, (int)std::ceil (std::max({p0.x(), p1.x(), p2.x()})));
    int minY = std::max(0,         (int)std::floor(std::min({p0.y(), p1.y(), p2.y()})));
    int maxY = std::min(height - 1,(int)std::ceil (std::max({p0.y(), p1.y(), p2.y()})));

    float area = edge(p0, p1, p2);
    if (fabsf(area) < 1e-6f) return;
    if (area > 0) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Eigen::Vector2f p{x + 0.5f, y + 0.5f};
            float w_0 = edge(p1, p2, p);
            float w_1 = edge(p2, p0, p);
            float w_2 = edge(p0, p1, p);

            if (w_0 <= 0 && w_1 <= 0 && w_2 <= 0) {
                float b0 = w_0 / area;
                float b1 = w_1 / area;
                float b2 = w_2 / area;

                float depth = b0 * s0.z() + b1 * s1.z() + b2 * s2.z();
                int idx = y * width + x;

                if (depth < zbuf[idx]) {
                    zbuf[idx] = depth;

                    Eigen::Vector3f fragPos  = b0 * w0 + b1 * w1 + b2 * w2;
                    Eigen::Vector3f fragNorm = (b0 * n0 + b1 * n1 + b2 * n2).normalized();
                    Eigen::Vector3f lit = Lighting::phong(light, fragPos, fragNorm, viewPos, color);

                    img.setPixel(x, y, qRgb(
                        (int)(lit.x() * 255),
                        (int)(lit.y() * 255),
                        (int)(lit.z() * 255)
                    ));
                }
            }
        }
    }
}

void Rasterizer::wireTriangle(QImage& img,
    const Eigen::Vector3f& s0, const Eigen::Vector3f& s1, const Eigen::Vector3f& s2)
{
    QColor c(200, 220, 255);
    drawLine(img, (int)s0.x(), (int)s0.y(), (int)s1.x(), (int)s1.y(), c);
    drawLine(img, (int)s1.x(), (int)s1.y(), (int)s2.x(), (int)s2.y(), c);
    drawLine(img, (int)s2.x(), (int)s2.y(), (int)s0.x(), (int)s0.y(), c);
}

void Rasterizer::drawLine(QImage& img, int x0, int y0, int x1, int y1, const QColor& c) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
            img.setPixel(x0, y0, c.rgb());
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}
