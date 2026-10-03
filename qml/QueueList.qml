import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The queue's rows (a QueueModel): number or ▶, title, artist · album (not when `compact`),
// length, × on hover. A click jumps to the row. The queue page and the expanded view's side
// panel both show it (features/player_ui.md §1.3, §1.6).
ListView {
    id: list
    property bool compact: false
    // A season's episodes: every row's title is the show, so a compact row shows "S1 E2 · title".
    property bool episodes: false
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: TyScrollBar {}
    delegate: Rectangle {
        id: row
        required property int index
        required property string title
        required property string subtitle
        required property var duration
        required property bool playing
        required property bool played
        width: ListView.view.width
        height: Theme.px(34)
        color: rowArea.containsMouse ? Theme.hover : "transparent"
        readonly property color ink: row.playing ? Theme.accent : row.played ? Theme.textFaint : Theme.text

        Rectangle { visible: row.playing; width: Theme.lineWidth; height: parent.height; color: Theme.accent }
        MouseArea {
            id: rowArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Player.jump(row.index)
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.px(12)
            anchors.rightMargin: Theme.px(4)
            spacing: Theme.px(16)
            Item {
                Layout.preferredWidth: Theme.px(28)
                Layout.fillHeight: true
                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !row.playing
                    text: row.index + 1
                    color: row.played ? Theme.textFaint : Theme.textDim
                }
                Icon { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; visible: row.playing; name: "play"; size: Theme.px(12); color: Theme.accent }
            }
            Text { Layout.fillWidth: true; Layout.preferredWidth: 6; text: list.compact && list.episodes && row.subtitle !== "" ? row.subtitle : row.title; elide: Text.ElideRight; color: row.ink }
            Text { visible: !list.compact; Layout.fillWidth: true; Layout.preferredWidth: 5; text: row.subtitle; elide: Text.ElideRight; color: row.played ? Theme.textFaint : Theme.textDim }
            Text { Layout.preferredWidth: Theme.px(60); horizontalAlignment: Text.AlignRight; text: row.duration > 0 ? Format.duration(row.duration) : ""; color: row.played ? Theme.textFaint : Theme.textDim }
            TyIconButton {
                opacity: rowArea.containsMouse || hovered ? 1 : 0
                iconName: "close"
                tip: "remove from the queue"
                onClicked: Player.remove(row.index)
            }
        }
    }
}
