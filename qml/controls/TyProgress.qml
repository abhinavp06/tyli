import QtQuick
import Tyli

// A 2 px progress line. While the total is unknown (a scan), a short green segment
// slides along it.
Item {
    id: control

    property bool indeterminate: true
    property real value: 0  // 0..1, when not indeterminate

    implicitWidth: 260
    implicitHeight: Theme.lineWidth
    clip: true

    Rectangle {
        anchors.fill: parent
        color: Theme.hairline
    }
    Rectangle {
        visible: !control.indeterminate
        width: parent.width * Math.max(0, Math.min(1, control.value))
        height: parent.height
        color: Theme.accent
    }
    Rectangle {
        id: segment
        visible: control.indeterminate
        width: parent.width * 0.18
        height: parent.height
        color: Theme.accent
        NumberAnimation on x {
            from: -segment.width
            to: control.width
            duration: 1600
            loops: Animation.Infinite
            running: control.indeterminate && control.visible
        }
    }
}
