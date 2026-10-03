import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The queue (features/player_ui.md §1.3, mockup player-queue.html): a page like Settings,
// opened from the bar. Played tracks stay above, dim; the current one is green. A click
// jumps to a row, × removes it, "clear" stops and empties the queue.
Item {
    id: page
    objectName: "queuePage"

    readonly property int next: Math.max(0, Player.queueLength - Player.current - 1)
    QueueModel { id: queue }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: Theme.px(20)
        anchors.bottomMargin: Theme.px(12)
        spacing: Theme.px(14)

        RowLayout {
            spacing: Theme.px(16)
            Text { text: "queue"; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
            Text {
                Layout.alignment: Qt.AlignBottom
                text: queue.count === 0 ? "empty" : page.next + " next" + (queue.remainingDuration > 0 ? " · " + Format.duration(queue.remainingDuration) : "")
                color: Theme.textDim
            }
            Item { Layout.fillWidth: true }
            LinkText {
                objectName: "clearQueue"
                visible: queue.count > 0
                text: "clear"
                onActivated: Player.clear()
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

        Text {
            visible: queue.count === 0
            text: "nothing queued: click a track or an album's play button"
            color: Theme.textDim
        }

        ListView {
            objectName: "queueList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: queue
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
                    Text { Layout.fillWidth: true; Layout.preferredWidth: 6; text: row.title; elide: Text.ElideRight; color: row.ink }
                    Text { Layout.fillWidth: true; Layout.preferredWidth: 5; text: row.subtitle; elide: Text.ElideRight; color: row.played ? Theme.textFaint : Theme.textDim }
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
    }
}
