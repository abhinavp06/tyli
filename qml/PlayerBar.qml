import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The now-playing bar (features/player_ui.md §1.2, mockup player-music.html), shown while
// something is loaded: what plays (a link to its page), the transport with a seek line, and
// like, add to playlist (later), the queue, mute, and volume.
Rectangle {
    id: bar
    objectName: "playerBar"

    property bool queueOpen: false
    signal openLink(var link)
    signal queueToggled()

    implicitHeight: Theme.px(64)
    color: Theme.background

    Rectangle { width: parent.width; height: 1; color: Theme.hairline }

    // The heart reads Player.revision, so a like anywhere (a track row, this button) shows here too.
    // `Player.revision >= 0` is always true: it only makes the binding depend on it (a bare
    // `Player.revision;` statement can be compiled away, and then nothing re-evaluates).
    readonly property bool liked: Player.revision >= 0 && Player.isLiked(Player.entryId)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.px(16)
        anchors.rightMargin: Theme.px(16)
        spacing: Theme.px(24)

        // ---- What plays
        RowLayout {
            Layout.preferredWidth: Theme.px(320)
            Layout.maximumWidth: Theme.px(320)
            spacing: Theme.px(12)
            CoverImage {
                Layout.preferredWidth: Theme.px(44)
                Layout.preferredHeight: Theme.px(44)
                source: Library.thumbnail(Player.coverEntry)
                online: true
                placeholder: Player.hasVideo ? "movies" : "music"
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: Player.title || (Player.state === "loading" ? "loading…" : "")
                    elide: Text.ElideRight
                    color: titleArea.containsMouse ? Theme.accent : Theme.text
                    MouseArea {
                        id: titleArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bar.openLink(Player.link)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: Player.subtitle
                    elide: Text.ElideRight
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
        }

        // ---- Transport
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.px(2)
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.px(14)
                TyIconButton { objectName: "previousButton"; iconName: "previous"; tip: "previous (restarts the track after 3 s)"; onClicked: Player.previous() }
                Rectangle {
                    objectName: "playPauseButton"
                    implicitWidth: 36   // fixed, like the play button: not scaled
                    implicitHeight: 36
                    color: ppArea.containsMouse ? Theme.hover : "transparent"
                    border.width: 1
                    border.color: ppArea.containsMouse ? Theme.accent : Theme.accentDim
                    Icon { anchors.centerIn: parent; size: 16; name: Player.state === "playing" ? "pause" : "play"; color: Theme.accent }
                    MouseArea { id: ppArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Player.toggle() }
                    ToolTip.visible: ppArea.containsMouse
                    ToolTip.text: Player.state === "playing" ? "pause (space)" : "play (space)"
                    ToolTip.delay: 500
                }
                TyIconButton { objectName: "nextButton"; iconName: "next"; tip: "next"; onClicked: Player.next() }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: Theme.px(640)
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.px(10)
                Text { Layout.preferredWidth: Theme.px(52); horizontalAlignment: Text.AlignRight; text: Format.duration(Player.position); color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                SeekLine {
                    objectName: "seekLine"
                    Layout.fillWidth: true
                    value: Player.duration > 0 ? Player.position / Player.duration : 0
                    onMoved: (v) => Player.seek(Math.round(v * Player.duration))
                }
                Text { Layout.preferredWidth: Theme.px(52); text: Format.duration(Player.duration); color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            }
        }

        // ---- Like, playlist (later), queue, volume
        RowLayout {
            Layout.preferredWidth: Theme.px(320)
            Layout.maximumWidth: Theme.px(320)
            spacing: Theme.px(6)
            Item { Layout.fillWidth: true }
            Item {
                objectName: "likeButton"
                implicitWidth: Theme.controlHeight
                implicitHeight: Theme.controlHeight
                Icon {
                    anchors.centerIn: parent
                    name: "heart"
                    filled: bar.liked
                    color: bar.liked || likeArea.containsMouse ? Theme.accent : Theme.text
                }
                MouseArea {
                    id: likeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Player.setLiked(Player.entryId, !bar.liked)
                }
                ToolTip.visible: likeArea.containsMouse
                ToolTip.text: bar.liked ? "liked (click to unlike)" : "like"
                ToolTip.delay: 500
            }
            TyIconButton { iconName: "playlist"; tip: "add to a playlist (comes with playlists)"; enabled: false }
            TyIconButton {
                objectName: "queueButton"
                iconName: "queue"
                tip: "the queue (" + Math.max(0, Player.queueLength - Player.current - 1) + " next)"
                checked: bar.queueOpen
                onClicked: bar.queueToggled()
            }
            TyIconButton { iconName: Player.muted ? "mute" : "volume"; tip: Player.muted ? "unmute (m)" : "mute (m)"; onClicked: Player.setMuted(!Player.muted) }
            SeekLine {
                objectName: "volumeLine"
                Layout.preferredWidth: Theme.px(90)
                value: Player.muted ? 0 : Player.volume / 100
                onMoved: (v) => {
                    Player.setVolume(Math.round(v * 100))
                    if (Player.muted) Player.setMuted(false)
                }
            }
        }
    }
}
