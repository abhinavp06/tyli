cmake_minimum_required(VERSION 3.23)

project(
    tyli
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TYLI_BUILD_TESTS "Build TYLI's Qt Test suites" ${PROJECT_IS_TOP_LEVEL})

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
find_package(Qt6 6.12 REQUIRED COMPONENTS Quick QuickControls2 Concurrent)
# Qt's recommended defaults: AUTOMOC (generates the QObject glue code), policies for 6.12.
qt_standard_project_setup(REQUIRES 6.12)

# -----------------------------------------------------------------------------
# tyli_ui: the QML module "Tyli" (bridge + screens) as a static library.
# The app and the tests both link it, so the tests load the real screens.
# qt_add_qml_module also creates "tyli_uiplugin", the plugin that registers the
# module's types; with a static library it must be linked and imported
# explicitly (Q_IMPORT_QML_PLUGIN(TyliPlugin)).
# -----------------------------------------------------------------------------
qt_add_library(tyli_ui STATIC)

qt_add_qml_module(tyli_ui
    URI Tyli
    VERSION 1.0
    # Tooling (qmllint) expects the module's build folder to end in its URI.
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Tyli
    SOURCES
        src/folder_files_model.cpp
        src/folder_files_model.hpp
        src/library_bridge.cpp
        src/library_bridge.hpp
    QML_FILES
        qml/CategorySettings.qml
        qml/FolderSettings.qml
        qml/Format.js
        qml/Main.qml
        qml/ScanningSettings.qml
        qml/SectionView.qml
        qml/SettingsView.qml
        qml/SyncDetailsDialog.qml
        qml/SyncSettings.qml
)

target_include_directories(tyli_ui PUBLIC src)
target_link_libraries(tyli_ui PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent hoardor)

# -----------------------------------------------------------------------------
# App
# -----------------------------------------------------------------------------
qt_add_executable(tyli
    src/main.cpp
)

target_link_libraries(tyli PRIVATE tyli_ui tyli_uiplugin)

set_target_properties(tyli PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

foreach(target tyli_ui tyli)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(TYLI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
