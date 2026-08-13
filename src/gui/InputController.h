#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <optional>
#include <set>
class QWidget;

namespace r3d {
class Model;

class InputController : public QObject {
    Q_OBJECT
public:
    InputController(Model *model, QWidget *view);
    ~InputController() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void on_resize(int width, int height);
signals:
    void camera_light_changed(bool enabled);
    void camera_position_changed(float x, float y, float z);

private:
    void begin_drag(const QPointF &position);
    void track_drag(const QPointF &position);
    void end_drag();
    void cancel_input();
    void on_tick();
    Model *model_;
    QWidget *view_;
    std::optional<QPointF> last_drag_position_;
    QPointF pending_drag_;
    std::set<int> held_keys_;
    QElapsedTimer elapsed_;
};
} // namespace r3d
