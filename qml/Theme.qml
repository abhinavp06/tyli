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

    // ---- Size. Every length in the UI goes through px(), so one number sizes it all
    // (2026-10-03: the user asked for bigger text and icons, ×1.15). Windows' own display
    // scaling comes on top, per monitor, through Qt: a 27" 1440p screen at 100% and a 15"
    // 1080p laptop at 125% show text at about the same physical size, with nothing to set.
    // The play button and the quality picker keep their 40 px (the user: "those are nice").
    readonly property real scale: 1.15
    function px(length) { return Math.round(length * scale) }

    // ---- Type (IBM Plex Mono, bundled; loaded in app_setup.cpp)
    readonly property string fontFamily: "IBM Plex Mono"
    readonly property int fontSize: px(13)         // 15
    readonly property int fontSizeSmall: px(11)    // 13
    readonly property int fontSizeHeading: px(15)  // 17
    readonly property int fontSizeTitle: px(18)    // 21
    readonly property int weightBold: Font.DemiBold

    // ---- Metrics
    readonly property int iconSize: 20             // up from 16, just under the play button's 40 px box
    readonly property int controlHeight: 36        // icon buttons, fields: just under 40
    readonly property int topBarHeight: px(52)
    readonly property int sidebarWidth: px(200)
    readonly property int sidebarCollapsedWidth: px(52)
    readonly property int pageMargin: px(32)
    readonly property int lineWidth: 2  // selection markers, progress
}
