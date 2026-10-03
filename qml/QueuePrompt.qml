import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// The queue prompt (features/player_ui.md §1.1, mockup player-music.html): something is
// playing and you clicked a track or an album's play button. The one deliberate popup
// (ARCHITECTURE, 2026-10-02). Esc cancels, Enter clears the queue and plays.
Popup {
    id: prompt
    objectName: "queuePrompt"

    property var entryIds: []      // "clear queue and play": all of them, from `start`
    property int start: 0
    property var addIds: []        // "add to queue": just the clicked track, or the whole album
    property string what: "this track"

    function ask(ids, from, toAdd, label) {
        entryIds = ids
        start = from
        addIds = toAdd
        what = label
        open()
    }
    function addToQueue() {
        Player.add(addIds)
        close()
    }
    function clearAndPlay() {
        Player.playNow(entryIds, start)
        close()
    }

    anchors.centerIn: Overlay.overlay
    width: Math.min(Theme.px(520), parent ? parent.width - Theme.px(48) : Theme.px(520))
    modal: true
    focus: true
    padding: Theme.px(20)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Theme.backdrop }
    background: Rectangle { color: Theme.background; border.width: 1; border.color: Theme.hairline }

    component Action: Text {
        id: action
        property bool primary: false
        signal activated()
        color: actionArea.containsMouse || primary ? Theme.accent : Theme.text
        MouseArea { id: actionArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: action.activated() }
    }

    contentItem: ColumnLayout {
        spacing: Theme.px(14)
        Keys.onReturnPressed: prompt.clearAndPlay()
        Keys.onEnterPressed: prompt.clearAndPlay()
        Text { text: "a queue is playing"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.text
            text: "Something is already playing (" + Math.max(0, Player.queueLength - Player.current - 1) + " left in the queue). What should "
                  + prompt.what + " do?"
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.px(22)
            Action { text: "cancel"; onActivated: prompt.close() }
            Action { objectName: "addToQueue"; text: "add to queue"; onActivated: prompt.addToQueue() }
            Action { objectName: "clearAndPlay"; text: "clear queue and play"; primary: true; onActivated: prompt.clearAndPlay() }
        }
    }
    onOpened: contentItem.forceActiveFocus()
}
