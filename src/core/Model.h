#pragma once

#include "core/Camera.h"
#include "core/Frame.h"
#include "core/Lighting.h"
#include "core/Mesh.h"
#include "core/ObjLoader.h"
#include "core/Observer.h"
#include "core/Primitives.h"
#include "core/Renderer.h"
#include "core/Scene.h"
#include "core/Settings.h"
#include "core/Types.h"

#include <string>

namespace r3d {

class Model {
public:
    Model();

    void subscribe(Observer<Frame> *observer);
    void unsubscribe(Observer<Frame> *observer);

    void load_default_scene();
    void add_primitive(primitives::Kind kind, const ObjectTransform &transform, float size,
                       float height);
    void load_object(const std::string &path, const ObjectTransform &transform);

    bool remove_object(const std::string &name);
    bool remove_object_by_id(std::uint64_t id);
    const std::vector<SceneObject> &objects() const;
    void toggle_camera_light();
    bool camera_light_enabled() const;
    void add_light(const DirectionalLight &light);
    void add_point_light(const PointLight &light);
    void set_ambient_light(const Color &light);
    void set_render_settings(const RenderSettings &settings);
    void advance_camera(float forward, float right, float yaw_degrees, float pitch_degrees);
    Vector3 camera_position() const;
    void move_camera_forward(float distance);
    void move_camera_backward(float distance);
    void move_camera_left(float distance);
    void move_camera_right(float distance);
    void pan_camera(float dx, float dy);
    void rotate_camera(float yaw_degrees, float pitch_degrees);
    void set_frame_size(Width width, Height height);

private:
    void publish_frame();

    bool camera_light_enabled_ = false;
    Scene scene_;
    Camera camera_;
    Renderer renderer_;
    ObservableData<Frame> frame_port_;
};

} // namespace r3d
