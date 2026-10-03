import QtQuick
import QtQml
import Tyli

// Which copy plays, when an album, movie, or show exists in several qualities.
// options: [{label, detail}]; the first is the best (selected by default).
// Fixed sizes, not scaled with Theme: the user likes it as it is (2026-10-03).
Rectangle {
    id: picker

    property var options: []
    property int currentIndex: 0
    signal picked(int index)

    implicitWidth: row.implicitWidth + 20
    implicitHeight: 40
    color: area.containsMouse ? Theme.hover : "transparent"
    border.width: 1
    border.color: area.containsMouse ? Theme.textFaint : Theme.hairline
    visible: options.length > 0

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10
        Text { anchors.verticalCenter: parent.verticalCenter; text: "quality"; color: Theme.textDim; font.pixelSize: 11 }
        Text { anchors.verticalCenter: parent.verticalCenter; text: picker.options[picker.currentIndex] ? picker.options[picker.currentIndex].label : ""; color: Theme.text; font.pixelSize: 13 }
        Icon { anchors.verticalCenter: parent.verticalCenter; name: "chevronDown"; size: 16; visible: picker.options.length > 1 }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: picker.options.length > 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (picker.options.length > 1) menu.popup(picker, 0, picker.height + 4)
    }
    TyMenu { id: menu }
    // Menus take dynamic items through an Instantiator, not a Repeater.
    Instantiator {
        model: picker.options
        delegate: TyMenuItem {
            required property var modelData
            required property int index
            text: (index === picker.currentIndex ? "✓ " : "  ") + modelData.label + (modelData.detail ? "   " + modelData.detail : "")
            onTriggered: picker.picked(index)
        }
        onObjectAdded: (index, object) => menu.insertItem(index, object)
        onObjectRemoved: (index, object) => menu.removeItem(object)
    }
}
