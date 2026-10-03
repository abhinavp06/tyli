import QtQuick
import QtQuick.Controls
import Tyli

// A grid card: the cover or poster, then a title and a subtitle. The whole card opens it.
Item {
    id: card

    property alias source: art.source
    property alias placeholder: art.placeholder
    property bool online: true
    property bool poster: false  // 2:3 instead of square
    property string title
    property string subtitle
    property real progress: 0    // how much you've watched (0–1): a green line under the art
    signal activated()

    implicitWidth: Theme.px(150)
    ToolTip.visible: area.containsMouse && titleText.truncated
    ToolTip.text: card.title
    ToolTip.delay: 500
    implicitHeight: art.height + Theme.px(7) + titleText.implicitHeight + subtitleText.implicitHeight + Theme.px(3)

    CoverImage {
        id: art
        width: card.width
        height: card.poster ? Math.round(card.width * 1.5) : card.width
        online: card.online
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: Theme.accent
            visible: area.containsMouse
        }
        Rectangle {
            objectName: "watchedLine"
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.lineWidth
            color: Theme.hairline
            visible: card.progress > 0
            Rectangle { width: parent.width * Math.min(1, card.progress); height: parent.height; color: Theme.accent }
        }
    }
    Text {
        id: titleText
        anchors.top: art.bottom
        anchors.topMargin: Theme.px(7)
        width: card.width
        text: card.title
        elide: Text.ElideRight
        color: !card.online ? Theme.textFaint : area.containsMouse ? Theme.accent : Theme.text
    }
    Text {
        id: subtitleText
        anchors.top: titleText.bottom
        anchors.topMargin: Theme.px(3)
        width: card.width
        text: card.subtitle
        elide: Text.ElideRight
        color: card.online ? Theme.textDim : Theme.textFaint
        font.pixelSize: Theme.fontSizeSmall
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: card.activated()
    }
}
