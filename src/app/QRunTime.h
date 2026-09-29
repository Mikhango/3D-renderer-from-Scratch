#pragma once

#include <QApplication>

namespace r3d {

class QRunTime : public QApplication {
public:
    QRunTime(int &argc, char **argv);
    bool notify(QObject *receiver, QEvent *event) override;
};

} // namespace r3d
