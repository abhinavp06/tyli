#include "app_setup.hpp"
#include "library_bridge.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

// tyli_ui is a static library, so its QML plugin must be pulled in explicitly.
Q_IMPORT_QML_PLUGIN(TyliPlugin)

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    // The app-data folder is named after this: %APPDATA%\TYLI, ~/.local/share/TYLI.
    QGuiApplication::setApplicationName("TYLI");
    QGuiApplication::setApplicationVersion(TYLI_VERSION);
    tyli::setupApplication();

    // Constructed before the QML engine and destroyed after it, so QML never sees it go away.
    tyli::LibraryBridge library(tyli::LibraryBridge::defaultDataDir());
    tyli::LibraryBridge::setInstance(&library);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Tyli", "Main");
    return app.exec();
}
