#pragma once

#include <Eigen/Dense>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace r3d {

using Index = Eigen::Index;
using Vector2 = Eigen::Vector2f;
using Vector3 = Eigen::Vector3f;
using Vector4 = Eigen::Vector4f;
using Matrix3 = Eigen::Matrix3f;
using Matrix4 = Eigen::Matrix4f;

using Positions = Eigen::Matrix<float, 4, Eigen::Dynamic>;

using Normals = Eigen::Matrix<float, 3, Eigen::Dynamic>;

using PositionView = Eigen::Block<const Positions, 4, Eigen::Dynamic, true>;
using NormalView = Eigen::Block<const Normals, 3, Eigen::Dynamic, true>;

enum class Width : int {};
enum class Height : int {};

inline int to_int(Width width) {
    return static_cast<int>(width);
}

inline int to_int(Height height) {
    return static_cast<int>(height);
}

template <typename Tag> class Length {
public:
    explicit constexpr Length(float value) : value_(value) {}

    constexpr float value() const { return value_; }

private:
    float value_;
};
using ObjectWidth = Length<struct ObjectWidthTag>;
using ObjectHeight = Length<struct ObjectHeightTag>;
using ObjectDepth = Length<struct ObjectDepthTag>;
using Radius = Length<struct RadiusTag>;
using BaseLength = Length<struct BaseLengthTag>;
enum class Stacks : int {};
enum class Slices : int {};

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

inline std::size_t pixel_count(Width width, Height height) {

    constexpr int kMaximumDimension = 8192;
    if (to_int(width) <= 0 || to_int(height) <= 0 || to_int(width) > kMaximumDimension ||
        to_int(height) > kMaximumDimension) {
        throw std::invalid_argument("Invalid frame size.");
    }
    return static_cast<std::size_t>(to_int(width)) * static_cast<std::size_t>(to_int(height));
}

inline bool valid_color(const Color &color) {
    return std::isfinite(color.r) && std::isfinite(color.g) && std::isfinite(color.b) &&
           color.r >= 0 && color.r <= 1 && color.g >= 0 && color.g <= 1 && color.b >= 0 &&
           color.b <= 1;
}
} // namespace r3d
