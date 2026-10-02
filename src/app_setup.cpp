#include "app_setup.hpp"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQuickStyle>

namespace tyli {

void setupApplication() {
    // Basic is the style Qt designed for customization. Fusion draws gradients and
    // bevels that can't be made pure black (features/theme.md §5).
    QQuickStyle::setStyle("Basic");

    // Bundled in the QML module's resources, so TYLI looks the same on every OS.
    for (const char* file : {":/qt/qml/Tyli/qml/fonts/IBMPlexMono-Regular.ttf",
                             ":/qt/qml/Tyli/qml/fonts/IBMPlexMono-SemiBold.ttf"}) {
        if (QFontDatabase::addApplicationFont(QString::fromLatin1(file)) < 0) {
            qWarning("TYLI: couldn't load the font %s; using the system's monospace font", file);
        }
    }
    QFont font(QStringLiteral("IBM Plex Mono"));
    font.setStyleHint(QFont::Monospace);  // the fallback if the bundled font is missing
    font.setPixelSize(13);
    QGuiApplication::setFont(font);
}

}
