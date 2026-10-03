import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// "Add to a playlist" (features/playlists_ui.md §1.4): opened from a track's playlist icon (or
// the now-playing bar's). "new playlist…" asks for a name right here; then the playlists in the
// sidebar's order, with a check on the ones the track is already in (those can't be picked).
// The status line says what happened. One instance, in Main (Window.window.addToPlaylist).
Popup {
    id: menu
    objectName: "playlistMenu"

    property var entryIds: []
    property var holding: []        // the playlists a single track is already in
    property bool naming: false

    function openFor(ids, anchor, label) {
        entryIds = ids
        holding = ids.length === 1 ? Player.playlistsWith(ids[0]) : []
        naming = false
        // Under the icon, its right edge aligned with the icon's; above it near the bottom.
        const p = anchor.mapToItem(parent, anchor.width, anchor.height)
        x = Math.max(Theme.px(8), Math.min(p.x - width, parent.width - width - Theme.px(8)))
        const below = p.y + Theme.px(4)
        y = below + implicitHeight > parent.height - Theme.px(8) ? p.y - anchor.height - implicitHeight - Theme.px(4) : below
        open()
    }
    function addTo(id) {
        Player.addToPlaylist(id, entryIds)
        close()
    }

    parent: Overlay.overlay
    width: Theme.px(280)
    padding: 1
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.background
        border.width: 1
        border.color: Theme.hairline
    }

    contentItem: ColumnLayout {
        spacing: 0
        // "new playlist…", then a name field in its place.
        MenuRow {
            objectName: "menuNewPlaylist"
            visible: !menu.naming
            iconName: "plus"
            iconShown: true
            text: "new playlist…"
            onClicked: {
                menu.naming = true
                newName.text = ""
                newName.forceActiveFocus()
            }
        }
        NameField {
            id: newName
            objectName: "menuNewPlaylistName"
            visible: menu.naming
            Layout.fillWidth: true
            Layout.margins: Theme.px(6)
            placeholderText: "name"
            onConfirmed: (name) => {
                const id = Player.createPlaylist(name)
                if (id > 0)
                    menu.addTo(id)
            }
            onCancelled: menu.naming = false
        }
        Rectangle { visible: Player.playlists.length > 0; Layout.fillWidth: true; height: 1; color: Theme.hairline; Layout.topMargin: Theme.px(2); Layout.bottomMargin: Theme.px(2) }
        // Every playlist (tens or hundreds): a Column, so each row exists; it scrolls past 360 px.
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(rowsColumn.implicitHeight, Theme.px(360))
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentHeight: rowsColumn.implicitHeight
            ScrollBar.vertical: TyScrollBar {}
            Column {
                id: rowsColumn
                width: parent.width
                Repeater {
                    model: Player.playlists
                    MenuRow {
                        required property var modelData
                        readonly property bool inIt: menu.holding.indexOf(modelData.id) >= 0
                        objectName: "menuPlaylist-" + modelData.name
                        width: rowsColumn.width
                        iconName: "check"
                        iconShown: inIt
                        text: modelData.name
                        enabled: !inIt
                        tip: inIt ? "already in it" : ""
                        onClicked: menu.addTo(modelData.id)
                    }
                }
            }
        }
    }

    component MenuRow: Rectangle {
        id: mrow
        property string iconName
        property bool iconShown: false
        property string text
        property string tip
        signal clicked()
        Layout.fillWidth: true
        implicitHeight: Theme.controlHeight
        color: mArea.containsMouse && enabled ? Theme.hover : "transparent"
        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.px(12)
            spacing: Theme.px(10)
            Item {
                width: Theme.px(16)
                height: Theme.px(16)
                anchors.verticalCenter: parent.verticalCenter
                Icon { anchors.centerIn: parent; visible: mrow.iconShown; name: mrow.iconName; size: Theme.px(14); color: Theme.accent }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: mrow.width - Theme.px(12 + 16 + 10 + 12)
                text: mrow.text
                elide: Text.ElideRight
                color: !mrow.enabled ? Theme.textDim : mArea.containsMouse ? Theme.accent : Theme.text
            }
        }
        MouseArea {
            id: mArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: mrow.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            ToolTip.visible: containsMouse && mrow.tip !== ""
            ToolTip.text: mrow.tip
            ToolTip.delay: 500
            onClicked: if (mrow.enabled) mrow.clicked()
        }
    }
}
