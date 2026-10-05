#pragma once

#include "core/Camera.h"
#include "core/Frame.h"
#include "core/Lighting.h"
#include "core/Rasterizer.h"
#include "core/Scene.h"
#include "core/Settings.h"
#include "core/Types.h"

namespace r3d {

class Renderer {
public:
    Renderer(Width width, Height height);

    void resize(Width width, Height height);
    void set_settings(const RenderSettings &settings);
    Frame render(const Scene &scene, const Camera &camera, bool camera_light = false);

private:
    Rasterizer rasterizer_;
    Width width_;
    Height height_;
    RenderSettings settings_;
};

} // namespace r3d
