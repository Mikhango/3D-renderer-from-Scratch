#include "core/ObjLoader.h"
#include <array>
#include <fstream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace r3d {
namespace {
[[noreturn]] void parsing_error() {
    throw std::runtime_error("Parsing error.");
}

Index parse_index(const std::string &token, std::size_t count) {
    try {
        std::size_t used = 0;
        const long long raw = std::stoll(token, &used);
        if (used != token.size() || raw == 0)
            parsing_error();
        const auto size = static_cast<long long>(count);
        if (raw > size || raw < -size)
            parsing_error();
        return static_cast<Index>(raw > 0 ? raw - 1 : size + raw);
    } catch (const std::logic_error &) {
        parsing_error();
    }
}

struct Corner {
    Index vertex;
    Index normal = -1;
};

Corner parse_corner(const std::string &token, std::size_t vertices, std::size_t textures,
                    std::size_t normals) {
    const auto first = token.find('/');
    Corner corner{parse_index(token.substr(0, first), vertices)};
    if (first == std::string::npos)
        return corner;
    const auto second = token.find('/', first + 1);
    if (second == std::string::npos) {
        parse_index(token.substr(first + 1), textures);
        return corner;
    }
    if (token.find('/', second + 1) != std::string::npos)
        parsing_error();
    if (second > first + 1)
        parse_index(token.substr(first + 1, second - first - 1), textures);
    corner.normal = parse_index(token.substr(second + 1), normals);
    return corner;
}

Vector3 read_vector(std::istringstream *stream) {
    Vector3 value;
    if (!(*stream >> value.x() >> value.y() >> value.z()) || !value.allFinite())
        parsing_error();
    return value;
}

float cross2(const Vector2 &a, const Vector2 &b, const Vector2 &c) {
    return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
}

std::vector<std::array<Corner, 3>> triangulate(const std::vector<Corner> &polygon,
                                               const std::vector<Vector3> &vertices) {
    if (polygon.size() < 3)
        parsing_error();
    Vector3 normal = Vector3::Zero();
    for (std::size_t i = 0; i < polygon.size(); ++i)
        normal +=
            vertices[polygon[i].vertex].cross(vertices[polygon[(i + 1) % polygon.size()].vertex]);
    if (normal.norm() < 1e-8f)
        parsing_error();
    Index axis = 0;
    normal.cwiseAbs().maxCoeff(&axis);
    std::vector<Vector2> projected;
    std::vector<std::size_t> remaining;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto &v = vertices[polygon[i].vertex];
        projected.emplace_back(v[(axis + 1) % 3], v[(axis + 2) % 3]);
        remaining.push_back(i);
    }
    const float sign = normal[axis] > 0 ? 1.0f : -1.0f;
    std::vector<std::array<Corner, 3>> result;
    while (remaining.size() > 3) {
        bool found = false;
        for (std::size_t i = 0; i < remaining.size(); ++i) {
            const auto a = remaining[(i + remaining.size() - 1) % remaining.size()],
                       b = remaining[i], c = remaining[(i + 1) % remaining.size()];
            if (sign * cross2(projected[a], projected[b], projected[c]) <= 1e-8f)
                continue;
            bool contains = false;
            for (auto j : remaining) {
                if (j == a || j == b || j == c)
                    continue;
                if (sign * cross2(projected[a], projected[b], projected[j]) >= 0 &&
                    sign * cross2(projected[b], projected[c], projected[j]) >= 0 &&
                    sign * cross2(projected[c], projected[a], projected[j]) >= 0) {
                    contains = true;
                    break;
                }
            }
            if (contains)
                continue;
            result.push_back({polygon[a], polygon[b], polygon[c]});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            found = true;
            break;
        }
        if (!found)
            parsing_error();
    }
    result.push_back({polygon[remaining[0]], polygon[remaining[1]], polygon[remaining[2]]});
    return result;
}

Matrix4 make_model_matrix(const ObjectTransform &transform) {
    if (!transform.rotation_axis.allFinite() || transform.rotation_axis.stableNorm() < 1e-6f)
        throw std::invalid_argument("Invalid axis of rotation.");
    if (!std::isfinite(transform.rotation_angle_degrees))
        throw std::invalid_argument("Invalid angle of rotation.");
    if (!transform.translation.allFinite())
        throw std::invalid_argument("Invalid translation vector.");
    Matrix4 matrix = Matrix4::Identity();
    matrix.topLeftCorner<3, 3>() =
        Eigen::AngleAxisf(std::remainder(transform.rotation_angle_degrees, 360.0f) *
                              std::numbers::pi_v<float> / 180.0f,
                          transform.rotation_axis.stableNormalized())
            .toRotationMatrix();
    matrix.topRightCorner<3, 1>() = transform.translation;
    return matrix;
}
} // namespace

void apply_object_transform(Mesh &mesh, const ObjectTransform &transform) {
    mesh.set_model_matrix(make_model_matrix(transform));
    mesh.set_color(transform.color);
    mesh.set_opacity(transform.opacity);
}

Mesh load_obj(const std::string &path, const ObjectTransform &transform) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("File was not opened.");
    std::vector<Vector3> vertices, normals;
    std::size_t textures = 0;
    std::vector<std::array<Corner, 3>> triangles;
    std::string line;
    while (std::getline(file, line)) {
        line = line.substr(0, line.find('#'));
        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag == "v") {
            Vector3 vertex = read_vector(&stream);
            float w = 1;
            stream >> std::ws;
            if (!stream.eof() && (!(stream >> w) || !std::isfinite(w) || w == 0))
                parsing_error();
            vertex /= w;
            if (!vertex.allFinite())
                parsing_error();
            vertices.push_back(vertex);
        } else if (tag == "vn") {
            const Vector3 normal = read_vector(&stream);
            if (normal.stableNorm() < 1e-6f)
                parsing_error();
            normals.push_back(normal.stableNormalized());
        } else if (tag == "vt") {
            float value = 0;
            int count = 0;
            while (stream >> value) {
                if (!std::isfinite(value))
                    parsing_error();
                ++count;
            }
            if (!stream.eof() || count < 1 || count > 3)
                parsing_error();
            ++textures;
        } else if (tag == "f") {
            std::vector<Corner> polygon;
            std::string token;
            while (stream >> token)
                polygon.push_back(parse_corner(token, vertices.size(), textures, normals.size()));
            const auto faces = triangulate(polygon, vertices);
            triangles.insert(triangles.end(), faces.begin(), faces.end());
        }
        if (tag == "v" || tag == "vn") {
            std::string extra;
            if (stream >> extra)
                parsing_error();
        }
    }
    if (file.bad() || triangles.empty())
        parsing_error();
    std::vector<Vector3> generated(vertices.size(), Vector3::Zero());
    for (const auto &t : triangles) {
        const Vector3 normal = (vertices[t[1].vertex] - vertices[t[0].vertex])
                                   .cross(vertices[t[2].vertex] - vertices[t[0].vertex]);
        if (!normal.allFinite() || normal.norm() < 1e-8f)
            parsing_error();
        for (const auto &corner : t)
            generated[corner.vertex] += normal;
    }
    Mesh mesh;
    mesh.reserve_vertices(static_cast<Index>(triangles.size() * 3));
    for (const auto &t : triangles) {
        const Index base = mesh.positions().cols();
        for (const auto &corner : t)
            mesh.add_vertex(vertices[corner.vertex], corner.normal >= 0
                                                         ? normals[corner.normal]
                                                         : generated[corner.vertex].normalized());
        mesh.add_face(base, base + 1, base + 2);
    }
    apply_object_transform(mesh, transform);
    return mesh;
}
} // namespace r3d
