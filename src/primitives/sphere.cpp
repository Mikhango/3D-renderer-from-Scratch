#include "primitives/sphere.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Primitives {

Mesh createSphere(float radius, int stacks, int slices) {
    Mesh mesh;
    mesh.color = {0.3f, 0.6f, 1.0f};

    for (int i = 0; i <= stacks; i++) {
        float phi = (float)M_PI * i / stacks;
        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * (float)M_PI * j / slices;

            Vertex v;
            v.pos = {
                radius * sinf(phi) * cosf(theta),
                radius * cosf(phi),
                radius * sinf(phi) * sinf(theta)
            };
            v.norm = v.pos.normalized();
            v.uv = { (float)j / slices, (float)i / stacks };
            mesh.verts.push_back(v);
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int r0 = i * (slices + 1);
            int r1 = (i + 1) * (slices + 1);
            mesh.tris.push_back({r0 + j, r1 + j, r0 + j + 1});
            mesh.tris.push_back({r0 + j + 1, r1 + j, r1 + j + 1});
        }
    }

    return mesh;
}

}
