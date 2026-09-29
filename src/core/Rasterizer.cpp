#include "core/Rasterizer.h"

#include <algorithm>

namespace r3d {
namespace {

bool owns_edge(const Vector2 &from, const Vector2 &to) {
    return to.y() > from.y() || (to.y() == from.y() && to.x() < from.x());
}

struct Bounds {
    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
};

int to_pixel_center(float coordinate) {
    return static_cast<int>(std::floor(coordinate - 0.5f));
}

struct ContinuousBounds {
    float min_x, max_x, min_y, max_y;
};

ContinuousBounds find_bounds(const ScreenTriangle &triangle) {
    const auto &first = triangle.screen_position(0);
    ContinuousBounds bounds{first.x(), first.x(), first.y(), first.y()};
    for (int i = 1; i < 3; ++i) {
        bounds.min_x = std::min(bounds.min_x, triangle.screen_position(i).x());
        bounds.max_x = std::max(bounds.max_x, triangle.screen_position(i).x());
        bounds.min_y = std::min(bounds.min_y, triangle.screen_position(i).y());
        bounds.max_y = std::max(bounds.max_y, triangle.screen_position(i).y());
    }
    return bounds;
}

Bounds discretize_bounds(const ContinuousBounds &bounds) {
    return {to_pixel_center(bounds.min_x), to_pixel_center(bounds.max_x),
            to_pixel_center(bounds.min_y), to_pixel_center(bounds.max_y)};
}

Bounds clip_bounds(Bounds bounds, Width width, Height height) {
    bounds.min_x = std::max(bounds.min_x, 0);
    bounds.max_x = std::min(bounds.max_x, to_int(width) - 1);
    bounds.min_y = std::max(bounds.min_y, 0);
    bounds.max_y = std::min(bounds.max_y, to_int(height) - 1);
    return bounds;
}

float compute_oriented_area(const Vector2 &a, const Vector2 &b, const Vector2 &c) {
    return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
}

float compute_edge(const Vector2 &a, const Vector2 &b, const Vector2 &p) {
    return (b.x() - a.x()) * (p.y() - a.y()) - (b.y() - a.y()) * (p.x() - a.x());
}

Vector3 compute_barycentric(const Vector2 &a, const Vector2 &b, const Vector2 &c, const Vector2 &p,
                            float area) {
    const float inv_area = 1.0f / area;
    const float w0 = compute_edge(b, c, p) * inv_area;
    const float w1 = compute_edge(c, a, p) * inv_area;
    const float w2 = compute_edge(a, b, p) * inv_area;
    return Vector3(w0, w1, w2);
}

Vector3 interpolate(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
                    const Vector3 &weights) {
    return v0 * weights.x() + v1 * weights.y() + v2 * weights.z();
}

} // namespace

ScreenTriangle::ScreenTriangle(const Vector2 screen_positions[3], const Vector3 view_positions[3],
                               const Vector3 normals[3], const Color &base_color, float opacity)
    : base_color_(base_color), opacity_(opacity) {
    for (int i = 0; i < 3; ++i) {
        screen_[i] = screen_positions[i];
        view_[i] = view_positions[i];
        normals_[i] = normals[i];
    }
}

const Vector2 &ScreenTriangle::screen_position(int i) const {
    return screen_[i];
}

const Vector3 &ScreenTriangle::view_position(int i) const {
    return view_[i];
}

const Vector3 &ScreenTriangle::normal(int i) const {
    return normals_[i];
}

const Color &ScreenTriangle::base_color() const {
    return base_color_;
}

float ScreenTriangle::opacity() const {
    return opacity_;
}

Rasterizer::Rasterizer(Width width, Height height) : depth_(width, height) {}

void Rasterizer::resize(Width width, Height height) {
    depth_.resize(width, height);
}

void Rasterizer::begin_frame() {
    depth_.reset();
    transparent_.clear();
}

void Rasterizer::finish_frame(Frame *frame) {
    std::stable_sort(transparent_.begin(), transparent_.end(),
                     [](const Fragment &a, const Fragment &b) { return a.depth < b.depth; });
    for (const auto &fragment : transparent_) {
        if (depth_.is_visible(fragment.x, fragment.y, fragment.depth))
            frame->blend_pixel(fragment.x, fragment.y, fragment.color, fragment.opacity);
    }
}

void Rasterizer::draw_triangle(const ScreenTriangle &triangle, const ShadingContext &shading,
                               Frame *frame) {
    const Vector2 &a = triangle.screen_position(0);
    const Vector2 &b = triangle.screen_position(1);
    const Vector2 &c = triangle.screen_position(2);

    const float area = compute_oriented_area(a, b, c);
    if (!std::isfinite(area) || area >= -1e-8f) {

        return;
    }

    const Bounds bounds =
        clip_bounds(discretize_bounds(find_bounds(triangle)), frame->width(), frame->height());
    if (bounds.min_x > bounds.max_x || bounds.min_y > bounds.max_y) {
        return;
    }

    for (int y = bounds.min_y; y <= bounds.max_y; ++y) {
        for (int x = bounds.min_x; x <= bounds.max_x; ++x) {
            const Vector2 pixel_center(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vector3 weights = compute_barycentric(a, b, c, pixel_center, area);
            if (weights.x() < 0.0f || weights.y() < 0.0f || weights.z() < 0.0f) {
                continue;
            }

            if ((weights.x() == 0 && !owns_edge(b, c)) || (weights.y() == 0 && !owns_edge(c, a)) ||
                (weights.z() == 0 && !owns_edge(a, b)))
                continue;
            draw_pixel(x, y, weights, triangle, shading, frame);
        }
    }
}

void Rasterizer::draw_pixel(int x, int y, const Vector3 &weights, const ScreenTriangle &triangle,
                            const ShadingContext &shading, Frame *frame) {
    if (triangle.opacity() == 0)
        return;
    Vector3 corrected(weights.x() / -triangle.view_position(0).z(),
                      weights.y() / -triangle.view_position(1).z(),
                      weights.z() / -triangle.view_position(2).z());
    corrected /= corrected.sum();
    const Vector3 position = interpolate(triangle.view_position(0), triangle.view_position(1),
                                         triangle.view_position(2), corrected);
    if (!depth_.is_visible(x, y, position.z()))
        return;
    const Vector3 normal =
        interpolate(triangle.normal(0), triangle.normal(1), triangle.normal(2), corrected)
            .normalized();
    Color color;
    if (shading.mode == RenderMode::Normals) {
        color = Color{normal.x() * 0.5f + 0.5f, normal.y() * 0.5f + 0.5f, normal.z() * 0.5f + 0.5f};
    } else if (shading.mode == RenderMode::Depth) {
        const float value = 1.0f / (1.0f - position.z());
        color = Color{value, value, value};
    } else {
        std::vector<DirectionalLight> lights = shading.lights;
        for (auto &light : lights) {
            const float visible = shading.shadows
                                      ? shading.shadows->visibility(position, light.direction,
                                                                    ZBuffer::kPositiveInfinity)
                                      : 1;
            light.intensity = Color{light.intensity.r * visible, light.intensity.g * visible,
                                    light.intensity.b * visible};
        }
        for (const auto &point : shading.point_lights) {
            const Vector3 delta = point.position - position;
            const float distance = delta.norm();
            if (distance < 1e-6f)
                continue;
            const Vector3 direction = delta / distance;
            const float visible =
                shading.shadows ? shading.shadows->visibility(position, direction, distance) : 1;
            const float attenuation = visible / (1.0f + 0.05f * distance * distance);
            lights.push_back(
                {direction, Color{point.intensity.r * attenuation, point.intensity.g * attenuation,
                                  point.intensity.b * attenuation}});
        }
        color = blinn_phong(lights, shading.ambient_light, shading.material, position, normal,
                            shading.eye_position, triangle.base_color());
    }
    if (triangle.opacity() < 1 && shading.mode == RenderMode::Shaded) {
        transparent_.push_back({x, y, position.z(), color, triangle.opacity()});
    } else {
        depth_.test_and_write(x, y, position.z());
        frame->set_pixel(x, y, color);
    }
}

} // namespace r3d
