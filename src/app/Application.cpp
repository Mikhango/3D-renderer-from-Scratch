#include "app/Application.h"

#include "gui/GuiController.h"
#include "gui/InputController.h"
#include "gui/RenderView.h"
#include "gui/SettingsWindow.h"

#include <QApplication>
#include <QTimer>

namespace r3d {

Application::Application() {
    build_views();
    build_controllers();
    connect_views();
}

Application::~Application() {
    model_.unsubscribe(render_view_.get());
}

int Application::run() {
    render_view_->show();
    settings_window_->show();

    QTimer::singleShot(0, render_view_.get(), [this] {
        render_view_->activateWindow();
        render_view_->setFocus();
    });
    return QApplication::exec();
}

void Application::build_views() {
    render_view_ = std::make_unique<RenderView>();

    settings_window_ = std::make_unique<SettingsWindow>();

    model_.subscribe(render_view_.get());
}

void Application::build_controllers() {
    input_controller_ = std::make_unique<InputController>(&model_, render_view_.get());
    gui_controller_ = std::make_unique<GuiController>(&model_, settings_window_.get());
}

void Application::connect_views() {
    QObject::connect(input_controller_.get(), &InputController::camera_light_changed,
                     settings_window_.get(), &SettingsWindow::show_camera_light);
    QObject::connect(render_view_.get(), &RenderView::view_resized, input_controller_.get(),
                     &InputController::on_resize);
    QObject::connect(input_controller_.get(), &InputController::camera_position_changed,
                     settings_window_.get(), &SettingsWindow::show_camera_position);
    QObject::connect(render_view_.get(), &RenderView::fps_reported, settings_window_.get(),
                     &SettingsWindow::show_fps);
}

} // namespace r3d
