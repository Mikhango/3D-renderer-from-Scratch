#pragma once

#include "core/Frame.h"
#include "core/Lighting.h"
#include "core/Settings.h"
#include "core/ShadowScene.h"
#include "core/Types.h"
#include "core/ZBuffer.h"
#include <memory>

#include <vector>

namespace r3d {

class ScreenTriangle {
public:
    ScreenTriangle(const Vector2 screen_positions[3], const Vector3 view_positions[3],
                   const Vector3 normals[3], const Color &base_color, float opacity = 1.0f);

    const Vector2 &screen_position(int i) const;
    const Vector3 &view_position(int i) const;
    const Vector3 &normal(int i) const;
    const Color &base_color() const;
    float opacity() const;

private:
    Vector2 screen_[3];
    Vector3 view_[3];
    Vector3 normals_[3];
    Color base_color_;
    float opacity_;
};

struct ShadingContext {
    std::vector<DirectionalLight> lights;
    Color ambient_light;
    Material material;
    std::vector<PointLight> point_lights;
    std::shared_ptr<const ShadowScene> shadows;
    RenderMode mode = RenderMode::Shaded;
    Vector3 eye_position = Vector3::Zero();
};

class Rasterizer {
public:
    Rasterizer(Width width, Height height);

    void resize(Width width, Height height);

    void begin_frame();
    void finish_frame(Frame *frame);
    void draw_triangle(const ScreenTriangle &triangle, const ShadingContext &shading, Frame *frame);

private:
    struct Fragment {
        int x, y;
        float depth;
        Color color;
        float opacity;
    };

    void draw_pixel(int x, int y, const Vector3 &weights, const ScreenTriangle &triangle,
                    const ShadingContext &shading, Frame *frame);
    ZBuffer depth_;
    std::vector<Fragment> transparent_;
};

} // namespace r3d
