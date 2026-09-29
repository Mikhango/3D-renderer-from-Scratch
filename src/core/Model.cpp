#include "core/Model.h"

#include "core/Primitives.h"
#include "core/Settings.h"
#include <numbers>

#include <string>

namespace r3d {
namespace {

Matrix4 make_translation_matrix(const Vector3 &translation) {
    Matrix4 matrix = Matrix4::Identity();
    matrix.block<3, 1>(0, 3) = translation;
    return matrix;
}

Scene make_default_scene() {
    Scene scene;

    Mesh sphere = primitives::create_sphere(Radius{1.0f}, Stacks{24}, Slices{32});
    sphere.set_color(Color{0.9f, 0.3f, 0.3f});
    sphere.set_model_matrix(make_translation_matrix(Vector3{-1.5f, 0.0f, 0.0f}));
    scene.add_object("sphere", sphere);

    Mesh box = primitives::create_box(primitives::BoxSize{
        .width = ObjectWidth{1.2f}, .height = ObjectHeight{1.2f}, .depth = ObjectDepth{1.2f}});
    box.set_color(Color{0.3f, 0.9f, 0.3f});
    box.set_model_matrix(make_translation_matrix(Vector3{1.5f, 0.0f, 0.0f}));
    scene.add_object("box", box);

    Mesh pyramid = primitives::create_pyramid(
        primitives::PyramidSize{.base = BaseLength{1.4f}, .height = ObjectHeight{1.6f}});
    pyramid.set_color(Color{0.3f, 0.4f, 0.9f});
    pyramid.set_model_matrix(make_translation_matrix(Vector3{0.0f, -0.8f, -2.5f}));
    scene.add_object("pyramid", pyramid);

    scene.add_light(DirectionalLight{Vector3{0.4f, 1.0f, 0.8f}, Color{1.0f, 1.0f, 1.0f}});
    return scene;
}

} // namespace

Model::Model() : renderer_(kDefaultWidth, kDefaultHeight) {
    camera_.set_aspect_ratio(static_cast<float>(to_int(kDefaultWidth)) /
                             static_cast<float>(to_int(kDefaultHeight)));
    load_default_scene();
}

void Model::subscribe(Observer<Frame> *observer) {
    frame_port_.subscribe(observer);
}

void Model::unsubscribe(Observer<Frame> *observer) {
    frame_port_.unsubscribe(observer);
}

void Model::load_default_scene() {
    scene_ = make_default_scene();
    publish_frame();
}

void Model::load_object(const std::string &path, const ObjectTransform &transform) {
    scene_.add_object(path, load_obj(path, transform));
    publish_frame();
}

void Model::add_primitive(primitives::Kind kind, const ObjectTransform &transform, float size,
                          float height) {
    Mesh mesh;
    std::string name;
    switch (kind) {
    case primitives::Kind::Cube:
        mesh = primitives::create_box({ObjectWidth{size}, ObjectHeight{size}, ObjectDepth{size}});
        name = "cube";
        break;
    case primitives::Kind::Sphere:
        mesh = primitives::create_sphere(Radius{size}, Stacks{24}, Slices{32});
        name = "sphere";
        break;
    case primitives::Kind::Pyramid:
        mesh = primitives::create_pyramid({BaseLength{size}, ObjectHeight{height}});
        name = "pyramid";
        break;
    default:
        throw std::invalid_argument("Unknown primitive.");
    }
    apply_object_transform(mesh, transform);
    scene_.add_object(name, std::move(mesh));
    publish_frame();
}

bool Model::remove_object(const std::string &name) {
    const bool removed = scene_.remove_object(name);
    if (removed) {
        publish_frame();
    }
    return removed;
}

const std::vector<SceneObject> &Model::objects() const {
    return scene_.objects();
}

bool Model::remove_object_by_id(std::uint64_t id) {
    const bool removed = scene_.remove_object_by_id(id);
    if (removed)
        publish_frame();
    return removed;
}

void Model::toggle_camera_light() {
    camera_light_enabled_ = !camera_light_enabled_;
    publish_frame();
}

bool Model::camera_light_enabled() const {
    return camera_light_enabled_;
}

void Model::add_light(const DirectionalLight &light) {
    scene_.add_light(light);
    publish_frame();
}

void Model::add_point_light(const PointLight &light) {
    scene_.add_point_light(light);
    publish_frame();
}

void Model::set_ambient_light(const Color &light) {
    scene_.set_ambient_light(light);
    publish_frame();
}

void Model::set_render_settings(const RenderSettings &settings) {
    camera_.set_vertical_fov(settings.vertical_fov_degrees);
    renderer_.set_settings(settings);
    publish_frame();
}

void Model::advance_camera(float forward, float right, float yaw_degrees, float pitch_degrees) {
    constexpr float to_radians = std::numbers::pi_v<float> / 180.0f;
    camera_.yaw(yaw_degrees * to_radians);
    camera_.pitch(pitch_degrees * to_radians);
    camera_.move_forward(forward);
    camera_.move_right(right);
    publish_frame();
}

Vector3 Model::camera_position() const {
    return camera_.position();
}

void Model::move_camera_forward(float distance) {
    camera_.move_forward(distance);
    publish_frame();
}

void Model::move_camera_backward(float distance) {
    camera_.move_backward(distance);
    publish_frame();
}

void Model::move_camera_left(float distance) {
    camera_.move_left(distance);
    publish_frame();
}

void Model::move_camera_right(float distance) {
    camera_.move_right(distance);
    publish_frame();
}

void Model::pan_camera(float dx, float dy) {
    camera_.pan(dx, dy);
    publish_frame();
}

void Model::rotate_camera(float yaw_degrees, float pitch_degrees) {
    constexpr float kDegreesToRadians = std::numbers::pi_v<float> / 180.0f;
    camera_.yaw(yaw_degrees * kDegreesToRadians);
    camera_.pitch(pitch_degrees * kDegreesToRadians);
    publish_frame();
}

void Model::set_frame_size(Width width, Height height) {
    renderer_.resize(width, height);
    camera_.set_aspect_ratio(static_cast<float>(to_int(width)) /
                             static_cast<float>(to_int(height)));
    publish_frame();
}

void Model::publish_frame() {
    frame_port_.set(renderer_.render(scene_, camera_, camera_light_enabled_));
}

} // namespace r3d
