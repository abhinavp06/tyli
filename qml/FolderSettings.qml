import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Tyli

// Settings → Folders: add, recategorize, relocate, and remove the library's folders.
Item {
    id: page

    function categoryIndex(categoryId) {
        return Library.categories.findIndex(c => c.id === categoryId)
    }

    // A path field with a Browse… button. Typing a path works too (handy on the VM).
    component PathField: RowLayout {
        property alias text: field.text
        spacing: 8
        TextField {
            id: field
            Layout.fillWidth: true
            placeholderText: "Folder path, e.g. E:\\Media\\Music"
        }
        Button {
            text: "Browse…"
            onClicked: folderDialog.open()
        }
        FolderDialog {
            id: folderDialog
            onAccepted: field.text = Library.localPath(selectedFolder)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: "FOLDERS"
                font.bold: true
                opacity: 0.6
            }
            BusyIndicator {
                running: Library.folderOperationRunning
                visible: running
                Layout.preferredHeight: 32
                Layout.preferredWidth: 32
            }
            Button {
                text: "+ Add folder"
                enabled: !Library.folderOperationRunning
                onClicked: {
                    addPath.text = ""
                    addName.text = ""
                    addMarker.checked = true
                    addDialog.open()
                }
            }
        }

        ListView {
            id: folderList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: Library.folders
            ScrollBar.vertical: ScrollBar {}
            delegate: Frame {
                id: row
                required property var modelData
                width: ListView.view.width
                RowLayout {
                    anchors.fill: parent
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: row.modelData.name
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: row.modelData.path
                            opacity: 0.7
                            elide: Text.ElideMiddle
                        }
                    }
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: Library.categories
                        textRole: "name"
                        currentIndex: page.categoryIndex(row.modelData.categoryId)
                        onActivated: (index) => Library.setFolderCategory(row.modelData.id, Library.categories[index].id)
                    }
                    Label {
                        text: row.modelData.useMarker ? "marker ✓" : "marker ✗"
                        opacity: 0.8
                        ToolTip.visible: markerHover.hovered
                        ToolTip.text: row.modelData.useMarker
                                      ? "Recognized by its .hoardor-root marker file, wherever the drive is mounted"
                                      : "No marker: recognized by its location and volume"
                        HoverHandler { id: markerHover }
                    }
                    CheckBox {
                        text: "case-sensitive"
                        checked: row.modelData.caseSensitive
                        onToggled: Library.setFolderCaseSensitive(row.modelData.id, checked)
                    }
                    Button {
                        text: "Relocate…"
                        enabled: !Library.folderOperationRunning
                        onClicked: {
                            relocateDialog.folderId = row.modelData.id
                            relocateDialog.title = "Relocate “" + row.modelData.name + "”"
                            relocatePath.text = row.modelData.path
                            relocateDialog.open()
                        }
                    }
                    Button {
                        text: "Remove"
                        onClicked: {
                            removeDialog.folderId = row.modelData.id
                            removeDialog.title = "Remove “" + row.modelData.name + "”?"
                            removeDialog.open()
                        }
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: folderList.count === 0
                opacity: 0.7
                text: "No folders yet. Add the folders that hold your media."
            }
        }
    }

    Dialog {
        id: addDialog
        title: "Add a folder"
        anchors.centerIn: parent
        width: Math.min(page.width - 80, 600)
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: addCategory.currentIndex = Math.max(0, addCategory.currentIndex)
        onAccepted: Library.addFolder(Library.categories[addCategory.currentIndex].id, addPath.text, addName.text,
                                      addMarker.checked)
        Component.onCompleted: standardButton(Dialog.Ok).enabled = Qt.binding(() => addPath.text.trim() !== "")

        GridLayout {
            width: parent.width
            columns: 2
            columnSpacing: 12
            rowSpacing: 8
            Label { text: "Folder" }
            PathField { id: addPath; Layout.fillWidth: true }
            Label { text: "Category" }
            ComboBox {
                id: addCategory
                Layout.fillWidth: true
                model: Library.categories
                textRole: "name"
            }
            Label { text: "Name" }
            TextField {
                id: addName
                Layout.fillWidth: true
                placeholderText: "Optional (defaults to the folder's name)"
            }
            Item { width: 1; height: 1 }
            CheckBox {
                id: addMarker
                Layout.fillWidth: true
                text: "Write a .hoardor-root marker file (recommended: the folder is found again if its drive letter changes)"
                contentItem: Label {
                    leftPadding: addMarker.indicator.width + addMarker.spacing
                    text: addMarker.text
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Dialog {
        id: relocateDialog
        property int folderId: -1
        anchors.centerIn: parent
        width: Math.min(page.width - 80, 600)
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: Library.relocateFolder(folderId, relocatePath.text)
        ColumnLayout {
            width: parent.width
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: "Where is this folder now? It's checked by its marker file (or a sample of its files) before anything changes."
            }
            PathField { id: relocatePath; Layout.fillWidth: true }
        }
    }

    Dialog {
        id: removeDialog
        property int folderId: -1
        anchors.centerIn: parent
        width: Math.min(page.width - 80, 520)
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: Library.removeFolder(folderId)
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: "Its entries are removed from the library. Files on disk are untouched."
        }
    }
}
