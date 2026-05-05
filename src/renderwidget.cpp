#include "renderwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent), m_renderer(800, 600)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rebuild();
}

void RenderWidget::setWireframe(bool on) {
    m_renderer.wireframe = on;
    rebuild();
}

void RenderWidget::resetCamera() {
    m_scene.camera.reset();
    rebuild();
}

void RenderWidget::redraw() {
    rebuild();
}

void RenderWidget::rebuild() {
    m_frame = m_renderer.renderFrame(m_scene);
    update();
}

void RenderWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (!m_frame.isNull())
        p.drawImage(0, 0, m_frame.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
}

void RenderWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    m_renderer.resize(e->size().width(), e->size().height());
    rebuild();
}

void RenderWidget::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->pos();
    if (e->button() == Qt::LeftButton)   m_lmb = true;
    if (e->button() == Qt::RightButton)  m_rmb = true;
    if (e->button() == Qt::MiddleButton) m_mmb = true;
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e) {
    QPoint d = e->pos() - m_lastPos;
    m_lastPos = e->pos();
    float dx = (float)d.x();
    float dy = (float)d.y();

    if (m_lmb) {
        m_scene.camera.orbit(dx * 0.01f, dy * 0.01f);
        rebuild();
    } else if (m_rmb) {
        m_scene.camera.zoom(dy * 0.05f);
        rebuild();
    } else if (m_mmb) {
        float s = m_scene.camera.radius * 0.002f;
        m_scene.camera.pan(dx * s, dy * s);
        rebuild();
    }
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton)   m_lmb = false;
    if (e->button() == Qt::RightButton)  m_rmb = false;
    if (e->button() == Qt::MiddleButton) m_mmb = false;
}

void RenderWidget::wheelEvent(QWheelEvent* e) {
    float delta = (float)e->angleDelta().y() / 120.0f;
    m_scene.camera.zoom(delta * 0.3f);
    rebuild();
}

void RenderWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_R) {
        resetCamera();
    } else if (e->key() == Qt::Key_W) {
        m_renderer.wireframe = !m_renderer.wireframe;
        rebuild();
    } else if (e->key() == Qt::Key_Plus || e->key() == Qt::Key_Equal) {
        m_scene.camera.zoom(-0.3f);
        rebuild();
    } else if (e->key() == Qt::Key_Minus) {
        m_scene.camera.zoom(0.3f);
        rebuild();
    } else {
        QWidget::keyPressEvent(e);
    }
}
