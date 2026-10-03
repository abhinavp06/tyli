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
            // "save as playlist": a name, then a new playlist of everything queued.
            NameField {
                id: saveName
                objectName: "saveQueueName"
                visible: false
                Layout.preferredWidth: Theme.px(280)
                placeholderText: "playlist name"
                onConfirmed: (name) => {
                    const id = Player.createPlaylist(name)
                    if (id > 0) {
                        Player.addToPlaylist(id, Player.queueIds())
                        visible = false
                    }
                }
                onCancelled: visible = false
            }
            LinkText {
                objectName: "saveQueue"
                visible: queue.count > 0 && !saveName.visible
                text: "save as playlist"
                onActivated: {
                    saveName.text = ""
                    saveName.visible = true
                    saveName.forceActiveFocus()
                }
            }
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

        QueueList {
            objectName: "queueList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: queue
        }
    }
}
