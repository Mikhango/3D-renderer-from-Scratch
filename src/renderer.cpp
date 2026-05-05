#include "renderer.h"

Renderer::Renderer(int w, int h) : rast(w, h) {}

void Renderer::resize(int w, int h) {
    rast.resize(w, h);
}

QImage Renderer::renderFrame(Scene& scene) {
    rast.wireframe = wireframe;
    scene.camera.aspect = (float)rast.width / (float)rast.height;
    scene.camera.update();

    std::vector<Mesh> meshes;
    for (auto& obj : scene.objects)
        meshes.push_back(obj.mesh);

    return rast.render(meshes, scene.camera, scene.light);
}
