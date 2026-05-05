#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QFrame>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("3D Renderer");
    resize(1100, 700);
    buildUI();

    QMenu* file = menuBar()->addMenu("File");
    connect(file->addAction("Exit"), &QAction::triggered, this, &QWidget::close);

    QMenu* view = menuBar()->addMenu("View");
    connect(view->addAction("Reset Camera"), &QAction::triggered, this, &MainWindow::onReset);

    QAction* wireAct = view->addAction("Wireframe");
    wireAct->setCheckable(true);
    connect(wireAct, &QAction::toggled, this, &MainWindow::onWireframe);

    statusBar()->showMessage("LMB: orbit | RMB/Scroll: zoom | MMB: pan | R: reset | W: wireframe");
}

void MainWindow::buildUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* lay = new QHBoxLayout(central);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(6);

    m_rw = new RenderWidget(this);
    lay->addWidget(m_rw, 1);

    QFrame* panel = new QFrame(this);
    panel->setFrameShape(QFrame::StyledPanel);
    panel->setFixedWidth(200);
    QVBoxLayout* pl = new QVBoxLayout(panel);
    pl->setContentsMargins(8, 8, 8, 8);
    pl->setSpacing(8);

    QGroupBox* objBox = new QGroupBox("Objects", panel);
    QVBoxLayout* ol = new QVBoxLayout(objBox);
    m_combo = new QComboBox(objBox);
    m_combo->addItems({"Sphere", "Box", "Pyramid"});
    ol->addWidget(m_combo);
    m_addBtn = new QPushButton("Add", objBox);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAdd);
    ol->addWidget(m_addBtn);
    m_clrBtn = new QPushButton("Clear", objBox);
    connect(m_clrBtn, &QPushButton::clicked, this, [this]() {
        m_rw->getScene().clear();
        m_rw->redraw();
    });
    ol->addWidget(m_clrBtn);
    pl->addWidget(objBox);

    QGroupBox* camBox = new QGroupBox("Camera", panel);
    QVBoxLayout* cl = new QVBoxLayout(camBox);
    m_resetBtn = new QPushButton("Reset (R)", camBox);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::onReset);
    cl->addWidget(m_resetBtn);
    pl->addWidget(camBox);

    QGroupBox* renderBox = new QGroupBox("Render", panel);
    QVBoxLayout* rl = new QVBoxLayout(renderBox);
    m_wire = new QCheckBox("Wireframe (W)", renderBox);
    connect(m_wire, &QCheckBox::toggled, this, &MainWindow::onWireframe);
    rl->addWidget(m_wire);
    pl->addWidget(renderBox);

    QGroupBox* lightBox = new QGroupBox("Light", panel);
    QVBoxLayout* ll = new QVBoxLayout(lightBox);

    auto addSlider = [&](const QString& name, QSlider*& sl, QLabel*& lbl, int def) {
        ll->addWidget(new QLabel(name, lightBox));
        sl = new QSlider(Qt::Horizontal, lightBox);
        sl->setRange(-10, 10);
        sl->setValue(def);
        ll->addWidget(sl);
        lbl = new QLabel(QString::number(def), lightBox);
        ll->addWidget(lbl);
    };

    addSlider("X:", m_lx, m_lxLbl, 2);
    addSlider("Y:", m_ly, m_lyLbl, 4);
    addSlider("Z:", m_lz, m_lzLbl, 3);

    connect(m_lx, &QSlider::valueChanged, this, &MainWindow::onLightX);
    connect(m_ly, &QSlider::valueChanged, this, &MainWindow::onLightY);
    connect(m_lz, &QSlider::valueChanged, this, &MainWindow::onLightZ);

    pl->addWidget(lightBox);
    pl->addStretch();

    lay->addWidget(panel);
}

void MainWindow::onAdd() {
    int i = m_combo->currentIndex();
    if (i == 0)      m_rw->getScene().addSphere(1.0f);
    else if (i == 1) m_rw->getScene().addBox(1.0f, 1.0f, 1.0f);
    else             m_rw->getScene().addPyramid(1.0f, 1.5f);
    m_rw->redraw();
}

void MainWindow::onWireframe(bool on) {
    if (m_wire->isChecked() != on)
        m_wire->setChecked(on);
    m_rw->setWireframe(on);
}

void MainWindow::onReset() {
    m_rw->resetCamera();
}

void MainWindow::onLightX(int v) {
    m_lxLbl->setText(QString::number(v));
    m_rw->getScene().light.pos.x() = (float)v;
    m_rw->redraw();
}

void MainWindow::onLightY(int v) {
    m_lyLbl->setText(QString::number(v));
    m_rw->getScene().light.pos.y() = (float)v;
    m_rw->redraw();
}

void MainWindow::onLightZ(int v) {
    m_lzLbl->setText(QString::number(v));
    m_rw->getScene().light.pos.z() = (float)v;
    m_rw->redraw();
}
