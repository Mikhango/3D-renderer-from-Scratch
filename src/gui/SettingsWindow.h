#pragma once
#include "core/Lighting.h"
#include "core/ObjLoader.h"
#include "core/Primitives.h"
#include "core/Scene.h"
#include "core/Settings.h"
#include <QMainWindow>
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace r3d {
class SettingsWindow : public QMainWindow {
    Q_OBJECT
public:
    SettingsWindow();
    void show_fps(double fps);
    void show_camera_light(bool enabled);
    void show_objects(const std::vector<SceneObject> &objects);
    void show_status(const QString &message);
    void show_camera_position(float x, float y, float z);
signals:
    void add_primitive_requested(r3d::primitives::Kind kind, const r3d::ObjectTransform &transform,
                                 float size, float height);
    void load_object_requested(const QString &path, const r3d::ObjectTransform &transform);
    void remove_object_requested(qulonglong id);
    void add_light_requested(const r3d::DirectionalLight &light);
    void add_point_light_requested(const r3d::PointLight &light);
    void ambient_light_requested(const r3d::Color &light);
    void settings_requested(const r3d::RenderSettings &settings);
    void exit_requested();

private:
    void build_ui();
    void on_load_object();
    void on_add_primitive(primitives::Kind kind);
    void on_remove_object();
    void on_add_light();
    void on_add_point_light();
    void on_ambient_light();
    void on_render_settings();
    void on_menu_number();
    QLabel *camera_light_label_ = nullptr;
    QLabel *fps_label_ = nullptr;
    QLabel *status_label_ = nullptr;
    QLabel *camera_label_ = nullptr;
    QLineEdit *menu_number_ = nullptr;
    QListWidget *object_list_ = nullptr;
    QPushButton *remove_button_ = nullptr;
};
} // namespace r3d
