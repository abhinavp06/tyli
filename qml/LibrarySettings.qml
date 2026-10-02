import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// Settings → Library: category pills; the selected category's actions, sync progress,
// and folders. Everything happens inline: adding a folder, last-sync details,
// relocating, confirming a removal, held removals.
Item {
    id: page

    property int selectedCategoryId: -1  // -1 = all folders
    readonly property var selectedCategory: Library.categories.find(c => c.id === selectedCategoryId)
    readonly property var shownFolders: selectedCategory ? Library.folders.filter(f => f.categoryId === selectedCategoryId)
                                                         : Library.folders
    readonly property int shownFiles: shownFolders.reduce((sum, f) => sum + f.fileCount, 0)

    // View state for this session.
    property var expanded: ({})        // folder id -> true: last-sync details open
    property var keptFolderIds: []     // held-removal lines dismissed with "keep for now"
    property bool addingFolder: false
    property string addError
    property bool addingCategory: false
    property bool editingCategory: false

    function folderCount(categoryId) { return Library.folders.filter(f => f.categoryId === categoryId).length }
    function toggleExpanded(id) {
        const copy = Object.assign({}, expanded)
        copy[id] = !copy[id]
        expanded = copy
    }

    onSelectedCategoryIdChanged: {
        addingFolder = false
        editingCategory = false
    }

    Connections {
        target: Library
        function onFolderOperationFinished(ok, message) {
            if (!page.addingFolder)
                return
            if (ok) {
                page.addingFolder = false
                page.addError = ""
            } else {
                page.addError = message
            }
        }
    }

    // Folder columns, shared by the header and the rows.
    readonly property int dotWidth: 16
    readonly property int filesWidth: 80
    readonly property int syncWidth: 170
    readonly property int markerWidth: 20
    readonly property int actionsWidth: 4 * Theme.controlHeight

    // ---- Small pieces used below
    component Label: Text {
        color: Theme.textDim
        font.pixelSize: Theme.fontSizeSmall
    }
    component KindChecks: Flow {
        property var kinds: []
        spacing: 12
        Repeater {
            model: Library.fileKinds
            TyCheckBox {
                required property string modelData
                text: modelData
                checked: parent.kinds.includes(modelData)
                onToggled: parent.kinds = checked ? parent.kinds.concat([modelData]) : parent.kinds.filter(k => k !== modelData)
            }
        }
    }
    component Chip: Rectangle {
        property alias text: chipText.text
        implicitWidth: chipText.implicitWidth + 16
        implicitHeight: 20
        color: "transparent"
        border.width: 1
        border.color: Theme.hairline
        Text {
            id: chipText
            anchors.centerIn: parent
            color: Theme.textDim
            font.pixelSize: Theme.fontSizeSmall
        }
    }
    // A path field with a browse icon. Typing a path works too.
    component PathField: RowLayout {
        property alias text: field.text
        property alias field: field
        spacing: 4
        TyField {
            id: field
            Layout.fillWidth: true
            placeholderText: "folder path, e.g. E:\\Media\\Music"
        }
        TyIconButton {
            iconName: "folder"
            tip: "browse for a folder"
            onClicked: dialog.open()
        }
        FolderDialog {
            id: dialog
            onAccepted: field.text = Library.localPath(selectedFolder)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        // ---- Category pills
        Flow {
            objectName: "categoryPills"
            Layout.fillWidth: true
            spacing: 8
            TyPill {
                text: "all"
                count: String(Library.folders.length)
                checked: page.selectedCategoryId === -1
                onClicked: page.selectedCategoryId = -1
            }
            Repeater {
                model: Library.categories
                TyPill {
                    required property var modelData
                    iconName: Format.categoryIcon(modelData)
                    text: modelData.name.toLowerCase()
                    count: String(page.folderCount(modelData.id))
                    checked: page.selectedCategoryId === modelData.id
                    onClicked: page.selectedCategoryId = modelData.id
                }
            }
            TyPill {
                iconName: "plus"
                tip: "add a category"
                onClicked: {
                    newCategoryName.text = ""
                    newCategoryKinds.kinds = ["audio"]
                    page.addingCategory = true
                    newCategoryName.forceActiveFocus()
                }
            }
        }

        // ---- New category (inline)
        RowLayout {
            visible: page.addingCategory
            Layout.fillWidth: true
            spacing: 16
            TyField {
                id: newCategoryName
                Layout.preferredWidth: 220
                placeholderText: "new category name"
                onAccepted: addCategoryButton.clicked()
            }
            KindChecks { id: newCategoryKinds; Layout.fillWidth: true }
            TyIconButton {
                id: addCategoryButton
                iconName: "check"
                primary: true
                tip: "add this category"
                enabled: newCategoryName.text.trim() !== ""
                onClicked: {
                    if (Library.addCategory(newCategoryName.text.trim(), newCategoryKinds.kinds)) {
                        page.addingCategory = false
                        const added = Library.categories.find(c => c.name === newCategoryName.text.trim())
                        if (added)
                            page.selectedCategoryId = added.id
                    }
                }
            }
            TyIconButton { iconName: "close"; tip: "cancel"; onClicked: page.addingCategory = false }
        }

        // ---- The selected category (or "all"): name, kinds, totals, actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: !page.editingCategory
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Row {
                    spacing: 12
                    Text {
                        text: page.selectedCategory ? page.selectedCategory.name.toLowerCase() : "all folders"
                        color: Theme.text
                        font.pixelSize: Theme.fontSizeHeading
                        font.weight: Theme.weightBold
                    }
                    Repeater {
                        model: page.selectedCategory ? page.selectedCategory.kinds : []
                        Chip { required property string modelData; text: modelData; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
                Text {
                    Layout.fillWidth: true  // lets this column (and so the row) take the width
                    color: Theme.textDim
                    text: {
                        let last = 0
                        for (const f of page.shownFolders)
                            last = Math.max(last, f.lastSync)
                        return Format.plural(page.shownFiles, "file") + " · " + Format.plural(page.shownFolders.length, "folder")
                             + " · last sync " + Format.ago(last)
                    }
                }
            }
            TyIconButton {
                objectName: "syncCategory"
                iconName: "sync"
                primary: true
                tip: page.selectedCategory ? "sync " + page.selectedCategory.name.toLowerCase() : "sync everything"
                enabled: page.shownFolders.length > 0
                onClicked: Library.requestSync(page.selectedCategory ? page.selectedCategory.id : -1)
            }
            TyIconButton {
                visible: page.selectedCategory !== undefined
                iconName: "edit"
                tip: "rename, choose its file kinds"
                onClicked: {
                    editName.text = page.selectedCategory.name
                    editKinds.kinds = page.selectedCategory.kinds
                    page.editingCategory = true
                }
            }
            TyIconButton {
                visible: page.selectedCategory !== undefined
                iconName: "trash"
                danger: true
                tip: "delete this category (only when it has no folders)"
                onClicked: {
                    if (Library.removeCategory(page.selectedCategory.id))
                        page.selectedCategoryId = -1
                }
            }
        }

        // ---- Which view the category opens (a TYLI preference; "automatic" decides from its files)
        Row {
            objectName: "viewChoice"
            visible: page.selectedCategory !== undefined && !page.editingCategory
            spacing: 16
            property string choice: page.selectedCategory ? Library.categoryViewChoice(page.selectedCategory.id) : ""
            Connections {
                target: Library
                function onCategoryViewsChanged() { parent.choice = page.selectedCategory ? Library.categoryViewChoice(page.selectedCategory.id) : "" }
            }
            Text { text: "view"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Repeater {
                model: [{ key: "", label: "automatic" }, { key: "music", label: "music" }, { key: "movies", label: "movies" }, { key: "shows", label: "shows" }]
                Text {
                    required property var modelData
                    readonly property bool on: parent.choice === modelData.key
                    text: modelData.label
                    color: on ? Theme.accent : choiceArea.containsMouse ? Theme.text : Theme.textDim
                    font.pixelSize: Theme.fontSizeSmall
                    MouseArea {
                        id: choiceArea
                        anchors.fill: parent
                        anchors.margins: -4
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Library.setCategoryView(page.selectedCategory.id, modelData.key)
                    }
                }
            }
        }

        // ---- Editing the category (inline)
        RowLayout {
            visible: page.editingCategory
            Layout.fillWidth: true
            spacing: 16
            TyField { id: editName; Layout.preferredWidth: 220; onAccepted: saveCategory.clicked() }
            KindChecks { id: editKinds; Layout.fillWidth: true }
            TyIconButton {
                id: saveCategory
                iconName: "check"
                primary: true
                tip: "save"
                enabled: editName.text.trim() !== ""
                onClicked: {
                    if (Library.updateCategory(page.selectedCategory.id, editName.text.trim(), editKinds.kinds))
                        page.editingCategory = false
                }
            }
            TyIconButton { iconName: "close"; tip: "cancel"; onClicked: page.editingCategory = false }
        }

        // ---- Sync progress
        RowLayout {
            Layout.fillWidth: true
            visible: Library.syncing
            spacing: 16
            TyProgress { width: 260; Layout.preferredWidth: 260 }
            Text {
                Layout.fillWidth: true
                color: Theme.textDim
                elide: Text.ElideRight
                text: {
                    const p = Library.syncProgress
                    if (p.folderName === undefined)
                        return "starting sync…"
                    return "syncing “" + p.folderName + "” · folder " + p.folderIndex + " of " + p.folderCount + " · "
                         + Format.plural(p.files, "file")
                }
            }
            TyIconButton { iconName: "stop"; tip: "cancel the sync (nothing is removed)"; onClicked: Library.cancelSync() }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

        // ---- Folder list header
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 16
            Item { Layout.preferredWidth: page.dotWidth }
            Label { Layout.preferredWidth: 200; text: "folder" }
            Label { Layout.fillWidth: true; text: "path" }
            Label { Layout.preferredWidth: page.filesWidth; horizontalAlignment: Text.AlignRight; text: "files" }
            Label { Layout.preferredWidth: page.syncWidth; text: "last sync" }
            Item { Layout.preferredWidth: page.markerWidth }
            Item {
                Layout.preferredWidth: page.actionsWidth
                Layout.preferredHeight: Theme.controlHeight
                TyIconButton {
                    objectName: "addFolder"
                    anchors.right: parent.right
                    iconName: "plus"
                    primary: true
                    enabled: page.selectedCategory !== undefined && !Library.folderOperationRunning
                    tip: page.selectedCategory ? "add a folder to " + page.selectedCategory.name.toLowerCase()
                                               : "pick a category to add a folder"
                    onClicked: {
                        addPath.text = ""
                        addName.text = ""
                        addMarker.checked = true
                        page.addError = ""
                        page.addingFolder = true
                        addPath.field.forceActiveFocus()
                    }
                }
            }
        }

        // ---- Folders
        ListView {
            id: folderList
            objectName: "folderList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            boundsBehavior: Flickable.StopAtBounds
            model: page.shownFolders
            ScrollBar.vertical: TyScrollBar {}

            delegate: Column {
                id: folder
                required property var modelData
                readonly property bool online: modelData.status === "online"
                readonly property bool open: page.expanded[modelData.id] === true
                readonly property var report: Library.folderReports[String(modelData.id)]
                property bool confirmingRemove: false
                property bool relocating: false
                width: ListView.view.width

                // The row
                Rectangle {
                    width: parent.width
                    height: 44
                    color: rowArea.containsMouse ? Theme.hover : "transparent"
                    MouseArea { id: rowArea; anchors.fill: parent; hoverEnabled: true; onClicked: page.toggleExpanded(folder.modelData.id) }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 16
                        Text {
                            Layout.preferredWidth: page.dotWidth
                            text: folder.online ? "●" : "○"
                            color: folder.online ? Theme.accent : Theme.textDim
                        }
                        Text {
                            Layout.preferredWidth: 200
                            text: folder.modelData.name
                            color: folder.online ? Theme.text : Theme.textDim
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: folder.modelData.path
                            color: folder.online ? Theme.textDim : Theme.textFaint
                            elide: Text.ElideMiddle
                        }
                        Text {
                            Layout.preferredWidth: page.filesWidth
                            horizontalAlignment: Text.AlignRight
                            text: Format.count(folder.modelData.fileCount)
                            color: folder.online ? Theme.text : Theme.textDim
                        }
                        Text {
                            Layout.preferredWidth: page.syncWidth
                            color: folder.online ? Theme.textDim : Theme.textFaint
                            elide: Text.ElideRight
                            text: folder.modelData.status === "unknown" ? "not synced yet"
                                : (folder.modelData.status === "offline" ? "offline · " : "") + Format.ago(folder.modelData.lastSync)
                        }
                        Icon {
                            Layout.preferredWidth: page.markerWidth
                            name: "flag"
                            color: folder.modelData.useMarker ? (folder.online ? Theme.accent : Theme.textDim) : Theme.textFaint
                            HoverHandler { id: markerHover }
                            ToolTip.visible: markerHover.hovered
                            ToolTip.text: folder.modelData.useMarker ? "has a .hoardor-root marker: found again if its drive letter changes"
                                                                     : "no marker: found by its location"
                        }
                        // Actions, or the removal confirmation in their place
                        Row {
                            Layout.preferredWidth: page.actionsWidth
                            layoutDirection: Qt.RightToLeft
                            visible: !folder.confirmingRemove
                            TyIconButton {
                                iconName: "trash"
                                danger: true
                                tip: "remove from the library (files on disk are untouched)"
                                onClicked: folder.confirmingRemove = true
                            }
                            TyIconButton {
                                iconName: "target"
                                tip: "relocate: point to its new location"
                                enabled: !Library.folderOperationRunning
                                onClicked: {
                                    relocatePath.text = folder.modelData.path
                                    folder.relocating = true
                                }
                            }
                            TyIconButton {
                                id: moveButton
                                iconName: "move"
                                tip: "move to another category"
                                enabled: Library.categories.length > 1
                                onClicked: moveMenu.popup(moveButton, 0, moveButton.height)
                                TyMenu {
                                    id: moveMenu
                                    Repeater {
                                        model: Library.categories.filter(c => c.id !== folder.modelData.categoryId)
                                        TyMenuItem {
                                            required property var modelData
                                            text: modelData.name.toLowerCase()
                                            onTriggered: Library.setFolderCategory(folder.modelData.id, modelData.id)
                                        }
                                    }
                                }
                            }
                            TyIconButton {
                                iconName: folder.open ? "chevronDown" : "chevronRight"
                                checked: folder.open
                                tip: folder.open ? "hide details" : "show the last sync and details"
                                onClicked: page.toggleExpanded(folder.modelData.id)
                            }
                        }
                        Row {
                            Layout.preferredWidth: page.actionsWidth
                            layoutDirection: Qt.RightToLeft
                            visible: folder.confirmingRemove
                            spacing: 4
                            TyIconButton { iconName: "close"; tip: "keep it"; onClicked: folder.confirmingRemove = false }
                            TyIconButton {
                                iconName: "check"
                                danger: true
                                tip: "yes, remove it (files on disk are untouched)"
                                onClicked: Library.removeFolder(folder.modelData.id)
                            }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: "remove?"; color: Theme.error }
                        }
                    }
                }

                // Relocate (inline)
                RowLayout {
                    visible: folder.relocating
                    width: parent.width - 52
                    x: 40
                    spacing: 8
                    Text { text: "now at"; color: Theme.textDim }
                    PathField { id: relocatePath; Layout.fillWidth: true; field.onAccepted: relocateOk.clicked() }
                    TyIconButton {
                        id: relocateOk
                        iconName: "check"
                        primary: true
                        tip: "relocate (checked by its marker or a sample of its files)"
                        onClicked: {
                            Library.relocateFolder(folder.modelData.id, relocatePath.text)
                            folder.relocating = false
                        }
                    }
                    TyIconButton { iconName: "close"; tip: "cancel"; onClicked: folder.relocating = false }
                }

                // Held removals (the mass-removal guard)
                Rectangle {
                    visible: folder.modelData.heldRemovals > 0 && !page.keptFolderIds.includes(folder.modelData.id)
                    x: 40
                    width: parent.width - 52
                    height: visible ? 40 : 0
                    color: "transparent"
                    Rectangle { width: Theme.lineWidth; height: parent.height; color: Theme.warning }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 4
                        Text {
                            Layout.fillWidth: true
                            color: Theme.text
                            elide: Text.ElideRight
                            text: "<font color='" + Theme.warning + "'>" + Format.plural(folder.modelData.heldRemovals, "file")
                                + " missing</font> (more than " + Library.settings.massRemovalThresholdPercent
                                + "%). removals are on hold."
                        }
                        TyIconButton {
                            iconName: "check"
                            danger: true
                            tip: "remove those " + Format.count(folder.modelData.heldRemovals) + " entries now"
                            onClicked: Library.applyHeldRemovals(folder.modelData.id)
                        }
                        TyIconButton {
                            iconName: "close"
                            tip: "keep them for now"
                            onClicked: page.keptFolderIds = page.keptFolderIds.concat([folder.modelData.id])
                        }
                    }
                }

                // Last sync and details (expanded)
                Column {
                    visible: folder.open
                    x: 52
                    width: parent.width - 64
                    topPadding: 2
                    bottomPadding: 12
                    spacing: 4
                    readonly property var errors: folder.open ? Library.scanErrors(folder.modelData.id) : []
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        color: Theme.textDim
                        textFormat: Text.StyledText
                        text: {
                            const r = folder.report
                            if (!r)
                                return "no sync yet this session"
                            const n = (v) => "<font color='" + Theme.text + "'>" + Format.count(v) + "</font>"
                            if (r.outcome === "offline")
                                return "last sync " + Format.ago(r.finishedAt) + " · offline: nothing was removed"
                            if (r.outcome === "cancelled")
                                return "last sync " + Format.ago(r.finishedAt) + " · cancelled: nothing was removed"
                            if (r.outcome === "failed")
                                return "last sync " + Format.ago(r.finishedAt) + " · <font color='" + Theme.error + "'>failed</font>"
                            return "last sync " + Format.ago(r.finishedAt) + " · +" + n(r.added) + " added · " + n(r.modified)
                                 + " modified · " + n(r.removed) + " removed · " + Format.count(r.unchanged) + " unchanged"
                                 + (r.unsettled > 0 ? " · <font color='" + Theme.warning + "'>" + Format.count(r.unsettled) + " settling</font>" : "")
                                 + (r.relocated ? " · found at a new location" : "")
                        }
                    }
                    Text {
                        visible: folder.report !== undefined && folder.report.message !== ""
                        width: parent.width
                        wrapMode: Text.Wrap
                        color: Theme.textDim
                        text: folder.report ? folder.report.message : ""
                    }
                    Repeater {
                        model: parent.errors
                        Text {
                            required property var modelData
                            width: parent.width
                            wrapMode: Text.WrapAnywhere
                            color: Theme.text
                            textFormat: Text.StyledText
                            text: "<font color='" + Theme.error + "'>×</font> "
                                + (modelData.path === "" ? "(the whole folder)" : modelData.path + (modelData.isDirectory ? "/" : ""))
                                + " <font color='" + Theme.textDim + "'>— " + modelData.message + "</font>"
                        }
                    }
                    Text {
                        color: Theme.textFaint
                        text: (folder.modelData.caseSensitive ? "case-sensitive" : "case-insensitive") + " drive (detected) · "
                            + (folder.modelData.useMarker ? "marker .hoardor-root" : "no marker")
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Theme.hairline }
            }

            Text {
                anchors.centerIn: parent
                visible: folderList.count === 0 && !page.addingFolder
                color: Theme.textDim
                text: page.selectedCategory ? "no folders in " + page.selectedCategory.name.toLowerCase() + " yet: add one with +"
                                            : "no folders yet: pick a category, then add one with +"
            }
        }

        // ---- Add a folder (inline, below the list)
        Column {
            Layout.fillWidth: true
            visible: page.addingFolder
            spacing: 4
            Rectangle {
                width: parent.width
                height: 48
                color: "transparent"
                Rectangle { width: Theme.lineWidth; height: parent.height; color: Theme.accent }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 16
                    Icon { Layout.preferredWidth: page.dotWidth; name: "plus"; color: Theme.accent }
                    PathField { id: addPath; Layout.fillWidth: true; Layout.preferredWidth: 3; field.onAccepted: addOk.clicked() }
                    TyField { id: addName; Layout.fillWidth: true; Layout.preferredWidth: 1; placeholderText: "name (optional)"; onAccepted: addOk.clicked() }
                    Row {
                        spacing: 0
                        TyIconButton {
                            id: addMarker
                            iconName: "flag"
                            checkable: true
                            checked: true
                            tip: checked ? "write a .hoardor-root marker (recommended): click to skip it"
                                         : "no marker: click to write one (recommended)"
                        }
                        TyIconButton {
                            id: addOk
                            iconName: "check"
                            primary: true
                            enabled: addPath.text.trim() !== "" && !Library.folderOperationRunning
                            tip: "add this folder"
                            onClicked: {
                                page.addError = ""
                                Library.addFolder(page.selectedCategoryId, addPath.text, addName.text, addMarker.checked)
                            }
                        }
                        TyIconButton { iconName: "close"; tip: "cancel"; onClicked: page.addingFolder = false }
                    }
                }
            }
            TyProgress { x: 40; width: 200; visible: Library.folderOperationRunning }
            Text {
                x: 40
                width: parent.width - 52
                visible: page.addError !== ""
                wrapMode: Text.Wrap
                color: Theme.error
                text: page.addError
            }
        }
    }
}
