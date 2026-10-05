#include "gui/SettingsWindow.h"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <optional>
#include <sstream>

namespace r3d {
namespace {
std::optional<QString> ask_text(QWidget *parent, const char *prompt, const QString &initial = {}) {
    bool ok = false;
    const auto text =
        QInputDialog::getText(parent, "Operations", prompt, QLineEdit::Normal, initial, &ok);
    return ok ? std::optional<QString>(text) : std::nullopt;
}

std::optional<float> ask_number(QWidget *parent, const char *prompt, const char *error, float low,
                                float high, float initial = 0) {
    for (;;) {
        const auto text = ask_text(parent, prompt, QString::number(initial));
        if (!text)
            return std::nullopt;
        bool ok = false;
        const float value = text->toFloat(&ok);
        if (ok && std::isfinite(value) && value >= low && value <= high)
            return value;
        QMessageBox::warning(parent, "Operations", error);
    }
}

struct ObjectInput {
    ObjectTransform transform;
    float size = 1;
    float height = 1.5f;
};

std::optional<ObjectInput> ask_object(QWidget *parent, const QString &title,
                                      std::optional<primitives::Kind> kind = std::nullopt) {
    QDialog dialog(parent);
    dialog.setObjectName("objectProperties");
    dialog.setWindowTitle(title);
    auto *form = new QFormLayout(&dialog);
    auto *hint =
        new QLabel(kind == primitives::Kind::Pyramid
                       ? "World position of base center. X: right, Y: up, Z: depth."
                   : kind ? "World position of center. X: right, Y: up, Z: depth."
                          : "World offset from OBJ coordinates. X: right, Y: up, Z: depth.",
                   &dialog);
    hint->setWordWrap(true);
    form->addRow(hint);
    auto field = [&](const char *name, const char *label, double value, double low, double high) {
        auto *spin = new QDoubleSpinBox(&dialog);
        spin->setObjectName(name);
        spin->setDecimals(3);
        spin->setRange(low, high);
        spin->setSingleStep(0.1);
        spin->setValue(value);
        form->addRow(label, spin);
        return spin;
    };
    auto *x = field("positionX", "X", 0, -1000000, 1000000);
    auto *y = field("positionY", "Y", 0, -1000000, 1000000);
    auto *z = field("positionZ", "Z", 0, -1000000, 1000000);
    QDoubleSpinBox *size = nullptr, *height = nullptr;
    if (kind) {
        const char *label = *kind == primitives::Kind::Sphere ? "Radius"
                            : *kind == primitives::Kind::Cube ? "Edge length"
                                                              : "Base edge length";
        size = field("objectSize", label, 1, 0.001, 10000);
        if (*kind == primitives::Kind::Pyramid)
            height = field("objectHeight", "Height", 1.5, 0.001, 10000);
    }
    auto *angle = field("rotationAngle", "Rotation (degrees)", 0, -360000, 360000);
    auto *ax = field("axisX", "Rotation axis X", 0, -1000000, 1000000);
    auto *ay = field("axisY", "Rotation axis Y", 1, -1000000, 1000000);
    auto *az = field("axisZ", "Rotation axis Z", 0, -1000000, 1000000);
    auto *red = field("colorR", "Red (0-255)", 179, 0, 255);
    auto *green = field("colorG", "Green (0-255)", 179, 0, 255);
    auto *blue = field("colorB", "Blue (0-255)", 179, 0, 255);
    for (auto *color : {red, green, blue}) {
        color->setDecimals(0);
        color->setSingleStep(1);
    }
    auto *opacity = field("objectOpacity", "Opacity", 1, 0, 1);
    auto *error = new QLabel(&dialog);
    error->setWordWrap(true);
    form->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Add object");
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (ax->value() == 0 && ay->value() == 0 && az->value() == 0) {
            error->setText("Rotation axis must not be zero (default: 0, 1, 0).");
            return;
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted)
        return std::nullopt;
    ObjectInput result;
    result.transform.translation = Vector3{float(x->value()), float(y->value()), float(z->value())};
    result.transform.rotation_axis =
        Vector3{float(ax->value()), float(ay->value()), float(az->value())};
    result.transform.rotation_angle_degrees = float(angle->value());
    result.transform.color =
        Color{float(red->value() / 255), float(green->value() / 255), float(blue->value() / 255)};
    result.transform.opacity = float(opacity->value());
    if (size)
        result.size = float(size->value());
    if (height)
        result.height = float(height->value());
    return result;
}

enum class VectorKind { Position, Direction, RGB };

std::optional<Vector3> ask_vector(QWidget *parent, const char *prompt, const char *error,
                                  VectorKind kind, const char *initial) {
    for (;;) {
        const auto text = ask_text(parent, prompt, initial);
        if (!text)
            return std::nullopt;
        std::istringstream stream(text->toStdString());
        Vector3 value;
        std::string extra;
        bool valid = static_cast<bool>(stream >> value.x() >> value.y() >> value.z()) &&
                     !(stream >> extra) && value.allFinite();
        if (valid && kind == VectorKind::Direction)
            valid = value.stableNorm() >= 1e-6f;
        if (valid && kind == VectorKind::RGB)
            valid = (value.array() >= 0).all() && (value.array() <= 255).all() &&
                    (value.array() == value.array().floor()).all();
        if (valid)
            return value;
        QMessageBox::warning(parent, "Operations", error);
    }
}

std::optional<Color> ask_intensity(QWidget *parent) {
    const auto r = ask_number(
        parent, "Enter red intensity of light (0 to 1):", "Invalid red intensity.", 0, 1, 1);
    if (!r)
        return std::nullopt;
    const auto g = ask_number(
        parent, "Enter green intensity of light (0 to 1):", "Invalid green intensity.", 0, 1, 1);
    if (!g)
        return std::nullopt;
    const auto b = ask_number(
        parent, "Enter blue intensity of light (0 to 1):", "Invalid blue intensity.", 0, 1, 1);
    if (!b)
        return std::nullopt;
    return Color{*r, *g, *b};
}
} // namespace

SettingsWindow::SettingsWindow() {
    build_ui();
}

void SettingsWindow::show_fps(double fps) {
    fps_label_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
}

void SettingsWindow::show_status(const QString &text) {
    status_label_->setText(text);
}

void SettingsWindow::show_camera_position(float x, float y, float z) {
    camera_label_->setText(
        QString("Camera: %1 %2 %3").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2));
}

void SettingsWindow::show_objects(const std::vector<SceneObject> &objects) {
    const auto *selected = object_list_->currentItem();
    const auto previous_id = selected ? selected->data(Qt::UserRole).toULongLong() : 0;
    object_list_->clear();
    for (const auto &object : objects) {
        auto *item = new QListWidgetItem(QString("#%1  %2")
                                             .arg(static_cast<qulonglong>(object.id))
                                             .arg(QString::fromStdString(object.name)),
                                         object_list_);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(object.id)));
        item->setToolTip(QString::fromStdString(object.name));
        if (object.id == previous_id)
            object_list_->setCurrentItem(item);
    }
    remove_button_->setEnabled(object_list_->currentItem() != nullptr);
}

void SettingsWindow::show_camera_light(bool enabled) {
    camera_light_label_->setText(
        QString("Camera light: %1. Y: toggle in render window.").arg(enabled ? "ON" : "OFF"));
}

void SettingsWindow::build_ui() {
    setWindowTitle("Settings");
    resize(kSettingsWidth, kSettingsHeight);
    auto *operations = menuBar()->addMenu("Operations");
    connect(operations->addAction("Load object"), &QAction::triggered, this,
            &SettingsWindow::on_load_object);
    connect(operations->addAction("Add light"), &QAction::triggered, this,
            &SettingsWindow::on_add_light);
    connect(operations->addAction("Exit"), &QAction::triggered, this,
            &SettingsWindow::exit_requested);
    auto *settings = menuBar()->addMenu("Settings");
    connect(settings->addAction("Add point light"), &QAction::triggered, this,
            &SettingsWindow::on_add_point_light);
    connect(settings->addAction("Ambient light"), &QAction::triggered, this,
            &SettingsWindow::on_ambient_light);
    connect(settings->addAction("Rendering / shadows"), &QAction::triggered, this,
            &SettingsWindow::on_render_settings);
    connect(settings->addAction("Remove object"), &QAction::triggered, this,
            &SettingsWindow::on_remove_object);
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(new QLabel("Operations:\n1. Load object\n2. Add light\n3. Exit", central));
    menu_number_ = new QLineEdit(central);
    menu_number_->setPlaceholderText("Enter option (1, 2, 3), then Enter");
    connect(menu_number_, &QLineEdit::returnPressed, this, &SettingsWindow::on_menu_number);
    layout->addWidget(menu_number_);
    auto *add_row = new QHBoxLayout();
    auto *load_button = new QPushButton("Load OBJ", central);
    load_button->setObjectName("loadObjButton");
    connect(load_button, &QPushButton::clicked, this, &SettingsWindow::on_load_object);
    add_row->addWidget(load_button);
    for (const auto kind :
         {primitives::Kind::Cube, primitives::Kind::Sphere, primitives::Kind::Pyramid}) {
        const QString name = kind == primitives::Kind::Cube     ? "Cube"
                             : kind == primitives::Kind::Sphere ? "Sphere"
                                                                : "Pyramid";
        auto *button = new QPushButton("Add " + name, central);
        button->setObjectName("add" + name + "Button");
        connect(button, &QPushButton::clicked, this, [this, kind] { on_add_primitive(kind); });
        add_row->addWidget(button);
    }
    layout->addLayout(add_row);
    layout->addWidget(new QLabel("Objects (select one to remove):", central));
    object_list_ = new QListWidget(central);
    object_list_->setObjectName("objectList");
    object_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(object_list_);
    remove_button_ = new QPushButton("Remove selected object", central);
    remove_button_->setObjectName("removeObjectButton");
    remove_button_->setEnabled(false);
    connect(remove_button_, &QPushButton::clicked, this, &SettingsWindow::on_remove_object);
    connect(object_list_, &QListWidget::itemSelectionChanged, this,
            [this] { remove_button_->setEnabled(!object_list_->selectedItems().isEmpty()); });
    layout->addWidget(remove_button_);
    camera_light_label_ = new QLabel(central);
    camera_light_label_->setWordWrap(true);
    layout->addWidget(camera_light_label_);
    show_camera_light(false);
    camera_label_ = new QLabel(central);
    layout->addWidget(camera_label_);
    fps_label_ = new QLabel("FPS: 0", central);
    layout->addWidget(fps_label_);
    status_label_ =
        new QLabel("WASD: move in render window. Hold LMB and drag: look around.", central);
    status_label_->setWordWrap(true);
    layout->addWidget(status_label_);
    setCentralWidget(central);
}

void SettingsWindow::on_load_object() {
    const auto path = ask_text(this, "Enter path to obj file:");
    if (!path)
        return;
    const auto input = ask_object(this, "Load OBJ: position and appearance");
    if (input)
        emit load_object_requested(*path, input->transform);
}

void SettingsWindow::on_add_primitive(primitives::Kind kind) {
    const QString name = kind == primitives::Kind::Cube     ? "Cube"
                         : kind == primitives::Kind::Sphere ? "Sphere"
                                                            : "Pyramid";
    const auto input = ask_object(this, "Add " + name, kind);
    if (input)
        emit add_primitive_requested(kind, input->transform, input->size, input->height);
}

void SettingsWindow::on_remove_object() {
    const auto selected = object_list_->selectedItems();
    if (selected.isEmpty()) {
        show_status("Select an object in the list first.");
        object_list_->setFocus();
        return;
    }
    emit remove_object_requested(selected.front()->data(Qt::UserRole).toULongLong());
}

void SettingsWindow::on_add_light() {
    const auto direction = ask_vector(this, "Enter direction vector of light in format x y z:",
                                      "Invalid direction vector.", VectorKind::Direction, "0 1 1");
    if (!direction)
        return;
    const auto color = ask_intensity(this);
    if (!color)
        return;
    emit add_light_requested(DirectionalLight{*direction, *color});
}

void SettingsWindow::on_add_point_light() {
    const auto point =
        ask_vector(this, "Enter light position in format x y z:", "Invalid light position.",
                   VectorKind::Position, "0 3 3");
    if (!point)
        return;
    const auto color = ask_intensity(this);
    if (!color)
        return;
    emit add_point_light_requested(PointLight{*point, *color});
}

void SettingsWindow::on_ambient_light() {
    const auto color = ask_intensity(this);
    if (color)
        emit ambient_light_requested(*color);
}

void SettingsWindow::on_render_settings() {
    bool ok = false;
    const auto mode = QInputDialog::getItem(this, "Rendering",
                                            "Mode:", {"Shaded", "Depth", "Normals"}, 0, false, &ok);
    if (!ok)
        return;
    const auto shadows = QInputDialog::getItem(this, "Rendering",
                                               "Shadows:", {"Enabled", "Disabled"}, 0, false, &ok);
    if (!ok)
        return;
    const auto fov = ask_number(
        this, "Vertical field of view (10 to 150 degrees):", "Invalid field of view.", 10, 150, 60);
    if (!fov)
        return;
    const RenderMode selected = mode == "Depth"     ? RenderMode::Depth
                                : mode == "Normals" ? RenderMode::Normals
                                                    : RenderMode::Shaded;
    emit settings_requested(RenderSettings{
        selected, shadows == "Enabled" ? ShadowMode::Enabled : ShadowMode::Disabled, *fov});
}

void SettingsWindow::on_menu_number() {
    const auto number = menu_number_->text().trimmed();
    menu_number_->clear();
    if (number == "1")
        on_load_object();
    else if (number == "2")
        on_add_light();
    else if (number == "3")
        emit exit_requested();
    else
        show_status("Unknown option.");
}
} // namespace r3d
