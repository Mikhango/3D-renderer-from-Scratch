#include "core/Frame.h"

#include <algorithm>

namespace r3d {

namespace {

std::uint8_t to_byte(float channel) {
    const float clamped = std::isfinite(channel) ? std::clamp(channel, 0.0f, 1.0f) : 0.0f;
    return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
}

} // namespace

Frame::Frame(Width width, Height height, const Color &background)
    : width_(width), height_(height), pixels_(pixel_count(width, height) * 4) {
    const std::uint8_t bytes[4] = {to_byte(background.r), to_byte(background.g),
                                   to_byte(background.b), 255};
    for (std::size_t i = 0; i < pixels_.size(); i += 4) {
        pixels_[i] = bytes[0];
        pixels_[i + 1] = bytes[1];
        pixels_[i + 2] = bytes[2];
        pixels_[i + 3] = bytes[3];
    }
}

void Frame::set_pixel(int x, int y, const Color &color) {
    const std::size_t index =
        (static_cast<std::size_t>(y) * to_int(width_) + static_cast<std::size_t>(x)) * 4;
    pixels_[index] = to_byte(color.r);
    pixels_[index + 1] = to_byte(color.g);
    pixels_[index + 2] = to_byte(color.b);
    pixels_[index + 3] = 255;
}

void Frame::blend_pixel(int x, int y, const Color &color, float opacity) {
    const auto index = (static_cast<std::size_t>(y) * to_int(width_) + x) * 4;
    const float inverse = 1.0f - opacity;
    set_pixel(x, y,
              Color{color.r * opacity + pixels_[index] / 255.0f * inverse,
                    color.g * opacity + pixels_[index + 1] / 255.0f * inverse,
                    color.b * opacity + pixels_[index + 2] / 255.0f * inverse});
}

Width Frame::width() const {
    return width_;
}

Height Frame::height() const {
    return height_;
}

const std::uint8_t *Frame::rgba() const {
    return pixels_.data();
}

} // namespace r3d
