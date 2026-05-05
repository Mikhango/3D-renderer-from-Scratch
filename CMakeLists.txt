cmake_minimum_required(VERSION 3.16)
project(Renderer3D VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt (try Qt6 first, fall back to Qt5)
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

# Find Eigen3 (no version constraint — works with Eigen 3.x and 5.x)
find_package(Eigen3 REQUIRED NO_MODULE)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/renderwidget.cpp
    src/renderer.cpp
    src/scene.cpp
    src/camera.cpp
    src/rasterizer.cpp
    src/lighting.cpp
    src/primitives/mesh.cpp
    src/primitives/sphere.cpp
    src/primitives/box.cpp
    src/primitives/pyramid.cpp
)

set(HEADERS
    include/mainwindow.h
    include/renderwidget.h
    include/renderer.h
    include/scene.h
    include/camera.h
    include/rasterizer.h
    include/lighting.h
    include/primitives/mesh.h
    include/primitives/sphere.h
    include/primitives/box.h
    include/primitives/pyramid.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Eigen3::Eigen
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Eigen3::Eigen
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()
