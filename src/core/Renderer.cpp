#include "core/Renderer.h"

#include <Eigen/Dense>

#include <algorithm>
#include <array>
#include <vector>

namespace r3d {
namespace {

struct ClipVertex {
    Vector4 clip;
    Vector3 view;
    Vector3 normal;
};

ClipVertex interpolate_vertex(const ClipVertex &v1, const ClipVertex &v2, float f1, float f2) {
    const float t = f1 / (f1 - f2);
    ClipVertex result;
    result.clip = v1.clip + (v2.clip - v1.clip) * t;
    result.view = v1.view + (v2.view - v1.view) * t;
    result.normal = v1.normal + (v2.normal - v1.normal) * t;
    return result;
}

float plane_left(const Vector4 &v) {
    return v.x() + v.w();
}

float plane_right(const Vector4 &v) {
    return v.w() - v.x();
}

float plane_bottom(const Vector4 &v) {
    return v.y() + v.w();
}

float plane_top(const Vector4 &v) {
    return v.w() - v.y();
}

float plane_near(const Vector4 &v) {
    return v.z() + v.w();
}

float plane_far(const Vector4 &v) {
    return v.w() - v.z();
}

using PlaneFunction = float (*)(const Vector4 &);

std::vector<ClipVertex> clip_against_plane(const std::vector<ClipVertex> &polygon,
                                           PlaneFunction plane) {
    std::vector<ClipVertex> result;
    if (polygon.empty()) {
        return result;
    }

    const std::size_t count = polygon.size();
    for (std::size_t i = 0; i < count; ++i) {
        const ClipVertex &current = polygon[i];
        const ClipVertex &previous = polygon[(i + count - 1) % count];
        const float f_current = plane(current.clip);
        const float f_previous = plane(previous.clip);

        if (f_current >= 0.0f) {
            if (f_previous < 0.0f) {
                result.push_back(interpolate_vertex(previous, current, f_previous, f_current));
            }
            result.push_back(current);
        } else if (f_previous >= 0.0f) {
            result.push_back(interpolate_vertex(previous, current, f_previous, f_current));
        }
    }
    return result;
}

std::vector<ClipVertex> clip_triangle(const ClipVertex triangle[3]) {
    const std::array<PlaneFunction, 6> planes = {plane_left, plane_right, plane_bottom,
                                                 plane_top,  plane_near,  plane_far};

    std::vector<ClipVertex> polygon(triangle, triangle + 3);
    for (const PlaneFunction plane : planes) {
        polygon = clip_against_plane(polygon, plane);
        if (polygon.empty()) {
            break;
        }
    }
    return polygon;
}

Vector2 to_screen_position(const Vector4 &clip, Width width, Height height) {
    const float frame_width = static_cast<float>(to_int(width));
    const float frame_height = static_cast<float>(to_int(height));
    const Vector3 ndc = clip.head<3>() / clip.w();

    return Vector2((ndc.x() + 1.0f) * 0.5f * frame_width, (1.0f - ndc.y()) * 0.5f * frame_height);
}

ShadingContext make_shading_context(const Scene &scene, const Matrix4 &view) {
    ShadingContext shading;
    shading.ambient_light = scene.ambient_light();
    for (const auto &point : scene.point_lights()) {
        const Vector4 position =
            view * Vector4(point.position.x(), point.position.y(), point.position.z(), 1);
        shading.point_lights.push_back({position.head<3>(), point.intensity});
    }
    shading.material = Material{};
    shading.eye_position = Vector3::Zero();

    const Matrix3 view_rotation = view.block<3, 3>(0, 0);
    shading.lights.reserve(scene.lights().size());
    for (const DirectionalLight &light : scene.lights()) {
        DirectionalLight view_light;
        view_light.direction = (view_rotation * light.direction).normalized();
        view_light.intensity = light.intensity;
        shading.lights.push_back(view_light);
    }
    return shading;
}

std::array<ClipVertex, 3> make_clip_triangle(const Face &face, const Positions &clip_positions,
                                             const Positions &view_positions,
                                             const Normals &view_normals) {
    std::array<ClipVertex, 3> triangle;
    const Index indices[3] = {face.a, face.b, face.c};
    for (int i = 0; i < 3; ++i) {
        triangle[i].clip = clip_positions.col(indices[i]);
        triangle[i].view = view_positions.block<3, 1>(0, indices[i]);
        triangle[i].normal = view_normals.col(indices[i]);
    }
    return triangle;
}

void rasterize_polygon(const std::vector<ClipVertex> &polygon, const Color &base_color,
                       float opacity, Width width, Height height, const ShadingContext &shading,
                       Rasterizer *rasterizer, Frame *frame) {
    if (polygon.size() < 3) {
        return;
    }

    std::array<Vector2, 3> screen;
    std::array<Vector3, 3> views;
    std::array<Vector3, 3> normals;
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
        const ClipVertex *corners[3] = {&polygon[0], &polygon[i], &polygon[i + 1]};
        for (int j = 0; j < 3; ++j) {
            screen[j] = to_screen_position(corners[j]->clip, width, height);
            views[j] = corners[j]->view;
            normals[j] = corners[j]->normal;
        }
        rasterizer->draw_triangle(
            ScreenTriangle(screen.data(), views.data(), normals.data(), base_color, opacity),
            shading, frame);
    }
}

} // namespace

Renderer::Renderer(Width width, Height height)
    : rasterizer_(width, height), width_(width), height_(height) {}

void Renderer::resize(Width width, Height height) {
    rasterizer_.resize(width, height);
    width_ = width;
    height_ = height;
}

void Renderer::set_settings(const RenderSettings &settings) {
    settings_ = settings;
}

Frame Renderer::render(const Scene &scene, const Camera &camera, bool camera_light) {
    Frame frame(width_, height_, Color{0.1f, 0.1f, 0.15f});
    rasterizer_.begin_frame();

    const Matrix4 view = camera.view_matrix();
    const Matrix4 view_projection = camera.projection_matrix() * view;
    ShadingContext shading = make_shading_context(scene, view);

    if (camera_light)
        shading.point_lights.push_back({Vector3::Zero(), Color{1, 1, 1}});
    shading.mode = settings_.mode;
    if (settings_.shadows == ShadowMode::Enabled && settings_.mode == RenderMode::Shaded)
        shading.shadows = std::make_shared<ShadowScene>(scene, view);

    for (const SceneObject &object : scene.objects()) {
        const Mesh &mesh = object.mesh;

        const Positions clip_positions = view_projection * mesh.model_matrix() * mesh.positions();
        const Positions view_positions = view * mesh.model_matrix() * mesh.positions();
        const Normals view_normals = view.block<3, 3>(0, 0) *
                                     mesh.model_matrix().block<3, 3>(0, 0).inverse().transpose() *
                                     mesh.normals();

        for (const Face &face : mesh.faces()) {
            const auto triangle =
                make_clip_triangle(face, clip_positions, view_positions, view_normals);
            if (!triangle[0].clip.allFinite() || !triangle[1].clip.allFinite() ||
                !triangle[2].clip.allFinite())
                continue;
            const std::vector<ClipVertex> polygon = clip_triangle(triangle.data());
            rasterize_polygon(polygon, mesh.color(), mesh.opacity(), width_, height_, shading,
                              &rasterizer_, &frame);
        }
    }
    rasterizer_.finish_frame(&frame);
    return frame;
}

} // namespace r3d
