#include "app/Application.h"
#include "app/Except.h"
#include "app/QRunTime.h"

int main(int argc, char *argv[]) {
    try {
        r3d::QRunTime runtime(argc, argv);
        r3d::Application application;
        return application.run();
    } catch (...) {
        r3d::react();
        return 1;
    }
}
