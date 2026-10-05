#include "core/Camera.h"
#include "core/Frame.h"
#include "core/Lighting.h"
#include "core/Mesh.h"
#include "core/ObjLoader.h"
#include "core/Observer.h"
#include "core/Primitives.h"
#include "core/Rasterizer.h"
#include "core/Renderer.h"
#include "core/ShadowScene.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace {
using namespace r3d;

void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

void near(const Vector3 &a, const Vector3 &b, const char *message) {
    require((a - b).norm() < 0.001f, message);
}

template <class F> void rejects(F fn, const char *message) {
    try {
        fn();
    } catch (const std::exception &) {
        return;
    }
    throw std::runtime_error(message);
}

struct TempFile {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("renderer-test-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj");

    ~TempFile() { std::filesystem::remove(path); }

    Mesh load(const std::string &text, const ObjectTransform &transform = {}) const {
        std::ofstream(path) << text;
        return load_obj(path.string(), transform);
    }
};

Mesh triangle(float z = 0) {
    Mesh mesh;
    mesh.add_vertex(Vector3(-1, -1, z), Vector3::UnitZ());
    mesh.add_vertex(Vector3(1, -1, z), Vector3::UnitZ());
    mesh.add_vertex(Vector3(0, 1, z), Vector3::UnitZ());
    mesh.add_face(0, 1, 2);
    return mesh;
}

Color pixel(const Frame &frame, int x, int y) {
    const auto i = (y * to_int(frame.width()) + x) * 4;
    return Color{frame.rgba()[i] / 255.f, frame.rgba()[i + 1] / 255.f, frame.rgba()[i + 2] / 255.f};
}

void camera_test() {
    Camera camera;
    const auto initial = camera.position();
    camera.yaw(std::numbers::pi_v<float> / 2);
    near(camera.position(), initial, "yaw must rotate in place");
    camera.pitch(0.5f);
    near(camera.position(), initial, "pitch must rotate in place");
    const Vector3 right = camera.right();
    camera.move_right(2);
    near(camera.position(), initial + 2 * right, "strafe after rotation");
    const auto before = camera.position();
    const Vector3 direction = camera.forward().normalized();
    camera.move_forward(2);
    near(camera.position(), before + 2 * direction, "forward follows yaw and pitch");
    require(camera.position().y() > before.y(), "looking up moves upward");
    camera.move_backward(2);
    near(camera.position(), before, "backward reverses forward along view direction");
    camera.pitch(-1.0f);
    const Vector3 downward = camera.forward().normalized();
    camera.move_forward(2);
    near(camera.position(), before + 2 * downward, "forward follows downward pitch");
    require(camera.position().y() < before.y(), "looking down moves downward");
    camera.move_backward(2);
    near(camera.position(), before, "backward reverses downward movement");
    camera.reset();
    camera.pitch(0.1f);
    Camera other;
    other.pitch(0.1f);
    require(camera.view_matrix().isApprox(other.view_matrix()), "reset clears pitch");
    rejects([&] { camera.set_aspect_ratio(0); }, "invalid aspect");
}

void obj_test() {
    TempFile f;
    const std::string base = "v -1 -1 0\nv 1 -1 0\nv 0 1 0\n";
    auto mesh = f.load(base + "vn 0 0 -1\nf 1//1 2//1 3//1 # comment\n");
    require(mesh.normals()(2, 0) == -1, "preserve supplied normals");
    require(f.load(base + "f -3 -2 -1\n").faces().size() == 1, "relative indices");
    for (const auto &face :
         {"f 1garbage 2 3", "f 0 2 3", "f 4 2 3", "f 1//x 2 3", "f 1/1 2/1 3/1", "f 1 2"})
        rejects([&] { f.load(base + face + "\n"); }, "malformed OBJ accepted");
    require(f.load(base + "vt 0 0\nf 1/1 2/1 3/1\n").faces().size() == 1, "texture index syntax");
    require(f.load("v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 1 0\nv 0 2 0\nf 1 2 3 4 5\n").faces().size() ==
                3,
            "concave triangulation");
    ObjectTransform transform;
    transform.translation = Vector3(2, 3, 4);
    transform.rotation_angle_degrees = 90;
    mesh = f.load(base + "f 1 2 3\n", transform);
    near(mesh.model_matrix().topRightCorner<3, 1>(), transform.translation, "OBJ translation");
    transform.rotation_axis.setZero();
    rejects([&] { f.load(base + "f 1 2 3\n", transform); }, "zero rotation axis");
    rejects([&] { f.load("v nan 0 0\nf 1 1 1\n"); }, "nonfinite OBJ");
    rejects([&] { load_obj(f.path.string() + "-absent", {}); }, "missing file");
}

void lighting_test() {
    const Color base{1, 1, 1};
    const Material material;
    const auto ambient =
        blinn_phong({}, base, material, Vector3(0, 0, -1), Vector3::UnitZ(), Vector3::Zero(), base);
    require(std::abs(ambient.r - material.ambient) < 1e-5, "ambient without direct lights");
    const DirectionalLight zero{Vector3::UnitZ(), Color{0, 0, 0}};
    const auto same = blinn_phong({zero, zero}, base, material, Vector3(0, 0, -1), Vector3::UnitZ(),
                                  Vector3::Zero(), base);
    require(same.r == ambient.r, "ambient independent of light count");
    Scene scene;
    rejects([&] { scene.add_light({Vector3::Zero(), base}); }, "zero light direction");
    rejects([&] { scene.add_point_light({Vector3::Zero(), Color{2, 0, 0}}); },
            "invalid light intensity");
}

void shadow_test() {
    Scene scene;
    scene.add_object("occluder", triangle(-2));
    ShadowScene shadows(scene, Matrix4::Identity());
    require(shadows.visibility(Vector3(0, 0, -4), Vector3::UnitZ(), 10) == 0,
            "shadow ray must hit");
    require(shadows.visibility(Vector3(0, 0, -4), Vector3::UnitZ(), 1) == 1,
            "point light before occluder");
    require(shadows.visibility(Vector3(4, 0, -4), Vector3::UnitZ(), 10) == 1, "unblocked ray");
    require(shadows.visibility(Vector3(0, 0, -2), Vector3::UnitZ(), 10) == 1, "self shadow bias");
    auto glass = triangle(-2);
    glass.set_opacity(0.5f);
    Scene transparent;
    transparent.add_object("glass", glass);
    require(std::abs(ShadowScene(transparent, Matrix4::Identity())
                         .visibility(Vector3(0, 0, -4), Vector3::UnitZ(), 10) -
                     0.5f) < 1e-5,
            "translucent shadow");
}

void raster_test() {
    constexpr Width width{9};
    constexpr Height height{9};
    Rasterizer raster(width, height);
    Frame frame(width, height, Color{});
    ShadingContext shade;
    shade.ambient_light = Color{1, 1, 1};
    shade.material.ambient = 1;
    shade.eye_position = Vector3::Zero();
    const Vector2 screen[3] = {{0, 0}, {0, 8}, {8, 0}};
    const Vector3 tilted[3] = {{0, 0, -1}, {0, 1, -10}, {1, 0, -10}};
    const Vector3 flat[3] = {{0, 0, -3}, {0, 1, -3}, {1, 0, -3}};
    const Vector3 normals[3] = {Vector3::UnitZ(), Vector3::UnitZ(), Vector3::UnitZ()};
    raster.begin_frame();
    raster.draw_triangle(ScreenTriangle(screen, tilted, normals, Color{1, 0, 0}), shade, &frame);
    raster.draw_triangle(ScreenTriangle(screen, flat, normals, Color{0, 1, 0}), shade, &frame);
    raster.finish_frame(&frame);
    require(pixel(frame, 2, 2).r > 0.9f, "perspective depth: tilted red must cover flat green");
    frame = Frame(width, height, Color{});
    raster.begin_frame();
    raster.draw_triangle(ScreenTriangle(screen, flat, normals, Color{0, 1, 0}), shade, &frame);
    raster.draw_triangle(ScreenTriangle(screen, tilted, normals, Color{1, 0, 0}, 0.5f), shade,
                         &frame);
    raster.finish_frame(&frame);
    const auto blended = pixel(frame, 2, 2);
    require(blended.r > 0.45f && blended.g > 0.45f, "transparent compositing");
    Frame reverse(width, height, Color{});
    raster.begin_frame();
    raster.draw_triangle(ScreenTriangle(screen, tilted, normals, Color{1, 0, 0}, 0.5f), shade,
                         &reverse);
    raster.draw_triangle(ScreenTriangle(screen, flat, normals, Color{0, 1, 0}), shade, &reverse);
    raster.finish_frame(&reverse);
    require(pixel(reverse, 2, 2).r == blended.r && pixel(reverse, 2, 2).g == blended.g,
            "transparency order independence");
}

void pipeline_test() {
    Camera camera;
    Renderer renderer(Width{64}, Height{64});
    Scene scene;
    auto mesh = triangle();
    mesh.set_color(Color{1, 0, 0});
    scene.add_object("triangle", mesh);
    auto frame = renderer.render(scene, camera);
    require(pixel(frame, 32, 32).r > 0.15f, "pipeline renders visible triangle");
    renderer.set_settings({RenderMode::Normals, ShadowMode::Disabled});
    frame = renderer.render(scene, camera);
    require(pixel(frame, 32, 32).b > 0.95f, "normal debug mode");
    renderer.set_settings({RenderMode::Depth, ShadowMode::Disabled});
    frame = renderer.render(scene, camera);
    require(pixel(frame, 32, 32).r == pixel(frame, 32, 32).g, "depth debug mode");
    Scene huge;
    Mesh covering;
    covering.add_vertex(Vector3(-100, -100, 0), Vector3::UnitZ());
    covering.add_vertex(Vector3(100, -100, 0), Vector3::UnitZ());
    covering.add_vertex(Vector3(0, 100, 0), Vector3::UnitZ());
    covering.add_face(0, 1, 2);
    huge.add_object("cover", covering);
    renderer.set_settings({RenderMode::Normals, ShadowMode::Disabled});
    frame = renderer.render(huge, camera);
    require(pixel(frame, 32, 32).b > 0.95f, "all vertices outside but triangle intersects frustum");
    require(scene.remove_object("triangle") && !scene.remove_object("triangle"), "remove object");
    Scene behind;
    behind.add_object("behind", triangle(10));
    frame = renderer.render(behind, camera);
    require(pixel(frame, 32, 32).b < 0.2f, "reject geometry behind eye");
    Scene points;
    points.add_object("triangle", mesh);
    points.set_ambient_light(Color{});
    points.add_point_light({Vector3(0, 0, 3), Color{1, 1, 1}});
    renderer.set_settings({});
    require(pixel(renderer.render(points, camera), 32, 32).r > 0.1f, "point light shading");
}

void integrated_shadow_test() {
    Camera camera;
    Renderer renderer(Width{64}, Height{64});
    Scene scene;
    scene.set_ambient_light(Color{0.2f, 0.2f, 0.2f});
    scene.add_object("receiver", triangle());
    Mesh blocker = triangle(1);
    Matrix4 placement = Matrix4::Identity();
    placement(0, 3) = 1;
    blocker.set_model_matrix(placement);
    scene.add_object("blocker", blocker);
    scene.add_light({Vector3(1, 0, 1), Color{1, 1, 1}});
    const auto shadowed = pixel(renderer.render(scene, camera), 32, 32);
    renderer.set_settings({RenderMode::Shaded, ShadowMode::Disabled});
    const auto lit = pixel(renderer.render(scene, camera), 32, 32);
    require(lit.r - shadowed.r > 0.1f, "shadow setting changes receiving surface brightness");
}

void shared_edge_test() {
    Rasterizer raster(Width{4}, Height{4});
    Frame frame(Width{4}, Height{4}, Color{});
    ShadingContext shading;
    shading.ambient_light = Color{1, 1, 1};
    shading.material.ambient = 1;
    const Vector2 first[3] = {{0, 0}, {0, 4}, {4, 0}}, second[3] = {{4, 0}, {0, 4}, {4, 4}};
    const Vector3 positions[3] = {{0, 0, -1}, {0, 1, -1}, {1, 0, -1}};
    const Vector3 normals[3] = {Vector3::UnitZ(), Vector3::UnitZ(), Vector3::UnitZ()};
    raster.begin_frame();
    raster.draw_triangle(ScreenTriangle(first, positions, normals, Color{1, 0, 0}, 0.5f), shading,
                         &frame);
    raster.draw_triangle(ScreenTriangle(second, positions, normals, Color{1, 0, 0}, 0.5f), shading,
                         &frame);
    raster.finish_frame(&frame);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            require(std::abs(pixel(frame, x, y).r - 0.5f) < 0.01f,
                    "shared edge alpha must be applied exactly once");
        }
}

void clipping_and_winding_test() {
    Camera camera;
    Renderer renderer(Width{32}, Height{32});
    renderer.set_settings({RenderMode::Normals, ShadowMode::Disabled});

    const Matrix4 inverse = camera.view_matrix().inverse();
    const std::array<std::array<Vector3, 3>, 6> triangles = {
        {{Vector3(-8, -1, -2), Vector3(1, -1, -2), Vector3(0, 1, -2)},
         {Vector3(-1, -1, -2), Vector3(8, -1, -2), Vector3(0, 1, -2)},
         {Vector3(-1, -8, -2), Vector3(1, -1, -2), Vector3(0, 1, -2)},
         {Vector3(-1, -1, -2), Vector3(1, -1, -2), Vector3(0, 8, -2)},
         {Vector3(-1, -1, -2), Vector3(1, -1, -2), Vector3(0, 0.05f, -0.05f)},
         {Vector3(-1, -1, -2), Vector3(1, -1, -2), Vector3(0, 100, -150)}}};
    for (const auto &vertices : triangles) {
        Mesh mesh;
        for (const auto &v : vertices) {
            const Vector4 world = inverse * Vector4(v.x(), v.y(), v.z(), 1);
            mesh.add_vertex(world.head<3>(), Vector3::UnitZ());
        }
        mesh.add_face(0, 1, 2);
        Scene scene;
        scene.add_object("clipped", mesh);
        const auto frame = renderer.render(scene, camera);
        int pixels = 0;
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x)
                if (pixel(frame, x, y).b > 0.9f)
                    ++pixels;
        require(pixels > 0, "triangle crossing frustum plane must survive clipping");
    }
    for (const auto &mesh : {primitives::create_box({}),
                             primitives::create_sphere(Radius{1}, Stacks{8}, Slices{12})}) {
        for (const auto &face : mesh.faces()) {
            const Vector3 a = mesh.positions().col(face.a).head<3>(),
                          b = mesh.positions().col(face.b).head<3>(),
                          c = mesh.positions().col(face.c).head<3>();
            const Vector3 normal = (b - a).cross(c - a);
            if (normal.norm() > 1e-6f)
                require(normal.dot(a + b + c) > 0, "primitive winding points outward");
        }
    }
}

void camera_light_test() {
    Scene scene;
    scene.set_ambient_light(Color{0, 0, 0});
    scene.add_object("sphere", primitives::create_sphere(Radius{1}, Stacks{12}, Slices{16}));
    Camera camera;
    Renderer renderer(Width{80}, Height{80});
    for (int step = 0; step < 3; ++step) {
        const auto dark = renderer.render(scene, camera);
        const auto lit = renderer.render(scene, camera, true);
        require(std::memcmp(dark.rgba(), lit.rgba(), 80 * 80 * 4) != 0,
                "camera light illuminates visible geometry");
        Scene reference = scene;
        reference.add_point_light({camera.position(), Color{1, 1, 1}});
        const auto expected = renderer.render(reference, camera);
        for (int i = 0; i < 80 * 80 * 4; ++i)
            require(std::abs(int(lit.rgba()[i]) - int(expected.rgba()[i])) <= 1,
                    "camera light matches a point light at current camera position");
        const auto off = renderer.render(scene, camera, false);
        require(std::memcmp(dark.rgba(), off.rgba(), 80 * 80 * 4) == 0,
                "disabling camera light restores lighting without persistent sources");
        camera.move_right(0.2f);
        camera.move_forward(0.3f);
        camera.yaw(0.03f);
        camera.pitch(0.02f);
    }
}

void invariants_test() {
    rejects([] { Frame frame(Width{-1}, Height{1}, Color{}); }, "negative frame size");
    ZBuffer depth(Width{2}, Height{2});
    require(depth.test_and_write(0, 0, -10), "first z write");
    require(depth.test_and_write(0, 0, -2) && !depth.test_and_write(0, 0, -3),
            "closer z comparison");
    rejects([&] { depth.resize(Width{0}, Height{2}); }, "invalid resize");
    require(depth.is_visible(1, 1, -1), "failed resize preserves storage");
    Mesh mesh;
    rejects([&] { mesh.add_face(0, 1, 2); }, "mesh indices checked");
    rejects([&] { mesh.set_model_matrix(Matrix4::Zero()); }, "singular transform");
    rejects([] { primitives::create_sphere(Radius{0}, Stacks{0}, Slices{0}); }, "bad sphere");

    struct Counter : Observer<int> {
        int seen = 0;

        void on_next(const int &value) override { seen = value; }
    } observer;

    ObservableData<int> port;
    rejects([&] { port.value(); }, "empty observer value");
    port.set(7);
    port.subscribe(&observer);
    require(observer.seen == 7, "subscription replays value");
    port.unsubscribe(&observer);
    port.set(8);
    require(observer.seen == 7, "unsubscribe");
}
} // namespace

int main() {
    try {
        camera_test();
        obj_test();
        lighting_test();
        shadow_test();
        raster_test();
        pipeline_test();
        integrated_shadow_test();
        shared_edge_test();
        clipping_and_winding_test();
        invariants_test();
        camera_light_test();
        std::cout << "PASS: camera, OBJ, lighting, shadows, perspective/depth/alpha, "
                     "pipeline/debug/clipping, invariants\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
