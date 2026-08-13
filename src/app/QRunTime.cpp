#include "app/QRunTime.h"
#include "app/Except.h"
#include "core/Settings.h"

namespace r3d {
QRunTime::QRunTime(int &argc, char **argv) : QApplication(argc, argv) {
    setApplicationName(kApplicationName);
    setApplicationVersion(kApplicationVersion);
}

bool QRunTime::notify(QObject *receiver, QEvent *event) {
    try {
        return QApplication::notify(receiver, event);
    } catch (...) {
        react();
        exit(1);
        return false;
    }
}
} // namespace r3d
