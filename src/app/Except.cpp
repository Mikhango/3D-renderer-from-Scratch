#include "app/Except.h"

#include <exception>
#include <iostream>

namespace r3d {

void react() {
    try {
        throw;
    } catch (const std::exception &error) {
        std::cerr << "Exception: " << error.what() << std::endl;
    } catch (...) {
        std::cerr << "Exception: unknown error" << std::endl;
    }
}

} // namespace r3d
