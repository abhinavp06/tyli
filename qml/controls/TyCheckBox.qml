import QtQuick
import QtQuick.Templates as T
import Tyli

// A check box drawn as text: [x] in green when on, [ ] when off.
T.CheckBox {
    id: control

    // The hand over anything clickable (Templates controls keep the arrow by themselves).
    HoverHandler { cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }

    implicitHeight: Theme.controlHeight
    // With text, the label's own left padding already makes room for the [x].
    implicitWidth: text !== "" ? label.implicitWidth : indicator.implicitWidth
    spacing: Theme.px(8)
    hoverEnabled: true

    indicator: Text {
        y: (control.height - height) / 2
        text: control.checked ? "[x]" : "[ ]"
        font: control.font
        color: !control.enabled ? Theme.textFaint
             : control.checked ? Theme.accent
             : control.hovered ? Theme.text
             : Theme.textDim
    }

    contentItem: Text {
        id: label
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textFaint
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
