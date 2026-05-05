#include "primitives/box.h"

namespace Primitives {

Mesh createBox(float w, float h, float d) {
    Mesh mesh;
    mesh.color = {0.2f, 0.8f, 0.3f};

    float hw = w / 2.0f;
    float hh = h / 2.0f;
    float hd = d / 2.0f;

    struct Face {
        Eigen::Vector3f p[4];
        Eigen::Vector3f n;
    };

    Face faces[6] = {
        {{{ hw, -hh, -hd}, { hw,  hh, -hd}, { hw,  hh,  hd}, { hw, -hh,  hd}}, { 1, 0, 0}},
        {{{-hw, -hh,  hd}, {-hw,  hh,  hd}, {-hw,  hh, -hd}, {-hw, -hh, -hd}}, {-1, 0, 0}},
        {{{-hw,  hh, -hd}, {-hw,  hh,  hd}, { hw,  hh,  hd}, { hw,  hh, -hd}}, { 0, 1, 0}},
        {{{-hw, -hh,  hd}, {-hw, -hh, -hd}, { hw, -hh, -hd}, { hw, -hh,  hd}}, { 0,-1, 0}},
        {{{-hw, -hh,  hd}, { hw, -hh,  hd}, { hw,  hh,  hd}, {-hw,  hh,  hd}}, { 0, 0, 1}},
        {{{ hw, -hh, -hd}, {-hw, -hh, -hd}, {-hw,  hh, -hd}, { hw,  hh, -hd}}, { 0, 0,-1}},
    };

    for (auto& face : faces) {
        int base = (int)mesh.verts.size();
        for (int i = 0; i < 4; i++) {
            Vertex v;
            v.pos  = face.p[i];
            v.norm = face.n;
            v.uv   = {0, 0};
            mesh.verts.push_back(v);
        }
        mesh.tris.push_back({base, base + 1, base + 2});
        mesh.tris.push_back({base, base + 2, base + 3});
    }

    return mesh;
}

}
