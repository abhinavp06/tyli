import QtQuick
import QtQuick.Templates as T
import Tyli

// A thin scroll bar: a 3 px faint thumb, shown while scrolling or hovering.
T.ScrollBar {
    id: control

    implicitWidth: Theme.px(9)
    implicitHeight: Theme.px(9)
    padding: Theme.px(3)
    minimumSize: 0.05
    hoverEnabled: true
    policy: T.ScrollBar.AsNeeded

    contentItem: Rectangle {
        implicitWidth: Theme.px(3)
        implicitHeight: Theme.px(3)
        color: control.pressed || control.hovered ? Theme.textDim : Theme.textFaint
        opacity: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1) ? 1 : 0
    }
}
