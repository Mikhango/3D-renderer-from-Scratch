#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include "renderwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onWireframe(bool on);
    void onReset();
    void onLightX(int v);
    void onLightY(int v);
    void onLightZ(int v);

private:
    void buildUI();

    RenderWidget* m_rw      = nullptr;
    QComboBox*    m_combo   = nullptr;
    QPushButton*  m_addBtn  = nullptr;
    QPushButton*  m_clrBtn  = nullptr;
    QCheckBox*    m_wire    = nullptr;
    QPushButton*  m_resetBtn = nullptr;
    QSlider*      m_lx      = nullptr;
    QSlider*      m_ly      = nullptr;
    QSlider*      m_lz      = nullptr;
    QLabel*       m_lxLbl   = nullptr;
    QLabel*       m_lyLbl   = nullptr;
    QLabel*       m_lzLbl   = nullptr;
};
