#include <hoardor/file/library.hpp>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringList>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Fusion");

    // Skeleton check that hoardor links and runs: list its default categories.
    QStringList categories;
    if (auto database = hoardor::db::Database::open_in_memory()) {
        if (auto library = hoardor::file::Library::open(*database)) {
            if (auto all = library->categories()) {
                for (const auto& category : *all) categories << QString::fromStdString(category.name);
            }
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("hoardorCategories", categories);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Tyli", "Main");
    return app.exec();
}
