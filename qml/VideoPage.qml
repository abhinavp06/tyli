import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml
import Tyli
import "Format.js" as Format

// The video page (features/player_ui.md §1.4, mockup player-video.html): the picture, with
// controls over it that hide (with the cursor) after 3 s without a mouse move. Keys: space,
// ← / → 10 s, f full screen, Esc leaves full screen, m mute.
Rectangle {
    id: page
    objectName: "videoPage"

    property bool fullScreen: false
    signal back()
    signal fullScreenToggled()

    // The picture needs Qt Quick on OpenGL (main.cpp). The software renderer (tests) has no
    // picture: the page is black with its controls, and the sound plays.
    readonly property bool canDraw: GraphicsInfo.api === GraphicsInfo.OpenGL
    readonly property bool videoReady: !canDraw || (picture.item && picture.item.ready)

    color: "#000000"

    Loader {
        id: picture
        anchors.fill: parent
        active: page.canDraw
        sourceComponent: VideoView {}
    }

    // ---- Showing and hiding the controls
    // An episode (not a movie): previous and next episode, through the season's queue.
    readonly property bool episode: Player.link !== undefined && Player.link.page === "show"
    readonly property bool hasPrevious: Player.current > 0
    readonly property bool hasNext: Player.current >= 0 && Player.current < Player.queueLength - 1
    property bool queueOpen: false   // the queue (a season's episodes) as a panel on the right
    property bool controlsShown: true
    function poke() {
        controlsShown = true
        hide.restart()
    }
    Timer { id: hide; interval: 3000; onTriggered: if (Player.state === "playing" && !page.queueOpen) page.controlsShown = false }
    Connections {
        target: Player
        function onStatusChanged() { if (Player.state !== "playing") page.controlsShown = true }
    }
    onEnabledChanged: if (enabled) { poke(); forceActiveFocus() }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: page.controlsShown ? Qt.ArrowCursor : Qt.BlankCursor
        onPositionChanged: page.poke()
        onClicked: { page.poke(); Player.toggle() }
        onDoubleClicked: page.fullScreenToggled()
    }

    focus: true
    Keys.onPressed: (event) => {
        page.poke()
        switch (event.key) {
        case Qt.Key_Space: Player.toggle(); break
        case Qt.Key_Left: Player.seekBy(-10000); break
        case Qt.Key_Right: Player.seekBy(10000); break
        case Qt.Key_F: page.fullScreenToggled(); break
        case Qt.Key_M: Player.setMuted(!Player.muted); break
        case Qt.Key_Q: page.queueOpen = !page.queueOpen; break
        case Qt.Key_N: if (page.episode && page.hasNext) Player.jump(Player.current + 1); break
        case Qt.Key_P: if (page.episode && page.hasPrevious) Player.jump(Player.current - 1); break
        case Qt.Key_Escape: if (page.fullScreen) page.fullScreenToggled(); else page.back(); break
        default: return
        }
        event.accepted = true
    }

    // "resumed at 42:10 · from the start", for a few seconds after resuming.
    property bool resumeShown: false
    Connections {
        target: Player
        function onStatusChanged() {
            if (Player.resumedFrom > 0 && Player.state === "playing" && !page.resumeSeen) {
                page.resumeShown = true
                page.resumeSeen = true
                resumeTimer.restart()
            }
        }
        function onNowPlayingChanged() { page.resumeSeen = false; page.resumeShown = false }
    }
    property bool resumeSeen: false
    Timer { id: resumeTimer; interval: 6000; onTriggered: page.resumeShown = false }

    // ---- Top: back, what plays
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: top.implicitHeight + Theme.px(20)
        color: Qt.rgba(0, 0, 0, 0.72)
        visible: page.controlsShown
        RowLayout {
            id: top
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.px(16)
            anchors.rightMargin: Theme.px(16)
            spacing: Theme.px(12)
            TyIconButton { objectName: "videoBack"; iconName: "back"; tip: "back (pauses; the bar keeps it)"; onClicked: page.back() }
            ColumnLayout {
                spacing: 0
                Text { text: Player.title; color: Theme.text }
                Text { text: Player.subtitle; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; visible: text !== "" }
            }
            Item { Layout.fillWidth: true }
            Row {
                visible: page.resumeShown
                spacing: Theme.px(6)
                Text { text: "resumed at " + Format.duration(Player.resumedFrom) + " ·"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                LinkText {
                    text: "from the start"
                    font.pixelSize: Theme.fontSizeSmall
                    onActivated: { Player.seek(0); page.resumeShown = false }
                }
            }
        }
    }

    // ---- Bottom: seek, transport, tracks, volume, full screen
    component TrackMenuButton: Text {
        id: menuButton
        property string label
        property var tracks: []
        property bool canTurnOff: false
        signal picked(int id)
        readonly property var current: tracks.find(t => t.selected)
        text: label + "  " + (current ? current.label : (canTurnOff ? "off" : "–"))
        color: menuArea.containsMouse ? Theme.accent : Theme.text
        visible: tracks.length > 0
        MouseArea { id: menuArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: menu.popup(menuButton, 0, -menu.height - Theme.px(6)) }
        TyMenu { id: menu }
        Instantiator {
            model: (menuButton.canTurnOff ? [{ id: -1, label: "off", selected: !menuButton.current }] : []).concat(menuButton.tracks)
            delegate: TyMenuItem {
                required property var modelData
                text: (modelData.selected ? "✓ " : "  ") + modelData.label
                onTriggered: menuButton.picked(modelData.id)
            }
            onObjectAdded: (index, object) => menu.insertItem(index, object)
            onObjectRemoved: (index, object) => menu.removeItem(object)
        }
    }

    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        width: parent.width
        height: bottom.implicitHeight + Theme.px(22)
        color: Qt.rgba(0, 0, 0, 0.72)
        visible: page.controlsShown
        ColumnLayout {
            id: bottom
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.px(16)
            anchors.rightMargin: Theme.px(16)
            spacing: Theme.px(8)
            SeekLine {
                objectName: "videoSeek"
                Layout.fillWidth: true
                value: Player.duration > 0 ? Player.position / Player.duration : 0
                onMoved: (v) => Player.seek(Math.round(v * Player.duration))
            }
            RowLayout {
                spacing: Theme.px(14)
                TyIconButton {
                    objectName: "previousEpisode"
                    visible: page.episode
                    enabled: page.hasPrevious
                    iconName: "previous"
                    tip: "previous episode (p)"
                    onClicked: Player.jump(Player.current - 1)
                }
                Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    color: ppArea.containsMouse ? Theme.hover : "transparent"
                    border.width: 1
                    border.color: ppArea.containsMouse ? Theme.accent : Theme.accentDim
                    Icon { anchors.centerIn: parent; size: 16; name: Player.state === "playing" ? "pause" : "play"; color: Theme.accent }
                    MouseArea { id: ppArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Player.toggle() }
                }
                TyIconButton {
                    objectName: "nextEpisode"
                    visible: page.episode
                    enabled: page.hasNext
                    iconName: "next"
                    tip: "next episode (n)"
                    onClicked: Player.jump(Player.current + 1)
                }
                TyIconButton { iconName: "rewind"; tip: "back 10 s (←)"; onClicked: Player.seekBy(-10000) }
                TyIconButton { iconName: "forward"; tip: "forward 10 s (→)"; onClicked: Player.seekBy(10000) }
                Text { text: Format.duration(Player.position) + " / " + Format.duration(Player.duration); color: Theme.textDim }
                Item { Layout.fillWidth: true }
                TrackMenuButton {
                    objectName: "audioMenu"
                    label: "audio"
                    tracks: Player.audioTracks
                    onPicked: (id) => Player.selectAudio(id)
                }
                TrackMenuButton {
                    objectName: "subtitleMenu"
                    label: "subtitles"
                    tracks: Player.subtitleTracks
                    canTurnOff: true
                    onPicked: (id) => Player.selectSubtitle(id)
                }
                // Right of the subtitles, left of the volume (the user, 2026-10-03).
                TyIconButton {
                    objectName: "videoQueueButton"
                    iconName: "queue"
                    checked: page.queueOpen
                    tip: page.queueOpen ? "close the queue (q)" : "the queue (q)" + (page.episode ? ": the season's episodes" : "")
                    onClicked: page.queueOpen = !page.queueOpen
                }
                TyIconButton { iconName: Player.muted ? "mute" : "volume"; tip: "mute (m)"; onClicked: Player.setMuted(!Player.muted) }
                SeekLine {
                    Layout.preferredWidth: Theme.px(90)
                    value: Player.muted ? 0 : Player.volume / 100
                    onMoved: (v) => { Player.setVolume(Math.round(v * 100)); if (Player.muted) Player.setMuted(false) }
                }
                TyIconButton { objectName: "fullScreenButton"; iconName: page.fullScreen ? "fullscreenExit" : "fullscreen"; tip: page.fullScreen ? "leave full screen (Esc)" : "full screen (f)"; onClicked: page.fullScreenToggled() }
            }
        }
    }

    // ---- The queue on the right: a season's episodes (or whatever the queue holds); a click jumps.
    // The controls stay up while it's open.
    QueuePanel {
        objectName: "videoQueuePanel"
        visible: page.queueOpen
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: bottomBar.top
        width: Math.min(Theme.px(460), parent.width * 0.45)
        closeName: "closeVideoQueue"
        listName: "videoQueueList"
        episodes: page.episode
        onClosed: page.queueOpen = false
    }
}
