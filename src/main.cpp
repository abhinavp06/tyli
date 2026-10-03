#include "app_setup.hpp"
#include "library_bridge.hpp"
#include "player_bridge.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtQml/qqmlextensionplugin.h>

// tyli_ui is a static library, so its QML plugin must be pulled in explicitly.
Q_IMPORT_QML_PLUGIN(TyliPlugin)

int main(int argc, char* argv[]) {
    // mpv draws video through OpenGL (its render API), so Qt Quick draws with OpenGL on every
    // OS, not Direct3D 11 as on Windows by default (features/player_ui.md, decision P4).
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication app(argc, argv);
    // The app-data folder is named after this: %APPDATA%\TYLI, ~/.local/share/TYLI.
    QGuiApplication::setApplicationName("TYLI");
    QGuiApplication::setApplicationVersion(TYLI_VERSION);
    tyli::setupApplication();

    // Constructed before the QML engine and destroyed after it, so QML never sees it go away.
    tyli::LibraryBridge library(tyli::LibraryBridge::defaultDataDir());
    tyli::LibraryBridge::setInstance(&library);
    // After the library (it names what plays) and, like it, outliving the QML engine.
    tyli::PlayerBridge player(library.databasePath());
    tyli::PlayerBridge::setInstance(&player);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Tyli", "Main");
    return app.exec();
}
