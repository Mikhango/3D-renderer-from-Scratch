#pragma once
#include "core/Types.h"

namespace r3d {
inline constexpr Width kDefaultWidth{800};
inline constexpr Height kDefaultHeight{600};
inline constexpr int kSettingsWidth = 420;
inline constexpr int kSettingsHeight = 280;
inline constexpr char kApplicationName[] = "3D Renderer";
inline constexpr char kApplicationVersion[] = "1.0";
enum class RenderMode { Shaded, Depth, Normals };
enum class ShadowMode { Disabled, Enabled };

struct RenderSettings {
    RenderMode mode = RenderMode::Shaded;
    ShadowMode shadows = ShadowMode::Enabled;
    float vertical_fov_degrees = 60.0f;
};
} // namespace r3d
