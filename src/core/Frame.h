#pragma once

#include "core/Types.h"

#include <cstdint>
#include <vector>

namespace r3d {

class Frame {
public:
    Frame(Width width, Height height, const Color &background);

    void set_pixel(int x, int y, const Color &color);

    void blend_pixel(int x, int y, const Color &color, float opacity);

    Width width() const;
    Height height() const;

    const std::uint8_t *rgba() const;

private:
    Width width_ = Width{0};
    Height height_ = Height{0};
    std::vector<std::uint8_t> pixels_;
};

} // namespace r3d
