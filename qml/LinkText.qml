import QtQuick
import Tyli

// Text that opens something (an artist, a year, a genre, a director, a drive): a dotted
// underline that turns green on hover.
Text {
    id: link
    signal activated()

    color: area.containsMouse ? Theme.accent : Theme.text

    Row {
        anchors.top: parent.bottom
        anchors.topMargin: 1
        width: parent.width
        clip: true
        spacing: 2
        Repeater {
            model: Math.ceil(link.width / 4)
            Rectangle { width: 2; height: 1; color: area.containsMouse ? Theme.accent : Theme.textFaint }
        }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        anchors.margins: -2
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: link.activated()
    }
}
