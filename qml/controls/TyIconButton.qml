import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Tyli

// A button that is just an icon: dim at rest, green on hover (red for destructive
// actions). `tip` is both the tooltip and the name screen readers announce.
T.Button {
    id: control

    property string iconName
    property string tip
    property bool primary: false  // green even at rest: the main action here
    property bool danger: false   // red on hover

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.name: tip
    ToolTip.visible: hovered && tip !== ""
    ToolTip.text: tip
    ToolTip.delay: 500

    background: Rectangle {
        color: control.hovered && control.enabled ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: Theme.accent
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            color: !control.enabled ? Theme.textFaint
                 : control.hovered || control.down ? (control.danger ? Theme.error : Theme.accent)
                 : control.primary || control.checked ? Theme.accent
                 : Theme.textDim
        }
    }
}
