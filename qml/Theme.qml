pragma Singleton
import QtQuick

// Every color, size, and font of TYLI's look (features/theme.md §2–3).
// Screens use these names, never literal colors.
QtObject {
    // ---- Palette: pure black; green is only text, thin lines, and small glyphs
    readonly property color background: "#000000"
    readonly property color hairline: "#1a1a1a"  // separators and borders, barely visible
    readonly property color hover: "#0d0d0d"     // the only non-black fill
    readonly property color text: "#c8c8c8"
    readonly property color textDim: "#767676"   // secondary text (4.6:1 on black)
    readonly property color textFaint: "#3c3c3c" // disabled, placeholders
    readonly property color accent: "#39ff6a"
    readonly property color accentDim: "#1e8a3c"
    readonly property color selection: "#0f2e18" // behind selected text
    readonly property color warning: "#ffb000"
    readonly property color error: "#ff4d4d"
    readonly property color backdrop: Qt.rgba(0, 0, 0, 0.6)

    // ---- Type (IBM Plex Mono, bundled; loaded in app_setup.cpp)
    readonly property string fontFamily: "IBM Plex Mono"
    readonly property int fontSize: 13
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeHeading: 15
    readonly property int fontSizeTitle: 18
    readonly property int weightBold: Font.DemiBold

    // ---- Metrics
    readonly property int iconSize: 16
    readonly property int controlHeight: 32
    readonly property int topBarHeight: 52
    readonly property int sidebarWidth: 200
    readonly property int sidebarCollapsedWidth: 52
    readonly property int pageMargin: 32
    readonly property int lineWidth: 2  // selection markers, progress
}
