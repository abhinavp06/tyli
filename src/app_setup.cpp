#include "app_setup.hpp"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQuickStyle>
#include <QWheelEvent>
#include <QWindow>

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
#include <QSet>
#include <QtGui/private/qguiapplication_p.h>
#include <qt_windows.h>
#include <dwmapi.h>
#endif

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

#ifdef Q_OS_WIN
// The full-screen border below is a one-pixel WS_BORDER frame, which Windows draws in its light
// theme's frame color: a white line around the video (the user, 2026-11-03). It has to stay (it's
// what keeps the window composited), so it's painted black instead, invisible against TYLI's black.
// Windows 11 also draws its own accent outline around windows; that one is turned off in full screen.
class BlackFullScreenBorder : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& type, void* message, qintptr* result) override {
        if (type != "windows_generic_MSG") return false;
        const auto* msg = static_cast<const MSG*>(message);
        switch (msg->message) {
        case WM_WINDOWPOSCHANGED:
            follow(msg->hwnd);
            return false;
        case WM_NCPAINT:
            if (!isFullScreenWithBorder(msg->hwnd)) return false;
            paintFrameBlack(msg->hwnd);
            *result = 0;
            return true;
        case WM_NCACTIVATE:
            // Windows redraws the frame on focus changes without a WM_NCPAINT; paint over it.
            if (!isFullScreenWithBorder(msg->hwnd)) return false;
            *result = DefWindowProcW(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            paintFrameBlack(msg->hwnd);
            return true;
        case WM_NCDESTROY:
            bordered_.remove(msg->hwnd);
            return false;
        }
        return false;
    }

private:
    // Qt's full screen with a border: a WS_POPUP window with WS_BORDER (no caption) over its monitor.
    static bool isFullScreenWithBorder(HWND hwnd) {
        const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (!(style & WS_POPUP) || (style & WS_CAPTION) != WS_BORDER) return false;
        RECT window;
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (!GetWindowRect(hwnd, &window) || !GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) return false;
        const RECT& m = monitor.rcMonitor;
        return window.left <= m.left && window.top <= m.top && window.right >= m.right && window.bottom >= m.bottom;
    }

    // Everything of the window outside its client area (the one-pixel frame), in black.
    static void paintFrameBlack(HWND hwnd) {
        RECT window, client;
        POINT origin{0, 0};
        if (!GetWindowRect(hwnd, &window) || !GetClientRect(hwnd, &client) || !ClientToScreen(hwnd, &origin)) return;
        OffsetRect(&client, origin.x - window.left, origin.y - window.top);
        OffsetRect(&window, -window.left, -window.top);
        if (HDC dc = GetWindowDC(hwnd)) {
            ExcludeClipRect(dc, client.left, client.top, client.right, client.bottom);
            FillRect(dc, &window, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            ReleaseDC(hwnd, dc);
        }
    }

    // On entering and leaving full screen: Windows 11's outline off and back, and our frame
    // painting (no DWM frame drawing) on and off. Both calls fail harmlessly on Windows 10.
    void follow(HWND hwnd) {
        const bool now = isFullScreenWithBorder(hwnd);
        if (now == bordered_.contains(hwnd)) return;
        if (now) bordered_.insert(hwnd); else bordered_.remove(hwnd);
        constexpr DWORD BorderColor = 34;  // DWMWA_BORDER_COLOR (Windows 11 SDKs only)
        const COLORREF color = now ? 0xFFFFFFFE : 0xFFFFFFFF;  // DWMWA_COLOR_NONE : DWMWA_COLOR_DEFAULT
        DwmSetWindowAttribute(hwnd, BorderColor, &color, sizeof(color));
        const DWMNCRENDERINGPOLICY policy = now ? DWMNCRP_DISABLED : DWMNCRP_USEWINDOWSTYLE;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        RedrawWindow(hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
    }

    QSet<HWND> bordered_;
};
#endif

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

#ifdef Q_OS_WIN
    // A borderless OpenGL window covering a whole monitor is taken over by the GPU driver as
    // exclusive full screen: the monitor switches mode (and out of Windows' HDR mode) when the
    // video page enters or leaves full screen, blanking for a second (the user, 2026-10-17).
    // A one-pixel border keeps the window composited by Windows, like a browser's full screen.
    // Qt's documented workaround ("Qt for Windows - Specific Issues"), through a private API.
    using QNativeInterface::Private::QWindowsApplication;
    if (auto* windows = qApp->nativeInterface<QWindowsApplication>()) windows->setHasBorderInFullScreenDefault(true);
    qApp->installNativeEventFilter(new BlackFullScreenBorder);  // lives as long as the app
#endif
}

}
