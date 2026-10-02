import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// A category's media. For now: its files across all its folders, paged from the
// database as you scroll. Albums, shows, and books arrive with metadata.
Item {
    id: view

    property var category
    signal addFolderRequested()

    readonly property var folders: category ? Library.folders.filter(f => f.categoryId === category.id) : []
    readonly property int fileCount: folders.reduce((sum, f) => sum + f.fileCount, 0)
    readonly property bool anySynced: folders.some(f => f.status !== "unknown")

    CategoryFilesModel {
        id: files
        categoryId: view.category ? view.category.id : -1
    }

    // Column widths shared by the header and the rows.
    readonly property int folderWidth: 170
    readonly property int sizeWidth: 84
    readonly property int kindWidth: 64
    readonly property int flagWidth: 72

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: 20
        anchors.bottomMargin: 12
        spacing: 14

        RowLayout {
            spacing: 16
            Text {
                text: view.category ? view.category.name.toLowerCase() : ""
                color: Theme.text
                font.pixelSize: Theme.fontSizeTitle
                font.weight: Theme.weightBold
            }
            Text {
                text: Format.plural(view.fileCount, "file") + " · " + Format.plural(view.folders.length, "folder")
                color: Theme.textDim
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 16
            visible: files.count > 0
            Text { Layout.fillWidth: true; text: "file"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { Layout.preferredWidth: view.folderWidth; text: "folder"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { Layout.preferredWidth: view.sizeWidth; horizontalAlignment: Text.AlignRight; text: "size"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { Layout.preferredWidth: view.kindWidth; text: "kind"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Item { Layout.preferredWidth: view.flagWidth }
        }

        ListView {
            id: list
            objectName: "fileList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: files
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: TyScrollBar {}

            delegate: Rectangle {
                id: row
                required property string relativePath
                required property string folderName
                required property bool folderOnline
                required property double size
                required property string kind
                required property bool unsettled
                width: ListView.view.width
                height: 28
                color: area.containsMouse ? Theme.hover : "transparent"
                // An offline folder's files stay listed, dimmed (nothing touches the drive).
                readonly property color main: folderOnline ? Theme.text : Theme.textFaint
                readonly property color side: folderOnline ? Theme.textDim : Theme.textFaint

                MouseArea { id: area; anchors.fill: parent; hoverEnabled: true }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 16
                    Text { Layout.fillWidth: true; text: row.relativePath; color: row.main; elide: Text.ElideMiddle }
                    Text { Layout.preferredWidth: view.folderWidth; text: row.folderName; color: row.side; elide: Text.ElideRight }
                    Text { Layout.preferredWidth: view.sizeWidth; horizontalAlignment: Text.AlignRight; text: Format.bytes(row.size); color: row.side }
                    Text { Layout.preferredWidth: view.kindWidth; text: row.kind; color: row.side }
                    Text { Layout.preferredWidth: view.flagWidth; text: row.unsettled ? "settling" : ""; color: Theme.warning }
                }
            }

            // Empty states
            Column {
                anchors.centerIn: parent
                spacing: 8
                visible: list.count === 0
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Theme.textDim
                    text: !view.category ? "no categories yet"
                        : view.folders.length === 0 ? "no folders in " + view.category.name.toLowerCase() + " yet"
                        : !view.anySynced ? "not synced yet"
                        : "no media files in these folders"
                }
                TyIconButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: view.category !== undefined && (view.folders.length === 0 || !view.anySynced)
                    iconName: view.folders.length === 0 ? "plus" : "sync"
                    primary: true
                    tip: view.folders.length === 0 ? "add a folder (opens settings)" : "sync " + (view.category ? view.category.name.toLowerCase() : "")
                    onClicked: view.folders.length === 0 ? view.addFolderRequested() : Library.requestSync(view.category.id)
                }
            }
        }
    }
}
