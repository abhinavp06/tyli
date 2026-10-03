#include "app_setup.hpp"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQuickStyle>
#include <QWheelEvent>
#include <QWindow>

namespace tyli {

namespace {

double wheelFactor = DefaultScrollSpeedPercent / 100.0;

// Makes every list and grid scroll wheelSpeed() times as far per wheel notch. Qt Quick scrolls
// in proportion to the wheel's delta (one notch = the system's "lines per notch" × 24 px), so
// scaling the delta once, where the event reaches a window, speeds up every view alike and
// still follows the system setting. Items aren't filtered: they get the window's scaled event.
class WheelSpeedFilter : public QObject {
public:
    using QObject::QObject;

    bool eventFilter(QObject* receiver, QEvent* event) override {
        if (event->type() != QEvent::Wheel || resending_ || !qobject_cast<QWindow*>(receiver)) return false;
        const auto* wheel = static_cast<QWheelEvent*>(event);
        const auto scaled = [](QPoint p) { return QPoint(qRound(p.x() * wheelFactor), qRound(p.y() * wheelFactor)); };
        QWheelEvent faster(wheel->position(), wheel->globalPosition(), scaled(wheel->pixelDelta()), scaled(wheel->angleDelta()),
                           wheel->buttons(), wheel->modifiers(), wheel->phase(), wheel->inverted(), wheel->source(),
                           wheel->pointingDevice());
        faster.setTimestamp(wheel->timestamp());
        resending_ = true;
        QCoreApplication::sendEvent(receiver, &faster);
        resending_ = false;
        event->setAccepted(faster.isAccepted());
        return true;
    }

private:
    bool resending_ = false;
};

}

void setWheelSpeed(double factor) { wheelFactor = factor; }
double wheelSpeed() { return wheelFactor; }

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
    font.setPixelSize(15);  // Theme.fontSize (qml/Theme.qml), for anything outside the window
    QGuiApplication::setFont(font);

    qApp->installEventFilter(new WheelSpeedFilter(qApp));
}

}
