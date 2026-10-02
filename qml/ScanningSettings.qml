import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// Settings → Scanning: which files count as media (extension → kind), and what to skip.
// Edits stay local until Save; hoardor validates them.
Item {
    id: page

    property bool dirty: false

    function load(settings) {
        kindsModel.clear()
        for (const row of settings.extensionKinds)
            kindsModel.append({ extension: row.extension, kind: row.kind })
        namesArea.text = settings.ignoredNames.join("\n")
        prefixesArea.text = settings.ignoredPrefixes.join("\n")
    }

    function lines(text) {
        return text.split("\n").map(s => s.trim()).filter(s => s !== "")
    }

    function save() {
        const kinds = []
        for (let i = 0; i < kindsModel.count; ++i)
            kinds.push({ extension: kindsModel.get(i).extension, kind: kindsModel.get(i).kind })
        if (Library.saveSettings({ extensionKinds: kinds, ignoredNames: lines(namesArea.text),
                                   ignoredPrefixes: lines(prefixesArea.text) }))
            dirty = false
    }

    Component.onCompleted: { load(Library.settings); dirty = false }
    Connections {
        target: Library
        function onSettingsChanged() { if (!page.dirty) page.load(Library.settings) }
    }

    ListModel { id: kindsModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: "SCANNING"
                font.bold: true
                opacity: 0.6
            }
            Button {
                text: "Reset to defaults"
                onClicked: { page.load(Library.defaultSettings()); page.dirty = true }
            }
            Button {
                objectName: "saveScanning"
                text: "Save"
                highlighted: page.dirty
                enabled: page.dirty
                onClicked: page.save()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // Extension → kind
            ColumnLayout {
                Layout.fillHeight: true
                Layout.preferredWidth: 380
                Label {
                    text: "File extensions (" + kindsModel.count + ")"
                    font.bold: true
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Files with other extensions aren't media and are skipped."
                }
                ListView {
                    id: kindsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: kindsModel
                    ScrollBar.vertical: ScrollBar {}
                    delegate: RowLayout {
                        id: kindRow
                        required property int index
                        required property string extension
                        required property string kind
                        width: ListView.view.width - 16
                        spacing: 8
                        TextField {
                            Layout.fillWidth: true
                            text: kindRow.extension
                            onTextEdited: { kindsModel.setProperty(kindRow.index, "extension", text); page.dirty = true }
                        }
                        ComboBox {
                            Layout.preferredWidth: 130
                            model: Library.fileKinds
                            currentIndex: Library.fileKinds.indexOf(kindRow.kind)
                            onActivated: (i) => { kindsModel.setProperty(kindRow.index, "kind", Library.fileKinds[i]); page.dirty = true }
                        }
                        ToolButton {
                            text: "✕"
                            onClicked: { kindsModel.remove(kindRow.index); page.dirty = true }
                        }
                    }
                }
                Button {
                    text: "+ Add extension"
                    onClicked: {
                        kindsModel.insert(0, { extension: "", kind: "audio" })
                        kindsList.positionViewAtBeginning()
                        page.dirty = true
                    }
                }
            }

            // Ignored names and prefixes
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: "Ignored names (one per line)"
                    font.bold: true
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Files and folders with exactly these names are skipped (ignoring case). A skipped folder isn't entered."
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TextArea {
                        id: namesArea
                        onTextChanged: if (activeFocus) page.dirty = true
                    }
                }
                Label {
                    text: "Ignored prefixes (one per line)"
                    font.bold: true
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Names starting with these are skipped (ignoring case), e.g. \".\" for hidden files."
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TextArea {
                        id: prefixesArea
                        onTextChanged: if (activeFocus) page.dirty = true
                    }
                }
            }
        }
    }
}
