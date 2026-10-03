import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// A playlist, or Liked Songs (playlistId -1), laid out like an album (features/playlists_ui.md,
// mockups playlist.html, liked.html): a cover (a mosaic of its first albums; a heart for Liked
// Songs), its name (the pencil renames it in a field with ✓ and ×), "N tracks · length ·
// updated", delete (an inline confirm) and play at the top right, then its tracks. A row plays
// the list from there; on hover its number becomes a handle to drag it, and × removes it.
// Liked Songs isn't edited by hand: the heart unlikes, and it has a "liked" column.
ColumnLayout {
    id: page
    objectName: "playlistPage"

    property var playlistId: 0
    readonly property bool liked: playlistId === -1
    property var info: ({})
    property bool renaming: false
    property bool confirmingDelete: false
    signal playRequested(var ids, int start, var addIds, string label, var source)
    signal openPage(int categoryId, string pageName, var params)
    signal deleted()

    // Reads playlistId itself, not `liked`: on a switch this runs before that binding updates.
    function refresh() {
        const isLiked = playlistId === -1
        info = isLiked ? Player.likedInfo() : (playlistId > 0 ? Player.playlistInfo(playlistId) : ({}))
        covers = isLiked ? [] : rows.coverEntries(4)
    }
    property var covers: []
    readonly property var source: liked ? ({ kind: "liked" }) : ({ kind: "playlist", id: playlistId })
    // What plays was started from this page (its rows mark the playing one).
    readonly property bool playingHere: Player.source?.kind === (liked ? "liked" : "playlist") && (liked || Player.source?.id === playlistId)
    function playFrom(index) {
        const ids = rows.entryIds()
        if (ids.length > 0)
            playRequested(ids, Math.max(0, index), index >= 0 ? [ids[index]] : ids, index >= 0 ? "this track" : (liked ? "Liked Songs" : info.name), source)
    }

    onPlaylistIdChanged: { renaming = false; confirmingDelete = false; refresh() }
    Component.onCompleted: refresh()
    Connections {
        target: Player
        function onPlaylistsChanged() { page.refresh() }
        function onStatesChanged() { if (page.liked) page.refresh() }
    }

    PlaylistModel {
        id: rows
        playlistId: page.playlistId
        onCountChanged: page.covers = page.liked ? [] : coverEntries(4)
    }

    spacing: Theme.px(16)

    // ---- The hero, like an album's
    Item {
        Layout.fillWidth: true
        implicitHeight: art.height

        Item {
            id: art
            width: Theme.px(220)
            height: width
            // Liked Songs: a heart on dim green.
            Rectangle {
                anchors.fill: parent
                visible: page.liked
                color: "#04140a"
                border.width: 1
                border.color: Theme.accentDim
                Icon { anchors.centerIn: parent; name: "heart"; size: Theme.px(64); color: Theme.accent }
            }
            // A playlist: its first four albums as a 2×2 mosaic; fewer, the first one; none, the icon.
            Grid {
                anchors.fill: parent
                visible: !page.liked && page.covers.length >= 4
                columns: 2
                Repeater {
                    model: page.covers.length >= 4 ? page.covers : []
                    CoverImage {
                        required property var modelData
                        width: art.width / 2
                        height: art.height / 2
                        source: Library.thumbnail(modelData)
                    }
                }
            }
            CoverImage {
                anchors.fill: parent
                visible: !page.liked && page.covers.length < 4
                source: page.covers.length > 0 ? Library.thumbnail(page.covers[0]) : ""
                placeholder: "playlist"
            }
        }

        ColumnLayout {
            anchors.left: art.right
            anchors.leftMargin: Theme.px(28)
            anchors.right: actions.left
            anchors.rightMargin: Theme.px(16)
            anchors.bottom: art.bottom
            spacing: Theme.px(8)
            Text { text: "playlist"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
            RowLayout {
                visible: !page.renaming
                spacing: Theme.px(10)
                Text {
                    objectName: "playlistTitle"
                    Layout.maximumWidth: Theme.px(720)
                    text: page.liked ? "Liked Songs" : (page.info.name || "")
                    color: Theme.text
                    font.pixelSize: Theme.px(26)
                    font.weight: Theme.weightBold
                    elide: Text.ElideRight
                }
                TyIconButton {
                    objectName: "renamePlaylist"
                    visible: !page.liked
                    iconName: "pencil"
                    tip: "rename"
                    onClicked: {
                        renameField.text = page.info.name
                        page.renaming = true
                        renameField.forceActiveFocus()
                        renameField.selectAll()
                    }
                }
            }
            NameField {
                id: renameField
                objectName: "renameField"
                visible: page.renaming
                Layout.preferredWidth: Theme.px(460)
                font.pixelSize: Theme.px(20)
                implicitHeight: Theme.px(44)
                onConfirmed: (name) => { if (Player.renamePlaylist(page.playlistId, name)) page.renaming = false }
                onCancelled: page.renaming = false
            }
            Text {
                objectName: "playlistFacts"
                color: Theme.textDim
                text: {
                    const n = Number(page.info.tracks || 0)
                    const parts = [Format.plural(n, "track")]
                    if (Number(page.info.durationMs || 0) > 0)
                        parts.push(Format.duration(page.info.durationMs))
                    if (!page.liked && page.info.updatedMs)
                        parts.push("updated " + Format.ago(page.info.updatedMs))
                    return parts.join(" · ")
                }
            }
        }

        Row {
            id: actions
            anchors.top: parent.top
            anchors.right: parent.right
            spacing: Theme.px(10)
            // Delete: an inline "delete? / keep" (no popup).
            Row {
                visible: page.confirmingDelete
                spacing: Theme.px(14)
                anchors.verticalCenter: parent.verticalCenter
                Text { anchors.verticalCenter: parent.verticalCenter; text: "delete “" + (page.info.name || "") + "”?"; color: Theme.textDim }
                LinkText {
                    objectName: "confirmDeletePlaylist"
                    anchors.verticalCenter: parent.verticalCenter
                    text: "delete"
                    color: Theme.error
                    onActivated: if (Player.deletePlaylist(page.playlistId)) page.deleted()
                }
                LinkText { anchors.verticalCenter: parent.verticalCenter; text: "keep"; onActivated: page.confirmingDelete = false }
            }
            TyIconButton {
                objectName: "deletePlaylist"
                visible: !page.liked
                anchors.verticalCenter: parent.verticalCenter
                iconName: "trash"
                danger: true
                tip: "delete this playlist"
                onClicked: page.confirmingDelete = !page.confirmingDelete
            }
            PlayButton {
                objectName: "playPlaylist"
                tip: page.liked ? "play your Liked Songs" : "play the playlist"
                onClicked: page.playFrom(-1)
            }
        }
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

    // ---- The tracks
    readonly property real fixedColumns: Theme.px(12 + 28 + 16 * 7 + 70 + 22 + 22 + 22) + (liked ? Theme.px(110) : 0)
    ListView {
        id: list
        objectName: "playlistRows"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: rows
        ScrollBar.vertical: TyScrollBar {}
        readonly property real titleWidth: (width - page.fixedColumns) * 0.4
        readonly property real otherWidth: (width - page.fixedColumns) * 0.3
        // A row being dragged: the gap it would drop into (a green line).
        property int dropGap: -1

        header: Row {
            height: Theme.px(28)
            spacing: Theme.px(16)
            leftPadding: Theme.px(12)
            Text { width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: list.titleWidth; text: "title"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: list.otherWidth; text: "artist"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: list.otherWidth; text: "album"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { visible: page.liked; width: Theme.px(110 - 16); text: "liked"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: Theme.px(70); horizontalAlignment: Text.AlignRight; text: "length"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
        }

        delegate: Rectangle {
            id: row
            required property int index
            required property var itemId
            required property var entryId
            required property string title
            required property string artist
            required property string album
            required property string albumArtist
            required property var duration
            required property bool online
            required property bool liked
            required property var added
            required property var rootId
            readonly property bool playing: page.playingHere && Player.entryId === entryId
            property bool dragging: false
            objectName: "playlistRow-" + index
            width: ListView.view.width
            height: Theme.px(34)
            color: area.containsMouse || dragging ? Theme.hover : "transparent"

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: page.playFrom(row.index)
            }
            Rectangle { visible: row.playing; width: Theme.lineWidth; height: parent.height; color: Theme.accent }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.px(16)
                leftPadding: Theme.px(12)
                Item {
                    width: Theme.px(28)
                    height: row.height
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !row.playing && !(gripArea.enabled && (area.containsMouse || gripArea.containsMouse || row.dragging))
                        text: row.index + 1
                        color: Theme.textDim
                    }
                    Icon { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; visible: row.playing; name: "play"; size: Theme.px(12); color: Theme.accent }
                    // The drag handle, in place of the number on hover (a playlist, not Liked Songs).
                    Icon {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !row.playing && gripArea.enabled && (area.containsMouse || gripArea.containsMouse || row.dragging)
                        name: "grip"
                        size: Theme.px(14)
                        color: gripArea.containsMouse || row.dragging ? Theme.accent : Theme.textDim
                    }
                    MouseArea {
                        id: gripArea
                        objectName: "grip-" + row.index
                        anchors.fill: parent
                        enabled: !page.liked
                        hoverEnabled: true
                        preventStealing: true
                        cursorShape: row.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        onPressed: row.dragging = true
                        onPositionChanged: (mouse) => {
                            if (!row.dragging)
                                return
                            const y = mapToItem(list.contentItem, mouse.x, mouse.y).y - (list.headerItem ? list.headerItem.height : 0)
                            list.dropGap = Math.max(0, Math.min(list.count, Math.round(y / row.height)))
                        }
                        onReleased: {
                            const gap = list.dropGap
                            row.dragging = false
                            list.dropGap = -1
                            const to = gap > row.index ? gap - 1 : gap
                            if (gap >= 0 && to !== row.index)
                                Player.movePlaylistItem(page.playlistId, row.itemId, to)
                        }
                        onCanceled: { row.dragging = false; list.dropGap = -1 }
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: list.titleWidth
                    text: row.title
                    elide: Text.ElideRight
                    color: row.playing ? Theme.accent : row.online ? Theme.text : Theme.textFaint
                }
                Item {
                    width: list.otherWidth
                    height: row.height
                    LinkText {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, parent.width)
                        text: row.artist
                        elide: Text.ElideRight
                        color: Theme.textDim
                        onActivated: page.openPage(Library.categoryOfFolder(row.rootId), "artist", { name: row.albumArtist, crumb: row.albumArtist })
                    }
                }
                Item {
                    width: list.otherWidth
                    height: row.height
                    LinkText {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, parent.width)
                        text: row.album
                        elide: Text.ElideRight
                        color: Theme.textDim
                        onActivated: page.openPage(Library.categoryOfFolder(row.rootId), "album", { albumArtist: row.albumArtist, album: row.album, crumb: row.album })
                    }
                }
                Text {
                    visible: page.liked
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.px(110 - 16)
                    text: Format.ago(row.added)
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSizeSmall
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.px(70)
                    horizontalAlignment: Text.AlignRight
                    text: row.duration > 0 ? Format.duration(row.duration) : ""
                    color: Theme.textDim
                }
                // Like (Liked Songs: unlike), add to another playlist, remove from this one.
                IconCell {
                    objectName: "rowHeart-" + row.index
                    iconName: "heart"
                    filled: row.liked
                    tint: row.liked ? Theme.accent : Theme.text
                    tip: row.liked ? "unlike" : "like"
                    onClicked: Player.setLiked(row.entryId, !row.liked)
                }
                IconCell {
                    id: addCell
                    objectName: "rowAdd-" + row.index
                    iconName: "playlist"
                    tip: "add to a playlist"
                    onClicked: Window.window.addToPlaylist([row.entryId], addCell, "“" + row.title + "”")
                }
                IconCell {
                    objectName: "rowRemove-" + row.index
                    visible: !page.liked
                    shown: area.containsMouse || hovered
                    iconName: "close"
                    danger: true
                    tip: "remove from this playlist"
                    onClicked: Player.removeFromPlaylist(page.playlistId, [row.itemId])
                }
            }
        }

        // Where a dragged row would land.
        Rectangle {
            visible: list.dropGap >= 0
            y: (list.headerItem ? list.headerItem.height : 0) + list.dropGap * Theme.px(34) - list.contentY - 1
            width: list.width
            height: Theme.lineWidth
            color: Theme.accent
        }

        Text {
            anchors.centerIn: parent
            visible: list.count === 0
            text: page.liked ? "nothing liked yet: the heart on any track" : "no tracks yet: add some with the playlist icon on any track"
            color: Theme.textFaint
        }
    }

    // A small icon button in a row (22 px), over the row's own click.
    component IconCell: Item {
        id: cell
        property string iconName
        property bool filled: false
        property color tint: Theme.text
        property bool danger: false
        property bool shown: true
        property string tip
        readonly property bool hovered: cellArea.containsMouse
        signal clicked()
        width: Theme.px(22)
        height: Theme.px(34)
        Icon {
            anchors.centerIn: parent
            visible: cell.shown
            name: cell.iconName
            size: Theme.px(14)
            filled: cell.filled
            color: cellArea.containsMouse ? (cell.danger ? Theme.error : Theme.accent) : cell.tint
        }
        MouseArea {
            id: cellArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            ToolTip.visible: containsMouse
            ToolTip.text: cell.tip
            ToolTip.delay: 500
            onClicked: cell.clicked()
        }
    }
}
