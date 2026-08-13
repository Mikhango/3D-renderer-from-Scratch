#include "core/ZBuffer.h"

#include <algorithm>

namespace r3d {

ZBuffer::ZBuffer(Width width, Height height)
    : width_(width), height_(height), depths_(pixel_count(width, height), kInfinity) {}

void ZBuffer::resize(Width width, Height height) {
    std::vector<float> next(pixel_count(width, height), kInfinity);
    depths_.swap(next);
    width_ = width;
    height_ = height;
}

void ZBuffer::reset() {
    std::fill(depths_.begin(), depths_.end(), kInfinity);
}

bool ZBuffer::is_visible(int x, int y, float depth) const {
    return depth > depths_[static_cast<std::size_t>(y) * to_int(width_) + x];
}

bool ZBuffer::test_and_write(int x, int y, float depth) {
    const std::size_t index =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(to_int(width_)) +
        static_cast<std::size_t>(x);
    if (depth > depths_[index]) {
        depths_[index] = depth;
        return true;
    }
    return false;
}

} // namespace r3d
