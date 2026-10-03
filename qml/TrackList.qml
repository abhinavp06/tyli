import QtQuick
import QtQuick.Controls
import Tyli
import "Format.js" as Format

// An album copy's tracks: number, title, then the columns chosen in Settings → General
// (length and bit rate by default). Disc headings ("CD 1") when the album has several discs.
// Liked and playlist columns are placeholders until playlists exist.
ListView {
    id: list

    property var tracks: []  // track maps (LibraryBridge::albumCopies)
    property string albumArtist
    readonly property bool severalDiscs: tracks.some(t => t.disc > 1) || tracks.some(t => t.discTotal > 1)
    readonly property var columns: Library.trackColumns
    signal trackClicked(var track)

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    // Tracks with a heading row before each disc ("CD 1"), when the album has several.
    readonly property var rows: {
        const out = []
        let disc = null
        for (const t of tracks) {
            if (severalDiscs && t.disc !== disc) {
                disc = t.disc
                out.push({ heading: "CD " + (disc > 0 ? disc : "?") })
            }
            out.push(t)
        }
        return out
    }
    model: rows
    ScrollBar.vertical: TyScrollBar {}

    header: Row {
        height: Theme.px(30)
        spacing: Theme.px(16)
        leftPadding: Theme.px(12)
        Text { width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.bottom: parent.bottom; anchors.bottomMargin: Theme.px(6) }
        Text { width: list.titleWidth; text: "title"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.bottom: parent.bottom; anchors.bottomMargin: Theme.px(6) }
        Repeater {
            model: list.columns
            Text {
                required property string modelData
                width: list.columnWidth(modelData)
                horizontalAlignment: Text.AlignRight
                text: list.columnTitle(modelData)
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.px(6)
            }
        }
    }

    readonly property real titleWidth: Math.max(Theme.px(160), width - Theme.px(12 + 28 + 16) - columns.reduce((w, c) => w + columnWidth(c) + Theme.px(16), 0) - Theme.px(2 * (22 + 16)))

    function columnWidth(c) {
        return Theme.px(({ length: 60, bitrate: 90, quality: 110, type: 56, size: 80, channels: 40 })[c] ?? 70)
    }
    function columnTitle(c) {
        return ({ length: "length", bitrate: "bit rate", quality: "quality", type: "type", size: "size", channels: "ch" })[c] ?? c
    }
    function columnText(c, t) {
        switch (c) {
        case "length": return Format.duration(t.duration)
        case "bitrate": return t.bitrate > 0 ? Format.count(t.bitrate) + " kbps" : ""
        case "quality": return t.lossless ? (t.bitDepth || "?") + "/" + Format.kilohertz(t.sampleRate) : Format.kilohertz(t.sampleRate) + " kHz"
        case "type": return t.codec.startsWith("pcm") ? "pcm" : t.codec
        case "size": return Format.bytes(t.size)
        case "channels": return t.channels
        }
        return ""
    }

    delegate: Loader {
        id: row
        required property var modelData
        required property int index
        width: list.width
        sourceComponent: row.modelData.heading !== undefined ? heading : track

        Component {
            id: heading
            Row {
                height: Theme.px(34)
                spacing: Theme.px(8)
                leftPadding: Theme.px(12)
                Icon { anchors.bottom: parent.bottom; anchors.bottomMargin: Theme.px(7); name: "disc"; size: Theme.px(14); color: Theme.accentDim }
                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.px(6)
                    text: row.modelData.heading
                    color: Theme.accentDim
                    font.pixelSize: Theme.fontSizeSmall
                    font.letterSpacing: 1
                }
            }
        }
        // Read here so every row follows likes and what plays.
        readonly property bool playing: row.modelData.entryId !== undefined && row.modelData.entryId === Player.entryId
        readonly property bool liked: Player.revision >= 0 && row.modelData.entryId !== undefined && Player.isLiked(row.modelData.entryId)

        Component {
            id: track
            Rectangle {
                width: list.width
                height: Theme.px(32)
                color: area.containsMouse ? Theme.hover : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.px(16)
                    leftPadding: Theme.px(12)
                    Item {
                        width: Theme.px(28)
                        height: parent.height
                        Text {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !row.playing
                            text: row.modelData.track > 0 ? row.modelData.track : ""
                            color: Theme.textDim
                        }
                        Icon { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; visible: row.playing; name: "play"; size: Theme.px(12); color: Theme.accent }
                    }
                    Text {
                        width: list.titleWidth
                        text: row.modelData.title
                        elide: Text.ElideRight
                        color: row.playing ? Theme.accent : !row.modelData.online ? Theme.textFaint : row.modelData.fromName ? Theme.textDim : Theme.text
                    }
                    Repeater {
                        model: list.columns
                        Text {
                            required property string modelData
                            width: list.columnWidth(modelData)
                            horizontalAlignment: Text.AlignRight
                            text: list.columnText(modelData, row.modelData)
                            color: Theme.textDim
                        }
                    }
                    // A white outline; liked: filled with Theme.accent, so it follows the theme
                    // (the user, 2026-10-03). The playlist icon waits for playlists.
                    Item {
                        width: Theme.px(22)
                        height: parent.height
                        Icon {
                            anchors.centerIn: parent
                            name: "heart"
                            size: Theme.px(14)
                            filled: row.liked
                            color: row.liked || heartArea.containsMouse ? Theme.accent : Theme.text
                        }
                    }
                    Icon { name: "playlist"; size: Theme.px(14); color: Theme.textFaint; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: list.trackClicked(row.modelData)
                }
                // Over the row's own click: the heart likes, it doesn't play.
                MouseArea {
                    id: heartArea
                    objectName: "heart-" + row.modelData.entryId
                    x: Theme.px(12) + Theme.px(28) + list.titleWidth + list.columns.reduce((w, c) => w + list.columnWidth(c) + Theme.px(16), 0) + Theme.px(16 * 2)
                    width: Theme.px(22)
                    height: parent.height
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Player.setLiked(row.modelData.entryId, !row.liked)
                }
            }
        }
    }
}
