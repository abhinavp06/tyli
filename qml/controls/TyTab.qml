import QtQuick
import QtQuick.Templates as T
import Tyli

// A text tab: dim, green with a 2 px underline when selected. The parent decides
// which tab is selected (bind `checked`).
T.TabButton {
    id: control

    // The hand over anything clickable (Templates controls keep the arrow by themselves).
    HoverHandler { cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }

    implicitWidth: label.implicitWidth + Theme.px(28)
    implicitHeight: Theme.controlHeight
    hoverEnabled: true

    contentItem: Text {
        id: label
        text: control.text
        font: control.font
        color: control.checked ? Theme.accent : control.hovered ? Theme.text : Theme.textDim
        verticalAlignment: Text.AlignVCenter
    }

    background: Item {
        Rectangle {
            visible: control.checked
            anchors.bottom: parent.bottom
            width: label.implicitWidth
            height: Theme.lineWidth
            color: Theme.accent
        }
    }
}
