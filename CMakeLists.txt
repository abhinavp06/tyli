cmake_minimum_required(VERSION 3.23)

project(
    tyli
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# hoardor (the engine library), built from the sibling checkout in the workspace.
# Added before Qt's project setup, so Qt's AUTOMOC never touches hoardor (it has no Qt).
# -----------------------------------------------------------------------------
set(TYLI_HOARDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../hoardor" CACHE PATH "Path to the hoardor checkout")
add_subdirectory(${TYLI_HOARDOR_DIR} ${CMAKE_BINARY_DIR}/hoardor)

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
# On this VM: -DCMAKE_PREFIX_PATH=/opt/Qt/6.12.0/gcc_64
# On Windows: -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
find_package(Qt6 6.12 REQUIRED COMPONENTS Quick QuickControls2)
# Qt's recommended defaults: AUTOMOC (generates the QObject glue code), policies for 6.12.
qt_standard_project_setup(REQUIRES 6.12)

# -----------------------------------------------------------------------------
# App
# -----------------------------------------------------------------------------
qt_add_executable(tyli
    src/main.cpp
)

qt_add_qml_module(tyli
    URI Tyli
    QML_FILES
        qml/Main.qml
)

target_link_libraries(tyli PRIVATE Qt6::Quick Qt6::QuickControls2 hoardor)

if(MSVC)
    target_compile_options(tyli PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(tyli PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(tyli PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)
