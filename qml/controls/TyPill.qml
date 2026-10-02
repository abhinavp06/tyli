import QtQuick
import QtQuick.Templates as T
import Tyli

// A rounded filter pill (categories in Settings → Library): optional icon, text, and a
// dim count. The parent decides which pill is selected (bind `checked`).
T.Button {
    id: control

    property string iconName
    property string count
    property string tip

    implicitHeight: 28
    implicitWidth: row.implicitWidth + (text === "" ? 16 : 28)
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.name: tip !== "" ? tip : text

    readonly property color tint: control.checked ? Theme.accent : control.hovered && control.text === "" ? Theme.accent : Theme.text

    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            Icon {
                visible: control.iconName !== ""
                anchors.verticalCenter: parent.verticalCenter
                name: control.iconName
                color: control.text === "" ? control.tint : control.checked ? Theme.accent : Theme.textDim
            }
            Text {
                visible: control.text !== ""
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.tint
            }
            Text {
                visible: control.count !== ""
                anchors.verticalCenter: parent.verticalCenter
                text: control.count
                font: control.font
                color: Theme.textDim
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: "transparent"
        border.width: 1
        border.color: control.checked ? Theme.accent : control.hovered ? Theme.textFaint : Theme.hairline
    }
}
