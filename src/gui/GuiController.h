#pragma once
#include "core/Lighting.h"
#include "core/ObjLoader.h"
#include "core/Settings.h"
#include <QObject>

namespace r3d {
class Model;
class SettingsWindow;

class GuiController : public QObject {
    Q_OBJECT
public:
    GuiController(Model *model, SettingsWindow *window);

private:
    void on_load_object(const QString &path, const ObjectTransform &transform);
    void on_remove_object(qulonglong id);
    void on_add_light(const DirectionalLight &light);
    void on_add_point_light(const PointLight &light);
    void on_ambient_light(const Color &light);
    void on_settings(const RenderSettings &settings);
    Model *model_;
    SettingsWindow *window_;
};
} // namespace r3d
