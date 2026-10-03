import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The bar across the window: the sidebar toggle and the wordmark (home) on the left; the sync
// status and the settings gear on the right.
Rectangle {
    id: bar

    property bool settingsOpen: false
    property alias searchText: search.text
    signal toggleSidebar()
    signal settingsRequested()
    signal syncStatusClicked()
    signal homeRequested()

    implicitHeight: Theme.topBarHeight
    color: Theme.background

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.hairline
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.px(10)
        anchors.rightMargin: Theme.px(16)
        spacing: Theme.px(12)

        TyIconButton {
            objectName: "sidebarToggle"
            iconName: "panel"
            tip: "collapse or expand the sidebar"
            onClicked: bar.toggleSidebar()
        }

        // tyli_ with a blinking green cursor; it opens the home page.
        Row {
            objectName: "wordmark"
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: bar.homeRequested() }
            Text {
                text: "tyli"
                color: Theme.text
                font.pixelSize: Theme.fontSizeHeading
                font.weight: Theme.weightBold
            }
            Text {
                id: cursor
                text: "_"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeHeading
                font.weight: Theme.weightBold
                Timer {
                    interval: 550
                    repeat: true
                    running: true
                    onTriggered: cursor.opacity = cursor.opacity > 0 ? 0 : 1
                }
            }
        }

        Item { Layout.preferredWidth: Theme.px(24) }

        // Search across the library; results replace the page while there's text (Esc clears).
        TyField {
            id: search
            objectName: "searchField"
            Layout.preferredWidth: Theme.px(300)
            placeholderText: "search"
            font.pixelSize: Theme.fontSizeSmall
            Keys.onEscapePressed: text = ""
        }

        Item { Layout.fillWidth: true }

        // Sync status: a progress line while syncing, otherwise when the last sync finished.
        Rectangle {
            objectName: "syncStatus"
            Layout.preferredHeight: Theme.controlHeight
            Layout.preferredWidth: statusRow.implicitWidth + Theme.px(16)
            color: statusArea.containsMouse ? Theme.hover : "transparent"
            visible: Library.syncing || Library.reading || Library.lastReport.finishedAt !== undefined
            Row {
                id: statusRow
                anchors.centerIn: parent
                spacing: Theme.px(12)
                TyProgress {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.px(140)
                    visible: Library.syncing || Library.reading
                    // A scan's total is unknown; reading tags counts down a known list.
                    indeterminate: Library.syncing || Library.metadataProgress.total === undefined
                    value: Library.metadataProgress.total ? Library.metadataProgress.done / Library.metadataProgress.total : 0
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: Theme.fontSizeSmall
                    color: Library.syncing || Library.reading ? Theme.textDim : Theme.textFaint
                    text: {
                        const m = Library.metadataProgress
                        if (!Library.syncing && Library.reading)
                            return m.total === undefined ? "reading tags…"
                                 : "reading tags · " + Format.count(m.done) + " / " + Format.count(m.total) + (m.perSecond > 0 ? " · " + m.perSecond + "/s" : "")
                        if (Library.syncing) {
                            const p = Library.syncProgress
                            return p.folderName === undefined ? "starting sync…"
                                 : "syncing " + p.folderName + " · " + Format.plural(p.files, "file")
                        }
                        return Library.lastReport.cancelled ? "sync cancelled " + Format.ago(Library.lastReport.finishedAt)
                                                            : "synced " + Format.ago(Library.lastReport.finishedAt)
                    }
                }
            }
            MouseArea {
                id: statusArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.syncStatusClicked()
            }
        }

        TyIconButton {
            objectName: "settingsButton"
            iconName: "gear"
            tip: bar.settingsOpen ? "close settings" : "settings"
            checked: bar.settingsOpen
            onClicked: bar.settingsRequested()
        }
    }
}
