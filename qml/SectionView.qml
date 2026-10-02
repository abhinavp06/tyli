import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One category's section: its folders, the files of the selected folder, Sync, and
// what the last sync did.
Item {
    id: section

    required property var category
    readonly property int categoryId: category.id
    readonly property var folders: Library.folders.filter(f => f.categoryId === categoryId)
    readonly property int fileCount: folders.reduce((sum, f) => sum + f.fileCount, 0)
    readonly property var report: Library.categoryReports[String(categoryId)]
    readonly property var selectedFolder: folders.find(f => f.id === selectedFolderId)

    property int selectedFolderId: -1
    // This section asked for a sync that hasn't finished yet (running or queued).
    property bool syncRequested: false
    readonly property bool syncRunningHere: Library.syncing
                                            && folders.some(f => f.id === Library.syncProgress.folderId)
    // Held-removal banners the user dismissed with "Keep for now", for this session.
    property var keptFolderIds: []

    signal openSettingsRequested(int subTab)

    onFoldersChanged: {
        if (!selectedFolder)
            selectedFolderId = folders.length > 0 ? folders[0].id : -1
    }

    Connections {
        target: Library
        function onSyncFinished(report) {
            if (report.categoryId === section.categoryId || report.categoryId === -1)
                section.syncRequested = false
            filesModel.reload()
        }
        function onSyncingChanged() {
            if (!Library.syncing)
                section.syncRequested = false  // cancelled, or everything finished
        }
    }

    FolderFilesModel {
        id: filesModel
        folderId: section.selectedFolderId
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ---- Header: name, totals, Sync
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Label {
                text: section.category.name
                font.pixelSize: 22
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                opacity: 0.7
                text: Format.plural(section.fileCount, "file") + " · " + Format.plural(section.folders.length, "folder")
            }
            Button {
                objectName: "syncButton"
                text: !section.syncRequested ? "Sync ⟳" : section.syncRunningHere ? "Syncing…" : "Queued…"
                enabled: !section.syncRequested && section.folders.length > 0
                onClicked: section.syncRequested = Library.requestSync(section.categoryId)
            }
        }

        // ---- Progress (only while a sync runs)
        RowLayout {
            Layout.fillWidth: true
            visible: Library.syncing
            spacing: 12
            ProgressBar {
                Layout.preferredWidth: 240
                indeterminate: true  // a scan doesn't know its total up front
            }
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: {
                    const p = Library.syncProgress
                    if (p.folderName === undefined)
                        return "Starting sync…"
                    return "Syncing “" + p.folderName + "” · folder " + p.folderIndex + " of " + p.folderCount
                            + " · " + Format.plural(p.files, "file")
                }
            }
            Button {
                text: "Cancel"
                onClicked: Library.cancelSync()
            }
        }

        // ---- Folders | files
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // Folders of this category
            Pane {
                SplitView.preferredWidth: 340
                SplitView.minimumWidth: 220
                padding: 0
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4
                    Label {
                        text: "FOLDERS"
                        font.bold: true
                        opacity: 0.6
                        leftPadding: 8
                        topPadding: 4
                    }
                    ListView {
                        id: folderList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: section.folders
                        delegate: ItemDelegate {
                            id: folderDelegate
                            required property var modelData
                            width: ListView.view.width
                            highlighted: modelData.id === section.selectedFolderId
                            // Offline: greyed out, but its files stay browsable (nothing touches the drive).
                            opacity: modelData.status === "offline" ? 0.55 : 1
                            onClicked: section.selectedFolderId = modelData.id
                            contentItem: ColumnLayout {
                                spacing: 2
                                RowLayout {
                                    Label {
                                        Layout.fillWidth: true
                                        text: (folderDelegate.modelData.status === "online" ? "● " : "○ ")
                                              + folderDelegate.modelData.name
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        text: folderDelegate.modelData.status === "unknown" ? "not synced yet"
                                                                                              : folderDelegate.modelData.status
                                        opacity: 0.7
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: folderDelegate.modelData.path
                                    opacity: 0.7
                                    elide: Text.ElideMiddle
                                }
                                Label {
                                    text: Format.plural(folderDelegate.modelData.fileCount, "file") + " · synced "
                                          + Format.ago(folderDelegate.modelData.lastSync)
                                    opacity: 0.7
                                }
                            }
                        }
                        // No folders yet
                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: section.folders.length === 0
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: "No folders in " + section.category.name + " yet."
                            }
                            Button {
                                Layout.alignment: Qt.AlignHCenter
                                text: "Add a folder…"
                                onClicked: section.openSettingsRequested(0)
                            }
                        }
                    }
                }
            }

            // Files of the selected folder, paged from the database as you scroll
            Pane {
                SplitView.fillWidth: true
                padding: 0
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4
                    Label {
                        Layout.fillWidth: true
                        text: section.selectedFolder ? "FILES IN “" + section.selectedFolder.name + "”" : "FILES"
                        font.bold: true
                        opacity: 0.6
                        leftPadding: 8
                        topPadding: 4
                        elide: Text.ElideRight
                    }
                    ListView {
                        id: fileList
                        objectName: "fileList"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: filesModel
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            id: fileDelegate
                            required property string relativePath
                            required property double size
                            required property string kind
                            required property bool unsettled
                            width: ListView.view.width
                            contentItem: RowLayout {
                                spacing: 12
                                Label {
                                    Layout.fillWidth: true
                                    text: fileDelegate.relativePath
                                    elide: Text.ElideMiddle
                                }
                                Label {
                                    Layout.preferredWidth: 80
                                    horizontalAlignment: Text.AlignRight
                                    text: Format.bytes(fileDelegate.size)
                                    opacity: 0.7
                                }
                                Label {
                                    Layout.preferredWidth: 64
                                    text: fileDelegate.kind
                                    opacity: 0.7
                                }
                                Label {
                                    Layout.preferredWidth: 64
                                    text: fileDelegate.unsettled ? "settling" : ""
                                    opacity: 0.7
                                    ToolTip.visible: fileDelegate.unsettled && fileDelegate.hovered
                                    ToolTip.text: "Probably still being copied: it's checked again on the next sync"
                                }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: fileList.count === 0
                            opacity: 0.7
                            text: !section.selectedFolder ? ""
                                  : section.selectedFolder.status === "unknown" ? "Press Sync to read this folder."
                                  : "No media files in this folder."
                        }
                    }
                }
            }
        }

        // ---- Last sync summary
        RowLayout {
            Layout.fillWidth: true
            visible: section.report !== undefined
            spacing: 12
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: {
                    const r = section.report
                    if (!r)
                        return ""
                    let text = "Last sync (" + Format.ago(r.finishedAt) + "): "
                    if (r.cancelled)
                        return text + "Cancelled — nothing was removed"
                    text += "+" + Format.count(r.added) + " added · " + Format.count(r.modified) + " modified · "
                            + Format.count(r.removed) + " removed · " + Format.plural(r.errors, "error")
                    const offline = r.folders.filter(f => f.outcome === "offline").length
                    if (offline > 0)
                        text += " · " + offline + " offline"
                    const failed = r.folders.filter(f => f.outcome === "failed").length
                    if (failed > 0)
                        text += " · " + failed + " failed"
                    return text
                }
            }
            Button {
                text: "Details"
                onClicked: {
                    detailsDialog.report = section.report
                    detailsDialog.open()
                }
            }
        }

        // ---- Held removals (the mass-removal guard)
        Repeater {
            model: section.folders.filter(f => f.heldRemovals > 0 && !section.keptFolderIds.includes(f.id))
            Frame {
                id: banner
                required property var modelData
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: "⚠ " + banner.modelData.name + ": " + Format.plural(banner.modelData.heldRemovals, "file")
                              + " are missing (more than " + Library.settings.massRemovalThresholdPercent
                              + "%). Removals are on hold."
                    }
                    Button {
                        text: "Show folder"
                        onClicked: section.selectedFolderId = banner.modelData.id
                    }
                    Button {
                        text: "Remove them"
                        onClicked: {
                            Library.applyHeldRemovals(banner.modelData.id)
                            filesModel.reload()
                        }
                    }
                    Button {
                        text: "Keep for now"
                        onClicked: section.keptFolderIds = section.keptFolderIds.concat([banner.modelData.id])
                    }
                }
            }
        }
    }

    SyncDetailsDialog {
        id: detailsDialog
        anchors.centerIn: parent
    }
}
