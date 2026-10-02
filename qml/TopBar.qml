import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The bar across the window: the sidebar toggle and the wordmark on the left; the sync
// status and the settings gear on the right.
Rectangle {
    id: bar

    property bool settingsOpen: false
    signal toggleSidebar()
    signal settingsRequested()
    signal syncStatusClicked()

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
        anchors.leftMargin: 10
        anchors.rightMargin: 16
        spacing: 12

        TyIconButton {
            objectName: "sidebarToggle"
            iconName: "panel"
            tip: "collapse or expand the sidebar"
            onClicked: bar.toggleSidebar()
        }

        // tyli_ with a blinking green cursor
        Row {
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

        Item { Layout.fillWidth: true }

        // Sync status: a progress line while syncing, otherwise when the last sync finished.
        Rectangle {
            objectName: "syncStatus"
            Layout.preferredHeight: Theme.controlHeight
            Layout.preferredWidth: statusRow.implicitWidth + 16
            color: statusArea.containsMouse ? Theme.hover : "transparent"
            visible: Library.syncing || Library.lastReport.finishedAt !== undefined
            Row {
                id: statusRow
                anchors.centerIn: parent
                spacing: 12
                TyProgress {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 140
                    visible: Library.syncing
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: Theme.fontSizeSmall
                    color: Library.syncing ? Theme.textDim : Theme.textFaint
                    text: {
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
