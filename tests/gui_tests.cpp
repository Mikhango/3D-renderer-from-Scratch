#include "app/Application.h"
#include "app/QRunTime.h"
#include "core/Model.h"
#include "gui/GuiController.h"
#include "gui/InputController.h"
#include "gui/RenderView.h"
#include "gui/SettingsWindow.h"
#include <QAction>
#include <QCursor>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTimer>
#include <chrono>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace {
void require(bool value, const char *text) {
    if (!value)
        throw std::runtime_error(text);
}

void wait_events(int milliseconds) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    do {
        QApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
}

bool has_text(QWidget *widget, const QString &text) {
    for (auto *label : widget->findChildren<QLabel *>())
        if (label->text() == text)
            return true;
    return false;
}

void mouse_sampling_test() {
    struct Frames : r3d::Observer<r3d::Frame> {
        std::optional<r3d::Frame> latest;

        void on_next(const r3d::Frame &frame) override { latest = frame; }
    } actual, expected;

    r3d::Model model, reference;
    model.set_frame_size(r3d::Width{80}, r3d::Height{60});
    reference.set_frame_size(r3d::Width{80}, r3d::Height{60});
    model.subscribe(&actual);
    reference.subscribe(&expected);
    r3d::RenderView view;
    view.resize(160, 120);
    view.show();
    view.activateWindow();
    view.setFocus();
    r3d::InputController input(&model, &view);
    wait_events(20);
    const QPoint original_cursor = QCursor::pos();
    QMouseEvent click(QEvent::MouseButtonPress, QPointF(80, 60), QPointF(80, 60), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &click);
    wait_events(35);
    require(QCursor::pos() == original_cursor, "drag start must not warp cursor");
    require(view.cursor().shape() != Qt::BlankCursor, "cursor remains visible during drag");
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "holding LMB without moving must not rotate");

    for (int i = 0; i < 12; ++i) {
        QMouseEvent move(QEvent::MouseMove, QPointF(100, 70), QPointF(100, 70), Qt::NoButton,
                         Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &move);
    }
    wait_events(45);
    reference.advance_camera(0, 0, -1.5f, -0.75f);
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "drag rotates by displacement, not distance from screen center");
    wait_events(35);
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "stationary held mouse must not keep rotating");
    require(QCursor::pos() == original_cursor, "drag must never recenter cursor");
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(100, 70), QPointF(100, 70),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &release);
    QMouseEvent free_move(QEvent::MouseMove, QPointF(140, 90), QPointF(140, 90), Qt::NoButton,
                          Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &free_move);
    wait_events(35);
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "released mouse must not rotate");
    QMouseEvent right(QEvent::MouseButtonPress, QPointF(80, 60), QPointF(80, 60), Qt::RightButton,
                      Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &right);
    QMouseEvent right_move(QEvent::MouseMove, QPointF(140, 90), QPointF(140, 90), Qt::NoButton,
                           Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &right_move);
    wait_events(35);
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "right drag must not rotate");

    QMouseEvent second_press(QEvent::MouseButtonPress, QPointF(20, 20), QPointF(20, 20),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &second_press);
    wait_events(35);
    require(std::memcmp(actual.latest->rgba(), expected.latest->rgba(), 80 * 60 * 4) == 0,
            "new drag must not jump to prior drag location");
    model.unsubscribe(&actual);
    reference.unsubscribe(&expected);
}
} // namespace

int main(int argc, char **argv) {
    r3d::QRunTime runtime(argc, argv);
    try {
        r3d::Model model;
        model.set_frame_size(r3d::Width{80}, r3d::Height{60});
        r3d::RenderView view;
        view.resize(160, 120);
        r3d::SettingsWindow settings;
        r3d::InputController input(&model, &view);
        r3d::GuiController gui(&model, &settings);
        model.subscribe(&view);
        view.show();
        settings.show();
        wait_events(50);
        const r3d::Vector3 before = model.camera_position();
        QKeyEvent press(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier);
        QApplication::sendEvent(&settings, &press);
        wait_events(35);
        require(model.camera_position().isApprox(before), "settings key must not move camera");
        view.activateWindow();
        view.setFocus();
        wait_events(10);
        QKeyEvent light_down(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier);
        QKeyEvent light_up(QEvent::KeyRelease, Qt::Key_Y, Qt::NoModifier);
        QKeyEvent light_repeat(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(&settings, &light_down);
        require(!model.camera_light_enabled(), "Y in Settings must not toggle light");
        QApplication::sendEvent(&view, &light_down);
        require(model.camera_light_enabled(), "Y enables camera light");
        QApplication::sendEvent(&view, &light_repeat);
        QApplication::sendEvent(&view, &light_up);
        require(model.camera_light_enabled(), "repeat/release must not toggle light");
        QKeyEvent russian_light(QEvent::KeyPress, 0x041D, Qt::NoModifier);
        QApplication::sendEvent(&view, &russian_light);
        require(!model.camera_light_enabled(), "Russian Y toggles light off");
        QApplication::sendEvent(&view, &press);
        wait_events(35);
        require(!model.camera_position().isApprox(before), "WASD works without mouse button");
        QMouseEvent click(QEvent::MouseButtonPress, QPointF(80, 60), QPointF(80, 60),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &click);
        require(view.cursor().shape() != Qt::BlankCursor, "drag cursor remains visible");
        QMouseEvent mouse_up(QEvent::MouseButtonRelease, QPointF(80, 60), QPointF(80, 60),
                             Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &mouse_up);
        const r3d::Vector3 after_mouse_release = model.camera_position();
        wait_events(35);
        require(!model.camera_position().isApprox(after_mouse_release),
                "mouse release must not cancel held WASD");
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&view, &escape);
        const r3d::Vector3 stopped = model.camera_position();
        wait_events(40);
        require(model.camera_position().isApprox(stopped), "escape clears held keys");
        require(view.cursor().shape() != Qt::BlankCursor, "escape restores cursor");

        for (const auto &binding :
             {std::pair{0x0426, r3d::Vector3(0, 0, -1)}, std::pair{0x0424, r3d::Vector3(-1, 0, 0)},
              std::pair{0x042B, r3d::Vector3(0, 0, 1)}, std::pair{0x0412, r3d::Vector3(1, 0, 0)}}) {
            const r3d::Vector3 start = model.camera_position();
            QKeyEvent down(QEvent::KeyPress, binding.first, Qt::NoModifier);
            QKeyEvent up(QEvent::KeyRelease, binding.first, Qt::NoModifier);
            QApplication::sendEvent(&view, &down);
            wait_events(35);
            QApplication::sendEvent(&view, &up);
            require((model.camera_position() - start).dot(binding.second) > 0.01f,
                    "Russian WASD direction");
            const r3d::Vector3 end = model.camera_position();
            wait_events(20);
            require(model.camera_position().isApprox(end), "Russian key release stops movement");
        }
        QApplication::sendEvent(&view, &press);
        QEvent focus_out(QEvent::FocusOut);
        QApplication::sendEvent(&view, &focus_out);
        const r3d::Vector3 focus_position = model.camera_position();
        wait_events(35);
        require(model.camera_position().isApprox(focus_position), "focus loss clears held keys");
        emit settings.load_object_requested("/nonexistent-renderer-test.obj", {});
        require(has_text(&settings, "File was not opened."), "exact load error text");
        emit settings.add_light_requested({r3d::Vector3::UnitZ(), r3d::Color{1, 1, 1}});
        require(has_text(&settings, "Light was added successfully."), "light success");
        auto *number = settings.findChild<QLineEdit *>();
        number->setText("99");
        QMetaObject::invokeMethod(number, "returnPressed", Qt::DirectConnection);
        require(has_text(&settings, "Unknown option."), "numeric menu error");
        auto *objects = settings.findChild<QListWidget *>("objectList");
        auto *remove = settings.findChild<QPushButton *>("removeObjectButton");
        require(objects && objects->count() == 3 && remove && !remove->isEnabled(),
                "default objects listed without implicit deletion selection");
        QTemporaryFile obj;
        require(obj.open(), "temporary OBJ");
        obj.write("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
        obj.flush();
        emit settings.load_object_requested(obj.fileName(), {});
        emit settings.load_object_requested(obj.fileName(), {});
        require(objects->count() == 5, "both duplicate objects listed");
        const auto first_id = model.objects()[3].id;
        const auto second_id = model.objects()[4].id;
        require(first_id != second_id, "duplicate instances have unique IDs");
        objects->setCurrentRow(4);
        remove->click();
        require(model.objects().size() == 4 && model.objects()[3].id == first_id,
                "selecting second duplicate deletes only second instance");
        require(!model.remove_object_by_id(second_id), "deleted ID must not delete another object");
        require(!remove->isEnabled(), "after deletion an explicit new selection is required");
        while (objects->count()) {
            objects->setCurrentRow(0);
            remove->click();
        }
        require(model.objects().empty() && !remove->isEnabled(), "empty scene removal disabled");

        for (const QString kind :
             {QString("Cube"), QString("Sphere"), QString("Pyramid"), QString("OBJ")}) {
            auto *button = settings.findChild<QPushButton *>(
                kind == "OBJ" ? "loadObjButton" : "add" + kind + "Button");
            require(button != nullptr, "visible add button");
            bool filled = false;
            QTimer form_timer;
            form_timer.setInterval(10);
            QObject::connect(&form_timer, &QTimer::timeout, &settings, [&] {
                auto *modal = QApplication::activeModalWidget();
                if (auto *path_dialog = qobject_cast<QInputDialog *>(modal)) {
                    path_dialog->setTextValue(obj.fileName());
                    path_dialog->accept();
                } else if (modal && modal->objectName() == "objectProperties") {
                    auto set = [&](const char *name, double value) {
                        auto *spin = modal->findChild<QDoubleSpinBox *>(name);
                        require(spin != nullptr, "object form field");
                        spin->setValue(value);
                    };
                    set("positionX", 2.5);
                    set("positionY", -1.25);
                    set("positionZ", -3);
                    set("colorR", 255);
                    set("colorG", 0);
                    set("colorB", 0);
                    set("objectOpacity", 0.5);
                    if (kind != "OBJ")
                        set("objectSize", 2);
                    if (kind == "Pyramid")
                        set("objectHeight", 3);
                    if (argc > 1 && kind == "Pyramid")
                        modal->grab().save(QString::fromLocal8Bit(argv[1]) + "-add.png");
                    filled = true;
                    modal->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
                }
            });
            form_timer.start();
            const auto count = model.objects().size();
            button->click();
            form_timer.stop();
            require(filled && model.objects().size() == count + 1, "form adds exactly one object");
            require(objects->count() == int(model.objects().size()),
                    "added object appears in list");
            const auto &mesh = model.objects().back().mesh;
            require(mesh.model_matrix().block<3, 1>(0, 3).isApprox(r3d::Vector3{2.5f, -1.25f, -3}),
                    "form coordinates reach object transform");
            require(mesh.opacity() == 0.5f, "form opacity reaches mesh");
            if (kind != "OBJ") {
                const float width =
                    mesh.positions().row(0).maxCoeff() - mesh.positions().row(0).minCoeff();
                require(std::abs(width - (kind == "Sphere" ? 4.f : 2.f)) < 0.001f,
                        "primitive dimensions reach geometry");
                if (kind == "Pyramid")
                    require(std::abs(mesh.positions().row(1).maxCoeff() - 3.f) < 0.001f,
                            "pyramid height reaches geometry");
            }
        }
        QTimer::singleShot(10, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            require(dialog != nullptr, "cancel form shown");
            dialog->reject();
        });
        const auto before_cancel = model.objects().size();
        settings.findChild<QPushButton *>("addCubeButton")->click();
        require(model.objects().size() == before_cancel, "cancel adds no object");

        int stage = 0;
        QTimer automation;
        automation.setInterval(10);
        QObject::connect(&automation, &QTimer::timeout, &settings, [&stage] {
            if (auto *warning = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                require(warning->text() == "Invalid direction vector.", "zero direction warning");
                warning->accept();
                stage = 2;
            } else if (auto *dialog =
                           qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                if (stage == 0) {
                    dialog->setTextValue("0 0 0");
                    dialog->accept();
                    stage = 1;
                } else if (stage == 2) {
                    require(dialog->labelText().contains("direction"), "must repeat vector prompt");
                    dialog->reject();
                    stage = 3;
                }
            }
        });
        automation.start();
        for (auto *action : settings.findChildren<QAction *>())
            if (action->text() == "Add light") {
                action->trigger();
                break;
            }
        automation.stop();
        require(stage == 3, "invalid input reprompt workflow");
        if (argc > 1) {
            model.load_default_scene();
            settings.show_objects(model.objects());
            model.set_frame_size(r3d::Width{800}, r3d::Height{600});
            view.resize(800, 600);
            view.grab().save(QString::fromLocal8Bit(argv[1]));
            settings.grab().save(QString::fromLocal8Bit(argv[1]) + "-settings.png");
        }
        model.unsubscribe(&view);
        view.hide();
        settings.hide();
        mouse_sampling_test();
        r3d::Application application;
        bool exited = false;
        QTimer::singleShot(80, [&exited] {
            int windows = 0;
            for (auto *window : QApplication::topLevelWidgets())
                if (window->isVisible())
                    ++windows;
            require(windows == 2, "application opens exactly two windows");
            for (auto *window : QApplication::topLevelWidgets()) {
                auto *settings = qobject_cast<r3d::SettingsWindow *>(window);
                if (settings && settings->isVisible()) {
                    exited = true;
                    emit settings->exit_requested();
                }
            }
        });
        require(application.run() == 0 && exited, "Operations Exit ends event loop cleanly");
        std::cout << "PASS: two windows, capture/WASD/Escape, dialog isolation, messages, numeric "
                     "menu, reprompt\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
