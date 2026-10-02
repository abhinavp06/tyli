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
        height: 30
        spacing: 16
        leftPadding: 12
        Text { width: 28; horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.bottom: parent.bottom; anchors.bottomMargin: 6 }
        Text { width: list.titleWidth; text: "title"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.bottom: parent.bottom; anchors.bottomMargin: 6 }
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
                anchors.bottomMargin: 6
            }
        }
    }

    readonly property real titleWidth: Math.max(160, width - 12 - 28 - 16 - columns.reduce((w, c) => w + columnWidth(c) + 16, 0) - 2 * (22 + 16))

    function columnWidth(c) {
        return ({ length: 60, bitrate: 90, quality: 110, type: 56, size: 80, channels: 40 })[c] ?? 70
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
                height: 34
                spacing: 8
                leftPadding: 12
                Icon { anchors.bottom: parent.bottom; anchors.bottomMargin: 7; name: "disc"; size: 14; color: Theme.accentDim }
                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    text: row.modelData.heading
                    color: Theme.accentDim
                    font.pixelSize: Theme.fontSizeSmall
                    font.letterSpacing: 1
                }
            }
        }
        Component {
            id: track
            Rectangle {
                width: list.width
                height: 32
                color: area.containsMouse ? Theme.hover : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 16
                    leftPadding: 12
                    Text { width: 28; horizontalAlignment: Text.AlignRight; text: row.modelData.track > 0 ? row.modelData.track : ""; color: Theme.textDim }
                    Text {
                        width: list.titleWidth
                        text: row.modelData.title
                        elide: Text.ElideRight
                        color: !row.modelData.online ? Theme.textFaint : row.modelData.fromName ? Theme.textDim : Theme.text
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
                    Icon { name: "heart"; size: 14; color: Theme.textFaint; anchors.verticalCenter: parent.verticalCenter }
                    Icon { name: "playlist"; size: 14; color: Theme.textFaint; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: list.trackClicked(row.modelData)
                }
            }
        }
    }
}
