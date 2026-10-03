import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The queue as a side panel: "queue", what's left, clear, ×, then the queue's rows (a click
// jumps there). The expanded player and the video page (a season's episodes) both use it.
Rectangle {
    id: panel

    property string closeName: "closeQueue"
    property string listName: "queueList"
    property bool episodes: false   // rows show "S1 E2 · title" (QueueList.episodes)
    signal closed()

    color: Theme.background
    QueueModel { id: queue }
    Rectangle { width: 1; height: parent.height; color: Theme.hairline }
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.px(20)
            Layout.rightMargin: Theme.px(12)
            Layout.preferredHeight: Theme.controlHeight + Theme.px(10)
            spacing: Theme.px(12)
            Text { text: "queue"; color: Theme.text }
            Text {
                text: Math.max(0, Player.queueLength - Player.current - 1) + " next"
                      + (queue.remainingDuration > 0 ? " · " + Format.duration(queue.remainingDuration) : "")
                color: Theme.textDim
            }
            Item { Layout.fillWidth: true }
            LinkText { visible: queue.count > 0; text: "clear"; onActivated: Player.clear() }
            TyIconButton { objectName: panel.closeName; iconName: "close"; tip: "close the queue"; onClicked: panel.closed() }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }
        QueueList {
            objectName: panel.listName
            Layout.fillWidth: true
            Layout.fillHeight: true
            compact: true
            episodes: panel.episodes
            model: queue
        }
    }
}
