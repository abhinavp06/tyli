import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The expanded player (features/player_ui.md §1.6, mockup player-expanded.html): over the
// window above the now-playing bar, a large cover with the title and links to the artist and
// album. The bar's queue button opens the queue as a panel on the right (its own × closes it),
// and the cover shrinks to stay visible. ↓ at the top right (also with the queue open), the
// bar's cover (now ↓), and Esc collapse it.
Rectangle {
    id: view
    objectName: "expandedPlayer"

    property bool queueOpen: false
    signal collapse()
    signal queueClosed()
    signal openLink(var link)

    color: Theme.background

    // Esc collapses: the view takes the keyboard while it's open, so a focused field elsewhere
    // (the search) can't swallow the key.
    focus: visible
    onVisibleChanged: if (visible) forceActiveFocus()
    Keys.onEscapePressed: collapse()


    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- "now playing", and ↓ at the top right
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.topBarHeight
            Layout.leftMargin: Theme.px(24)
            Layout.rightMargin: Theme.px(16)
            Text { text: "now playing"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
            Item { Layout.fillWidth: true }
            TyIconButton { objectName: "collapseButton"; iconName: "chevronDown"; tip: "collapse (Esc)"; onClicked: view.collapse() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- The cover and what plays
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - Theme.px(64), Theme.px(720))
                    spacing: Theme.px(22)
                    CoverImage {
                        objectName: "expandedCover"
                        Layout.alignment: Qt.AlignHCenter
                        // Smaller with the queue open, so both stay in view.
                        readonly property real side: view.queueOpen ? Math.min(view.height * 0.34, Theme.px(300))
                                                                    : Math.min(view.height * 0.52, Theme.px(480))
                        Layout.preferredWidth: side
                        Layout.preferredHeight: side
                        source: Library.thumbnail(Player.coverEntry)
                        online: true
                        placeholder: Player.hasVideo ? "movies" : "music"
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: Player.title
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: Theme.px(24)
                        font.weight: Theme.weightBold
                    }
                    // Music: artist · album as links, then the year and quality.
                    Row {
                        Layout.alignment: Qt.AlignHCenter
                        visible: Player.artist !== ""
                        spacing: Theme.px(10)
                        LinkText { objectName: "expandedArtist"; text: Player.artist; onActivated: view.openLink(Player.artistLink) }
                        Text { text: "·"; color: Theme.textFaint }
                        LinkText { objectName: "expandedAlbum"; text: Player.album; onActivated: view.openLink(Player.link) }
                        Text { visible: Player.detail !== ""; text: "·"; color: Theme.textFaint }
                        Text { visible: Player.detail !== ""; text: Player.detail; color: Theme.textDim }
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: Player.artist === ""
                        horizontalAlignment: Text.AlignHCenter
                        text: Player.subtitle
                        color: Theme.textDim
                    }
                }
            }

            // ---- The queue, as a panel on the right (shared with the video page)
            QueuePanel {
                objectName: "queuePanel"
                visible: view.queueOpen
                Layout.fillHeight: true
                Layout.preferredWidth: Theme.px(460)
                closeName: "closeQueuePanel"
                listName: "queuePanelList"
                onClosed: view.queueClosed()
            }
        }
    }
}
