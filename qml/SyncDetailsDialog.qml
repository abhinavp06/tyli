import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// Each folder's part of a sync report, with what couldn't be read.
Dialog {
    id: dialog

    property var report

    title: "Last sync"
    modal: true
    standardButtons: Dialog.Close
    width: Math.min(parent ? parent.width - 80 : 720, 720)
    height: Math.min(parent ? parent.height - 80 : 560, 560)

    readonly property var outcomeText: ({
        synced: "Synced",
        offline: "Offline: nothing was removed",
        cancelled: "Cancelled: nothing was removed",
        failed: "Failed"
    })

    ScrollView {
        anchors.fill: parent
        clip: true
        ColumnLayout {
            width: dialog.availableWidth
            spacing: 16
            Repeater {
                model: dialog.report ? dialog.report.folders : []
                ColumnLayout {
                    id: folderReport
                    required property var modelData
                    readonly property var errors: dialog.visible ? Library.scanErrors(modelData.folderId) : []
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: folderReport.modelData.folderName + " · " + dialog.outcomeText[folderReport.modelData.outcome]
                        font.bold: true
                    }
                    Label {
                        visible: folderReport.modelData.message !== ""
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: folderReport.modelData.message
                    }
                    Label {
                        visible: folderReport.modelData.outcome === "synced"
                        text: "+" + Format.count(folderReport.modelData.added) + " added · "
                              + Format.count(folderReport.modelData.modified) + " modified · "
                              + Format.count(folderReport.modelData.removed) + " removed · "
                              + Format.count(folderReport.modelData.unchanged) + " unchanged · "
                              + Format.count(folderReport.modelData.unsettled) + " still being copied"
                    }
                    Label {
                        visible: folderReport.modelData.relocated
                        text: "Found at a new location: the folder path was updated."
                    }
                    Label {
                        visible: folderReport.modelData.removalsHeld
                        text: Format.plural(folderReport.modelData.heldRemovals, "removal") + " on hold (mass-removal guard)."
                    }
                    Label {
                        visible: folderReport.errors.length > 0
                        text: "Couldn't read " + Format.plural(folderReport.errors.length, "item") + ":"
                    }
                    Repeater {
                        model: folderReport.errors
                        Label {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            wrapMode: Text.WrapAnywhere
                            opacity: 0.8
                            text: (modelData.path === "" ? "(the whole folder)" : modelData.path
                                                           + (modelData.isDirectory ? "/" : ""))
                                  + " — " + modelData.message
                        }
                    }
                }
            }
        }
    }
}
