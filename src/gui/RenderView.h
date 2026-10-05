#pragma once
#include "core/Frame.h"
#include "core/Observer.h"
#include <QWidget>
#include <chrono>

namespace r3d {
class RenderView : public QWidget, public Observer<Frame> {
    Q_OBJECT
public:
    RenderView();
    void on_next(const Frame &frame) override;
signals:
    void view_resized(int width, int height);
    void fps_reported(double fps);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Frame frame_{Width{1}, Height{1}, Color{}};
    std::chrono::steady_clock::time_point report_time_ = std::chrono::steady_clock::now();
    int frames_ = 0;
};
} // namespace r3d
