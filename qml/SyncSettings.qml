import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// Settings → Sync: how syncs behave. The ranges come from hoardor's limits table.
Item {
    id: page

    property bool dirty: false
    readonly property var defaults: Library.defaultSettings()

    // [key, label, explanation]
    readonly property var fields: [
        ["settleWindowSeconds", "Settle window (seconds)",
         "A file changed this recently is probably still being copied: it's marked unsettled until a later sync."],
        ["massRemovalThresholdPercent", "Mass-removal threshold (%)",
         "A sync that would remove more than this share of a folder's files holds the removals for your confirmation."],
        ["batchMaxRows", "Batch size (rows)",
         "Database writes commit after this many rows…"],
        ["batchMaxMilliseconds", "Batch time (ms)",
         "…or this many milliseconds, so other writes never wait long."],
        ["progressIntervalFiles", "Progress interval (files)",
         "Sync progress is reported every this many files."],
        ["relocationSampleSize", "Relocation sample size",
         "Relocating a folder without a marker checks this many known files…"],
        ["relocationMinMatchPercent", "Relocation match (%)",
         "…and needs this share of them to be there, with the same size."]
    ]

    function load(settings) {
        startup.checked = settings.syncOnStartup
        for (let i = 0; i < spinners.count; ++i)
            spinners.itemAt(i).spin.value = settings[fields[i][0]]
    }

    function save() {
        const map = { syncOnStartup: startup.checked }
        for (let i = 0; i < spinners.count; ++i)
            map[fields[i][0]] = spinners.itemAt(i).spin.value
        if (Library.saveSettings(map))
            dirty = false
    }

    Component.onCompleted: { load(Library.settings); dirty = false }
    Connections {
        target: Library
        function onSettingsChanged() { if (!page.dirty) page.load(Library.settings) }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: "SYNC"
                font.bold: true
                opacity: 0.6
            }
            Button {
                text: "Reset to defaults"
                onClicked: { page.load(page.defaults); page.dirty = true }
            }
            Button {
                objectName: "saveSync"
                text: "Save"
                highlighted: page.dirty
                enabled: page.dirty
                onClicked: page.save()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label { text: "Sync at startup" }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            opacity: 0.6
                            text: "Sync everything in the background when TYLI starts. The window is usable right away."
                        }
                    }
                    CheckBox {
                        id: startup
                        Layout.preferredWidth: 160
                        onToggled: page.dirty = true
                    }
                    Label {
                        Layout.preferredWidth: 260
                        text: "default: " + (page.defaults.syncOnStartup ? "on" : "off")
                        opacity: 0.6
                    }
                }

                Repeater {
                    id: spinners
                    model: page.fields
                    RowLayout {
                        id: field
                        required property var modelData
                        property alias spin: spinBox
                        readonly property var range: Library.settingLimits[modelData[0]]
                        Layout.fillWidth: true
                        spacing: 16
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Label { text: field.modelData[1] }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                opacity: 0.6
                                text: field.modelData[2]
                            }
                        }
                        SpinBox {
                            id: spinBox
                            Layout.preferredWidth: 160
                            editable: true
                            from: field.range.min
                            to: field.range.max
                            onValueModified: page.dirty = true
                        }
                        Label {
                            Layout.preferredWidth: 260
                            text: "default: " + page.defaults[field.modelData[0]] + "  (allowed " + field.range.min + "–"
                                  + field.range.max + ")"
                            opacity: 0.6
                        }
                    }
                }
            }
        }
    }
}
