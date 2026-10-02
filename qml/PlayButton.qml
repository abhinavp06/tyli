import QtQuick
import QtQuick.Controls
import Tyli

// The page's main action, at the top right. Playback comes with the player (the next
// feature), so for now it says so.
Rectangle {
    id: button
    property string tip: "play"

    implicitWidth: 40
    implicitHeight: 40
    color: area.containsMouse ? Theme.hover : "transparent"
    border.width: 1
    border.color: area.containsMouse ? Theme.accent : Theme.accentDim
    Icon { anchors.centerIn: parent; name: "play"; color: Theme.accent }
    ToolTip.visible: area.containsMouse
    ToolTip.text: button.tip + " (comes with the player)"
    ToolTip.delay: 300
    MouseArea { id: area; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
}
