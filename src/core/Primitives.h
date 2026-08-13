#pragma once

#include "core/Mesh.h"
#include "core/Types.h"

namespace r3d::primitives {

enum class Kind { Cube, Sphere, Pyramid };

struct BoxSize {
    ObjectWidth width{1.0f};
    ObjectHeight height{1.0f};
    ObjectDepth depth{1.0f};
};

struct PyramidSize {
    BaseLength base{1.0f};
    ObjectHeight height{1.5f};
};

Mesh create_sphere(Radius radius, Stacks stacks, Slices slices);
Mesh create_box(const BoxSize &size);
Mesh create_pyramid(const PyramidSize &size);

} // namespace r3d::primitives
