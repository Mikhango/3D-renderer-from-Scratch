#include "primitives/pyramid.h"

namespace Primitives {

Mesh createPyramid(float baseSize, float height) {
    Mesh mesh;
    mesh.color = {1.0f, 0.5f, 0.1f};

    float h = baseSize / 2.0f;

    Eigen::Vector3f apex = { 0.0f, height, 0.0f };
    Eigen::Vector3f bl   = {-h, 0.0f, -h};
    Eigen::Vector3f br   = { h, 0.0f, -h};
    Eigen::Vector3f fr   = { h, 0.0f,  h};
    Eigen::Vector3f fl   = {-h, 0.0f,  h};

    auto addTri = [&](Eigen::Vector3f p0, Eigen::Vector3f p1, Eigen::Vector3f p2) {
        Eigen::Vector3f n = (p1 - p0).cross(p2 - p0).normalized();
        int base = (int)mesh.verts.size();
        for (auto p : {p0, p1, p2}) {
            Vertex v;
            v.pos  = p;
            v.norm = n;
            v.uv   = {0, 0};
            mesh.verts.push_back(v);
        }
        mesh.tris.push_back({base, base + 1, base + 2});
    };

    addTri(bl, br, apex);
    addTri(br, fr, apex);
    addTri(fr, fl, apex);
    addTri(fl, bl, apex);
    addTri(bl, fl, fr);
    addTri(bl, fr, br);

    return mesh;
}

}
