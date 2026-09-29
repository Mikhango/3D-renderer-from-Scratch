#include "gui/InputController.h"
#include "core/Model.h"
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <optional>

namespace r3d {
namespace {
constexpr int kTickMs = 16;
constexpr float kSpeed = 3;
constexpr float kSensitivity = 0.075f;

std::optional<int> movement_key(const QKeyEvent &event) {
    switch (event.key()) {
    case Qt::Key_W:
    case 0x0426:
    case 0x0446:
        return Qt::Key_W; // W / Ц
    case Qt::Key_A:
    case 0x0424:
    case 0x0444:
        return Qt::Key_A; // A / Ф
    case Qt::Key_S:
    case 0x042B:
    case 0x044B:
        return Qt::Key_S; // S / Ы
    case Qt::Key_D:
    case 0x0412:
    case 0x0432:
        return Qt::Key_D; // D / В
    default:
        break;
    }
#ifdef Q_OS_MACOS
    if (event.spontaneous()) {
        switch (event.nativeVirtualKey()) {
        case 13:
            return Qt::Key_W;
        case 0:
            return Qt::Key_A;
        case 1:
            return Qt::Key_S;
        case 2:
            return Qt::Key_D;
        default:
            return std::nullopt;
        }
    }
#endif
    return std::nullopt;
}

QPointF mouse_position(const QMouseEvent &event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event.position();
#else
    return event.localPos();
#endif
}
} // namespace

InputController::InputController(Model *model, QWidget *view) : model_(model), view_(view) {
    qApp->installEventFilter(this);
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &InputController::on_tick);
    elapsed_.start();
    timer->start(kTickMs);
}

InputController::~InputController() {
    cancel_input();
    qApp->removeEventFilter(this);
}

bool InputController::eventFilter(QObject *watched, QEvent *event) {
    if (watched != view_)
        return QObject::eventFilter(watched, event);
    if (event->type() == QEvent::FocusOut || event->type() == QEvent::WindowDeactivate ||
        event->type() == QEvent::Hide) {
        cancel_input();
        return false;
    }
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        const auto *key = static_cast<const QKeyEvent *>(event);
        if (!view_->hasFocus() || QApplication::activeModalWidget())
            return false;
        if (key->key() == Qt::Key_Escape) {
            cancel_input();
            return true;
        }
        bool light_key = key->key() == Qt::Key_Y || key->key() == 0x041D || key->key() == 0x043D;
#ifdef Q_OS_MACOS
        light_key = light_key || (key->spontaneous() && key->nativeVirtualKey() == 16);
#endif
        if (light_key) {
            if (event->type() == QEvent::KeyPress && !key->isAutoRepeat() &&
                !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
                model_->toggle_camera_light();
                emit camera_light_changed(model_->camera_light_enabled());
            }
            return true;
        }
        const auto movement = movement_key(*key);
        if (!movement)
            return false;
        if (key->isAutoRepeat())
            return true;
        if (event->type() == QEvent::KeyRelease) {
            held_keys_.erase(*movement);
        } else if (!(key->modifiers() &
                     (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
            held_keys_.insert(*movement);
        }
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        const auto *mouse = static_cast<const QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            begin_drag(mouse_position(*mouse));
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        const auto *mouse = static_cast<const QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            track_drag(mouse_position(*mouse));
            end_drag();
            return true;
        }
    }
    if (event->type() == QEvent::MouseMove) {
        const auto *mouse = static_cast<const QMouseEvent *>(event);
        if (!(mouse->buttons() & Qt::LeftButton)) {
            end_drag();
            return false;
        }
        track_drag(mouse_position(*mouse));
        return last_drag_position_.has_value();
    }
    return QObject::eventFilter(watched, event);
}

void InputController::on_resize(int width, int height) {
    if (width > 0 && height > 0)
        model_->set_frame_size(Width{width}, Height{height});
}

void InputController::begin_drag(const QPointF &position) {
    view_->activateWindow();
    view_->setFocus(Qt::MouseFocusReason);
    last_drag_position_ = position;
    view_->setWindowTitle(QString(kApplicationName) + " - Dragging (LMB) | WASD to move");
}

void InputController::track_drag(const QPointF &position) {
    if (!last_drag_position_)
        return;
    pending_drag_ += position - *last_drag_position_;
    last_drag_position_ = position;
}

void InputController::end_drag() {
    last_drag_position_.reset();
    view_->setWindowTitle(QString(kApplicationName) + " - WASD to move | Drag LMB to look");
}

void InputController::cancel_input() {
    end_drag();
    pending_drag_ = QPointF{};
    held_keys_.clear();
}

void InputController::on_tick() {
    const float seconds = std::min(0.1f, static_cast<float>(elapsed_.restart()) / 1000.0f);
    float forward = static_cast<float>(held_keys_.count(Qt::Key_W)) -
                    static_cast<float>(held_keys_.count(Qt::Key_S));
    float right = static_cast<float>(held_keys_.count(Qt::Key_D)) -
                  static_cast<float>(held_keys_.count(Qt::Key_A));
    const float length = std::hypot(forward, right);
    if (length > 0) {
        forward /= length;
        right /= length;
    }
    const QPointF delta = pending_drag_;
    pending_drag_ = QPointF{};
    model_->advance_camera(forward * kSpeed * seconds, right * kSpeed * seconds,
                           -static_cast<float>(delta.x()) * kSensitivity,
                           -static_cast<float>(delta.y()) * kSensitivity);

    const auto position = model_->camera_position();
    emit camera_position_changed(position.x(), position.y(), position.z());
}
} // namespace r3d
