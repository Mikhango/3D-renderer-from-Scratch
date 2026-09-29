#pragma once

#include "core/Types.h"

#include <limits>
#include <vector>

namespace r3d {

class ZBuffer {
public:
    static constexpr float kPositiveInfinity = std::numeric_limits<float>::infinity();
    static constexpr float kInfinity = -std::numeric_limits<float>::infinity();

    ZBuffer(Width width, Height height);

    void resize(Width width, Height height);
    void reset();

    bool is_visible(int x, int y, float depth) const;
    bool test_and_write(int x, int y, float depth);

private:
    Width width_ = Width{0};
    Height height_ = Height{0};
    std::vector<float> depths_;
};

} // namespace r3d
