#pragma once
#include <QWidget>
#include <QImage>
#include <QPoint>
#include "renderer.h"
#include "scene.h"

class RenderWidget : public QWidget {
    Q_OBJECT

public:
    explicit RenderWidget(QWidget* parent = nullptr);

    Scene&    getScene()    { return m_scene; }
    Renderer& getRenderer() { return m_renderer; }

    void setWireframe(bool on);
    void resetCamera();
    void redraw();

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    Scene    m_scene;
    Renderer m_renderer;
    QImage   m_frame;
    QPoint   m_lastPos;
    bool     m_lmb = false;
    bool     m_rmb = false;
    bool     m_mmb = false;

    void rebuild();
};
