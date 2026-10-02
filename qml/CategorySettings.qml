import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// Settings → Categories: the library's sections and the file kinds each accepts.
Item {
    id: page

    // A row of checkboxes, one per file kind.
    component KindPicker: Flow {
        property var kinds: []
        spacing: 4
        Repeater {
            model: Library.fileKinds
            CheckBox {
                required property string modelData
                text: modelData
                checked: parent.kinds.includes(modelData)
                onToggled: parent.kinds = checked ? parent.kinds.concat([modelData])
                                                  : parent.kinds.filter(k => k !== modelData)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: "CATEGORIES"
            font.bold: true
            opacity: 0.6
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.7
            text: "Each category is a tab. Sync reads every media file in a category's folders; the accepted kinds will decide what its media list shows (the next feature)."
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: Library.categories
            ScrollBar.vertical: ScrollBar {}
            delegate: Frame {
                id: row
                required property var modelData
                readonly property bool changed: nameField.text !== modelData.name
                                                || kindPicker.kinds.slice().sort().join() !== modelData.kinds.slice().sort().join()
                width: ListView.view.width
                RowLayout {
                    anchors.fill: parent
                    spacing: 12
                    TextField {
                        id: nameField
                        Layout.preferredWidth: 200
                        text: row.modelData.name
                    }
                    KindPicker {
                        id: kindPicker
                        Layout.fillWidth: true
                        kinds: row.modelData.kinds
                    }
                    Button {
                        text: "Save"
                        enabled: row.changed && nameField.text.trim() !== ""
                        onClicked: Library.updateCategory(row.modelData.id, nameField.text, kindPicker.kinds)
                    }
                    Button {
                        // hoardor refuses while folders belong to it, and says why.
                        text: "Delete"
                        onClicked: Library.removeCategory(row.modelData.id)
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                spacing: 12
                TextField {
                    id: newName
                    Layout.preferredWidth: 200
                    placeholderText: "New category name"
                }
                KindPicker {
                    id: newKinds
                    Layout.fillWidth: true
                }
                Button {
                    text: "+ Add category"
                    enabled: newName.text.trim() !== ""
                    onClicked: {
                        if (Library.addCategory(newName.text, newKinds.kinds)) {
                            newName.text = ""
                            newKinds.kinds = []
                        }
                    }
                }
            }
        }
    }
}
