import QtQuick
import QtQuick.Templates as T
import Tyli

T.MenuItem {
    id: control

    implicitWidth: Math.max(Theme.px(160), label.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.px(12)
    rightPadding: Theme.px(12)
    hoverEnabled: true

    contentItem: Text {
        id: label
        text: control.text
        font: control.font
        color: !control.enabled ? Theme.textFaint : control.highlighted || control.hovered ? Theme.accent : Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: control.highlighted || control.hovered ? Theme.hover : "transparent"
    }
}
