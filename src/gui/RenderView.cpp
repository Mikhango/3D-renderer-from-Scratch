#include "gui/RenderView.h"
#include "core/Settings.h"
#include <QImage>
#include <QPainter>
#include <QResizeEvent>

namespace r3d {
RenderView::RenderView() {
    setWindowTitle(QString(kApplicationName) + " - WASD to move | Drag LMB to look");
    resize(to_int(kDefaultWidth), to_int(kDefaultHeight));
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void RenderView::on_next(const Frame &frame) {
    frame_ = frame;
    update();
    ++frames_;
    const auto now = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(now - report_time_).count();
    if (seconds >= 0.5) {
        emit fps_reported(frames_ / seconds);
        report_time_ = now;
        frames_ = 0;
    }
}

void RenderView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    const QImage image(frame_.rgba(), to_int(frame_.width()), to_int(frame_.height()),
                       QImage::Format_RGBA8888);
    painter.drawImage(rect(), image);
}

void RenderView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    emit view_resized(event->size().width(), event->size().height());
}
} // namespace r3d
