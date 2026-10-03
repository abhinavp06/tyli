cmake_minimum_required(VERSION 3.23)

project(
    tyli
    VERSION 0.2.0
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
find_package(Qt6 6.12 REQUIRED COMPONENTS Quick QuickControls2 Concurrent OpenGL)
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

# Theme.qml is a singleton (one shared instance of the palette and sizes).
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(tyli_ui
    URI Tyli
    VERSION 1.0
    # Tooling (qmllint) expects the module's build folder to end in its URI.
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Tyli
    SOURCES
        src/app_setup.cpp
        src/app_setup.hpp
        src/copy_grouper.cpp
        src/copy_grouper.hpp
        src/group_model.cpp
        src/group_model.hpp
        src/item_model.cpp
        src/item_model.hpp
        src/library_bridge.cpp
        src/library_bridge.hpp
        src/media_fields.cpp
        src/media_fields.hpp
        src/player_bridge.cpp
        src/player_bridge.hpp
        src/queue_model.cpp
        src/queue_model.hpp
        src/thumbnail_cache.cpp
        src/thumbnail_cache.hpp
        src/video_item.cpp
        src/video_item.hpp
    QML_FILES
        qml/AlbumBrowser.qml
        qml/AlbumView.qml
        qml/ArtistView.qml
        qml/BrowseHeader.qml
        qml/CategoryView.qml
        qml/CoverCard.qml
        qml/CardFlow.qml
        qml/CoverGrid.qml
        qml/CoverImage.qml
        qml/ExpandedPlayer.qml
        qml/Format.js
        qml/GeneralSettings.qml
        qml/HeroHeader.qml
        qml/Icon.qml
        qml/LibrarySettings.qml
        qml/LinkText.qml
        qml/Main.qml
        qml/MovieView.qml
        qml/MusicFilterView.qml
        qml/MusicView.qml
        qml/PersonView.qml
        qml/PlayButton.qml
        qml/PlayerBar.qml
        qml/QualityPicker.qml
        qml/QueueList.qml
        qml/QueuePage.qml
        qml/QueuePrompt.qml
        qml/SettingsView.qml
        qml/SearchView.qml
        qml/SeekLine.qml
        qml/ShowView.qml
        qml/ShowState.qml
        qml/SeasonView.qml
        qml/Sidebar.qml
        qml/SortBar.qml
        qml/StatusLine.qml
        qml/Theme.qml
        qml/TopBar.qml
        qml/TrackList.qml
        qml/VideoGridView.qml
        qml/VideoPage.qml
        qml/controls/TyCheckBox.qml
        qml/controls/TyField.qml
        qml/controls/TyIconButton.qml
        qml/controls/TyMenu.qml
        qml/controls/TyMenuItem.qml
        qml/controls/TyPill.qml
        qml/controls/TyProgress.qml
        qml/controls/TyScrollBar.qml
        qml/controls/TyTab.qml
    RESOURCES
        qml/fonts/IBMPlexMono-Regular.ttf
        qml/fonts/IBMPlexMono-SemiBold.ttf
        qml/fonts/OFL.txt
)

target_include_directories(tyli_ui PUBLIC src)
target_link_libraries(tyli_ui PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::OpenGL hoardor)

# -----------------------------------------------------------------------------
# App
# -----------------------------------------------------------------------------
qt_add_executable(tyli
    src/main.cpp
)

target_link_libraries(tyli PRIVATE tyli_ui tyli_uiplugin)
target_compile_definitions(tyli PRIVATE TYLI_VERSION="${PROJECT_VERSION}")

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
