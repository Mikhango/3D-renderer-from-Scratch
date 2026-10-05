#pragma once

#include "core/Model.h"

#include <memory>

class QWidget;

namespace r3d {

class RenderView;
class SettingsWindow;
class InputController;
class GuiController;

class Application {
public:
    Application();
    ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    int run();

private:
    void build_views();
    void build_controllers();
    void connect_views();

    Model model_;
    std::unique_ptr<RenderView> render_view_;
    std::unique_ptr<SettingsWindow> settings_window_;
    std::unique_ptr<InputController> input_controller_;
    std::unique_ptr<GuiController> gui_controller_;
};

} // namespace r3d
