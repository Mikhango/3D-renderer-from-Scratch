#pragma once
#include <QImage>
#include "scene.h"
#include "rasterizer.h"

class Renderer {
public:
    bool wireframe = false;

    Renderer(int w, int h);
    void resize(int w, int h);
    QImage renderFrame(Scene& scene);

private:
    Rasterizer rast;
};
