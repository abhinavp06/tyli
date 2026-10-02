import QtQuick
import QtQuick.Shapes

// One of TYLI's stroke icons (24×24 paths, drawn for the theme), in any color.
// Shapes instead of image files, so an icon takes its color from a binding.
Item {
    id: icon

    property string name
    property color color: Theme.textDim
    property real size: Theme.iconSize

    implicitWidth: size
    implicitHeight: size

    readonly property var paths: ({
        music: "M9 17V5l10-2v12 M4 17a2.5 2.5 0 1 0 5 0a2.5 2.5 0 1 0 -5 0 M14 15a2.5 2.5 0 1 0 5 0a2.5 2.5 0 1 0 -5 0",
        movies: "M3 4h18v16H3z M7 4v16 M17 4v16 M3 9h4 M3 15h4 M17 9h4 M17 15h4",
        shows: "M3 7h18v12H3z M8 3l4 4 4-4",
        books: "M3 5h6a3 3 0 0 1 3 3v12a2 2 0 0 0-2-2H3z M21 5h-6a3 3 0 0 0-3 3v12a2 2 0 0 1 2-2h7z",
        folder: "M3 6h6l2 2h10v11H3z",
        gear: "M9 12a3 3 0 1 0 6 0a3 3 0 1 0 -6 0 M5 12a7 7 0 1 0 14 0a7 7 0 1 0 -14 0 M12 2v3 M12 19v3 M2 12h3 M19 12h3 M4.9 4.9L7 7 M17 17l2.1 2.1 M4.9 19.1L7 17 M17 7l2.1-2.1",
        panel: "M3 4h18v16H3z M9 4v16",
        plus: "M12 5v14 M5 12h14",
        sync: "M19 12a7 7 0 0 1-12.2 4.7 M5 12a7 7 0 0 1 12.2-4.7 M17.5 3.5v4h-4 M6.5 20.5v-4h4",
        stop: "M7 7h10v10H7z",
        edit: "M4 20h4L19 9l-4-4L4 16z",
        trash: "M4 7h16 M9 7V4h6v3 M6 7l1 13h10l1-13",
        target: "M6 12a6 6 0 1 0 12 0a6 6 0 1 0 -12 0 M12 2v5 M12 17v5 M2 12h5 M17 12h5",
        move: "M4 8h13 M13 4l4 4-4 4 M20 16H7 M11 12l-4 4 4 4",
        chevronDown: "M6 9l6 6 6-6",
        chevronRight: "M9 6l6 6-6 6",
        check: "M5 12l5 5 9-10",
        close: "M6 6l12 12 M18 6L6 18",
        flag: "M5 21V4h11l-2 4 2 4H5",
        reset: "M5 12a7 7 0 1 0 2-5 M4 3v5h5",
        warning: "M12 4l9 16H3z M12 10v4 M12 16.5v1",
        back: "M15 5l-7 7 7 7",
        play: "M7 4l13 8-13 8z",
        disc: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M9.5 12a2.5 2.5 0 1 0 5 0a2.5 2.5 0 1 0 -5 0",
        drive: "M3 7h18v10H3z M7 12h.01 M11 12h6",
        heart: "M12 20s-7-4.4-7-10a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 5.6-7 10-7 10z",
        playlist: "M4 6h12 M4 11h12 M4 16h7 M17 14v6 M14 17h6"
    })

    Shape {
        width: 24
        height: 24
        scale: icon.size / 24
        transformOrigin: Item.TopLeft
        // Antialiased strokes on the GPU without multisampling (the software renderer ignores it).
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: icon.color
            strokeWidth: 1.5
            fillColor: "transparent"
            capStyle: ShapePath.SquareCap
            joinStyle: ShapePath.MiterJoin
            PathSvg { path: icon.paths[icon.name] ?? "" }
        }
    }
}
