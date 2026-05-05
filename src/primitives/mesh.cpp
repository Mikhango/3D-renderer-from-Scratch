#include "primitives/mesh.h"

void Mesh::recalcNormals() {
    for (auto& v : verts)
        v.norm = Eigen::Vector3f::Zero();

    for (const auto& t : tris) {
        Eigen::Vector3f e1 = verts[t.b].pos - verts[t.a].pos;
        Eigen::Vector3f e2 = verts[t.c].pos - verts[t.a].pos;
        Eigen::Vector3f fn = e1.cross(e2);
        verts[t.a].norm += fn;
        verts[t.b].norm += fn;
        verts[t.c].norm += fn;
    }

    for (auto& v : verts)
        if (v.norm.norm() > 1e-6f)
            v.norm.normalize();
}
