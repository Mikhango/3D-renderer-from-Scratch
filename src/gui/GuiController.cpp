#include "gui/GuiController.h"
#include "core/Model.h"
#include "gui/SettingsWindow.h"
#include <QApplication>
#include <stdexcept>

namespace r3d {
GuiController::GuiController(Model *model, SettingsWindow *window)
    : model_(model), window_(window) {
    window_->show_objects(model_->objects());
    connect(
        window_, &SettingsWindow::add_primitive_requested, this,
        [this](primitives::Kind kind, const ObjectTransform &transform, float size, float height) {
            try {
                model_->add_primitive(kind, transform, size, height);
                window_->show_objects(model_->objects());
                window_->show_status("Object was added successfully.");
            } catch (const std::invalid_argument &error) {
                window_->show_status(QString::fromUtf8(error.what()));
            }
        });
    connect(window_, &SettingsWindow::load_object_requested, this, &GuiController::on_load_object);
    connect(window_, &SettingsWindow::remove_object_requested, this,
            &GuiController::on_remove_object);
    connect(window_, &SettingsWindow::add_light_requested, this, &GuiController::on_add_light);
    connect(window_, &SettingsWindow::add_point_light_requested, this,
            &GuiController::on_add_point_light);
    connect(window_, &SettingsWindow::ambient_light_requested, this,
            &GuiController::on_ambient_light);
    connect(window_, &SettingsWindow::settings_requested, this, &GuiController::on_settings);
    connect(window_, &SettingsWindow::exit_requested, qApp, &QApplication::quit);
}

void GuiController::on_load_object(const QString &path, const ObjectTransform &transform) {
    try {
        model_->load_object(path.toStdString(), transform);
        window_->show_objects(model_->objects());
        window_->show_status("Object was loaded successfully.");
    } catch (const std::runtime_error &error) {
        window_->show_status(QString::fromUtf8(error.what()));
    } catch (const std::invalid_argument &error) {
        window_->show_status(QString::fromUtf8(error.what()));
    }
}

void GuiController::on_remove_object(qulonglong id) {
    window_->show_status(model_->remove_object_by_id(id) ? "Object was removed successfully."
                                                         : "Object was not found.");
    window_->show_objects(model_->objects());
}

void GuiController::on_add_light(const DirectionalLight &light) {
    model_->add_light(light);
    window_->show_status("Light was added successfully.");
}

void GuiController::on_add_point_light(const PointLight &light) {
    model_->add_point_light(light);
    window_->show_status("Light was added successfully.");
}

void GuiController::on_ambient_light(const Color &light) {
    model_->set_ambient_light(light);
    window_->show_status("Ambient light was updated successfully.");
}

void GuiController::on_settings(const RenderSettings &settings) {
    model_->set_render_settings(settings);
}
} // namespace r3d
