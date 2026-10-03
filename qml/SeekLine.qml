import QtQuick
import Tyli

// A thin green line to seek or set a level (features/player_ui.md §1.2): click or drag sets
// it. It thickens under the mouse; `buffered` shows faint behind the played part.
Item {
    id: line

    property real value: 0      // 0..1
    property real buffered: 0   // 0..1
    signal moved(real value)

    implicitHeight: Theme.px(14)   // the hit area; the line itself is thin
    readonly property real thickness: area.containsMouse || area.pressed ? Theme.lineWidth * 2 : Theme.lineWidth

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: line.thickness
        color: Theme.hairline
        Rectangle { width: parent.width * Math.max(0, Math.min(1, line.buffered)); height: parent.height; color: Theme.textFaint }
        Rectangle { width: parent.width * Math.max(0, Math.min(1, area.pressed ? area.dragged : line.value)); height: parent.height; color: Theme.accent }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        property real dragged: 0
        function at(x) { return Math.max(0, Math.min(1, x / width)) }
        onPressed: (mouse) => dragged = at(mouse.x)
        onPositionChanged: (mouse) => { if (pressed) dragged = at(mouse.x) }
        onReleased: (mouse) => line.moved(at(mouse.x))
    }
}
