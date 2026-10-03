import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// Settings → General: scanning (which files count as media), sync behavior, and about.
// Every change saves at once (hoardor validates it); a changed value gets a reset icon.
Item {
    id: page

    readonly property var settings: Library.settings
    readonly property var defaults: Library.defaultSettings()

    function save(map) { return Library.saveSettings(map) }

    // ---- Scanning helpers: settings.extensionKinds is [{extension, kind}]
    function extensionsOf(kind) {
        return settings.extensionKinds.filter(e => e.kind === kind).map(e => e.extension)
    }
    function removeExtension(extension) {
        save({ extensionKinds: settings.extensionKinds.filter(e => e.extension !== extension) })
    }
    function addExtensions(kind, text) {
        // Accepts "flac", ".flac", or several separated by spaces or commas.
        const added = text.split(/[\s,]+/).map(e => e.replace(/^\./, "").toLowerCase()).filter(e => e !== "")
        const rows = settings.extensionKinds.filter(e => !added.includes(e.extension))
        return save({ extensionKinds: rows.concat(added.map(e => ({ extension: e, kind: kind }))) })
    }
    function removeFrom(key, value) {
        const map = {}
        map[key] = settings[key].filter(v => v !== value)
        save(map)
    }
    function addTo(key, text) {
        const value = text.trim()
        if (value === "" || settings[key].includes(value))
            return true
        const map = {}
        map[key] = settings[key].concat([value])
        return save(map)
    }

    // ---- Small pieces
    component SectionTitle: Text {
        color: Theme.text
        font.pixelSize: Theme.fontSizeHeading
        font.weight: Theme.weightBold
    }
    component Chip: Rectangle {
        id: chip
        property alias text: chipText.text
        signal removed()
        implicitWidth: chipRow.implicitWidth + Theme.px(16)
        implicitHeight: Theme.px(22)
        color: "transparent"
        border.width: 1
        border.color: Theme.hairline
        Row {
            id: chipRow
            anchors.centerIn: parent
            spacing: Theme.px(6)
            Text { id: chipText; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.verticalCenter: parent.verticalCenter }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "×"
                color: removeArea.containsMouse ? Theme.error : Theme.textFaint
                font.pixelSize: Theme.fontSizeSmall
                MouseArea {
                    id: removeArea
                    anchors.fill: parent
                    anchors.margins: -4
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: chip.removed()
                }
            }
        }
    }
    // A row of chips with a + that turns into a small field.
    component ChipRow: RowLayout {
        id: chipRowRoot
        property string label
        property var values: []
        property string addTip
        signal removed(string value)
        signal added(string text)
        property bool adding: false
        spacing: Theme.px(16)
        Text { Layout.preferredWidth: Theme.px(110); Layout.alignment: Qt.AlignTop; topPadding: Theme.px(3); text: chipRowRoot.label; color: Theme.textDim }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(6)
            Repeater {
                model: chipRowRoot.values
                Chip { required property string modelData; text: modelData; onRemoved: chipRowRoot.removed(modelData) }
            }
            TyPill {
                visible: !chipRowRoot.adding
                implicitHeight: Theme.px(22)
                iconName: "plus"
                tip: chipRowRoot.addTip
                onClicked: { chipRowRoot.adding = true; addField.text = ""; addField.forceActiveFocus() }
            }
            TyField {
                id: addField
                visible: chipRowRoot.adding
                implicitHeight: Theme.px(22)
                implicitWidth: Theme.px(160)
                font.pixelSize: Theme.fontSizeSmall
                placeholderText: "type, then enter"
                onAccepted: { chipRowRoot.added(text); chipRowRoot.adding = false }
                onActiveFocusChanged: if (!activeFocus) chipRowRoot.adding = false
                Keys.onEscapePressed: chipRowRoot.adding = false
            }
        }
    }
    // A numeric setting: validated against hoardor's limits as you type, saved when you
    // leave the field or press enter.
    component NumberSetting: RowLayout {
        id: setting
        property string key
        property string title
        property string help
        // A hoardor setting by default; a UI preference overrides these four.
        property var range: Library.settingLimits[key]
        property int stored: page.settings[key]
        property int fallback: page.defaults[key]
        property var save: (value) => {
            const map = {}
            map[key] = value
            page.save(map)
        }
        readonly property bool valid: field.acceptableInput && Number(field.text) >= range.min && Number(field.text) <= range.max
        spacing: Theme.px(16)
        Layout.fillWidth: true
        onStoredChanged: field.text = String(stored)
        Component.onCompleted: field.text = String(stored)
        function saveValue(value) { save(value) }
        function commit() {
            if (valid && Number(field.text) !== stored)
                saveValue(Number(field.text))
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1  // only the width left over: long help wraps instead of running under the field
            spacing: 0
            Text { text: setting.title; color: Theme.text }
            Text { Layout.fillWidth: true; text: setting.help; color: Theme.textDim; wrapMode: Text.Wrap }
        }
        TyField {
            id: field
            Layout.preferredWidth: Theme.px(120)
            horizontalAlignment: TextInput.AlignRight
            validator: RegularExpressionValidator { regularExpression: /[0-9]{0,9}/ }
            invalid: !setting.valid
            onEditingFinished: setting.commit()
            Accessible.name: setting.title
        }
        Item {
            Layout.preferredWidth: Theme.controlHeight
            Layout.preferredHeight: Theme.controlHeight
            TyIconButton {
                anchors.fill: parent
                visible: setting.stored !== setting.fallback
                iconName: "reset"
                tip: "reset to " + setting.fallback
                onClicked: setting.saveValue(setting.fallback)
            }
        }
        Text {
            Layout.preferredWidth: Theme.px(280)
            color: setting.valid ? Theme.textFaint : Theme.error
            text: setting.valid ? "default " + setting.fallback + " · " + setting.range.min + "–" + setting.range.max
                                : "must be between " + setting.range.min + " and " + setting.range.max
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.implicitHeight + Theme.px(24)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: TyScrollBar {}

        ColumnLayout {
            id: content
            width: flick.width - Theme.px(16)
            spacing: Theme.px(22)

            // ---- Scanning
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.px(10)
                RowLayout {
                    SectionTitle { text: "scanning" }
                    Text { Layout.fillWidth: true; text: "· which files count as media"; color: Theme.textFaint }
                    TyIconButton {
                        iconName: "reset"
                        tip: "reset scanning to the defaults"
                        onClicked: page.save({ extensionKinds: page.defaults.extensionKinds, ignoredNames: page.defaults.ignoredNames,
                                               ignoredPrefixes: page.defaults.ignoredPrefixes })
                    }
                }
                Repeater {
                    model: Library.fileKinds
                    ChipRow {
                        required property string modelData
                        Layout.fillWidth: true
                        label: modelData
                        values: page.extensionsOf(modelData)
                        addTip: "add extensions to " + modelData
                        onRemoved: (value) => page.removeExtension(value)
                        onAdded: (text) => page.addExtensions(modelData, text)
                    }
                }
                ChipRow {
                    Layout.fillWidth: true
                    label: "skip names"
                    values: page.settings.ignoredNames
                    addTip: "add a file or folder name to skip"
                    onRemoved: (value) => page.removeFrom("ignoredNames", value)
                    onAdded: (text) => page.addTo("ignoredNames", text)
                }
                ChipRow {
                    Layout.fillWidth: true
                    label: "skip prefixes"
                    values: page.settings.ignoredPrefixes
                    addTip: "add a name prefix to skip"
                    onRemoved: (value) => page.removeFrom("ignoredPrefixes", value)
                    onAdded: (text) => page.addTo("ignoredPrefixes", text)
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

            // ---- Sync
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.px(8)
                SectionTitle { text: "sync" }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.px(16)
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1  // the width left over, as in NumberSetting
                        spacing: 0
                        Text { text: "sync at startup"; color: Theme.text }
                        Text { Layout.fillWidth: true; text: "sync everything in the background when tyli starts"; color: Theme.textDim }
                    }
                    TyCheckBox {
                        Layout.preferredWidth: Theme.px(120)
                        checked: page.settings.syncOnStartup
                        onToggled: page.save({ syncOnStartup: checked })
                        Accessible.name: "sync at startup"
                    }
                    Item { Layout.preferredWidth: Theme.controlHeight }
                    Text { Layout.preferredWidth: Theme.px(280); text: "default " + (page.defaults.syncOnStartup ? "on" : "off"); color: Theme.textFaint }
                }
                NumberSetting { key: "parallelDevices"; title: "drives at a time"; help: "sync and tag reading run one worker per physical drive (never two on one disk); this many drives at once" }
                NumberSetting { key: "settleWindowSeconds"; title: "settle window (s)"; help: "a file changed this recently is still being copied; checked again next sync" }
                NumberSetting { key: "massRemovalThresholdPercent"; title: "mass-removal guard (%)"; help: "hold removals above this share of a folder's files for your confirmation" }
                NumberSetting { key: "relocationSampleSize"; title: "relocation sample size"; help: "relocating a folder without a marker checks this many known files" }
                NumberSetting { key: "relocationMinMatchPercent"; title: "relocation match (%)"; help: "share of those files that must match to accept the new location" }
                Text { Layout.topMargin: Theme.px(8); text: "advanced"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                NumberSetting { key: "batchMaxRows"; title: "batch size (rows)"; help: "database writes commit after this many rows…" }
                NumberSetting { key: "batchMaxMilliseconds"; title: "batch time (ms)"; help: "…or this many milliseconds, so other writes never wait long" }
                NumberSetting { key: "progressIntervalFiles"; title: "progress interval (files)"; help: "sync progress is reported every this many files" }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

            // ---- Interface
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.px(8)
                SectionTitle { text: "interface" }
                NumberSetting {
                    objectName: "scrollSpeed"
                    title: "scroll speed (%)"
                    help: "how far one mouse-wheel notch scrolls, compared with the system's own (100 = the same)"
                    range: ({ min: 50, max: 500 })
                    stored: Library.scrollSpeed
                    fallback: Library.defaultScrollSpeed
                    save: (value) => { Library.scrollSpeed = value }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

            // ---- Track columns (album pages)
            ColumnLayout {
                objectName: "trackColumns"
                Layout.fillWidth: true
                spacing: Theme.px(8)
                RowLayout {
                    SectionTitle { text: "track columns" }
                    Text { Layout.fillWidth: true; text: "· what an album's track list shows, besides # and title"; color: Theme.textFaint }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: Theme.px(22)
                    Repeater {
                        model: Library.allTrackColumns
                        Row {
                            required property string modelData
                            spacing: Theme.px(8)
                            TyCheckBox {
                                checked: Library.trackColumns.includes(modelData)
                                onToggled: Library.trackColumns = checked ? Library.trackColumns.concat([modelData])
                                                                          : Library.trackColumns.filter(c => c !== modelData)
                                Accessible.name: modelData
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: ({ length: "length", bitrate: "bit rate", quality: "bit depth / sample rate", type: "file type",
                                         size: "size", channels: "channels" })[modelData]
                                color: Theme.text
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

            // ---- About
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.px(6)
                SectionTitle { text: "about" }
                Text { text: "tyli " + Qt.application.version; color: Theme.textDim }
                Text { Layout.fillWidth: true; text: "library " + Library.databasePath; color: Theme.textDim; wrapMode: Text.WrapAnywhere }
            }
        }
    }
}
