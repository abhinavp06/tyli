import QtQuick
import QtQuick.Controls
import Tyli
import "Format.js" as Format

// The categories, each with its icon, name, and file count. Collapses to icons only.
Rectangle {
    id: sidebar

    property bool collapsed: false
    property int currentCategoryId: -1
    signal categoryClicked(int id)

    implicitWidth: collapsed ? Theme.sidebarCollapsedWidth : Theme.sidebarWidth
    color: Theme.background

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.hairline
    }

    ListView {
        id: list
        objectName: "categoryList"
        anchors.fill: parent
        anchors.topMargin: 14
        anchors.rightMargin: 1
        clip: true
        model: Library.categories
        delegate: Rectangle {
            id: item
            required property var modelData
            readonly property bool current: modelData.id === sidebar.currentCategoryId
            readonly property int files: Library.folders.filter(f => f.categoryId === modelData.id)
                                                       .reduce((sum, f) => sum + f.fileCount, 0)
            width: ListView.view.width
            height: 34
            color: area.containsMouse ? Theme.hover : "transparent"

            Accessible.role: Accessible.Button
            Accessible.name: modelData.name

            // The selected category's green marker line.
            Rectangle {
                visible: item.current
                width: Theme.lineWidth
                height: parent.height
                color: Theme.accent
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: 18
                spacing: 12
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: Format.categoryIcon(item.modelData)
                    color: item.current ? Theme.accent : Theme.textDim
                }
                Text {
                    visible: !sidebar.collapsed
                    anchors.verticalCenter: parent.verticalCenter
                    width: sidebar.width - 18 - Theme.iconSize - 12 - 64
                    text: item.modelData.name.toLowerCase()
                    color: item.current ? Theme.accent : Theme.text
                    elide: Text.ElideRight
                }
            }
            Text {
                visible: !sidebar.collapsed
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 14
                text: Format.count(item.files)
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sidebar.categoryClicked(item.modelData.id)
                ToolTip.visible: sidebar.collapsed && containsMouse
                ToolTip.text: item.modelData.name.toLowerCase()
                ToolTip.delay: 300
            }
        }
    }
}
