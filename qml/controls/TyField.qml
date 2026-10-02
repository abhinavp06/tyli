import QtQuick
import QtQuick.Templates as T
import Tyli

// A text field: black, a hairline border that turns green with focus (red when
// `invalid`), a green caret, green selected text.
T.TextField {
    id: control

    property bool invalid: false

    implicitWidth: 200
    implicitHeight: Theme.controlHeight
    leftPadding: 10
    rightPadding: 10
    verticalAlignment: TextInput.AlignVCenter
    hoverEnabled: true

    color: enabled ? Theme.text : Theme.textFaint
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.selection
    selectedTextColor: Theme.accent

    cursorDelegate: Rectangle {
        width: 1
        color: Theme.accent
        visible: control.activeFocus && blink.on
        Timer {
            id: blink
            property bool on: true
            interval: 550
            repeat: true
            running: control.activeFocus
            onRunningChanged: on = true
            onTriggered: on = !on
        }
    }

    // The template doesn't draw the placeholder itself.
    Text {
        x: control.leftPadding
        width: control.width - control.leftPadding - control.rightPadding
        height: control.height
        verticalAlignment: Text.AlignVCenter
        text: control.placeholderText
        color: control.placeholderTextColor
        font: control.font
        elide: Text.ElideRight
        visible: control.length === 0 && control.preeditText === ""
    }

    background: Rectangle {
        color: Theme.background
        border.width: 1
        border.color: control.invalid ? Theme.error
                    : control.activeFocus ? Theme.accent
                    : control.hovered ? Theme.textFaint
                    : Theme.hairline
    }
}
