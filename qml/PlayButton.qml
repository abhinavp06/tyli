import QtQuick
import QtQuick.Controls
import Tyli

// The page's main action, at the top right: play (the album, the movie, the next episode).
Rectangle {
    id: button
    property string tip: "play"
    signal clicked()

    implicitWidth: 40
    implicitHeight: 40
    color: area.containsMouse ? Theme.hover : "transparent"
    border.width: 1
    border.color: area.containsMouse ? Theme.accent : Theme.accentDim
    Icon { anchors.centerIn: parent; name: "play"; size: 16; color: Theme.accent }  // fixed: not scaled with Theme (the user likes it as is)
    ToolTip.visible: area.containsMouse
    ToolTip.text: button.tip
    ToolTip.delay: 300
    MouseArea { id: area; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.clicked() }
}
