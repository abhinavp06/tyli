import QtQuick
import QtQuick.Templates as T
import Tyli

// A thin scroll bar: a 3 px faint thumb, shown while scrolling or hovering.
T.ScrollBar {
    id: control

    implicitWidth: 9
    implicitHeight: 9
    padding: 3
    minimumSize: 0.05
    hoverEnabled: true
    policy: T.ScrollBar.AsNeeded

    contentItem: Rectangle {
        implicitWidth: 3
        implicitHeight: 3
        color: control.pressed || control.hovered ? Theme.textDim : Theme.textFaint
        opacity: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1) ? 1 : 0
    }
}
