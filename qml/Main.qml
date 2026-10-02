import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: "TYLI"

    function showMessage(title, text) {
        messageDialog.title = title
        messageDialog.text = text
        messageDialog.open()
    }

    function openSettings(subTab) {
        settingsButton.checked = true
        settings.currentIndex = subTab
    }

    header: ToolBar {
        visible: Library.ready
        RowLayout {
            anchors.fill: parent
            spacing: 0

            TabBar {
                id: tabs
                objectName: "mainTabs"
                Layout.fillWidth: true
                Repeater {
                    model: Library.categories
                    TabButton {
                        text: modelData.name
                        width: implicitWidth + 24
                        // A category tab always shows its section, even if Settings was open.
                        onClicked: settingsButton.checked = false
                    }
                }
            }
            ToolButton {
                id: settingsButton
                objectName: "settingsButton"
                text: "⚙ Settings"
                checkable: true
            }
        }
    }

    // The sections and Settings are siblings rather than one StackLayout, so new
    // categories can't change Settings' position.
    StackLayout {
        id: sections
        anchors.fill: parent
        visible: Library.ready && !settingsButton.checked
        currentIndex: tabs.currentIndex
        Repeater {
            model: Library.categories
            SectionView {
                // A delegate with required properties gets its model data only through them.
                required property var modelData
                category: modelData
                onOpenSettingsRequested: (subTab) => window.openSettings(subTab)
            }
        }
    }

    SettingsView {
        id: settings
        anchors.fill: parent
        visible: Library.ready && settingsButton.checked
    }

    // Shown instead of everything else when the library database can't be opened.
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 640)
        visible: !Library.ready
        spacing: 12
        Label {
            text: "The library can't be opened"
            font.pixelSize: 20
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: Library.openError
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            opacity: 0.7
            text: "Database: " + Library.databasePath
        }
    }

    Dialog {
        id: messageDialog
        property alias text: messageLabel.text
        anchors.centerIn: parent
        width: Math.min(window.width - 80, 520)
        modal: true
        standardButtons: Dialog.Ok
        Label {
            id: messageLabel
            width: parent.width
            wrapMode: Text.Wrap
        }
    }

    Connections {
        target: Library
        function onErrorOccurred(message) { window.showMessage("Something went wrong", message) }
        function onFolderOperationFinished(ok, message) {
            window.showMessage(ok ? "Done" : "That didn't work", message)
        }
    }
}
