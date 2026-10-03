import QtQuick
import QtQuick.Controls
import Tyli
import "Format.js" as Format

// The categories, each with its icon and name (no file counts: the user, 2026-10-03).
// Collapses to icons only.
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
        anchors.topMargin: Theme.px(14)
        anchors.rightMargin: 1
        clip: true
        model: Library.categories
        delegate: Rectangle {
            id: item
            required property var modelData
            objectName: "sidebarItem-" + modelData.name.toLowerCase()
            readonly property bool current: modelData.id === sidebar.currentCategoryId
            width: ListView.view.width
            height: Theme.px(34)
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
                id: row
                anchors.verticalCenter: parent.verticalCenter
                x: Math.round((Theme.sidebarCollapsedWidth - Theme.iconSize) / 2)  // the icon stays put when collapsed
                spacing: Theme.px(12)
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: Format.categoryIcon(item.modelData)
                    color: item.current ? Theme.accent : Theme.textDim
                }
                Text {
                    visible: !sidebar.collapsed
                    anchors.verticalCenter: parent.verticalCenter
                    width: sidebar.width - row.x - Theme.iconSize - row.spacing - Theme.px(14)
                    text: item.modelData.name.toLowerCase()
                    color: item.current ? Theme.accent : Theme.text
                    elide: Text.ElideRight
                }
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
