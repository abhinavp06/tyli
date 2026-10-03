import QtQuick
import Tyli

// A name to type: a field with a green ✓ (keep) and a × (cancel) inside it, like the search
// field's × (the user, 2026-10-03). Enter and Esc do the same. Used to name and rename playlists.
TyField {
    id: field

    signal confirmed(string name)
    signal cancelled()

    function keep() {
        if (text.trim() !== "")
            confirmed(text.trim())
    }

    rightPadding: okButton.width + noButton.width + Theme.px(10)
    Keys.onReturnPressed: keep()
    Keys.onEnterPressed: keep()
    Keys.onEscapePressed: cancelled()

    // Inset like the search field's ×, so a hover never covers the outline.
    TyIconButton {
        id: noButton
        objectName: field.objectName + "Cancel"
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height - 6
        height: parent.height - 6
        iconName: "close"
        tip: "cancel (Esc)"
        onClicked: field.cancelled()
    }
    TyIconButton {
        id: okButton
        objectName: field.objectName + "Ok"
        anchors.right: noButton.left
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height - 6
        height: parent.height - 6
        iconName: "check"
        primary: true
        enabled: field.text.trim() !== ""
        tip: "keep (Enter)"
        onClicked: field.keep()
    }
}
