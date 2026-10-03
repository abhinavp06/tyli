import QtQuick
import QtQuick.Controls
import Tyli
import "Format.js" as Format

// The categories, each with its icon and name (no file counts: the user, 2026-10-03), then
// "playlists": Liked Songs (fixed on top), the pinned playlists, and the rest, each group in the
// user's order (drag to reorder within a group). A pin and a "playing from here" speaker sit on
// the right (features/playlists_ui.md). Collapses to icons only.
Rectangle {
    id: sidebar
    objectName: "sidebar"

    property bool collapsed: false
    property int currentCategoryId: -1
    property var currentPlaylist: 0   // the playlist page shown: -1 Liked Songs, an id, or 0
    signal categoryClicked(int id)
    signal playlistClicked(var id)    // -1: Liked Songs

    implicitWidth: collapsed ? Theme.sidebarCollapsedWidth : Theme.sidebarWidth
    color: Theme.background

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.hairline
    }

    readonly property int rowHeight: Theme.px(34)
    readonly property var pinned: Player.playlists.filter(p => p.pinned)
    readonly property var others: Player.playlists.filter(p => !p.pinned)
    // A drag in progress: its group ("pinned" / "others") and the gap it would drop into.
    property string dropGroup: ""
    property int dropGap: -1

    Flickable {
        anchors.fill: parent
        anchors.topMargin: Theme.px(14)
        anchors.rightMargin: 1
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: column.implicitHeight + Theme.px(14)
        ScrollBar.vertical: TyScrollBar {}

        Column {
            id: column
            width: parent.width

            // The categories: a Repeater, so every row exists (a ListView sized to its own
            // content would make none). `count` is the number of categories.
            Repeater {
                id: list
                objectName: "categoryList"
                model: Library.categories
                delegate: Rectangle {
                    id: item
                    required property var modelData
                    objectName: "sidebarItem-" + modelData.name.toLowerCase()
                    readonly property bool current: modelData.id === sidebar.currentCategoryId
                    width: column.width
                    height: Theme.px(34)
                    color: area.containsMouse ? Theme.hover : "transparent"

                    Accessible.role: Accessible.Button
                    Accessible.name: modelData.name

                    // The selected category's green marker line.
                    Rectangle {
                        visible: item.current
                        width: Theme.lineWidth
                        height: parent.height
                        color: Theme.accent
                    }
                    Row {
                        id: row
                        anchors.verticalCenter: parent.verticalCenter
                        x: Math.round((Theme.sidebarCollapsedWidth - Theme.iconSize) / 2)  // the icon stays put when collapsed
                        spacing: Theme.px(12)
                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: Format.categoryIcon(item.modelData)
                            color: item.current ? Theme.accent : Theme.textDim
                        }
                        Text {
                            visible: !sidebar.collapsed
                            anchors.verticalCenter: parent.verticalCenter
                            width: sidebar.width - row.x - Theme.iconSize - row.spacing - Theme.px(14)
                            text: item.modelData.name.toLowerCase()
                            color: item.current ? Theme.accent : Theme.text
                            elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sidebar.categoryClicked(item.modelData.id)
                        ToolTip.visible: sidebar.collapsed && containsMouse
                        ToolTip.text: item.modelData.name.toLowerCase()
                        ToolTip.delay: 300
                    }
                }
            }

            // ---- Playlists
            Item {
                width: parent.width
                height: Theme.px(46)
                Rectangle { y: Theme.px(14); width: parent.width; height: 1; color: Theme.hairline }
                Text {
                    visible: !sidebar.collapsed
                    x: Math.round((Theme.sidebarCollapsedWidth - Theme.iconSize) / 2)
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.px(6)
                    text: "playlists"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSizeSmall
                    font.letterSpacing: 1
                }
                TyIconButton {
                    objectName: "newPlaylistButton"
                    visible: !sidebar.collapsed
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.px(8)
                    anchors.bottom: parent.bottom
                    width: Theme.px(26)
                    height: Theme.px(26)
                    iconName: "plus"
                    tip: "new playlist"
                    onClicked: {
                        newName.visible = !newName.visible
                        if (newName.visible) {
                            newName.text = ""
                            newName.forceActiveFocus()
                        }
                    }
                }
            }
            NameField {
                id: newName
                objectName: "newPlaylistName"
                visible: false
                x: Theme.px(12)
                width: parent.width - Theme.px(24)
                placeholderText: "name"
                onConfirmed: (name) => {
                    const id = Player.createPlaylist(name)
                    if (id > 0) {
                        visible = false
                        sidebar.playlistClicked(id)
                    }
                }
                onCancelled: visible = false
            }
            PlaylistRow { objectName: "sidebarLiked"; liked: true; name: "Liked Songs" }
            Item {
                width: parent.width
                height: pinnedColumn.height
                Column {
                    id: pinnedColumn
                    width: parent.width
                    Repeater {
                        model: sidebar.pinned
                        PlaylistRow {
                            required property var modelData
                            required property int index
                            entry: modelData
                            name: modelData.name
                            group: "pinned"
                            groupIndex: index
                            groupSize: sidebar.pinned.length
                            container: pinnedColumn
                        }
                    }
                }
                DropLine { group: "pinned" }
            }
            Rectangle {
                visible: sidebar.pinned.length > 0 && sidebar.others.length > 0
                x: Theme.px(18)
                width: parent.width - Theme.px(36)
                height: 1
                color: Theme.hairline
            }
            Item {
                width: parent.width
                height: othersColumn.height
                Column {
                    id: othersColumn
                    width: parent.width
                    Repeater {
                        model: sidebar.others
                        PlaylistRow {
                            required property var modelData
                            required property int index
                            entry: modelData
                            name: modelData.name
                            group: "others"
                            groupIndex: index
                            groupSize: sidebar.others.length
                            container: othersColumn
                        }
                    }
                }
                DropLine { group: "others" }
            }
        }
    }

    // Where a dragged playlist would land: a green line in the gap.
    component DropLine: Rectangle {
        property string group
        visible: sidebar.dropGroup === group && sidebar.dropGap >= 0
        y: sidebar.dropGap * sidebar.rowHeight - 1
        width: parent.width
        height: Theme.lineWidth
        color: Theme.accent
    }

    // One playlist (or Liked Songs): icon, name, then on the right a green speaker while playing
    // from it and the pin (on hover; always on pinned ones). A press-and-drag reorders it within
    // its group; a click opens it.
    component PlaylistRow: Rectangle {
        id: plRow
        property var entry: null          // {id, name, pinned, …}; null for Liked Songs
        property bool liked: false
        property string name
        property string group
        property int groupIndex: 0
        property int groupSize: 0
        property Item container: null
        readonly property var key: liked ? -1 : (entry ? entry.id : 0)
        readonly property bool current: sidebar.currentPlaylist === key
        readonly property bool playing: Player.active && (liked ? Player.source.kind === "liked"
                                                             : Player.source.kind === "playlist" && Player.source.id === key)
        property bool dragging: false
        property real pressY: 0

        objectName: liked ? "sidebarLiked" : "sidebarPlaylist-" + name
        width: parent ? parent.width : 0
        height: sidebar.rowHeight
        color: plArea.containsMouse || dragging ? Theme.hover : "transparent"
        Accessible.role: Accessible.Button
        Accessible.name: name

        Rectangle { visible: plRow.current; width: Theme.lineWidth; height: parent.height; color: Theme.accent }
        Row {
            id: plContent
            anchors.verticalCenter: parent.verticalCenter
            x: Math.round((Theme.sidebarCollapsedWidth - Theme.iconSize) / 2)
            spacing: Theme.px(12)
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: plRow.liked ? "heart" : "playlist"
                color: plRow.current ? Theme.accent : Theme.textDim
            }
            Text {
                visible: !sidebar.collapsed
                anchors.verticalCenter: parent.verticalCenter
                width: plRow.width - plContent.x - Theme.iconSize - plContent.spacing - rightIcons.width - Theme.px(18)
                text: plRow.name
                color: plRow.current ? Theme.accent : Theme.text
                elide: Text.ElideRight
            }
        }
        MouseArea {
            id: plArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: plRow.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
            preventStealing: plRow.dragging
            ToolTip.visible: sidebar.collapsed && containsMouse
            ToolTip.text: plRow.name
            ToolTip.delay: 300
            onPressed: (mouse) => { plRow.pressY = mouse.y; plRow.dragging = false }
            onPositionChanged: (mouse) => {
                if (!pressed || plRow.liked || !plRow.container)
                    return
                if (!plRow.dragging && Math.abs(mouse.y - plRow.pressY) > Theme.px(6))
                    plRow.dragging = true
                if (plRow.dragging) {
                    const y = mapToItem(plRow.container, mouse.x, mouse.y).y
                    sidebar.dropGroup = plRow.group
                    sidebar.dropGap = Math.max(0, Math.min(plRow.groupSize, Math.round(y / sidebar.rowHeight)))
                }
            }
            onReleased: {
                if (plRow.dragging) {
                    const gap = sidebar.dropGap
                    plRow.dragging = false
                    sidebar.dropGroup = ""
                    sidebar.dropGap = -1
                    // The gap is in the list before the move; the index is after it.
                    const to = gap > plRow.groupIndex ? gap - 1 : gap
                    if (gap >= 0 && to !== plRow.groupIndex)
                        Player.movePlaylist(plRow.key, to)
                }
            }
            onCanceled: { plRow.dragging = false; sidebar.dropGroup = ""; sidebar.dropGap = -1 }
            onClicked: if (!plRow.dragging) sidebar.playlistClicked(plRow.key)
        }
        Row {
            id: rightIcons
            visible: !sidebar.collapsed
            anchors.right: parent.right
            anchors.rightMargin: Theme.px(10)
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.px(6)
            Icon {
                objectName: "playingFrom-" + plRow.name
                anchors.verticalCenter: parent.verticalCenter
                visible: plRow.playing
                name: "volume"
                size: Theme.px(14)
                color: Theme.accent
            }
            Item {
                visible: !plRow.liked
                width: Theme.px(22)
                height: Theme.px(22)
                anchors.verticalCenter: parent.verticalCenter
                Icon {
                    anchors.centerIn: parent
                    visible: plRow.entry !== null && (plRow.entry.pinned || plArea.containsMouse || pinArea.containsMouse)
                    name: "pin"
                    size: Theme.px(14)
                    filled: plRow.entry !== null && plRow.entry.pinned
                    color: pinArea.containsMouse ? Theme.accent : Theme.textDim
                }
                MouseArea {
                    id: pinArea
                    objectName: "pin-" + plRow.name
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    ToolTip.visible: containsMouse
                    ToolTip.text: plRow.entry && plRow.entry.pinned ? "unpin" : "pin"
                    ToolTip.delay: 500
                    onClicked: Player.setPlaylistPinned(plRow.key, !plRow.entry.pinned)
                }
            }
        }
    }
}
